/**
 * unix_socket_ops.c - Horizon kernel Unix domain socket operations
 * 
 * This file contains the implementation of Unix domain socket operations.
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

/* External functions */
extern struct unix_socket *unix_socket_create(int type);
extern void unix_socket_destroy(struct unix_socket *sock);
extern int unix_socket_bind(struct unix_socket *sock, const char *addr, int len);
extern int unix_socket_connect(struct unix_socket *sock, const char *addr, int len);
extern int unix_socket_listen(struct unix_socket *sock, int backlog);
extern struct unix_socket *unix_socket_accept(struct unix_socket *sock);
extern ssize_t unix_socket_send(struct unix_socket *sock, const void *buf, size_t len, int flags);
extern ssize_t unix_socket_recv(struct unix_socket *sock, void *buf, size_t len, int flags);
extern ssize_t unix_socket_sendto(struct unix_socket *sock, const void *buf, size_t len, int flags, const char *addr, int addr_len);
extern ssize_t unix_socket_recvfrom(struct unix_socket *sock, void *buf, size_t len, int flags, char *addr, int *addr_len);
extern int unix_socket_shutdown(struct unix_socket *sock, int how);
extern int unix_socket_pair(int type, struct unix_socket **sock1, struct unix_socket **sock2);

/**
 * Create a Unix domain socket
 * 
 * @param sock The socket
 * @return 0 on success, or a negative error code
 */
static int unix_create(socket_t *sock) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Check the socket type */
    int type;
    
    switch (sock->type) {
        case SOCK_STREAM:
            type = UNIX_STREAM;
            break;
        
        case SOCK_DGRAM:
            type = UNIX_DGRAM;
            break;
        
        case SOCK_SEQPACKET:
            type = UNIX_SEQPACKET;
            break;
        
        default:
            return -1;
    }
    
    /* Create a Unix socket */
    struct unix_socket *unix_sock = unix_socket_create(type);
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Set the private data */
    sock->private_data = unix_sock;
    
    return 0;
}

/**
 * Release a Unix domain socket
 * 
 * @param sock The socket
 * @return 0 on success, or a negative error code
 */
static int unix_release(socket_t *sock) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Destroy the Unix socket */
    unix_socket_destroy(unix_sock);
    
    /* Clear the private data */
    sock->private_data = NULL;
    
    return 0;
}

/**
 * Bind a Unix domain socket
 * 
 * @param sock The socket
 * @param addr The address
 * @param addrlen The address length
 * @return 0 on success, or a negative error code
 */
static int unix_bind(socket_t *sock, const struct sockaddr *addr, socklen_t addrlen) {
    /* Check parameters */
    if (sock == NULL || addr == NULL) {
        return -1;
    }
    
    /* Check the address family */
    if (addr->sa_family != AF_UNIX) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Get the Unix address */
    const struct sockaddr_un *unix_addr = (const struct sockaddr_un *)addr;
    
    /* Bind the Unix socket */
    return unix_socket_bind(unix_sock, unix_addr->sun_path, strlen(unix_addr->sun_path));
}

/**
 * Connect a Unix domain socket
 * 
 * @param sock The socket
 * @param addr The address
 * @param addrlen The address length
 * @return 0 on success, or a negative error code
 */
static int unix_connect(socket_t *sock, const struct sockaddr *addr, socklen_t addrlen) {
    /* Check parameters */
    if (sock == NULL || addr == NULL) {
        return -1;
    }
    
    /* Check the address family */
    if (addr->sa_family != AF_UNIX) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Get the Unix address */
    const struct sockaddr_un *unix_addr = (const struct sockaddr_un *)addr;
    
    /* Connect the Unix socket */
    return unix_socket_connect(unix_sock, unix_addr->sun_path, strlen(unix_addr->sun_path));
}

/**
 * Listen for connections on a Unix domain socket
 * 
 * @param sock The socket
 * @param backlog The backlog
 * @return 0 on success, or a negative error code
 */
static int unix_listen(socket_t *sock, int backlog) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Listen on the Unix socket */
    return unix_socket_listen(unix_sock, backlog);
}

/**
 * Accept a connection on a Unix domain socket
 * 
 * @param sock The socket
 * @param addr The address
 * @param addrlen The address length
 * @return The new socket, or NULL on error
 */
static socket_t *unix_accept(socket_t *sock, struct sockaddr *addr, socklen_t *addrlen) {
    /* Check parameters */
    if (sock == NULL) {
        return NULL;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return NULL;
    }
    
    /* Accept a connection */
    struct unix_socket *new_unix_sock = unix_socket_accept(unix_sock);
    
    if (new_unix_sock == NULL) {
        return NULL;
    }
    
    /* Create a new socket */
    socket_t *new_sock = kmalloc(sizeof(socket_t), MEM_KERNEL | MEM_ZERO);
    
    if (new_sock == NULL) {
        unix_socket_destroy(new_unix_sock);
        return NULL;
    }
    
    /* Initialize the socket */
    new_sock->domain = sock->domain;
    new_sock->type = sock->type;
    new_sock->protocol = sock->protocol;
    new_sock->state = SOCK_CONNECTED;
    new_sock->ops = sock->ops;
    new_sock->private_data = new_unix_sock;
    
    /* Set the address */
    if (addr != NULL && addrlen != NULL) {
        /* This would be implemented with actual peer address */
        *addrlen = 0;
    }
    
    return new_sock;
}

/**
 * Get the name of a Unix domain socket
 * 
 * @param sock The socket
 * @param addr The address
 * @param addrlen The address length
 * @return 0 on success, or a negative error code
 */
static int unix_getname(socket_t *sock, struct sockaddr *addr, socklen_t *addrlen) {
    /* Check parameters */
    if (sock == NULL || addr == NULL || addrlen == NULL) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* This would be implemented with actual socket name */
    *addrlen = 0;
    
    return 0;
}

/**
 * Send a message on a Unix domain socket
 * 
 * @param sock The socket
 * @param msg The message
 * @param flags The flags
 * @return The number of bytes sent, or a negative error code
 */
static ssize_t unix_sendmsg(socket_t *sock, const struct msghdr *msg, int flags) {
    /* Check parameters */
    if (sock == NULL || msg == NULL) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Check if the socket is connected */
    if (sock->state != SOCK_CONNECTED && msg->msg_name == NULL) {
        return -1;
    }
    
    /* Send the message */
    ssize_t total = 0;
    
    for (size_t i = 0; i < msg->msg_iovlen; i++) {
        /* Get the I/O vector */
        const struct iovec *iov = &msg->msg_iov[i];
        
        /* Send the data */
        ssize_t ret;
        
        if (msg->msg_name != NULL) {
            /* Get the address */
            const struct sockaddr_un *addr = msg->msg_name;
            
            /* Send to the address */
            ret = unix_socket_sendto(unix_sock, iov->iov_base, iov->iov_len, flags, addr->sun_path, strlen(addr->sun_path));
        } else {
            /* Send to the connected socket */
            ret = unix_socket_send(unix_sock, iov->iov_base, iov->iov_len, flags);
        }
        
        if (ret < 0) {
            return ret;
        }
        
        /* Add to the total */
        total += ret;
    }
    
    return total;
}

/**
 * Receive a message from a Unix domain socket
 * 
 * @param sock The socket
 * @param msg The message
 * @param flags The flags
 * @return The number of bytes received, or a negative error code
 */
static ssize_t unix_recvmsg(socket_t *sock, struct msghdr *msg, int flags) {
    /* Check parameters */
    if (sock == NULL || msg == NULL) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Receive the message */
    ssize_t total = 0;
    
    for (size_t i = 0; i < msg->msg_iovlen; i++) {
        /* Get the I/O vector */
        struct iovec *iov = &msg->msg_iov[i];
        
        /* Receive the data */
        ssize_t ret;
        
        if (msg->msg_name != NULL) {
            /* Get the address */
            struct sockaddr_un *addr = msg->msg_name;
            
            /* Receive from the address */
            ret = unix_socket_recvfrom(unix_sock, iov->iov_base, iov->iov_len, flags, addr->sun_path, &msg->msg_namelen);
        } else {
            /* Receive from the connected socket */
            ret = unix_socket_recv(unix_sock, iov->iov_base, iov->iov_len, flags);
        }
        
        if (ret < 0) {
            return ret;
        }
        
        /* Add to the total */
        total += ret;
    }
    
    return total;
}

/**
 * Shut down a Unix domain socket
 * 
 * @param sock The socket
 * @param how The how
 * @return 0 on success, or a negative error code
 */
static int unix_shutdown(socket_t *sock, int how) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Shut down the Unix socket */
    return unix_socket_shutdown(unix_sock, how);
}

/**
 * Poll a Unix domain socket
 * 
 * @param sock The socket
 * @param wait The wait table
 * @return The poll mask
 */
static unsigned int unix_poll(socket_t *sock, struct poll_table_struct *wait) {
    /* Check parameters */
    if (sock == NULL) {
        return POLLERR;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return POLLERR;
    }
    
    /* Poll the Unix socket */
    unsigned int mask = 0;
    
    /* Add the wait queues to the poll table */
    if (wait != NULL) {
        poll_wait(NULL, &unix_sock->wait_read, wait);
        poll_wait(NULL, &unix_sock->wait_write, wait);
    }
    
    /* Check if the socket is readable */
    if (!list_empty(&unix_sock->messages)) {
        mask |= POLLIN | POLLRDNORM;
    }
    
    /* Check if the socket is writable */
    mask |= POLLOUT | POLLWRNORM;
    
    /* Check if the socket is disconnected */
    if (unix_sock->state == UNIX_DISCONNECTED) {
        mask |= POLLHUP;
    }
    
    return mask;
}

/**
 * Get socket options
 * 
 * @param sock The socket
 * @param level The level
 * @param optname The option name
 * @param optval The option value
 * @param optlen The option length
 * @return 0 on success, or a negative error code
 */
static int unix_getsockopt(socket_t *sock, int level, int optname, void *optval, socklen_t *optlen) {
    /* Check parameters */
    if (sock == NULL || optval == NULL || optlen == NULL) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Get the socket option */
    /* This would be implemented with actual socket options */
    
    return -1;
}

/**
 * Set socket options
 * 
 * @param sock The socket
 * @param level The level
 * @param optname The option name
 * @param optval The option value
 * @param optlen The option length
 * @return 0 on success, or a negative error code
 */
static int unix_setsockopt(socket_t *sock, int level, int optname, const void *optval, socklen_t optlen) {
    /* Check parameters */
    if (sock == NULL || optval == NULL) {
        return -1;
    }
    
    /* Get the Unix socket */
    struct unix_socket *unix_sock = sock->private_data;
    
    if (unix_sock == NULL) {
        return -1;
    }
    
    /* Set the socket option */
    /* This would be implemented with actual socket options */
    
    return -1;
}

/**
 * Create a pair of connected sockets
 * 
 * @param domain The domain
 * @param type The type
 * @param protocol The protocol
 * @param sock1 The first socket
 * @param sock2 The second socket
 * @return 0 on success, or a negative error code
 */
int unix_socketpair(int domain, int type, int protocol, socket_t *sock1, socket_t *sock2) {
    /* Check parameters */
    if (sock1 == NULL || sock2 == NULL) {
        return -1;
    }
    
    /* Check the domain */
    if (domain != AF_UNIX) {
        return -1;
    }
    
    /* Check the socket type */
    int unix_type;
    
    switch (type) {
        case SOCK_STREAM:
            unix_type = UNIX_STREAM;
            break;
        
        case SOCK_DGRAM:
            unix_type = UNIX_DGRAM;
            break;
        
        case SOCK_SEQPACKET:
            unix_type = UNIX_SEQPACKET;
            break;
        
        default:
            return -1;
    }
    
    /* Create a pair of Unix sockets */
    struct unix_socket *unix_sock1, *unix_sock2;
    
    if (unix_socket_pair(unix_type, &unix_sock1, &unix_sock2) < 0) {
        return -1;
    }
    
    /* Initialize the sockets */
    sock1->domain = domain;
    sock1->type = type;
    sock1->protocol = protocol;
    sock1->state = SOCK_CONNECTED;
    sock1->private_data = unix_sock1;
    
    sock2->domain = domain;
    sock2->type = type;
    sock2->protocol = protocol;
    sock2->state = SOCK_CONNECTED;
    sock2->private_data = unix_sock2;
    
    return 0;
}

/* Unix domain socket operations */
const struct socket_ops unix_socket_ops = {
    .create = unix_create,
    .release = unix_release,
    .bind = unix_bind,
    .connect = unix_connect,
    .listen = unix_listen,
    .accept = unix_accept,
    .getsockname = unix_getname,
    .getpeername = unix_getname,
    .sendmsg = unix_sendmsg,
    .recvmsg = unix_recvmsg,
    .shutdown = unix_shutdown,
    .poll = unix_poll,
    .getsockopt = unix_getsockopt,
    .setsockopt = unix_setsockopt,
};
