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
/* File os/linux/kernelax.c (maintained by: DG9OBU)                     */
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

#ifdef KERNELIF
#include "kernelax.h"

/* allgemeine Funktionen zum Anzeigen von "errno"-Fehlermeldungen */
static void show_error(MBHEAD *);

/* Funktionen fuer den AX.25-Teil */
BOOLEAN ifax_setup(DEVICE *);
void ifax_close(DEVICE *);
void ifax_rx(int);
void ifax_tx(void);
void ifax_housekeeping(void);

BOOLEAN update_parms = FALSE;

/**************************************************************************/
/* Funktion zur Anzeige der Fehlernummer und des Fehlertextes             */
/**************************************************************************/
void show_error(MBHEAD* mbp)
{
    putprintf(mbp, "(errno: %u, errtxt: %s)\r", errno, strerror(errno));
}

/**************************************************************************/
/* Verbindet sich mit einem Kernel-AX.25-Interface                        */
/**************************************************************************/
BOOLEAN ifax_setup(DEVICE *l1pp)
{
  struct ifreq ifr;
  struct sockaddr sa;

  MBHEAD *mbp;

  /* Bereits laufende und nicht passende Ports fassen wir nicht an */
  if (   (l1pp->port_active != TRUE)
      || ((l1pp->kisstype != KISS_KAX25) && (l1pp->kisstype != KISS_KAX25KJD))
      || (l1pp->kisslink > -1))
    return (FALSE);

  /* Socket anlegen */
  if ((l1pp->kisslink = socket(AF_PACKET, SOCK_PACKET, htons(ETH_P_ALL))) < 0)
  {
    mbp = putals("Can't create socket !\r");
    show_error(mbp);
    seteom(mbp);

    /* da in der Regel auf "-1" geprueft wird ... */
    l1pp->kisslink = -1;
    return (FALSE);
  }

  /* Interfacestruktur bereinigen und Interface auswaehlen */
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, l1pp->device);

  /* vom Kernel die aktuellen Interfaceeinstellungen holen */
  if (ioctl(l1pp->kisslink, SIOCGIFFLAGS, &ifr) < 0)
  {
    /* dieses Interface gab es aber nicht */
    mbp = putals("Can't get current interface flags from kernel !\r");
    show_error(mbp);
    seteom(mbp);

    /* Interface schliessen und als nicht nutzbar markieren */
    close(l1pp->kisslink);
    l1pp->kisslink = -1;
    return (FALSE);
  }

  /* Flags des Interfaces sichern */
  l1pp->oldifparms = ifr.ifr_flags;

  /* Sicherheitshalber Interface aktivieren */
  ifr.ifr_flags |= IFF_UP;

/*  brauchen wir das wirklich ??? */
/*  ifr.ifr_flags |= IFF_PROMISC; */

  /* Aenderungen zum Interface bringen */
  if (ioctl(l1pp->kisslink, SIOCSIFFLAGS, &ifr) < 0)
  {
    mbp = putals("Can't set new interface flags !\r");
    show_error(mbp);
    seteom(mbp);

    /* Schliessen und alte Einstellungen wiederherstellen */
    ifax_close(l1pp);
    return (FALSE);
  }

  /* Socketstruktur loeschen und danach fuellen */
  memset(&sa, 0, sizeof(sa));
  strcpy(sa.sa_data, l1pp->device);
  sa.sa_family = AF_INET;

  /* Interface binden */
  if (bind(l1pp->kisslink, &sa, sizeof(struct sockaddr)) < 0)
  {
    mbp = putals("Can't bind interface !\r");
    show_error(mbp);
    seteom(mbp);

    /* Schliessen und alte Einstellungen wiederherstellen */
    ifax_close(l1pp);
    return (FALSE);
  }

  return (TRUE);
}

/**************************************************************************/
/* Interface schliessen                                                   */
/**************************************************************************/
void ifax_close(DEVICE *l1pp)
{
  struct ifreq ifr;

  /* nur bei Interfaces mit gueltigem Descriptor */
  if (l1pp->kisslink != -1)
  {
    /* Alten Interface-Zustand wiederherstellen */
    ifr.ifr_flags = l1pp->oldifparms;
    ioctl(l1pp->kisslink, SIOCSIFFLAGS, &ifr);

    /* Schliessen und als unbenutzt markieren */
    close(l1pp->kisslink);
    l1pp->kisslink = -1;
  }
}

/**************************************************************************/
/* Portinfo-String fuer den PORT-Befehl                                   */
/**************************************************************************/
void ifax_hwstr(int port, MBHEAD *mbp)
{
  putprintf(mbp, " Interface %s", &(l1port[l1ptab[port]].device[0]));
}

/**************************************************************************/
/* Daten vom Interface abholen, vorher muss mit select geprueft werden    */
/* ob auch Daten da sind (Stichwort select() )                            */
/**************************************************************************/
void ifax_rx(int fd)
{
  int i = 0;
  int j = 0;
  int port = 0;
  char buf[2048];

  MBHEAD *mbhd;

  for (port = 0; port < L2PNUM; port++)
  {
    /* nur aktive Kernelports und davon den richtigen mit dem aktiven fd */
    if (   (l1port[l1ptab[port]].port_active != TRUE)
        || (l1port[l1ptab[port]].kisstype < KISS_KAX25)
        || (l1port[l1ptab[port]].kisstype > KISS_KAX25KJD)
        || (l1port[l1ptab[port]].kisslink != fd))
      continue;

    memset(buf, 0, sizeof(buf));
    errno = 0;

    i = recvfrom(l1port[l1ptab[port]].kisslink, &buf[0], 2048, 0, NULL, 0);

    /* Fehler abfangen */
    if (i <= 0)
    {
      if (i < 0)
      {
        /* es konnte nicht richtig gesendet werden, Port schliessen */
        l1detach(port);
      }
      return;
    }

    /* bei normalem Kernelstack das KISS-Kommandobyte auswerten */
    if (l1port[l1ptab[port]].kisstype == KISS_KAX25)
    {
      j = 1;
      /* Sortieren was da gekommen ist, Parameteraenderungen uebernehmen */
      switch (buf[0] & 0x0F)
      {
        case 0 : break; /* normale Frames */
        case PARAM_TXDELAY:
          portpar[port].txdelay = (int)buf[1];
          autopar(port);
          return;
        case PARAM_SLOTTIME:
          portpar[port].slottime = (int)buf[1];
          autopar(port);
          return;
        case PARAM_PERSIST:
          portpar[port].persistance = (int)buf[1];
          autopar(port);
          return;
        case PARAM_FULLDUP:
          if (buf[1])
            portpar[port].l1mode = portpar[port].l1mode & MODE_d;
          else
            portpar[port].l1mode = portpar[port].l1mode & ~MODE_d;
          autopar(port);
          return;
        default : return; /* alles unbekannte wird nicht weitergereicht */
      }
    }

    /* Puffer besorgen und Port eintragen */
    (mbhd = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2port = port;

    /* Puffer kopieren */
    for (; j < i; j++)
      putchr(buf[j], mbhd);

    /* in Empfangsliste einhaengen */
    relink((LEHEAD *)mbhd, (LEHEAD *)rxfl.tail);
  }
}

/**************************************************************************/
/* Sendedaten an das Interface schicken, keine Parameter                  */
/**************************************************************************/
void ifax_tx(void)
{
  struct sockaddr to;
  char buf[2048];
  int count = 1;
  int port;

  MBHEAD *txfhdl;
  LHEAD  *l2flp;

  l2flp = txl2fl;

  for (port = 0; port < L2PNUM; l2flp++, port++)
  {
    /* Feststellen, ob der Port hier behandelt werden kann */
    if (   (l1port[l1ptab[port]].port_active != TRUE)
        || (l1port[l1ptab[port]].kisstype < KISS_KAX25)
        || (l1port[l1ptab[port]].kisstype > KISS_KAX25KJD)
        || (l1port[l1ptab[port]].kisslink < 0)
       )
      continue;

    if (portenabled(port))
    {
      if (kick[port])                   /* was zum senden...            */
      {
        if (l2flp->head == l2flp)
        {
          /* nichts mehr zu senden da */
          kick[port] = FALSE;
          continue;
        }

        memset(&buf[0], 0, sizeof(buf));

        /* KJD-Stack */
        if (l1port[l1ptab[port]].kisstype == KISS_KAX25KJD)
          count = 0;

        ulink((LEHEAD *)(txfhdl = (MBHEAD *) l2flp->head)); /* Zeiger holen */

        while (txfhdl->mbgc < txfhdl->mbpc) /* solange Daten vorhanden sind */
          buf[count++] = getchr(txfhdl);

        relink((LEHEAD *)txfhdl,          /* als gesendet betrachten und in */
              (LEHEAD *)stfl.tail);       /* die gesendet Liste umhaengen   */

        kick[port] = ((LHEAD *)l2flp->head != l2flp);

        memset(&to, 0, sizeof(to));
        errno = 0;

        /* Interface setzen auf dem dieses Paket gesendet werden soll */
        strcpy(to.sa_data, l1port[l1ptab[port]].device);

        /* Puffer senden und Fehler feststellen */
        if (sendto(l1port[l1ptab[port]].kisslink, &buf[0], count, 0, &to, sizeof(to)) < 0)
          l1detach(port);
      }
    }
    else /* Port kann nicht senden, Frames einfach kopieren */
      while (l2flp->head != l2flp)
        relink(ulink((LEHEAD *) l2flp->head), (LEHEAD *)stfl.tail);
  }
}

/**************************************************************************/
/* Setzt die Parameter (Slottime, TX-Delay usw.)                          */
/**************************************************************************/
void ifax_param(int port)
{
  struct sockaddr to;
  unsigned char buf[2];
  static int count = 2; /* alle KISS-Befehle sind zwei Bytes lang */

  /* Feststellen, ob der Port hier behandelt werden kann */
  if (   (l1port[l1ptab[port]].port_active != TRUE)
      || (l1port[l1ptab[port]].kisstype != KISS_KAX25)
      || (l1port[l1ptab[port]].kisslink < 0)
     )
    return;

#ifdef PCISCC4_KAX25
  /* PCISCC4-Devices unter normalem Kernel-AX.25 mit dem Treiber von F6FBB     */
  /* oder unter einem 2.2.x-KJD-Kernel. Hier funktioniert leider die normale   */
  /* KISS-Mimik nicht, also muessen die Einstellungen per ioctl() zum Treiber. */
  if (strncmp("dscc", l1port[l1ptab[port]].device, 4) == 0)
  {
    struct ifreq ifr;
    struct devcfg_t cfg;

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, l1port[l1ptab[port]].device);
/*
    if (ioctl(l1port[l1ptab[port]].kisslink, SIOCGIFFLAGS, &ifr) < 0)
    {
      notify(1, "*** PCISCC4: Can't select interface %s", l1port[l1ptab[port]].device);
      return;
    }
*/
    ifr.ifr_data = (caddr_t)&cfg;

    /* aktuelle Einstellungen holen */
    if (ioctl(l1port[l1ptab[port]].kisslink, SIOCPCISCCGDCFG, &ifr) < 0)
    {
      notify(1, "*** PCISCC4: Can't read current setting from interface %s", l1port[l1ptab[port]].device);
      return;
    }

    /* Einstellungen modifizieren */
    /* Semi/Vollduplex einstellen */
    cfg.duplex = (fullduplex(port) ? CFG_DUPLEX_FULLPTT : CFG_DUPLEX_HALF);

    /* TX-Delay (Achtung: Anzahl Bits, nicht Millisekunden) */
    cfg.txdelval = (int)((portpar[port].speed * 100) * (portpar[port].txdelay * 10) / 1000);

    /* TX-Tail */
#ifdef SETTAILTIME
    cfg.txtailval = (int)((portpar[port].speed * 100) * (portpar[port].tailtime * 10) / 1000);
#endif
    /* Slottime */
    cfg.slottime = (int)((portpar[port].speed * 100) * (portpar[port].slottime * 10) / 1000);;

    /* Persistance */
    cfg.persist = (dama(port)) ? 255 : portpar[port].persistance;

    /* Neue Einstellungen zum Interface bringen */
    ifr.ifr_data = (caddr_t)&cfg;

    /* und ab damit zum Treiber */
    if (ioctl(l1port[l1ptab[port]].kisslink, SIOCPCISCCSDCFG, &ifr) < 0)
      notify(1, "*** PCISCC4: Can't set new settings on interface %s", l1port[l1ptab[port]].device);

    return;
  }
#endif

  /* Hier ist Schluss fuer KJD-Kernel, der kennt die KISS-Methoden nicht. */
  if (l1port[l1ptab[port]].kisstype == KISS_KAX25KJD)
    return;

  /* Alle anderen Interfaces landen hier, normale KISS-Methode zum */
  /* Einstellen der Portparameter. Sicherheitshalber alle Einstellungen */
  /* separat vornehmen. */
  /* Interface waehlen */
  memset(&to, 0, sizeof(to));
  strcpy(to.sa_data, l1port[l1ptab[port]].device);

  /* TX-Delay senden */
  buf[0] = PARAM_TXDELAY;
  buf[1] = portpar[port].txdelay;

  if (sendto(l1port[l1ptab[port]].kisslink, &buf[0], count, 0, &to, sizeof(to)) < 0)
  {
    notify(1, "*** KERNELAX: Can't set TX-Delay on interface %s", l1port[l1ptab[port]].device);
    return;
  }

  /* Slottime senden */
  buf[0] = PARAM_SLOTTIME;
  buf[1] = portpar[port].slottime;

  if (sendto(l1port[l1ptab[port]].kisslink, &buf[0], count, 0, &to, sizeof(to)) < 0)
  {
    notify(1, "*** KERNELAX: Can't set Slottime on interface %s", l1port[l1ptab[port]].device);
    return;
  }

  /* Persistance senden */
  buf[0] = PARAM_PERSIST;
  buf[1] = (dama(port) ? 255 : portpar[port].persistance);

  if (sendto(l1port[l1ptab[port]].kisslink, &buf[0], count, 0, &to, sizeof(to)) < 0)
  {
    notify(1, "*** KERNELAX: Can't set Persistance on interface %s", l1port[l1ptab[port]].device);
    return;
  }

  /* Vollduplex setzen */
  buf[0] = PARAM_FULLDUP;
  buf[1] = (fullduplex(port) ? 1 : 0);

  if (sendto(l1port[l1ptab[port]].kisslink, &buf[0], count, 0, &to, sizeof(to)) < 0)
  {
    notify(1, "*** KERNELAX: Can't set Half/Fullduplex on interface %s", l1port[l1ptab[port]].device);
    return;
  }

  return;
}

/**************************************************************************/
/* Level 1 Kontroll- und Kommandointerface                                */
/**************************************************************************/
void ifax_l1ctl(int req)
{
  switch (req)
  {
    case L1CCMD : update_parms = TRUE; break; /* Portparameter setzen */
    default     : break;                /* den Rest koennen wir nicht */
  }
}

/**************************************************************************/
/* Generelle Funktion, sendet ausstehende Frames und bringt ggf.          */
/* Parameteraenderungen zum Interface usw.                                */
/**************************************************************************/
void ifax_housekeeping(void)
{
  register int port;

  ifax_tx();

  if (update_parms)
  {
    for (port = 0; port < L2PNUM; ++port)
    {
      if (l1port[l1ptab[port]].kisstype != KISS_KAX25)
        continue;

      if (commandflag[port])
      {
        ifax_param(port);
        commandflag[port] = FALSE;
      }
    }
    update_parms = FALSE;
  }
}

/**************************************************************************/
/* DCD/PTT-Status fuer Kernelinterfaces abfragen                          */
/**************************************************************************/
int ifax_dcd(int port)
{
  int state = 0;

  if (   (l1port[l1ptab[port]].port_active != TRUE)
      || (   (l1port[l1ptab[port]].kisstype != KISS_KAX25)
          && (l1port[l1ptab[port]].kisstype != KISS_KAX25KJD)
         )
      || (l1port[l1ptab[port]].kisslink < 0)
     )
    return 0; /* Device gehoert hier nicht hin */

#ifdef PCISCC4_KAX25
  /* PCISCC4-Devices mit dem Treiber von F6FBB fuer 2.4.x-Kernel */
  /* oder 2.2.x-KJD-Kernel */
  /* (dscc0, dscc1, ...) */
  if (strncmp("dscc", l1port[l1ptab[port]].device, 4) == 0)
  {
    struct ifreq ifr;
    unsigned long devstat;

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, l1port[l1ptab[port]].device);

    if (ioctl(l1port[l1ptab[port]].kisslink, SIOCPCISCCGDSTAT, &ifr) < 0)
    {
      notify(1, "*** PCISCC4: Can't read DCD/PTT state from interface %s", l1port[l1ptab[port]].device);
      return 0;
    }

    devstat = (unsigned long)ifr.ifr_data;

    /* DCD */
    if (devstat & STATUS_CD)
      state |= (WORD)(DCDFLAG);

    /* PTT */
    if (devstat & STATUS_RTS)
      state |= (WORD)(PTTFLAG);

    /* noch mehr PTT-Zustaende (mit Daten in der PCISCC4) */
    switch (devstat & 15)
    {
      case TX_DELAY :
      case TX_XMIT  : state |= (WORD)(TXBFLAG); break;

      default       : break;
    }

    return state;
  }
#endif

#ifdef HDLC_DCDPTTSTAT
  /* hdlcdrv wird benutzt von bcsf, bcsh, bce, bcp, sm (2.4.x) */
  if (   (strncmp("bc", l1port[l1ptab[port]].device, 2) == 0)
      || (strncmp("sm", l1port[l1ptab[port]].device, 2) == 0)
     )
  {
    struct ifreq ifr;
    struct hdlcdrv_ioctl ifrpar;

    ifr.ifr_data = (caddr_t)&ifrpar;
    ifrpar.cmd = HDLCDRVCTL_GETSTAT;

    strcpy(ifr.ifr_name, l1port[l1ptab[port]].device);

    if (ioctl(l1port[l1ptab[port]].kisslink, SIOCDEVPRIVATE, &ifr) < 0)
    {
        notify(1, "*** HDLCDRV: Can't read DCD/PTT state from interface %s", l1port[l1ptab[port]].device);
        return 0;
    }

    /* DCD */
    if (ifrpar.data.cs.dcd)
      state |= (WORD)(DCDFLAG);

    /* PTT */
    if (ifrpar.data.cs.ptt)
      state |= (WORD)(PTTFLAG);

    return state;
  }
#endif

#ifdef SCC_DCDPTTSTAT
  /* SCC-Devices (scc0, scc1, ... ) */
  if (strncmp("scc", l1port[l1ptab[port]].device, 3) == 0)
  {
    struct ifreq ifr;
    struct scc_kiss_cmd kiss_cmd;

    ifr.ifr_data = (caddr_t)&kiss_cmd;
    kiss_cmd.command = PARAM_RTS;

    strcpy(ifr.ifr_name, l1port[l1ptab[port]].device);

    /* PTT-Zustand holen */
    if (ioctl(l1port[l1ptab[port]].kisslink, SIOCSCCGKISS, &ifr) < 0)
    {
        notify(1, "*** SCC: Can't read PTT state from interface %s", l1port[l1ptab[port]].device);
        return 0;
    }

    /* PTT */
    if (kiss_cmd.param)
      state |= (WORD)(PTTFLAG);

#ifdef OBU_SCC_DCD
    /* Funktioniert NUR mit gepatchten Kerneln !!! (siehe history/sccpatch.txt) */
    ifr.ifr_data = (caddr_t)&kiss_cmd;
    kiss_cmd.command = PARAM_DCD;

    strcpy(ifr.ifr_name, l1port[l1ptab[port]].device);

    /* DCD-Zustand holen */
    if (ioctl(l1port[l1ptab[port]].kisslink, SIOCSCCGKISS, &ifr) < 0)
    {
        notify(1, "*** SCC: Can't read DCD state from interface %s", l1port[l1ptab[port]].device);
        return state; /* PTT kann gesetzt sein, also Status melden ! */
    }

    /* DCD */
    if (kiss_cmd.param)
      state |= (WORD)(DCDFLAG);
#endif

    return state;
  }
#endif

  return 0;
}

#endif

/* End of os/linux/kernelax.c */
