# Statements & control flow

GridBASIC program keywords. Line numbers are optional but traditional.

Multiple statements on one line: separate with **`:`**

```basic
10 A=1: PRINT A: END
```

Assignment works with or without **`LET`**.

---

## Output & input

### `PRINT` / `?`

**Syntax:** `PRINT expr`; `PRINT expr; expr` · `PRINT , expr` (tab spacing)  
**Example:**

```basic
10 PRINT "hello"; 42
20 ? GRID.WHOAMI$
30 END
```

### `INPUT`

**Syntax:** `INPUT var` · `INPUT prompt$; var`  
**Example:**

```basic
10 INPUT "Name"; N$
20 PRINT "Hello, "; N$
30 END
```

### `LINE INPUT`

**Syntax:** `LINE INPUT s$`  
**Description:** Read full line including spaces.  
**Example:**

```basic
10 LINE INPUT MSG$
20 PRINT LEN(MSG$)
30 END
```

---

## Variables & data

### `LET`

**Syntax:** `LET x = expr` (optional)  
**Example:** `10 LET N = 42`

### `CONST`

**Syntax:** `CONST name = expr`  
**Example:**

```basic
10 CONST MAX=100
20 PRINT MAX
30 END
```

### `DIM`

**Syntax:** `DIM A(n)` · `DIM M(rows, cols)`  
**Example:**

```basic
10 DIM M(3,3)
20 M(2,3) = 23
30 PRINT M(2,3)
40 END
```

### `DATA` / `READ` / `RESTORE`

**Example:**

```basic
10 DATA 1, 2, 3
20 READ A, B, C
30 PRINT A + C
40 RESTORE
50 READ X
60 END
```

### `RANDOMIZE`

**Syntax:** `RANDOMIZE` · `RANDOMIZE seed`  
**Example:**

```basic
10 RANDOMIZE 9999
20 PRINT RND(100)
30 END
```

### `LOCAL` / `SHARED`

**Inside SUB/FUNCTION:** `LOCAL x` · `SHARED y` (declare shared in main first)  
**Example:** see `SUB` below

---

## Conditionals

### `IF` / `THEN` / `ELSE` / `ELSEIF`

**Example:**

```basic
10 IF N > 0 THEN PRINT "pos" ELSE PRINT "non-pos"
20 IF A=1 THEN PRINT "one" ELSEIF A=2 THEN PRINT "two" ELSE PRINT "other"
30 END
```

### `SELECT CASE` / `CASE` / `END SELECT`

**Example:**

```basic
10 SELECT CASE N
20 CASE 1
30   PRINT "one"
40 CASE 2, 3
50   PRINT "two or three"
60 CASE ELSE
70   PRINT "other"
80 END SELECT
90 END
```

Nested `SELECT` blocks are supported.

---

## Loops

### `FOR` / `TO` / `STEP` / `NEXT`

**Example:**

```basic
10 FOR I = 1 TO 5 STEP 1
20   PRINT I
30 NEXT I
40 END
```

### `WHILE` / `WEND`

**Example:**

```basic
10 N = 5
20 WHILE N > 0
30   PRINT N
40   N = N - 1
50 WEND
60 END
```

### `REPEAT` / `UNTIL`

**Example:**

```basic
10 N = 0
20 REPEAT
30   N = N + 1
40   PRINT N
50 UNTIL N >= 3
60 END
```

### `EXIT FOR` · `EXIT WHILE` · `CONTINUE FOR` · `CONTINUE WHILE`

**Example:**

```basic
10 FOR I = 1 TO 10
20   IF I = 5 THEN EXIT FOR
30   PRINT I
40 NEXT I
50 END
```

---

## Jumps & errors

### `GOTO` · `GOSUB` · `RETURN`

**Example:**

```basic
10 GOSUB 100
20 END
100 PRINT "subroutine"
110 RETURN
```

### `ON expr GOTO` · `ON expr GOSUB`

**Description:** 1-based branch table.  
**Example:**

```basic
10 ON N GOTO 100, 200, 300
100 PRINT "one": END
200 PRINT "two": END
300 PRINT "three": END
```

### `ON ERROR GOTO` · `RESUME`

**Example:**

```basic
10 ON ERROR GOTO 900
20 PRINT 1/0
30 END
900 PRINT ERR$
910 RESUME NEXT
920 END
```

---

## Procedures

### `DEF FN name(x)=expr`

**Example:**

```basic
10 DEF FN DBL(X)=X*2
20 PRINT DBL(21)
30 END
```

### `SUB` / `END SUB` · `FUNCTION` / `END FUNCTION` · `CALL`

**Example:**

```basic
10 SUB GREET(N$)
20   PRINT "Hello, "; N$
30 END SUB
40 FUNCTION DOUBLE(X)
50   DOUBLE = X * 2
60 END FUNCTION
70 CALL GREET("Flynn")
80 PRINT DOUBLE(21)
90 END
```

---

## Options

### `OPTION BASE 0|1`

**Description:** Array index origin (default 0).  
**Example:**

```basic
10 OPTION BASE 1
20 DIM A(5)
30 END
```

---

## Comments & termination

### `REM` / `'`

**Example:** `10 REM comment` · `20 ' also comment`

### `END` · `STOP`

**Description:** `END` normal exit; `STOP` halts program.

---

## Grid statements

Any identifier starting with **`GRID.`** is a Grid binding — see [GRID bindings](grid-bindings.md).

**Example:**

```basic
10 GRID.CLS
20 PRINT GRID.STATUS$
30 GRID.LOG "demo ran"
40 END
```

---

## See also

- [Built-ins & operators](builtins.md)
- [Preprocessor](preprocessor.md)
- [GRID bindings](grid-bindings.md)
