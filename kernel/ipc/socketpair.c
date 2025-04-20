/**
 * socketpair.c - Horizon kernel socket pair implementation
 * 
 * This file contains the implementation of socket pairs.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/ipc.h>
#include <horizon/fs/vfs.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/net.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Create a pair of connected sockets
 * 
 * @param domain The domain
 * @param type The type
 * @param protocol The protocol
 * @param sv The socket vector
 * @return 0 on success, or a negative error code
 */
int socketpair_create(int domain, int type, int protocol, int sv[2]) {
    /* Check parameters */
    if (sv == NULL) {
        return -1;
    }
    
    /* Check the domain */
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
    
    /* Connect the sockets */
    /* This would be implemented with actual socket pair creation */
    
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
    
    /* Set the socket vector */
    sv[0] = fd1;
    sv[1] = fd2;
    
    return 0;
}
