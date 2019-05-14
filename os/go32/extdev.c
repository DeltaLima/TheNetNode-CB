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
/* File os/go32/extdev.c (maintained by: ???)                           */
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

#ifdef EXTDEV
#include "api.h"
#endif

extern unsigned dev_irq;
static int ext_major = 0;

/* den Portstatus unter beachtung der Kanalkopplung holen               */
static int ext_state(int port)
{
  int state;

  state = l1_state(port);        /* den Portstatus lesen                */
  if (multibaud(port&1))         /* der untere Port muss das 'm' haben  */
    state = state | l1_state(port^1);
  return(state);
}

/* Die externen Treiber mit Sendedaten versorgen und Daten holen        */
static void ext_timer(UWORD ticks)
{
  L1FRAME   *fp;
  PORTINFO  *p;
  int        port;
  unsigned   state;
  MBHEAD    *txfhdl;
  MBHEAD    *rxfhd;
  LHEAD     *l2flp = txl2fl;

  if (dev_irq == 0) return; /* keine externe Treiber */

  while ((fp = l1_rx_frame()) != NULL) { /* solange noch was empfangen  */
    (rxfhd = cpyflatmb(fp->data, fp->len))->l2port = fp->port;
    relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);
  }

  for (port = 0; port < L2PNUM; port++, l2flp++) {/* Ports durchsehen   */
    p = &portpar[port];                  /* ob etwas zu senden ist      */
    if (p->major != ext_major) continue;
    if(kick[port]) {
      state = ext_state(port);           /* Port-Zustand holen          */
      if (fullduplex(port)) state |= CH_FDX;
      if (((state & CH_FDX) == 0) &&
          ((state & CH_PTT) == 0)) {     /* Sender noch nicht an        */
        if (p->l1_tx_timer > ticks) {
          p->l1_tx_timer -= ticks;
        } else {
          if ((state & CH_DCD) ||
              (state & CH_RXB) ||
              (rand()%256>p->persistance))
            p->l1_tx_timer = p->slottime;
          else
            p->l1_tx_timer = 0;
        }
      } else p->l1_tx_timer = 0;

      if (p->l1_tx_timer == 0) {
        if ((fp = l1_get_txbuf(port)) != NULL) {
          fp->len = 0;
          fp->port = port;
          fp->txdelay = portpar[port].txdelay;
          ulink((LEHEAD *)(txfhdl = (MBHEAD *) l2flp->head));/*Zeiger holen*/
          if (txfhdl->mbpc < 400)
            fp->len = cpymbflat(fp->data, txfhdl);
          relink((LEHEAD *)txfhdl,   /* als gesendet betrachten und in */
            (LEHEAD *)stfl.tail);    /* die gesendet Liste umhaengen   */
          kick[port] = ((LHEAD *)l2flp->head != l2flp);
          l1_kick();
        }
      }
    }
  }
}

static WORD ext_dcd(PORTINFO *port)
{
  int state = 0;
  int hw_state;

  hw_state = l1_state(port->minor);
  if (hw_state & CH_DCD)          /* DCD an, kein Duplex, nicht */
    state |= DCDFLAG;             /* senden                     */
  if (hw_state & CH_RXB)          /* Daten im empfaenger, nicht */
    state |= RXBFLAG;             /* senden (auch bei Duplex)   */
  /* der externe Treiber sagt uns, wann die PTT wirklich an ist
     und nicht wann das letzte Zeichen raus ist, das ist fuer
     Duplex nicht zu gebrauchen, dafuer ist hier aber auch die
     Verzoegerung zwischen uebergabe an stfl und Sendung nicht
     so gross */
  if (hw_state & CH_PTT)          /* PTT an, kein Duplex, warten*/
    state |= PTTFLAG;             /* bis PTT aus                */
  return(state);
}

/* Durch die feste Bindung minor=port ist die Pruefung auf Doppelbelegung */
/* nicht notwendig. Man koennte das aendern, aber das verwirrt dann ganz  */
/* schoen, wenn die Ports nicht in Ladereihenfolge genommen werden.       */
static int ext_attach(int port, int unused_devnr, BOOLEAN check_only)
{
  if (dev_irq > 0)
    if (port < l1_enum_ports()) {
      if (!check_only)
        portpar[port].minor = port;
      return(1);
    }
  return(0);
}

static void ext_info(int what, int port, MBHEAD *mbp)
{
  char          str[11];
  L1STATISTICS *stat;
  int           cnt;

  switch (what) {
    case HW_INF_IDENT :
      putstr("EXTDEV", mbp);
      break;
    case HW_INF_INFO :
      strncpy(str, l1_ident(port), 10);
      str[10] = 0; /* maximal 10 Zeichen */
      putstr(str, mbp);
      break;
    case HW_INF_STAT :
      for (port = cnt = 0; port < l1_enum_ports(); port++)
        if (portpar[port].major == ext_major) {
          if (cnt++ == 0)
            putstr("\rExternal-Statistics:\r\r", mbp);
          stat = l1_stat(port, 0);
          putprintf(mbp, "  %-10.10s TxErr: %5lu  RxOvr: %5lu  OFlow: %5lu  IOErr: %5lu\r",
                         l1_ident(port),
                         stat->tx_error, stat->rx_overrun, stat->rx_bufferoverflow,
                         stat->io_error);
        }
      break;
    case HW_INF_CLEAR :
      l1_stat(port, 1);
      /* durchfallen */
    default :
      default_l1info(what, port, mbp);
  }
}

static void ext_ctl(int req, int port)
{
  switch (req) {
    case L1CCMD :
    case L1CRES :
      l1_init_port(port, portpar[port].speed, portpar[port].l1mode);
      break;
  }
  default_l1ctl(req, port); /* Flags loeschen */
};

static int register_extdev(void)
{
  MAJOR *m;

  if (find_devirq()) {                  /* Externes Interface suchen    */
    m = register_major();
    m->name   = "EXTDEV";
    m->dcd    = ext_dcd;
    m->attach = ext_attach;
    m->info   = ext_info;
    m->timer  = ext_timer;
    m->ctl    = ext_ctl;
    return(ext_major = num_major);
  }
  return(0);
}

/* End of os/go32/extdev.c */
