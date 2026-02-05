/*
 * ClaudeOS Shell - Built-in Commands
 * Worker1 - Shell Claude
 *
 * Phase 4 Updates:
 * - Real uptime using timer_get_ticks()
 * - sleep <ms> command using timer_sleep_ms()
 * - ps command to list processes
 * - kill <pid> command to terminate processes
 * - claude AI assistant command
 */

#include "shell.h"
#include "../include/io.h"
#include "../include/timer.h"
#include "../include/process.h"
#include "../include/ai.h"
#include "../fs/vfs.h"

/* String utilities (no libc in freestanding mode) */
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}

static size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static char *strcpy(char *dst, const char *src) {
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

static int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s2 && *s1 == *s2) { s1++; s2++; n--; }
    return n ? *s1 - *s2 : 0;
}

/* Forward declaration of external shell state (set by shell.c) */
extern shell_state_t *g_shell_state;

/* Forward declarations for new commands */
int builtin_uname(int argc, char **argv);
int builtin_whoami(int argc, char **argv);
int builtin_env(int argc, char **argv);
int builtin_export(int argc, char **argv);
int builtin_date(int argc, char **argv);
int builtin_uptime(int argc, char **argv);
int builtin_mkdir(int argc, char **argv);
int builtin_touch(int argc, char **argv);
int builtin_write(int argc, char **argv);
int builtin_reboot(int argc, char **argv);
int builtin_sleep(int argc, char **argv);
int builtin_ps(int argc, char **argv);
int builtin_kill(int argc, char **argv);
int builtin_claude(int argc, char **argv);

/* Command table - add new builtins here */
static shell_command_t builtin_commands[] = {
    {"help",    "Display available commands",        builtin_help},
    {"echo",    "Print arguments to screen",         builtin_echo},
    {"clear",   "Clear the screen",                  builtin_clear},
    {"exit",    "Exit the shell",                    builtin_exit},
    {"pwd",     "Print working directory",           builtin_pwd},
    {"cd",      "Change directory",                  builtin_cd},
    {"ls",      "List directory contents",           builtin_ls},
    {"cat",     "Display file contents",             builtin_cat},
    {"history", "Show command history",              builtin_history},
    {"uname",   "Print system information",          builtin_uname},
    {"whoami",  "Print current user name",           builtin_whoami},
    {"env",     "Print environment variables",       builtin_env},
    {"export",  "Set environment variable",          builtin_export},
    {"date",    "Print current date/time",           builtin_date},
    {"uptime",  "Show system uptime",                builtin_uptime},
    {"mkdir",   "Create a directory",                builtin_mkdir},
    {"touch",   "Create empty file",                 builtin_touch},
    {"write",   "Write text to file",                builtin_write},
    {"reboot",  "Reboot the system",                 builtin_reboot},
    /* Phase 4 commands */
    {"sleep",   "Sleep for N milliseconds",          builtin_sleep},
    {"ps",      "List running processes",            builtin_ps},
    {"kill",    "Terminate a process by PID",        builtin_kill},
    {"claude",  "AI assistant - ask me anything!",   builtin_claude},
    {NULL, NULL, NULL}  /* Sentinel */
};

/* Get the builtin command table */
shell_command_t *get_builtin_commands(void) {
    return builtin_commands;
}

/* Find a builtin command by name */
shell_command_t *find_builtin(const char *name) {
    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        if (strcmp(builtin_commands[i].name, name) == 0) {
            return &builtin_commands[i];
        }
    }
    return NULL;
}

/* help - Display available commands */
int builtin_help(int argc, char **argv) {
    (void)argc; (void)argv;

    display_print("\n");
    display_print("  ╔═══════════════════════════════════════════════════╗\n");
    display_print("  ║           ClaudeOS Shell Commands                 ║\n");
    display_print("  ╠═══════════════════════════════════════════════════╣\n");

    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        display_print("  ║  ");
        display_print(builtin_commands[i].name);

        /* Padding for alignment */
        int pad = 10 - strlen(builtin_commands[i].name);
        while (pad-- > 0) display_putchar(' ');

        display_print(" - ");
        display_print(builtin_commands[i].description);

        /* Right padding */
        pad = 35 - strlen(builtin_commands[i].description);
        while (pad-- > 0) display_putchar(' ');
        display_print("║\n");
    }

    display_print("  ╚═══════════════════════════════════════════════════╝\n");
    display_print("\n");
    display_print("  Operators: | (pipe), > (redirect), >> (append), & (background)\n\n");

    return 0;
}

/* echo - Print arguments */
int builtin_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        display_print(argv[i]);
        if (i < argc - 1) display_putchar(' ');
    }
    display_putchar('\n');
    return 0;
}

/* clear - Clear screen */
int builtin_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    display_clear();
    return 0;
}

/* exit - Exit shell */
int builtin_exit(int argc, char **argv) {
    (void)argc; (void)argv;
    if (g_shell_state) {
        g_shell_state->running = 0;
    }
    display_print("Goodbye!\n");
    return 0;
}

/* pwd - Print working directory */
int builtin_pwd(int argc, char **argv) {
    (void)argc; (void)argv;
    if (g_shell_state && g_shell_state->cwd) {
        display_print(g_shell_state->cwd);
        display_putchar('\n');
    } else {
        display_print("/\n");
    }
    return 0;
}

/* cd - Change directory */
int builtin_cd(int argc, char **argv) {
    if (!g_shell_state) return 1;

    const char *target;
    char full_path[256];

    if (argc < 2) {
        /* cd with no args goes to home */
        target = "/home/claude";
    } else {
        /* Handle relative paths */
        if (argv[1][0] != '/') {
            int i = 0;
            const char *cwd = g_shell_state->cwd ? g_shell_state->cwd : "/";
            while (*cwd && i < 254) full_path[i++] = *cwd++;
            if (i > 0 && full_path[i-1] != '/') full_path[i++] = '/';
            const char *rel = argv[1];
            while (*rel && i < 255) full_path[i++] = *rel++;
            full_path[i] = '\0';
            target = full_path;
        } else {
            target = argv[1];
        }
    }

    /* Validate path exists and is a directory */
    fs_stat_t stat;
    if (vfs_stat(target, &stat) != 0) {
        display_print("cd: ");
        display_print(argv[1] ? argv[1] : target);
        display_print(": No such file or directory\n");
        return 1;
    }

    if (stat.st_type != FS_DIRECTORY) {
        display_print("cd: ");
        display_print(argv[1] ? argv[1] : target);
        display_print(": Not a directory\n");
        return 1;
    }

    /* Update cwd */
    g_shell_state->cwd = shell_strdup(target);
    return 0;
}

/* ls - List directory contents */
int builtin_ls(int argc, char **argv) {
    const char *path;
    char full_path[256];

    /* Determine path to list */
    if (argc > 1) {
        /* If relative path and we have cwd, make absolute */
        if (argv[1][0] != '/' && g_shell_state && g_shell_state->cwd) {
            /* Build absolute path */
            int i = 0;
            const char *cwd = g_shell_state->cwd;
            while (*cwd && i < 254) full_path[i++] = *cwd++;
            if (i > 0 && full_path[i-1] != '/') full_path[i++] = '/';
            const char *rel = argv[1];
            while (*rel && i < 255) full_path[i++] = *rel++;
            full_path[i] = '\0';
            path = full_path;
        } else {
            path = argv[1];
        }
    } else {
        /* No argument - use cwd */
        path = (g_shell_state && g_shell_state->cwd) ? g_shell_state->cwd : "/";
    }

    /* Check if path exists and is a directory */
    fs_stat_t stat;
    if (vfs_stat(path, &stat) != 0) {
        display_print("ls: cannot access '");
        display_print(path);
        display_print("': No such file or directory\n");
        return 1;
    }

    if (stat.st_type != FS_DIRECTORY) {
        /* It's a file - just print its name */
        display_print(argv[1] ? argv[1] : path);
        display_putchar('\n');
        return 0;
    }

    /* List directory contents */
    int index = 0;
    fs_dirent_t *entry;

    while ((entry = vfs_readdir(path, index)) != NULL) {
        /* Color/indicator based on type */
        if (entry->type == FS_DIRECTORY) {
            display_print(entry->name);
            display_print("/");
        } else {
            display_print(entry->name);
        }
        display_print("  ");
        index++;

        /* Newline every 4 items for readability */
        if (index % 4 == 0) {
            display_putchar('\n');
        }
    }

    if (index % 4 != 0) {
        display_putchar('\n');
    }

    if (index == 0) {
        display_print("(empty directory)\n");
    }

    return 0;
}

/* cat - Display file contents */
int builtin_cat(int argc, char **argv) {
    if (argc < 2) {
        display_print("cat: missing file operand\n");
        return 1;
    }

    char full_path[256];
    const char *path;

    /* Build absolute path if relative */
    if (argv[1][0] != '/' && g_shell_state && g_shell_state->cwd) {
        int i = 0;
        const char *cwd = g_shell_state->cwd;
        while (*cwd && i < 254) full_path[i++] = *cwd++;
        if (i > 0 && full_path[i-1] != '/') full_path[i++] = '/';
        const char *rel = argv[1];
        while (*rel && i < 255) full_path[i++] = *rel++;
        full_path[i] = '\0';
        path = full_path;
    } else {
        path = argv[1];
    }

    /* Check if file exists */
    fs_stat_t stat;
    if (vfs_stat(path, &stat) != 0) {
        display_print("cat: ");
        display_print(argv[1]);
        display_print(": No such file or directory\n");
        return 1;
    }

    if (stat.st_type == FS_DIRECTORY) {
        display_print("cat: ");
        display_print(argv[1]);
        display_print(": Is a directory\n");
        return 1;
    }

    /* Open and read the file */
    int fd = vfs_open(path, O_RDONLY);
    if (fd < 0) {
        display_print("cat: ");
        display_print(argv[1]);
        display_print(": Cannot open file\n");
        return 1;
    }

    /* Read and display contents */
    char buf[256];
    ssize_t bytes;
    while ((bytes = vfs_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[bytes] = '\0';
        display_print(buf);
    }

    vfs_close(fd);
    return 0;
}

/* history - Show command history */
int builtin_history(int argc, char **argv) {
    (void)argc; (void)argv;

    if (!g_shell_state) {
        display_print("history: shell state unavailable\n");
        return 1;
    }

    for (int i = 0; i < g_shell_state->history_count; i++) {
        /* Print index */
        char num[8];
        int n = i + 1;
        int j = 0;
        if (n >= 100) num[j++] = '0' + (n / 100) % 10;
        if (n >= 10)  num[j++] = '0' + (n / 10) % 10;
        num[j++] = '0' + n % 10;
        num[j] = '\0';

        display_print("  ");
        display_print(num);
        display_print("  ");
        display_print(g_shell_state->history[i]);
        display_putchar('\n');
    }
    return 0;
}

/*
 * ===========================================================================
 * SYSTEM INFO COMMANDS
 * ===========================================================================
 */

#define CLAUDEOS_VERSION "0.2.0"
#define CLAUDEOS_NAME    "ClaudeOS"
#define CLAUDEOS_ARCH    "i386"

/* uname - Print system information */
int builtin_uname(int argc, char **argv) {
    int show_all = 0;
    int show_kernel = 0;
    int show_machine = 0;
    int show_version = 0;

    /* Parse flags */
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'a': show_all = 1; break;
                    case 's': show_kernel = 1; break;
                    case 'm': show_machine = 1; break;
                    case 'r': case 'v': show_version = 1; break;
                }
            }
        }
    }

    /* Default: show kernel name */
    if (!show_all && !show_kernel && !show_machine && !show_version) {
        show_kernel = 1;
    }

    if (show_all || show_kernel) {
        display_print(CLAUDEOS_NAME);
        display_putchar(' ');
    }
    if (show_all || show_version) {
        display_print(CLAUDEOS_VERSION);
        display_putchar(' ');
    }
    if (show_all || show_machine) {
        display_print(CLAUDEOS_ARCH);
    }
    display_putchar('\n');
    return 0;
}

/* whoami - Print current user */
int builtin_whoami(int argc, char **argv) {
    (void)argc; (void)argv;
    display_print("claude\n");
    return 0;
}

/*
 * ===========================================================================
 * ENVIRONMENT VARIABLES
 * ===========================================================================
 */

#define MAX_ENV_VARS 32
#define MAX_ENV_NAME 32
#define MAX_ENV_VALUE 128

typedef struct {
    char name[MAX_ENV_NAME];
    char value[MAX_ENV_VALUE];
} env_var_t;

static env_var_t env_vars[MAX_ENV_VARS];
static int env_count = 0;
static int env_initialized = 0;

/* Initialize default environment */
static void env_init(void) {
    if (env_initialized) return;

    /* Set default variables */
    /* Manually copy strings since we don't have strcpy in kernel */
    const char *path = "/bin:/usr/bin";
    const char *home = "/home/claude";
    const char *user = "claude";
    const char *shell = "/bin/csh";

    /* PATH */
    int i = 0;
    for (const char *p = "PATH"; *p; p++) env_vars[0].name[i++] = *p;
    env_vars[0].name[i] = '\0';
    i = 0;
    for (const char *p = path; *p; p++) env_vars[0].value[i++] = *p;
    env_vars[0].value[i] = '\0';

    /* HOME */
    i = 0;
    for (const char *p = "HOME"; *p; p++) env_vars[1].name[i++] = *p;
    env_vars[1].name[i] = '\0';
    i = 0;
    for (const char *p = home; *p; p++) env_vars[1].value[i++] = *p;
    env_vars[1].value[i] = '\0';

    /* USER */
    i = 0;
    for (const char *p = "USER"; *p; p++) env_vars[2].name[i++] = *p;
    env_vars[2].name[i] = '\0';
    i = 0;
    for (const char *p = user; *p; p++) env_vars[2].value[i++] = *p;
    env_vars[2].value[i] = '\0';

    /* SHELL */
    i = 0;
    for (const char *p = "SHELL"; *p; p++) env_vars[3].name[i++] = *p;
    env_vars[3].name[i] = '\0';
    i = 0;
    for (const char *p = shell; *p; p++) env_vars[3].value[i++] = *p;
    env_vars[3].value[i] = '\0';

    env_count = 4;
    env_initialized = 1;
}

/* env - Print all environment variables */
int builtin_env(int argc, char **argv) {
    (void)argc; (void)argv;
    env_init();

    for (int i = 0; i < env_count; i++) {
        display_print(env_vars[i].name);
        display_putchar('=');
        display_print(env_vars[i].value);
        display_putchar('\n');
    }
    return 0;
}

/* Helper: compare strings */
static int str_eq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

/* Helper: copy string */
static void str_copy(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* export - Set environment variable (export NAME=VALUE) */
int builtin_export(int argc, char **argv) {
    env_init();

    if (argc < 2) {
        /* No args - same as env */
        return builtin_env(argc, argv);
    }

    /* Parse NAME=VALUE */
    char *arg = argv[1];
    char *eq = arg;
    while (*eq && *eq != '=') eq++;

    if (!*eq) {
        display_print("export: invalid format. Use: export NAME=VALUE\n");
        return 1;
    }

    /* Split at = */
    *eq = '\0';
    char *name = arg;
    char *value = eq + 1;

    /* Find existing or add new */
    for (int i = 0; i < env_count; i++) {
        if (str_eq(env_vars[i].name, name)) {
            str_copy(env_vars[i].value, value, MAX_ENV_VALUE);
            return 0;
        }
    }

    /* Add new */
    if (env_count < MAX_ENV_VARS) {
        str_copy(env_vars[env_count].name, name, MAX_ENV_NAME);
        str_copy(env_vars[env_count].value, value, MAX_ENV_VALUE);
        env_count++;
    } else {
        display_print("export: environment full\n");
        return 1;
    }

    return 0;
}

/*
 * ===========================================================================
 * TIME/DATE COMMANDS (stubs - need kernel RTC driver)
 * ===========================================================================
 */

/* date - Print current date/time */
int builtin_date(int argc, char **argv) {
    (void)argc; (void)argv;
    /* TODO: Get actual time from Kernel's RTC driver */
    display_print("Wed Feb  4 00:00:00 UTC 2026\n");
    display_print("[date: Real time requires RTC driver from Kernel Claude]\n");
    return 0;
}

/* Helper to print an integer */
static void print_int(uint32_t num) {
    char buf[16];
    int i = 0;
    if (num == 0) {
        display_putchar('0');
        return;
    }
    char tmp[16];
    while (num > 0) {
        tmp[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        buf[16 - i] = tmp[i - 1];
        i--;
    }
    for (int j = 16 - (sizeof(buf) / sizeof(buf[0])); buf[j]; j++) {
        display_putchar(buf[j]);
    }
}

/* uptime - Show system uptime (Phase 4: Real timer!) */
int builtin_uptime(int argc, char **argv) {
    (void)argc; (void)argv;

    uint32_t uptime_sec = timer_get_uptime_seconds();
    uint64_t ticks = timer_get_ticks();

    uint32_t days = uptime_sec / 86400;
    uint32_t hours = (uptime_sec % 86400) / 3600;
    uint32_t minutes = (uptime_sec % 3600) / 60;
    uint32_t seconds = uptime_sec % 60;

    display_print("up ");

    /* Days */
    if (days > 0) {
        char num[12];
        int i = 0;
        uint32_t n = days;
        if (n == 0) { num[i++] = '0'; }
        else {
            char tmp[12];
            int j = 0;
            while (n > 0) { tmp[j++] = '0' + (n % 10); n /= 10; }
            while (j > 0) { num[i++] = tmp[--j]; }
        }
        num[i] = '\0';
        display_print(num);
        display_print(" day");
        if (days != 1) display_putchar('s');
        display_print(", ");
    }

    /* Hours */
    {
        char num[12];
        int i = 0;
        uint32_t n = hours;
        if (n == 0) { num[i++] = '0'; }
        else {
            char tmp[12];
            int j = 0;
            while (n > 0) { tmp[j++] = '0' + (n % 10); n /= 10; }
            while (j > 0) { num[i++] = tmp[--j]; }
        }
        num[i] = '\0';
        display_print(num);
        display_print(":");
    }

    /* Minutes */
    {
        char num[12];
        uint32_t n = minutes;
        if (n < 10) display_putchar('0');
        int i = 0;
        if (n == 0) { num[i++] = '0'; }
        else {
            char tmp[12];
            int j = 0;
            while (n > 0) { tmp[j++] = '0' + (n % 10); n /= 10; }
            while (j > 0) { num[i++] = tmp[--j]; }
        }
        num[i] = '\0';
        display_print(num);
        display_print(":");
    }

    /* Seconds */
    {
        char num[12];
        uint32_t n = seconds;
        if (n < 10) display_putchar('0');
        int i = 0;
        if (n == 0) { num[i++] = '0'; }
        else {
            char tmp[12];
            int j = 0;
            while (n > 0) { tmp[j++] = '0' + (n % 10); n /= 10; }
            while (j > 0) { num[i++] = tmp[--j]; }
        }
        num[i] = '\0';
        display_print(num);
    }

    /* Tick count */
    display_print(" (");
    {
        char num[20];
        int i = 0;
        uint64_t n = ticks;
        if (n == 0) { num[i++] = '0'; }
        else {
            char tmp[20];
            int j = 0;
            while (n > 0) { tmp[j++] = '0' + (n % 10); n /= 10; }
            while (j > 0) { num[i++] = tmp[--j]; }
        }
        num[i] = '\0';
        display_print(num);
    }
    display_print(" ticks)\n");

    return 0;
}

/*
 * ===========================================================================
 * FILESYSTEM MODIFICATION COMMANDS
 * ===========================================================================
 */

/* Helper: resolve path to absolute */
static void resolve_path(const char *path, char *out, int max) {
    if (path[0] == '/') {
        /* Already absolute */
        int i = 0;
        while (path[i] && i < max - 1) {
            out[i] = path[i];
            i++;
        }
        out[i] = '\0';
    } else {
        /* Relative - prepend cwd */
        const char *cwd = (g_shell_state && g_shell_state->cwd) ?
                          g_shell_state->cwd : "/";
        int i = 0;
        while (*cwd && i < max - 2) out[i++] = *cwd++;
        if (i > 0 && out[i-1] != '/') out[i++] = '/';
        while (*path && i < max - 1) out[i++] = *path++;
        out[i] = '\0';
    }
}

/* Helper: get parent path and name from full path */
static void split_path(const char *path, char *parent, char *name, int max) {
    /* Find last slash */
    int last_slash = -1;
    int len = 0;
    while (path[len]) {
        if (path[len] == '/') last_slash = len;
        len++;
    }

    if (last_slash <= 0) {
        /* Root or no slash */
        parent[0] = '/';
        parent[1] = '\0';
        int i = 0;
        const char *p = (last_slash == 0) ? path + 1 : path;
        while (*p && i < max - 1) name[i++] = *p++;
        name[i] = '\0';
    } else {
        /* Copy parent */
        int i = 0;
        while (i < last_slash && i < max - 1) {
            parent[i] = path[i];
            i++;
        }
        parent[i] = '\0';

        /* Copy name */
        i = 0;
        const char *p = path + last_slash + 1;
        while (*p && i < max - 1) name[i++] = *p++;
        name[i] = '\0';
    }
}

/* mkdir - Create a directory */
int builtin_mkdir(int argc, char **argv) {
    if (argc < 2) {
        display_print("mkdir: missing operand\n");
        return 1;
    }

    char full_path[256];
    char parent_path[256];
    char dir_name[64];

    resolve_path(argv[1], full_path, 256);
    split_path(full_path, parent_path, dir_name, 64);

    /* Check if already exists */
    fs_stat_t stat;
    if (vfs_stat(full_path, &stat) == 0) {
        display_print("mkdir: cannot create directory '");
        display_print(argv[1]);
        display_print("': File exists\n");
        return 1;
    }

    /* Find parent directory */
    fs_node_t *parent = vfs_lookup(parent_path);
    if (!parent) {
        display_print("mkdir: cannot create directory '");
        display_print(argv[1]);
        display_print("': No such file or directory\n");
        return 1;
    }

    /* Create the directory */
    fs_node_t *new_dir = vfs_create_dir(parent, dir_name);
    if (!new_dir) {
        display_print("mkdir: cannot create directory '");
        display_print(argv[1]);
        display_print("': Operation failed\n");
        return 1;
    }

    return 0;
}

/* touch - Create empty file or update timestamp */
int builtin_touch(int argc, char **argv) {
    if (argc < 2) {
        display_print("touch: missing file operand\n");
        return 1;
    }

    char full_path[256];
    char parent_path[256];
    char file_name[64];

    resolve_path(argv[1], full_path, 256);
    split_path(full_path, parent_path, file_name, 64);

    /* Check if already exists */
    fs_stat_t stat;
    if (vfs_stat(full_path, &stat) == 0) {
        /* File exists - in a real fs we'd update timestamp */
        return 0;
    }

    /* Find parent directory */
    fs_node_t *parent = vfs_lookup(parent_path);
    if (!parent) {
        display_print("touch: cannot touch '");
        display_print(argv[1]);
        display_print("': No such file or directory\n");
        return 1;
    }

    /* Create empty file */
    fs_node_t *new_file = vfs_create_file(parent, file_name, "");
    if (!new_file) {
        display_print("touch: cannot touch '");
        display_print(argv[1]);
        display_print("': Operation failed\n");
        return 1;
    }

    return 0;
}

/* write - Write content to file (write <file> <text...>) */
int builtin_write(int argc, char **argv) {
    if (argc < 3) {
        display_print("write: usage: write <file> <text...>\n");
        return 1;
    }

    char full_path[256];
    char parent_path[256];
    char file_name[64];

    resolve_path(argv[1], full_path, 256);
    split_path(full_path, parent_path, file_name, 64);

    /* Build content from remaining args */
    char content[512];
    int pos = 0;
    for (int i = 2; i < argc && pos < 510; i++) {
        const char *p = argv[i];
        while (*p && pos < 510) content[pos++] = *p++;
        if (i < argc - 1 && pos < 510) content[pos++] = ' ';
    }
    content[pos++] = '\n';
    content[pos] = '\0';

    /* Check if file exists */
    fs_stat_t stat;
    if (vfs_stat(full_path, &stat) == 0) {
        display_print("write: '");
        display_print(argv[1]);
        display_print("' exists (ramfs doesn't support overwrite yet)\n");
        return 1;
    }

    /* Find parent directory */
    fs_node_t *parent = vfs_lookup(parent_path);
    if (!parent) {
        display_print("write: cannot create '");
        display_print(argv[1]);
        display_print("': No such directory\n");
        return 1;
    }

    /* Create file with content */
    fs_node_t *new_file = vfs_create_file(parent, file_name, content);
    if (!new_file) {
        display_print("write: failed to create file\n");
        return 1;
    }

    return 0;
}

/*
 * ===========================================================================
 * SYSTEM CONTROL COMMANDS
 * ===========================================================================
 */

/* External kernel function - Kernel Claude will provide this */
extern void kernel_reboot(void);

/* reboot - Reboot the system */
int builtin_reboot(int argc, char **argv) {
    (void)argc; (void)argv;

    display_print("\n");
    display_print("  Rebooting ClaudeOS...\n");
    display_print("\n");

    /* Try keyboard controller reset (standard PC method) */
    /* Write 0xFE to keyboard controller command port 0x64 */
    __asm__ volatile (
        "cli\n"                    /* Disable interrupts */
        "movb $0xFE, %%al\n"       /* Reset command */
        "outb %%al, $0x64\n"       /* Send to keyboard controller */
        :
        :
        : "al"
    );

    /* If that didn't work, try triple fault */
    __asm__ volatile (
        "lidt (%%eax)\n"           /* Load invalid IDT */
        "int $0x03\n"              /* Trigger interrupt -> triple fault */
        :
        : "a" (0)
    );

    /* Should never reach here */
    display_print("Reboot failed. Please reset manually.\n");
    return 1;
}

/*
 * ===========================================================================
 * PHASE 4 COMMANDS - Timer, Process, AI
 * ===========================================================================
 */

/* Helper: parse string to integer */
static int str_to_int(const char *s) {
    int num = 0;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') {
        num = num * 10 + (*s - '0');
        s++;
    }
    return neg ? -num : num;
}

/* Helper: integer to string */
static void int_to_str(uint32_t num, char *buf) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char tmp[16];
    int i = 0;
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

/* sleep - Sleep for N milliseconds */
int builtin_sleep(int argc, char **argv) {
    if (argc < 2) {
        display_print("sleep: usage: sleep <milliseconds>\n");
        return 1;
    }

    int ms = str_to_int(argv[1]);
    if (ms <= 0) {
        display_print("sleep: invalid time: ");
        display_print(argv[1]);
        display_print("\n");
        return 1;
    }

    display_print("Sleeping for ");
    display_print(argv[1]);
    display_print(" ms...\n");

    timer_sleep_ms((uint32_t)ms);

    display_print("Done.\n");
    return 0;
}

/* ps - List running processes */
int builtin_ps(int argc, char **argv) {
    (void)argc; (void)argv;

    uint32_t pids[MAX_PROCESSES];
    uint32_t count = process_list(pids, MAX_PROCESSES);

    display_print("\n");
    display_print("  PID  STATE       NAME\n");
    display_print("  ---  ----------  ----------------\n");

    if (count == 0) {
        /* No scheduler yet - show placeholder */
        display_print("    1  RUNNING     kernel\n");
        display_print("    2  RUNNING     shell\n");
        display_print("\n");
        display_print("  (Process scheduler not fully active yet)\n");
    } else {
        for (uint32_t i = 0; i < count; i++) {
            process_t *proc = process_get(pids[i]);
            if (proc) {
                /* PID */
                display_print("  ");
                char num[8];
                int_to_str(proc->pid, num);
                /* Right-align PID in 3 chars */
                int len = 0;
                while (num[len]) len++;
                for (int p = 0; p < 3 - len; p++) display_putchar(' ');
                display_print(num);
                display_print("  ");

                /* State */
                const char *state = process_state_name(proc->state);
                display_print(state);
                /* Pad to 10 chars */
                int slen = 0;
                while (state[slen]) slen++;
                for (int p = 0; p < 10 - slen; p++) display_putchar(' ');
                display_print("  ");

                /* Name */
                display_print(proc->name);
                display_print("\n");
            }
        }
    }

    display_print("\n");

    /* Show process count */
    display_print("Total processes: ");
    char num[8];
    int_to_str(count > 0 ? count : 2, num);
    display_print(num);
    display_print("\n");

    return 0;
}

/* kill - Terminate a process by PID */
int builtin_kill(int argc, char **argv) {
    if (argc < 2) {
        display_print("kill: usage: kill <pid>\n");
        return 1;
    }

    int pid = str_to_int(argv[1]);
    if (pid <= 0) {
        display_print("kill: invalid PID: ");
        display_print(argv[1]);
        display_print("\n");
        return 1;
    }

    /* Prevent killing kernel or shell */
    if (pid == 1) {
        display_print("kill: cannot kill kernel (PID 1)\n");
        return 1;
    }
    if (pid == 2) {
        display_print("kill: cannot kill shell (PID 2)\n");
        return 1;
    }

    /* Try to kill the process */
    int result = process_kill((uint32_t)pid);

    if (result == 0) {
        display_print("Process ");
        display_print(argv[1]);
        display_print(" terminated.\n");
    } else {
        display_print("kill: process ");
        display_print(argv[1]);
        display_print(" not found or cannot be killed\n");
        return 1;
    }

    return 0;
}

/*
 * ===========================================================================
 * CLAUDE AI ASSISTANT - THE KILLER FEATURE!
 * ===========================================================================
 */

/* claude - AI assistant command */
int builtin_claude(int argc, char **argv) {
    /* Initialize AI system */
    ai_init();

    if (argc == 1) {
        /* No arguments - enter interactive mode */
        ai_interactive_mode();
        return 0;
    }

    /* Build question from arguments */
    char question[256];
    int pos = 0;

    for (int i = 1; i < argc && pos < 250; i++) {
        const char *arg = argv[i];
        while (*arg && pos < 250) {
            question[pos++] = *arg++;
        }
        if (i < argc - 1 && pos < 250) {
            question[pos++] = ' ';
        }
    }
    question[pos] = '\0';

    /* Process the question */
    char response[512];
    ai_process_question(question, response);
    display_print(response);

    return 0;
}
