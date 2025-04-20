/**
 * net.h - Networking subsystem definitions
 * 
 * This file contains definitions for the networking subsystem.
 */

#ifndef _KERNEL_NET_H
#define _KERNEL_NET_H

#include <horizon/types.h>

/* Protocol families */
#define PF_UNSPEC       0       /* Unspecified */
#define PF_LOCAL        1       /* Local to host (pipes and file-domain) */
#define PF_INET         2       /* IP protocol family */
#define PF_INET6        10      /* IPv6 */
#define PF_PACKET       17      /* Packet family */
#define PF_MAX          32      /* Maximum protocol family */

/* Address families */
#define AF_UNSPEC       PF_UNSPEC
#define AF_LOCAL        PF_LOCAL
#define AF_INET         PF_INET
#define AF_INET6        PF_INET6
#define AF_PACKET       PF_PACKET
#define AF_MAX          PF_MAX

/* Socket types */
#define SOCK_STREAM     1       /* Stream socket */
#define SOCK_DGRAM      2       /* Datagram socket */
#define SOCK_RAW        3       /* Raw socket */
#define SOCK_RDM        4       /* Reliably-delivered message */
#define SOCK_SEQPACKET  5       /* Sequential packet socket */
#define SOCK_PACKET     10      /* Packet socket */

/* Socket options */
#define SOL_SOCKET      1       /* Socket level */
#define SO_REUSEADDR    2       /* Reuse address */
#define SO_KEEPALIVE    9       /* Keep connection alive */
#define SO_BROADCAST    6       /* Broadcast */
#define SO_LINGER       13      /* Linger on close */
#define SO_SNDBUF       7       /* Send buffer size */
#define SO_RCVBUF       8       /* Receive buffer size */
#define SO_ERROR        4       /* Get error status and clear */
#define SO_TYPE         3       /* Get socket type */

/* IP protocols */
#define IPPROTO_IP      0       /* Internet Protocol */
#define IPPROTO_ICMP    1       /* Internet Control Message Protocol */
#define IPPROTO_TCP     6       /* Transmission Control Protocol */
#define IPPROTO_UDP     17      /* User Datagram Protocol */
#define IPPROTO_RAW     255     /* Raw IP packets */

/* Socket address structure */
typedef struct sockaddr {
    u16 sa_family;              /* Address family */
    char sa_data[14];           /* Address data */
} sockaddr_t;

/* Internet address structure */
typedef struct in_addr {
    u32 s_addr;                 /* IPv4 address */
} in_addr_t;

/* Internet socket address structure */
typedef struct sockaddr_in {
    u16 sin_family;             /* Address family (AF_INET) */
    u16 sin_port;               /* Port number */
    in_addr_t sin_addr;         /* IPv4 address */
    char sin_zero[8];           /* Padding */
} sockaddr_in_t;

/* Socket structure */
typedef struct socket {
    int type;                   /* Socket type */
    int protocol;               /* Socket protocol */
    int state;                  /* Socket state */
    struct socket_ops *ops;     /* Socket operations */
    void *private;              /* Private data */
} socket_t;

/* Socket operations */
typedef struct socket_ops {
    int (*bind)(socket_t *sock, const sockaddr_t *addr, int addrlen);
    int (*connect)(socket_t *sock, const sockaddr_t *addr, int addrlen);
    int (*listen)(socket_t *sock, int backlog);
    int (*accept)(socket_t *sock, sockaddr_t *addr, int *addrlen);
    int (*send)(socket_t *sock, const void *buf, size_t len, int flags);
    int (*recv)(socket_t *sock, void *buf, size_t len, int flags);
    int (*close)(socket_t *sock);
} socket_ops_t;

/* Network device structure */
typedef struct net_device {
    char name[16];              /* Device name */
    u8 hw_addr[6];              /* Hardware address */
    u32 ip_addr;                /* IP address */
    u32 netmask;                /* Network mask */
    u32 broadcast;              /* Broadcast address */
    u32 mtu;                    /* Maximum transmission unit */
    int flags;                  /* Device flags */
    struct net_device_ops *ops; /* Device operations */
    void *private;              /* Private data */
    struct net_device *next;    /* Next device in list */
} net_device_t;

/* Network device operations */
typedef struct net_device_ops {
    int (*open)(net_device_t *dev);
    int (*stop)(net_device_t *dev);
    int (*start_xmit)(net_device_t *dev, void *data, size_t len);
    int (*get_stats)(net_device_t *dev, void *stats);
    int (*set_mac_addr)(net_device_t *dev, void *addr);
    int (*do_ioctl)(net_device_t *dev, int cmd, void *arg);
} net_device_ops_t;

/* Network protocol structure */
typedef struct net_protocol {
    int protocol;               /* Protocol number */
    int (*handler)(void *data, size_t len);
    struct net_protocol *next;  /* Next protocol in list */
} net_protocol_t;

/* Networking subsystem functions */
void net_init(void);
int socket_create(int family, int type, int protocol, socket_t **sock);
int socket_bind(socket_t *sock, const sockaddr_t *addr, int addrlen);
int socket_connect(socket_t *sock, const sockaddr_t *addr, int addrlen);
int socket_listen(socket_t *sock, int backlog);
int socket_accept(socket_t *sock, sockaddr_t *addr, int *addrlen, socket_t **newsock);
int socket_send(socket_t *sock, const void *buf, size_t len, int flags);
int socket_recv(socket_t *sock, void *buf, size_t len, int flags);
int socket_close(socket_t *sock);
int net_device_register(net_device_t *dev);
int net_device_unregister(net_device_t *dev);
int net_protocol_register(net_protocol_t *proto);
int net_protocol_unregister(net_protocol_t *proto);

#endif /* _KERNEL_NET_H */
