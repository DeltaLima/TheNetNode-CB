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
/* File os/go32/kiss.c (maintained by: ???)                             */
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

#define SMACK 1
#define RMNC  2

#if defined(__GO32__)
#define KI_BUFFERSIZE      8192         /* bei DPMI kann es mehr sein   */
#else
#define KI_BUFFERSIZE      700          /* Groesse eines Buffers        */
#endif

#include "kiss.h"

__kiss_struct kissdev[MAXKISS];
static int kiss_major = 0;

/************************************************************************/
/* Level1 Init                                                          */
/************************************************************************/
static BOOLEAN kiss_init(void)
{
  int kiss;
  char str[20];
#ifdef PC
  int dev,adr,irq;
#endif
#ifdef ST
  char device[16];
#endif
  int  bd;
  __kiss_struct *kp;
  int  numactive = 0;

  for (kiss = 0; kiss < MAXKISS; kiss++)
  {
    kp = &kissdev[kiss];
    sprintf(str, "KISS%u", kiss+1);
#ifdef PC
    read_envcom(str, &dev, &adr, &irq);
    if (dev != -1 && blkbuf) {
      kp->dev = open_rs232(dev,adr,irq,KI_BUFFERSIZE,KI_BUFFERSIZE);
      kp->active = (kp->dev != -1);
    } else kp->active = FALSE;
#endif /* PC */
#ifdef ST
    read_envdev(str, device, "");
    kp->active = ((*device && (kp->dev = open_rs232(device)) > 0));
#endif /* ST */
    if (kp->active) {
      bd = setbaud(kp->dev, 96);
      setstopbits(kp->dev, 1);
      printf(" setting to %u00 8N1.\n", bd);
    }
    kp->kisslinkport = -1;
    kp->rx_state = 0;
    kp->rxfhd = NULL;
    if (kp->active)
      numactive++;
  }
  return(numactive);
}

/************************************************************************/
/* Level1 Exit                                                          */
/************************************************************************/
static void kiss_exit(void)
{
  int kiss;

  for (kiss = 0; kiss < MAXKISS; kiss++)
    if (kissdev[kiss].active) close_rs232(kissdev[kiss].dev);
}

/*                        Berechnung eines Teil-CRC 16 nach SMACK Methode */
/**************************************************************************/
static UBYTE smack_crc(UWORD *crc, UBYTE c)
{
  static const UWORD crc_table[] = {
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

  *crc = (*crc >> 8) ^ crc_table[(*crc ^ c) & 0xFF];
  return(c);
}

/*                         Berechnung eines Teil-CRC 16 nach RMNC Methode */
/**************************************************************************/
static UBYTE rmnc_crc(UWORD *crc, UBYTE c)
{
  static const UWORD Crc_rmnc_table[] = {
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

  *crc = (*crc << 8) ^ Crc_rmnc_table[((*crc >> 8) ^ c) & 0xff];
  return(c);
}

/* dies ist eine Inline-Code-Sequence, die benutzt wird, um ein Zeichen */
/* Slip-kodiert auszugeben. Da es ein Macro ist, darf ch nur einmal     */
/* abgefragt werden. tmp ist eine Registervariable und speichert ch     */
/* fuer eine 2. Abfrage zwischen.                                       */
/* Dies wuerde sauberer mit einem inline tag gehen, aber Borland kennt  */
/* soetwas nicht.                                                       */
#define slip_encode(ch) switch ((tmp=ch)&0xFF) { \
    case FEND: TX_CHAR(FESC);                    \
               TX_CHAR(TFEND);                   \
               break;                            \
    case FESC: TX_CHAR(FESC);                    \
               TX_CHAR(TFESC);                   \
               break;                            \
    default:   TX_CHAR(tmp);                    }

/************************************************************************/
/* kisslink_put_frame()  -  Frame(s) im Ringpuffer ablegen              */
/*----------------------------------------------------------------------*/
static void kisslink_put_frame(__kiss_struct *kp)
{
  UWORD    crc;
  LHEAD   *l2flp;
  MBHEAD  *txfhdl;
  UBYTE   *out;
  UBYTE    register tmp;

  l2flp = &txl2fl[kp->kisslinkport];

  /* Nur senden wenn letztes Frame raus ist */

  if (!rs232_out_status(kp->dev))
  {
    portpar[kp->kisslinkport].reset_port =
    commandflag[kp->kisslinkport] =
    testflag[kp->kisslinkport] = FALSE;

    if (kick[kp->kisslinkport]) {
      ulink((LEHEAD *)(txfhdl = (MBHEAD *) l2flp->head));/*Zeiger holen*/
      TX_BEG();                      /* Sender bereit */
      TX_CHAR(FEND);                 /* neues Frame   */
      /* das Frame nach der SMACK-Methode einpacken und senden */
      if (kp->use_crc == SMACK) {
        crc = 0;
        TX_CHAR(smack_crc(&crc, 0x80));
        while (txfhdl->mbgc < txfhdl->mbpc) /* solange Daten vorhanden sind */
          slip_encode(smack_crc(&crc, getchr(txfhdl)));
        slip_encode(crc);
        slip_encode(crc >> 8);
      } else
      /* das Frame nach der RMNC-Methode einpacken und senden */
      if (kp->use_crc == RMNC) {
        crc = 0xFFFF;
        TX_CHAR(rmnc_crc(&crc, 0x20));
        while (txfhdl->mbgc < txfhdl->mbpc) /* solange Daten vorhanden sind */
          slip_encode(rmnc_crc(&crc, getchr(txfhdl)));
        slip_encode(crc >> 8);
        slip_encode(crc);
      } else {
      /* das Frame ohne CRC einpacken und senden */
        TX_CHAR(0x00);
        while (txfhdl->mbgc < txfhdl->mbpc) /* solange Daten vorhanden sind */
          slip_encode(getchr(txfhdl));
      }
      TX_CHAR(FEND);                       /* Frame bendet */
      rs232_write(kp->dev, blkbuf, out);   /* und an die RS232 damit */

      relink((LEHEAD *)txfhdl,         /* als gesendet betrachten und in */
            (LEHEAD *)stfl.tail);      /* die gesendet Liste umhaengen    */

      kick[kp->kisslinkport] = ((LHEAD *)l2flp->head != l2flp);
    }
  }
}

/************************************************************************/
/* kisslink_get_frame() - Frame(s) aus Ringbuffer holen                 */
/*----------------------------------------------------------------------*/
static void kisslink_get_frame(__kiss_struct *kp)
{
  UBYTE          ch, *in = blkbuf;
  int            state;
  int            n;
  int            use_crc;
  UWORD          rx_crc;
  MBHEAD        *rxfhd;

  /* mit if() abzubrechen ist quatsch, while (n) ist auch nurn if()    */
  /* ich hab mir angeschaut was mein compiler macht, keine Vorteile    */

  if ((n = rs232_read(kp->dev, blkbuf, BLOCKSIZE))==0) return;

  /* das war allerdings bevor ich state,rxfhd,rx_crc und use_crc rein- */
  /* genommen habe ... sri odo                                         */

  state   = kp->rx_state;
  rxfhd   = kp->rxfhd;
  rx_crc  = kp->crc;
  use_crc = kp->use_crc;

  while (n)                            /* Zeichen auswerten                */
  {
    ch = *in++;                        /* Zeichen holen                    */
    n--;
    switch (state)                     /* ueber Status verzweigen          */
    {
      case WFEND:                      /* Frame Anfang erwartet            */
          if (ch == FEND)
            state = GTYPE;             /* Frameanfang entdeckt             */
          continue;

      case GTYPE:
          if (ch != FEND)              /* zuletzt FEND bekommen           */
          {
            if ((ch & 0x0F) > 0) {
              state = WFEND;           /* Kommandos                       */
              continue;
            }
            state = GFRAM;             /* Daten                           */
            if (rxfhd)                 /* wenn Frame aktiv, auf den Muell */
              relink((LEHEAD *)rxfhd, (LEHEAD *)trfl.tail);
            (rxfhd = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2port = kp->kisslinkport;

            switch (use_crc = kp->use_crc) {
              case SMACK :
                rx_crc = 0;
                smack_crc(&rx_crc, 0x80);
                break;
              case RMNC :
                rx_crc = 0xFFFF;
                rmnc_crc(&rx_crc, 0x20);
                break;
            }
          }
          continue;

      case GFRAM:                      /* <daten> vom TNC holen           */
          switch (ch)
          {
            case FEND:
              state = WFEND;           /* Frame-Ende = Anfang fuer nextes */
              if (rxfhd) {             /* nur wenn ein Frame aktiv        */
                switch (use_crc) {
                  case SMACK :
                    if (rxfhd->mbpc > 2)
                      rxfhd->mbpc -= 2;
                    if (rx_crc) {
                      kp->crcerrors++;
                      continue;        /* wirklich Fehler                 */
                    }
                    relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);
                    rxfhd = NULL;      /* kein RX-Frame aktiv             */
                    break;
                  case RMNC  :
                    if (rxfhd->mbpc > 2)
                      rxfhd->mbpc -= 2;
                    if ((rx_crc & 0xffff) != 0x7070) {
                      kp->crcerrors++;
                      continue;        /* wirklich Fehler                 */
                    }
                  default:
                    relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);
                    rxfhd = NULL;      /* kein RX-Frame aktiv             */
                }
              }
              continue;                 /* Das war's.. fertig!            */

            case FESC:
              state++;                  /* FESC wird zu FESC-TFESC        */
              continue;                 /* Rueckwandlung..                */
          }
          break;

      case GFRMT:
          switch (ch)                   /* TFEND/TFESC-Rueckwandlung    */
          {
            case TFEND:
              ch = FEND;
              break;

            case TFESC:
              ch = FESC;
              break;

            default:
              kp->sliperrors++;
              state = WFEND;
              continue;
          }
          state--;
          break;

    } /* switch(state) */

    switch (use_crc) {
        case SMACK : smack_crc(&rx_crc, ch);
                     break;
        case RMNC  : rmnc_crc(&rx_crc, ch);
                     break;
    }
    putchr(ch, rxfhd);                /* Zeichen in Frame               */
    if (rxfhd->mbpc > L2MFLEN)        /* Framelaengencheck               */
      state = WFEND;
  } /* solange Zeichen in der RS232 */

  kp->rx_state = state;
  kp->rxfhd    = rxfhd;
  kp->crc      = rx_crc;
}

/************************************************************************/
/* Level1 RX/TX  fuer KISSLINK                                          */
/*               wird staendig in der main() Hauptschleife aufgerufen.  */
/************************************************************************/
static void kisslink(void)
{
  int kiss;
  __kiss_struct *kp;

  for (kiss = 0, kp = kissdev; kiss < MAXKISS; kiss++, kp++)
  {
    if ((kp->kisslinkport != -1) && kp->active)
    {
      kisslink_get_frame(kp);
      kisslink_put_frame(kp);
    }
  }
}

static WORD kiss_dcd(PORTINFO *port)
{
  __kiss_struct *kp;
  int state = 0;

  kp = &kissdev[port->minor];

  if (rs232_out_status(kp->dev))
    state |= PTTFLAG;
  if (kp->rx_state > 1)
    state |= DCDFLAG;
  return(state);
}

static void kiss_ctl(int req, int port)
{
  __kiss_struct *kp;
  int            minor;

  minor = portpar[port].minor;

  kp = &kissdev[minor];

  switch (req) {
    case L1CCMD :
    case L1CRES :
      portpar[kp->kisslinkport].speed =
      setbaud(kp->dev, portpar[kp->kisslinkport].speed);
      /* macht setbaud automatisch */
#ifndef MC68K
      setstopbits(kp->dev,1);
#else   /* statttdessen crc-mode vorbereiten */
      setkissmode(port);
#endif
      break;
  }
  default_l1ctl(req, port); /* Flags loeschen */
};

#define MINOR_SMACK_FLAG (SMACK << 8)
#define MINOR_RMNC_FLAG  (RMNC << 8)
#define MINOR_MASK       (0xFF)

static int kiss_istome(int major, char *devname)
{
  char   name[21], *cp;
  int    minor = 0;

  strncpy(name, devname, 20); /* Minor bestimmen und abschneiden */
  name[20] = 0;
  for (cp = name; isalpha(*cp); cp++); /* Zahl suchen */
  if (isdigit(*cp)) minor = atoi(cp);
  *cp = 0; /* kann nicht schaden */

  if (minor < 1 || minor > MAXKISS)
    return(NO_MINOR); /* falscher Geraetekanal (minor) */

  minor--;

  if (strnicmp(name, "KISS", strlen(name))==0)
    return(minor);
  else if (strnicmp(name, "SMACK", strlen(name))==0)
    return(minor|MINOR_SMACK_FLAG);
  else if (strnicmp(name, "RKISS", strlen(name))==0)
    return(minor|MINOR_RMNC_FLAG);

  return(NO_MINOR);
}

static int kiss_attach(int port, int minor, BOOLEAN check_only)
{
  __kiss_struct *kp;
  int            use_crc;

  use_crc  = (minor >> 8);
  minor   &= MINOR_MASK;

  kp = &kissdev[minor];

  if (kp->active) { /* Geraetekanal bereit? */
    if (kp->kisslinkport == -1) {
      if (!check_only) {
        kp->kisslinkport = port;
        kp->use_crc = use_crc;
        portpar[port].minor = minor;
        clear_rs232(minor);
      }
      return(1);
    }
    if (kp->kisslinkport == port)
      return(1);
  }
  return(0); /* versuchte Doppeleintragung */
}

static int kiss_detach(int port)
{
  int kiss = portpar[port].minor;

  kissdev[kiss].kisslinkport = -1;
  return(1);
}

static void kiss_info(int what, int port, MBHEAD *mbp)
{
  char *name[] = {"KISS", "SMACK", "RKISS"}; /* SMACK=1, RMNC=2 */
  int   minor, cnt;
  __kiss_struct *kp;

  minor = portpar[port].minor;

  switch (what) {
    case HW_INF_IDENT :
    case HW_INF_INFO :
      putprintf(mbp, "%s%u", name[kissdev[minor].use_crc], minor+1);
      break;
    case HW_INF_STAT :
      for (minor = cnt = 0, kp = kissdev; minor < MAXKISS; minor++, kp++)
        if (kp->kisslinkport != -1) {
          if (cnt++ == 0)
            putstr("\rKISS-Statistics:\r\r", mbp);
          putprintf(mbp, "  KISS%u      RxCRC: %5u  RxErr: %5u\r",
                         minor+1, kp->crcerrors, kp->sliperrors);
        }
      break;
    case HW_INF_CLEAR :

      /* durchfallen */
    default:
      default_l1info(what, port, mbp);
  }
}

static int register_kiss(void)
{
  MAJOR *m;

  if (kiss_init()) {
    m = register_major();
    m->name    = "KISS/SMACK/RKISS";
    m->istome  = kiss_istome;
    m->exit    = kiss_exit;
    m->handle  = kisslink;
    m->ctl     = kiss_ctl;
    m->dcd     = kiss_dcd;
    m->attach  = kiss_attach;
    m->detach  = kiss_detach;
    m->info    = kiss_info;
    return(kiss_major = num_major);
  }
  return(0);
}

/*----------------------------------------------------------------------*/

/* End of os/go32/kiss.c */
