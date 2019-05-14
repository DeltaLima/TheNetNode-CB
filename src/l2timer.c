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
/* File src/l2timer.c (maintained by: DF6LN)                            */
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

/************************************************************************\
*                                                                        *
* "level 2 timer"                                                        *
*                                                                        *
* Ausfuehren der Level-2-Millisekundentimer 1, 2, 3 in allen aktiven     *
* Links (herunterzaehlen und bei Ablauf reagieren).                      *
* In ticks wird die Anzahl der vergangenen 10ms-Intervalle (Ticks) seit  *
* dem letzten Aufruf dieser Routine angegeben.                           *
* Die Reaktion auf den T2 wird in l2tx() durchgefuehrt.                  *
*                                                                        *
* Hinweis zum Datendurchsatz:                                            *
* Da i2tolx() maximal alle 10ms ausgefuehrt wird, ist der Durchsatz      *
* AUF EINEM LINK auf etwa                                                *
*                                                                        *
* (NoAckBuf * Framelaenge * 1s/10ms * 8) 10*256*100*8 = 2048000 Bits/s   *
*                                                                        *
* beschraenkt, das ist erstmal keine echte Huerde, sollte aber im Auge   *
* behalten werden.                                                       *
*                                                                        *
\************************************************************************/
void
l2timr(void)
{
  int             port;
  LHEAD          *llp;
  LNKBLK         *nextlp;
  static ULONG    last_tic = 0L;
  UWORD           ticks;
#if MAX_TRACE_LEVEL > 2
  char            notify_call1[10];
  char            notify_call2[10];
#endif

  if ((ticks = (UWORD)(tic10 - last_tic)) != 0)
  {
    last_tic = tic10;

    for (llp = &l2actl[port = 0];       /* alle Ports durchgehen        */
         port < L2PNUM;
         port++, llp++)
      for (lnkpoi = (LNKBLK *)llp->head;
           lnkpoi != (LNKBLK *)llp;     /* alle Links des Ports pruefen */
           lnkpoi = nextlp)
      {
        nextlp = lnkpoi->next;          /* Nachfolger schonmal merken   */

        if (lnkpoi->flag & L2FACKHTH)   /* Bestaetigung von HTH         */
        {
          if (lnkpoi->state == L2SHTH)
          {
            stxfad();                   /* Sendepfad setzen             */
            txfCR = 0;                  /* Response                     */
            txfPF = L2CPF;              /* Final                        */
            xua();                      /* UA- als Bestaetigung         */
            l2newstate(L2SIXFER);       /* neuer State: Connected       */
          }
          lnkpoi->flag &= ~L2FACKHTH;   /* Flag loeschen                */
        }
        if (lnkpoi->flag & L2FREJHTH)   /* Ablehnung von HTH            */
        {
          if (lnkpoi->state == L2SHTH)
          {
            stxfad();                   /* Sendepfad setzen             */
            txfCR = 0;                  /* Response                     */
            txfPF = L2CPF;              /* Final                        */
            xdm();                      /* DM- als Bestaetigung         */
          }
          lnkpoi->flag &= ~L2FREJHTH;   /* Flag loeschen                */
        }
/* Wenn RTT-Messung freigegeben ist, dann RTT um ticks erhoehen         */
        if (lnkpoi->RTT != 0)
          lnkpoi->RTT += ticks;

        if (lnkpoi->T3 != 0)            /* wenn Timer 3 aktiv ...       */
        {
          if (lnkpoi->T3 <= ticks)      /* wenn Timer 3 abgelaufen ...  */
          {
            clrT3();                    /* ... Timer 3 stoppen und      */
            l2stma(stbl24);             /* Statetable T3 EXPIRES        */
          }                             /* ausfuehren                   */
          else
            lnkpoi->T3 -= ticks;        /* sonst herunterzaehlen        */
        }
#ifdef L2PROFILER
        l2profiler();                   /* Spielzeug DB7KG              */
#endif

        if (   (lnkpoi->tosend > 150)   /* Der Link laeuft ueber..      */
            && (lnkpoi->state >= L2SIXFER)
            && (lnkpoi->state != L2SHTH))
        {
#if MAX_TRACE_LEVEL > 2
          call2str(notify_call1, lnkpoi->srcid);
          call2str(notify_call2, lnkpoi->dstid);
          notify(3, "%s->%s: too many frames in queue",
                 notify_call1, notify_call2);
#endif
          lnkpoi->flag |= L2FDIMM;      /* dann abwerfen                */
        }

        if (lnkpoi->flag & L2FDIMM)     /* Link sofort kappen           */
        {
          l2stma(stbl20);               /* LOCAL STOP COMMAND           */
          lnkpoi->flag &= ~L2FDIMM;
          continue;
        }

        if (   (lnkpoi->flag & L2FDSLE) /* Disconnect wenn alles raus?  */
            && (!lnkpoi->tosend))       /* und nix mehr anstehend?      */
        {
          reslnk();                     /* Sequenzvars/Timer zurueck    */
          l2stma(stbl20);               /* LOCAL STOP COMMAND           */
          continue;
        }

/*----------------------------------------------------------------------*/
/* sonst empfangene I-Pakete an hoeheren Level uebertragen und          */
/* Busy-Condition pruefen / setzen / aufheben                           */
/*                                                                      */
/* "Busy werden"    - weniger als 30 Freibuffer oder so viele I-Pakete  */
/*                    empfangen und nicht abgeholt, wie "Erstickungs-   */
/*                    zaehler" conctl angibt                            */
/*                                                                      */
/* "Busy aufloesen" - wieder mehr als 62 Freibuffer und weniger als     */
/*                    halb so viele empfangen und nicht abgeholt wie    */
/*                    conctl angibt                                     */
/*----------------------------------------------------------------------*/
        i2tolx(FALSE);
        if (!(lnkpoi->flag & L2FBUSY))          /* nicht busy           */
        {
          if (nmbfre < 30 || lnkpoi->rcvd >= conctl)
          {
            lnkpoi->flag |= L2FBUSY;            /* busy werden          */
            lnkpoi->busyrx = 0;
            l2stma(stbl21);                     /* STATION BECOMES BUSY */
          }
        }
        else
          if (nmbfre > 62 && lnkpoi->rcvd < conctl / 2)
          {
            lnkpoi->flag &= ~L2FBUSY;          /* "busy" aufloesen      */
            l2stma(stbl22);                    /* BUSY CONDITION CLEARS */
          }

        if (   busy(port)             /* T1 und T2 nur wenn DCD aus ist */
            || dama(port))
          continue;

        if (lnkpoi->T1 != 0)            /* wenn Timer 1 aktiv ...       */
        {
          if (lnkpoi->T1 <= ticks)      /* wenn Timer 1 abgelaufen ...  */
          {
            lnkpoi->T1 = 0;             /* ... Timer 1 stoppen          */
            lnkpoi->RTT = 0;            /* RTT-Messung stoppen          */
            setT3();

            ++lnkpoi->tries;

#ifdef T1TIMERMOD
            /* Nach 3 erfolglosen versuchen,     */
            /* wird der SRTT-Wert vergroessert. */
            if (  (lnkpoi->tries > 5)
                /* SRTT-Wert kleiner als 50. */
                &&(lnkpoi->SRTT < 50))
            {
              lnkpoi->RTT = lnkpoi->SRTT;
              lnkpoi->RTT += 100;
              /* SRTT-Wert neu berechnen. */
              clrRTT();
            }
#endif /* T1TIMERMOD */

/* bei Wiederholungen wird Maxframe um 1 reduziert - wer nix empfaengt  */
/* landet irgendwann bei Maxframe 1                                     */
            if (lnkpoi->tries > 1)
              change_maxframe(lnkpoi, -1);

            if (lnkpoi->tries <         /* zu viele Retries ?           */
                portpar[port].retry)
              l2stma(stbl23);           /* Statet. T1 EXPIRES           */
            else                        /* zu viele Retries :           */
            {
              lnkpoi->tries = 0;        /* Retryzaehler leer            */
              l2stma(stbl25);           /* N2 IS EXCEEDED               */
            }
          }
          else
            lnkpoi->T1 -= ticks;        /* sonst herunterzaehlen        */
        }

        if (lnkpoi->T2 != 0)            /* wenn Timer 2 aktiv ...       */
        {
          if (lnkpoi->T2 <= ticks)      /* wenn Timer 2 abgelaufen ...  */
            lnkpoi->T2 = 0;             /* ... Timer 2 stoppen          */
          else
            lnkpoi->T2 -= ticks;        /* sonst herunterzaehlen        */
        }

        if (lnkpoi->T2 == 0)            /* Timer 2 abgelaufen oder      */
        {                               /* nicht aktiv?                 */
#ifdef DAMASLAVE
          if (damaslaveon(port))
            continue;
#endif
          if (lnkpoi->RStype != 0)      /* T2 abgelaufen                */
          {
            stxfad();                   /* ... dann Responseframe bauen */
            txfCR = txfPF = 0;
#ifdef __WIN32__
            txfctl = setNR((UBYTE)(!(lnkpoi->flag & L2FBUSY) ? lnkpoi->RStype : L2CRNR));
#else
            txfctl = setNR(!(lnkpoi->flag & L2FBUSY) ? lnkpoi->RStype : L2CRNR);
#endif /* WIN32 */
#ifdef EAX25
            txfEAX = FALSE;
            /* Kontrollbytes fuer EAX.25 aufbauen bzw. umbauen */
            if (lnkpoi->bitmask == 0x7F)
            {
              txfctl &= 0x0F;  /* erstes Kontrollbyte aendern */
              txfctlE = (lnkpoi->VR << 1) | (txfPF >> 4); /* V(R) und Pollflag    */
              txfEAX = TRUE;
            }
#endif
            sdl2fr(makfhd(L2FUS), TRUE);      /* und senden             */
            clrT2();                          /* Responsemodus loeschen */
          }
        }
      }                         /* fuer alle nicht disconnecteten Links */
    timDAMA(ticks);
  }
}

/************************************************************************/
/*                                                                      */
/* "set T2 and xmit RR response"                                        */
/*                                                                      */
/* Timer 2 setzen und nach Ablauf RR als Response senden.               */
/*                                                                      */
/************************************************************************/
void
t2rrr(void)
{
  setT2(L2CRR);
}

/************************************************************************/
/*                                                                      */
/* "set T2 and xmit RNR response"                                       */
/*                                                                      */
/* Timer 2 setzen und nach Ablauf RNR als Response senden.              */
/*                                                                      */
/************************************************************************/
void
t2rnrr(void)
{
  setT2(L2CRNR);
}

/************************************************************************/
/*                                                                      */
/* "set T2 and xmit REJ response"                                       */
/*                                                                      */
/* Timer 2 setzen und nach Ablauf REJ als Response senden.              */
/*                                                                      */
/************************************************************************/
void
t2rejr(void)
{
  setT2(L2CREJ);
}

/************************************************************************/
/*                                                                      */
/* "set T1"                                                             */
/*                                                                      */
/* Den Timer 1 anhand des SRTT setzen. Wenn wir bereits erfolglose      */
/* Versuche hatten (tries != 0), wird der Timer 1 vergroessert. Die     */
/* Laufzeitmessung (RTT) wird neu gestartet.                            */
/*                                                                      */
/************************************************************************/
void
setT1(void)
{
  lnkpoi->T1 = lnkpoi->SRTT * L2_BETA;
  if (lnkpoi->T1 < 20)                  /* T1 -> min. 200ms             */
    lnkpoi->T1 = 20;
  if (lnkpoi->tries)                    /* im Wiederholungsfalle        */
  {
#ifdef EAX25
    /* Wenn EAXMODE == 2 und schon mehrere SABME verschickt, dann       */
    /* Rueckfall auf AX.25 nach zwei SABME. */
    if (   (lnkpoi->state == L2SLKSUP)  /* Wenn der Link nicht steht    */
        && (portpar[lnkpoi->liport].eax_behaviour == 2) /* Mode stimmt  */
        && (lnkpoi->bitmask == 0x7F))    /* Ein EAX-Link                 */
      lnkpoi->bitmask = 0x07;            /* AX.25 umschalten             */
#endif
    if (   (lnkpoi->state >= L2SIXFER)  /* Wenn der Link steht          */
        && (lnkpoi->state != L2SHTH))
      lnkpoi->T1 *= 2;                  /* T1 verdoppeln                */
  }
  if (lnkpoi->T1 > 6000)                /* T1 auf 1 Minute begrenzen    */
    lnkpoi->T1 = 6000;
  setRTT();
}

/************************************************************************/
/*                                                                      */
/* "clear T1"                                                           */
/*                                                                      */
/* Timer 1 loeschen, es ist eine Reaktion eingetreten. Die Anzahl der   */
/* Fehlversuche (tries) wird geloescht, ebenso der Timer 3 neu gesetzt. */
/*                                                                      */
/************************************************************************/
void
clrT1(void)
{
  lnkpoi->T1 = 0;
  lnkpoi->tries = 0;
  setT3();
}

/************************************************************************/
/*                                                                      */
/* "set T2"                                                             */
/*                                                                      */
/* Timer 2 starten und festlegen, welches Frame nach Ablauf zu senden   */
/* ist.                                                                 */
/*                                                                      */
/************************************************************************/
void
setT2(UBYTE Stype)
{
  lnkpoi->RStype = Stype;
  lnkpoi->T2 = portpar[lnkpoi->liport].T2;
}

/************************************************************************/
/*                                                                      */
/* "clear T2"                                                           */
/*                                                                      */
/* Timer 2 loeschen.                                                    */
/*                                                                      */
/************************************************************************/
void
clrT2(void)
{
  lnkpoi->T2 = 0;
  lnkpoi->RStype = 0;
}

/************************************************************************/
/*                                                                      */
/* "set T3"                                                             */
/*                                                                      */
/* Timer 3 mit Defaultwert initialisieren.                              */
/*                                                                      */
/************************************************************************/
void
setT3(void)
{
#ifdef PORT_MANUELL
  lnkpoi->T3 = portpar[lnkpoi->liport].T3;
#else
   lnkpoi->T3 = T3par;
#endif /* PORT_MANUELL */
}

/************************************************************************/
/*                                                                      */
/* "clear T3"                                                           */
/*                                                                      */
/* Timer 1 loeschen.                                                    */
/* Fehlversuche (tries) wird geloescht, ebenso der Timer 3 neu gesetzt. */
/* Aus der vergangenen Zeit seit dem setzen des T1 wird der RTT und     */
/* damit der SRTT neu berechnet.                                        */
/*                                                                      */
/************************************************************************/
void
clrT3(void)
{
  lnkpoi->T3 = 0;
}

/************************************************************************/
/*                                                                      */
/* "set RTT"                                                            */
/*                                                                      */
/* RTT starten und VS merken.                                           */
/*                                                                      */
/************************************************************************/
void
setRTT(void)
{
  lnkpoi->RTT = 1;
  lnkpoi->RTTvs = lnkpoi->VS;
}

/************************************************************************/
/*                                                                      */
/* "clear RTT"                                                          */
/*                                                                      */
/* Aus der vergangenen Zeit seit dem setzen des T1 wird der RTT und     */
/* damit der SRTT neu berechnet.                                        */
/*                                                                      */
/************************************************************************/
void
clrRTT(void)
{
  /*
   * Nach RTT-Berechnung aus KA9Q's TCP/IP-Paket:
   *
   * SRTT' = (Alpha * SRTT + RTT) / (Alpha + 1)
   *
   * Alpha getrennt parametrisierbar fuer fallendes/steigendes RTT:
   * Alpha1 = steigendes RTT (kleines Alpha -> schnell reagieren)
   * Alpha2 = fallendes  RTT (grosses Alpha -> langsam reagieren)
   *
   */
  LNKBLK *lp = lnkpoi;
  UWORD   rtt = lp->RTT,
          srtt = lp->SRTT,
          irtt = portpar[lp->liport].IRTT;

  if (rtt > srtt)
    srtt = (L2_ALPHA1 * srtt + rtt) / (L2_ALPHA1 + 1);
  else
    srtt = (L2_ALPHA2 * srtt + rtt) / (L2_ALPHA2 + 1);

  if (srtt < irtt / 10)
    srtt = irtt / 10;
  if (srtt > irtt * 10)
    srtt = irtt * 10;

#ifdef SRTTMAXMOD
   if (srtt > SRTTMAXMOD)      /* SRTT-Wert ist groesser als SRTTMAX. */
       srtt = SRTTMAXMOD;      /* auf SRTTMAX setzen.                 */
#endif /* SRTTMAXMOD */

  lp->RTT = 0;
  lp->SRTT = srtt;
}

/************************************************************************\
*                                                                        *
* "check no activity"                                                    *
*                                                                        *
* Alle aktiven Links (lnktbl, Linkstatus "Information Transfer") auf     *
* "keine Aktivitaet" abtesten. Ist der Keine-Aktivitaet-Timer aktiv      *
* (!= 0) und nach Dekrementieren abgelaufen, Disconnect einleiten.       *
*                                                                        *
* ACHTUNG: Diese Funktion muss sekuendlich aufgerufen werden,            *
*          wird aber nur fuer TheNetNode benoetigt.                      *
*                                                                        *
\************************************************************************/
void
chknoa(void)
{
  MBHEAD *mbp;
  int     port;
  LHEAD  *llp;

  for (port = 0, llp = &l2actl[0];      /* alle Ports durchgehen        */
       port < L2PNUM;
       port++, llp++)
    for (lnkpoi = (LNKBLK *)llp->head;
         lnkpoi != (LNKBLK *)llp;       /* alle Links des Ports pruefen */
         lnkpoi = lnkpoi->next)
    {
      if (!(--lnkpoi->noatou))
      {
        mbp = (MBHEAD *)allocb(ALLOC_MBHEAD);
        mbp->l2link = lnkpoi;                   /* Linkpointer und      */
        mbp->type = 2;                          /* Typ (2=L2) setzen    */
        putchr('\r', mbp);
        putalt(alias, mbp);
        putid(myid, mbp);

#ifndef THENETMOD
        putstr("> Timeout (", mbp);
        putnum(ininat, mbp);                 /* Timeout in s ausgeben   */
        putstr("s)\r", mbp);
        seteom(mbp);                         /* Text an User senden...  */
#else
        /* L4TIMEOUT */
        if (SearchTHENET() == NULL)                /* Kein THENET-Typ,  */
        {
          putstr("> Timeout (", mbp);
          putnum(ininat, mbp);               /* Timeout in s ausgeben   */
          putstr("s) run off.\r", mbp);
          seteom(mbp);                       /* Text an User senden...  */
        }
        else                          /* THENET-TYP, keine Info senden. */
          dealmb(mbp);                             /* Buffer entsorgen. */
#endif /* THENETMOD */

        if (   (lnkpoi->state == L2SRBS)     /* remote busy?            */
            || (lnkpoi->state == L2SWARBS)
            || (lnkpoi->state == L2SRSRBS)
            || (lnkpoi->state == L2SBBS)     /* oder beide busy?        */
            || (lnkpoi->state == L2SWABBS)
            || (lnkpoi->state == L2SRSBBS)
            || (lnkpoi->state == L2SHTH))    /* oder Hop-To-Hop         */
          lnkpoi->flag |= L2FDIMM;           /* sofort abwerfen         */
        else
          lnkpoi->flag |= L2FDSLE;           /* sonst wenn alles raus   */
      }
   }
}

/************************************************************************\
*                                                                        *
* "set initial SRTT"                                                     *
*                                                                        *
* Anfangswert fuer Smoothed Round Trip Timer setzen                      *
*                                                                        *
\************************************************************************/
void
setiSRTT(void)
{
  char  *viap;                          /* Zeiger in via-Liste          */
  UWORD  n;                             /* Digizaehler                  */

  viap = lnkpoi->viaidl;                /* Anfang via-Liste             */
  n = 0;                                /* noch kein Digi gezaehlt      */
  while (*viap != '\0')                 /* Digianzahl ermitteln         */
  {
    if (!(viap[L2IDLEN - 1] & L2CH))
      ++n;
    viap += L2IDLEN;
  }
  n *= 2;
  ++n;                                  /* Digianzahl * 2 + 1           */
#ifndef SETISRTTMOD
  lnkpoi->SRTT = portpar[lnkpoi->liport].IRTT * n;
#else
 lnkpoi->SRTT = portpar[lnkpoi->liport].IRTT  * 1;
#endif /* SETISRTTMOD */
}

/* End of src/l2timer.c */
