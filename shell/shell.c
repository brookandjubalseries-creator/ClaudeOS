/*
 * ClaudeOS Shell - Main
 * Worker1 - Shell Claude
 *
 * The main shell REPL (Read-Eval-Print-Loop)
 */

#include "shell.h"
#include "../include/io.h"
#include "../include/kmalloc.h"

/* Use kernel allocator */
#define malloc(s) kmalloc(s)
#define free(p) kfree(p)

/* String function */
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}

/* Global shell state pointer for builtins */
shell_state_t *g_shell_state = NULL;

/* External functions from builtins.c */
extern shell_command_t *find_builtin(const char *name);

/* Initialize shell state */
void shell_init(shell_state_t *state) {
    state->cwd = shell_strdup("/");
    state->history_count = 0;
    state->history_pos = 0;
    state->running = 1;

    for (int i = 0; i < SHELL_MAX_HISTORY; i++) {
        state->history[i] = NULL;
    }

    g_shell_state = state;
}

/* Add command to history */
void history_add(shell_state_t *state, const char *line) {
    if (!line || !*line) return;

    /* Don't add duplicates of last command */
    if (state->history_count > 0 &&
        strcmp(state->history[state->history_count - 1], line) == 0) {
        return;
    }

    /* If history is full, shift everything down */
    if (state->history_count >= SHELL_MAX_HISTORY) {
        free(state->history[0]);
        for (int i = 1; i < SHELL_MAX_HISTORY; i++) {
            state->history[i - 1] = state->history[i];
        }
        state->history_count = SHELL_MAX_HISTORY - 1;
    }

    state->history[state->history_count++] = shell_strdup(line);
    state->history_pos = state->history_count;
}

/* Get history entry by offset from current position */
const char *history_get(shell_state_t *state, int offset) {
    int idx = state->history_pos + offset;
    if (idx < 0 || idx >= state->history_count) {
        return NULL;
    }
    state->history_pos = idx;
    return state->history[idx];
}

/* Print shell prompt */
void shell_print_prompt(shell_state_t *state) {
    display_print("claude@os:");
    display_print(state->cwd ? state->cwd : "/");
    display_print("$ ");
}

/* Execute a single command */
static int execute_command(shell_state_t *state, shell_cmd_t *cmd) {
    (void)state;

    if (cmd->argc == 0 || !cmd->argv[0]) {
        return 0;
    }

    /* Check for builtin */
    shell_command_t *builtin = find_builtin(cmd->argv[0]);
    if (builtin) {
        return builtin->handler(cmd->argc, cmd->argv);
    }

    /* Not a builtin - would exec external program */
    /* TODO: Implement with Kernel Claude's process management */
    display_print(cmd->argv[0]);
    display_print(": command not found\n");
    return 127;
}

/* Execute a pipeline */
int executor_run(shell_state_t *state, pipeline_t *pipeline) {
    if (!pipeline || pipeline->count == 0) {
        return 0;
    }

    /* For now, just execute commands sequentially */
    /* TODO: Implement proper piping with Kernel Claude's pipe() syscall */
    int status = 0;
    for (int i = 0; i < pipeline->count; i++) {
        status = execute_command(state, &pipeline->commands[i]);
    }

    return status;
}

/* Print helpers */
void shell_print(const char *str) {
    display_print(str);
}

void shell_println(const char *str) {
    display_print(str);
    display_putchar('\n');
}

/* Main shell loop */
void shell_run(shell_state_t *state) {
    char input[SHELL_MAX_INPUT];

    /* Print welcome banner */
    display_print("\n");
    display_print("   ██████╗██╗      █████╗ ██╗   ██╗██████╗ ███████╗ ██████╗ ███████╗\n");
    display_print("  ██╔════╝██║     ██╔══██╗██║   ██║██╔══██╗██╔════╝██╔═══██╗██╔════╝\n");
    display_print("  ██║     ██║     ███████║██║   ██║██║  ██║█████╗  ██║   ██║███████╗\n");
    display_print("  ██║     ██║     ██╔══██║██║   ██║██║  ██║██╔══╝  ██║   ██║╚════██║\n");
    display_print("  ╚██████╗███████╗██║  ██║╚██████╔╝██████╔╝███████╗╚██████╔╝███████║\n");
    display_print("   ╚═════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝\n");
    display_print("\n");
    display_print("                    Version 0.2.0 - Built by MultiClaude Team\n");
    display_print("              Kernel Claude | Shell+FS Claude | Boss Claude\n");
    display_print("\n");
    display_print("  Type 'help' for commands, 'cat /etc/motd' for welcome message\n");
    display_print("  NEW: Type 'claude' for AI assistant - ask me anything!\n");
    display_print("\n");

    while (state->running) {
        /* Print prompt */
        shell_print_prompt(state);

        /* Read input line */
        int len = keyboard_read_line(input, SHELL_MAX_INPUT);
        if (len < 0) {
            /* EOF or error */
            break;
        }

        /* Skip empty lines */
        if (len == 0 || input[0] == '\0') {
            continue;
        }

        /* Add to history */
        history_add(state, input);

        /* Tokenize */
        int token_count;
        token_t *tokens = lexer_tokenize(input, &token_count);
        if (!tokens) {
            continue;
        }

        /* Parse */
        pipeline_t *pipeline = parser_parse(tokens, token_count);
        lexer_free_tokens(tokens, token_count);

        if (!pipeline) {
            continue;
        }

        /* Execute */
        executor_run(state, pipeline);

        /* Cleanup */
        parser_free_pipeline(pipeline);
    }
}

/* Cleanup shell state */
void shell_cleanup(shell_state_t *state) {
    if (state->cwd) {
        free(state->cwd);
        state->cwd = NULL;
    }

    for (int i = 0; i < state->history_count; i++) {
        if (state->history[i]) {
            free(state->history[i]);
            state->history[i] = NULL;
        }
    }

    g_shell_state = NULL;
}

/* Shell entry point - called from kernel */
void shell_main(void) {
    shell_state_t state;
    shell_init(&state);
    shell_run(&state);
    shell_cleanup(&state);
}
