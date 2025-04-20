/**
 * user.h - User space definitions
 * 
 * This file contains definitions for user space support.
 */

#ifndef _KERNEL_USER_H
#define _KERNEL_USER_H

#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/vmm.h>
#include <horizon/fs.h>
#include <horizon/security.h>

/* User process structure */
typedef struct user_process {
    u32 pid;                    /* Process ID */
    char name[32];              /* Process name */
    vm_context_t *context;      /* Virtual memory context */
    void *entry_point;          /* Entry point */
    void *stack;                /* Stack pointer */
    u32 stack_size;             /* Stack size */
    security_context_t *security; /* Security context */
    file_t **files;             /* Open files */
    u32 max_files;              /* Maximum number of open files */
    struct user_process *parent; /* Parent process */
    struct user_process *children; /* Child processes */
    struct user_process *next;  /* Next process in list */
} user_process_t;

/* User space functions */
void user_init(void);
user_process_t *user_create_process(const char *name, const char *path, char *const argv[], char *const envp[]);
int user_destroy_process(user_process_t *process);
int user_exec(user_process_t *process, const char *path, char *const argv[], char *const envp[]);
int user_fork(user_process_t *process);
int user_wait(user_process_t *process, int *status);
int user_exit(user_process_t *process, int status);
int user_open(user_process_t *process, const char *path, u32 flags);
int user_close(user_process_t *process, int fd);
ssize_t user_read(user_process_t *process, int fd, void *buf, size_t count);
ssize_t user_write(user_process_t *process, int fd, const void *buf, size_t count);
off_t user_seek(user_process_t *process, int fd, off_t offset, int whence);
int user_ioctl(user_process_t *process, int fd, u32 request, void *arg);
int user_mmap(user_process_t *process, void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int user_munmap(user_process_t *process, void *addr, size_t length);
int user_setuid(user_process_t *process, u32 uid);
int user_setgid(user_process_t *process, u32 gid);
int user_getuid(user_process_t *process);
int user_getgid(user_process_t *process);
int user_geteuid(user_process_t *process);
int user_getegid(user_process_t *process);
int user_seteuid(user_process_t *process, u32 euid);
int user_setegid(user_process_t *process, u32 egid);
int user_chdir(user_process_t *process, const char *path);
int user_getcwd(user_process_t *process, char *buf, size_t size);
int user_mkdir(user_process_t *process, const char *path, u32 mode);
int user_rmdir(user_process_t *process, const char *path);
int user_link(user_process_t *process, const char *oldpath, const char *newpath);
int user_unlink(user_process_t *process, const char *path);
int user_chmod(user_process_t *process, const char *path, u32 mode);
int user_chown(user_process_t *process, const char *path, u32 uid, u32 gid);
int user_stat(user_process_t *process, const char *path, void *buf);
int user_fstat(user_process_t *process, int fd, void *buf);
int user_lstat(user_process_t *process, const char *path, void *buf);
int user_access(user_process_t *process, const char *path, u32 mode);
int user_pipe(user_process_t *process, int pipefd[2]);
int user_dup(user_process_t *process, int oldfd);
int user_dup2(user_process_t *process, int oldfd, int newfd);
int user_fcntl(user_process_t *process, int fd, u32 cmd, void *arg);
int user_socket(user_process_t *process, int domain, int type, int protocol);
int user_bind(user_process_t *process, int sockfd, const void *addr, u32 addrlen);
int user_connect(user_process_t *process, int sockfd, const void *addr, u32 addrlen);
int user_listen(user_process_t *process, int sockfd, int backlog);
int user_accept(user_process_t *process, int sockfd, void *addr, u32 *addrlen);
int user_send(user_process_t *process, int sockfd, const void *buf, size_t len, int flags);
int user_recv(user_process_t *process, int sockfd, void *buf, size_t len, int flags);
int user_sendto(user_process_t *process, int sockfd, const void *buf, size_t len, int flags, const void *dest_addr, u32 addrlen);
int user_recvfrom(user_process_t *process, int sockfd, void *buf, size_t len, int flags, void *src_addr, u32 *addrlen);
int user_shutdown(user_process_t *process, int sockfd, int how);
int user_setsockopt(user_process_t *process, int sockfd, int level, int optname, const void *optval, u32 optlen);
int user_getsockopt(user_process_t *process, int sockfd, int level, int optname, void *optval, u32 *optlen);
int user_getsockname(user_process_t *process, int sockfd, void *addr, u32 *addrlen);
int user_getpeername(user_process_t *process, int sockfd, void *addr, u32 *addrlen);

#endif /* _KERNEL_USER_H */
