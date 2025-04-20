/**
 * syscalls_net.c - Horizon kernel network system calls
 * 
 * This file contains the implementation of the network system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/net.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: socket */
long sys_socket(long domain, long type, long protocol, long unused1, long unused2, long unused3) {
    /* Create a socket */
    socket_t *sock = sock_create(domain, type, protocol);
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Create a file descriptor */
    file_t *file;
    int fd = file_anon_fd(sock, &file);
    
    if (fd < 0) {
        sock_close(sock);
        return -1;
    }
    
    return fd;
}

/* System call: bind */
long sys_bind(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Bind the socket */
    return sock_bind(sock, (const struct sockaddr *)addr, addrlen);
}

/* System call: connect */
long sys_connect(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Connect the socket */
    return sock_connect(sock, (const struct sockaddr *)addr, addrlen);
}

/* System call: listen */
long sys_listen(long sockfd, long backlog, long unused1, long unused2, long unused3, long unused4) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Listen on the socket */
    return sock_listen(sock, backlog);
}

/* System call: accept */
long sys_accept(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Accept a connection */
    socket_t *newsock = sock_accept(sock, (struct sockaddr *)addr, (socklen_t *)addrlen);
    
    if (newsock == NULL) {
        return -1;
    }
    
    /* Create a file descriptor */
    file_t *newfile;
    int fd = file_anon_fd(newsock, &newfile);
    
    if (fd < 0) {
        sock_close(newsock);
        return -1;
    }
    
    return fd;
}

/* System call: accept4 */
long sys_accept4(long sockfd, long addr, long addrlen, long flags, long unused1, long unused2) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Accept a connection */
    socket_t *newsock = sock_accept(sock, (struct sockaddr *)addr, (socklen_t *)addrlen);
    
    if (newsock == NULL) {
        return -1;
    }
    
    /* Create a file descriptor */
    file_t *newfile;
    int fd = file_anon_fd(newsock, &newfile);
    
    if (fd < 0) {
        sock_close(newsock);
        return -1;
    }
    
    /* Set the flags */
    if (flags & SOCK_NONBLOCK) {
        newfile->f_flags |= O_NONBLOCK;
    }
    
    if (flags & SOCK_CLOEXEC) {
        newfile->f_flags |= O_CLOEXEC;
    }
    
    return fd;
}

/* System call: getsockname */
long sys_getsockname(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Get the socket name */
    return sock_getsockname(sock, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

/* System call: getpeername */
long sys_getpeername(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Get the peer name */
    return sock_getpeername(sock, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

/* System call: socketpair */
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
    
    /* Set the file descriptors */
    int *fds = (int *)sv;
    fds[0] = fd1;
    fds[1] = fd2;
    
    return 0;
}

/* System call: send */
long sys_send(long sockfd, long buf, long len, long flags, long unused1, long unused2) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Send the data */
    return sock_send(sock, (const void *)buf, len, flags);
}

/* System call: recv */
long sys_recv(long sockfd, long buf, long len, long flags, long unused1, long unused2) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Receive the data */
    return sock_recv(sock, (void *)buf, len, flags);
}

/* System call: sendto */
long sys_sendto(long sockfd, long buf, long len, long flags, long dest_addr, long addrlen) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Send the data */
    return sock_sendto(sock, (const void *)buf, len, flags, (const struct sockaddr *)dest_addr, addrlen);
}

/* System call: recvfrom */
long sys_recvfrom(long sockfd, long buf, long len, long flags, long src_addr, long addrlen) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Receive the data */
    return sock_recvfrom(sock, (void *)buf, len, flags, (struct sockaddr *)src_addr, (socklen_t *)addrlen);
}

/* System call: sendmsg */
long sys_sendmsg(long sockfd, long msg, long flags, long unused1, long unused2, long unused3) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Send the message */
    return sock_sendmsg(sock, (const struct msghdr *)msg, flags);
}

/* System call: recvmsg */
long sys_recvmsg(long sockfd, long msg, long flags, long unused1, long unused2, long unused3) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Receive the message */
    return sock_recvmsg(sock, (struct msghdr *)msg, flags);
}

/* System call: shutdown */
long sys_shutdown(long sockfd, long how, long unused1, long unused2, long unused3, long unused4) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Shut down the socket */
    return sock_shutdown(sock, how);
}

/* System call: setsockopt */
long sys_setsockopt(long sockfd, long level, long optname, long optval, long optlen, long unused1) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Set the socket option */
    return sock_setsockopt(sock, level, optname, (const void *)optval, optlen);
}

/* System call: getsockopt */
long sys_getsockopt(long sockfd, long level, long optname, long optval, long optlen, long unused1) {
    /* Get the socket */
    file_t *file = process_get_file(task_current(), sockfd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the socket */
    socket_t *sock = file->private_data;
    
    if (sock == NULL) {
        return -1;
    }
    
    /* Get the socket option */
    return sock_getsockopt(sock, level, optname, (void *)optval, (socklen_t *)optlen);
}

/* System call: poll */
long sys_poll(long fds, long nfds, long timeout, long unused1, long unused2, long unused3) {
    /* Poll the file descriptors */
    return do_poll((struct pollfd *)fds, nfds, timeout);
}

/* System call: ppoll */
long sys_ppoll(long fds, long nfds, long tsp, long sigmask, long sigsetsize, long unused1) {
    /* Poll the file descriptors */
    return sys_ppoll((struct pollfd *)fds, nfds, (const struct timespec *)tsp, (const sigset_t *)sigmask);
}

/* System call: select */
long sys_select(long nfds, long readfds, long writefds, long exceptfds, long timeout, long unused1) {
    /* Select the file descriptors */
    return sys_select(nfds, (fd_set *)readfds, (fd_set *)writefds, (fd_set *)exceptfds, (struct timeval *)timeout);
}

/* System call: pselect6 */
long sys_pselect6(long nfds, long readfds, long writefds, long exceptfds, long timeout, long sigmask) {
    /* Select the file descriptors */
    return sys_pselect6(nfds, (fd_set *)readfds, (fd_set *)writefds, (fd_set *)exceptfds, (const struct timespec *)timeout, (const sigset_t *)sigmask);
}

/* System call: epoll_create */
long sys_epoll_create(long size, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Create an epoll instance */
    return epoll_create(size);
}

/* System call: epoll_create1 */
long sys_epoll_create1(long flags, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Create an epoll instance */
    return epoll_create1(flags);
}

/* System call: epoll_ctl */
long sys_epoll_ctl(long epfd, long op, long fd, long event, long unused1, long unused2) {
    /* Control an epoll instance */
    return epoll_ctl(epfd, op, fd, (struct epoll_event *)event);
}

/* System call: epoll_wait */
long sys_epoll_wait(long epfd, long events, long maxevents, long timeout, long unused1, long unused2) {
    /* Wait for events on an epoll instance */
    return epoll_wait(epfd, (struct epoll_event *)events, maxevents, timeout);
}

/* System call: epoll_pwait */
long sys_epoll_pwait(long epfd, long events, long maxevents, long timeout, long sigmask, long sigsetsize) {
    /* Wait for events on an epoll instance */
    return epoll_pwait(epfd, (struct epoll_event *)events, maxevents, timeout, (const sigset_t *)sigmask);
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
    syscall_register(SYS_POLL, sys_poll);
    syscall_register(SYS_PPOLL, sys_ppoll);
    syscall_register(SYS_SELECT, sys_select);
    syscall_register(SYS_PSELECT6, sys_pselect6);
    syscall_register(SYS_EPOLL_CREATE, sys_epoll_create);
    syscall_register(SYS_EPOLL_CREATE1, sys_epoll_create1);
    syscall_register(SYS_EPOLL_CTL, sys_epoll_ctl);
    syscall_register(SYS_EPOLL_WAIT, sys_epoll_wait);
    syscall_register(SYS_EPOLL_PWAIT, sys_epoll_pwait);
}
