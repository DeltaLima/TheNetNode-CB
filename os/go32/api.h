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
/* File include/api.h (maintained by: ???)                              */
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

/* basiert auf flexdrv.h von DK7WJ und DL8MBT                           */

typedef unsigned char   byte;
typedef signed char     i8;
typedef signed short    i16;
typedef unsigned short  u16;
typedef unsigned long   u32;

#define TNL1_VERSION 1

#define DRVR_VERSION 3

/* Interne Funktion (Treiber Initialisierung/Deinitialisierung) */

#define API_UNLINK        3 /* Aufforderung an den Treiber, sich von der
                               Treiber-Kette zu loesen */
#define API_CHECKVERSION  4 /* pruefen ob die Version uebereinstimmt */
#define API_CHECKINIT     6 /* pruefen ob sich der Treiber initialisieren darf */
#define API_NEXTCHAN     16 /* den naechsten freien Kanal holen */

/* API-Funktionen */

#define API_INITCHAN     17 /* einen Kanal initialisieren */
#define API_RXFRAME      19 /* das naechste Frame empfangen */
#define API_GETBUF       20 /* Speicher fuer das Frame holen ??? */
#define API_TXFRAME      21 /* das/die Frame(s) im Buffer senden */
#define API_TXCALIB      22 /* Calibrierung senden */
#define API_GETSTATE     23 /* Kanal-Status abfragen */
#define API_GETSCALE     24 /* Kanal-Skalierung abfragen */
#define API_GETIDENT     25 /* das Ident dieses Treiber abfragen */
#define API_ISACTIVE     26 /* ist dieser Kanal aktiv ? */
#define API_GETSTAT      27 /* Statistik dieses Ports lesen */

typedef struct dev_data {
    u16 api_code;
    u16 return1;
    u16 return2;
    u16 channel;
    u16 arg1;
    u16 arg2;
    u16 arg3;
    u16 arg4;
} dev_data;

#define MAXFRAMELEN 400     /* Maximale Laenge eines Frames */

/* Frameaufbau */
typedef struct
{
    i16  len;               /* Laenge des Frames */
    byte port;              /* Kanalnummer */
    byte txdelay;           /* RX: Gemessenes TxDelay [*10ms],
                                   0 wenn nicht unterstuetzt
                               TX: Zu sendendes TxDelay (optional) */
    byte data[MAXFRAMELEN]; /* L1-Frame (ohne CRC) */
} L1FRAME;

/* Struct fuer Kanalstatistik (wird noch erweitert) */
typedef struct
{
    u32 tx_error;           /* Underrun oder anderes Problem */
    u32 rx_overrun;         /* Wenn Hardware das unterstuetzt */
    u32 rx_bufferoverflow;
    u32 tx_frames;          /* Gesamt gesendete Frames */
    u32 rx_frames;          /* Gesamt empfangene Frames */
    u32 io_error;
    u32 reserve[4];         /* fuer Erweiterungen, erstmal 0 lassen! */
} L1STATISTICS;

/* Masken fuer den L1-Kanalstatus */
#define CH_RXB 0x40
#define CH_PTT 0x20
#define CH_DCD 0x10
#define CH_FDX 0x08
#define CH_TBY 0x04

int l1_init_port(unsigned port, unsigned baud, unsigned mode);
void far *l1_rx_frame(void);
L1FRAME far *l1_get_txbuf(unsigned port);
void l1_kick(void);
void l1_calibrate(unsigned port, unsigned minutes);
unsigned l1_state(unsigned port);
unsigned l1_scale(unsigned port);
char far *l1_ident(unsigned port);
int l1_active(unsigned port);
L1STATISTICS far *l1_stat(unsigned port, int del);
int find_devirq(void);
int l1_enum_ports(void);
/* not yet implemented in my api */
char far * far l1_version(byte kanal);
void set_led(byte kanal, byte ledcode);
#define LED_CON 2
#define LED_STA 4

/* End of $RCSfile$ */
