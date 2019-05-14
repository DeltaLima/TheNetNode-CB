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
/* File include/l3.h (maintained by: ???)                               */
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

/* Layer 3 Mode Flags --------------------------------------------------*/

#define DG 1
#define VC 2
#define VC_FAR 4
#define OPTIONS_MASK (DG+VC+VC_FAR)

#define DIRTY           60001U

/* Layer 3 Nodes Options -----------------------------------------------*/

#define OPT_SSID_RANGE 0x100       /* SSID-Bereich anzeigen             */
#define OPT_ALIAS      0x200       /* ALIAS ausgeben                    */
#define OPT_VCTEST     0x400       /* VC Routentest                     */
#define OPT_DGTEST     0x800       /* DG Routentest (NRR)               */

#define ISCALLMASK      (1<<0)
#define ISIDENTMASK     (1<<1)
#define ISMINQUAL       (1<<2)
#define ISMAXQUAL       (1<<3)
#define ISNBRCALL       (1<<4)

/* NRR-Options                                                          */

#define LT_MASK   0x7F
#define ECHO_FLAG 0x80

/* INP Option and Framing Codes                                         */

#define INP_RIF         0xFF
#define INP_EOP         0x00

#define INP_ALIAS       0               /* Alias                        */
#define INP_IPA         1               /* IP-Adresse + Subnet          */

/* Kennung fuer IP-Frames ueber Netrom                                  */

#define L3TCPUDP 0
