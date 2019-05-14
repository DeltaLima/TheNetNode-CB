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
/* File os/linux/l1linux.c (maintained by: DF6LN)                       */
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

#include "tnn.h"

UWORD wd_timer[L2PNUM];          /* Wenn dieser Timer gegen 0 geht, wird  */
                                 /* der Port zurueckgesetzt.              */
#define WATCHDOG_TIMEOUT 18000   /* Nach 3 Minuten ohne RX Port reset     */


static void config_tnc(int);
static void kissframe_to_tnc(int, BOOLEAN);
static void append_crc_16(char *, int *);
static int check_crc_16(char *, int *);
static void append_crc_rmnc(char *, int *);
#ifdef LOOPBACK
static void loopback(void);
#endif

static  UWORD   tic1m = 0;
#ifndef ATTACH
static  BOOLEAN tokenflag;
#endif /* ATTACH */
static  ULONG   recovery_count;
static  LONG    lost_token = 0;
static  LONG    token_pro_sec = 0L;     /* Anzahl Token/Sekunde         */
static  LONG    token_max_sec = 0L;     /* Maximale Anzahl              */
static  LONG    token_min_sec = 0L;     /* Minimale Anzahl              */
static  LONG    token_count = 0L;       /* Zaehler fuer Token           */
        LONG    rounds_pro_sec = 0L;    /* Anzahl Rounds/Sekunde        */
        LONG    rounds_max_sec = 0L;    /* Maximale Anzahl              */
        LONG    rounds_min_sec = 0L;    /* Minimale Anzahl              */
        LONG    rounds_count   = 0L;    /* Zaehler fuer Rounds          */

int   tkcom = -1;
UWORD tkbaud = 0;

static ULONG cd_timer[L2PNUM];   /*  0 =  Frame wurde gesendet..          */
                                 /*  != 0 Frame an TNC uebergeben, wird   */
                                 /*       bearbeitet bzw. Sendung laeuft. */
#define CD_TIMEOUT 3000          /* 30 s Timeout, falls Bestaetigung      */
                                 /* verloren gegangen ist (Tokenring?)    */
static const int Crc_16_table[256];
static const int Crc_rmnc_table[256];
static int check_crc_rmnc(char *, int *);
#ifndef ATTACH
static void tf_set_kiss(DEVICE *);
#endif /* ATTACH */

ULONG   tnn_buffers;
char    tnn_initfile[MAXPATH];
char    tnn_dir[MAXPATH];
char    tnn_errfile[MAXPATH];
char    tnn_procfile[MAXPATH];
BOOLEAN kiss_active;
DEVICE  l1port[L1PNUM];
WORD    max_device;
WORD    used_l1ports;
WORD    l1ptab[L2PNUM];
WORD    l2ptab[L1PNUM];

#ifdef VANESSA
extern BOOLEAN van_test(int);
WORD   found_vanessa;
static BOOLEAN check_van = FALSE;
#endif

#ifdef KERNELIF
extern void ifip_clearstat(void);
#endif

static ULONG token_sent = 0L;

/************************************************************************/
/*                                                                      */
/* Initialisierung der Variablen                                        */
/*                                                                      */
/************************************************************************/

void
l1init(void)
{
  register WORD i;

  /* alle Ports initialisieren */
  for (i = 0; i < L2PNUM; ++i)
  {
    portpar[i].speed = 12;
    portpar[i].major = NO_MAJOR;
    portstat[i].reset_count = 0L;
    portpar[i].reset_port = FALSE;
    cd_timer[i] = 0;
    kick[i] = FALSE;                            /* kein TNC sendet      */
    testflag[i] = FALSE;
    commandflag[i] = TRUE;
    wd_timer[i] = WATCHDOG_TIMEOUT;
  }

  /* Zaehler ruecksetzen */
  for (i = 0; i < L1PNUM; ++i)
    l1port[i].bad_frames = 0L;

  show_recovery = TRUE;
  recovery_count = 0L;
  tic1m = 0;

#ifdef VANESSA
  /* Vanessen suchen und einrichten */
  if (check_van)
   {
    found_vanessa = 0;
    vanessa_l1init();
    for (i = 0; i < L2PNUM; ++i)
     {
      if (kissmode(i) == KISS_VAN)
       {
         if (!van_test(i))
          l1port[l1ptab[i]].kisstype = KISS_NIX;
         else
         {
          l1port[l1ptab[i]].port_active = TRUE;
          ++found_vanessa;
         }
       }
     }
   }
#endif
#ifdef AX_IPX
  /* IPX-Port einrichten */
  for (i = 0; i < L2PNUM; ++i)
   {
    if (kissmode(i) == KISS_IPX)
     {
      l1port[l1ptab[i]].port_active = TRUE;
      break;                               /* nur 1 IPX-Port */
     }
   }
#endif
#ifdef AX25IP
  /* AXIP23-Port einrichten */
  for (i = 0; i < L2PNUM; ++i)
   {
    if (kissmode(i) == KISS_AXIP)
     {
      l1port[l1ptab[i]].port_active = TRUE;
      break;                               /* nur 1 AX25IP-Port */
     }
   }
#endif
#ifdef KERNELIF
  /* Kernel-AX.25-Ports einrichten */
  for (i = 0; i < L2PNUM; ++i)
    if (   (kissmode(i) == KISS_KAX25)
        || (kissmode(i) == KISS_KAX25KJD))
      l1port[l1ptab[i]].port_active = TRUE;
#endif
#ifdef SIXPACK
  /* 6PACK-Ring einrichten */
  for (i = 0; i < L2PNUM; ++i)
    if (kissmode(i) == KISS_6PACK)
    {
      Sixpack_l1init();
      break;
    }
#endif
#ifdef L1TELNET
  /* TELNET-Port einrichten */
  for (i = 0; i < L2PNUM; ++i)
   {
    if (kissmode(i) == KISS_TELNET)
     {
      l1port[l1ptab[i]].port_active = TRUE;
      break;                               /* nur 1 TELNET-Port */
     }
   }
#endif /* L1TELNET */
#ifdef L1HTTPD
  /* HTTPD-Port einrichten */
  for (i = 0; i < L2PNUM; ++i)
   {
    if (kissmode(i) == KISS_HTTPD)
     {
      l1port[l1ptab[i]].port_active = TRUE;
      break;                               /* nur 1 HTTPD-Port */
     }
   }
#endif /* L1HTTPD */
#ifdef L1IPCONV
  /* IPCONVERS-Port einrichten */
  for (i = 0; i < L2PNUM; ++i)
   {
    if (kissmode(i) == KISS_IPCONV)
     {
      l1port[l1ptab[i]].port_active = TRUE;
      break;                               /* nur 1 IPCONV-Port */
     }
   }
#endif /* L1IPCONV */
#ifdef L1IRC
  /* IPCONVERS-Port einrichten */
  for (i = 0; i < L2PNUM; ++i)
   {
    if (kissmode(i) == KISS_IRC)
     {
      l1port[l1ptab[i]].port_active = TRUE;
      break;                               /* nur 1 IPCONV-Port */
     }
   }
#endif /* L1IRC */
}

/************************************************************************/
/*                                                                      */
/* Serielle Schnittstellen initialisieren                               */
/* ( KISS, SMACK, Tokenring, teilweise 6PACK )                          */
/************************************************************************/

BOOLEAN
init_kisslink(void)
{
  WORD    i;
  UWORD   fehler;
  DEVICE *l1pp;
  struct  serial_struct ser_io;
  FILE   *tmp_lockfile;
  pid_t   mypid;
  int     flags;

  fehler = 0;            /* Merker fuer Fortschritt der Initialisierung */
  mypid = getpid();     /* eigene PID holen                             */

/************************************************************************/
/*                                                                      */
/* erstmal Lock-Files loeschen, wenn gefordert.                         */
/*                                                                      */
/************************************************************************/

  /* alte Lockfiles loeschen wenn gewuenscht */
  for (i = 0; i < L1PNUM; ++i)
  {
    l1pp = &l1port[i];
    if (unlock)
      if (*(l1pp->tnn_lockfile) != '\0')
        unlink(l1pp->tnn_lockfile);
    if (i == max_device)
      break;
  }

/************************************************************************/
/*                                                                      */
/* Lockfiles schreiben und Kiss-Ports initialisieren.                   */
/*                                                                      */
/************************************************************************/

  /* alle L1-ports durch gehen */
  for (i = 0; i < L1PNUM; ++i)
  {
    l1pp = &l1port[i];

/************************************************************************/
/*                                                                      */
/* Wenn Vanessa, naechsten Port.                                        */
/*                                                                      */
/************************************************************************/

#ifdef VANESSA
    if (stricmp("VANESSA", l1pp->device) == 0)
    {
      check_van = TRUE;
      l1pp->kisstype = KISS_VAN; /* falls Kisstype nicht angegeben      */
      if (i == max_device)
        break;
      else
        continue;
    }
#endif

/************************************************************************/
/*                                                                      */
/* Wenn IPX, auch naechsten Port.                                       */
/*                                                                      */
/************************************************************************/

#ifdef AX_IPX
    if (stricmp("IPX", l1pp->device) == 0)
    {
      l1pp->kisstype = KISS_IPX; /* falls Kisstype nicht angegeben      */
      l1pp->port_active = TRUE;
      if (i == max_device)
        break;
      else
        continue;
    }
#endif

/************************************************************************/
/*                                                                      */
/* Wenn AX25IP, auch naechsten Port.                                    */
/*                                                                      */
/************************************************************************/

#ifdef AX25IP
    if (stricmp("AX25IP", l1pp->device) == 0)
    {
      l1pp->kisstype = KISS_AXIP; /* falls Kisstype nicht angegeben     */
      l1pp->port_active = TRUE;
      if (i == max_device)
        break;
      else
        continue;
    }
#endif

/************************************************************************/
/*                                                                      */
/* Kernel-AX25 hat hier auch nix zu suchen                              */
/*                                                                      */
/************************************************************************/
#ifdef KERNELIF
    if (   (l1pp->kisstype == KISS_KAX25)
        || (l1pp->kisstype == KISS_KAX25KJD))
    {
      l1pp->port_active = TRUE;
      if (i == max_device)
        break;
      else
        continue;
    }
#endif

/************************************************************************/
/*                                                                      */
/* 6PACK hat hier auch nix zu suchen                                    */
/*                                                                      */
/************************************************************************/
#ifdef SIXPACK
    if (l1pp->kisstype == KISS_6PACK)
    {
      l1pp->port_active = TRUE;
      if (i == max_device)
        break;
      else
        continue;
    }
#endif

#ifdef L1TELNET
/************************************************************************/
/*                                                                      */
/* Wenn TELNET, auch naechsten Port.                                    */
/*                                                                      */
/************************************************************************/

    if (l1pp->kisstype == KISS_TELNET)
    {
      int TCPPort = atoi(l1pp->device);

      if (  (TCPPort < 1)
          ||(TCPPort > 65536))
      {
        printf("Warnung: Alte Syntax device %s ungueltig!!!\n"
               "         Neue Syntax device 1..65535.\n"
               "TCP-Port wird auf default 10023 gesetzt.\n"
               "Aenderung in der TNN.INI vornehmen!\n", l1pp->device);
        strncpy(l1pp->device, "10023", MAXPATH);
      }
      else
      {
        l1pp->port_active = TRUE;
      }

      if (i == max_device)
        break;
      else
        continue;
    }
#endif /* L1TELNET */

#ifdef L1HTTPD
/************************************************************************/
/*                                                                      */
/* Wenn HTTPD, auch naechsten Port.                                     */
/*                                                                      */
/************************************************************************/

    if (l1pp->kisstype == KISS_HTTPD)
    {
      int TCPPort = atoi(l1pp->device);

      if (  (TCPPort < 1)
          ||(TCPPort > 65536))
      {
        printf("Warnung: Alte Syntax device %s ungueltig!!!\n"
               "         Neue Syntax device 1..65535.\n"
               "TCP-Port wird auf default 18080 gesetzt.\n"
               "Aenderung in der TNN.INI vornehmen!\n", l1pp->device);
        strncpy(l1pp->device, "18080", MAXPATH);
      }
      else
        l1pp->port_active = TRUE;

      if (i == max_device)
        break;
      else
        continue;
    }
#endif /* L1HTTPD */

#ifdef L1IPCONV
/************************************************************************/
/*                                                                      */
/* Wenn IPCONV, auch naechsten Port.                                    */
/*                                                                      */
/************************************************************************/

    if (l1pp->kisstype == KISS_IPCONV)
    {
      int TCPPort = atoi(l1pp->device);

      if (  (TCPPort < 1)
          ||(TCPPort > 65536))
      {
        printf("Warnung: Alte Syntax device %s ungueltig!!!\n"
               "         Neue Syntax device 1..65535.\n"
               "TCP-Port wird auf default 13600 gesetzt.\n"
               "Aenderung in der TNN.INI vornehmen!\n", l1pp->device);
        strncpy(l1pp->device, "13600", MAXPATH);
      }
      else
        l1pp->port_active = TRUE;

      if (i == max_device)
        break;
      else
        continue;
    }
#endif /* L1IPCONV */

#ifdef L1IRC
/************************************************************************/
/*                                                                      */
/* Wenn IRC, auch naechsten Port.                                    */
/*                                                                      */
/************************************************************************/

    if (l1pp->kisstype == KISS_IRC)
    {
      int TCPPort = atoi(l1pp->device);

      if (  (TCPPort < 1)
          ||(TCPPort > 65536))
      {
        printf("Warnung: Alte Syntax device %s ungueltig!!!\n"
               "         Neue Syntax device 1..65535.\n"
               "TCP-Port wird auf default 13600 gesetzt.\n"
               "Aenderung in der TNN.INI vornehmen!\n", l1pp->device);
        strncpy(l1pp->device, "13600", MAXPATH);
      }
      else
        l1pp->port_active = TRUE;

      if (i == max_device)
        break;
      else
        continue;
    }
#endif /* L1IRC */

/************************************************************************/
/*                                                                      */
/* Zuerst Lock-File schreiben.                                          */
/*                                                                      */
/************************************************************************/
    l1pp->lock = -1;
    if (*(l1pp->tnn_lockfile) != '\0')
    {
      l1pp->lock = open(l1pp->tnn_lockfile, O_CREAT|O_EXCL, 0666);
      if (l1pp->lock == -1)
      {
        fehler = 1;
        printf("Error: device %s is locked by other user;\n"
               "       or unable to create lockfile %s\n",
               l1pp->device, l1pp->tnn_lockfile);
        printf("(%s)\n", strerror(errno));
      }
      else
      {
        close(l1pp->lock);
        tmp_lockfile = fopen(l1pp->tnn_lockfile, "w");
        fprintf(tmp_lockfile, "%10d\n", mypid);
        fclose(tmp_lockfile);
      }
    }

/************************************************************************/
/*                                                                      */
/* Port oeffnen.                                                        */
/*                                                                      */
/************************************************************************/

    if (fehler == 0)                  /* nur wenn Lock-File geschrieben */
    {                                 /* wurde oder nicht gefordert war */
      l1pp->kisslink = open(l1pp->device, O_RDWR);
      if (l1pp->kisslink == -1)
      {
        fehler = 2;
        printf("Error: can't open device %s\n", l1pp->device);
        printf("        (%s)\n", strerror(errno));
      }
    }

/************************************************************************/
/*                                                                      */
/* Alte Einstellungen der seriellen Schnittstelle merken                */
/*                                                                      */
/************************************************************************/
    if (fehler == 0)    /* nur wenn Port geoeffnet!                     */
    {
      tcgetattr(l1pp->kisslink, &(l1pp->org_termios));
      if (l1pp->speed == B38400)                        /* >= 38400 Bd  */
      {
        if (ioctl(l1pp->kisslink, TIOCGSERIAL, &ser_io) < 0)
        {
          fehler = 3;
          printf("Error: can't get actual settings for device %s\n", l1pp->device);
          printf("       (%s)\n", strerror(errno));
        }
      }
    }

/************************************************************************/
/*                                                                      */
/* Neue Einstellungen der seriellen Schnittstelle setzen                */
/*                                                                      */
/************************************************************************/

    if (fehler == 0)
    {
      l1pp->wrk_termios = l1pp->org_termios;
      l1pp->wrk_termios.c_cc[VTIME] = 0;        /* empfangene Daten     */
      l1pp->wrk_termios.c_cc[VMIN] = 0;         /* sofort abliefern     */
      l1pp->wrk_termios.c_iflag = IGNBRK;       /* BREAK ignorieren     */
      l1pp->wrk_termios.c_oflag = 0;            /* keine Delays oder    */
      l1pp->wrk_termios.c_lflag = 0;            /* Sonderbehandlungen   */

      l1pp->wrk_termios.c_cflag |= (CS8         /* 8 Bit                */
                                    |CREAD      /* RX ein               */
                                    |CLOCAL);   /* kein Handshake       */
      l1pp->wrk_termios.c_cflag &= ~(CSTOPB     /* 1 Stop-Bit           */
                                     |PARENB    /* ohne Paritaet        */
                                     |HUPCL);   /* kein Handshake       */

      /* TEST DG9OBU */
      /* pty's stellen wir auf non-blocking io */
      if (l1pp->speed == B0)
      {
        if ((flags = fcntl(l1pp->kisslink, F_GETFL, 0)) < 0)
          printf("Error: can't read pty's current flags !!!\n");

        if (fcntl(l1pp->kisslink, F_SETFL, (flags | O_NONBLOCK)) < 0)
          printf("Error: can't set pty's nonblocking-flag !!!\n");
      }

      if (l1pp->speed != B0)                    /* B0 -> pty soll ver-  */
      {                                         /* wendet werden        */
        cfsetispeed(&(l1pp->wrk_termios),       /* Empfangsparameter    */
                    l1pp->speed);               /* setzen               */
        cfsetospeed(&(l1pp->wrk_termios),       /* Sendeparameter       */
                    l1pp->speed);               /* setzen               */

        if (l1pp->speed == B38400)              /* wenn >= 38400 Bd     */
        {
          ser_io.flags &= ~ASYNC_SPD_MASK;      /* Speed-Flag -> 0      */
          ser_io.flags |= l1pp->speedflag;      /* Speed-Flag setzen    */

          if (ioctl(l1pp->kisslink, TIOCSSERIAL, &ser_io) < 0)
          {
            fehler = 4;
            printf("Error: can't set device settings on device %s\n", l1pp->device);
            printf("       (%s)\n", strerror(errno));
          }
        }
      }
    }

/************************************************************************/
/*                                                                      */
/* Serielle Schnittstelle auf neue Parameter einstellen, KISS-Empfang   */
/* initialisieren                                                       */
/*                                                                      */
/************************************************************************/

    if (fehler == 0)
    {
      tcsetattr(l1pp->kisslink, TCSADRAIN, &(l1pp->wrk_termios));
      l1pp->rx_state = ST_BEGIN;
      l1pp->rx_port = 0;
      l1pp->tx_port = 0;
      l1pp->port_active = TRUE;

      if (l1pp->kisstype == KISS_TF)
        tf_set_kiss(l1pp);
    }
    else
      break;
    if (i == max_device)
      break;
   }

/************************************************************************/
/*                                                                      */
/* Wenn kein Fehler aufgetreten ist, dann fuer gewisse Interfaces die   */
/* ersten Aktionen ausfuehren.                                          */
/*                                                                      */
/* Tokenring : Token senden                                             */
/* 6PACK     : TNC-Zaehlung anstossen                                   */
/*                                                                      */
/************************************************************************/

  if (fehler == 0)
  {
        /* Tokenring */
    if (l1port[0].kisstype == KISS_TOK)
    {
      send_kisscmd(0xff, CMD_TOKEN, 0);         /* Token senden          */
      token_sent = tic10;                       /* Zeitpunkt der Sendung */
      tokenflag = FALSE;                        /* Token unterwegs       */
    }
    return(FALSE);
  }

/************************************************************************/
/*                                                                      */
/* Wenn bei einem Teil der Initialisierung ein Fehler aufgetreten ist,  */
/* muessen alle bisher vorgenommenen Parameteraenderungen der seriellen */
/* Schnittstellen rueckgaengig gemacht werden. Ausserdem muessen alle   */
/* geoeffneten Files wieder geschlossen werden.                         */
/*                                                                      */
/************************************************************************/

  do
  {
    /* Port war schon offen, alte Einstellungen wiederherstellen */
    if ((fehler > 3) && (l1port[i].kisslink != -1))
      tcsetattr(l1port[i].kisslink, TCSADRAIN, &(l1port[i].org_termios));

    /* Port war schon offen, aber noch nicht veraendert, nur schliessen */
    if ((fehler > 2) && (l1port[i].kisslink != -1))
    {
      close(l1port[i].kisslink);
      l1port[i].kisslink = -1;
    }

    /* Port war nicht offen, aber es existiert ein Lockfile, schliessen */
    if ((fehler > 1) && (l1port[i].lock != -1))
    {
      close(l1port[i].lock);
      l1port[i].lock = -1;
    }

    /* Lockfile loeschen */
    if (l1port[i].tnn_lockfile[0] != '\0')
    {
      unlink(l1port[i].tnn_lockfile);
      l1port[i].tnn_lockfile[0] = '\0';
    }

    fehler = 4;         /* beim naechsten Durchgang alles restaurieren  */
  } while (--i >= 0);

  return(TRUE);
}

/************************************************************************/
/*                                                                      */
/* Serielle Schnittstellen wieder freigeben (fuer Programmende)         */
/*                                                                      */
/************************************************************************/

void exit_kisslink(void)
{
  register WORD i;
  char buffer[] = { FEND, 0xFF };

  for (i = 0; i < L1PNUM; ++i)
   {
    /* abgeschaltete Ports */
          if (!l1port[i].port_active)
      continue;
#ifdef AX_IPX
        /* AX_IPX */
        if (l1port[i].kisstype > KISS_IPX)
      continue;
#endif
#ifdef KERNELIF
    /* Kernel-AX.25 */
    if (   (l1port[i].kisstype == KISS_KAX25)
        || (l1port[i].kisstype == KISS_KAX25KJD))
      continue;
#endif
#ifdef AX25IP
    /* AX25IP */
    if (l1port[i].kisstype == KISS_AXIP)
      continue;
#endif
#ifdef VANESSA
    if (l1port[i].kisstype == KISS_VAN)
      continue;
#endif
#ifdef SIXPACK
    /* 6PACK */
    if (l1port[i].kisstype == KISS_6PACK)
      continue;
#endif
#ifdef L1TELNET
    /* TELNET-Server */
    if (l1port[i].kisstype == KISS_TELNET)
      continue;
#endif /* L1TELNET */
#ifdef L1HTTPD
    /* HTTPD-Server */
    if (l1port[i].kisstype == KISS_HTTPD)
      continue;
#endif /* L1HTTPD */
#ifdef L1IPCONV
    /* IPCONV-Server */
    if (l1port[i].kisstype == KISS_IPCONV)
      continue;
#endif /* L1IPCONV */
#ifdef L1IRC
    /* IRC-Server */
    if (l1port[i].kisstype == KISS_IRC)
      continue;
#endif /* L1IRC */

    /* TNC mit TheFirmware */
    if (l1port[i].kisstype == KISS_TF)
    {
      /* KISS-Kommando senden (TNC-Reset) */
      write(l1port[i].kisslink, buffer, 2);
      sleep(1);
    }

    /* Alte Porteinstellungen wiederherstellen */
    tcsetattr(l1port[i].kisslink, TCSADRAIN, &(l1port[i].org_termios));
    /* Port schliessen */
    close(l1port[i].kisslink);

    /* Lockfile loeschen */
    if (*(l1port[i].tnn_lockfile) != '\0')
      unlink(l1port[i].tnn_lockfile);
    /* alle Devices bearbeitet ? */
    if (i == max_device)
      break;
  }
  kiss_active = FALSE;
}

/************************************************************************/
/*                                                                      */
/* Level1 Ende, Ports schliessen und aufraeumen                         */
/*                                                                      */
/************************************************************************/

void l1exit(void)
{
#ifdef KERNELIF
  /* Kernel-AX.25 */
  register unsigned int i;

  for (i = 0;i < L1PNUM; ++i)
    if (   (l1port[i].kisstype == KISS_KAX25)
        || (l1port[i].kisstype == KISS_KAX25KJD))
      ifax_close(&(l1port[i]));
#endif
  /* KISS-Schnittstellen */
  if (kiss_active)
    exit_kisslink();
#ifdef VANESSA
  /* Vanessa */
  vanessa_l1exit();
#endif
#ifdef AX_IPX
  /* AX25IPX */
  axipx_l1exit();
#endif
#ifdef AX25IP
  /* AX25IP */
  ax25ip_l1exit();
#endif
#ifdef SIXPACK
  /* 6PACK */
  Sixpack_l1exit();
#endif
#ifdef L1TELNET
  /* TCPIP-Interface schliessen. */
  L1ExitTCP(KISS_TELNET);
#endif /* L1TELNET */
#ifdef L1HTTPD
  /* TCPIP-Interface schliessen. */
  L1ExitTCP(KISS_HTTPD);
#endif /* L1HTTPD */
#ifdef L1IPCONV
  /* TCPIP-Interface schliessen. */
  L1ExitTCP(KISS_IPCONV);
#endif /* L1IPCONV */
#ifdef L1IRC
  /* TCPIP-Interface schliessen. */
  L1ExitTCP(KISS_IRC);
#endif /* L1IRC */
}


/************************************************************************/
/*                                                                      */
/* Level1 Timer                                                         */
/* Watchdog-Timer fuer die einzelnen L2-Ports                           */
/*                                                                      */
/************************************************************************/

void l1timr(UWORD ticks)
{
  register unsigned int i;

  if ((tic1m += ticks) > 6000)                          /* alle Minute  */
  {
    tic1m -= 6000;
    /* Statistik fuer den Tokenring */
    token_pro_sec = (token_count / 60L);

    if (   (token_max_sec != 0)
        && (token_min_sec != 0))
    {
      token_max_sec = max(token_pro_sec, token_max_sec);
      token_min_sec = min(token_pro_sec, token_min_sec);
    }
    else
      token_min_sec = token_max_sec = token_pro_sec;

    token_count = 0L;
  }

  /* DCD (TX) fuer die Tokenring-Ports checken */
  for (i = 0; i < L2PNUM; ++i)
    if (wd_timer[i] < ticks)
    {
      wd_timer[i] = WATCHDOG_TIMEOUT;
      portpar[i].reset_port = TRUE;
    }
}

/************************************************************************/
/*                                                                      */
/* Level1 RX/TX                                                         */
/*              wird staendig in der main() Hauptschleife aufgerufen.   */
/*                                                                      */
/************************************************************************/

void l1rxtx(void)
{
  register UBYTE i;
  DEVICE *l1pp;

#ifdef VANESSA
  if (found_vanessa != 0)
    vanessa();
#endif
#ifdef AX_IPX
  axipx();
#endif
#ifdef AX25IP
  ax25ip();
#endif
#ifdef KERNELIF
  ifax_housekeeping();
#endif
#ifdef SIXPACK
  Sixpack_Housekeeping();
#endif

  ++rounds_count;              /* Anzahl der Hauptschleifendurchlaeufe  */
#ifdef LOOPBACK
  /* Loopback-Funktionalitaet */
  loopback();
#endif

  /* alle Ports durchgehen */
  for (i = 0; i < L2PNUM; ++i)
  {
    /* uninitialisierter Port */
    if (l1ptab[i] == -1)
      continue;
    /* deaktivierter Port */
        if (!portenabled(i))
      continue;
    l1pp = &l1port[l1ptab[i]];
    /* Tokenring darf nur senden, wenn wir das Token haben */
    if (   (l1pp->kisstype == KISS_TOK) /* Wenn Token noch unterwegs,   */
        && (tokenflag == FALSE))        /* nicht senden!                */
      continue;

#ifdef AX_IPX
    if (l1pp->kisstype == KISS_IPX)
      continue;
#endif
#ifdef AX25IP
    if (l1pp->kisstype == KISS_AXIP)
      continue;
#endif
#ifdef VANESSA
    if (l1pp->kisstype == KISS_VAN)
      continue;
#endif
#ifdef KERNELIF
    if (   (l1pp->kisstype == KISS_KAX25)
        || (l1pp->kisstype == KISS_KAX25KJD))
      continue;
#endif
#ifdef SIXPACK
    if (l1pp->kisstype == KISS_6PACK)
      continue;
#endif
#ifdef L1TELNET
    if (l1pp->kisstype == KISS_TELNET)
      continue;
#endif /* L1TELNET */
#ifdef L1HTTPD
    if (l1pp->kisstype == KISS_HTTPD)
      continue;
#endif /* L1HTTPD */
#ifdef L1IPCONV
    if (l1pp->kisstype == KISS_IPCONV)
      continue;
#endif /* L1IPCONV */
#ifdef L1IRC
    if (l1pp->kisstype == KISS_IRC)
      continue;
#endif /* L1IRC */

/************************************************************************/
/*                                                                      */
/* Nicht-Tokenring-TNCs konfigurieren, falls notwendig                  */
/*                                                                      */
/************************************************************************/

    if (commandflag[i])                         /* TNC konfigurieren    */
      config_tnc(i);

/************************************************************************/
/*                                                                      */
/* Tokenring ist frei - oder SMACK-/KISS-Link                           */
/*                                                                      */
/************************************************************************/
    if (kick[i])        /* Es soll auf diesem Port gesendet werden      */
    {
      kissframe_to_tnc(i, FALSE);  /* Alles fuer diesen Port senden      */
      cd_timer[i] = tic10;        /* Wir senden, Belegt-Timer starten   */
    }
  }
  if (l1port[0].kisstype != KISS_TOK)         /* kein Tokenring da?     */
    return;

/************************************************************************/
/*                                                                      */
/* Tokenring ueberwachen - wenn Token empfangen, ggf. neue Daten senden */
/* oder TNC(s) resetten bzw. konfigurieren. Wenn zu lange kein Token    */
/* angekommen, Verlust des Token annehmen und neues Token generieren.   */
/*                                                                      */
/************************************************************************/

  if (tokenflag == FALSE)               /* nichts gekommen?             */
  {
    if (tic10 > (token_sent + TOKENTIMEOUT)) /* noch einmal warten..    */
    {
      if (++lost_token >= 150)          /* zu viele Token-Recoveries    */
        HALT("Tokenring");

      l1port[0].rx_state = ST_BEGIN;    /* auf FEND warten              */
      send_kisscmd(0xff, CMD_TOKEN, 0); /* Token senden                 */
      token_sent = tic10;               /* nix gekommen...              */
      ++recovery_count;                 /* fuer die Statistik           */

      for (i = 0; i < L2PNUM; ++i)      /* ggf neue Parameter an TNC    */
      {
        if (   portenabled(i)
            && (kissmode(i) == KISS_TOK))
        {
                        commandflag[i] = TRUE;      /* Portparameter neu einstellen */
          kissframe_to_tnc(i, TRUE);    /* zur Sicherheit bei Recovery  */
        }                               /* Restframes loeschen          */
      }

      if (show_recovery == TRUE)
      {
        notify(1, "*** Token-Recovery (%lu)", lost_token);
        if (!blicnt)
          xprintf("*** Token-Recovery (%lu)\n", lost_token);
      }
    }
  }
  else                                          /* Token ist angekommen */
  {
    for (i = 0; i < L2PNUM; ++i)
    {
      if (l1ptab[i] == -1)                      /* Port uninitialisiert */
        continue;
      if (!portenabled(i))                      /* Port ist aus         */
        continue;
      l1pp = &l1port[l1ptab[i]];
      if (l1pp->kisstype != KISS_TOK)           /* nicht Tokenring      */
        continue;
      if (portpar[i].reset_port == TRUE)        /* diesen Port resetten?*/
      {
        send_kisscmd(i, CMD_TNCRES, 0);
        portpar[i].reset_port = FALSE;
        commandflag[i] = TRUE;          /* nach Reset TNC konfigurieren */
        break;                          /* nur 1 RESET/Token, mehr      */
      }                                 /* kommt evtl. nicht durch, da  */
    }                                   /* TNC taub ist nach RESET      */

    send_kisscmd(0xff, CMD_TOKEN, 0);   /* Token senden                 */
    token_sent = tic10;
    tokenflag = FALSE;
    lost_token = 0;
  }
}

/************************************************************************/
/*                                                                      */
/* Loopback-Ports bearbeiten                                            */
/*                                                                      */
/************************************************************************/
#ifdef LOOPBACK
static void loopback(void)
{
  LHEAD  *l2flp;
  MBHEAD *txfhd;
  MBHEAD *rxfhd;
  register int port;
  int     rxport;

  l2flp = txl2fl;

  for (port = 0; port < L2PNUM; l2flp++, port++)/* jeden Port durchlaufen */
  {
    if (kissmode(port) == KISS_LOOP)
    {
/* wenn zwei L2-Ports mit aufeinanderfolgender Nummer (um genau zu sein */
/* ein Port mit gerader Nummer und der mit der naechsthoeheren Nummer)  */
/* auf LOOP gesetzt sind, auf den Nachbarport senden; andernfalls auf   */
/* Sendeport                                                            */
      rxport = port ^ 1;
      if (   !portenabled(rxport)
          || (kissmode(rxport) != KISS_LOOP))
        rxport = port;

      while (kick[port])                        /* was zum senden...    */
      {
        ulink((LEHEAD *)(txfhd = (MBHEAD *) l2flp->head));
        rxfhd = (MBHEAD *) allocb(ALLOC_MBHEAD);

        rxfhd->l2port = rxport;
        while (txfhd->mbpc > txfhd->mbgc)
          putchr(getchr(txfhd), rxfhd);
        relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail); /* in Rx-Liste    */
        relink((LEHEAD *)txfhd, (LEHEAD *)stfl.tail); /* Frame gesendet */
        kick[port] = ((LHEAD *)l2flp->head != l2flp);
      }
    }
  }
}
#endif

/************************************************************************/
/*                                                                      */
/* TNC gemaess Port-Parametern konfigurieren                            */
/*                                                                      */
/************************************************************************/

static void config_tnc(int l2port)
{
  PORTINFO *ppp;
  DEVICE *l1pp;

  l1pp = &l1port[l1ptab[l2port]];
  ppp = &portpar[l2port];

  if (   (l1pp->kisstype != KISS_TOK)   /* DAMA geht nur mit Tokenring  */
      && (l1pp->kisstype != KISS_VAN))  /* oder Vanessa                 */
    ppp->l2mode &= ~MODE_a;

  if (   (l1pp->kisstype == KISS_TOK)   /* Tokenring aber Token noch    */
      && (tokenflag == FALSE))          /* unterwegs? -> beim naechsten */
    return;                             /* Durchgang nochmal probieren  */

  send_kisscmd(l2port, CMD_TXDELAY, ppp->txdelay);

  /* Persistance je nach DAMA einstellen */
  if (dama(l2port))
    send_kisscmd(l2port, CMD_PERSIST, 255);
  else
    send_kisscmd(l2port, CMD_PERSIST, ppp->persistance);

  send_kisscmd(l2port, CMD_SLOTTIME, ppp->slottime);
  send_kisscmd(l2port, CMD_TXTAIL, TAILTIME);
  send_kisscmd(l2port, CMD_FULLDUP, fullduplex(l2port) ? 1 : 0);
  send_kisscmd(l2port, CMD_DAMA, (dama(l2port)) ? 1 : 0);
  commandflag[l2port] = FALSE;
}

/************************************************************************/
/*                                                                      */
/* Level1 Control                                                       */
/*               eine Aktion fuer einen Hardware-Port anfordern         */
/*                                                                      */
/************************************************************************/

void l1ctl(int req, int port)
{
  /* je nach Request die notwendigen Aktionen bestimmen */
  switch (req)
  {
    /* Port resetten */
    case L1CRES: portpar[port].reset_port = TRUE;
                 break;
    /* Parameter zum Port bringen */
        case L1CCMD: commandflag[port] = TRUE;
                 break;
    /* Testsignal senden */
    case L1CTST: testflag[port] = TRUE;
                 kick[port] = TRUE;
                 break;
    /* sonstiges */
    default:     break;
  }
#ifdef VANESSA
  if (found_vanessa != 0)
    vanessa_l1ctl(req);
#endif
#ifdef AX_IPX
  axipx_l1ctl(req, port);
#endif
#ifdef AX25IP
  ax25ip_l1ctl(req, port);
#endif
#ifdef KERNELIF
  ifax_l1ctl(req);
#endif
#ifdef SIXPACK
  Sixpack_l1ctl(req, port);
#endif
#ifdef L1TCPIP
  L1ctlTCP(req, port);
#endif /* L1TCPIP */
}

/************************************************************************/
/*                                                                      */
/* Kommando fuer TNC-Parameter an KISS-Link senden                      */
/*                                                                      */
/************************************************************************/

void send_kisscmd(int l2prt, int cmd, int value)
{
  char tx_buffer[10];   /* Buffer fuer KISS-Frame - 10 Zeichen reicht!  */
  char *tx_bufptr;
  unsigned len;
  char val2;
  DEVICE *l1pp;

  /* ohne KISS geht es hier nicht weiter */
  if (!kiss_active)
    return;

  if (l2prt != 0xff)                   /* wenn nicht Token              */
    l1pp = &l1port[l1ptab[l2prt]];
  else
    l1pp = l1port;                     /* Token = Tokenring = 1. Device */

  switch (l1pp->kisstype)              /* pruefen, ob Kommando zum      */
  {                                    /* Kiss-Modus passt              */
    case KISS_RMNC:
    case KISS_SCC:
      return;
      break;
    case KISS_TOK:
      if (cmd < CMD_TXDELAY) return;
      if ((cmd > CMD_TOKEN) && (cmd != CMD_TNCRES)) return;
      break;
    default:
      if ((cmd < CMD_TXDELAY) || (cmd > CMD_FULLDUP)) return;
      break;
  }

  tx_bufptr = tx_buffer;        /* KISS-Frame in Puffer schreiben       */
  *tx_bufptr++ = FEND;
  len = 1;

  if (l1pp->kisstype == KISS_TOK)       /* Port-Nummer nur bei Token-   */
  {                                     /* ring                         */
    *tx_bufptr++ = l2prt;
    len++;
  }

  *tx_bufptr++ = (char)cmd;
  ++len;

  if ((l2prt != 0xff) && (cmd != CMD_TNCRES))   /* Parameter folgt      */
  {
    val2 = (char)(value & 0xFF);        /* Parameter nur 0 - 255        */
    switch (val2)                       /* ggf. Sonderbehandlung        */
    {
      case FEND:                        /* FEND -> FESC - TFEND         */
        *tx_bufptr++ = FESC;
        *tx_bufptr++ = TFEND;
        len += 2;
        break;
      case FESC:                        /* FESC -> FESC - TFESC         */
        *tx_bufptr++ = FESC;
        *tx_bufptr++ = TFESC;
        len += 2;
        break;
      default:                          /* keine Sonderbehandlung       */
        *tx_bufptr++ = val2;
        len++;
        break;
    }
  }
  *tx_bufptr++ = FEND;                  /* Ende Kommando-Frame          */
  ++len;

  write(l1pp->kisslink, tx_buffer, len);/* Kommando-Frame absenden      */
}

/************************************************************************/
/*                                                                      */
/* send all frames in txbuffer over kisslink                            */
/*                                                                      */
/************************************************************************/

static void kissframe_to_tnc(int l2prt, BOOLEAN recovery)
{
  char tx_buffer[2*MAXKISSLEN];
  char *tx_bufptr;
  unsigned len;
  unsigned short ch1;
  int i;
  char tmp_buffer[MAXKISSLEN];
  char *tmp_bufptr;
  int tmp_buflen;
  DEVICE *l1pp;
  MBHEAD *txfhdl;
  LHEAD  *l2flp;
  ULONG   count = 0L;

  /* Testsignal senden ? */
  if (testflag[l2prt] == TRUE)
  {
    count = (portpar[l2prt].speed * 1000) / 8;  /* 10 sec lang          */
    if (count < 1024)                           /* min. 1kB             */
      count = 1024;
  }

  l1pp = &l1port[l1ptab[l2prt]];
  kick[l2prt] = FALSE;                  /* alles senden                 */
  l2flp = (LHEAD *) &txl2fl[l2prt];

  while (   (l2flp->head != l2flp)      /* solange Frames vorhanden     */
         || (testflag[l2prt] == TRUE))  /* oder TEST gefordert          */
  {
    if (l2flp->head != l2flp)           /* erst Info, danach erst TEST  */
    {
      txfhdl = (MBHEAD *) l2flp->head;
      ulink((LEHEAD *) txfhdl);
      tmp_bufptr = tmp_buffer;            /* Zwischenpuffer auf Anfang  */
      *tmp_bufptr++ = (char)0x00;               /* fuer KISS-Befehl "Daten    */
      tmp_buflen = 1;                     /* folgen"                    */

      while (txfhdl->mbgc < txfhdl->mbpc) /* noch Zeichen im Frame      */
      {
        ch1 = getchr(txfhdl);              /* 1 Zeichen aus Frame holen */
        *tmp_bufptr++ = (char)(ch1 & 0xFF);/* Zeichen in Zwischenpuffer */
        ++tmp_buflen;
      }

      relink((LEHEAD *) txfhdl, (LEHEAD *) stfl.tail);
    }
    else                                /* TEST                         */
    {
      tmp_bufptr = tmp_buffer;          /* Zwischenpuffer auf Anfang    */
      *tmp_bufptr++ = 0x00;             /* fuer KISS-Befehl "Daten      */
      tmp_buflen = 1;                   /* folgen"                      */
      while (count > 0)                 /* noch Zeichen fuer Test       */
      {
        if (tmp_buflen == MAXKISSLEN-3) /* Buffer voll?                 */
          break;
        *tmp_bufptr++ = 0;              /* Zeichen in Zwischenpuffer    */
        ++tmp_buflen;
        if (--count == 0)               /* fertig?                      */
          testflag[l2prt] = FALSE;
      }
    }
    if (!portenabled(l2prt))    /* Port aus -> Frame ist abgeholt, auf  */
      continue;                 /* zum naechsten ..                     */
    if (recovery)               /* bei Token-Recovery                   */
      continue;                 /* zum naechsten ..                     */
    tx_bufptr = tx_buffer;      /* Sendepuffer auf Anfang               */
    *tx_bufptr++ = FEND;        /* KISS-Frame beginnt mit FEND          */

    len = 1;
    switch (l1pp->kisstype)     /* KISS-Befehl "Daten folgen" abhaengig */
    {                           /* vom KISS-Modus                       */
      case KISS_NORMAL:         /* KISS einfach                         */
      case KISS_SCC:
        *tmp_buffer = 0x00;
        break;

      case KISS_SMACK:                          /* SMACK                */
      case KISS_TF:                             /* TheFirmware          */
        *tmp_buffer = 0x80;
        append_crc_16(tmp_buffer, &tmp_buflen); /* CRC ueber gesamtes   */
        break;                                  /* Frame anhaengen      */

      case KISS_RMNC:                           /* RMNC-KISS            */
        *tmp_buffer = 0x20;
        append_crc_rmnc(tmp_buffer, &tmp_buflen);/* CRC ueber gesamtes  */
        break;                                   /* Frame anhaengen     */

      case KISS_TOK:                            /* Tokenring-KISS       */
        *tmp_buffer = 0x00;
        *tx_bufptr++ = l2prt;                   /* L2-Port vorweg       */
        ++len;
        break;
    }
    tmp_bufptr = tmp_buffer;          /* Zwischenpuffer auf Anfang      */

    for (i = 0; i < tmp_buflen; ++i)  /* Zwischenpuffer -> Sendepuffer  */
    {                                 /* kopieren                       */
      switch (*tmp_bufptr)
      {
        case FEND:                    /* FEND -> FESC / TFEND           */
          *tx_bufptr++ = FESC;
          *tx_bufptr++ = TFEND;
          len += 2;
          break;

        case FESC:                      /* FESC -> FESC / TFESC         */
          *tx_bufptr++ = FESC;
          *tx_bufptr++ = TFESC;
          len += 2;
          break;

        default:                        /* keine Sonderbehandlung       */
          *tx_bufptr++ = *tmp_bufptr;
          ++len;
          break;
      }
      ++tmp_bufptr;
    }
    *tx_bufptr++ = FEND;                        /* Frameende = FEND     */
    ++len;

    write(l1pp->kisslink, tx_buffer, len);      /* Frame -> TNC         */
  }
}

/************************************************************************/
/*                                                                      */
/* Empfangsframe in TNN-Puffer schreiben - CRC ist ggf. schon geprueft  */
/*                                                                      */
/************************************************************************/

static void frame_to_l1(WORD port, char *buffer, int len)
{
  static MBHEAD *rxfrhd;
  register int i;

  rxfrhd = (MBHEAD *) allocb(ALLOC_MBHEAD);
  rxfrhd->l2port = port;

  for (i = 0; i < len; ++i)
    putchr(buffer[i], rxfrhd);

  relink((LEHEAD *) rxfrhd, (LEHEAD *)rxfl.tail);
}

/************************************************************************/
/*                                                                      */
/* Frame auf Gueltigkeit pruefen (Laenge, CRC) und gueltige Frames      */
/* weiterreichen                                                        */
/*                                                                      */
/************************************************************************/

static void frame_valid(WORD port, char *buffer, int len, int type)
{
  switch (type)
   {
    case KISS_NORMAL:           /* KISS einfach, Tokenring oder SCC:    */
    case KISS_TOK:              /* nur Framelaenge kann geprueft werden */
    case KISS_SCC:
      if (len <= MAXFRAMELEN)
        frame_to_l1(port, buffer+1, len-1);
      break;
    case KISS_SMACK:                    /* SMACK: CRC und Framelaenge   */
    case KISS_TF:                       /* TF auch mit SMACK            */
      if (!check_crc_16(buffer, &len))  /* pruefen                      */
        if (len <= MAXFRAMELEN)
          frame_to_l1(port, buffer+1, len-1);
      break;
    case KISS_RMNC:                     /* RMNC-KISS: CRC und Frame-    */
      if (!check_crc_rmnc(buffer, &len))/* laenge pruefen               */
        if (len <= MAXFRAMELEN)
          frame_to_l1(port, buffer+1, len-1);
      break;
   }
}


/************************************************************************/
/*                                                                      */
/* put data received over kisslink in rxbuffer                          */
/*                                                                      */
/************************************************************************/

void framedata_to_queue(int dev, char *buffer, int len)
{
  char *bufptr;
  register int i;
  UBYTE ch;
  DEVICE *l1pp;
  static WORD k;

  l1pp = &l1port[dev];

  if (l1pp->kisstype == KISS_TOK)     /* Waehrend Empfang kein Token- */
    token_sent = tic10;               /* recovery                     */

  for (bufptr = buffer, i = 0; i < len; ++i, bufptr++)
  {
    ch = *bufptr;                       /* 1 Zeichen aus Puffer holen   */
    switch (l1pp->rx_state)
    {
      case ST_BEGIN:                    /* Frameanfang suchen           */
        if (ch == FEND)                 /* alles ausser FEND ignorieren */
          l1pp->rx_state = ST_PORT;
        break;
      case ST_PORT:                     /* Portnummer folgt             */
        if (ch != FEND)                 /* zusaetzliche FEND ignorieren */
        {
          switch (l1pp->kisstype)
          {
            case KISS_NORMAL:
            case KISS_SCC:

/************************************************************************/
/*                                                                      */
/* KISS einfach: Die Portnummer ist als Bit 4-7 kodiert. Daher sind     */
/* eigentlich 16 Ports moeglich. Dies wird von dieser Software aber     */
/* (noch?) nicht unterstuetzt. Gibt es ueberhaupt TNC-Software, die     */
/* normalen KISS-Modus mit mehreren Ports unterstuetzt? Wer kann das    */
/* einbauen und testen?                                                 */
/*                                                                      */
/************************************************************************/

              if ((ch & 0x8F) == 0x00)
                l1pp->rx_port = (ch & 0x70) >> 4;
              else
              {
                l1pp->rx_state = ST_BEGIN;
                l1pp->bad_frames++;
              }
              break;
            case KISS_SMACK:
            case KISS_TF:

/************************************************************************/
/*                                                                      */
/* SMACK: Die Portnummer ist als Bit 4-6 kodiert. Daher sind eigentlich */
/* 8 Ports moeglich. Dies wird von dieser Software aber (noch?) nicht   */
/* unterstuetzt. Gibt es ueberhaupt TNC-Software, die den SMACK-Modus   */
/* mit mehreren Ports unterstuetzt? Wer kann das einbauen und testen?   */
/*                                                                      */
/************************************************************************/

              if ((ch & 0x8F) == 0x80)
                l1pp->rx_port = (ch & 0x70) >> 4;
              else
              {
                l1pp->rx_state = ST_BEGIN;
                l1pp->bad_frames++;
              }
              break;
            case KISS_RMNC:
              if ((ch & 0xFF) == 0x20)
                l1pp->rx_port = 0;
              else
              {
                l1pp->rx_state = ST_BEGIN;
                l1pp->bad_frames++;
              }
              break;
            case KISS_TOK:

/************************************************************************/
/*                                                                      */
/* Tokenring: Die Portnummer ist als einzelnes Byte definiert. Daher    */
/* sind eigentlich 255 Ports moeglich. TNN unterstuetzt aber nur die    */
/* Ports 0 - 15. Ausserdem wird Port 255 fuer das Token verwendet.      */
/*                                                                      */
/************************************************************************/

              l1pp->rx_port = ch;
              if (   (ch != 0xff)               /* nicht Token-Port?    */
                      && ((ch >= L2PNUM)        /* gueltige Portnummer? */
                  || (l1ptab[(WORD) ch] != 0))) /* Port definiert?      */
              {
                l1pp->rx_state = ST_BEGIN;
                l1pp->bad_frames++;
              }
              else
                l1pp->rx_state = ST_TOKCMD;
              break;
          }

/************************************************************************/
/*                                                                      */
/* Die Portnummer wird jetzt ueberprueft. Ausser beim Tokenring MUSS es */
/* 0 sein.                                                              */
/*                                                                      */
/************************************************************************/

          if (l1pp->kisstype != KISS_TOK)
          {
            if (l1pp->rx_state != ST_BEGIN)
            {
              if (l1pp->rx_port != 0x00)
              {
                l1pp->rx_state = ST_BEGIN;
                l1pp->bad_frames++;
              }
              else
              {
                l1pp->rx_state = ST_DATA;
                l1pp->rx_bufptr = l1pp->rx_buffer;
                *l1pp->rx_bufptr++ = ch;
                l1pp->rx_buflen = 1;
              }
            }
          }
        }
        break;
      case ST_DATA:                     /* Daten kommen                 */
        switch (ch)
        {
          case FEND:                            /* Frameende?           */
            if (l1pp->kisstype != KISS_TOK)     /* ggf. Portnummer aus  */
              l1pp->rx_port = l2ptab[dev];      /* Tabelle holen        */
            frame_valid(l1pp->rx_port,          /* Frame pruefen und    */
                        l1pp->rx_buffer,        /* an L2 weitergeben    */
                        l1pp->rx_buflen,
                        l1pp->kisstype);
            l1pp->rx_state = ST_PORT;           /* naechstes Frame kann */
            break;                              /* kommen               */
          case FESC:                            /* FESC -> Sonderfall   */
            l1pp->rx_state = ST_ESC;
            break;
          default:                              /* normales Zeichen     */
            *l1pp->rx_bufptr = ch;
            l1pp->rx_bufptr++;
            l1pp->rx_buflen++;
            if (l1pp->rx_buflen > MAXKISSLEN)   /* Frame zu lang?       */
            {
              l1pp->rx_state = ST_BEGIN;
              l1pp->bad_frames++;
            }
            break;
        }
        break;
      case ST_ESC:                      /* zuletzt FESC empfangen       */
        switch (ch)
        {
          case TFEND:                   /* FESC / TFEND -> FEND         */
            *l1pp->rx_bufptr = FEND;
            l1pp->rx_bufptr++;
            l1pp->rx_buflen++;
            if (l1pp->rx_buflen > MAXKISSLEN)   /* Frame zu lang?       */
            {
              l1pp->rx_state = ST_BEGIN;
              l1pp->bad_frames++;
            }
            else
              l1pp->rx_state = ST_DATA;
            break;
          case TFESC:                   /* FESC / TFESC -> FESC         */
            *l1pp->rx_bufptr = FESC;
            l1pp->rx_bufptr++;
            l1pp->rx_buflen++;
            if (l1pp->rx_buflen > MAXKISSLEN)   /* Frame zu lang?       */
            {
              l1pp->rx_state = ST_BEGIN;
              l1pp->bad_frames++;
            }
            else
              l1pp->rx_state = ST_DATA;
            break;
          default:
            l1pp->rx_state = ST_BEGIN;
            l1pp->bad_frames++;
            break;
        }
        break;
      case ST_TOKCMD:                           /* Tokenring nach Port  */
        switch (ch & 0xff)                      /* kommt Kommando       */
        {
          case 0:                               /* Kommando: "Daten     */
            l1pp->rx_state = ST_DATA;           /* folgen"              */
            l1pp->rx_bufptr = l1pp->rx_buffer;
            *l1pp->rx_bufptr++ = ch;
            l1pp->rx_buflen = 1;
            break;
          case CMD_TOKEN:                       /* Kommando: "Token"    */
            if (l1pp->rx_port == 0xff)
              l1pp->rx_state = ST_TOKEN;
            else
              l1pp->rx_state = ST_BEGIN;
            break;
          case MSG_TNCRES:                      /* Kommando: "TNC-      */
            if (l1pp->rx_port != 0xff)          /* Meldung - Reset"     */
            {
              k = 0;
              l1pp->rx_state = ST_TNCRES;
            }
            else
            {
              l1pp->rx_state = ST_BEGIN;
              l1pp->bad_frames++;
            }
            break;
          case MSG_SENTDAMA:                    /* Kommando: "TNC-      */
            if (l1pp->rx_port != 0xff)          /* Meldung: DAMA-Frames */
              l1pp->rx_state = ST_DAMA;         /* gesendet"            */
            else
            {
              l1pp->rx_state = ST_BEGIN;
              l1pp->bad_frames++;
            }
            break;
          default:                              /* unbekanntes Kommando */
            l1pp->rx_state = ST_BEGIN;
            l1pp->bad_frames++;
            break;
        }
        break;
      case ST_TOKEN:                    /* nach Token kommt FEND        */
        if (ch == FEND)
        {
          tokenflag = TRUE;
          l1pp->rx_state = ST_PORT;
          ++token_count;
        }
        else
        {
          l1pp->rx_state = ST_BEGIN;
          l1pp->bad_frames++;
        }
        break;
      case ST_TNCRES:                   /* nach TNC-Meldung kommt FEND  */
        if (ch == FEND)
        {
          commandflag[l1pp->rx_port] = TRUE;
          portstat[l1pp->rx_port].reset_count++;
          l1pp->rx_state = ST_PORT;
          printf("TNC-Reset - Port %d\r\n", l1pp->rx_port);
        }
        else
        {
          if (++k > 6)
          {
            l1pp->rx_state = ST_BEGIN;
            l1pp->bad_frames++;
          }
        }
        break;
      case ST_DAMA:                     /* nach TNC-Meldung kommt FEND  */
        if (ch == FEND)
        {
          cd_timer[l1pp->rx_port] = 0;  /* Kanal frei: Timer stoppen    */
          l1pp->rx_state = ST_PORT;
        }
        else
        {
          l1pp->rx_state = ST_BEGIN;
          l1pp->bad_frames++;
        }
        break;
      default:
        l1pp->rx_state = ST_BEGIN;
        l1pp->bad_frames++;
        break;
    }
  }
}

/************************************************************************/
/*                                                                      */
/* Markieren, dass fuer TNC #port Frames zum Senden vorliegen           */
/*                                                                      */
/************************************************************************/

void kicktx(int port)
{
  kick[port] = TRUE;
}

/************************************************************************/
/*                                                                      */
/*      iscd(): Carrier Detect       Wichtig fuer DAMA !!!              */
/*                                                                      */
/*      Return: abhaengig von der Hardware des Ports - nur KISS wird    */
/*              direkt behandelt. PTTFLAG = auf DAMA-Port noch nicht    */
/*              alles gesendet, 0 sonst.                                */
/*                                                                      */
/************************************************************************/

WORD iscd(int l2port)
{
  switch (kissmode(l2port))
  {
#ifdef VANESSA
    case KISS_VAN   : return (vanessa_dcd(l2port)); break;
#endif
#ifdef AX_IPX
    case KISS_IPX   : return (axipx_dcd(l2port)); break;
#endif
#ifdef AX25IP
    case KISS_AXIP  : return (ax25ip_dcd(l2port)); break;
#endif
#if defined(KERNELIF) && (defined(PCISCC4_KAX25) || defined(HDLC_DCDPTTSTAT) || defined(SCC_DCDPTTSTAT))
    case KISS_KAX25 : return (ifax_dcd(l2port)); break;
#endif
#ifdef SIXPACK
    case KISS_6PACK : return (Sixpack_DCD(l2port)); break;
#endif
#ifdef L1TELNET
    case KISS_TELNET: return (TcpDCD(l2port)); break;
#endif /* L1TELNET */
#ifdef L1HTTPD
    case KISS_HTTPD: return (TcpDCD(l2port)); break;
#endif /* L1HTTPD */
#ifdef L1IPCONV
    case KISS_IPCONV: return (TcpDCD(l2port)); break;
#endif /* L1IPCONV */
#ifdef L1IRC
    case KISS_IRC: return (TcpDCD(l2port)); break;
#endif /* L1IRC */

    default         : break;
  }

  /* DCD fuer Tokenring */
  if (   dama(l2port)               /* Nur DAMA-Ports und belegt..  */
      && cd_timer[l2port])
  {
    if ((tic10 - cd_timer[l2port]) > CD_TIMEOUT) /* Timeout abgelaufen? */
      cd_timer[l2port] = 0;         /* Timer stoppen                  */

    if (cd_timer[l2port])           /* Timer laeuft: Port ist belegt  */
      return(PTTFLAG);              /* Einen belegten Port gefunden!  */
  }/*Nur DAMA*/

  return(FALSE);
}

/************************************************************************/
/*                                                                      */
/* Aufzaehlen der vorhandenen Layer 1 Geraete. Dies sind nicht die      */
/* tatsaechlich installierten sondern die compilierten.                 */
/*                                                                      */
/************************************************************************/

void l1enum(MBHEAD *mbp)
{
#ifdef LOOPBACK
  putstr("   * KISS-Protocols: TOKENRING KISS SMACK RKISS TF LOOP\r", mbp);
#else
  putstr("   * KISS-Protocols: TOKENRING KISS SMACK RKISS TF\r", mbp);
#endif
#ifdef SIXPACK
  putstr("   * 6PACK\r", mbp);
#endif
#ifdef VANESSA
  putstr("   * VANESSA\r", mbp);
#endif
#ifdef AX_IPX
  putstr("   * IPX\r", mbp);
#endif
#ifdef AX25IP
  putstr("   * AX25IP\r", mbp);
#endif
#ifdef KERNELIF
  putstr("   * Kernel-AX.25\r", mbp);
  putstr("   * IP-Tunnel\r", mbp);
#endif
#ifdef L1TELNET
  putstr("   * TELNET-Server\r", mbp);
#endif /* L1TELNET */
#ifdef L1HTTPD
  putstr("   * HTTPD-Server\r", mbp);
#endif /* L1HTTPD */
#ifdef L1IPCONV
  putstr("   * IPCONVERS-Server\r", mbp);
#endif /* L1IPCONV */
#ifdef L1IRC
  putstr("   * IRC-Server\r", mbp);
#endif /* L1IRC */
}

/************************************************************************/
/*                                                                      */
/* L1-Statistik loeschen (alle Ports)                                   */
/*                                                                      */
/************************************************************************/

void l1sclr(const char *str)
{
  register unsigned int i;

  token_pro_sec = 0;
  token_max_sec = 0;
  token_min_sec = 0;
  token_count = 0;
  recovery_count = 0L;

  for (i = 0; i < L1PNUM; ++i)
    l1port[i].bad_frames = 0L;

#ifdef KERNELIF
  ifip_clearstat();
#endif
}

/************************************************************************/
/*                                                                      */
/* L1-Statistik (Tokenring) anzeigen                                    */
/*                                                                      */
/************************************************************************/

void l1stat(const char *name, MBHEAD *mbp)
{
  int     i;
  BOOLEAN flag = FALSE;

  if (tkcom >= 0)
  {
    putprintf(mbp, "\rTokens/sec - min.: %u; last: %u; max.: %u\r", token_min_sec, token_pro_sec, token_max_sec);

    if (token_max_sec != 0)                     /* nicht vor 1. Messung */
      putprintf(mbp, "TOKENRING load: %u%%\r", 100-(((ULONG)token_pro_sec)*100L) / ((ULONG)token_max_sec));

    if (recovery_count != 0)
      putprintf(mbp, "\rToken-Recoveries: %lu\r", recovery_count);
  }

  for (i = 0; i < L1PNUM; ++i)
  {
    if (l1port[i].bad_frames != 0)
    {
      if (!flag)
      {
        flag = TRUE;
        putstr("\rBad frames:\r", mbp);
      }
      putprintf(mbp, "Device %s = %lu\r", l1port[i].device, l1port[i].bad_frames);
    }
  }
}

/************************************************************************/
/*                                                                      */
/* L1 beenden                                                           */
/*                                                                      */
/************************************************************************/

void l1detach(int l2prt)
{
  portpar[l2prt].major = NO_MAJOR;
#ifdef AX_IPX
  if (kissmode(l2prt) == KISS_IPX)
    axipx_l1exit();
#endif
#ifdef AX25IP
  if (kissmode(l2prt) == KISS_AXIP)
    ax25ip_l1exit();
#endif
#ifdef KERNELIF
  if (   (kissmode(l2prt) == KISS_KAX25)
      || (kissmode(l2prt) == KISS_KAX25KJD))
    ifax_close(&(l1port[l1ptab[l2prt]]));
#endif
#ifdef L1TELNET
  if (kissmode(l2prt) == KISS_TELNET)
    L1ExitTCP(kissmode(l2prt));
#endif /* L1TELNET */
#ifdef L1HTTPD
  if (kissmode(l2prt) == KISS_HTTPD)
    L1ExitTCP(kissmode(l2prt));
#endif /* L1HTTPD */
#ifdef L1IPCONV
  if (kissmode(l2prt) == KISS_IPCONV)
    L1ExitTCP(kissmode(l2prt));
#endif /* L1IPCONV */
#ifdef L1IRC
  if (kissmode(l2prt) == KISS_IRC)
    L1ExitTCP(kissmode(l2prt));
#endif /* L1IRC */
}

/************************************************************************/
/*                                                                      */
/* L1 starten                                                           */
/*                                                                      */
/************************************************************************/

int l1attach(int l2prt, char *buf)
{
  if (l1port[l1ptab[l2prt]].port_active == FALSE)
/* Port undefiniert in tnn.ini - der Port kann auf LOOP gesetzt werden  */
   {
    if (strnicmp("LOOP", buf, min(4, strlen(buf))) == 0)
     {
      kissmode(l2prt) = KISS_LOOP;
      portpar[l2prt].major = 1;
      return(1);
     }
    return(0);
   }
  if (strnicmp("TOKENRING", buf, min(9, strlen(buf))) == 0)
   {
    if (kissmode(l2prt) == KISS_TOK)
     {
      portpar[l2prt].major = 1;
      return(1);
     }
   }

#ifdef SIXPACK
  if (strnicmp("6PACK", buf, min(5, strlen(buf))) == 0)
   {
    if (kissmode(l2prt) == KISS_6PACK)
     {
      portpar[l2prt].major = 1;
      return(1);
     }
   }
#endif

  if (strnicmp("KISS", buf, min(4, strlen(buf))) == 0)
   {
    if ((kissmode(l2prt) != KISS_NIX) && (kissmode(l2prt) != KISS_VAN))
     {
      portpar[l2prt].major = 1;
      return(1);
     }
   }
  if (strnicmp("SMACK", buf, min(5, strlen(buf))) == 0)
   {
    if (kissmode(l2prt) == KISS_SMACK)
     {
      portpar[l2prt].major = 1;
      return(1);
     }
   }
  if (strnicmp("TF", buf, 2) == 0)
   {
    if (kissmode(l2prt) == KISS_TF)
     {
      portpar[l2prt].major = 1;
      return(1);
     }
   }
  if (strnicmp("RKISS", buf, min(5, strlen(buf))) == 0)
   {
    if (kissmode(l2prt) == KISS_RMNC)
     {
      portpar[l2prt].major = 1;
      return(1);
     }
   }
#ifdef VANESSA
  if (strnicmp("VANESSA", buf, min(7, strlen(buf))) == 0)
   {
    if (kissmode(l2prt) == KISS_VAN)
     {
      portpar[l2prt].major = 1;
      return(1);
     }
   }
#endif
#ifdef AX_IPX
  if (strnicmp("IPX", buf, min(3, strlen(buf))) == 0)
   {
    if (kissmode(l2prt) == KISS_IPX)
     {
      if (!axipx_l1init(l2prt))
        return(0);
      portpar[l2prt].major = 1;
      return(1);
     }
   }
#endif
#ifdef AX25IP
  if (strnicmp("AX25IP", buf, min(6, strlen(buf))) == 0)
   {
    if (kissmode(l2prt) == KISS_AXIP)
     {
      if (!ax25ip_l1init(l2prt))
        return(0);
      portpar[l2prt].major = 1;
      return(1);
     }
   }
#endif
#ifdef KERNELIF
  if (strnicmp("KERNEL", buf, min(6, strlen(buf))) == 0)
   {
    if (   (kissmode(l2prt) == KISS_KAX25)
        || (kissmode(l2prt) == KISS_KAX25KJD))
    {
      if (!ifax_setup(&(l1port[l1ptab[l2prt]])))
        return(0);
      portpar[l2prt].major = 1;
      return(1);
     }
   }
#endif
#ifdef L1TELNET
   if (strnicmp("TELNET", buf, min(6, strlen(buf))) == FALSE)
   {
     if (kissmode(l2prt) == KISS_TELNET)
     {
       if (!L1InitTCP(kissmode(l2prt), l2prt, htons((unsigned short)atoi(l1port[l1ptab[l2prt]].device))))
         return(FALSE);

       portpar[l2prt].major = TRUE;
       return(TRUE);
     }
   }
#endif /* L1TELNET */
#ifdef L1HTTPD
   if (strnicmp("HTTPD", buf, min(6, strlen(buf))) == FALSE)
   {
     if (kissmode(l2prt) == KISS_HTTPD)
     {
       if (!L1InitTCP(kissmode(l2prt), l2prt, htons((unsigned short)atoi(l1port[l1ptab[l2prt]].device))))
         return(FALSE);

       portpar[l2prt].major = TRUE;
       return(TRUE);
     }
   }
#endif /* L1HTTPD */
#ifdef L1IPCONV
   if (strnicmp("IPCONV", buf, min(6, strlen(buf))) == FALSE)
   {
     if (kissmode(l2prt) == KISS_IPCONV)
     {
       if (!L1InitTCP(kissmode(l2prt), l2prt, htons((unsigned short)atoi(l1port[l1ptab[l2prt]].device))))
         return(FALSE);

       portpar[l2prt].major = TRUE;
       return(TRUE);
     }
   }
#endif /* L1IPCONV */
#ifdef L1IRC
   if (strnicmp("IRC", buf, min(3, strlen(buf))) == FALSE)
   {
     if (kissmode(l2prt) == KISS_IRC)
     {
       if (!L1InitTCP(kissmode(l2prt), l2prt, htons((unsigned short)atoi(l1port[l1ptab[l2prt]].device))))
         return(FALSE);

       portpar[l2prt].major = TRUE;
       return(TRUE);
     }
   }
#endif /* L1IRC */
  if (strnicmp("SSC", buf, min(3, strlen(buf))) == 0)
   {
    if (kissmode(l2prt) == KISS_SCC)
     {
      portpar[l2prt].major = 1;
      return(1);
     }
   }

  if (stricmp("ON", buf) == 0)
   {
    if (kissmode(l2prt) != KISS_NIX)
     {
#ifdef AX_IPX
      if (kissmode(l2prt) == KISS_IPX)
       {
        if (!axipx_l1init(l2prt))
          return(0);
       }
#endif
#ifdef AX25IP
      if (kissmode(l2prt) == KISS_AXIP)
       {
        if (!ax25ip_l1init(l2prt))
          return(0);
       }
#endif
#ifdef KERNELIF
      if (   (kissmode(l2prt) == KISS_KAX25)
          || (kissmode(l2prt) == KISS_KAX25KJD))
       {
        if (!ifax_setup(&(l1port[l1ptab[l2prt]])))
          return(0);
       }
#endif
#ifdef L1TELNET
      if (kissmode(l2prt) == KISS_TELNET)
      {
        if (!L1InitTCP(kissmode(l2prt), l2prt, htons((unsigned short)atoi(l1port[l1ptab[l2prt]].device))))
          return(FALSE);
       }
#endif /* L1TELNET */
#ifdef L1HTTPD
      if (kissmode(l2prt) == KISS_HTTPD)
      {
        if (!L1InitTCP(kissmode(l2prt), l2prt, htons((unsigned short)atoi(l1port[l1ptab[l2prt]].device))))
          return(FALSE);
       }
#endif /* L1HTTPD */
#ifdef L1IPCONV
      if (kissmode(l2prt) == KISS_IPCONV)
      {
        if (!L1InitTCP(kissmode(l2prt), l2prt, htons((unsigned short)atoi(l1port[l1ptab[l2prt]].device))))
          return(FALSE);
       }
#endif /* L1IPCONV */
#ifdef L1IRC
      if (kissmode(l2prt) == KISS_IRC)
      {
        if (!L1InitTCP(kissmode(l2prt), l2prt, htons((unsigned short)atoi(l1port[l1ptab[l2prt]].device))))
          return(FALSE);
       }
#endif /* L1IRC */

      portpar[l2prt].major = 1;
      return(1);
     }
   }
  return(0);
}

/************************************************************************/
/*                                                                      */
/* Infostring fuer den Port-Befehl zusammenbauen.                       */
/*                                                                      */
/************************************************************************/

void l1hwstr(int l2prt, MBHEAD *mbp)
{
  l1hwcfg(l2prt, mbp);
  if ((kissmode(l2prt) != KISS_VAN) && (kissmode(l2prt) < KISS_IPX))
  {
    putchr(' ', mbp);
    putstr(l1port[l1ptab[l2prt]].device, mbp);
  }
  else
  {
#ifdef VANESSA
    if (kissmode(l2prt) == KISS_VAN)
      van_hwstr(l2prt, mbp);
#endif
#ifdef AX_IPX
    if (kissmode(l2prt) == KISS_IPX)
      axipx_hwstr(l2prt, mbp);
#endif
#ifdef AX25IP
    if (kissmode(l2prt) == KISS_AXIP)
      ax25ip_hwstr(l2prt, mbp);
#endif
#ifdef KERNELIF
    if (   (kissmode(l2prt) == KISS_KAX25)
        || (kissmode(l2prt) == KISS_KAX25KJD))
      ifax_hwstr(l2prt, mbp);
#endif
#ifdef L1TELNET
    if (kissmode(l2prt) == KISS_TELNET)
      HwstrTCP(KISS_TELNET, l2prt, mbp);
#endif /* L1TELNET */
#ifdef L1HTTPD
    if (kissmode(l2prt) == KISS_HTTPD)
      HwstrTCP(KISS_HTTPD, l2prt, mbp);
#endif /* L1HTTPD */
#ifdef L1IPCONV
    if (kissmode(l2prt) == KISS_IPCONV)
      HwstrTCP(KISS_IPCONV, l2prt, mbp);
#endif /* L1IPCONV */
#ifdef L1IRC
    if (kissmode(l2prt) == KISS_IRC)
      HwstrTCP(KISS_IRC, l2prt, mbp);
#endif /* L1IRC */
  }
}

/************************************************************************/
/*                                                                      */
/* Infostring fuer SAVEPARM zusammenbauen.                              */
/*                                                                      */
/************************************************************************/

void l1hwcfg(int l2prt, MBHEAD *mbp)
{
  if (!portenabled(l2prt))
    putstr("OFF", mbp);
  else
  {
    switch (kissmode(l2prt))
    {
      case KISS_SMACK:  putstr("SMACK", mbp);
                        break;
      case KISS_TF:     putstr("TF", mbp);
                        break;
      case KISS_TOK:    putstr("TOKENRING", mbp);
                        break;
#ifdef LOOPBACK
      case KISS_LOOP:   putstr("LOOP", mbp);
                        break;
#endif
      case KISS_RMNC:   putstr("RKISS", mbp);
                        break;
#ifdef VANESSA
      case KISS_VAN:    putstr("VANESSA", mbp);
                        break;
#endif
#ifdef AX_IPX
      case KISS_IPX:    putstr("IPX", mbp);
                        break;
#endif
#ifdef AX25IP
      case KISS_AXIP:   putstr("AX25IP", mbp);
                        break;
#endif
#ifdef KERNELIF
      case KISS_KAX25:
      case KISS_KAX25KJD: putstr("KERNEL", mbp);
                          break;
#endif
#ifdef SIXPACK
      case KISS_6PACK:  putstr("6PACK", mbp);
                        break;
#endif
#ifdef L1TELNET
      case KISS_TELNET: putstr("TELNET", mbp);
                        break;
#endif /* L1TELNET */
#ifdef L1HTTPD
      case KISS_HTTPD:  putstr("HTTPD ", mbp);
                        break;
#endif /* L1HTTPD */
#ifdef L1IPCONV
      case KISS_IPCONV: putstr("IPCONV", mbp);
                        break;
#endif /* L1IPCONV */
#ifdef L1IRC
      case KISS_IRC:    putstr("IRC   ", mbp);
                        break;
#endif /* L1IRC */

      default:          putstr("KISS", mbp);
                        break;
    }
  }
}

/*
 * Blocktransferroutinen fuer den Level 1
 * Message-Buffer koennen Blockweise in einen linearen Buffer umgewandelt
 * werden und wieder zurueck. Dies darf aber so ohne weiteres nur im
 * Level 1 erfolgen, da der Buffer zurueckgespult wird.
 */

/* Message-Buffer -> linearen Buffer */
int cpymbflat(char *buf, MBHEAD *fbp)
{
  MB    *bp;
  LHEAD *llp = &fbp->mbl;           /* Zeiger auf den Listenkopf        */
  int    i   = fbp->mbpc;           /* Anzahl der Bytes im Frame        */

  for (bp = (MB *)llp->head; bp != (MB *)llp;
       bp = bp->nextmb, i -= sizeof_MBDATA, buf += sizeof_MBDATA)
    memcpy(buf, bp->data, sizeof_MBDATA);
  return(fbp->mbpc);
}

/* linearer Buffer -> Message-Buffer */
MBHEAD *cpyflatmb(char *buf, int size)
{
  MBHEAD *mbhd;
  MB     *bp;
  LHEAD  *llp;

  mbhd = (MBHEAD *)allocb(ALLOC_MBHEAD); /* einen Buffer fuer den Kopf  */
  mbhd->mbpc = size;                /* soviel wird mal drinstehen       */
  llp = &mbhd->mbl;                 /* Zeiger auf den Listenkopf        */
  for ( ; size > 0; size -= sizeof_MBDATA, buf += sizeof_MBDATA) {
    memcpy((bp = (MB *)allocb(ALLOC_MB))->data, buf, sizeof_MBDATA);
    relink((LEHEAD *)bp, (LEHEAD *)llp->tail);
  }
  rwndmb(mbhd);                     /* mbbp richtig setzen              */
  return(mbhd);
}

/************************************************************************/
/*                                                                      */
/* TNC mit TheFirmware in KISS-Modus schalten                           */
/*                                                                      */
/************************************************************************/

#ifndef ATTACH
static void tf_set_kiss(DEVICE *l1pp)
#else
void tf_set_kiss(DEVICE *l1pp)
#endif /* ATTACH */
{
  char  buffer1[] = {0x18, 0x12, 0x18, ESC, '@', 'K', CR};
  char  buffer2[] = {FESC, FEND, FEND, 0x80, 0, 0, FEND};

  write(l1pp->kisslink, buffer1, 7);
  sleep(5);
  write(l1pp->kisslink, buffer2, 7);
}

/* tfkiss: TNC-emulation for Linux
   Copyright (C) 1995-96 by Mark Wahl
   CRC calculation (crc.c)
   created: Mark Wahl DL4YBG 95/10/08
   updated: Mark Wahl DL4YBG 96/01/31
*/

/* CRC-table for SMACK */
static const int
Crc_16_table[] = {
  0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
  0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
  0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
  0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
  0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
  0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
  0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
  0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
  0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
  0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
  0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
  0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
  0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
  0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
  0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
  0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
  0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
  0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
  0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
  0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
  0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
  0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
  0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
  0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
  0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
  0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
  0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
  0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
  0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
  0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
  0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
  0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

/* CRC-table for RMNC-KISS */
static const int
Crc_rmnc_table[] = {
  0x0f87, 0x1e0e, 0x2c95, 0x3d1c, 0x49a3, 0x582a, 0x6ab1, 0x7b38,
  0x83cf, 0x9246, 0xa0dd, 0xb154, 0xc5eb, 0xd462, 0xe6f9, 0xf770,
  0x1f06, 0x0e8f, 0x3c14, 0x2d9d, 0x5922, 0x48ab, 0x7a30, 0x6bb9,
  0x934e, 0x82c7, 0xb05c, 0xa1d5, 0xd56a, 0xc4e3, 0xf678, 0xe7f1,
  0x2e85, 0x3f0c, 0x0d97, 0x1c1e, 0x68a1, 0x7928, 0x4bb3, 0x5a3a,
  0xa2cd, 0xb344, 0x81df, 0x9056, 0xe4e9, 0xf560, 0xc7fb, 0xd672,
  0x3e04, 0x2f8d, 0x1d16, 0x0c9f, 0x7820, 0x69a9, 0x5b32, 0x4abb,
  0xb24c, 0xa3c5, 0x915e, 0x80d7, 0xf468, 0xe5e1, 0xd77a, 0xc6f3,
  0x4d83, 0x5c0a, 0x6e91, 0x7f18, 0x0ba7, 0x1a2e, 0x28b5, 0x393c,
  0xc1cb, 0xd042, 0xe2d9, 0xf350, 0x87ef, 0x9666, 0xa4fd, 0xb574,
  0x5d02, 0x4c8b, 0x7e10, 0x6f99, 0x1b26, 0x0aaf, 0x3834, 0x29bd,
  0xd14a, 0xc0c3, 0xf258, 0xe3d1, 0x976e, 0x86e7, 0xb47c, 0xa5f5,
  0x6c81, 0x7d08, 0x4f93, 0x5e1a, 0x2aa5, 0x3b2c, 0x09b7, 0x183e,
  0xe0c9, 0xf140, 0xc3db, 0xd252, 0xa6ed, 0xb764, 0x85ff, 0x9476,
  0x7c00, 0x6d89, 0x5f12, 0x4e9b, 0x3a24, 0x2bad, 0x1936, 0x08bf,
  0xf048, 0xe1c1, 0xd35a, 0xc2d3, 0xb66c, 0xa7e5, 0x957e, 0x84f7,
  0x8b8f, 0x9a06, 0xa89d, 0xb914, 0xcdab, 0xdc22, 0xeeb9, 0xff30,
  0x07c7, 0x164e, 0x24d5, 0x355c, 0x41e3, 0x506a, 0x62f1, 0x7378,
  0x9b0e, 0x8a87, 0xb81c, 0xa995, 0xdd2a, 0xcca3, 0xfe38, 0xefb1,
  0x1746, 0x06cf, 0x3454, 0x25dd, 0x5162, 0x40eb, 0x7270, 0x63f9,
  0xaa8d, 0xbb04, 0x899f, 0x9816, 0xeca9, 0xfd20, 0xcfbb, 0xde32,
  0x26c5, 0x374c, 0x05d7, 0x145e, 0x60e1, 0x7168, 0x43f3, 0x527a,
  0xba0c, 0xab85, 0x991e, 0x8897, 0xfc28, 0xeda1, 0xdf3a, 0xceb3,
  0x3644, 0x27cd, 0x1556, 0x04df, 0x7060, 0x61e9, 0x5372, 0x42fb,
  0xc98b, 0xd802, 0xea99, 0xfb10, 0x8faf, 0x9e26, 0xacbd, 0xbd34,
  0x45c3, 0x544a, 0x66d1, 0x7758, 0x03e7, 0x126e, 0x20f5, 0x317c,
  0xd90a, 0xc883, 0xfa18, 0xeb91, 0x9f2e, 0x8ea7, 0xbc3c, 0xadb5,
  0x5542, 0x44cb, 0x7650, 0x67d9, 0x1366, 0x02ef, 0x3074, 0x21fd,
  0xe889, 0xf900, 0xcb9b, 0xda12, 0xaead, 0xbf24, 0x8dbf, 0x9c36,
  0x64c1, 0x7548, 0x47d3, 0x565a, 0x22e5, 0x336c, 0x01f7, 0x107e,
  0xf808, 0xe981, 0xdb1a, 0xca93, 0xbe2c, 0xafa5, 0x9d3e, 0x8cb7,
  0x7440, 0x65c9, 0x5752, 0x46db, 0x3264, 0x23ed, 0x1176, 0x00ff
};

static void append_crc_16(char *buffer, int *len)
{
  register int i;
  int crc_16 = 0;
  char *bufptr = buffer;

  for (i = 0; i < *len; ++i)
    crc_16 = (crc_16 >> 8) ^ Crc_16_table[(crc_16 ^ *bufptr++) & 0xff];

  *bufptr++ = crc_16;
  *bufptr++ = (crc_16 >> 8);
  *len += 2;
}

static int check_crc_16(char *buffer, int *len)
{
  register int i;
  int crc_16 = 0;
  char *bufptr = buffer;

  if (*len < 3)
    return(1);

  for (i = 0; i < *len; ++i)
    crc_16 = (crc_16 >> 8) ^ Crc_16_table[(crc_16 ^ *bufptr++) & 0xff];

  if (crc_16)
    return(1);

  *len -= 2;
  return(0);
}

static void append_crc_rmnc(char *buffer, int *len)
{
  register int i;
  int crc_rmnc = 0xFFFF;
  char *bufptr = buffer;

  for (i = 0; i < *len; ++i)
    crc_rmnc =   (crc_rmnc << 8) ^ Crc_rmnc_table[((crc_rmnc >> 8) ^ *bufptr++) & 0xff];

  *bufptr++ = (crc_rmnc >> 8);
  *bufptr++ = crc_rmnc;
  *len += 2;
}

static int check_crc_rmnc(char *buffer, int *len)
{
  register int i;
  int crc_rmnc = 0xFFFF;
  char *bufptr = buffer;

  if (*len < 3)
    return(1);

  for (i = 0; i < *len; ++i)
    crc_rmnc =   (crc_rmnc << 8) ^ Crc_rmnc_table[((crc_rmnc >> 8) ^ *bufptr++) & 0xff];

  if ((crc_rmnc & 0xFFFF) != 0x7070)
    return(1);

  *len -= 2;
  return(0);
}

/* End of os/linux/l1linux.c */
