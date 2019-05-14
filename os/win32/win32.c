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
/* File os/win32/win32.c (maintained by: DF6LN)                         */
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

#define _TNN_WIN32_C

#include "tnn.h"

static  struct  timeval tv;
static  struct  timezone tz;

static  struct  timeval tv_old;

#ifndef NO_WATCHDOG
static  int     wdpid[2];
static  int     watch_dog[2];
#endif
int        consolefd = EOF;

#ifndef NO_WATCHDOG
static void     watchdog_init(void);
#endif
#ifdef DEBUG_MODUS
static void     sigterm(int);
static void     SigPipe(int);
#endif /* DEBUG_MODUS */

static void     setfiletime(struct ffblk *);

char    start_name[MAXPATH];
char    stop_name[MAXPATH];

UWORD   maxrounds = 1;          /* 1 = 100 Runden (default, wenn kein   */
                                /* "rounds"-Eintrag in tnn.ini          */

/* COM-Schnittststellen oeffnen. */
int open_win (char *name, int baud)
{ 
  HANDLE h;
  COMMTIMEOUTS ct;    
  DCB dcb;

  h = CreateFile (name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL,// | FILE_FLAG_OVERLAPPED,
                  NULL);
  if ((int)h == EOF)
    return EOF;

  if (SetCommMask (h, 0) == FALSE)
  {
    printf ("SetCommMask", "%s", strerror (GetLastError ()));
    Break();
  }

  if (SetupComm (h, 4096, 4096) == FALSE)
  {
    printf ("SetupComm", "%s", strerror (GetLastError ()));
    Break();
  }
  
  if (PurgeComm (h, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR) == FALSE)
  {
    printf ("PurgeComm", "%s", strerror (GetLastError ()));
    Break();
  }

  ct.ReadIntervalTimeout = 0xFFFFFFFF;
  ct.ReadTotalTimeoutMultiplier = 0;
  ct.ReadTotalTimeoutConstant = 0;
  ct.WriteTotalTimeoutMultiplier = 0;
  ct.WriteTotalTimeoutConstant = 0;

  if (SetCommTimeouts (h, &ct) == FALSE)
  {
    printf ("SetCommTimeout", "%s", strerror (GetLastError ()));
    Break();
  }

  memset (&dcb, 0, sizeof (DCB));
  dcb.DCBlength = sizeof (DCB);
  GetCommState (h, &dcb);
  dcb.BaudRate = baud;
  dcb.fBinary = 1;
  dcb.fParity = 0;
  dcb.fOutxCtsFlow = 0;      // CTS output flow control
  dcb.fOutxDsrFlow = 0;      // DSR output flow control
  dcb.fDtrControl = 0;       // DTR flow control type
  dcb.fDsrSensitivity = 0;   // DSR sensitivity
  dcb.fTXContinueOnXoff = 1; // XOFF continues Tx
  dcb.fOutX = 0;             // XON/XOFF out flow control
  dcb.fInX = 0;              // XON/XOFF in flow control
  dcb.fErrorChar = 0;        // enable error replacement
  dcb.fNull = 0;             // enable null stripping
  dcb.fRtsControl = 0;       // RTS flow control
  dcb.fAbortOnError = 0;     // abort reads/writes on error
  dcb.fDummy2 = 0;           // reserved
  dcb.wReserved = 0;         // not currently used
  dcb.XonLim = 0;            // transmit XON threshold
  dcb.XoffLim = 0;           // transmit XOFF threshold
  dcb.ByteSize = 8;          // number of bits/byte, 4-8
  dcb.Parity = 0;            // 0-4=no,odd,even,mark,space
  dcb.StopBits = 0;          // 0,1,2 = 1, 1.5, 2
  dcb.XonChar = 0;           // Tx and Rx XON character
  dcb.XoffChar = 0;          // Tx and Rx XOFF character
  dcb.ErrorChar = 0;         // error replacement character
  dcb.EofChar = 0;           // end of input character
  dcb.EvtChar = 0;           // received event character

  if (SetCommState (h, &dcb) == FALSE)
  {
    printf ("Fehler: Baudrate kann nicht initalisiert werden!\n"
            "Eintrag \"speed xxx\" (xxx=Baudrate) fehlt in der tnn.ini!\n");
    Break();
  }

  return((int)h);
}

int w_read (int fd, void *data, int maxlen)
{
  DWORD ret = 0;
  static OVERLAPPED ovl = { 0, 0, 0, 0, 0 };

  ReadFile ((HANDLE) fd, data, maxlen, &ret, &ovl);
  return ret;
}

int w_write (int fd, void *data, int maxlen)
{
  DWORD ret = 0;
  static OVERLAPPED ovl = { 0, 0, 0, 0, 0 };

  WriteFile ((HANDLE) fd, data, maxlen, &ret, &ovl);
  return ret;
}

/*
   String in Kleinbuchstaben umwandeln - aus DJGPP GNU-Paket fuer MS-DOS
   von D.J. Delorie
*/

char *
strlwr(char *s)
{
  char *p = s;
  while (*s)
  {
    if ((*s >= 'A') && (*s <= 'Z'))
      *s += 'a'-'A';
    s++;
  }
  return(p);
}


/*
   String in Grossbuchstaben umwandeln - aus DJGPP GNU-Paket fuer MS-DOS
   von D.J. Delorie
*/

char *
strupr(char *s)
{
  char *p = s;
  while (*s)
  {
    if ((*s >= 'a') && (*s <= 'z'))
      *s += 'A'-'a';
    s++;
  }
  return(p);
}

/************************************************************************/
/*                                                                      */
/* Als Ersatz fuer die Funktionen "findfirst" und "findnext" (bei DOS)  */
/* werden die Funktionen "xfindfirst" und "xfindnext" verwendet. Hier   */
/* wird nur der fuer TNN benoetigte Teil in die Struktur ffblk einge-   */
/* tragen. Der zuletzt gefundene Filename muss in ffblk->ff_name erhal- */
/* ten bleiben, weil dieser beim Aufruf von "xfindnext" gesucht wird.   */
/*                                                                      */
/************************************************************************/

int
xfindfirst(const char *pathname, struct ffblk *ffblk, int attrib)
{
  char          *fnpoi;
  DIR           *dp;
  struct dirent *dirp;
  int           retval;

  strcpy(ffblk->ff_path, pathname);             /* Filename incl. Pfad  */
  normfname(ffblk->ff_path);                    /* '\\' -> '/'          */
  fnpoi = strrchr(ffblk->ff_path, FILE_SEP);    /* Pfad angegeben?      */
  if (fnpoi == NULL)                            /* - nein ..            */
   {
    strcpy(ffblk->ff_find, ffblk->ff_path);     /* Filename kopieren    */
    strcpy(ffblk->ff_path, textpath);           /* default: textpath    */
   }
  else                                          /* mit Pfad             */
   {
    if (fnpoi == ffblk->ff_path+strlen(ffblk->ff_path)) /* ohne Name    */
      return(-1);                                       /* Unsinn       */
    strcpy(ffblk->ff_find, ++fnpoi);    /* nur Filename                 */
    *fnpoi = NUL;                       /* Filename im Pfad loeschen    */
   }

  if ((dp = opendir(ffblk->ff_path)) == NULL) /* Directory vorhanden?   */
    return(-1);
  retval = -1;                          /* default: nix gefunden        */
  while ((dirp = readdir(dp)) != NULL)  /* Eintrag vorhanden?           */
   {
    if ((fnmatch(ffblk->ff_find, dirp->d_name,
                 FNM_PATHNAME|FNM_PERIOD) != 0))
      continue;
    strcpy(ffblk->ff_name, dirp->d_name);
    setfiletime(ffblk);
    retval = 0;
    break;
   }
  closedir(dp);
  return(retval);
}

/************************************************************************/
/*                                                                      */
/* Erst den zuletzt gefundenen Eintrag suchen (steht in ffblk->ff_name) */
/* und den darauffolgenden passenden Eintrag zurueckmelden.             */
/*                                                                      */
/************************************************************************/

int
xfindnext(struct ffblk *ffblk)
{
  DIR           *dp;
  struct dirent *dirp;
  int           retval;

  if ((dp = opendir(ffblk->ff_path)) == NULL) return(-1);
  retval = -1;                          /* default: nix gefunden        */
  while ((dirp = readdir(dp)) != NULL)
   {
    if ((fnmatch(ffblk->ff_name, dirp->d_name,
                 FNM_PATHNAME|FNM_PERIOD) != 0))
      continue;
    retval = 1;
    break;
   }
  if (retval == 1)
   {
    retval = -1;                          /* default: nix gefunden      */
    while ((dirp = readdir(dp)) != NULL)
     {
      if ((fnmatch(ffblk->ff_find, dirp->d_name,
                   FNM_PATHNAME|FNM_PERIOD) != 0))
        continue;
      strcpy(ffblk->ff_name, dirp->d_name);
      setfiletime(ffblk);
      retval = 0;
      break;
     }
   }
  closedir(dp);
  return(retval);
}

/************************************************************************/
/*                                                                      */
/* Bei xfindfirst und xfindnext Datum und Uhrzeit im Fileblock setzen   */
/*                                                                      */
/************************************************************************/

static void
setfiletime(struct ffblk *ffblk)
{
  struct stat  filestat;
  struct tm   *filetime;
  struct stat  FSize;
  char         fn[MAXPATH];

  sprintf(fn, "%s%s", ffblk->ff_path, ffblk->ff_name);
  stat(fn, &filestat);
  filetime = gmtime(&filestat.st_mtime);
  ffblk->ff_ftime =   ((filetime->tm_sec / 2) & 0x1f)
                    + ((filetime->tm_min & 0x3f) << 5)
                    + ((filetime->tm_hour & 0x1f) << 11);
  ffblk->ff_fdate =   (filetime->tm_mday & 0x1f)
                    + (((filetime->tm_mon & 0x0f) + 1) << 5)
                    + (((filetime->tm_year - 80) & 0x7f) << 9);

  if (!stat(ffblk->ff_name, &FSize))
    ffblk->ff_fsize = FSize.st_size;
}

/************************************************************************/
/*                                                                      */
/* Ermitteln des freien Festplattenplatzes in Bytes.                    */
/*                                                                      */
/* Damit es fuer grosse Festplatten keinen Ueberlauf gibt (Rueckgabe    */
/* ist vom Typ LONG), wird ab 1GB freiem Speicher immer 1GB zurueck-    */
/* gemeldet - sieht da jemand Probleme?                                 */
/*                                                                      */
/************************************************************************/

LONG
getdiskfree(char *path)
{
  DWORD free = 0;

  GetDiskFreeSpace("C:\\", NULL, NULL, NULL, &free);
  return(free);
}

/************************************************************************/
/*                                                                      */
/* Fuer das erweiterte AUTOBIN-Protokoll wird die File-Zeit in einem    */
/* Bitfeld gespeichert (s. Doku zu DPBOX), wie es BC++ bei dem Befehl   */
/* getftime verwendet. Hier wird die Win32-Zeit der letzten Aenderung   */
/* verwendet.                                                           */
/*                                                                      */
/************************************************************************/

int getftime(int handle, struct ftime *ftimep)
{
  struct stat filestat;
  struct tm   *filetime;

  if (fstat(handle, &filestat) == -1)
          return(-1);

  filetime = gmtime(&filestat.st_mtime);

  ftimep->ft_tsec  = filetime->tm_sec / 2;
  ftimep->ft_min   = filetime->tm_min;
  ftimep->ft_hour  = filetime->tm_hour;
  ftimep->ft_day   = filetime->tm_mday;
  ftimep->ft_month = filetime->tm_mon + 1;
  ftimep->ft_year  = filetime->tm_year - 80;

  return(0);
}

/************************************************************************/
/*                                                                      */
/* 10ms-Ticker updaten                                                  */
/*                                                                      */
/************************************************************************/
void
update_timer(void)
{
  fd_set             rmask;
  fd_set             wmask;
  struct timeval     timevalue;
  int                max_fd = 0;
  int                count;
  register int       i;
  int                len;
  char               buffer[1024];
  static UWORD       roundcounter = 1;
#ifndef NO_WATCHDOG
  int                lasttic10 = tic10;
#endif

  gettimeofday(&tv, &tz);

  tic10 = (tv.tv_sec - tv_old.tv_sec) * 100 +
          (tv.tv_usec - tv_old.tv_usec) / 10000;

#ifndef NO_WATCHDOG
  if (tic10 - lasttic10)
    if (write(watch_dog[1], "\0", 1) != 1)
      exit(1);
#endif

  FD_ZERO(&rmask);
  FD_ZERO(&wmask);

  FD_SET((unsigned)consolefd, &rmask);        /* Eingabe Console      */
  max_fd = 1;

  if (kiss_active)
  {
    for (i = 0; i < L1PNUM; ++i)
    {
      if (l1port[i].kisslink < 0)
        continue;

#ifdef VANESSA
      if (l1port[i].kisstype == KISS_VAN)
        continue;
#endif
#ifdef AX_IPX
      if (l1port[i].kisstype == KISS_IPX)
        continue;
#endif
#ifdef AX25IP
      if (l1port[i].kisstype == KISS_AXIP)
        continue;
#endif
#ifdef KERNELIF
      if (   (l1port[i].kisstype == KISS_KAX25)
          || (l1port[i].kisstype == KISS_KAX25KJD))
        continue;
#endif
#ifdef SIXPACK
      if (l1port[i].kisstype == KISS_6PACK)
        continue;
#endif

      if (l1port[i].port_active)
      {
        FD_SET((unsigned)l1port[i].kisslink, &wmask);
        if (l1port[i].kisslink > max_fd -1)
          max_fd = l1port[i].kisslink + 1;
      }
    }
  }

  /* fuer alle Interfaces die benutzten Filedescriptoren fuer select() */
  /* ermitteln und eintragen */
  for (i = 0; i < L1PNUM; ++i)
  {
    /* nur aktive Ports bearbeiten */
    if (   (l1port[i].kisslink < 0) || (l1port[i].port_active != TRUE)
#ifdef SIXPACK
        || (l1port[i].kisstype == KISS_6PACK)   /* der hat hier nichts zu suchen */
#endif
       )
       continue;


    /* Interfacetypen durchgehen */
  } /* for(...) */

  timevalue.tv_usec = 10000;
  timevalue.tv_sec = 0;

  if (++roundcounter < maxrounds)      /* schon genug Runden vergangen? */
    timevalue.tv_usec = 0;
  else
    roundcounter = 0;                  /* Durchlaufzaehler ruecksetzen  */

  count = select(max_fd, &rmask, NULL, NULL, &timevalue);

  if (count == -1)
    return;

  /* alle Ports durchgehen */
  for (i = 0; i < L1PNUM; ++i)
  {
    /* unbenutzte Ports nicht bearbeiten */
    if (   (l1port[i].kisslink < 0) || (l1port[i].port_active != TRUE)
#ifdef SIXPACK
        || (l1port[i].kisstype == KISS_6PACK)   /* der hat hier nichts zu suchen */
#endif
       )
       continue;
  }

  if (kiss_active)
  {
    for (i = 0; i < L1PNUM; ++i)
    {
      if (l1port[i].kisslink == -1)
        continue;
#ifdef AX25IP
      if (l1port[i].kisstype == KISS_AXIP)
        continue;
#endif
#ifdef SIXPACK
      if (l1port[i].kisstype == KISS_6PACK)
        continue;
#endif
      if (l1port[i].port_active)
        if (FD_ISSET(l1port[i].kisslink, &wmask))
          if ((len = read(l1port[i].kisslink, buffer, 1024)) != 0)
            framedata_to_queue(i, buffer, len);
    }
  }
}

/************************************************************************/
/*                                                                      */
/* Rechner neu starten                                                  */
/*                                                                      */
/************************************************************************/

void
reboot_system(void)
{
  exit_all();
/* Hierher wird man wohl nur kommen, wenn tnn nicht als root gestartet  */
/* wurde. Nun gibt es 2 Moeglichkeiten: entweder wir starten von vorne, */
/* weil die Resourcen ja breits freigegeben wurden, oder aber wir       */
/* beenden das Programm, weil das ja mehr oder weniger gefordert wurde. */
/* Sinnvoller ist es wohl, wenn wir das Programm verlassen, in der      */
/* Hoffnung, dass es danach regulaer von einem Batch erneut gestartet   */
/* wird. Das ist jedenfalls besser, als nix zu tun.                     */
  Break();
  exit(1);
}

/************************************************************************/
/*                                                                      */
/* Shell-Befehl - Win32-spezifischer Teil:                              */
/* Das gewuenschte Programm wird als Hintergrundprozess gestartet.      */
/*                                                                      */
/************************************************************************/

BOOLEAN
tnnshell(char *cmdline)
{
  return(TRUE);
}

/************************************************************************/
/*                                                                      */
/* Pruefen, ob Kind-Prozesse beendet sind (vom Sysop gestartete externe */
/* Programme). Bei Zeitueberschreitung wird das Programm mit einer      */
/* entsprechenden Fehlermeldung abgebrochen.                            */
/*                                                                      */
/************************************************************************/

void
shellsrv(void)
{
}

/************************************************************************/
/*                                                                      */
/* Daten von der Shell abholen und zum User bringen                     */
/*                                                                      */
/************************************************************************/

void
shell_to_user(void)
{
}

/************************************************************************/
/*                                                                      */
/* User-Eingabe in mhdp verarbeiten                                     */
/*                                                                      */
/************************************************************************/

BOOLEAN
l7tosh(MBHEAD *mhdp)
{
  if (userpo->status == US_EXTP)            /* externer Prozess aktiv?  */
  {
    if (userpo->child_iactive == FALSE)
    {
      dealmb(mhdp);                           /* Abbruchzeile ignorieren  */
      userpo->mbhd = NULL;
      kill((pid_t *)(userpo->child_pid), SIGKILL);    /* Abbruch (Watchdog)       */
      putmsg("SHELL ABORTED by User\r");
      userpo->child_pid = 0;                  /* Prozess ist fertig       */
      userpo->child_fd = -1;
      userpo->child_iactive = FALSE;
      userpo->child_timeout = 0;
      userpo->status = US_CCP;
      return(TRUE);
    }
    return (TRUE);
  }
  return (FALSE);
}

/************************************************************************/
/*                                                                      */
/* Filenamen pruefen                                                    */
/*                                                                      */
/************************************************************************/

BOOLEAN
good_file_name(const char *file)
{
  return(TRUE);
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

void
tnnexec(char *file)
{
  char filename[MAXPATH+1];

  strncpy(filename, file, MAXPATH);
  filename[MAXPATH] = 0;
  
  if (xaccess(filename,0)==0)
  {
    l1exit();
    /* bei DOS16 -> done_umb() */
    exit_timer();
    exit_hardware();
    execl(filename, filename, NULL);
    perror("EXEC");
    HALT("tnnexec");
  }
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

/* Winsocket Initialisieren. */
static int startWinsock(void)
{
  WSADATA wsa;
  return WSAStartup(MAKEWORD(2,0),&wsa);
}

void
init_console(void)
{
  console_titel();

#ifdef L1TCPIP
  /* Interface Initialisieren. */
  InitIFC();
#endif /* L1TCPIP */

  setvbuf(stdout, NULL, _IONBF, 0);           /* Ausgabe ungepuffert  */

  /* Winsocket Initialisieren. */
  if (startWinsock())
  {
#ifdef SPEECH
    printf(speech_message(334));
#else
    printf("Error: Winsock cannot be initialized!\r");
#endif
    return;
  }

  if ((consolefd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return;

  consfile = stdout;
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

void
exit_console(void)
{
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

void hputc(char c)
{
  if (!ishmod)
  { /* keine Ausgaben im Hostmode */
    if (c == CR) /* nur fuer PC/Console notwenig! */
      putchar('\n');
    else
      putchar(c);
  }
}

/* return if character available */

BOOLEAN ishget()
{
  if (kbhit())
  {
    if (ishmod)
      hgetc();       /* im Hostmode Console ignorieren */
    else
      return(TRUE);  /* im Terminalmode melden, dass was da ist */
  }

  return(FALSE);     /* keine Umleitung, also auch nix da */
}

/* return if output to console is stopped */

BOOLEAN ishput(void)
{
  return(FALSE);
}

/* get one character from console */

char hgetc(void)
{
  #define Prefix 0
  #define ALT_X '-'
  char    retval = 0;

  if (kbhit())
  {
    if ((retval = getch()) == Prefix)
    {
      if (getch() == ALT_X)
        quit_program(0);
    }
  }

  return(retval);
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

BOOLEAN
init_hardware(int argc, char *argv[])
{
  int scanned = 1;

  umask(0);
  time(&sys_time);

  /* Kommandozeilenparameter auswerten */
  /* Den Verbose-Modus schon hier feststellen */
  while (scanned < argc)
  {
    /* Parameter "-v" (ausfuehrlichere Meldungen) */
    if (strcmp(argv[scanned], "-v") == 0)
    {
      bVerbose = TRUE;
    }
    ++scanned;
  }

  VMSG("--- Verbose logging started !\n");

  VMSG("--- Reading ini-file ...\n");
  if (read_init_file(argc, argv))
    return(TRUE);

/*
  if (tnn_errfile[0] != NUL)
  {
  }
*/
  VMSG("--- Allocating mem for %u buffers ...\n", tnn_buffers);

  RAMBOT = (char *)malloc(tnn_buffers * sizeof(MAX_BUFFER));
  RAMTOP = RAMBOT + tnn_buffers * sizeof(MAX_BUFFER);

  if (RAMBOT == NULL)
  {
    VMSG("!!! Allocation mem for %ld buffers failed, trying to allocate for %u buffers now ...\n", tnn_buffers, TNN_BUFFERS);
    RAMBOT = (char *)malloc(TNN_BUFFERS * sizeof(MAX_BUFFER));
    RAMTOP = RAMBOT + TNN_BUFFERS * sizeof(MAX_BUFFER);
    if (RAMBOT == NULL)
     {
      fprintf(stderr, "malloc for buffers failed\n");
      exit(1);
     }
    else
      tnn_buffers = TNN_BUFFERS;
   }

  if (kiss_active)
  {
    VMSG("--- Initializing KISS-interfaces ...\n");
    if (init_kisslink())
    {
      free(RAMBOT);
      Break();
      exit(1);
    }
  }

#ifndef NO_WATCHDOG
  VMSG("--- Initializing watchdog ...\n");
  watchdog_init();
#endif

  VMSG("--- Installing signal-handlers ...\n");
#ifdef DEBUG_MODUS
  signal(SIGSEGV, sigsegv);
  signal(SIGTERM, sigterm);
  signal(SIGPIPE, SigPipe);
#endif

  signal(SIGHUP, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  gettimeofday(&tv, &tz);
  tv_old = tv;

  return(FALSE);
}

#ifndef NO_WATCHDOG
/************************************************************************/
/*                                                                      */
/* Watchdog - ein vom TNN-Hauptprogramm unabhaengiger Prozess muss      */
/* regelmaessig vom Hauptprogramm Daten erhalten. Bleiben diese Daten   */
/* laenger als 60s aus, wird TNN beendet.                               */
/*                                                                      */
/************************************************************************/
static void
watchdog_init(void)
{
}
#endif

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

void
exit_hardware(void)
{
  free(RAMBOT);

  if (kiss_active)
    exit_kisslink();
}

#ifdef DEBUG_MODUS
/************************************************************************/
/* Signalbehandlungsfunktion fuer das Signal SIGTERM                    */
/************************************************************************/
static void
sigterm(int signo)
{
  signal(SIGTERM, SIG_IGN);
  printf("sigterm\n");
  quit_program(-1);
}

void
sigsegv(int signo)
{
  signal(SIGSEGV, SIG_IGN);
  printf("\nTheNetNode verursachte einen unbekannten Systemfehler!!!\n");
  printf("Letzte Funktion war: %s\n", lastfunc);
  Break();

  quit_program(-1);
}

/************************************************************************/
/*                                                                      */
/* Signalbehandlungsfunktion fuer das Signal SIGPIPE                    */
/*                                                                      */
/* Ursache, in bereits geschlossene Socket-Verbindung schreiben/lesen.  */
/*                                                                      */
/************************************************************************/
void SigPipe(int signo)
{
  signal(SIGPIPE, SIG_IGN);
  Break();
}

#endif

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

int
exit_all(void)
{
  exit_mh();                            /* MH-Liste sichern             */
  save_stat();
  save_configuration();
  personalmanager(SAVE, NULL, NULL);    /* Pers. Daten fuer Convers     */

  /* noch laufende Shells killen */
  for (userpo  = (USRBLK *) usccpl.head;
       userpo != (USRBLK *) &usccpl;
       userpo  = (USRBLK *) userpo->unext)
  {
    if (userpo->status != US_EXTP)      /* externer Prozess aktiv?      */
      continue;

      kill((pid_t *)(userpo->child_pid), SIGKILL);      /* Abbruch              */
  }

  /* Buffer freigeben */
  free(RAMBOT);

  /* Ab hier stehen keine Buffer mehr zur Verfuegung, alles was mit Buffern */
  /* zu tun hat, gibt nun nen Segfault ! */

  if (kiss_active)
    exit_kisslink();

  printf("\r\n");
  return(0);
}

static void message_handler(void)
{
  printf("\a\a\n\n*** ACHTUNG ***\n"
         "TheNetNode beenden mit ESC QUIT Enter im Konsolen-Fenster !!!.\n");
}

BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
  switch( fdwCtrlType ) 
  { 

    case CTRL_C_EVENT: 
      message_handler();
      return( TRUE );
 
    // CTRL-CLOSE: confirm that the user wants to exit. 
    case CTRL_CLOSE_EVENT: 
      message_handler();
      return( TRUE ); 
 
    // Pass other signals to the next handler. 
    case CTRL_BREAK_EVENT: 
      message_handler();
      return FALSE; 
 
    case CTRL_LOGOFF_EVENT: 
      message_handler();
      return FALSE; 
 
    case CTRL_SHUTDOWN_EVENT: 
      message_handler();
      return FALSE; 

#ifdef DEBUG_MDOUS
    case SIGSEGV: 
      sigsegv(fdwCtrlType);
      return FALSE; 
#endif 

    default: 
      return FALSE; 
  } 
} 

void console_titel(void)
{
  TCHAR szOldTitle[MAX_PATH];
  TCHAR szNewTitle[MAX_PATH];
  char call[10];

  call2str(call,myid);

  if( GetConsoleTitle(szOldTitle, MAX_PATH) )
  {
    wsprintf(szNewTitle, TEXT("%s%s)"), signon,call);

    if( !SetConsoleTitle(szNewTitle) )
      printf("SetConsoleTitle failed (%d)\n", GetLastError());
  }
}

/* Code vom Stefan DAC922. */
void gettimeofdaywindows(struct timeval *tv, struct timezone *tz)
{
  SYSTEMTIME systm;
  FILETIME   ftm;
  LONGLONG   ll;

  GetSystemTime(&systm);
  if (!SystemTimeToFileTime(&systm,&ftm))
    exit(1);

  ll = (LONGLONG) ftm.dwHighDateTime;
  ll <<= 32;
  ll |= (LONGLONG) ftm.dwLowDateTime;
  ll -= 116444736000000000;
  ll /= 10000000;
  tv->tv_sec = (long) ll;
  tv->tv_usec = (long) systm.wMilliseconds * 1000;
}

void kill(HANDLE Handle, WORD flag)
{
  TerminateProcess(Handle, flag);
  _endthread();
}

/* Ersatz fuer "system("pause"), bei Fernsteuerung      */
/* eines Knotens kann man schlecht eine Taste druecken. */
void Break(void)
{
  time_t lasttime;
  time_t now;

  now = time(NULL);
  lasttime = time(NULL);

  while (lasttime + 10 > now)
  {
    now = time(NULL);
  }
}

#ifdef L1TCPIP
/* Pruefe auf TCPIP-Port's */
int CheckPortTCP(int port)
{
  switch(kissmode(port))
  {
#ifdef L1TELNET
    case KISS_TELNET :
     return(TRUE);
#endif /* L1TELNET */

#ifdef L1HTTPD
    case KISS_HTTPD :
     return(TRUE);
#endif /* L1HTTPD */

#ifdef L1IPCONV
    case KISS_IPCONV :
     return(TRUE);
#endif /* L1IPCONV */

#ifdef L1IRC
    case KISS_IRC :
     return(TRUE);
#endif /* L1IRC */

    default :
     return(FALSE);
  }

  return(FALSE);
}
#endif /* L1TCPIP */

/*
  Dummy-Funktionen
*/

ULONG
coreleft(void) { return(FALSE); }
void
init_timer(void) {}
void
DIinc(void) {}
void
decEI(void) {}
void
exit_timer(void) {}
void
init_rs232(void) {}

/* End of os/win32/win32.c */
