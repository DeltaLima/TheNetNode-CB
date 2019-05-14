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
/* File os/go32/vanessa.c (maintained by: ???)                          */
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

#include "vanessa.h"
#ifdef __GO32__
#include <sys/farptr.h>
#include <sys/movedata.h>
#include <go32.h>
#endif

#ifdef __GO32__
#define ReadVanW(port,what) _farpeekw(_dos_ds,0xD0000+((port)<<12)+(what))
#define WriteVanW(port,what,data) _farpokew(_dos_ds,0xD0000+((port)<<12)+(what),data)
#define ReadVanB(port,what) _farpeekb(_dos_ds,0xD0000+((port)<<12)+(what))
#define WriteVanB(port,what,data) _farpokeb(_dos_ds,0xD0000+((port)<<12)+(what),data)
#define ReadVanX(port,what,data,size) dosmemget(0xD0000+((port<<12)+(what)), size, data)
#define WriteVanX(port,what,data,size) dosmemput(data, size, 0xD0000+((port<<12)+(what)))
#else
#define ReadVanW(port,what) (*((unsigned *)MK_FP(0xD000,((port)<<12)+(what))))
#define WriteVanW(port,what,data) *((unsigned *)MK_FP(0xD000,((port)<<12)+(what)))=data
#define ReadVanB(port,what) (*((unsigned char *)MK_FP(0xD000,((port)<<12)+(what))))
#define WriteVanB(port,what,data) *((unsigned char *)MK_FP(0xD000,((port)<<12)+(what)))=data
#define ReadVanX(port,what,data,size) memcpy(data, MK_FP(0xD000,((port<<12)+(what))),size)
#define WriteVanX(port,what,data,size) memcpy(MK_FP(0xD000,((port<<12)+(what))),data,size)
#endif

/* lokale Funktionen */
static void vanessa_send_test(int);
static void vanessa_get_frame(void);
static void vanessa_put_frame(void);
static void vanessa_config(WORD);
static void vanessa_reset_tnc(void);
void        vanessa(void);
BOOLEAN     van_test(int);
WORD        vanessa_dcd(PORTINFO *);
void        vanessa_l1init(void);

static BOOLEAN van_tst, van_res, van_cmd;


static char van_version[L2PNUM];       /* Versions-Kennung              */
static char van_revision[L2PNUM];      /* der VANESSA                   */
static char van_patch[L2PNUM];

static int van_major = 0;

/************************************************************************/
/* Level1 RX/TX fuer VANESSA                                            */
/*               wird staendig in der main() Hauptschleife aufgerufen.  */
/************************************************************************/
void vanessa(void)
{
  WORD port;

  vanessa_get_frame();          /* read VANESSA CRAM */
  vanessa_put_frame();          /* write frame into  VANESSA CRAM  */

  for (port=1; port < L2PNUM; port += 2)      /* hb9pae 930521 */
  {
    if (portpar[port].major == van_major)
    {
      if (ReadVanW(port, dp_cCMD)==0) /* pruefen of vanessa restart      */
      {
        commandflag[port]   = TRUE;   /* wenn TRUE, parameter uebergeben */
        commandflag[port-1] = TRUE;
        portstat[port].reset_count++;  /* und restart zaehlen             */
        WriteVanW(port, dp_cCMD, 1);
      }
    }
  }

  if (van_res)                      /* auxiliary cmd to Vanessa        */
    vanessa_reset_tnc();            /* VANESSA resetten                */

  if (van_cmd || van_tst)
    for (port=0; port < L2PNUM; port++)      /* hb9pae 930521 */
    {
      if (portpar[port].major == van_major)
      {
        if (commandflag[port])
        {
          vanessa_config(port);           /* VANESSA Port neue Parameter */
          commandflag[port] = FALSE;
        }

        if (testflag[port])
        {
          vanessa_send_test(port);        /* VANESSA Test-Folge senden   */
          testflag[port] = FALSE;
        }
      } /* gueltiger VANESSA-Port */
    } /* alle Ports */

  van_tst = van_res = van_cmd = FALSE;
}

/************************************************************************/
/* Vanessa INIT                                                         */
/************************************************************************/
void vanessa_l1init(void)
{
  WORD port;

  for (port=0; port<L2PNUM; port++)
    if (portpar[port].major == van_major)
    {
      vanessa_config(port);                  /* erst mal konfigurieren */
      outportb(ledio_adr[port], LED_OFF);    /* LED Vanessa off        */
      WriteVanW(port,dp_iFProd,!CRAM_VALID); /* CRAM Vanessa loeschen  */
    }
  van_tst = van_res = van_cmd = FALSE;
}


/************************************************************************/
/* Frame aus Kette holen und in Ringbuffer legen                        */
/*----------------------------------------------------------------------*/
void vanessa_put_frame(void)
{
  int     port, len;
  MBHEAD *txfhdl;
  LHEAD  *l2flp;

  l2flp = txl2fl;
  for (port=0; port<L2PNUM; l2flp++, port++) /* jeden Port durchlaufen  */
    if (kick[port])                     /* was zum senden...            */
      if (portpar[port].major == van_major)
        if (ReadVanW(port,dp_oFProd) != CRAM_VALID) /* CRAM Buffer free */
        {
          ulink((LEHEAD *)(txfhdl = (MBHEAD *) l2flp->head)); /*Zeiger holen*/
          len = cpymbflat(blkbuf, txfhdl);
          WriteVanX(port, dp_oBuffer, blkbuf, len);
          relink((LEHEAD *)txfhdl,    /* als gesendet betrachten und in */
            (LEHEAD *)stfl.tail);     /* die gesendet Liste umhaengen   */
          kick[port] = ((LHEAD *)l2flp->head != l2flp);
          WriteVanW(port,dp_iBufEmpty,1);       /* Frame wird gesendet  */
          WriteVanW(port,dp_oLFrm,len);         /* set size and ...     */
          WriteVanW(port,dp_oFProd,CRAM_VALID); /* Terminate            */
        } /* if CRAM_VALID... */
} /* end */

/************************************************************************/
/* vanessa_get_frame()  -  Frame(s) aus dem VANESSA CRAM holen          */
/*----------------------------------------------------------------------*/
void vanessa_get_frame(void)
{
  int port;
  WORD l;
  MBHEAD *rxfhd;

  for (port=0; port < L2PNUM; port++)
    if (portpar[port].major == van_major)
      while (ReadVanW(port,dp_iFProd)==CRAM_VALID)  /* frame available  */
      {
        outportb(ledio_adr[port],LED_ON);
        l = ReadVanW(port,dp_iLFrm);             /* get size            */
        if (l >= BLOCKSIZE) l = 0;
        if (l > 0) {
          ReadVanX(port, dp_iBuffer, blkbuf, l);
          (rxfhd = cpyflatmb(blkbuf, l))->l2port = port;
          relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);
        }
        WriteVanW(port,dp_iFProd,!CRAM_VALID);     /* CRAM free         */
        outportb(ledio_adr[port],LED_OFF);
      } /* Frame empfangen */
}

/************************************************************************/
/*   VANESSA mit Parametern versorgen                                   */
/*----------------------------------------------------------------------*/
void vanessa_config(WORD port)
{
  WORD portspeed;
  PORTINFO *p;

  if (portpar[port].major == van_major)
  {
    p = portpar+port;
    WriteVanW(port,dp_Slottime, p->slottime & 0xff);
    WriteVanW(port,dp_TxDelay, p->txdelay & 0xff);
    WriteVanW(port,dp_Persistance, p->persistance & 0xff);
    WriteVanW(port,dp_FullDuplex, fullduplex(port)?1:0);

    if (extclock(port)) portspeed = 0xff07; /* External Clock!! */
    else
    switch (p->speed)
    {
      case  12:  portspeed = 0xff00; break;
      case  24:  portspeed = 0xff01; break;
      case  48:  portspeed = 0xff02; break;
      case  96:  portspeed = 0xff03; break;
      case 192:  portspeed = 0xff04; break;
      case 384:  portspeed = 0xff05; break;
      default:   portspeed = 0xff00; p->speed = 12;
    }
    WriteVanW(port,dp_vanspeed,portspeed);
    /* mhm, das sieht mir komisch aus ##BUMP## DB7KG */
    WriteVanB((port&0xfe), ((dp_cData)+0x74F), multibaud(port) ? 1 : 0);
    WriteVanB(port, ((dp_cData)+0x74F), multibaud(port) ? 1 : 0);
    WriteVanW(port, dp_oMagic, 0);
    WriteVanW(port, dp_reReadParams, CRAM_VALID);
    outportb(ledio_adr[port], LED_OFF);    /* LED Vanessa off        */
    WriteVanW(port,dp_iFProd,!CRAM_VALID); /* CRAM Vanessa loeschen  */
  }
}

/************************************************************************/
/* Reset Befehl an alle VANESSA's geben                                 */
/*----------------------------------------------------------------------*/
void vanessa_reset_tnc(void)
{
  int port;

  for (port = 0; port < L2PNUM; port++) /* Alle FP's durchgehen         */
  {
    if (   portpar[port].reset_port     /* soll dieser Reset bekommen?  */
        && portpar[port].major == van_major
       )
    {
      portpar[port].reset_port = FALSE; /* rueckstellen                 */
      outportb(reset_adr[port>>1],0xFF);/*  set RESET and...            */
      delay(50);                        /* .. wait some time            */
      outportb(reset_adr[port>>1],0x0); /*  remove RESET again          */
    }
  }
}


/************************************************************************/
/* TEST-Befehl ausfuehren, funktioniert nur ab VANESSA Firmware 4.0 !!  */
/*----------------------------------------------------------------------*/
void vanessa_send_test(int port)
{
  if (portpar[port].major == van_major)
  {
    if (ReadVanW(port,dp_oFProd) != CRAM_VALID)    /* CRAM Buffer free  */
    {
      WriteVanW(port,dp_oLFrm,-1);                 /* Frame Size = -1   */
      WriteVanW(port,dp_oFProd,CRAM_VALID);        /* CRAM buffer valid */
    }
    testflag[port] = FALSE;                        /* TEST fertig       */
  }
}

/*******************************************************************/
/* Testen ob ueberhaupt ne VAN da ist                              */
/* Patch dazu gekommen, 22/02/97 DG1KWA                            */
/* PS: in der alten Routine war auch noch ein Fehler :-)           */
/*******************************************************************/

BOOLEAN van_test(int port)
{
  van_version[port] = 4; /* aktuelle Version steht nicht im DP */
  van_revision[port] = ReadVanB((port&0xFE),dp_cData+0x751) / 16;
  van_patch[port] = ReadVanB((port&0xFE),dp_cData+0x74E); /* new */
  return(ReadVanW((port&0xFE)+1,dp_cData+0x750)==0x3412);
}

WORD vanessa_dcd(PORTINFO *port)
{
  int state = 0;
  int minor = port->minor;

  if (ReadVanW(minor,dp_oFProd) == CRAM_VALID) /* Sendedaten in der VAN */
    state |= TXBFLAG;
  if (ReadVanW(minor,dp_iFProd) == CRAM_VALID) /* Empfangsdaten */
    state |= RXBFLAG;
  if (ReadVanB(minor,dp_iPTT))
    state |= PTTFLAG;
  if (!ReadVanB(minor,dp_iDCD))
    state |= DCDFLAG;
  return(state);                /* Kanal ist frei             */
}

static void vanessa_ctl(int req, int port)
{
  switch(req) {
    case L1CRES: van_res = TRUE;
                 break;
    case L1CCMD: van_cmd = TRUE;
                 break;
    case L1CTST: van_tst = TRUE;
  }
}

static int vanessa_attach(int port, int unused_minor, BOOLEAN check_only)
{
  if (van_test(port)) {
    if (!check_only) /* nur pruefen? */
      portpar[port].minor = port;
    return(1);
  }
  return(0);
}

static void vanessa_info(int what, int port, MBHEAD *mbp)
{
  switch (what) {
    case HW_INF_IDENT :
      default_l1info(what, port, mbp);
      break;
    case HW_INF_INFO :
      putprintf(mbp, "VAN v%x.%02x%c",
                     van_version[port],
                     van_revision[port],
                     van_patch[port]);
      break;
    default :
      default_l1info(what, port, mbp);
  }
}

static int register_vanessa(void)
{
  MAJOR *m;
  int    i;

  for (i = 0; i < L2PNUM; i++)
    if (van_test(i)) { /* das koennte Probleme machen! */
      m = register_major();
      m->name   = "VANESSA";
      m->handle = vanessa;
      m->ctl    = vanessa_ctl;
      m->dcd    = vanessa_dcd;
      m->attach = vanessa_attach;
      m->info   = vanessa_info;
      return(van_major = num_major);
    }
  return(0);
}

/* End of os/go32/vanessa.c */
