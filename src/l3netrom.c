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
/* File src/l3netrom.c (maintained by: ???)                             */
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
#include "l3local.h"

static int      rtt2qual(ULONG rtt);
static int      qual2rtt(unsigned);
static void     fastlearn(PEER *, char *);
static void     rx_ui_broadcast(PEER *, MBHEAD *);
static void     add_netrom_info(MBHEAD **, NODE *, PEER *, PEER *, unsigned);
static void     add_thenet_info(MBHEAD **, NODE *, PEER *, PEER *, unsigned);
static void     addbro(MBHEAD **, NODE *, PEER *, PEER *, unsigned,
                       unsigned, int);


/************************************************************************/
/*                                                                      */
/* Umrechnung NETROM-Qualitaet <-> Laufzeit                             */
/*                                                                      */
/************************************************************************/
static int
rtt2qual(ULONG rtt)                 /* Laufzeit zu Qualitaet            */
{
  int qual;

  if (rtt)
  {
    qual = 255 - (unsigned)(rtt / autoqual);
    if (qual > 254)
      qual = 254;
    if (qual < 3)
      qual = 3;
    return (qual);
  }
  return (0);                       /* ausgefallen                      */
}

static int
qual2rtt(unsigned qual)             /* Qualitaet zu Laufzeit            */
{
  if (qual < 2)
    return (0);                     /* unerreichbar                     */
  return ((256 - qual) * autoqual);
}

/************************************************************************/
/*                                                                      */
/* Die Absenderknoten muessen in der Nodesliste stehen, damit wir       */
/* Antworten zurueckschicken koennen. Ist ein Absender noch nicht       */
/* bekannt, wird dieser mit einer Laufzeit von 599,99s eingetragen.     */
/*                                                                      */
/************************************************************************/
static void
fastlearn(PEER *pp, char *id)
{
  INDEX index = find_node_this_ssid(id);
#if MAX_TRACE_LEVEL > 6
  char  notify_call1[10];
  char  notify_call2[10];
#endif

  if (index != NO_INDEX)            /* Node bekannt und Qualitaet gut   */
    if (pp->routes[index].quality)  /* dann nicht lernen                */
      return;
  if ((index = add_route(pp, id, LEARN)) != NO_INDEX)
  {
    update_lt(pp, index, DEFAULT_LT);
#ifndef THENETMOD
    pp->routes[index].timeout = ROUTE_TIMEOUT;
#else
    /* Lebensdauer einer Node markieren. */
    pp->routes[index].timeout = obcini;
#endif /* THENETMOD */
#if MAX_TRACE_LEVEL > 6
    call2str(notify_call1, orgnod);
    call2str(notify_call2, pp->l2link->call);
    notify(7, "fastlearn %s via %s",
           notify_call1, notify_call2);
#endif
  }
}

/************************************************************************\
*                                                                        *
* Informationstransfer von Level2 zum NET/ROM-Router                     *
*                                                                        *
* Die empfangenen Frames vom Nachbarn werden ausgewertet und ggf. an den *
* eigenen L4 (Verbindungsebene) oder an einen anderen Nachbarn           *
* weitergeleitet.                                                        *
*                                                                        *
\************************************************************************/
void
netrom_rx(PEER *rxpp)
{
  MBHEAD     *mbp;              /* Buffer fuer ein Frame                */
  char huge  *savmbbp;          /* mbbp-Sicherung                       */
  WORD        savmbgc;          /* mbgc-Sicherung                       */
  NODE       *srcnod;
  NODE       *dstnod;
  PEER       *pp;
  MHEARD     *mhp;
  int         mask;
#if MAX_TRACE_LEVEL > 2
  char        notify_call1[10];
  char        notify_call2[10];
  char        notify_call3[10];
#endif

/* Zunaechst empfangene Frames verarbeiten, bis Liste leer...           */
  while (   (mbp = (MBHEAD *)(rxpp->rxfl.head))
         != (MBHEAD *)&(rxpp->rxfl))
  {
    ulink((LEHEAD *)mbp);               /* 1 Frame aus der Liste holen  */
    if (mbp->l2link == NULL)
      rx_ui_broadcast(rxpp, mbp);       /* ist UI-Broadcast             */
    else                                /* Info Frame                   */
    {
      if (rx_inp_broadcast(rxpp, mbp))  /* INP Broadcast?               */
      {
        dealmb(mbp);
        return;                         /* max 1 Frame pro Runde        */
      }
      savmbbp = mbp->mbbp;              /* Position im Frame merken     */
      savmbgc = mbp->mbgc;              /* fuer Weiterleitung           */
      if (   (getfid(orgnod, mbp))      /* Call des Quellknotens holen  */
          && (getfid(desnod, mbp))      /* Call des Zielknotens holen   */
          && (mbp->mbgc < mbp->mbpc))   /* und Info im Frame?           */
      {
        if (   valcal(orgnod) == ERRORS /* Absender und Zielrufzeichen  */
            || valcal(desnod) == ERRORS /* pruefen                      */
           )
        {
#if MAX_TRACE_LEVEL > 2
          call2str(notify_call1, orgnod);
          call2str(notify_call2, desnod);
          call2str(notify_call3, rxpp->l2link->call);
          notify(3, "invalid node %s>%s from %s (flushed)",
                 notify_call1, notify_call2, notify_call3);
                                        /* wir sagens dem Sysop         */
#endif
          dealmb(mbp);                  /* Frame wegwerfen              */
          continue;                     /* ... und zum naechsten Frame  */
        }
        --(*(mbp->mbbp));               /* Restlebensdauer um 1 runter  */
        time_to_live = getchr(mbp);     /* Restlebensdauer lesen        */

        if ((mbp->mbpc - mbp->mbgc) >= 5)  /* ist es lang genug ?       */
        {
/* Frames, die von einem Knoten kommen, der noch nicht in unsere        */
/* Knoten-Liste eingetragen ist, fuehren zu einem Eintrag des Quell-    */
/* Knotens mit Qualitaet 59999 in unsere Nodes-Liste (Fast-Learn)       */

          if (   !cmpid(myid, orgnod)           /* kein eigenes Frame   */
              && !cmpid(myid, rxpp->l2link->call))
            fastlearn(rxpp, orgnod);

          l4hdr0 = getchr(mbp);         /* Header holen                 */
          l4hdr1 = getchr(mbp);         /* ist eigentlich Sache des L4, */
          l4hdr2 = getchr(mbp);         /* aber der L3 verschickt auch  */
          l4hdr3 = getchr(mbp);         /* L4-Messframes, deswegen      */
          l4hdr4 = getchr(mbp);         /* wird das schon hier er-      */
          l4opco = l4hdr4 & L4OPMASK;   /* ledigt.                      */

          if (l4opco == L3TCPUDP && l4hdr0 == 0 && l4hdr1 == 1)  /* NRR */
          {
            nrr_rx(mbp, rxpp);
            dealmb(mbp);                 /* Frame wegwerfen             */
            continue;                    /* ... und zum naechsten Frame */
          }

/*----- Auswertung der eigenen und fremden L3RTT Frames ----------------*/
          if (   cmpid(desnod, "L3RTT \140")  /* Ziel und Inhalt OK!    */
              && (   cmpid(orgnod, rxpp->l2link->call)
                  || cmpid(orgnod, myid)))    /* und von dem Nachbarn   */
          {                                   /* oder eigenes           */
            rx_l3rtt_frame(rxpp, mbp, savmbbp, savmbgc);
            continue;
          }

/* L3-MH-Liste fuehren                                                  */
          if ((mhp = mh_lookup(&l3heard, orgnod)) == NULL)
            mhp = mh_add(&l3heard);
          if (mhp)
          {                                /* Eintrag vorhanden?        */
            mh_update(&l3heard, mhp, orgnod, mbp->l2port);
            mhp->rx_bytes += (mbp->mbpc - mbp->mbgc);
          }

/*----------------- Frame ist fuer mich --------------------------------*/
          if (cmpid(myid, desnod))
          {
#ifdef IPROUTE
            if (l4opco == L3TCPUDP)
            {                              /* L3 TCP/UDP-Frame          */
              mbp->l2fflg = L2CIP;         /* es ist ein IP-Frame       */
              relink((LEHEAD *)mbp, (LEHEAD *)iprxfl.tail);
              continue;                    /* Frame verarbeitet         */
            }
#endif
            mbp->l3_typ = L3NORMAL;        /* kein LOCAL und kein L3RTT */
            if (iscall(orgnod, &srcnod, NULL, DG))     /* Absender-Node */
            {
              l4rx(srcnod, NULL, mbp);     /* Frame an Level 4 geben    */
              continue;                    /* naechstes Frame           */
            }
#if MAX_TRACE_LEVEL > 4
            call2str(notify_call1, orgnod);
            notify(5, "unreachable source node %s", notify_call1);
#endif
            dealmb(mbp);                   /* Frame entsorgen           */
            continue;
          }

/* MH-Liste Senderichtung                                               */
          if ((mhp = mh_lookup(&l3heard, desnod)) != NULL)
            mhp->tx_bytes += (mbp->mbpc - mbp->mbgc);

/*---- nur CONREQ kann auf L2 umgeroutet werden */
          if ((l4opco == L4CONREQ) || (l4istome(orgnod, desnod)))
            mask = DG | VC | VC_FAR;
          else
            mask = DG;

/*---- Level 3 Frame nicht fuer mich, sondern weiterleiten -----*/
          if (iscall(desnod, &dstnod, &pp, mask))
          {                                         /* Ziel erreichbar? */
/* wenn das Ziel nicht im DG-Mode erreicht werden kann, dann muessen    */
/* wir umsetzen.                                                        */
            if ((pp->options & DG) == 0)
            {
              mbp->l3_typ = L3LOCAL;
              mbp->l2link = (LNKBLK *)dstnod;             /* Ziel-Node  */
              if (iscall(orgnod, &srcnod, NULL, DG))      /* Absender   */
              {
                l4rx(srcnod, dstnod, mbp);
                continue;                  /* naechstes Frame           */
              }
#if MAX_TRACE_LEVEL > 4
              call2str(notify_call1, orgnod);
              notify(5, "unreachable source node %s", notify_call1);
#endif
              dealmb(mbp);                 /* Frame entsorgen           */
              continue;
            }

            if (time_to_live)
            {                              /* noch Restlebensdauer?     */
              mbp->mbbp = savmbbp;
              mbp->mbgc = savmbgc;

              if (cmpid(myid, orgnod))  /* eigenes Frame gehoert, weg   */
              {                         /* damit                        */
#if MAX_TRACE_LEVEL > 4
                call2str(notify_call1, desnod);
                call2str(notify_call2, rxpp->l2link->call);
                notify(5, "own frame to %s received fm %s (loop)",
                       notify_call1, notify_call2);
#endif
                dealmb(mbp);
                continue;
              }

/* Falls wir ein Frame ueber den Nachbarn zurueckschicken               */

              if (rxpp == pp)        /* wuerden, dann vernichten wir es */
              {
#if MAX_TRACE_LEVEL > 4
                call2str(notify_call1, orgnod);
                call2str(notify_call2, desnod);
                call2str(notify_call3, rxpp->l2link->call);
                notify(5, "frame fm %s to %s via %s flushed (neigb loop)",
                       notify_call1, notify_call2, notify_call3);
#endif
                dealmb(mbp);
                continue;
              }

              toneig(pp, mbp);          /* Info absetzen                */
              continue;
            }                           /* noch Restlebensdauer         */
          }                             /* Ziel bekannt                 */
        }                               /* Laenge ok                    */
      }                                 /* Info ist im Frame            */
    }                                   /* es ist ein I-Frame           */
    dealmb(mbp);
  }
}

/************************************************************************/
/*                                                                      */
/* L3 UI-Frame (Broadcast)                                              */
/*                                                                      */
/************************************************************************/
static void
rx_ui_broadcast(PEER *rxpp, MBHEAD *mbp)
{
  char  alias[L2CALEN];         /* Alias Rundspruch sendender Knoten    */
  INDEX index;

  if (worqua != 0)                      /* darf Auswertung erfolgen?    */
  {
    if (   mbp->mbgc < mbp->mbpc        /* Info im Frame?               */
        && (getchr(mbp) & 0xFF) == 0xFF /* stimmt Signatur              */
        && ge6chr(alias, mbp))          /* Alias des Absender-Knotens   */
    {
      if (  (alias[0] != '#')     /* NOROUTE, Keine Route '#' im Alias, */
#ifdef THENETMOD
          ||(NoRoute)                    /* oder NOROUTE eingeschaltet, */
#endif /* THENETMOD */
         )             /* Broadcast-Node zulassen, ansonsten ablehnen.  */
      {
        cpyals(rxpp->l2link->alias, alias);
        if ((index = add_route(rxpp, rxpp->l2link->call, 1)) != NO_INDEX)
        {
          update_lt(rxpp, index, 1);
          if (update_alias(index, alias))
            propagate_node_update(index);
          rx_broadcast(rxpp, mbp);
#ifdef THENETMOD
          if (  (rxpp->soll_typ == NETROM)          /* Noch kein TYP gesetzt. */
              ||(rxpp->soll_typ == THENET))  /* Typ soll ein THENET-TYP sein. */
            rxpp->typ = THENET;                 /* TYP auf THENET-TYP setzen. */

          if (!rxpp->obscnt)               /* Restlebensdauer fuer Rundspruch */
          {                                            /* noch nicht gesetzt. */
            MBHEAD *rxmbp = NULL;

            bcast(&rxmbp,                   /* Broadcast-Bake to CALL senden, */
                  rxpp->l2link->call,   /* damit der Link sofort nutzbar ist. */
                  rxpp->l2link->port);
          }

          rxpp->obscnt  = obcbro;  /* Restlebensdauer fuer Rundspruch setzen. */

          if (rxpp->quality < 1)            /* Nur wenn ungueltige Qualitaet, */
            rxpp->quality = worqua;             /* minimale Qualitaet setzen. */

#endif /* THENETMOD */
        }
      }
    }
  } /* Signatur ist gut */
} /* Auswertung zulaessig */

/*----------------------------------------------------------------------*/
void
rx_broadcast(PEER *rxpp, MBHEAD *mbp)
{
  char   desnod[L2IDLEN];
  char   beaide[L2CALEN];
  char   nbr_call[L2IDLEN];
  int    rx_qual;
  INDEX  index;
  PEER  *pp;
#if MAX_TRACE_LEVEL > 8
  char   notify_call1[10];
  char   notify_call2[10];
  char   notify_call3[10];
#endif

  if (rxpp->typ == INP)
    return;

#ifdef THENETMOD
  if (rxpp->l2link->alias[0] == '#')             /* Unser Partner ist GEHEIM, */
    return;                               /* damit wir nehmen keine NODES an. */
#endif /* THENETMOD */

  while (   (getfid(desnod, mbp))
         && (valcal(desnod))
         && (ge6chr(beaide, mbp))
         && (getfid(nbr_call, mbp))
         && (valcal(nbr_call))
         && (mbp->mbgc < mbp->mbpc))
  {
    rx_qual = getchr(mbp) & 0xFF;

#if MAX_TRACE_LEVEL > 8
    call2str(notify_call1, rxpp->l2link->call);
    call2str(notify_call2, desnod);
    call2str(notify_call3, nbr_call);
    notify(9, "%-9.9s>%-6.6s:%-9.9s Q%u v %-9.9s",
           notify_call1, beaide, notify_call2, rx_qual, notify_call2);
#endif

#ifdef THENETMOD
    if (beaide[0] == '#')
    {
#if MAX_TRACE_LEVEL > 2
      call2str(notify_call1, rxpp->l2link->call);
      call2str(notify_call2, desnod);
      notify(3, "NETROM: %s sent secret node %s", notify_call1, notify_call2);
#endif
      /* XNet hat (derzeit ?) einen Fehler, der Nodes mit Geheim-Aliassen */
      /* sendet. Da diese Nodes nicht wirklich geheim sind nehmen wir sie */
      /* trotzdem, loeschen aber ihre Aliasse. */
      if (    (strncasecmp(&beaide[1], "temp", 4) == 0)
           || (strncasecmp(&beaide[1], "tmp", 3) == 0)
         )
      {
        /* fehlerhaften Alias loeschen */
        cpyals(beaide, DONT_CHANGE_ALIAS); /* Alias loeschen */
#if MAX_TRACE_LEVEL > 2
        notify(3, "NETROM: not real secret node, corrected");
#endif
      }
    }
#endif /* THENETMOD */

    if (   (cmpid(myid, nbr_call) == TRUE)  /* er wuerde an mich senden,  */
        || (rx_qual == LEARNQUAL)           /* oder er kennt keinen Weg,  */
        || (beaide[0] == '#'))              /* oder Ziel (jetzt) "hidden" */
      rx_qual = 0;                        /* also den Weg austragen     */

    if (cmpid(myid, desnod) == TRUE)                /* nicht uns selber */
      continue;

    switch (rxpp->typ)
      {
        case NETROM:
        case THENET:
          if (rx_qual < worqua)
            rx_qual = 0;                                 /* ausgefallen */
          break;
        case TNN:
          if (rx_qual < 2)
            rx_qual = 0;                                 /* ausgefallen */
          break;
      }

    if ((index = add_route(rxpp, desnod, qual2rtt(rx_qual))) != NO_INDEX)
    {
      update_lt(rxpp, index, DEFAULT_LT);

      if (!cmpcal(netp->nodetab[index].alias, nulide))
        if (find_best_qual(index, &pp, DG) > 0)     /* beste Qualitaet  */
          if (pp != rxpp)
            continue;

      if (update_alias(index, beaide))      /* Alias hat sich geaendert */
        propagate_node_update(index);
    }
  }
}

void
sdl3ui(PEER *pp, MBHEAD *mbp)            /* UI-Broadcast senden         */
{
  unsigned int i;
  STAT  *statp;

  if (pp->l2link->digil[0] == 0)       /* direkter Nachbar              */
    sdui("",                           /* Level 2 senden                */
         "NODES \140",                 /* an das Ziel "NODES"           */
          myid,
#ifdef __WIN32__
         (char)pp->l2link->port,
#else
         pp->l2link->port,
#endif /* WIN32 */
          mbp);
  else
      sdui(pp->l2link->digil,            /* Level 2 senden                */
           pp->l2link->call,
           myid,
#ifdef __WIN32__
         (char)pp->l2link->port,
#else
         pp->l2link->port,
#endif /* WIN32 */
           mbp);

  /* Da gesendete UI-Frames nicht in der Statistik an der dafuer vorgesehenen */
  /* Stelle erfasst werden koennen, dort ist das eigentliche Ziel (der Link-  */
  /* partner) nicht mehr bekannt, die Statistik hier fuehren.                 */

  for (statp = mh, i = 0; i < MAXSTAT; statp++, i++)
  {
    if (!(*statp->call))               /* nur benutzte Eintraege interessieren */
      continue;

    if (cmpid(pp->l2link->call, statp->call))      /* richtiger Stat-Eintrag */
    {
      if (pp->l2link->digil[0] == NUL)             /* ohne via Angabe      */
      {
        statp->UIno[1]++;
        break;
      }
      else
      {                                            /* mit via */
        if (cmpid(pp->l2link->digil, statp->viacall))
        {
          statp->UIno[1]++;
          break;
        }
      }
    }
  }
  dealmb(mbp);                         /* Buffer wieder freigeben       */
}

/************************************************************************/
/*                                                                      */
/* "send broadcast"                                                     */
/*                                                                      */
/* Ein Broadcast-Frame auf die Reise schicken. Es wird je nach Nachbar- */
/* typ als UI oder als I-Frame gesendet. "nodemb" wird auf NULL ge-     */
/* setzt, um einen fehlenden Node-Buffer zu signalisieren. Der naechste */
/* Aufruf von addbro() erzeugt dann ein neues Broadcast-Frame.          */
/*                                                                      */
/*----------------------------------------------------------------------*/
void
brosnd(MBHEAD **mbpp, PEER *pp)
{
  if (*mbpp == NULL)
  {
    if (pp->typ == THENET /* || pp->typ == NETROM*/)
    {
      (*mbpp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l3_typ = L2CUI;
#ifdef __WIN32__
      putchr((char)0xFF, *mbpp);
#else
      putchr(0xFF, *mbpp);
#endif /* WIN32 */
      pu6chr(alias, *mbpp);
#if MAX_TRACE_LEVEL > 6
      notify(7, "brosnd");
#endif
    }
    else
      return;
  }

  rwndmb(*mbpp);                /* vor dem Senden Buffer zurueckspulen  */

  if ((*mbpp)->l3_typ == L2CI)  /* Als I-Frame ueber den Link senden    */
    toneig(pp, *mbpp);
  else
  {
    (*mbpp)->l2fflg = L2CNETROM;/* Als UI-Bake rausschicken an "NODES"  */
    sdl3ui(pp, *mbpp);          /* PID = Level 3                        */
  }
  *mbpp = NULL;                 /* und Buffer loeschen                  */
}

/************************************************************************/
/*                                                                      */
/* "add broadcast"                                                      */
/*                                                                      */
/* Einen Weg zu dem Broadcast-Frame fuer einen Nachbarn hinzufuegen.    */
/* Das Frame wird gesendet, sobald es voll ist oder in brosrv() wenn    */
/* der Timeout abgelaufen ist.                                          */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void
add_netrom_info(MBHEAD **mbpp, NODE *node, PEER *topp,
                PEER *viapp, unsigned qualit)
{
  int qual = rtt2qual(qualit);

  /* schlechte Wege werden unterdrueckt, abmelden kennt NETROM nicht    */
  if (qual <= worqua)
    return;

  if (*mbpp)
    if (((*mbpp)->mbpc + 21) > 256)      /* bis Frame voll              */
      brosnd(mbpp, topp);

  if (*mbpp == NULL)
  {                                      /* neues Frame holen           */
    (*mbpp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l3_typ = L2CUI;
#ifdef __WIN32__
    putchr((char)0xFF, *mbpp);
#else
    putchr(0xFF, *mbpp);
#endif /* WIN32 */
    pu6chr(alias, *mbpp);
  }

  putfid(node->id, *mbpp);               /* Call des Zieles             */
  pu6chr(node->alias, *mbpp);            /* Ident des Zieles            */
  if (qualit && viapp)
    putfid(viapp->l2link->call, *mbpp);  /* Nachbar des Zieles          */
  else
    putfid(myid, *mbpp);                 /* Nachbar geloescht           */
#ifdef __WIN32__
  putchr((char)rtt2qual(qualit), *mbpp);
#else
  putchr(rtt2qual(qualit), *mbpp);
#endif /* WIN32 */
}

/************************************************************************/
/*                                                                      */
/* Einen Weg zu dem Infocast-Frame fuer einen Nachbarn hinzufuegen.     */
/* Das Frame wird gesendet, sobald es voll ist oder in brosrv() wenn    */
/* der Timeout abgelaufen ist.                                          */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void
add_thenet_info(MBHEAD **mbpp, NODE *node, PEER *topp,
                PEER *viapp, unsigned qualit)
{
  if (*mbpp)
    if (((*mbpp)->mbpc + 21) > 256)      /* bis Frame voll              */
      brosnd(mbpp, topp);

  if (*mbpp == NULL)
  {                                      /* neues Frame holen           */
    (*mbpp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l3_typ = L2CI;
    putfid(myid, *mbpp);                 /* von eigenes Call            */
    putfid("L3RTT \140", *mbpp);         /* an Pseudo-Destination       */
    *((*mbpp)->mbbp - 1) |= 1;           /* EOA setzen                  */
    putchr(0x02, *mbpp);                 /* Lifetime = 2                */
    putchr(0x00, *mbpp);                 /* Circuit-Index               */
    putchr(0x00, *mbpp);                 /* Circuit-ID                  */
    putchr(0x00, *mbpp);                 /* TX-Sequence-Number          */
    putchr(0x00, *mbpp);                 /* RX-Sequence-Number          */
    putchr(0x05, *mbpp);                 /* OpCode & Flags              */
    putstr("BROAD", *mbpp);              /* Framekennung                */
  }

  putfid(node->id, *mbpp);               /* Call des Zieles             */
  pu6chr(node->alias, *mbpp);            /* Ident des Zieles            */
  if (qualit && viapp)
    putfid(viapp->l2link->call, *mbpp);  /* Nachbar des Zieles          */
  else
    putfid(myid, *mbpp);                 /* Nachbar geloescht           */
#ifdef __WIN32__
  putchr((char)rtt2qual(qualit), *mbpp);
#else
  putchr(rtt2qual(qualit), *mbpp);
#endif /* WIN32 */
}

static void
addbro(MBHEAD **mbpp, NODE *node, PEER *topp, PEER *viapp,
       unsigned qualit, unsigned lastqual, int lt)
{
  switch (topp->typ)
    {
      case THENET:
      case NETROM:
        add_netrom_info(mbpp, node, topp, viapp, qualit);
        break;
      case TNN:
        add_thenet_info(mbpp, node, topp, viapp, qualit);
        break;
      case INP:
        add_inp_info(mbpp, node, topp, qualit, lastqual, lt);
        break;
    }
}

/*----------------------------------------------------------------------*/
/* Broadcast fuer einen Nachbarn                                        */
void
inform_peer(PEER *pp, int kind_of)
{
  INDEX     index;
  int       max_nodes = netp->max_nodes;
  NODE     *np = netp->nodetab;
  ROUTE    *rp = pp->routes;
  PEER     *bestpp;
  UWORD    quality;
  unsigned int reported_quality;
  unsigned int diff;
  UWORD     maxtime;

  int       timeout;
  MBHEAD   *mbp = NULL;                       /* noch kein Buffer offen */

  BOOLEAN   old_netrom = (pp->typ == NETROM || pp->typ == THENET);

  for (index = 0; index < max_nodes; index++, np++, rp++)
  {
    if (np->id[0] == 0)                         /* kein Eintrag         */
      continue;

    if (cmpid(np->id, pp->l2link->call))        /* der Nachbar          */
      continue;

#ifdef LINKSMOD_LOCALMOD
    if (CheckLocalLink(np->id) == TRUE)         /* Ist ein Local-Link. */
      continue;                                /* Zum nechsten Eintrag. */
#endif /* LINKSMOD_LOCALMOD */

/************************************************************************/
/**** HACK: wird nur benoetigt, solange wir nur die Flexnet Nachbarn ****/
/**** und nicht deren Ziele melden wollen                            ****/
#ifndef FLEX_ROUTINGFIX
    if (   ((quality = find_best_qual(index, &bestpp, VC | VC_FAR | DG))
            != 0)
        && (bestpp->options == VC_FAR)
#else
/* Hiermit werden keine FLEXNET-Routen durchgelassen, FLEXGATE deaktiviert. */
/* Es duerfen nur LOCAL und LOCAL_M (VC) ins Netrom-Netz geroutet werden.   */
/* LOCAL_N und LOCAL_V werden weiter oben behandelt und duerfen nicht       */
/* weiter geroutet werden !!!                                               */
    if (   ((quality = find_best_qual(index, &bestpp, HOST_MASK)) != 0)
        && (bestpp->options == VC)
#endif /* FLEX_ROUTINGFIX */
        && (cmpid(bestpp->l2link->call, np->id)))
    {
    }
    else
/************************************************************************/

/** An Nachbarn, die das gleiche Call haben, wie der beste Weg (also    */
/* auch der beste Weg selbst) melden wir 0.                             */
      quality = find_best_qual(index, &bestpp, HOST_MASK);
    if (cmpid(bestpp->l2link->call, pp->l2link->call))
        quality = 0;

/*** Alternativ:                                                      ***/
/*** quality = find_best_notvia(index, pp, &bestpp, HOST_MASK);       ***/

    reported_quality = rp->reported_quality;
    if (np->alias[0] == '#')            /* versteckter Node             */
    {
      if (!reported_quality)            /* hatten wir gemeldet?         */
        continue;                       /* nein, also auch jetzt nicht  */
      quality = 0;                      /* ja, also abmelden            */
    }
    if (kind_of == ALL)
    {
      /* gesicherten Nachbarn brauchen wir nur Ziele zu uebertragen,    */
      /* die wir schon mal gemeldet hatten                              */
      if (!old_netrom && !reported_quality)
        continue;
      /* wir senden alle Routen ohne Timeout und diejenigen, die noch   */
      /* mindestens 30% des Timeouts uebrig haben                       */
      timeout = rp->timeout;
      if (   (timeout == 0)
#ifndef THENETMOD
          || (timeout >= (ROUTE_TIMEOUT / 3)))
#else
          || (timeout >= (obcini / 3)))
#endif /* THENETMOD */
      {
        addbro(&mbp, netp->nodetab + index, pp, bestpp, quality,
               reported_quality, bestpp->routes[index].lt);
        rp->reported_quality = quality;/* merken was wir gemeldet haben */
      }
    }
    else
    {
/* Die maximal zulaessige Laufzeit fuer Meldungen ist entweder 600s     */
/* oder ein vom Nachbarn vorgegebener kleinerer Wert. Hat der Nachbar   */
/* eine max. Laufzeit gemeldet, so wird diese beruecksichtigt.          */
      maxtime = pp->maxtime;
      if (!maxtime || maxtime > HORIZONT)
        maxtime = HORIZONT;
/* Zusaetzliches Filter: Bei der ersten Meldung eines Zieles darf die   */
/* Laufzeit hoechstens die Haelfte des Maximalwertes betragen, damit    */
/* wir nicht schon bei der ersten Verschlechterung das Ziel wieder      */
/* abmelden muessen.                                                    */
      if (!reported_quality)                    /* noch nix gemeldet?   */
        if (quality > (maxtime / 2))            /* zu schlecht?         */
          continue;

      if (reported_quality != DIRTY)
      {
        if (quality == reported_quality)
          continue;                                  /* keine Aenderung */

/* Wenn ein Ziel ausfaellt oder neu gemeldet wird, dann brauchen wir    */
/* nicht zu kontrollieren, ob die Meldung unterdrueckt werden kann.     */
/* Diese Meldungen sind zwingend.                                       */
        if (quality && reported_quality)
        {
/* Verbesserungen werden nur gemeldet, wenn Sie mindestens um 50% von   */
/* der letzten Meldung abweichen. Ebenfalls nicht gemeldet werden sehr  */
/* geringe Verbesserungen (<= 100ms) bei sehr schnellen Links           */
/* (Aenderung z.B. von 50ms auf 90ms wird nicht gemeldet, obwohl es 60% */
/* sind). Die Verbesserung muss mindestens ueber der halben Laufzeit    */
/* zum empfangenden Segment liegen. Die Verbesserung wird nur gemeldet  */
/* wenn der Link nicht verstopft ist!                                   */
          if (quality < reported_quality)
          {
            diff = reported_quality - quality;
            if (diff < reported_quality / 2)  /* Filter 1:              */
              continue;                       /*       50% Verbesserung */
            if (diff < 10)                    /* Filter 2:              */
              continue;                       /*       mindestens 100ms */
            if (diff < pp->quality / 2)       /* Filter 3:              */
              continue;                       /*    adaptiv nach Zielr. */
            if (pp->nbrl2l->tosend > 7)       /* Filter 4:              */
              continue;                       /*        Link verstopft? */
          }
          else
          {
/* Verschlechterungen muessen immer sofort uebertragen werden! Wir      */
/* senken die Qualitaet bei der Meldung vorbeugend um 12.5% ab und      */
/* zusaetzlich um die 1/2 Laufzeit zu diesem Nachbarn. Bei weiterem     */
/* Abfall sollen weitere hektische Meldungen dadurch vermieden werden.  */
#ifdef __WIN32__
            quality += quality / 8 + (unsigned short)pp->quality / 2;
#else
            quality += quality / 8 + pp->quality / 2;
#endif /* WIN32 */
/* Ist die neue Laufzeit groesser als der Nachbar wuenscht, wird das    */
/* Ziel abgemeldet.                                                     */
            if (quality > maxtime)
              quality = 0;
          }
        }
      }

      addbro(&mbp, netp->nodetab + index, pp, bestpp, quality,
             reported_quality, bestpp->routes[index].lt);

      rp->reported_quality = quality;  /* merken was wir gemeldet haben */

      /* bei abgemeldeten Nodes die Lifetime loeschen */
      if ((rp->reported_quality == 0) && (rp->quality == 0))
        rp->lt = 0;
    }
  }
      brosnd(&mbp, pp);
}

/* End of src/l3netrom.c */
