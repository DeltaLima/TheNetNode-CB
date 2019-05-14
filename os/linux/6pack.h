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
/* File os/linux/6pack.h (maintained by: DG9OBU)                        */
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

#ifndef __SIXPACK_H__
#define __SIXPACK_H__

#define MAX_TNC         8   /* Maximal kann es acht TNC im Ring geben */

/* Zustaende des Rings */
#define RING_HALT       0   /* Ring nicht nutzbar */
#define RING_IDLE       1   /* Ring offen, aber noch nicht initialisiert */
#define RING_INIT       2   /* TNC-Zaehlung, warten auf Empfang des Zaehlpaketes */
#define RING_RUN        3   /* Ring laeuft */

/* Zustaende des RX */
#define RX_WAIT_SOF     1   /* Warten auf einleitendes SOF */
#define RX_WAIT_END     2   /* Warten auf abschliessendes SOF */

/* 6PACK-Kommandocodes */

#define CMD_SOF     0x40    /* Start/End of frame       (PC <> TNC) */
#define CMD_LED     0x60    /* Grundmaske fuer LED-Kommando */
#define CMD_CON     0x08    /* LED CON                  (PC -> TNC) */
#define CMD_STA     0x10    /* LED STA                  (PC -> TNC) */
#define CMD_CAL     0xE0    /* Calibrate                (PC <> TNC) */
#define CMD_INIT    0xE8    /* Kanalzuweisung           (PC <> TNC) */
#define CMD_PRIO    0x80    /* Prioritaetsmeldung       (PC <> TNC) */

/* Meldungen */
#define MSG_TXU     0x48    /* TX-Underrun              (TNC -> PC) */
#define MSG_RXO     0x50    /* RX-Overrun               (TNC -> PC) */
#define MSG_RXB     0x58    /* RX Buffer overflow       (TNC -> PC) */

/* Bitmasken fuer Aufbau und Auswertung der Prioritaetsmeldung */
#define MASK_TXC    0x20    /* "TX-Zaehler + 1" */
#define MASK_RXC    0x10    /* "RX-Zaehler + 1" */
#define MASK_DCD    0x08    /* "DCD" */

/* Hilfsmasken */
#define MASK_DATA       0xC0
#define MASK_CMD        0xF8
#define MASK_TNCNUM     0x07
#define MASK_PRIOCMD    0x80
#define MASK_NORMCMD    0x40
#define MASK_CALCNT     0x40
#define MASK_CHAN       0x08

#define LED_CON         0x08
#define LED_STA         0x10

#define TXWDTIME        45      /* 450ms Watchdog beim Senden */
#define PROBE_TIMEOUT   1000    /* 10 Sekunden (= 1000 x 10ms) warten auf Zaehlpaket */
#define TNC_TIMEOUT     500     /* 5 Sekunden Timeout fuer TNC */
#define TNC_WATCHDOG    250     /* 2,5 Sekunden Watchdog bei Empfang */

/* Variablen und Strukturen */
typedef struct tnc {
  /* Kanalsteuerung */
  BOOLEAN       bPresent;       /* Gibt es diesen TNC im Ring ? */
  ULONG         tLastHeard;     /* Zeitpunkt des letzten Lebenszeichen */
  ULONG         tNextArbit;     /* Zeitpunkt der naechsten Kanalarbitrierung */
  ULONG         tWatchdog;      /* normaler Watchdog */
  ULONG         tTXWatchdog;    /* Watchdog waehrend TX und Calibrate */
  ULONG         tPTTWatchdog;   /* Watchdog fuer PTT-Kontrolle */
  UBYTE         cLED;           /* LEDs am TNC */
  BOOLEAN       bDCD;           /* DCD-Status */
  UBYTE         cTXC;           /* TX-Zaehler */
  UBYTE         cRXC;           /* RX-Zaehler */
  UBYTE         cTestCount;     /* Anzahl der zu sendenden Calibrates a 15 Sekunden */
  /* Statistik */
  UWORD         uTXU;           /* Fehlerzaehler TX-Underrun */
  UWORD         uRXO;           /* Fehlerzaehler RX-Overrun */
  UWORD         uRXB;           /* Fehlerzaehler RX Buffer Overflow */
  UWORD         uReset;         /* Anzahl der TNC-Resets */
  UWORD         uChecksumErr;   /* Anzahl Checksummenfehler */
} TNC;

/* Puffer fuer empfangene, rohe 6PACK-Daten */
static UBYTE cRXRawBuffer[1024];
static UWORD  uRXRawBufferSize = 0;

/* Puffer fuer dekodierte 6PACK-Frames */
static UBYTE cRXBuffer[1024];
static UWORD  uRXBufferSize = 0;

static TNC RING[(UBYTE)MAX_TNC];

#ifndef ATTACH
int iDescriptor = -1;
#endif /* ATTACH */

fd_set rset;
static struct timeval tv;

static UWORD uLogLevel = 0;

static UWORD uMap_TNCtoPort[MAX_TNC];   /* TNC  -> Port */
static UBYTE uMap_PorttoTNC[L2PNUM];    /* Port -> TNC  */

static ULONG tLastInit = 0;
static UWORD uNumProbes = 0;

UBYTE uRingStatus  = RING_HALT;
UBYTE uRXStatus    = RX_WAIT_SOF;
UBYTE cReceiveFrom = 0xFF;

/* oeffentliche Funktionen */
void Sixpack_Housekeeping(void);
void Sixpack_l1init(void);
void Sixpack_l1exit(void);
WORD Sixpack_DCD(UWORD);
void ccp6pack(void);

/* interne Funktionen */
void Sixpack_RX(void);
void Sixpack_TX(void);

void startCount(void);
void toRing(UBYTE *, UWORD);

void initTNC(UBYTE);

/* Einfache inline-Funktionen fuer die wir (hoffentlich) */
/* keine Debugsymbole brauchen ...                       */

/************************************************************************/
/* Erzeugt das 6PACK LED-Kommando                                       */
/************************************************************************/
UBYTE cmdLED(UBYTE uTNC) { return (CMD_LED | RING[uTNC].cLED | (uTNC & MASK_TNCNUM)); }

/************************************************************************/
/* Erzeugt das 6PACK Calibrate-Kommando                                 */
/************************************************************************/
UBYTE cmdCAL(UBYTE uTNC) { return (CMD_CAL | (uTNC & MASK_TNCNUM)); }

/************************************************************************/
/* Erzeugt das 6PACK Kanalzuweisungs-Kommando                           */
/************************************************************************/
UBYTE cmdINIT(UBYTE uTNC) { return (CMD_INIT | (uTNC & MASK_TNCNUM)); }

/************************************************************************/
/* Erzeugt das 6PACK "TX-Zaehler + 1"-Kommando                          */
/************************************************************************/
UBYTE cmdTXC(UBYTE uTNC) { return (CMD_PRIO | MASK_TXC | (uTNC & MASK_TNCNUM)); }

/************************************************************************/
/* Erzeugt das 6PACK "Start of frame"-Kommando                          */
/************************************************************************/
UBYTE cmdSOF(UBYTE uTNC) { return (CMD_SOF | (uTNC & MASK_TNCNUM)); }

/************************************************************************/
/* Setzt die angegebene LED in der TNC-Struktur                         */
/************************************************************************/
void setLED(UBYTE uTNC, UBYTE cLED) { if (uTNC < MAX_TNC) RING[uTNC].cLED |= cLED; }

/************************************************************************/
/* Loescht die angegebene LED in der TNC-Struktur                       */
/************************************************************************/
void resetLED(UBYTE uTNC, UBYTE cLED) { if (uTNC < MAX_TNC) RING[uTNC].cLED &= !cLED; }

UBYTE assignPorts(UBYTE);

BOOLEAN DataToBuffer(UBYTE);
BOOLEAN DecodeRawBuffer(UBYTE);

void SixpackLog(const char *, ...);

#define LOGL1 if(uLogLevel>0)(void)SixpackLog
#define LOGL2 if(uLogLevel>1)(void)SixpackLog
#define LOGL3 if(uLogLevel>2)(void)SixpackLog
#define LOGL4 if(uLogLevel>3)(void)SixpackLog

#endif /* __SIXPACK_H__ */

/* End of os/linux/6pack.h */
