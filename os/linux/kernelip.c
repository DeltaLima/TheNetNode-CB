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
/* File os/linux/kernelip.c (maintained by: DG9OBU)                     */
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
#include "kernelip.h"

struct kip kip_info;

/* Funktionen zum Anzeigen von "errno"-Fehlermeldungen */
static void show_error(MBHEAD *);

/* Funktionen fuer den IP-Teil */
static void ifip_close(struct kip *);
static BOOLEAN ifip_setup(struct kip *);
static void ifip_init(struct kip *);
static BOOLEAN ifip_usable(struct kip *);

/**************************************************************************/
/* Funktion zur Anzeige der Fehlernummer und des Fehlertextes             */
/**************************************************************************/
void show_error(MBHEAD* mbp)
{
    putprintf(mbp, "(errno: %u, %s)\r", errno, strerror(errno));
}

/**************************************************************************/
/* Interface zur Steuerung der Kernelfunktionen                           */
/*------------------------------------------------------------------------*/
void ccpkif(void)
{
  MBHEAD *mbp;

  ipaddr  t_ip_addr = 0L;               /* temporaere IP                */
  int     t_ip_bits = 32;               /* temporaere Subnetzbits       */

  /* Funktionen im Sysop-Modus */
  if (issyso())
  {
      if (strnicmp((char *)clipoi, "CLEAR", 5) == 0)
      {
          ifip_clearstat();

          mbp = putals("Interface statistics cleared.\r");
          prompt(mbp);
          seteom(mbp);
          return;
      }

      if (strnicmp((char *)clipoi, "DOWN", 4) == 0)
      {
          mbp = putals("Interface ");

          if (kip_info.if_active == TRUE)
          {
              ifip_close(&kip_info);

              if (xaccess("KIF_DOWN.TNB", 0) == 0)
                 runbatch("KIF_DOWN.TNB");

              rt_drop(kip_info.kernel_ip, 32, FALSE);
              putstr("put to DOWN-state.\r", mbp);
          }
          else
              putstr("is DOWN.\r", mbp);

          prompt(mbp);
          seteom(mbp);
          return;
      }

      if (strnicmp((char *)clipoi, "UP", 2) == 0)
      {
          mbp = putals("Interface ");

          if (kip_info.if_available == FALSE)
          {
              putstr("is unavailable !!!\r", mbp);
              prompt(mbp);
              seteom(mbp);
              return;
          }

          if (kip_info.kernel_ip == 0L)
          {
              putstr("is not properly configured yet, set Kernel-IP with SETKIP !!!\r", mbp);
              prompt(mbp);
              seteom(mbp);
              return;
          }

          if (my_ip_addr == 0L)
          {
              putstr("is not properly configured yet, set Node-IP with IPA !!!\r", mbp);
              prompt(mbp);
              seteom(mbp);
              return;
          }

          if (kip_info.if_active == TRUE)
          {
              putstr("is already UP !\r", mbp);
              prompt(mbp);
              seteom(mbp);
              return;
          }

          if (ifip_setup(&kip_info) == FALSE)
              putstr("setup failed !!!\r", mbp);
          else
          {
              rt_add(kip_info.kernel_ip, kip_info.maskbits, 0L, KERNEL_PORT, 0, 0, 0, FALSE);

              if (xaccess("KIF_UP.TNB", 0) == 0)
                 runbatch("KIF_UP.TNB");

              putstr("setup successfully\r", mbp);
          }

          prompt(mbp);
          seteom(mbp);
          return;
      }

      if (strnicmp((char *)clipoi, "INIT", 4) == 0)
      {
          if (kip_info.if_active == TRUE)
          {
              mbp = putals("Interface is UP, put it to DOWN-state before initializing !!!\r");
              prompt(mbp);
              seteom(mbp);
              return;
          }

          mbp = putals("Kernel-Interface initialization ");

          ifip_init(&kip_info);

          if (ifip_usable(&kip_info) == FALSE)
              putstr("FAILED, feature not available !!!\r", mbp);
          else
              putstr("successful\r", mbp);

          if (my_ip_addr == 0)
              putstr("WARNING: Node IP-Adress not yet set, set with IPA-Command !!!\r", mbp);

          prompt(mbp);
          seteom(mbp);
          return;
      }

      if (strnicmp((char *)clipoi, "STATUS", 5) == 0)
      {
          mbp = putals("Kernel-Interface status:\r");

          putstr("Kernel feature available : ", mbp);

          if (kip_info.if_available == FALSE)
              putstr("NO, not initialized or unavailable, try INIT.\r", mbp);
          else
          {
              putstr("yes, ",mbp);

              switch (kip_info.if_style)
              {
                  case 256: putstr("Kernel 2.4.x/2.6.x-style\r", mbp);
                            break;

                  default : putstr("Kernel 2.2.x-style\r", mbp);
              }
          }

          putstr("Kernel-Interface active  : ", mbp);

          if (kip_info.if_active == FALSE)
              putstr("no\r", mbp);
          else
              putstr("yes\r",mbp);

          putstr("IP-Adress of Kernel is   : ", mbp);

          if (kip_info.kernel_ip == 0)
              putstr("not defined\r", mbp);
          else
          {
              show_ip_addr(kip_info.kernel_ip, mbp);
              putchr('/', mbp);
              putnum(kip_info.maskbits, mbp);
              putchr('\r', mbp);
          }

          putstr("Bytes sent via Kernel    : ", mbp);
          putnum(kip_info.bytes_tx, mbp);

          putstr("\rBytes rcvd via Kernel    : ", mbp);
          putnum(kip_info.bytes_rx, mbp);

          putchr('\r', mbp);
          prompt(mbp);
          seteom(mbp);
          return;
      }

      if (strnicmp((char *)clipoi, "SETKIP", 6) == 0)
      {
          if (kip_info.if_active == TRUE)
          {
              mbp = putals("Interface is UP, put it DOWN before changing settings !!! \r");
              prompt(mbp);
              seteom(mbp);
              return;
          }

          nextspace(&clicnt, &clipoi);
          if (!skipsp(&clicnt, &clipoi))
          {
              mbp = putals("Usage KERN SETKIP <IP[/Bitmask]>\r");
              prompt(mbp);
              seteom(mbp);
              return;
          }

          mbp = putals("Kernel-IP ");

          if (get_ip_addr(&t_ip_addr, &clicnt, &clipoi) == TRUE)  /* IP lesen */
          {
              /* Unmoegliche IP-Adressen abfangen (nur niederwertigstes Byte) */
              /* eigentlich muesste man die anderen auch noch pruefen...      */
              if (((t_ip_addr & 0xFF) == 0L) || ((t_ip_addr & 0xFF) == 0xFF))
              {
                  putstr("is invalid !\r", mbp);
                  prompt(mbp);
                  seteom(mbp);
                  return;
              }

              if (skipsp(&clicnt, &clipoi)) /* sind noch weitere Zeichen da? */
              {
                if (*clipoi++ == '/')          /* Subnetz-Trenner vorhanden? */
                {
                  clicnt--;
                  /* Subnetz-Bits lesen und Idiotencheck machen              */
                  t_ip_bits = nxtnum(&clicnt, &clipoi);
                  if (t_ip_bits > 32 || t_ip_bits < 1)
                    t_ip_bits = 32;
                }
              }

              kip_info.kernel_ip = t_ip_addr;

              if (t_ip_addr == 0L)
                  kip_info.maskbits = 0;
              else
                  kip_info.maskbits = t_ip_bits;    /* Subnetz-Bits uebernehmen */

              putstr("set to ", mbp);
              show_ip_addr(kip_info.kernel_ip, mbp);
              putchr('/', mbp);
              putnum(kip_info.maskbits, mbp);
              putchr('\r', mbp);
          }
          else
              putstr("could not be read ! Don't use hostnames, numbers only !\r", mbp);

          prompt(mbp);
          seteom(mbp);
          return;

      }

      mbp = putals("Usage: KERNELIF [INIT, CLEAR, SETKIP, UP, DOWN, STATUS]\r");
      prompt(mbp);
      seteom(mbp);
      return;
  }
  else
    invmsg();
}

/* Den Infoblock initialisieren */
static void ifip_init(struct kip *if_info)
{
    if_info->if_available = FALSE;
    if_info->if_style = 0;
    if_info->if_active = FALSE;
    if_info->kernel_ip = 0L;
    if_info->maskbits = 32;
    if_info->if_fd = -1;
    if_info->bytes_rx = 0L;
    if_info->bytes_tx = 0L;
}

/* Pruefen, ob notwendige Funktionen verfuegbar sind */
static BOOLEAN ifip_usable(struct kip *if_info)
{
    int fd = 0;
    int i = 0;

    char ifdev[14];

    if ((fd = open(IFIP, O_RDWR)) > 0)
    {
        if_info->if_available = TRUE;
        if_info->if_style = 256;
        close(fd);
        return TRUE;
    }

    for (i = 0; i < 255; i++)
    {
        sprintf(ifdev, "/dev/tun%d", i);
        if ((fd = open(ifdev, O_RDWR)) > 0)
        {
            if_info->if_available = TRUE;
            if_info->if_style = i;
            close(fd);
            return TRUE;
        }
    }

    if_info->if_available = FALSE;
    if_info->if_style = 0;
    return FALSE;
}

/* Den Link zum Kernel schliessen und im Infoblock als inaktiv vermerken */
static void ifip_close(struct kip *if_info)
{
    if ((if_info->if_active == TRUE) && (if_info->if_fd >= 0))
    {
        close(if_info->if_fd);
        if_info->if_fd = -1;
        if_info->if_active = FALSE;
    }
}

/* Das Interface kreieren */
BOOLEAN ifip_setup(struct kip *if_info)
{
    int fd = 0;
    int sd = 0;
    int err = 0;
    int i = 0;

    char dev[IFNAMSIZ];
    char ifdev[14];

    unsigned long netmask;

    MBHEAD *mbp;

    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    memset(dev, 0, sizeof(dev));

    /* Neue Variante (>2.4.6 und 2.6.x) */
    if (if_info->if_style == 256)
    {
        sprintf(dev, "tnn");

        if ((fd = open(IFIP, O_RDWR)) <= 0)
        {
            close(fd);
            mbp = putals("Can't open device !!!\r");
            show_error(mbp);
            seteom(mbp);
            return FALSE;
        }

        ifr.ifr_flags = (IFF_TUN | IFF_NO_PI);
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

        if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 )
        {
            close(fd);
            mbp = putals("Can't set miscelleanous device flags, trying fallback ...\r");
            seteom(mbp);

            if ((fd = open(IFIP, O_RDWR)) <= 0)
            {
                close(fd);
                mbp = putals("Sorry, can't open device anymore !!!\r");
                show_error(mbp);
                seteom(mbp);
                return FALSE;
            }

            memset(&ifr, 0, sizeof(ifr));
            ifr.ifr_flags = (IFF_TUN | IFF_NO_PI);

            if (((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) || (fd <= 0))
            {
                close(fd);
                mbp = putals("Can't set device flags, giving up !!!\r");
                show_error(mbp);
                seteom(mbp);
                return FALSE;
            }

            mbp = putals("");
            putprintf(mbp, "Info: interface's name changed from '%s' to '%s' !!!\r", dev, ifr.ifr_name);
            seteom(mbp);

            strcpy(dev, ifr.ifr_name);
        }
        strncpy(dev, ifr.ifr_name, IFNAMSIZ);
    }
    else
    {   /* Alte Variante (2.2.x) */
        sprintf(ifdev, "/dev/tun%d", if_info->if_style);

        fd = open(ifdev, O_RDWR);

        if (fd <= 0)
        {
            close(fd);
            mbp = putals("Can't open device !!!\r");
            show_error(mbp);
            seteom(mbp);
            return FALSE;
        }

        sprintf(dev, "tun%d", if_info->if_style);
    }

    memset(&ifr, 0, sizeof(ifr));

    sd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sd <= 0)
    {
        close(sd);
        mbp = putals("Can't set up interface !!!\r");
        show_error(mbp);
        seteom(mbp);
        return FALSE;
    }

    strcpy(ifr.ifr_name, dev);

    ifr.ifr_addr.sa_family = AF_INET;

    ifr.ifr_addr.sa_data[0] = 0;
    ifr.ifr_addr.sa_data[1] = 0;
    ifr.ifr_addr.sa_data[6] = 0;

    for (i = 0; i < 4; i++)
        ifr.ifr_addr.sa_data[5-i] = (if_info->kernel_ip >> 8 * i) & 0xff;

    if (ioctl(sd, SIOCSIFADDR, &ifr) < 0)
    {
        close(sd);
        mbp = putals("Can't set kernel-side ip-adress !!!\r");
        show_error(mbp);
        seteom(mbp);
        return FALSE;
    }

    for (i = 0; i < 4; i++)
        ifr.ifr_addr.sa_data[5-i] = (my_ip_addr >> 8 * i) & 0xff;

    if (ioctl(sd, SIOCSIFDSTADDR, &ifr) < 0)
    {
        close(sd);
        mbp = putals("Can't set tnn-side ip-adress !!!\r");
        show_error(mbp);
        seteom(mbp);
        return FALSE;
    }

    netmask = (~0L) << (32 - if_info->maskbits);

    for (i = 0; i < 4; i++)
        ifr.ifr_addr.sa_data[5-i] = (netmask >> 8 * i) & 0xFF;

    if (ioctl(sd, SIOCSIFNETMASK, &ifr) < 0)
    {
        close(sd);
        mbp = putals("Can't set kernel-side netmask !!!\r");
        show_error(mbp);
        seteom(mbp);
        return FALSE;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if ((err = ioctl(sd, SIOCGIFFLAGS, &ifr)) < 0)
    {
        close(sd);
        mbp = putals("Can't read current interface flags !!!\r");
        show_error(mbp);
        seteom(mbp);
        return FALSE;
    }

/*    ifr.ifr_flags &= IFF_NOARP; */
    ifr.ifr_flags |= IFF_UP;
/*    ifr.ifr_flags |= IFF_RUNNING; */

    if ((err = ioctl(sd, SIOCSIFFLAGS, &ifr)) < 0)
    {
        close(sd);
        mbp = putals("Can't set new interface flags !!!\r");
        show_error(mbp);
        seteom(mbp);
        return FALSE;
    }

    close(sd);

    strcpy(dev, ifr.ifr_name);

    if_info->if_active = TRUE;
    if_info->if_fd = fd;

    return TRUE;
}

/* Die Statistik des Infoblocks loeschen */
void ifip_clearstat(void)
{
    kip_info.bytes_rx = 0L;
    kip_info.bytes_tx = 0L;
}

/* Statistik ausgeben */
void ifip_dispstat(MBHEAD *mbp)
{
    putstr("\rKernel-Interface statistics:", mbp);

    putstr("\rBytes received : ", mbp);
    putnum(kip_info.bytes_rx, mbp);

    putstr("\rBytes sent     : ", mbp);
    putnum(kip_info.bytes_tx, mbp);

    putstr("\r", mbp);
}

/* Interface aktiv ? Wenn ja wird der verwendete Descriptor gemeldet, wenn */
/* nicht wird -1 zurueckgemeldet */
int ifip_active(void)
{
    if (kip_info.if_active == TRUE)
         return (kip_info.if_fd);
    else return (-1);
}

/* Ein IP-Frame vom Kernel holen und in den internen Router einschleusen. */
/* Sollte nur aufgerufen werden, wenn der Descriptor lesbar ist. */
void ifip_frame_to_router(void)
{
    char buf[4096];
    int i = -1;
    int j = 0;

    MBHEAD *mbhd;

    if ((i = read(kip_info.if_fd, &buf[0], 4096)) <= 0)
    {
        /* irgendwas lief schief, wir machen dicht */
        close(kip_info.if_fd);
        kip_info.if_fd = -1;
        kip_info.if_active = FALSE;

        return;
    }

    kip_info.bytes_rx += (ULONG)i;

    (mbhd = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2port = KERNEL_PORT;

    for (j = 0; j < i; j++)
      putchr(buf[j], mbhd);

    rwndmb(mbhd);

    relink((LEHEAD *)mbhd, (LEHEAD *)iprxfl.tail);
}

void ifip_frame_to_kernel(MBHEAD *mhbp)
{
    char buf[2048];
    int i = -1;
    int j = 0;

    /* falls das Interface nicht aktiv ist und noch Routing-Leichen da sind */
    /* die noch was auf dem Port abladen, einfach wegschmeissen */
    if (kip_info.if_active == FALSE)
    {
      dealmb(mhbp);
      return;
    }

    rwndmb(mhbp);

    while (mhbp->mbgc < mhbp->mbpc) /* solange Daten vorhanden sind */
        buf[j++] = getchr(mhbp);

    dealmb(mhbp);

    kip_info.bytes_tx += (ULONG)j;

    if ((i = write(kip_info.if_fd, &buf[0], j)) <= 0)
    {
        /* irgendwas lief schief, wir machen dicht */
        close(kip_info.if_fd);
        kip_info.if_fd = -1;
        kip_info.if_active = FALSE;
    }
}

/* Die Konfiguration des Kernelinterfaces fuer parms.tnb dumpen */
void dump_kernel(MBHEAD *mbp)
{
  putstr(";\r; Kernel Interface Configuration\r;\r", mbp);

  /* nur bei AKTIVIERTEM und LAUFENDEN Interface und wenn dem Kernel */
  /* eine IP-Adresse zugewiesen worden ist                           */
  if ((kip_info.if_active == FALSE) ||  (kip_info.kernel_ip == 0))
  {
    putstr("; (no information dumped because interface was not running\r", mbp);
    putstr(";  or not properly configured)\r", mbp);
    return;
  }

  putstr("KERN INIT\r", mbp);

  /* die IP-Adresse des Kernels dumpen */
  putstr("KERN SETKIP ", mbp);
  show_ip_addr(kip_info.kernel_ip, mbp);
  putstr("\r", mbp);

  putstr("KERN UP\r", mbp);
}

#endif

/* End of os/linux/kernelip.c */
