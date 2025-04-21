/**
 * net.c - Horizon kernel network-related system calls
 *
 * This file contains the implementation of network-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/net.h>
#include <horizon/task.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Socket system call */
long sys_socket(long domain, long type, long protocol, long arg4, long arg5, long arg6) {
    /* Create a socket */
    return socket_create(domain, type, protocol);
}

/* Bind system call */
long sys_bind(long sockfd, long addr, long addrlen, long arg4, long arg5, long arg6) {
    /* Bind a socket to an address */
    return socket_bind(sockfd, (const struct sockaddr *)addr, addrlen);
}

/* Connect system call */
long sys_connect(long sockfd, long addr, long addrlen, long arg4, long arg5, long arg6) {
    /* Connect a socket to an address */
    return socket_connect(sockfd, (const struct sockaddr *)addr, addrlen);
}

/* Listen system call */
long sys_listen(long sockfd, long backlog, long arg3, long arg4, long arg5, long arg6) {
    /* Listen for connections on a socket */
    return socket_listen(sockfd, backlog);
}

/* Accept system call */
long sys_accept(long sockfd, long addr, long addrlen, long arg4, long arg5, long arg6) {
    /* Accept a connection on a socket */
    return socket_accept(sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

/* Get socket name system call */
long sys_getsockname(long sockfd, long addr, long addrlen, long arg4, long arg5, long arg6) {
    /* Get socket name */
    return socket_getsockname(sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

/* Get peer name system call */
long sys_getpeername(long sockfd, long addr, long addrlen, long arg4, long arg5, long arg6) {
    /* Get peer name */
    return socket_getpeername(sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

/* Socket pair system call */
long sys_socketpair(long domain, long type, long protocol, long sv, long arg5, long arg6) {
    /* Create a pair of connected sockets */
    return socket_socketpair(domain, type, protocol, (int *)sv);
}

/* Send system call */
long sys_send(long sockfd, long buf, long len, long flags, long arg5, long arg6) {
    /* Send a message on a socket */
    return socket_send(sockfd, (const void *)buf, len, flags);
}

/* Receive system call */
long sys_recv(long sockfd, long buf, long len, long flags, long arg5, long arg6) {
    /* Receive a message from a socket */
    return socket_recv(sockfd, (void *)buf, len, flags);
}

/* Send to system call */
long sys_sendto(long sockfd, long buf, long len, long flags, long dest_addr, long addrlen) {
    /* Send a message on a socket to a specific address */
    return socket_sendto(sockfd, (const void *)buf, len, flags, (const struct sockaddr *)dest_addr, addrlen);
}

/* Receive from system call */
long sys_recvfrom(long sockfd, long buf, long len, long flags, long src_addr, long addrlen) {
    /* Receive a message from a socket */
    return socket_recvfrom(sockfd, (void *)buf, len, flags, (struct sockaddr *)src_addr, (socklen_t *)addrlen);
}

/* Send message system call */
long sys_sendmsg(long sockfd, long msg, long flags, long arg4, long arg5, long arg6) {
    /* Send a message on a socket */
    return socket_sendmsg(sockfd, (const struct msghdr *)msg, flags);
}

/* Receive message system call */
long sys_recvmsg(long sockfd, long msg, long flags, long arg4, long arg5, long arg6) {
    /* Receive a message from a socket */
    return socket_recvmsg(sockfd, (struct msghdr *)msg, flags);
}

/* Shutdown system call */
long sys_shutdown(long sockfd, long how, long arg3, long arg4, long arg5, long arg6) {
    /* Shut down part of a full-duplex connection */
    return socket_shutdown(sockfd, how);
}

/* Get socket option system call */
long sys_getsockopt(long sockfd, long level, long optname, long optval, long optlen, long arg6) {
    /* Get socket options */
    return socket_getsockopt(sockfd, level, optname, (void *)optval, (socklen_t *)optlen);
}

/* Set socket option system call */
long sys_setsockopt(long sockfd, long level, long optname, long optval, long optlen, long arg6) {
    /* Set socket options */
    return socket_setsockopt(sockfd, level, optname, (const void *)optval, optlen);
}

/* Initialize network-related system calls */
void net_syscalls_init(void) {
    /* Register network-related system calls */
    syscall_register(SYS_SOCKET, sys_socket);
    syscall_register(SYS_BIND, sys_bind);
    syscall_register(SYS_CONNECT, sys_connect);
    syscall_register(SYS_LISTEN, sys_listen);
    syscall_register(SYS_ACCEPT, sys_accept);
    syscall_register(SYS_GETSOCKNAME, sys_getsockname);
    syscall_register(SYS_GETPEERNAME, sys_getpeername);
    syscall_register(SYS_SENDTO, sys_sendto);
    syscall_register(SYS_RECVFROM, sys_recvfrom);
    syscall_register(SYS_SENDMSG, sys_sendmsg);
    syscall_register(SYS_RECVMSG, sys_recvmsg);
    syscall_register(SYS_SHUTDOWN, sys_shutdown);
    syscall_register(SYS_GETSOCKOPT, sys_getsockopt);
    syscall_register(SYS_SETSOCKOPT, sys_setsockopt);
}
