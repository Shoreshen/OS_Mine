#pragma once

#define NR_CPUS             8
#define STACK_SIZE          32768
#define INT_STACK_BUFF      STACK_SIZE/8
#define MAX_SYSTEM_CALL_NR  128
#define PAGE_OFFSET	        ((unsigned long)0xffff800000000000)
#define NR_IRQS             24
#define KB_BUF_SIZE	        100
#define MACRO_PRINT         0
#define TIMER_SIRQ          1<<0
#define TASK_FILE_MAX	    10
#define MAX_FILE_DIR_LEN	100

#pragma region APIC
    //delivery mode
    #define	APIC_DEL_MODE_Fixed             0	//LAPIC	IOAPIC ICR
    #define	APIC_DEL_MODE_LP                1	//IOAPIC ICR
    #define	APIC_DEL_MODE_SMI               2	//LAPIC IOAPIC ICR
    #define	APIC_DEL_MODE_NMI               4	//LAPIC	IOAPIC ICR
    #define	APIC_DEL_MODE_INIT              5	//LAPIC	IOAPIC ICR
    #define	APIC_DEL_MODE_Start_up          6	//ICR
    #define	APIC_DEL_MODE_ExtINT            7	//IOAPIC

    //timer mode
    #define APIC_Timer_One_Shot		        0
    #define APIC_Timer_Periodic		        1
    #define APIC_Timer_TSC_Deadline	        2

    //mask
    #define APIC_MASK_SET	                1
    #define APIC_MASK_UNSET	                0

    //trigger mode
    #define APIC_TIG_MODE_Edge		        0
    #define APIC_TIG_MODE_Level		        1

    //delivery status
    #define APIC_DEL_STATU_Idle		        0
    #define APIC_DEL_STATU_Send_Pending	    1

    //destination shorthand
    #define APIC_DEST_SHORT_NONE        	    0
    #define APIC_DEST_SHORT_Self			    1
    #define APIC_DEST_SHORT_ALL_INCLUDE_Self	2
    #define APIC_DEST_SHORT_ALL_EXCLUDE_Self	3

    //destination mode
    #define APIC_DEST_MODE_PHYSICAL	        0
    #define APIC_DEST_MODE_LOGIC		    1

    //level
    #define ICR_LEVEL_DE_ASSERT		        0
    #define ICR_LEVLE_ASSERT		        1

    //remote irr
    #define APIC_IRR_RESET		            0
    #define APIC_IRR_ACCEPT		            1

    //pin polarity
    #define APIC_POLARITY_HIGH	            0
    #define APIC_POLARITY_LOW	            1
#pragma endregion

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

#pragma region Task
#define TASK_RUNNING		    (1 << 0)
#define	TASK_UNINTERRUPTIBLE	(1 << 2)
#define	TASK_ZOMBIE		        (1 << 3)
#define PF_KTHREAD	            (1 << 0)
#define PF_VFORK	            (1UL << 2)

#define NEED_SCHEDULE	(1UL << 1)
#pragma endregion

#pragma region asm
#define NULL 0
#define sti __asm__ __volatile__ ("sti\n\t":::"memory")
#define cli __asm__ __volatile__ ("cli\n\t":::"memory")
#define min(a,b) (a>b? b:a)
#define io_mfence __asm__ __volatile__ ("mfence	\n\t":::"memory")
    //mfence: force finishing all memory(RAM or cache) store/load precede to mfence
        //The reason for this is out of order execution of instructions.
        //It ensures all memory execution becomes globally visible before any load or store instruction that follows the MFENCE instruction.
        //note it does not write cache into memory.
#define nop __asm__ __volatile__ ("nop	\n\t")
#define hlt __asm__ __volatile__ ("hlt	\n\t")
#pragma endregion

#pragma region NVME
#define GET_DBL(n) 0x1000 + (2 * (4 << NVME.MBAR->MBAR_H.cap.dstrd))
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