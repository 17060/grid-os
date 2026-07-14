#ifndef AI_H
#define AI_H

#include <stddef.h>

/* Host bridge (TCP 10.0.2.2:8766 or COM1 GRIDAI frames) with offline fallback. */
int ai_ask(const char *prompt, char *out, size_t cap);
int ai_explain(const char *line, char *out, size_t cap);
int ai_fix(const char *code, char *out, size_t cap);
int ai_complete(const char *code, char *out, size_t cap);
int ai_models(char *out, size_t cap);
/* AssimBASIC: run / chat against the host LLM (or offline fallback). */
int ai_run(const char *prompt, char *out, size_t cap);
int ai_chat(const char *prompt, char *out, size_t cap);

#endif
