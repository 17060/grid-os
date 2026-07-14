/* GridBasic IDE — frontend logic. No dependencies. */

const KEYWORDS = new Set([
  "print","input","let","dim","if","then","else","elseif","end","endif","for","to","step",
  "next","while","wend","do","loop","repeat","until","goto","gosub","return","rem","select",
  "case","function","fn","sub","def","lambda","class","extends","new","me","self","match",
  "when","is","in","async","await","spawn","go","try","catch","finally","throw","raise",
  "import","from","export","as","const","var","type","enum","trait","impl","struct","and",
  "or","not","mod","div","xor","shl","shr","true","false","none","nil","null","break",
  "continue","pass","stop","yield","defer","chan","with","exit","option","base","data",
  "read","restore","randomize","using","on","error","resume","shared","local","global",
  "public","private","static","abstract","override","virtual","constructor","ctor","init"
]);
const BOOLS = new Set(["true","false","none","nil","null"]);

function escapeHtml(s) {
  return s.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");
}

function highlight(code) {
  // tokenize line by line to keep newlines; comments take whole rest of line
  const out = [];
  for (const line of code.split("\n")) {
    let i = 0, buf = "";
    const flush = () => { if (buf) { out.push(escapeHtml(buf)); buf = ""; } };
    while (i < line.length) {
      const c = line[i];
      // comment: ' or REM
      if (c === "'" || (line.slice(i, i+3).toLowerCase() === "rem" && (i===0 || !/\w/.test(line[i-1])) && (i+3>=line.length || !/\w/.test(line[i+3])))) {
        flush();
        out.push('<span class="tok-com">' + escapeHtml(line.slice(i)) + '</span>');
        i = line.length;
        break;
      }
      // f-string / string (double quote)
      if (c === '"') {
        flush();
        // detect f" or $"
        let j = i, isf = false;
        // we already know c == '"'; check preceding char for f/$ via lookahead from before
        // (handled below); here just consume string
        let s = '"'; j++;
        while (j < line.length && line[j] !== '"') {
          if (line[j] === "\\" && j+1 < line.length) { s += line[j] + line[j+1]; j += 2; continue; }
          s += line[j]; j++;
        }
        if (j < line.length) { s += '"'; j++; }
        out.push('<span class="tok-str">' + escapeHtml(s) + '</span>');
        i = j;
        continue;
      }
      // number
      if (/[0-9]/.test(c) || (c === "." && /[0-9]/.test(line[i+1] || ""))) {
        flush();
        let s = "";
        while (i < line.length && /[0-9a-fxXbBeE._+\-]/.test(line[i]) && !(line[i]==="+"||line[i]==="-") || (/[eE]/.test(line[i-1]) && (line[i]==="+"||line[i]==="-"))) {
          s += line[i]; i++;
        }
        out.push('<span class="tok-num">' + escapeHtml(s) + '</span>');
        continue;
      }
      // identifier / keyword
      if (/[A-Za-z_$]/.test(c)) {
        flush();
        let s = "";
        while (i < line.length && /[\w$]/.test(line[i])) { s += line[i]; i++; }
        // type sigil
        if (line[i] === "$" || line[i] === "%") { s += line[i]; i++; }
        const low = s.toLowerCase().replace(/[$%]$/, "");
        if (KEYWORDS.has(low)) out.push('<span class="tok-kw">' + escapeHtml(s) + '</span>');
        else if (BOOLS.has(low)) out.push('<span class="tok-bool">' + escapeHtml(s) + '</span>');
        else if (line[i] === "(") out.push('<span class="tok-fn">' + escapeHtml(s) + '</span>');
        else out.push('<span class="tok-ident">' + escapeHtml(s) + '</span>');
        continue;
      }
      // operators
      if (/[+\-*/%^=<>!&|?:.@,;(){}\[\]]/.test(c)) {
        flush();
        out.push('<span class="tok-op">' + escapeHtml(c) + '</span>');
        i++;
        continue;
      }
      buf += c; i++;
    }
    flush();
    out.push("\n");
  }
  return out.join("");
}

// ---- editor ------------------------------------------------------------
const editor = document.getElementById("editor");
const highlightEl = document.getElementById("highlight");
const cursorEl = document.getElementById("cursor");
let currentFile = "untitled.gb";
let dirty = false;

function syncHighlight() {
  const code = editor.value + "\n";
  highlightEl.innerHTML = highlight(code);
  highlightEl.scrollTop = editor.scrollTop;
  highlightEl.scrollLeft = editor.scrollLeft;
}
function updateCursor() {
  const pos = editor.selectionStart;
  const before = editor.value.slice(0, pos);
  const ln = before.split("\n").length;
  const col = pos - (before.lastIndexOf("\n") + 1) + 1;
  cursorEl.textContent = `Ln ${ln}, Col ${col}`;
}
editor.addEventListener("input", () => { syncHighlight(); updateCursor(); dirty = true; });
editor.addEventListener("scroll", () => {
  highlightEl.scrollTop = editor.scrollTop;
  highlightEl.scrollLeft = editor.scrollLeft;
});
editor.addEventListener("keyup", updateCursor);
editor.addEventListener("click", updateCursor);
editor.addEventListener("keydown", (e) => {
  if (e.key === "Tab") {
    e.preventDefault();
    const s = editor.selectionStart, en = editor.selectionEnd;
    editor.value = editor.value.slice(0, s) + "  " + editor.value.slice(en);
    editor.selectionStart = editor.selectionEnd = s + 2;
    syncHighlight();
  }
  if ((e.ctrlKey || e.metaKey) && e.key === "Enter") { e.preventDefault(); runProgram(); }
  if ((e.ctrlKey || e.metaKey) && e.key === "s") { e.preventDefault(); saveFile(); }
});
syncHighlight(); updateCursor();

// ---- status helpers ----------------------------------------------------
const statusEl = document.getElementById("status");
const runStateEl = document.getElementById("run-state");
function setStatus(s, cls) { statusEl.textContent = s; runStateEl.textContent = s; runStateEl.className = cls || ""; }

// ---- file tree ---------------------------------------------------------
async function loadFiles() {
  const tree = document.getElementById("filetree");
  const r = await fetch("/api/files");
  const j = await r.json();
  tree.innerHTML = "";
  for (const item of j.files) tree.appendChild(renderFileItem(item));
}
function renderFileItem(item) {
  const el = document.createElement("div");
  el.className = "item " + item.type;
  el.innerHTML = `<span class="icon">${item.type === "dir" ? "▸" : "▦"}</span><span>${item.name}</span>`;
  if (item.type === "file") {
    el.onclick = () => openFile(item.path, el);
  } else if (item.children) {
    const wrap = document.createElement("div");
    wrap.className = "children";
    for (const c of item.children) wrap.appendChild(renderFileItem(c));
    el.onclick = () => { wrap.style.display = wrap.style.display === "none" ? "block" : "none"; };
    el.after(wrap);
  }
  return el;
}
async function openFile(path, el) {
  document.querySelectorAll(".filetree .item.active").forEach(i => i.classList.remove("active"));
  if (el) el.classList.add("active");
  const r = await fetch("/api/file?path=" + encodeURIComponent(path));
  const j = await r.json();
  if (j.error) { alert(j.error); return; }
  editor.value = j.content;
  currentFile = path;
  document.getElementById("current-file").textContent = path;
  syncHighlight(); updateCursor(); dirty = false;
}

// ---- save / new --------------------------------------------------------
async function saveFile() {
  const path = currentFile || prompt("Save as:", "untitled.gb");
  if (!path) return;
  const r = await fetch("/api/save", {
    method: "POST", headers: {"Content-Type": "application/json"},
    body: JSON.stringify({path, content: editor.value})
  });
  const j = await r.json();
  if (j.ok) { setStatus("saved"); dirty = false; currentFile = path; document.getElementById("current-file").textContent = path; loadFiles(); }
  else alert(j.error);
}
document.getElementById("btn-save").onclick = saveFile;
document.getElementById("btn-new").onclick = () => {
  if (dirty && !confirm("Discard unsaved changes?")) return;
  editor.value = ""; currentFile = "untitled.gb"; document.getElementById("current-file").textContent = currentFile; syncHighlight(); dirty = false;
};
document.getElementById("btn-open").onclick = () => { openFile(prompt("Open path:", "examples/tour.gb")); };

// ---- examples ----------------------------------------------------------
async function loadExamples() {
  const sel = document.getElementById("examples");
  const r = await fetch("/api/examples");
  const j = await r.json();
  for (const ex of j.examples) {
    const opt = document.createElement("option");
    opt.value = ex.name; opt.textContent = ex.name;
    sel.appendChild(opt);
  }
  sel.onchange = () => {
    const ex = j.examples.find(e => e.name === sel.value);
    if (ex) { editor.value = ex.content; currentFile = ex.name; document.getElementById("current-file").textContent = ex.name; syncHighlight(); dirty = false; }
    sel.selectedIndex = 0;
  };
}

// ---- run / console -----------------------------------------------------
const consoleEl = document.getElementById("console");
const stdinEl = document.getElementById("stdin");
let currentRunId = null;
let pollTimer = null;
let pollSince = 0;
let runDone = false;

function appendConsole(text, cls) {
  const span = document.createElement("div");
  span.className = "line-" + (cls || "out");
  span.textContent = text;
  consoleEl.appendChild(span);
  consoleEl.scrollTop = consoleEl.scrollHeight;
}

async function runProgram() {
  if (currentRunId) { stopProgram(); }
  consoleEl.innerHTML = "";
  setStatus("running…", "running");
  const r = await fetch("/api/run", {
    method: "POST", headers: {"Content-Type": "application/json"},
    body: JSON.stringify({source: editor.value, path: currentFile})
  });
  const j = await r.json();
  if (!j.id) { setStatus("error", "error"); return; }
  currentRunId = j.id; pollSince = 0; runDone = false;
  pollOutput();
}

async function pollOutput() {
  if (!currentRunId) return;
  try {
    const r = await fetch(`/api/output?id=${currentRunId}&since=${pollSince}`);
    const j = await r.json();
    if (j.events) {
      for (const ev of j.events) handleEvent(ev);
      pollSince = j.next;
    }
  } catch (e) {}
  if (runDone) { currentRunId = null; return; }
  if (currentRunId) pollTimer = setTimeout(pollOutput, 120);
}

function handleEvent(ev) {
  if (ev.type === "output") appendConsole(ev.text, "out");
  else if (ev.type === "error") { appendConsole("Error: " + ev.message, "err"); setStatus("error", "error"); }
  else if (ev.type === "status") appendConsole(ev.message, "status");
  else if (ev.type === "end") {
    appendConsole(ev.error ? ("✗ " + ev.error) : "✓ done", ev.error ? "err" : "status");
    setStatus(ev.error ? "error" : "done", ev.error ? "error" : "");
    runDone = true;
  } else if (ev.type === "grid") handleGrid(ev.cmd);
}

async function stopProgram() {
  if (!currentRunId) return;
  await fetch("/api/stop", {method: "POST", headers: {"Content-Type":"application/json"}, body: JSON.stringify({id: currentRunId})});
  setStatus("stopping…");
}

document.getElementById("btn-run").onclick = runProgram;
document.getElementById("btn-stop").onclick = stopProgram;

stdinEl.addEventListener("keydown", (e) => {
  if (e.key === "Enter" && currentRunId) {
    const line = stdinEl.value;
    appendConsole(line, "out");
    fetch("/api/input", {method: "POST", headers: {"Content-Type":"application/json"}, body: JSON.stringify({id: currentRunId, line})});
    stdinEl.value = "";
  }
});

// ---- canvas / grid events ---------------------------------------------
const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");
let drawColor = "#4ea1ff";
function resizeCanvas() {
  const r = canvas.getBoundingClientRect();
  canvas.width = Math.max(200, Math.floor(r.width));
  canvas.height = Math.max(200, Math.floor(r.height));
  drawGrid();
}
function drawGrid() {
  ctx.fillStyle = "#0a0e13"; ctx.fillRect(0,0,canvas.width, canvas.height);
  ctx.strokeStyle = "#15202c"; ctx.lineWidth = 1;
  for (let x=0; x<canvas.width; x+=20){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,canvas.height);ctx.stroke();}
  for (let y=0;y<canvas.height;y+=20){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(canvas.width,y);ctx.stroke();}
}
function handleGrid(cmd) {
  if (cmd.type === "clear") { drawGrid(); return; }
  if (cmd.type === "color") { drawColor = cmd.color; return; }
  if (cmd.type === "plot") { ctx.fillStyle = drawColor; ctx.fillRect(cmd.x, cmd.y, 3, 3); return; }
  if (cmd.type === "line") { ctx.strokeStyle = drawColor; ctx.lineWidth = 2; ctx.beginPath(); ctx.moveTo(cmd.x1, cmd.y1); ctx.lineTo(cmd.x2, cmd.y2); ctx.stroke(); return; }
  if (cmd.type === "rect") { ctx.strokeStyle = drawColor; ctx.strokeRect(cmd.x, cmd.y, cmd.w, cmd.h); return; }
  if (cmd.type === "circle") { ctx.strokeStyle = drawColor; ctx.beginPath(); ctx.arc(cmd.x, cmd.y, Math.max(1,cmd.r), 0, Math.PI*2); ctx.stroke(); return; }
  if (cmd.type === "text") { ctx.fillStyle = drawColor; ctx.font = "14px monospace"; ctx.fillText(cmd.text, cmd.x, cmd.y); return; }
  if (cmd.type === "gui_show") { switchPanel("canvas"); return; }
  if (cmd.type === "alert") { appendConsole("[alert] " + cmd.message, "status"); return; }
  if (cmd.type === "log") { appendConsole("[grid] " + cmd.message, "grid"); return; }
}
document.getElementById("canvas-clear").onclick = drawGrid;
window.addEventListener("resize", resizeCanvas);
resizeCanvas();

// ---- panel switching ---------------------------------------------------
function switchPanel(name) {
  document.querySelectorAll(".ptab").forEach(b => b.classList.toggle("active", b.dataset.panel === name));
  document.querySelectorAll(".panel").forEach(p => p.classList.toggle("active", p.id === "panel-" + name));
}
document.querySelectorAll(".ptab").forEach(b => b.onclick = () => switchPanel(b.dataset.panel));

// ---- run-in-panel helper (separate run, output into a given element) ---
const panelRuns = {};
async function runInPanel(source, logEl, idKey, onInput) {
  if (panelRuns[idKey]) { fetch("/api/stop",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({id:panelRuns[idKey]})}); }
  logEl.innerHTML = "";
  const r = await fetch("/api/run", {method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({source})});
  const j = await r.json();
  if (!j.id) return;
  panelRuns[idKey] = j.id;
  let since = 0;
  const tick = async () => {
    if (!panelRuns[idKey] || panelRuns[idKey] !== j.id) return;
    try {
      const rr = await fetch(`/api/output?id=${j.id}&since=${since}`);
      const jj = await rr.json();
      for (const ev of (jj.events||[])) {
        if (ev.type === "output") { logEl.textContent += ev.text; logEl.scrollTop = logEl.scrollHeight; }
        else if (ev.type === "error") { logEl.textContent += "Error: " + ev.message + "\n"; }
        else if (ev.type === "grid") handleGrid(ev.cmd);
        else if (ev.type === "end") { logEl.textContent += ev.error ? ("✗ "+ev.error+"\n") : "✓ done\n"; panelRuns[idKey] = null; logEl.scrollTop = logEl.scrollHeight; setTimeout(tick, 400); return; }
      }
      since = jj.next;
    } catch(e){}
    if (panelRuns[idKey] === j.id) setTimeout(tick, 150);
  };
  tick();
}

// ---- IRC panel ---------------------------------------------------------
const ircLog = document.getElementById("irc-log");
function ircSource(host, port, nick, tls, chan) {
  const tlsLit = tls ? "true" : "false";
  return [
    'import irc',
    'let conn = IRC.connect("' + host + '", ' + port + ', "' + nick + '", ' + tlsLit + ')',
    'conn.join("' + chan + '")',
    'conn.on_message(fn(ch, who, msg) => print(f"[{ch}] <{who}> {msg}"))',
    'conn.on_join(fn(ch, who) => print(f"* {who} joined {ch}"))',
    'spawn fn() => {',
    '  while true',
    '    let line = input()',
    '    if line = "/quit" then',
    '      conn.quit()',
    '      stop',
    '    end if',
    '    if line = "" then continue end if',
    '    conn.send("' + chan + '", line)',
    '    print(f"[' + chan + '] <' + nick + '> {line}")',
    '  wend',
    '}',
    'print(f"Connected to ' + host + ' as ' + nick + '; typing below sends to ' + chan + '. /quit to leave.")',
    'conn.loop()'
  ].join("\n");
}
document.getElementById("irc-connect").onclick = () => {
  const host = document.getElementById("irc-host").value.trim();
  const port = parseInt(document.getElementById("irc-port").value) || 6667;
  const nick = document.getElementById("irc-nick").value.trim() || "GridBot";
  const tls = document.getElementById("irc-tls").checked;
  const chan = document.getElementById("irc-channel").value.trim() || "#gridbasic";
  runInPanel(ircSource(host, port, nick, tls, chan), ircLog, "irc");
};
document.getElementById("irc-join").onclick = () => {
  const ch = document.getElementById("irc-channel").value.trim();
  if (panelRuns.irc) {
    fetch("/api/input",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({id:panelRuns.irc,line:"/join "+ch})});
  }
};
document.getElementById("irc-input").addEventListener("keydown", (e) => {
  if (e.key === "Enter" && panelRuns.irc) {
    fetch("/api/input",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({id:panelRuns.irc,line:e.target.value})});
    e.target.value = "";
  }
});

// ---- Crypto panel ------------------------------------------------------
const cryptoInfo = document.getElementById("crypto-info");
document.getElementById("crypto-new-wallet").onclick = () => {
  runInPanel(`import crypto
let w = CRYPTO.wallet()
print "Address : " + w.address
print "Public  : " + w.pub_compressed()
print "Private : " + w.priv_hex()
let sig = CRYPTO.sign(w, "gridcoin demo")
print "Sig OK  : " + str(CRYPTO.verify(w.pub_compressed(), "gridcoin demo", sig))`, cryptoInfo, "crypto");
};
document.getElementById("crypto-mine").onclick = () => {
  runInPanel(`import crypto
let c = CRYPTO.blockchain(3)
let w = CRYPTO.wallet()
c.transfer(w, w.address, 50)
c.mine_pending(w)
print "Height  : " + str(c.height())
print "Balance : " + str(c.balance_of(w.address)) + " GRID"
print "Valid   : " + str(c.is_valid())
print "Hash    : " + c.last().hash`, cryptoInfo, "crypto");
};

// ---- AI panel ----------------------------------------------------------
const aiOutput = document.getElementById("ai-output");
let trainedCorpus = "";
document.getElementById("ai-train").onclick = () => {
  trainedCorpus = document.getElementById("ai-corpus").value;
  aiOutput.textContent = "Trained on " + trainedCorpus.length + " chars.";
};
document.getElementById("ai-generate").onclick = () => {
  const prompt = document.getElementById("ai-prompt").value.trim();
  const corpus = trainedCorpus || document.getElementById("ai-corpus").value || "";
  const c = corpus.replace(/"/g, '\\"').replace(/\n/g, " ");
  const p = prompt.replace(/"/g, '\\"');
  runInPanel(`import ai
let m = AI.model("markov")
m.train("${c}")
print AI.generate("${p}", length=40)`, aiOutput, "ai");
};

// ---- boot --------------------------------------------------------------
(async function init() {
  const r = await fetch("/api/info");
  const j = await r.json();
  document.getElementById("version").textContent = "IDE " + j.version;
  await loadFiles();
  await loadExamples();
  // load tour by default if present
  try { await openFile("examples/tour.gb"); } catch(e) {
    editor.value = `' Welcome to GridBasic!\nPRINT "Hello, GridBasic!"\n\n' Try the tour from the Examples dropdown.\n`;
    syncHighlight();
  }
  setStatus("ready");
})();
