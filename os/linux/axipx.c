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
/* File os/linux/axipx.c (maintained by: DG1KWA)                        */
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

/************************************************************************/
/*                                                                      */
/* AXIPX.C  AX.25 Frames in IPX-Frames (Novell)                         */
/*          Ideen zu diesem Treiber aus WAMPES (by DK5SG) und           */
/*          und axipx.c by DG2FEF / Matthias Welwarsky                  */
/*          Implementation in TNN-LINUX by DG1KWA / Andreas             */
/*                                                                      */
/* ACHTUNG !                                                            */
/* Der Kernel MUSS mit IPX-Option compiliert werden !                   */
/* Syntax : ipx_interface add eth0 -p EtherII                           */
/*                                                                      */
/************************************************************************/

#include "tnn.h"

#ifdef AX_IPX

#define MAX_FRAME       2048

static struct sockaddr_ipx bcaddr = {
  AF_IPX, 0x73CE, 0x00L,
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  25
};

static struct sockaddr_ipx myaddr;
static LHEAD  *l2flp;
static DEVICE *l1pp;
static int axipx_port;
static BOOLEAN axipx_active = FALSE;

void axipx_send (void); /* Sende-Routine             */
void axipx_recv(void);  /* Empfangsroutine           */

/***********************************************************************/
void axipx(void)
{
  /* nur wenn AXIPX aktiv */
  if (!axipx_active)
    return;

  /* ausstehende Frames senden */
  axipx_send();
}

/* Socket fuer Kommunikation oeffenen und initialisieren */
BOOLEAN axipx_l1init(int l2port)
{
  socklen_t addrlen = 0;

  if (axipx_active == TRUE)
    return(TRUE);

  axipx_port = l2port;
  l1pp = &l1port[l1ptab[l2port]];
  l1pp->kisslink = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
  l2flp = (LHEAD *) &txl2fl[l2port];

  if (l1pp->kisslink < 0)
  {
#ifdef SPEECH
    printf(speech_message(331), strerror(errno));
#else
    printf("cannot create socket for IPX : %s\n", strerror(errno));
#endif
    return(FALSE);
  }

  memset(&myaddr, 0, sizeof(myaddr));
  myaddr.sipx_family  = AF_IPX;
  myaddr.sipx_network = 0x00L;
  myaddr.sipx_port    = htons(0xCE73);

  if (bind(l1pp->kisslink, (struct sockaddr *) &myaddr, sizeof(myaddr)))
  {
#ifdef SPEECH
    printf(speech_message(332), strerror(errno));
#else
    printf("cannot bind IPX address: %s\n", strerror(errno));
#endif
    close(l1pp->kisslink);
    l1pp->kisslink = -1;
    return(FALSE);
  }

  addrlen = sizeof(myaddr);
  if (getsockname(l1pp->kisslink, (struct sockaddr *) &myaddr, &addrlen) < 0)
  {
    perror("getsockname(ipx)");
  }

  if (fcntl(l1pp->kisslink, F_SETFL, FNDELAY) < 0)
  {
#ifdef SPEECH
  printf(speech_message(333));
#else
      printf("Error: can't set non-blocking I/O on IPX socket");
#endif
      close(l1pp->kisslink);
      l1pp->kisslink = -1;
      return(FALSE);
  }

  axipx_active = TRUE;
  return(TRUE);
}

void axipx_l1exit(void)
{
  if (axipx_active == FALSE)
    return;

  close(l1pp->kisslink);
  axipx_active = FALSE;
  l1pp->kisslink = -1;
}


void axipx_l1ctl(int req, int port)
{
        /* nur eigene Ports bearbeiten */
        if (kissmode(port) != KISS_IPX)
                return;

        switch(req)
        {
                /* Testpattern auf dem Port senden (wird unterdrueckt) */
        case L1CTST : testflag[port] = FALSE;
                      kick[port] = FALSE;
                      break;

        default : break;
        }
}

void axipx_hwstr(int port, MBHEAD *mbp)
{
}

BOOLEAN axipx_dcd(int port)
{
  return (FALSE);
}

/* IPX-Sende-Routine */
void axipx_send(void)
{
  char    buf[MAX_FRAME];
  WORD    len = 0;
  MBHEAD *txfhdl;

  if (kick[axipx_port])
  { /* haben wir was zu senden ? */
          /* Sicherheitsabfrage, da kisck[] auch manipuliert werden kann */
          if (l2flp->head == l2flp)
          {
                  kick[axipx_port] = FALSE;
                  return;
          }

    ulink((LEHEAD *)(txfhdl = (MBHEAD *) l2flp->head));/*Zeiger holen*/

    while (    (txfhdl->mbgc < txfhdl->mbpc) /* solange Daten vorhanden sind */
                   && (len < MAX_FRAME))
      buf[len++] = getchr(txfhdl);

    relink((LEHEAD *)txfhdl,          /* als gesendet betrachten und in */
      (LEHEAD *)stfl.tail);           /* die gesendet Liste umhaengen   */

    kick[axipx_port] = ((LHEAD *)l2flp->head != l2flp);

    if (sendto(l1pp->kisslink, (char *)buf, len, 0,
              (struct sockaddr *)&bcaddr, sizeof(bcaddr)) < 0)
    {
      perror("sendto(l1pp->kisslink) in axipx_send()");
    }
  }
}

/* IPX Empfangs-Routine */
void axipx_recv(void)
{
  struct sockaddr_ipx  addr;
  socklen_t            addrlen = sizeof(addr);
  int                  l = 0;
  int                  i = 0;
  UBYTE                buf[MAX_FRAME];
  UBYTE               *bufptr;
  MBHEAD              *rxfhd;

  if (!axipx_active)
    return;

  l = recvfrom(l1pp->kisslink, (char *) (bufptr = buf), sizeof(buf), 0,
              (struct sockaddr *) &addr, &addrlen);

  if (l > 0)
  {
     /* unseren eigenen Frames wollen wir nicht empfangen */
     if (!memcmp(addr.sipx_node, myaddr.sipx_node, IPX_NODE_LEN))
       return;

     (rxfhd = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2port = axipx_port;

     for (i = 0; i < l; i++)
        putchr(buf[i], rxfhd); /* Buffer umkopieren */

     relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);
  }
}

#endif

/* End of os/linux/axipx.c */
