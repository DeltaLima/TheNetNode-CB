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
/* File include/l2s.h (maintained by: DF6LN)                            */
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

/*                        Zugriff anderer Module auf die State-Tabellen   */
/**************************************************************************/

/*
 *   nicht aufgefuehrte Tabellen werden durch das Programm direkt
 *   ausgefuehrt
 */

extern    STENTRY stbl00[];             /* I WITH POLL (COMMAND)          */
extern    STENTRY stbl00dama[];         /* I WITH POLL (COMMAND, DAMA)    */
extern    STENTRY stbl01[];             /* I WITHOUT POLL (COMMAND)       */
extern    STENTRY stbl01dama[];         /* I WITHOUT POLL (COMMAND, DAMA) */
extern    STENTRY stbl02[];             /* RR WITH POLL (COMMAND)         */
extern    STENTRY stbl03[];             /* RR WITHOUT POLL (COMMAND)      */
#define           stbl04      stbl02    /* REJ WITH POLL (COMMAND)        */
#define           stbl05      stbl03    /* REJ WITHOUT POLL (COMMAND)     */
extern    STENTRY stbl06[];             /* RNR WITH POLL (COMMAND)        */
extern    STENTRY stbl07[];             /* RNR WITHOUT POLL (COMMAND)     */
extern    STENTRY stbl08[];             /* SABM EITHER (COMMAND)          */
extern    STENTRY stbl08a[];            /* SABM EITHER (COMMAND, VIA)     */
extern    STENTRY stbl09[];             /* DISC EITHER (COMMAND)          */
extern    STENTRY stbl10[];             /* RR WITH FINAL (RESPONSE)       */
#ifdef DAMASLAVE
extern    STENTRY stbl10a[];            /* DAMA Poll                      */
#endif
#define           stbl11      stbl03    /* RR WITHOUT FINAL (RESPONSE)    */
#define           stbl12      stbl10    /* REJ WITH FINAL (RESPONSE)      */
#define           stbl13      stbl03    /* REJ WITHOUT FINAL (RESPONSE)   */
extern    STENTRY stbl14[];             /* RNR WITH FINAL (RESPONSE)      */
#define           stbl15      stbl07    /* RNR WITHOUT FINAL (RESPONSE)   */
extern    STENTRY stbl16[];             /* UA EITHER (RESPONSE)           */
extern    STENTRY stbl17[];             /* DM EITHER (RESPONSE)           */
extern    STENTRY stbl18[];             /* FRMR EITHER (RESPONSE)         */
extern    STENTRY stbl19[];             /* LOCAL START COMMAND            */
extern    STENTRY stbl20[];             /* LOCAL STOP COMMAND             */
extern    STENTRY stbl20a[];            /* LOCAL STOP COMMAND, FORCED     */
extern    STENTRY stbl21[];             /* STATION BECOMES BUSY           */
extern    STENTRY stbl22[];             /* BUSY CONDITION CLEARS          */
extern    STENTRY stbl23[];             /* T1 EXPIRES (VERSION 2)         */
extern    STENTRY stbl24[];             /* T3 EXPIRES                     */
extern    STENTRY stbl25[];             /* N2 IS EXCEEDED                 */
extern    STENTRY stbl26[];             /* INVALID N(S) RECEIVED  no Poll */
extern    STENTRY stb26b[];             /* INVALID N(S) RECEIVED  Poll    */
extern    STENTRY stbl27[];             /* INVALID N(R) RECEIVED          */
#define           stbl28      stbl27    /* UNRECOGNIZED FRAME RECEIVED    */

/* End of include/l2s.h */
