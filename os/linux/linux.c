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
/* File os/linux/linux.c (maintained by: DF6LN)                         */
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

#define _TNN_LINUX_C

#include "tnn.h"

static  struct  hostqueue *hostq_root;
static  struct  hostqueue *hostq_last;

static  struct  termios oconin_termios;
static  struct  termios nconin_termios;
static  struct  termios oconout_termios;
static  struct  termios nconout_termios;

static  struct  timeval tv;
static  struct  timezone tz;

static  struct  timeval tv_old;
static  struct  sockaddr_un serv_addr;

static  int     hostq_len;
static  int     servlen;
static  int     consockfd;
static  int     sockfd;

#ifndef NO_WATCHDOG
static  int     wdpid[2];
static  int     watch_dog[2];
#endif

UBYTE   console_type = CONS_TERM_DO_SETUP;

#ifndef NO_WATCHDOG
static void     watchdog_init(void);
#endif
#ifdef DEBUG_MODUS
static void     sigterm(int);
#endif
static void     host_to_queue(char *, int);
static void     setfiletime(struct ffblk *);

static float    load[3];
static float    sys_load = 0;
static int      tnn_load;

BOOLEAN unlock;
/* BOOLEAN use_socket; */

char    tnn_socket[MAXPATH];
char    start_name[MAXPATH];
char    stop_name[MAXPATH];

UWORD   maxrounds = 1;          /* 1 = 100 Runden (default, wenn kein   */
                                /* "rounds"-Eintrag in tnn.ini          */
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
  fnpoi = strrchr(ffblk->ff_path, '/');         /* Pfad angegeben?      */
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
}

/************************************************************************/
/*                                                                      */
/* Temporaeren Filenamen generieren und pruefen auf Verwendbarkeit,     */
/* d.h. Grossbuchstaben sind nicht erlaubt, damit die Datei bei ccpread */
/* gefunden wird.                                                       */
/*                                                                      */
/* 31.3.02 DG9OBU   auf Verwendung von mkstemp() umgestellt             */
/************************************************************************/

char *
xtempnam(const char *directory, const char *prefix)
{
  char   *fn = NULL;
  size_t i;
  register int x;

  const char* sixx = "XXXXXX\000";

  char namebuf[PATH_MAX];
  char xe[128];

  /* den Dateinamen erstellen, prefix und sechs X fuer mkstemp() */
  memset(xe, 0, sizeof(xe));
  strcpy(xe, prefix);
  strcat(xe, sixx);

  /* solange bis brauchbarer Filename gefunden */
  while (fn == NULL)
  {
    /* Namen zusammenbauen */
    memset(namebuf, 0, sizeof(namebuf));
    strcpy(namebuf, directory);
    strcat(namebuf, xe);

    /* File generieren */
    if ((x = mkstemp(&namebuf[0])) > 0)
    {
      /* und gleich wieder schliessen und loeschen */
      close(x);
      unlink(&namebuf[0]);

      /* Namen pruefen */
      for (i = 0; i < strlen(&namebuf[0]); ++i)    /* Filenamen untersuchen     */
        if (namebuf[i] != tolower(namebuf[i]))     /* Grossbuchstabe geht nicht */
          break;

      /* wenn Namenspruefung erfolgreich war, dann Namen melden */
      if (i == strlen(namebuf))
      {
        fn = malloc(PATH_MAX);
        strcpy(fn, &namebuf[0]);
        return(fn);
      }
    }
    else break;
  }

  /* nicht erfolgreich */
  return(NULL);
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
  FILE  *fp;
  char  str[100];
  ULONG frei = 0L;

  sprintf(str, "df %s", path);

  if ((fp = popen(str, "r")) != NULL)
  {
    fgets(str, 100, fp);
    fgets(str, 100, fp);
    pclose(fp);
    sscanf(str, "%*s %*s %*s %lu", &frei);

    if (frei >= 1024 * 1024 * 1024)
      frei = (1024 * 1024 * 1024);
    else
      frei = (frei * 1024);
  }

  return (frei);
}

/************************************************************************/
/*                                                                      */
/* Fuer das erweiterte AUTOBIN-Protokoll wird die File-Zeit in einem    */
/* Bitfeld gespeichert (s. Doku zu DPBOX), wie es BC++ bei dem Befehl   */
/* getftime verwendet. Hier wird die Linux-Zeit der letzten Aenderung   */
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
/* Fuer DOS-Kompatibilitaet - Programm starten und Exit-Code des        */
/* Programms zurueckgeben                                               */
/*                                                                      */
/************************************************************************/

int
spawnl(int mode, const char *path, const char *arg0, const char *arg1,
       const char *arg2, const char *arg3, const char *arg4, const char *arg5)
{
  pid_t pid;
  int   status;

  if (mode != P_WAIT)
   {
    errno = EINVAL;
    return (-1);
   }

  if ((pid = fork()) < 0)
   {
    errno = ENOMEM;
    return (-1);
   }
  else
  {
    if (pid == 0)
    {
      if (execl(path, arg0, arg1, arg2, arg3, arg4, arg5, NULL) == -1)
      {
        exit(-1);
      }
    }
  }

  waitpid(pid, &status, 0);
  return(status);
}

/************************************************************************/
/*                                                                      */
/* Groesse des freien RAM feststellen (wenig aussagekraeftig durch      */
/* SWAP-Partition). Schlaegt der Aufruf fehl, wird 0 gemeldet.          */
/*                                                                      */
/************************************************************************/

ULONG
coreleft(void)
{
  FILE  *fp;
  char  str[100];
  ULONG frei = 0L;

  if ((fp = popen("free -b", "r")) != NULL)
  {
    fgets(str, 100, fp);
    fgets(str, 100, fp);
    pclose(fp);
    sscanf(str, "%*s %*s %*s %lu", &frei);
  }
  return(frei);
}

/************************************************************************/
/*                                                                      */
/* Befehlszeile auf Zeichen pruefen, die von der Shell ausgewertet      */
/* werden. Diese werden entwertet durch ein vorangesetztes '\\'.        */
/*                                                                      */
/************************************************************************/

void
security_check(char *cmd)
{
  char *cmdpoi = cmd;
  char tmpcmd[MAXPATH];
  register int i = 0;
  char ch;

  for (; i < MAXPATH - 2; ++i)
  {
    ch = *cmdpoi++;
    if (ch == NUL)
                break;
    if (strchr("*?\\|&;()<>$'`{}[]^#\"", ch) != NULL)
      tmpcmd[i++] = '\\';
    tmpcmd[i] = ch;

  }

  tmpcmd[i] = NUL;
  strcpy(cmd, tmpcmd);
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
  struct timeval     timevalue;
  int                max_fd = 0;
  int                count;
  register int       i;
  int                len;
  char               buffer[1024];
  socklen_t          clilen;
  struct sockaddr_un cli_addr;
  static UWORD       roundcounter = 1;
#ifndef NO_WATCHDOG
  int                lasttic10 = tic10;
#endif
#ifdef KERNELIF
  int                kif_fd = -1;
#endif

  gettimeofday(&tv, &tz);

  tic10 = (tv.tv_sec - tv_old.tv_sec) * 100 +
          (tv.tv_usec - tv_old.tv_usec) / 10000;

#ifndef NO_WATCHDOG
  if (tic10 - lasttic10)
    if (write(watch_dog[1], "\0", 1) != 1)
    {
    exit(1);
   }
#endif

  FD_ZERO(&rmask);

  /* Behandlung der Konsole bzw. Socket-Verbindung */
  switch (console_type)
  {
      /* Ein/Ausgabe ueber normales Terminal */
      case CONS_TERM_RUNNING:
        FD_SET(0, &rmask);                          /* Eingabe Console      */
        max_fd = 1;
        break;

      /* Ein/Ausgabe ueber Socket, aber noch nicht verbunden */
      case CONS_SOCKET_WAITING:
        FD_SET(sockfd, &rmask);
        if (sockfd > max_fd - 1)
          max_fd = sockfd + 1;
        break;

      /* Ein/Ausgabe ueber verbundenen Socket */
      case CONS_SOCKET_CONNECTED:
        FD_SET(consockfd, &rmask);
        if (consockfd > max_fd - 1)
          max_fd = consockfd + 1;
        break;

      /* Alles andere ... */
      default:
        break;
  }

/*

  if (!use_socket)
  {

    FD_SET(0, &rmask);
    max_fd = 1;
  }
  else
  {
    if (!soc_con)
    {
      FD_SET(sockfd, &rmask);
      if (sockfd > max_fd - 1)
        max_fd = sockfd + 1;
    }
    else
    {

      FD_SET(consockfd, &rmask);
      if (consockfd > max_fd - 1)
        max_fd = consockfd + 1;
    }
  }
*/

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
        FD_SET(l1port[i].kisslink, &rmask);
        if (l1port[i].kisslink > max_fd -1)
          max_fd = l1port[i].kisslink + 1;
      }
    }
  }

#ifdef KERNELIF
  /* IP-Interface Filedescriptor ermitteln */
  if ((kif_fd = ifip_active()) != -1) /* ein aktives Kernelinterface */
  {
      FD_SET(kif_fd, &rmask);
      if (kif_fd > max_fd - 1)
        max_fd = kif_fd + 1;
  }
#endif

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
 if (
#ifdef KERNELIF
        (l1port[i].kisstype == KISS_KAX25)
     || (l1port[i].kisstype == KISS_KAX25KJD)
#endif
#if defined(KERNELIF) && defined(AX_IPX)
     ||
#endif
#ifdef AX_IPX
         (l1port[i].kisstype == KISS_IPX)
#endif
    )
 {
      FD_SET(l1port[i].kisslink, &rmask);
      if (l1port[i].kisslink > max_fd -1)
        max_fd = l1port[i].kisslink + 1;
    }
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

  /* Konsolen/Socketverbindung bearbeiten */
  switch (console_type)
  {
    /* Konsole ist ein Terminal */
    case CONS_TERM_RUNNING:
      /* Zeichen auf der Standardeingabe ? */
      if (FD_ISSET(0, &rmask))
      {
        if ((len = read(0, buffer, 1024)) != -1)
          host_to_queue(buffer, len);
        else
        {
          /* Oh, oh, Fehler beim Lesen von der Konsole */
          /* Es werden nun weitere Konsolenoperationen */
          /* unterdrueckt. */
          console_type = CONS_NO_CONSOLE;
        }
      }
      break;

    /* Socket wartet auf Verbindung */
    case CONS_SOCKET_WAITING:
      /* Wartende Verbindung annehmen */
      if (FD_ISSET(sockfd, &rmask))
      {
        clilen = sizeof(cli_addr);
        consockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (consockfd >= 0)
          console_type = CONS_SOCKET_CONNECTED;
      }
      break;

    /* Daten von verbundenem Socket */
    case CONS_SOCKET_CONNECTED:
      if (FD_ISSET(consockfd, &rmask))
      {
        len = read(consockfd, buffer, 1024);
        if ((len == -1) || (len == 0))
        {
          close(consockfd);
          consockfd = -1;
          console_type = CONS_SOCKET_WAITING;
        }
        else
          host_to_queue(buffer, len);
      }
      break;

    default:
      break;
  }

/*

  if (!use_socket)
  {
    if (FD_ISSET(0, &rmask))
      if ((len = read(0, buffer, 1024)) != 0)
        host_to_queue(buffer, len);
  }
  else
  {
    if (!soc_con)
    {
      if (FD_ISSET(sockfd, &rmask))
      {
        clilen = sizeof(cli_addr);
        consockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (consockfd >= 0)
          soc_con = TRUE;
      }
    }
    else
      if (FD_ISSET(consockfd, &rmask))
      {
        len = read(consockfd, buffer, 1024);
        if ((len == -1) || (len == 0))
        {
          close(consockfd);
          soc_con = FALSE;
        }
        else
          host_to_queue(buffer, len);
      }
  }
*/

#ifdef KERNELIF
  /* IP-Interface RX */
  if ((kif_fd != -1) && (FD_ISSET(kif_fd, &rmask)))
    ifip_frame_to_router();
#endif

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


    /* pruefen ob dieser Port einen lesbaren Filedescriptor hat */
    if (FD_ISSET(l1port[i].kisslink, &rmask))
    {
      /* je nach Interfacetyp den richtigen RX aufrufen */
      switch (l1port[i].kisstype)
      {
#ifdef KERNELIF
        /* Kernel-AX25-Port RX */
        case KISS_KAX25 :
        case KISS_KAX25KJD : ifax_rx(l1port[i].kisslink);
                             break;
#endif
#ifdef AX_IPX
        /* AXIPX-RX */
        case KISS_IPX : axipx_recv();
                        break;
#endif
        default : continue;
      }
    }
  }

  if (kiss_active)
  {
    for (i = 0; i < L1PNUM; ++i)
    {
      if (l1port[i].kisslink == -1)
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
        if (FD_ISSET(l1port[i].kisslink, &rmask))
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
  sync();
  /* CRASH(); */
  /*system("/sbin/shutdown -r now");*/
/* Hierher wird man wohl nur kommen, wenn tnn nicht als root gestartet  */
/* wurde. Nun gibt es 2 Moeglichkeiten: entweder wir starten von vorne, */
/* weil die Resourcen ja breits freigegeben wurden, oder aber wir       */
/* beenden das Programm, weil das ja mehr oder weniger gefordert wurde. */
/* Sinnvoller ist es wohl, wenn wir das Programm verlassen, in der      */
/* Hoffnung, dass es danach regulaer von einem Batch erneut gestartet   */
/* wird. Das ist jedenfalls besser, als nix zu tun.                     */
  exit(1);
}

/************************************************************************/
/*                                                                      */
/* Shell-Befehl - Linux-spezifischer Teil:                              */
/* Das gewuenschte Programm wird als Hintergrundprozess gestartet.      */
/*                                                                      */
/************************************************************************/

BOOLEAN
tnnshell(char *cmdline)
{
  char sysline[MAXPATH + 1];

  char cFirstArg[MAXPATH + 1];

  char *pFirstArg = NULL;


  MBHEAD* mbp;

  struct termios term;

  /* In TNB-Files darf die interaktive Shell nicht verwendet werden */
  if ((tnnb_aktiv == TRUE) && (strlen(cmdline) == 0))
  {
    putmsg("Interactive shell is not allowed in tnb-files !\r");
    return (FALSE);
  }

  /* Haben wir eine Shell ? */
  if (cShell[0] == 0)
  {
    putmsg("No shell set, use 'setshell' to set one !\r");
    return (FALSE);
  }

  /* Initialisieren */
  memset(sysline, 0, MAXPATH);
  memset(cFirstArg, 0, MAXPATH);
  memset(&term, 0, sizeof(struct termios));

  term.c_iflag = ICRNL | IXOFF;
  term.c_oflag = OPOST | ONOCR; /* ONLCR */
  term.c_cflag = CS8 | CREAD | CLOCAL;
  term.c_lflag = ISIG | ICANON;

/*
  term.c_iflag = IGNCR | IXOFF;
  term.c_oflag = OPOST | ONLCR | ONLRET | OCRNL;
  term.c_cflag = CS8 | CREAD | CLOCAL;
  term.c_lflag = ISIG | ICANON;
*/

  term.c_cc[VINTR]  =  3;  /* ^C */
  term.c_cc[VSUSP]  =  26; /* ^Z */
  term.c_cc[VQUIT]  =  28;
  term.c_cc[VERASE] =   8; /* ^H */
  term.c_cc[VKILL]  =  24; /* ^X */
  term.c_cc[VEOF]   =   4; /* ^D */

  strncpy(sysline, cmdline, MAXPATH);
  sysline[MAXPATH] = NUL;   /* Sicher ist sicher ... */

  /* Das erste Argument fuer exec bestimmen */
  if ((pFirstArg = strrchr(cShell, '/')) == NULL)
  {
    pFirstArg = cShell;
  }
  else
  {
    ++pFirstArg;    /* wir wollen das erste Zeichen hinter dem "/" */
  }

  if (strlen(cmdline) == 0)
    cFirstArg[0] = '-';

  strcat(cFirstArg, pFirstArg);

  /* neue Shell abforken */
  if ((userpo->child_pid = forkpty(&userpo->child_fd, NULL, &term, NULL)) == -1)
  {
    /* forkpty() nicht moeglich */
    userpo->child_pid = 0;
    putmsg("Can't fork child process !\r");
    return(FALSE);
  }
  else
  {
    if (userpo->child_pid == 0)        /* Kind-Prozess (die Shell)          */
    {
      if (strlen(cmdline) == 0)
      {
        execl(cShell, cFirstArg, NULL);
      }
      else
      {
        execl(cShell, cFirstArg, "-c", cmdline, NULL);
      }

      /* Wenn wir hier ankommen, dann ist wirklich was nicht ok ... */
      exit(-1);
     }
     else                               /* Hauptprogramm                     */
     {
      /* Timeout und Interaktivitaet setzen je nach Shell-Typ */
      /* Achtung, Timeout in 100ms-Schritten ! */
      if (strlen(cmdline) == 0)
      {
        mbp = getmbp();
        putprintf(mbp, "Invoking shell, type 'exit' to end your session.\r");
        seteom(mbp);
        userpo->child_timeout = 30000;      /* 5 Min. Timeout               */
        userpo->child_iactive = TRUE;       /* interaktive Shell            */
      }
      else
      {
        userpo->child_timeout = 6000;       /* 1 Min. Timeout               */
        userpo->child_iactive = FALSE;      /* keine interaktive Shell      */
      }

      userpo->status = US_EXTP;             /* externes Programm laeuft     */
      return(TRUE);
    }
  }
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
  pid_t c_pid;

  MBHEAD* mbp;

  for (userpo  = (USRBLK *) usccpl.head;
       userpo != (USRBLK *) &usccpl;
       userpo  = (USRBLK *) userpo->unext)
  {
    if (userpo->status != US_EXTP)      /* externer Prozess aktiv?      */
      continue;                         /* nein ..                      */

    if (userpo->child_pid == 0)         /* externer Prozess aktiv?      */
      continue;                         /* nein ..                      */

    c_pid = waitpid(userpo->child_pid,  /* dieser Prozess beendet?      */
                    NULL, WNOHANG);

    /* Prozess noch nicht beendet */
    if (c_pid == 0)                      /* noch nicht fertig?           */
    {
      /* Timeout-Warnung fuer interaktive Shell */
      if ((userpo->child_iactive == TRUE) && (userpo->child_timeout <= 6000))
      {
        MBHEAD* mbp;

        switch (userpo->child_timeout)
        {
          case 1000  :
          case 2000  :
          case 3000  :
          case 6000  : mbp = putals("WARNING: Your shell times out in ");
                       putprintf(mbp, "%u seconds !!!\r", (userpo->child_timeout / 100));
                       seteom(mbp);
                       break;

          default    : break;
        }
      }

      /* Shell-Timeout */
      if (--userpo->child_timeout == 0)         /* zu lange inaktiv?    */
      {
        kill(-(userpo->child_pid), SIGKILL);    /* Abbruch (Watchdog)   */
        c_pid = waitpid(userpo->child_pid,      /* Prozess-Status holen */
                NULL, 0);                       /* sonst gips 'n Zombie */
        userpo->child_pid = 0;
        putmsg("SHELL SESSION ABORTED (TIMEOUT)\r");
        break;
      }
      else
      {
        shell_to_user();
        continue;
      }
    }

    /* Shell ist geschlossen */
    if (userpo->child_fd != -1)
    {
      if (c_pid > 0)
        shell_to_user();    /* Letzte Daten abholen */

      /* Shell nun schon geschlossen ? */
      if (userpo->child_fd != -1)
      {
        close(userpo->child_fd);
        userpo->child_fd = -1;
      }
    }

    userpo->child_pid = 0;                      /* Prozess ist fertig   */
    userpo->child_timeout = 0;
    userpo->child_iactive = FALSE;
    userpo->status = US_CCP;

    /* und ab die Post ... */
    mbp = getmbp();
    prompt(mbp);
    seteom(mbp);
  }
}

/************************************************************************/
/*                                                                      */
/* Daten von der Shell abholen und zum User bringen                     */
/*                                                                      */
/************************************************************************/

void
shell_to_user(void)
{
  fd_set rset;

  struct timeval tvalue;

  int i;

  UBYTE usrtype;

  /* nur gueltige Descriptoren */
  if (userpo->child_fd == -1)
    return;

  /* Pruefen, ob die Verbindung noch Daten aufnehmen kann, ist eine L2/L4- */
  /* Verbindung zu voll, dann keine neuen Pakete generieren. An die Hostkonsole wird */
  /* ungebremst ausgegeben. */
  usrtype = g_utyp(userpo->uid);

  if (   ((usrtype == L2_USER) || (usrtype == L4_USER))
      && (((LNKBLK *)g_ulink(userpo->uid))->tosend > 100)
     )
    return;

  FD_ZERO(&rset);
  FD_SET(userpo->child_fd, &rset);

  tvalue.tv_usec = 0;
  tvalue.tv_sec = 0;

  /* Daten holen */
  if ((i = select((userpo->child_fd + 1), &rset, NULL, NULL, &tvalue)) < 0)
  {
    putmsg("ERROR while select()-ing descriptor !!!\r");
    close(userpo->child_fd);
    userpo->child_fd = -1;
    userpo->child_timeout = 1;  /* Aufraeumen macht shellsrv() */
  }
  else
  {
    /* Daten Shell -> User */
    if ((i > 0) && (FD_ISSET(userpo->child_fd, &rset)))
    {
      unsigned char data[8192];

      unsigned int uZaehler;

      ssize_t rd;


      MBHEAD* mbp;

      if ((rd = read(userpo->child_fd, data, 8192)) == -1)
      {
        /* Ist der abgearbeitete Befehl ohne jegliche Rueckgabe, dann wird der Fehler 5 */
        /* erzeugt. Im nicht-interaktiven Modus unterdruecken wir die Fehlermeldung, im */
        /* interaktiven Modus wird er normal angezeigt. */
        if ((errno == EIO) && (userpo->child_iactive == TRUE))
        {
          mbp = putals("");
          putprintf(mbp, "ERROR %u while read()-ing descriptor !!! Shell terminated !\r", errno);
          seteom(mbp);
        }
        close(userpo->child_fd);
        userpo->child_fd = -1;
        userpo->child_timeout = 1;  /* Aufraeumen macht shellsrv() */
      }

      if (rd > 0)
      {
        mbp = getmbp();

        for (uZaehler = 0; uZaehler < rd; ++uZaehler)
        {
          if (data[uZaehler] == 0x0D)
            continue;

          if (data[uZaehler] == 0x0A)
          {
            putchr(0x0D, mbp);
            continue;
          }

          putchr(data[uZaehler], mbp);
        }

        seteom(mbp);

        /* Timeout ruecksetzen */
        if (userpo->child_iactive == TRUE)
          userpo->child_timeout = 30000;
      }
    }
  }
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
      kill(-(userpo->child_pid), SIGKILL);    /* Abbruch (Watchdog)       */
      waitpid(userpo->child_pid,              /* Prozess-Status holen     */
            NULL, 0);                         /* sonst gips 'n Zombie     */
      putmsg("SHELL ABORTED by User\r");
      userpo->child_pid = 0;                  /* Prozess ist fertig       */
      userpo->child_fd = -1;
      userpo->child_iactive = FALSE;
      userpo->child_timeout = 0;
      userpo->status = US_CCP;
      return(TRUE);
    }
    else
    {
      struct timeval tvalue;
      int i;

      fd_set wset;

      FD_ZERO(&wset);
      FD_SET(userpo->child_fd, &wset);

      tvalue.tv_usec = 0;
      tvalue.tv_sec = 0;

      if ((i = select((userpo->child_fd + 1), NULL, &wset, NULL, &tvalue)) < 0)
      {
        putmsg("ERROR while select()-ing descriptor !!!)\r");
        userpo->child_fd = -1;
        userpo->child_timeout = 1;  /* Aufraeumen macht shellsrv() */
      }
      else
      {
        /* Daten User -> Shell */
        if ((i > 0) && (FD_ISSET(userpo->child_fd, &wset) && (mhdp->mbgc < mhdp->mbpc)))
        {
          unsigned char data[1024];
          unsigned int uBufSize = 0;

          while (mhdp->mbgc < mhdp->mbpc)
            data[uBufSize++] = getchr(mhdp);

          /* Daten zum Shell-Child schicken */
          if (write(userpo->child_fd, data, uBufSize) < 0)
          {
            /* Oh oh, das hat nicht geklappt */
            userpo->child_fd = -1;
            userpo->child_timeout = 1;  /* Aufraeumen macht shellsrv() */
          }
          else
          {
            /* Timeout ruecksetzen */
            if (userpo->child_iactive == TRUE)
              userpo->child_timeout = 30000;
          }

          dealmb(mhdp);
          userpo->mbhd = NULL;
        }
      }
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
  if (   (strncmp("/bin/", file, 5) == 0)
      || (strncmp("/dev/", file, 5) == 0)
      || (strncmp("/proc/", file, 6) == 0)
      || (strncmp("/sbin/", file, 6) == 0)
      || (strncmp("/usr/bin/", file, 9) == 0)
      || (strncmp("/usr/sbin/", file, 10) == 0)
      || (strncmp("/usr/local/bin/", file, 16) == 0)
      || (strncmp("/usr/local/sbin/", file, 17) == 0))
    return(FALSE);

  return(TRUE);
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

void
tnnexec(char *file_name)
{
  if (access(file_name, X_OK) == 0)     /* nur ausfuehrbares Programm   */
   {
    exit_all();                         /* Resourcen freigeben          */
    execl(file_name, NULL, NULL);       /* Programm starten             */
/* Hierher darf man eigentlich nicht kommen! Daher verlassen wir das    */
/* Programm in der Hoffnung, dass es danach regulaer von einem Batch    */
/* erneut gestartet wird.                                               */
    exit(1);
   }
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

void
init_console(void)
{
#ifdef L1TCPIP
  /* Interface Initialisieren. */
  InitIFC();
#endif /* L1TCPIP */

  if (console_type == CONS_TERM_DO_SETUP)
  {
    setvbuf(stdout, NULL, _IONBF, 0);           /* Ausgabe ungepuffert  */
    tcgetattr(fileno(stdin), &oconin_termios);
    tcgetattr(fileno(stdout), &oconout_termios);
    nconin_termios = oconin_termios;
    nconin_termios.c_cc[VTIME] = 0;
    nconin_termios.c_cc[VMIN] = 1;
    nconin_termios.c_cc[VSTART] = -1;
    nconin_termios.c_cc[VSTOP] = -1;
    nconin_termios.c_iflag = IGNBRK;
    nconin_termios.c_oflag = OPOST|ONLCR;
    nconin_termios.c_lflag = 0;
    nconin_termios.c_cflag = (CS8|CREAD|CLOCAL);
    tcsetattr(fileno(stdin), TCSADRAIN, &nconin_termios);
    nconout_termios = oconout_termios;
    nconout_termios.c_cc[VTIME] = 0;
    nconout_termios.c_cc[VMIN] = 1;
    nconout_termios.c_cc[VSTART] = -1;
    nconout_termios.c_cc[VSTOP] = -1;
    nconout_termios.c_iflag = IGNBRK|ICRNL;
    nconout_termios.c_oflag = OPOST|ONLCR|ONLRET|OCRNL;
    nconout_termios.c_lflag = 0;
    nconout_termios.c_cflag = (CS8|CREAD|CLOCAL);
    tcsetattr(fileno(stdout), TCSAFLUSH, &nconout_termios);

    consfile = stdout;
    console_type = CONS_TERM_RUNNING;
   }
  else
    consfile = NULL;

  hostq_root = NULL;
  hostq_last = NULL;
  hostq_len = 0;
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

void
exit_console(void)
{
  struct hostqueue *hostq_ptr;

  while (hostq_root != NULL)
  {
    hostq_ptr = hostq_root;
    hostq_root = hostq_ptr->next;
    free(hostq_ptr);
  }

  hostq_last = NULL;
  hostq_len = 0;
  exit_proc();

  if (console_type != CONS_TERM_RUNNING)
      return;

  tcsetattr(1, TCSADRAIN, &oconout_termios);
  tcsetattr(0, TCSADRAIN, &oconin_termios);

  console_type = CONS_NO_CONSOLE;
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

static void
alloc_hostqbuf(void)
{
  struct hostqueue *hostq_ptr;

  hostq_ptr = (struct hostqueue *) malloc(sizeof(struct hostqueue));
  if (hostq_ptr == NULL)
  {
    exit_all();
    exit(1);
  }

  hostq_ptr->first = 0;
  hostq_ptr->last = 0;
  hostq_ptr->next = NULL;
  if (hostq_root == NULL)
  {
    hostq_root = hostq_ptr;
  }
  else
  {
    hostq_last->next = hostq_ptr;
    hostq_last = hostq_ptr;
  }

  hostq_last = hostq_ptr;
}

/* put one character in host-queue */

static void
puthostq(char ch)
{
  struct hostqueue *hostq_ptr;

  if (hostq_root == NULL)
    alloc_hostqbuf();

  hostq_ptr = hostq_last;

  if (hostq_ptr->last >= HOSTQ_BUFLEN)
  {
    alloc_hostqbuf();
    hostq_ptr = hostq_last;
  }

  hostq_ptr->buffer[hostq_ptr->last] = ch;
  hostq_ptr->last++;
  hostq_len++;
}

/* get one character out of host-queue */

static char
gethostq(void)
{
  struct hostqueue *hostq_ptr;
  char ch;

  if ((hostq_len == 0) || (hostq_root == NULL)) /* sanity check         */
    return(0);

  hostq_ptr = hostq_root;
  ch = hostq_ptr->buffer[hostq_ptr->first];
  hostq_ptr->first++;
  hostq_len--;

  if (   (hostq_ptr->first == hostq_ptr->last)
      || (hostq_ptr->first == HOSTQ_BUFLEN))
  {
    if (hostq_ptr->next == NULL)
        {
      hostq_root = NULL;
      hostq_last = NULL;
    }
    else
      hostq_root = hostq_ptr->next;

    free(hostq_ptr);
  }
  return(ch);
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

void
hputc(char c)
{
  int res;

  if (tnb_ch)
   {
    if (consfile != NULL)
      fputc(c, consfile);

    return;
   }

  switch (console_type)
  {
    case CONS_SOCKET_CONNECTED:

      res = write(consockfd, &c, 1);

      if (res == -1)
       {
        close(consockfd);
        consockfd = -1;
        console_type = CONS_SOCKET_WAITING;
       }

      break;

    case CONS_TERM_RUNNING:

      if ((c == '\n') || (c == '\r'))
        putchar(LF);
      else
        putchar(c);
      break;

    default:
      break;

   }
}

/* return if character available */

BOOLEAN
ishget(void)
{
  return(hostq_len != 0);
}

/* return if output to console is stopped */

BOOLEAN
ishput(void)
{
  return(FALSE);
}

/* get one character from console */

char
hgetc(void)
{
  char ch = gethostq();

  if (console_type == CONS_TERM_RUNNING)
    if (ch == LF)
      ch = CR;

  return (ch);
}

/* put received characters in hostbuffer */

static void
host_to_queue(char *buffer, int len)
{
  char *bufptr;
  register int i = 0;

  bufptr = buffer;

  while (i < len)
  {
    puthostq(*bufptr);
    ++i;
    bufptr++;
  }
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/************************************************************************/

BOOLEAN
init_hardware(int argc, char *argv[])
{
  char *pEnvShell = NULL;
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

  /* Shell loeschen */
  memset(cShell, 0, sizeof(cShell));

  VMSG("--- Determining shell-program ...\n");

  /* Shell aus dem Environment holen */
  if ((pEnvShell = getenv("SHELL")) == NULL)
    fprintf(stderr, "Warning: SHELL not set, can't use external programs !!!\n");
  else
  {
    /* Shell kopieren */
    strncpy(cShell, pEnvShell, sizeof(cShell));

    /* und sicherheitshalber abschliessen */
    cShell[sizeof(cShell)] = 0;
    VMSG("--- Shell is '%s'\n", cShell);
  }

  VMSG("--- Reading ini-file ...\n");
  if (read_init_file(argc, argv))
    return(TRUE);

  VMSG("--- Entering pid-file checks ...\n");
  if (!init_proc())
    exit(1);
/*
  if (tnn_errfile[0] != NUL)
  {
  }
*/
  VMSG("--- Allocating mem for %ld buffers ...\n", tnn_buffers);

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
      exit(1);
    }
  }

  if (console_type == CONS_SOCKET_DO_SETUP)
  {
    VMSG("--- Initializing UNIX-socket interface ...\n");
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      fprintf(stderr, "Can't open socket !\n");
      fprintf(stderr, "Error %u : %s\n", errno, strerror(errno));
      fprintf(stderr, "Exiting.\n");
      close(sockfd);
      free(RAMBOT);

      if (kiss_active)
        exit_kisslink();
      exit(1);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, tnn_socket);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    unlink(serv_addr.sun_path);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
    {
      fprintf(stderr, "Can't bind socket !\n");
      fprintf(stderr, "Error %u : %s\n", errno, strerror(errno));
      fprintf(stderr, "Exiting.\n");
      close(sockfd);
      free(RAMBOT);

      if (kiss_active)
        exit_kisslink();
      exit(1);
    }

    if (listen(sockfd, 5) < 0)
    {
      fprintf(stderr, "Can't listen to socket !\n");
      fprintf(stderr, "Error %u : %s\n", errno, strerror(errno));
      fprintf(stderr, "Exiting.\n");
      free(RAMBOT);
      if (kiss_active)
        exit_kisslink();
      exit(1);
    }

    console_type = CONS_SOCKET_WAITING;

    if (getppid() != 1)
    {
      if (fork() != 0)
      {
        /* Parent */
        exit_proc();    /* eigenes PID-File loeschen */
        exit(0);
      }
      else
      {
        sleep(1);       /* warten, dass Parent sein PID-File loescht */
        /* Child */
        if (!init_proc())
          exit(1);
      }
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
#endif

  signal(SIGHUP, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

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
  pid_t  pid;
  time_t wdzeit;
  time_t zeit;
  int    num;
  int    status;
  char   buf[1000];

  VMSG("--- Initializing watchdog ...\n");

  if (pipe(watch_dog) < 0)
  {
    fprintf(stderr, "Error %u while creating watchdog's pipe: %s\n", errno, strerror(errno));
    fprintf(stderr, "Exiting.\n");
    exit(1);
  }

  /* Descriptoren auf non-blocking einstellen */
  num = fcntl(watch_dog[0], F_GETFL, 0);
  num |= O_NONBLOCK;
  fcntl(watch_dog[0], F_SETFL, num);

  num = fcntl(watch_dog[1], F_GETFL, 0);
  num |= O_NONBLOCK;
  fcntl(watch_dog[1], F_SETFL, num);

  /* Forken */
  if ((pid = fork()) < 0)
  {
    fprintf(stderr, "Error %u while fork()-ing in watchdog: %s\n", errno, strerror(errno));
    fprintf(stderr, "Exiting.\n");
    exit(1);
  }
  else
  {
    /* Na wer sind wir denn ... */
    if (pid == 0)
    {
      /* wir sind Child (das Hauptprogramm) */
      wdpid[0] = getpid();
      close(watch_dog[0]);

      sleep(1);

      return;
    }
    else
    {
      /* und wir sind Parent (der Watchdog) */
      wdpid[0] = pid;       /* Child  */
      wdpid[1] = getpid();  /* Parent */

      close(watch_dog[1]);

      VMSG(" +-> main loop has pid %u\n", wdpid[0]);
      VMSG(" +-> watchdog has pid %u\n", wdpid[1]);

      signal(SIGHUP, SIG_IGN);
      signal(SIGTSTP, SIG_IGN);
      signal(SIGTTIN, SIG_IGN);
      signal(SIGTTOU, SIG_IGN);
      signal(SIGSEGV, sigsegv);

      time(&zeit);
      time(&wdzeit);

      LOOP
      {
        /* Hat sich das Child beendet ? */
        if (waitpid(wdpid[0], &status, WNOHANG))
        {
          /* Child hat sich beendet */
          /* Es gibt nichts mehr zu killen, pid wegwerfen */
          wdpid[0] = 0;

          VMSG("!!! watchdog: child has exited !\n");

          if (bVerbose == TRUE)
          {
            if (WIFEXITED(status))
              VMSG(" +-> child exited with code %u\n", WEXITSTATUS(status));

            if (WIFSIGNALED(status))
            {
              VMSG(" +-> child exited because of signal %u\n", WTERMSIG(status));

              if (WCOREDUMP(status))
                VMSG(" +-> child wrote a core dump !\n");
            }
          }

          break;
        }

        /* Child sollte laufen, dann schauen, ob es was geschickt hat */
        num = read(watch_dog[0], &buf, sizeof(buf));

        if (   (num == 0)   /* schreibendes Ende der Pipe geschlossen    */
            || ((num < 0) && (errno != EWOULDBLOCK))) /* sonstige Fehler */
          break;

        if (num > 0)
        {
          time(&wdzeit);
          continue;
        }

        time(&zeit);

        if ((zeit - wdzeit) > 60)
          break;

        sleep(1);
      }

      time(&wdzeit);
      close(watch_dog[0]);

      if (wdpid[0] != 0)
      {
        VMSG("!!! watchdog: child has exited abnormally !\n");

        kill(-wdpid[0], SIGSEGV);
        /* Falls aus irgendeinem Grund SIGSEGV nicht zum Programmende fuehrt,   */
        /* kommt ein SIGKILL hinterher. Damit wird sichergestellt, dass der     */
        /* Childprozess beendet wird.                                           */
        sleep(1);
        kill(-wdpid[0], SIGKILL);
      }

      /* Hauptprogramm beenden */
     exit(0);
    }
  }
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
#ifdef DEBUG_MODUS
  printf("\nTheNetNode verursachte einen unbekannten Systemfehler!!!\n");
  printf("Letzte Funktion war: %s\n", lastfunc);
#endif
  quit_program(-1);
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

      kill(-(userpo->child_pid), SIGKILL);      /* Abbruch              */
      waitpid(userpo->child_pid, NULL, 0);      /* Prozess-Status holen */
  }

  /* Buffer freigeben */
  free(RAMBOT);

  /* Ab hier stehen keine Buffer mehr zur Verfuegung, alles was mit Buffern */
  /* zu tun hat, gibt nun nen Segfault ! */

  if (console_type == CONS_SOCKET_CONNECTED)
  {
    close(consockfd);
    consockfd = -1;
    console_type = CONS_SOCKET_WAITING;
  }

  if (console_type == CONS_SOCKET_WAITING)
  {
    close(sockfd);
    sockfd = -1;
  }
  else
    exit_console();

  if (kiss_active)
    exit_kisslink();

  exit_proc();
  printf("\r\n");
  return(0);
}

/************************************************************************/
/*                                                                      */
/* Systembelastung durch TNN berechnen                                  */
/* diese Routine muss alle 10 Sekunden aufgerufen werden                */
/*                                                                      */
/************************************************************************/
void
calculate_load(void)
{
  static long   clktck = 0;
  static int    oldtime;
  static float  oldjiffies[2];
  static pid_t  tnnpid;
  FILE         *fp;
  int           utime;
  int           stime;
  register int  gtime;
  char          proc_name[80];

  /* im ersten Durchlauf alles aufraeumen */
  if (clktck == 0)
  {
    clktck = sysconf(_SC_CLK_TCK);
    tnnpid = getpid();
    oldjiffies[0] = 0;
    oldjiffies[1] = 0;
    oldtime = 0;
    load[0] = 0;
    load[1] = 0;
    load[2] = 0;
  }

  sprintf(proc_name , "/proc/%d/stat", tnnpid);

  /* stat-Datei von TNN oeffnen                                         */
  if ((fp = fopen(proc_name, "r")))
  {
    fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d "
               "%*d %*u %*u %*u %*u %*u %d %d",
                &utime, &stime);
    fclose(fp);

    /* User- & System-Time addieren und in Sekunden umrechnen           */
    gtime = (utime + stime) - oldtime;

    /* die letzten Zeiten fuer neue Berechnung merken                   */
    oldtime = utime + stime;

    /* Prozente ausrechnen                                              */
    tnn_load = ((gtime * 100) / (10 * clktck));
  }
  else
    tnn_load = -1;                               /* Fehler !            */

  /*  Systemload aus /proc/loadavg lesen                                */
  if ((fp = fopen("/proc/loadavg", "r")))
  {
    fscanf(fp, "%f %f %f %*s %*s", &load[0], &load[1], &load[2]);
    fclose(fp);
  }
  else
    load[0] = -1;

  /* gesamte Systembelastung im letzten Messintervall errechnen         */
  if ((fp = fopen("/proc/uptime", "r")))
  {
    float actjiffies[2];

    fscanf(fp, "%f %f", &actjiffies[0], &actjiffies[1]);
    fclose(fp);

    if (oldjiffies[0] != 0 && oldjiffies[1] != 0)
      sys_load = 100 * (1 - ((actjiffies[1] - oldjiffies[1]) / (actjiffies[0] - oldjiffies[0])));

    oldjiffies[0] = actjiffies[0];
    oldjiffies[1] = actjiffies[1];
  }
  else
    sys_load = -1;
}

/************************************************************************/
/*                                                                      */
/* Load-Werte ausgeben                                                  */
/*                                                                      */
/************************************************************************/
void
print_load(MBHEAD *mbp)
{
  ULONG         user = 0;
  ULONG         sys = 0;
  struct rusage sysusage;

  if (tnn_load >= 0)
    putprintf(mbp, "\r          TNN-Load: %i%%", tnn_load);
  else
    putprintf(mbp, "\r          TNN-Load: can't read from /proc !");

  if (sys_load >= 0)
    putprintf(mbp, "\r           Sysload: %.2f%%", sys_load);
  else
    putprintf(mbp, "\r           Sysload: can't read from /proc !");

  /* IO-Last ausgeben (siehe auch man proc)                            */
  if (load[0] >= 0)
    putprintf(mbp, "\r           Loadavg: %.2f %.2f %.2f",
              load[0], load[1], load[2]);
  else                 /* mhh...Kernel ohne /proc-System compiliert ?  */
    putprintf(mbp, "\r           Loadavg: can't read from /proc/loadavg !");

  /* verbrauchte Systemzeit ausgeben, wenn moeglich                     */
  if (getrusage(RUSAGE_SELF, &sysusage) == 0)
  {
    user = (ULONG)sysusage.ru_utime.tv_sec * 1000L
                + sysusage.ru_utime.tv_usec / 1000L;
    sys =  (ULONG)sysusage.ru_stime.tv_sec * 1000L
                + sysusage.ru_stime.tv_usec / 1000L;

    if (getrusage(RUSAGE_CHILDREN, &sysusage) == 0)
    {
      user += (ULONG) sysusage.ru_utime.tv_sec * 1000L
                    + sysusage.ru_utime.tv_usec / 1000L;
      sys +=  (ULONG) sysusage.ru_stime.tv_sec * 1000L
                    + sysusage.ru_stime.tv_usec / 1000L;
    }

    if (user > 1000000L || sys > 1000000L)
      putprintf(mbp, "\r     CPU time used: user %lus, system %lus",
                user / 1000L, sys / 1000L);
    else
      putprintf(mbp, "\r     CPU time used: user %lums, system %lums",
                user, sys);
  }
}

/* ermoeglicht das Setzen einer anderen Shell */
void ccpsetshell(void)
{
  MBHEAD *mbp;
  char cBuf[512];

  const size_t tBufSize = sizeof(cBuf);

  register size_t tCmdlen = 0;

  struct stat tFileStat;

  /* Sysop will aendern und noch was in der Zeile da ? */
  if (issyso())
  {
    /* Sind noch Usereingaben da ? */
    if (skipsp(&clicnt, &clipoi))
    {
      /* Frischer Buffer */
      memset(cBuf, 0, tBufSize);

      /* Zeile lesen */
      for (; tCmdlen < tBufSize; ++tCmdlen)
      {
        if ((!clicnt) || (*clipoi == ' '))
          break;
        clicnt--;
        cBuf[tCmdlen] = (*clipoi++);
      }
    }
    else    /* Syntax anzeigen */
      {
        mbp = putals("usage:\r'setshell /path/to/shell' to set a new shell\r");
        putprintf(mbp, "'setshell ?'              to get the current setting\r");
        prompt(mbp);
        seteom(mbp);
        return;
      }
  }
  else      /* kein Sysop */
    {
      invmsg();
      return;
    }

  /* Aktuelle Einstellung anzeigen */
  if ((tCmdlen == 1) && (cBuf[0] == '?'))
  {
    mbp = putals("");
    putprintf(mbp, "current shell is : %s\r", cShell);
    prompt(mbp);
    seteom(mbp);
    return;
  }

  /* Neue Shell grob ueberpruefen */
  if (access(cBuf, F_OK) == -1)     /* gibt es das File ? */
  {
    mbp = putals("");
    putprintf(mbp, "error accessing file '%s': %s\r", cBuf, strerror(errno));
    prompt(mbp);
    seteom(mbp);
    return;
  }

  /* Nur regulaere Dateien und Links erlaubt */
  if (stat(cBuf, &tFileStat) == -1)
  {
    mbp = putals("");
    putprintf(mbp, "can't get stats for '%s': %s\r", cBuf, strerror(errno));
    prompt(mbp);
    seteom(mbp);
    return;
  }
  else
  {
    /* Pruefung */
    if (   ((tFileStat.st_mode & S_IFDIR) == S_IFDIR)   /* directory */
        || ((tFileStat.st_mode & S_IFSOCK) == S_IFSOCK) /* socket */
        || ((tFileStat.st_mode & S_IFBLK) == S_IFBLK)   /* block device */
        || ((tFileStat.st_mode & S_IFCHR) == S_IFCHR)   /* character device */
        || ((tFileStat.st_mode & S_IFIFO) == S_IFIFO)   /* fifo */
       )
    {
      mbp = putals("");
      putprintf(mbp, "'%s' is not a file, can't use !\r", cBuf);
      prompt(mbp);
      seteom(mbp);
      return;
    }
  }

  /* Ausfuehrbar muss es auch noch sein */
  if (access(cBuf, X_OK) == -1)     /* nur ausfuehrbares Programm */
  {
    mbp = putals("");
    putprintf(mbp, "file '%s' is NOT marked executable, not accepted !!!\r", cBuf);
    prompt(mbp);
    seteom(mbp);
    return;
  }

  /* Uebernehmen */
  strncpy(cShell, cBuf, min(sizeof(cShell), tBufSize));
  mbp = putals("");
  putprintf(mbp, "shell changed to : %s\r", cShell);
  prompt(mbp);
  seteom(mbp);
  return;
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

/* End of os/linux/linux.c */
