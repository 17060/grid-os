# 04 — Statement keywords

47 encyclopedia entries.

## `PRINT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `PRINT expr [; expr] ...` |
| **Purpose** | Output text and values |
| **Action** | Evaluates expressions and writes to console; ; suppresses spacing, , uses columns |
| **Sample** | `programs/encyclopedia/kw-print.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: PRINT
20 REM Where: GridBASIC program
30 REM Purpose: Output text and values
40 REM Action: Evaluates expressions and writes to console; ; suppresses spacing, , uses columns
50 PRINT "hello"; 42
60 PRINT GRID.WHOAMI$
70 END
```

---

## `?`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `? expr` |
| **Purpose** | PRINT shorthand |
| **Action** | Same as PRINT |
| **Sample** | `programs/encyclopedia/kw-qmark.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: ?
20 REM Where: GridBASIC program
30 REM Purpose: PRINT shorthand
40 REM Action: Same as PRINT
50 ? "quick print"
60 END
```

---

## `LET`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `LET x = expr` |
| **Purpose** | Assign variable |
| **Action** | Stores expression result in variable |
| **Sample** | `programs/encyclopedia/kw-let.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: LET
20 REM Where: GridBASIC program
30 REM Purpose: Assign variable
40 REM Action: Stores expression result in variable
50 LET N = 42
60 PRINT N
70 END
```

---

## `CONST`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `CONST name = expr` |
| **Purpose** | Declare constant |
| **Action** | Creates read-only binding |
| **Sample** | `programs/encyclopedia/kw-const.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: CONST
20 REM Where: GridBASIC program
30 REM Purpose: Declare constant
40 REM Action: Creates read-only binding
50 CONST MAX = 10
60 PRINT MAX
70 END
```

---

## `IF`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `IF cond THEN stmt [ELSE stmt]` |
| **Purpose** | Conditional branch |
| **Action** | Runs THEN block when condition true, optional ELSE when false |
| **Sample** | `programs/encyclopedia/kw-if.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: IF
20 REM Where: GridBASIC program
30 REM Purpose: Conditional branch
40 REM Action: Runs THEN block when condition true, optional ELSE when false
50 IF 1 THEN PRINT "yes" ELSE PRINT "no"
60 END
```

---

## `ELSEIF`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `ELSEIF cond THEN stmt` |
| **Purpose** | Chained condition |
| **Action** | Tests another condition after IF |
| **Sample** | `programs/encyclopedia/kw-elseif.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: ELSEIF
20 REM Where: GridBASIC program
30 REM Purpose: Chained condition
40 REM Action: Tests another condition after IF
50 IF A=1 THEN PRINT "a" ELSEIF A=2 THEN PRINT "b"
60 END
```

---

## `SELECT CASE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `SELECT CASE expr ... END SELECT` |
| **Purpose** | Multi-way branch |
| **Action** | Dispatches on expression value to matching CASE block |
| **Sample** | `programs/encyclopedia/kw-select.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: SELECT CASE
20 REM Where: GridBASIC program
30 REM Purpose: Multi-way branch
40 REM Action: Dispatches on expression value to matching CASE block
50 SELECT CASE 2
60 CASE 1
70   PRINT "one"
80 CASE 2
90   PRINT "two"
100 END SELECT
110 END
```

---

## `FOR`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `FOR i = a TO b [STEP s] ... NEXT` |
| **Purpose** | Counted loop |
| **Action** | Increments loop variable each iteration until past end bound |
| **Sample** | `programs/encyclopedia/kw-for.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: FOR
20 REM Where: GridBASIC program
30 REM Purpose: Counted loop
40 REM Action: Increments loop variable each iteration until past end bound
50 FOR I = 1 TO 3
60   PRINT I
70 NEXT I
80 END
```

---

## `STEP`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `FOR i = a TO b STEP s` |
| **Purpose** | FOR step size |
| **Action** | Sets increment for FOR loop |
| **Sample** | `programs/encyclopedia/kw-step.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: STEP
20 REM Where: GridBASIC program
30 REM Purpose: FOR step size
40 REM Action: Sets increment for FOR loop
50 FOR I = 0 TO 10 STEP 2
60   PRINT I
70 NEXT I
80 END
```

---

## `NEXT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `NEXT [var]` |
| **Purpose** | End FOR loop |
| **Action** | Advances loop; exits when past TO bound |
| **Sample** | `programs/encyclopedia/kw-next.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: NEXT
20 REM Where: GridBASIC program
30 REM Purpose: End FOR loop
40 REM Action: Advances loop; exits when past TO bound
50 FOR J = 1 TO 2
60 PRINT J
70 NEXT J
80 END
```

---

## `EXIT FOR`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `EXIT FOR` |
| **Purpose** | Leave FOR early |
| **Action** | Jumps to statement after NEXT |
| **Sample** | `programs/encyclopedia/kw-exit-for.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: EXIT FOR
20 REM Where: GridBASIC program
30 REM Purpose: Leave FOR early
40 REM Action: Jumps to statement after NEXT
50 FOR I = 1 TO 5
60 IF I = 3 THEN EXIT FOR
70 PRINT I
80 NEXT I
90 END
```

---

## `CONTINUE FOR`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `CONTINUE FOR` |
| **Purpose** | Skip to next FOR iter |
| **Action** | Jumps to NEXT without finishing body |
| **Sample** | `programs/encyclopedia/kw-continue-for.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: CONTINUE FOR
20 REM Where: GridBASIC program
30 REM Purpose: Skip to next FOR iter
40 REM Action: Jumps to NEXT without finishing body
50 FOR I = 1 TO 4
60 IF I = 2 THEN CONTINUE FOR
70 PRINT I
80 NEXT I
90 END
```

---

## `WHILE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `WHILE cond ... WEND` |
| **Purpose** | Pre-test loop |
| **Action** | Repeats while condition true |
| **Sample** | `programs/encyclopedia/kw-while.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: WHILE
20 REM Where: GridBASIC program
30 REM Purpose: Pre-test loop
40 REM Action: Repeats while condition true
50 N = 3
60 WHILE N > 0
70   PRINT N
80   N = N - 1
90 WEND
100 END
```

---

## `WEND`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `WEND` |
| **Purpose** | End WHILE loop |
| **Action** | Returns to WHILE condition test |
| **Sample** | `programs/encyclopedia/kw-wend.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: WEND
20 REM Where: GridBASIC program
30 REM Purpose: End WHILE loop
40 REM Action: Returns to WHILE condition test
50 X = 1
60 WHILE X < 3
70 PRINT X
80 X = X + 1
90 WEND
100 END
```

---

## `EXIT WHILE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `EXIT WHILE` |
| **Purpose** | Leave WHILE early |
| **Action** | Exits to statement after WEND |
| **Sample** | `programs/encyclopedia/kw-exit-while.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: EXIT WHILE
20 REM Where: GridBASIC program
30 REM Purpose: Leave WHILE early
40 REM Action: Exits to statement after WEND
50 WHILE 1
60 PRINT "once"
70 EXIT WHILE
80 WEND
90 END
```

---

## `REPEAT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `REPEAT ... UNTIL cond` |
| **Purpose** | Post-test loop |
| **Action** | Runs body at least once |
| **Sample** | `programs/encyclopedia/kw-repeat.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: REPEAT
20 REM Where: GridBASIC program
30 REM Purpose: Post-test loop
40 REM Action: Runs body at least once
50 REPEAT
60   PRINT "loop"
70 UNTIL 1
80 END
```

---

## `UNTIL`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `UNTIL expr` |
| **Purpose** | End REPEAT loop |
| **Action** | Tests exit condition after body |
| **Sample** | `programs/encyclopedia/kw-until.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: UNTIL
20 REM Where: GridBASIC program
30 REM Purpose: End REPEAT loop
40 REM Action: Tests exit condition after body
50 N = 0
60 REPEAT
70 N = N + 1
80 UNTIL N >= 2
90 END
```

---

## `GOTO`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `GOTO line` |
| **Purpose** | Unconditional jump |
| **Action** | Transfers control to line number |
| **Sample** | `programs/encyclopedia/kw-goto.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: GOTO
20 REM Where: GridBASIC program
30 REM Purpose: Unconditional jump
40 REM Action: Transfers control to line number
50 GOTO 50
60 PRINT "skip"
70 50 PRINT "landed"
80 END
```

---

## `GOSUB`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `GOSUB line` |
| **Purpose** | Call subroutine |
| **Action** | Pushes return address and jumps |
| **Sample** | `programs/encyclopedia/kw-gosub.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: GOSUB
20 REM Where: GridBASIC program
30 REM Purpose: Call subroutine
40 REM Action: Pushes return address and jumps
50 GOSUB 100
60 END
70 100 PRINT "sub"
80 RETURN
90 END
```

---

## `RETURN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `RETURN` |
| **Purpose** | Return from GOSUB |
| **Action** | Pops return address and resumes |
| **Sample** | `programs/encyclopedia/kw-return.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: RETURN
20 REM Where: GridBASIC program
30 REM Purpose: Return from GOSUB
40 REM Action: Pops return address and resumes
50 GOSUB 100
60 END
70 100 PRINT "back soon"
80 RETURN
90 END
```

---

## `ON GOTO`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `ON n GOTO a,b,c` |
| **Purpose** | Branch table jump |
| **Action** | 1-based index selects GOTO target |
| **Sample** | `programs/encyclopedia/kw-on-goto.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: ON GOTO
20 REM Where: GridBASIC program
30 REM Purpose: Branch table jump
40 REM Action: 1-based index selects GOTO target
50 ON 2 GOTO 100,200
60 100 PRINT "one"
70 200 PRINT "two"
80 END
```

---

## `ON ERROR GOTO`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `ON ERROR GOTO line` |
| **Purpose** | Error handler |
| **Action** | Jumps to line on runtime error |
| **Sample** | `programs/encyclopedia/kw-on-error.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: ON ERROR GOTO
20 REM Where: GridBASIC program
30 REM Purpose: Error handler
40 REM Action: Jumps to line on runtime error
50 ON ERROR GOTO 900
60 X = 1/0
70 900 PRINT ERR$
80 END
90 END
```

---

## `RESUME`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `RESUME [NEXT|line]` |
| **Purpose** | Resume after error |
| **Action** | Continues after error handler |
| **Sample** | `programs/encyclopedia/kw-resume.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: RESUME
20 REM Where: GridBASIC program
30 REM Purpose: Resume after error
40 REM Action: Continues after error handler
50 ON ERROR GOTO 100
60 X=1/0
70 100 PRINT "err"
80 RESUME NEXT
90 END
```

---

## `DEF FN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `DEF FN f(x)=expr` |
| **Purpose** | Single-line function |
| **Action** | Defines inline numeric function |
| **Sample** | `programs/encyclopedia/kw-def-fn.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: DEF FN
20 REM Where: GridBASIC program
30 REM Purpose: Single-line function
40 REM Action: Defines inline numeric function
50 DEF FN SQ(X) = X * X
60 PRINT FN SQ(5)
70 END
```

---

## `SUB`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `SUB name ... END SUB` |
| **Purpose** | Procedure |
| **Action** | Defines callable subroutine with CALL |
| **Sample** | `programs/encyclopedia/kw-sub.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: SUB
20 REM Where: GridBASIC program
30 REM Purpose: Procedure
40 REM Action: Defines callable subroutine with CALL
50 SUB HI(N$)
60   PRINT "Hi "; N$
70 END SUB
80 CALL HI("grid")
90 END
```

---

## `FUNCTION`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `FUNCTION f ... END FUNCTION` |
| **Purpose** | Function procedure |
| **Action** | Returns value via name assignment |
| **Sample** | `programs/encyclopedia/kw-function.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: FUNCTION
20 REM Where: GridBASIC program
30 REM Purpose: Function procedure
40 REM Action: Returns value via name assignment
50 FUNCTION DOUBLE(X)
60   DOUBLE = X * 2
70 END FUNCTION
80 PRINT DOUBLE(21)
90 END
```

---

## `CALL`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `CALL name(args)` |
| **Purpose** | Invoke SUB |
| **Action** | Calls defined subroutine |
| **Sample** | `programs/encyclopedia/kw-call.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: CALL
20 REM Where: GridBASIC program
30 REM Purpose: Invoke SUB
40 REM Action: Calls defined subroutine
50 SUB P()
60   PRINT "ok"
70 END SUB
80 CALL P()
90 END
```

---

## `LOCAL`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `LOCAL x` |
| **Purpose** | Local variable |
| **Action** | Declares variable local to SUB/FUNCTION |
| **Sample** | `programs/encyclopedia/kw-local.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: LOCAL
20 REM Where: GridBASIC program
30 REM Purpose: Local variable
40 REM Action: Declares variable local to SUB/FUNCTION
50 SUB T()
60   LOCAL X
70   X = 1
80   PRINT X
90 END SUB
100 CALL T()
110 END
```

---

## `SHARED`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `SHARED x` |
| **Purpose** | Shared variable |
| **Action** | Marks module-level shared in SUB |
| **Sample** | `programs/encyclopedia/kw-shared.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: SHARED
20 REM Where: GridBASIC program
30 REM Purpose: Shared variable
40 REM Action: Marks module-level shared in SUB
50 SHARED C
60 SUB B()
70   C = 1
80 END SUB
90 CALL B()
100 PRINT C
110 END
```

---

## `OPTION BASE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `OPTION BASE 0|1` |
| **Purpose** | Array origin |
| **Action** | Sets default array lower bound |
| **Sample** | `programs/encyclopedia/kw-option-base.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: OPTION BASE
20 REM Where: GridBASIC program
30 REM Purpose: Array origin
40 REM Action: Sets default array lower bound
50 OPTION BASE 1
60 DIM A(3)
70 A(1) = 9
80 PRINT A(1)
90 END
```

---

## `INPUT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `INPUT [prompt$;] var` |
| **Purpose** | Read input |
| **Action** | Reads user input into variable |
| **Sample** | `programs/encyclopedia/kw-input.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: INPUT
20 REM Where: GridBASIC program
30 REM Purpose: Read input
40 REM Action: Reads user input into variable
50 INPUT "Enter"; N
60 PRINT N
70 END
```

---

## `LINE INPUT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `LINE INPUT s$` |
| **Purpose** | Read full line |
| **Action** | Reads entire line including spaces |
| **Sample** | `programs/encyclopedia/kw-line-input.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: LINE INPUT
20 REM Where: GridBASIC program
30 REM Purpose: Read full line
40 REM Action: Reads entire line including spaces
50 LINE INPUT MSG$
60 PRINT LEN(MSG$)
70 END
```

---

## `DIM`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `DIM A(n) or DIM M(r,c)` |
| **Purpose** | Declare array |
| **Action** | Allocates numeric array storage |
| **Sample** | `programs/encyclopedia/kw-dim.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: DIM
20 REM Where: GridBASIC program
30 REM Purpose: Declare array
40 REM Action: Allocates numeric array storage
50 DIM A(2)
60 A(0) = 7
70 PRINT A(0)
80 END
```

---

## `DATA`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `DATA v1, v2, ...` |
| **Purpose** | Static literals |
| **Action** | Declares DATA values for READ |
| **Sample** | `programs/encyclopedia/kw-data.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: DATA
20 REM Where: GridBASIC program
30 REM Purpose: Static literals
40 REM Action: Declares DATA values for READ
50 DATA 10, 20
60 READ A, B
70 PRINT A + B
80 END
```

---

## `READ`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `READ var [, var ...]` |
| **Purpose** | Read DATA |
| **Action** | Reads next values from DATA pool |
| **Sample** | `programs/encyclopedia/kw-read.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: READ
20 REM Where: GridBASIC program
30 REM Purpose: Read DATA
40 REM Action: Reads next values from DATA pool
50 DATA 3, 4
60 READ X, Y
70 PRINT X * Y
80 END
```

---

## `RESTORE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `RESTORE` |
| **Purpose** | Reset DATA pointer |
| **Action** | Rewinds READ to first DATA |
| **Sample** | `programs/encyclopedia/kw-restore.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: RESTORE
20 REM Where: GridBASIC program
30 REM Purpose: Reset DATA pointer
40 REM Action: Rewinds READ to first DATA
50 DATA 1
60 READ A
70 RESTORE
80 READ B
90 PRINT A; B
100 END
```

---

## `RANDOMIZE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `RANDOMIZE [seed]` |
| **Purpose** | Seed PRNG |
| **Action** | Seeds RND/GRID.RND generator |
| **Sample** | `programs/encyclopedia/kw-randomize.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: RANDOMIZE
20 REM Where: GridBASIC program
30 REM Purpose: Seed PRNG
40 REM Action: Seeds RND/GRID.RND generator
50 RANDOMIZE 42
60 PRINT RND(100)
70 END
```

---

## `REM`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `REM comment` |
| **Purpose** | Comment to EOL |
| **Action** | Ignored text until end of line |
| **Sample** | `programs/encyclopedia/kw-rem.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: REM
20 REM Where: GridBASIC program
30 REM Purpose: Comment to EOL
40 REM Action: Ignored text until end of line
50 REM this is a comment
60 PRINT "ok"
70 END
```

---

## `'`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `' comment` |
| **Purpose** | Apostrophe comment |
| **Action** | Alternate comment form to end of line |
| **Sample** | `programs/encyclopedia/kw-tick.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: '
20 REM Where: GridBASIC program
30 REM Purpose: Apostrophe comment
40 REM Action: Alternate comment form to end of line
50 ' apostrophe comment
60 PRINT 1
70 END
```

---

## `END`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `END` |
| **Purpose** | End program |
| **Action** | Stops execution immediately |
| **Sample** | `programs/encyclopedia/kw-end.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: END
20 REM Where: GridBASIC program
30 REM Purpose: End program
40 REM Action: Stops execution immediately
50 PRINT "done"
60 END
70 PRINT "never"
80 END
```

---

## `STOP`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `STOP` |
| **Purpose** | Break program |
| **Action** | Halts with break message |
| **Sample** | `programs/encyclopedia/kw-stop.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: STOP
20 REM Where: GridBASIC program
30 REM Purpose: Break program
40 REM Action: Halts with break message
50 PRINT "halt"
60 STOP
70 END
```

---

## `:`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `stmt : stmt` |
| **Purpose** | Statement separator |
| **Action** | Runs multiple statements on one line |
| **Sample** | `programs/encyclopedia/kw-colon.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: :
20 REM Where: GridBASIC program
30 REM Purpose: Statement separator
40 REM Action: Runs multiple statements on one line
50 A = 1: B = 2: PRINT A + B
60 END
```

---

## `AND`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `AND` |
| **Purpose** | Logical conjunction |
| **Action** | True if both operands true |
| **Sample** | `programs/encyclopedia/kw-and.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: AND
20 REM Where: GridBASIC program
30 REM Purpose: Logical conjunction
40 REM Action: True if both operands true
50 IF 1 AND 1 THEN PRINT "and"
60 END
```

---

## `OR`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `OR` |
| **Purpose** | Logical disjunction |
| **Action** | True if either operand true |
| **Sample** | `programs/encyclopedia/kw-or.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: OR
20 REM Where: GridBASIC program
30 REM Purpose: Logical disjunction
40 REM Action: True if either operand true
50 IF 0 OR 1 THEN PRINT "or"
60 END
```

---

## `NOT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `NOT` |
| **Purpose** | Logical negation |
| **Action** | Inverts boolean value |
| **Sample** | `programs/encyclopedia/kw-not.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: NOT
20 REM Where: GridBASIC program
30 REM Purpose: Logical negation
40 REM Action: Inverts boolean value
50 IF NOT 0 THEN PRINT "not"
60 END
```

---

## `MOD`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `MOD` |
| **Purpose** | Modulo operator |
| **Action** | Remainder after division |
| **Sample** | `programs/encyclopedia/kw-mod.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: MOD
20 REM Where: GridBASIC program
30 REM Purpose: Modulo operator
40 REM Action: Remainder after division
50 PRINT 10 MOD 3
60 END
```

---

## `DIV`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program source |
| **Syntax** | `DIV` |
| **Purpose** | Integer division |
| **Action** | Quotient truncated toward zero |
| **Sample** | `programs/encyclopedia/kw-div.bas` |
| **See also** | [statements.md](../statements.md) |

```basic
10 REM Encyclopedia: DIV
20 REM Where: GridBASIC program
30 REM Purpose: Integer division
40 REM Action: Quotient truncated toward zero
50 PRINT 10 DIV 3
60 END
```

---
