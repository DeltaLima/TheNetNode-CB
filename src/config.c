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
/* File src/config.c (maintained by: ???)                               */
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

/*----------------------------------------------------------------------*/
/* ACHTUNG:     ALLE DEFAULT-WERTE SIND HIER EXPLIZIT ANGEGEBEN !!      */
/* ========     BEI AENDERUNGEN UNBEDINGT AUF DIE ANZAHL DER ARRAY-     */
/*              ELEMENTE ACHTEN !!                                      */
/*----------------------------------------------------------------------*/

BOOLEAN bVerbose = FALSE;       /* Erweitertes Logging an der Konsole ? */
 
/*---------------------------------------------- Parameter fuer Level 2 */

UWORD DamaSpeedFactor = 0;      /* 0=ausgeschaltet..                    */
#ifndef PORT_MANUELL
UWORD paclen = 236;
#endif /* PORT_MANUELL */
UWORD dama_enable = 0;          /* 1=DAMA-Betrieb, 0=RTT-Betrieb        */
UWORD dama_init = 100;          /* Anfangswert fuer DAMA Timer (10 ms)  */
UWORD dama_max  = 15;           /* Maximaler Aktivitaetszaehlerstand    */
UWORD MaxPollCnt = 0;           /* Maximale Anzahl von non-DAMA Polls.. */

/*---------------------------------------------- Parameter fuer Level 3 */

UWORD broint_ui                 /* Rundspruchintervall                  */
    = 30;

UWORD broint_i                  /* Rundspruchintervall                  */
    = 60;

UWORD timliv                    /* Anfangswert Paketlebensdauer         */
    = 30;

UWORD worqua                    /* minimale Qualitaet fuer Autoupdate   */
    = 40;

UWORD mymaxtime                 /* max. Laufzeit zu einem Ziel          */
    = 0;

#ifdef AXIPR_HTML
char htmlpath[MAXPATH]           /* Pfadname fuer HTML                    */
        = HTMLPATH;
#endif

#ifdef MH_LISTE
UWORD MHdefault =  1;
UWORD MHlen     = 30;
#endif

#ifdef TIMEOUT_MANUELL
UWORD ininat =  3600;
#endif /* TIMEOUT_MANUELL */

#ifdef PACSAT
LONG  last_fid = -1L;
LONG  first_fid = -1L;
UWORD pacsat_free = 1000;
UWORD pacsat_timer = 200;       /* in ms, Timeout fuer Sendefreigabe    */
UWORD pacsat_frames = 15;

char pacsatid[]                 /* Rufzeichen                           */
        = { ' ',' ',' ',' ',' ',' ','\140'};
#endif


/*---------------------------------------------- Parameter fuer Level 4 */

UWORD trawir                    /* Level4 vorgeschlagene Fenstergr.     */
        = 10;

/*---------------------------------------------- Parameter fuer Level 7 */

UWORD conctl                    /* gepufferte Frames je Verbindung      */
        = 10;

UWORD save_timer                /* Autom. Speichern in 10 min-Intervall */
                                /* 0 = abgeschaltet                     */
        = 6;

BOOLEAN hostco                  /* Flag Hostconnect erlaubt j/n         */
        = FALSE;

WORD max_jh                     /* Anzahl Eintraege in HEARD-Liste      */
        = 150;

char promptstr[80]                /* Prompt                               */
        = "%c de %d (%t)>";

UWORD syspro_flag               /* SYSOP-Protokoll : enabled/disabled   */
        = TRUE;

/*----------------------------------------------------------------------*/
/* Folgende Parameter werden in TNNxxx.PAS abgespeichert                */
/*----------------------------------------------------------------------*/

char paswrd[85] =               /* SYSOP-Passwort                       */
"12345678901234567890123456789012345678901234567890123456789012345678901234567890";

UWORD paswle                    /* Laenge des Passworts                 */
        = 80;

char pass[85]                   /* Console-Passwort                     */
        = "Geheim";

char alias[]                    /* Ident                                */
        = { 'T','e','s','t',' ',' ','\0' };

char myid[]                     /* Rufzeichen                           */
        = { 'X','X','0','X','X',' ','\140'};

char hostid[]                    /* Rufzeichen fuer Host-Console        */
        = { 'X','X','0','X','X','X','\140'};

UWORD testid                     /* SSID fuer den Linktest              */
        = 15;

UWORD convid                     /* SSID fuer den Convers               */
        = 1;

char textpath[MAXPATH]           /* Pfadname fuer Systemtexte/Hilfe     */
        = TEXTPATH;

char textcmdpath[MAXPATH]        /* Pfadname fuer Texte als Commands    */
        = TEXTCMDPATH;

char userexepath[MAXPATH]        /* Pfadname fuer ext. User-Programme   */
        = USEREXEPATH;

char sysopexepath[MAXPATH]       /* Pfadname fuer ext. Sysop-Programme  */
        = SYSEXEPATH;

#ifdef PACSAT
char pacsatpath[MAXPATH]         /* Pfadname fuer PacSat                */
        = PACSATPATH;
#endif

char msgpath[MAXPATH]            /* Pfad fuer Nachrichten (MSG-Befehl)  */
        = MSGPATH;

#ifndef MC68K
char exename[MAXPATH];           /* Name der EXE                        */
#endif

/*----------------------------------------------------------------------*/
/* Folgende Parameter werden in TNNxxx.STA abgespeichert                */
/*----------------------------------------------------------------------*/

STAT mh[MAXSTAT];               /* Statistik-Tabelle                    */

PORTSTAT portstat[L2PNUM];      /* TNC-Statistik                        */

UWORD maxlfz = 1200;

UWORD proto = 0;

#ifdef HOSTMYCALL
char hostuserid[] = { 'X','X','0','X','X','X','\0'};
#endif

/* End of src/config.c */
