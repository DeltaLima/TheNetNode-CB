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
/* File os/go32/16550.c (maintained by: ???)                            */
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

#include "pc.h"

#define TRX (base+0)
#define IER (base+1)                    /* interrupt Enable register    */
#define IIR (base+2)                    /* Interrupt ident. register    */
#define FCR (base+2)                    /* Fifo  Control register       */
#define LCR (base+3)                    /* Line  Control register       */
#define MCR (base+4)                    /* Modem Control register       */
#define LSR (base+5)                    /* Line  Status register        */
#define MSR (base+6)                    /* Modem Status register        */

typedef struct {
  unsigned irq;
  unsigned base;
  unsigned oldmask;
  unsigned in_buffersize;
  char    *in_buffer;
  unsigned in_buffer_in;
  unsigned in_buffer_out;
  unsigned out_buffersize;
  char    *out_buffer;
  unsigned out_buffer_in;
  unsigned out_buffer_out;
  int      out_aktiv;
  int      is_16550;
  int      active;
} __com_struct;

__com_struct asydev[MAXCOMS];

/**********************************************************************/
/* Aus dem Enviroment die Konfiguration fuer eine Schnittstelle lesen */
/**********************************************************************/
void read_envcom(char *what, int *dev, int *adr, int *irq)
{
  *dev = 0;
  *adr = 0;
  *irq = 0;

  if (getenv(what) != NULL)
    sscanf(getenv(what), "%u,%x,%u", dev, adr, irq);

  if (*adr == 0)
  {
    switch (*dev)
    {
      case 1 : *adr = 0x3f8; break;
      case 2 : *adr = 0x2f8; break;
      case 3 : *adr = 0x3e8; break;
      case 4 : *adr = 0x2e8; break;
    }
  }

  if (*irq == 0)
  {
    switch (*dev)
    {
      case 1 : *irq = 4; break;
      case 2 : *irq = 3; break;
      case 3 : *irq = 5; break;
      case 4 : *irq = 7; break;
    }
  }

  if (*irq == 0 || *adr == 0 || *dev == 0) {
    *adr = *irq = *dev = 0;
  }
  (*dev)--;
}

void rs232_int(int dev) {

  int iir;                              /* Interrupt Ident Register     */
  register int i = 16;
  __com_struct *ap = &asydev[dev];
  register int base;

  base = ap->base;

  if (!ap->is_16550) {
    /*********************************************************************
    * <<<<<<<<<<< interrupt function for 8250 and 16450 >>>>>>>>>>>>>>>  *
    *********************************************************************/
    while ((iir = inportb(IIR)) != 1) {      /* while interrup pending  */
      switch (iir) {
        case 0x04:
            /*************************************************************
             * interrupt type is - Receiver data available               *
             * interrupt reset with : read rx buffer register            *
             ************************************************************/
                                        /* read data + clear interrupt  */
            ap->in_buffer[ap->in_buffer_in++] = inportb(TRX);
            if (ap->in_buffer_in == ap->in_buffersize)
              ap->in_buffer_in = 0;
          break;

        case 0x02:
            /*************************************************************
             * interrupt type is - Transmitter Holding Registre Empty    *
             *   source int. is - Transmitter Holding Registre Empty     *
             * interrupt reset with : reading iir if source              *
             *                        or write in tx holding register    *
             ************************************************************/
          if (ap->out_buffer_in != ap->out_buffer_out) {
              outportb(TRX, ap->out_buffer[ap->out_buffer_out++]);
              if (ap->out_buffer_out == ap->out_buffersize)
                ap->out_buffer_out = 0;
          }
          else
            ap->out_aktiv = FALSE;
          break;

        case 0x06:
            /*************************************************************
             * interrupt type is - receiver line status                  *
             *   source int is  - Overun error   or  - Parity erorr      *
             *                  - framming error or  - Break interrupt   *
             * interrupt reset with : reading the Line Status Register   *
             ************************************************************/
          ap->in_buffer[ap->in_buffer_in++] = 0xdb;
          if (ap->in_buffer_in == ap->in_buffersize)
            ap->in_buffer_in = 0;
          ap->in_buffer[ap->in_buffer_in++] = 0x00; /* erzeugt Bad Frame*/
          if (ap->in_buffer_in == ap->in_buffersize)
            ap->in_buffer_in = 0;
          inportb(LSR);                          /* reset this interupt */
          break;

        case 0x00:
            /*************************************************************
             * interrupt type is - Modem status  (normal never activ)    *
             *     source int is - CTS or DSR or Ring or DCD             *
             * interrupt reset with : reading Modem Status Register      *
             ************************************************************/
          inportb(MSR);                       /* reset interrupt        */
          break;
        } /* end switch  */
      } /* end while   */
    } /*  end if     */

  else {
    /*********************************************************************
     * <<<<<<<<<<<<<<<<< interrupt function for 16550 >>>>>>>>>>>>>>>>>> *
     ********************************************************************/
    while ((iir = inportb(IIR)&0x8f) != 0x81){ /* while interrup pending */
      switch (iir) {
        case 0x84:
        case 0x8C:
            /*************************************************************
             * interrupt type is - Receiver data available (0x84)  or    *
             *                   - Caractere timeout   (0x8C)            *
             * interrupt reset with : read rx buffer register  Drops     *
             *                       below the trigger Level (0x84)      *
             *                     : read all the caracter in rx buffer  *
             ************************************************************/
                                                  /* while data ready ? */
           while((inportb(LSR) & 0x01) == 0x01) {
             ap->in_buffer[ap->in_buffer_in++] = inportb(TRX);
             if (ap->in_buffer_in == ap->in_buffersize)
               ap->in_buffer_in = 0;
           }
           break;

        case 0x82:
            /*************************************************************
             * interrupt type is - Transmitter Holding Register Empty    *
             *    source int. is - Transmitter Holding Register Empty    *
             * interrupt reset with : reading iir if source              *
             *                        or write in tx holding register    *
             ************************************************************/
          if (ap->out_buffer_in == ap->out_buffer_out )
            ap->out_aktiv = FALSE;
          else {
              do {
                outportb(TRX, ap->out_buffer[ap->out_buffer_out++]);
                if (ap->out_buffer_out == ap->out_buffersize)
                  ap->out_buffer_out = 0;
                }
              while((ap->out_buffer_in != ap->out_buffer_out) && (--i > 0));
            }
          break;

        case 0x86:
            /*************************************************************
             * interrupt type is - receiver line status                  *
             *   source int is  - Overun error   or  - Parity erorr      *
             *                  - framming error or  - Break interrupt   *
             * interrupt reset with : reading the Line Status Register   *
             ************************************************************/
          ap->in_buffer[ap->in_buffer_in++] = 0xdb;
          if (ap->in_buffer_in == ap->in_buffersize)
            ap->in_buffer_in = 0;
          ap->in_buffer[ap->in_buffer_in++] = 0x00;  /* erzeugt Bad Frame    */
          if (ap->in_buffer_in == ap->in_buffersize)
            ap->in_buffer_in = 0;
          inportb(LSR);                          /* reset this interupt  */
          break;

        case 0x80:
            /*************************************************************
             * interrupt type is - Modem status  (normal never activ)    *
             *    source int is - CTS or DSR or Ring or DCD              *
             * interrupt reset with : reading Modem Status Register      *
             ************************************************************/
          inportb(MSR);                               /* reset interrupt */
          break;

      } /* end switch */
    } /* end while  int */
  } /* end else 8250 16450 / 16550 */
} /* end function */

void clear_sio(int dev) {
  int oldier;
  int c = 1;
  __com_struct *ap = &asydev[dev];
  register int base;

  base = ap->base;

  /* Empfangs-Overrun loeschen */
  while (inportb(LSR) != 0x60 && (c++ <= 1600)) inportb(TRX);

  /* Interruptfehlerquellen loeschen */
  oldier = inportb(IER);               /* alle Interrupts sperren */
  outportb(IER, 0);

  c = 1;
  while (inportb(IIR) == 0 && c <= 10) {
    if ((inportb(IIR) & 6) == 6) {
       inportb(LSR);
       eoi();
    }
    if ((inportb(IIR) & 4) == 4) {
       inportb(TRX);
       eoi();
    }
    if (inportb(IIR) == 0) {
       inportb(MSR);
       eoi();
    }
    c++;
  }

  outportb(IER, oldier);
}

void clear_rs232(int dev) {

  __com_struct *ap = &asydev[dev];
  register int base;

  if (!ap->active) return;

  base = ap->base;

  clear_sio(dev);

  disable();                          /* Int vorsichtshalber abschalten */
  if (ap->is_16550) {
    outportb(FCR, 0x87);              /* FIFO ON , reset rx and tx FIFO */
    outportb(FCR, 0x81);              /* trigger level = 14 bytes       */
  }

  do
  {
    inportb(TRX);                     /* Datenregister leeren           */
  }
  while((inportb(LSR) & 0x01) == 0x01);

  ap->in_buffer_in = ap->in_buffer_out = ap->out_buffer_in =
  ap->out_buffer_out = 0;
  ap->out_aktiv = FALSE;

  enable();                                            /* int wieder an */
}

/*----------------------------------------------------------------------*/
/* Initialisieren der Schnittstelle, OHNE Baudrate setzen!              */
/* Buffer initialisieren, Interrupt Vektoren setzen                     */
/*                                                                      */
/* 16550A-FIFO-Bits  (Portadresse [Basis+2])                            */
/* ================                                                     */
/*                                                                      */
/* Bit     Read: IIR               Write: FIFO Control                  */
/* ---------------------------------------------------                  */
/*  0      Interrupt Pending       FIFO enable                          */
/*  1      ID-Bit 1                FIFO Reset Receive                   */
/*  2      ID-Bit 2                FIFO Reset Transmit                  */
/*  3      ID-Bit 3 (Timeout)      0 (im PC/AT)                         */
/*  4      0                       0                                    */
/*  5      0                       0                                    */
/*  6      FIFO ON (nur 16550A)    Trigger Level TL0                    */
/*  7      FIFO ON                 Trigger Level TL1                    */
/*                                                                      */
/*                                 TL1 TL0   Trigger-Level              */
/*                                  0   0    1 Byte                     */
/*                                  0   1    4 Bytes                    */
/*                                  1   0    8 Bytes                    */
/*                                  1   1   14 Bytes                    */
/*----------------------------------------------------------------------*/
int open_rs232(int dev, int base, int irq, int insize, int outsize)
{
  __com_struct *ap = &asydev[dev];

  if (ap->active) {
    printf("*** WARNING: COM%u is already in use!\n",dev+1);
    return(-1);
  }

  ap->active = TRUE;

  ap->base = base;
  ap->irq = irq;

  clear_sio(dev);

  outportb(FCR, 0x01);                    /* enable FIFO mode (16550)   */

  printf("--- /dev/COM%u (0x%03X,IRQ%u), UART is ", dev+1, base, irq);

  if ((inportb(FCR) & 0x80) == 0x80) {    /* test if FIFO enabled       */
    ap->is_16550 = TRUE;
    printf("16550A");
    outportb(FCR,0x87);               /* FIFO ON, clear rx and tx FIFO  */
  }
  else {
    ap->is_16550 = FALSE;
    printf("8250/16450");
  }

  do {
    inportb(TRX);                       /* Datenregister leeren         */
  } while((inportb(LSR) & 0x01) == 0x01);

  outportb(LCR, 0x07);                  /* 8bit + 2 stop, DLAB auf 0    */
  outportb(IER, 0x07);                  /* Interrupt: Rx, TX, RLS       */
  outportb(MCR, 0x08);                  /* Interrupt frei (out2)        */

  ap->in_buffer_in = ap->in_buffer_out = ap->out_buffer_in =
  ap->out_buffer_out = 0;
  ap->out_aktiv = FALSE;

  disable();
  allocirq(irq, 0, rs232_int, dev);     /* Vektor setzen                */
  ap->oldmask = getmask(irq);
  maskon(irq);
  ap->out_buffer = malloc(ap->in_buffersize = insize);
  ap->in_buffer  = malloc(ap->out_buffersize = outsize);
  enable();                             /* Interrupt frei               */

  return(dev);
}

/*----------------------------------------------------------------------*/
/* Schnittstelle sperren                                                */
/*----------------------------------------------------------------------*/
void close_rs232(int dev) {
  __com_struct *ap = &asydev[dev];
  register int base;

  if (!ap->active) return;

  base = ap->base;
  clear_sio(dev);

  disable();
  if (!ap->oldmask) maskoff(ap->irq);
  outportb(IER, 0x00);                  /* keine Interrupt Freigabe     */
  outportb(MCR, 0x08);                  /* Out2 sperrt Interrupt        */
  if (ap->is_16550)
    outportb(FCR,0x00);                 /* FIFO OFF                     */
  freeirq(ap->irq);
  enable();
}

/*----------------------------------------------------------------------*/
/* Neuer Syntax nach DL1XAO, Implementierung DB7KG:                     */
/* Setzt die naechstbeste Baudrate und liefert die gesetzte zurueck.    */
/*----------------------------------------------------------------------*/
int setbaud(int dev, int argu) {

  __com_struct *ap = &asydev[dev];
  register int base;
  int baudtab[] = {12,24,48,96,192,384,576,1152,0}, *i, lsb;

  if (!ap->active) return(0);

  base = ap->base;

  clear_sio(dev);

  for (i = baudtab; *i; i++)
    if (argu <= *i) break;

  if (*i) argu = *i;      /* wenn mehr als Maximalwert gewuenscht,*/
  else argu = 1152;       /* dann begrenzen                       */

  lsb = 1152 / argu;      /* Teiler berechnen                     */

  outportb(LCR, 0x87);    /* auf Baudraten Register  2 stop bit   */
  outportb(TRX, lsb );    /* LSB Teilerfaktor                     */
  outportb(IER, 0x00);    /* MSB                                  */
  outportb(LCR, 0x07);    /* 8 Bit, keine Paritaet, 2 Stop        */

  return(argu);
}

/*----------------------------------------------------------------------*/
/* Anzahl der Stopbits setzen                                           */
/*----------------------------------------------------------------------*/
void setstopbits(int dev, int stops)
{
  __com_struct *ap = &asydev[dev];
  register int base;

  base = ap->base;

  if (!ap->active) return;

  clear_sio(dev);
  if (stops == 2) outportb(LCR, 0x07); /* 8 Bit, keine Paritaet, 2 Stop */
  else outportb(LCR, 0x03);            /* 8 Bit, k. Paritaet, 1 Stop    */
  clear_sio(dev);
}

/*----------------------------------------------------------------------*/
/* Zeichen aus Buffer an Programm liefern                               */
/*----------------------------------------------------------------------*/
int rs232_in(int dev) {
  char  c;
  register __com_struct *ap = &asydev[dev];

  if (!ap->active) return(-1);

  if (ap->in_buffer_in != ap->in_buffer_out) {
    c = ap->in_buffer[ap->in_buffer_out++];
    if (ap->in_buffer_out == ap->in_buffersize)
      ap->in_buffer_out = 0;
    return(c);
  }
  return(-1);
}

/*----------------------------------------------------------------------*/
/* Zeichen an Schnittstelle geben, ggfs im Buffer speichern             */
/*----------------------------------------------------------------------*/
void rs232_out(int dev, int c) {
  register __com_struct *ap = &asydev[dev];

  if (!ap->active) return;

  disable();
  if (ap->out_aktiv) {
    ap->out_buffer[ap->out_buffer_in++] = c;
    if (ap->out_buffer_in == ap->out_buffersize)
      ap->out_buffer_in = 0;
  }
  else {
    ap->out_aktiv = TRUE;
    outportb(ap->base,c);
  }
  enable();
}

/*----------------------------------------------------------------------*/
/* Blockweise von der Schnittstelle lesen                               */
/*----------------------------------------------------------------------*/
int rs232_read(int dev, UBYTE *buf, int max) {
  register __com_struct *ap = &asydev[dev];
  int n;

  if (!ap->active) return(-1);

  disable();
  for (n = 0; n < max; n++) {
    if (ap->in_buffer_in != ap->in_buffer_out) {
      *buf++ = ap->in_buffer[ap->in_buffer_out++];
      if (ap->in_buffer_out == ap->in_buffersize)
        ap->in_buffer_out = 0;
    } else {
      enable();
      return(n);
    }
  }
  enable();
  return(n);
}

/*----------------------------------------------------------------------*/
/* Speicherblock an Schnittstelle geben, ggfs im Buffer speichern       */
/*----------------------------------------------------------------------*/
void rs232_write(int dev, UBYTE *start, UBYTE *end) {
  register __com_struct *ap = &asydev[dev];

  if (!ap->active) return;

  disable();
  if (!ap->out_aktiv) {
    ap->out_aktiv = TRUE;
    outportb(ap->base, *start++);
  }
  while((end - start) > 0L) {
    ap->out_buffer[ap->out_buffer_in++] = *start++;
    if (ap->out_buffer_in == ap->out_buffersize)
      ap->out_buffer_in = 0;
  }
  enable();
}

/*----------------------------------------------------------------------*/
/* Status Eingabe                                                       */
/*----------------------------------------------------------------------*/
int rs232_in_status(int dev)
{
  __com_struct *ap = &asydev[dev];

  if (!ap->active) return(0);

  return(!(ap->in_buffer_in == ap->in_buffer_out));
}

/*----------------------------------------------------------------------*/
/* Status Eingabe                                                       */
/*----------------------------------------------------------------------*/
int rs232_out_status(int dev)
{
  __com_struct *ap = &asydev[dev];

  if (!ap->active) return(0);

/*#ifndef no_db7kg_fast_and_dirty_hack_for_db0fd
  if (ap->out_buffer_in == ap->out_buffer_out)
    ap->out_aktiv = FALSE;
#endif*/
  return(ap->out_aktiv);
}

void init_rs232(void)
{
  int i;

  for (i = 0; i < MAXCOMS; i++)
    asydev[i].active = FALSE;
}

void exit_rs232(void)
{
  int i;

  for (i = 0; i < MAXCOMS; i++)
    if (asydev[i].active)
      close_rs232(i);
}

/*----------------------------------------------------------------------*/

/* End of os/go32/16550.c */
