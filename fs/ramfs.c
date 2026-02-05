/*
 * ClaudeOS RAM Filesystem - Implementation
 * Worker1 - Shell+FS Claude
 *
 * An in-memory filesystem that provides the initial directory structure.
 * All data is stored in RAM - nothing persists across reboots.
 */

#include "vfs.h"

/* Simple string functions */
static void str_copy(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int str_len(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

/*
 * ===========================================================================
 * Static Allocation (no malloc yet)
 * ===========================================================================
 */

/* Pre-allocated nodes and children arrays */
static fs_node_t nodes[FS_MAX_FILES];
static fs_node_t *children_arrays[FS_MAX_FILES][FS_MAX_CHILDREN];
static int node_count = 0;

/* Pre-allocated file content buffers */
#define FILE_BUF_SIZE 1024
#define MAX_FILE_BUFS 16
static char file_buffers[MAX_FILE_BUFS][FILE_BUF_SIZE];
static int file_buf_count = 0;

static uint32_t next_inode = 1;

/*
 * ===========================================================================
 * Node Creation
 * ===========================================================================
 */

static fs_node_t *alloc_node(void) {
    if (node_count >= FS_MAX_FILES) {
        return NULL;
    }
    fs_node_t *node = &nodes[node_count];

    /* Clear the node */
    for (int i = 0; i < (int)sizeof(fs_node_t); i++) {
        ((char *)node)[i] = 0;
    }

    /* Set up children array pointer */
    node->children = children_arrays[node_count];
    for (int i = 0; i < FS_MAX_CHILDREN; i++) {
        node->children[i] = NULL;
    }

    node_count++;
    return node;
}

static char *alloc_file_buffer(void) {
    if (file_buf_count >= MAX_FILE_BUFS) {
        return NULL;
    }
    char *buf = file_buffers[file_buf_count++];
    /* Clear buffer */
    for (int i = 0; i < FILE_BUF_SIZE; i++) {
        buf[i] = 0;
    }
    return buf;
}

/* Create a directory node */
fs_node_t *vfs_create_dir(fs_node_t *parent, const char *name) {
    fs_node_t *node = alloc_node();
    if (!node) return NULL;

    str_copy(node->name, name, FS_NAME_MAX);
    node->type = FS_DIRECTORY;
    node->inode = next_inode++;
    node->parent = parent;
    node->child_count = 0;
    node->size = 0;
    node->data = NULL;
    node->ops = NULL;

    /* Add to parent's children */
    if (parent && parent->child_count < FS_MAX_CHILDREN) {
        parent->children[parent->child_count++] = node;
    }

    return node;
}

/* Create a file node with content */
fs_node_t *vfs_create_file(fs_node_t *parent, const char *name, const char *content) {
    fs_node_t *node = alloc_node();
    if (!node) return NULL;

    str_copy(node->name, name, FS_NAME_MAX);
    node->type = FS_FILE;
    node->inode = next_inode++;
    node->parent = parent;
    node->child_count = 0;
    node->ops = NULL;

    /* Copy content to buffer */
    if (content) {
        char *buf = alloc_file_buffer();
        if (buf) {
            int len = str_len(content);
            if (len >= FILE_BUF_SIZE) len = FILE_BUF_SIZE - 1;
            for (int i = 0; i < len; i++) {
                buf[i] = content[i];
            }
            buf[len] = '\0';
            node->data = buf;
            node->size = len;
        }
    } else {
        node->data = NULL;
        node->size = 0;
    }

    /* Add to parent's children */
    if (parent && parent->child_count < FS_MAX_CHILDREN) {
        parent->children[parent->child_count++] = node;
    }

    return node;
}

/*
 * ===========================================================================
 * Initialize the RAM Filesystem
 * ===========================================================================
 */

/* External function to set root */
extern void vfs_set_root(fs_node_t *root);

void ramfs_init(void) {
    /* Create root directory */
    fs_node_t *root = alloc_node();
    str_copy(root->name, "/", FS_NAME_MAX);
    root->type = FS_DIRECTORY;
    root->inode = next_inode++;
    root->parent = root;  /* Root is its own parent */
    root->child_count = 0;

    vfs_set_root(root);

    /*
     * Create initial directory structure:
     *
     * /
     * ├── bin/
     * ├── dev/
     * ├── etc/
     * │   ├── motd
     * │   └── hostname
     * ├── home/
     * │   └── claude/
     * │       ├── .profile
     * │       └── welcome.txt
     * └── tmp/
     */

    /* /bin - for future external programs */
    vfs_create_dir(root, "bin");

    /* /dev - device files (future) */
    vfs_create_dir(root, "dev");

    /* /etc - configuration files */
    fs_node_t *etc = vfs_create_dir(root, "etc");
    vfs_create_file(etc, "motd",
        "================================================================================\n"
        "                    Welcome to ClaudeOS v0.2.0\n"
        "                  Built by the MultiClaude Team\n"
        "================================================================================\n"
        "\n"
        "This operating system was collaboratively built by multiple Claude instances:\n"
        "  - Kernel Claude: Boot, memory, interrupts, keyboard, timer, processes\n"
        "  - Shell+FS Claude: Shell, commands, filesystem, AI assistant\n"
        "  - Boss Claude: Architecture, coordination, integration\n"
        "\n"
        "NEW IN v0.2.0:\n"
        "  - Type 'claude' for your AI assistant - ask anything about ClaudeOS!\n"
        "  - Real uptime tracking with timer\n"
        "  - Process management with 'ps' and 'kill'\n"
        "  - Sleep command for delays\n"
        "\n"
        "Type 'help' to see available commands.\n"
        "Type 'claude' to chat with your AI assistant!\n"
        "\n"
    );
    vfs_create_file(etc, "hostname", "claudeos\n");
    vfs_create_file(etc, "version", "ClaudeOS 0.2.0 (built by MultiClaude Team)\n");

    /* /home - user home directories */
    fs_node_t *home = vfs_create_dir(root, "home");
    fs_node_t *claude_home = vfs_create_dir(home, "claude");
    vfs_create_file(claude_home, ".profile",
        "# ClaudeOS Shell Profile\n"
        "export PATH=/bin:/usr/bin\n"
        "export HOME=/home/claude\n"
        "export USER=claude\n"
    );
    vfs_create_file(claude_home, "welcome.txt",
        "Hello! I'm Claude, your friendly AI assistant.\n"
        "This is your home directory on ClaudeOS.\n"
        "\n"
        "TALK TO ME!\n"
        "  claude                 - Enter interactive chat mode\n"
        "  claude how do I <x>   - Ask how to do something\n"
        "  claude what is <x>    - Learn about a command\n"
        "  claude system status  - Get system information\n"
        "\n"
        "Basic commands:\n"
        "  ls          - List files in current directory\n"
        "  cat <file>  - Display file contents\n"
        "  pwd         - Print working directory\n"
        "  cd <dir>    - Change directory\n"
        "  help        - Show all commands\n"
        "\n"
        "Have fun exploring! I'm here to help!\n"
    );

    /* /tmp - temporary files */
    fs_node_t *tmp = vfs_create_dir(root, "tmp");
    vfs_create_file(tmp, "test.txt", "This is a test file in /tmp.\n");

    /* /usr - user programs (future) */
    fs_node_t *usr = vfs_create_dir(root, "usr");
    vfs_create_dir(usr, "bin");
    vfs_create_dir(usr, "lib");
}
