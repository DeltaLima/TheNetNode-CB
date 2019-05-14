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
/* File os/linux/vanlinux.c (maintained by: DG1KWA)                     */
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

#ifdef VANESSA

#include "vanlinux.h"

/* Macros aus /usr/include/asm/io.h - da die bei einigen Linux-Distri-  */
/* butionen fehlen ..                                                   */

/*
 * readX/writeX() are used to access memory mapped devices. On some
 * architectures the memory mapped IO stuff needs to be accessed
 * differently. On the x86 architecture, we just read/write the
 * memory location directly.
 */

#ifndef readb
#define readb(addr) (*(volatile unsigned char *) (addr))
#endif
#ifndef readw
#define readw(addr) (*(volatile unsigned short *) (addr))
#endif
#ifndef readl
#define readl(addr) (*(volatile unsigned int *) (addr))
#endif

#ifndef writeb
#define writeb(b, addr) ((*(volatile unsigned char *) (addr)) = (b))
#endif
#ifndef writew
#define writew(b, addr) ((*(volatile unsigned short *) (addr)) = (b))
#endif
#ifndef writel
#define writel(b, addr) ((*(volatile unsigned int *) (addr)) = (b))
#endif

#ifndef memset_io
#define memset_io(a, b, c)        memset((void *)(a), (b), (c))
#endif
#ifndef memcpy_fromio
#define memcpy_fromio(a, b, c)    memcpy((a), (void *)(b), (c))
#endif
#ifndef memcpy_toio
#define memcpy_toio(a, b, c)      memcpy((void *)(a), (b), (c))
#endif

#define ReadVanW(port, what) readw(base + (port<<12) + what)
#define WriteVanW(port, what, data) writew(data, base + (port<<12) + what)
#define ReadVanB(port, what) readb(base+(port<<12)+what)
#define WriteVanB(port, what, data) writeb(data, base + (port<<12) + what)
#define outportb(adr, data) outb(data, adr)
#define delay(data) usleep(100000)

/* lokale Funktionen */
static void vanessa_ctl(int);
static BOOLEAN van_enabled(int);
static void vanessa_send_test(int);
static void vanessa_get_frame(void);
static void vanessa_put_frame(void);
static void vanessa_config(WORD port);
static void vanessa_reset_tnc(void);

static BOOLEAN van_tst, van_res, van_cmd;
static char *base;
static char van_version[L2PNUM];       /* Versions-Kennung              */
static char van_revision[L2PNUM];      /* der VANESSA                   */
static char van_patch[L2PNUM];

/************************************************************************/
/* Level1 RX/TX fuer VANESSA                                            */
/*               wird staendig in der main() Hauptschleife aufgerufen.  */
/************************************************************************/
void vanessa(void)
{
  register unsigned int port;

  vanessa_get_frame();          /* read VANESSA CRAM               */
  vanessa_put_frame();          /* write frame into  VANESSA CRAM  */

  for (port = 1; port < L2PNUM; port += 2)      /* hb9pae 930521 */
  {
    if (van_enabled(port))
    {
      if (ReadVanW(port, dp_cCMD) == 0) /* pruefen of vanessa restart     */
      {
        commandflag[port]   = TRUE;    /* wenn TRUE, parameter uebergeben */
        commandflag[port-1] = TRUE;
        portstat[port].reset_count++;   /* und restart zaehlen             */
        WriteVanW(port, dp_cCMD, 1);
      }
    }
  }

  if (van_res)                      /* auxiliary cmd to Vanessa        */
  {
    vanessa_reset_tnc();            /* VANESSA resetten                */
    van_res = FALSE;
  }

  if (van_cmd || van_tst)
  {
    for (port = 0; port < L2PNUM; ++port)    /* hb9pae 930521 */
    {
      if (van_enabled(port))
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
    van_tst = van_cmd = FALSE;
  } /* if ( ... ) */
}

/************************************************************************/
/* Vanessa INIT                                                         */
/************************************************************************/
void vanessa_l1init(void)
{
  register unsigned int port;
  register int fd;

  /* Device Memory oeffnen */
  if ((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    printf("Error: can't open \"/dev/mem\", errno = %u\n", errno); /* sollte nie passieren */
    printf("(%s)\n", strerror(errno));
    exit(1);
  }

  /* Device als Speicherabbild bereitstellen, Offset bei D0000, Groesse 64 kB */
  if ((base = mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xD0000)) == MAP_FAILED)
  {
    printf("Error: can't map 64kb memory at 0xD0000, errno = %u\n", errno);
    printf("(%s)\n", strerror(errno));
    close(fd);
    exit(1);
  }

  /* Device wieder schliessen */
  close(fd);

  /* IO-Ports anfordern */
  for (port = 0; port < L2PNUM; ++port)
  {
    /* LED IO-Adresse belegen */
    if (ioperm(ledio_adr[port], 3, 1) == -1)
    {
      printf("Error: can't reserve io at address 0x%x, errno = %u\n", ledio_adr[port], errno);
      printf("(%s)\n", strerror(errno));
    }

    /* Reset IO-Adresse belegen */
    if (ioperm(reset_adr[port / 2], 3, 1) == -1)
    {
      printf("Error: can't reserve io at address 0x%x, errno = %u\n", ledio_adr[port], errno);
      printf("(%s)\n", strerror(errno));
    }
  }

  /* Vanessen konfigurieren */
  for (port = 0; port < L2PNUM; ++port)
  {
    if (van_enabled(port))
    {
      vanessa_config(port);                    /* erst mal konfigurieren */
      outportb(ledio_adr[port], LED_OFF);      /* LED Vanessa off        */
      WriteVanW(port, dp_iFProd, !CRAM_VALID);
    }
  }

  van_tst = van_res = van_cmd = FALSE;
}


/************************************************************************/
/* Frame aus Kette holen und in Ringbuffer legen                        */
/*----------------------------------------------------------------------*/
void vanessa_put_frame(void)
{
  register unsigned int port;
  register unsigned int len;
  register unsigned int live;
  MBHEAD *txfhdl;
  LHEAD  *l2flp;

  for (live = 0; live < 8; ++live)        /* max 4 Frames pro CRAM Buffer */
  {
    l2flp = txl2fl;
    for (port = 0; port < L2PNUM; l2flp++, ++port) /* jeden Port durchlaufen  */
    {
      if (kissmode(port) != KISS_VAN)
        continue;
      if (portenabled(port))
      {
        if (kick[port])                   /* was zum senden...            */
        {
          if (l2flp->head == l2flp)
          {
            kick[port] = FALSE;
            continue;
          }
          if (ReadVanW(port, dp_oFProd) != CRAM_VALID) /* CRAM Buffer free */
          {
            len = 0;
            ulink((LEHEAD *)(txfhdl = (MBHEAD *) l2flp->head));/*Zeiger holen*/
            while (txfhdl->mbgc < txfhdl->mbpc) /* solange Daten vorhanden sind */
              WriteVanB(port, dp_oBuffer + (len++), getchr(txfhdl));

            relink((LEHEAD *)txfhdl,   /* als gesendet betrachten und in */
              (LEHEAD *)stfl.tail);    /* die gesendet Liste umhaengen   */

            kick[port] = ((LHEAD *)l2flp->head != l2flp);
            WriteVanW(port, dp_iBufEmpty, 1);  /* Frame wird gesendet...  */
            WriteVanW(port, dp_oLFrm, len);          /* set size and ..   */
            WriteVanW(port, dp_oFProd, CRAM_VALID);  /* terminate         */
            usleep(50);
          } /* if CRAM_VALID... */
        } /* if kick[port]... */
      } /* Port enabled */
      else
        while (l2flp->head != l2flp)
          relink(ulink((LEHEAD *) l2flp->head), (LEHEAD *)stfl.tail);
    } /* for (port ....  */
  } /* for (live .... */
} /* end */


/************************************************************************/
/* vanessa_get_frame()  -  Frame(s) aus dem VANESSA CRAM holen          */
/*----------------------------------------------------------------------*/
void vanessa_get_frame(void)
{
  register unsigned int port;
  WORD i, l;
  MBHEAD *rxfhd;

  for (port = 0; port < L2PNUM; ++port)
  {
    if (van_enabled(port))
    {
      while (ReadVanW(port, dp_iFProd) == CRAM_VALID)
      {
        outportb(ledio_adr[port], LED_ON);

        l = ReadVanW(port, dp_iLFrm);

        if (l > 0)
        {
          (rxfhd = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2port = port;

          for (i = 0; i < l; ++i)
            putchr(ReadVanB(port, dp_iBuffer+i), rxfhd);

          relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);
        }
        WriteVanW(port, dp_iFProd, !CRAM_VALID);
        outportb(ledio_adr[port], LED_OFF);
        usleep(50);
      }
    }
  }
}

/************************************************************************/
/*   VANESSA mit Parametern versorgen                                   */
/*----------------------------------------------------------------------*/
void vanessa_config(WORD port)
{
  UWORD portspeed;

  if (van_enabled(port))
  {
    WriteVanW(port, dp_Slottime, portpar[port].slottime & 0xff);
    WriteVanW(port, dp_TxDelay, portpar[port].txdelay & 0xff);
    WriteVanW(port, dp_Persistance, (dama(port)
    ? 0Xff : portpar[port].persistance & 0xff));
    WriteVanW(port, dp_FullDuplex, fullduplex(port)?1:0);

    if (extclock(port))
      portspeed = 0xff07;
    else
        {
      switch (portpar[port].speed)
      {
        case  12:  portspeed = 0xff00; break;
        case  24:  portspeed = 0xff01; break;
        case  48:  portspeed = 0xff02; break;
        case  96:  portspeed = 0xff03; break;
        case 192:  portspeed = 0xff04; break;
        case 384:  portspeed = 0xff05; break;
        default:   portspeed = 0xff00; portpar[port].speed = 12;
      }
    }

    WriteVanW(port, dp_vanspeed, portspeed);

    WriteVanB((port&0xfe), ((dp_cData)+0x74F), multibaud(port) ? 1 : 0);
    WriteVanB(port, ((dp_cData)+0x74F), (multibaud(port) ? 1 : 0));
    WriteVanW(port, dp_oMagic, 0);
    WriteVanW(port, dp_reReadParams, CRAM_VALID);
    outportb(ledio_adr[port], LED_OFF);
    WriteVanW(port, dp_iFProd, !CRAM_VALID);
  }
}

/************************************************************************/
/* Reset Befehl an alle VANESSA's geben                                 */
/*----------------------------------------------------------------------*/
void vanessa_reset_tnc(void)
{
  register unsigned int port;

  for (port = 0; port < L2PNUM; ++port) /* Alle FP's durchgehen         */
  {
    if (   portpar[port].reset_port     /* soll dieser Reset bekommen?  */
        && van_enabled(port)
       )
    {
      portpar[port].reset_port = FALSE;     /* rueckstellen             */
      outportb(reset_adr[port >> 1], 0xFF); /*  set RESET and...        */
      delay(50);                            /* .. wait some time        */
      outportb(reset_adr[port >> 1], 0x0);  /*  remove RESET again      */
    }
  }
}


/************************************************************************/
/* TEST-Befehl ausfuehren, funktioniert nur ab VANESSA Firmware 4.0 !!  */
/*----------------------------------------------------------------------*/
void vanessa_send_test(int port)
{
  if (van_enabled(port))
  {
    if (ReadVanW(port, dp_oFProd) != CRAM_VALID)    /* CRAM Buffer free  */
    {
      WriteVanW(port, dp_oLFrm, -1);                 /* Frame Size = -1   */
      WriteVanW(port, dp_oFProd, CRAM_VALID);        /* CRAM buffer valid */
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
 van_version[port] = 4;         /* aktuelle Version steht nicht im DP */
 van_revision[port] = ReadVanB((port&0xFE), dp_cData+0x751) / 16;
 van_patch[port] = ReadVanB((port&0xFE), dp_cData+0x74E); /* new */
 return(ReadVanW(((port&0xFE)+1), dp_cData+0x750) == 0x3412);
}

WORD vanessa_dcd(int port)
{
  WORD state = 0;

  if (!van_enabled(port))
    return 0;

  if (ReadVanW(port,dp_oFProd) == CRAM_VALID) /* Sendedaten in der VAN     */
    state |= (WORD) TXBFLAG;
  if (ReadVanW(port,dp_iFProd) == CRAM_VALID) /* Empfangsdaten in der VAN  */
    state |= (WORD) RXBFLAG;
  if (ReadVanB(port,dp_iPTT))                 /* wir senden !              */
    state |= (WORD) PTTFLAG;
  if (!ReadVanB(port,dp_iDCD))                /* wir empfangen etwas       */
    state |= (WORD) DCDFLAG;

  return(state);
}

static void vanessa_ctl(int req)
{
  switch (req)
  {
    case L1CRES : van_res = TRUE; break;
    case L1CCMD : van_cmd = TRUE; break;
    case L1CTST : van_tst = TRUE; break;

    default : break;
  }
}

/************************************************************************/
/* Vanessa EXIT (nur fuer LINUX)                                        */
/************************************************************************/
void vanessa_l1exit(void)
{
  register unsigned int port;

  /* Mem wieder freigeben... */
  munmap(base, 0x10000);

  /* und die IO-Ports */
  for (port = 0; port < L2PNUM; ++port)
  {
    ioperm(ledio_adr[port], 3, 0);
    ioperm(reset_adr[port / 2], 3, 0);
  }
}

void vanessa_l1ctl(int req)
{
  vanessa_ctl(req);
}

static BOOLEAN van_enabled(int port)
{
  return(portenabled(port) && (kissmode(port) == KISS_VAN));
}

void van_hwstr(int port, MBHEAD *mbp)
{
  putprintf(mbp, " v4.%02x%c", van_revision[port], van_patch[port]);
}

#endif
