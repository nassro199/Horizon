/**
 * unix_socket.c - Horizon kernel Unix domain socket implementation
 * 
 * This file contains the implementation of Unix domain sockets.
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

/* Unix socket types */
#define UNIX_STREAM     1
#define UNIX_DGRAM      2
#define UNIX_SEQPACKET  3

/* Unix socket states */
#define UNIX_UNCONNECTED    0
#define UNIX_CONNECTING     1
#define UNIX_CONNECTED      2
#define UNIX_DISCONNECTED   3

/* Unix socket address */
struct unix_address {
    int len;                /* Length of address */
    char name[108];         /* Socket name */
};

/* Unix socket message */
struct unix_message {
    struct list_head list;  /* List of messages */
    int len;                /* Message length */
    char *data;             /* Message data */
};

/* Unix socket */
struct unix_socket {
    struct list_head list;          /* List of Unix sockets */
    int type;                       /* Socket type */
    int state;                      /* Socket state */
    struct unix_address *address;   /* Socket address */
    struct unix_socket *peer;       /* Peer socket */
    struct list_head messages;      /* List of messages */
    struct wait_queue_head wait_read;  /* Wait queue for readers */
    struct wait_queue_head wait_write; /* Wait queue for writers */
    struct mutex mutex;             /* Mutex */
    int refcount;                   /* Reference count */
};

/* List of Unix sockets */
static LIST_HEAD(unix_socket_list);

/* Unix socket mutex */
static struct mutex unix_socket_mutex;

/**
 * Initialize the Unix socket subsystem
 */
void unix_socket_init(void) {
    /* Initialize the mutex */
    mutex_init(&unix_socket_mutex);
}

/**
 * Create a Unix socket
 * 
 * @param type The socket type
 * @return The Unix socket, or NULL on error
 */
struct unix_socket *unix_socket_create(int type) {
    /* Check the type */
    if (type != UNIX_STREAM && type != UNIX_DGRAM && type != UNIX_SEQPACKET) {
        return NULL;
    }
    
    /* Allocate a new Unix socket */
    struct unix_socket *sock = kmalloc(sizeof(struct unix_socket), MEM_KERNEL | MEM_ZERO);
    
    if (sock == NULL) {
        return NULL;
    }
    
    /* Initialize the Unix socket */
    sock->type = type;
    sock->state = UNIX_UNCONNECTED;
    sock->address = NULL;
    sock->peer = NULL;
    INIT_LIST_HEAD(&sock->messages);
    init_waitqueue_head(&sock->wait_read);
    init_waitqueue_head(&sock->wait_write);
    mutex_init(&sock->mutex);
    sock->refcount = 1;
    
    /* Add the Unix socket to the list */
    mutex_lock(&unix_socket_mutex);
    list_add(&sock->list, &unix_socket_list);
    mutex_unlock(&unix_socket_mutex);
    
    return sock;
}

/**
 * Destroy a Unix socket
 * 
 * @param sock The Unix socket
 */
void unix_socket_destroy(struct unix_socket *sock) {
    /* Check parameters */
    if (sock == NULL) {
        return;
    }
    
    /* Lock the mutex */
    mutex_lock(&unix_socket_mutex);
    
    /* Decrement the reference count */
    sock->refcount--;
    
    /* Check if the socket should be freed */
    if (sock->refcount == 0) {
        /* Remove the socket from the list */
        list_del(&sock->list);
        
        /* Free the address */
        if (sock->address != NULL) {
            kfree(sock->address);
        }
        
        /* Free all messages */
        struct unix_message *msg, *tmp;
        
        list_for_each_entry_safe(msg, tmp, &sock->messages, list) {
            /* Remove the message from the list */
            list_del(&msg->list);
            
            /* Free the message data */
            kfree(msg->data);
            
            /* Free the message */
            kfree(msg);
        }
        
        /* Free the socket */
        kfree(sock);
    }
    
    /* Unlock the mutex */
    mutex_unlock(&unix_socket_mutex);
}

/**
 * Find a Unix socket by address
 * 
 * @param addr The socket address
 * @param len The address length
 * @return The Unix socket, or NULL if not found
 */
struct unix_socket *unix_socket_find(const char *addr, int len) {
    /* Check parameters */
    if (addr == NULL || len <= 0 || len > 108) {
        return NULL;
    }
    
    /* Find the Unix socket */
    struct unix_socket *sock;
    
    mutex_lock(&unix_socket_mutex);
    
    list_for_each_entry(sock, &unix_socket_list, list) {
        if (sock->address != NULL && sock->address->len == len && memcmp(sock->address->name, addr, len) == 0) {
            /* Increment the reference count */
            sock->refcount++;
            
            mutex_unlock(&unix_socket_mutex);
            return sock;
        }
    }
    
    mutex_unlock(&unix_socket_mutex);
    
    return NULL;
}

/**
 * Bind a Unix socket to an address
 * 
 * @param sock The Unix socket
 * @param addr The socket address
 * @param len The address length
 * @return 0 on success, or a negative error code
 */
int unix_socket_bind(struct unix_socket *sock, const char *addr, int len) {
    /* Check parameters */
    if (sock == NULL || addr == NULL || len <= 0 || len > 108) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&sock->mutex);
    
    /* Check if the socket is already bound */
    if (sock->address != NULL) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Check if the address is already in use */
    if (unix_socket_find(addr, len) != NULL) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Allocate a new address */
    struct unix_address *address = kmalloc(sizeof(struct unix_address), MEM_KERNEL | MEM_ZERO);
    
    if (address == NULL) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Initialize the address */
    address->len = len;
    memcpy(address->name, addr, len);
    
    /* Set the address */
    sock->address = address;
    
    /* Unlock the mutex */
    mutex_unlock(&sock->mutex);
    
    return 0;
}

/**
 * Connect a Unix socket to another socket
 * 
 * @param sock The Unix socket
 * @param addr The socket address
 * @param len The address length
 * @return 0 on success, or a negative error code
 */
int unix_socket_connect(struct unix_socket *sock, const char *addr, int len) {
    /* Check parameters */
    if (sock == NULL || addr == NULL || len <= 0 || len > 108) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&sock->mutex);
    
    /* Check if the socket is already connected */
    if (sock->state == UNIX_CONNECTED) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Find the peer socket */
    struct unix_socket *peer = unix_socket_find(addr, len);
    
    if (peer == NULL) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Check if the peer socket is listening */
    if (peer->type == UNIX_STREAM && peer->state != UNIX_UNCONNECTED) {
        unix_socket_destroy(peer);
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Connect the sockets */
    sock->peer = peer;
    sock->state = UNIX_CONNECTED;
    
    /* Unlock the mutex */
    mutex_unlock(&sock->mutex);
    
    return 0;
}

/**
 * Listen for connections on a Unix socket
 * 
 * @param sock The Unix socket
 * @param backlog The backlog
 * @return 0 on success, or a negative error code
 */
int unix_socket_listen(struct unix_socket *sock, int backlog) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&sock->mutex);
    
    /* Check if the socket is bound */
    if (sock->address == NULL) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Check if the socket is already listening */
    if (sock->state != UNIX_UNCONNECTED) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Set the socket state */
    sock->state = UNIX_CONNECTING;
    
    /* Unlock the mutex */
    mutex_unlock(&sock->mutex);
    
    return 0;
}

/**
 * Accept a connection on a Unix socket
 * 
 * @param sock The Unix socket
 * @return The new Unix socket, or NULL on error
 */
struct unix_socket *unix_socket_accept(struct unix_socket *sock) {
    /* Check parameters */
    if (sock == NULL) {
        return NULL;
    }
    
    /* Lock the mutex */
    mutex_lock(&sock->mutex);
    
    /* Check if the socket is listening */
    if (sock->state != UNIX_CONNECTING) {
        mutex_unlock(&sock->mutex);
        return NULL;
    }
    
    /* Wait for a connection */
    while (sock->peer == NULL) {
        /* Unlock the mutex */
        mutex_unlock(&sock->mutex);
        
        /* Wait for a connection */
        wait_event_interruptible(sock->wait_read, sock->peer != NULL);
        
        /* Lock the mutex */
        mutex_lock(&sock->mutex);
    }
    
    /* Get the peer socket */
    struct unix_socket *peer = sock->peer;
    
    /* Create a new socket */
    struct unix_socket *new_sock = unix_socket_create(sock->type);
    
    if (new_sock == NULL) {
        mutex_unlock(&sock->mutex);
        return NULL;
    }
    
    /* Connect the sockets */
    new_sock->peer = peer;
    new_sock->state = UNIX_CONNECTED;
    peer->peer = new_sock;
    peer->state = UNIX_CONNECTED;
    
    /* Reset the listening socket */
    sock->peer = NULL;
    
    /* Unlock the mutex */
    mutex_unlock(&sock->mutex);
    
    return new_sock;
}

/**
 * Send a message on a Unix socket
 * 
 * @param sock The Unix socket
 * @param buf The buffer
 * @param len The buffer length
 * @param flags The flags
 * @return The number of bytes sent, or a negative error code
 */
ssize_t unix_socket_send(struct unix_socket *sock, const void *buf, size_t len, int flags) {
    /* Check parameters */
    if (sock == NULL || buf == NULL || len == 0) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&sock->mutex);
    
    /* Check if the socket is connected */
    if (sock->state != UNIX_CONNECTED) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Get the peer socket */
    struct unix_socket *peer = sock->peer;
    
    if (peer == NULL) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Lock the peer mutex */
    mutex_lock(&peer->mutex);
    
    /* Allocate a new message */
    struct unix_message *msg = kmalloc(sizeof(struct unix_message), MEM_KERNEL | MEM_ZERO);
    
    if (msg == NULL) {
        mutex_unlock(&peer->mutex);
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Allocate the message data */
    msg->data = kmalloc(len, MEM_KERNEL);
    
    if (msg->data == NULL) {
        kfree(msg);
        mutex_unlock(&peer->mutex);
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Initialize the message */
    msg->len = len;
    memcpy(msg->data, buf, len);
    
    /* Add the message to the peer's message list */
    list_add_tail(&msg->list, &peer->messages);
    
    /* Wake up readers */
    wake_up_interruptible(&peer->wait_read);
    
    /* Unlock the peer mutex */
    mutex_unlock(&peer->mutex);
    
    /* Unlock the mutex */
    mutex_unlock(&sock->mutex);
    
    return len;
}

/**
 * Receive a message from a Unix socket
 * 
 * @param sock The Unix socket
 * @param buf The buffer
 * @param len The buffer length
 * @param flags The flags
 * @return The number of bytes received, or a negative error code
 */
ssize_t unix_socket_recv(struct unix_socket *sock, void *buf, size_t len, int flags) {
    /* Check parameters */
    if (sock == NULL || buf == NULL || len == 0) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&sock->mutex);
    
    /* Check if the socket is connected */
    if (sock->state != UNIX_CONNECTED) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Wait for a message */
    while (list_empty(&sock->messages)) {
        /* Check if the socket is non-blocking */
        if (flags & MSG_DONTWAIT) {
            mutex_unlock(&sock->mutex);
            return -1;
        }
        
        /* Unlock the mutex */
        mutex_unlock(&sock->mutex);
        
        /* Wait for a message */
        wait_event_interruptible(sock->wait_read, !list_empty(&sock->messages));
        
        /* Lock the mutex */
        mutex_lock(&sock->mutex);
    }
    
    /* Get the first message */
    struct unix_message *msg = list_first_entry(&sock->messages, struct unix_message, list);
    
    /* Remove the message from the list */
    list_del(&msg->list);
    
    /* Copy the message data */
    size_t copy_len = MIN(len, msg->len);
    memcpy(buf, msg->data, copy_len);
    
    /* Free the message data */
    kfree(msg->data);
    
    /* Free the message */
    kfree(msg);
    
    /* Unlock the mutex */
    mutex_unlock(&sock->mutex);
    
    return copy_len;
}

/**
 * Send a message to a Unix socket
 * 
 * @param sock The Unix socket
 * @param buf The buffer
 * @param len The buffer length
 * @param flags The flags
 * @param addr The socket address
 * @param addr_len The address length
 * @return The number of bytes sent, or a negative error code
 */
ssize_t unix_socket_sendto(struct unix_socket *sock, const void *buf, size_t len, int flags, const char *addr, int addr_len) {
    /* Check parameters */
    if (sock == NULL || buf == NULL || len == 0) {
        return -1;
    }
    
    /* Check if the socket is connected */
    if (sock->state == UNIX_CONNECTED) {
        return unix_socket_send(sock, buf, len, flags);
    }
    
    /* Check if the socket is a datagram socket */
    if (sock->type != UNIX_DGRAM) {
        return -1;
    }
    
    /* Check if the address is provided */
    if (addr == NULL || addr_len <= 0 || addr_len > 108) {
        return -1;
    }
    
    /* Find the peer socket */
    struct unix_socket *peer = unix_socket_find(addr, addr_len);
    
    if (peer == NULL) {
        return -1;
    }
    
    /* Lock the peer mutex */
    mutex_lock(&peer->mutex);
    
    /* Allocate a new message */
    struct unix_message *msg = kmalloc(sizeof(struct unix_message), MEM_KERNEL | MEM_ZERO);
    
    if (msg == NULL) {
        mutex_unlock(&peer->mutex);
        unix_socket_destroy(peer);
        return -1;
    }
    
    /* Allocate the message data */
    msg->data = kmalloc(len, MEM_KERNEL);
    
    if (msg->data == NULL) {
        kfree(msg);
        mutex_unlock(&peer->mutex);
        unix_socket_destroy(peer);
        return -1;
    }
    
    /* Initialize the message */
    msg->len = len;
    memcpy(msg->data, buf, len);
    
    /* Add the message to the peer's message list */
    list_add_tail(&msg->list, &peer->messages);
    
    /* Wake up readers */
    wake_up_interruptible(&peer->wait_read);
    
    /* Unlock the peer mutex */
    mutex_unlock(&peer->mutex);
    
    /* Destroy the peer reference */
    unix_socket_destroy(peer);
    
    return len;
}

/**
 * Receive a message from a Unix socket
 * 
 * @param sock The Unix socket
 * @param buf The buffer
 * @param len The buffer length
 * @param flags The flags
 * @param addr The socket address
 * @param addr_len The address length
 * @return The number of bytes received, or a negative error code
 */
ssize_t unix_socket_recvfrom(struct unix_socket *sock, void *buf, size_t len, int flags, char *addr, int *addr_len) {
    /* Check parameters */
    if (sock == NULL || buf == NULL || len == 0) {
        return -1;
    }
    
    /* Check if the socket is connected */
    if (sock->state == UNIX_CONNECTED) {
        return unix_socket_recv(sock, buf, len, flags);
    }
    
    /* Check if the socket is a datagram socket */
    if (sock->type != UNIX_DGRAM) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&sock->mutex);
    
    /* Wait for a message */
    while (list_empty(&sock->messages)) {
        /* Check if the socket is non-blocking */
        if (flags & MSG_DONTWAIT) {
            mutex_unlock(&sock->mutex);
            return -1;
        }
        
        /* Unlock the mutex */
        mutex_unlock(&sock->mutex);
        
        /* Wait for a message */
        wait_event_interruptible(sock->wait_read, !list_empty(&sock->messages));
        
        /* Lock the mutex */
        mutex_lock(&sock->mutex);
    }
    
    /* Get the first message */
    struct unix_message *msg = list_first_entry(&sock->messages, struct unix_message, list);
    
    /* Remove the message from the list */
    list_del(&msg->list);
    
    /* Copy the message data */
    size_t copy_len = MIN(len, msg->len);
    memcpy(buf, msg->data, copy_len);
    
    /* Set the address */
    if (addr != NULL && addr_len != NULL) {
        /* This would be implemented with actual peer address */
        *addr_len = 0;
    }
    
    /* Free the message data */
    kfree(msg->data);
    
    /* Free the message */
    kfree(msg);
    
    /* Unlock the mutex */
    mutex_unlock(&sock->mutex);
    
    return copy_len;
}

/**
 * Shut down a Unix socket
 * 
 * @param sock The Unix socket
 * @param how The how
 * @return 0 on success, or a negative error code
 */
int unix_socket_shutdown(struct unix_socket *sock, int how) {
    /* Check parameters */
    if (sock == NULL) {
        return -1;
    }
    
    /* Lock the mutex */
    mutex_lock(&sock->mutex);
    
    /* Check if the socket is connected */
    if (sock->state != UNIX_CONNECTED) {
        mutex_unlock(&sock->mutex);
        return -1;
    }
    
    /* Set the socket state */
    sock->state = UNIX_DISCONNECTED;
    
    /* Get the peer socket */
    struct unix_socket *peer = sock->peer;
    
    if (peer != NULL) {
        /* Lock the peer mutex */
        mutex_lock(&peer->mutex);
        
        /* Set the peer state */
        peer->state = UNIX_DISCONNECTED;
        
        /* Wake up readers and writers */
        wake_up_interruptible(&peer->wait_read);
        wake_up_interruptible(&peer->wait_write);
        
        /* Unlock the peer mutex */
        mutex_unlock(&peer->mutex);
    }
    
    /* Wake up readers and writers */
    wake_up_interruptible(&sock->wait_read);
    wake_up_interruptible(&sock->wait_write);
    
    /* Unlock the mutex */
    mutex_unlock(&sock->mutex);
    
    return 0;
}

/**
 * Create a pair of connected Unix sockets
 * 
 * @param type The socket type
 * @param sock1 The first socket
 * @param sock2 The second socket
 * @return 0 on success, or a negative error code
 */
int unix_socket_pair(int type, struct unix_socket **sock1, struct unix_socket **sock2) {
    /* Check parameters */
    if (sock1 == NULL || sock2 == NULL) {
        return -1;
    }
    
    /* Create the first socket */
    struct unix_socket *s1 = unix_socket_create(type);
    
    if (s1 == NULL) {
        return -1;
    }
    
    /* Create the second socket */
    struct unix_socket *s2 = unix_socket_create(type);
    
    if (s2 == NULL) {
        unix_socket_destroy(s1);
        return -1;
    }
    
    /* Connect the sockets */
    s1->peer = s2;
    s1->state = UNIX_CONNECTED;
    s2->peer = s1;
    s2->state = UNIX_CONNECTED;
    
    /* Set the sockets */
    *sock1 = s1;
    *sock2 = s2;
    
    return 0;
}
