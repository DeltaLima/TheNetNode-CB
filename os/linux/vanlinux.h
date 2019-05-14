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
/* File os/linux/vanlinux.h (maintained by: DG1KWA)                     */
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

#define MAXLFRAME       1024+70         /* ADR + Data           */
#define DP_BLOCK        4096            /* CRAM SIZE            */
#define CRAM_VALID      0x00            /* FLAG dp_xFProd       */
#define LED_OFF         1               /* Vanessa LED          */
#define LED_ON          0               /* vanessa LED          */

#define ID_CMD          0x0600          /* new ID Calls         */

/* Data type definitions --------------------------------------- */

#define dp_iFProd       (0)
#define dp_iLFrm        (dp_iFProd+2)
#define dp_iDCD         (dp_iLFrm+2)
#define dp_iPTT         (dp_iDCD+1)
#define dp_iBuffer      (dp_iPTT+1)
#define dp_iBufEmpty    (dp_iBuffer+MAXLFRAME)
#define dp_oFProd       (dp_iBufEmpty+2)
#define dp_oLFrm        (dp_oFProd+2)
#define dp_oCtl         (dp_oLFrm+2)
#define dp_oBuffer      (dp_oCtl+2)
#define dp_oMagic       (dp_oBuffer+MAXLFRAME)
#define dp_reReadParams (dp_oMagic+2)
#define dp_Slottime     (dp_reReadParams+2)
#define dp_TxDelay      (dp_Slottime+2)
#define dp_vanspeed     (dp_TxDelay+2)
#define dp_Persistance  (dp_vanspeed+2)
#define dp_FullDuplex   (dp_Persistance+2)
#define dp_cCMD         (dp_FullDuplex+2)
#define dp_cCNT         (dp_cCMD+2)
#define dp_cADR         (dp_cCNT+2)
#define dp_cData        (dp_cADR+2)

/* VANESSA specific adresses                                            */

WORD ledio_adr[] = {0x306, 0x307, 0x30E, 0x30F, 0x316, 0x317, 0x31e, 0x31f,
                    0x126, 0x127, 0x12E, 0x12F, 0x136, 0x137, 0x13E, 0x13F};
WORD reset_adr[] = {0x300, 0x308, 0x310, 0x318, 0x120, 0x128, 0x130, 0x138};


BOOLEAN van_test(int);

/* End of os/linux/vanlinux.h */
