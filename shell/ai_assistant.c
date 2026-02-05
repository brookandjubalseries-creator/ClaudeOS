/*
 * ClaudeOS AI Assistant - Implementation
 * Author: Worker1 (Shell+FS Claude)
 *
 * THE KILLER FEATURE OF CLAUDEOS!
 * A Claude-powered assistant built right into the OS.
 *
 * Usage:
 *   claude <question>     - Ask a question directly
 *   claude                - Enter interactive mode
 *
 * Features:
 *   - Command help ("how do I list files")
 *   - System status ("system status")
 *   - Interactive chat mode
 */

#include "../include/ai.h"
#include "../include/io.h"
#include "../include/types.h"
#include "../include/timer.h"
#include "../include/kmalloc.h"
#include "../fs/vfs.h"

/*
 * ===========================================================================
 * String Utilities (no libc in kernel)
 * ===========================================================================
 */

static int ai_strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void ai_strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

static void ai_strncpy(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int ai_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static void ai_strcat(char *dst, const char *src) {
    while (*dst) dst++;
    while ((*dst++ = *src++));
}

/* Case-insensitive comparison */
static int ai_strcasecmp(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return *a - *b;
}

/* Convert to lowercase */
static char ai_tolower(char c) {
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

/* Check if string contains substring (case insensitive) */
static int ai_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) return 0;
    int hlen = ai_strlen(haystack);
    int nlen = ai_strlen(needle);
    if (nlen > hlen) return 0;

    for (int i = 0; i <= hlen - nlen; i++) {
        int match = 1;
        for (int j = 0; j < nlen; j++) {
            char hc = ai_tolower(haystack[i + j]);
            char nc = ai_tolower(needle[j]);
            if (hc != nc) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

/* Integer to string */
static void ai_itoa(uint32_t num, char *buf) {
    char tmp[16];
    int i = 0;
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (num > 0) {
        tmp[i++] = '0' + (num % 10);
        num /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

/*
 * ===========================================================================
 * Command Knowledge Base
 * ===========================================================================
 */

static const ai_command_info_t command_db[] = {
    /* Filesystem commands */
    {
        "ls", "ls [directory]",
        "List files and directories in the current or specified directory",
        "ls /home/claude",
        "filesystem"
    },
    {
        "cd", "cd <directory>",
        "Change the current working directory",
        "cd /home/claude",
        "filesystem"
    },
    {
        "pwd", "pwd",
        "Print the current working directory path",
        "pwd",
        "filesystem"
    },
    {
        "cat", "cat <file>",
        "Display the contents of a file",
        "cat /etc/motd",
        "filesystem"
    },
    {
        "mkdir", "mkdir <directory>",
        "Create a new directory",
        "mkdir projects",
        "filesystem"
    },
    {
        "touch", "touch <file>",
        "Create an empty file or update timestamp",
        "touch notes.txt",
        "filesystem"
    },
    {
        "write", "write <file> <text>",
        "Create a new file with the specified text content",
        "write hello.txt Hello World!",
        "filesystem"
    },

    /* System commands */
    {
        "help", "help",
        "Display a list of all available commands",
        "help",
        "system"
    },
    {
        "clear", "clear",
        "Clear the screen",
        "clear",
        "system"
    },
    {
        "exit", "exit",
        "Exit the shell (but where would you go?)",
        "exit",
        "system"
    },
    {
        "reboot", "reboot",
        "Restart the computer",
        "reboot",
        "system"
    },
    {
        "uname", "uname [-a|-s|-m|-r]",
        "Print system information (name, version, architecture)",
        "uname -a",
        "system"
    },
    {
        "uptime", "uptime",
        "Show how long the system has been running",
        "uptime",
        "system"
    },
    {
        "sleep", "sleep <milliseconds>",
        "Pause execution for the specified number of milliseconds",
        "sleep 1000",
        "system"
    },
    {
        "ps", "ps",
        "List running processes",
        "ps",
        "system"
    },
    {
        "kill", "kill <pid>",
        "Terminate a process by its process ID",
        "kill 42",
        "system"
    },

    /* User commands */
    {
        "whoami", "whoami",
        "Print the current username (it's claude!)",
        "whoami",
        "user"
    },
    {
        "echo", "echo <text>",
        "Print text to the screen",
        "echo Hello World",
        "user"
    },
    {
        "history", "history",
        "Show the command history",
        "history",
        "user"
    },
    {
        "env", "env",
        "Print all environment variables",
        "env",
        "user"
    },
    {
        "export", "export NAME=VALUE",
        "Set an environment variable",
        "export EDITOR=vim",
        "user"
    },
    {
        "date", "date",
        "Print the current date and time",
        "date",
        "user"
    },

    /* AI Assistant */
    {
        "claude", "claude [question]",
        "Your friendly AI assistant! Ask me anything about ClaudeOS",
        "claude how do I list files",
        "ai"
    },

    /* Sentinel */
    { NULL, NULL, NULL, NULL, NULL }
};

/*
 * ===========================================================================
 * Question Pattern Matching
 * ===========================================================================
 */

ai_question_type_t ai_detect_question_type(const char *question) {
    if (!question) return AI_QUESTION_UNKNOWN;

    /* Check for common patterns */
    if (ai_contains(question, "how do") ||
        ai_contains(question, "how can") ||
        ai_contains(question, "how to")) {
        return AI_QUESTION_HOW;
    }

    if (ai_contains(question, "what is") ||
        ai_contains(question, "what does") ||
        ai_contains(question, "what's")) {
        return AI_QUESTION_WHAT;
    }

    if (ai_contains(question, "where is") ||
        ai_contains(question, "where can") ||
        ai_contains(question, "where do")) {
        return AI_QUESTION_WHERE;
    }

    if (ai_contains(question, "why")) {
        return AI_QUESTION_WHY;
    }

    if (ai_contains(question, "list") ||
        ai_contains(question, "show") ||
        ai_contains(question, "display")) {
        return AI_QUESTION_LIST;
    }

    if (ai_contains(question, "system") ||
        ai_contains(question, "status") ||
        ai_contains(question, "memory") ||
        ai_contains(question, "uptime") ||
        ai_contains(question, "process")) {
        return AI_QUESTION_SYSTEM;
    }

    if (ai_contains(question, "help") ||
        ai_contains(question, "assist")) {
        return AI_QUESTION_HELP;
    }

    return AI_QUESTION_UNKNOWN;
}

/*
 * ===========================================================================
 * Command Matching
 * ===========================================================================
 */

/* Keywords for command matching */
typedef struct {
    const char *keyword;
    const char *command;
} keyword_map_t;

static const keyword_map_t keyword_to_command[] = {
    /* Filesystem keywords */
    { "list", "ls" },
    { "files", "ls" },
    { "directory", "ls" },
    { "directories", "ls" },
    { "folder", "ls" },
    { "folders", "ls" },
    { "dir", "ls" },
    { "change", "cd" },
    { "navigate", "cd" },
    { "go", "cd" },
    { "move", "cd" },
    { "path", "pwd" },
    { "where", "pwd" },
    { "current", "pwd" },
    { "read", "cat" },
    { "view", "cat" },
    { "show", "cat" },
    { "display", "cat" },
    { "content", "cat" },
    { "contents", "cat" },
    { "create", "mkdir" },
    { "make", "mkdir" },
    { "new", "mkdir" },
    { "write", "write" },
    { "save", "write" },
    { "empty", "touch" },

    /* System keywords */
    { "clear", "clear" },
    { "cls", "clear" },
    { "screen", "clear" },
    { "exit", "exit" },
    { "quit", "exit" },
    { "leave", "exit" },
    { "restart", "reboot" },
    { "reboot", "reboot" },
    { "reset", "reboot" },
    { "version", "uname" },
    { "info", "uname" },
    { "system", "uname" },
    { "uptime", "uptime" },
    { "running", "uptime" },
    { "time", "uptime" },
    { "sleep", "sleep" },
    { "wait", "sleep" },
    { "pause", "sleep" },
    { "delay", "sleep" },
    { "process", "ps" },
    { "processes", "ps" },
    { "task", "ps" },
    { "tasks", "ps" },
    { "kill", "kill" },
    { "stop", "kill" },
    { "terminate", "kill" },
    { "end", "kill" },

    /* User keywords */
    { "user", "whoami" },
    { "username", "whoami" },
    { "who", "whoami" },
    { "print", "echo" },
    { "say", "echo" },
    { "output", "echo" },
    { "history", "history" },
    { "previous", "history" },
    { "commands", "history" },
    { "environment", "env" },
    { "variables", "env" },
    { "variable", "export" },
    { "set", "export" },
    { "date", "date" },

    { NULL, NULL }
};

const ai_command_info_t *ai_find_relevant_command(const char *question) {
    if (!question) return NULL;

    /* First, check if any command name is directly mentioned */
    for (int i = 0; command_db[i].name != NULL; i++) {
        if (ai_contains(question, command_db[i].name)) {
            return &command_db[i];
        }
    }

    /* Next, check keyword mappings */
    for (int i = 0; keyword_to_command[i].keyword != NULL; i++) {
        if (ai_contains(question, keyword_to_command[i].keyword)) {
            /* Find the command info for this keyword */
            for (int j = 0; command_db[j].name != NULL; j++) {
                if (ai_strcmp(command_db[j].name, keyword_to_command[i].command) == 0) {
                    return &command_db[j];
                }
            }
        }
    }

    return NULL;
}

int ai_command_help(const char *command, char *response) {
    if (!command || !response) return -1;

    for (int i = 0; command_db[i].name != NULL; i++) {
        if (ai_strcasecmp(command_db[i].name, command) == 0) {
            ai_strcpy(response, "[Claude AI] The '");
            ai_strcat(response, command_db[i].name);
            ai_strcat(response, "' command ");
            ai_strcat(response, command_db[i].description);
            ai_strcat(response, "\n\n            Usage: ");
            ai_strcat(response, command_db[i].usage);
            ai_strcat(response, "\n            Example: ");
            ai_strcat(response, command_db[i].example);
            ai_strcat(response, "\n");
            return 0;
        }
    }

    ai_strcpy(response, "[Claude AI] I don't know about that command.\n"
                        "            Type 'help' to see all available commands.\n");
    return -1;
}

/*
 * ===========================================================================
 * System Status
 * ===========================================================================
 */

void ai_system_status(char *response) {
    char num_buf[16];

    ai_strcpy(response, "[Claude AI] System Status Report\n");
    ai_strcat(response, "            ----------------------\n");

    /* Uptime */
    uint32_t uptime = timer_get_uptime_seconds();
    uint32_t hours = uptime / 3600;
    uint32_t mins = (uptime % 3600) / 60;
    uint32_t secs = uptime % 60;

    ai_strcat(response, "            Uptime: ");
    if (hours > 0) {
        ai_itoa(hours, num_buf);
        ai_strcat(response, num_buf);
        ai_strcat(response, "h ");
    }
    ai_itoa(mins, num_buf);
    ai_strcat(response, num_buf);
    ai_strcat(response, "m ");
    ai_itoa(secs, num_buf);
    ai_strcat(response, num_buf);
    ai_strcat(response, "s\n");

    /* Memory */
    size_t mem_used = kmalloc_used();
    size_t mem_free = kmalloc_free();

    ai_strcat(response, "            Memory Used: ");
    ai_itoa((uint32_t)(mem_used / 1024), num_buf);
    ai_strcat(response, num_buf);
    ai_strcat(response, " KB\n");

    ai_strcat(response, "            Memory Free: ");
    ai_itoa((uint32_t)(mem_free / 1024), num_buf);
    ai_strcat(response, num_buf);
    ai_strcat(response, " KB\n");

    /* Ticks */
    uint64_t ticks = timer_get_ticks();
    ai_strcat(response, "            Timer Ticks: ");
    ai_itoa((uint32_t)ticks, num_buf);
    ai_strcat(response, num_buf);
    ai_strcat(response, "\n");

    ai_strcat(response, "\n            Everything looks good!\n");
}

/*
 * ===========================================================================
 * Question Processing
 * ===========================================================================
 */

int ai_process_question(const char *question, char *response) {
    if (!question || !response) return -1;

    ai_question_type_t qtype = ai_detect_question_type(question);

    /* Handle system status queries */
    if (qtype == AI_QUESTION_SYSTEM) {
        ai_system_status(response);
        return 0;
    }

    /* Try to find a relevant command */
    const ai_command_info_t *cmd = ai_find_relevant_command(question);

    if (cmd) {
        /* Generate a helpful response based on question type */
        switch (qtype) {
            case AI_QUESTION_HOW:
                ai_strcpy(response, "[Claude AI] To do that, use the '");
                ai_strcat(response, cmd->name);
                ai_strcat(response, "' command!\n");
                ai_strcat(response, "            Usage: ");
                ai_strcat(response, cmd->usage);
                ai_strcat(response, "\n            Example: ");
                ai_strcat(response, cmd->example);
                ai_strcat(response, "\n");
                break;

            case AI_QUESTION_WHAT:
                ai_strcpy(response, "[Claude AI] The '");
                ai_strcat(response, cmd->name);
                ai_strcat(response, "' command ");
                ai_strcat(response, cmd->description);
                ai_strcat(response, "\n            Usage: ");
                ai_strcat(response, cmd->usage);
                ai_strcat(response, "\n");
                break;

            case AI_QUESTION_WHERE:
                ai_strcpy(response, "[Claude AI] You can use the '");
                ai_strcat(response, cmd->name);
                ai_strcat(response, "' command for that.\n");
                ai_strcat(response, "            ");
                ai_strcat(response, cmd->description);
                ai_strcat(response, "\n");
                break;

            default:
                ai_strcpy(response, "[Claude AI] You might want to try the '");
                ai_strcat(response, cmd->name);
                ai_strcat(response, "' command.\n");
                ai_strcat(response, "            ");
                ai_strcat(response, cmd->description);
                ai_strcat(response, "\n            Usage: ");
                ai_strcat(response, cmd->usage);
                ai_strcat(response, "\n            Example: ");
                ai_strcat(response, cmd->example);
                ai_strcat(response, "\n");
                break;
        }
        return 0;
    }

    /* Check for specific directory questions */
    if (ai_contains(question, "/etc") || ai_contains(question, "etc")) {
        ai_strcpy(response, "[Claude AI] The /etc directory contains system configuration files.\n"
                            "            Files: motd (welcome message), hostname, version\n"
                            "            Try: ls /etc  or  cat /etc/motd\n");
        return 0;
    }

    if (ai_contains(question, "/home") || ai_contains(question, "home")) {
        ai_strcpy(response, "[Claude AI] The /home directory contains user home directories.\n"
                            "            Your home is /home/claude - it has welcome.txt and .profile\n"
                            "            Try: cd /home/claude  then  ls\n");
        return 0;
    }

    if (ai_contains(question, "/tmp") || ai_contains(question, "tmp") ||
        ai_contains(question, "temporary")) {
        ai_strcpy(response, "[Claude AI] The /tmp directory is for temporary files.\n"
                            "            Feel free to create files there with 'write' or 'touch'.\n"
                            "            Try: ls /tmp\n");
        return 0;
    }

    /* Generic helpful response */
    if (qtype == AI_QUESTION_HELP) {
        ai_strcpy(response, "[Claude AI] I'm here to help! Here's what I can do:\n"
                            "            - Explain any command: 'claude what does ls do'\n"
                            "            - Guide you: 'claude how do I create a file'\n"
                            "            - System info: 'claude system status'\n"
                            "            - Or just type 'help' for all commands\n");
        return 0;
    }

    /* Fallback response */
    ai_strcpy(response, "[Claude AI] I'm not sure I understand that question.\n"
                        "            Try asking things like:\n"
                        "            - 'claude how do I list files'\n"
                        "            - 'claude what does cat do'\n"
                        "            - 'claude system status'\n"
                        "            Or type 'help' to see all commands.\n");
    return 0;
}

/*
 * ===========================================================================
 * Interactive Mode
 * ===========================================================================
 */

void ai_interactive_mode(void) {
    char input[AI_INPUT_MAX];
    char response[AI_RESPONSE_MAX];

    /* Welcome message */
    display_print("\n");
    display_print("[Claude AI] Hi! I'm your ClaudeOS assistant.\n");
    display_print("            Ask me anything about the system!\n");
    display_print("            Type 'exit' to leave chat mode.\n");
    display_print("\n");

    while (1) {
        /* Print prompt */
        display_print("You> ");

        /* Read input */
        int len = keyboard_read_line(input, AI_INPUT_MAX);
        if (len < 0) break;

        /* Skip empty lines */
        if (len == 0 || input[0] == '\0') continue;

        /* Check for exit */
        if (ai_strcasecmp(input, "exit") == 0 ||
            ai_strcasecmp(input, "quit") == 0 ||
            ai_strcasecmp(input, "bye") == 0) {
            display_print("[Claude AI] Goodbye! Type 'claude' anytime to chat again.\n\n");
            break;
        }

        /* Check for help */
        if (ai_strcasecmp(input, "help") == 0) {
            display_print("[Claude AI] You can ask me about:\n");
            display_print("            - Commands: 'what does ls do', 'how to create a file'\n");
            display_print("            - System: 'system status', 'how much memory'\n");
            display_print("            - Directories: 'what's in /etc'\n");
            display_print("            Or just describe what you want to do!\n\n");
            continue;
        }

        /* Check for commands shortcut */
        if (ai_strcasecmp(input, "commands") == 0 ||
            ai_strcasecmp(input, "list commands") == 0) {
            display_print("[Claude AI] Here are the command categories:\n\n");
            display_print("            FILESYSTEM: ls, cd, pwd, cat, mkdir, touch, write\n");
            display_print("            SYSTEM: help, clear, exit, reboot, uname, uptime, sleep, ps, kill\n");
            display_print("            USER: whoami, echo, history, env, export, date\n");
            display_print("            AI: claude (that's me!)\n\n");
            display_print("            Ask about any command for more details!\n\n");
            continue;
        }

        /* Process the question */
        ai_process_question(input, response);
        display_print(response);
        display_print("\n");
    }
}

/*
 * ===========================================================================
 * Initialization
 * ===========================================================================
 */

void ai_init(void) {
    /* Nothing to initialize for now - command_db is static */
    /* Future: could load additional command info from files */
}
