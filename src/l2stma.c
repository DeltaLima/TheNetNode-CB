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
/* File src/l2stma.c (maintained by: DF6LN)                             */
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

#define NUMSTATS 17           /* Anzahl States AX.25 Level 2            */
                              /* + HTH-Wartezustand (nicht AX.25)       */

/*
 *    0 L2SDSCED  -  disconnected
 *    1 L2SLKSUP  -  link setup
 *    2 L2SFRREJ  -  frame reject
 *    3 L2DSCRQ   -  disconnect request
 *    4 L2SIXFER  -  information transfer
 *    5 L2SRS     -  REJ sent
 *    6 L2SWA     -  waiting acknowledge
 *    7 L2SDBS    -  device busy
 *    8 L2SRBS    -  remote busy
 *    9 L2SBBS    -  both busy
 *   10 L2SWADBS  -  waiting ack and device busy
 *   11 L2SWARBS  -  waiting ack and remote busy
 *   12 L2SWABBS  -  waiting ack and both busy
 *   13 L2SRSDBS  -  REJ sent and device busy
 *   14 L2SRSRBS  -  REJ sent and remote busy
 *   15 L2SRSBBS  -  REJ sent and both busy
 *   kein AX.25-State, Wartezustaende fuer AutoDigi:
 *   16 L2SHTH    -  HTH waiting
 */

/* I WITH POLL (COMMAND) */

STENTRY stbl00[NUMSTATS] =
{
  { xdm,   L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xfrmr, L2SFRREJ, L2MNIX   },
  { xdm,   L2SDSCED, L2MDISCF }, /* war L2SDSCRQ, 05.02.98, DB7KG */
  { xrrr,  L2SIXFER, L2MNIX   },
  { xrrr,  L2SIXFER, L2MNIX   },
  { xrrr,  L2SWA   , L2MNIX   },
  { xrnrr, L2SDBS  , L2MNIX   },
  { xrrr,  L2SRBS  , L2MNIX   },
  { xrnrr, L2SBBS  , L2MNIX   },
  { xrnrr, L2SWADBS, L2MNIX   },
  { xrrr,  L2SWARBS, L2MNIX   },
  { xrnrr, L2SWABBS, L2MNIX   },
  { xrnrr, L2SRSDBS, L2MNIX   },
  { xrrr,  L2SRBS  , L2MNIX   },
  { xrnrr, L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }

};

/* I WITH POLL (COMMAND), DAMA-Mode */

/*
 *  Aenderung gegenueber stbl00:
 *
 *  Wenn der Link-Zustand "Warten auf Final" ist, dann genuegt ein Info-
 *  Frame als Antwort auf einen Poll, um den Link-Zustand wieder auf
 *  "Info-Transfer" zu setzen.
 */

STENTRY stbl00dama[NUMSTATS] =
{
  { xdm,   L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xfrmr, L2SFRREJ, L2MNIX   },
  { xdm,   L2SDSCED, L2MDISCF }, /* war L2SDSCRQ, 05.02.98, DB7KG */
  { xrrr,  L2SIXFER, L2MNIX   },
  { xrrr,  L2SIXFER, L2MNIX   },
  { xrrr,  L2SIXFER, L2MNIX   },
  { xrnrr, L2SDBS  , L2MNIX   },
  { xrrr,  L2SRBS  , L2MNIX   },
  { xrnrr, L2SBBS  , L2MNIX   },
  { xrnrr, L2SWADBS, L2MNIX   },
  { xrrr,  L2SWARBS, L2MNIX   },
  { xrnrr, L2SWABBS, L2MNIX   },
  { xrnrr, L2SRSDBS, L2MNIX   },
  { xrrr,  L2SRBS  , L2MNIX   },
  { xrnrr, L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* I WITHOUT POLL (COMMAND) */

STENTRY stbl01[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { t2rrr, L2SIXFER, L2MNIX   },
  { t2rrr, L2SIXFER, L2MNIX   },
  { t2rrr, L2SWA   , L2MNIX   },
  { t2rnrr,L2SDBS  , L2MNIX   },
  { t2rrr, L2SRBS  , L2MNIX   },
  { t2rnrr,L2SBBS  , L2MNIX   },
  { t2rnrr,L2SWADBS, L2MNIX   },
  { t2rrr, L2SWARBS, L2MNIX   },
  { t2rnrr,L2SWABBS, L2MNIX   },
  { t2rnrr,L2SRSDBS, L2MNIX   },
  { t2rrr, L2SRBS  , L2MNIX   },
  { t2rnrr,L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* I WITHOUT POLL (COMMAND), DAMA-Mode */

/*
 *  Aenderung gegenueber stbl01:
 *
 *  Wenn der Link-Zustand "Warten auf Final" ist, dann genuegt ein Info-
 *  Frame als Antwort auf einen Poll, um den Link-Zustand wieder auf
 *  "Info-Transfer" zu setzen.
 */

STENTRY stbl01dama[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { t2rrr, L2SIXFER, L2MNIX   },
  { t2rrr, L2SIXFER, L2MNIX   },
  { t2rrr, L2SIXFER, L2MNIX   },
  { t2rnrr,L2SDBS  , L2MNIX   },
  { t2rrr, L2SRBS  , L2MNIX   },
  { t2rnrr,L2SBBS  , L2MNIX   },
  { t2rnrr,L2SWADBS, L2MNIX   },
  { t2rrr, L2SWARBS, L2MNIX   },
  { t2rnrr,L2SWABBS, L2MNIX   },
  { t2rnrr,L2SRSDBS, L2MNIX   },
  { t2rrr, L2SRBS  , L2MNIX   },
  { t2rnrr,L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* RR WITH POLL (COMMAND) */

STENTRY stbl02[NUMSTATS] =
{
  { xdm,   L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xfrmr, L2SFRREJ, L2MNIX   },
  { xdm,   L2SDSCED, L2MDISCF }, /* war L2SDSCRQ, 05.02.98, DB7KG */
  { xrrr,  L2SIXFER, L2MNIX   },
  { xrrr,  L2SRS   , L2MNIX   },
  { xrrr,  L2SWA   , L2MNIX   },
  { xrnrr, L2SDBS  , L2MNIX   },
  { xrrr,  L2SIXFER, L2MNIX   },
  { xrnrr, L2SDBS  , L2MNIX   },
  { xrnrr, L2SWADBS, L2MNIX   },
  { xrrr,  L2SWA   , L2MNIX   },
  { xrnrr, L2SWADBS, L2MNIX   },
  { xrnrr, L2SRSDBS, L2MNIX   },
  { xrrr,  L2SRS   , L2MNIX   },
  { xrnrr, L2SRSDBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* RR WITHOUT POLL (COMMAND) */

STENTRY stbl03[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { xnull, L2SIXFER, L2MNIX   },
  { xnull, L2SRS   , L2MNIX   },
  { xnull, L2SWA   , L2MNIX   },
  { xnull, L2SDBS  , L2MNIX   },
  { xnull, L2SIXFER, L2MNIX   },
  { xnull, L2SDBS  , L2MNIX   },
  { xnull, L2SWADBS, L2MNIX   },
  { xnull, L2SWA   , L2MNIX   },
  { xnull, L2SWADBS, L2MNIX   },
  { xnull, L2SRSDBS, L2MNIX   },
  { xnull, L2SRS   , L2MNIX   },
  { xnull, L2SRSDBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* RNR WITH POLL (COMMAND) */

STENTRY stbl06[NUMSTATS] =
{
  { xdm,   L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xfrmr, L2SFRREJ, L2MNIX   },
  { xdm,   L2SDSCED, L2MDISCF }, /* war L2SDSCRQ, 05.02.98, DB7KG */
  { xrrr,  L2SRBS  , L2MNIX   },
  { xrrr,  L2SRSRBS, L2MNIX   },
  { xrrr,  L2SWARBS, L2MNIX   },
  { xrnrr, L2SBBS  , L2MNIX   },
  { xrrr,  L2SRBS  , L2MNIX   },
  { xrnrr, L2SBBS  , L2MNIX   },
  { xrnrr, L2SWABBS, L2MNIX   },
  { xrrr,  L2SWARBS, L2MNIX   },
  { xrnrr, L2SWABBS, L2MNIX   },
  { xrnrr, L2SRSBBS, L2MNIX   },
  { xrrr,  L2SRSRBS, L2MNIX   },
  { xrnrr, L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* RNR WITHOUT POLL (COMMAND) */

STENTRY stbl07[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { xnull, L2SRBS  , L2MNIX   },
  { xnull, L2SRSRBS, L2MNIX   },
  { xnull, L2SWARBS, L2MNIX   },
  { xnull, L2SBBS  , L2MNIX   },
  { xnull, L2SRBS  , L2MNIX   },
  { xnull, L2SBBS  , L2MNIX   },
  { xnull, L2SWABBS, L2MNIX   },
  { xnull, L2SWARBS, L2MNIX   },
  { xnull, L2SWABBS, L2MNIX   },
  { xnull, L2SRSBBS, L2MNIX   },
  { xnull, L2SRSRBS, L2MNIX   },
  { xnull, L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* SABM EITHER (COMMAND) */

STENTRY stbl08[NUMSTATS] =
{
  { xua,   L2SIXFER, L2MCONNT },
  { xua,   L2SIXFER, L2MCONNT },
  { xua,   L2SIXFER, L2MLRESF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SIXFER, L2MLRESF },
  { xua,   L2SIXFER, L2MLRESF },
  { xua,   L2SIXFER, L2MLRESF },
  { xua,   L2SDBS  , L2MLRESF },
  { xua,   L2SIXFER, L2MLRESF },
  { xua,   L2SDBS  , L2MLRESF },
  { xua,   L2SDBS  , L2MLRESF },
  { xua,   L2SIXFER, L2MLRESF },
  { xua,   L2SDBS  , L2MLRESF },
  { xua,   L2SDBS  , L2MLRESF },
  { xua,   L2SIXFER, L2MLRESF },
  { xua,   L2SIXFER, L2MLRESF },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH,   L2MNIX   }
};

/* SABM EITHER (COMMAND, VIA) */

STENTRY stbl08a[NUMSTATS] =
{
  { xnull, L2SHTH,   L2MCONNT },
  { xnull, L2SHTH,   L2MCONNT },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
  { xnull, L2SHTH,   L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH,   L2MNIX   }
};

/* DISC EITHER (COMMAND) */

STENTRY stbl09[NUMSTATS] =
{
  { xdm,   L2SDSCED, L2MNIX   },
  { xdm,   L2SDSCED, L2MBUSYF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
  { xua,   L2SDSCED, L2MDISCF },
/*** ENDE AX.25 States ***/
  { xnull, L2SDSCED, L2MDISCF },
};

/* RR WITH FINAL (RESPONSE) */

STENTRY stbl10[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { xnull, L2SIXFER, L2MNIX   },
  { xnull, L2SRS   , L2MNIX   },
  { xnull, L2SIXFER, L2MNIX   },
  { xnull, L2SDBS  , L2MNIX   },
  { xnull, L2SIXFER, L2MNIX   },
  { xnull, L2SDBS  , L2MNIX   },
  { xnull, L2SDBS  , L2MNIX   },
  { xnull, L2SIXFER, L2MNIX   },
  { xnull, L2SDBS  , L2MNIX   },
  { xnull, L2SRSDBS, L2MNIX   },
  { xnull, L2SRS   , L2MNIX   },
  { xnull, L2SRSDBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

#ifdef DAMASLAVE
/* DAMA POLL */

STENTRY stbl10a[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { xnull, L2SIXFER, L2MNIX   },
  { xnull, L2SRS   , L2MNIX   },
  { xnull, L2SIXFER, L2MNIX   },
  { xnull, L2SDBS  , L2MNIX   },
  { xnull, L2SRBS  , L2MNIX   },
  { xnull, L2SBBS  , L2MNIX   },
  { xnull, L2SDBS  , L2MNIX   },
  { xnull, L2SRBS  , L2MNIX   },
  { xnull, L2SBBS  , L2MNIX   },
  { xnull, L2SRSDBS, L2MNIX   },
  { xnull, L2SRSRBS, L2MNIX   },
  { xnull, L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};
#endif

/* RNR WITH FINAL (RESPONSE) */

STENTRY stbl14[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { xnull, L2SRBS  , L2MNIX   },
  { xnull, L2SRSRBS, L2MNIX   },
  { xnull, L2SRBS  , L2MNIX   },
  { xnull, L2SBBS  , L2MNIX   },
  { xnull, L2SRBS  , L2MNIX   },
  { xnull, L2SBBS  , L2MNIX   },
  { xnull, L2SBBS  , L2MNIX   },
  { xnull, L2SRBS  , L2MNIX   },
  { xnull, L2SBBS  , L2MNIX   },
  { xnull, L2SRSBBS, L2MNIX   },
  { xnull, L2SRSRBS, L2MNIX   },
  { xnull, L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* UA EITHER (RESPONSE) */

STENTRY stbl16[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SIXFER, L2MCONNT },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SIXFER, L2MNIX   },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
  { xsabm, L2SLKSUP, L2MLREST },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* DM EITHER (RESPONSE) */

STENTRY stbl17[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SDSCED, L2MBUSYF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
  { xnull, L2SDSCED, L2MDISCF },
/*** ENDE AX.25 States ***/
  { xnull, L2SDSCED, L2MDISCF }
};

/* FRMR EITHER (RESPONSE) */

STENTRY stbl18[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xnull, L2SDSCRQ, L2MNIX   },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
  { xsabm, L2SLKSUP, L2MFRMRF },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MFRMRF }
};

/* LOCAL START COMMAND */
STENTRY stbl19[NUMSTATS] =
{
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   }, /* xnull */
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xsabm, L2SLKSUP, L2MNIX   }
};

/* LOCAL STOP COMMAND */
STENTRY stbl20[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xdisc, L2SDSCED, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   }, /* xnull */
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SDSCED, L2MDISCF }
};

/* STATION BECOMES BUSY */

STENTRY stbl21[NUMSTATS] =
{
  { xnull, L2SDSCED , L2MNIX  },
  { xnull, L2SLKSUP , L2MNIX  },
  { xnull, L2SFRREJ , L2MNIX  },
  { xnull, L2SDSCRQ , L2MNIX  },
  { t2rnrr,L2SDBS   , L2MNIX  },
  { t2rnrr,L2SRSDBS , L2MNIX  },
  { t2rnrr,L2SWADBS , L2MNIX  },
  { xnull, L2SDBS   , L2MNIX  },
  { t2rnrr,L2SBBS   , L2MNIX  },
  { xnull, L2SBBS   , L2MNIX  },
  { xnull, L2SWADBS , L2MNIX  },
  { t2rnrr,L2SWABBS , L2MNIX  },
  { xnull, L2SWABBS , L2MNIX  },
  { xnull, L2SRSDBS , L2MNIX  },
  { t2rnrr,L2SRSBBS , L2MNIX  },
  { xnull, L2SRSBBS , L2MNIX  },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH   , L2MNIX  }
};

/************************************************************************\
*                                                                        *
* BUSY CONDITION CLEARS                                                  *
*                                                                        *
* geaendert: RR mit Poll senden und T1 starten. Dadurch wird sicherge-   *
* stellt, dass die Gegenstation vom neuen Linkzustand erfaehrt.          *
* ON5ZS: zusaetzlich auch in den State "waiting acknowledge" uebergehen, *
* damit wir auch wirklich die Antwort bestaetigt loswerden.              *
*                                                                        *
\************************************************************************/

/* BUSY CONDITION CLEARS */

STENTRY stbl22[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { xnull, L2SIXFER, L2MNIX   },
  { xnull, L2SRS   , L2MNIX   },
  { xnull, L2SWA   , L2MNIX   },
  { xrrc,  L2SWA   , L2MNIX   },
  { xnull, L2SRBS  , L2MNIX   },
  { xrrc,  L2SWARBS, L2MNIX   },
  { xrrc,  L2SWA   , L2MNIX   },
  { xnull, L2SWARBS, L2MNIX   },
  { xrrc,  L2SWARBS, L2MNIX   },
  { xrrc,  L2SWA   , L2MNIX   },
  { xnull, L2SRSRBS, L2MNIX   },
  { xrrc,  L2SWARBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* T1 EXPIRES (VERSION 2) */

STENTRY stbl23[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xfrmr, L2SFRREJ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
#ifdef IPOLL_FRAME
  { sdipoll,  L2SWA   , L2MNIX   },
  { sdipoll,  L2SWA   , L2MNIX   },
  { sdipoll,  L2SWA   , L2MNIX   },
#else
  { xrrc,  L2SWA   , L2MNIX   },
  { xrrc,  L2SWA   , L2MNIX   },
  { xrrc,  L2SWA   , L2MNIX   },
#endif /* IPOLL_FRAME */
  { xrnrc, L2SWADBS, L2MNIX   },
  { xrrc,  L2SWARBS, L2MNIX   },
  { xrnrc, L2SWABBS, L2MNIX   },
  { xrnrc, L2SWADBS, L2MNIX   },
  { xrrc,  L2SWARBS, L2MNIX   },
  { xrnrc, L2SWABBS, L2MNIX   },
  { xrnrc, L2SWADBS, L2MNIX   },
  { xrrc,  L2SWARBS, L2MNIX   },
  { xrnrc, L2SWADBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* T3 EXPIRES */

STENTRY stbl24[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xsabm, L2SLKSUP, L2MNIX   },
  { xfrmr, L2SFRREJ, L2MNIX   },
  { xdisc, L2SDSCRQ, L2MNIX   },
  { xrrc,  L2SWA   , L2MNIX   },
  { xrrc,  L2SWA   , L2MNIX   },
  { xnull, L2SWA   , L2MNIX   },
  { xrnrc, L2SWADBS, L2MNIX   },
  { xrrc,  L2SWARBS, L2MNIX   },
  { xrnrc, L2SWABBS, L2MNIX   },
  { xnull, L2SWADBS, L2MNIX   },
  { xnull, L2SWARBS, L2MNIX   },
  { xnull, L2SWABBS, L2MNIX   },
  { xrnrc, L2SWADBS, L2MNIX   },
  { xrrc,  L2SWARBS, L2MNIX   },
  { xrnrc, L2SWADBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SDSCED, L2MFAILW }
};

/* N2 IS EXCEEDED */

STENTRY stbl25[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xdisc, L2SDSCED, L2MFAILW }, /* N2 abgelaufen, disconnecten DG9OBU */
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
  { xnull, L2SDSCED, L2MFAILW },
/*** ENDE AX.25 States ***/
  { xnull, L2SDSCED, L2MFAILW }
};

/* INVALID N(S) RECEIVED WITHOUT POLL */

STENTRY stbl26[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { t2rejr,L2SRS   , L2MNIX   },
  { xnull, L2SRS   , L2MNIX   },
  { xnull, L2SWA   , L2MNIX   },
  { t2rnrr,L2SDBS  , L2MNIX   },
  { t2rejr,L2SRSRBS, L2MNIX   },
  { t2rnrr,L2SBBS  , L2MNIX   },
  { xnull, L2SWADBS, L2MNIX   },
  { xnull, L2SWARBS, L2MNIX   },
  { xnull, L2SWABBS, L2MNIX   },
  { t2rnrr,L2SRSDBS, L2MNIX   },
  { xnull, L2SRSRBS, L2MNIX   },
  { t2rnrr,L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* INVALID N(S) RECEIVED WITH POLL */

STENTRY stb26b[NUMSTATS] =
{
  { xdm,   L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xfrmr, L2SFRREJ, L2MNIX   },
  { xdm,   L2SDSCED, L2MDISCF }, /* war L2SDSCRQ, 05.02.98, DB7KG */
  { xrejr, L2SRS   , L2MNIX   },
  { xrejr, L2SRS   , L2MNIX   },
  { xrejr, L2SWA   , L2MNIX   },
  { xrnrr, L2SDBS  , L2MNIX   },
  { xrejr, L2SRSRBS, L2MNIX   },
  { xrnrr, L2SBBS  , L2MNIX   },
  { xrnrr, L2SWADBS, L2MNIX   },
  { xrejr, L2SWARBS, L2MNIX   },
  { xrnrr, L2SWABBS, L2MNIX   },
  { xrnrr, L2SRSDBS, L2MNIX   },
  { xrejr, L2SRSRBS, L2MNIX   },
  { xrnrr, L2SRSBBS, L2MNIX   },
/*** ENDE AX.25 States ***/
  { xnull, L2SHTH  , L2MNIX   }
};

/* INVALID N(R) RECEIVED */

STENTRY stbl27[NUMSTATS] =
{
  { xnull, L2SDSCED, L2MNIX   },
  { xnull, L2SLKSUP, L2MNIX   },
  { xnull, L2SFRREJ, L2MNIX   },
  { xnull, L2SDSCRQ, L2MNIX   },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
  { xfrmr, L2SFRREJ, L2MFRMRT },
/*** ENDE AX.25 States ***/
  { xnull, L2SDSCED, L2MDISCF }
};

/************************************************************************\
*                                                                        *
* "message functions"                                                    *
*                                                                        *
* Fuer jede Meldung gibt es eine Funktion, die noch Level 2 Arbeiten     *
* erledigt. Sie wird vor der Meldung aufgerufen.                         *
*                                                                        *
\************************************************************************/

MFENTRY mftb[10] =
{
  { xnull  },                 /*    keine Nachricht                     */
  { inilnk },                 /*    CONNECTED to                        */
  { i2xclr },                 /*    DISCONNECTED from                   */
  { clrlnk },                 /*    BUSY from                           */
  { i2xclr },                 /*    LINK FAILURE with                   */
  { /*inilnk*/ reslnk },      /*    LINK RESET from                     */
  { reslnk },                 /*    LINK RESET to                       */
  { xnull  },                 /*    FRAME REJECT from                   */
  { xnull  },                 /*    FRAME REJECT to                     */
  { xnull  }                  /*    BUSY to                             */
};

/************************************************************************\
*                                                                        *
* "level 2 state machine"                                                *
*                                                                        *
* Ausfuehren der Zustandsuebergangsfunktion des Linkstatus (state) des   *
* aktuellen Linkblocks (lnkpoi) in der Statetable stbl, danach einnehmen *
* des durch die Statetable gegebenen neuen Zustands.                     *
*                                                                        *
\************************************************************************/
void
l2stma(STENTRY stbl[])
{
  STENTRY *st;
  int      msg;

  st = &stbl[lnkpoi->state];        /* Zeiger auf den Zustandseintrag   */
  msg = st->msg;                    /* Meldung zum Layer                */

  st->func();                       /* Zustandsuebergangsfunktion       */
  l2newstate(st->newstate);         /* neuen State setzen               */
  if (msg != L2MNIX)
   {
    mftb[msg].func();               /* Meldungsfunktion                 */
#ifdef __WIN32__
    l2tolx((short)msg);             /* Ereignis melden                  */
#else
    l2tolx(msg);                    /* Ereignis melden                  */
#endif /* WIN32 */
   }
}

/************************************************************************\
*                                                                        *
* "level 2 new state"                                                    *
*                                                                        *
* In einen neuen Zustand uebergehen. Da hier die aktiven Links gezaehlt  *
* werden, duerfen Status-Wechel nur hier durchgefuehrt werden.           *
*                                                                        *
\************************************************************************/
void
l2newstate(WORD newstate)
{
  WORD      oldstate;
  PORTINFO *p;
  int       port;

  oldstate = lnkpoi->state;         /* alten Status merken              */
/****************************************************/
/*   Konvertierung von 'short ' in 'unsigned char ',*/
/*   moeglicher Datenverlust                        */
/*   Modifiziert durch Oliver Kern                  */
/*                     Str. d. Zukunft 29           */
/*                             01612 Glaubitz               */
#ifdef __WIN32__
  lnkpoi->state = (UBYTE)newstate;
#else
  lnkpoi->state = newstate;
#endif
/****************************************************/
  if (oldstate && !newstate)        /* ein Link wird abgebaut           */
   {
    relink(ulink((LEHEAD *)lnkpoi), /* aus der aktiv-Liste nehmen       */
           (LEHEAD *)l2frel.tail);  /* in die Freiliste haengen         */
    nmblks--;                       /* nun haben wir einen weniger      */
    (p = &portpar[port = lnkpoi->liport])->nmblks--;
    if (!multiconn(port, lnkpoi->realid))        /* letzter Connect?    */
     {
      p->nmbstn--;
      autopers(port);               /* Port-Parameter anpassen          */
     }
    getMCs();                       /* Veraenderung DAMA melden         */
   }
  if (!oldstate &&  newstate)       /* ein Link wird aufgebaut          */
   {
    if (++nmblks > nmblks_max)      /* Maximalwert fuer die Statistik   */
      nmblks_max = nmblks;
    (p = &portpar[port = lnkpoi->liport])->nmblks++;
    if (!multiconn(port, lnkpoi->realid))        /* erster Connect?     */
     {
      p->nmbstn++;
      autopers(port);               /* Port-Parameter anpassen          */
     }
    relink(ulink((LEHEAD *)lnkpoi), /* Link aus der frei-Liste nehmen   */
           (LEHEAD *)l2actl[lnkpoi->liport].tail);
    getMCs();                       /* Veraenderung DAMA melden         */
   }
}

/* End of src/l2stma.c */
