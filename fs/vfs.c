/*
 * ClaudeOS Virtual Filesystem - Implementation
 * Worker1 - Shell+FS Claude
 */

#include "vfs.h"

/* Simple string functions (no libc in kernel) */
static int str_len(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static int str_cmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static void str_cpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

static void str_ncpy(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/*
 * ===========================================================================
 * File Descriptor Table
 * ===========================================================================
 */

#define MAX_OPEN_FILES 16

typedef struct {
    fs_node_t *node;
    int flags;
    size_t offset;
    int in_use;
} file_desc_t;

static file_desc_t fd_table[MAX_OPEN_FILES];

static int fd_alloc(void) {
    /* Skip 0, 1, 2 for stdin/stdout/stderr */
    for (int i = 3; i < MAX_OPEN_FILES; i++) {
        if (!fd_table[i].in_use) {
            fd_table[i].in_use = 1;
            return i;
        }
    }
    return -1;
}

static void fd_free(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES) {
        fd_table[fd].in_use = 0;
        fd_table[fd].node = NULL;
        fd_table[fd].offset = 0;
    }
}

/*
 * ===========================================================================
 * Filesystem Root
 * ===========================================================================
 */

static fs_node_t *fs_root = NULL;

fs_node_t *vfs_get_root(void) {
    return fs_root;
}

void vfs_set_root(fs_node_t *root) {
    fs_root = root;
}

/*
 * ===========================================================================
 * Path Operations
 * ===========================================================================
 */

int vfs_is_absolute(const char *path) {
    return path && path[0] == '/';
}

/* Split path into components */
static const char *path_next_component(const char *path, char *component, int max_len) {
    /* Skip leading slashes */
    while (*path == '/') path++;
    if (!*path) return NULL;

    /* Copy component */
    int i = 0;
    while (*path && *path != '/' && i < max_len - 1) {
        component[i++] = *path++;
    }
    component[i] = '\0';

    return path;
}

/* Lookup a node by path */
fs_node_t *vfs_lookup(const char *path) {
    if (!path || !fs_root) return NULL;

    /* Handle root */
    if (path[0] == '/' && path[1] == '\0') {
        return fs_root;
    }

    return vfs_lookup_from(fs_root, path);
}

fs_node_t *vfs_lookup_from(fs_node_t *start, const char *path) {
    if (!start || !path) return NULL;

    fs_node_t *current = start;
    char component[FS_NAME_MAX];

    /* If absolute path, start from root */
    if (path[0] == '/') {
        current = fs_root;
        path++;
    }

    while ((path = path_next_component(path, component, FS_NAME_MAX)) != NULL) {
        /* Handle . and .. */
        if (component[0] == '.' && component[1] == '\0') {
            continue;  /* Current directory */
        }
        if (component[0] == '.' && component[1] == '.' && component[2] == '\0') {
            if (current->parent) {
                current = current->parent;
            }
            continue;
        }

        /* Must be a directory to traverse into */
        if (current->type != FS_DIRECTORY) {
            return NULL;
        }

        /* Find child */
        if (current->ops && current->ops->finddir) {
            current = current->ops->finddir(current, component);
        } else {
            /* Default finddir for ramfs */
            fs_node_t *found = NULL;
            for (int i = 0; i < current->child_count; i++) {
                if (str_cmp(current->children[i]->name, component) == 0) {
                    found = current->children[i];
                    break;
                }
            }
            current = found;
        }

        if (!current) return NULL;
    }

    return current;
}

/*
 * ===========================================================================
 * File Operations
 * ===========================================================================
 */

int vfs_open(const char *path, int flags) {
    fs_node_t *node = vfs_lookup(path);
    if (!node) return -1;

    int fd = fd_alloc();
    if (fd < 0) return -1;

    fd_table[fd].node = node;
    fd_table[fd].flags = flags;
    fd_table[fd].offset = 0;

    if (node->ops && node->ops->open) {
        node->ops->open(node, flags);
    }

    return fd;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].in_use) {
        return -1;
    }

    fs_node_t *node = fd_table[fd].node;
    if (node && node->ops && node->ops->close) {
        node->ops->close(node);
    }

    fd_free(fd);
    return 0;
}

ssize_t vfs_read(int fd, void *buf, size_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].in_use) {
        return -1;
    }

    fs_node_t *node = fd_table[fd].node;
    if (!node) return -1;

    ssize_t read = 0;

    if (node->ops && node->ops->read) {
        read = node->ops->read(node, buf, size, fd_table[fd].offset);
    } else if (node->type == FS_FILE && node->data) {
        /* Default read for ramfs */
        size_t avail = node->size - fd_table[fd].offset;
        if (size > avail) size = avail;
        char *src = (char *)node->data + fd_table[fd].offset;
        char *dst = (char *)buf;
        for (size_t i = 0; i < size; i++) {
            dst[i] = src[i];
        }
        read = size;
    }

    if (read > 0) {
        fd_table[fd].offset += read;
    }

    return read;
}

ssize_t vfs_write(int fd, const void *buf, size_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].in_use) {
        return -1;
    }

    fs_node_t *node = fd_table[fd].node;
    if (!node) return -1;

    ssize_t written = 0;

    if (node->ops && node->ops->write) {
        written = node->ops->write(node, buf, size, fd_table[fd].offset);
    }
    /* Note: default ramfs doesn't support write - would need memory allocation */

    if (written > 0) {
        fd_table[fd].offset += written;
    }

    return written;
}

int vfs_seek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].in_use) {
        return -1;
    }

    fs_node_t *node = fd_table[fd].node;
    if (!node) return -1;

    size_t new_offset;

    switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = fd_table[fd].offset + offset;
            break;
        case SEEK_END:
            new_offset = node->size + offset;
            break;
        default:
            return -1;
    }

    fd_table[fd].offset = new_offset;
    return new_offset;
}

/*
 * ===========================================================================
 * Directory Operations
 * ===========================================================================
 */

static fs_dirent_t dirent_buf;

fs_dirent_t *vfs_readdir(const char *path, int index) {
    fs_node_t *node = vfs_lookup(path);
    if (!node || node->type != FS_DIRECTORY) {
        return NULL;
    }

    fs_node_t *child = NULL;

    if (node->ops && node->ops->readdir) {
        child = node->ops->readdir(node, index);
    } else if (index < node->child_count) {
        child = node->children[index];
    }

    if (!child) return NULL;

    str_ncpy(dirent_buf.name, child->name, FS_NAME_MAX);
    dirent_buf.inode = child->inode;
    dirent_buf.type = child->type;

    return &dirent_buf;
}

/*
 * ===========================================================================
 * Stat
 * ===========================================================================
 */

int vfs_stat(const char *path, fs_stat_t *stat) {
    fs_node_t *node = vfs_lookup(path);
    if (!node || !stat) return -1;

    stat->st_ino = node->inode;
    stat->st_type = node->type;
    stat->st_size = node->size;
    stat->st_nlink = 1;

    return 0;
}

/*
 * ===========================================================================
 * VFS Init
 * ===========================================================================
 */

/* External: ramfs will set up the root */
extern void ramfs_init(void);

void vfs_init(void) {
    /* Clear file descriptor table */
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        fd_table[i].in_use = 0;
        fd_table[i].node = NULL;
        fd_table[i].offset = 0;
        fd_table[i].flags = 0;
    }

    /* Reserve stdin, stdout, stderr */
    fd_table[0].in_use = 1;  /* stdin */
    fd_table[1].in_use = 1;  /* stdout */
    fd_table[2].in_use = 1;  /* stderr */

    /* Initialize the RAM filesystem */
    ramfs_init();
}
