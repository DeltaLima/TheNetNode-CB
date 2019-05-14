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
/* File src/global.c (maintained by: ???)                               */
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

#include "config.c"

PORTINFO portpar[L2PNUM];       /* Konfiguration der Ports              */
SUSPEND  sustab[MAXSUSPEND];    /* Suspend Tabelle                      */
#ifdef PACSAT
WORD    pacsat_enabled[L2PNUM]; /* PACSAT Broadcast eingeschaltet?      */
#endif
BEACON  beacon[L2PNUM];         /* Baken fuer alle Ports                */

/* Variablen fuer Level 1 */

char huge *RAMBOT;      /* Zeiger auf Anfang des freien Speichers       */
char huge *RAMTOP;      /* Zeiger auf Ende des freien Speichers         */

ULONG throughput = 0L;  /* Gesammtdurchsatz                             */
ULONG thbps = 0L;       /* Gesammtdurchsatz in bps                      */
ULONG thbps_max = 0L;   /* Gesammtdurchsatz in bps (Maximalwert)        */

WORD kick[L2PNUM];
WORD commandflag[L2PNUM];
WORD testflag[L2PNUM];
WORD show_recovery;

UWORD nmblks;           /* Anzahl aktiver Level-2-Links (1..127)        */
UWORD nmblks_max;       /* Maximalanzahl der Level-2-Links              */
                        /* empfangenes Frame :                          */
char rxfhdr[L2AFLEN+1]; /*   Header (Ziel/Quell/via-Id's), 0-term.      */
UBYTE rxfctl;           /*   Kontrollbyte ohne P/F-Bit                  */
UBYTE rxfPF;            /*   V2-Frame: 0x10 = P/F gesetzt, 0 sonst      */
UBYTE rxfCR;            /*   V2-Frame: 0x80 = Command-Frame             */
                        /*             0x00 = Response-Frame            */
BOOLEAN rxfDA;          /*   Frame ist DAMA-Frame                       */
#ifdef EAX25
UBYTE rxfctlE;          /*   zeites Controlbyte                         */
BOOLEAN rxfEAX;         /*   Frame ist Extended-AX.25-Frame             */
#endif
int  rxfprt;            /*   Empfangsport                               */
                        /* zu sendendes Frame :                         */
char txfhdr[L2AFLEN+1]; /*   Header (Ziel/Quell/via-Id's), 0-term.      */
UBYTE txfctl;           /*   Kontrollbyte ohne P/F-Bit                  */
#ifdef EAX25
UBYTE txfctlE;          /*   zweites Kontrollbyte                       */
BOOLEAN txfEAX;         /*   Kennzeichnung EAX.25-Frame fuer L2-TX      */
#endif
UBYTE txfPF;            /*   V2-Frame: 0x10 = P/F gesetzt, 0 sonst      */
UBYTE txfCR;            /*   V2-Frame: 0x80 = Command-Frame             */
                        /*             0x00 = Response-Frame            */
int  txfprt;            /*   Sendeport                                  */

UWORD nmbfre;           /* "number free", Anzahl freier Buffer          */
UWORD nmbfre_min;       /* Minimale Zahl freier Buffer                  */
UWORD nmbfre_max;       /* Maximale Zahl freier Buffer                  */

LHEAD freel;            /* "free list",                                 */
                        /* Listenkopf Freibuffer                        */
LHEAD rxfl;             /* "rx frame list",                             */
                        /* Listenkopf empfangene Frames                 */
LHEAD stfl;             /* "sent frame list",                           */
                        /* Listenkopf gesendete Frames                  */
LHEAD trfl;             /* "trash frame list",                          */
                        /* Listenkopfe Frames fuer den Muelleimer       */
LHEAD damarl[L2PNUM];   /* "DAMA random list" - fuer Frames ausserhalb  */
                        /* der DAMA-Steuerung (UI / UA / DM)            */
LHEAD txl2fl[L2PNUM];   /* "tx level 2 frame list",                     */
                        /* Listenkoepfe (je Port einer) zu sendende     */
                        /* Frames                                       */
LNKBLK *lnktbl;         /* "link table", fuer jeden moeglichen          */
                        /* Level-2-Link ein Eintrag                     */
LNKBLK *lnkpoi;         /* "link pointer", globaler Zeiger auf den      */
                        /* gerade aktuellen Linkblock (in lnktbl)       */
LHEAD   l2frel;         /* Liste der freien Linkbloecke                 */
LHEAD   l2actl[L2PNUM]; /* Aktive Linkbloecke je Port                   */

char    l2als[L2CALEN]; /* Alias gross geschrieben fuer Connects und    */
                        /* UI-Digipeating                               */

/*----------------------------------------------- Variable fuer Level 3 */

LHEAD l3rxfl;           /* Level3 empfangene Frames                     */
LHEAD l3txl;            /* Level3 zu sendende Frames                    */

const char *typtbl =    /* Tabelle der Link-Typen                       */
     "I N+N-N F L L+"
#ifdef LINKSMOD_LOCALMOD
     "L-"
     "L#"
#endif /* LINKSMOD_LOCALMOD */
;

UWORD autoipr = 3;      /* IP-Routen aus INP-Infos erstellen            */

/*----------------------------------------------- Variable fuer Layer 4 */

UBYTE l4hdr0;           /* Layer4 Header, Byte 1                        */
UBYTE l4hdr1;           /* Layer4 Header, Byte 2                        */
UBYTE l4hdr2;           /* Layer4 Header, Byte 3                        */
UBYTE l4hdr3;           /* Layer4 Header, Byte 4                        */
UBYTE l4hdr4;           /* Layer4 Header, Byte 5                        */
UBYTE l4opco;           /* Layer4 Opcode, Flags                         */
UBYTE l4pidx;           /* Layer4 Antwort, Partnerindex                 */
UBYTE l4pcid;           /* Layer4 Antwort, Partner-ID                   */
UBYTE l4ahd2;           /* Layer4 Antwort, Byte 2                       */
UBYTE l4ahd3;           /* Layer4 Antwort, Byte 3                       */
UBYTE l4aopc;           /* Layer4 Antwort, Opcode                       */
LHEAD l4rxfl;           /* fuer Layer4 eingegangene Frames              */

UBYTE nmbcir;           /* Anzahl aktiver Level-4-Circuits              */
UBYTE nmbcir_max;       /* Maximalanzahl der Level-4-Circuits           */

CIRBLK *cirtab;         /* Circuit Tabelle                              */
CIRBLK *cirpoi;         /* Pointer in Circuit Tabelle                   */

/*----------------------------------------------- Variable fuer Level 7 */

char *clipoi;           /* Pointer in CLI Zeile                         */
char clilin[256];       /* Zeile fuer CLI                               */

char usrcal[L2IDLEN];   /* Call des aktuellen Users                     */
char ncall[L2IDLEN];    /* Call des Nachbarn                            */
char ndigi[L2VLEN+1];   /* Digiliste zum Nachbarn                       */
#ifdef CONNECTMOD_SET_NODE
char updigi[L2IDLEN + 1];/* Einstiegsknoten vom aktuellen User          */
#endif /* CONNECTMOD_SET_NODE */

WORD clicnt;            /* Zaehler fuer Zeichen in CLI Zeile            */

WORD tic1s;             /* zaehlt Zeit bis 1s, wird dann rueckgesetzt   */

LHEAD usccpl;           /* Kopf der CCP-User Liste                      */
LHEAD userhd;           /* Kopf der User Liste                          */
USRBLK *userpo;         /* Pointer in User Liste                        */

LHEAD cq_user;          /* Liste User im CQ-Modus                       */
LHEAD cq_statl;         /* Connect-Meldungen bei CQ                     */

PTCENT *ptctab;         /* Patchcord Tabelle                            */

UWORD dmagic = 0;       /* Optionen zum Debuggen / Profiler             */
UWORD dummy = 0;

char  loadname[MAXPATH] = "";
char  loadtmp[MAXPATH]  = "";
FILE *loadfp;

/* Rufzeichen der Mailbox */
char boxid[L2IDLEN]   = { '\0', ' ', ' ', ' ', ' ', ' ', '\140'};
/* Rufzeichen des DX-Clusters */
char dxcid[L2IDLEN]   = { '\0', ' ', ' ', ' ', ' ', ' ', '\140'};

char anycall[L2IDLEN] = { '*',' ',' ',' ',' ',' ','\140' };
char nullid[L2IDLEN]  = { ' ',' ',' ',' ',' ',' ','\140' };
char cqdest[L2IDLEN]  = { 'C','Q',' ',' ',' ',' ','\140' };
char dmmsg[]          = "Busy from ";
char conmsg[]         = "Connected to ";
char recmsg[]         = "Reconnected to ";
char failmsg[]        = "Failure with ";
char invcalmsg[]      = "Invalid Call";

/* siehe naechste Struktur ... */
#ifdef EXPERT
extern UWORD max_lt;             /* Maximale LT                          */

extern UWORD l4_beta1;           /* RETRY-TIMER (T1) = SRTT * BETA1      */
extern UWORD l4_beta2;           /* ACK-TIMER (T2) = SRTT * BETA2        */
extern UWORD l4_beta3;           /* BUSY/REQ-TIMEOUT (T3) = SRTT * BETA3 */
#endif

PARAM partab[] = {                           /* Parameter Tabelle          */
  {&conctl,       7,    127, "NoAckBuf"},    /* gebufferte Frames          */
  {&mymaxtime,    0, 60000U, "L3-MaxTime"},  /* max. Laufzeit -> Ziel      */
  {&save_timer,   0, 0x7FFF, "SaveConfig"},  /* Config-File sichern *10min */
  {&DamaSpeedFactor,0, 9600, "DAMA-Speedf"},
  {&dama_max,     0,     30, "DAMA-MaxPri"},
  {&MaxPollCnt,   0,    255, "DAMA-MaxPol"}, /* Maximale Userpolls..       */
  {&dama_init,    0,   1000, "DAMA-Tout"},
  {&proto,        0,      2, "CommandLog"},
  {&syspro_flag,  0,      1, "SysopLog"},
  {&testid,       0,     15, "TestSSID"},
  {&convid,       0,     15, "ConvSSID"},
  {&autoipr,      0,      3, "AutoIPR"}      /* automatische INP IP-Routen */

#ifdef TIMEOUT_MANUELL
  ,{&ininat,     60,  54000, "Timeout"}      /* MANUELL: TIMEOUT           */
#endif /* TIMEOUT_MANUELL */

#ifdef MH_LISTE
  ,{&MHdefault,   0,       1,"MH-Default"}   /* Auswahl zwischen TNN und Flexnet-Stil */
  ,{&MHlen,       5,     100,"MH-len"}       /* Laenge der MH-Liste */
#endif

#ifdef BEACON_STATUS
  ,{&statustim,300L,   3600L,"Statusbake"}  /* hiermit kann man die Aussendung */
                                             /* der Statusbake steuern.         */
#endif                                       /* (Standard: aller 5min. Aussend.)*/
#ifdef EXPERT
  ,{&worqua,      0,    255, "Min-Quality"}, /* min Qualiatet Autoupdate   */
  {&max_lt,       5,     50, "L3-MaxLT"},
  {&broint_i,     6,    960, "Infocast"},    /* Rundspruchintervall in 10s */
  {&broint_ui,    1,    240, "Broadcast"},   /* Rundspruchintervall in 10s */
  {&timliv,       4,    127, "Lifetime"},    /* Anfangswert Pktlebensdauer */
  {&trawir,       4,    127, "T-Window"},    /* Fenstergroesse in Level4   */
#ifndef PORT_MANUELL
  {&paclen,      36,    256, "Paclen"},      /* Paketlaenge                */
#endif /* PORT_MANUELL */
  {&l4_beta3,     1,   1000, "L4-Bsytim"},
  {&l4_beta1,     1,   1000, "L4-Retrans"},
  {&l4_beta2,     1,   1000, "L4-Acktim"}
#endif
#ifdef L2PROFILER
  ,{&dmagic,       0, 0xFFFF, "unused"}      /*                            */
#endif
};

int partablen = (int)(sizeof(partab)/sizeof(PARAM));

/*-------------------------------------------------------- Befehlstabelle */
COMAND cmdtab[] = {
  {"?",        (void (*)) ccphelp,  "?" },
  {"A",        (void (*)) NULL,     NULL},
#ifdef ALIASCMD
  {"ALIAS",    (void (*)) ccpalias, NULL},
#endif
#ifdef IPROUTE
  {"ARP",      (void (*)) ccparp,   NULL},
#endif
#if defined(__LINUX__) || defined(__WIN32__) && defined(AX25IP)
  {"AXIPR",    (void (*)) ccpaxipr, NULL},
#endif
#if (defined(__LINUX__) || defined(__WIN32__)) && defined(ATTACH)
  {"ATTACH",   (void (*)) ccpattach,NULL},
#endif
  {"//ECHO",   (void (*)) ccpecho,  NULL},
  {"BYE",      (void (*)) ccpquit,  NULL},
#ifdef PACSAT
  {"BOX",      (void (*)) ccpbox,   NULL},
#endif
  {"BEACON",   (void (*)) ccpbea,   NULL},
#ifdef BUFFER_DEBUG
  {"BUFFER",   (void (*)) ccpbuf,   NULL},
#endif
  {"CONNECT",  (void (*)) ccpcon,   NULL},
  {"C!",       (void (*)) ccpcon,   "NO"},
  {"CLEAR",    (void (*)) ccpclr,   NULL},
  {"CONVERS",  (void (*)) ccpcvs,   NULL},
  {"CQ",       (void (*)) ccpcq,    NULL},
#if defined(MC68302) || defined(__WIN32__)
  {"COPY",     (void (*)) ccpcopy,  NULL},
#endif /* WIN32 */
  {"DEST",     (void (*)) ccpdest,  NULL},
  {"DATE",     (void (*)) ccptim,   NULL},
#if (defined(__LINUX__) || defined(__WIN32__)) && defined(ATTACH)
  {"DETACH",   (void (*)) ccpdetach,NULL},
#endif
#if defined(MC68302) || defined(__WIN32__)
  {"DELETE",   (void (*)) ccpdelete,NULL},
  {"DIR",      (void (*)) ccpdir,   NULL},
#endif /* WIN32 */
#ifndef MC68302
  {"DOS",      (void (*)) ccpshell, NULL},
#endif
  {"DCD",      (void (*)) ccpdcd,   NULL},
  {"DXCLUSTER",(void (*)) ccpdxc,   NULL},
  {"EDIT",     (void (*)) ccpedi,   NULL},
  {"ESC",      (void (*)) ccpesc,   NULL},
#ifdef GRAPH
  {"GRAPH",    (void (*)) ccpgraph, NULL},
#endif
  {"HELP",     (void (*)) ccphelp,  NULL},
  {"HARDWARE", (void (*)) NULL,     NULL},
#ifdef L1HTTPD
  {"HTTPD",    (void (*)) ccphttpd, NULL},
#endif /* L1HTTPD */
  {"INFO",     (void (*)) NULL,     NULL},
#ifdef IPROUTE
  {"IPADDR",   (void (*)) ccpipa,   NULL},
#if 0
  {"IPBCST",   (void (*)) ccpipb,   NULL},     /* IPB ist gesperrt! */
#endif
#ifdef L1IPCONV
  {"IPCONVERS",(void (*)) ccpipconv,NULL},
#endif /* L1IPCONV */
  {"IPROUT",   (void (*)) ccpipr,   NULL},
#endif
#ifdef L1IRC
  {"IRC",      (void (*)) ccpirc,   NULL},
#endif /* L1IRC */
#ifdef KERNELIF
  {"KERNELIF", (void (*)) ccpkif,   NULL},
#endif
  {"KILL",     (void (*)) ccpkill,  NULL},
  {"LINKS",    (void (*)) ccplnk,   NULL},
  {"L3MHEARD", (void (*)) ccpl3mh,  NULL},
  {"LOAD",     (void (*)) ccpload,  NULL},
#ifdef THENETMOD
  {"L4PARMS",  (void (*)) ccpL4par, NULL},
#endif /* THENETMOD */
  {"MAILBOX",  (void (*)) ccpmail,  NULL},
  {"MHEARD",   (void (*)) ccpl2mh,  NULL},
#ifdef USER_MONITOR
  {"MONITOR",  (void (*)) ccptrace, NULL},
#endif /* USER_MONITOR */
#ifdef HOSTMYCALL
  {"MYHOST",   (void (*)) ccpmyhost,NULL},
#endif /* HOSTMYCALL */
  {"NODES",    (void (*)) ccpnod,   NULL},
  {"NEWS",     (void (*)) NULL,     NULL},
#ifdef PARMS_PORTMOD
  {"PORT",     (void (*)) ccpport,  NULL},
#else
  {"PARMS",    (void (*)) ccppar,   NULL},
#endif
#ifdef PARMS_PORTMOD
  {"PARMS",    (void (*)) ccppar,   NULL},
#else
  {"PORT",     (void (*)) ccpport,  NULL},
#endif
#ifdef PACSAT
  {"PACSAT",   (void (*)) ccppacsat,NULL},
#endif
#ifdef PADDLE
  {"PADDLE",   (void (*)) ccppaddle,NULL},
#endif
#ifdef IPROUTE
  {"PING",     (void (*)) ccpping,  NULL},
#endif
  {"PROMPT",   (void (*)) ccpprompt,NULL},
#ifdef USERPROFIL
  {"PROFIL",   (void (*)) ccpprofil,NULL},
#endif /* USERPROFIL */
#ifdef PROFILING
  {"PROFILE",  (void (*)) ccp_profile,NULL},
#endif
  {"QUIT",     (void (*)) ccpquit,  NULL},
  {"ROUTES",   (void (*)) ccprou,   NULL},
  {"READ",     (void (*)) ccpread,  NULL},
  {"READBIN",  (void (*)) ccpreadb, NULL},
  {"RESET",    (void (*)) ccpres,   NULL},
  {"RUNBATCH", (void (*)) ccprun,   NULL},
  {"STAT",     (void (*)) ccpsta,   NULL},
#ifndef MC68302
  {"SHELL",    (void (*)) ccpshell, NULL},
#endif
#ifdef __LINUX__
  {"SETSHELL", (void (*)) ccpsetshell, NULL},
#endif
  {"SPARAM",   (void (*)) ccpsave,  NULL},
#ifdef SPEECH
  {"SPEECH",   (void (*)) ccpspeech,NULL},
#endif
  {"SYSOP",    (void (*)) ccpsys,   NULL},
#ifdef SYSOPPASSWD
  {"SYSPASS", (void (*)) ccppasswd ,NULL},
#endif /* SYSOPPASSWD */
  {"START",    (void (*)) ccpstart, NULL},
  {"SUSPEND",  (void (*)) ccpsusp,  NULL},
  {"TALK",     (void (*)) ccptalk,  NULL},
  {"TEST",     (void (*)) ccptst,   NULL},
#ifdef L1TELNET
  {"TELNET",   (void (*)) ccptelnet,NULL},
#endif /* L1TELNET */
  {"TIME",     (void (*)) ccptim,   NULL},
  {"TRACE",    (void (*)) ccptrace, NULL},
  {"USER",     (void (*)) ccpuse,   NULL},
  {"VERSION",  (void (*)) ccpver,   NULL},
#if defined(__LINUX__) && defined(SIXPACK)
  {"6PACK",    (void (*)) ccp6pack, NULL},
#endif
  {NULL,                  NULL,     NULL}
};

/*-------------------------------------- Befehlstabelle fuer Sysop-Kanal*/
COMAND syscmdtab[] = {
  {"SYSOP",    (void (*)) ccpsys,   NULL},
  {NULL,                  NULL,     NULL}
};

/*-------------------------------------------------------- L1 Portmodes */
L1MODETAB l1modetab[] =
{
#ifdef PORT_SYNRONATION
 {'s' , MODE_ss},
#endif
#ifdef PORT_SUSPEND
 {'l' , MODE_l},
#endif
 {'d' , MODE_d},
 {'c' , MODE_c},
 {'r' , MODE_r},
 {'t' , MODE_t},
 {'e' , MODE_e},
 {'m' , MODE_m},
 {'z' , MODE_z},
 {'\0', 0     }
};

/*----------------------------------------------------------- PP-CONVERSD */

/* Variable fuer conversd */
const char  *convtype = "conversd";
#ifdef CONVNICK
const char  *myfeatures =  "Admpun";
#else
 const char  *myfeatures =  "Admpu";
#endif
WORD        cvs_pc;                   /* Protokoll auf Kanal 32767 */
time_t      boottime;
time_t      currtime;
char        *myhostname;
char        myrev[10];
char        timestamp[16];
CONNECTION  *connections;
PERMLINK    *permarray[MAXCVSHOST];
DESTINATION *destinations;
CHANNEL     *channels;

/*----------------------------------- Befehlstabelle (BEFEHLSINTERPRETER) */
HOSTCMD hostcmdtab[] = {
  {"@",  extcmd},
  {"B",  Bcmd},
  {"C",  Ccmd},
  {"D",  Dcmd},
  {"E",  Ecmd},
  {"G",  Gcmd},
  {"I",  Icmd},
  {"J",  Jcmd},
  {"K",  Kcmd},
  {"L",  Lcmd},
  {"M",  Mcmd},
  {"Q",  Qcmd},
  {"R",  Rcmd},
  {"S",  Scmd},
  {"T",  Tcmd},
  {"Y",  Ycmd},
  {"V",  Vcmd},
  {0,    NULL}
};

time_t start_time;
time_t clear_time;

FILE  *consfile;

ULONG bytecnt;
UWORD crctab[256];              /* crc-Tabelle fuer program_load()       */
UWORD crc;                      /* crc fuer program_load()               */
ULONG checksum;                 /* checksumme fuer program_load()        */

/* Variable fuer L7.C */
ULONG lastic;

/* Variable fuer L7MONI.C */
WORD stamp;

/* Variable fuer TIMER.C */
volatile ULONG tic10;

/* Variable fuer Crash_Debugging */
#if defined(CRASHDEBUG) || defined(__LINUX__)
char *wowarich;
char *wowarich2;
#endif

BOOLEAN startup;
BOOLEAN tnnb_aktiv;
ULONG MEMORY_NEEDED = 0x2ff00L / sizeof(MAX_BUFFER);
time_t sys_time;
struct tm *sys_localtime;

#ifdef BEACON_STATUS
UWORD statustim  = 300L;
#endif

#ifdef AXIPR_UDP
BOOLEAN LookAX25IP = FALSE;                   /* Wir durfen keine Frame lesen. */
#endif /* AXIPR_UDP */

#ifdef DEBUG_MODUS
char    lastbuf[80];
char   *lastfunc = lastbuf;
#endif /* DEBUG_MODUS */

/* End of src/global.c */
