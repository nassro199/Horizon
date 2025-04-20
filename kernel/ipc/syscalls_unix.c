/**
 * syscalls_unix.c - Horizon kernel Unix domain socket system calls
 * 
 * This file contains the implementation of the Unix domain socket system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/ipc.h>
#include <horizon/net.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* External functions */
extern int unix_socketpair(int domain, int type, int protocol, socket_t *sock1, socket_t *sock2);
extern const struct socket_ops unix_socket_ops;

/**
 * Register Unix domain socket operations
 */
void unix_socket_register(void) {
    /* Register Unix domain socket operations */
    socket_register_ops(AF_UNIX, &unix_socket_ops);
}

/**
 * Create a pair of connected sockets
 * 
 * @param domain The domain
 * @param type The type
 * @param protocol The protocol
 * @param sv The socket vector
 * @return 0 on success, or a negative error code
 */
long sys_socketpair(long domain, long type, long protocol, long sv, long unused1, long unused2) {
    /* Check parameters */
    if (domain != AF_UNIX) {
        return -1;
    }
    
    /* Create the sockets */
    socket_t *sock1 = sock_create(domain, type, protocol);
    
    if (sock1 == NULL) {
        return -1;
    }
    
    socket_t *sock2 = sock_create(domain, type, protocol);
    
    if (sock2 == NULL) {
        sock_close(sock1);
        return -1;
    }
    
    /* Create the socket pair */
    if (unix_socketpair(domain, type, protocol, sock1, sock2) < 0) {
        sock_close(sock1);
        sock_close(sock2);
        return -1;
    }
    
    /* Create the file descriptors */
    file_t *file1, *file2;
    int fd1 = file_anon_fd(sock1, &file1);
    
    if (fd1 < 0) {
        sock_close(sock1);
        sock_close(sock2);
        return -1;
    }
    
    int fd2 = file_anon_fd(sock2, &file2);
    
    if (fd2 < 0) {
        file_close(file1);
        sock_close(sock2);
        return -1;
    }
    
    /* Set the file descriptors */
    int *fds = (int *)sv;
    fds[0] = fd1;
    fds[1] = fd2;
    
    return 0;
}

/**
 * Register Unix domain socket system calls
 */
void unix_syscalls_init(void) {
    /* Register Unix domain socket operations */
    unix_socket_register();
    
    /* Register Unix domain socket system calls */
    syscall_register(SYS_SOCKETPAIR, sys_socketpair);
}
