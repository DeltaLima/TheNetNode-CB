/************************************************************************/
/*                                                                      */
/*    *****                       *****                                 */
/*      *****                   *****                                   */
/*        *****               *****                                     */
/*          *****           *****                                       */
/*  ***************       ***************                               */
/*  *****************   *****************                               */
/*  ***************       ***************                               */
/*          *****           *****           TheNetNode                  */
/*        *****               *****         Portable                    */
/*      *****                   *****       Network                     */
/*    *****                       *****     Software                    */
/*                                                                      */
/* File os/win32/win32.h (maintained by: DF6LN)                         */
/*                                                                      */
/* This file is part of "TheNetNode" - Software Package                 */
/*                                                                      */
/* Copyright (C) 1998 - 2008 NORD><LINK e.V. Braunschweig               */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the NORD><LINK ALAS (Allgemeine Lizenz fuer    */
/* Amateurfunk Software) as published by Hans Georg Giese (DF2AU)       */
/* on 13/Oct/1992; either version 1, or (at your option) any later      */
/* version.                                                             */
/*                                                                      */
/* This program is distributed WITHOUT ANY WARRANTY only for further    */
/* development and learning purposes. See the ALAS (Allgemeine Lizenz   */
/* fuer Amateurfunk Software).                                          */
/*                                                                      */
/* You should have received a copy of the NORD><LINK ALAS (Allgemeine   */
/* Lizenz fuer Amateurfunk Software) along with this program; if not,   */
/* write to NORD><LINK e.V., Hinter dem Berge 5, D-38108 Braunschweig   */
/*                                                                      */
/* Dieses Programm ist PUBLIC DOMAIN, mit den Einschraenkungen durch    */
/* die ALAS (Allgemeine Lizenz fuer Amateurfunk Software), entweder     */
/* Version 1, veroeffentlicht von Hans Georg Giese (DF2AU),             */
/* am 13.Oct.1992, oder (wenn gewuenscht) jede spaetere Version.        */
/*                                                                      */
/* Dieses Programm wird unter Haftungsausschluss vertrieben, aus-       */
/* schliesslich fuer Weiterentwicklungs- und Lehrzwecke. Naeheres       */
/* koennen Sie der ALAS (Allgemeine Lizenz fuer Amateurfunk Software)   */
/* entnehmen.                                                           */
/*                                                                      */
/* Sollte dieser Software keine ALAS (Allgemeine Lizenz fuer Amateur-   */
/* funk Software) beigelegen haben, wenden Sie sich bitte an            */
/* NORD><LINK e.V., Hinter dem Berge 5, D-38108 Braunschweig            */
/*                                                                      */
/************************************************************************/

#include <winsock2.h>

/*------------------ Definitionen der anderen Versionen aufheben -------*/

#undef  TOKENRING
#undef  COMKISS
#undef  EXTDEV

#if defined(i386) && !defined(MIPS)
#define VANESSA
#endif

#define AX25IP

/*------------------ Definitionen fuer Win32-Version -------------------*/

#define L1PNUM L2PNUM           /* max. 16 Schnittstellen im PC         */
#define HOSTQ_BUFLEN 1024       /* Groesse Tastatur-/Bildschirmspeicher */
#define TNN_BUFFERS 10000       /* Vorgabe Bufferzahl                   */

/* definitions for kiss-commands */
#define CMD_TXDELAY 1
#define CMD_PERSIST 2
#define CMD_SLOTTIME 3
#define CMD_TXTAIL 4
#define CMD_FULLDUP 5
#define CMD_DAMA 6
#define CMD_TOKEN 6
#define CMD_TNCRES 0x0d
#define MSG_TOKEN CMD_TOKEN
#define MSG_TNCRES 0x0c
#define MSG_SENTDAMA 0x0e

/* definitions for different KISS-types */
#define KISS_NIX       -1
#define KISS_NORMAL     0
#define KISS_SMACK      1
#define KISS_RMNC       2
#define KISS_TOK        3
#define KISS_VAN        4
#define KISS_SCC        5
#define KISS_TF         6
#define KISS_IPX        7
#define KISS_AXIP       8
#define KISS_LOOP       9
#define KISS_KAX25     10
#define KISS_KAX25KJD  11
#define KISS_6PACK     12

/* set this to the highest available mode (used for error-checking) */
#ifndef L1TCPIP
#define KISS_MAX       12
#endif /* L1TCPIP */

/* maximum length of a received frame, TNN does not accept more!        */
#define MAXFRAMELEN L2MFLEN
#define CRCLEN 2
#define MAXKISSLEN MAXFRAMELEN+CRCLEN

/* defines for rx_state */
#define ST_BEGIN 0
#define ST_PORT 1
#define ST_DATA 2
#define ST_ESC 3
#define ST_TOKCMD 4
#define ST_TOKEN 5
#define ST_TNCRES 6
#define ST_DAMA 7

/* special codes used for KISS-protocol */
#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

#define MAXPATH 255

#define ASYNC_SPD_HI	  0x0010	/* Use 56000 instead of 38400 bps  */
#define ASYNC_SPD_VHI	 0x0020 /* Use 115200 instead of 38400 bps */
#define ASYNC_SPD_SHI	 0x1000	/* Use 230400 instead of 38400 bps */
#define ASYNC_SPD_WARP	0x1010	/* Use 460800 instead of 38400 bps */

#define B9600    9600
#define B19200  19200
#define B38400  38400
#define B57600  57600

#define MAXDRIVE  26
#define MAXDIR   255
#define MAXFILE  255
#define MAXEXT   255

#define IPTOS_THROUGHPUT 0x08
#define ENOTCONN         WSAENOTCONN
#define EWOULDBLOCK      WSAEWOULDBLOCK

#define SIGHUP   1
#define SIGKILL  9
#define SIGPIPE 13
#define SIGCHLD 17
#define SIGTSTP 20
#define SIGTTIN SIGBREAK  /* Ctrl-Break sequence */
#define SIGTTOU SIGABRT   /* abnormal termination triggered by abort call */

#define findfirst _findfirst
#define findnext  _findnext

#define gettimeofday gettimeofdaywindows
#define vsnprintf _vsnprintf
#define sleep(x) Sleep(x*1000)
#define close closesocket
#define read  w_read
#define write w_write

#define fcntl ioctlsocket
#define F_SETFL FIONBIO
#define FNDELAY &noblockingmode

#define xtempnam(x, y) tempnam(x, y)

#define	IP_TOS 3                /* IP type of service and preced        */


typedef struct
{
  BOOLEAN port_active;          /* Hardwareport ist aktiv               */
  char    device[MAXPATH];      /* Device fuer diesen Port              */
  char    tnn_lockfile[MAXPATH];/* Lockfile fuer Device dieses Ports    */
  int     lock;                 /* Filenummer fuer Lock-File            */
  int     kisslink;             /* Filenummer fuer Device               */
  UWORD   speed;                /* Baudrate des Device                  */
  WORD    speedflag;            /* Flag fuer Baudraten > 38400          */
  WORD    kisstype;             /* 0 = Kiss, 1 = SMACK, 2 = RMNC-KISS,  */
                                /* 3 = Tokenring, 4 = Vanessa, 5 = SCC, */
                                /* 6 = TheFirmware, 7 = IPX, 8 = AX25IP */
  WORD    rx_state;
  WORD    rx_port;
  WORD    tx_port;
  char    rx_buffer[MAXKISSLEN];
  char   *rx_bufptr;
  int     rx_buflen;
  ULONG   bad_frames;           /* Zaehler fuer Bad-Frames auf dem Port */
} DEVICE;

struct hostqueue {
  int first;
  int last;
  char buffer[HOSTQ_BUFLEN];
  struct hostqueue *next;
};

struct ffblk
{
  char     ff_path[MAXPATH];
  char     ff_find[NAME_MAX];
  char     ff_name[NAME_MAX];
  unsigned ff_fdate;
  unsigned ff_ftime;
  unsigned long ff_fsize;  /* Fuer Dateigrosse. */
};

struct ftime
{
  unsigned ft_tsec      : 5;
  unsigned ft_min       : 6;
  unsigned ft_hour      : 5;
  unsigned ft_day       : 5;
  unsigned ft_month     : 4;
  unsigned ft_year      : 7;
};

struct timezone
{
	int tz_minuteswest; /* Minutes west of GMZ */
	int tz_dsttime;     /* Nonzero if DST is ever in effect. */
};

typedef unsigned int socklen_t;

/* ------------------ Globale Variablen der Win32-Version --------------*/

 extern ULONG   tnn_buffers;
 extern char    tnn_initfile[MAXPATH];
 extern char    tnn_dir[MAXPATH];
 extern char    tnn_errfile[MAXPATH];
 extern WORD    max_device;
 extern DEVICE  l1port[L1PNUM];
 extern BOOLEAN kiss_active;
#ifdef ATTACH
 extern WORD    tokenring_ports;
 extern WORD    sixpack_ports;
#endif /* ATTACH */
 extern WORD    used_l1ports;
 extern WORD    l1ptab[L2PNUM];
 extern WORD    l2ptab[L1PNUM];
 extern DEVICE  *l1dp;
 extern UWORD   maxrounds;

 extern LONG bad_frames;
 extern time_t last_recovery;

 extern char start_name[MAXPATH];
 extern char stop_name[MAXPATH];

 char cShell[512];      /* Pfad zu Shell */

 static u_long  noblockingmode = FIONBIO;
 /* Variable fuer Winsock Initialisieren. */
 extern int     consolefd;

/* ------------------- Funktionen bei Win32-Version --------------------*/

char     *strlwr(char *);
char     *strupr(char *);
int      getftime(int, struct ftime *);
BOOLEAN  init_kisslink(void);
void     exit_kisslink(void);
void     send_kisscmd(int, int, int);
void     framedata_to_queue(int, char *, int);
void     put_error(char *);
void     hputud(unsigned);
void     reset_hardware(void);
int      exit_all(void);
BOOLEAN  read_init_file(int, char *[]);
BOOLEAN  init_proc(void);
void     exit_proc(void);
void     add_tnndir(char *);
void     security_check(char *);
BOOLEAN  good_file_name(const char *);
char    *xtempnam(const char *, const char *);
void     calculate_load(void);
void     print_load(MBHEAD *);
void     shell_to_user(void);
void     ccpsetshell(void);

BOOLEAN     good_file_name(const char *);
int         getftime(int, struct ftime *);
int         exit_all(void);
int         open_win (char *, int);
int         w_read (int, void *, int);
int         w_write (int, void *, int);
int         strcasecmp(const char *, const char *);
void        console_titel(void);
void        gettimeofdaywindows(struct timeval *, struct timezone *);
BOOL        CtrlHandler(DWORD);
#ifdef _MSC_VER
int         strncasecmp(const char *, const char *, int);
#endif
void        kill(HANDLE Handle, WORD flag);
void        Break(void);
void        SetFileSize(struct ffblk *);

/* Blocktransfer-Funktionen */
int cpymbflat(char *, MBHEAD *);
MBHEAD *cpyflatmb(char *, int);

#ifdef AX25IP
void    ax25ip(void);
BOOLEAN ax25ip_l1init(int);
void    ax25ip_l1exit(void);
void    ax25ip_l1ctl(int, int);
void    ax25ip_hwstr(int, MBHEAD *);
BOOLEAN ax25ip_dcd(int);
void    ccpaxipr(void);
#endif

#define kissmode(x) l1port[l1ptab[x]].kisstype
#define CRASH() *((int *) NULL) = 0

/* 6PACK */
#ifdef SIXPACK
void Sixpack_Housekeeping(void);
WORD Sixpack_DCD(UWORD);
void Sixpack_l1init(void);
void Sixpack_l1exit(void);
void Sixpack_l1ctl(int, UWORD);
void ccp6pack(void);
#endif

#ifdef DEBUG_MODUS
void     sigsegv(int);
#endif

/* End of os/win32/win32.h */
