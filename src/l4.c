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
/* File src/l4.c (maintained by: ???)                                   */
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

/* Layer 4 parameter ---------------------------------------------------*/

#define L4_ACKDEL    2          /* L4 Acknowledge Wartezeit fuer Circuit*/
#define L4_BSYDEL  180          /* L4 Busy-Wartezeit in Sekunden        */

UWORD   l4_beta1 = 3;           /* RETRANS-TIMER (T1) = SRTT * BETA1    */
UWORD   l4_beta2 = 1;           /* ACK-TIMER (T2) = SRTT * BETA2        */
UWORD   l4_beta3 = 20;          /* BUSY/REQ-TIMEOUT (T3) = SRTT * BETA3 */
#define L4_BETA1 (l4_beta1)
#define L4_BETA2 (l4_beta2)
#define L4_BETA3 (l4_beta3)

#define L4_ALPHA1    2          /* Faktor fuer steigenden L4RTT         */
#define L4_ALPHA2    3          /* Faktor fuer fallenden L4RTT          */
#define L4_IRTT  1000U          /* Startwert SRTT (IRTT)                */

#define L4_RETRY     3          /* Anzahl der maximalen Retries         */

/*----------------------------------------------------------------------*/

LHEAD l4frel;                   /* freie L4-Controllbloecke             */
LHEAD l4actl;                   /* benutzte L4-Controllebloecke         */

/*----------------------------------------------------------------------*/

static void l4rtt(CIRBLK *, int);
static void l4setT1(CIRBLK *, MBHEAD *);
static void l4setT2(CIRBLK *);
static void l4setT3(CIRBLK *, int);
static void l4clrT3(CIRBLK *);
static void l4newstate(UWORD);
static void clr4rx(BOOLEAN);
static void chksts(void);
static void takfrm(MBHEAD *);
static void sndfrm(int, MBHEAD *);
static void l4nsta(WORD);
static void endcir(void);
static void clrcir(void);
static void sconrq(void);
static void sdisrq(void);
#ifdef NEW_L4
static void spidchg(UBYTE);
#endif
static void sndack(void);
static void ackhdr(void);
static void itol3(MBHEAD *);
static void kilfra(void);

/*----------------------------------------------------------------------*/

void initl4(void)                     /* Level 4 initialisieren         */
{
  int i;                              /* Sratch Zaehler                 */

  inithd(&l4rxfl);                    /* Liste fuer empfangene Frames   */
  inithd(&l4frel);                    /* freie L4-Controllbloecke       */
  inithd(&l4actl);                    /* benutzte L4-Controllebloecke   */

  for (i = 0, cirpoi = cirtab;        /* gesamte Circuit Tabelle        */
       i < NUMCIR;                    /* bearbeiten                     */
       ++i, ++cirpoi)
  {
    cirpoi->state  = L4SDSCED;        /* Eintrag ist leer               */
    cirpoi->numrx  = 0;               /* keine Frames empfangen         */
    cirpoi->numtx  = 0;               /* keine Frames zu senden         */
    cirpoi->ll4txNR  = 0;             /* last l4 ack sent               */
    cirpoi->fragme = NULL;            /* kein Frame Fragment da         */
    inithd(&cirpoi->mbhdrx);          /* Empfangskette ist leer         */
    inithd(&cirpoi->mbhdtx);          /* Sendekette ist leer            */
    inithd(&cirpoi->mbhdos);          /* Kette "ausser Reihenfolge"     */
    clrcir();                         /* Rest initialisieren            */
    relink((LEHEAD *)cirpoi,          /* in die Liste fuer unbenutzte   */
           (LEHEAD *)l4frel.tail);    /* Controllbloecke                */
  }
  nmbcir = nmbcir_max = 0;            /* Circuit-Zaehler                */
}

/*----------------------------------------------------------------------*/
#define START 0
#define STOP  1
#define CLEAR 2
static void l4rtt(CIRBLK *cirp, int what)
{
  UWORD   RTT, SRTT;

  switch (what) {
    case START : cirp->RTT = 1;
                 cirp->RTTvs = cirp->l4vs;
                 break;

    case STOP  : RTT  = cirp->RTT;
                 SRTT = cirp->SRTT;

                 if (RTT > SRTT)
                   SRTT = (L4_ALPHA1 * SRTT + RTT * 100) / (L4_ALPHA1 + 1);
                 else
                   SRTT = (L4_ALPHA2 * SRTT + RTT * 100) / (L4_ALPHA2 + 1);
                 if (SRTT < L4_IRTT / 10)
                   SRTT = L4_IRTT / 10;
                 if (SRTT > L4_IRTT * 10)
                   SRTT = L4_IRTT * 10;

                 cirp->SRTT = SRTT;

    case CLEAR : cirp->RTT =
                 cirp->RTTvs = 0;
                 break;
  }
}

/************************************************************************/
/*                                                                      */
/* RETRY TIMER (T1)                                                     */
/*                                                                      */
/* Der T1 wird fuer jedes Frame in der Sendewarteschlange verwaltet und */
/* bei jeder Aussendung dieses Frames gestartet. Wenn er ablaeuft, wird */
/* das Frame erneut ausgesendet. L4_BETA1 muss so gewaehlt werden, dass */
/* der T1 nicht zu frueh ablaeuft und damit das Frame doppelt geschickt */
/* wird.                                                                */
/*                                                                      */
/************************************************************************/
static void l4setT1(CIRBLK *cirp, MBHEAD *fbp) {
  if (fbp->l4trie > 1)
    fbp->l4time = (cirp->SRTT/100 + 1) * 2 * L4_BETA1;
  else
    fbp->l4time = (cirp->SRTT/100 + 1) * L4_BETA1;
  if (fbp->l4time < 30)             /* nicht unter 30s                  */
    fbp->l4time = 30;
}

/************************************************************************/
/*                                                                      */
/* ACKNOWLEDGE TIMER (T2)                                               */
/*                                                                      */
/* Der T2 bestimmt die maximale Verzoegerung, bis ein empfangenes Frame */
/* bestaetigt wird. L4_BETA sollte so gewaehlt werden, dass Frames      */
/* moeglichst schnell bestaetigt werden, aber nicht zuviele ACK-Frames  */
/* generiert werden.                                                    */
/*                                                                      */
/************************************************************************/
static void l4setT2(CIRBLK *cirp) {
  UWORD frack;

  frack = (cirp->SRTT/100 + 1) * L4_BETA2;

  if (frack < 10)
    frack = 10;

  if (   cirp->acktim > frack
      && cirp->numrx < (cirp->window / 2))
    cirp->acktim = frack;
}

/************************************************************************/
/*                                                                      */
/* CONREQ/BUSY TIMEOUT (T3)                                             */
/*                                                                      */
/* Der T3 bestimmt die Wartezeit zwischen der Wiederholung von CONACK/  */
/* DISREQ und der Linkkontrolle bei Busy.                               */
/* Der T3 sollte nicht zu gering gewaehlt werden.                       */
/*                                                                      */
/************************************************************************/
static void l4setT3(CIRBLK *cirp, int what) {
#define L4TCREQ  0
#define L4TDREQ  1
#define L4TBUSY  2
  switch (what) {
    case L4TCREQ  :
    case L4TDREQ  : cirp->traout = (cirp->SRTT/100 + 1) * L4_BETA3;
                    if (cirp->traout < 30)
                        cirp->traout = 30;
                    break;
    case L4TBUSY  : cirp->traout = L4_BSYDEL;
                    break;
  }
}

/*----------------------------------------------------------------------*/
static void l4clrT3(CIRBLK *cirp) {
  cirp->traout = 0;
}

/*----------------------------------------------------------------------*/
static void l4newstate(UWORD state)
{
  if (state) {                         /* Verbindung steht              */
    if (cirpoi->state == L4SDSCED) {   /* ein neuer Connect             */
      ulink((LEHEAD *)cirpoi);         /* Circuit aus der Freiliste     */
      relink((LEHEAD *)cirpoi,         /* in die Liste fuer benutzte    */
             (LEHEAD *)l4actl.tail);   /* Controllbloecke               */
      if (++nmbcir > nmbcir_max)       /* Maximalanzahl der Circuits    */
        nmbcir_max = nmbcir;
    }
  } else {                             /* Verbindungsabbruch            */
    if (cirpoi->state != L4SDSCED) {   /* Circuit wird inaktiv          */
      ulink((LEHEAD *)cirpoi);         /* Circuit aus der Aktivliste    */
      relink((LEHEAD *)cirpoi,         /* in die Liste fuer unbenutzte  */
             (LEHEAD *)l4frel.tail);   /* Controllbloecke               */
      nmbcir--;                        /* einen aktiven Circuit weniger */
    }
  }
#ifdef __WIN32__
  cirpoi->state = (char)state;               /* neuen Status setzen           */
#else
  cirpoi->state = state;               /* neuen Status setzen           */
#endif /* WIN32 */
}

/*----------------------------------------------------------------------*/
void l4tx(void)                        /* Frames senden                 */
{
  UWORD unack;                         /* unbestaetigte Frames          */
  UWORD isweg;                         /* schon gesendete Frames        */
  MBHEAD *fbp;                         /* naechstes Frame               */
  CIRBLK *nxtcir;                      /* Zeiger auf naechsten CIRBLK   */

  for (cirpoi  = (CIRBLK *) l4actl.head;  /* alle aktiven Circuits      */
       cirpoi != (CIRBLK *) &l4actl.head; /* durchgehen                 */
       cirpoi  = nxtcir)                  /* und zum naechsten Circuit  */
  {
    nxtcir = (CIRBLK *) cirpoi->head;  /* wegen l4newstate()...         */

    if ((cirpoi->state == L4SIXFER) &&     /* Eintrag hat L4-Verbindung */
        (!(cirpoi->l4flag & L4FPBUSY))) {  /* und Partner nicht choked  */

      unack = (cirpoi->l4vs - cirpoi->l4rxvs) & 0x7F;
      if ((unack < cirpoi->numtx)          /* nur wenn Fenstergroesse   */
          && (unack < cirpoi->window)) {   /* noch nicht erreicht       */

        fbp = (MBHEAD *) cirpoi->mbhdtx.head; /* Anfang der Sendeliste  */
        for (isweg = 0; isweg < unack; ++isweg)/* schon gesendete offene*/
          fbp = (MBHEAD *) fbp->nextmh;        /* Frames, die auf Be-   */
                                               /* staetigung warten,    */
                                               /* uebergehen            */
        do {
          fbp->l4trie = 0;                    /* noch kein Versuch      */
          sndfrm(cirpoi->l4vs++, fbp);        /* naechstes Frame senden */

          fbp = (MBHEAD *) fbp->nextmh;       /* naechstes Frame        */
        } while ((++unack < cirpoi->numtx)    /* bis keine Frames mehr  */
                 &&(unack < cirpoi->window)); /* o.Fenstergroesse ueber-*/
                                              /* schritten              */
        if (!cirpoi->RTT)                     /* keine Messung unterwegs*/
          l4rtt(cirpoi, START);               /* dann neue starten      */
      }
    }
  }
}

/*----------------------------------------------------------------------*/
/* Diese Routine prueft, ob das aktuelle Frame ein Circuit von uns sein */
/* koennte. Das ist ziemlich unsauber, eigentlich sollte nur im L4      */
/* landen, was auch direkt an uns addressiert ist. Das Lokal-Konzept    */
/* von alten NET/ROMs zwingt uns aber, auch "fremde" Frames zu durch-   */
/* suchen.                                                              */
BOOLEAN l4istome(char *srcid, char *dstid)
{
  if (l4opco != L4CONREQ)             /* es ist kein Connect Request    */
  {
    if (   (l4hdr0 < NUMCIR)          /* Index im richtigen Bereich?    */
                                      /* die Verbindung haben wir       */
        && (cirtab[l4hdr0].state != L4SDSCED)
                                      /* und die ID stimmt              */
        && (cirtab[l4hdr0].ideige == l4hdr1))
    {
      cirpoi = &cirtab[l4hdr0];       /* CIRCUIT-Eintrag ist gueltig!   */
      if (l4opco != L4CONACK)         /* CONACK kann vom Host kommen    */
          if (!cmpid(srcid, cirpoi->l3node))
              return(FALSE);          /* Absender muss stimmen          */
      if (!cmpid(dstid, myid))        /* nicht direkt an uns ?          */
          if (!cmpid(dstid, cirpoi->destca))
              return(FALSE);          /* Ziel muss stimmen              */
      return(TRUE);
    }
  }
  return(FALSE);
}

/*----------------------------------------------------------------------*/
void l4rx(NODE *srcnod, NODE *dstnod, MBHEAD *fbp)   /* Frame empfangen */
{
  char usrcall[L2IDLEN];        /* Call des Users                       */
  char orgnod[L2IDLEN];         /* Call des Absender Knotens            */
  UWORD fenste;                 /* Fenstergroesse                       */
  int i;                        /* Scratch Zaehler                      */
  MBHEAD *antwor;               /* Antwort auf das empfange Frame       */
  CIRBLK *cirent;               /* Eintrag des Users in der CIRTAB      */
  CIRBLK  nocir;                /* Platzhalter wenn kein CIRCUIT        */
  char *viapoi;                 /* Zeiger fuer VIA-Liste                */
  char upno[L2IDLEN];           /* Uplink Knoten                        */
  char upnov[L2VLEN+1];         /* L2-via zu dem Uplink Knoten          */

/* Hier gab es frueher oefters Abstuerze, da itol3() immer einen        */
/* gueltigen cirpoi erwartet, aber wir haben eventuell keinen, weil wir */
/* die Verbindung nicht kennen. Dies betrifft spaete Versionen von TN   */
/* und einige fruehe Versionen von TNN.                                 */

  cirpoi = &nocir;
  cpyid(cirpoi->l3node, srcnod->id);

  if (l4opco != L4CONREQ)             /* es ist kein Connect Request    */
  {
    if (   (l4hdr0 < NUMCIR)          /* Index im richtigen Bereich?    */
        && (cirtab[l4hdr0].state != L4SDSCED)   /* Verbindung haben wir */
        && (cirtab[l4hdr0].ideige == l4hdr1))   /* und die ID stimmt    */
    {
      cirpoi = &cirtab[l4hdr0];       /* CIRCUIT-Eintrag ist gueltig!   */
      l4pidx = cirpoi->idxpar;        /* Partner-Index uebernehmen      */
      l4pcid = cirpoi->idpart;        /* Partner-ID uebernehmen         */
    }
    else
    {
      if (   (l4opco != L4CONACK)     /* alles ausser CONACK = Muell    */
          && !(l4hdr4 & L4CCHOKE)
          && !(l4opco & L4DISREQ))
      {
        l4pidx = l4hdr2;              /* Partner Circuit-Index          */
        l4pcid = l4hdr3;              /* und -ID fuer Antwort           */
        l4ahd2 =
        l4ahd3 = 0;
        l4aopc = L4DISREQ;            /* Disconnect Request als Antwort */
        itol3(gennhd());              /* Antwort &nocir->l3node=despoi  */
      }
      dealmb(fbp);                    /* schlechter Header, entsorgen   */
      return;
    }
  }

  switch (l4opco)                     /* Ueber Opcode verzweigen        */
  {
    case L4CONREQ:                    /* Connect-Request oder ein       */
      if (((fbp->mbpc - fbp->mbgc) >= 15) &&      /* Frame lang genug ? */
          ((fenste = getchr(fbp) & 0x7F) != 0) && /* und Fenster da     */
          (getfid(usrcall, fbp) == TRUE) && /* gueltiges Usercall       */
          (getfid(orgnod, fbp) == TRUE))    /* gueltiger Absender       */
      {
/*****************************************************************************\
*                                                                             *
* Protokollerweiterung: Beim Connect-Request-Frame wird im Anschluss an die   *
* Fenstergroesse der Uplinkknoten und die Via-Liste beim Uplink uebertragen   *
* (nullterminiert), also maximal 64 Bytes zusaetzlich. Diese Erweiterung ist  *
* kompatibel zur bisherigen Software, da laengere Frames nicht weiter unter-  *
* sucht werden. Um auch hier spaetere Erweiterungen zu ermoeglichen, wird     *
* der Rest des Frames nicht untersucht. Ist kein Uplinkknoten im Frame        *
* enthalten, also bei aelterer Software, wird das Call des Absenderknotens    *
* als Uplinkknoten eingesetzt.                                                *
*                                                                             *
\*****************************************************************************/
          if (   !(fbp->mbpc - fbp->mbgc >= L2IDLEN)  /* neue Software?       */
              || (!getfid(upno,fbp)))              /* kein Uplinkknoten?      */
          {
            cpyid(upno,orgnod);  /* Absenderknoten als Uplinkknoten annehmen  */
            *upnov = '\0';       /* und dort keine Digikette                  */
          }
          else                   /* neue Software                             */
          {
            for (viapoi = upnov;                 /* Vialiste holen            */
                 viapoi < upnov + L2VLEN;        /* max. 8 Calls              */
                 viapoi = viapoi + L2IDLEN)      /* viapoi -> naechstes Call  */
            {
              if (!getfid(viapoi, fbp)) break;   /* Ende Vialiste?            */
            }
            *viapoi = '\0';               /* Ende markieren                   */
          }

          l4pidx = l4hdr0;        /* Index                            */
          l4pcid = l4hdr1;        /* und ID des Parnters merken       */
          cirent = NULL;
          for (i = 0, cirpoi = cirtab; /* Circuit Tabelle absuchen    */
               i < NUMCIR;++i, ++cirpoi)
          {
            if (cirpoi->state != L4SDSCED) {  /* Verbingung besteht   */
              if (   (cirpoi->idxpar == l4hdr0) /* PartnerIndex stimmt*/
                  && (cirpoi->idpart == l4hdr1) /* Parner-ID stimmt   */
                  && cmpid(usrcall, cirpoi->upcall)/* UserCall stimmt */
                  && cmpid(orgnod, cirpoi->downca))/* Absender stimmt */
                 break;           /* wir haben ihn gefunden !         */
            }
            else                  /* sonst merken wir uns den freien  */
              if (cirent == NULL) /* Platz in der Circuit Table       */
                cirent = cirpoi;
          }
          if (i == NUMCIR) {      /* Eintrag war nicht in der Liste   */
            if ((cirent != NULL) && /* wenn wir noch ein Plaetzchen   */
                (fvalca(usrcall) == YES))       /* haben und Call ok  */
            {
              cirpoi = cirent;      /* dann Eintrag nehmen            */
              cpyid(cirpoi->upcall, usrcall);  /* Usercall setzen     */
              cpyid(cirpoi->downca, orgnod);   /* Absenderknoten      */
              cpyid(cirpoi->upnod,  upno);     /* Uplink Knoten       */
              cpyidl(cirpoi->upnodv, upnov); /* Uplink via's          */

              cirpoi->idxpar = l4hdr0;       /* Partner-Index merken  */
              cirpoi->idpart = l4hdr1;       /* Parner-ID merken      */

              cirpoi->ideige = rand() % 256; /* eigene Zufalls-ID     */
              cpyid(cirpoi->l3node, srcnod->id);
              if (fbp->l3_typ == L3LOCAL)    /* ein Lokal?            */
                cpyid(cirpoi->destca, dstnod->id);
              else
                cpyid(cirpoi->destca, myid);

              cirpoi->tranoa = ininat;    /* Timeout setzen           */
            }
            else
            {                    /* kein Platz oder ungueltiges Call  */
              l4ahd2 =           /* Antwort aufbauen                  */
              l4ahd3 = 0;
              l4aopc = L4CONACK | L4CCHOKE;  /* Antwortframe aufbauen */
              antwor = gennhd();
              antwor->l2link = NULL;
              cpyid(antwor->destcall, srcnod->id);
              relink((LEHEAD *)antwor,(LEHEAD *) l3txl.tail);
              break;            /* und in die Sendekette haengen.     */
            }
          }                     /* den Eintrag gibt es schon          */

#ifdef CONL3LOCAL
          /* Unser Nachbar ist in der Liste und     */
          /* schickt ein weiteres statusflag CONREQ.*/
          if (i != NUMCIR)
              /* damit brechen wir hier ab. */
              break;
#endif
          /* Vom Partner vorgeschlagene Fenstergroesse uebernehmen    */
          /* wenn sie nicht groesser als unsere maximal-Groesse ist.  */
          cirpoi->window = (fenste > trawir) ? trawir : fenste;
          clrcir();               /* Eintrag initialisieren           */
#ifdef CONL3LOCAL
          /* Statusflag CONACK NUR fuer das eigene Node Mycall */
          /* vorbereiten/verschicken.                          */
          if (fbp->l3_typ != L3LOCAL)
          {
#endif
            l4ahd2 = (UBYTE) (cirpoi - cirtab); /* Unseren Index setzen */
            l4ahd3 = cirpoi->ideige;            /* Unsere (Zufalls-) ID */
            l4aopc = L4CONACK;                  /* Connect Acknowledge  */
            putchr(cirpoi->window, (antwor = gennhd())); /* endgueltige */
                               /* Fenstergroesse zurueck an den Partner */
            itol3(antwor);                      /* senden               */
#ifdef CONL3LOCAL
           }
#endif
          switch (cirpoi->state) {
            case L2SDSCED :
            case L2SLKSUP :
#ifdef CONL3LOCAL
              if (fbp->l3_typ != L3LOCAL)
                l4newstate(L4SIXFER);     /* STATUS = connected   */
              else
                /* Es gibt noch keine Verbindung! */
                l4newstate(L4SLKSUP);     /* STATUS = link setup  */
#else
                l4newstate(L4SIXFER);     /* STATUS = connected   */
#endif
              l2tol7(L4MCONNT, cirpoi, L4_USER);
          }
      }
      break;

      case L4CONACK:
        if (cirpoi->state == L4SLKSUP) {   /* nur wenn Connect von uns  */
          if (!(l4hdr4 & L4CCHOKE)) {      /* verlangt war und Partner  */
            if (fbp->mbgc < fbp->mbpc) {   /* nicht choked.             */
              cirpoi->window = getchr(fbp); /* Entgueltige Fenstergr.   */
              cirpoi->idpart = l4hdr3;     /* Partner ID                */
              cirpoi->idxpar = l4hdr2;     /* Partner Index             */
              clrcir();                    /* Eintrag initialisieren    */
              cirpoi->tranoa = ininat;     /* Timeout setzen            */
              l4newstate(L4SIXFER);        /* Status = connected        */
              /**********************************************************/
              /* Protokollerweiterung nach DB7KG, 23.11.1996            */
              /* Bei jedem eingehenden Connect-ACK setzen wir den Ziel- */
              /* Node auf den Absender, von dem das Frame kam. Damit    */
              /* soll bei einem Connect an einen Local der tatsaechliche*/
              /* L3 Partner als Ziel gewaehlt werden, damit es keine    */
              /* Probleme beim umrouten dieser Verbindungen gibt.       */
              /* (Ein umrouten ist dann nur bis zum Partner moeglich,   */
              /* da kuenftig Frames mit dieser Addresse gesendet werden)*/
              /* HINWEIS: Impelementierung hier so nur moeglich, weil   */
              /* l4rx() direkt aus dem L3 aufgerufen wird! (fuer DF2AU) */
              /**********************************************************/
              cpyid(cirpoi->l3node, srcnod->id);
              l2tol7(L4MCONNT, cirpoi, L4_USER); /* L7 melden           */
            }
          }
          else                             /* Partner ist choked (busy) */
              l4nsta(L4MBUSYF);            /* an L7 melden              */
        }
        break;

      case L4DISREQ:                       /* Disconnect Request        */
        l4ahd2 =
        l4ahd3 = 0;                        /* Antwort aufbauen          */
        l4aopc = L4DISACK;                 /* Opcode: Disconnect Ackn.  */
        itol3(gennhd());                   /* Frame senden              */
        clr4rx(1);                         /* restliche Infos senden    */
        l4nsta(L4MDISCF);                  /* an L7 melden              */
        break;

      case L4DISACK:                       /* Disconnect Acknowledge    */
        if (cirpoi->state == L4SDSCRQ)     /* Hatten wir Disc gesendet ?*/
          l4nsta(L4MDISCF);                /* wenn ja dann dem L7 melden*/
        break;

      case L4INFTRA:                       /* Infoframe                 */
        if (cirpoi->state != L4SIXFER)     /* nur wenn connected        */
          break;
        chksts();                          /* Status Info auswerten     */
        /* TEST DG9OBU */
        if (((fbp->l4seq =                 /* passt Frame ins Fenster   */
          (l4hdr2 - cirpoi->l4vr) & 0x7f) < cirpoi->window)
          && !(cirpoi->l4flag & L4FBUSY))
        {                                  /* und Partner nicht busy    */
          fbp->morflg = (l4hdr4 & L4CMORE) != 0;  /* Fragmentiert ?     */
          if (fbp->l4seq == 0) {           /* passt Sequenz?            */
            takfrm(fbp);                   /* Frame uebernehmen         */
            /* Frames, die ausser der Reihe kamen ueberpruefen          */
            for (i = 1, antwor = (MBHEAD *) cirpoi->mbhdos.head;
                 (MBHEAD *) &(cirpoi->mbhdos) != antwor;
                 antwor = (MBHEAD *) antwor->nextmh)
            {
              if ((antwor->l4seq -= i) == 0) {   /* passt das Frame ?   */
                  fbp = (MBHEAD *) antwor->prevmh; /* ja, nehmen und    */
                  takfrm((MBHEAD *) ulink((LEHEAD *)antwor));/*naechstes*/
                  antwor = fbp;     /* Frame ueberpruefen               */
                  ++i;              /* Offset eins weiter               */
              }
            }
            cirpoi->l4rs = 0;      /* Antwort: ACK (Bestaetigung)       */

            /* Dynamisches ACKDEL */

            i =  cirpoi->numrx - (cirpoi->window /2);
            if (i < 1) i = 1;
            cirpoi->acktim = i * L4_ACKDEL ;
          }
          else
          {                        /* Sequenz passt nicht              */
            for (antwor = (MBHEAD *) cirpoi->mbhdos.head;;) {
              /* Wenn die Kette dort zuende ist, dann dort einhaengen */
              if ((MBHEAD *) &(cirpoi->mbhdos) == antwor) {
                relink((LEHEAD *)fbp, (LEHEAD *)cirpoi->mbhdos.tail);
                break;
              }
              /* Wenn das Frame schoneinmal gekommen ist -> weg damit */
              if (antwor->l4seq == fbp->l4seq) {
                dealmb(fbp);
                break;
              }
              /* sonst an der passenden Stelle einhaengen             */
              if (antwor->l4seq > fbp->l4seq) {
                relink((LEHEAD *)fbp, (LEHEAD *)antwor->prevmh);
                break;
              }
              antwor = (MBHEAD *) antwor->nextmh; /* ein Frame weiter */
            }

            if (cirpoi->l4rs == 0) {      /* Wenn ACK gesendet werden soll */
              cirpoi->l4rs = 1;           /* dann nun einen N(ot)AK senden */
              cirpoi->acktim = L4_ACKDEL; /* ACK-Wartezeit setzen          */
            }
          }
          return;                     /* Frame verarbeitet                */
        }
        else                          /* ungueltiges Frame oder wir haben */
          cirpoi->acktim = L4_ACKDEL; /* zuwenig Buffers (choked), dann   */
        break;                        /* Antwort verzoegern.              */

      case L4INFACK:                  /* Info Acknowledge                 */
        if (cirpoi->state == L4SIXFER)/* nur wenn connected               */
          chksts();                   /* Statusinformation auswerten      */

#ifdef NEW_L4
        break;

      case L4PIDCHG:                  /* PID-Change                       */
        cirpoi->pid = l4hdr3;         /* neue PID uebernehmen             */
#endif
   }
   dealmb(fbp);                       /* hier landen wir bei ungueltigen  */
                                      /* Opcodes oder wenn die Bearbeitung*/

                                      /* fertig ist, auf jeden Fall Frame */
}                                     /* wegwerfen.                       */

/*------------------------------------------------------------------------*/
void l4rest(void)                     /* sonstige L4-Funktionen           */
{
  UWORD   rx_unack;
  UBYTE   w2;
  CIRBLK *nxtcir;

  for (cirpoi  = (CIRBLK *)  l4actl.head; /* alle aktiven Circuits      */
       cirpoi != (CIRBLK *) &l4actl.head; /* durchgehen                 */
       cirpoi  = nxtcir) {
    nxtcir = (CIRBLK *) cirpoi->head;  /* vorher schon merken           */
    if ((cirpoi->l4flag & L4FDIMM))    /* sofortiger Abwurf             */
    {
         l4nsta(L4MDISCF);             /* Disconnect melden             */
    }
    else
    if (cirpoi->state == L4SIXFER) {/* nur fuer connectete Eintraege    */
      if (   (cirpoi->l4flag & L4FDSLE) /* Abwurf gefordert             */
          && (cirpoi->numtx == 0))    /* und alle Infos gesendet        */
        endcir();                    /* Eintrag loeschen                */
      else
      {
        clr4rx(0);                   /* sonst Info senden               */

        /* Fruehes Ack */
        rx_unack = ((cirpoi->l4vr | 0x100) - cirpoi->ll4txNR) & 0x7F;
        w2 = cirpoi->window/2;

        if (!(cirpoi->l4flag & L4FBUSY)) {   /* wir sind nicht busy     */
          if (nmbfre < 30) {                 /* Aber zu wenig Platz     */
            cirpoi->l4flag |= L4FBUSY; /* Dann sind wir nun Busy !      */
            cirpoi->l4rs = 0;          /* Antwort sofort senden         */
            sndack();                  /* ACK senden                    */
          }
          else {                       /* ich bin nicht busy            */
            if (   cirpoi->acktim > 1
                && rx_unack >= w2
                && cirpoi->numrx < w2)   /* und es ist noch Platz       */
              sndack();
          }
        }
        else {  /* im Moment sind wir Busy */
          if (   nmbfre > 62               /* wieder genug Platz ?      */
              && cirpoi->numrx < w2) {     /* Und nicht zuviel ?        */
            cirpoi->l4flag &= ~L4FBUSY;    /* Busy aufheben             */
            sndack();                      /* und ACK senden            */
          }
        }
      }
    }
  }
}

#ifdef L4TIMEOUTAUSGABE
/************************************************************************/
/*                                                                      */
/* Vor dem Disconect eine Info-Meldung an den User senden.              */
/*                                                                      */
/************************************************************************/
static void l4DiscInfo(void)
{
  MBHEAD *mbp;

  if ((mbp = (MBHEAD *) allocb(ALLOC_MBHEAD)) != NULL)    /* Buffer besorgen. */
  {
    putchr('\r', mbp);
    putalt(alias, mbp);
    putid(myid, mbp);

    putstr("> Timeout (", mbp);
    putnum(ininat, mbp);                           /* Timeout in s ausgeben   */
    putstr("s) run off.\r", mbp);
    rwndmb(mbp);

    sndfrm(cirpoi->l4vs++, mbp);                               /* Info senden */
    dealmb(mbp);                                         /* Buffer entsorgen. */
  }
}
#endif /* L4TIMEOUTAUSGABE */

/*----------------------------------------------------------------------*/
void trasrv(void)                   /* Timerservice fuer den L4         */
{
  UWORD   actsts;                   /* Status des aktuellen Eintrages   */
  UWORD   fropen;                   /* Zahl der unbestaetigten Frames   */
  UWORD   tosend;                   /* Zahl der zu sendenden Frames     */
  MBHEAD *fbp;                      /* aktuelles Frame                  */
  UWORD   rx_unack;
  CIRBLK *nxtcir;
  char    txvs;

  for (cirpoi  = (CIRBLK *)  l4actl.head; /* alle aktiven Circuits      */
       cirpoi != (CIRBLK *) &l4actl.head; /* durchgehen                 */
       cirpoi  = nxtcir) {
    nxtcir = (CIRBLK *) cirpoi->head;  /* vorher schon merken           */
    if ((actsts = cirpoi->state) != L4SDSCED) { /* nur fuer aktive Verb.*/
      if (cirpoi->RTT)              /* RTT-Messung                      */
        cirpoi->RTT++;
      if (cirpoi->traout != 0) {    /* Timeout noch nicht abgelaufen?   */
        if (--cirpoi->traout == 0) {/* Timeout nun abgelaufen ?         */
          if (actsts == L4SIXFER) { /* nur fuer connectete Eintraege    */
            cirpoi->l4flag &= ~L4FPBUSY; /* nichts mehr senden          */
          } else {
            if (++cirpoi->l4try < L4_RETRY) /* nochmal Versuchen ?      */
            {
              if (actsts == L4SLKSUP)  /* CON REQ kam nicht an, nochmal */
                sconrq();
              else
                sdisrq();             /* DISC REQ kam nicht an, nochmal */
            } else
              l4nsta(L4MFAILW);       /* Fehler an L7 melden            */
          }
        }
      }
      else {                          /* Timeout ist abgelaufen         */
        if ((actsts == L4SIXFER) /* connected und Frames unbestaetigt ? */
             && ((fropen = (cirpoi->l4vs - cirpoi->l4rxvs) & 0x7F) != 0)) {
                                             /* Frames wiederholen      */
          for (tosend = 0, fbp = (MBHEAD *) cirpoi->mbhdtx.head;
               tosend < fropen;
             ++tosend, fbp = (MBHEAD *) fbp->nextmh) {
            if (--fbp->l4time == 0) {        /* wenn Timeout um         */
              if (++fbp->l4trie < L4_RETRY) {/* und noch Versuch frei   */
                txvs = fbp->l4seq;           /* Framenummer holen       */
                if (cirpoi->RTTvs == txvs)   /* bei Wiederholung RTT-   */
                  l4rtt(cirpoi, CLEAR);      /* Messung verwerfen       */
                sndfrm(txvs, fbp);           /* Frames senden           */
              } else {
                l4nsta(L4MFAILW);            /* sonst L7 failure melden */
                break;
              }
            }
          }
        }
      }

      if (actsts == L4SIXFER) {         /* connected ?                  */
        if (   (cirpoi->acktim != 0)    /* ACK noch nicht gesendet      */
            && (--cirpoi->acktim == 0)) /* aber Timer laeuft gerade aus */
        {
          /* schnelle Flow-control */
          rx_unack = ((cirpoi->l4vr | 0x100) - cirpoi->ll4txNR) & 0x7F;

          if (   cirpoi->numrx > (cirpoi->window / 2)
              && rx_unack >= cirpoi->window)
          {
            cirpoi->l4flag |= L4FBUSY;
            cirpoi->l4rs = 0;
          }
          sndack();                 /* dann ein ACK senden              */
        }

        if (   (cirpoi->tranoa != 0) /* No-activity Timeout laeuft aus  */
            && (--cirpoi->tranoa == 0))
#ifndef L4TIMEOUTAUSGABE
             endcir();               /* Verbindung trennen              */
#else
        {
          l4DiscInfo();/* Vor Trennung, Timeout-Meldung zum User senden.*/
          endcir();                  /* Verbindung trennen              */
        }
#endif /* L4TIMEOUTAUSGABE */
      }
    }
  }
}

/*----------------------------------------------------------------------*/
void newcir(void)                   /* neuen Circuit-Eintrag aufbauen   */
{
  clrcir();                         /* Eintrag erstmal loeschen         */
  cirpoi->ideige = rand() % 256;    /* eigene ID zufaellig erzeugen     */
  cirpoi->l4try = 0;                /* noch keine Sende-Versuche gemacht*/
  sconrq();                         /* Connect Request senden           */
  l4newstate(L4SLKSUP);             /* neuer Status: Verbindungsaufbau  */
}

/*----------------------------------------------------------------------*/
void discir(void)                   /* Circuit aufloesen                */
{
  if ((cirpoi->state == L4SLKSUP) ||/* Status Connect Request oder      */
        (cirpoi->state == L4SDSCRQ))  /* Disconnect Request ?           */
  {
     /* Bis jetzt wurde ein L4 Disconnect nicht nach oben gemeldet,     */
     /* wenn der State noch LINK SETUP oder DISCONNECT REQUEST war.     */
     /* Ich nehme an, das war wegen der reentranz-Problematik so        */
     /* geloest worden. Jetzt wird ein Flag gesetzt, das eigenliche     */
     /* Melden passiert in L4rest.                                      */
     cirpoi->l4flag |= L4FDIMM;      /* spaeter in l4rest               */
#ifdef CONL3LOCAL
     /* Statusflag CONACK + CHOKE Flag senden  */
     /* wenn Verbindungsstatus Setup/Disc ist. */
     bsycir();
#endif
  }
  else {
     kilfra();                       /* Fragmentliste loeschen          */
     dealml((LEHEAD *)&cirpoi->mbhdrx); /* Empfangsliste loeschen       */
     cirpoi->numrx = 0;              /* Empfangszaehler zuruecksetzen   */
     cirpoi->l4flag |= L4FDSLE;      /* fuer Abwurf markieren           */
  }
}

/************************************************************************/
/*                                                                      */
/* Info vom L7 an Circuit senden                                        */
/*                                                                      */
/************************************************************************/
BOOLEAN
itocir(BOOLEAN cflg, MBHEAD *mbp)
{
  CIRBLK *cblk;

  cblk = (CIRBLK *)mbp->l2link;
  if (cblk->l4flag & L4FDIMM)   /* Circuit ist abgefuellt?              */
  {
    dealmb(mbp);                /* Info abnehmen (gibt Platz)           */
    return (TRUE);              /* Info ist weg                         */
  }
  if (   (cblk->numtx < conctl) /* noch Platz ?                         */
      || (cflg == TRUE))        /* oder immer senden                    */
  {
/* Infoframe in die TX-Liste des Circuit haengen                        */
    relink(ulink((LEHEAD *)mbp), (LEHEAD *)cblk->mbhdtx.tail);
    ++cblk->numtx;                             /* Framezaehler erhoehen */
/* Um ein Abfuellen des Knotens durch einzelne User auch im L4 zu       */
/* verhindern, wird wie im L2 die max. Zahl an Frames auf 150 begrenzt  */
     if (cblk->numtx >= 150)
       cblk->l4flag |= L4FDIMM;
/* Das morflg des Frames wird jetzt im L7 geloescht vor dem Aufruf von  */
/* itocir, falls das Frame nicht von einem L4-Partner kommt.            */
     cblk->tranoa = ininat;          /* Timeout neu aufziehen           */
     return(TRUE);                   /* ok zurueckmelden                */
  }
  return(FALSE);                     /* kein Platz zurueckmelden        */
}

/* Informationstransfer von Layer 4 nach Layer X                        */
/* Solange noch empfangene Pakete vorhanden sind, werden diese an       */
/* andere Layer durch Aufruf von fmlink() uebertragen. Bei geseztem     */
/* Ueberfuellungskontroll-Flag (conctl == TRUE) wird die Uebertragung   */
/* abgebrochen, wenn der andere Layer keine weiteren Daten mehr auf-    */
/* aufnehmen kann.                                                      */
/* Nach erfolgter Uebertragung wird die Anzahl der uebertragenen Zeichen*/
/* fuer die Statistik gezaehlt und der No-Activity-Timer neu gesetzt.   */
static void clr4rx(BOOLEAN conctrl)
{
    MBHEAD *mbp;

    while (cirpoi->numrx != 0) {/* solange noch Frames vorhanden    */
      mbp = (MBHEAD *) cirpoi->mbhdrx.head; /* ein Frame holen      */
      mbp->l2link = (LNKBLK *) cirpoi; /*zugehoerigen Circuit merken*/
      mbp->type = 4;                   /* User ist Circuit          */
#ifdef NEW_L4
      mbp->l2fflg = cirpoi->pid;       /* PID einstellen            */
#else
      mbp->l2fflg = L2CPID;            /* L4 immer PID F0           */
#endif
      if (!fmlink(conctrl, mbp)) break;/* Ende bei Fehler           */
      --cirpoi->numrx;                 /* ein Frame weniger         */
      cirpoi->tranoa = ininat;         /* Timeout neu setzen        */
      l4setT2(cirpoi);                 /* ACK_TIMER neu setzen      */
    }
}

/*----------------------------------------------------------------------*/
static void chksts(void)            /* Status des Frames auswerten      */
{
  UWORD frofs;                      /* bestaetigter Offset              */
  UWORD fropen;                     /* unbesteatigter Offset            */

  if ((fropen = (cirpoi->l4vs - cirpoi->l4rxvs) & 0x7F) != 0) {
                                    /* Frames offen ?                   */
    if ((frofs = (l4hdr3 - cirpoi->l4rxvs) & 0x7F) != 0) {
                                    /* neu bestaetigte Frames ?         */
      if (frofs <= fropen) {        /* Frames wurden bestaetigt         */
        while (frofs-- != 0) {      /* solange bestaetigte Frames ueber */
          dealmb((MBHEAD *)ulink((LEHEAD *)cirpoi->mbhdtx.head)); /* weg*/
          cirpoi->numtx--;          /* einen weniger zu senden          */
          if (cirpoi->l4rxvs++ ==   /* einen mehr bestaetigt            */
              cirpoi->RTTvs)        /* Messframe bestaetigt?            */
            l4rtt(cirpoi, STOP);    /* Messung auswerten                */
        }
      }
    }
  }

  if (!(l4hdr4 & L4CCHOKE)) {        /* Partner choked ?                 */
     cirpoi->l4flag &= ~L4FPBUSY;    /* nein, merken                     */
     l4clrT3(cirpoi);                /* Timeout kann nicht kommen        */
     if ((l4hdr4 & L4CNAK) &&        /* NAK-Flag ?                       */
          (cirpoi->l4vs != cirpoi->l4rxvs)) { /* und noch was offen ?    */
                                              /* dann wiederholen        */
       if (cirpoi->RTTvs == cirpoi->l4rxvs)
         l4rtt(cirpoi, CLEAR);       /* bei Wiederholung RTT verwerfen   */
       sndfrm(cirpoi->l4rxvs, (MBHEAD *)cirpoi->mbhdtx.head);
     }
  }
  else { /* Partner ist choked (busy) */
     cirpoi->l4flag |= L4FPBUSY;     /* merken                           */
     cirpoi->l4vs = cirpoi->l4rxvs;  /* keine Frames offen               */
     l4setT3(cirpoi, L4TBUSY);       /* warten                           */
  }
}

/*----------------------------------------------------------------------*/
static void takfrm(MBHEAD *mbp)     /* empfangenes Frame uebernehmen    */
{
  BOOLEAN more;         /* morflg des zu verarbeitenden Frames          */
  BOOLEAN copy = FALSE; /* umkopieren noetig wegen Fragmentierung       */
  MBHEAD *fragmn;       /* dieses Fragment wird bearbeitet              */
  int     max;          /* maximale Info-Laenge fuer Weiterleitung      */
  int     sum  = 0;     /* Summe vorhandenes Fragment + neues Frame     */

  if (!(cirpoi->l4flag & L4FDSLE))  /* Circuit ist nicht zu beenden     */
   {
    more = mbp->morflg;

/* Framegroesse fuer vorhandenes Fragment und neues Frame bestimmen.    */
/* Da fuer das Fragment in einen neuen Buffer geschrieben wurde, ist    */
/* (fragmn->mbgc == 0) und muss nicht von fragmn->mbpc subtrahiert      */
/* werden.                                                              */

    if ((fragmn = cirpoi->fragme) != NULL)
     {
      sum += fragmn->mbpc;
     }
    sum += mbp->mbpc - mbp->mbgc;

/* Im L7 nachfragen, welche maximale Paketlaenge verarbeitet werden     */
/* kann (CCP / Hostmode / L2 -> 256 Bytes; L4 -> 236 Bytes)             */

    max = ptc_p_max(cirpoi, L4_USER);

/* muss umkopiert werden?                                               */
    if ((sum > max) || (fragmn != NULL) || (more))
      copy = TRUE;

    while (copy)
     {
/* ggf. neuen Buffer fuer Arbeits-Fragment holen                        */
      if (fragmn == NULL)
       {
        fragmn = (MBHEAD *) allocb(ALLOC_MBHEAD);
        cirpoi->fragme = fragmn;
       }
/* erstmal vermuten, dass nach diesem Fragment noch mehr folgt          */
      fragmn->morflg = 1;
/* aus neuem Frame ans Fragment anhaengen, bis max erreicht ist, oder   */
/* neues Frame leer                                                     */
      while ((fragmn->mbpc < max) && (mbp->mbpc > mbp->mbgc))
       {
        putchr(getchr(mbp), fragmn);
       }
/* Fragment fertig - Rest berechnen                                     */
      sum -= fragmn->mbpc;
      if (!sum)
       {
/* neues Frame ist leer und kann weg                                    */
        dealmb(mbp);
        mbp = NULL;
        copy = FALSE;
/* wenn nichts mehr folgt, dies im Fragment vermerken                   */
        if (!more)
          fragmn->morflg = 0;
       }
/* Fragment weiterreichen, wenn max erreicht oder wenn nix mehr kommt   */
      if ((fragmn->mbpc == max) || (fragmn->morflg == 0))
       {
        rwndmb(fragmn);
        relink((LEHEAD *)fragmn, (LEHEAD *) cirpoi->mbhdrx.tail);
        fragmn = NULL;
        cirpoi->fragme = NULL;
        ++cirpoi->numrx;
       }
     }

    if (mbp != NULL)                /* Frame noch da?                   */
     {
      relink((LEHEAD *)mbp,(LEHEAD *) cirpoi->mbhdrx.tail); /* in L4-RX */
      ++cirpoi->numrx;              /* ein Frame mehr empfangen         */
     }
   }
  else                              /* Verbindung soll abgeworfen werden*/
    dealmb(mbp);                    /* Frame einfach vernichten         */
  ++cirpoi->l4vr;                   /* RX-Sequenz erhoehen              */
}

/*----------------------------------------------------------------------*/
static void sndfrm(int txsequ, MBHEAD *mbp)     /* Frame senden         */
{
  char huge *next;                  /* Pointer auf Ende des Headers     */
  MBHEAD *netmhd;                   /* Netzwerk Header                  */
#ifdef __WIN32__
  WORD mtu;                          /* MTU auf dem Sende-Port           */
#else
  int mtu;                          /* MTU auf dem Sende-Port           */
#endif
#ifdef NEW_L4
  if (   (mbp->type == 2)                /* nur L2-User                 */
      && (mbp->l2fflg != cirpoi->pid)    /* andere PID als bisher ?     */
      && (cmpcal(cirpoi->upnod, myid)))  /* nur lokale Uplinks beachten */
  {
    spidchg(mbp->l2fflg);           /* neue PID mitteilen               */
    cirpoi->pid = mbp->l2fflg;      /* und selber merken                */
  }
#endif

  l4ahd2 =                          /* Sendesequenzzaehler in den Header*/
  mbp->l4seq = txsequ;              /* und im Buffer merken             */
  l4aopc = L4INFTRA;                /* Opcode: Info                     */
  if (mbp->morflg)
    l4aopc |= L4CMORE;
  ackhdr();                         /* Rest des Headers erzeugen        */
  next = (netmhd = gennhd()) ->mbbp;/* Buffer holen und Position Opcode */
                                    /* merken                           */
  mtu = 256;                        /* NETROM immer 256 Bytes Frames    */

#ifdef __WIN32__
  if (splcpy(((short)(mtu - netmhd->mbpc)), netmhd, mbp)) /* Info umkopieren     */
#else
  if (splcpy((mtu - netmhd->mbpc), netmhd, mbp)) /* Info umkopieren     */
#endif /* WIN32 */
  {                                 /* hat nicht alles reingepasst      */
    ++cirpoi->numtx;                /* ein Frame mehr gesendet          */
    mbp->morflg = TRUE;             /* markieren, es kommt noch mehr    */
  }

  if (mbp->morflg)
     *(next -1) |= L4CMORE;         /* more Flag im Opcode setzen       */

  itol3(netmhd);                    /* Frame an L3 geben                */
  l4setT1(cirpoi, mbp);             /* Timeout neu aufziehen            */
}

/*----------------------------------------------------------------------*/
static void l4nsta(WORD frtyp)      /* Statusaenderung im L4            */
{
  l4newstate(L4SDSCED);             /* Status: Disconnected             */
  clrcir();                         /* Eintrag zuruecksetzen            */
  dealml((LEHEAD *) &cirpoi->mbhdrx);  /* Empfangsliste loeschen        */
  dealml((LEHEAD *) &cirpoi->mbhdtx);  /* Sendeliste loeschen           */
  cirpoi->numrx =                   /* Zaehler zuruecksetzen            */
  cirpoi->numtx = 0;
  l2tol7(frtyp, cirpoi, L4_USER);
}

/*----------------------------------------------------------------------*/
static void endcir(void)        /* Circuit aufloesen                    */
{
  clrcir();                     /* Eintrag in CIRTAB loeschen           */
  cirpoi->l4try = 0;            /* Versuche zuruecksetzen               */
  sdisrq();                     /* Abwurf einleiten                     */
  l4newstate(L4SDSCRQ);         /* neuer Status: DISC-REQ gegeben       */
}

/*----------------------------------------------------------------------*/
static void clrcir(void)        /* Eintrag in CIRTAB loeschen           */
{
  kilfra();                     /* Fragmente loeschen                   */
  dealml((LEHEAD *)&cirpoi->mbhdos);    /* Messageliste dafuer auch     */
#ifdef __WIN32__
  cirpoi->l4rxvs = 0;           /* alle Sequenzen auf 0                 */
  cirpoi->l4vs =   0;
  cirpoi->l4vr =   0;
  cirpoi->l4rs =   0;           /* ACK-NAK Flag                         */
  cirpoi->l4try =  0;
#else
  cirpoi->l4rxvs =              /* alle Sequenzen auf 0                 */
  cirpoi->l4vs =
  cirpoi->l4vr =
  cirpoi->l4rs =                /* ACK-NAK Flag                         */
  cirpoi->l4try =
#endif /* WIN32 */
  cirpoi->ll4txNR = 0;
  cirpoi->l4flag = 0;           /* niemand choked, kein DISC-REQ        */
  cirpoi->acktim = 0;
  cirpoi->SRTT = L4_IRTT;       /* SRTT initialisieren                  */
#ifdef NEW_L4
  cirpoi->pid = L2CPID;         /* standardmaessig PID F0               */
#endif
  l4clrT3(cirpoi);              /* Timer loeschen                       */
  l4rtt(cirpoi, CLEAR);         /* RTT-Messung stoppen und verwerfen    */
  resptc(g_uid(cirpoi, L4_USER));
}

/*----------------------------------------------------------------------*/
static void sconrq(void)        /* CON-REQ senden                       */
{
  MBHEAD *mbp;                  /* Buffer fuer Frame                    */
  char   *viapoi;               /* Zeiger in Vialiste                   */

  l4pidx = cirpoi - &cirtab[0]; /* Index setzen                         */
  l4pcid = cirpoi->ideige;      /* Partner und eigener Index            */
  l4ahd2 =                      /* zwei Bytes leer                      */
  l4ahd3 = 0;
  l4aopc = L4CONREQ;            /* Opcode                               */
#ifdef __WIN32__
  putchr ((char)trawir, (mbp = gennhd()));    /* Rest des Headers             */
#else
  putchr (trawir, (mbp = gennhd()));    /* Rest des Headers             */
#endif /* WIN32 */
  putfid(cirpoi->upcall, mbp);  /* beide Calls in das Frame             */
  putfid(myid, mbp);
  putfid(cirpoi->upnod, mbp);   /* Uplinkknoten dazu                    */
  viapoi = cirpoi->upnodv;      /* Uplink-Vialiste auch                 */
  while (*viapoi != '\0')       /* Ende Vialiste erreicht?              */
   {
    putfid(viapoi, mbp);        /* Call in Puffer                       */
    viapoi = viapoi + L2IDLEN;  /* zum naechsten Call                   */
   }
  putchr('\0', mbp);            /* 0 markiert Ende Vialiste             */
  itol3(mbp);                   /* an Layer 3 liefern                   */
  l4setT3(cirpoi, L4TCREQ);     /* Timeout setzen                       */
}

/*----------------------------------------------------------------------*/
static void sdisrq(void)        /* DISQ-REQ senden                      */
{
  l4pidx = cirpoi->idxpar;      /* Index setzen                         */
  l4pcid = cirpoi->idpart;      /* und ID                               */
  l4ahd2 =                      /* 2 Bytes leer                         */
  l4ahd3 = 0;
  l4aopc = L4DISREQ;            /* Opcode                               */
  itol3(gennhd());              /* Rest des Headers und dann an Layer 3 */
  l4setT3(cirpoi, L4TDREQ);     /* Timeout setzen                       */
}

#ifdef NEW_L4
/*----------------------------------------------------------------------*/
static void spidchg(UBYTE pid)  /* PID-CHG senden (ohne Timeout)        */
{
  l4pidx = cirpoi->idxpar;      /* Index setzen                         */
  l4pcid = cirpoi->idpart;      /* und ID                               */
  l4ahd2 = 0;                   /* erste Byte leer                      */
  l4ahd3 = pid;                 /* zweite Byte enthaelt die PID         */
  l4aopc = L4PIDCHG;            /* Opcode                               */
  itol3(gennhd());              /* Rest des Headers und dann an Layer 3 */
}
#endif

/*----------------------------------------------------------------------*/
/* Fast L4 von DB7KG                                                    */
/*----------------------------------------------------------------------*/
static void sndack(void)        /* ACK senden                           */
{
  l4ahd2 = 0x00;
  l4aopc = L4INFACK;            /* Opcode                               */
  ackhdr();                     /* Rest des Headers                     */
  itol3(gennhd());              /* schnelle Weiterleitung               */
}

/*----------------------------------------------------------------------*/
static void ackhdr(void)        /* ACK Header erzeugen                  */
{
  l4pidx = cirpoi->idxpar;                      /* Partner Index        */
  l4pcid = cirpoi->idpart;                      /* Partner ID           */
  l4ahd3 = cirpoi->l4vr;                        /* RX-Sequenz           */

  cirpoi->ll4txNR = cirpoi->l4vr & 0xFF;        /* Letzten ACK senden   */

  if (cirpoi->l4flag & L4FBUSY)                 /* selbst choked        */
     l4aopc |= L4CCHOKE;                        /* dann Flag setzen     */
  else {
     if (cirpoi->l4rs == 1) {           /* wird es ein NAK Header?      */
        l4aopc |= L4CNAK;               /* dann Flag setzen             */
        cirpoi->l4rs = 2;               /* NAK als gesendet markieren   */
     }
  }
  cirpoi->acktim = 0;                   /* ACK Timer ruecksetzen        */
}

/*----------------------------------------------------------------------*/
static void itol3(MBHEAD *mbp)                  /* Info an Layer 3      */
{
  mbp->l2link = NULL;                           /* erstmal ggf. CRASHEN */
  cpyid(mbp->destcall, cirpoi->l3node);
  relink((LEHEAD *)mbp, (LEHEAD *) l3txl.tail); /* nur umhaengen        */
}

/*----------------------------------------------------------------------*/
MBHEAD *gennhd(void)            /* Netzwerk Header erzeugen             */
{
  int i;
  MBHEAD *mbp;                  /* Buffer fuer Info                     */

  mbp = (MBHEAD *) allocb(ALLOC_MBHEAD);        /* Buffer besorgen      */
  for (i = 0; i < 15; ++i)      /* die ersten 15 Bytes fuer Header leer */
     putchr(0, mbp);
  putchr(l4pidx, mbp);          /* Transport Header schreiben           */
  putchr(l4pcid, mbp);
  putchr(l4ahd2, mbp);
  putchr(l4ahd3, mbp);
  putchr(l4aopc, mbp);

  return(mbp);                  /* Buffer wird zurueckgegeben           */
}

/*----------------------------------------------------------------------*/
static void kilfra(void)        /* Fragmente loeschen                   */
{
  if (cirpoi->fragme != NULL) {                 /* schon leer?          */
     dealmb(cirpoi->fragme);                    /* Fragment loeschen    */
     cirpoi->fragme = NULL;                     /* Eintrag loeschen     */
  }
}

/*-------------------------------------------------------------------------*/
void l3tol4(NODE *totnod)     /* Meldung L3 -> L4: Knoten, auf den despoi  */
{                             /* zeigt, wird aus der Nodesliste gestrichen */
  CIRBLK  *nxtcir;

  for (cirpoi  = (CIRBLK *) l4actl.head;/* alle aktiven Circuits           */
       cirpoi != (CIRBLK *)&l4actl.head;/* durchgehen, Eintraege suchen,   */
       cirpoi  = nxtcir) {              /* die zum Zielknoten despoi gehen */
    nxtcir = (CIRBLK *) cirpoi->head;   /* vorher schon merken             */
    if (cmpid(cirpoi->l3node, totnod->id)) /* Zielknoten wurde geloescht   */
    {
      clr4rx(TRUE);      /* empfangene Frames abliefern                    */
      l4nsta(L4MFAILW);  /* Meldung an L7: Failure (Zielknoten ist weg)    */
    }
  }
}

#ifdef CONL3LOCAL
/* wartenden L4 circuit bestaetigen, CONACK */
void ackcir(CIRBLK *cp)
{
  MBHEAD *antwor;

  /* Partner Index        */
  l4pidx = cp->idxpar;
  /* Partner ID           */
  l4pcid = cp->idpart;
  /* Unseren Index setzen */
  l4ahd2 = (UBYTE) (cp - cirtab);
  /* Unsere (Zufalls-) ID */
  l4ahd3 = cp->ideige;
  /* Connect Acknowledge  */
  l4aopc = L4CONACK;
 /* endgueltige Fenstergroesse zurueck an den Partner senden */
  putchr(cp->window, (antwor = gennhd()));
  antwor->l2link = NULL;
  /* Call hinzufuegen. */
  cpyid(antwor->destcall, cp->l3node);
  /* Nur umhaengen. */
  relink((LEHEAD *)antwor,(LEHEAD *) l3txl.tail);
  /* Verbindungstatus auf information transfer setzen. */
  cp->state = L4SIXFER;
}

/* fehlgeschlagener L4 circuit bestaetigen, CONACK + CHOKE */
void bsycir(void)
{

 MBHEAD *antwor;

  /* Partner Index        */
  l4pidx = cirpoi->idxpar;
  /* Partner ID           */
  l4pcid = cirpoi->idpart;
  /* Unseren Index setzen */
  l4ahd2 = (UBYTE) (cirpoi - cirtab);
  /* Unsere (Zufalls-) ID */
  l4ahd3 = cirpoi->ideige;
  /* Connect Acknowledge  */
  l4aopc = L4CONACK | L4CCHOKE;
  /* endgueltige Fenstergroesse zurueck an den Partner senden */
  putchr(cirpoi->window, (antwor = gennhd()));
  antwor->l2link = NULL;
  /* Call hinzufuegen. */
  cpyid(antwor->destcall, cirpoi->l3node);
  /* Nur umhaengen. */
  relink((LEHEAD *)antwor,(LEHEAD *) l3txl.tail);
}
#endif

#ifdef L4KILL
/******************************************************************************/
/*                                                                            */
/*  L4-User killen (ccpkill kann nur L2).                                     */
/*                                                                            */
/******************************************************************************/
UWORD KillL4(char *call, char *msg)
{
  UWORD   kill_zaehler = 0,
          i;
  BOOLEAN kill = 0;

                                               /* Circuit Tabelle absuchen    */
  for (i = 0, cirpoi = cirtab; i < NUMCIR;++i, ++cirpoi)
  {
    if (cirpoi->state != L4SDSCED)                    /* Verbingung besteht   */
    {
      kill = (cmpid(call, cirpoi->upcall) ||     /* Rufzeichenvergleich. */
              cmpid(call, cirpoi->downca));

      if (kill)                                       /* Rufzeichen gefunden. */
      {
        if (*msg)                                    /* Mitteilung vom Sysop. */
        {
          MBHEAD *mbp;
                                                          /* Buffer besorgen. */
          if ((mbp = (MBHEAD *) allocb(ALLOC_MBHEAD)) != NULL)
          {
            putchr('\r', mbp);

#ifdef SPEECH
            putprintf(mbp, speech_message(207), msg);
#else
            putprintf(mbp, "\r*** Msg from Sysop: %s ***\r", msg);
#endif
            rwndmb(mbp);                                /* Msg zurueckspulen. */

            sndfrm(cirpoi->l4vs++, mbp);                        /* Msg senden */
            dealmb(mbp);                                 /* Buffer entsorgen. */
          }
        }

        kill_zaehler++;                                       /* zaehlen      */
        endcir();                                       /* Circuit aufloesen. */
      }
    }
  }

  return(kill_zaehler);
}
#endif /* L4KILL */

/* End of src/l4.c */
