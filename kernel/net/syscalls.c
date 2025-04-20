/**
 * syscalls.c - Horizon kernel network system calls
 * 
 * This file contains the implementation of the network system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/net.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: socket */
long sys_socket(long domain, long type, long protocol, long unused1, long unused2, long unused3) {
    /* Create a socket */
    return net_socket(domain, type, protocol);
}

/* System call: bind */
long sys_bind(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Bind a socket to an address */
    return net_bind(sockfd, (const struct sockaddr *)addr, addrlen);
}

/* System call: connect */
long sys_connect(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Connect a socket to an address */
    return net_connect(sockfd, (const struct sockaddr *)addr, addrlen);
}

/* System call: listen */
long sys_listen(long sockfd, long backlog, long unused1, long unused2, long unused3, long unused4) {
    /* Listen for connections on a socket */
    return net_listen(sockfd, backlog);
}

/* System call: accept */
long sys_accept(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Accept a connection on a socket */
    return net_accept(sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

/* System call: accept4 */
long sys_accept4(long sockfd, long addr, long addrlen, long flags, long unused1, long unused2) {
    /* Accept a connection on a socket with flags */
    return net_accept4(sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen, flags);
}

/* System call: getsockname */
long sys_getsockname(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Get socket name */
    return net_getsockname(sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

/* System call: getpeername */
long sys_getpeername(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Get peer socket name */
    return net_getpeername(sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

/* System call: socketpair */
long sys_socketpair(long domain, long type, long protocol, long sv, long unused1, long unused2) {
    /* Create a pair of connected sockets */
    return net_socketpair(domain, type, protocol, (int *)sv);
}

/* System call: send */
long sys_send(long sockfd, long buf, long len, long flags, long unused1, long unused2) {
    /* Send a message on a socket */
    return net_send(sockfd, (const void *)buf, len, flags);
}

/* System call: recv */
long sys_recv(long sockfd, long buf, long len, long flags, long unused1, long unused2) {
    /* Receive a message from a socket */
    return net_recv(sockfd, (void *)buf, len, flags);
}

/* System call: sendto */
long sys_sendto(long sockfd, long buf, long len, long flags, long dest_addr, long addrlen) {
    /* Send a message on a socket to a specific address */
    return net_sendto(sockfd, (const void *)buf, len, flags, (const struct sockaddr *)dest_addr, addrlen);
}

/* System call: recvfrom */
long sys_recvfrom(long sockfd, long buf, long len, long flags, long src_addr, long addrlen) {
    /* Receive a message from a socket and get the sender's address */
    return net_recvfrom(sockfd, (void *)buf, len, flags, (struct sockaddr *)src_addr, (socklen_t *)addrlen);
}

/* System call: sendmsg */
long sys_sendmsg(long sockfd, long msg, long flags, long unused1, long unused2, long unused3) {
    /* Send a message on a socket using a message structure */
    return net_sendmsg(sockfd, (const struct msghdr *)msg, flags);
}

/* System call: recvmsg */
long sys_recvmsg(long sockfd, long msg, long flags, long unused1, long unused2, long unused3) {
    /* Receive a message from a socket using a message structure */
    return net_recvmsg(sockfd, (struct msghdr *)msg, flags);
}

/* System call: shutdown */
long sys_shutdown(long sockfd, long how, long unused1, long unused2, long unused3, long unused4) {
    /* Shut down part of a full-duplex connection */
    return net_shutdown(sockfd, how);
}

/* System call: setsockopt */
long sys_setsockopt(long sockfd, long level, long optname, long optval, long optlen, long unused1) {
    /* Set socket options */
    return net_setsockopt(sockfd, level, optname, (const void *)optval, optlen);
}

/* System call: getsockopt */
long sys_getsockopt(long sockfd, long level, long optname, long optval, long optlen, long unused1) {
    /* Get socket options */
    return net_getsockopt(sockfd, level, optname, (void *)optval, (socklen_t *)optlen);
}

/* System call: sendmmsg */
long sys_sendmmsg(long sockfd, long msgvec, long vlen, long flags, long unused1, long unused2) {
    /* Send multiple messages on a socket using message structures */
    return net_sendmmsg(sockfd, (struct mmsghdr *)msgvec, vlen, flags);
}

/* System call: recvmmsg */
long sys_recvmmsg(long sockfd, long msgvec, long vlen, long flags, long timeout, long unused1) {
    /* Receive multiple messages from a socket using message structures */
    return net_recvmmsg(sockfd, (struct mmsghdr *)msgvec, vlen, flags, (struct timespec *)timeout);
}

/* System call: gethostname */
long sys_gethostname(long name, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Get the hostname */
    return net_gethostname((char *)name, len);
}

/* System call: sethostname */
long sys_sethostname(long name, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Set the hostname */
    return net_sethostname((const char *)name, len);
}

/* System call: getdomainname */
long sys_getdomainname(long name, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Get the domain name */
    return net_getdomainname((char *)name, len);
}

/* System call: setdomainname */
long sys_setdomainname(long name, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Set the domain name */
    return net_setdomainname((const char *)name, len);
}

/* System call: socketcall */
long sys_socketcall(long call, long args, long unused1, long unused2, long unused3, long unused4) {
    /* Socket system call multiplexer */
    long *a = (long *)args;
    
    switch (call) {
        case SYS_SOCKET:
            return sys_socket(a[0], a[1], a[2], 0, 0, 0);
        case SYS_BIND:
            return sys_bind(a[0], a[1], a[2], 0, 0, 0);
        case SYS_CONNECT:
            return sys_connect(a[0], a[1], a[2], 0, 0, 0);
        case SYS_LISTEN:
            return sys_listen(a[0], a[1], 0, 0, 0, 0);
        case SYS_ACCEPT:
            return sys_accept(a[0], a[1], a[2], 0, 0, 0);
        case SYS_GETSOCKNAME:
            return sys_getsockname(a[0], a[1], a[2], 0, 0, 0);
        case SYS_GETPEERNAME:
            return sys_getpeername(a[0], a[1], a[2], 0, 0, 0);
        case SYS_SOCKETPAIR:
            return sys_socketpair(a[0], a[1], a[2], a[3], 0, 0);
        case SYS_SEND:
            return sys_send(a[0], a[1], a[2], a[3], 0, 0);
        case SYS_RECV:
            return sys_recv(a[0], a[1], a[2], a[3], 0, 0);
        case SYS_SENDTO:
            return sys_sendto(a[0], a[1], a[2], a[3], a[4], a[5]);
        case SYS_RECVFROM:
            return sys_recvfrom(a[0], a[1], a[2], a[3], a[4], a[5]);
        case SYS_SHUTDOWN:
            return sys_shutdown(a[0], a[1], 0, 0, 0, 0);
        case SYS_SETSOCKOPT:
            return sys_setsockopt(a[0], a[1], a[2], a[3], a[4], 0);
        case SYS_GETSOCKOPT:
            return sys_getsockopt(a[0], a[1], a[2], a[3], a[4], 0);
        case SYS_SENDMSG:
            return sys_sendmsg(a[0], a[1], a[2], 0, 0, 0);
        case SYS_RECVMSG:
            return sys_recvmsg(a[0], a[1], a[2], 0, 0, 0);
        case SYS_ACCEPT4:
            return sys_accept4(a[0], a[1], a[2], a[3], 0, 0);
        case SYS_RECVMMSG:
            return sys_recvmmsg(a[0], a[1], a[2], a[3], a[4], 0);
        case SYS_SENDMMSG:
            return sys_sendmmsg(a[0], a[1], a[2], a[3], 0, 0);
        default:
            return -1;
    }
}

/* Register network system calls */
void net_syscalls_init(void) {
    /* Register network system calls */
    syscall_register(SYS_SOCKET, sys_socket);
    syscall_register(SYS_BIND, sys_bind);
    syscall_register(SYS_CONNECT, sys_connect);
    syscall_register(SYS_LISTEN, sys_listen);
    syscall_register(SYS_ACCEPT, sys_accept);
    syscall_register(SYS_ACCEPT4, sys_accept4);
    syscall_register(SYS_GETSOCKNAME, sys_getsockname);
    syscall_register(SYS_GETPEERNAME, sys_getpeername);
    syscall_register(SYS_SOCKETPAIR, sys_socketpair);
    syscall_register(SYS_SEND, sys_send);
    syscall_register(SYS_RECV, sys_recv);
    syscall_register(SYS_SENDTO, sys_sendto);
    syscall_register(SYS_RECVFROM, sys_recvfrom);
    syscall_register(SYS_SENDMSG, sys_sendmsg);
    syscall_register(SYS_RECVMSG, sys_recvmsg);
    syscall_register(SYS_SHUTDOWN, sys_shutdown);
    syscall_register(SYS_SETSOCKOPT, sys_setsockopt);
    syscall_register(SYS_GETSOCKOPT, sys_getsockopt);
    syscall_register(SYS_SENDMMSG, sys_sendmmsg);
    syscall_register(SYS_RECVMMSG, sys_recvmmsg);
    syscall_register(SYS_GETHOSTNAME, sys_gethostname);
    syscall_register(SYS_SETHOSTNAME, sys_sethostname);
    syscall_register(SYS_GETDOMAINNAME, sys_getdomainname);
    syscall_register(SYS_SETDOMAINNAME, sys_setdomainname);
    syscall_register(SYS_SOCKETCALL, sys_socketcall);
}
