/*
 * ClaudeOS Shell - Parser
 * Worker1 - Shell Claude
 *
 * Parses tokens into command pipelines
 */

#include "shell.h"
#include "../include/kmalloc.h"

/* Use kernel allocator */
#define malloc(s) kmalloc(s)
#define free(p) kfree(p)

#define MAX_CMDS_IN_PIPELINE 8
#define MAX_ARGS 16

/* Parse tokens into a pipeline structure */
pipeline_t *parser_parse(token_t *tokens, int token_count) {
    if (!tokens || token_count == 0) return NULL;

    pipeline_t *pipeline = malloc(sizeof(pipeline_t));
    if (!pipeline) return NULL;

    pipeline->commands = malloc(sizeof(shell_cmd_t) * MAX_CMDS_IN_PIPELINE);
    if (!pipeline->commands) {
        free(pipeline);
        return NULL;
    }

    pipeline->count = 0;
    pipeline->background = 0;

    /* Current command being built */
    char **argv = malloc(sizeof(char*) * MAX_ARGS);
    int argc = 0;
    char *redirect_in = NULL;
    char *redirect_out = NULL;
    int append = 0;

    int i = 0;
    while (i < token_count && tokens[i].type != TOKEN_EOF) {
        token_t *tok = &tokens[i];

        switch (tok->type) {
            case TOKEN_WORD:
                if (argc < MAX_ARGS - 1) {
                    argv[argc++] = shell_strdup(tok->value);
                }
                break;

            case TOKEN_REDIRECT_IN:
                i++;
                if (i < token_count && tokens[i].type == TOKEN_WORD) {
                    redirect_in = shell_strdup(tokens[i].value);
                }
                break;

            case TOKEN_REDIRECT_OUT:
                i++;
                if (i < token_count && tokens[i].type == TOKEN_WORD) {
                    redirect_out = shell_strdup(tokens[i].value);
                    append = 0;
                }
                break;

            case TOKEN_REDIRECT_APP:
                i++;
                if (i < token_count && tokens[i].type == TOKEN_WORD) {
                    redirect_out = shell_strdup(tokens[i].value);
                    append = 1;
                }
                break;

            case TOKEN_PIPE:
            case TOKEN_SEMICOLON:
                /* Finish current command */
                if (argc > 0) {
                    argv[argc] = NULL;
                    shell_cmd_t *cmd = &pipeline->commands[pipeline->count];
                    cmd->argv = argv;
                    cmd->argc = argc;
                    cmd->redirect_in = redirect_in;
                    cmd->redirect_out = redirect_out;
                    cmd->append = append;
                    pipeline->count++;

                    /* Reset for next command */
                    argv = malloc(sizeof(char*) * MAX_ARGS);
                    argc = 0;
                    redirect_in = NULL;
                    redirect_out = NULL;
                    append = 0;
                }
                break;

            case TOKEN_BACKGROUND:
                pipeline->background = 1;
                break;

            default:
                break;
        }
        i++;
    }

    /* Finish last command */
    if (argc > 0) {
        argv[argc] = NULL;
        shell_cmd_t *cmd = &pipeline->commands[pipeline->count];
        cmd->argv = argv;
        cmd->argc = argc;
        cmd->redirect_in = redirect_in;
        cmd->redirect_out = redirect_out;
        cmd->append = append;
        pipeline->count++;
    } else {
        free(argv);
    }

    /* If no commands parsed, free and return NULL */
    if (pipeline->count == 0) {
        free(pipeline->commands);
        free(pipeline);
        return NULL;
    }

    return pipeline;
}

/* Free a pipeline structure */
void parser_free_pipeline(pipeline_t *pipeline) {
    if (!pipeline) return;

    for (int i = 0; i < pipeline->count; i++) {
        shell_cmd_t *cmd = &pipeline->commands[i];
        if (cmd->argv) {
            for (int j = 0; j < cmd->argc; j++) {
                if (cmd->argv[j]) free(cmd->argv[j]);
            }
            free(cmd->argv);
        }
        if (cmd->redirect_in) free(cmd->redirect_in);
        if (cmd->redirect_out) free(cmd->redirect_out);
    }
    free(pipeline->commands);
    free(pipeline);
}
