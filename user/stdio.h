#pragma once

#define NULL 0

#pragma region POSIX
#define	E2BIG			1		/* Argument list too long or Lack of space in an output buffer or Argument is greater than the system-imposed maximum */
#define	EACCES			2		/* Permission denied */
#define	EADDRINUSE		3		/* Address in use */
#define	EADDRNOTAVAIL	4		/* Address not available */
#define	EAFNOSUPPORT	5		/* Address family not supported */
#define	EAGAIN			6		/* Resource temporarily unavailable */
#define	EALREADY		7		/* Connection already in progress */
#define	EBADF			8		/* Bad file descriptor */
#define	EBADMSG			9		/* Bad message */

#define	EBUSY			10		/* Resource busy */
#define	ECANCELED		11		/* Operation canceled */
#define	ECHILD			12		/* No child process */
#define	ECONNABORTED	13		/* Connection aborted */
#define	ECONNREFUSED	14		/* Connection refused */
#define	ECONNRESET		15		/* Connection reset */
#define	EDEADLK			16		/* Resource deadlock would occur */
#define	EDESTADDRREQ	17		/* Destination address required */
#define	EDOM			18		/* Domain error */
#define	EDQUOT			19		/* Reserved */

#define	EEXIST			20		/* File exists */
#define	EFAULT			21		/* Bad address */
#define	EFBIG			22		/* File too large */
#define	EHOSTUNREACH	23		/* Host is unreachable */
#define	EIDRM			24		/* Identifier removed */
#define	EILSEQ			25		/* Illegal byte sequence */
#define	EINPROGRESS		26		/* Operation in progress or O_NONBLOCK is set for the socket file descriptor and the connection cannot be immediately established */
#define	EINTR			27		/* Interrupted function call */
#define	EINVAL			28		/* Invalid argument */
#define	EIO				29		/* Input/output error */

#define	EISCONN			30		/* Socket is connected */
#define	EISDIR			31		/* Is a directory */
#define	ELOOP			32		/* Symbolic link loop */
#define	EMFILE			33		/* File descriptor value too large or too many open streams */
#define	EMLINK			34		/* Too many links */
#define	EMSGSIZE		35		/* Message too large or Inappropriate message buffer length */
#define	EMULTIHOP		36		/* Reserved */
#define	ENAMETOOLONG	37		/* Filename too long */
#define	ENETDOWN		38		/* Network is down */
#define	ENETRESET		39		/* The connection was aborted by the network */

#define	ENETUNREACH		40		/* Network unreachable */
#define	ENFILE			41		/* Too many files open in system */
#define	ENOBUFS			42		/* No buffer space available */
#define	ENODATA			43		/* No message available */
#define	ENODEV			44		/* No such device */
#define	ENOENT			45		/* No such file or directory */
#define	ENOEXEC			46		/* Executable file format error */
#define	ENOLCK			47		/* No locks available */
#define	ENOLINK			48		/* Reserved */
#define	ENOMEM			49		/* Not enough space */

#define	ENOMSG			50		/* No message of the desired type */
#define	ENOPROTOOPT		51		/* Protocol not available */
#define	ENOSPC			52		/* No space left on a device */
#define	ENOSR			53		/* No STREAM resources */
#define	ENOSTR			54		/* Not a STREAM */
#define	ENOSYS			55		/* Function not implemented */
#define	ENOTCONN		56		/* Socket not connected */
#define	ENOTDIR			57		/* Not a directory */
#define	ENOTEMPTY		58		/* Directory not empty */
#define	ENOTRECOVERABLE	59		/* State not recoverable */

#define	ENOTSOCK		60		/* Not a socket */
#define	ENOTSUP			61		/* Not supported */
#define	ENOTTY			62		/* Inappropriate I/O control operation */
#define	ENXIO			63		/* No such device or address */
#define	EOPNOTSUPP		64		/* Operation not supported on socket */
#define	EOVERFLOW		65		/* Value too large to be stored in data type */
#define	EOWNERDEAD		66		/* Previous owner died */
#define	EPERM			67		/* Operation not permitted */
#define	EPIPE			68		/* Broken pipe */
#define	EPROTO			69		/* Protocol error */

#define	EPROTONOSUPPORT	70		/* Protocol not supported */
#define	EPROTOTYPE		71		/* Protocol wrong type for socket */
#define	ERANGE			72		/* Result too large or too small */
#define	EROFS			73		/* Read-only file system */
#define	ESPIPE			74		/* Invalid seek */
#define	ESRCH			75		/* No such process */
#define	ESTALE			76		/* Reserved */
#define	ETIME			77		/* STREAM ioctl( ) timeout */
#define	ETIMEDOUT		78		/* Connection timed out or Operation timed out */
#define	ETXTBSY			79		/* Text file busy */

#define	EWOULDBLOCK		80		/* Operation would block */
#define	EXDEV			81		/* Improper link */
#pragma endregion

#pragma region File_Ops
#define	O_RDONLY	00000000	/* Open read-only */
#define	O_WRONLY	00000001	/* Open write-only */
#define	O_RDWR		00000002	/* Open read/write */
#define	O_ACCMODE	00000003	/* Mask for file access modes */

#define	O_CREAT		00000100	/* Create file if it does not exist */
#define	O_EXCL		00000200	/* Fail if file already exists */
#define	O_NOCTTY	00000400	/* Do not assign controlling terminal */
#define	O_TRUNC		00001000	/* If the file exists and is a regular file, and the file is successfully opened O_RDWR or O_WRONLY, its length shall be truncated to 0 */
#define	O_APPEND	00002000	/* the file offset shall be set to the end of the file */
#define	O_NONBLOCK	00004000	/* Non-blocking I/O mode */

#define	O_EXEC		00010000	/* Open for execute only (non-directory files) */
#define	O_SEARCH	00020000	/* Open directory for search only */
#define	O_DIRECTORY	00040000	/* must be a directory */
#define	O_NOFOLLOW	00100000	/* Do not follow symbolic links */

#define	SEEK_SET	0	/* Seek relative to start-of-file */
#define	SEEK_CUR	1	/* Seek relative to current position */
#define	SEEK_END	2	/* Seek relative to end-of-file */
#define SEEK_MAX	3
#pragma endregion

#pragma region shell
struct	buildincmd
{
	char *name;
	int (*function)(int, unsigned long*);
};

char *current_dir = NULL;
int find_cmd(char *cmd);
void run_cmd(int index,int argc,unsigned long* arg_pos);
#pragma endregion

unsigned long putstring(char *string);
unsigned long open(char *filename, unsigned long flags);
unsigned long close(unsigned long fd);
unsigned long read(unsigned long fd, void *buf, unsigned long count);
unsigned long write(unsigned long fd, void *buf, unsigned long count);
unsigned long lseek(unsigned long fd, long offset, unsigned long whence);
unsigned long fork(void);
unsigned long brk(unsigned long new_brk_end);
unsigned long clear(void);
unsigned long reboot(unsigned long cmd);
unsigned long chdir(unsigned long filename);
unsigned long lsdir(unsigned long filename);

int printf(char *fmt, ...);

//String func
int strlen(char * String);
int strcmp(char * FirstPart,char * SecondPart);
char * strncpy(char * Dest,char * Src,long Count);
char * strcpy(char * Dest,char * Src);
char * strcat(char * Dest,char * Src);

//Memory
void* memcpy(void *From, void *To, long Num);
void* memset(void *Address, unsigned char C, long Count);
int memcmp(void *FirstPart, void *SecondPart, long Count);
unsigned long malloc(unsigned long size);
void free(void *addr);