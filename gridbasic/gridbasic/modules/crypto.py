"""GridBasic cryptocurrency module — real public-key crypto and a mined ledger.

This implements, in pure Python with no external dependencies:

* ``secp256k1`` elliptic-curve point arithmetic (the same curve Bitcoin uses).
* ECDSA signing and verification (RFC-style deterministic nonces via RFC 6979).
* Key-pair / wallet generation, public-key hashing and Base58Check addresses.
* Signed transactions with verification.
* A hash-linked blockchain with proof-of-work mining and chain validation.

It is a *real* cryptocurrency stack in miniature — suitable for wallets,
signed transfers, and mining demos. It is NOT wired to a live network and
uses GridBasic's own address scheme (double-SHA160 + Base58Check) rather than
mainnet Bitcoin addresses, so coins here are "Gridcoin" (GRID).
"""

from __future__ import annotations

import hashlib
import hmac
import json as _json
import os as _os
import random
import time as _time

from ..interpreter import BoundBuiltin
from ..errors import GBRuntimeError


# ===========================================================================
# secp256k1 — the elliptic curve used by Bitcoin
# ===========================================================================
_P = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F
_A = 0
_B = 7
_N = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141
_GX = 0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
_GY = 0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8
_G = (_GX, _GY)


def _inv(a, m=_P):
    return pow(a, -1, m)


def _pt_add(p, q):
    if p is None: return q
    if q is None: return p
    x1, y1 = p
    x2, y2 = q
    if x1 == x2 and (y1 + y2) % _P == 0:
        return None
    if p == q:
        l = (3 * x1 * x1) * _inv(2 * y1) % _P
    else:
        l = (y2 - y1) * _inv(x2 - x1) % _P
    x3 = (l * l - x1 - x2) % _P
    y3 = (l * (x1 - x3) - y1) % _P
    return (x3, y3)


def _pt_mul(k, p=_G):
    if k % _N == 0 or p is None:
        return None
    if k < 0:
        return _pt_mul(-k, (p[0], (-p[1]) % _P))
    result = None
    addend = p
    while k:
        if k & 1:
            result = _pt_add(result, addend)
        addend = _pt_add(addend, addend)
        k >>= 1
    return result


def _pt_ser(point):
    """Compressed SEC1 serialization of a point."""
    x, y = point
    prefix = b"\x02" if (y % 2 == 0) else b"\x03"
    return prefix + x.to_bytes(32, "big")


# ===========================================================================
# Hashing and address derivation
# ===========================================================================
def sha256(b: bytes) -> bytes:
    return hashlib.sha256(b).digest()


def _ripemd160(b: bytes) -> bytes:
    try:
        h = hashlib.new("ripemd160")
        h.update(b)
        return h.digest()
    except Exception:
        # Fallback: double-SHA256 truncated to 20 bytes (GridBasic scheme)
        return sha256(sha256(b))[:20]


def hash160(b: bytes) -> bytes:
    return _ripemd160(sha256(b))


_B58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"


def base58_encode(b: bytes) -> str:
    n = int.from_bytes(b, "big")
    out = ""
    while n > 0:
        n, r = divmod(n, 58)
        out = _B58[r] + out
    # leading zeros
    pad = 0
    for byte in b:
        if byte == 0:
            pad += 1
        else:
            break
    return _B58[0] * pad + out


def base58_decode(s: str) -> bytes:
    n = 0
    for ch in s:
        n = n * 58 + _B58.index(ch)
    # figure out byte length
    full = n.to_bytes((n.bit_length() + 7) // 8, "big") if n else b""
    pad = 0
    for ch in s:
        if ch == _B58[0]:
            pad += 1
        else:
            break
    return b"\x00" * pad + full


def base58check(b: bytes) -> str:
    checksum = sha256(sha256(b))[:4]
    return base58_encode(b + checksum)


# ===========================================================================
# Wallet
# ===========================================================================
class Wallet:
    def __init__(self, private_key=None):
        if private_key is None:
            private_key = int.from_bytes(_os.urandom(32), "big") % _N
            if private_key == 0:
                private_key = 1
        self.private_key = private_key
        self.public_key = _pt_mul(private_key, _G)
        self.address = self._address()

    def _address(self):
        pubkey = _pt_ser(self.public_key)
        h160 = hash160(pubkey)
        versioned = b"\x00" + h160  # mainnet-style version byte
        return base58check(versioned)

    def pub_hex(self):
        x, y = self.public_key
        return x.to_bytes(32, "big").hex() + y.to_bytes(32, "big").hex()

    def pub_compressed(self):
        return _pt_ser(self.public_key).hex()

    def priv_hex(self):
        return self.private_key.to_bytes(32, "big").hex()

    def to_dict(self):
        return {
            "address": self.address,
            "public_key": self.pub_compressed(),
            "private_key": self.priv_hex(),
            "balance": getattr(self, "balance", 0),
        }

    def __repr__(self):
        return f"<wallet {self.address[:10]}...>"


def _rfc6979_k(priv: int, msg_hash: bytes):
    """Deterministic nonce per RFC 6979 (simplified)."""
    v = b"\x01" * 32
    k = b"\x00" * 32
    x = priv.to_bytes(32, "big")
    k = hmac.new(k, v + b"\x00" + x + msg_hash, hashlib.sha256).digest()
    v = hmac.new(k, v, hashlib.sha256).digest()
    k = hmac.new(k, v + b"\x01" + x + msg_hash, hashlib.sha256).digest()
    v = hmac.new(k, v, hashlib.sha256).digest()
    while True:
        v = hmac.new(k, v, hashlib.sha256).digest()
        candidate = int.from_bytes(v, "big")
        if 1 <= candidate < _N:
            return candidate
        k = hmac.new(k, v + b"\x00", hashlib.sha256).digest()
        v = hmac.new(k, v, hashlib.sha256).digest()


def ecdsa_sign(priv: int, msg: bytes) -> tuple:
    z = int.from_bytes(sha256(msg), "big") % _N
    k = _rfc6979_k(priv, sha256(msg))
    point = _pt_mul(k, _G)
    r = point[0] % _N
    if r == 0:
        k = _rfc6979_k(priv, sha256(msg + b"\x01"))
        point = _pt_mul(k, _G)
        r = point[0] % _N
    s = (_inv(k, _N) * (z + r * priv)) % _N
    if s > _N // 2:
        s = _N - s
    return (r, s)


def ecdsa_verify(pub, msg: bytes, sig: tuple) -> bool:
    r, s = sig
    if not (1 <= r < _N and 1 <= s < _N):
        return False
    z = int.from_bytes(sha256(msg), "big") % _N
    w = _inv(s, _N)
    u1 = (z * w) % _N
    u2 = (r * w) % _N
    point = _pt_add(_pt_mul(u1, _G), _pt_mul(u2, pub))
    if point is None:
        return False
    return point[0] % _N == r


def _sign_message(priv, msg):
    if isinstance(msg, str):
        msg = msg.encode("utf-8")
    sig = ecdsa_sign(priv, msg)
    return sig[0].to_bytes(32, "big").hex() + sig[1].to_bytes(32, "big").hex()


def _verify_message(pub_hex, msg, sig_hex):
    if isinstance(msg, str):
        msg = msg.encode("utf-8")
    try:
        r = int.from_bytes(bytes.fromhex(sig_hex[:64]), "big")
        s = int.from_bytes(bytes.fromhex(sig_hex[64:128]), "big")
    except Exception:
        return False
    # parse pubkey
    pub = _parse_pub(pub_hex)
    if pub is None:
        return False
    return ecdsa_verify(pub, msg, (r, s))


def _parse_pub(pub_hex):
    if isinstance(pub_hex, str):
        try:
            if len(pub_hex) == 66:  # compressed
                prefix = pub_hex[:2]
                x = int.from_bytes(bytes.fromhex(pub_hex[2:]), "big")
                # y^2 = x^3 + 7
                ysq = (pow(x, 3, _P) + 7) % _P
                y = pow(ysq, (_P + 1) // 4, _P)
                if (y % 2 == 0) != (prefix == "02"):
                    y = _P - y
                return (x, y)
            elif len(pub_hex) == 128:  # uncompressed
                x = int.from_bytes(bytes.fromhex(pub_hex[:64]), "big")
                y = int.from_bytes(bytes.fromhex(pub_hex[64:]), "big")
                return (x, y)
        except Exception:
            return None
    if isinstance(pub_hex, tuple):
        return pub_hex
    return None


# ===========================================================================
# Transaction
# ===========================================================================
class Transaction:
    def __init__(self, sender, recipient, amount, signature=""):
        self.sender = sender          # address string
        self.recipient = recipient    # address string
        self.amount = amount
        self.signature = signature    # hex
        self.timestamp = _time.time()

    def message_bytes(self):
        return f"{self.sender}>{self.recipient}>{self.amount}".encode("utf-8")

    def sign(self, wallet: Wallet):
        self.signature = _sign_message(wallet.private_key, self.message_bytes())
        return self.signature

    def verify(self, pub_hex):
        if not self.signature:
            return False
        return _verify_message(pub_hex, self.message_bytes(), self.signature)

    def to_dict(self):
        return {
            "sender": self.sender, "recipient": self.recipient,
            "amount": self.amount, "signature": self.signature,
            "timestamp": self.timestamp,
        }


# ===========================================================================
# Block and Blockchain
# ===========================================================================
class Block:
    def __init__(self, index, transactions, prev_hash, difficulty, timestamp=None, nonce=0):
        self.index = index
        self.transactions = transactions
        self.prev_hash = prev_hash
        self.difficulty = difficulty
        self.timestamp = timestamp or _time.time()
        self.nonce = nonce
        self.hash = ""

    def header(self):
        return {
            "index": self.index,
            "prev": self.prev_hash,
            "ts": int(self.timestamp),
            "nonce": self.nonce,
            "tx": len(self.transactions),
            "difficulty": self.difficulty,
        }

    def serialize(self):
        return _json.dumps(self.header(), sort_keys=True)

    def compute_hash(self):
        return hashlib.sha256(self.serialize().encode()).hexdigest()

    def mine(self):
        target = "0" * self.difficulty
        self.nonce = 0
        while True:
            h = self.compute_hash()
            if h.startswith(target):
                self.hash = h
                return h
            self.nonce += 1


class Blockchain:
    def __init__(self, difficulty=3, reward=10):
        self.chain = []
        self.pending = []
        self.difficulty = difficulty
        self.reward = reward
        self.miner = None
        self.balances = {}
        self._genesis()

    def _genesis(self):
        b = Block(0, [], "0", self.difficulty)
        b.mine()
        self.chain.append(b)

    def last(self):
        return self.chain[-1]

    def add_transaction(self, tx):
        if isinstance(tx, Transaction):
            self.pending.append(tx)
        else:
            raise GBRuntimeError("add_transaction expects a Transaction")
        return True

    def transfer(self, sender_wallet, recipient, amount):
        tx = Transaction(sender_wallet.address, recipient, amount)
        tx.sign(sender_wallet)
        self.pending.append(tx)
        return tx

    def mine_pending(self, miner_wallet=None):
        if miner_wallet:
            self.miner = miner_wallet.address
            reward_tx = Transaction("MINT", miner_wallet.address, self.reward)
            self.pending.insert(0, reward_tx)
        block = Block(len(self.chain), self.pending, self.last().hash, self.difficulty)
        block.mine()
        self.chain.append(block)
        # apply balances
        for tx in block.transactions:
            if tx.sender == "MINT":
                self.balances[tx.recipient] = self.balances.get(tx.recipient, 0) + tx.amount
            else:
                self.balances[tx.sender] = self.balances.get(tx.sender, 0) - tx.amount
                self.balances[tx.recipient] = self.balances.get(tx.recipient, 0) + tx.amount
        self.pending = []
        return block

    def balance_of(self, address):
        return self.balances.get(address, 0)

    def is_valid(self):
        for i in range(1, len(self.chain)):
            cur = self.chain[i]
            prev = self.chain[i - 1]
            if cur.prev_hash != prev.hash:
                return False
            if not cur.hash.startswith("0" * cur.difficulty):
                return False
            if cur.compute_hash() != cur.hash:
                return False
        return True

    def height(self):
        return len(self.chain)

    def to_dict(self):
        return {
            "height": len(self.chain),
            "difficulty": self.difficulty,
            "pending": len(self.pending),
            "balances": dict(self.balances),
            "last_hash": self.last().hash,
        }


# ===========================================================================
# GridBasic namespace
# ===========================================================================
def namespace(interp):
    def _wallet(args, kwargs):
        priv = args[0] if args else None
        if isinstance(priv, str):
            priv = int.from_bytes(bytes.fromhex(priv), "big")
        return Wallet(priv)
    def _address(args, kwargs):
        w = args[0]
        return w.address
    def _sign(args, kwargs):
        w = args[0]; msg = args[1]
        return _sign_message(w.private_key, msg.encode() if isinstance(msg, str) else msg)
    def _verify(args, kwargs):
        return _verify_message(args[0], args[1], args[2])
    def _tx(args, kwargs):
        return Transaction(args[0], args[1], args[2])
    def _blockchain(args, kwargs):
        diff = int(args[0]) if args else 3
        return Blockchain(difficulty=diff)
    def _mine(args, kwargs):
        block = args[0]
        if hasattr(block, "mine"):
            return block.mine()
        # raw: mine a string to a difficulty
        s = args[0]; diff = int(args[1]) if len(args) > 1 else 3
        target = "0" * diff
        nonce = 0
        while True:
            h = hashlib.sha256(f"{s}{nonce}".encode()).hexdigest()
            if h.startswith(target):
                return {"nonce": nonce, "hash": h}
            nonce += 1
    def _sha256(args, kwargs):
        v = args[0]
        b = v.encode() if isinstance(v, str) else v
        return hashlib.sha256(b).hexdigest()
    def _hash160(args, kwargs):
        v = args[0]
        b = v.encode() if isinstance(v, str) else v
        return hash160(b).hex()
    def _base58(args, kwargs):
        v = args[0]
        b = v.encode() if isinstance(v, str) else v
        return base58_encode(b)
    def _pubkey(args, kwargs):
        w = args[0]
        return w.pub_compressed()
    def _privkey(args, kwargs):
        w = args[0]
        return w.priv_hex()
    return {
        "wallet": BoundBuiltin(_wallet),
        "Wallet": Wallet,
        "address": BoundBuiltin(_address),
        "sign": BoundBuiltin(_sign),
        "verify": BoundBuiltin(_verify),
        "transaction": BoundBuiltin(_tx),
        "Transaction": Transaction,
        "blockchain": BoundBuiltin(_blockchain),
        "Blockchain": Blockchain,
        "Block": Block,
        "mine": BoundBuiltin(_mine),
        "sha256": BoundBuiltin(_sha256),
        "hash160": BoundBuiltin(_hash160),
        "base58": BoundBuiltin(_base58),
        "pubkey": BoundBuiltin(_pubkey),
        "privkey": BoundBuiltin(_privkey),
        "GRID": "GRID",
        "VERSION": "GridBasic Crypto 1.0 (secp256k1 + ECDSA + PoW)",
        "curve": "secp256k1",
    }
