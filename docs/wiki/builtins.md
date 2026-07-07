# Built-ins & operators

Used inside expressions in `PRINT`, `IF`, assignments, etc.

---

## Math functions

| Name | Syntax | Example |
|------|--------|---------|
| `ABS` | `ABS(x)` | `PRINT ABS(-5)` → 5 |
| `INT` | `INT(x)` | `PRINT INT(3.7)` → 3 |
| `SGN` | `SGN(x)` | `PRINT SGN(-1)` → -1 |
| `SQR` | `SQR(x)` | `PRINT SQR(16)` → 4 |
| `RND` | `RND(n)` | `PRINT RND(100)` → 0…99 |
| `PI` | `PI` | `PRINT PI` |
| `MIN` | `MIN(a,b)` | `PRINT MIN(3,7)` → 3 |
| `MAX` | `MAX(a,b)` | `PRINT MAX(3,7)` → 7 |
| `FIX` | `FIX(x)` | Truncate toward zero |
| `ROUND` | `ROUND(x)` | Round to nearest integer |

**Sample:**

```basic
10 PRINT SQR(2), PI
20 PRINT MIN(10, 20), MAX(10, 20)
30 END
```

---

## String functions

| Name | Syntax | Notes |
|------|--------|-------|
| `LEN` | `LEN(s$)` | String length |
| `VAL` | `VAL(s$)` | Parse number from string |
| `ASC` | `ASC(s$)` | First character code |
| `CHR$` | `CHR$(n)` | Character from code |
| `STR$` | `STR$(n)` | Number as string |
| `UPPER$` | `UPPER$(s$)` | Uppercase |
| `LOWER$` | `LOWER$(s$)` | Lowercase |
| `LEFT$` | `LEFT$(s$, n)` | First n chars |
| `RIGHT$` | `RIGHT$(s$, n)` | Last n chars |
| `MID$` | `MID$(s$, start, len)` | Substring |
| `INSTR$` | `INSTR$(hay$, needle$ [, start])` | Find substring (0 if none) |
| `TRIM$` | `TRIM$(s$)` | Trim spaces |
| `LTRIM$` | `LTRIM$(s$)` | Trim left |
| `RTRIM$` | `RTRIM$(s$)` | Trim right |
| `SPACE$` | `SPACE$(n)` | n spaces |
| `STRING$` | `STRING$(n, c$)` | Repeat character |
| `ERR$` | `ERR$` | Last error message (after `ON ERROR`) |

**Sample:**

```basic
10 S$ = "  Flynn Grid  "
20 PRINT LEN(TRIM$(S$))
30 PRINT INSTR$("Grid OS", "OS")
40 PRINT UPPER$("grid")
50 END
```

---

## Expression keywords

| Keyword | Use | Example |
|---------|-----|---------|
| `AND` | Logical and | `IF A AND B THEN …` |
| `OR` | Logical or | `IF A OR B THEN …` |
| `NOT` | Logical not | `IF NOT FLAG THEN …` |
| `MOD` | Modulo | `PRINT 10 MOD 3` → 1 |
| `DIV` | Integer division | `PRINT 10 DIV 3` → 3 |

---

## Operators

| Type | Operators |
|------|-----------|
| Arithmetic | `+` `-` `*` `/` `^` |
| Comparison | `=` `<>` `#` `<` `>` `<=` `>=` |
| String concat | `+` (when either side is string) |

**Sample:**

```basic
10 A = 7 / 2
20 PRINT A
30 PRINT "hello " + "grid"
40 END
```

---

## User-defined functions

| Form | Example |
|------|---------|
| `DEF FN f(x)=expr` | `DEF FN SQ(X)=X*X` |
| `FUNCTION … END FUNCTION` | Named return via function name |
| `SUB … END SUB` + `CALL` | Procedures |

See [Statements](statements.md).

---

## See also

- [GRID bindings](grid-bindings.md) — `GRID.RND`, `GRID.TIME`, etc.
- [Statements](statements.md)
