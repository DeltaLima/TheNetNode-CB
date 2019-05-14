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
/* File os/linux/init.c (maintained by: DF6LN)                          */
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

/* tfkiss: TNC-emulation for Linux
   Copyright (C) 1995-96 by Mark Wahl
   Procedures for initialization (init.c)
   created: Mark Wahl DL4YBG 95/09/17
   updated: Mark Wahl DL4YBG 96/03/11
 */

#include "tnn.h"

typedef enum
{
  DIRECTORY, ERRORFILE, PROCFILE, SOCKET, START, STOP, BUFFERS,
  DEV, LOCKFILE, SPEED, KISSTYPE, PORT, PERMS, ROUNDS, NIX
}
INITYP;

typedef struct
{
  const char     *name;
  INITYP          typ;
}
INICMD;

/* Hilfezaehler fuer korrekte Initialisierung mehrerer Ports */
#ifdef ATTACH
WORD tokenring_ports = 0;
WORD sixpack_ports   = 0;
#else
WORD tokenring_ports = -1;
WORD sixpack_ports = -1;
#endif /* ATTACH */

/* Mappingtabelle Parameter-Namen auf Enum-Werte */
static          INICMD
                inicmdtab[] =
{
  {"tnn_dir",           DIRECTORY},
  {"tnn_errfile",       ERRORFILE},
  {"tnn_procfile",      PROCFILE},
  {"tnn_socket",        SOCKET},
  {"tnn_start",         START},
  {"tnn_stop",          STOP},
  {"buffers",           BUFFERS},
  {"device",            DEV},
  {"tnn_lockfile",      LOCKFILE},
  {"speed",             SPEED},
  {"kisstype",          KISSTYPE},
  {"port",              PORT},
  {"perms",             PERMS},
  {"rounds",            ROUNDS},
  {"",                  NIX}
};

/*************************************************************************/
/* Analysiert den Wert eines INI-Eintrags                                */
/*************************************************************************/
static BOOLEAN
analyse_value(const char *str1, const char *str2)
{
  INICMD  *ip;
  int      tmp;
  unsigned u;

  /* Parameter in der Parametertabelle suchen */
  for (ip = inicmdtab; ip->typ < NIX; ip++)
  {
    if (stricmp(str1, ip->name) == 0)
      break; /* gefunden */
  }

  /* Parameter auswerten */
  switch (ip->typ)
  {
    /* Parameter "tnn_dir" */
    case DIRECTORY:
      /* Verzeichnisname kopieren und Laenge merken */
      strcpy(tnn_dir, str2);
      tmp = strlen(tnn_dir);
      /* abschliessendes "/" an den Pfad anhaengen */
      if (tnn_dir[strlen(tnn_dir) - 1] != '/')
        strcat(tnn_dir, "/");

      chdir(tnn_dir);
      /* in das Verzeichnis wechseln */
      if (chdir(tnn_dir) == -1)
        printf("WARNING: error while changing directory : %s", strerror(errno));

      return (FALSE);

    /* Parameter "tnn_errfile" */
    case ERRORFILE:
      /* Dateinamen kopieren */
      strcpy(tnn_errfile, str2);
      return (FALSE);

    /* Parameter "tnn_procfile" */
    case PROCFILE:
      /* Dateiname kopieren */
      strcpy(tnn_procfile, str2);
      return (FALSE);

    /* Parameter "tnn_socket" */
    case SOCKET:
      /* nur wenn Socket noch nicht gesetzt */
      if (console_type != CONS_SOCKET_DO_SETUP)
      {
        /* Socketnamen jetzt benutzt */
        strcpy(tnn_socket, str2);
        console_type = CONS_SOCKET_DO_SETUP;
        return (FALSE);
      }
      return (TRUE);

    /* Parameter "tnn_start" */
    case START:
      /* Programmnamen kopieren */
      strcpy(start_name, str2);
      return (FALSE);

    /* Parameter "tnn_stop" */
    case STOP:
      /* Programmnamen kopieren */
      strcpy(stop_name, str2);
      return (FALSE);

    /* Parameter "buffers" */
    case BUFFERS:
      /* Anzahl Buffer lesen */
      if (sscanf(str2, "%lu", &tnn_buffers) != 1)
        return (TRUE);

      return (FALSE);

    /* Parameter "rounds" */
    case ROUNDS:
      /* Anzahl Runden lesen */
      if (sscanf(str2, "%u", &u) != 1)
        return (TRUE);

      /* gegen Ueberlauf durch negative Werte durch Spielkinder, der sscanf   */
      /* liefert bei signed-Werten einen sehr hohen unsigned-Wert             */
      if (u > 65500)
        u = 65500;

      /* gewuenschten Wert runterteilen um das nicht spaeter im Vergleicher   */
      /* tun zu muessen, auf Nachkommastellen keinen Wert legen               */
      maxrounds = (u / 100);

      /* zu wenig Drehzahl ist ungesund                                       */
      if (maxrounds < 1)
        maxrounds = 1;

      return (FALSE);

    /* Parameter "perms" */
    case PERMS:
      /* Wert fuer umask lesen */
      if (sscanf(str2, "%o", &u) != 1)
        return (TRUE);

      /* neue umask setzen */
      umask(u);
      return (FALSE);

    /* Parameter "device" */
    case DEV:
      /* schon alle Ports vergeben ? */
      if (max_device >= L1PNUM - 1)
        return (TRUE);

      /* Device merken */
      strcpy(l1port[++max_device].device, str2);
      return (FALSE);

    /* Parameter "tnn_lockfile" */
    case LOCKFILE:
      if (   (max_device < 0)                           /* kein Device  */
          || (l1port[max_device].tnn_lockfile[0]))      /* Name doppelt */
        return (TRUE);

      /* Lockfile merken */
      strcpy(l1port[max_device].tnn_lockfile, str2);
      return (FALSE);

    /* Parameter Speed */
    case SPEED:
      if (   (max_device < 0)                           /* kein Device    */
          || (sscanf(str2, "%d", &tmp) != 1))           /* Wert ermitteln */
        return (TRUE);

      l1port[max_device].speedflag = 0;

      /* Geschwindigkeits-Code suchen */
      switch (tmp)
      {
        case 0:
          l1port[max_device].speed = 0;
          return (FALSE);

        case 9600:
          l1port[max_device].speed = B9600;
          return (FALSE);

        case 19200:
          l1port[max_device].speed = B19200;
          return (FALSE);

        case 38400:
          l1port[max_device].speed = B38400;
          return (FALSE);

        case 57600:
#ifdef B57600
          l1port[max_device].speed = B57600;
#else
          l1port[max_device].speed = B38400;
          l1port[max_device].speedflag = ASYNC_SPD_HI;
#endif
          return (FALSE);

        case 115200:
#ifdef B115200
          l1port[max_device].speed = B115200;
#else
          l1port[max_device].speed = B38400;
          l1port[max_device].speedflag = ASYNC_SPD_VHI;
#endif
          return (FALSE);

        case 230400:
#ifdef B230400
          l1port[max_device].speed = B230400;
#else
          l1port[max_device].speed = B38400;
          l1port[max_device].speedflag = ASYNC_SPD_SHI;
#endif
          return (FALSE);

        case 460800:
#ifdef B460800
          l1port[max_device].speed = B460800;
#else
          l1port[max_device].speed = B38400;
#endif
          l1port[max_device].speedflag = ASYNC_SPD_WARP;
          return (FALSE);

        /* unbekannte Geschwindigkeiten */
        default:
          return (TRUE);
      }
      return (FALSE);

    /* Parameter "kisstype" */
    case KISSTYPE:
      if (   (max_device < 0)                           /* kein Device   */
          || (sscanf(str2, "%d", &tmp) != 1)            /* Wet ermitteln */
#ifndef AX_IPX
          || (tmp == KISS_IPX)
#endif
#ifndef AX25IP
          || (tmp == KISS_AXIP)
#endif
#ifndef VANESSA
          || (tmp == KISS_VAN)
#endif
#ifndef KERNELIF
          || (tmp == KISS_KAX25)
          || (tmp == KISS_KAX25KJD)
#endif
#ifndef SIXPACK
          || (tmp == KISS_6PACK)
          || ((tmp == KISS_6PACK) && (max_device != 0))
#endif
          || (tmp < KISS_NORMAL)
          || (tmp > KISS_MAX)
          || ((tmp == KISS_TOK) && (max_device != 0)))
        return (TRUE);

      /* Typ merken */
      l1port[max_device].kisstype = tmp;

      /* Geschwindigkeit fuer Tokenring-Port */
      if (tmp == KISS_TOK)
      {
        tkcom = 1;
        switch (l1port[0].speed)
        {
          case B9600:
            tkbaud = 96;
            break;

          case B19200:
            tkbaud = 192;
            break;

          case B38400:
            tkbaud = 384;
#ifndef B57600
            if (l1port[0].speedflag == ASYNC_SPD_HI)
              tkbaud = 576;
#endif
#ifndef B115200
            if (l1port[0].speedflag == ASYNC_SPD_VHI)
              tkbaud = 1152;
#endif
#ifndef B230400
            if (l1port[0].speedflag == ASYNC_SPD_SHI)
              tkbaud = 2304;
#endif
#ifndef B460800
            if (l1port[0].speedflag == ASYNC_SPD_WARP)
              tkbaud = 4608;
#endif

            break;

#ifdef B57600
          case B57600:
            tkbaud = 576;
            break;
#endif
#ifdef B115200
          case B115200:
            tkbaud = 1152;
            break;
#endif
#ifdef B230400
          case B230400:
            tkbaud = 2304;
            break;
#endif
#ifdef B460800
          case B460800:
            tkbaud = 4608;
            break;
#endif
        }
      }
      return (FALSE);

    /* Parameter "port" */
    case PORT:
      if (   (max_device < 0)
          || (sscanf(str2, "%d", &tmp) != 1)
          || (tmp < 0)
          || (tmp > L2PNUM)
          || (l1port[max_device].kisstype < 0))
        return (TRUE);

      /* Sonderbehandlung fuer Tokenring-Ports */
      if (l1port[max_device].kisstype == KISS_TOK)
      {
        /* noch Ports verfuegbar ? */
        if (++tokenring_ports >= L2PNUM)
          return (TRUE);

        l1ptab[tmp] = 0;
        l2ptab[max_device] = -1;
        ++used_l1ports;
        return (FALSE);
      }

#ifdef SIXPACK
      /* Sonderbehandlung fuer 6PACK-Ports */
      if (l1port[max_device].kisstype == KISS_6PACK)
      {
        /* noch Ports verfuegbar ? */
        if (++sixpack_ports >= L2PNUM)
          return (TRUE);

        l1ptab[tmp] = max_device;
        l2ptab[max_device] = -1;
        ++used_l1ports;
        return (FALSE);
      }
#endif
      /* alle andere Devices */
      /* noch Ports verfuegbar ? */
      if (++used_l1ports >= L2PNUM)
        return (TRUE);

      if (   l1ptab[tmp] == -1                          /* Port noch nicht      */
          && l2ptab[max_device] == -1)                  /* verwendet?           */
      {
        l1ptab[tmp] = max_device;
        l2ptab[max_device] = tmp;
      }

      /* Evtl. sollte noch geprueft werden, ob die Zuordnung der Vanessa-     */
      /* ports konsistent ist.                                                */
      return (FALSE);

    default:
      return (TRUE);
  }
  return (FALSE);
}

/************************************************************************/
/* Fuegt das TNN-Verzeichnis vorne hinzu                                */
/************************************************************************/
void
add_tnndir(char *str)
{
  char temp[80];

  /* leere Strings verarbeiten wir nicht */
  if (str[0] == NUL)
          return;

  /* nur bearbeiten, wenn erstes Zeichen nicht / ist */
  if (str[0] != '/')
          {
          strcpy(temp, tnn_dir);
          strcat(temp, str);
          strcpy(str, temp);
         }
}

/************************************************************************/
/* Lesen des TNN-INI-Files und des Kommandozeilenparameter              */
/************************************************************************/
BOOLEAN
read_init_file(int argc, char *argv[])
{
  FILE           *init_file_fp;
  BOOLEAN         wrong_usage = FALSE;
  BOOLEAN         file_end = FALSE;
  BOOLEAN         file_corrupt = FALSE;
  BOOLEAN         warning = FALSE;
  char            line[82];
  char            str1[82];
  char            str2[82];
  char            tmp_str[80];
  int             rslt;
  char           *str_ptr;
  int             scanned;
  int             i;
  DEVICE         *l1pp;
#ifdef AX_IPX
  int             axipx_ports = 0;
#endif
#ifdef AX25IP
  int             ax25ip_ports = 0;
#endif
#ifdef L1TELNET
  int             telnet_ports = 0;
#endif /* L1TELNET */
#ifdef L1HTTPD
  int             httpd_ports = 0;
#endif /* L1HTTPD */
#ifdef L1IPCONV
  int             ipconv_ports = 0;
#endif /* L1IPCONV */
#ifdef L1IRC
  int             irc_ports = 0;
#endif /* L1IRC */

  /* erst mal alles initialisieren */
  tnn_buffers = TNN_BUFFERS;
  max_device = -1;
#ifdef ATTACH
  tokenring_ports = 0;
  sixpack_ports   = 0;
#else
  tokenring_ports = -1;
#endif /* ATTACH */
  used_l1ports = -1;
  kiss_active = FALSE;
  tnn_socket[0] =
  start_name[0] =
  stop_name[0] = NUL;
  console_type = CONS_TERM_DO_SETUP; /* wir erwarten erst einmal eine Konsole */

  /* alle L1-Ports durchgehen und initialisieren */
  for (i = 0; i < L1PNUM; ++i)
  {
    l1pp = &l1port[i];
    l1pp->device[0] = NUL;
    l1pp->tnn_lockfile[0] = NUL;
    l1pp->speed = 0;
    l1pp->speedflag = 0;
    l1pp->kisstype = KISS_NIX;
    l1pp->kisslink = -1;
    l1pp->port_active = FALSE;
    l2ptab[i] = -1;
  }

  /* alle L2-Ports durchgehen und initialisieren */
  for (i = 0; i < L2PNUM; ++i)
  {
    l1ptab[i] = -1;
    CLR_L1MODE(i);
    SET_L1MODE(i, MODE_off);
  }

  strcpy(tnn_initfile, "tnn.ini");
  scanned = 1;
  unlock = 0;

  /* Kommandozeilenparameter auswerten */
  while ((scanned < argc) && (!wrong_usage))
  {
        /* Parameter "-i" (alternatives INI-File) */
    if (strcmp(argv[scanned], "-i") == 0)
    {
      scanned++;
      if (scanned < argc)
      {
        strcpy(tnn_initfile, argv[scanned]);
      }
      else
        wrong_usage = TRUE;
    }
    else if (strcmp(argv[scanned], "-s") == 0)
           /* Parameter "-s" (alternativ Socket) */
                {
          scanned++;
      if (scanned < argc)
      {
        strcpy(tnn_socket, argv[scanned]);
        console_type = CONS_SOCKET_DO_SETUP;
      }
      else
        wrong_usage = TRUE;
    }
    else if (strcmp(argv[scanned], "-u") == 0)
                /* Parameter "-u" (Lock-Files loeschen) */
                {
      unlock = 1;
    }
    else if (strcmp(argv[scanned], "-v") != 0)
    {
      wrong_usage = TRUE;
    }
    scanned++;
  }

  /* falsche / unbekannte Kommandozeilenparameter */
  if (wrong_usage)
  {
    printf("Usage : tnn [-i <init-file>] [-s <tnn-socket>] [-u] [-v]\n");
    return (TRUE);
  }

  /* INI-File oeffnen */
  if (!(init_file_fp = fopen(tnn_initfile, "r")))
  {
    warning = TRUE;
    str_ptr = getenv("HOME");
    if (str_ptr != NULL)
    {
      sprintf(tmp_str, "%s/%s", str_ptr, tnn_initfile);
      if ((init_file_fp = fopen(tmp_str, "r")) != NULL)
        warning = FALSE;
    }
#ifdef INIPATH
    if (!init_file_fp)
    {
      sprintf(tmp_str, "%s/%s", INIPATH, tnn_initfile);
      if ((init_file_fp = fopen(tmp_str, "r")) != NULL)
        warning = FALSE;
    }
#endif
  }

  /* Datei nicht gefunden */
  if (warning)
  {
#ifdef ATTACH
    /* tnn_procfile muss definiert sein! */
    strcpy(tnn_procfile, "tnn.pid");
    return(FALSE);
#else
    printf("ERROR: %s not found\n\n", tnn_initfile);
    return (TRUE);
#endif /* ATTACH */
  }

  /* solange noch was im INI_File ist */
  while (!file_end)
  {
    /* eine Zeile einlesen */
        if (fgets(line, 82, init_file_fp) == NULL)
    {
      file_end = TRUE;
    }
    else
    {
      /* Zeile konnte nicht eingelesen werden */
          if (strlen(line) == 82)
      {
        file_end = TRUE;
        file_corrupt = TRUE;
      }
      else
      {
        if (line[0] != '#')
        {                       /* ignore comment-lines */
          rslt = sscanf(line, "%s %s", str1, str2);
          switch (rslt)
            {
              case EOF:        /* ignore blank lines */
                break;
              case 2:
                if (analyse_value(str1, str2)) /* Zeile analysieren */
                {
                  file_end = TRUE;
                  file_corrupt = TRUE;
                }
                break;
              default:
                file_end = TRUE;
                file_corrupt = TRUE;
                break;
            }
        }
      }
    }
  }
  fclose(init_file_fp);

  if (file_corrupt)
  {
    printf("ERROR: %s is in wrong format, wrong line:\n%s\n\n", tnn_initfile, line);
    return (TRUE);
  }
  else
  {
    for (i = 0; i < L1PNUM; i++)
    {
#ifdef AX_IPX
      if (l1port[i].kisstype == KISS_IPX)
      {
        if (axipx_ports == 0)
          ++axipx_ports;
        else
          l1port[i].kisstype = KISS_NIX;
      }
#endif
#ifdef AX25IP
      if (l1port[i].kisstype == KISS_AXIP)
      {
        if (ax25ip_ports == 0)
          ++ax25ip_ports;
        else
          l1port[i].kisstype = KISS_NIX;
      }
#endif
#ifdef L1TELNET
      if (l1port[i].kisstype == KISS_TELNET)
      {
        if (telnet_ports == 0)
          ++telnet_ports;
        else
          l1port[i].kisstype = KISS_NIX;
      }
#endif /* L1TELNET */
#ifdef L1HTTPD
      if (l1port[i].kisstype == KISS_HTTPD)
      {
        if (httpd_ports == 0)
          ++httpd_ports;
        else
          l1port[i].kisstype = KISS_NIX;
      }
#endif /* L1HTTPD */
#ifdef L1IPCONV
      if (l1port[i].kisstype == KISS_IPCONV)
      {
        if (ipconv_ports == 0)
          ++ipconv_ports;
        else
          l1port[i].kisstype = KISS_NIX;
      }
#endif /* L1IPCONV */
#ifdef L1IRC
      if (l1port[i].kisstype == KISS_IRC)
      {
        if (irc_ports == 0)
          ++irc_ports;
        else
          l1port[i].kisstype = KISS_NIX;
      }
#endif /* L1IPCONV */

      if (*(l1port[i].tnn_lockfile) != NUL)
        add_tnndir(l1port[i].tnn_lockfile);
    }
    add_tnndir(tnn_errfile);
    add_tnndir(tnn_procfile);
    add_tnndir(tnn_socket);

    if (used_l1ports >= 0)
      kiss_active = TRUE;

        /* externes Programm starten wenn gesetzt */
    if (start_name[0])
      system(start_name);

    return (FALSE);
  }
}

/************************************************************************/
/* Proc-File schreiben                                                  */
/************************************************************************/
BOOLEAN
init_proc(void)
{
  FILE *fp = NULL;
  pid_t pid = 0;
  int killresult = 0;

  VMSG("--- Checking pid-file ...\n");

  fp = fopen(tnn_procfile, "r");

  if (fp != NULL)               /* altes tnn.pid vorhanden?     */
  {
    if (fscanf(fp, "%d", &pid) == 1)  /* alte PID lesen               */
    {
      killresult = kill(pid, 0);

      if (killresult != -1)   /* lebt das alte Programm noch? */
      {
        fclose(fp);
        return (FALSE);         /* lebt noch, also ENDE!        */
      }
    }
    fclose(fp);
  }

  VMSG("--- Creating new pid-file ...\n");

  /* neues File anleegn */
  fp = fopen(tnn_procfile, "w+");

  if (fp == NULL)
  {
    printf("ERROR: Can't create process file\n");
    return (FALSE);
  }

  /* aktuelle PID hineinschreiben */
  pid = getpid();
  fprintf(fp, "%d", pid);
  fclose(fp);

  return (TRUE);
}

/************************************************************************/
/* Proc-File loeschen                                                   */
/************************************************************************/
void
exit_proc(void)
{
  unlink(tnn_procfile);
}

/* End of os/linux/init.c */
