/*
 * ClaudeOS AI Assistant Interface - ai.h
 * Author: Worker1 (Shell+FS Claude)
 * Description: Built-in AI assistant that helps users with commands and system info
 *
 * THE KILLER FEATURE OF CLAUDEOS!
 * A Claude-powered assistant built right into the OS.
 */

#ifndef _CLAUDEOS_AI_H
#define _CLAUDEOS_AI_H

#include "types.h"

/* Response buffer size */
#define AI_RESPONSE_MAX     512
#define AI_INPUT_MAX        256

/* Command info for AI knowledge base */
typedef struct {
    const char *name;           /* Command name */
    const char *usage;          /* Usage syntax */
    const char *description;    /* What it does */
    const char *example;        /* Example usage */
    const char *category;       /* Category (filesystem, system, etc.) */
} ai_command_info_t;

/* Question types detected by pattern matching */
typedef enum {
    AI_QUESTION_HOW,        /* "how do I..." */
    AI_QUESTION_WHAT,       /* "what is/does..." */
    AI_QUESTION_WHERE,      /* "where is..." */
    AI_QUESTION_WHY,        /* "why..." */
    AI_QUESTION_LIST,       /* "list...", "show..." */
    AI_QUESTION_SYSTEM,     /* "system...", "status..." */
    AI_QUESTION_HELP,       /* asking for help */
    AI_QUESTION_UNKNOWN     /* couldn't determine type */
} ai_question_type_t;

/*
 * ===========================================================================
 * AI Assistant Functions
 * ===========================================================================
 */

/**
 * Initialize the AI assistant
 * Sets up the command knowledge base
 */
void ai_init(void);

/**
 * Process a question and generate a response
 * @param question The user's question
 * @param response Buffer to store the response (must be AI_RESPONSE_MAX bytes)
 * @return 0 on success, -1 on error
 */
int ai_process_question(const char *question, char *response);

/**
 * Enter interactive AI chat mode
 * User can ask multiple questions until typing 'exit'
 */
void ai_interactive_mode(void);

/**
 * Get system status report
 * @param response Buffer to store the status (must be AI_RESPONSE_MAX bytes)
 */
void ai_system_status(char *response);

/**
 * Get help for a specific command
 * @param command The command name
 * @param response Buffer to store help text (must be AI_RESPONSE_MAX bytes)
 * @return 0 if found, -1 if unknown command
 */
int ai_command_help(const char *command, char *response);

/**
 * Detect the type of question being asked
 * @param question The user's question
 * @return The detected question type
 */
ai_question_type_t ai_detect_question_type(const char *question);

/**
 * Extract keywords from a question
 * @param question The question to analyze
 * @param keywords Array to store extracted keywords (up to 8)
 * @return Number of keywords extracted
 */
int ai_extract_keywords(const char *question, const char **keywords);

/**
 * Find the most relevant command for a question
 * @param question The user's question
 * @return Pointer to command info, or NULL if none found
 */
const ai_command_info_t *ai_find_relevant_command(const char *question);

#endif /* _CLAUDEOS_AI_H */
