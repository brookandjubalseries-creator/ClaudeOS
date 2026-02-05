/*
 * ClaudeOS Shell - Lexer/Tokenizer
 * Worker1 - Shell Claude
 *
 * Tokenizes shell input into words and operators
 */

#include "shell.h"
#include "../include/kmalloc.h"

/* Use kernel allocator instead of libc */
#define malloc(s) kmalloc(s)
#define free(p) kfree(p)

/* String functions (no libc in freestanding mode) */
static size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void *memcpy(void *dst, const void *src, size_t n) {
    char *d = dst;
    const char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

#define MAX_TOKENS 64

/* Helper: check if character is a special operator char */
static int is_operator_char(char c) {
    return c == '|' || c == '>' || c == '<' || c == '&' || c == ';';
}

/* Helper: check if character is whitespace */
static int is_whitespace(char c) {
    return c == ' ' || c == '\t';
}

/* Helper: duplicate a string (kernel doesn't have strdup) */
char *shell_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}

/* Tokenize input string */
token_t *lexer_tokenize(const char *input, int *token_count) {
    if (!input || !token_count) return NULL;

    token_t *tokens = malloc(sizeof(token_t) * MAX_TOKENS);
    if (!tokens) return NULL;

    int count = 0;
    const char *p = input;

    while (*p && count < MAX_TOKENS - 1) {
        /* Skip whitespace */
        while (is_whitespace(*p)) p++;
        if (!*p) break;

        /* Check for operators */
        if (*p == '|') {
            tokens[count].type = TOKEN_PIPE;
            tokens[count].value = shell_strdup("|");
            count++;
            p++;
        }
        else if (*p == '>' && *(p+1) == '>') {
            tokens[count].type = TOKEN_REDIRECT_APP;
            tokens[count].value = shell_strdup(">>");
            count++;
            p += 2;
        }
        else if (*p == '>') {
            tokens[count].type = TOKEN_REDIRECT_OUT;
            tokens[count].value = shell_strdup(">");
            count++;
            p++;
        }
        else if (*p == '<') {
            tokens[count].type = TOKEN_REDIRECT_IN;
            tokens[count].value = shell_strdup("<");
            count++;
            p++;
        }
        else if (*p == '&') {
            tokens[count].type = TOKEN_BACKGROUND;
            tokens[count].value = shell_strdup("&");
            count++;
            p++;
        }
        else if (*p == ';') {
            tokens[count].type = TOKEN_SEMICOLON;
            tokens[count].value = shell_strdup(";");
            count++;
            p++;
        }
        /* Handle quoted strings */
        else if (*p == '"' || *p == '\'') {
            char quote = *p++;
            const char *start = p;
            while (*p && *p != quote) p++;

            size_t len = p - start;
            tokens[count].type = TOKEN_WORD;
            tokens[count].value = malloc(len + 1);
            if (tokens[count].value) {
                memcpy(tokens[count].value, start, len);
                tokens[count].value[len] = '\0';
            }
            count++;

            if (*p == quote) p++; /* Skip closing quote */
        }
        /* Regular word */
        else {
            const char *start = p;
            while (*p && !is_whitespace(*p) && !is_operator_char(*p)) p++;

            size_t len = p - start;
            tokens[count].type = TOKEN_WORD;
            tokens[count].value = malloc(len + 1);
            if (tokens[count].value) {
                memcpy(tokens[count].value, start, len);
                tokens[count].value[len] = '\0';
            }
            count++;
        }
    }

    /* Add EOF token */
    tokens[count].type = TOKEN_EOF;
    tokens[count].value = NULL;
    count++;

    *token_count = count;
    return tokens;
}

/* Free tokens */
void lexer_free_tokens(token_t *tokens, int count) {
    if (!tokens) return;
    for (int i = 0; i < count; i++) {
        if (tokens[i].value) {
            free(tokens[i].value);
        }
    }
    free(tokens);
}
