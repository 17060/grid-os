# 05 — Built-in functions

27 encyclopedia entries.

## `ABS`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `ABS(x)` |
| **Purpose** | Absolute value |
| **Action** | Evaluates ABS and returns result |
| **Sample** | `programs/encyclopedia/fn-abs.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: ABS
20 REM Where: GridBASIC expression
30 REM Purpose: Absolute value
40 REM Action: Evaluates ABS and returns result
50 PRINT ABS(-7)
60 END
```

---

## `INT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `INT(x)` |
| **Purpose** | Floor toward -inf |
| **Action** | Evaluates INT and returns result |
| **Sample** | `programs/encyclopedia/fn-int.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: INT
20 REM Where: GridBASIC expression
30 REM Purpose: Floor toward -inf
40 REM Action: Evaluates INT and returns result
50 PRINT INT(3.9)
60 END
```

---

## `SGN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `SGN(x)` |
| **Purpose** | Sign -1/0/1 |
| **Action** | Evaluates SGN and returns result |
| **Sample** | `programs/encyclopedia/fn-sgn.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: SGN
20 REM Where: GridBASIC expression
30 REM Purpose: Sign -1/0/1
40 REM Action: Evaluates SGN and returns result
50 PRINT SGN(-5)
60 END
```

---

## `SQR`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `SQR(x)` |
| **Purpose** | Square root |
| **Action** | Evaluates SQR and returns result |
| **Sample** | `programs/encyclopedia/fn-sqr.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: SQR
20 REM Where: GridBASIC expression
30 REM Purpose: Square root
40 REM Action: Evaluates SQR and returns result
50 PRINT SQR(16)
60 END
```

---

## `RND`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `RND(n)` |
| **Purpose** | Random 0..n-1 |
| **Action** | Evaluates RND and returns result |
| **Sample** | `programs/encyclopedia/fn-rnd.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: RND
20 REM Where: GridBASIC expression
30 REM Purpose: Random 0..n-1
40 REM Action: Evaluates RND and returns result
50 RANDOMIZE 1
60 PRINT RND(10)
70 END
```

---

## `PI`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `PI` |
| **Purpose** | Pi constant |
| **Action** | Evaluates PI and returns result |
| **Sample** | `programs/encyclopedia/fn-pi.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: PI
20 REM Where: GridBASIC expression
30 REM Purpose: Pi constant
40 REM Action: Evaluates PI and returns result
50 PRINT PI
60 END
```

---

## `MIN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `MIN(a,b,...)` |
| **Purpose** | Minimum value |
| **Action** | Evaluates MIN and returns result |
| **Sample** | `programs/encyclopedia/fn-min.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: MIN
20 REM Where: GridBASIC expression
30 REM Purpose: Minimum value
40 REM Action: Evaluates MIN and returns result
50 PRINT MIN(3, 9)
60 END
```

---

## `MAX`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `MAX(a,b,...)` |
| **Purpose** | Maximum value |
| **Action** | Evaluates MAX and returns result |
| **Sample** | `programs/encyclopedia/fn-max.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: MAX
20 REM Where: GridBASIC expression
30 REM Purpose: Maximum value
40 REM Action: Evaluates MAX and returns result
50 PRINT MAX(3, 9)
60 END
```

---

## `FIX`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `FIX(x)` |
| **Purpose** | Truncate toward zero |
| **Action** | Evaluates FIX and returns result |
| **Sample** | `programs/encyclopedia/fn-fix.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: FIX
20 REM Where: GridBASIC expression
30 REM Purpose: Truncate toward zero
40 REM Action: Evaluates FIX and returns result
50 PRINT FIX(-3.7)
60 END
```

---

## `ROUND`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `ROUND(x)` |
| **Purpose** | Round to nearest |
| **Action** | Evaluates ROUND and returns result |
| **Sample** | `programs/encyclopedia/fn-round.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: ROUND
20 REM Where: GridBASIC expression
30 REM Purpose: Round to nearest
40 REM Action: Evaluates ROUND and returns result
50 PRINT ROUND(2.6)
60 END
```

---

## `LEN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `LEN(s$)` |
| **Purpose** | String length |
| **Action** | Evaluates LEN and returns result |
| **Sample** | `programs/encyclopedia/fn-len.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: LEN
20 REM Where: GridBASIC expression
30 REM Purpose: String length
40 REM Action: Evaluates LEN and returns result
50 PRINT LEN("grid")
60 END
```

---

## `VAL`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `VAL(s$)` |
| **Purpose** | Parse number from string |
| **Action** | Evaluates VAL and returns result |
| **Sample** | `programs/encyclopedia/fn-val.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: VAL
20 REM Where: GridBASIC expression
30 REM Purpose: Parse number from string
40 REM Action: Evaluates VAL and returns result
50 PRINT VAL("42")
60 END
```

---

## `ASC`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `ASC(s$)` |
| **Purpose** | First char code |
| **Action** | Evaluates ASC and returns result |
| **Sample** | `programs/encyclopedia/fn-asc.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: ASC
20 REM Where: GridBASIC expression
30 REM Purpose: First char code
40 REM Action: Evaluates ASC and returns result
50 PRINT ASC("A")
60 END
```

---

## `CHR$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `CHR$(n)` |
| **Purpose** | Char from code |
| **Action** | Evaluates CHR$ and returns result |
| **Sample** | `programs/encyclopedia/fn-chrs.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: CHR$
20 REM Where: GridBASIC expression
30 REM Purpose: Char from code
40 REM Action: Evaluates CHR$ and returns result
50 PRINT CHR$(65)
60 END
```

---

## `STR$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `STR$(n)` |
| **Purpose** | Number as string |
| **Action** | Evaluates STR$ and returns result |
| **Sample** | `programs/encyclopedia/fn-strs.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: STR$
20 REM Where: GridBASIC expression
30 REM Purpose: Number as string
40 REM Action: Evaluates STR$ and returns result
50 PRINT STR$(99)
60 END
```

---

## `UPPER$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `UPPER$(s$)` |
| **Purpose** | Uppercase string |
| **Action** | Evaluates UPPER$ and returns result |
| **Sample** | `programs/encyclopedia/fn-uppers.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: UPPER$
20 REM Where: GridBASIC expression
30 REM Purpose: Uppercase string
40 REM Action: Evaluates UPPER$ and returns result
50 PRINT UPPER$("grid")
60 END
```

---

## `LOWER$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `LOWER$(s$)` |
| **Purpose** | Lowercase string |
| **Action** | Evaluates LOWER$ and returns result |
| **Sample** | `programs/encyclopedia/fn-lowers.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: LOWER$
20 REM Where: GridBASIC expression
30 REM Purpose: Lowercase string
40 REM Action: Evaluates LOWER$ and returns result
50 PRINT LOWER$("GRID")
60 END
```

---

## `LEFT$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `LEFT$(s$, n)` |
| **Purpose** | Left substring |
| **Action** | Evaluates LEFT$ and returns result |
| **Sample** | `programs/encyclopedia/fn-lefts.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: LEFT$
20 REM Where: GridBASIC expression
30 REM Purpose: Left substring
40 REM Action: Evaluates LEFT$ and returns result
50 PRINT LEFT$("hello", 2)
60 END
```

---

## `RIGHT$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `RIGHT$(s$, n)` |
| **Purpose** | Right substring |
| **Action** | Evaluates RIGHT$ and returns result |
| **Sample** | `programs/encyclopedia/fn-rights.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: RIGHT$
20 REM Where: GridBASIC expression
30 REM Purpose: Right substring
40 REM Action: Evaluates RIGHT$ and returns result
50 PRINT RIGHT$("hello", 2)
60 END
```

---

## `MID$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `MID$(s$, start, len)` |
| **Purpose** | Middle substring |
| **Action** | Evaluates MID$ and returns result |
| **Sample** | `programs/encyclopedia/fn-mids.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: MID$
20 REM Where: GridBASIC expression
30 REM Purpose: Middle substring
40 REM Action: Evaluates MID$ and returns result
50 PRINT MID$("hello", 2, 3)
60 END
```

---

## `INSTR$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `INSTR$(hay$, needle$)` |
| **Purpose** | Find substring |
| **Action** | Evaluates INSTR$ and returns result |
| **Sample** | `programs/encyclopedia/fn-instrs.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: INSTR$
20 REM Where: GridBASIC expression
30 REM Purpose: Find substring
40 REM Action: Evaluates INSTR$ and returns result
50 PRINT INSTR$("Grid OS", "OS")
60 END
```

---

## `TRIM$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `TRIM$(s$)` |
| **Purpose** | Trim spaces |
| **Action** | Evaluates TRIM$ and returns result |
| **Sample** | `programs/encyclopedia/fn-trims.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: TRIM$
20 REM Where: GridBASIC expression
30 REM Purpose: Trim spaces
40 REM Action: Evaluates TRIM$ and returns result
50 PRINT LEN(TRIM$("  x  "))
60 END
```

---

## `LTRIM$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `LTRIM$(s$)` |
| **Purpose** | Trim left |
| **Action** | Evaluates LTRIM$ and returns result |
| **Sample** | `programs/encyclopedia/fn-ltrims.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: LTRIM$
20 REM Where: GridBASIC expression
30 REM Purpose: Trim left
40 REM Action: Evaluates LTRIM$ and returns result
50 PRINT LTRIM$("  hi")
60 END
```

---

## `RTRIM$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `RTRIM$(s$)` |
| **Purpose** | Trim right |
| **Action** | Evaluates RTRIM$ and returns result |
| **Sample** | `programs/encyclopedia/fn-rtrims.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: RTRIM$
20 REM Where: GridBASIC expression
30 REM Purpose: Trim right
40 REM Action: Evaluates RTRIM$ and returns result
50 PRINT RTRIM$("hi  ")
60 END
```

---

## `SPACE$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `SPACE$(n)` |
| **Purpose** | N spaces |
| **Action** | Evaluates SPACE$ and returns result |
| **Sample** | `programs/encyclopedia/fn-spaces.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: SPACE$
20 REM Where: GridBASIC expression
30 REM Purpose: N spaces
40 REM Action: Evaluates SPACE$ and returns result
50 PRINT "["; SPACE$(3); "]"
60 END
```

---

## `STRING$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `STRING$(n, c$)` |
| **Purpose** | Repeat char |
| **Action** | Evaluates STRING$ and returns result |
| **Sample** | `programs/encyclopedia/fn-strings.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: STRING$
20 REM Where: GridBASIC expression
30 REM Purpose: Repeat char
40 REM Action: Evaluates STRING$ and returns result
50 PRINT STRING$(5, "*")
60 END
```

---

## `ERR$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC expression |
| **Syntax** | `ERR$` |
| **Purpose** | Last error message |
| **Action** | Evaluates ERR$ and returns result |
| **Sample** | `programs/encyclopedia/fn-errs.bas` |
| **See also** | [builtins.md](../builtins.md) |

```basic
10 REM Encyclopedia: ERR$
20 REM Where: GridBASIC expression
30 REM Purpose: Last error message
40 REM Action: Evaluates ERR$ and returns result
50 ON ERROR GOTO 100
60 X=1/0
70 100 PRINT ERR$
80 END
```

---
