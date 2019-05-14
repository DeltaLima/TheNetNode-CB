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
/* File include/host.h (maintained by: DF6LN)                           */
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

#define HBUFLEN     256       /* Anzahl Zeichen maximal Hosteingabebuffer */

                              /* "host mode response",                    */
                              /* Hostmodeframe-Antworttypen :             */
#define HMRINFO     0         /*   "info"                                 */
#define HMRSMSG     1         /*   "success message"                      */
#define HMRFMSG     2         /*   "failure message"                      */
#define HMRSTAT     3         /*   "status"                               */
#define HMRMONH     4         /*   "monitor header"                       */
#define HMRMONIH    5         /*   "monitor I frame header"               */
#define HMRMONI     6         /*   "monitor I frame"                      */
#define HMRCONI     7         /*   "connected I frame"                    */

                              /* "host mode state",                       */
                              /* Hostmode-Eingabezustaende :              */
#define HMSCHNL     0         /*   naechstes Eingabebyte ist Kanal        */
#define HMSCMD      1         /*   naechstes Eingabebyte ist Kommando     */
#define HMSLEN      2         /*   naechstes Eingabebyte ist Laenge       */
#define HMSINPUT    3         /*   naechste Eingabebytes sind Eingabetext */

                              /* Zpar-Flags :                             */
#define FZFLOW      0x01      /*   Flow ein                               */
#define FZXONOFF    0x02      /*   XON/XOFF ein                           */

                              /* ASCII-Kontrollzeichen :                  */
#define BELL        0x07      /*   Klingel                                */
#define BS          0x08      /*   Backspace                              */
#define TAB         0x09      /*   Tab                                    */
#define LF          0x0A      /*   Linefeed                               */
#define CR          0x0D      /*   Carriage Return                        */
#define CONTROLR    0x12      /*   DC2                                    */
#define CONTROLU    0x15      /*   NAK                                    */
#define CONTROLX    0x18      /*   CAN                                    */
#define DEL         0x7F      /*   Delete                                 */

                              /* Messagebuffer-Anwahl :                   */
#define MBINFO      0         /*   alle Info-Pakete                       */
#define MBSTATUS    1         /*   alle Statuspakete                      */
#define MBALL       2         /*   alle Pakete                            */

#define HMEALC      0         /* Fehlermeldungen fuer rsperr              */
#define HMEIPA      1
#define HMEICS      2
#define HMELIG      3
#define HMEFAO      4
#define HMEFNO      5
#define HMENWC      6
#define HMEPOR      7

/* End of include/host.h */
