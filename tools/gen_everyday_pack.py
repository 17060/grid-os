#!/usr/bin/env python3
"""Generate Flynn Everyday pack — games, apps, type-ins for GridBASIC (1980s home-computer feel)."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GAMES_DIR = ROOT / "programs" / "games"
APPS_DIR = ROOT / "programs" / "apps"
TYPEINS_DIR = ROOT / "programs" / "typeins"
MINE_DIR = ROOT / "programs" / "mine"
EVERYDAY_DIR = ROOT / "programs"


@dataclass
class Demo:
    filename: str
    title: str
    body: list[str]


def fmt_demo(demo: Demo) -> str:
    lines: list[str] = []
    n = 10
    lines.append(f"{n} REM {demo.title}")
    for stmt in demo.body:
        n += 10
        lines.append(f"{n} {stmt}")
    n += 10
    lines.append(f"{n} END")
    return "\n".join(lines) + "\n"


def write_demo(directory: Path, demo: Demo) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    (directory / demo.filename).write_text(fmt_demo(demo), encoding="utf-8")


def games() -> list[Demo]:
    return [
        Demo("g01-guess.bas", "g01 -- number guess", [
            'PRINT "=== Guess the Number (1-100) ==="',
            "RANDOMIZE GRID.TIME",
            "LET S = INT(RND(100)) + 1",
            "LET T = 0",
            "FOR R = 1 TO 8",
            '  PRINT "Guess? ";',
            '  INPUT G',
            "  LET T = T + 1",
            "  IF G = S THEN PRINT \"You win in \"; T; \" tries!\": END",
            "  IF G < S THEN PRINT \"Higher...\" ELSE PRINT \"Lower...\"",
            "NEXT R",
            'PRINT "The number was "; S',
        ]),
        Demo("g02-dice.bas", "g02 -- dice roller", [
            'PRINT "=== Flynn Dice ==="',
            "RANDOMIZE GRID.TIME",
            "FOR I = 1 TO 5",
            "  LET D = INT(RND(6)) + 1",
            '  PRINT "Roll "; I; ": "; D',
            "NEXT I",
        ]),
        Demo("g03-stars.bas", "g03 -- star banner", [
            'PRINT "=== Star Banner ==="',
            "FOR I = 1 TO 5",
            '  PRINT STRING$(I * 4, "*")',
            "NEXT I",
            'PRINT "=== END OF LINE ==="',
        ]),
        Demo("g04-plotdemo.bas", "g04 -- grid plot art", [
            'PRINT "=== GRID Plot Demo ==="',
            "GRID.CLS",
            "FOR X = 5 TO 75 STEP 5",
            "  GRID.PLOT X, 12, (X / 5) MOD 4",
            "NEXT X",
            "GRID.LINE 5, 12, 75, 12, 1",
            'PRINT "Plotted on the Flynn grid overlay"',
        ]),
        Demo("g05-beepscale.bas", "g05 -- beep scale", [
            'PRINT "=== Flynn Beep Scale ==="',
            "FOR N = 1 TO 8",
            "  GRID.BEEP N * 2",
            "  GRID.WAIT 3",
            "NEXT N",
            'PRINT "End of line."',
        ]),
        Demo("g06-counter.bas", "g06 -- fast counter", [
            'PRINT "=== Grid Counter ==="',
            "FOR I = 1 TO 20",
            '  PRINT "Tick "; I;',
            "  GRID.WAIT 1",
            "NEXT I",
            'PRINT ""',
            'PRINT "Done!"',
        ]),
        Demo("g07-fortune.bas", "g07 -- fortune cookie", [
            'PRINT "=== Flynn Fortune ==="',
            "RANDOMIZE GRID.TIME",
            "LET F = INT(RND(5)) + 1",
            "IF F = 1 THEN PRINT \"The Grid favors the bold.\"",
            "IF F = 2 THEN PRINT \"End of line — but a new cycle begins.\"",
            "IF F = 3 THEN PRINT \"Save your work. Flynn always does.\"",
            "IF F = 4 THEN PRINT \"Type :run and see what happens.\"",
            "IF F = 5 THEN PRINT \"Programs are spells. Cast wisely.\"",
        ]),
        Demo("g08-mathquiz.bas", "g08 -- math quiz", [
            'PRINT "=== Flynn Math Quiz ==="',
            "RANDOMIZE GRID.TIME",
            "LET A = INT(RND(12)) + 1",
            "LET B = INT(RND(12)) + 1",
            'PRINT "What is "; A; " + "; B; "?";',
            'INPUT ANS',
            "IF ANS = A + B THEN PRINT \"Correct!\" ELSE PRINT \"Answer: \"; A + B",
        ]),
        Demo("g09-target.bas", "g09 -- coordinate target", [
            'PRINT "=== Target Grid ==="',
            "RANDOMIZE GRID.TIME",
            "LET TX = INT(RND(70)) + 5",
            "LET TY = INT(RND(20)) + 3",
            "GRID.CLS",
            "GRID.PLOT TX, TY, 2",
            'PRINT "Target plotted at "; TX; ","; TY',
            'PRINT "Use GRID.PLOT in your own games!"',
        ]),
        Demo("g10-circle.bas", "g10 -- circle art", [
            'PRINT "=== Grid Circle ==="',
            "GRID.CLS",
            "GRID.CIRCLE 40, 12, 10, 3",
            "GRID.CIRCLE 40, 12, 5, 1",
            'PRINT "GRID.CIRCLE — Atari ST vibes on Flynn\'s Grid"',
        ]),
        Demo("g11-paint.bas", "g11 -- sine paint", [
            'PRINT "=== Sine Paint ==="',
            "GRID.CLS",
            "FOR X = 0 TO 79",
            "  LET Y = 12 + INT(SIN(X / 8) * 6)",
            "  GRID.PLOT X, Y, 2",
            "NEXT X",
            'PRINT "Wave complete."',
        ]),
        Demo("g12-race.bas", "g12 -- ascii race", [
            'PRINT "=== Flynn Race ==="',
            "FOR P = 1 TO 40",
            '  PRINT TAB(P); ">"',
            "  GRID.WAIT 1",
            "NEXT P",
            'PRINT "Winner!"',
        ]),
        Demo("g13-memory.bas", "g13 -- remember the number", [
            'PRINT "=== Memory Flash ==="',
            "RANDOMIZE GRID.TIME",
            "LET S = INT(RND(900)) + 100",
            'PRINT "Remember: "; S',
            "GRID.WAIT 8",
            "GRID.CLS",
            'PRINT "What was it? ";',
            'INPUT G',
            "IF G = S THEN PRINT \"Sharp program!\" ELSE PRINT \"It was \"; S",
        ]),
        Demo("g14-hilo.bas", "g14 -- hi-lo cards", [
            'PRINT "=== Hi-Lo ==="',
            "RANDOMIZE GRID.TIME",
            "LET C = INT(RND(13)) + 1",
            'PRINT "Card value: "; C',
            'PRINT "Next higher (1) or lower (0)? ";',
            'INPUT H',
            "LET N = INT(RND(13)) + 1",
            'PRINT "Next card: "; N',
            "IF H = 1 AND N > C THEN PRINT \"Win!\"",
            "IF H = 0 AND N < C THEN PRINT \"Win!\"",
        ]),
        Demo("g15-maze.bas", "g15 -- mini maze", [
            'PRINT "=== Mini Maze ==="',
            'PRINT "+--+--+--+--+"',
            'PRINT "|  |     |  |"',
            'PRINT "+--+  +--+--+"',
            'PRINT "|     ->   |"',
            'PRINT "+--+--+--+--+"',
            'PRINT "Find the arrow. Make your own with PRINT!"',
        ]),
        Demo("g16-invaders-lite.bas", "g16 -- invader row", [
            'PRINT "=== Invader Row ==="',
            'PRINT "<> <> <> <> <>"',
            "FOR S = 1 TO 10",
            '  PRINT TAB(S); "<> <> <>"',
            "  GRID.WAIT 2",
            "NEXT S",
        ]),
        Demo("g17-pong-lite.bas", "g17 -- pong bounce", [
            'PRINT "=== Pong Lite ==="',
            "LET B = 10",
            "FOR I = 1 TO 30",
            "  GRID.LOCATE 1, 20",
            '  PRINT TAB(B); "|"',
            "  LET B = B + 1",
            "  IF B > 70 THEN LET B = 5",
            "  GRID.WAIT 1",
            "NEXT I",
        ]),
        Demo("g18-melody.bas", "g18 -- short melody", [
            'PRINT "=== Flynn Melody ==="',
            "GRID.BEEP 4",
            "GRID.WAIT 2",
            "GRID.BEEP 6",
            "GRID.WAIT 2",
            "GRID.BEEP 8",
            "GRID.WAIT 2",
            "GRID.BEEP 6",
            "GRID.WAIT 2",
            "GRID.BEEP 4",
            'PRINT "Compose with GRID.BEEP n"',
        ]),
        Demo("g19-identity.bas", "g19 -- who on the grid", [
            'PRINT "=== Flynn Identity ==="',
            'PRINT GRID.WHOAMI$',
            "PRINT GRID.DISC.STATUS$",
            'PRINT "You are a User on Flynn\'s Grid."',
        ]),
        Demo("g20-graduation.bas", "g20 -- games graduation", [
            'PRINT "=== Flynn Games Pack ==="',
            'PRINT "You played the Grid. End of line."',
            "PRINT GRID.STATUS$",
            'PRINT "Share your .bas: Esc :publish mygame"',
        ]),
    ]


def apps() -> list[Demo]:
    return [
        Demo("a01-calc.bas", "a01 -- calculator", [
            'PRINT "=== Flynn Calc ==="',
            'PRINT "Enter A, B separated by comma in INPUT not supported — use fixed demo"',
            "LET A = 144",
            "LET B = 12",
            'PRINT A; " / "; B; " = "; A / B',
            'PRINT A; " * "; B; " = "; A * B',
        ]),
        Demo("a02-timer.bas", "a02 -- countdown", [
            'PRINT "=== Countdown ==="',
            "FOR T = 5 TO 1 STEP -1",
            '  PRINT "T-minus "; T',
            "  GRID.WAIT 5",
            "NEXT T",
            'PRINT "Launch!"',
        ]),
        Demo("a03-notes.bas", "a03 -- quick note", [
            'PRINT "=== Flynn Notes ==="',
            'GRID.VAULT.PUT "note", "Remember to save your programs!"',
            "GRID.VAULT.SYNC",
            'PRINT GRID.VAULT.GET$("note")',
        ]),
        Demo("a04-password.bas", "a04 -- password gen", [
            'PRINT "=== Password Gen ==="',
            "RANDOMIZE GRID.TIME",
            'PRINT "Pin: "; INT(RND(9000)) + 1000',
        ]),
        Demo("a05-convert.bas", "a05 -- celsius to f", [
            'PRINT "=== Temp Convert ==="',
            "LET C = 25",
            "LET F = C * 9 / 5 + 32",
            'PRINT C; " C = "; F; " F"',
        ]),
        Demo("a06-clock.bas", "a06 -- grid ticks", [
            'PRINT "=== Grid Clock ==="',
            "FOR I = 1 TO 5",
            '  PRINT "Ticks: "; GRID.TIME',
            "  GRID.WAIT 5",
            "NEXT I",
        ]),
        Demo("a07-journal.bas", "a07 -- journal line", [
            'PRINT "=== Journal ==="',
            'GRID.LOG "Flynn journal entry from GridBASIC"',
            "PRINT GRID.LOG.TAIL$(3)",
        ]),
        Demo("a08-catalog.bas", "a08 -- disk catalog", [
            'PRINT "=== Your Programs ==="',
            'PRINT GRID.GFS.LIST$("/programs/mine")',
            'PRINT "Save with Esc :publish name"',
        ]),
        Demo("a09-ping.bas", "a09 -- network ping", [
            'PRINT "=== Flynn Net ==="',
            'PRINT GRID.PING("gateway")',
            "PRINT GRID.NET.STATUS$",
        ]),
        Demo("a10-about.bas", "a10 -- about", [
            'PRINT "=== Flynn\'s Grid ==="',
            "PRINT GRID.STATUS$",
            'PRINT "GridBASIC — program the machine like 1980."',
            'PRINT "Esc :catalog :games :apps :help"',
        ]),
    ]


def typeins() -> list[Demo]:
    demos: list[Demo] = []
    snippets = [
        ("t01", "hello", ['PRINT "HELLO, FLYNN!"']),
        ("t02", "loop", ["FOR I = 1 TO 5", '  PRINT I', "NEXT I"]),
        ("t03", "if", ["LET X = 10", "IF X > 5 THEN PRINT \"BIG\""]),
        ("t04", "string", ['LET A$ = "Grid"', 'LET B$ = "BASIC"', 'PRINT A$ + " " + B$']),
        ("t05", "rnd", ["RANDOMIZE 42", 'PRINT "RND(10)="; RND(10)']),
        ("t06", "sub", ['SUB HI()', '  PRINT "Hi!"', "END SUB", "CALL HI()"]),
        ("t07", "plot1", ["GRID.PLOT 10, 10, 1", "GRID.PLOT 11, 10, 2"]),
        ("t08", "beep1", ["GRID.BEEP 5", "GRID.WAIT 3", "GRID.BEEP 8"]),
        ("t09", "cls", ['PRINT "Before"', "GRID.CLS", 'PRINT "After CLS"']),
        ("t10", "whoami", ['PRINT GRID.WHOAMI$']),
        ("t11", "time", ['PRINT GRID.TIME']),
        ("t12", "color", ["GRID.COLOR 11", 'PRINT "Cyan text"']),
        ("t13", "data", ["DATA 1,2,3", "READ A,B,C", "PRINT A+B+C"]),
        ("t14", "dim", ["DIM A(3)", "LET A(1)=9", "PRINT A(1)"]),
        ("t15", "while", ["LET N=0", "WHILE N < 3", "  LET N=N+1", "  PRINT N", "WEND"]),
        ("t16", "repeat", ["LET K=0", "REPEAT", "  LET K=K+1", "  PRINT K", "UNTIL K>=3"]),
        ("t17", "select", ["LET X=2", "SELECT CASE X", "CASE 2", '  PRINT "two"', "END SELECT"]),
        ("t18", "minmax", ['PRINT MIN(3,7); MAX(3,7)']),
        ("t19", "trim", ['PRINT TRIM$("  flynn  ")']),
        ("t20", "endline", ['PRINT "End of line."']),
    ]
    for slug, tag, body in snippets:
        demos.append(Demo(f"{slug}-{tag}.bas", f"{slug} -- type-in {tag}", body))
    return demos


def write_menu(directory: Path, title: str, demos: list[Demo], shell_cmd: str) -> None:
    vfs = f"/programs/{directory.name}"
    lines = [
        "10 REM " + title,
        "20 GRID.CLS",
        f'30 PRINT "=== {title} ==="',
        f'40 PRINT "Path: {vfs}/"',
        f'50 PRINT "Total: {len(demos)}"',
        '60 PRINT ""',
    ]
    n = 70
    for i, demo in enumerate(demos, start=1):
        short = demo.filename.replace(".bas", "")
        desc = demo.title.split(" -- ", 1)[-1][:36]
        lines.append(f'{n} PRINT "{i:03d} {short}  {desc}"')
        n += 10
    lines.append(f'{n} PRINT ""')
    n += 10
    lines.append(f'{n} PRINT "Shell: {shell_cmd}   IDE: Esc :load {directory.name}/<name>"')
    n += 10
    lines.append(f"{n} END")
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "menu.bas").write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_everyday_menu(g: list[Demo], a: list[Demo], t: list[Demo]) -> None:
    content = """10 REM Flynn Everyday catalog
20 GRID.CLS
30 PRINT "=== FLYNN EVERYDAY — GridBASIC like 1980 ==="
40 PRINT ""
50 PRINT "  games     20 arcade & puzzle programs"
60 PRINT "  apps      10 everyday utilities"
70 PRINT "  typeins   20 magazine type-in snippets"
80 PRINT "  mine      your published programs"
90 PRINT "  academy   security labs (advanced)"
100 PRINT ""
110 PRINT "IDE: Esc :catalog :games :apps :typeins"
120 PRINT "     :publish myprog  saves to /programs/mine/"
130 PRINT "Shell: catalog | games | tutorial | help"
140 PRINT ""
150 PRINT GRID.STATUS$
160 END
"""
    (EVERYDAY_DIR / "everyday.bas").write_text(content, encoding="utf-8")


def write_academy_menu() -> None:
    content = """10 REM Flynn Academy — advanced security labs
20 GRID.CLS
30 PRINT "=== Flynn Academy (QEMU lab only) ==="
40 PRINT "Shell: redteam blackhat whiteteam blueteam"
50 PRINT "       purpleteam greenteam yellowteam orangeteam"
60 PRINT "       greyteam daemonteam"
70 PRINT "IDE: Esc :redteam ... :daemonteam"
80 PRINT ""
90 PRINT "Everyday users: Esc :catalog :games :tutorial"
100 END
"""
    (EVERYDAY_DIR / "academy.bas").write_text(content, encoding="utf-8")


def write_mine_welcome() -> None:
    content = """10 REM Your programs live here
20 PRINT "=== /programs/mine/ ==="
30 PRINT "Publish from IDE: Esc :publish hello"
40 PRINT "Loads as: :load mine/hello"
50 PRINT GRID.GFS.LIST$("/programs/mine")
60 END
"""
    MINE_DIR.mkdir(parents=True, exist_ok=True)
    (MINE_DIR / "welcome.bas").write_text(content, encoding="utf-8")


def main() -> int:
    g = games()
    a = apps()
    t = typeins()
    assert len(g) == 20 and len(a) == 10 and len(t) == 20

    for d in (GAMES_DIR, APPS_DIR, TYPEINS_DIR, MINE_DIR):
        if d.exists():
            for old in d.glob("*.bas"):
                old.unlink()

    for demo in g:
        write_demo(GAMES_DIR, demo)
    for demo in a:
        write_demo(APPS_DIR, demo)
    for demo in t:
        write_demo(TYPEINS_DIR, demo)

    write_menu(GAMES_DIR, "Flynn Games Pack", g, "games")
    write_menu(APPS_DIR, "Flynn Apps Pack", a, "apps")
    write_menu(TYPEINS_DIR, "Flynn Type-In Library", t, "typeins")
    write_everyday_menu(g, a, t)
    write_academy_menu()
    write_mine_welcome()

    print(f"Generated {len(g)} games in {GAMES_DIR}")
    print(f"Generated {len(a)} apps in {APPS_DIR}")
    print(f"Generated {len(t)} type-ins in {TYPEINS_DIR}")
    print(f"Catalog: {EVERYDAY_DIR / 'everyday.bas'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
