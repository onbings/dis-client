/*
 * Copyright (c) 2015-2020, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module defines the bof error code
 *
 * Name:        boferr.h
 * Author:      Bernard HARMEL: onbings@gmail.com
 * Web:			    onbings.dscloud.me
 * Revision:    1.0
 *
 * Rem:         Nothing
 *
 * History:
 *
 * V 1.00  Dec 26 2013  BHA : Initial release
 */
#pragma once

/*
Bof error code are mapped on linux errno value
We add also specify Bof error code
A Bof_GetLastError is also provided and provice convertion from win32 error code to BOFERR
*/
enum BOFERR : int
{
  BOF_ERR_NO_ERROR = 0, /*! No error */
  // https://elixir.bootlin.com/linux/latest/source/include/uapi/asm-generic/errno-base.h
  BOF_ERR_EPERM, /* Operation not permitted */        // 1
  BOF_ERR_ENOENT,                                     /* No such file or directory */
  BOF_ERR_ESRCH,                                      /* No such process */
  BOF_ERR_EINTR,                                      /* Interrupted system call */
  BOF_ERR_EIO,                                        /* I/O error */
  BOF_ERR_ENXIO,                                      /* No such device or address */
  BOF_ERR_E2BIG,                                      /* Argument list too long */
  BOF_ERR_ENOEXEC,                                    /* Exec format error */
  BOF_ERR_EBADF,                                      /* Bad file number */
  BOF_ERR_ECHILD, /* No child processes */            // 10
  BOF_ERR_EAGAIN,                                     /* Try again */
  BOF_ERR_ENOMEM,                                     /* Out of memory */
  BOF_ERR_EACCES,                                     /* Permission denied */
  BOF_ERR_EFAULT,                                     /* Bad address */
  BOF_ERR_ENOTBLK,                                    /* Block device required */
  BOF_ERR_EBUSY,                                      /* Device or resource busy */
  BOF_ERR_EEXIST,                                     /* File exists */
  BOF_ERR_EXDEV,                                      /* Cross-device link */
  BOF_ERR_ENODEV,                                     /* No such device */
  BOF_ERR_ENOTDIR, /* Not a directory */              // 20
  BOF_ERR_EISDIR,                                     /* Is a directory */
  BOF_ERR_EINVAL,                                     /* Invalid argument */
  BOF_ERR_ENFILE,                                     /* File table overflow */
  BOF_ERR_EMFILE,                                     /* Too many open files */
  BOF_ERR_ENOTTY,                                     /* Not a typewriter */
  BOF_ERR_ETXTBSY,                                    /* Text file busy */
  BOF_ERR_EFBIG,                                      /* File too large */
  BOF_ERR_ENOSPC,                                     /* No space left on device */
  BOF_ERR_ESPIPE,                                     /* Illegal seek */
  BOF_ERR_EROFS, /* Read-only file system */          // 30
  BOF_ERR_EMLINK,                                     /* Too many links */
  BOF_ERR_EPIPE,                                      /* Broken pipe */
  BOF_ERR_EDOM,                                       /* Math argument out of domain of func */
  BOF_ERR_ERANGE, /* Math result not representable */ // 34
  // https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/errno.h
  BOF_ERR_EDEADLK, /* Resource deadlock would occur */     // 35
  BOF_ERR_ENAMETOOLONG,                                    /* File name too long */
  BOF_ERR_ENOLCK,                                          /* No record locks available */
  BOF_ERR_ENOSYS,                                          /* Invalid system call number */
  BOF_ERR_ENOTEMPTY,                                       /* Directory not empty */
  BOF_ERR_ELOOP, /* Too many symbolic links encountered */ // 40
  // See Below    BOF_ERR_EWOULDBLOCK=BOF_ERR_EAGAIN	/* Operation would block */
  BOF_ERR_ENOMSG = 42,                             /* No message of desired type */
  BOF_ERR_EIDRM,                                   /* Identifier removed */
  BOF_ERR_ECHRNG,                                  /* Channel number out of range */
  BOF_ERR_EL2NSYNC,                                /* Level 2 not synchronized */
  BOF_ERR_EL3HLT,                                  /* Level 3 halted */
  BOF_ERR_EL3RST,                                  /* Level 3 reset */
  BOF_ERR_ELNRNG,                                  /* Link number out of range */
  BOF_ERR_EUNATCH,                                 /* Protocol driver not attached */
  BOF_ERR_ENOCSI, /* No CSI structure available */ // 50
  BOF_ERR_EL2HLT,                                  /* Level 2 halted */
  BOF_ERR_EBADE,                                   /* Invalid exchange */
  BOF_ERR_EBADR,                                   /* Invalid request descriptor */
  BOF_ERR_EXFULL,                                  /* Exchange full */
  BOF_ERR_ENOANO,                                  /* No anode */
  BOF_ERR_EBADRQC,                                 /* Invalid request code */
  BOF_ERR_EBADSLT,                                 /* Invalid slot */
  // See Below    BOF_ERR_EDEADLOCK	BOF_ERR_EDEADLK
  BOF_ERR_EBFONT = 59,                                        /* Bad font file format */
  BOF_ERR_ENOSTR, /* Device not a stream */                   // 60
  BOF_ERR_ENODATA,                                            /* No data available */
  BOF_ERR_ETIME,                                              /* Timer expired */
  BOF_ERR_ENOSR,                                              /* Out of streams resources */
  BOF_ERR_ENONET,                                             /* Machine is not on the network */
  BOF_ERR_ENOPKG,                                             /* Package not installed */
  BOF_ERR_EREMOTE,                                            /* Object is remote */
  BOF_ERR_ENOLINK,                                            /* Link has been severed */
  BOF_ERR_EADV,                                               /* Advertise error */
  BOF_ERR_ESRMNT,                                             /* Srmount error */
  BOF_ERR_ECOMM, /* Communication error on send */            // 70
  BOF_ERR_EPROTO,                                             /* Protocol error */
  BOF_ERR_EMULTIHOP,                                          /* Multihop attempted */
  BOF_ERR_EDOTDOT,                                            /* RFS specific error */
  BOF_ERR_EBADMSG,                                            /* Not a data message */
  BOF_ERR_EOVERFLOW,                                          /* Value too large for defined data type */
  BOF_ERR_ENOTUNIQ,                                           /* Name not unique on network */
  BOF_ERR_EBADFD,                                             /* File descriptor in bad state */
  BOF_ERR_EREMCHG,                                            /* Remote address changed */
  BOF_ERR_ELIBACC,                                            /* Can not access a needed shared library */
  BOF_ERR_ELIBBAD, /* Accessing a corrupted shared library */ // 80
  BOF_ERR_ELIBSCN,                                            /* .lib section in a.out corrupted */
  BOF_ERR_ELIBMAX,                                            /* Attempting to link in too many shared libraries */
  BOF_ERR_ELIBEXEC,                                           /* Cannot exec a shared library directly */
  BOF_ERR_EILSEQ,                                             /* Illegal byte sequence */
  BOF_ERR_ERESTART,                                           /* Interrupted system call should be restarted */
  BOF_ERR_ESTRPIPE,                                           /* Streams pipe error */
  BOF_ERR_EUSERS,                                             /* Too many users */
  BOF_ERR_ENOTSOCK,                                           /* Socket operation on non-socket */
  BOF_ERR_EDESTADDRREQ,                                       /* Destination address required */
  BOF_ERR_EMSGSIZE, /* Message too long */                    // 90
  BOF_ERR_EPROTOTYPE,                                         /* Protocol wrong type for socket */
  BOF_ERR_ENOPROTOOPT,                                        /* Protocol not available */
  BOF_ERR_EPROTONOSUPPORT,                                    /* Protocol not supported */
  BOF_ERR_ESOCKTNOSUPPORT,                                    /* Socket type not supported */
  BOF_ERR_EOPNOTSUPP,                                         /* Operation not supported on transport endpoint */
  BOF_ERR_EPFNOSUPPORT,                                       /* Protocol family not supported */
  BOF_ERR_EAFNOSUPPORT,                                       /* Address family not supported by protocol */
  BOF_ERR_EADDRINUSE,                                         /* Address already in use */
  BOF_ERR_EADDRNOTAVAIL,                                      /* Cannot assign requested address */
  BOF_ERR_ENETDOWN, /* Network is down */                     // 100
  BOF_ERR_ENETUNREACH,                                        /* Network is unreachable */
  BOF_ERR_ENETRESET,                                          /* Network dropped connection because of reset */
  BOF_ERR_ECONNABORTED,                                       /* Software caused connection abort */
  BOF_ERR_ECONNRESET,                                         /* Connection reset by peer */
  BOF_ERR_ENOBUFS,                                            /* No buffer space available */
  BOF_ERR_EISCONN,                                            /* Transport endpoint is already connected */
  BOF_ERR_ENOTCONN,                                           /* Transport endpoint is not connected */
  BOF_ERR_ESHUTDOWN,                                          /* Cannot send after transport endpoint shutdown */
  BOF_ERR_ETOOMANYREFS,                                       /* Too many references: cannot splice */
  BOF_ERR_ETIMEDOUT, /* Connection timed out */               // 110
  BOF_ERR_ECONNREFUSED,                                       /* Connection refused */
  BOF_ERR_EHOSTDOWN,                                          /* Host is down */
  BOF_ERR_EHOSTUNREACH,                                       /* No route to host */
  BOF_ERR_EALREADY,                                           /* Operation already in progress */
  BOF_ERR_EINPROGRESS,                                        /* Operation now in progress */
  BOF_ERR_ESTALE,                                             /* Stale file handle */
  BOF_ERR_EUCLEAN,                                            /* Structure needs cleaning */
  BOF_ERR_ENOTNAM,                                            /* Not a XENIX named type file */
  BOF_ERR_ENAVAIL,                                            /* No XENIX semaphores available */
  BOF_ERR_EISNAM, /* Is a named type file */                  // 120
  BOF_ERR_EREMOTEIO,                                          /* Remote I/O error */
  BOF_ERR_EDQUOT,                                             /* Quota exceeded */
  BOF_ERR_ENOMEDIUM,                                          /* No medium found */
  BOF_ERR_EMEDIUMTYPE,                                        /* Wrong medium type */
  BOF_ERR_ECANCELED,                                          /* Operation Canceled */
  BOF_ERR_ENOKEY,                                             /* Required key not available */
  BOF_ERR_EKEYEXPIRED,                                        /* Key has expired */
  BOF_ERR_EKEYREVOKED,                                        /* Key has been revoked */
  BOF_ERR_EKEYREJECTED,                                       /* Key was rejected by service */
  BOF_ERR_EOWNERDEAD, /* Owner died */                        // 130
  BOF_ERR_ENOTRECOVERABLE,                                    /* State not recoverable */
  BOF_ERR_ERFKILL,                                            /* Operation not possible due to RF-kill */
  BOF_ERR_EHWPOISON,                                          /* Memory page has hardware error */

  // Not errno compatble error code
  BOF_ERR_SPECIFIC = 10000, /* Dummy name */
  BOF_ERR_ALREADY_OPENED,   /*! Object already opened */
  BOF_ERR_NOT_OPENED,       /*! Not opened */
  BOF_ERR_CREATE,           /*! Error during create access*/
  BOF_ERR_READ,             /*! Error during read access*/
  BOF_ERR_WRITE,            /*! Error during write access*/
  BOF_ERR_PURGE,            /*! Error during purge access*/
  BOF_ERR_SEEK,             /*! Error during seek access*/
  BOF_ERR_BIND,             /*! Error during bind access*/
  BOF_ERR_CLOSE,            /*! Error during close access*/
  BOF_ERR_INTERNAL,         /*! An internal error occurred while performing the requested operation */
  BOF_ERR_INIT,             /*! Init error */
  BOF_ERR_NOT_INIT,         /*! Not init error */
  BOF_ERR_NO_MORE,          /*! No more handle/connection available*/
  BOF_ERR_TOO_SMALL,        /*! Buffer/value provided is too small*/
  BOF_ERR_TOO_BIG,          /*! Buffer/value provided is too big*/
  BOF_ERR_WRONG_SIZE,       /*! Bad buffer/value size	*/
  BOF_ERR_FOUND,            /*! Resource found*/
  BOF_ERR_NOT_FOUND,        /*! Resource not found*/
  BOF_ERR_FREE,             /*! Object is free */
  BOF_ERR_FULL,             /*! Object is full */
  BOF_ERR_EMPTY,            /*! Object is empty */
  BOF_ERR_EOF,              /*! End of File */
  BOF_ERR_OPERATION_FAILED, /*! Operation could not be completed*/
  BOF_ERR_OVERRUN,          /*! Overrun*/
  BOF_ERR_UNDERRUN,         /*! Underrun*/
  BOF_ERR_STARTED,          /*! Started*/
  BOF_ERR_STOPPED,          /*! Already stopped*/
  BOF_ERR_CANNOT_START,     /*! Cannot start requested operation*/
  BOF_ERR_CANNOT_STOP,      /*! Cannot stop requested operation*/
  BOF_ERR_INTERFACE,        /*! Bad interface*/
  BOF_ERR_PROTOCOL,         /*! Bad protocol*/
  BOF_ERR_ADDRESS,          /*! Bad address*/
  BOF_ERR_FORMAT,           /*! Bad format*/
  BOF_ERR_WRONG_MODE,       /*! Wrong mode */
  BOF_ERR_NOT_AVAILABLE,    /*! Not available */
  BOF_ERR_INVALID_COMMAND,  /*! Invalid command */
  BOF_ERR_INVALID_ANSWER,   /*! Invalid answer */
  BOF_ERR_INVALID_CHECKSUM, /*! Invalid checksum */
  BOF_ERR_BAD_TYPE,         /*! Bad type */
  BOF_ERR_NOT_SUPPORTED,    /*! Not supported */
  BOF_ERR_PARSER,           /*! Paser error */
  BOF_ERR_INVALID_STATE,    /*! Invalid state*/
  BOF_ERR_YES,              /*! Yes*/
  BOF_ERR_NO,               /*! No*/
  BOF_ERR_CANCEL,           /*! Cancel state*/
  BOF_ERR_INDEX,            /*! Index error*/
  BOF_ERR_CURSOR,           /*! Cursor error*/
  BOF_ERR_DUPLICATE,        /*! Duplicate value*/
  BOF_ERR_SET,              /*! Set error*/
  BOF_ERR_GET,              /*! Get error*/
  BOF_ERR_PENDING,          /*! Pending*/
  BOF_ERR_RUNNING,          /*! Running*/
  BOF_ERR_NOT_RUNNING,      /*! Not running*/
  BOF_ERR_LOCK,             /*! Lock error*/
  BOF_ERR_UNLOCK,           /*! Unlock error*/
  BOF_ERR_FINISHED,         /*! Finished */
  BOF_ERR_SERIALIZE,        /*! Serialize */
  BOF_ERR_DESERIALIZE,      /*! Deserialize */
  BOF_ERR_INVALID_SRC,      /*! Invalid source" */
  BOF_ERR_INVALID_DST,      /*! Invalid destination" */
  BOF_ERR_BAD_STATUS,       /*! Bad status" */
  BOF_ERR_BAD_ID,           /*! Bad id" */
  BOF_ERR_INPUT,            /*! Input */
  BOF_ERR_OUTPUT,           /*! Output */
  BOF_ERR_CODEC,            /*! Codec */
  BOF_ERR_INVALID_HANDLE,   /*! Invalid handle value */
  BOF_ERR_DONT_EXIST,       /* Don't exist */
  BOF_ERR_OUT_OF_RANGE,     /* Out of range */
  BOF_ERR_EXCEPTION,        ///< Exception
  BOF_ERR_ABORT,            ///< Abort
  BOF_ERR_DMA,              ///< Dma
  BOF_ERR_DMA_IN,           ///< Dma in
  BOF_ERR_DMA_OUT,          ///< Dma out
  BOF_ERR_STORAGE,          ///< Storage
  BOF_ERR_BUFFER,           ///< Buffer
  BOF_ERR_RESET,            ///< Reset
  BOF_ERR_MAP,              ///< Map
  BOF_ERR_UNMAP,            ///< Unmap
  BOF_ERR_SCHEDULER,        ///< Scheduler
  BOF_ERR_PRIORITY,         ///< Priority
  BOF_ERR_HARD_INIT,        ///< Hw Init
  BOF_ERR_SOFT_INIT,        ///< Stf Init
  BOF_ERR_LOAD,             ///< Load
  BOF_ERR_SAVE,             ///< Save
  BOF_ERR_NO_CLIENT,        ///< No client
  BOF_ERR_ALREADY_CLOSED,   ///< Object already closed 
  BOF_ERR_NOT_STARTED,      ///< Not started
  BOF_ERR_SKIP,             ///< Skipped
  BOF_ERR_MTU,              ///< Mtu
  BOF_ERR_QOS,              ///< Qos
  BOF_ERR_IP ,              ///< Ip
  BOF_ERR_EXIT_THREAD,      ///< For Bof_CreateThread/Bof_LaunchTread
};
// #define BOF_ERR_EWOULDBLOCK BOF_ERR_EAGAIN
// #define BOF_ERR_EDEADLOCK BOF_ERR_EDEADLK
