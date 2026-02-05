/**
 * ClaudeOS System Call Interface - syscall.h
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: INT 0x80 system call interface
 */

#ifndef _CLAUDEOS_SYSCALL_H
#define _CLAUDEOS_SYSCALL_H

#include "types.h"

/* System call numbers */
#define SYS_EXIT        0   /* Exit process */
#define SYS_READ        1   /* Read from file descriptor */
#define SYS_WRITE       2   /* Write to file descriptor */
#define SYS_GETPID      3   /* Get process ID */
#define SYS_SLEEP       4   /* Sleep for milliseconds */
#define SYS_YIELD       5   /* Yield CPU */
#define SYS_FORK        6   /* Fork process (not implemented) */
#define SYS_EXEC        7   /* Execute program (not implemented) */
#define SYS_WAIT        8   /* Wait for child (not implemented) */
#define SYS_OPEN        9   /* Open file */
#define SYS_CLOSE       10  /* Close file */
#define SYS_STAT        11  /* Get file status */
#define SYS_MKDIR       12  /* Create directory */
#define SYS_RMDIR       13  /* Remove directory */
#define SYS_UNLINK      14  /* Delete file */
#define SYS_CHDIR       15  /* Change directory */
#define SYS_GETCWD      16  /* Get current directory */
#define SYS_GETTIME     17  /* Get system time */
#define SYS_UPTIME      18  /* Get system uptime */

/* System call count */
#define SYS_MAX         19

/* Standard file descriptors */
#define STDIN_FD        0
#define STDOUT_FD       1
#define STDERR_FD       2

/* System call result codes */
#define SYSCALL_SUCCESS     0
#define SYSCALL_ERROR      -1
#define SYSCALL_ENOENT     -2   /* No such file or directory */
#define SYSCALL_EBADF      -3   /* Bad file descriptor */
#define SYSCALL_EINVAL     -4   /* Invalid argument */
#define SYSCALL_ENOMEM     -5   /* Out of memory */
#define SYSCALL_EACCES     -6   /* Permission denied */
#define SYSCALL_EEXIST     -7   /* File exists */
#define SYSCALL_ENOTSUP    -8   /* Not supported */

/**
 * Initialize system call handler
 * Registers INT 0x80 handler
 */
void syscall_init(void);

/**
 * System call handler (called from ISR)
 * @param syscall_num System call number (from EAX)
 * @param arg1 First argument (EBX)
 * @param arg2 Second argument (ECX)
 * @param arg3 Third argument (EDX)
 * @return Result value (returned in EAX)
 */
int32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

/*
 * User-space system call wrappers
 * These would be used by programs to invoke system calls
 */

/**
 * Exit the current process
 * @param status Exit code
 */
void sys_exit(int32_t status);

/**
 * Read from a file descriptor
 * @param fd File descriptor
 * @param buf Buffer to read into
 * @param count Number of bytes to read
 * @return Number of bytes read, or error code
 */
int32_t sys_read(int32_t fd, void* buf, uint32_t count);

/**
 * Write to a file descriptor
 * @param fd File descriptor
 * @param buf Buffer to write from
 * @param count Number of bytes to write
 * @return Number of bytes written, or error code
 */
int32_t sys_write(int32_t fd, const void* buf, uint32_t count);

/**
 * Get current process ID
 * @return Process ID
 */
int32_t sys_getpid(void);

/**
 * Sleep for specified milliseconds
 * @param ms Milliseconds to sleep
 * @return 0 on success
 */
int32_t sys_sleep(uint32_t ms);

/**
 * Yield CPU to other processes
 * @return 0 on success
 */
int32_t sys_yield(void);

/**
 * Get system uptime in seconds
 * @return Uptime in seconds
 */
int32_t sys_uptime(void);

/**
 * Get system time (ticks since boot)
 * @return Tick count
 */
uint64_t sys_gettime(void);

#endif /* _CLAUDEOS_SYSCALL_H */
