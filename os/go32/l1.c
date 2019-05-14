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
/* File os/go32/l1.c (maintained by: ???)                               */
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

#include "hardware.h"                   /* Hardwaredefinition           */
#include "pc.h"

/* Die Tokenring-Variablen befinden sich jetzt hier, damit oberhalb des */
/* L1 keine Defines mehr notwendig sind. Dies ist ein vertretbarer      */
/* Overhead, wenn man keinen Tokenring hat.                             */

int   tkcom = -1;                       /* Tokenring-Schnittstelle      */
UWORD tkbaud = 96;                      /* Baudrate Schnittstelle       */

LONG  rounds_pro_sec = 0;               /* Anzahl Rounds/Sekunde        */
LONG  rounds_max_sec = 0;               /* Maximale Anzahl              */
LONG  rounds_min_sec = 0;               /* Minimale Anzahl              */
LONG  rounds_count   = 0;               /* Zaehler fuer Rounds          */

static ULONG cd_timer[L2PNUM];   /*  0 =  Frame wurde gesendet..          */
                                 /*  != 0 Frame an TNC ubergeben, wird    */
                                 /*       bearbeitet bzw. Sendung laeuft. */
#define CD_TIMEOUT 3000          /* 30Sekunden Timeout, falls Bestaetigung*/
                                 /* verloren gegangen ist (Tokenring)     */

#define BLOCKSIZE 2048           /* maximale Framegroesse                 */
static char *blkbuf;             /* Buffer fuer Blocktransfer             */
#define TX_CHAR(x) *out++ = x    /* ein Zeichen in den Puffer             */
#define TX_BEG(); out = blkbuf;  /* den Puffer initialisieren             */

void   register_majors(void);

/* Tokenring und Kisslink teilen sich diese Definitionen, deshalb         */
/* stehen die auch gleich hier.                                           */

#define FEND    0xC0             /* Kiss Frame Ende                       */
#define FESC    0xDB             /* Kiss Frame Escape                     */
#define TFEND   0xDC             /* transponiertes Frame Ende             */
#define TFESC   0xDD             /* transponiertes Escape                 */

#define WFEND 0                  /* warten auf FEND                       */
#define GPORT 1                  /* Portnummer lesen                      */
#define GTYPE 2                  /* Daten oder Parameterframe?            */
#define GFRAM 3                  /* Frame empfangen                       */
#define GFRMT 4                  /* TFESC gesendet, Byte einfuegen        */
#define TOKEN 5                  /* Token empfangen                       */
#define TOKN2 6                  /* Token gueltig, Daten und neues Token  */
#define RESET 7                  /* reset empfangen                       */
#define DAMAP 8                  /* DAMA PTT empfangen                    */

/* Geraeteverwaltung major/minor                                          */
/* Jedem MAC-Treiber wird dynamisch eine major-Nummer zugeteilt. Wenn es  */
/* zwischen weiteren minors (Untergeraete) unterscheiden moechte, muss    */
/* eine eigene l1istome-Routine diese aus dem String auslesen (z.B.       */
/* KISS3). Die Default-Routine vergleicht nur den Namen (z.B. VANESSA).   */
/* Wenn keine minors explizit verwaltet werden, findet ein direktes       */
/* Mapping Port->Minor statt.                                             */

static MAJOR majortab[MAX_MAJOR+1]; /* Tabelle der MAC-Treiber            */
static int num_major = 0;

/* Default-Handler. Diese koennen die MAC-Treiber-Handler ersetzen oder   */
/* ergaenzen, indem der Aufruf durchgereicht wird.                        */

/* Geraet-Initialisierung.                                                */
/* Das Geraet und alle Minors werden vorinitialisiert. Diese Funktion     */
/* wird beim Systemstart nach erfolgreicher registrierung aufgerufen.     */
static void default_l1init(void)
{
}

/* Geraet-Deinitialisierung.                                              */
/* Beim Runterfahren des Systems wird diese Routine aufgerufen.           */
#define default_l1exit default_l1init

/* Geraet-Behandlung                                                      */
/* Hier koennen staendige Aufgaben erledigt werden, z.B. die Interrupt-   */
/* Buffer leeren und Frames an den L2 schicken. Nur hier darf auf L1-     */
/* Strukturen zugegriffen werden, nie aus dem Interrupt.                  */
#define default_l1handle default_l1init

/* Geraet-Konfiguration                                                   */
/* Hier werden Konfigurations/Test/Reset-Anfragen bearbeit.               */
static void
default_l1ctl(int req, int port)
{
  PORTINFO *p = &portpar[port];

  if (p->reset_port)
    portstat[port].reset_count++;
  p->reset_port =
  commandflag[port] =
  testflag[port] = FALSE;
}

/* Geraet-DCD                                                             */
/* Wenn moeglich sollte das Geraet hier seinen tatsaechlichen DCD-Status  */
/* liefern. Es duerfen keine Interpretationen (z.B. Vollduplex) geschehen,*/
/* das macht der L2 selbst.                                               */
static WORD default_l1dcd(PORTINFO *port)
{
  return(0);
}

/* Feststellen, ob wir mit dem L1-MAC-Namen gemeint sind. Es wird         */
/* immer minor 0 geliefert (nicht geeignet fuer KISS usw).                */
static int default_l1istome(int major, char *devname)
{
  if (strnicmp(majortab[major].name, devname, strlen(devname))==0)
    return(0);
  return(NO_MINOR);
}

/* Geraet-Timerbehandlung.                                                */
static void default_l1timer(UWORD ticks)
{
}

/* Geraet anschliessen                                                    */
/* Ein Geraet und Minor an einen Port anschliessen. Dies muss             */
/* nicht immer Erfolg haben. Der Minor wurde mittels dem jeweiligen       */
/* l1istome ermittelt.                                                    */
/* Ist checkonly TRUE, dann soll nur geprueft werden, ob ein Eintrag      */
/* moeglich WAERE, aber NICHTS ausgefuehrt werden.                        */
static int default_l1attach(int port, int unused_minor, BOOLEAN unused_checkonly)
{
  return(1);
}

/* Geraet-Abschalten                                                      */
/* Einen Port abschalten, wenn das Geraet eine Trennung zulaesst.         */
static int default_l1detach(int unused_port)
{
  return(1);
}

/* Geraet-Informationen                                                   */
/* what=HW_INF_IDENT sollte der Geraetename geliefert werden. Wenn das    */
/*                   Geraet mehrere hat (z.B. KISS/SMACK/RKISS), soll es  */
/*                   den aktuellen liefern (benutzt von SPARAM).          */
/* what=HW_INF_INFO  sollte max. 10 Zeichen Geraetekonfiguration liefern, */
/*                   im Normalfalle auch der Name, eventuell mit etwas    */
/*                   mehr Info.                                           */
/* what=...          sollte spaeter mal hsbus_stat() u.ae. ersetzen       */
static void
default_l1info(int what, int port, MBHEAD *mbp)
{
  switch (what)
  {
    case HW_INF_IDENT :
    case HW_INF_INFO :
      putstr(majortab[portpar[port].major].name, mbp);
  }
}

/* MAC-Treiber registrieren. Es werden die default-Handler eingetragen,   */
/* nur die benoetigten Routinen muessen vom Treiber ersetzt werden. Jeder */
/* Treiber sollte vor der Registrierung pruefen, ob ueberhaupt ein Geraet */
/* im System vorhanden ist und sich ggfs garnicht installieren. Dies      */
/* beschleunigt den allgemeinen Ablauf im L1.                             */
static MAJOR *register_major(void)
{
  MAJOR *m;

  if (num_major > MAX_MAJOR)
    return(NULL);
  m = &majortab[++num_major];

  m->istome = default_l1istome;
  m->init   = default_l1init;
  m->exit   = default_l1exit;
  m->handle = default_l1handle;
  m->ctl    = default_l1ctl;
  m->dcd    = default_l1dcd;
  m->attach = default_l1attach;
  m->detach = default_l1detach;
  m->info   = default_l1info;
  m->timer  = default_l1timer;

  return(m);
}

/* Einen Port mit einem MAC-Treiber verbinden. Falls der Port bereits   */
/* belegt ist, wird vorher l1detach() aufgerufen. Dieser darf aber auch */
/* fehlschlagen (wenn der Port nicht umkonfigurierbar ist, z.B. Host-   */
/* terminal). Der Geraetetreiber setzt nur seinen minor, den major      */
/* setzt l1attach selbst (der Geraetetreiber kennt eigentlich seinen    */
/* major nicht).                                                        */
int l1attach(int port, char *devname)
{
  int            major;
  int            minor;
  PORTINFO *p = &portpar[port];

  /* als erstes suchen wir einen passenden major */
  for (major = 1; major <= num_major; major++)
    if ((minor = majortab[major].istome(major, devname)) != NO_MINOR)
      break;

  if (major > num_major)
    return(0); /* Geraet unbekannt */

  if (!majortab[major].attach(port, minor, TRUE/*check only*/))
    return(0); /* erstmal nur antesten */

  /* hier angekommen ist das neue Geraet bereit, ein eventuell altes      */
  /* schalten wir aus.                                                    */

  if (p->major != NO_MAJOR)
    l1detach(port);

  if (p->major == NO_MAJOR) { /* nur wenn Abschaltung erfolgreich war */
    if (majortab[major].attach(port, minor, FALSE/*attach*/)) {
      p->major = major;
      return(1);
    }
  }

  return(0); /* Port belegt / nicht belegbar */
}

/* Einen Port von einem MAC-Treiber trennen. Dies darf fehlschlagen,     */
/* wenn z.B. der L1 anderweitig konfiguriert wurde (Linux).              */
void l1detach(int port)
{
  int     major;
  MBHEAD *mbp;
  LHEAD  *mlp;

  major = portpar[port].major;
  if (major != NO_MAJOR)
    if (majortab[major].detach(port))
      portpar[port].major = NO_MAJOR;
  if (portpar[port].major == NO_MAJOR) {
    mlp = &txl2fl[port];
    while ((LHEAD *)mlp->head != mlp) {
      mbp = (MBHEAD *)ulink((LEHEAD *)mlp->head);
      relink((LEHEAD *)mbp, (LEHEAD *)stfl.tail);
    }
    kick[port] = FALSE;
  }
}

/************************************************************************/
/* Initialisierung der Variablen und der Hardware                       */
/************************************************************************/

void l1init(void)
{
  MAJOR    *m;
  int       i;
  PORTINFO *p;

  for (i=0, p = portpar; i<L2PNUM; i++, p++)
  {
    p->reset_port = FALSE; /* Tnx an Odo */
    p->speed = 12;
    p->major = NO_MAJOR;
    p->minor = -1;
    cd_timer[i] = 0;
    kick[i] = FALSE;            /* kein TNC sendet              */
    testflag[i] = FALSE;
    commandflag[i] = FALSE;
  }

  blkbuf = malloc(BLOCKSIZE);
  if (blkbuf == NULL) memerr();

  show_recovery = TRUE;

  register_majors();

  for (m = &majortab[1]; m <= &majortab[num_major]; m++)
    m->init();

  l1sclr("*");
}

/************************************************************************/
/* Layer 1 deinitialisieren                                             */
/*----------------------------------------------------------------------*/
void l1exit (void)
{
  MAJOR *m;

  for (m = &majortab[1]; m <= &majortab[num_major]; m++)
    m->exit();
}

/************************************************************************/
/* Timer-Behandlung fuer den L1                                         */
/*               wird aus timsrv() herraus aufgerufen                   */
/*----------------------------------------------------------------------*/
void l1timr(UWORD ticks)
{
  MAJOR *m;

  for (m = &majortab[1]; m <= &majortab[num_major]; m++)
    m->timer(ticks);
}

/************************************************************************/
/* Level1 RX/TX                                                         */
/*               wird staendig in der main() Hauptschleife aufgerufen.  */
/*----------------------------------------------------------------------*/
void l1rxtx(void)
{
  MAJOR *m;
  int    major;

  rounds_count++;              /* Anzahl der Hauptschleifendurchlaeufe  */
  for (m = &majortab[major = 1]; major <= num_major; major++, m++)
    m->handle();
}

/************************************************************************/
/* Markieren, dass fuer TNC #port Frames zum Senden vorliegen           */
/*----------------------------------------------------------------------*/
void kicktx(int port)
{
  kick[port] = TRUE;
}

/************************************************************************/
/*                                                                      */
/*      iscd(): Carrier Detect                                          */
/*                                                                      */
/* Ueberpruefung des Kanalstatus fuer einen Port. Wenn auf einem        */
/* Kanal der Sender getastet ist, wird 2 geliefert. Wenn auf dem        */
/* Kanal Daten empfangen werden, wird 1 geliefert. 0 wird geliefert,    */
/* wenn der Kanal frei ist, oder auf Vollduplex geschaltet ist.         */
/*                                                                      */
/* Hinweis: Einige Geraete unterstuetzen diese Funktion nur ungenau     */
/*          (z.B. Tokenring)                                            */
/*                                                                      */
/*----------------------------------------------------------------------*/
WORD iscd(int port)
{
  int       state = 0;
  int       major;
  PORTINFO *p;

  p     = portpar+port;
  major = p->major;
  if (major != NO_MAJOR) {
    if ((LHEAD *)txl2fl[port].head != &txl2fl[port])
      state |= TXBFLAG;
    return(state | majortab[major].dcd(p));
  }
  return(state);
}

/************************************************************************/
/* Level1 Control                                                       */
/*               eine Aktion fuer einen Hardware-Port nachfragen        */
/*----------------------------------------------------------------------*/
void l1ctl(int req, int port)
{
  int major;

  switch(req) {
    case L1CRES: portpar[port].reset_port = TRUE;
                 break;
    case L1CCMD: commandflag[port] = TRUE;
                 break;
    case L1CTST: testflag[port] = TRUE;
  }

  major = portpar[port].major;
  if (major != NO_MAJOR)
    majortab[major].ctl(req, port);
}

/* Infostring fuer den Port-Befehl zusammenbauen.                       */
void l1hwstr(int port, MBHEAD *mbp)
{
  if (portpar[port].major == NO_MAJOR)
    putstr("OFF", mbp);
  else
    majortab[portpar[port].major].info(HW_INF_INFO, port, mbp);
}

/* Infostring fuer SAVEPARM zusammenbauen.                              */
void l1hwcfg(int port, MBHEAD *mbp)
{
  if (portpar[port].major == NO_MAJOR)
    putstr("OFF", mbp);
  else
    majortab[portpar[port].major].info(HW_INF_IDENT, port, mbp);
}

/*
 * Blocktransferroutinen fuer den Level 1
 * Message-Buffer koennen Blockweise in einen linearen Buffer umgewandelt
 * werden und wieder zurueck. Dies darf aber so ohne weiteres nur im
 * Level 1 erfolgen, da der Buffer zurueckgespult wird.
 */

static int cpymbflat(char *buf, MBHEAD *fbp) {
  MB    *bp;
  LHEAD *llp = &fbp->mbl;           /* Zeiger auf den Listenkopf        */
  int    i   = fbp->mbpc;           /* Anzahl der Bytes im Frame        */

  for (bp = (MB *)llp->head; bp != (MB *)llp;
       bp = bp->nextmb, i -= sizeof_MBDATA, buf += sizeof_MBDATA)
    memcpy(buf, bp->data, sizeof_MBDATA);
  return(fbp->mbpc);
}

static MBHEAD *cpyflatmb(char *buf, int size) {
  MBHEAD *mbhd;
  MB     *bp;
  LHEAD  *llp;

  mbhd = (MBHEAD *)allocb(ALLOC_MBHEAD); /* einen Buffer fuer den Kopf  */
  mbhd->mbpc = size;                /* soviel wird mal drinstehen       */
  llp = &mbhd->mbl;                 /* Zeiger auf den Listenkopf        */
  for ( ; size > 0; size -= sizeof_MBDATA, buf += sizeof_MBDATA) {
    memcpy((bp = (MB *)allocb(ALLOC_MB))->data, buf, sizeof_MBDATA);
    relink((LEHEAD *)bp, (LEHEAD *)llp->tail);
  }
  rwndmb(mbhd);                     /* mbbp richtig setzen              */
  return(mbhd);
}

#ifdef VANESSA
 #include "vanessa.c"
#endif

#ifdef EXTDEV
 #include "extdev.c"
#endif

#ifdef TOKENRING
 #include "tokenrng.c"
#endif

#ifdef COMKISS
 #include "kiss.c"
#endif

#ifdef SCC
 #include "scc.c"
#endif

#ifdef PAR96
 #include "par.c"
#endif

#include "loop.c"

#ifdef TCP_STACK
 #include "tcp.c"
#endif /* TCP_STACK */

/* Geraetetabelle.                                                      */
/* In dieser Tabelle werden alle Geraete registriert. Bezuege aus L1.C  */
/* auf Variablen der Treiber sind verboten.                             */
/* Die Reihenfolge hier bestimmt auch die Reihenfolge bei der STAT-     */
/* Ausgabe.                                                             */
DEVTABLE devtable[] =
{
  REGISTER_DEVICE("LOOPBACK", register_loopback),
#ifdef TOKENRING
  REGISTER_DEVICE("TOKENRING", register_tokenring),
#endif
#ifdef COMKISS
  REGISTER_DEVICE("KISS", register_kiss),
#endif
#ifdef VANESSA
  REGISTER_DEVICE("VANESSA", register_vanessa),
#endif
#ifdef EXTDEV
  REGISTER_DEVICE("EXTDEV", register_extdev),
#endif
#ifdef SCC
  REGISTER_DEVICE("SCC", register_scc),
#endif
#ifdef PAR96
  REGISTER_DEVICE("PAR96", register_par),
#endif
#ifdef TCP_STACK
  REGISTER_DEVICE("TCPIP", RegisterTCP),
#endif /* TCP_STACK */
  REGISTER_DEVICE(NULL, NULL)
};

/* Hier werden alle majors nacheinander gebeten, sich zu registrieren.  */
/* Sie muessen es aber nicht, falls z.B. keine Schnittstellen frei oder */
/* vorhanden sind.                                                      */
void register_majors(void)
{
  DEVTABLE *t;

  for (t = devtable; t->reg_func; t++)
    t->major = t->reg_func();
}

/* Aufzaehlen der vorhandenen Layer 1 Geraete. Dies sind nicht die      */
/* tatsaechlich installierten, sondern die compilierten.                */
void l1enum(MBHEAD *mbp)
{
  DEVTABLE *t;

  for (t = devtable; t->reg_func; t++) {
    putstr(t->major ? " *" : " ", mbp);
    putstr(t->name, mbp);
  }
}

/* Information zu einem Geraet abrufen (Statistik).                    */
void l1stat(const char *devname, MBHEAD *mbp)
{
  DEVTABLE *t;

  for (t = devtable; t->reg_func; t++)
    if (strcmp(t->name, devname) == 0 || *devname == '*') /* Name stimmt? */
      if (t->major)
        majortab[t->major].info(HW_INF_STAT, 0, mbp);
}

/* Portstatistik loeschen                                              */
void l1sclr(const char *devname)
{
  DEVTABLE *t;

  for (t = devtable; t->reg_func; t++)
    if (strcmp(t->name, devname) == 0 || *devname == '*') /* Name stimmt? */
      if (t->major)
        majortab[t->major].info(HW_INF_CLEAR, 0, NULL);
}

/* End of os/go32/l1.c */
