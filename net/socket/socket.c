/**
 * socket.c - Socket implementation
 * 
 * This file contains the implementation of the socket interface.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/net.h>
#include <horizon/mm.h>
#include <horizon/syscall.h>

/* Socket system calls */

/* Socket system call */
static long sys_socket(long family, long type, long protocol, long arg4, long arg5, long arg6)
{
    socket_t *sock;
    int result = socket_create((int)family, (int)type, (int)protocol, &sock);
    
    if (result < 0) {
        return result;
    }
    
    /* This would allocate a file descriptor and associate it with the socket */
    /* For now, just return a dummy file descriptor */
    return 3;
}

/* Bind system call */
static long sys_bind(long sockfd, long addr, long addrlen, long arg4, long arg5, long arg6)
{
    /* This would look up the socket from the file descriptor */
    socket_t *sock = NULL;
    
    if (sock == NULL) {
        return -1;
    }
    
    return socket_bind(sock, (const sockaddr_t *)addr, (int)addrlen);
}

/* Connect system call */
static long sys_connect(long sockfd, long addr, long addrlen, long arg4, long arg5, long arg6)
{
    /* This would look up the socket from the file descriptor */
    socket_t *sock = NULL;
    
    if (sock == NULL) {
        return -1;
    }
    
    return socket_connect(sock, (const sockaddr_t *)addr, (int)addrlen);
}

/* Listen system call */
static long sys_listen(long sockfd, long backlog, long arg3, long arg4, long arg5, long arg6)
{
    /* This would look up the socket from the file descriptor */
    socket_t *sock = NULL;
    
    if (sock == NULL) {
        return -1;
    }
    
    return socket_listen(sock, (int)backlog);
}

/* Accept system call */
static long sys_accept(long sockfd, long addr, long addrlen, long arg4, long arg5, long arg6)
{
    /* This would look up the socket from the file descriptor */
    socket_t *sock = NULL;
    
    if (sock == NULL) {
        return -1;
    }
    
    socket_t *newsock;
    int result = socket_accept(sock, (sockaddr_t *)addr, (int *)addrlen, &newsock);
    
    if (result < 0) {
        return result;
    }
    
    /* This would allocate a file descriptor and associate it with the new socket */
    /* For now, just return a dummy file descriptor */
    return 4;
}

/* Send system call */
static long sys_send(long sockfd, long buf, long len, long flags, long arg5, long arg6)
{
    /* This would look up the socket from the file descriptor */
    socket_t *sock = NULL;
    
    if (sock == NULL) {
        return -1;
    }
    
    return socket_send(sock, (const void *)buf, (size_t)len, (int)flags);
}

/* Recv system call */
static long sys_recv(long sockfd, long buf, long len, long flags, long arg5, long arg6)
{
    /* This would look up the socket from the file descriptor */
    socket_t *sock = NULL;
    
    if (sock == NULL) {
        return -1;
    }
    
    return socket_recv(sock, (void *)buf, (size_t)len, (int)flags);
}

/* Close system call */
static long sys_close(long fd, long arg2, long arg3, long arg4, long arg5, long arg6)
{
    /* This would look up the socket from the file descriptor */
    socket_t *sock = NULL;
    
    if (sock == NULL) {
        return -1;
    }
    
    return socket_close(sock);
}

/* Initialize the socket interface */
void socket_init(void)
{
    /* Register the socket system calls */
    syscall_register(SYS_SOCKET, sys_socket);
    syscall_register(SYS_BIND, sys_bind);
    syscall_register(SYS_CONNECT, sys_connect);
    syscall_register(SYS_LISTEN, sys_listen);
    syscall_register(SYS_ACCEPT, sys_accept);
    syscall_register(SYS_SEND, sys_send);
    syscall_register(SYS_RECV, sys_recv);
    syscall_register(SYS_CLOSE, sys_close);
}
