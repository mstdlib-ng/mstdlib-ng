#cmakedefine HAVE_IO_H
#cmakedefine HAVE_STRING_H
#cmakedefine HAVE_STRINGS_H
#cmakedefine HAVE_ERRNO_H

#cmakedefine HAVE_UNISTD_H
/* Work Around MacOS SDKs that do this stupid logic:
 *    #if !defined(HAVE_UNISTD_H) || HAVE_UNISTD_H
 * Public headers really should NEVER depend on things like HAVE_*
 */
#ifdef HAVE_UNISTD_H
#  undef HAVE_UNISTD_H
#  define HAVE_UNISTD_H 1
#endif

#cmakedefine HAVE_SYS_IOCTL_H
#cmakedefine HAVE_SYS_SELECT_H
#cmakedefine HAVE_SYS_TIME_H
#cmakedefine HAVE_SYS_TYPES_H
#cmakedefine HAVE_SYS_SOCKET_H
#cmakedefine HAVE_SYS_UN_H
#cmakedefine HAVE_NETINET_IN_H
#cmakedefine HAVE_ARPA_INET_H
#cmakedefine HAVE_NETDB_H
#cmakedefine HAVE_NETINET_TCP_H
#cmakedefine HAVE_VALGRIND_H
#cmakedefine HAVE_STDDEF_H
#cmakedefine HAVE_STDALIGN_H

#cmakedefine HAVE_STRCASECMP
#cmakedefine HAVE_STRNCASECMP
#cmakedefine HAVE__STRICMP
#cmakedefine HAVE__STRNICMP
#cmakedefine HAVE_SIGTIMEDWAIT
#cmakedefine HAVE_SIGWAIT
#cmakedefine HAVE_LOCALTIME_R
#cmakedefine HAVE_INET_PTON
#cmakedefine HAVE_INET_NTOP
#cmakedefine HAVE_GETCONTEXT
#cmakedefine HAVE_SECURE_GETENV

#cmakedefine HAVE_GETPWUID_5
#cmakedefine HAVE_GETPWUID_4

#cmakedefine HAVE_GETGRGID_5
#cmakedefine HAVE_GETGRGID_4

#cmakedefine HAVE_GETPWNAM_5
#cmakedefine HAVE_GETPWNAM_4

#cmakedefine HAVE_GETGRNAM_5
#cmakedefine HAVE_GETGRNAM_4

#cmakedefine HAVE_ST_BIRTHTIME

#cmakedefine HAVE_DIRENT_TYPE
#cmakedefine STRUCT_TM_HAS_GMTOFF
#cmakedefine STRUCT_TM_HAS_ZONE

#cmakedefine HAVE_SOCKLEN_T
#cmakedefine HAVE_SOCKADDR_STORAGE
#cmakedefine HAVE_MAX_ALIGN_T
#cmakedefine HAVE_ALIGNOF
#cmakedefine HAVE_ACCEPT4
#cmakedefine HAVE_PIPE2
#cmakedefine HAVE_CONFSTR

#cmakedefine _FILE_OFFSET_BITS @_FILE_OFFSET_BITS@
#cmakedefine _LARGE_FILES
#cmakedefine _LARGE_FILE_SOURCE

#cmakedefine HAVE_EXECINFO_H

#cmakedefine HAVE_ATOMIC_H
#cmakedefine HAVE_STDATOMIC_H
#cmakedefine HAVE_PTHREAD
#cmakedefine HAVE_PTHREAD_H
#cmakedefine HAVE_PTHREAD_NP_H
#cmakedefine HAVE_PTHREAD_INIT
#cmakedefine HAVE_PTHREAD_YIELD
#cmakedefine HAVE_PTHREAD_RWLOCK_INIT
#cmakedefine HAVE_PTHREAD_RWLOCKATTR_SETKIND_NP
#cmakedefine HAVE_PTHREAD_SETAFFINITY_NP
#cmakedefine HAVE_SCHED_SETAFFINITY
#cmakedefine HAVE_CPU_SET_T
#cmakedefine HAVE_CPUSET_T

#cmakedefine PTHREAD_SLEEP_USE_POLL
#cmakedefine PTHREAD_SLEEP_USE_SELECT
#cmakedefine PTHREAD_SLEEP_USE_NANOSLEEP

#cmakedefine HAVE_SYSCONF

#cmakedefine HAVE_KQUEUE
#cmakedefine HAVE_EPOLL
#cmakedefine HAVE_EPOLL_CREATE1

#cmakedefine HAVE_DLFCN_H
#cmakedefine HAVE_DLOPEN
#cmakedefine HAVE_LOADLIBRARY

#cmakedefine HAVE_VA_COPY
#cmakedefine HAVE_VA___COPY
#cmakedefine VA_LIST_IS_ARRAY_TYPE
#cmakedefine VA_LIST_IS_POINTER_TYPE

#cmakedefine MSTDLIB_USE_VALGRIND
#define MSTDLIB_INTERNAL
