/**
 * syscall.h - Horizon kernel system call definitions
 *
 * This file contains definitions for the system call interface.
 * The system call numbers are compatible with Linux for x86.
 */

#ifndef _HORIZON_SYSCALL_H
#define _HORIZON_SYSCALL_H

#include <horizon/types.h>

/* System call numbers (Linux compatible) */
#define SYS_EXIT                1
#define SYS_FORK                2
#define SYS_READ                3
#define SYS_WRITE               4
#define SYS_OPEN                5
#define SYS_CLOSE               6
#define SYS_WAITPID             7
#define SYS_CREAT               8
#define SYS_LINK                9
#define SYS_UNLINK              10
#define SYS_EXECVE              11
#define SYS_CHDIR               12
#define SYS_TIME                13
#define SYS_MKNOD               14
#define SYS_CHMOD               15
#define SYS_LCHOWN              16
#define SYS_BREAK               17
#define SYS_OLDSTAT             18
#define SYS_LSEEK               19
#define SYS_GETPID              20
#define SYS_MOUNT               21
#define SYS_UMOUNT              22
#define SYS_SETUID              23
#define SYS_GETUID              24
#define SYS_STIME               25
#define SYS_PTRACE              26
#define SYS_ALARM               27
#define SYS_OLDFSTAT            28
#define SYS_PAUSE               29
#define SYS_UTIME               30
#define SYS_STTY                31
#define SYS_GTTY                32
#define SYS_ACCESS              33
#define SYS_NICE                34
#define SYS_FTIME               35
#define SYS_SYNC                36
#define SYS_KILL                37
#define SYS_RENAME              38
#define SYS_MKDIR               39
#define SYS_RMDIR               40
#define SYS_DUP                 41
#define SYS_PIPE                42
#define SYS_TIMES               43
#define SYS_PROF                44
#define SYS_BRK                 45
#define SYS_SETGID              46
#define SYS_GETGID              47
#define SYS_SIGNAL              48
#define SYS_GETEUID             49
#define SYS_GETEGID             50
#define SYS_ACCT                51
#define SYS_UMOUNT2             52
#define SYS_LOCK                53
#define SYS_IOCTL               54
#define SYS_FCNTL               55
#define SYS_MPX                 56
#define SYS_SETPGID             57
#define SYS_ULIMIT              58
#define SYS_OLDOLDUNAME         59
#define SYS_UMASK               60
#define SYS_CHROOT              61
#define SYS_USTAT               62
#define SYS_DUP2                63
#define SYS_GETPPID             64
#define SYS_GETPGRP             65
#define SYS_SETSID              66
#define SYS_SIGACTION           67
#define SYS_SGETMASK            68
#define SYS_SSETMASK            69
#define SYS_SETREUID            70
#define SYS_SETREGID            71
#define SYS_SIGSUSPEND          72
#define SYS_SIGPENDING          73
#define SYS_SETHOSTNAME         74
#define SYS_SETRLIMIT           75
#define SYS_GETRLIMIT           76
#define SYS_GETRUSAGE           77
#define SYS_GETTIMEOFDAY        78
#define SYS_SETTIMEOFDAY        79
#define SYS_GETGROUPS           80
#define SYS_SETGROUPS           81
#define SYS_SELECT              82
#define SYS_SYMLINK             83
#define SYS_OLDLSTAT            84
#define SYS_READLINK            85
#define SYS_USELIB              86
#define SYS_SWAPON              87
#define SYS_REBOOT              88
#define SYS_READDIR             89
#define SYS_MMAP                90
#define SYS_MUNMAP              91
#define SYS_TRUNCATE            92
#define SYS_FTRUNCATE           93
#define SYS_FCHMOD              94
#define SYS_FCHOWN              95
#define SYS_GETPRIORITY         96
#define SYS_SETPRIORITY         97
#define SYS_PROFIL              98
#define SYS_STATFS              99
#define SYS_FSTATFS             100
#define SYS_IOPERM              101
#define SYS_SOCKETCALL          102
#define SYS_SYSLOG              103
#define SYS_SETITIMER           104
#define SYS_GETITIMER           105
#define SYS_STAT                106
#define SYS_LSTAT               107
#define SYS_FSTAT               108
#define SYS_OLDUNAME            109
#define SYS_IOPL                110
#define SYS_VHANGUP             111
#define SYS_IDLE                112
#define SYS_VM86OLD             113
#define SYS_WAIT4               114
#define SYS_SWAPOFF             115
#define SYS_SYSINFO             116
#define SYS_IPC                 117
#define SYS_FSYNC               118
#define SYS_SIGRETURN           119
#define SYS_CLONE               120
#define SYS_SETDOMAINNAME       121
#define SYS_UNAME               122
#define SYS_MODIFY_LDT          123
#define SYS_ADJTIMEX            124
#define SYS_MPROTECT            125
#define SYS_SIGPROCMASK         126
#define SYS_CREATE_MODULE       127
#define SYS_INIT_MODULE         128
#define SYS_DELETE_MODULE       129
#define SYS_GET_KERNEL_SYMS     130
#define SYS_QUOTACTL            131
#define SYS_GETPGID             132
#define SYS_FCHDIR              133
#define SYS_BDFLUSH             134
#define SYS_SYSFS               135
#define SYS_PERSONALITY         136
#define SYS_AFS_SYSCALL         137
#define SYS_SETFSUID            138
#define SYS_SETFSGID            139
#define SYS__LLSEEK             140
#define SYS_GETDENTS            141
#define SYS__NEWSELECT          142
#define SYS_FLOCK               143
#define SYS_MSYNC               144
#define SYS_READV               145
#define SYS_WRITEV              146
#define SYS_GETSID              147
#define SYS_FDATASYNC           148
#define SYS__SYSCTL             149
#define SYS_MLOCK               150
#define SYS_MUNLOCK             151
#define SYS_MLOCKALL            152
#define SYS_MUNLOCKALL          153
#define SYS_SCHED_SETPARAM      154
#define SYS_SCHED_GETPARAM      155
#define SYS_SCHED_SETSCHEDULER  156
#define SYS_SCHED_GETSCHEDULER  157
#define SYS_SCHED_YIELD         158
#define SYS_SCHED_GET_PRIORITY_MAX 159
#define SYS_SCHED_GET_PRIORITY_MIN 160
#define SYS_SCHED_RR_GET_INTERVAL 161
#define SYS_NANOSLEEP           162
#define SYS_MREMAP              163
#define SYS_SETRESUID           164
#define SYS_GETRESUID           165
#define SYS_VM86                166
#define SYS_QUERY_MODULE        167
#define SYS_POLL                168
#define SYS_NFSSERVCTL          169
#define SYS_SETRESGID           170
#define SYS_GETRESGID           171
#define SYS_PRCTL               172
#define SYS_RT_SIGRETURN        173
#define SYS_RT_SIGACTION        174
#define SYS_RT_SIGPROCMASK      175
#define SYS_RT_SIGPENDING       176
#define SYS_RT_SIGTIMEDWAIT     177
#define SYS_RT_SIGQUEUEINFO     178
#define SYS_RT_SIGSUSPEND       179
#define SYS_PREAD64             180
#define SYS_PWRITE64            181
#define SYS_CHOWN               182
#define SYS_GETCWD              183
#define SYS_CAPGET              184
#define SYS_CAPSET              185
#define SYS_SIGALTSTACK         186
#define SYS_SENDFILE            187
#define SYS_GETPMSG             188
#define SYS_PUTPMSG             189
#define SYS_VFORK               190
#define SYS_UGETRLIMIT          191
#define SYS_MMAP2               192
#define SYS_TRUNCATE64          193
#define SYS_FTRUNCATE64         194
#define SYS_STAT64              195
#define SYS_LSTAT64             196
#define SYS_FSTAT64             197
#define SYS_LCHOWN32            198
#define SYS_GETUID32            199
#define SYS_GETGID32            200
#define SYS_GETEUID32           201
#define SYS_GETEGID32           202
#define SYS_SETREUID32          203
#define SYS_SETREGID32          204
#define SYS_GETGROUPS32         205
#define SYS_SETGROUPS32         206
#define SYS_FCHOWN32            207
#define SYS_SETRESUID32         208
#define SYS_GETRESUID32         209
#define SYS_SETRESGID32         210
#define SYS_GETRESGID32         211
#define SYS_CHOWN32             212
#define SYS_SETUID32            213
#define SYS_SETGID32            214
#define SYS_SETFSUID32          215
#define SYS_SETFSGID32          216
#define SYS_PIVOT_ROOT          217
#define SYS_MINCORE             218
#define SYS_MADVISE             219
#define SYS_GETDENTS64          220
#define SYS_FCNTL64             221
#define SYS_GETTID              224
#define SYS_READAHEAD           225
#define SYS_SETXATTR            226
#define SYS_LSETXATTR           227
#define SYS_FSETXATTR           228
#define SYS_GETXATTR            229
#define SYS_LGETXATTR           230
#define SYS_FGETXATTR           231
#define SYS_LISTXATTR           232
#define SYS_LLISTXATTR          233
#define SYS_FLISTXATTR          234
#define SYS_REMOVEXATTR         235
#define SYS_LREMOVEXATTR        236
#define SYS_FREMOVEXATTR        237
#define SYS_TKILL               238
#define SYS_SENDFILE64          239
#define SYS_FUTEX               240
#define SYS_SCHED_SETAFFINITY   241
#define SYS_SCHED_GETAFFINITY   242
#define SYS_SET_THREAD_AREA     243
#define SYS_GET_THREAD_AREA     244
#define SYS_IO_SETUP            245
#define SYS_IO_DESTROY          246
#define SYS_IO_GETEVENTS        247
#define SYS_IO_SUBMIT           248
#define SYS_IO_CANCEL           249
#define SYS_FADVISE64           250
#define SYS_EXIT_GROUP          252
#define SYS_LOOKUP_DCOOKIE      253
#define SYS_EPOLL_CREATE        254
#define SYS_EPOLL_CTL           255
#define SYS_EPOLL_WAIT          256
#define SYS_REMAP_FILE_PAGES    257
#define SYS_SET_TID_ADDRESS     258
#define SYS_TIMER_CREATE        259
#define SYS_TIMER_SETTIME       260
#define SYS_TIMER_GETTIME       261
#define SYS_TIMER_GETOVERRUN    262
#define SYS_TIMER_DELETE        263
#define SYS_CLOCK_SETTIME       264
#define SYS_CLOCK_GETTIME       265
#define SYS_CLOCK_GETRES        266
#define SYS_CLOCK_NANOSLEEP     267
#define SYS_STATFS64            268
#define SYS_FSTATFS64           269
#define SYS_TGKILL              270
#define SYS_UTIMES              271
#define SYS_FADVISE64_64        272
#define SYS_VSERVER             273
#define SYS_MBIND               274
#define SYS_GET_MEMPOLICY       275
#define SYS_SET_MEMPOLICY       276
#define SYS_MQ_OPEN             277
#define SYS_MQ_UNLINK           278
#define SYS_MQ_TIMEDSEND        279
#define SYS_MQ_TIMEDRECEIVE     280
#define SYS_MQ_NOTIFY           281
#define SYS_MQ_GETSETATTR       282
#define SYS_KEXEC_LOAD          283
#define SYS_WAITID              284
#define SYS_OPENAT              295
#define SYS_SOCKET              359
#define SYS_BIND                361
#define SYS_CONNECT             362
#define SYS_LISTEN              363
#define SYS_ACCEPT              364
#define SYS_GETSOCKOPT          365
#define SYS_SETSOCKOPT          366
#define SYS_GETSOCKNAME         367
#define SYS_GETPEERNAME         368
#define SYS_SENDTO              369
#define SYS_SENDMSG             370
#define SYS_RECVFROM            371
#define SYS_RECVMSG             372
#define SYS_SHUTDOWN            373

/* Maximum number of system calls */
#define MAX_SYSCALLS            400

/* System call handler type */
typedef long (*syscall_handler_t)(long, long, long, long, long, long);

/* System call table */
extern syscall_handler_t syscall_table[MAX_SYSCALLS];

/* Initialize the system call subsystem */
void syscall_init(void);

/* Register a system call handler */
long syscall_register(u32 num, syscall_handler_t handler);

/* Unregister a system call handler */
long syscall_unregister(u32 num);

/* System call handler */
long syscall_handler(u32 num, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

#endif /* _HORIZON_SYSCALL_H */
