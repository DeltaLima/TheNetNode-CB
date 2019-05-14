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
/* File include/l2.h (maintained by: DF6LN)                             */
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

#define DEF_N2     20                   /* Default: Retries               */
                                        /* (bei DAMA: DEF_N2 / 2)         */

#define L2_ALPHA1   7                   /* Faktor fuer steigenden L2RTT   */
#define L2_ALPHA2  15                   /* Faktor fuer fallenden L2RTT    */
#define L2_BETA     4                   /* Multiplikator fuer FRACK-Ber.  */

/**************************************************************************/
#define L2CALEN  6                      /* Laenge Call im Level 2         */
#define L2IDLEN  (L2CALEN + 1)          /* Laenge Call + SSID = ID        */
#define L2INUM   2                      /* Anzahl ID's im an/von-Feld     */
#define L2VNUM   8                      /* Anzahl ID's im via-Feld        */
#define L2ILEN   (L2INUM * L2IDLEN)     /* Laenge an/von-Feld             */
#define L2VLEN   (L2VNUM * L2IDLEN)     /* Laenge via-Feld                */
#define L2AFLEN  (L2ILEN + L2VLEN)      /* Laenge Level 2 Adressfeld      */
#define L2HLEN   (L2AFLEN + 2)          /*   10 * 7 =    70 Bytes Adresse */
                                        /*            +   1 Byte Control  */
                                        /*            +   1 Byte PID      */
                                        /*            -----               */
                                        /*               72               */
#define L2MILEN 256                     /* max. Laenge des Info-Feldes    */
#define L2MFLEN (L2MILEN+L2HLEN)        /* maximale Framelaenge L2        */
                                        /*               72 Header        */
                                        /*            + 256 Byte Info     */
                                        /*            -----               */
                                        /*              328 Bytes         */

/**************************************************************************/
                              /* "layer 2 state", (state, s.u.) :         */
#define L2SDSCED    0         /*    disconnected                          */
#define L2SLKSUP    1         /*    link setup                            */
#define L2SFRREJ    2         /*    frame reject                          */
#define L2SDSCRQ    3         /*    disconnect request                    */
#define L2SIXFER    4         /*    information transfer                  */
#define L2SRS       5         /*    REJ sent                              */
#define L2SWA       6         /*    waiting acknowledge                   */
#define L2SDBS      7         /*    device busy                           */
#define L2SRBS      8         /*    remote busy                           */
#define L2SBBS      9         /*    both busy                             */
#define L2SWADBS   10         /*    waiting ack and device busy           */
#define L2SWARBS   11         /*    waiting ack and remote busy           */
#define L2SWABBS   12         /*    waiting ack and both busy             */
#define L2SRSDBS   13         /*    REJ sent and device busy              */
#define L2SRSRBS   14         /*    REJ sent and remote busy              */
#define L2SRSBBS   15         /*    REJ sent and both busy                */
#define L2SHTH     16         /*    waiting l7 partner connect            */

                              /* "layer 2 message", Status vom Level 2 :  */
#define L2MNIX     0          /*    keine Nachricht                       */
#define L2MCONNT   1          /*    CONNECTED to                          */
#define L2MDISCF   2          /*    DISCONNECTED from                     */
#define L2MBUSYF   3          /*    BUSY from                             */
#define L2MFAILW   4          /*    LINK FAILURE with                     */
#define L2MLRESF   5          /*    LINK RESET from                       */
#define L2MLREST   6          /*    LINK RESET to                         */
#define L2MFRMRF   7          /*    FRAME REJECT from                     */
#define L2MFRMRT   8          /*    FRAME REJECT to                       */
#define L2MBUSYT   9          /*    BUSY to                               */

                              /* "layer 2 control", Frametypen :          */
                              /*                                          */
                              /*                       Command/   Poll/   */
                              /*    Typ       Gruppe   Response   Final   */
                              /* ---------------------------------------- */
#define L2CI       0x00       /*      I         I         C         P     */
#define L2CUI      0x03       /*     UI         U        C/R       P/F    */
#define L2CSABM    0x2F       /*   SABM         U         C         P     */
#define L2CDISC    0x43       /*   DISC         U         C         P     */
#define L2CUA      0x63       /*     UA         U         R         F     */
#define L2CDM      0x0F       /*     DM         U         R         F     */
#define L2CFRMR    0x87       /*   FRMR         U         R         F     */
#define L2CRR      0x01       /*     RR         S        C/R       P/F    */
#define L2CREJ     0x09       /*    REJ         S        C/R       P/F    */
#define L2CRNR     0x05       /*    RNR         S        C/R       P/F    */

#ifdef EAX25
#define L2CSABME   0x6F       /*   SABM         U         C         P     */
#endif
                              /* "layer 2 control", spezielle Bits :      */
#define L2CPF      0x10       /*   Poll/Final                             */
#define L2CCR      0x80       /*   Command/Response                       */
#define L2CH       0x80       /*   "has been repeated"                    */
#define L2CEOA     0x01       /*   End of Address                         */
#define L2CDAMA    0x20       /*   geloeschtes Bit = DAMA                 */

#ifdef EAX25
#define L2CEAX     0x40       /*   geloeschtes Bit = Extended AX.25 Frame */
#endif
                              /* "layer 2 control", Masken :              */
#define L2CNOIM    0x01       /*   "no I mask", kein I-Frame              */
#define L2CNOSM    0x02       /*   "no S mask", kein S-Frame              */
#define L2CNONRM   0x03       /*   "no N(R) mask", kein N/R-Frame         */

                              /* "layer 2 control", Protokollidentifier   */
#define L2CPID     0xF0       /* kein L3 Protokoll                        */
#define L2CFRAG    0x08       /* AX25 Fragmentierung                      */
#define L2CNETROM  0xCF       /* NET/ROM TheNet(Node) Interlink-Protokoll */
#define L2CTEXNET  0xC3       /* TexNet (Kompatibilitaetsgruende), wie CF */
#define L2CIP      0xCC       /* eingekapseltes IP-Frame                  */
#define L2CARP     0xCD       /* eingekapseltes ARP-Frame                 */
#define L2CFLEXNET 0xCE       /* FlexNet-Protokoll                        */

                              /* "layer 2 control", Flags (flag, s.u.) :  */
#define L2FDAMA1   0x0001     /*      benutzt fuer DAMA-Runden-Steuerung  */
#define L2FDAMA2   0x0002     /*      benutzt fuer DAMA-Runden-Steuerung  */
#define L2FCTEXT   0x0004     /*   1 = C-Text schon mal gesendet          */
#define L2FDACK    0x0008     /*   1 = Frame wartet auf Bestaetigung      */
#define L2FCMDEL   0x0010     /* Maxframe verzoegert vergroessern         */
#define L2FREPEAT  0x0020     /* Frames sollen wiederholt werden          */
#define L2FBUSY    0x0040     /*   Device busy (ich !)                    */
#define L2FDSLE    0x0080     /*   "disc if send list empty"              */
#define L2FDIMM    0x0100     /*   "disconnect immedialtly"               */

#define L2FACKHTH  0x0400     /*   ankommende HTH-Verbindung bestaetigen  */
#define L2FREJHTH  0x0800     /*   ankommende HTH-Verbindung ablehnen     */

                              /* im Framebufferkopf (l2fflag, s.u.) :     */
#define L2FT1ST    0x0001     /*   nach Aussendung ist T1 zu starten      */
#define L2FUS      0x0002     /*   Sendeframe ist U- oder S-Frame (nicht  */
                              /*   digipeatet)                            */

/* End of include/l2.h */
