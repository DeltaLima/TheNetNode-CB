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
/* File src/l2dama.c (maintained by: DF6LN)                             */
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
/*
 *      Statische Variablen und Funktionen nur fuer src/l2dama.c
 */

static WORD     dama_timer[DAMA_CH];
static WORD     pause_delay[DAMA_CH];
#define MIN_DELAY 3000

static void     xmit_damail(BOOLEAN *, int);
static void     xmit_damarl(int, BOOLEAN *);
static int      dama_dcd(int);
static BOOLEAN  damatx(void);

/*----------------------------------------------------------------------*/
/*      DAMA Initialisierung (Aufruf von l2init())                      */
/*----------------------------------------------------------------------*/
void
iniDAMA(void)
{
  WORD i;

  for (i = 0; i < DAMA_CH; i++)
  {
    dama_timer[i] = 0;
    pause_delay[i] = 0;
  }

  for (i = 0; i < L2PNUM; i++)
  {
    inithd(&damarl[i]);
    portpar[i].dch = DAMA_CH;
  }

  for (i = 0, lnkpoi = lnktbl; i < LINKNMBR; ++i, ++lnkpoi)
  {
    inithd(&lnkpoi->damail);
    lnkpoi->pollcnt = 0;      /* Anzahl verbotener Polls des DAMA-Users */
    lnkpoi->priold = -1;      /* Link-Prioritaets-Merker/Flag           */
    lnkpoi->indx = 1;         /* Index des Multiconnect                 */
    lnkpoi->zael = 1;         /* Aktiver Multiconnect                   */
    lnkpoi->anzl = 0;         /* Anzahl der Verbindungen bisher         */
    clrDAMA();
  }
}

/*----------------------------------------------------------------------*/
/* Feststellen, ob auf einem der Ports des Dama-Kanals der Sender       */
/* getastet ist, oder ein User noch etwas antwortet.                    */
/*----------------------------------------------------------------------*/
#define NO_DAMA     2
#define DAMA_DCD    1
#define DAMA_NO_DCD 0
static int
dama_dcd(int kanal)
{
  int       port;
  int       state = NO_DAMA;
  PORTINFO *pp;

  for (port = 0, pp = portpar;          /* Alle DAMA-Ports pruefen..    */
       port < L2PNUM;
       port++, pp++)
  {
    if (pp->dch != kanal)               /* Port gehoert zum Dama-Kanal? */
      continue;
    if (iscd(port))                     /* nicht busy(), iscd() !       */
      return (DAMA_DCD);
    state = DAMA_NO_DCD;
  }
  return (state);
}

/*----------------------------------------------------------------------*/
/* Wenn Info zu senden, gemaess Fenster passende Zahl an I-Frames in    */
/* die Dama-Sendeliste des aktiven Links (lnkpoi) haengen - das         */
/* passiert tatsaechlich in sdi / sdl2fr                                */
/*----------------------------------------------------------------------*/
static BOOLEAN
damatx(void)
{
  int   n;
  char  VS;

  if (lnkpoi->tosend)                   /* was zu senden?               */
  {
    if (lnkpoi->flag & L2FREPEAT)       /* soll wiederholt werden?      */
    {
      lnkpoi->flag &= ~L2FREPEAT;
      if ((n = outsdI()) != 0)          /* wieviel darf ich?            */
      {
        VS = lnkpoi->VS;                /* V(S) merken                  */
        lnkpoi->VS = lnkpoi->lrxdNR;    /* V(S) resetten                */
        sdi(0, n);                      /* nur ausstehende I's senden   */
        lnkpoi->VS = VS;                /* V(S) zurueck                 */
        return (TRUE);
      }
    }
    if ((n = itxwnd()) > 0)             /* Fenster noch offen?          */
    {
      sdi(outsdI(), n);                 /* Frames generieren            */
      lnkpoi->priold = lnkpoi->damapm;  /* akt. Priori. merken          */
      clrDAMA();                        /* DAMA-Parameter clear         */
      return (TRUE);
    }
  }
  return (FALSE);                       /* wir haben nix gesendet       */
}

/*----------------------------------------------------------------------*/
/* Daten aus der Dama-Sendeliste des aktiven Links (lnkpoi)             */
/* tatsaechlich aussenden - diese Funktion wird nur aufgerufen, wenn    */
/* der Link auch senden darf                                            */
/*----------------------------------------------------------------------*/
static void
xmit_damail(BOOLEAN *txflag, int k)
{
  MBHEAD *fbp;

  while (   (fbp = (MBHEAD *)lnkpoi->damail.head)
         != (MBHEAD *)&lnkpoi->damail)
  {                                     /* sind noch Frames da ?        */
    ulink((LEHEAD *)fbp);               /* ja:                          */
    relink((LEHEAD *)fbp,
           (LEHEAD *)txl2fl[lnkpoi->liport].tail);
    kicktx(lnkpoi->liport);             /* Frame markieren              */
#ifdef DAMASLAVE
    if (txflag == NULL)
      continue;
#endif
    lnkpoi->flag |= L2FDAMA1 | L2FDAMA2 /* DAMA Flag Bit 1 & 2 setzen   */
      | L2FDACK;                        /* Bestaetigungs-Flag setzen    */
    *txflag = TRUE;                     /* was da, TX-Flag setzen !     */
    dama_timer[k] = dama_init;          /* DAMA-Timer wieder setzen     */
  }
}

/*----------------------------------------------------------------------*/
/* Daten aus der Dama-Random-Liste des angegebenen Ports tatsaechlich   */
/* aussenden.                                                           */
/*----------------------------------------------------------------------*/
static void
xmit_damarl(int port, BOOLEAN *txflag)
{
  MBHEAD *fbp;

  while (   (fbp = (MBHEAD *)damarl[port].head)
         != (MBHEAD *)&damarl[port])
  {                                     /* sind noch Frames da ?        */
    ulink((LEHEAD *)fbp);               /* ja:                          */
    relink((LEHEAD *)fbp,
           (LEHEAD *)txl2fl[port].tail);
    kicktx(port);                       /* Frame markieren              */
#ifdef DAMASLAVE
    if (txflag != NULL)
#endif
      *txflag = TRUE;                   /* was da, TX-Flag setzen !     */
  }
}

/*--------------------------------------------------------------------------
 *
 *      DAMA-Timer abhaengige Funktionen ausfueheren
 *
 *      Aufruf erfolgt von l2timr().
 *
 *      Funktion:
 *      dama_timer um ticks vermindern; wenn DAMA-Betrieb freigegeben und
 *      dama_timer abgelaufen, dann L2-Link-Tabelle nach DAMA-Uplink durch-
 *      suchen und, falls Info vorliegt I-Frames, sonst Poll senden.
 *      Anschliessend diesen Uplink als bearbeitet markieren.
 *      Dabei jeden User nur einmal pro Runde an die Reihe nehmen !
 *
 *-------------------------------------------------------------------------*/
void
timDAMA(UWORD ticks)
{
  WORD     i;
  BOOLEAN  txflag;
  WORD     dsf;
  LHEAD   *llp;
  WORD     port;
  int      dcd,
           k;

#ifdef DAMASLAVE
  for (llp = &l2actl[port = 0];           /* alle Ports durchgehen      */
       port < L2PNUM;
       port++, llp++)
  {
    if (damaslaveon(port))                /* DAMA-Slave aktiv?          */
    {
      if (--portpar[port].damaok == 0)    /* Slave-Timeout abgelaufen?  */
        portpar[port].sendok = 1;         /* ja - Reste senden          */
      if (portpar[port].sendok == 1)
      {
        xmit_damarl(port, NULL);
        for (lnkpoi = (LNKBLK *)llp->head;
             lnkpoi != (LNKBLK *)llp;
             lnkpoi = lnkpoi->next)
        {
          damatx();                     /* Info-Frames generieren       */
          xmit_damail(NULL, -1);        /* und gleich senden            */
        }
        portpar[port].sendok = 0;
      }
    }
  }
#endif

  for (k = 0; k < DAMA_CH; k++)         /* jeden Dama-Kanal einzeln     */
  {                                     /* bearbeiten                   */
    if ((dcd = dama_dcd(k)) == NO_DAMA) /* kein DAMA Port auf dem Kanal */
      continue;

    dcd &= 1;                           /* DCD extrahieren              */

    if (!dcd && (dama_timer[k] > 0))    /* Frequenz (DCD) frei und      */
    {
      if (dama_timer[k] <= ticks)       /* DAMA-Timer abgelaufen?       */
        dama_timer[k] = 0;
      else                              /* DAMA-Zaehler runterzaehlen   */
        dama_timer[k] -= ticks;
    }

    if (dama_timer[k] == 0)             /* DAMA-Timer abgelaufen?       */
    {
      if (!dcd)                         /* DAMA nur weiter, wenn        */
      {                                 /* letztes Frame gesendet wurde */
        txflag = FALSE;
        for (port = 0; port < L2PNUM; port++)
        {
          if (portpar[port].dch == k)
            xmit_damarl(port, &txflag);
          if (txflag)
            break;
        }
      }

      if (!dcd && !txflag)
      {                                         /* (Carrier detect)     */
        for (i = 0, lnkpoi = lnktbl;
             i < LINKNMBR;
             ++i, ++lnkpoi)
        {                                    /* alle Links nacheinander */
          if (lnkpoi->state == L2SDSCED)
            continue;
          if (portpar[lnkpoi->liport].dch != k)
            continue;
          lnkpoi->flag &= ~L2FDAMA2;         /* DAMA-Flag 2 loeschen    */
          if ((lnkpoi->flag & L2FDAMA1) == 0)/* DAMA-Flag 1 geloescht?  */
          {                                  /* ja: war noch nicht dran */
            lnkpoi->flag |= L2FDAMA1;        /* DAMA-Flag 1 setzen      */
                                             /* als "dran" kennzeichnen */
            if (lnkpoi->zael == lnkpoi->indx)/* darf Link senden?       */
            {
              if (damatx() == FALSE)       /* Neue I-Frames generieren  */
                if (   lnkpoi->damapm == 0 /* Nur bei hoechster Priori. */
                    && lnkpoi->damapc == 0)/* pG: 23.8.95               */
                  if ((MBHEAD *)lnkpoi->damail.head
                      == (MBHEAD *)&lnkpoi->damail)
                                           /* Warten auf Bestaetigung   */
                  {                        /* oder nix in der Liste ?   */
                    l2stma(stbl23);        /* ja: dann gleich pollen    */
                    incDAMA();             /* und Prioritaet runter     */
                    if (lnkpoi->state != L2SHTH)
                      txflag = TRUE;       /* Poll wird gesendet        */
                  }
              xmit_damail(&txflag, k);
              break;                   /* DAMA User gefunden, abbrechen */
            } /* Sende-Erlaubnis? */
          } /* DAMA-Flag 1 geloescht? */
        } /* alle Links */

        if (i < LINKNMBR && !txflag)
        {
/*
 * Keine Info zum senden gefunden, also Poll senden !
 * ====> leere Polls ???
 */
/* Zeit abgelaufen und Link an der Reihe? */
          if (   lnkpoi->damapc == 0
              && lnkpoi->zael == lnkpoi->indx)
          {                             /* auch nur EIN Poll pro Runde  */
            ++lnkpoi->tries;            /* Tries erhoehen               */

/* bei Wiederholungen wird Maxframe um 1 reduziert - wer nix empfaengt  */
/* landet irgendwann bei Maxframe 1                                     */
            if (lnkpoi->tries > 1)
              change_maxframe(lnkpoi, -1);

            incDAMA();                  /* Prioritaet runtersetzen      */
            if (lnkpoi->tries < portpar[lnkpoi->liport].retry)
            {
              l2stma(stbl23);           /* T1 expires  (fuer Poll)      */
              if (lnkpoi->state != L2SHTH)
                txflag = TRUE;
            }
            else
            {
              lnkpoi->tries = 0;                /* Tries abgelaufen!    */
              l2stma(stbl25);                   /* N2 IS EXCEEDED       */
            }
            xmit_damail(&txflag, k);
          } /* DAMA && Sende-Genehmigung */
        } /* lnkpoi != && !txflag */

        if (i == LINKNMBR)
        {
/*
 * Keinen zu bearbeitenden DAMA-Link gefunden, oder schon
 * alle DAMA-Links bearbeitet . Also ist die Runde zuende!
 * DAMA-Flags loeschen und Prioritaetszaehler runterzaehlen.
 * Leere Polls auf MC-Links durch IXFER ersetzen.
 */

/* DAMA-Flags loeschen */
          for (llp = &l2actl[port = 0];
               port < L2PNUM;
               port++, llp++)
          {
            if (portpar[port].dch != k)
              continue;
            if (dama(port))
            {
              for (lnkpoi = (LNKBLK *)llp->head;
                   lnkpoi != (LNKBLK *)llp;
                   lnkpoi = lnkpoi->next)
              {
/* DAMA-Flag Bit 1 & 2 loeschen                                         */
                lnkpoi->flag &= ~(L2FDAMA1 + L2FDAMA2);
/*
 * Naechsten MC-Link freigeben (zael erhoehen) DAMA-Speed-Faktor beachten.
 */
                if ((dsf = (DamaSpeedFactor / 100)
                            / portpar[lnkpoi->liport].speed)
                    < 1)
                  dsf = 1;
/* Zaehler aktualisieren                                                */
                if (lnkpoi->zael < lnkpoi->anzl * dsf)
                  lnkpoi->zael++;
                else
                  lnkpoi->zael = 1;
              }
            }
          }

/* Nach jeder DAMA-Runde pruefen, ob eine zusaetzliche Pause eingefuegt */
/* werden soll. Diese Zusatzpause gibt die Frequenz fuer neue Stationen */
/* frei, auch wenn die DCD-Pausen sonst zu kurz sind. Nach solch einer  */
/* Zusatzpause wird allerdings (MIN_DELAY * 10ms) keine weitere Pause   */
/* eingefuegt. Die Pausenlaenge ist ueber Parameter "DAMA-Tout"         */
/* einstellbar.                                                         */
          if (pause_delay[k] == 0)
          {
            pause_delay[k] = MIN_DELAY + dama_init;
            dama_timer[k] = dama_init;
          }

        }
      } /* Frequenz frei? */
    } /* DAMA-Timer abgelaufen? */

/*
 * Bei jedem Aufruf von timDAMA() die DAMA-Prioritaet um ticks
 * herunterzaehlen
 */

    for (llp = &l2actl[port = 0];       /* alle Ports durchgehen        */
         port < L2PNUM;
         port++, llp++)
    {
      if (portpar[port].dch != k)
        continue;
      if (dama(port))                   /* nur wenns ein DAMA-Port ist  */
      {
        for (lnkpoi = (LNKBLK *)llp->head;
             lnkpoi != (LNKBLK *)llp;
             lnkpoi = lnkpoi->next)
        {
          if (lnkpoi->damapc >= ticks)
            lnkpoi->damapc -= ticks;
          else
            lnkpoi->damapc = 0;
        }
      }
    }

    if (pause_delay[k] > 0)             /* Pausen-Verzoegerung aktiv?   */
    {
      if (pause_delay[k] <= ticks)      /* Verzoegerung abgelaufen?     */
        pause_delay[k] = 0;
      else                              /* Verzoegerung runterzaehlen   */
        pause_delay[k] -= ticks;
    }
  }
}


/*----------------------------------------------------------------------*/
/*
 * DAMA Aktivitaetszaehler und -merker auf 0 setzen
 * (wenn User I-Frames gesendet hat)
 */
void
clrDAMA(void)
{
  lnkpoi->damapm = 0;
  lnkpoi->damapc = 0;
}

/*----------------------------------------------------------------------*/
/*      DAMA Aktivitaetsmerker inkrementieren                           */
/*      Reduzierung der Poll-Rate auf 1min. nach 10min. Inaktivitaet.   */
/*----------------------------------------------------------------------*/
void
incDAMA(void)
{
  if (lnkpoi->damapm < dama_max)
    ++lnkpoi->damapm;

  if (   lnkpoi->noatou > (ininat - 600)     /* Keine Aktivitaet??      */
      || lnkpoi->tries > 1)                  /* Bei Retries schneller.. */
    lnkpoi->damapc = lnkpoi->damapm * 100;   /* Prioritaet normal..     */
  else
    lnkpoi->damapc = 6000;                   /* Prioritaet auf 1 Minute */
}

/*----------------------------------------------------------------------*/
/*
 *      DAMA Timer loeschen
 */

void
clearDT(UWORD time_val)
{
  int k;

  if (   dama(lnkpoi->liport)
      && (lnkpoi->flag & (L2FDAMA1 + L2FDAMA2)) == (L2FDAMA1 + L2FDAMA2))
  {
    lnkpoi->flag &= ~L2FDAMA2;
    lnkpoi->tries = 0;
    k = portpar[lnkpoi->liport].dch;
    dama_timer[k] = time_val;
  }
}

/*----------------------------------------------------------------------*/
/* POLL-Frames von Usern sind bei DAMA verboten und werden angemeckert. */
/* Zunaechst wird eine Warnung verschickt, dann ein Disconnect.         */
/*----------------------------------------------------------------------*/
void
polDAMA(void)
{
  MBHEAD *mbp;
  MHEARD *mhp;

  if (lnkpoi->state != L2SDSCED)    /* Wenn es ein aktiver Link ist..   */
  {
    lnkpoi->pollcnt++;              /* Neuen Verstoss mitzaehlen..      */
    if ((mhp = mh_lookup(&l2heard, lnkpoi->dstid)) != NULL)
      mhp->damawarn++;

    if (MaxPollCnt)                 /* DAMA-Kontrolle eingeschaltet?    */
    {
/*......................................................................*/
/* Anzahl der maximal zugelassenen Verstoesse ueberschritten, es folgt  */
/* Fehlermeldung und Zwangsdisconnect!!!                                */
/*......................................................................*/
      mbp = (MBHEAD *)allocb(ALLOC_MBHEAD);     /* Buffer besorgen..    */
      mbp->l2link = lnkpoi;                     /* Linkpointer und      */
      mbp->type = 2;
#ifdef SPEECH
      putstr(speech_message(138), mbp);
#else
      putstr("\r*** from DAMA-Master ", mbp);
#endif
      putcal(myid, mbp);                        /* Digi-Call ausgeben   */
      if (lnkpoi->pollcnt >= MaxPollCnt)
      {
      /* dealml((LEHEAD *)&lnkpoi->sendil); */ /* Sende-Liste loeschen  */
      /* lnkpoi->tosend = 0;                */ /* kein Frame zu senden  */

      /* Das loeschen der Sende-Liste ist nicht ganz ungefaehrlich, je  */
      /* nach dem, in welchem State wir gerade sind, lieber den Schrott */
      /* senden und dann abwerfen, DB7KG                                */

#ifdef SPEECH
        putprintf(mbp, speech_message(131),
                  lnkpoi->pollcnt);             /* Verstoesse ausgeben  */
#else
        putprintf(mbp, "> DISCONNECT: Too many non-DAMA Polls (%u) !!\r",
                  lnkpoi->pollcnt);             /* Verstoesse ausgeben  */
#endif
        seteom(mbp);
        dsclnk();                               /* Disconnect einleiten */
      }
      else
      {                                         /* Vorwarnung!          */
#ifdef SPEECH
        putprintf(mbp, speech_message(132),
                  lnkpoi->pollcnt, MaxPollCnt);
#else
        putprintf(mbp, "> WARNING: non-DAMA Poll #%u,"
                  " Disconnect after %u !!\r",
                  lnkpoi->pollcnt, MaxPollCnt);
#endif
        seteom(mbp);
      }
    }
  }
}

/*************************************************************************
 *
 * "get Multi Connect Tabels"
 *
 *  "getMCs" wird jedesmal aufgerufen, wenn ein neuer L2-Link dazukommt
 *  oder abgemeldet wird. (Connect/Disconnect)
 *
 *  "getMCs" stellt DAMA Multi-Connects (MC's) fest und baut die Multi-
 *  Connect-Tabellen lnkpoi->indz und lnkpoi->anzl auf und korrigiert
 *  lnkpoi->zael bei Link-Aenderungen auf plausible Werte.
 *
 *  lnkpoi->indz repraesentiert die Indizes einer MC-Station.
 *  lnkpoi->anzl repraesentiert die Anzahl der MC's einer Station.
 *
 *  lnkpoi->zael wird dann nach jeder Runde erhoeht bzw. zurueckgesetzt.
 *  Bei Gleichheit von indz und zael erhaelt dieser Link spaeter die
 *  "Sende-Erlaubnis" !
 *
 *  getMCs wird nur aufgerufen, wenn wirklich noetig
 *  (in src/l2stma.c in l2newstate). getMCs arbeitet LINEAR.
 *
 ************************************************************************/
void
getMCs(void)
{
  WORD    multi,
          indx,
          port,
          k;
  LHEAD  *llp;
  LNKBLK *lp;
  char   *id;

  lnkpoi->zael = 1;                     /* Zaehler immer zurueck        */

  id = lnkpoi->dstid;                   /* geht etwas schneller         */
  k = portpar[lnkpoi->liport].dch;

  multi = 0;
  for (llp = &l2actl[port = 0];         /* alle Ports durchgehen        */
       port < L2PNUM;
       port++, llp++)
  {
    if (portpar[port].dch != k)         /* nur fuer diesen Dama-Kanal!  */
      continue;
    if (dama(port))                     /* nur wenns ein DAMA-Port ist  */
      for (lp = (LNKBLK *)llp->head;
           lp != (LNKBLK *)llp;
           lp = lp->next)
        if (cmpcal(lp->dstid, id))
          multi++;
  }

  if (   (!dama(lnkpoi->liport))        /* kein DAMA-Link?              */
      || (!multi))                      /* oder letzter MultiConnect    */
  {
    lnkpoi->indx = 1;                   /* Index zuruecksetzen!         */
    lnkpoi->anzl = 0;                   /* Anzahl der Verbindungen!     */
    return;
  }

/*
 * Hinweis zum Ablauf:
 * Die Linkliste wird durchlaufen und fuer jeden Multiconnect-Link
 * die Anzahl der aktiven Multiconnects (->anzl) und ein Index gesetzt.
 * Der aktuelle Link (lnkpoi) wird hier auch gefunden und behandelt,
 * sofern dies noetig ist (L2MCONN).
 */

  indx = 0;
  for (llp = &l2actl[port = 0];         /* alle Ports durchgehen        */
       port < L2PNUM;
       port++, llp++)
  {
    if (portpar[port].dch != k)         /* nur fuer diesen Dama-Kanal!  */
      continue;
    if (dama(port))                     /* nur wenns ein DAMA-Port ist  */
      for (lp = (LNKBLK *)llp->head;
           lp != (LNKBLK *)llp;
           lp = lp->next)
        if (cmpcal(lp->dstid, id))      /* gleiches Call ?              */
        {
          lp->indx = ++indx;            /* Index setzen                 */
          lp->anzl = multi;             /* Anzahl der Connects          */
        }
  }
}

/************************************************************************\
*                                                                        *
* "already connected"                                                    *
*                                                                        *
* Hat dieses Rufzeichen auf diesem Port schon einmal connected?          *
*                                                                        *
\************************************************************************/
BOOLEAN
multiconn(int port, char *id)
{
  LHEAD  *llp;
  LNKBLK *lp;

  llp = &l2actl[port];
  for (lp = (LNKBLK *)llp->head;
       lp != (LNKBLK *)llp;             /* alle Links des Ports pruefen */
       lp = lp->next)
    if (cmpcal(lp->realid, id))
      return (TRUE);
  return (FALSE);
}

/* End of src/l2dama.c */
