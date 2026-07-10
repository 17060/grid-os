# 08 — Preprocessor directives

4 encyclopedia entries.

## `#IF expr`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC source (# directive at line start) |
| **Syntax** | `#IF expr` |
| **Purpose** | Conditional compile |
| **Action** | Includes following lines only if expr true |
| **Sample** | `programs/encyclopedia/pp-if.bas` |
| **See also** | [preprocessor.md](../preprocessor.md) |

```basic
#IF GRIDOS7
10 PRINT "on Grid OS 7"
#ENDIF
20 END
```

---

## `#ELSE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC source (# directive at line start) |
| **Syntax** | `#ELSE` |
| **Purpose** | Preprocessor else |
| **Action** | Alternative branch for false #IF |
| **Sample** | `programs/encyclopedia/pp-else.bas` |
| **See also** | [preprocessor.md](../preprocessor.md) |

```basic
#IF 0
10 PRINT "skip"
#ELSE
10 PRINT "kept"
#ENDIF
20 END
```

---

## `#ENDIF`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC source (# directive at line start) |
| **Syntax** | `#ENDIF` |
| **Purpose** | End #IF block |
| **Action** | Closes conditional preprocessor block |
| **Sample** | `programs/encyclopedia/pp-endif.bas` |
| **See also** | [preprocessor.md](../preprocessor.md) |

```basic
#IF 1
10 PRINT "yes"
#ENDIF
20 END
```

---

## `#INCLUDE "path"`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC source (# directive at line start) |
| **Syntax** | `#INCLUDE "path"` |
| **Purpose** | Include file |
| **Action** | Inserts GFS file before parse |
| **Sample** | `programs/encyclopedia/pp-include.bas` |
| **See also** | [preprocessor.md](../preprocessor.md) |

```basic
#INCLUDE "/programs/hello.bas"
```

---
