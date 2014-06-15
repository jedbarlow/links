/* os_dep.h
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#ifndef OS_DEP_H
#define OS_DEP_H

#define SYS_UNIX	1
#define SYS_OS2		2
#define SYS_WIN_32	3
#define SYS_BEOS	4
#define SYS_RISCOS	5
#define SYS_ATHEOS	6
#define SYS_SPAD	7
#define SYS_INTERIX	8
#define SYS_OPENVMS	9
#define SYS_DOS		10

/* hardcoded limit of 10 OSes in default.c */

#ifdef UNIX
#undef UNIX
#endif

#if defined(__EMX__)
#define OS2
#elif defined(_WIN32) || defined(__CYGWIN__)
#define WIN32
#elif defined(__INTERIX)
#define INTERIX
#elif defined(__BEOS__) || defined(__HAIKU__)
#define BEOS
#elif defined(__riscos__)
#define RISCOS
#elif defined(__ATHEOS__) || defined(__SYLLABLE__)
#define ATHEOS
#elif defined(__SPAD__)
#define SPAD
#elif defined(__VMS)
#define OPENVMS
#elif defined(__DJGPP)
#define DOS
#else
#define UNIX
#endif

#if defined(UNIX)

static inline int dir_sep(unsigned char x) { return x == '/'; }
#define NEWLINE "\n"
#define FS_UNIX_RIGHTS
#define FS_UNIX_HARDLINKS
#define FS_UNIX_SOFTLINKS
#define FS_UNIX_USERS
#define SYSTEM_ID SYS_UNIX
#define SYSTEM_NAME "Unix"
#define DEFAULT_SHELL "/bin/sh"
#define GETSHELL getenv("SHELL")
#define SHARED_CONFIG_DIR "/etc/"
#ifdef HAVE_SYS_UN_H
#define USE_AF_UNIX
#else
#define DONT_USE_AF_UNIX
#endif
#define ASSOC_BLOCK
#define ASSOC_CONS_XWIN

#elif defined(OS2)

static inline int dir_sep(unsigned char x) { return x == '/' || x == '\\'; }
#define NEWLINE "\r\n"
/*#define NO_ASYNC_LOOKUP*/
#define SYSTEM_ID SYS_OS2
#define SYSTEM_NAME "OS/2"
#define DEFAULT_SHELL "cmd.exe"
#define GETSHELL getenv("COMSPEC")
#define NO_FG_EXEC
#define NO_CTRL_Z
#define DOS_FS
#define DOS_FS_8_3
#define NO_FILE_SECURITY
#define NO_FORK_ON_EXIT
#define ASSOC_CONS_XWIN
#define DISABLE_SMB

#elif defined(WIN32)

static inline int dir_sep(unsigned char x) { return x == '/' || x == '\\'; }
#define NEWLINE "\r\n"
/*#define NO_ASYNC_LOOKUP*/
#define SYSTEM_ID SYS_WIN_32
#define SYSTEM_NAME "Win32"
#define DEFAULT_SHELL "cmd.exe"
#define GETSHELL getenv("COMSPEC")
#define NO_FG_EXEC
#define NO_CTRL_Z
#define DOS_FS
#define SET_WINDOW_TITLE_UTF_8
#define ASSOC_CONS_XWIN
#define DISABLE_SMB
#ifdef __CYGWIN__
#define OS_BAD_SIGNALS
#endif
#ifndef HAVE_PTHREADS
#define HAVE_PTHREADS
#endif
#if defined(HAVE_SYS_UN_H) && !defined(_UWIN)
#define USE_AF_UNIX
#else
#define DONT_USE_AF_UNIX
#endif

#elif defined(INTERIX)

static inline int dir_sep(unsigned char x) { return x == '/'; }
#define NEWLINE "\n"
#define FS_UNIX_RIGHTS
#define FS_UNIX_HARDLINKS
#define FS_UNIX_SOFTLINKS
#define FS_UNIX_USERS
#define SYSTEM_ID SYS_INTERIX
#define SYSTEM_NAME "Interix"
#define DEFAULT_SHELL "/bin/sh"
#define GETSHELL getenv("SHELL")
#define SHARED_CONFIG_DIR "/etc/"
#ifdef HAVE_SYS_UN_H
#define USE_AF_UNIX
#else
#define DONT_USE_AF_UNIX
#endif
#define ASSOC_BLOCK
#define ASSOC_CONS_XWIN

#elif defined(BEOS)

static inline int dir_sep(unsigned char x) { return x == '/'; }
#define NEWLINE "\n"
#define NO_ASYNC_LOOKUP /* async lookup works on BeOS but crashes the Haiku kernel */
#define FS_UNIX_RIGHTS
#define FS_UNIX_SOFTLINKS
#define FS_UNIX_USERS
#define SYSTEM_ID SYS_BEOS
#define SYSTEM_NAME "BeOS"
#define DEFAULT_SHELL "/bin/sh"
#define GETSHELL getenv("SHELL")
#define NO_CTRL_Z
#define SHARED_CONFIG_DIR "/etc/"
#define NO_FORK_ON_EXIT
#define ASSOC_BLOCK

#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_NET_SOCKET_H
#include <net/socket.h>
#endif
#include <sys/socket.h>

#include "beos.h"

#elif defined(RISCOS)

static inline int dir_sep(unsigned char x) { return x == '/' || x == '\\'; }
#define NEWLINE "\n"
#define SYSTEM_ID SYS_RISCOS
#define SYSTEM_NAME "RISC OS"
#define DEFAULT_SHELL "gos"
#define GETSHELL getenv("SHELL")
#define NO_FG_EXEC
#define NO_CTRL_Z
#define NO_FILE_SECURITY
#define NO_FORK_ON_EXIT
#define DISABLE_SMB

#elif defined(ATHEOS)

static inline int dir_sep(unsigned char x) { return x == '/'; }
#define NEWLINE "\n"
#define FS_UNIX_RIGHTS
#define FS_UNIX_HARDLINKS
#define FS_UNIX_SOFTLINKS
#define FS_UNIX_USERS
#define SYSTEM_ID SYS_ATHEOS
#define SYSTEM_NAME "Atheos"
#define DEFAULT_SHELL "/bin/sh"
#define GETSHELL getenv("SHELL")
#define SHARED_CONFIG_DIR "/etc/"
#define ASSOC_BLOCK

#elif defined(SPAD)

static inline int dir_sep(unsigned char x) { return x == '/'; }
#define NEWLINE "\n"
#define SYSTEM_ID SYS_SPAD
#define SYSTEM_NAME "Spad"
#define DEFAULT_SHELL "LIB.:/SHELL.EXE"
#define GETSHELL "LIB.:/SHELL.EXE"
#define NO_CTRL_Z
#define SHARED_CONFIG_DIR "ETC.:/"
#ifdef HAVE_SYS_UN_H
#define USE_AF_UNIX
#else
#define DONT_USE_AF_UNIX
#endif
#define ASSOC_BLOCK
#define ASSOC_CONS_XWIN
#define NO_FORK_ON_EXIT

#elif defined(OPENVMS)

static inline int dir_sep(unsigned char x) { return x == '/'; }
#define NEWLINE "\n"
#define FS_UNIX_RIGHTS
#define SYSTEM_ID SYS_OPENVMS
#define SYSTEM_NAME "VMS"
#define DEFAULT_SHELL "/DCL"
#define GETSHELL NULL
#define SHARED_CONFIG_DIR "/etc/"
#ifndef HAVE_PTHREADS
#define HAVE_PTHREADS
#endif
#define DONT_USE_AF_UNIX
#define ASSOC_BLOCK
#define ASSOC_CONS_XWIN
#define NO_CTRL_Z
#define NO_FORK_ON_EXIT
#define DISABLE_SMB

#elif defined(DOS)

static inline int dir_sep(unsigned char x) { return x == '/' || x == '\\'; }
#define NEWLINE "\r\n"
#define NO_ASYNC_LOOKUP
#define SYSTEM_ID SYS_DOS
#define SYSTEM_NAME "DOS"
#define DEFAULT_SHELL "command.com"
#define GETSHELL getenv("COMSPEC")
#define SHARED_CONFIG_DIR "/dev/env/DJDIR/etc"
#define NO_CTRL_Z
#define DOS_FS
#define DOS_FS_8_3
#define NO_FILE_SECURITY
#define DONT_USE_AF_UNIX
#define NO_FORK_ON_EXIT
#define DISABLE_SMB

#endif

#ifdef FS_UNIX_USERS
#if !defined(HAVE_GETPWUID) || !defined(HAVE_GETGRGID)
#undef FS_UNIX_USERS
#endif
#endif

#endif /* #ifndef OS_DEP_H */
