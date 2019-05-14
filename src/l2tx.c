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
/* File src/l2tx.c (maintained by: DF6LN)                               */
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

/*----------------------------------------------------------------------*/
/*                                                                      */
/* "level 2 transmitter"                                                */
/*                                                                      */
/* Falls auf dem Port keine Aktivitaet ist (PTT aus) und fuer eine      */
/* Verbindung das Sendefenster noch nicht gefuellt ist, senden wir      */
/* soviele Frames, wie in das Fenster passen. Auf Vollduplex-Ports      */
/* sendet sdi() grundsaetzlich immer nur EIN Frame, damit zwischen-     */
/* zeitliche Aenderungen am ->VR gleich bestaetigt werden koennen.      */
/* (Dies waere nicht moeglich, wenn die Frames bereits im L1 Sender     */
/* sitzen und auf Sendung warten).                                      */
/* Wenn l2tx() einen Port als frei erkennt, werden fuer alle Links      */
/* Informationen gesendet. Der Sender ist dann wieder gesperrt, bis     */
/* alles auf dem Port gesendet wurde.                                   */
/* Die Linkliste (pro Port) haelt die letzten Links (also die Ver-      */
/* bindungen, die zuletzt bedient und bestaetigt wurden) am Ende,       */
/* deshalb wird sie hier von vorn nach hinten gelesen. Damit wird       */
/* eine gleichmaessige Verteilung erreicht, es gibt kein "Festsaugen"   */
/* an einem Link.                                                       */
/*                                                                      */
/* Frames aus Gesendet-Liste holen und in die Monitorframeliste um-     */
/* haengen. Entsprechend dem Frameinhalt ggf. Timer 1 starten.          */
/*                                                                      */
/*----------------------------------------------------------------------*/
void
l2tx(void)
{
  WORD    port;                 /* Laufindex                            */
  LHEAD  *llp;
  int     n;

  for (llp = &l2actl[port = 0]; /* alle Ports durchgehen                */
       port < L2PNUM;
       port++, llp++)
  {
    if (busy(port))
      continue;                 /* Port ist noch blockiert              */
    if (dama(port))
      continue;                 /* nicht senden auf DAMA-Ports          */
#ifdef DAMASLAVE
    if (damaslaveon(port))
      continue;                 /* nicht senden auf DAMA-Slave-Ports    */
#endif

    for (lnkpoi = (LNKBLK *)llp->head;
         lnkpoi != (LNKBLK *)llp;       /* alle Links des Ports pruefen */
         lnkpoi = lnkpoi->next)
    {
      if (lnkpoi->RStype != L2CREJ)     /* REJ nicht durch I ersetzen   */
      {
        if (lnkpoi->flag & L2FREPEAT)
          sdoi();
        else
          if ((n = itxwnd()) > 0)      /* darf ich ueberhaupt noch was? */
            sdi(outsdI(), n);          /* Frames generieren (mit clrT2) */
      }
    }
  }
}

/************************************************************************/
/*                                                                      */
/* Gesendetliste aufraeumen                                             */
/*                                                                      */
/************************************************************************/
void
clrstfl(void)
{
  MBHEAD *sfbp;                 /* Sendeframebufferpointer              */

  while ((sfbp = (MBHEAD *)stfl.head) != (MBHEAD *)&stfl)
  {
    ulink((LEHEAD *)sfbp);                      /* Frame holen          */
    if ((sfbp->l2fflg & L2FT1ST) != FALSE)      /* ist T1 zu starten ?  */
    {
      lnkpoi = sfbp->l2link;                    /* Zeiger auf Linkblock */
      setT1();                                  /* T1 starten           */
    }
    sfbp->tx = 1;
    if (takfhd(sfbp))
                monitor(sfbp);
          dealmb(sfbp);
  }
}

/************************************************************************/
/*                                                                      */
/* Frame-Reject Frame aufbauen und senden                               */
/*                                                                      */
/************************************************************************/
void
sdfrmr(char ZYXW)
{
  UBYTE *frmrip;

  if (lnkpoi->state >= L2SIXFER && lnkpoi->state != L2SHTH)
  {
    frmrip = lnkpoi->frmr;
#ifdef EAX25
    /* EAX.25-FRMR aufbauen */
    if (lnkpoi->bitmask == 0x7F)
    {
      *frmrip++ = rxfctl;
      if ((rxfctl & 0x03) == 3)
        *frmrip++ = 0;
      else
        *frmrip++ = rxfctlE;

      *frmrip++ = (lnkpoi->VS << 1) & 0xFE;
      *frmrip++ = (lnkpoi->VR << 1) | (!rxfCR ? 0x1 : 0x0);
      *frmrip = ZYXW;
    }
    else
    {
#endif
      /* AX.25-FRMR aufbauen */
      *frmrip++ = rxfctl | rxfPF;
      *frmrip++ =   (lnkpoi->VR << 5)
                  | (!rxfCR ? 0x10 : 0)
                  | (lnkpoi->VS << 1);
      *frmrip = ZYXW;
#ifdef EAX25
    }
#endif
#if MAX_TRACE_LEVEL > 0
/***DEBUG***/
    notify(1, "Frame rejected %6.6s->%6.6s VS=%u ltxdNR=%u VR=%u"
           " lrxdNR=%u error=%u state=%u",
           lnkpoi->srcid, lnkpoi->dstid,
           lnkpoi->VS, lnkpoi->ltxdNR,
           lnkpoi->VR, lnkpoi->lrxdNR,
           ZYXW, lnkpoi->state);
/***DEBUG***/
#endif
    clrT1();                    /* Timer stoppen                        */
    clrT2();
    lnkpoi->RTT = 0;
    l2stma(stbl27);             /* INVALID N(R) RECEIVED                */
  }
}

/************************************************************************/
/*                                                                      */
/* "xmit null"                                                          */
/*                                                                      */
/* Nichts tun. Leerfunktion fuer die Statetable.                        */
/*                                                                      */
/************************************************************************/
void
xnull(void)
{
}

/************************************************************************/
/*                                                                      */
/* "xmit RR command"                                                    */
/*                                                                      */
/* RR als Command senden.                                               */
/*                                                                      */
/************************************************************************/
void
xrrc(void)
{
  stxcfr();
  sendS(L2CRR);
}

/************************************************************************/
/*                                                                      */
/* "xmit RR response"                                                   */
/*                                                                      */
/* RR als Response senden.                                              */
/*                                                                      */
/************************************************************************/
void
xrrr(void)
{
  sendS(L2CRR);
}

/************************************************************************/
/*                                                                      */
/* "xmit RNR command"                                                   */
/*                                                                      */
/* REJ als Command senden.                                              */
/*                                                                      */
/************************************************************************/
void
xrnrc(void)
{
  stxcfr();
  xrnrr();
}

/************************************************************************/
/*                                                                      */
/* "xmit RNR response"                                                  */
/*                                                                      */
/* RNR als Response senden.                                             */
/*                                                                      */
/************************************************************************/
void
xrnrr(void)
{
  sendS(L2CRNR);
}

/************************************************************************/
/*                                                                      */
/* "xmit REJ response"                                                  */
/*                                                                      */
/* REJ als Response senden.                                             */
/*                                                                      */
/************************************************************************/
void
xrejr(void)
{
  sendS(L2CREJ);
}

/************************************************************************/
/*                                                                      */
/* "send supervisory frame"                                             */
/*                                                                      */
/* Ein Supervisory-Frame aufbauen, Timer 2 loeschen und das Frame       */
/* an den aktuellen Link senden.                                        */
/*                                                                      */
/************************************************************************/
void
sendS(UBYTE control)
{
  clrT2();

#ifdef __WIN32__
  txfctl = (unsigned char)setNR(control);
#else
  txfctl = setNR(control);
#endif /* WIN32 */
#ifdef EAX25
  /* Kontrollbytes fuer EAX.25 aufbauen bzw. umbauen */
  if (lnkpoi->bitmask == 0x7F)
  {
    txfctl &= 0x0F;  /* erstes Kontrollbyte aendern */
    txfctlE = (lnkpoi->VR << 1) | (txfPF >> 4); /* V(R) und Pollflag    */
    txfEAX = TRUE;
  }
#endif

  sdl2fr(makfhd((!txfCR ? L2FUS : L2FUS | L2FT1ST)), FALSE);
}

/************************************************************************/
/*                                                                      */
/* "xmit DM"                                                            */
/*                                                                      */
/* Ein DM-Frame generieren und an die aktuelle Adresse (txf...)         */
/* senden.                                                              */
/*                                                                      */
/************************************************************************/
void
xdm(void)
{
  txfctl = L2CDM;
  sdl2fr(makfhd(L2FUS), TRUE);
}

/************************************************************************/
/*                                                                      */
/* "xmit UA"                                                            */
/*                                                                      */
/* Ein UA-Frame generieren und an die aktuelle Adresse (txf...)         */
/* senden.                                                              */
/*                                                                      */
/************************************************************************/
void
xua(void)
{
  txfctl = L2CUA;
  sdl2fr(makfhd(L2FUS), TRUE);
}

/************************************************************************/
/*                                                                      */
/* "xmit SABM"                                                          */
/*                                                                      */
/* Ein SABM-Frame generieren und an die Adresse des aktuellen Linkblock */
/* senden.                                                              */
/*                                                                      */
/************************************************************************/
void
xsabm(void)
{
  stxcfr();
#ifdef EAX25
  /* bei gewuenschter EAX.25-Verbindung je nach Porteinstellung einen   */
  /* anderen Kopf aufbauen */

  /* Modus bestimmen je nach Einstellung des Ports */
  switch (portpar[lnkpoi->liport].eax_behaviour)
  {
    /* EAX.25 auf diesem Port nicht erlaubt */
    case 0: lnkpoi->bitmask = 0x07;
            break;

    /* EAX.25 auf diesem Port ist zwingend !!! */
    case 3: lnkpoi->bitmask = 0x7F;
            break;

    /* EAX.25 auf diesem Port nach MHeard-Tabelle */
    default: break;
  }

  if (lnkpoi->bitmask == 0x7F)
    txfctl = L2CSABME;
  else
#endif
    txfctl = L2CSABM;

#ifdef RTTSTART_MOD
  {
    PEER *pSeg;
    PEER *peertab = netp->peertab;    /* Segment-Tabelle                */

                                         /* Segment-Tabelle durchgehen. */
    for (pSeg = peertab; pSeg < &peertab[netp->max_peers]; pSeg++)
    {
      if (!pSeg->used)                         /* Unbenutzte Eintraege, */
        continue;                              /* zum naechsten.        */

      if (cmpid(pSeg->l2link->call, lnkpoi->dstid)) /* Callvergleich.   */
        pSeg->rttstart = tic10;                 /* Zeitmessung starten. */
    }
  }
#endif /* RTTSTART_MOD */

  sdl2fr(makfhd((L2FUS | L2FT1ST)), FALSE);
}

/************************************************************************/
/*                                                                      */
/* "xmit DISC"                                                          */
/*                                                                      */
/* Ein DISC-Frame generieren und an die Adresse des aktuellen Linkblock */
/* senden.                                                              */
/*                                                                      */
/************************************************************************/
void
xdisc(void)
{
  stxcfr();
  txfctl = L2CDISC;
  sdl2fr(makfhd((L2FUS | L2FT1ST)), FALSE);
}

/************************************************************************/
/*                                                                      */
/* "xmit FRMR"                                                          */
/*                                                                      */
/* FRMR-Frame generieren und mit der RIP-Information aus dem Linkblock  */
/* fuellen.                                                             */
/*                                                                      */
/************************************************************************/
void
xfrmr(void)
{
  UBYTE  *frmrip;
  MBHEAD *fbp;

  stxfad();
  txfctl = L2CFRMR;
  fbp = makfhd((L2FUS | L2FT1ST));
  frmrip = lnkpoi->frmr;
#ifdef EAX25
  /* EAX.25 FRMR ist zwei Bytes laenger */
  if (lnkpoi->bitmask == 0x7F)
  {
    putchr(*frmrip++, fbp);
    putchr(*frmrip++, fbp);
  }
#endif
  putchr(*frmrip++, fbp);
  putchr(*frmrip++, fbp);
  putchr(*frmrip, fbp);
  sdl2fr(fbp, TRUE);
}

/************************************************************************/
/*                                                                      */
/* "set tx command frame"                                               */
/*                                                                      */
/* TX-Frame-Adressierung setzen (siehe stxfad()) und Frame zum          */
/* Kommandoframe machen mit gesetztem Pollbit (txfCR, txfPF).           */
/*                                                                      */
/************************************************************************/
void
stxcfr(void)
{
  stxfad();                             /* Adressierung                 */
  txfCR = L2CCR;                        /* Command!                     */
#ifdef DAMASLAVE
  if (damaslaveon(txfprt))              /* Slave darf nicht pollen      */
    txfPF = 0;
  else
#endif
    txfPF = L2CPF;                      /* Pollbit!                     */
}

/************************************************************************/
/*                                                                      */
/* "set tx frame address"                                               */
/*                                                                      */
/* Adressierung des aktuellen Sendeframes (txfhdr, txfprt) setzen aus   */
/* den im aktuellen Linkblock (lnkpoi) gegebenen Parametern (srcid,     */
/* destid, viaidl, liport).                                             */
/*                                                                      */
/************************************************************************/
void
stxfad(void)
{
  cpyid(txfhdr + L2IDLEN, lnkpoi->srcid);       /* von ...              */

  /*
   *    DAMA-Bit loeschen, wenn DAMA-Betrieb (geloeschtes Bit = DAMA !)
   */
  if (dama(lnkpoi->liport))
    txfhdr[L2ILEN - 1] &= ~L2CDAMA;

#ifdef EAX25
  if (lnkpoi->bitmask == 0x7F)
    txfhdr[L2ILEN - 1] &= ~L2CEAX;
#endif

  cpyid(txfhdr, lnkpoi->dstid);                 /* nach ...             */
  cpyidl(txfhdr + L2ILEN, lnkpoi->viaidl);      /* ueber ...            */

  txfprt = lnkpoi->liport;                      /* auf Port ...         */
}

/************************************************************************/
/*                                                                      */
/* "set NR"                                                             */
/*                                                                      */
/* Im aktuellen Linkblock (lnkpoi) die zuletzt gesendete N(R) (ltxdNR)  */
/* auf V(R) (VR) setzen und Framecontrolbyte control fuer Frameaus-     */
/* sendung mit der N(R) versehen und zurueckgeben.                      */
/*                                                                      */
/* Return :  control mit N(R) versehen                                  */
/*                                                                      */
/************************************************************************/

UBYTE
setNR(UBYTE control)
{
  lnkpoi->ltxdNR = lnkpoi->VR;                /* neue N(R)              */
  return (((lnkpoi->VR << 5) | control));     /* N(R) ins Kontrollfeld  */
}

/************************************************************************/
/*                                                                      */
/* "send level 2 frame"                                                 */
/*                                                                      */
/* Framebuffer, auf dessen Kopf fbp zeigt, rewinden und in die dem Port */
/* (l2port) entsprechende Level-2-Sendeframeliste einhaengen, wenn noch */
/* genug Buffer im System frei sind. Andernfalls nicht senden, sondern  */
/* sofort in die Gesendet-Liste (stfl) einhaengen.                      */
/*                                                                      */
/************************************************************************/
void
sdl2fr(MBHEAD *fbp, BOOLEAN send_immediately)
{
  UBYTE port;                           /* Portnummer                   */

#ifndef __WIN32__
  port = fbp->l2port;                   /* Portnummer holen             */
#else
  port = (unsigned char)fbp->l2port;    /* Portnummer holen             */
#endif /* WIN32 */

  if (nmbfre > 14 && portenabled(port)) /* noch genug Buffer und Port   */
  {                                     /* eingeschaltet?               */
    rwndmb(fbp);                        /* ja - Framebuffer rewinden    */
                                        /* Frame in Sendeliste          */
#ifdef L1TCPIP
    /* TCPIP-Frames haben hier nix zu suchen. */
    if (CheckPortTCP((UWORD)port))
    {
      /* Frame entsorgen. */
      dealmb(fbp);
      return;
    }
#endif /* L1TCPIP */

    if (  dama(port)          /* DAMA-Frames werden in timDAMA gesendet */
#ifdef DAMASLAVE
        || damaslaveon(port)  /* DAMA-Slave-Frames auch                 */
#endif
       )
    {
      if (send_immediately == FALSE)
        relink((LEHEAD *)fbp, (LEHEAD *)(lnkpoi->damail.tail));
      else
        relink((LEHEAD *)fbp, (LEHEAD *)&damarl[port]);
      return;
    }
    relink((LEHEAD *)fbp, (LEHEAD *)txl2fl[port].tail);
    kicktx(port);                       /* es ist was zu senden !       */
  }
  else /* kein Platz oder Port aus - Frame als gesendet betrachten      */
  {
    relink((LEHEAD *)fbp, (LEHEAD *)stfl.tail);
  }
}

/************************************************************************/
/*                                                                      */
/* "make frame header"                                                  */
/*                                                                      */
/* Die Header-Daten aus "txf.." werden in einen Buffer geschrieben.     */
/* Sie wurden entweder von getfhd() aus dem Empfangsframe durch         */
/* Spiegelung generiert oder aus dem Linkblock gesetzt.                 */
/*                                                                      */
/************************************************************************/
MBHEAD *
makfhd(int fflag)
{
  MBHEAD *fbp;

  txfhdr[L2IDLEN - 1] |= txfCR;
  txfhdr[L2ILEN - 1] |= txfCR ^ L2CCR;

  putfid(txfhdr, fbp = (MBHEAD *)allocb(ALLOC_MBHEAD));

  if (dama(txfprt))
    txfhdr[L2ILEN - 1] &= ~L2CDAMA;

#ifdef EAX25
  if ((txfEAX == TRUE) && ((txfctl & 0x03) != 0x03))
    txfhdr[L2ILEN - 1] &= ~L2CEAX;
  else
    txfhdr[L2ILEN - 1] |= L2CEAX;
#endif

  putfid(txfhdr + L2IDLEN, fbp);
  putvia(txfhdr + L2ILEN, fbp);

#ifdef EAX25
  if ((txfEAX == TRUE) && ((txfctl & 0x03) != 0x03))
  {
    putchr((UBYTE)(txfctl), fbp);
    putchr((UBYTE)(txfctlE | (txfPF >> 4)), fbp);
    txfEAX = FALSE;
  }
  else
#endif
    putchr((UBYTE)(txfctl | txfPF), fbp);

  fbp->l2link = lnkpoi;
  fbp->type = 2;
  fbp->l2fflg = fflag;
  fbp->l2port = txfprt;
  fbp->repeated = 0;
  return (fbp);
}

/************************************************************************\
*                                                                        *
* "information to link"                                                  *
*                                                                        *
* Infobuffer, auf den imbp zeigt, an in diesem Buffer festgelegten Link  *
* (l2link) zwecks Aussendung als Paket weitergeben. Wenn nocgnc == TRUE  *
* keine "Erstickungskontrolle", sonst conctl beachten (s.u.).            *
* Der Infobuffer wird bei Weitergabe an den Link mit der normalen        *
* Level 2 PID versehen, der Keine-Aktivitaets-Timer wird neu gestartet.  *
*                                                                        *
* Return:  TRUE  - imbp wurde angenommen und an den Link weitergegeben   *
*          FALSE - imbp wurde nicht angenommen wegen Congestion Control  *
*                  = Grenze der pro Link maximal zu speichernden Pakete  *
*                    (conctl) wuerde ueberschritten werden               *
*                                                                        *
\************************************************************************/
BOOLEAN
itolnk(int pid, BOOLEAN nocgnc, MBHEAD *imbp)
{
  LNKBLK *linkp;
#if 0
  extern void coredump(void);
#endif

  linkp = imbp->l2link;

/* DEBUG */
  if (linkp->state == L2SDSCED)
  {
    LNKBLK         *l_bak;

    l_bak = lnkpoi;                     /* damit wir das im Dump sehen  */
    lnkpoi = linkp;
#if MAX_TRACE_LEVEL > 2
    notify(1, "send to disc'ed link %6.6s > %6.6s via %s",
           linkp->srcid, linkp->dstid, linkp->viaidl);
#endif
#if 0
    coredump();
#endif
    dealmb((MBHEAD *)ulink((LEHEAD *)imbp));
    lnkpoi = l_bak;
    return (TRUE);
  }

  if (linkp->state == L2SDSCRQ)
  {
    dealmb((MBHEAD *)ulink((LEHEAD *)imbp));
    return (TRUE);
  }

  if (linkp->tosend < conctl || nocgnc == TRUE)
  {
    imbp->l2fflg = pid;                         /* PID uebernehmen      */
    imbp->repeated = 0;
    relink(ulink((LEHEAD *)imbp),               /* -> ab in den Link    */
           (LEHEAD *)linkp->sendil.tail);
    ++linkp->tosend;                            /* ein Sendepaket mehr  */

#ifndef THENETMOD
    linkp->noatou = ininat;
#else                                           /* L4TIMEOUT */
    linkp->noatou = SetL4Timeout();             /* L2/L4-Timeout setzen.*/
#endif /* THENETMOD */
    return (TRUE);                              /* ... imbp angenommen  */
  }
  return (FALSE);                               /* ... imbp abgelehnt   */
}

/************************************************************************\
*                                                                        *
* "number of outstanding I's"                                            *
*                                                                        *
* Anzahl der ausstehenden Info-Frames des aktuellen Linkblock (lnkpoi)   *
* berechnen.                                                             *
*                                                                        *
\************************************************************************/
char
outsdI(void)
{
  return ((lnkpoi->VS - lnkpoi->lrxdNR) & lnkpoi->bitmask);
}

/************************************************************************\
*                                                                        *
* "is tx window open"                                                    *
*                                                                        *
* Kann der Sender des aktuellen Linkblocks noch Informationen aufnehmen? *
* Geliefert wird die Anzahl der Frames, die noch in das aktuelle Fenster *
* passen.                                                                *
*                                                                        *
\************************************************************************/
char
itxwnd(void)
{
  int outstd,
      n,
      k;

  if (lnkpoi->tosend)                   /* ueberhaupt was zu senden?    */
  {
    switch (lnkpoi->state)              /* duerfen wir was senden?      */
      {
        case L2SIXFER:
        case L2SRS:
        case L2SDBS:
        case L2SRSDBS:
          outstd = outsdI();            /* soviel stehen noch aus       */
          n = lnkpoi->tosend;           /* und soviel ist noch uebrig   */
          k = lnkpoi->maxframe;
          if (   (outstd < k)           /* Fenster noch nicht voll      */
              && (n > outstd))          /* und Sendebuffer nicht leer   */
            return (fullduplex(lnkpoi->liport) ? 1 : k);
      }
  }
  return (0);                          /* senden momentan nicht erlaubt */
}

/************************************************************************\
*                                                                        *
* "send outstanding I's"                                                 *
*                                                                        *
* Aus dem aktuellen Linkblock (lnkpoi) soviele I-Frames senden, wie im   *
* Moment unbestaetigt ausstehen.                                         *
* Es wird der komplette Durchgang wiederholt (ab dem 1. unbestaetigtem)  *
* Frame.                                                                 *
* WICHTIG: Nicht senden, wenn wir auf die Antwort der Gegenstation       *
*          warten (wir haben Poll gesendet). Dann kommt sowieso der      *
*          Final demnaechst und wir koennen weitersenden.                *
*                                                                        *
\************************************************************************/
void
sdoi(void)
{
  UWORD n;                              /* Anzahl I's zu senden         */
  int   VS;

  switch (lnkpoi->state)                /* nur wenn der State stimmt    */
  {
    case L2SIXFER:
    case L2SRS:
    case L2SDBS:
    case L2SRSDBS:
      if ((n = outsdI()) != 0)        /* wieviel darf ich?            */
      {
        VS = lnkpoi->VS;
        lnkpoi->VS = lnkpoi->lrxdNR;  /* V(S) resetten                */
        sdi(0, n);                    /* I's senden                   */
        lnkpoi->VS = VS;
      }
      lnkpoi->flag &= ~L2FREPEAT;
      break;
  }
}

/************************************************************************\
*                                                                        *
* "send I"                                                               *
*                                                                        *
* Aus dem aktuellen Linkblock (lnkpoi) maximal max I-Frames aus der      *
* Infomessageliste aufbauen und senden. Die Frames werden als Command-   *
* frames ohne Poll/Final-Bit gesendet. V(S) wird fuer jedes gesendete    *
* Frame erhoeht modulo 7. Timer 2 wird abgeschaltet.                     *
*                                                                        *
\************************************************************************/
void
sdi(int outstd, int max)
{
  WORD    n,
          m,
          k;                            /* Zaehler zu sendende Infos    */
  MBHEAD *sendip;                       /* Kopfzeiger Infobuffer        */
  MBHEAD *fbp;                          /* Kopfzeiger Framebuffer       */

  sendip = (MBHEAD *)lnkpoi->sendil.head;       /* erstes Frame         */
  for (n = 0; n < outstd; n++)
    sendip = (MBHEAD *)sendip->nextmh;  /* soviele uebergehen           */

  if (max > lnkpoi->tosend - outstd)    /* maximal soviel wie da ist    */
    max = lnkpoi->tosend - outstd;

  k = lnkpoi->maxframe;

  if (max + outstd > k)                 /* und nicht uebers Fenster     */
    max = outstd < k
          ? k - outstd                  /* Rest bis zum Fensterende     */
          : 0;                          /* Fenster wurde verkleinert    */

  for (n = 1;
       n <= max;                        /* aus der Linkblock-           */
       ++n, sendip = (MBHEAD *)sendip->nextmh)  /* infoliste senden     */
  {                                     /* wenn vorhanden               */
    stxfad();                           /* Frameadresse aufbauen        */
    txfCR = L2CCR;                      /* Command!                     */

#ifdef EAX25
    /* EAX.25-Kontrollbytes setzen */
    if (lnkpoi->bitmask == 0x7F)
    {
      txfctl = (lnkpoi->VS << 1) & 0xFE;          /* Controlbyte 1 setzen */
      txfctlE = (lnkpoi->VR << 1) & 0xFE;         /* Controlbyte 2 setzen */
      lnkpoi->ltxdNR = lnkpoi->VR;                /* neue N(R)            */
      txfEAX = TRUE;
    }
    else
    {
#endif
      /* AX.25-Kontrollbyte setzen */
#ifdef __WIN32__
      txfctl = (unsigned char)setNR((char)(lnkpoi->VS << 1));    /* Controlbyte I setzen */
#else
      txfctl = setNR((char)(lnkpoi->VS << 1));    /* Controlbyte I setzen */
#endif /* WIN32 */
#ifdef EAX25
    }
#endif
    ++lnkpoi->VS;                               /* V(S) erhoehen        */
    lnkpoi->VS &= lnkpoi->bitmask;              /* modulo               */

    txfPF = 0;                          /* normalerweise ohne POLL      */
    if (   (((lnkpoi->lrxdNR + k) & lnkpoi->bitmask) == lnkpoi->VS)
        || (lnkpoi->tosend == outsdI()))
    {
      if (dama(lnkpoi->liport))
        txfPF = L2CPF;                  /* letztes Frame mit POLL!      */
      fbp = makfhd(L2FT1ST);            /* Ja, T1 und RTT Zeitmessung   */
    }
    else
      fbp = makfhd(0x00);               /* Nein, T1/RTT nicht starten   */

    fbp->repeated = sendip->repeated;
    putchr(sendip->l2fflg, fbp);        /* Frame aufbauen, PID          */

    /*
     *  Frame aufspalten, wenn zu lang. Gesplittet werden aber nur
     *  Frames mit Standard-AX.25-PID (ohne L3-Protokoll). Die maximale
     *  Laenge ist 256 Bytes bzw. wenn niedriger der MTU-Wert fuer den
     *  jeweiligen Port.
     */
    if (sendip->l2fflg == L2CPID)
#ifdef PORT_MANUELL
      m = portpar[lnkpoi->liport].paclen;
#else
      m = L2MILEN;                      /* Fest auf 256 Bytes begrenzen */
#endif /* PORT_MANUELL */
        else
      m = min(L2MILEN, portpar[lnkpoi->liport].mtu);

    if (splcpy(m, fbp, sendip) == TRUE) /* Ein Frame mehr zu senden     */
      ++lnkpoi->tosend;

#ifdef MAXFRAMEDEBUG
    fbp->lnkflag = lnkpoi->flag;
    fbp->lmf = lnkpoi->maxframe;
#ifdef EAX25
    if (lnkpoi->bitmask == 0x7F)
#ifdef __WIN32__
      fbp->pmf = (unsigned char)portpar[lnkpoi->liport].maxframe_eax;
#else
      fbp->pmf = portpar[lnkpoi->liport].maxframe_eax;
#endif /* WIN32 */
    else
#endif
#ifdef __WIN32__
      fbp->pmf = (unsigned char)portpar[lnkpoi->liport].maxframe;
#else
      fbp->pmf = portpar[lnkpoi->liport].maxframe;
#endif /* WIN32 */
    fbp->tosend = lnkpoi->tosend;
#endif

    sdl2fr(fbp, FALSE);          /* Frame senden                        */
    ++sendip->repeated;          /* Frame wird ggf. wiederholt gesendet */
    clrT2();                     /* Timer 2 abschalten                  */
  }
}

/************************************************************************\
*                                                                        *
* "send UI"                                                              *
*                                                                        *
* UI-Frame aufbauen und senden. Das UI-Frame wird an ID dest geschickt   *
* ueber den Port port und die via-Liste (nullterminiert) vial, als       *
* Quelle wird source genommen, die Infobytes des Frames stehen im        *
* Messagebuffer, auf dessen Kopf mbhd zeigt, die PID wird aus l2fflg     *
* dieses Buffers genommen.                                               *
*                                                                        *
\************************************************************************/
void
sdui(const char *vial, const char *dest, const char *source, char port,
     MBHEAD *mbhd)
{
  MBHEAD *fbp;

#ifdef L1TCPIP
  /* TCPIP-Frames haben hier nix zu suchen. */
  if (CheckPortTCP((UWORD)port))
    return;
#endif /* L1TCPIP */

  cpyid(txfhdr + L2IDLEN, source);
  cpyid(txfhdr, dest);
  cpyidl(txfhdr + L2ILEN, vial);
  txfprt = port;
  txfCR = L2CCR;
#ifndef UIPOLLFIX
  txfPF = 0;
#else
  txfPF = L2CPF;
#endif /* UIPOLLFIX */
  txfctl = L2CUI;
  putchr(mbhd->l2fflg, fbp = makfhd(0));

  while (mbhd->mbgc < mbhd->mbpc)
    putchr(getchr(mbhd), fbp);

  sdl2fr(fbp, TRUE);
}

#ifdef IPOLL_FRAME
void sdipoll()
 {
    static MBHEAD     *sendip;               /* Kopfzeiger Infobuffer     */
    static MBHEAD     *fbp;                  /* Kopfzeiger Framebuffer    */

    sendip = (MBHEAD *)lnkpoi->sendil.head;
    stxfad();                                /* Frameadresse aufbauen     */
    if (   sendip->mbpc < portpar[lnkpoi->liport].ipoll_paclen  /* FEF wird's ein IPoll ?    */
        && lnkpoi->tries < portpar[lnkpoi->liport].ipoll_retry
        && lnkpoi->tosend)
     {
      txfCR = L2CCR;                         /* Command !                 */
      txfPF = L2CPF;                         /* Poll/Final !              */
#ifdef __WIN32__
      txfctl = setNR((UBYTE)(lnkpoi->lrxdNR << 1));   /* Controlbyte I wie zuvor   */
#else
      txfctl = setNR(lnkpoi->lrxdNR << 1);   /* Controlbyte I wie zuvor   */
#endif /* WIN32 */
      putchr(sendip->l2fflg,                 /* Frame aufbauen, PID       */
             fbp = makfhd(L2FT1ST));
      splcpy(256,fbp,sendip);                /* Message umkopieren        */
      sdl2fr(fbp, FALSE);                           /* Frame senden              */
      clrT2();                               /* Timer 2 abschalten        */
      clrT3();                               /* Timer 3 abschalten        */
     }
    else                                     /* FEF Nein, kein Ipoll...   */
      xrrc();                                /* FEF ... RR+ senden        */
 }
#endif /* IPOLL_FRAME */

/* End of src/l2tx.c */
