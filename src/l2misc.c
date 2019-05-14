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
/* File src/l2misc.c (maintained by: DF6LN)                             */
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

static void l2rest(void);
static void inilbl(void);

/*----------------------------------------------------------------------*/
/*                                                                      */
/* "level 2"                                                            */
/*                                                                      */
/* Der Level 2. Es werden alle Level-2-internen Aktionen ausgefuehrt    */
/* und Meldungen an hoehere Level weitergegeben (Informationstransfer   */
/* von/zum Level 2 und Kommandos an den Level 2 geschehen von ausser-   */
/* halb).                                                               */
/*                                                                      */
/* Der Level 2 laeuft wie folgt ab :                                    */
/*                                                                      */
/*  - Aufruf von l2init()                                               */
/*                                                                      */
/*  - zyklisches Aufrufen von l2()                                      */
/*                                                                      */
/*  Statusaenderungen im Level 2 (Connects, Disconnects, Failures, usw) */
/*  werden hoeheren Leveln vom Level 2 aus ueber                        */
/*                                                                      */
/*   l2tolx(<status>)  ->  l2tol3(<status>), l2tol7(<status>,lnkpoi,2)  */
/*                                                                      */
/*  mitgeteilt.                                                         */
/*                                                                      */
/*  Ein Connectwunsch wird dem Level 2 ueber das Besetzen eines leeren  */
/*  Linkblocks mit Quell- und Ziel- sowie Digicalls und Aufrufen von    */
/*  newlnk() mitgeteilt (lnkpoi zeigt auf Linkblock !).                 */
/*  Ein newlnk() auf einen bestehenden Link erzeugt einen Link Reset.   */
/*                                                                      */
/*  Ein Disconnectwunsch (oder bei mehrmaligem Aufruf der sofortige     */
/*  Disconnect) wird ueber das Setzen von lnkpoi auf den jeweiligen     */
/*  Linkblock und Aufruf von dsclnk() erreicht.                         */
/*                                                                      */
/*  Der Informationstransfer zum Level 2 geschieht von aussen durch     */
/*  Aufruf von itolnk(...), vom Level 2 durch itolx(..), welches dann   */
/*  fmlink() aus dem hoeheren Level aufruft.                            */
/*                                                                      */
/*  Ueber sdui(..) koennen unproto-Pakete (UI-Frames) gesendet werden.  */
/*                                                                      */
/*  Level-3-Pakete (Level-3-UI-Pakete oder Infopakete in Sequenz eines  */
/*  Level-2-3-Links) werden ausgefiltert und in die Level-3-Frameliste  */
/*  eingehaengt.                                                        */
/*                                                                      */
/*----------------------------------------------------------------------*/

/* Die Reihenfolge der Bearbeitung ist hier entscheidend. Zuerst werden */
/* die empfangenen Frames verarbeitet, dann werden I-Frames erzeugt und */
/* erst dann wird ueber T1/T2 entschieden.                              */

void
l2(void)
{
  clrstfl();
  l2rx();
  l2tx();
  l2timr();
  l2rest();
}

/*----------------------------------------------------------------------*/
void
l2init(void)
{
  WORD i;

  inithd(&rxfl);
  inithd(&stfl);
  inithd(&trfl);
  inithd(&freel);
  inithd(&l2frel);

  init_buffers();

  for (i = 0; i < L2PNUM; ++i)
  {
    inithd(&txl2fl[i]);
    inithd(&l2actl[i]);
  }

  for (i = 0, lnkpoi = lnktbl; i < LINKNMBR; ++i, ++lnkpoi)
  {
    lnkpoi->state = L2SDSCED;
    lnkpoi->rcvd = lnkpoi->tosend = 0;
    lnkpoi->tmbp = NULL;
    lnkpoi->zael = 1;                                   /* MultiConnect */
    inithd(&lnkpoi->rcvdil);
    inithd(&lnkpoi->sendil);
    inilbl();
    reslnk();
    relink((LEHEAD *)lnkpoi, (LEHEAD *)l2frel.tail);
  }

  iniDAMA();
  for (i = 0; i < L2CALEN; i++)
    l2als[i] = toupper(alias[i]);
}

void
autopar(int port)
{
  PORTINFO *p = portpar + port;
  UWORD     baud;               /* Baudrate/100 auf dem Port            */
  UWORD     P;                  /* Persistance                          */
  UWORD     W;                  /* Slottime                             */
  UWORD     IRTT;               /* Initial RTT                          */
  UWORD     T2;                 /* Timer2                               */
  UWORD     N2;                 /* Retry                                */
  BOOLEAN   l1update;

/* T2 wird berechnet als die Laenge der Aussendung eines Frames bei     */
/* der eingestellten Baudrate + 10%.                                    */
  baud = p->speed;

  T2 = 1;                               /* sehr schneller Link          */
  if ((baud != 0) && (baud < 2888))
    T2 = 2888 / baud;                   /* sonst adaptiv nach Baudrate  */

  if (dama(port))
    T2 <<= 1;                           /* *2 bei DAMA nach DG3AAH      */

/* Der IRTT wird auf T2*2 festgelegt, dies sollte immer
 * ausreichen, damit eine Antwort kommt. Es wird noch der TXDELAY
 * dazugerechnet, in der Annahme die Gegenstation habe etwa einen
 * gleich schnellen Transceiver.
 */
  IRTT = (T2 + p->txdelay) * 2;
  IRTT = max(10, IRTT);

/* Der Retry ist DEF_N2 bei DUPLEX/CSMA und DEF_N2/2 bei DAMA.          */
  N2 = DEF_N2;
  if (dama(port))
    N2 /= 2;

/* P-PERSISTENCE
 * Bei DAMA und VOLLDUPLEX wird P immer auf 255 gesetzt.
 * Bei einem normalen Einstieg ohne DAMA oder Halb-Duplex
 * verwenden wir defaultmaessig P=160
 */
  P = 255;

/* Persistence und Slottime bei Linkports anders setzen */
  if (!fullduplex(port))
  {
    if (islinkport(port))
      P = 200;
    else if (!dama(port))
      P = 160;                                  /* Idee DB2OS und DG9FU */
  }

/* Als Slottime wird der aktuelle TX-Delay Wert genommen.               */
  W = p->txdelay;
  if (W == 0)
    W++;

  l1update = (P != p->persistance)
    || (W != p->slottime);

#ifdef EXPERTPARAMETER
  /* Neue Werte nur bei Autoparameter setzen */
  if (p->l2autoparam & MODE_apers)  p->persistance = P;
  if (p->l2autoparam & MODE_aslot)  p->slottime = W;
  if (p->l2autoparam & MODE_aIRTT)  p->IRTT = IRTT;
  if (p->l2autoparam & MODE_aT2)    p->T2 = T2;
  if (p->l2autoparam & MODE_aretry) p->retry = N2;
#else
  p->persistance = P;                           /* Parameter neu setzen */
  p->slottime = W;
  p->IRTT = IRTT;
  p->T2 = T2;
  p->retry = N2;
#endif

  if (l1update)                         /* neue L1-Parameter setzen     */
    l1ctl(L1CCMD, port);
}

void
autopers(int port)
{
#ifdef DAMASLAVE
   PORTINFO *p = portpar + port;

   if (damaslaveon(port))               /* wenn DAMA erkannt, dann      */
   {
     p->persistance = 255;              /* sofort antworten             */
     p->slottime = 0;                   /* keine Wartezeit              */
     l1ctl(L1CCMD, port);               /* Aenderungen zum TNC bringen  */
   }
#endif

#if 0                                           /* DB2OS will es so ... */
  PORTINFO *p = portpar + port;
  UWORD     P;                                  /* Persistance          */
  BOOLEAN   l1update;

/* Persistance wird berechnet als 255-(Anzahl der User)*10. Bei         */
/* Vollduplex und DAMA wird die Persistance immer auf 255 gesetzt.      */
  P = 255;
  if ((p->nmbstn > 0) && !fullduplex(port) && !dama(port))
    if (p->nmbstn <= 15)
      P = 255 - (p->nmbstn * 10);
    else
      P = 100;

  l1update = (P != p->persistance);

  p->persistance = P;                   /* Parameter neu setzen         */

  if (l1update)                         /* neue L1-Parameter setzen     */
    l1ctl(L1CCMD, port);
#endif
}

#ifdef PORT_SYNRONATION
static int check_syn_port(int port)
{
  int i     = 0;
  int found = 0;

  /* alle Ports pruefen. */
  for(i = 0; i < L2PNUM; i++)
  {
    /* Ist Port aktiv? */
    if (!portenabled(i))
      /* Port ist ausgeschaltet, */
      /* zum naechsten Port.     */
      continue;

    /* Den eignen Port nicht pruefen*/
    if (port == i)
      continue;

    if (port_synronation_enabled(i))
    {
      /* Port mit Synronation gefunden. */
      /* Pruefe den Port auf DCD/PTT.   */
      found = iscd(i);
      continue;
    }
  }
  return(found);
}
#endif

/* Ist ein Port gerade aktiv ? (Blockierung des Senders und der Timer   */
BOOLEAN
busy(int port)
{
  if (fullduplex(port))
    return ((iscd(port) & (RXBFLAG | TXBFLAG)) != 0);
  else
#ifndef PORT_SYNRONATION
    return ((iscd(port) & (DCDFLAG | PTTFLAG | RXBFLAG | TXBFLAG)) != 0);
#else
    if (port_synronation_enabled(port))
    {
      if (check_syn_port(port) || (iscd(port) & (DCDFLAG | PTTFLAG | RXBFLAG | TXBFLAG)))
        return(TRUE);
      else
        return(FALSE);
    }
    return ((iscd(port) & (DCDFLAG | PTTFLAG | RXBFLAG | TXBFLAG)) != 0);
#endif
}

/************************************************************************\
*                                                                        *
* "level 2 rest"                                                         *
*                                                                        *
* Muellbufferliste frei machen (aus Interruptroutinen entstandener       *
* Muell, der besser ausserhalb der Interrupts deallokiert wird aus       *
* Zeitgruenden).                                                         *
*                                                                        *
\************************************************************************/
static void
l2rest(void)
{
  dealml((LEHEAD *)&trfl);      /* Muellbufferliste frei machen         */
}

#ifdef L2PROFILER
void
l2profiler(void)
{
  MBHEAD *mbp;

  if (dmagic == MAGIC_L2PROFILE)
  {                                     /* im Level 2 ordentlich Krach  */
    while (   lnkpoi->tosend < 10       /* machen                       */
           && !(lnkpoi->flag & (L2FDSLE | L2FDIMM)))
    {
      mbp = (MBHEAD *)allocb(ALLOC_MBHEAD);
      while (mbp->mbpc < 256)
        putchr(' ', mbp);
      rwndmb(mbp);
      i3tolnk(0x12, lnkpoi, mbp);
    }
  }
}
#endif

#ifndef MC68K
/************************************************************************/
/*                                                                      */
/* "is to me"                                                           */
/*                                                                      */
/* Ist das uebergebene Rufzeichen unser myid oder alias? Der Alias kann */
/* mit beliebiger SSID connected werden.                                */
/*                                                                      */
/************************************************************************/
BOOLEAN
istome(const char *id)
{
  return (   cmpid(myid, id)            /* Stimmt Rufzeichen?           */
          || cmpcal(l2als, id));        /* oder ALIAS?                  */
}
#endif

/************************************************************************/
/*                                                                      */
/* "is to me or via me?"                                                */
/*                                                                      */
/* Pruefen, ob wir als naechster an der Reihe sind mit Digipeaten. Dazu */
/* schauen wir uns das erste Call an, das kein gesetztes Hbit hat, dass */
/* muessen wir selber sein oder wir schmeissen das Frame weg,           */
/* andernfalls duerfen wir das Frame auswerten.                         */
/*                                                                      */
/* Rueckgabewerte : 0 = nicht via uns und auch nicht fuer uns           */
/*                  1 = gedigipeated/kein via, und Verbindung an uns    */
/*                  2 = Verbindung via, aber NICHT an uns               */
/* TEST DG9OBU      3 = Verbindung an unseren SSID-Bereich/Local        */
/*                  4 = AX.25-ARP an uns                                */
/************************************************************************/
int
istomev(void)
{
  char *viap = rxfhdr + L2ILEN;                /* Zeiger auf erstes Via */
  char *savedviap = viap;  /* Zur Sicherheit irgendwo hin zeigen lassen */

  int iViaNum = -1;
  int iViaNumMax = 0;

  while (*viap)
  {
    ++iViaNumMax;
    if (!(viap[L2IDLEN - 1] & L2CH))
    {
      if (iViaNum == -1)
      {
        savedviap = viap;
        iViaNum = iViaNumMax;
      }
    }
    viap += L2IDLEN;
  }

  if ((iViaNumMax > 0) && (iViaNum > -1))
  {
    if (iViaNum < iViaNumMax)
    {
      /* Wir sind nicht das letzte Via in der Liste */
      return (istome(savedviap) ? 2 : 0);            /* Verbindung via uns?  */
    }
    else
    {
      /* Wir sind das letzte Via in der Liste */
      if (istome(savedviap))
      {
        return((cmpid(rxfhdr, "QST   \140")) ? 5 : 2);
      }
      else
      {
        return(0);
      }
    }
  }

/* Hier angekommen haben entweder alle schon gedigipeated oder es gibt  */
/* keine VIAs, auf jeden Fall duerfen wir den Link nehmen, wenn er      */
/* direkt an uns (MyCall ODER Node-Alias) gerichtet ist.                */
  if (istome(rxfhdr))
    return 1;

/* TEST DG9OBU */
/* Verbindung nicht direkt an mich, sondern in meinen SSID-Bereich */
  if (    (cmpcal(rxfhdr, myid))        /* hat mein Call, SSID egal */
       && (SSIDinrange(SSID(rxfhdr)))   /* liegt in meinem SSID-Bereich */
     )
    return 3;

#ifdef PROXYFUNC
  if (isproxy(rxfhdr))
    return 2;
#endif

#ifdef IPROUTE
  /* AX.25-ARP fuer mich ? */
  /* (bzw. an QST und alle in der VIA-Liste haben schon gedigipeated */
  /* oder keine VIA-Liste vorhanden)                                 */
  if (cmpid(rxfhdr, "QST   \140"))
    return 4;
#endif

  /* Verbindung ist nicht fuer uns */
  return 0;
}

/************************************************************************\
*                                                                        *
* "new link"                                                             *
*                                                                        *
* Link (lnkpoi) neu aufbauen.                                            *
*                                                                        *
\************************************************************************/
void
newlnk(void)
{
  reslnk();                     /* Sequenzvars/Timer ruecksetzen        */
  setiSRTT();                   /* RTT-Timer neu starten                */
  l2stma(stbl19);               /* LOCAL START COMMAND                  */
}

/************************************************************************\
*                                                                        *
* "disconnect link"                                                      *
*                                                                        *
* Disconnect-Wunsch an aktuellen Link (lnkpoi) :                         *
*                                                                        *
*   Linkstatus "Disconnected"                                            *
*   -> Ax.25-Parameter "frisch"                                          *
*                                                                        *
*   Linkstatus "Link Setup" oder "Disconnect Request"                    *
*   -> Link NICHT aufloesen, nur Flag vormerken fuer l2rest              *
*                                                                        *
*   sonst                                                                *
*   -> Empfangsinfoframeliste loeschen, Linkstatus bleibt, Flag "nach    *
*      Loswerden aller zu sendenden Infoframes disconnecten" setzen      *
*                                                                        *
\************************************************************************/
void
dsclnk(void)
{
  WORD lstate;

  if ((lstate = lnkpoi->state) == L2SDSCED)     /* Disced, nur          */
    inilbl();                                   /* AX-Pars neu          */
  else
  {
    dealml((LEHEAD *)&lnkpoi->rcvdil);          /* RX-Infoframe-        */
    lnkpoi->rcvd = 0;                           /* loeschen und         */

    if (   lstate == L2SLKSUP                   /* Linksetup oder       */
        || lstate == L2SDSCRQ                   /* Discreq,             */
        || lstate == L2SHTH                     /* oder wartet          */
        || (lnkpoi->flag & L2FDSLE))
      lnkpoi->flag |= L2FDIMM;                  /* sofort weg           */
    else
      lnkpoi->flag |= L2FDSLE;                  /* wenn alles raus      */
  }
}

/************************************************************************\
*                                                                        *
* "initialize link"                                                      *
*                                                                        *
* Aktuellen Linkblock (lnkpoi) initialisieren. Sequenzvariablen und      *
* Timer initialisieren, Quellcall/Zielcall/via-Liste/ Port setzen aus    *
* der txf...-Liste.                                                      *
*                                                                        *
\************************************************************************/
void
inilnk(void)
{
  reslnk();                                /* Sequenzvars/Timer init.   */
  cpyid(lnkpoi->srcid, txfhdr + L2IDLEN);  /* Quellcall                 */
  cpyid(lnkpoi->dstid, txfhdr);            /* Zielcall                  */
  cpyidl(lnkpoi->viaidl, txfhdr + L2ILEN); /* via-Liste                 */
  lnkpoi->liport = txfprt;                 /* Port                      */
  setiSRTT();                              /* RTT                       */
  lnkpoi->pollcnt = 0;                     /* Anzahl verbotener Polls   */
                                           /* des DAMA-Users            */
  lnkpoi->noatou = ininat;                 /* Timeout initialisieren    */
#ifdef EAX25
  if (lnkpoi->bitmask == 0x7F)
#ifdef __WIN32__
    lnkpoi->maxframe = (unsigned char)portpar[lnkpoi->liport].maxframe_eax;
#else
    lnkpoi->maxframe = portpar[lnkpoi->liport].maxframe_eax;
#endif /* WIN32 */
  else
#endif
#ifdef __WIN32__
    lnkpoi->maxframe = (unsigned char)portpar[lnkpoi->liport].maxframe;
#else
    lnkpoi->maxframe = portpar[lnkpoi->liport].maxframe;
#endif /* WIN32 */
}

/************************************************************************\
*                                                                        *
* "clear link"                                                           *
*                                                                        *
* Aktuellen Link (lnkpoi) aufloesen. Alle Sequenzvariablen und Timer     *
* zuruecksetzen, Sende- und Empfangsinfoframelise loeschen, Linkblock    *
* neu mit AX.25-Parametern besetzen.                                     *
*                                                                        *
\************************************************************************/
void
clrlnk(void)
{
  reslnk();                           /* Sequenzvars/Timer ruecksetzen  */
  dealml((LEHEAD *)&lnkpoi->rcvdil);  /* Empfangsinfoliste loeschen     */
  dealml((LEHEAD *)&lnkpoi->sendil);  /* Sendeinfoliste loeschen        */
  dealml((LEHEAD *)&lnkpoi->damail);  /* DAMA-Puffer loeschen           */
  lnkpoi->rcvd = lnkpoi->tosend = 0;  /* entsprechende Zaehler loeschen */
  if (lnkpoi->tmbp != NULL)           /* RX-Fragmente auch              */
  {
    dealmb(lnkpoi->tmbp);
    lnkpoi->tmbp = NULL;
  }
  inilbl();                           /* Linkblock "frisch"             */
}

/************************************************************************\
*                                                                        *
* "reset link"                                                           *
*                                                                        *
* Aktuellen Link (lnkpoi) zuruecksetzen. Alle Sequenzvariablen und Timer *
* initialisieren.                                                        *
*                                                                        *
\************************************************************************/
void
reslnk(void)
{
#ifdef __WIN32__
  lnkpoi->VS =     0;
  lnkpoi->VR =     0;
  lnkpoi->ltxdNR = 0;
  lnkpoi->lrxdNR = 0;
  lnkpoi->RTT =    0;
#else
  lnkpoi->VS =
  lnkpoi->VR =
  lnkpoi->ltxdNR =
  lnkpoi->lrxdNR =
  lnkpoi->RTT =
#endif /* WIN32 */
  lnkpoi->flag = 0;
  lnkpoi->noatou = ininat;
  resptc(g_uid(lnkpoi, L2_USER));
  clrDAMA();
  lnkpoi->SRTT = portpar[lnkpoi->liport].IRTT;
  clrT1();
  clrT2();
}

/************************************************************************\
*                                                                        *
* "initialize link block"                                                *
*                                                                        *
* Aktuellen Linkblock (lnkpoi) initialisieren.                           *
*                                                                        *
\************************************************************************/
static void
inilbl(void)
{
  lnkpoi->liport = 0;
  lnkpoi->bitmask = 0x07;
  clrDAMA();
}

/************************************************************************\
*                                                                        *
* "acknowledge link"                                                     *
*                                                                        *
* Diese Funktion wird vom L7 aufgerufen, um eine eingehende HTH-         *
* Verbindung zu bestaetigen. Es wird lediglich ein Flag gesetzt, in      *
* l2rest wird dann die eigentliche Reaktion ausgeloest.                  *
*                                                                        *
\************************************************************************/
void
acklnk(LNKBLK *lp)
{
  if (lp->state == L2SHTH)
    lp->flag |= L2FACKHTH;
}

/************************************************************************\
*                                                                        *
* "reject link"                                                          *
*                                                                        *
* Diese Funktion wird vom L7 aufgerufen, um eine eingehende HTH-         *
* Verbindung abzulehnen (Partner ist Busy).                              *
*                                                                        *
\************************************************************************/
void
rejlnk(LNKBLK *lp)
{
  if (lp->state == L2SHTH)
    lp->flag |= L2FREJHTH;
}

/************************************************************************\
*                                                                        *
* "get link"                                                             *
*                                                                        *
* Einen Link anhand port, srcid, dstid und vial suchen und liefern. Wenn *
* wir noch keinen Link haben, dann einen neuen (leeren) liefern.         *
*                                                                        *
\************************************************************************/
LNKBLK *
getlnk(UBYTE liport, char *srcid, char *dstid, char *viaidl)
{
  LHEAD  *llp;
  LNKBLK *lp;
  char   *p;

  if (!portenabled(liport))
    return(NULL);

  llp = &l2actl[liport];                /* auf dem entsprechenden Port  */

  for (lp = (LNKBLK *)llp->tail;
       lp != (LNKBLK *)llp;             /* alle Links des Ports pruefen */
       lp = lp->prev)
  {
    if (cmpid(lp->srcid, srcid))
      if (cmpid(lp->dstid, dstid)
          && cmpidl(lp->viaidl, viaidl))
      {
        ulink((LEHEAD *)lp);
        relink((LEHEAD *)lp, (LEHEAD *)llp->tail);
        return (lp);            /* wir haben einen Link gefunden        */
      }
  }

  if ((LHEAD *)l2frel.head == &l2frel)
      return (NULL);              /* wir haben keinen freien Link         */

  if (   (fvalca(dstid) != YES)
      || (nmbfre < 78))         /* falsches Call oder kein Speicher?    */
      return (NULL);

  lp = (LNKBLK *)l2frel.head;
  cpyid(lp->srcid, srcid);      /* Werte setzen                         */
  cpyid(lp->dstid, dstid);
  cpyidl(lp->viaidl, viaidl);

  lp->liport = liport;
/* In den Linkblock wird ein Zeiger auf das Rufzeichen abgelegt, das in */
/* Wirklichkeit der Ansprechpartner dieses Linkes ist. Es ist das Erste */
/* Rufzeichen im via-Feld ohne H-Bit oder das Ziel-Rufzeichen selbst.   */
  for (p = lp->viaidl; *p; p += L2IDLEN)
    if ((p[L2IDLEN - 1] & L2CH) == 0)
      break;
  lp->realid = *p ? p : lp->dstid;
  return (lp);                          /* leeren Block liefern         */
}

/************************************************************************/
/*                                                                      */
/* Maxframe-Automatik: Fuer den angegebenen Link wird Maxframe um den   */
/* Wert dif geaendert. Dabei wird der Bereich 1 .. Port-Maxframe        */
/* eingehalten. Zum Vergroessern des Link-Maxframe muss zweimal in      */
/* Folge alles bestaetigt sein.                                         */
/*                                                                      */
/************************************************************************/
void
change_maxframe(LNKBLK *link, int dif)
{
#if MAX_TRACE_LEVEL > 4
  char  notify_call1[10];
  char  notify_call2[10];
#endif
  int   old = link->maxframe;
  int   new_max;
  int   port_max;

#ifdef EAX25
  /* Fuer EAX.25-Links ein anderes Maxframe verwenden */
  if (link->bitmask == 0x7F)
    port_max = portpar[link->liport].maxframe_eax;
  else
#endif
    port_max = portpar[link->liport].maxframe;

  if (!automaxframe(link->liport))
  {
    link->maxframe = port_max;
    return;
  }

  if (dif > 0)                          /* soll vergroessert werden?    */
  {
    if (old == port_max)                /* schon Maximalwert, den Rest  */
      return;                           /* kann man sich sparen         */

    if (link->flag & L2FCMDEL)          /* 2. Durchgang in Folge?       */
      link->flag &= ~L2FCMDEL;
    else
    {
      link->flag |= L2FCMDEL;           /* 1. Durchgang - nix aendern   */
      dif = 0;
    }
  }
  else
    link->flag &= ~L2FCMDEL;

  new_max = old + dif;
  if (new_max < 1)
    new_max = 1;
  if (new_max > port_max)
    new_max = port_max;

  if ((new_max != old) | (link->flag & L2FCMDEL))
  {
    link->maxframe = new_max;
#if MAX_TRACE_LEVEL > 4
    call2str(notify_call1, link->srcid);
    call2str(notify_call2, link->dstid);
    notify(5, "Max %s - %s %d->%d",
           notify_call1, notify_call2, old, new_max);
#endif
  }
}

/************************************************************************/
/*                                                                      */
/* UI-Frames von einem Port auf einen anderen Port durchreichen. Diese  */
/* Funktion ist vorgesehen, um z.B. Mail-Beacons von Mailboxes zu       */
/* ermoeglichen. Als Digipeater-Call ist der TNN-Alias zu verwenden mit */
/* der Nummer des gewuenschten Sende-Ports als SSID. Fuer die Ueber-    */
/* tragung muessen folgende Voraussetzungen erfuellt sein:              */
/*                                                                      */
/*   - nur PID 0xF0 wird akzeptiert                                     */
/*   - der Sende-Port darf nicht der Empfangsport sein                  */
/*                                                                      */
/* Um fuer F6FBB-Mailboxes eine Antwort-Funktion zu ermoeglichen, wird  */
/* der SSID des Digipeater-Calls bei der Aussendung auf den Empfangs-   */
/* port gesetzt.                                                        */
/*                                                                      */
/************************************************************************/

void
gateway_ui(char *fmid, char *toid, char *rxvia, MBHEAD *info)
{
  int   port;
  char *vp;
  char  to[L2IDLEN];

  DEST  dst;

  BOOLEAN bCanDigipeat = FALSE;
  BOOLEAN bSuppressHBit = FALSE;

  /* Gesamte via-Liste durchgehen */
  for (vp = rxvia; *vp != '\0'; vp += L2IDLEN)
  {
    /* Hat dieser via-Digi schon gedigipeated ? Ja, dann zum naechsten */
    if (vp[L2IDLEN-1] & L2CH)
      continue;

    /* Mailbake fuer Mailbox-Systeme, ermoeglicht die Aussendung des */
    /* UI-Frames gezielt auf einem anderen Port */
    /* Unser *Alias* und PID F0 ? */
#ifndef UIDIGIMOD
    if ((cmpcal(vp, l2als)) && (info->l2fflg == L2CPID))
#else
    if (cmpcal(vp, myid))
#endif /* UIDIGIMOD */
    {
      /* Port holen, auf dem ausgesendet werden soll */
      port = SSID(vp);
      /* Pruefen, ob Einschraenkungen erfuellt */
#ifndef UIDIGIMOD
      if ((port < L2PNUM) && portenabled(port) && (port != rxfprt))
#else
      if ((port < L2PNUM) && portenabled(port))
#endif /* UIDIGIMOD */
      {
        /* Via umbauen (Empfangsport und digipeated-Flag eintragen) */
        vp[L2IDLEN-1] = (vp[L2IDLEN-1] & ~0x1E) | (rxfprt << 1) | L2CH;
#ifdef __WIN32__
        sdui(rxvia, toid, fmid, (char)port, info);  /* UI-Frame senden */
#else
        sdui(rxvia, toid, fmid, port, info);  /* UI-Frame senden */
#endif /* WIN32 */
        /* Frame loeschen */
        dealmb(info);
        return;
      }
    }

    /* UI-Digipeating fuer andere L2-Frames jeglicher PID. Das gewuenschte    */
    /* Ziel muss uns bekannt UND per L2 erreichbar sein, NETROM-Ziele koennen */
    /* per UI (noch) nicht erreicht werden, da schicken wir das UI-Frame im   */
    /* L2 weiter.                                                             */

    /* Sind wir der naechste Digi in der Via-Liste ? */
    if (cmpid(vp, myid))
    {
      char *tp = vp + L2IDLEN;     /* naechster Hop in der Liste (oder NUL) */

      /* Noch ein Digi nach uns in der via Liste ? */
      if (*tp != '\0')
      {
        /* Ja, dann feststellen, ob wir den kennen und wo wir lang muessen. */
        /* Es darf sich nur um ein Flexnet- oder lokales L2-Ziel handeln,   */
        /* via-Pfade mit Calls auf Userports werden ignoriert.              */

        /* Flexnet- oder lokales, erreichbares L2-Ziel ? */
        if (l3_find_route(tp, &dst) == NODE_AVAILABLE)
        {
          /* FlexNet, LOCAL und LOCAL_M */
/*
+          if (   (dst.typ == FLEXNET)
+              || (islocal(tp) == YES)
              )
+*/
              bCanDigipeat = TRUE;
        }
      }
      else
      {
        /* Keiner mehr nach uns in der Liste, dann schauen, ob wir das Ziel in der */
        /* MHeard-Liste kennen oder es ein lokaler Link von uns ist */
        cpyid(to, toid);  /* wir brauchen nur das erste Call */

        /* Lokaler User (MHeard) ? */
        if (isheard(to, &dst) == TRUE)
          bCanDigipeat = TRUE;
        else
        {
          /* Lokal angeschlossener Knoten ? */
          /* Ein erreichbarer Local oder Local ohne Messung */
          if (islocal(to) == YES)
          {
            /* Es ist ein erreichbarer Local, jetzt noch den Port holen */
            if (l3_find_route(to, &dst) == NODE_AVAILABLE)
              bCanDigipeat = TRUE;
          }
        }
      }
    }
    else    /* Wir sind eigentlich nicht dran, kennen aber einen Weg zum naechsten */
    {       /* Digi in der Liste ? Das machen wir aber nur fuer RX auf Linkports ! */
      if (    (l3_find_route(vp, &dst) == NODE_AVAILABLE)
           && (islinkport(rxfprt))
         )
      {
        /* FlexNet, LOCAL und LOCAL_M */
/*
        if (   (dst.typ == FLEXNET)
            || (islocal(vp) == YES)
           )
         {
*/
            bCanDigipeat = TRUE;
            bSuppressHBit = TRUE;   /* H-Bit nicht veraendern */
/*
         }
*/
      }
    }

    /* Frame digipeaten ? Zielport ok ? */
    if (   (bCanDigipeat == TRUE)
        && (portenabled(dst.port))
        && (dst.port >= 0)
        && (dst.port < L2PNUM)
       )
    {
      /* Wenn wir nicht mit in den Vias waren, dann HBit nicht anfassen */
      if (bSuppressHBit == FALSE)
        /* Wir haben gedigipeated */
        vp[L2IDLEN - 1] |= L2CH;

#ifdef __WIN32__
      sdui(rxvia, toid, fmid, (char)dst.port, info);  /* UI-Frame senden */
#else
      sdui(rxvia, toid, fmid, dst.port, info);  /* UI-Frame senden */
#endif /* WIN32 */
      /* Frame loeschen */
      dealmb(info);
      return;
    }

    /* Via-Bearbeitung beendet */
    break;
  }

  /* Frame loeschen */
  dealmb(info);
  return;
}

/* End of src/l2misc.c */
