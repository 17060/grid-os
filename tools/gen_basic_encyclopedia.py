#!/usr/bin/env python3
"""Generate GridBASIC IDE encyclopedia markdown + sample programs.

Output:
  docs/wiki/encyclopedia/*.md   — full reference with purpose/actions/samples
  programs/encyclopedia/*.bas   — one runnable (or documented) sample per entry

Run: python3 tools/gen_basic_encyclopedia.py
     make gen-encyclopedia
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WIKI_OUT = ROOT / "docs" / "wiki" / "encyclopedia"
PROG_OUT = ROOT / "programs" / "encyclopedia"


@dataclass
class Entry:
    slug: str
    title: str
    category: str
    where: str
    syntax: str
    purpose: str
    actions: str
    sample: list[str]
    see_also: list[str] = field(default_factory=list)
    runnable: bool = True


def fmt_bas(title: str, where: str, purpose: str, actions: str, body: list[str]) -> list[str]:
    lines = [
        f"10 REM Encyclopedia: {title}",
        f"20 REM Where: {where}",
        f"30 REM Purpose: {purpose}",
        f"40 REM Action: {actions}",
    ]
    n = 50
    for stmt in body:
        lines.append(f"{n} {stmt}")
        n += 10
    lines.append(f"{n} END")
    return lines


def shell_meta(slug: str, cmd: str, purpose: str, actions: str, extra: list[str] | None = None) -> Entry:
    body = [f'PRINT "At grid> type: {cmd}"'] + (extra or [])
    return Entry(
        slug=f"cmd-{slug}",
        title=cmd,
        category="shell",
        where="grid> shell (Esc from IDE)",
        syntax=cmd,
        purpose=purpose,
        actions=actions,
        sample=fmt_bas(cmd, "grid> shell", purpose, actions, body),
        see_also=["shell-from-ide.md"],
        runnable=True,
    )


def colon_meta(slug: str, cmd: str, purpose: str, actions: str, body: list[str] | None = None) -> Entry:
    b = body or [f'PRINT "Press Esc then type: {cmd}"']
    return Entry(
        slug=f"ide-{slug}",
        title=cmd,
        category="ide",
        where="IDE colon bar (Esc)",
        syntax=cmd,
        purpose=purpose,
        actions=actions,
        sample=fmt_bas(cmd, "IDE colon bar", purpose, actions, b),
        see_also=["colon-commands.md"],
    )


def kw(slug: str, title: str, syntax: str, purpose: str, actions: str, body: list[str], **kw_args) -> Entry:
    return Entry(
        slug=f"kw-{slug}",
        title=title,
        category="keyword",
        where="GridBASIC program source",
        syntax=syntax,
        purpose=purpose,
        actions=actions,
        sample=fmt_bas(title, "GridBASIC program", purpose, actions, body),
        see_also=["statements.md"],
        **kw_args,
    )


def builtin(slug: str, title: str, syntax: str, purpose: str, actions: str, body: list[str]) -> Entry:
    return Entry(
        slug=f"fn-{slug}",
        title=title,
        category="builtin",
        where="GridBASIC expression",
        syntax=syntax,
        purpose=purpose,
        actions=actions,
        sample=fmt_bas(title, "GridBASIC expression", purpose, actions, body),
        see_also=["builtins.md"],
    )


def grid_entry(slug: str, title: str, syntax: str, purpose: str, actions: str, body: list[str], fn: bool = False) -> Entry:
    return Entry(
        slug=f"grid-{slug}",
        title=title,
        category="grid-fn" if fn else "grid-stmt",
        where="GridBASIC program (GRID binding)",
        syntax=syntax,
        purpose=purpose,
        actions=actions,
        sample=fmt_bas(title, "GridBASIC GRID binding", purpose, actions, body),
        see_also=["grid-bindings.md"],
    )


def pp(slug: str, title: str, syntax: str, purpose: str, actions: str, body: list[str]) -> Entry:
    return Entry(
        slug=f"pp-{slug}",
        title=title,
        category="preprocessor",
        where="GridBASIC source (# directive at line start)",
        syntax=syntax,
        purpose=purpose,
        actions=actions,
        sample=body,  # preprocessor samples include # lines
        see_also=["preprocessor.md"],
    )


def build_entries() -> list[Entry]:
    e: list[Entry] = []

    # --- Editor keys ---
    editor = [
        ("editor-left", "Left arrow", "Move cursor one column left"),
        ("editor-right", "Right arrow", "Move cursor one column right"),
        ("editor-up", "Up arrow", "Move cursor up one line; at grid> recalls history"),
        ("editor-down", "Down arrow", "Move cursor down; at grid> recalls history"),
        ("editor-home", "Home", "Jump to start of current line"),
        ("editor-end", "End", "Jump to end of current line"),
        ("editor-enter", "Enter", "In editor: split line; at grid>: submit command"),
        ("editor-backspace", "Backspace", "Delete char left or merge lines"),
        ("editor-delete", "Delete", "Delete char right or merge with line below"),
        ("editor-tab", "Tab", "Insert four spaces in editor"),
        ("editor-esc", "Esc", "Open grid> command bar from editor"),
        ("editor-ctrlc", "Ctrl+C", "Ignored in editor buffer"),
    ]
    for slug, title, action in editor:
        e.append(Entry(
            slug=slug, title=title, category="editor", where="IDE editor",
            syntax=title, purpose=f"Editor navigation/editing: {title}",
            actions=action,
            sample=fmt_bas(title, "IDE editor", f"Key: {title}", action,
                           [f'PRINT "Use key: {title}"']),
            see_also=["editor-keys.md"],
        ))

    # --- IDE colon commands ---
    ide_cmds = [
        ("run", ":run [:r] [path]", "Run buffer or GFS file", "Executes GridBASIC from editor or path",
         ['PRINT "IDE-RUN-OK"']),
        ("save", ":save <name>", "Save buffer to Flynn disk", "Writes /programs/<name>.bas",
         ['PRINT "saved"']),
        ("load", ":load <name>", "Load program into editor", "Reads /programs/<name>.bas or .grid",
         ['PRINT GRID.STATUS$']),
        ("new", ":new", "Clear editor buffer", "Resets buffer, path, dirty flag",
         ['PRINT "fresh buffer"']),
        ("list", ":list [:l]", "List buffer fullscreen", "Shows numbered program; key to return",
         ['PRINT "line 1"']),
        ("compile", ":compile <name>", "Compile to bytecode", "Writes GRIDBC to /programs/<name>.grid",
         ['PRINT "compile me from buffer"']),
        ("find", ":find <text>", "Search buffer", "Case-insensitive search from cursor",
         ['PRINT "search target"']),
        ("goto", ":goto <line>", "Jump to line", "Moves editor cursor to 1-based line",
         ['PRINT "line 1"', 'PRINT "line 2"']),
        ("mods", ":mods [category]", "List IDE modules", "Runs pkg mods with optional filter",
         ['PRINT GRID.PKG.MODS$']),
        ("mod-run", ":mod run <name>", "Run IDE module", "Executes package module fullscreen",
         ['PRINT "use :mod run disc-status"']),
        ("mod-load", ":mod load <name>", "Load module source", "Opens module .bas in editor",
         ['PRINT "use :mod load ide-cheatsheet"']),
        ("pkg-list", ":pkg list", "List packages", "Shows installed Grid packages",
         ['PRINT GRID.PKG.LIST$']),
        ("pkg-mods", ":pkg mods [cat]", "List modules via :pkg", "Same as :mods",
         ['PRINT GRID.PKG.MODS$']),
        ("samples", ":samples", "List sample programs", "Shell passthrough: samples",
         ['PRINT "Esc then :samples"']),
        ("tutorial", ":tutorial [:t]", "Interactive tutorial", "Multi-step IDE walkthrough",
         ['PRINT "Esc then :tutorial"']),
        ("help", ":help [:h]", "IDE help overlay", "Full command and language summary",
         ['PRINT "Esc then :help"']),
        ("quit", ":quit [:q]", "Quit hint", "Reminds to use poweroff; does not exit OS",
         ['PRINT "use grid> poweroff"']),
        ("ai-ask", ":ai ask <prompt>", "Ask Grid AI", "Sends prompt to host bridge or offline help",
         ['PRINT GRID.AI.ASK$("What is PRINT?", "ASK")']),
        ("ai-explain", ":ai explain", "Explain current line", "AI explains editor line under cursor",
         ['PRINT "move cursor then :ai explain"']),
        ("ai-complete", ":ai complete", "Complete buffer", "AI suggests completion for buffer",
         ['PRINT "partial code then :ai complete"']),
        ("ai-fix", ":ai fix <code>", "Fix GridBASIC code", "AI suggests corrected source",
         ['PRINT "paste code to :ai fix"']),
        ("ai-models", ":ai models", "Show AI models", "Bridge model info",
         ['PRINT GRID.AI.MODELS$']),
        ("redteam", ":redteam", "Red team lab menu", "Lists 100 offensive recon demos",
         ['PRINT "Esc then :redteam"']),
        ("blackhat", ":blackhat", "Black hat lab menu", "Lists 100 malicious-pattern demos",
         ['PRINT "Esc then :blackhat"']),
        ("whiteteam", ":whiteteam", "White team lab menu", "Lists 100 ethical demos",
         ['PRINT "Esc then :whiteteam"']),
        ("blueteam", ":blueteam", "Blue team lab menu", "Lists 100 SOC demos",
         ['PRINT "Esc then :blueteam"']),
        ("purpleteam", ":purpleteam", "Purple team chains", "Lists 25 attack/detect/fix demos",
         ['PRINT "Esc then :purpleteam"']),
        ("greenteam", ":greenteam", "Green hat lab", "Lists 75 DevSecOps demos",
         ['PRINT "Esc then :greenteam"']),
        ("yellowteam", ":yellowteam", "Yellow hat lab", "Lists 50 audit demos",
         ['PRINT "Esc then :yellowteam"']),
        ("orangeteam", ":orangeteam", "Orange hat lab", "Lists 50 threat intel demos",
         ['PRINT "Esc then :orangeteam"']),
        ("greyteam", ":greyteam", "Grey hat lab", "Lists 100 gray-ethics demos",
         ['PRINT "Esc then :greyteam"']),
        ("daemonteam", ":daemonteam", "Flynn daemon lab", "Lists 50 IDE background demos",
         ['PRINT "Esc then :daemonteam"']),
    ]
    for slug, syn, purpose, actions, body in ide_cmds:
        e.append(colon_meta(slug, syn.split()[0], purpose, actions, body))

    # --- Shell commands ---
    shells = [
        ("help", "help", "Show shell command list", "Prints all grid> commands"),
        ("disc", "disc", "Show identity disc", "Displays disc level, XP, entity"),
        ("whoami", "whoami", "Show entity type", "Prints User or Program"),
        ("caps", "caps", "Show capabilities", "Decodes granted cap bitmask"),
        ("status", "status", "Runtime status", "Prints Grid OS status string"),
        ("cycles", "cycles", "Elapsed cycles", "Same summary as status"),
        ("vision", "vision", "Flynn principles", "Prints founding vision text"),
        ("clear", "clear", "Clear screen", "Clears console and redraws banner"),
        ("about", "about", "About Grid OS", "Version and credits"),
        ("poweroff", "poweroff", "Exit Grid OS", "Triggers isa-debug-exit in QEMU"),
        ("spawn", "spawn [name]", "Run ring-3 program", "Loads ELF sandbox (default gridsh)"),
        ("spawn-bg", "spawn bg <name>", "Background spawn", "Queues sandbox job"),
        ("spawn-list", "spawn list", "List spawnable programs", "Shows /programs/*.elf catalog"),
        ("catalog", "catalog", "Spawn catalog", "Lists ring-3 programs on disk"),
        ("programs", "programs", "Spawn history", "Shows previously spawned programs"),
        ("jobs", "jobs", "Background jobs", "Lists active sandbox jobs"),
        ("kill", "kill <#>", "Stop job", "Terminates background job by number"),
        ("kill-all", "kill all", "Stop all jobs", "Terminates every background job"),
        ("fg", "fg <#>", "Foreground job", "Runs background job in foreground"),
        ("wait", "wait", "Wait for jobs", "Blocks until all bg jobs finish"),
        ("echo", "echo <text>", "Print text", "Writes argument to console"),
        ("ls", "ls [path]", "List GridFS", "Directory listing, default /"),
        ("cat", "cat <path>", "Read GridFS file", "Prints file contents"),
        ("gfs", "gfs", "Flynn archive status", "Shows GFS mount info"),
        ("gfs-list", "gfs list", "List GFS root", "Lists top-level Flynn paths"),
        ("gfs-seed", "gfs seed", "Re-seed disk files", "Restores default GFS entries"),
        ("log", "log", "Full audit log", "Prints entire audit trail"),
        ("log-tail", "log tail", "Recent audit entries", "Last 10 log lines"),
        ("vault-list", "vault list", "List vault keys", "Shows persistent key/value nodes"),
        ("vault-put", "vault put <k> <v>", "Store vault node", "Sets key to value in vault"),
        ("vault-get", "vault get <k>", "Read vault node", "Prints value for key"),
        ("vault-save", "vault save", "Vault memory snapshot", "CRC-seals in-memory vault"),
        ("vault-sync", "vault sync", "Persist vault", "Writes vault to arcade disk"),
        ("vault-export", "vault export", "Export vault COM1", "Serial export frame"),
        ("vault-import", "vault import", "Import vault COM1", "Serial import frame"),
        ("net", "net status", "Network status", "virtio-net and IP summary"),
        ("net-ping", "net ping <host>", "ICMP ping", "Pings host or built-in name"),
        ("net-poll", "net poll", "Drain net queue", "Processes pending packets"),
        ("http-get", "http get <host> [port] <path>", "HTTP GET", "HTTP/1.1 GET with keep-alive"),
        ("http-post", "http post <host> [port] <path> <body>", "HTTP POST", "HTTP/1.1 POST request"),
        ("irc-connect", "irc connect <h> <p> <nick>", "IRC connect", "Opens persistent IRC TCP session"),
        ("irc-join", "irc join <#chan>", "IRC join", "JOIN channel on active session"),
        ("irc-say", "irc say <#c> <msg>", "IRC message", "Sends PRIVMSG"),
        ("irc-read", "irc read", "IRC read queue", "Prints queued server lines"),
        ("irc-status", "irc status", "IRC session info", "Connection summary"),
        ("irc-quit", "irc quit", "IRC disconnect", "Sends QUIT and closes"),
        ("portal", "portal", "GridLink portal", "Portal connection status"),
        ("portal-export", "portal export", "Export vault link", "COM1 vault frame"),
        ("portal-import", "portal import", "Import vault link", "Receive vault from host"),
        ("portal-recv", "portal recv", "Receive program", "Install /programs/* via GridLink"),
        ("portal-pkg", "portal pkg", "Receive package", "Install .gridpkg bundle"),
        ("pkg-list", "pkg list", "List packages", "Installed Grid packages"),
        ("pkg-mods", "pkg mods [cat]", "List IDE modules", "Module names, optional category"),
        ("pkg-info", "pkg info <name>", "Package details", "Version, desc, module list"),
        ("pkg-install", "pkg install <manifest>", "Install package", "Register MANIFEST on disk"),
        ("pkg-remove", "pkg remove <name>", "Remove package", "Uninstall package files"),
        ("pkg-recv", "pkg recv", "Receive PKG COM1", "GridLink package frame"),
        ("serial-status", "serial status", "COM1 status", "Online/offline at 0x3F8"),
        ("serial-write", "serial write <text>", "COM1 transmit", "Writes text to serial port"),
        ("serial-read", "serial read", "COM1 read line", "Reads one line from COM1"),
        ("iso-zone", "iso zone", "ISO research zone", "Zone status summary"),
        ("iso-list", "iso list", "List ISO entities", "Research zone entity list"),
        ("iso-spawn", "iso spawn [name]", "Spawn ISO", "Seeds new ISO research entity"),
        ("iso-inspect", "iso inspect <id>", "Inspect ISO", "Genome and disc details"),
        ("iso-evolve", "iso evolve <id>", "Evolve ISO", "Mutate genome in sandbox"),
        ("iso-quarantine", "iso quarantine <id>", "Quarantine ISO", "Isolates anomaly"),
        ("iso-release", "iso release <id>", "Release ISO", "Restores quarantined entity"),
        ("iso-autopilot", "iso autopilot on|off", "ISO autopilot", "Background evolution toggle"),
        ("basic", "basic", "Open IDE", "Enters GridBASIC IDE"),
        ("basic-run", "basic run <file>", "Run GFS program", "Executes .bas or .grid from disk"),
        ("basic-compile", "basic compile <in> <out>", "Compile file", "Produces .grid bytecode"),
        ("basic-mod-run", "basic mod run <name>", "Run module", "Executes IDE package module"),
        ("basic-samples", "basic samples", "List samples", "Flynn disk .bas listing"),
        ("tutorial", "tutorial", "Run tutorial", "Executes /programs/tutorial.bas"),
        ("samples", "samples", "List samples", "Same as basic samples listing"),
        ("basictest", "basictest", "Interpreter test", "Deterministic self-test (OK15)"),
        ("redteam", "redteam", "Red team lab", "Lists /programs/redteam/ demos"),
        ("blackhat", "blackhat", "Black hat lab", "Lists /programs/blackhat/ demos"),
        ("whiteteam", "whiteteam", "White team lab", "Lists /programs/whiteteam/ demos"),
        ("blueteam", "blueteam", "Blue team lab", "Lists /programs/blueteam/ demos"),
        ("purpleteam", "purpleteam", "Purple team lab", "Lists /programs/purpleteam/ chains"),
        ("greenteam", "greenteam", "Green hat lab", "Lists /programs/greenteam/ demos"),
        ("yellowteam", "yellowteam", "Yellow hat lab", "Lists /programs/yellowteam/ demos"),
        ("orangeteam", "orangeteam", "Orange hat lab", "Lists /programs/orangeteam/ demos"),
        ("greyteam", "greyteam", "Grey hat lab", "Lists /programs/greyteam/ demos"),
        ("daemonteam", "daemonteam", "Flynn daemon lab", "Lists /programs/daemonteam/ demos"),
        ("ai", "ai", "AI command summary", "Shows ai subcommands"),
        ("ai-ask", "ai ask <prompt>", "Ask AI", "Sends prompt to bridge"),
        ("ai-explain", "ai explain [line]", "Explain BASIC line", "AI line explanation"),
        ("ai-fix", "ai fix <code>", "Fix BASIC code", "AI suggested fix"),
        ("ai-complete", "ai complete <code>", "Complete code", "AI completion"),
        ("ai-models", "ai models", "AI model info", "Bridge model listing"),
        ("btc", "btc", "Bitcoin summary", "Shows btc subcommands"),
        ("btc-status", "btc status", "BTC bridge status", "Connection to host node"),
        ("btc-balance", "btc balance", "Wallet balance", "getbalance via bridge"),
        ("btc-info", "btc info", "Blockchain info", "getblockchaininfo RPC"),
        ("btc-network", "btc network", "Network info", "getnetworkinfo RPC"),
        ("btc-call", "btc call <method> [json]", "Arbitrary RPC", "Generic JSON-RPC call"),
        ("theme-flynn", "theme flynn", "Flynn theme", "Cyan shell prompt colors"),
        ("theme-clu", "theme clu", "CLU theme", "Red shell prompt colors"),
        ("recognizer", "recognizer", "Patrol easter egg", "Recognizer flyover animation"),
        ("recognizer-start", "recognizer start", "Start patrol", "Background recognizer service"),
        ("recognizer-stop", "recognizer stop", "Stop patrol", "Stops recognizer service"),
        ("ide", "ide [file]", "Grid Workbench", "Opens GEM + AmigaDOS workshop"),
        ("workshop", "workshop [file]", "Grid Workbench", "Alias for ide command"),
    ]
    for slug, cmd, purpose, actions in shells:
        e.append(shell_meta(slug, cmd, purpose, actions))

    # --- Statement keywords ---
    e += [
        kw("print", "PRINT", "PRINT expr [; expr] ...", "Output text and values",
           "Evaluates expressions and writes to console; ; suppresses spacing, , uses columns",
           ['PRINT "hello"; 42', 'PRINT GRID.WHOAMI$']),
        kw("qmark", "?", "? expr", "PRINT shorthand", "Same as PRINT", ['? "quick print"']),
        kw("let", "LET", "LET x = expr", "Assign variable", "Stores expression result in variable",
           ['LET N = 42', 'PRINT N']),
        kw("const", "CONST", "CONST name = expr", "Declare constant", "Creates read-only binding",
           ['CONST MAX = 10', 'PRINT MAX']),
        kw("if", "IF", "IF cond THEN stmt [ELSE stmt]", "Conditional branch",
           "Runs THEN block when condition true, optional ELSE when false",
           ['IF 1 THEN PRINT "yes" ELSE PRINT "no"']),
        kw("elseif", "ELSEIF", "ELSEIF cond THEN stmt", "Chained condition",
           "Tests another condition after IF", ['IF A=1 THEN PRINT "a" ELSEIF A=2 THEN PRINT "b"']),
        kw("select", "SELECT CASE", "SELECT CASE expr ... END SELECT", "Multi-way branch",
           "Dispatches on expression value to matching CASE block",
           ['SELECT CASE 2', 'CASE 1', '  PRINT "one"', 'CASE 2', '  PRINT "two"', 'END SELECT']),
        kw("for", "FOR", "FOR i = a TO b [STEP s] ... NEXT", "Counted loop",
           "Increments loop variable each iteration until past end bound",
           ['FOR I = 1 TO 3', '  PRINT I', 'NEXT I']),
        kw("step", "STEP", "FOR i = a TO b STEP s", "FOR step size", "Sets increment for FOR loop",
           ['FOR I = 0 TO 10 STEP 2', '  PRINT I', 'NEXT I']),
        kw("next", "NEXT", "NEXT [var]", "End FOR loop", "Advances loop; exits when past TO bound",
           ['FOR J = 1 TO 2', 'PRINT J', 'NEXT J']),
        kw("exit-for", "EXIT FOR", "EXIT FOR", "Leave FOR early", "Jumps to statement after NEXT",
           ['FOR I = 1 TO 5', 'IF I = 3 THEN EXIT FOR', 'PRINT I', 'NEXT I']),
        kw("continue-for", "CONTINUE FOR", "CONTINUE FOR", "Skip to next FOR iter", "Jumps to NEXT without finishing body",
           ['FOR I = 1 TO 4', 'IF I = 2 THEN CONTINUE FOR', 'PRINT I', 'NEXT I']),
        kw("while", "WHILE", "WHILE cond ... WEND", "Pre-test loop", "Repeats while condition true",
           ['N = 3', 'WHILE N > 0', '  PRINT N', '  N = N - 1', 'WEND']),
        kw("wend", "WEND", "WEND", "End WHILE loop", "Returns to WHILE condition test",
           ['X = 1', 'WHILE X < 3', 'PRINT X', 'X = X + 1', 'WEND']),
        kw("exit-while", "EXIT WHILE", "EXIT WHILE", "Leave WHILE early", "Exits to statement after WEND",
           ['WHILE 1', 'PRINT "once"', 'EXIT WHILE', 'WEND']),
        kw("repeat", "REPEAT", "REPEAT ... UNTIL cond", "Post-test loop", "Runs body at least once",
           ['REPEAT', '  PRINT "loop"', 'UNTIL 1']),
        kw("until", "UNTIL", "UNTIL expr", "End REPEAT loop", "Tests exit condition after body",
           ['N = 0', 'REPEAT', 'N = N + 1', 'UNTIL N >= 2']),
        kw("goto", "GOTO", "GOTO line", "Unconditional jump", "Transfers control to line number",
           ['GOTO 50', 'PRINT "skip"', '50 PRINT "landed"']),
        kw("gosub", "GOSUB", "GOSUB line", "Call subroutine", "Pushes return address and jumps",
           ['GOSUB 100', 'END', '100 PRINT "sub"', 'RETURN']),
        kw("return", "RETURN", "RETURN", "Return from GOSUB", "Pops return address and resumes",
           ['GOSUB 100', 'END', '100 PRINT "back soon"', 'RETURN']),
        kw("on-goto", "ON GOTO", "ON n GOTO a,b,c", "Branch table jump", "1-based index selects GOTO target",
           ['ON 2 GOTO 100,200', '100 PRINT "one"', '200 PRINT "two"']),
        kw("on-error", "ON ERROR GOTO", "ON ERROR GOTO line", "Error handler", "Jumps to line on runtime error",
           ['ON ERROR GOTO 900', 'X = 1/0', '900 PRINT ERR$', 'END']),
        kw("resume", "RESUME", "RESUME [NEXT|line]", "Resume after error", "Continues after error handler",
           ['ON ERROR GOTO 100', 'X=1/0', '100 PRINT "err"', 'RESUME NEXT']),
        kw("def-fn", "DEF FN", "DEF FN f(x)=expr", "Single-line function", "Defines inline numeric function",
           ['DEF FN SQ(X) = X * X', 'PRINT FN SQ(5)']),
        kw("sub", "SUB", "SUB name ... END SUB", "Procedure", "Defines callable subroutine with CALL",
           ['SUB HI(N$)', '  PRINT "Hi "; N$', 'END SUB', 'CALL HI("grid")']),
        kw("function", "FUNCTION", "FUNCTION f ... END FUNCTION", "Function procedure", "Returns value via name assignment",
           ['FUNCTION DOUBLE(X)', '  DOUBLE = X * 2', 'END FUNCTION', 'PRINT DOUBLE(21)']),
        kw("call", "CALL", "CALL name(args)", "Invoke SUB", "Calls defined subroutine",
           ['SUB P()', '  PRINT "ok"', 'END SUB', 'CALL P()']),
        kw("local", "LOCAL", "LOCAL x", "Local variable", "Declares variable local to SUB/FUNCTION",
           ['SUB T()', '  LOCAL X', '  X = 1', '  PRINT X', 'END SUB', 'CALL T()']),
        kw("shared", "SHARED", "SHARED x", "Shared variable", "Marks module-level shared in SUB",
           ['SHARED C', 'SUB B()', '  C = 1', 'END SUB', 'CALL B()', 'PRINT C']),
        kw("option-base", "OPTION BASE", "OPTION BASE 0|1", "Array origin", "Sets default array lower bound",
           ['OPTION BASE 1', 'DIM A(3)', 'A(1) = 9', 'PRINT A(1)']),
        kw("input", "INPUT", "INPUT [prompt$;] var", "Read input", "Reads user input into variable",
           ['INPUT "Enter"; N', 'PRINT N']),
        kw("line-input", "LINE INPUT", "LINE INPUT s$", "Read full line", "Reads entire line including spaces",
           ['LINE INPUT MSG$', 'PRINT LEN(MSG$)']),
        kw("dim", "DIM", "DIM A(n) or DIM M(r,c)", "Declare array", "Allocates numeric array storage",
           ['DIM A(2)', 'A(0) = 7', 'PRINT A(0)']),
        kw("data", "DATA", "DATA v1, v2, ...", "Static literals", "Declares DATA values for READ",
           ['DATA 10, 20', 'READ A, B', 'PRINT A + B']),
        kw("read", "READ", "READ var [, var ...]", "Read DATA", "Reads next values from DATA pool",
           ['DATA 3, 4', 'READ X, Y', 'PRINT X * Y']),
        kw("restore", "RESTORE", "RESTORE", "Reset DATA pointer", "Rewinds READ to first DATA",
           ['DATA 1', 'READ A', 'RESTORE', 'READ B', 'PRINT A; B']),
        kw("randomize", "RANDOMIZE", "RANDOMIZE [seed]", "Seed PRNG", "Seeds RND/GRID.RND generator",
           ['RANDOMIZE 42', 'PRINT RND(100)']),
        kw("rem", "REM", "REM comment", "Comment to EOL", "Ignored text until end of line",
           ['REM this is a comment', 'PRINT "ok"']),
        kw("tick", "'", "' comment", "Apostrophe comment", "Alternate comment form to end of line",
           ["' apostrophe comment", 'PRINT 1']),
        kw("end", "END", "END", "End program", "Stops execution immediately",
           ['PRINT "done"', 'END', 'PRINT "never"']),
        kw("stop", "STOP", "STOP", "Break program", "Halts with break message",
           ['PRINT "halt"', 'STOP']),
        kw("colon", ":", "stmt : stmt", "Statement separator", "Runs multiple statements on one line",
           ['A = 1: B = 2: PRINT A + B']),
    ]

    # Expression keywords
    for name, slug, purpose, actions, body in [
        ("AND", "and", "Logical conjunction", "True if both operands true", ['IF 1 AND 1 THEN PRINT "and"']),
        ("OR", "or", "Logical disjunction", "True if either operand true", ['IF 0 OR 1 THEN PRINT "or"']),
        ("NOT", "not", "Logical negation", "Inverts boolean value", ['IF NOT 0 THEN PRINT "not"']),
        ("MOD", "mod", "Modulo operator", "Remainder after division", ['PRINT 10 MOD 3']),
        ("DIV", "div", "Integer division", "Quotient truncated toward zero", ['PRINT 10 DIV 3']),
    ]:
        e.append(kw(slug, name, name, purpose, actions, body))

    # Builtins
    builtins = [
        ("ABS", "ABS(x)", "Absolute value", ['PRINT ABS(-7)']),
        ("INT", "INT(x)", "Floor toward -inf", ['PRINT INT(3.9)']),
        ("SGN", "SGN(x)", "Sign -1/0/1", ['PRINT SGN(-5)']),
        ("SQR", "SQR(x)", "Square root", ['PRINT SQR(16)']),
        ("RND", "RND(n)", "Random 0..n-1", ['RANDOMIZE 1', 'PRINT RND(10)']),
        ("PI", "PI", "Pi constant", ['PRINT PI']),
        ("MIN", "MIN(a,b,...)", "Minimum value", ['PRINT MIN(3, 9)']),
        ("MAX", "MAX(a,b,...)", "Maximum value", ['PRINT MAX(3, 9)']),
        ("FIX", "FIX(x)", "Truncate toward zero", ['PRINT FIX(-3.7)']),
        ("ROUND", "ROUND(x)", "Round to nearest", ['PRINT ROUND(2.6)']),
        ("LEN", "LEN(s$)", "String length", ['PRINT LEN("grid")']),
        ("VAL", "VAL(s$)", "Parse number from string", ['PRINT VAL("42")']),
        ("ASC", "ASC(s$)", "First char code", ['PRINT ASC("A")']),
        ("CHR$", "CHR$(n)", "Char from code", ['PRINT CHR$(65)']),
        ("STR$", "STR$(n)", "Number as string", ['PRINT STR$(99)']),
        ("UPPER$", "UPPER$(s$)", "Uppercase string", ['PRINT UPPER$("grid")']),
        ("LOWER$", "LOWER$(s$)", "Lowercase string", ['PRINT LOWER$("GRID")']),
        ("LEFT$", "LEFT$(s$, n)", "Left substring", ['PRINT LEFT$("hello", 2)']),
        ("RIGHT$", "RIGHT$(s$, n)", "Right substring", ['PRINT RIGHT$("hello", 2)']),
        ("MID$", "MID$(s$, start, len)", "Middle substring", ['PRINT MID$("hello", 2, 3)']),
        ("INSTR$", "INSTR$(hay$, needle$)", "Find substring", ['PRINT INSTR$("Grid OS", "OS")']),
        ("TRIM$", "TRIM$(s$)", "Trim spaces", ['PRINT LEN(TRIM$("  x  "))']),
        ("LTRIM$", "LTRIM$(s$)", "Trim left", ['PRINT LTRIM$("  hi")']),
        ("RTRIM$", "RTRIM$(s$)", "Trim right", ['PRINT RTRIM$("hi  ")']),
        ("SPACE$", "SPACE$(n)", "N spaces", ['PRINT "["; SPACE$(3); "]"']),
        ("STRING$", "STRING$(n, c$)", "Repeat char", ['PRINT STRING$(5, "*")']),
        ("ERR$", "ERR$", "Last error message", ['ON ERROR GOTO 100', 'X=1/0', '100 PRINT ERR$']),
    ]
    for title, syn, purpose, body in builtins:
        e.append(builtin(title.lower().replace("$", "s"), title, syn, purpose,
                           f"Evaluates {title} and returns result", body))

    # GRID statements
    grid_stmts = [
        ("CLS", "GRID.CLS", "Clear screen", "Clears console display", ['GRID.CLS', 'PRINT "clean slate"']),
        ("LOG", "GRID.LOG msg$", "Audit log entry", "Appends message to audit trail", ['GRID.LOG "encyclopedia sample"']),
        ("COLOR", "GRID.COLOR n", "Set text color", "Changes console color index", ['GRID.COLOR 2', 'PRINT "colored"']),
        ("LOCATE", "GRID.LOCATE row, col", "Move cursor", "Positions console cursor", ['GRID.LOCATE 5, 10', 'PRINT "here"']),
        ("WAIT", "GRID.WAIT ticks", "Busy wait", "Delays for timer ticks", ['GRID.WAIT 10', 'PRINT "done"']),
        ("PRINT-grid", "GRID.PRINT expr", "Print with newline", "Prints expression plus newline", ['GRID.PRINT "grid line"']),
        ("SPAWN", "GRID.SPAWN name$", "Spawn ring-3", "Runs sandbox ELF by name", ['PRINT "use GRID.SPAWN \"gridsh\""']),
        ("SPAWN-BG", "GRID.SPAWN.BG name$", "Background spawn", "Queues sandbox job", ['PRINT "use GRID.SPAWN.BG \"gridloop\""']),
        ("VAULT-PUT", "GRID.VAULT.PUT k$, v$", "Vault store", "Sets vault key/value", ['GRID.VAULT.PUT "enc", "demo"', 'GRID.VAULT.SYNC']),
        ("VAULT-SYNC", "GRID.VAULT.SYNC", "Vault persist", "Writes vault to arcade disk", ['GRID.VAULT.SYNC']),
        ("VAULT-EXPORT", "GRID.VAULT.EXPORT", "Vault export", "Exports vault over COM1", ['PRINT "requires COM1 bridge"']),
        ("VAULT-IMPORT", "GRID.VAULT.IMPORT", "Vault import", "Imports vault from COM1", ['PRINT "requires COM1 bridge"']),
        ("GFS-WRITE", "GRID.GFS.WRITE path$, data$", "Write GFS file", "Creates/overwrites Flynn disk file", ['GRID.GFS.WRITE "/programs/encyclopedia/tmp.txt", "hi"']),
        ("JOBS-KILL", "GRID.JOBS.KILL n", "Kill bg job", "Stops background job number", ['PRINT GRID.JOBS.LIST$']),
        ("ISO-SPAWN", "GRID.ISO.SPAWN name$", "Spawn ISO entity", "Seeds ISO research entity", ['PRINT GRID.ISO.LIST$']),
        ("ISO-EVOLVE", "GRID.ISO.EVOLVE id", "Evolve ISO", "Mutates ISO genome", ['PRINT GRID.ISO.LIST$']),
        ("SERIAL-WRITE", "GRID.SERIAL.WRITE expr", "COM1 write", "Transmits text on serial port", ['GRID.SERIAL.WRITE "ping"']),
        ("PLOT", "GRID.PLOT x,y,c", "Plot pixel", "Draws on VGA grid", ['GRID.PLOT 10, 10, 2']),
        ("LINE", "GRID.LINE x0,y0,x1,y1,c", "Draw line", "Line on VGA grid", ['GRID.LINE 0, 0, 20, 20, 1']),
        ("CIRCLE", "GRID.CIRCLE cx,cy,r,c", "Draw circle", "Circle on VGA grid", ['GRID.CIRCLE 40, 40, 10, 3']),
        ("BEEP", "GRID.BEEP freq, ms", "Beep speaker", "Plays tone for milliseconds", ['GRID.BEEP 440, 100']),
        ("NOTE", "GRID.NOTE n, ms", "Play note", "Plays note number", ['GRID.NOTE 60, 100']),
        ("RECOG-START", "GRID.RECOGNIZER.START", "Start patrol", "Starts recognizer background service", ['GRID.RECOGNIZER.START', 'PRINT GRID.RECOGNIZER.STATUS$']),
        ("RECOG-STOP", "GRID.RECOGNIZER.STOP", "Stop patrol", "Stops recognizer service", ['GRID.RECOGNIZER.STOP']),
        ("PORTAL-RECV", "GRID.PORTAL.RECV", "Portal recv file", "Receives GridLink program", ['PRINT "needs host portal"']),
        ("PORTAL-PKG", "GRID.PORTAL.PKG", "Portal recv pkg", "Receives .gridpkg bundle", ['PRINT "needs host portal"']),
        ("PORTAL-DUEL", "GRID.PORTAL.DUEL", "Lightcycle duel", "Portal duel ping + spawn", ['PRINT "GridLink duel"']),
        ("PKG-INSTALL", "GRID.PKG.INSTALL path$", "Install package", "Registers MANIFEST path", ['PRINT GRID.PKG.LIST$']),
        ("PKG-REMOVE", "GRID.PKG.REMOVE name$", "Remove package", "Uninstalls package", ['PRINT GRID.PKG.LIST$']),
        ("PKG-MOD-RUN", "GRID.PKG.MOD.RUN name$", "Run module", "Runs IDE module by name", ['PRINT GRID.PKG.MODS$']),
        ("PKG-RECV", "GRID.PKG.RECV", "Receive package", "COM1 GridLink PKG frame", ['PRINT "needs COM1"']),
        ("IRC-CONNECT", "GRID.IRC.CONNECT h$,p,n$", "IRC connect", "Opens IRC TCP session", ['PRINT GRID.IRC.STATUS$']),
        ("IRC-JOIN", "GRID.IRC.JOIN chan$", "IRC join", "JOIN channel", ['PRINT GRID.IRC.STATUS$']),
        ("IRC-SAY", "GRID.IRC.SAY tgt$, msg$", "IRC message", "Send PRIVMSG", ['PRINT GRID.IRC.STATUS$']),
        ("IRC-QUIT", "GRID.IRC.QUIT", "IRC quit", "Disconnect IRC session", ['PRINT GRID.IRC.STATUS$']),
        ("BTC-SEND", "GRID.BTC.SEND addr$, amt", "Send bitcoin", "sendtoaddress via bridge", ['PRINT GRID.BTC.STATUS$']),
        ("AI-PRINT", "GRID.AI.PRINT prompt$ [, mode$]", "Full AI output", "Prints full AI response to console", ['GRID.AI.PRINT "What is LET?", "EXPLAIN"']),
        ("BTC-PRINT", "GRID.BTC.PRINT method$ [, params$]", "Full BTC output", "Prints full RPC response", ['GRID.BTC.PRINT "getnetworkinfo"']),
        ("WORKSHOP-SPAWN", "GRID.WORKSHOP.SPAWN name$", "Workbench spawn", "Spawns from Grid Workbench", ['PRINT "use ide/workshop"']),
    ]
    for slug, syn, purpose, actions, body in grid_stmts:
        e.append(grid_entry(slug.lower(), syn.split()[0] if "GRID." in syn else syn, syn, purpose, actions, body))

    # GRID functions
    grid_fns = [
        ("TIME", "GRID.TIME", "Timer ticks", ['PRINT GRID.TIME']),
        ("RND-grid", "GRID.RND(n)", "Grid random", ['PRINT GRID.RND(10)']),
        ("PING", "GRID.PING(host$)", "Ping host", ['PRINT GRID.PING("gateway")']),
        ("SERIAL-READ", "GRID.SERIAL.READ$", "COM1 read line", ["PRINT LEN(GRID.SERIAL.READ$)"]),
        ("STATUS", "GRID.STATUS$", "OS status string", ['PRINT GRID.STATUS$']),
        ("CAP", "GRID.CAP(n)", "Capability check", ['PRINT GRID.CAP(1)']),
        ("INKEY", "GRID.INKEY$", "Poll keyboard", ['PRINT GRID.INKEY$']),
        ("VAULT-GET", "GRID.VAULT.GET$(k$)", "Read vault key", ['PRINT GRID.VAULT.GET$("motd")']),
        ("VAULT-LIST", "GRID.VAULT.LIST$", "List vault keys", ['PRINT GRID.VAULT.LIST$']),
        ("GFS-READ", "GRID.GFS.READ$(path$)", "Read GFS file", ['PRINT LEN(GRID.GFS.READ$("/etc/hosts"))']),
        ("GFS-LIST", "GRID.GFS.LIST$(path$)", "List GFS dir", ['PRINT GRID.GFS.LIST$("/programs")']),
        ("HTTP-GET", "GRID.HTTP.GET$(h$,port,path$)", "HTTP GET body", ['PRINT LEN(GRID.HTTP.GET$("gateway", 80, "/"))']),
        ("HTTP-POST", "GRID.HTTP.POST$(...)", "HTTP POST body", ['PRINT "needs bridge/path"']),
        ("DNS", "GRID.DNS.RESOLVE$(host$)", "Resolve DNS", ['PRINT GRID.DNS.RESOLVE$("grid")']),
        ("NET-STATUS", "GRID.NET.STATUS$", "Network summary", ['PRINT GRID.NET.STATUS$']),
        ("LOG-TAIL", "GRID.LOG.TAIL$(n)", "Recent audit lines", ['PRINT GRID.LOG.TAIL$(3)']),
        ("WHOAMI", "GRID.WHOAMI$", "Entity type", ['PRINT GRID.WHOAMI$']),
        ("CAPS", "GRID.CAPS$", "Capability mask", ['PRINT GRID.CAPS$']),
        ("JOBS-LIST", "GRID.JOBS.LIST$", "Background jobs", ['PRINT GRID.JOBS.LIST$']),
        ("ISO-LIST", "GRID.ISO.LIST$", "ISO entities", ['PRINT GRID.ISO.LIST$']),
        ("DISC-STATUS", "GRID.DISC.STATUS$", "Disc summary", ['PRINT GRID.DISC.STATUS$']),
        ("DISC-ENTITY", "GRID.DISC.ENTITY$", "Disc entity", ['PRINT GRID.DISC.ENTITY$']),
        ("DISC-LEVEL", "GRID.DISC.LEVEL", "Disc level", ['PRINT GRID.DISC.LEVEL']),
        ("DISC-XP", "GRID.DISC.XP", "Disc XP", ['PRINT GRID.DISC.XP']),
        ("RECOG-STATUS", "GRID.RECOGNIZER.STATUS$", "Patrol status", ['PRINT GRID.RECOGNIZER.STATUS$']),
        ("PKG-LIST", "GRID.PKG.LIST$", "Package list", ['PRINT GRID.PKG.LIST$']),
        ("PKG-MODS", "GRID.PKG.MODS$", "Module list", ['PRINT GRID.PKG.MODS$']),
        ("AI-ASK", "GRID.AI.ASK$(prompt$)", "Ask AI", ['PRINT GRID.AI.ASK$("What is DIM?", "ASK")']),
        ("AI-COMPLETE", "GRID.AI.COMPLETE$(frag$)", "AI complete", ['PRINT LEN(GRID.AI.COMPLETE$("PRINT "))']),
        ("AI-EXPLAIN", "GRID.AI.EXPLAIN$(line$)", "AI explain", ['PRINT GRID.AI.EXPLAIN$("PRINT 1")']),
        ("AI-FIX", "GRID.AI.FIX$(code$)", "AI fix code", ['PRINT LEN(GRID.AI.FIX$("PRNT 1"))']),
        ("AI-MODELS", "GRID.AI.MODELS$", "AI models info", ['PRINT GRID.AI.MODELS$']),
        ("IRC-READ", "GRID.IRC.READ$", "IRC read line", ['PRINT GRID.IRC.READ$']),
        ("IRC-STATUS", "GRID.IRC.STATUS$", "IRC status", ['PRINT GRID.IRC.STATUS$']),
        ("IRC-CONNECT-fn", "GRID.IRC.CONNECT$(h$,p,n$)", "IRC connect result", ['PRINT GRID.IRC.CONNECT$("gateway", 6667, "u")']),
        ("BTC-CALL", "GRID.BTC.CALL$(m$, params$)", "BTC RPC call", ['PRINT GRID.BTC.CALL$("getnetworkinfo", "")']),
        ("BTC-INFO", "GRID.BTC.INFO$", "Blockchain info", ['PRINT GRID.BTC.INFO$']),
        ("BTC-NETWORK", "GRID.BTC.NETWORK$", "Network info", ['PRINT GRID.BTC.NETWORK$']),
        ("BTC-WALLET", "GRID.BTC.WALLET$", "Wallet info", ['PRINT GRID.BTC.WALLET$']),
        ("BTC-BALANCE", "GRID.BTC.BALANCE$", "Wallet balance", ['PRINT GRID.BTC.BALANCE$']),
        ("BTC-ADDRESS", "GRID.BTC.ADDRESS$", "New address", ['PRINT GRID.BTC.ADDRESS$']),
        ("BTC-HELP", "GRID.BTC.HELP$", "BTC help text", ['PRINT GRID.BTC.HELP$']),
        ("BTC-STATUS", "GRID.BTC.STATUS$", "Bridge status", ['PRINT GRID.BTC.STATUS$']),
    ]
    for slug, syn, purpose, body in grid_fns:
        e.append(grid_entry(slug.lower(), syn, syn, purpose, f"Returns result of {syn}", body, fn=True))

    # Preprocessor
    e += [
        pp("if", "#IF expr", "#IF expr", "Conditional compile", "Includes following lines only if expr true",
           ["#IF GRIDOS7", "10 PRINT \"on Grid OS 7\"", "#ENDIF", "20 END"]),
        pp("else", "#ELSE", "#ELSE", "Preprocessor else", "Alternative branch for false #IF",
           ["#IF 0", "10 PRINT \"skip\"", "#ELSE", "10 PRINT \"kept\"", "#ENDIF", "20 END"]),
        pp("endif", "#ENDIF", "#ENDIF", "End #IF block", "Closes conditional preprocessor block",
           ["#IF 1", "10 PRINT \"yes\"", "#ENDIF", "20 END"]),
        pp("include", "#INCLUDE \"path\"", "#INCLUDE \"path\"", "Include file", "Inserts GFS file before parse",
           ["#INCLUDE \"/programs/hello.bas\""]),
    ]

    return e


def write_sample(entry: Entry) -> None:
    PROG_OUT.mkdir(parents=True, exist_ok=True)
    path = PROG_OUT / f"{entry.slug}.bas"
    text = "\n".join(entry.sample) + "\n"
    path.write_text(text, encoding="utf-8")


def render_entry_md(entry: Entry) -> str:
    sample_name = f"programs/encyclopedia/{entry.slug}.bas"
    sample_block = "\n".join(entry.sample)
    see = ", ".join(f"[{s}](../{s})" for s in entry.see_also) if entry.see_also else "—"
    return f"""## `{entry.title}`

| Field | Value |
|-------|-------|
| **Where** | {entry.where} |
| **Syntax** | `{entry.syntax}` |
| **Purpose** | {entry.purpose} |
| **Action** | {entry.actions} |
| **Sample** | `{sample_name}` |
| **See also** | {see} |

```basic
{sample_block}
```

---
"""


def write_wiki(entries: list[Entry]) -> None:
    WIKI_OUT.mkdir(parents=True, exist_ok=True)
    by_cat: dict[str, list[Entry]] = {}
    for ent in entries:
        by_cat.setdefault(ent.category, []).append(ent)

    cat_titles = {
        "editor": "01 — Editor keys",
        "ide": "02 — IDE colon commands",
        "shell": "03 — Flynn shell commands",
        "keyword": "04 — Statement keywords",
        "builtin": "05 — Built-in functions",
        "grid-stmt": "06 — GRID statements",
        "grid-fn": "07 — GRID functions",
        "preprocessor": "08 — Preprocessor directives",
    }

    index_lines = [
        "# GridBASIC IDE Encyclopedia (complete)",
        "",
        "Every Grid OS GridBASIC IDE command, shell command, language keyword, built-in,",
        "`GRID.*` binding, and preprocessor directive — with purpose, action, and sample program.",
        "",
        f"**Total entries:** {len(entries)}",
        "",
        "Sample programs live in `programs/encyclopedia/` on the Flynn disk after `make seed-disk`.",
        "",
        "## Volumes",
        "",
    ]

    for cat, title in cat_titles.items():
        if cat not in by_cat:
            continue
        fname = f"{cat.replace('_', '-')}.md" if cat not in ("grid-stmt", "grid-fn") else {
            "grid-stmt": "grid-statements.md",
            "grid-fn": "grid-functions.md",
        }[cat]
        if cat in ("grid-stmt", "grid-fn"):
            pass
        slug_map = {
            "editor": "editor-keys.md",
            "ide": "ide-commands.md",
            "shell": "shell-commands.md",
            "keyword": "keywords.md",
            "builtin": "builtins-ref.md",
            "grid-stmt": "grid-statements.md",
            "grid-fn": "grid-functions.md",
            "preprocessor": "preprocessor-ref.md",
        }
        fname = slug_map[cat]
        index_lines.append(f"- [{title}]({fname}) — {len(by_cat[cat])} entries")
        body = [f"# {title}", "", f"{len(by_cat[cat])} encyclopedia entries.", ""]
        for ent in by_cat[cat]:
            body.append(render_entry_md(ent))
        (WIKI_OUT / fname).write_text("\n".join(body), encoding="utf-8")

    index_lines += [
        "",
        "## Quick use",
        "",
        "```text",
        "grid> basic run /programs/encyclopedia/menu.bas",
        "grid> basic run /programs/encyclopedia/kw-print.bas",
        "Esc :help",
        "```",
        "",
        "Regenerate: `make gen-encyclopedia`",
        "",
    ]
    (WIKI_OUT / "README.md").write_text("\n".join(index_lines) + "\n", encoding="utf-8")


def write_menu(entries: list[Entry]) -> None:
    lines = [
        "10 REM GridBASIC encyclopedia sample index",
        "20 GRID.CLS",
        "30 PRINT \"=== GridBASIC Encyclopedia Samples ===\"",
        f"40 PRINT \"Total: {len(entries)} entries\"",
        "50 PRINT \"Path: /programs/encyclopedia/\"",
        "60 PRINT \"\"",
    ]
    n = 70
    for i, ent in enumerate(entries, start=1):
        if n > 9900:
            break
        lines.append(f'{n} PRINT "{i:03d} {ent.slug}.bas  {ent.title}"')
        n += 10
    lines += [f"{n} PRINT \"\"", f"{n + 10} END", ""]
    (PROG_OUT / "menu.bas").write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    entries = build_entries()
    if PROG_OUT.exists():
        for old in PROG_OUT.glob("*.bas"):
            old.unlink()
    PROG_OUT.mkdir(parents=True, exist_ok=True)
    for ent in entries:
        write_sample(ent)
    write_menu(entries)
    write_wiki(entries)
    print(f"Generated {len(entries)} encyclopedia entries")
    print(f"  wiki:   {WIKI_OUT}")
    print(f"  samples: {PROG_OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
