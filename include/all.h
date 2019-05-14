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
/* File include/all.h (maintained by: you)                              */
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

/************************************************************************/
/* Aktivierung einzelner Elemente in TNN                                */
/************************************************************************/

#define IPROUTE                       /* DB7KG's IP-Router              */
#define PACSAT                        /* PACSAT-Server                  */
#define PPCONVERS                     /* PP-Convers einbauen            */
#define GRAPH                         /* Graphische Statistiken         */
#define PORTGRAPH                     /* Portspezifische Statistiken    */
#define USER_PASSWORD                 /* Userpassword Option von SQ2FRB */
/*#define FLEXHOST*/                    /* FlexNet durchmelden zu NET/ROM */
#define MAXFRAMEDEBUG                 /* zusaetzliche Anzeige bei Trace */
#define AUTO_UPDATE                   /* Automatisch CFG-Files updaten  */
#define KERNELIF                      /* Interface zum Linuxkernel      */
#define ALIASCMD                      /* Kommandoaliasse definierbar    */
#define EAX25                         /* Extended-AX.25 (modulo 128)    */
#define USERMAXCON                    /* Connectanzahl-Limitierung      */
#define SIXPACK                       /* 6PACK-Ring (vorerst nur Linux) */
#define NEW_L4                        /* neuer L4 mit PID-Transport     */
#define HDLC_DCDPTTSTAT               /* DCD/PTT-Info bei HDLC-Devices  */
#define SCC_DCDPTTSTAT                /* DCD/PTT-Info bei SCC-Devices   */
/*#define PCISCC4_KAX25*/             /* PCISCC4 unter Kernel 2.4.x mit */
/*#define OBU_SCC_DCD*/               /* mod. SCC-Treiber mit DCD       */
/*#define LOOPBACK*/                  /* Loopback-Funktion              */
                                      /* F6FBB-Treiber (KEIN KJD-Kernel)*/

/*#define SETTAILTIME*/               /* TX-Tailtime setzbar            */
/*#define DAMASLAVE*/                 /* DAMA-Slave-Modus               */
                                      /* noch unvollstaendig!           */
/*#define L2PROFILER*/                /* Spielzeug fuer DB7KG           */
/*#define L3TABDEBUG*/                /* L3-Nodetabellen-Debugging      */
#define EXPERT                        /* EXPERTEN-Modus, mehr Paras     */
#define BUFFER_DEBUG                  /* Bufferanalyse fuer Fehlersuche */
/*define INSANE_BUFFER_DEBUG*/        /* noch viel mehr Bufferanalyse   */
/*#define CVS_ZAPPING*/               /* Diagnosetool fuer Convers      */
/*#define PROFILING*/                 /* Programm-Profiling-Tool        */
#define NO_WATCHDOG                   /* Linux: kein Watchdog           */
#define CONL3LOCAL                    /* Connect L3LOCAL verfeinert.    */
#define CONVNICK                      /* Convers mit Nickname-Support   */
#define EXPERTPARAMETER               /* Port-Autoparameter schaltbar   */


/************************************************************************/
/*                                                                      */
/* Ab hier darf NICHTS mehr geaendert werden! Also FINGER WEG !!!       */
/* DO NOT change below the point ! Keep your HANDS OFF !                */
/*                                                                      */
/************************************************************************/

/* MIPS bekommt keinen Watchdog, die Systeme haben soweiso schon wenig */
/* Speicher, da muss nicht auch noch der Watchdog mit laufen */
/* PACSAT fliegt ebenfalls raus */
/* SCC-Sachen auch raus, da die Hardware nicht verfuegbar ist */
#ifdef MIPS
#define NO_WATCHDOG
#undef PACSAT
#undef SCC_DCDPTTSTAT
#undef OBU_SCC_DCD
#endif

#define MAX_TRACE_LEVEL 9             /* max. verfuegbarer Trace Level  */

/* PORTGRAPH braucht GRAPH */
#ifdef PORTGRAPH
#define GRAPH
#endif

/************************************************************************/
/* Deaktivierung einzelner Elemente wenn fuer Zielsystem unmoeglich     */
/* (Diese Definitionen duerfen nicht editiert werden !!!)               */
/************************************************************************/

#ifndef __LINUX__
#undef KERNELIF
#undef PCISCC4_KAX25
#ifndef __WIN32__
#undef SIXPACK
#endif
#undef HDLC_DCDPTTSTAT
#undef SCC_DCDPTTSTAT
#undef OBU_SCC_DCD
#endif

#ifdef __WIN32__
/* Kein Watchdog */
#define NO_WATCHDOG
#endif /* WIN32 */

#ifdef __GO32__
#undef OS_STACK
#undef OS_IPLINK
#undef SPEECH
#endif /* __GO32__ */

#if defined(MC68K)
#undef PACSAT
#endif

/* Unser ganz privater Assert ******************************************/
#ifdef MC68302
  #define HALT(x) {dbg(x); reboot_system();}
#else
  #define HALT(x) {xprintf("Halted by " x); reboot_system();}
#endif

/* Unser ganz privater Debugger ****************************************/
#define dbg(x)
#if defined(ST) || defined(__LINUX__)
#undef dbg
#define dbg(x) wowarich=x
#endif
#ifdef MC68302
#undef dbg
#define dbg(x) *(char **)0x11CL=x
#endif

/************************************************************************/
/* Definition von Konstanten die in gesammten TNN genutzt werden.       */
/************************************************************************/

#define LOOP        for( ; ; )          /* Endlosschleife               */

#if defined (__GO32__) || defined(__LINUX__) || defined(__WIN32__)
#define LINKNMBR    400
#else
#define LINKNMBR    250                 /* maximale Anzahl Links        */
#endif
#define NUMCIR      200                 /* maximale Anzahl Circuits     */
#ifndef L1TCPIP
#define MAXHST      30+1                /* maximale Anzahl Hostkanaele  */
#define NUMPAT      (LINKNMBR+NUMCIR+MAXHST)
                                        /* Eintraege Patchcordliste     */
#else
#define MAXHST      31                  /* maximale Anzahl Hostkanaele  */
#define NUMPAT      (LINKNMBR+NUMCIR+MAXHST+MAXTCPIP)
                                        /* Eintraege Patchcordliste     */
#endif /* L1TCPIP */

#define DEFL2L      1
#define TAILTIME    3                   /* Tailtime fuer die KISS-TNCs  */

#define TOKENTIMEOUT 200                /* Timeout fuer Tokenring bis   */
                                        /* Token wieder da sein muss    */
                                        /* Achtung: Wert in 10ms-Ticks  */

#define L2PNUM      16                  /* Anzahl L2-Ports              */
#if L2PNUM > 16
#error "L2PNUM >16 is not tested!"
#endif

#define DAMA_CH     L2PNUM      /* Anzahl der Dama-Kanaele              */

#ifdef MC68302
# define MAXCOMS 3              /* Anzahl der seriellen Ports           */
#else
# define MAXCOMS 4
#endif
#define MAXKISS MAXCOMS         /* Anzahl der KISSLINKS                 */

#define MAXSUSPEND  50          /* Maximalanzahl Sperrungen             */
#define MAXCVSHOST  10          /* Maximaleintraege ConversHosts        */
#define MAXSTAT     16          /* Anzahl der Statistik-Eintraege       */
#define MAXNMBSTN   16          /* Quantisierung Stationen/Port         */

#define MINBUFF     256

#define NUL         ((char) 0x00)       /* ASCII-Zeichen                */
#define BELL        0x07
#define BS          0x08
#define TAB         0x09
#define LF          0x0A
#define CR          0x0D
#define XON         0x11
#define XOFF        0x13
#define ESC         0x1B
#define DEL         0x7F

#define MONI        0x01                /* Monitor: I-Frames            */
#define MONU        0x02                /*          UI-Frames           */
#define MONS        0x04                /*          S-Frames            */
#define MONC        0x08                /* Anzeige, auch wenn connected */
#define MONF        0x10                /* Anzeige des Info-Feldes      */
#define MONT        0x20                /* Sende-/Empfangszeit          */
#define MONL        0x40                /* Info-Laengen-Anzeige         */

#define SECONDS_PER_DAY   86400L   /* the number of seconds in one day  */
#define SECONDS_PER_HOUR   3600L   /*  "    "    "    "      "  "  hour */
#define SECONDS_PER_MIN      60L   /*  "    "    "    "      "  "  min. */

#ifdef CRASHDEBUG
#define TRACE(x) wowarich2 = "x"
#else
#define TRACE(x)
#endif

/************************************************************************/
/*                Einiges fuer TNC3                                     */
/************************************************************************/
#ifdef MC68302
#define __BOOLEAN
#include <apbind.h>

#undef stdout             /* stdout Simulation */
extern FILE *stdout;

#define MAXPATH   20

#define xchdir(a);

#endif

/************************************************************************/
/* hier noch einige Sachen fuer PP-conversd                             */
/************************************************************************/

#define MAXCHANNEL      32767          /* hoechster conversd Kanal      */

#if defined(__TURBOC__) || defined(__STDC__) || defined(__WIN32__)
#define __ARGS(x)       x
#ifndef __DOTS
#define __DOTS          ,...
#endif
#else
#define __ARGS(x)       ()
#define const
#ifndef __DOTS
#define __DOTS
#endif
#endif

#if !defined (min)
#define min(a,b) ((a) >= (b) ? (b) : (a))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

/*#define uchar(x) ((x) & 0xff) wers braucht, solls anmachen, DL1XAO*/
#define uchar(x) (x)

#ifdef CONVNICK
#define REV "$Revision: 3.14c $"
#else
 #define REV "$Revision: 3.12c $"
#endif

#define INIT 0   /* Befehle fuer personalmanager und convers_config */
#define SAVE 1
#define SET  2
#define GET  3

/************************************************************************/
/* Dateitrennungszeichen usw fuer das Filesystem festlegen              */
/************************************************************************/
#define SEPARATORS "\\/"            /* die DOS- und die Unix-Konvention */

                                    /* File-Flags                       */
#define FF_LWR 1                    /* Dateinamen sind immer klein      */
#define FF_TXT 2                    /* Unterscheidung TEXT/BIN bei open */

#ifdef __LINUX__
#define FILE_SEP '/'                /* Linux und falcOS haben /         */
#define FILE_FLAGS FF_LWR
#define NO_DISKDRIVE                /* keine Laufwerksbuchstaben        */
/* die folgenden Pfade koennen auch ueber einen Compilerswitch im       */
/* makefile definiert werden                                            */
#ifndef TEXTPATH
#define TEXTPATH "/usr/local/tnn/"
#endif
#ifndef TEXTCMDPATH
#define TEXTCMDPATH TEXTPATH "textcmd/"
#endif
#ifndef USEREXEPATH
#define USEREXEPATH TEXTPATH "userexe/"
#endif
#ifndef SYSEXEPATH
#define SYSEXEPATH TEXTPATH "sysexe/"
#endif
#ifndef MSGPATH
#define MSGPATH TEXTPATH "msg/"
#endif
#ifdef SPEECH
#ifndef SPEECHPATH
#define SPEECHPATH TEXTPATH "speech/"
#endif
#endif
#ifdef PACSAT
#ifndef PACSATPATH
#define PACSATPATH TEXTPATH "pacsat/"
#endif
#endif
#ifdef AXIPR_HTML
#ifndef HTMLPATH
#define HTMLPATH "/usr/local/httpd/htdocs/"
#endif
#endif
#define STRIPCHR CR
#define ENDCHR LF
#define PORTABLE
#else /* nicht __LINUX__ */
#define FILE_SEP '\\'               /* DOS und ST haben das alte \      */
#define STRIPCHR LF
#define ENDCHR CR
#ifndef MC68302
#define FILE_FLAGS FF_TXT
#ifndef TEXTPATH
#define TEXTPATH "TNN\\"
#endif
#ifndef TEXTCMDPATH
#define TEXTCMDPATH TEXTPATH "TEXTCMD\\"
#endif
#ifndef USEREXEPATH
#define USEREXEPATH TEXTPATH "USEREXE\\"
#endif
#ifndef SYSEXEPATH
#define SYSEXEPATH TEXTPATH "SYSEXE\\"
#endif
#ifdef PACSAT
#ifndef PACSATPATH
#define PACSATPATH TEXTPATH "PACSAT\\"
#endif
#endif
#ifdef AXIPR_HTML
#ifndef HTMLPATH
#define HTMLPATH TEXTPATH
#endif
#endif
#ifndef MSGPATH
#define MSGPATH TEXTPATH "MSG\\"
#endif
#ifdef SPEECH
#ifndef SPEECHPATH
#define SPEECHPATH TEXTPATH "SPEECH\\"
#endif
#endif
#else /* MC68302 */
#define FILE_FLAGS FF_LWR
#define TEXTPATH "r:\\"
#define TEXTCMDPATH TEXTPATH
#define USEREXEPATH TEXTPATH
#define SYSEXEPATH TEXTPATH
#define MSGPATH TEXTPATH
#endif
#endif

#define GRAPH_LINES        15
#define GRAPH_INTERVAL     10 /* Alle 10 s einen neuen Wert speichern   */
#define GRAPH_STD_ELEMENTS 60 /* 60 Elemente je Stunde                  */
#define GRAPH_DAY_ELEMENTS 48 /* 24 * 2 halbe Stunden je Tag            */
#define GRAPH_WEK_ELEMENTS 56 /* 8 * 7 Tagesabschnitte je Woche         */

#ifdef BUFFER_DEBUG
#define ALLOC_LEHEAD       1
#define ALLOC_MBHEAD       2
#define ALLOC_USRBLK1      3
#define ALLOC_USRBLK2      4
#define ALLOC_L2LINK       5
#define ALLOC_MB           6
#define ALLOC_MONBUF       7
#define ALLOC_CQBUF        8
#define ALLOC_IP_ROUTE     9
#define ALLOC_ARP_TAB     10
#define ALLOC_MHEARD      11
#define ALLOC_PACSATBLK   12
#ifdef USERPROFIL
#define ALLOC_USEPROF     13
#endif /* USEPROFIL. */
#ifdef TCP_STACK
#define ALLOC_TCPSTACK    14
#endif /* TCP_STACK. */
#ifdef L1TCPIP
#define ALLOC_L1TCPIP     15
#endif /* L1TCPIP */
#ifdef L1HTTPD
#define ALLOC_L1HTTPD_RX  16
#define ALLOC_L1HTTPD_TX  17
#endif /* L1HTTPD */
#ifdef L1IPCONV
#define ALLOC_L1IPCONV    18
#endif /* L1IPCONV */
#ifdef L1IRC
#define ALLOC_L1IRC       19
#endif /* L1IRC */
#define ALLOC_INPOPT      20
#define ALLOC_NO_OWNER    21
#define ALLOC_MAXELEMENTE 22
#else
#define ALLOC_LEHEAD
#define ALLOC_MBHEAD
#define ALLOC_USRBLK1
#define ALLOC_USRBLK2
#define ALLOC_L2LINK
#define ALLOC_MB
#define ALLOC_MONBUF
#define ALLOC_CQBUF
#define ALLOC_IP_ROUTE
#define ALLOC_ARP_TAB
#define ALLOC_MHEARD
#define ALLOC_PACSATBLK
#ifdef USERPROFIL
#define ALLOC_USEPROF
#endif /* USERPROFIL. */
#ifdef TCP_STACK
#define ALLOC_TCPSTACK
#endif /* TCP_STACK. */
#ifdef L1TCPIP
#define ALLOC_L1TCPIP
#endif /* L1TCPIP */
#ifdef L1HTTPD
#define ALLOC_L1HTTPD_RX
#define ALLOC_L1HTTPD_TX
#endif /* L1HTTPD */
#ifdef L1IPCONV
#define ALLOC_L1IPCONV
#endif /* L1IPCONV */
#ifdef L1IRC
#define ALLOC_L1IRC
#endif /* L1IRC */
#define ALLOC_INPOPT
#define ALLOC_NO_OWNER
#define ALLOC_MAXELEMENTE
#endif

/* End of include/all.h */

