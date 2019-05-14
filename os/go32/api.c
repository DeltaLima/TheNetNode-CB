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
/* File os/go32/api.c (maintained by: ???)                              */
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

#include "hardware.h"

#ifdef EXTDEV

#include <go32.h>
#include <dpmi.h>
#include "api.h"
#include "api32.h"

char     manifest[20];
unsigned dev_irq = 0;
__dpmi_raddr tx_rframe;
L1FRAME  tx_pframe;

/* Anzahl der Kanaele holen */
int l1_enum_ports(void)
{
    __dpmi_regs r;

    if (dev_irq == 0) return(0);
    r.x.ax = API32_ENUM_PORTS;
    __dpmi_int(dev_irq+1, &r);
    return(r.x.ax);
}

/* einen Kanal initialisieren */
int l1_init_port(unsigned port, unsigned baud, unsigned mode)
{
    __dpmi_regs r;

    if (dev_irq == 0) return(0);
    r.x.ax = API32_INIT_PORT;
    r.x.bx = port & 0xFFFF;
    r.x.cx = baud & 0xFFFF;
    r.x.dx = mode & 0xFFFF;
    __dpmi_int(dev_irq+1, &r);
    return(r.x.ax);
}

/* das naechste Frame empfangen */
void *l1_rx_frame(void)
{
    static L1FRAME frame_buf;
    __dpmi_regs r;

    if (dev_irq == 0) return(NULL);
    r.x.ax = API32_RX_FRAME;
    __dpmi_int(dev_irq+1, &r);
    if (r.x.ax) {
      dosmemget((((int)r.x.es)<<4)+r.x.bx,sizeof(L1FRAME),&frame_buf);
      return(&frame_buf);
    }
    return(NULL);
}

/* einen Sende-Buffer holen */
L1FRAME far *l1_get_txbuf(unsigned port)
{
    __dpmi_regs r;

    if (dev_irq == 0) return(NULL);
    r.x.ax = API32_GET_TXBUF;
    r.x.bx = port & 0xFFFF;
    __dpmi_int(dev_irq+1, &r);
    if (r.x.ax) { /* Buffer bekommen */
      tx_rframe.segment = r.x.es;
      tx_rframe.offset16 = r.x.bx;
      return(&tx_pframe);
    }
    return(NULL);
}

/* den Sender anstossen */
void l1_kick(void)
{
    __dpmi_regs r;

    if (dev_irq == 0) return;
    dosmemput(&tx_pframe,sizeof(L1FRAME),
        ((int)tx_rframe.segment<<4)+tx_rframe.offset16);
    r.x.ax = API32_KICK;
    __dpmi_int(dev_irq+1, &r);
}

/* Calibrierung durchfuehren */
void l1_calibrate(unsigned port, unsigned minutes)
{
    __dpmi_regs r;

    if (dev_irq == 0) return;
    r.x.ax = API32_CALIBRATE;
    r.x.bx = port & 0xFFFF;
    r.x.cx = minutes & 0xFFFF;
    __dpmi_int(dev_irq+1, &r);
}

/* den Kanal-Status abfragen */
unsigned l1_state(unsigned port)
{
    __dpmi_regs r;

    if (dev_irq == 0) return(0);
    r.x.ax = API32_STATE;
    r.x.bx = port & 0xFFFF;
    __dpmi_int(dev_irq+1, &r);
    return(r.x.ax);
}

/* den Hardwarenamen eines Kanals holen */
char far *l1_ident(unsigned port)
{
    static char ident[20];
    __dpmi_regs r;

    if (dev_irq == 0) return(NULL);
    r.x.ax = API32_IDENT;
    r.x.bx = port & 0xFFFF;
    __dpmi_int(dev_irq+1, &r);
    dosmemget((((int)r.x.es)<<4)+r.x.bx, 20, ident);
    return(ident);
}

/* ist dieser Kanal aktiv? */
int l1_active(unsigned port)
{
    __dpmi_regs r;

    if (dev_irq == 0) return(0);
    r.x.ax = API32_ACTIVE;
    r.x.bx = port & 0xFFFF;
    __dpmi_int(dev_irq+1, &r);
    return(r.x.ax);
}

/* die L1-Statistik lesen bzw loeschen */
L1STATISTICS far *l1_stat(unsigned port, int del)
{
    static L1STATISTICS pstat;
    __dpmi_regs r;

    if (dev_irq == 0) return(NULL);
    r.x.ax = API32_STAT;
    r.x.bx = port & 0xFFFF;
    r.x.cx = del & 0xFFFF;
    __dpmi_int(dev_irq+1, &r);
    dosmemget(((int)r.x.es<<4)+r.x.bx, sizeof(L1STATISTICS), &pstat);
    return(&pstat);
}

int find_devirq(void)
{
    unsigned irq;
    __dpmi_raddr p;

    for (irq = 0x60; irq <= 0xF0; irq++) { /* freien IRQ suchen */
        __dpmi_get_real_mode_interrupt_vector(irq, &p);
        if (p.segment) {
            dosmemget((((int)p.segment)<<4)+p.offset16, 20, manifest);
            if (strncmp(manifest,"FlexNet",7)==0)
            {   /* schon installiert */
                dev_irq = irq;
                return(1);
            }
        }
    }
    return(0);
}

#endif

/* End of os/go32/api.c */
