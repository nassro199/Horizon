/**
 * socket_adv.c - Horizon kernel advanced socket operations
 * 
 * This file contains the implementation of advanced socket operations.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/net.h>
#include <horizon/fs/vfs.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Send a message on a socket
 * 
 * @param sock The socket
 * @param msg The message
 * @param flags The flags
 * @return The number of bytes sent, or a negative error code
 */
ssize_t sock_sendmsg(socket_t *sock, const struct msghdr *msg, int flags) {
    /* Check parameters */
    if (sock == NULL || msg == NULL) {
        return -1;
    }
    
    /* Check if the socket is connected */
    if (sock->state != SOCK_CONNECTED && sock->type != SOCK_DGRAM) {
        return -1;
    }
    
    /* Check if the socket has a send operation */
    if (sock->ops == NULL || sock->ops->sendmsg == NULL) {
        return -1;
    }
    
    /* Send the message */
    return sock->ops->sendmsg(sock, msg, flags);
}

/**
 * Receive a message from a socket
 * 
 * @param sock The socket
 * @param msg The message
 * @param flags The flags
 * @return The number of bytes received, or a negative error code
 */
ssize_t sock_recvmsg(socket_t *sock, struct msghdr *msg, int flags) {
    /* Check parameters */
    if (sock == NULL || msg == NULL) {
        return -1;
    }
    
    /* Check if the socket is connected */
    if (sock->state != SOCK_CONNECTED && sock->type != SOCK_DGRAM) {
        return -1;
    }
    
    /* Check if the socket has a receive operation */
    if (sock->ops == NULL || sock->ops->recvmsg == NULL) {
        return -1;
    }
    
    /* Receive the message */
    return sock->ops->recvmsg(sock, msg, flags);
}

/**
 * Send a message on a socket to a specific address
 * 
 * @param sock The socket
 * @param msg The message
 * @param flags The flags
 * @param addr The address
 * @param addrlen The address length
 * @return The number of bytes sent, or a negative error code
 */
ssize_t sock_sendto(socket_t *sock, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen) {
    /* Check parameters */
    if (sock == NULL || buf == NULL) {
        return -1;
    }
    
    /* Create a message header */
    struct msghdr msg;
    struct iovec iov;
    
    /* Initialize the I/O vector */
    iov.iov_base = (void *)buf;
    iov.iov_len = len;
    
    /* Initialize the message header */
    msg.msg_name = (void *)addr;
    msg.msg_namelen = addrlen;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    
    /* Send the message */
    return sock_sendmsg(sock, &msg, flags);
}

/**
 * Receive a message from a socket from a specific address
 * 
 * @param sock The socket
 * @param buf The buffer
 * @param len The buffer length
 * @param flags The flags
 * @param addr The address
 * @param addrlen The address length
 * @return The number of bytes received, or a negative error code
 */
ssize_t sock_recvfrom(socket_t *sock, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen) {
    /* Check parameters */
    if (sock == NULL || buf == NULL) {
        return -1;
    }
    
    /* Create a message header */
    struct msghdr msg;
    struct iovec iov;
    
    /* Initialize the I/O vector */
    iov.iov_base = buf;
    iov.iov_len = len;
    
    /* Initialize the message header */
    msg.msg_name = addr;
    msg.msg_namelen = addrlen != NULL ? *addrlen : 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    
    /* Receive the message */
    ssize_t ret = sock_recvmsg(sock, &msg, flags);
    
    /* Set the address length */
    if (ret >= 0 && addrlen != NULL) {
        *addrlen = msg.msg_namelen;
    }
    
    return ret;
}

/**
 * Send data on a socket
 * 
 * @param sock The socket
 * @param buf The buffer
 * @param len The buffer length
 * @param flags The flags
 * @return The number of bytes sent, or a negative error code
 */
ssize_t sock_send(socket_t *sock, const void *buf, size_t len, int flags) {
    /* Send to the connected address */
    return sock_sendto(sock, buf, len, flags, NULL, 0);
}

/**
 * Receive data from a socket
 * 
 * @param sock The socket
 * @param buf The buffer
 * @param len The buffer length
 * @param flags The flags
 * @return The number of bytes received, or a negative error code
 */
ssize_t sock_recv(socket_t *sock, void *buf, size_t len, int flags) {
    /* Receive from the connected address */
    return sock_recvfrom(sock, buf, len, flags, NULL, NULL);
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
int sock_getsockopt(socket_t *sock, int level, int optname, void *optval, socklen_t *optlen) {
    /* Check parameters */
    if (sock == NULL || optval == NULL || optlen == NULL) {
        return -1;
    }
    
    /* Check if the socket has a getsockopt operation */
    if (sock->ops == NULL || sock->ops->getsockopt == NULL) {
        return -1;
    }
    
    /* Get the socket option */
    return sock->ops->getsockopt(sock, level, optname, optval, optlen);
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
int sock_setsockopt(socket_t *sock, int level, int optname, const void *optval, socklen_t optlen) {
    /* Check parameters */
    if (sock == NULL || optval == NULL) {
        return -1;
    }
    
    /* Check if the socket has a setsockopt operation */
    if (sock->ops == NULL || sock->ops->setsockopt == NULL) {
        return -1;
    }
    
    /* Set the socket option */
    return sock->ops->setsockopt(sock, level, optname, optval, optlen);
}

/**
 * Get socket name
 * 
 * @param sock The socket
 * @param addr The address
 * @param addrlen The address length
 * @return 0 on success, or a negative error code
 */
int sock_getsockname(socket_t *sock, struct sockaddr *addr, socklen_t *addrlen) {
    /* Check parameters */
    if (sock == NULL || addr == NULL || addrlen == NULL) {
        return -1;
    }
    
    /* Check if the socket has a getsockname operation */
    if (sock->ops == NULL || sock->ops->getsockname == NULL) {
        return -1;
    }
    
    /* Get the socket name */
    return sock->ops->getsockname(sock, addr, addrlen);
}

/**
 * Get peer name
 * 
 * @param sock The socket
 * @param addr The address
 * @param addrlen The address length
 * @return 0 on success, or a negative error code
 */
int sock_getpeername(socket_t *sock, struct sockaddr *addr, socklen_t *addrlen) {
    /* Check parameters */
    if (sock == NULL || addr == NULL || addrlen == NULL) {
        return -1;
    }
    
    /* Check if the socket has a getpeername operation */
    if (sock->ops == NULL || sock->ops->getpeername == NULL) {
        return -1;
    }
    
    /* Get the peer name */
    return sock->ops->getpeername(sock, addr, addrlen);
}

/**
 * Shut down part of a full-duplex connection
 * 
 * @param sock The socket
 * @param how The how
 * @return 0 on success, or a negative error code
 */
int sock_shutdown(socket_t *sock, int how) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Check if the socket has a shutdown operation */
    if (sock->ops == NULL || sock->ops->shutdown == NULL) {
        return -1;
    }
    
    /* Shut down the socket */
    return sock->ops->shutdown(sock, how);
}

/**
 * Listen for connections on a socket
 * 
 * @param sock The socket
 * @param backlog The backlog
 * @return 0 on success, or a negative error code
 */
int sock_listen(socket_t *sock, int backlog) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Check if the socket has a listen operation */
    if (sock->ops == NULL || sock->ops->listen == NULL) {
        return -1;
    }
    
    /* Listen on the socket */
    return sock->ops->listen(sock, backlog);
}

/**
 * Accept a connection on a socket
 * 
 * @param sock The socket
 * @param addr The address
 * @param addrlen The address length
 * @return The new socket, or a negative error code
 */
socket_t *sock_accept(socket_t *sock, struct sockaddr *addr, socklen_t *addrlen) {
    /* Check parameters */
    if (sock == NULL) {
        return NULL;
    }
    
    /* Check if the socket has an accept operation */
    if (sock->ops == NULL || sock->ops->accept == NULL) {
        return NULL;
    }
    
    /* Accept a connection */
    return sock->ops->accept(sock, addr, addrlen);
}

/**
 * Connect a socket
 * 
 * @param sock The socket
 * @param addr The address
 * @param addrlen The address length
 * @return 0 on success, or a negative error code
 */
int sock_connect(socket_t *sock, const struct sockaddr *addr, socklen_t addrlen) {
    /* Check parameters */
    if (sock == NULL || addr == NULL) {
        return -1;
    }
    
    /* Check if the socket has a connect operation */
    if (sock->ops == NULL || sock->ops->connect == NULL) {
        return -1;
    }
    
    /* Connect the socket */
    return sock->ops->connect(sock, addr, addrlen);
}

/**
 * Bind a socket
 * 
 * @param sock The socket
 * @param addr The address
 * @param addrlen The address length
 * @return 0 on success, or a negative error code
 */
int sock_bind(socket_t *sock, const struct sockaddr *addr, socklen_t addrlen) {
    /* Check parameters */
    if (sock == NULL || addr == NULL) {
        return -1;
    }
    
    /* Check if the socket has a bind operation */
    if (sock->ops == NULL || sock->ops->bind == NULL) {
        return -1;
    }
    
    /* Bind the socket */
    return sock->ops->bind(sock, addr, addrlen);
}

/**
 * Create a socket
 * 
 * @param domain The domain
 * @param type The type
 * @param protocol The protocol
 * @return The socket, or a negative error code
 */
socket_t *sock_create(int domain, int type, int protocol) {
    /* Check if the domain is supported */
    if (domain < 0 || domain >= MAX_SOCK_DOMAIN) {
        return NULL;
    }
    
    /* Get the socket operations */
    const struct socket_ops *ops = sock_get_ops(domain, type, protocol);
    
    if (ops == NULL) {
        return NULL;
    }
    
    /* Allocate a new socket */
    socket_t *sock = kmalloc(sizeof(socket_t), MEM_KERNEL | MEM_ZERO);
    
    if (sock == NULL) {
        return NULL;
    }
    
    /* Initialize the socket */
    sock->domain = domain;
    sock->type = type;
    sock->protocol = protocol;
    sock->state = SOCK_UNCONNECTED;
    sock->ops = ops;
    
    /* Create the socket */
    if (ops->create != NULL) {
        int ret = ops->create(sock);
        
        if (ret < 0) {
            kfree(sock);
            return NULL;
        }
    }
    
    return sock;
}

/**
 * Close a socket
 * 
 * @param sock The socket
 * @return 0 on success, or a negative error code
 */
int sock_close(socket_t *sock) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Close the socket */
    if (sock->ops != NULL && sock->ops->close != NULL) {
        sock->ops->close(sock);
    }
    
    /* Free the socket */
    kfree(sock);
    
    return 0;
}

/**
 * Poll a socket
 * 
 * @param sock The socket
 * @param wait The wait table
 * @return The poll mask
 */
unsigned int sock_poll(socket_t *sock, struct poll_table_struct *wait) {
    /* Check parameters */
    if (sock == NULL) {
        return 0;
    }
    
    /* Check if the socket has a poll operation */
    if (sock->ops == NULL || sock->ops->poll == NULL) {
        return 0;
    }
    
    /* Poll the socket */
    return sock->ops->poll(sock, wait);
}

/**
 * Get socket error
 * 
 * @param sock The socket
 * @return The error code
 */
int sock_error(socket_t *sock) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Get the error */
    return sock->err;
}

/**
 * Set socket error
 * 
 * @param sock The socket
 * @param err The error code
 */
void sock_set_error(socket_t *sock, int err) {
    /* Check parameters */
    if (sock == NULL) {
        return;
    }
    
    /* Set the error */
    sock->err = err;
}
