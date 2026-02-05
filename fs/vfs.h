/*
 * ClaudeOS Virtual Filesystem - Header
 * Worker1 - Shell+FS Claude
 *
 * Provides a unified interface for filesystem operations.
 * Currently supports in-memory ramfs, can be extended for disk-based fs.
 */

#ifndef CLAUDEOS_VFS_H
#define CLAUDEOS_VFS_H

#include "../include/types.h"

/* File types */
#define FS_FILE      0x01
#define FS_DIRECTORY 0x02
#define FS_CHARDEV   0x03
#define FS_BLOCKDEV  0x04
#define FS_SYMLINK   0x05

/* Open flags */
#define O_RDONLY     0x0000
#define O_WRONLY     0x0001
#define O_RDWR       0x0002
#define O_CREAT      0x0100
#define O_TRUNC      0x0200
#define O_APPEND     0x0400

/* Seek origins */
#define SEEK_SET     0
#define SEEK_CUR     1
#define SEEK_END     2

/* Limits */
#define FS_NAME_MAX  64
#define FS_PATH_MAX  256
#define FS_MAX_FILES 128
#define FS_MAX_CHILDREN 32

/* Forward declaration */
struct fs_node;

/* File operations function pointers */
typedef struct {
    int (*open)(struct fs_node *node, int flags);
    int (*close)(struct fs_node *node);
    ssize_t (*read)(struct fs_node *node, void *buf, size_t size, size_t offset);
    ssize_t (*write)(struct fs_node *node, const void *buf, size_t size, size_t offset);
    struct fs_node* (*readdir)(struct fs_node *node, int index);
    struct fs_node* (*finddir)(struct fs_node *node, const char *name);
} fs_ops_t;

/* Filesystem node (inode-like structure) */
typedef struct fs_node {
    char name[FS_NAME_MAX];     /* Filename */
    uint8_t type;               /* FS_FILE, FS_DIRECTORY, etc. */
    uint32_t flags;             /* Permissions, etc. */
    uint32_t size;              /* File size in bytes */
    uint32_t inode;             /* Inode number */

    /* For in-memory filesystem */
    void *data;                 /* File contents (for files) */
    struct fs_node *parent;     /* Parent directory */
    struct fs_node **children;  /* Children (for directories) */
    int child_count;            /* Number of children */

    /* Operations */
    fs_ops_t *ops;

    /* Device info (for device files) */
    uint32_t major;
    uint32_t minor;
} fs_node_t;

/* File stat structure */
typedef struct {
    uint32_t st_ino;
    uint8_t  st_type;
    uint32_t st_size;
    uint32_t st_nlink;
} fs_stat_t;

/* Directory entry */
typedef struct {
    char name[FS_NAME_MAX];
    uint32_t inode;
    uint8_t type;
} fs_dirent_t;

/*
 * ===========================================================================
 * VFS Interface Functions
 * ===========================================================================
 */

/* Initialize the virtual filesystem */
void vfs_init(void);

/* Get the root filesystem node */
fs_node_t *vfs_get_root(void);

/* Path operations */
fs_node_t *vfs_lookup(const char *path);
fs_node_t *vfs_lookup_from(fs_node_t *start, const char *path);

/* File operations */
int vfs_open(const char *path, int flags);
int vfs_close(int fd);
ssize_t vfs_read(int fd, void *buf, size_t size);
ssize_t vfs_write(int fd, const void *buf, size_t size);
int vfs_seek(int fd, int offset, int whence);

/* Directory operations */
fs_dirent_t *vfs_readdir(const char *path, int index);
int vfs_mkdir(const char *path);

/* Stat */
int vfs_stat(const char *path, fs_stat_t *stat);

/* Create file (for ramfs) */
fs_node_t *vfs_create_file(fs_node_t *parent, const char *name, const char *content);
fs_node_t *vfs_create_dir(fs_node_t *parent, const char *name);

/*
 * ===========================================================================
 * Utility Functions
 * ===========================================================================
 */

/* Get absolute path from node */
int vfs_get_path(fs_node_t *node, char *buf, size_t size);

/* Check if path is absolute */
int vfs_is_absolute(const char *path);

/* Normalize path (resolve . and ..) */
int vfs_normalize_path(const char *path, char *out, size_t size);

#endif /* CLAUDEOS_VFS_H */
