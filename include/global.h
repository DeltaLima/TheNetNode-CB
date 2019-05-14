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
/* File include/global.h (maintained by: ???)                           */
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

/* Variablen fuer Level 1 */

extern char huge *RAMBOT;       /* Zeiger auf Anfang des freien Speichers  */
extern char huge *RAMTOP;       /* Zeiger auf Ende des freien Speichers    */

extern WORD actch;              /* bearbeiteter Host-Kanal                 */
extern BOOLEAN tnb_ch;

extern WORD stamp;

extern WORD kick[];
extern WORD commandflag[];
extern WORD testflag[];
extern WORD show_recovery;
extern WORD watchdog;           /* Watchdog, Timer erhoeht                  */

extern ULONG throughput;        /* Gesammtdurchsatz                         */
extern ULONG thbps;             /* Gesammtdurchsatz in bps                  */
extern ULONG thbps_max;         /* Gesammtdurchsatz in bps                  */

#define WATCHDOG_TIMEOUT 18000  /* Nach 3 Minuten ohne RX Port reset        */

extern char myid[];             /* Call (normal) + SSID (1 Bit linksgesch.) */
extern char hostid[];           /* Call fuer Host-Console                   */
extern char alias[];            /* Ident der Station                        */
extern char boxid[];            /* Rufzeichen der Mailbox                   */
extern char dxcid[];            /* Rufzeichen des Clusters                  */
extern UWORD convid;            /* SSID fuer Convers                        */
extern UWORD testid;            /* SSID fuer Linktest                       */
extern UWORD autoipr;           /* automatische INP IP-Routen               */

extern UWORD nmblks;            /* Anzahl aktiver Level-2-Links (1..127)    */
extern UWORD nmblks_max;        /* Maximalanzahl der Level-2-Links          */
                                /* empfangenes Frame :                      */
extern char rxfhdr[];           /*   Header (Ziel/Quell/via-Id's), 0-term.  */
extern UBYTE rxfctl;            /*   Kontrollbyte ohne P/F-Bit              */
extern UBYTE rxfPF;             /*   V2-Frame: 0x10 = P/F gesetzt, 0 sonst  */
extern UBYTE rxfCR;             /*   V2-Frame: 0x80 = Command-Frame         */
                                /*             0x00 = Response-Frame        */
extern BOOLEAN rxfDA;           /*   Frame ist DAMA-Frame                   */
#ifdef EAX25
extern UBYTE rxfctlE;           /*   zweites Controlbyte                    */
extern BOOLEAN rxfEAX;          /*   Frame ist Extended-AX.25-Frame         */
#endif
extern int  rxfprt;             /*   Empfangs-Port                          */
                                /* zu sendendes Frame :                     */
extern char txfhdr[L2AFLEN+1];  /*   Header (Ziel/Quell/via-Id's), 0-term.  */
extern UBYTE txfctl;            /*   Kontrollbyte ohne P/F-Bit              */
#ifdef EAX25
extern UBYTE txfctlE;           /*   zweites Controlbyte                    */
extern BOOLEAN txfEAX;          /*   Kennzeichnung fuer EAX.25-Frames       */
#endif
extern UBYTE txfPF;             /*   V2-Frame: 0x10 = P/F gesetzt, 0 sonst  */
extern UBYTE txfCR;             /*   V2-Frame: 0x80 = Command-Frame         */
                                /*             0x00 = Response-Frame        */
extern int  txfprt;

extern UWORD DamaSpeedFactor;
extern UWORD paclen;
extern UWORD dama_init;         /* Anfangswert fuer DAMA Timer (10 ms)  */
extern UWORD dama_max;          /* Maximaler Aktivitaetszaehlerstand    */
extern UWORD MaxPollCnt;        /* Max. Anzahl von erlaubten Polls..    */

#define T3par 18000

extern UWORD nmbfre;            /* "number free", Anzahl freier 32-Byte-    */
                                /* Buffer (36 Byte mit Kopf)                */
extern UWORD nmbfre_min;        /* min Buffer                               */
extern UWORD nmbfre_max;        /* max Buffer                               */

extern LHEAD freel;             /* "free list",                             */
                                /* Listenkopf Freibuffer                    */
extern LHEAD rxfl;              /* "rx frame list",                         */
                                /* Listenkopf empfangene Frames             */
extern LHEAD stfl;              /* "sent frame list",                       */
                                /* Listenkopf gesendete Frames              */
extern LHEAD trfl;              /* "trash frame list",                      */
                                /* Listenkopfe Frames fuer den Muelleimer   */
extern LHEAD damarl[L2PNUM];    /* "DAMA random list" - fuer Frames ausser- */
                                /* halb der DAMA-Steuerung (UI / UA / DM)   */
extern LHEAD txl2fl[L2PNUM];    /* "tx level 2 frame list",                 */
                                /* Listenkoepfe (je Port einer) zu sendende */
                                /* Frames                                   */
extern LNKBLK *lnktbl;          /* "link table", fuer jeden moeglichen      */
                                /* Level-2-Link ein Eintrag                 */
extern LNKBLK *lnkpoi;          /* "link pointer", globaler Zeiger auf den  */
                                /* gerade aktuellen Linkblock (in lnktbl)   */
extern LHEAD   l2frel;          /* Liste der freien Linkbloecke             */
extern LHEAD   l2actl[L2PNUM];  /* Aktive Linkbloecke je Port               */

extern char    l2als[L2CALEN];  /* Alias gross geschrieben fuer Connects    */
                                /* und UI-Digipeating                       */



/****************************************************************************/
/*** Variable fuer Level 3                                                ***/

extern NETWORK *netp;

extern UWORD broint_ui;        /* Broadcast-Interval                        */
extern UWORD broint_i;         /* Broadcast-Interval                        */
extern UWORD timliv;           /* Anfangswert Paketlebensdauer              */
extern UWORD worqua;           /* minimal Qualitaet fuer Autoupdate         */
extern UWORD mymaxtime;        /* max. Laufzeit zu einem Ziel               */

#define autoqual 10

extern UWORD l3rtt_time;       /* Uhr fuer L3-RTT Berechnung                */
extern int   num_nodes_max;    /* bisher max. registrierte Nodes            */

extern LHEAD l3rxfl;           /* Level3 empfangene Frames                  */
extern LHEAD l3txl;            /* Level3 zu senddende Frames                */

extern MBHEAD *aliasbp;        /* Alias-Liste fuer Flexnet                  */

/****************************************************************************/
/*** Variable fuer Level 4                                            ***/
extern UWORD trawir;           /* Level4 vorgeschlagene Fenstergr.      */
extern LHEAD l4rxfl;           /* fuer Level4 eingegangene Frames       */
extern UBYTE l4hdr0;           /* Layer4 Header, Byte 1                 */
extern UBYTE l4hdr1;           /* Layer4 Header, Byte 2                 */
extern UBYTE l4hdr2;           /* Layer4 Header, Byte 3                 */
extern UBYTE l4hdr3;           /* Layer4 Header, Byte 4                 */
extern UBYTE l4hdr4;           /* Layer4 Header, Byte 5                 */
extern UBYTE l4opco;           /* Layer4 Opcode, Flags                  */
extern UBYTE l4pidx;           /* Layer4 Antwort, Partnerindex          */
extern UBYTE l4pcid;           /* Layer4 Antwort, Partner-ID            */
extern UBYTE l4ahd2;           /* Layer4 Antwort, Byte 2                */
extern UBYTE l4ahd3;           /* Layer4 Antwort, Byte 3                */
extern UBYTE l4aopc;           /* Layer4 Antwort, Opcode                */

extern UBYTE nmbcir;           /* Anzahl aktiver Level-4-Circuits       */
extern UBYTE nmbcir_max;       /* Maximalanzahl der Level-4-Circuits    */

extern const char *typtbl;

extern CIRBLK *cirtab;          /* Circuit Tabelle                      */
extern CIRBLK *cirpoi;          /* Pointer in Circuit Tabelle           */

/************************************************************************/
/*** Variable fuer Level 7                                              */

extern char *clipoi;            /* Pointer in CLI Zeile                 */
extern char clilin[256];        /* Zeile fuer CLI                       */

extern char usrcal[L2IDLEN];    /* Call des aktuellen Users             */
extern char ncall[L2IDLEN];     /* Call des Nachbarn                    */
extern char ndigi[L2VLEN+1];    /* Digiliste zum Nachbarn               */
#ifdef CONNECTMOD_SET_NODE
extern char updigi[L2IDLEN +1]; /* Einstiegsknoten vom aktuellen User   */
#endif /* CONNECTMOD_SET_NODE */
extern UBYTE nport;             /* Port des Nachbarn                    */

extern WORD clicnt;             /* Zaehler fuer Zeichen in CLI Zeile    */
extern UWORD paswle;            /* Laenge des Passworts                 */
extern char paswrd[];

#ifndef TIMEOUT_MANUELL
#define ininat 3600
#endif /* TIMEOUT_MANUELL */

extern UWORD conctl;           /* congestion control                    */
extern UWORD save_timer;       /* automatisches Speichern Configuration */
extern UWORD nquali;           /* Qualitaet des Knotens                 */
extern WORD  tic1s;            /* Zaehler fuer 100 x 10 ms = 1  s       */

extern PORTINFO portpar[L2PNUM];/* Konfiguration der KISS-TNC's         */

extern LHEAD usccpl;           /* Kopf der CCP-User Liste               */
extern LHEAD userhd;           /* Kopf der User Liste                   */
extern USRBLK *userpo;         /* Pointer in User Liste                 */

extern LHEAD cq_user;          /* Liste User im CQ-Modus                */
extern LHEAD cq_statl;         /* Connect-Meldungen bei CQ              */

extern PTCENT *ptctab;         /* Patchcord Tabelle                     */

/************************************************************************/
/*** Variable fuer Host-Interface                                       */

extern LHEAD   smonfl;
extern LHEAD   statml;
extern int     monlin;
extern int     stalin;
extern int     numhsts;
extern BOOLEAN ishmod;
extern int     Ypar;
extern MBHEAD *mifmbp;

extern char *blipoi;           /* Pointer in bline              */
extern char blixfl;            /* X-on / X-off, Hostinterface   */
extern BOOLEAN hostco;         /* Flag Hostconnect erlaubt j/n  */
extern WORD blicnt;            /* zaehlt Zeichen in bline       */
extern HOSTUS *hstubl;         /* Kontrollblock fuer Host-User  */
extern HOSTUS *hstusr;         /* Pointer auf akt. Host User    */


extern STAT mh[MAXSTAT];
extern PORTSTAT portstat[L2PNUM];

extern char textpath[];
extern char confpath[];
extern char msgpath[];
extern char textcmdpath[];
extern char userexepath[];
extern char sysopexepath[];
extern char pacsatpath[];
extern char exename[];

extern FILE *loadfp;
extern char loadname[];
extern char loadtmp[];

extern char anycall[];
extern char nullid[];
#define nulide nullid
extern char cqdest[];          /* Call fuer CQ-Ruf              */
extern char dmmsg[];           /* Disconnect Meldung            */
extern char conmsg[];          /* Connect Meldung               */
extern char recmsg[];          /* Reconnect Meldung             */
extern char failmsg[];         /* Failure with Meldung          */
extern char invcalmsg[];       /* Invalid Call Meldung          */
extern char promptstr[];

extern char pass[];

extern PARAM partab[];
extern int partablen;

/* Befehlstabelle */
extern COMAND cmdtab[];
extern COMAND syscmdtab[];
extern L1MODETAB l1modetab[];
extern HOSTCMD hostcmdtab[];
extern FILE  *consfile;
extern WORD startup_running;

extern MHTAB l2heard;
extern MHTAB l3heard;

extern time_t start_time;
extern time_t clear_time;

extern UWORD tkbaud;
extern int tkcom;

extern ULONG bytecnt;
extern ULONG checksum;
extern UWORD crc;
extern UWORD crctab[];

extern UWORD syspro_flag;
extern char signon[];
extern char version[];
extern char cfgfile[];
#ifdef AUTO_UPDATE
extern char oldcfgfile[];
#endif

extern ULONG lastic;
extern volatile ULONG tic10;
extern UBYTE port_status;

extern SUSPEND  sustab[];

extern UWORD dmagic;
#define MAGIC_L2PROFILE 2205        /* Par 1 = 2205 -> L2 Traffic       */

/* Definitionen for PP-Convers  */
extern WORD        cvs_pc;
extern time_t      currtime;
extern time_t      boottime;
extern const char  *convtype;
extern const char  *myfeatures;
extern char        *myhostname;
extern char        myrev[];
extern char        timestamp[];
extern PERMLINK    *permarray[MAXCVSHOST];
extern CONNECTION  *connections;
extern DESTINATION *destinations;
extern CHANNEL     *channels;

extern LONG rounds_pro_sec;             /* Anzahl Rounds/Sekunde        */
extern LONG rounds_max_sec;             /* Maximale Anzahl              */
extern LONG rounds_min_sec;             /* Minimale Anzahl              */
extern LONG rounds_count;               /* Zaehler fuer Rounds          */

extern BEACON beacon[L2PNUM];

extern time_t t;

#if defined(CRASHDEBUG) || defined(__LINUX__)
extern char *wowarich;
extern char *wowarich2;
#endif

#ifdef PACSAT
extern WORD  pacsat_enabled[];
extern UWORD pacsat_timer;
extern UWORD pacsat_frames;
extern UWORD pacsat_free;
extern LONG  first_fid;
extern LONG  last_fid;
extern char  pacsatid[];
#endif

extern unsigned int_level;              /* Interrupt-Vertiefungs-Level */

extern ULONG MEMORY_NEEDED;   /* fuer Message Buffer   */
extern UWORD proto;
extern char loginstr[];
extern char infostr[];

extern char author[];

extern BOOLEAN    tnnb_aktiv;

extern time_t sys_time;
extern struct tm *sys_localtime;

#ifdef GRAPH
extern TGRAPH graph;
#endif

#ifdef AXIPR_HTML
extern char htmlpath[];
#endif

#ifdef MH_LISTE
extern UWORD MHdefault;
extern UWORD MHlen;
#endif

#ifdef TIMEOUT_MANUELL
extern UWORD ininat;
#endif /* TIMEOUT_MANUELL */

#ifdef HOSTMYCALL
extern char hostuserid[];
#endif

#ifdef BEACON_STATUS
extern UWORD statustim;
#endif


#ifdef AXIPR_UDP
extern BOOLEAN LookAX25IP;           /* Wir durfen keine Frame lesen. */
#endif /* AXIPR_UDP */

#ifdef DEBUG_MODUS
extern char    lastbuf[];
extern char   *lastfunc;
#endif /* DEBUG_MODUS */

extern BOOLEAN bVerbose;

/* End of global.h */
