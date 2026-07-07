# Preprocessor

GridBASIC runs a **preprocessor** before interpret or compile. Directives start with **`#`** at the beginning of a line (after optional whitespace).

---

## `#IF expr`

**Description:** If `expr` is false, following lines are excluded until `#ELSE` or `#ENDIF`.  
**Recognized true values:** `1`, `TRUE`, `true`, `GRIDOS`, `GRIDOS7`, any non-zero leading digit.

**Example:**

```basic
#IF GRIDOS7
10 PRINT "Running on Grid OS 7"
#ELSE
10 PRINT "Other environment"
#ENDIF
20 END
```

---

## `#ELSE`

**Description:** Alternative branch for the innermost `#IF`.

---

## `#ENDIF`

**Description:** End conditional block.

---

## `#INCLUDE "path"`

**Description:** Insert contents of a GFS file before parsing. Path is Flynn disk path (e.g. `/programs/shared.bas`). Nested includes up to depth 4.

**Example — `/programs/config.bas`:**

```basic
10 CONST APP$ = "Flynn IDE"
```

**Main program:**

```basic
#INCLUDE "/programs/config.bas"
10 PRINT APP$
20 END
```

---

## Full sample

```basic
#IF 1
10 REM included block
20 PRINT "GridBASIC preprocessor"
#ELSE
10 PRINT "skipped"
#ENDIF
30 END
```

---

## See also

- [Statements](statements.md)
- `basic compile` / `:compile` — preprocessor runs before bytecode generation
