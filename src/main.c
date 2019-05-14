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
/* File src/main.c (maintained by: ???)                                 */
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

#include <assert.h>

#ifdef __GO32__
#define ptrdiff_t int
#endif

extern int console_login_status;

static void     initvar(void);
static void     main_loop(void);
static void     mainf(void);
#ifdef SETPATH
static void     SetPath(void);
#endif /* SETPATH */

static void initvar(void)
{
  int       port, i;
  PORTINFO  *p;
  BEACON   *b;

  memset(mh, 0, sizeof(STAT)*MAXSTAT);

  memset(portstat, 0, sizeof(PORTSTAT)*L2PNUM);

  for (port = 0, p = &portpar[0], b = &beacon[0];
       port < L2PNUM;
       port++, p++, b++)
  {
    sprintf(p->name, "Port%u", port);          /* Portnamen eintragen */

    p->mtu = 256;

    p->l1mode =
    p->l2mode = 0;
    p->l1_tx_timer = 0;
    p->txdelay = 25;
    p->persistance = 128;
    p->slottime = 10;
#ifdef PORT_MANUELL
    p->paclen = 128;
    p->IRTT   = 200;
    p->T2     = 10;
    p->T3     = 18000;
#else
    p->IRTT = 500;
    p->T2 = 150;
#endif /* PORT_MANUELL */

#ifdef IPOLL_FRAME
    p->ipoll_paclen = 128;
    p->ipoll_retry  = 3;
#endif /* IPOLL_FRAME */

#ifdef PORT_L2_CONNECT_TIME
    p->l2_connect_time = 120;
#endif
#ifdef PORT_L2_CONNECT_RETRY
    p->l2_connect_retry = 3;
#endif

#ifdef AUTOROUTING
    p->poAuto = L_NOROUTE;
#endif /* AUTOROUTING */

#ifdef THENETMOD
    p->broadcast = broint_ui;       /* Timer fuer Broadcast-Nodes Bake. */
#endif /* THENETMOD */

    p->maxframe = 2;
#ifdef EAX25
    p->maxframe_eax = 16;                      /* Standard: Maxframe 16 */
    p->eax_behaviour = 1;                      /* Mode nach MHEARD      */
#endif
    p->retry = 15;

#ifdef SETTAILTIME
#ifdef MC68302
    p->tailtime = TAILTIME*10;
#else
    p->tailtime = TAILTIME;
#endif
#endif

#ifdef EXPERTPARAMETER
    p->l2autoparam = ~0L;
#endif

    p->nmblks =
    p->nmbstn = 0;

#ifdef DAMASLAVE
    p->damaok = 0;
    p->sendok = 0;
#endif

#ifdef USERMAXCON
    p->maxcon = 0;
#endif

#ifdef PACSAT
    pacsat_enabled[port] = FALSE;
#endif
    cpyid(b->beades, "QST   \140");
    b->beadil[0] = b->text[0] = NUL;
    b->interval = b->beatim = b->telemetrie = 0;
  }

  for (i = 0; i < MAXSUSPEND; i++)
    *sustab[i].call = NUL;

#ifdef GRAPH
  graphclear();
#endif
}


void memerr(void) {
  hputs("*** ERROR: Not enough Memory!\n");
  exit(10);
}

int main(int argc, char *argv[])
{
#if defined(__GO32__)
  static char *line;
  char drive[MAXDRIVE];
  char path[MAXDIR];
  char name[MAXFILE];
  char ext[MAXEXT];
#endif

#ifdef __WIN32__
  SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE );
#endif
  /* Hardwarespezifische Sachen (Uhr nachstellen, UTC ins Enviroment,   */
  /* Kommandozeile auswerten steht jetzt in init_hardware().            */

  if (init_hardware(argc, argv))        /* Hardwarespezifische Sachen   */
    exit(1);

  time(&start_time);                    /* Startzeit merken             */
  clear_time = start_time;

  /**********************************************************************/
  /* Speicher besorgen                                                  */
  /**********************************************************************/

  VMSG("--- Allocating mem for LinkTable ...\n");
  lnktbl = (LNKBLK *) malloc (sizeof(LNKBLK) * (LINKNMBR ));
  VMSG("--- Allocating mem for CircuitTable ...\n");
  cirtab = (CIRBLK *) malloc (sizeof(CIRBLK) * (NUMCIR   ));
  VMSG("--- Allocating mem for PatchcordTable ...\n");
  ptctab = (PTCENT *) malloc (sizeof(PTCENT) * (NUMPAT   ));
  VMSG("--- Allocating mem for HostTable ...\n");
  hstubl = (HOSTUS *) malloc (sizeof(HOSTUS) * (MAXHST   ));
#ifdef L1TCPIP
  VMSG("--- Allocating mem for TcpipTable ...\n");
  tcptbl = (TCPIP *)  malloc (sizeof(TCPIP)  * (MAXTCPIP ));
#endif /* L1TCPIP */

  if (lnktbl == NULL ||
      cirtab == NULL ||
      ptctab == NULL ||
#ifdef L1TCPIP
      tcptbl == NULL ||
#endif /* L1TCPIP */
      hstubl == NULL
     ) memerr();

  VMSG("--- Initializing variables ...\n");
  initvar();                            /* Variablen Initialisieren     */

  /**********************************************************************/
  /* Configuration laden                                                */
  /**********************************************************************/
  *confpath = NUL;
#if defined(__GO32__)
  fnsplit(argv[0],drive,path,name,ext); /* Pfad zu TNN.EXE ermitteln,   */
  strcpy(confpath,drive);               /* dort auch die .cfg Dateien!  */
  strcat(confpath,path);
  strcpy(exename, argv[0]);             /* Name der EXE merken          */
  normfname(exename);                   /* normieren                    */
#endif

  /* wird aus dem aktuellen Verzeichniss gestartet, dann ist das der    */
  /* ConfPath (Dose gibt "" zurueck !)                                  */
#ifndef MC68302                                      /* DL1HAZ 01.02.95 */
  if (*confpath == NUL)              /* sonst wie Atari                 */
    getcwd(confpath,MAXPATH);        /* Aktuelles Verzeichnis, Suchpfad */
  addslash(confpath);
#endif

  if (load_configuration() == FALSE)    /* Configuration laden          */
     hprintf("*** WARNING: Error reading %s.PAS ***\n",cfgfile);
  if (load_stat() == FALSE)             /* Statistik laden              */
     hprintf("*** WARNING: Error reading %s.STA ***\n",cfgfile);
#ifdef GRAPH
  if (load_graph() == FALSE)             /* Graphen laden               */
     hprintf("*** WARNING: Error reading GRAPH.STA ***\n");
  graph.enabled = FALSE;                 /* wird erst nach einer Minute */
                                         /* auf TRUE gesetzt            */
#endif

#ifdef SETPATH
  SetPath();
#endif /* WIN32 */

#ifdef MC68302
  strcpy(confpath, textpath);
  strcpy(msgpath, textpath);
#else

  xchdir(textpath);                     /* Aktuelles Laufwerk und Pfad  */
#if defined(__GO32__)
  line = malloc(MAXPATH);               /* Pfad zu den Konfigurations-  */
  strcpy(line,"CONFPATH=");             /* dateien                      */
  strcat(line,confpath);
  if (putenv(line) == -1)
    printf("*** WARNING: Enviroment full ***\n");
  line = malloc(MAXPATH);               /* Temporaerpfad umleiten       */
  strcpy(line,"TMP=");
  strcat(line,textpath);
  if (putenv(line) == -1)
    printf("*** WARNING: Enviroment full ***\n");
#ifdef __GO32__
  line = malloc(MAXPATH);               /* Temporaerpfad umleiten        */
  strcpy(line,"TMPDIR=");
  strcat(line,textpath);
  if (putenv(line) == -1)
    printf("*** WARNING: Enviroment full ***\n");
#endif
#endif
  if (getenv("MSGPATH") == NULL) {
#if defined(__GO32__)
     line = malloc(MAXPATH);
     strcpy(line,"MSGPATH=");            /* MSGPATH auf TEXTPATH setzten */
     strcat(line,textpath);
     if (putenv(line) == -1)
       printf("*** WARNING: Enviroment full ***\n");
#endif
#if defined(__WIN32__)
     strcpy(msgpath, textpath);          /* MSGPATH auf TEXTPATH setzten */
     strcat(msgpath, "msg");
     addslash(msgpath);
#endif /* WIN32 */
  }
  else
   {
    strcpy(msgpath, getenv("MSGPATH"));
    addslash(msgpath);
   }

#endif

#ifndef MC68302
  init_rs232();                         /* Schnittstellen               */
  init_host();                          /* Host-Schnittstelle           */
  init_timer();                         /* Timer initialisieren         */
#else
  init_host();                          /* Host-Schnittstelle           */
  init_timer();                         /* Timer initialisieren         */
  mem_init();
#endif

  /**********************************************************************/
  /* Software initialisieren                                            */
  /**********************************************************************/

  VMSG("--- Initializing Level 1 ...\n");
  l1init();                             /* Level 1 initialisieren       */
  VMSG("--- Initializing Level 2 ...\n");
  l2init();                             /* L2                           */
  VMSG("--- Initializing Level 3 ...\n");
  l3init();                             /* L3                           */
  VMSG("--- Initializing Level 4 ...\n");
  initl4();                             /* L4                           */
  VMSG("--- Initializing Level 7 ...\n");
  initl7();                             /* L7                           */
#ifdef PACSAT
  VMSG("--- Initializing PACSAT ...\n");
  pacsat_init();                        /* PACSAT-Broadcast             */
#endif
#ifdef IPROUTE
  VMSG("--- Initializing IP-router ...\n");
  ipinit();                             /* IP-Router initialisieren     */
#endif
#ifdef L1TCPIP
  VMSG("--- Initializing L1TCPIP ...\n");
  InitTCP();                            /* TCPIP Initialisieren.        */
#endif /* L1TCPIP */
  VMSG("--- Initializing MHeard ...\n");
  init_mh();                            /* MHEARD Liste laden           */
#ifdef SPEECH
  speech_init();                         /* Sprache initialisieren       */
#endif

  VMSG("*** Entering main loop ...\n");
  mainf();                              /* ab in die Hauptschleife      */

  return(0);                            /* hierhin kommen wir niemals!  */
}

#ifdef ST
char *wowarich;
#endif

/************************************************************************/
/* die Hauptschleife                                                 ---*/
/*----------------------------------------------------------------------*/
static void main_loop(void)
{
  prof_start(PROF_SUM);

  PROFILE(PROF_L2,"l2",l2);
   PROFILE(PROF_L3RX,"l3rx",l3rx);
     PROFILE(PROF_L7RX,"l7rx",l7rx);
      PROFILE(PROF_TIM,"timsrv",timsrv);
#ifdef PACSAT
      PROFILE(PROF_PAC,"pacsrv",pacsrv);
#endif
#ifdef IPROUTE
      PROFILE(PROF_IP,"ipserv",ipserv);
#endif
#ifdef TCP_STACK
      PROFILE(PROF_STACK,"StackSRV",StackSRV);
#endif /* TCP_STACK */
#ifdef L1TCPIP
      PROFILE(PROF_TCPIP,"TcpipSRV",TcpipSRV);
#endif /* L1TCPIP */
      PROFILE(PROF_L4,"l4rest",l4rest);
      PROFILE(PROF_L3,"l3rest",l3rest);
     PROFILE(PROF_L7TX,"l7tx",l7tx);
   PROFILE(PROF_L4TX,"l4tx",l4tx);
  PROFILE(PROF_L3TX,"l3tx",l3tx);
  PROFILE(PROF_L1,"l1rxtx",l1rxtx);
  prof_stop(PROF_SUM);

#ifndef MC68K
  PROFILE(PROF_UPD,"timer",update_timer);
#endif
}

/************************************************************************/
/* das Programm persoenlich...                                       ---*/
/*----------------------------------------------------------------------*/
static void mainf(void)
{
#ifdef ST
  ULONG t;
#endif

  hputs(signon);
  hputid(myid);
  hprintf(")\r"
          "Copyright by NORD><LINK e.V. (c)1998 - 2008\r"
          "This version is compiled for %d Ports. "
          "Maximum %d L2-Links and %d Circuits.\r"
#ifdef PPCONVERS
          "conversd %5.5s-pp (c) dc6iq/dk5sg, TNN-Version by DL1XAO\r%s\r",
          L2PNUM, LINKNMBR, NUMCIR, strchr(REV, ':')+2,author);
#else
          "\r", L2PNUM, LINKNMBR, NUMCIR);
#endif

#ifdef AXIPR_UDP
  /* Sperre setzen. */
  LookAX25IP = TRUE;
#endif /* AXIPR_UDP */

  VMSG("--- Entering startup-loop ...\n");

  while (startup_running)
  {
      l1rxtx();
#ifndef MC68K
      update_timer();
#endif
      hostsv();
      hostim();
      l7rx();
      l7tx();
      shellsrv();             /* fuer externe Programme in der tnb */
      dealml((LEHEAD*)&rxfl); /* waehrend dem Start-Batch sind wir taub */
  }

  VMSG("--- Startup done.\n");

#ifdef ST
  t = tic10 + 500;
  while (t > tic10) /* zusaetzliche 5s Ruhe fuer den Tokenring */
  {
      l1rxtx();
      hostsv();
      hostim();
      l7rx();
      l7tx();
      dealml((LEHEAD*)&rxfl);
  }
#endif

#ifdef AXIPR_UDP
  /* Sperre zuruecksetzen. */
  LookAX25IP = FALSE;
#endif /* AXIPR_UDP */
  VMSG("*** All done. Here we go ...\n\n");

  hputs("password: ");

/* Die Start-TNB wurde geladen, jetzt koennen wir anfangen */
  LOOP
    main_loop();
}

/************************************************************************/
/* Programm Ende, aufraeumen                                            */
/************************************************************************/
void quit_program(int exit_code)
{
#ifdef SPEECH
  printf(speech_message(280));
#else
  printf("Programm wird beendet!\n");
#endif
  if (   console_login_status
      || exit_code == -1
#ifdef ST
      || exit_code == 2
#endif
     ) {
    exit_mh();                          /* MH-Liste sichern             */
    save_stat();
#ifdef GRAPH
    save_graph();
#endif
#ifndef MC68302                         /*hier nur noch auf Anforderung */
#ifndef DEBUG
    if (nmbfre >= 600)                  /* nur wenn genug Buffer da !   */
       save_parms();                    /* Braucht 500 Buffer!          */
#endif
    personalmanager(SAVE,NULL,NULL);    /* Pers. Daten fuer Convers     */
#ifdef USERPROFIL
    SaveProfil();
#endif /* USERPROFIL */
    exit_timer();                       /* Timer in Ursprungszustand    */
    l1exit();                           /* L1 zuruecksetzen!            */
    exit_hardware();                    /* Andere Hardware in Ursprungs.*/
    exit_host();                        /* Console schliessen           */
#else
    personalmanager(SAVE,NULL,NULL);    /* Pers. Daten fuer Convers     */
    exit_hardware();                    /* Andere Hardware in Ursprungs.*/
#endif

#ifdef ALIASCMD
        /* Aliasliste loeschen */
        clean_aliaslist();
#endif

        /* Netzwerk deregistieren */
        unregister_network();

    /* Speicher freigeben */
    free (lnktbl);
    free (cirtab);
    free (ptctab);
    free (hstubl);
#ifdef L1TCPIP
    free (tcptbl);
#endif /* L1TCPIP */

    /* und tschuess.... */
    exit(exit_code);
  }
}

#ifdef SETPATH
/* System Pfade setzen. */
static void SetPath(void)
 {
#ifdef __LINUX__
  mode_t mode = 0775;
#endif /* LINUX */
#ifdef __GO32__
  mode_t mode = S_IWUSR;
#endif /* GO32 */

  strcpy(textpath, confpath);     /* TEXTPATH setzen */
  strcpy(textcmdpath, confpath); /* TEXTCMDPATH setzen */
  strcat(textcmdpath, "textcmd");
  addslash(textcmdpath);
#if defined(__LINUX__) || defined(__GO32__)
  mkdir(textcmdpath, mode);      /* Verzeichnis textcmd erstellen */
#else
  mkdir(textcmdpath);            /* Verzeichnis textcmd erstellen */
#endif /* LINUX / GO32 */

  strcpy(userexepath, textpath); /* USEREXEPATH setzen */
  strcat(userexepath, "userexe");
  addslash(userexepath);
#if defined(__LINUX__) || defined(__GO32__)
  mkdir(userexepath, mode);      /* Verzeichnis USEREXE erstllen */
#else
  mkdir(userexepath);            /* Verzeichnis USEREXE erstllen */
#endif /* LINUX / GO32 */

  strcpy(sysopexepath, textpath);/* SYSOEXEPATH setzen */
  strcat(sysopexepath, "sysexe");
  addslash(sysopexepath);
#if defined(__LINUX__) || defined(__GO32__)
  mkdir(sysopexepath, mode);     /* Verzeichnis SYSEXE erstllen */
#else
  mkdir(sysopexepath);           /* Verzeichnis SYSEXE erstllen */
#endif /* LINUX / GO32 */

  strcpy(msgpath, textpath);     /* MSGPATH setzen    */
  strcat(msgpath, "msg");
  addslash(msgpath);
#if defined(__LINUX__) || defined(__GO32__)
  mkdir(msgpath, mode);          /* Verzeichnis MSG erstllen */
#else
  mkdir(msgpath);                /* Verzeichnis MSG erstllen */
#endif /* LINUX / GO32 */

#ifdef PACSAT
  strcpy(pacsatpath, textpath);  /* PACSATPATH setzen */
  strcat(pacsatpath, "pacsat");
  addslash(pacsatpath);
#if defined(__LINUX__) || defined(__GO32__)
  mkdir(pacsatpath, mode);       /* Verzeichnis PACSAT erstllen */
#else
  mkdir(pacsatpath);             /* Verzeichnis PACSAT erstllen */
#endif /* LINUX / GO32 */
#endif /* PACSAT */

#ifdef SPEECH
  strcpy(speechpath, textpath);  /* SPEECHPATH setzen    */
  strcat(speechpath, "speech");
  addslash(speechpath);
#if defined(__LINUX__) || defined(__GO32__)
  mkdir(speechpath, mode);       /* Verzeichnis SPEECH erstllen */
#else
  mkdir(speechpath);             /* Verzeichnis SPEECH erstllen */
#endif /* LINUX / GO32 */
#endif /* SPEECH */
}
#endif /* SETPATH */

/* End of src/main.c */
