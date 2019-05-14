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
/* File include/l4.h (maintained by: ???)                               */
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

/* Layer 4 state -------------------------------------------------------*/

#define L4SDSCED        0       /* disconnected                         */
#define L4SLKSUP        1       /* link setup                           */
#define L4SIXFER        2       /* information transfer                 */
#define L4SDSCRQ        3       /* disconnect request                   */


/* Layer 4 Opcodes -----------------------------------------------------*/

#define L4CONREQ 0x01                   /* L4 Connect-Request           */
#define L4CONACK 0x02                   /* L4 Connect-Acknowledge       */
#define L4DISREQ 0x03                   /* L4 Disconnect-Request        */
#define L4DISACK 0x04                   /* L4 Disconnect-Acknowledge    */
#define L4INFTRA 0x05                   /* L4 Information-Transfer      */
#define L4INFACK 0x06                   /* L4 Information-Acknowldge    */

#ifdef NEW_L4
#define L4PIDCHG 0x07                   /* L4 PID-Change                */
#endif

/* L4OPMASK: Maske fuer Layer 4 Opcodes --------------------------------*/

#define L4OPMASK 0x0F   /* 0x07 .. Bit 3 ist bisher immer 0! */

/* Layer 4 Control-Bits ------------------------------------------------*/

#define L4CMORE         0x20            /* L4 More-Follows-Flag         */
#define L4CNAK          0x40            /* L4 No Acknowledge-Flag       */
#define L4CCHOKE        0x80            /* L4 Choke-Flag                */

/* Layer 4 Control-Flags -----------------------------------------------*/

#define L4FPBUSY        0x20            /* Device busy (Partner)        */
#define L4FBUSY         0x40            /* Device busy (selbst!)        */
#define L4FDSLE         0x80            /* "disc if send list empty"    */
#define L4FDIMM         0x10            /* "disconnect immidiatly"      */

/* Layer 4 Message, Status von Layer 4 ---------------------------------*/

#define L4MCONNT        L2MCONNT        /* CONNECTED to                 */
#define L4MDISCF        L2MDISCF        /* DISCONNECTED from            */
#define L4MBUSYF        L2MBUSYF        /* BUSY from                    */
#define L4MFAILW        L2MFAILW        /* LINK FAILURE with            */

/* End of include/l4.h */
