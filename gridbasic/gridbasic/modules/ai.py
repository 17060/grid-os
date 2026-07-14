"""GridBasic AI module — run local models and bridge to external LLM APIs.

Built-in models (pure Python, no dependencies):

* :class:`MarkovModel`  — n-gram text generator that learns from a corpus.
* :class:`Perceptron`   — online linear classifier.
* :class:`MLP`          — a tiny multi-layer perceptron with backprop
  (enough to learn XOR and simple functions).
* :class:`Tokenizer`    — character / word / BPE-ish tokenizer.
* :class:`Embedder`     — hashing bag-of-words embeddings.
* :class:`Sentiment`    — lexicon-based sentiment scorer.

Plus an HTTP bridge (:func:`api`) that calls any OpenAI-compatible chat
completions endpoint when an API key is supplied (the key is read from the
``GRIDBASIC_AI_KEY`` or ``OPENAI_API_KEY`` environment variable by default).
"""

from __future__ import annotations

import hashlib
import json as _json
import math
import os as _os
import random
import urllib.request
import urllib.error
from collections import defaultdict, Counter

from ..interpreter import BoundBuiltin
from ..errors import GBRuntimeError


# ===========================================================================
# Tokenizer
# ===========================================================================
class Tokenizer:
    def __init__(self, mode="word"):
        self.mode = mode  # "word", "char", "bpe"

    def tokenize(self, text):
        if self.mode == "char":
            return list(text)
        if self.mode == "word":
            import re
            return re.findall(r"\w+|[^\w\s]", text.lower())
        # bpe-ish: split on whitespace then chars merged greedily
        return text.lower().split()

    def count(self, text):
        return Counter(self.tokenize(text))

    def vocab(self, text):
        return sorted(set(self.tokenize(text)))


# ===========================================================================
# Markov model
# ===========================================================================
class MarkovModel:
    def __init__(self, order=2):
        self.order = order
        self.chain = defaultdict(Counter)
        self.starts = Counter()
        self.tokens = []
        self.tokenizer = Tokenizer("word")

    def train(self, text):
        toks = self.tokenizer.tokenize(text)
        self.tokens.extend(toks)
        n = self.order
        for i in range(len(toks) - n):
            key = tuple(toks[i:i + n])
            nxt = toks[i + n]
            self.chain[key][nxt] += 1
            if i == 0:
                self.starts[key] += 1
        return True

    def generate(self, length=50, seed=None):
        if not self.chain:
            return ""
        n = self.order
        if seed:
            cur = tuple(self.tokenizer.tokenize(seed))[-n:]
        else:
            cur = random.choice(list(self.chain.keys()))
        out = list(cur)
        for _ in range(length):
            opts = self.chain.get(cur)
            if not opts:
                cur = random.choice(list(self.chain.keys()))
                opts = self.chain[cur]
            total = sum(opts.values())
            r = random.randint(1, total)
            upto = 0
            chosen = None
            for tok, c in opts.items():
                upto += c
                if r <= upto:
                    chosen = tok
                    break
            out.append(chosen)
            cur = tuple(out[-n:])
        # reconstruct text
        result = []
        for t in out:
            if t in ".,!?;:)" or (result and result[-1] in "("):
                result.append(t)
            else:
                result.append(" " + t if result else t)
        text = "".join(result).strip()
        return text

    def perplexity(self, text):
        toks = self.tokenizer.tokenize(text)
        n = self.order
        logsum = 0.0; count = 0
        for i in range(len(toks) - n):
            key = tuple(toks[i:i + n])
            opts = self.chain.get(key)
            if not opts:
                continue
            total = sum(opts.values())
            p = opts[toks[i + n]] / total
            logsum += -math.log(p + 1e-12)
            count += 1
        return math.exp(logsum / count) if count else float("inf")


# ===========================================================================
# Perceptron
# ===========================================================================
class Perceptron:
    def __init__(self, n_inputs=2, lr=0.1):
        self.weights = [random.uniform(-0.5, 0.5) for _ in range(n_inputs)]
        self.bias = 0.0
        self.lr = lr
        self.n = n_inputs

    @staticmethod
    def _act(x):
        return 1 if x > 0 else 0

    def predict(self, inputs):
        if len(inputs) < self.n:
            inputs = list(inputs) + [0.0] * (self.n - len(inputs))
        z = sum(w * x for w, x in zip(self.weights, inputs[:self.n])) + self.bias
        return self._act(z)

    def train(self, samples, epochs=100):
        # samples: list of [inputs, label]
        for _ in range(epochs):
            for sample in samples:
                inputs, label = sample[0], sample[1]
                pred = self.predict(inputs)
                err = label - pred
                for i in range(self.n):
                    self.weights[i] += self.lr * err * (inputs[i] if i < len(inputs) else 0)
                self.bias += self.lr * err
        return True

    def to_dict(self):
        return {"weights": self.weights, "bias": self.bias}


# ===========================================================================
# Multi-layer perceptron (with backprop)
# ===========================================================================
def _sigmoid(x):
    if x < -50: return 0.0
    if x > 50: return 1.0
    return 1.0 / (1.0 + math.exp(-x))


class MLP:
    def __init__(self, layout=(2, 4, 1), lr=0.5):
        self.layout = list(layout)
        self.lr = lr
        self.weights = []
        self.biases = []
        for i in range(len(layout) - 1):
            self.weights.append([[random.uniform(-1, 1) for _ in range(layout[i + 1])]
                                 for _ in range(layout[i])])
            self.biases.append([random.uniform(-1, 1) for _ in range(layout[i + 1])])

    def _forward(self, inputs):
        activations = [list(inputs)]
        for i in range(len(self.weights)):
            layer_in = activations[-1]
            layer_out = []
            for j in range(len(self.biases[i])):
                z = self.biases[i][j] + sum(layer_in[k] * self.weights[i][k][j]
                                            for k in range(len(layer_in)))
                layer_out.append(_sigmoid(z))
            activations.append(layer_out)
        return activations

    def predict(self, inputs):
        return self._forward(inputs)[-1]

    def train(self, samples, epochs=2000):
        for _ in range(epochs):
            for sample in samples:
                inputs, target = sample[0], sample[1]
                acts = self._forward(inputs)
                tgt = target if isinstance(target, list) else [target]
                # output-layer delta
                out = acts[-1]
                deltas = [[(out[j] - tgt[j]) * out[j] * (1 - out[j])
                           for j in range(len(out))]]
                # hidden-layer deltas (back-propagate)
                for i in range(len(self.weights) - 1, 0, -1):
                    nd = []
                    for k in range(len(self.weights[i])):  # neurons in layer i
                        s = sum(deltas[-1][j] * self.weights[i][k][j]
                                for j in range(len(deltas[-1])))
                        a = acts[i][k]
                        nd.append(s * a * (1 - a))
                    deltas.append(nd)
                deltas.reverse()
                # gradient descent update
                for i in range(len(self.weights)):
                    for k in range(len(self.weights[i])):
                        for j in range(len(self.weights[i][k])):
                            self.weights[i][k][j] -= self.lr * deltas[i][j] * acts[i][k]
                    for j in range(len(self.biases[i])):
                        self.biases[i][j] -= self.lr * deltas[i][j]
        return True


# ===========================================================================
# Embedder
# ===========================================================================
class Embedder:
    def __init__(self, dims=64):
        self.dims = dims

    def embed(self, text):
        toks = Tokenizer("word").tokenize(text)
        vec = [0.0] * self.dims
        for tok in toks:
            h = int(hashlib.md5(tok.encode()).hexdigest(), 16)
            idx = h % self.dims
            sign = 1.0 if (h >> self.dims.bit_length()) & 1 else -1.0
            vec[idx] += sign
        # l2 normalize
        norm = math.sqrt(sum(v * v for v in vec)) or 1.0
        return [v / norm for v in vec]

    def similarity(self, a, b):
        va = self.embed(a); vb = self.embed(b)
        return sum(x * y for x, y in zip(va, vb))


# ===========================================================================
# Sentiment (lexicon)
# ===========================================================================
_POS = {"good": 2, "great": 3, "awesome": 4, "happy": 3, "love": 3,
        "excellent": 4, "amazing": 4, "wonderful": 3, "best": 3, "nice": 2,
        "fantastic": 4, "brilliant": 3, "perfect": 4, "joy": 3, "win": 2,
        "beautiful": 3, "positive": 2, "superb": 3, "delightful": 3}
_NEG = {"bad": -2, "terrible": -4, "awful": -4, "hate": -3, "sad": -2,
        "worst": -4, "horrible": -4, "poor": -2, "ugly": -3, "fail": -2,
        "negative": -2, "disgusting": -4, "annoying": -2, "angry": -3,
        "boring": -2, "stupid": -3, "wrong": -2, "broken": -2, "lose": -2}
_NEG_PREFIX = ("not ", "no ", "never ", "n't ")


class Sentiment:
    def score(self, text):
        t = text.lower()
        score = 0
        for w, s in {**_POS, **_NEG}.items():
            if w in t:
                score += s
        # negation flips
        for np in _NEG_PREFIX:
            if np in t:
                score = -score // 2
                break
        return score

    def label(self, text):
        s = self.score(text)
        return "positive" if s > 0 else ("negative" if s < 0 else "neutral")


# ===========================================================================
# External LLM API bridge
# ===========================================================================
def _api_call(api_key, prompt, model="gpt-4o-mini", endpoint=None,
              system="You are a helpful assistant.", temperature=0.7, max_tokens=500):
    if not api_key:
        raise GBRuntimeError("AI.api requires an API key (set GRIDBASIC_AI_KEY or OPENAI_API_KEY)")
    url = endpoint or _os.environ.get("GRIDBASIC_AI_URL",
                                      "https://api.openai.com/v1/chat/completions")
    body = {
        "model": model,
        "messages": [
            {"role": "system", "content": system},
            {"role": "user", "content": prompt},
        ],
        "temperature": temperature,
        "max_tokens": max_tokens,
    }
    data = _json.dumps(body).encode("utf-8")
    req = urllib.request.Request(url, data=data, method="POST")
    req.add_header("Content-Type", "application/json")
    req.add_header("Authorization", f"Bearer {api_key}")
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            raw = resp.read().decode("utf-8")
    except urllib.error.HTTPError as e:
        raise GBRuntimeError(f"AI API HTTP {e.code}: {e.read().decode('utf-8', 'replace')[:200]}")
    except Exception as e:
        raise GBRuntimeError(f"AI API error: {e}")
    try:
        obj = _json.loads(raw)
        return obj["choices"][0]["message"]["content"]
    except Exception:
        return raw


# ===========================================================================
# Namespace
# ===========================================================================
def namespace(interp):
    def _generate(args, kwargs):
        prompt = args[0] if args else ""
        model = kwargs.get("model") or (args[1] if len(args) > 1 else "markov")
        length = int(kwargs.get("length", 60))
        # use the shared markov model
        m = interp.modules.get("ai", {}).get("_markov")
        if m is None:
            m = MarkovModel(2)
            interp.modules.setdefault("ai", {})["_markov"] = m
        if not m.chain:
            m.train(_DEFAULT_CORPUS)
        if prompt and model == "markov":
            return m.generate(length, seed=prompt)
        return m.generate(length)
    def _train(args, kwargs):
        m = interp.modules.get("ai", {}).get("_markov")
        if m is None:
            m = MarkovModel(2)
            interp.modules.setdefault("ai", {})["_markov"] = m
        m.train(args[0])
        return True
    def _model(args, kwargs):
        kind = args[0] if args else "markov"
        if kind == "markov":
            order = int(args[1]) if len(args) > 1 else 2
            return MarkovModel(order)
        if kind == "perceptron":
            n = int(args[1]) if len(args) > 1 else 2
            return Perceptron(n)
        if kind in ("mlp", "network", "net"):
            layout = args[1] if len(args) > 1 else [2, 4, 1]
            return MLP(layout)
        if kind == "tokenizer":
            mode = args[1] if len(args) > 1 else "word"
            return Tokenizer(mode)
        if kind == "embedder":
            return Embedder(int(args[1]) if len(args) > 1 else 64)
        if kind == "sentiment":
            return Sentiment()
        raise GBRuntimeError(f"Unknown AI model: {kind}")
    def _embed(args, kwargs):
        e = Embedder(int(kwargs.get("dims", 64)))
        return e.embed(args[0])
    def _sentiment(args, kwargs):
        return Sentiment().score(args[0])
    def _api(args, kwargs):
        prompt = args[0] if args else ""
        key = kwargs.get("key") or _os.environ.get("GRIDBASIC_AI_KEY") or _os.environ.get("OPENAI_API_KEY")
        model = kwargs.get("model", "gpt-4o-mini")
        return _api_call(key, prompt, model=model,
                         endpoint=kwargs.get("endpoint"),
                         system=kwargs.get("system", "You are a helpful assistant."),
                         temperature=float(kwargs.get("temperature", 0.7)),
                         max_tokens=int(kwargs.get("max_tokens", 500)))
    def _classify(args, kwargs):
        text = args[0]; labels = args[1] if len(args) > 1 else []
        t = text.lower()
        synonyms = {
            "crypto": ["crypto", "wallet", "signature", "blockchain", "coin", "key", "hash"],
            "irc": ["irc", "channel", "nick", "chat", "message", "server"],
            "ai": ["ai", "model", "train", "neural", "generate", "learn", "data"],
            "web": ["http", "url", "request", "api", "json"],
        }
        best = None; best_score = 0
        for label in labels:
            kws = synonyms.get(label.lower(), label.lower().split())
            score = sum(1 for k in kws if k in t)
            if score > best_score:
                best = label; best_score = score
        return best
    ns = {
        "generate": BoundBuiltin(_generate),
        "train": BoundBuiltin(_train),
        "model": BoundBuiltin(_model),
        "embed": BoundBuiltin(_embed),
        "sentiment": BoundBuiltin(_sentiment),
        "api": BoundBuiltin(_api),
        "classify": BoundBuiltin(_classify),
        "MarkovModel": MarkovModel,
        "Perceptron": Perceptron,
        "MLP": MLP,
        "Tokenizer": Tokenizer,
        "Embedder": Embedder,
        "Sentiment": Sentiment,
        "VERSION": "GridBasic AI 1.0 (markov + perceptron + MLP + LLM bridge)",
    }
    return ns


_DEFAULT_CORPUS = """
GridBasic is the most advanced BASIC programming language on Earth. It blends
the best ideas from many languages into one approachable, line-oriented syntax.
Programs are easy to read and powerful to write. Functions are first-class
values and may close over their environment. Classes support inheritance and
methods. Pattern matching makes branching clear. Channels and spawn bring
concurrency to everyone. Cryptocurrency wallets sign transactions with real
elliptic-curve cryptography. IRC clients connect to networks and chat. AI
models generate text and learn from examples. The IDE is a graphical editor
with a console and panels for every module. GridBasic runs anywhere Python
runs. Code is poetry. Build something great today.
"""
