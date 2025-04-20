/**
 * net.c - Networking subsystem implementation
 * 
 * This file contains the implementation of the networking subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/net.h>
#include <horizon/mm.h>

/* Network device list */
static net_device_t *net_devices = NULL;

/* Network protocol list */
static net_protocol_t *net_protocols = NULL;

/* Initialize the networking subsystem */
void net_init(void)
{
    /* Initialize the device and protocol lists */
    net_devices = NULL;
    net_protocols = NULL;
}

/* Create a socket */
int socket_create(int family, int type, int protocol, socket_t **sock)
{
    /* Allocate a socket structure */
    socket_t *new_sock = kmalloc(sizeof(socket_t), MEM_KERNEL | MEM_ZERO);
    
    if (new_sock == NULL) {
        return -1;
    }
    
    /* Initialize the socket */
    new_sock->type = type;
    new_sock->protocol = protocol;
    new_sock->state = 0;
    new_sock->ops = NULL;
    new_sock->private = NULL;
    
    /* Set the socket operations based on the family and type */
    switch (family) {
        case AF_INET:
            /* Set up Internet Protocol socket operations */
            /* This would be implemented with actual protocol handlers */
            break;
        
        case AF_LOCAL:
            /* Set up local socket operations */
            /* This would be implemented with actual protocol handlers */
            break;
        
        default:
            /* Unsupported address family */
            kfree(new_sock);
            return -1;
    }
    
    /* Return the socket */
    *sock = new_sock;
    
    return 0;
}

/* Bind a socket to an address */
int socket_bind(socket_t *sock, const sockaddr_t *addr, int addrlen)
{
    if (sock == NULL || addr == NULL) {
        return -1;
    }
    
    /* Check if the socket has bind operations */
    if (sock->ops == NULL || sock->ops->bind == NULL) {
        return -1;
    }
    
    /* Call the socket's bind operation */
    return sock->ops->bind(sock, addr, addrlen);
}

/* Connect a socket to an address */
int socket_connect(socket_t *sock, const sockaddr_t *addr, int addrlen)
{
    if (sock == NULL || addr == NULL) {
        return -1;
    }
    
    /* Check if the socket has connect operations */
    if (sock->ops == NULL || sock->ops->connect == NULL) {
        return -1;
    }
    
    /* Call the socket's connect operation */
    return sock->ops->connect(sock, addr, addrlen);
}

/* Listen for connections on a socket */
int socket_listen(socket_t *sock, int backlog)
{
    if (sock == NULL) {
        return -1;
    }
    
    /* Check if the socket has listen operations */
    if (sock->ops == NULL || sock->ops->listen == NULL) {
        return -1;
    }
    
    /* Call the socket's listen operation */
    return sock->ops->listen(sock, backlog);
}

/* Accept a connection on a socket */
int socket_accept(socket_t *sock, sockaddr_t *addr, int *addrlen, socket_t **newsock)
{
    if (sock == NULL || newsock == NULL) {
        return -1;
    }
    
    /* Check if the socket has accept operations */
    if (sock->ops == NULL || sock->ops->accept == NULL) {
        return -1;
    }
    
    /* Allocate a new socket structure */
    socket_t *new_sock = kmalloc(sizeof(socket_t), MEM_KERNEL | MEM_ZERO);
    
    if (new_sock == NULL) {
        return -1;
    }
    
    /* Initialize the new socket */
    new_sock->type = sock->type;
    new_sock->protocol = sock->protocol;
    new_sock->state = 0;
    new_sock->ops = sock->ops;
    new_sock->private = NULL;
    
    /* Call the socket's accept operation */
    int result = sock->ops->accept(sock, addr, addrlen);
    
    if (result < 0) {
        kfree(new_sock);
        return result;
    }
    
    /* Return the new socket */
    *newsock = new_sock;
    
    return 0;
}

/* Send data on a socket */
int socket_send(socket_t *sock, const void *buf, size_t len, int flags)
{
    if (sock == NULL || buf == NULL) {
        return -1;
    }
    
    /* Check if the socket has send operations */
    if (sock->ops == NULL || sock->ops->send == NULL) {
        return -1;
    }
    
    /* Call the socket's send operation */
    return sock->ops->send(sock, buf, len, flags);
}

/* Receive data from a socket */
int socket_recv(socket_t *sock, void *buf, size_t len, int flags)
{
    if (sock == NULL || buf == NULL) {
        return -1;
    }
    
    /* Check if the socket has recv operations */
    if (sock->ops == NULL || sock->ops->recv == NULL) {
        return -1;
    }
    
    /* Call the socket's recv operation */
    return sock->ops->recv(sock, buf, len, flags);
}

/* Close a socket */
int socket_close(socket_t *sock)
{
    if (sock == NULL) {
        return -1;
    }
    
    /* Check if the socket has close operations */
    if (sock->ops == NULL || sock->ops->close == NULL) {
        return -1;
    }
    
    /* Call the socket's close operation */
    int result = sock->ops->close(sock);
    
    /* Free the socket structure */
    kfree(sock);
    
    return result;
}

/* Register a network device */
int net_device_register(net_device_t *dev)
{
    if (dev == NULL) {
        return -1;
    }
    
    /* Add the device to the list */
    dev->next = net_devices;
    net_devices = dev;
    
    return 0;
}

/* Unregister a network device */
int net_device_unregister(net_device_t *dev)
{
    if (dev == NULL) {
        return -1;
    }
    
    /* Remove the device from the list */
    if (net_devices == dev) {
        net_devices = dev->next;
    } else {
        net_device_t *prev = net_devices;
        while (prev != NULL && prev->next != dev) {
            prev = prev->next;
        }
        
        if (prev != NULL) {
            prev->next = dev->next;
        }
    }
    
    return 0;
}

/* Register a network protocol */
int net_protocol_register(net_protocol_t *proto)
{
    if (proto == NULL) {
        return -1;
    }
    
    /* Add the protocol to the list */
    proto->next = net_protocols;
    net_protocols = proto;
    
    return 0;
}

/* Unregister a network protocol */
int net_protocol_unregister(net_protocol_t *proto)
{
    if (proto == NULL) {
        return -1;
    }
    
    /* Remove the protocol from the list */
    if (net_protocols == proto) {
        net_protocols = proto->next;
    } else {
        net_protocol_t *prev = net_protocols;
        while (prev != NULL && prev->next != proto) {
            prev = prev->next;
        }
        
        if (prev != NULL) {
            prev->next = proto->next;
        }
    }
    
    return 0;
}
