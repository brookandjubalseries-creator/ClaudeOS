/*
 * ClaudeOS Shell - Header
 * Worker1 - Shell Claude
 */

#ifndef CLAUDEOS_SHELL_H
#define CLAUDEOS_SHELL_H

#include "../include/types.h"

/* Configuration */
#define SHELL_MAX_INPUT     256
#define SHELL_MAX_ARGS      16
#define SHELL_MAX_HISTORY   50
#define SHELL_PROMPT        "claude@os:%s$ "

/* Command structure */
typedef struct {
    char *name;
    char *description;
    int (*handler)(int argc, char **argv);
} shell_command_t;

/* Token types for lexer */
typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,         /* | */
    TOKEN_REDIRECT_OUT, /* > */
    TOKEN_REDIRECT_APP, /* >> */
    TOKEN_REDIRECT_IN,  /* < */
    TOKEN_BACKGROUND,   /* & */
    TOKEN_SEMICOLON,    /* ; */
    TOKEN_EOF
} token_type_t;

typedef struct {
    token_type_t type;
    char *value;
} token_t;

/* Command pipeline structure */
typedef struct {
    char **argv;
    int argc;
    char *redirect_in;
    char *redirect_out;
    int append;         /* 1 if >> instead of > */
} shell_cmd_t;

typedef struct {
    shell_cmd_t *commands;
    int count;
    int background;
} pipeline_t;

/* Shell state */
typedef struct {
    char *cwd;
    char *history[SHELL_MAX_HISTORY];
    int history_count;
    int history_pos;
    int running;
} shell_state_t;

/* Function prototypes */

/* Shell core */
void shell_init(shell_state_t *state);
void shell_run(shell_state_t *state);
void shell_cleanup(shell_state_t *state);
void shell_print_prompt(shell_state_t *state);

/* Lexer */
token_t *lexer_tokenize(const char *input, int *token_count);
void lexer_free_tokens(token_t *tokens, int count);

/* Parser */
pipeline_t *parser_parse(token_t *tokens, int token_count);
void parser_free_pipeline(pipeline_t *pipeline);

/* Executor */
int executor_run(shell_state_t *state, pipeline_t *pipeline);

/* Builtins */
int builtin_help(int argc, char **argv);
int builtin_echo(int argc, char **argv);
int builtin_clear(int argc, char **argv);
int builtin_exit(int argc, char **argv);
int builtin_pwd(int argc, char **argv);
int builtin_cd(int argc, char **argv);
int builtin_ls(int argc, char **argv);
int builtin_cat(int argc, char **argv);
int builtin_history(int argc, char **argv);

/* History */
void history_add(shell_state_t *state, const char *line);
const char *history_get(shell_state_t *state, int offset);

/* Utility */
char *shell_strdup(const char *s);
void shell_print(const char *str);
void shell_println(const char *str);

#endif /* CLAUDEOS_SHELL_H */
