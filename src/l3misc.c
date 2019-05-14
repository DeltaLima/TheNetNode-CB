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
/* File src/l3misc.c (maintained by: ???)                               */
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

NETWORK *netp = NULL;

static int      get_next_prim(int);
static void     init_network(void);
static void     brosrv2(void);
static void     brosrv10(int);
static void     brosrv60(void);
static void     brosrv360(int, PEER *);
static void     netrom_status(PEER *, WORD);
static void     mstatus(PEER *, WORD);
static BOOLEAN  brotim10 = FALSE;

#ifdef AUTOROUTING
static PEER    *ReadLink(UBYTE pid);
#endif /* AUTOROUTING */

static int
get_next_prim(int num)          /* naechste Primzahl berechnen          */
{
  int i, j;

  for (i = num; ; i++)          /* intuitive Methode ausreichend        */
  {
    for (j = 2; j < i; j++)
      if (i % j == 0)
        break;
    if (i == j)                 /* eine Primzahl gefunden               */
      return(i);
  }
}

/* einen Router initialisieren und Tabellen anlegen */
BOOLEAN register_network(int max_peers, int max_nodes)
{
  char    huge *memp;

  memp = (char *)calloc(1, sizeof(NETWORK)+
                           sizeof(NODE)*max_nodes+
                           sizeof(PEER)*max_peers);
  if (memp) {
    netp = (NETWORK *) memp; memp += sizeof(NETWORK);
    netp->max_peers = max_peers;
    netp->max_nodes = max_nodes;
    netp->peertab = (PEER *) memp; memp += sizeof(PEER)*netp->max_peers;
    netp->nodetab = (NODE *) memp /*, memp += sizeof(NODE)*netp->max_nodes */;
    inithd(&netp->nodelis);
    return(TRUE);
  }
  return(FALSE);
}

/* einen Router deinitialisieren */
void unregister_network(void)
{
  free(netp);
}

/************************************************************************\
*                                                                        *
* Netzwerk-Heap initialisieren                                           *
*                                                                        *
\************************************************************************/
static void
init_network(void)
{
  char *s;

  if ((s = getenv("TNNCFG")) != NULL)
    sscanf(s, "%d,%d", &max_nodes, &max_peers);

  max_nodes = max(max_nodes, 1);
  max_peers = max(max_peers, 1);
  if (!register_network(max_peers, get_next_prim(max_nodes)))
    memerr();
  xprintf("*** NETWORK startup, %d Nodes, %d Neighbours\n",
          netp->max_nodes, netp->max_peers);
}

/************************************************************************\
*                                                                        *
* Initialisierung des Level 3                                            *
*                                                                        *
* FlexNet und Net/ROM Router intialisieren, Ziellisten loeschen.         *
*                                                                        *
\************************************************************************/
void
l3init(void)                    /* Level 3 initialisieren               */
{
  inithd(&l3rxfl);              /* Liste empfangene Frames loeschen     */
  inithd(&l3txl);               /* Liste zu sendende Frames loeschen    */
  init_network();
}

/************************************************************************/
/*                                                                      */
/* Informationstransfer von Level3 zum Level2                           */
/* Uebergeben werden reine Datenpackete ohne AX.25-Header. Diese werden */
/* dann an den angegebenen Link gehaengt und mit der gewuenschten PID   */
/* gesendet. Grosse Frames (> 256 Bytes) werden fragmentiert und mit    */
/* PID 0x08 gesendet (die gewuenschte PID wird im 1. Fragment ueber-    */
/* mittelt).                                                            */
/*                                                                      */
/************************************************************************/
void
i3tolnk(UBYTE pid, LNKBLK *linkp, MBHEAD *imbp)
{
  int           anz_frag,
                info;
  MBHEAD       *tmpmbp;
  BOOLEAN       frame1 = TRUE;

  info = ((imbp->mbpc) - (imbp->mbgc));         /* Laenge bestimmen     */
  ptctab[g_uid(linkp, L2_USER)].infotx += info;

#ifndef THENETMOD
  linkp->noatou = ininat;
#else                                        /* L4TIMEOUT */
  linkp->noatou = SetL4Timeout();            /* L2/L4-Timeout setzen.   */
#endif /* THENETMOD */

  if (info < 257)
  {
    imbp->l2fflg = pid;                         /* PID uebernehmen      */
    imbp->repeated = 0;
    relink((LEHEAD *)imbp,                      /* -> ab in den Link    */
           (LEHEAD *)linkp->sendil.tail);
    ++linkp->tosend;                            /* ein Sendepaket mehr  */
  }
  else
  {
/* fuer grosse Pakete generieren wir AX25-Fragmente                     */
    anz_frag = info / 255;              /* wieviele Folgefragmente?     */
    if (anz_frag > 127)                 /* zu gross geht nicht          */
    {
      dealmb(imbp);
      return;
    }
    do
    {
      tmpmbp = (MBHEAD *)allocb(ALLOC_MBHEAD);  /* Buffer holen         */
      tmpmbp->l2fflg = L2CFRAG;         /* wir senden mit PID 08        */
      tmpmbp->repeated = 0;
      if (frame1)
      {
#ifdef __WIN32__
        putchr((char)(anz_frag | 0x80), tmpmbp);/* es folgen n Fragmente        */
#else
        putchr(anz_frag | 0x80, tmpmbp);/* es folgen n Fragmente        */
#endif /* WIN32 */
        putchr(pid, tmpmbp);            /* mit pid                      */
        frame1 = FALSE;
      }
      else
#ifdef __WIN32__
        putchr((char)anz_frag, tmpmbp);       /* es folgen n Fragmente        */
#else
        putchr(anz_frag, tmpmbp);       /* es folgen n Fragmente        */
#endif /* WIN32 */
      while (tmpmbp->mbpc < 256)        /* max. 256 Bytes im Fragment   */
      {
        if (!(imbp->mbpc - imbp->mbgc)) /* Frameende                    */
          break;
        putchr(getchr(imbp), tmpmbp);   /* Daten umkopieren             */
      }
      rwndmb(tmpmbp);                       /* zurueckspulen            */
      relink((LEHEAD *)tmpmbp,              /* in die Sendeliste damit  */
             (LEHEAD *)linkp->sendil.tail);
      ++linkp->tosend;                      /* ein Frame mehr zu senden */
    } while (anz_frag-- > 0);
/* wir sind fertig                                                      */
    dealmb(imbp);
  }
}

/*----------------------------------------------------------------------*/
/* l3rx()           Empfangene Frames der Nachbarn verarbeiten.         */
/*----------------------------------------------------------------------*/
void
l3rx(void)
{
  PEER       *pp;
  int         max_peers = netp->max_peers;
  static int  peer = 0;

  if (++peer >= max_peers)
    peer = 0;

  if (peer < max_peers)
  {
    pp = &netp->peertab[peer];

    if (pp->used)
    {
      switch (pp->typ)
      {
        case FLEXNET:
          flexnet_rx(pp);
          break;
        case NETROM:
        case TNN:
        case THENET:
        case INP:
          netrom_rx(pp);
      }
    }
  }
}

/*..................................................................... */
/*..  Empfangsliste abgearbeitet, nun zu sendende Frames verarbeiten .. */
/*..................................................................... */
void
l3tx(void)                      /* Level 3 Service                      */
{
  UWORD   rxgetc;               /* Getcount des Frames                  */
  MBHEAD *mbp;                  /* Pointer auf aktuellen Framekopf      */
  NODE   *dstnod;
  PEER   *bestpp;
  MHEARD *mhp;

  /* solange L3-Sende-Liste nicht leer    */
  while (   (mbp = (MBHEAD *)l3txl.head)
         != (MBHEAD *)&l3txl.head)
  {
    ulink((LEHEAD *)mbp);                   /* Frame aushaengen         */
    dstnod = (NODE *)mbp->l2link;           /* Pointer auf Nachbarn     */
    if (dstnod == NULL)
      dstnod = netp->nodetab
               + find_node_this_ssid(mbp->destcall);

    if (   nmbfre > 14                      /* Platz im Speicher?       */
        && (valcal(dstnod->id) == YES)
        && find_best_qual((int)(dstnod - netp->nodetab),
                          &bestpp, DG) > 0)
    {
      rwndmb(mbp);                      /* Pointer auf Ausgangsstellung */
      getchr(mbp);                      /* alles initialisieren         */
      --mbp->mbbp;
      rxgetc = mbp->mbpc;               /* Framelaenge merken           */
      mbp->mbpc = 1;                    /* auf Anfang                   */
      putfid(myid, mbp);                /* Absender ins Frame           */
      putfid(dstnod->id, mbp);          /* Ziel ins Frame               */
      *(mbp->mbbp - 1) |= 1;            /* Ende der Adresse setzen      */
#ifdef __WIN32__
      putchr((char)timliv, mbp);              /* Lebensdauer setzen           */
#else
      putchr(timliv, mbp);              /* Lebensdauer setzen           */
#endif /* WIN32 */
      mbp->mbpc = rxgetc;               /* Putcount zurueck             */
      rwndmb(mbp);                      /* Pointer wieder aufziehen     */
      toneig(bestpp, mbp);              /* Frame an besten Nachbarn     */
      /* MH-Liste                     */
      if ((mhp = mh_lookup(&l3heard, dstnod->id)) != NULL)
        mhp->tx_bytes += (mbp->mbpc);
    }
    else                                /* kein Ziel def. o. kein Platz */
      dealmb(mbp);                      /* Frame wegwerfen              */
  }
}

/*..................................................................... */
/*....  sonstige Funktionen abarbeiten  ............................... */
/*..................................................................... */
void
l3rest(void)
{
  if (brotim10)
    brosrv2();
}

/*----------------------------------------------------------------------*/
void
brosrv(void)
{
  static int  tim10 = 1;
  static int  tim60 = 1;

  local_flex_srv();                 /* Time-Server der anderen Module   */

  if (++tim60 >= 60)                /* eine Minute vorbei               */
  {
    tim60 = 0;
    brosrv60();
  }

  if (++tim10 >= 10)
  {                                 /* 10 Sekunden vorbei               */
    drop_unreachable_nodes();
    tim10 = 0;
    brotim10 = TRUE;
  }
}

static void
brosrv2(void)
{
  static int  peer = 0;
  int         max_peers = netp->max_peers;

  if (++peer >= max_peers)
  {
    peer = 0;
    brotim10 = FALSE;
  }

  if (peer < max_peers)
    brosrv10(peer);
}

/************************************************************************/
/* 10-sekuendlicher Zeitserver                                          */
/************************************************************************/
static void
brosrv10(int i)
{
  PEER *pp = &netp->peertab[i];
  int   broint;

  if (pp->used)                 /* nur benutzte Peers bearbeiten */
  {
    if (pp->typ > NETROM)       /* nur fuer NETROM und verwandte Protokolle */
      return;

    if (nmbfre < 300)           /* nicht mehr genug Speicher            */
      return;

#ifdef THENETMOD
    if (pp->typ == THENET)                         /* Segment ist ein THENET. */
      return;   /* Funktion BroadCastBake() ist fuer den Rest verantwortlich. */
#endif /* THENETMOD. */

    if (pp->nbrl2l == NULL)     /* noch kein Link?                      */
      return;                   /* den grossen UI-Broadcast sparen wir  */
                                /* uns, uns hoert ja eh keiner.         */

    /* wenn der Link sich noch im Aufbau befindet oder ziemlich verstopft   */
    /* ist, wird die Nodes-Information zurueckgehalten.                     */
    if (   pp->nbrl2l->state < L2SIXFER
        || pp->nbrl2l->tosend > 20)             /* Link ist voll        */
      return;

    /* Bei INP senden wir erst los, wenn beide Messungen da sind.       */
    if (pp->typ == INP)
      if (!pp->my_quality || !pp->his_quality)
        return;

    /* Wollen wir einen INP-Link haben, haben einen Connect aber noch   */
    /* nicht zu INP umgeschaltet und die RTT-Messung laeuft noch, dann  */
    /* senden wir keinen Nodes-Broadcast irgendeiner Art */
    if ((pp->soll_typ == INP) && (pp->typ != INP) && (pp->rttstart != 0))
      return;

#ifdef L4NOBAKE
    if (pp->typ == NETROM)   /* Steht noch ein Routing-Typ fest, senden */
      return;                /* wir keine Bake  irgendeiner Art.        */
#endif /* L4NOBAKE */

    if (pp->typ != THENET)
      inform_peer(pp, CHANGES);     /* nur die Aenderungen durchsagen   */

    if (pp->typ != INP)
    {
      broint = (pp->typ == TNN) ? broint_i : broint_ui;
      if (++pp->brotim >= broint)
      {
        /* Broadcast anfordern              */
        pp->brotim = 0;
        /* und weg damit                    */
        inform_peer(pp, ALL);
      }
    }

    /* Peer ist INP-Nachbar, den Broadcast-Timer geringfuegig anders */
    /* benutzen als bei den anderen Typen */
    if (pp->typ == INP)
    {
      if (++pp->brotim >= 360)  /* jede Stunde einmal alles melden     */
      {
        pp->brotim = 0;         /* merken dass wir gebroadcastet haben */
        brosrv360(netp->max_nodes, pp);
      }
    }
  }
}

/************************************************************************/
/* minuetlicher Zeitserver                                              */
/************************************************************************/
static void
brosrv60(void)
{
  INDEX  index;
  NODE  *np;
  ROUTE *rp;
  PEER  *pp;
  int    i;
  int    max_nodes = netp->max_nodes;

#ifdef THENETMOD
  BroadCastBake();          /* Eine Broadcast-Nodes Bake zum Nachbarn senden. */
#endif /* THENETMOD */

  /* alle Peers durchgehen */
  for (i = netp->max_peers, pp = netp->peertab; i--; pp++)
  {
    if (!pp->used)   /* unbenutzte interessieren nicht */
      continue;

#ifdef THENETMOD
    if (OBSinitTimer(pp))      /* Restlebensdauer fuer Rundspruch minimieren. */
      continue;                    /* Segment wurde geloescht, zum naechsten. */
#endif /* THENETMOD */

    /* alle Nodes dieses Peers durchgehen */
    for (index = 0, np = netp->nodetab, rp = pp->routes;
         index < max_nodes;
         index++, np++, rp++)
    {
    if (np->id[0])                      /* nur benutzte Eintraege */
      {
        if (rp->lt > max_lt)              /* bei zu hoher Lifetime abmelden */
          update_route(pp, index, 0);

        if (rp->timeout)                  /* Route mit Timeout ? */
        {
          if (--rp->timeout == 0)         /* Timeout abgelaufen ? */
            update_route(pp, index, 0);   /* ja, dann abmelden */
        }
        else
            if (!pp->secured)               /* ungesicherte Route ? */
#ifndef THENETMOD
                rp->timeout = ROUTE_TIMEOUT;  /* dann Timeout neu setzen */
#else
                rp->timeout = obcini;         /* dann Timeout neu setzen */
#endif /* THENETMOD */
      } /* if (np->id[0]) */
    }  /* for */
   } /* for */
}

/************************************************************************/
/* stuendlicher Zeitserver                                              */
/************************************************************************/
static void
brosrv360(int max_nodes, PEER *pp)
{
  NODE  *np = netp->nodetab;
  ROUTE *rp = pp->routes;
  int    x;

#if MAX_TRACE_LEVEL > 2
  int    node_count = 0;
  char   notify_call[10];
#endif

  /* alle Eintraege durchgehen dabei keine Ruecksicht auf die    */
  /* Horizonte nehmen, dies soll sicherstellen, dass hier auch   */
  /* Nodes an den Nachbarn gemeldet werden die bisher immer      */
  /* durch die Filter gefallen sind weil die Aenderungen zu      */
  /* gering waren.                                               */
  for (x = max_nodes; x--; np++, rp++)
  {
    if (!np->id[0])              /* freier Eintrag?              */
      continue;

    /* Nodes die zur Abmeldung ausstehen nicht anfassen */
    if (rp->quality == 0 && rp->reported_quality != 0)
      continue;

    rp->reported_quality = 0;           /* Eintrag muss gemeldet werden */
#if MAX_TRACE_LEVEL > 2
    node_count++;
  }
  call2str(notify_call, pp->l2link->call);
  notify(3, "broadcasting INP nodes to %s (%u of %u nodes triggered for update)", notify_call, node_count, max_nodes);
#else
  }
#endif
}

/*----------------------------------------------------------------------*/
/* 6 Zeichen aus Message-Puffer lesen und nach Buffer schreiben         */
/* Rueckgabe: TRUE = hat funktioniert                                   */
/*----------------------------------------------------------------------*/
BOOLEAN
ge6chr(char *buffer, MBHEAD *mbp)
{
  int i,
      ch;

  for (i = 6;
       (mbp->mbpc > mbp->mbgc) && i;    /* noch Zeichen da?             */
       i--)
  {
    *buffer++ = ch = getchr(mbp);
    if (ch < ' ')
      return (FALSE);                   /* ungueltiges Zeichen          */
    if (ch > 127)
      return (FALSE);
  }
  return (i == 0);                      /* waren genug Zeichen da?      */
}

/************************************************************************/
/* 6 Zeichen aus Buffer in Message-Puffer schreiben                     */
/************************************************************************/
void
pu6chr(char *buffer, MBHEAD *mbp)
{
  WORD i;

  for (i = 0; i < 6; ++i)
    putchr(*buffer++, mbp);
}

/************************************************************************/
/* Eine Status-Aenderung fuer NET/ROM behandeln                         */
/************************************************************************/
static void
netrom_status(PEER *pp, WORD status)
{
#ifdef PORT_L2_CONNECT_TIME
  UWORD port = 0;
#endif
  if (pp->typ <= NETROM)
  {                                         /* nur NET/ROM interessiert */
    switch (status)
    {
      case L2MCONNT:
        connect_peer(pp);
        break;

      case L2MLREST:
      case L2MLRESF:
        reset_peer(pp);
        break;

      case L2MFAILW :
      case L2MDISCF :
#ifdef PORT_L2_CONNECT_TIME
        port = pp->l2link->port;
        pp->l2link->sabmtime = portpar[port].l2_connect_time;
#endif /* PORT_L2_CONNECT_TIME */

        if (  (pp->typ != THENET)                /* Alle Typen ausser THENET. */
            ||((pp->typ == THENET)                     /* oder THENET-Typ und */
            && (status == L2MFAILW)))                      /* Status FAILURE. */
        {                                    /* Route als ausgefallen melden. */
          update_peer_quality(pp, 0L, DONT_CHANGE_QUAL);
          memset(pp->routes, 0, netp->max_nodes * sizeof(ROUTE));
          pp->num_routes = 0;                 /* Routen loeschen              */
          drop_unreachable_nodes();           /* Routes/Nodes loeschen        */

#ifdef AUTOROUTING
          if (pp->l2link->ppAuto == AUTO_ROUTE)/* Segment ist eine Auto-Route.*/
          {
#ifdef AXIPR_HTML
            /* Protokoll fuer HTML-Ausgabe setzen. */
            SetHTML(pp->l2link->port, pp->l2link->call, NULL, FALSE);
#endif /* AXIPR_HTML */
            /* Link austragen. */
            unregister_neigb(pp->l2link->call, pp->l2link->digil, pp->l2link->port);
            return;
          }
#endif /* AUTOROUTING */

        }

        break;

      default:
        disconnect_peer(pp);
        update_peer_quality(pp, 0, DONT_CHANGE_QUAL);
        check_all_destot();
        break;
    }
    pp->rttstart = 0;
    pp->rtt_time = 0;
    switch (pp->typ)
    {
      case TNN:
      case INP:
        set_peer_typ(pp, NETROM);
    }
    pp->brotim = 0;                         /* mit Broadcast beginnen   */
  }
}

/*----------------------------------------------------------------------*/
/* eine Status-Aenderung an alle L3-Module senden                       */
/* Die Module muessen selber darauf achten, das sie die Meldung nur     */
/* annehmen, wenn der Typ des pp stimmt!                                */
/*----------------------------------------------------------------------*/
static void
mstatus(PEER *pp, WORD status)
{
  netrom_status(pp, status);                /* neuen Status melden      */
  flex_status(pp, status);                  /* an NETROM & FlexNet      */
  local_status(pp, status);                 /* und unsere Locals        */
}

/*----------------------------------------------------------------------*/
/* Neue Status-Meldung verarbeiten                                      */
/* 0=nicht verwendet, 1=connectet, 2=disconnectet, 3=busy, 4=Failure    */
/*----------------------------------------------------------------------*/
BOOLEAN
l2tol3(WORD status)
{
  PEER *pp;
#if MAX_TRACE_LEVEL > 2
  char  notify_call[10];
#endif

  switch (status)
    {
      case L2MCONNT:                        /* CONNECTED to             */
      case L2MLREST:                        /* LINK RESET to            */
        if ((pp = ispeer()) == NULL)
          return (l2toip(status));          /* der L7 ist dran          */
        newnbr(pp);                         /* Nachbar eintragen        */
        mstatus(pp, status);                /* Status melden            */
        return (TRUE);                      /* Status verarbeitet       */

      case L2MLRESF:                        /* LINK RESET from          */
        if ((pp = ispeer()) == NULL)
          return (l2toip(status));          /* der L7 ist dran          */
        mstatus(pp, status);                /* Status melden            */
        disnbr(pp);                         /* Nachbar inaktiv          */
        newnbr(pp);                         /* Nachbar eintragen        */
        mstatus(pp, status);                /* Status melden            */
#if MAX_TRACE_LEVEL > 2
        call2str(notify_call, rxfhdr + L2IDLEN);
        notify(3, "Linkreset %6s", notify_call);
#endif
        return (TRUE);                      /* Status verarbeitet       */

      case L2MDISCF:                        /* DISCONNECTED from        */
      case L2MBUSYF:                        /* BUSY from                */
      case L2MFAILW:                        /* LINK FAILURE with        */
        if ((pp = ispeer()) == NULL)
          return (l2toip(status));          /* der L7 ist dran          */
#ifdef AXIPR_UDP
        newnbr(pp);                         /* Nachbar eintragen        */
#endif
        mstatus(pp, status);                /* melden                   */
        disnbr(pp);                         /* Nachbar inaktiv          */
#ifdef AUTOROUTING
        /* Segment steht auf Auto-Route. */
        if (pp->l2link->ppAuto == AUTO_ROUTE)
          return(FALSE);
        else
#endif /* AUTOROUTING */
        return (TRUE);                      /* nicht in den L7          */

      case L2MFRMRF:                        /* FRAME REJECT from        */
      case L2MFRMRT:                        /* FRAME REJECT to          */
        if ((pp = ispeer()) == NULL)
          return (l2toip(status));          /* der L7 ist dran          */
        return (TRUE);                      /* Status verarbeitet       */

      case L2MBUSYT:                        /* BUSY to                  */
        return (TRUE);                      /* hier abfangen            */
    }

  return (FALSE);                           /* ungueltige Meldung       */
}

/************************************************************************\
*                                                                        *
* "to level 3 switch"                                                    *
*                                                                        *
* Aus I- oder UI-Frame (Framekopf fbp, Getzeiger/Zaehler auf 1. Byte     *
* hinter Level-2-Adressfeld) PID holen, falls vorhanden. Falls es nicht  *
* Level-2-PID ist, das Paket an die Level-3-Empfangsframeliste l3rxfl    *
* haengen. Im Framekopf wird in jedem Fall l2fflg auf PID, wenn          *
* vorhanden, oder 0 gesetzt, l2link auf den aktuellen Link (lnkpoi).     *
* Fuer Level-3-Frames wird der Nachbar bestimmt.                         *
*                                                                        *
* Return:  TRUE -  das I/UI-Frame hat ein Nicht-Level-2-PID und wurde an *
*                  die Level-3-Empfangsframeliste gehaengt               *
*          FALSE - Frame hat Standard-Level-2-PID                        *
*                                                                        *
\************************************************************************/
BOOLEAN
tol3sw(MBHEAD *fbp)
{
  PEER  *pp;
  UBYTE  pid = fbp->l2fflg;
  int    state;
  int    filter_ip_frame;

  fbp->l2link = lnkpoi;           /* Linkverbindung von diesem Frame    */
  fbp->l2port = rxfprt;           /* Auf diesem Port kam das Frame      */

  if (lnkpoi)
  {
    state = ptctab[g_uid(lnkpoi, L2_USER)].state;
    filter_ip_frame = (state == D_IPLINK || state == U_IPLINK);
  }
  else
    filter_ip_frame = TRUE;

/* Protokolle, die nicht an einen Nachbarn gebunden sind, direkt        */
/* abfangen                                                             */
  switch (pid)
    {
#ifdef IPROUTE
      case L2CIP:
        if (filter_ip_frame)
        {
          if (fbp->l2link)        /* fuer Level 2 Links Info zaehlen    */
            ptctab[g_uid(fbp->l2link, L2_USER)].inforx
                         += (fbp->mbpc - fbp->mbgc);

          relink((LEHEAD *)fbp, (LEHEAD *)iprxfl.tail);
          return (TRUE);          /* Frame verarbeitet                  */
        }
        else
          break;

      case L2CARP:
        if (filter_ip_frame)
        {
          if (fbp->l2link)        /* fuer Level 2 Links Info zaehlen    */
            ptctab[g_uid(fbp->l2link, L2_USER)].inforx
                         += (fbp->mbpc - fbp->mbgc);

          relink((LEHEAD *)fbp, (LEHEAD *)arprxfl.tail);
          return (TRUE);          /* Frame verarbeitet                  */
        }
        else
          break;
#endif

#ifdef L2PROFILER
      case 0x12:                  /* Spielzeug fuer DB7KG (PID 12)      */
        dealmb(fbp);
        return (TRUE);
#endif
    }

#ifndef AUTOROUTING
  if ((pp = ispeer()) == NULL)    /* kein Frame fuer einen Nachbarn?    */
    return (FALSE);               /* zum Level 7 durchlassen            */
#else
  /* Pruefen, ob sich jemand Automatisch anbinden will. */
  /* Wir untersuchen anhand der PID das Segment.        */
  if ((pp = ReadLink(pid)) == NULL)
    return (FALSE);               /* zum Level 7 durchlassen            */
#endif

/* Hier landen nur Frames, die zu einem bestimmten Nachbarn gehoeren.   */
/* Nun muss noch Protokoll-ID und Protokoll verglichen werden.          */

 if (fbp->l2link)                /* fuer Level 2 Links Info zaehlen    */
    ptctab[g_uid(fbp->l2link, L2_USER)].inforx
                 += (fbp->mbpc - fbp->mbgc);

  switch (fbp->l2fflg)
    {
/* Protokoll/Nachbartyp feststellen                                     */
/* NET/ROM-Protokoll fuer TheNet, TheNetNode, TNX1J(TexNet)             */
/* KA9Q und Devirate, Wampes, Linux usw., BPQ, andere Switches          */
      case L2CTEXNET:             /* fuer TexNet/TheNetX1J              */
      case L2CNETROM:
        if (pp->typ > NETROM)
          break;                  /* nicht richtiger Nachbar-Typ?       */
        rxneig(pp, fbp);          /* Frame zur Nachbar-Bearbeitung      */
        return (TRUE);            /* ... und fertig                     */

                                  /* Flexnet Protokoll fuer             */
      case L2CFLEXNET:            /* Flexnet eben, BayCom, Digiware     */
        if (pp->typ != FLEXNET)
          break;
        if (lnkpoi == NULL)
          break;                  /* UI-Frames sind verboten            */
        rxneig(pp, fbp);          /* FlexNet-Internode-Frame            */
        return (TRUE);            /* toL7: Frame verarbeitet            */

#ifdef IPROUTE                    /* IP/ARP auf Interlink ist erlaubt   */
      case L2CIP:
        relink((LEHEAD *)fbp, (LEHEAD *)iprxfl.tail);
        return (TRUE);            /* Frame verarbeitet                  */

      case L2CARP:
        relink((LEHEAD *)fbp, (LEHEAD *)arprxfl.tail);
        return (TRUE);            /* Frame verarbeitet                  */
#endif
    }
  dealmb(fbp);                  /* unpassende/unbekannte PID, falscher  */
                                /* Nachbar, Fehler...                   */
  return (TRUE);
}

/*----------------------------------------------------------------------*/
/* Kopiert zwei Calls ins Ziel (via-Beschraenkung fuer Linkeintraege)   */
/*----------------------------------------------------------------------*/
void
cpyidl2(char *dest, const char *source)
{
  int count = 2;

  while (*source != '\0' && count-- > 0)
  {
    memcpy(dest, source, L2IDLEN);
    source += L2IDLEN;
    dest += L2IDLEN;
  }
  *dest = '\0';
}

#define TO       0      /* dir */
#define BACK     1

#define NOTKNOWN 1      /* tome und ret */
#define ISLOCAL  2
#define VIAFLEX  3
#define DIRECT   4
#define LOOPED   5

/* Diese Funktion analysiert die empfangenen NRR-Frames.                */
/* Etliche Fehler werden erkannt und entsprechend reagiert.             */
/*                                                                      */
/* tome   gibt an, ob mich das Frame was angeht                         */
/* ret    besagt, ob das Frame zurueckgeschickt werden soll und wieso   */
/* dir    gibt die NRR-Richtung an                                      */
/* looped ist wahr, wenn unsere ID im Hinweg vorkam und dir gleich TO   */
void
nrr_rx(MBHEAD *mbp, PEER *rxpp)
{
  NRRLIST  list[30],
          *l;
  int      nlist,
           ret,
           tome,
           dir;
  BOOLEAN  looped;
  PEER    *topp;

  if (!time_to_live)                           /* zuspaet               */
     return;


  looped = tome = ret = FALSE;
  dir = TO;
  topp = NULL;

  if (cmpid(desnod, myid))                     /* das geht uns an...    */
    tome = DIRECT;
  else if (iscall(desnod, NULL,                /* sonst den besten Weg  */
                  &topp, DG | VC | VC_FAR))
  {                                            /* bestimmen             */
    if (topp->options & VC)                    /* ein local von uns     */
      tome = ISLOCAL;
    else if (topp->options & VC_FAR)           /* wuerde via flex gehen */
      tome = VIAFLEX;
  }

  nlist = 0;
  l = list;
  while (mbp->mbpc - mbp->mbgc >= L2IDLEN + 1)
  {                                                /* Eintrag da?       */
    if (getfid(l->id, mbp) == TRUE)
    {
      l->lt = getchr(mbp);
      if (l->lt & ECHO_FLAG)
      {                                        /* EchoFlag              */
        dir = BACK;
        looped = FALSE;
      }
      if (dir == TO && cmpid(l->id, myid))     /* Loop in Hin-Liste     */
        looped = TRUE;
      nlist++;
      l++;
    }
  }
  l->id[0] = '\0';                             /* Liste beenden         */

  if (   !nlist                                /* leere Liste           */
      || rxpp == topp)                         /* Loop in den Nodes     */
      return;

  if (cmpid(list[0].id, myid))
  {                                            /* das ist meine Anwort  */
    nrr2usr(list, time_to_live);
    return;
  }

  if (dir == BACK && (   topp == NULL          /* Ziel unbekannt        */
                      || tome == VIAFLEX))     /* bzw. Flex im Rueckweg */
      return;


  if (topp == NULL && !tome)                   /* Ziel unbekannt        */
    ret = NOTKNOWN;
  else if (looped)                             /* Loop in der Liste     */
    ret = LOOPED;
  else if (tome)                               /* wir sind das Ziel     */
    ret = tome;

  if (ret)
  {                                            /* Weg umdrehen          */
    cpyid(desnod, orgnod);
    cpyid(orgnod, myid);
  }

  if (nlist < 29)
  {                                            /* wir passen noch dran  */
    cpyid(l->id, myid);
    l->lt = time_to_live + 1;
    if (ret)
    {
      l->id[L2CALEN] &= 0x1f;
      l->id[L2CALEN] |= (ret - 1) << 5;        /* Fehler in SSID-Feld   */
      l->lt |= ECHO_FLAG;
    }
    l++;
    l->id[0] = '\0';                           /* Liste beenden         */
  }

  send_nrr_frame(list);
}

/* Hier wird das NRR-Frame zusammengebastelt und verschickt             */
void
send_nrr_frame(NRRLIST *l)
{
  PEER   *pp;
  NODE   *dstnod;
  MBHEAD *mbp;

  if (iscall(desnod, &dstnod, &pp, DG))
  {
    mbp = (MBHEAD *)allocb(ALLOC_MBHEAD);

    putfid(orgnod, mbp);
    putfid(desnod, mbp);
    putchr(time_to_live, mbp);
    putchr(l4hdr0, mbp);
    putchr(l4hdr1, mbp);
    putchr(l4hdr2, mbp);
    putchr(l4hdr3, mbp);
    putchr(l4hdr4, mbp);
    while (*l->id)
    {
      putfid(l->id, mbp);
      putchr(l->lt, mbp);
      l++;
    }
    rwndmb(mbp);
    toneig(pp, mbp);
  }
}

/*----------------------------------------------------------------------*/
/* Diese Routine bereitet das Senden des NRR-Requestframes vor          */
void
request_nrr(char *id, UID uid)
{
  NRRLIST list[2];

  cpyid(desnod, id);                                         /* L3-Teil */
  cpyid(orgnod, myid);
#ifdef __WIN32__
  time_to_live = (char)timliv;
#else
  time_to_live = timliv;
#endif /* WIN32 */

  l4hdr0 = 0;                                                /* L4-Teil */
  l4hdr1 = 1;

  l4hdr2 = uid >> 8;
  l4hdr3 = uid & 0xFF;
  l4hdr4 = 0;

  cpyid(list[0].id, myid);                                   /* Daten   */
  list[0].lt = time_to_live + 1;
  list[1].id[0] = NUL;

  send_nrr_frame(list);
}

/*----------------------------------------------------------------------*/
/* Diese Routine ermittelt, ob es auf einem Port Interlinks gibt        */
BOOLEAN islinkport(int port)
{
  PEER *pp;
  int i;

  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
    if ((pp->used) && (pp->l2link->port == port))
      return TRUE;

  return FALSE;
}

/* TEST DG9OBU */
/*----------------------------------------------------------------------*/
/* Prueft, ob das Call id ein lokaler Eintrag bei uns ist.              */
/* NO     = kein Local von uns                                          */
/* YES    = ein Local von uns UND erreichbar (oder Local ohne Messung)  */
/* ERRORS = ein Local von uns, aber derzeit nicht erreichbar            */
/*----------------------------------------------------------------------*/
TRILLIAN islocal(const char* id)
{
  PEER *pp;
  register int i;
  register int max_peers = netp->max_peers;

  for (i = 0, pp = netp->peertab; i < max_peers; ++i, pp++)
  {
    if (   (pp->used)
        && (cmpid(id, pp->l2link->call))        /* Call + SSID stimmt */
       )
    {
      if (pp->typ == LOCAL)
        return YES;
      else if (pp->typ == LOCAL_M)
        return (pp->my_quality != 0) ? YES : ERRORS;
    }
  }

  /* kein Local von uns */
  return NO;
}

#ifdef PROXYFUNC
BOOLEAN isproxy(const char* id)
{
  PEER *pp;
  register int i;
  register int max_peers = netp->max_peers;

  for (i = 0, pp = netp->peertab; i < max_peers; ++i, pp++)
  {
    if (   (pp->used)
        && (cmpid(id, pp->l2link->call))        /* Call + SSID stimmt */
       )
    {
      if (pp->proxy == TRUE)
        return TRUE;
    }
  }

  /* kein Local von uns */
  return FALSE;
}
#endif

/* TEST DG9OBU */
/*----------------------------------------------------------------------*/
/* Bestimmt die aktuelle SSID-Range des Knotens. Es werden alle lokalen */
/* Nodes mit dem gleichen Call wie der Knoten, sowie das Knotencall     */
/* selber ausgewertet. Die uebergebenen Variablen werden nur geaendert, */
/* wenn der aktuelle SSID-Bereich von den uebergebenen Werten abweicht. */
/*----------------------------------------------------------------------*/
void getSSIDrange(int *min, int *max)
{
  PEER *pp;
  register int i;

  /* Knotencall auswerten */
  if (SSID(myid) < *min)
    *min = SSID(myid);

  if (SSID(myid) > *max)
    *max = SSID(myid);

  /* Alle lokalen Links auswerten */
  for (i = 0, pp = netp->peertab; i < max_peers; ++i, pp++)
    if (   (pp->used)                /* nur benutzte Eintraege auswerten */
        && (cmpcal(myid, pp->l2link->call))    /* Call stimmt, SSID egal */
        && (pp->l2link->alias[0] != '#')     /* keine versteckten Locals */
        && (   (pp->typ == LOCAL) /* nur Locals, L+ muss erreichbar sein */
            || ((pp->typ == LOCAL_M) /* && (pp->my_quality != 0L) */ )
           )
       )
    {
      if (SSID(pp->l2link->call) < *min)
        *min = SSID(pp->l2link->call);

      if (SSID(pp->l2link->call) > *max)
        *max = SSID(pp->l2link->call);
    }
}

/* TEST DG9OBU */
/*----------------------------------------------------------------------*/
/* Bestimmt die aktuelle maximale SSID des Knotens.                     */
/* Naeheres siehe Funktion getSSIDrange().                              */
/*----------------------------------------------------------------------*/
int maxSSID(void)
{
  int ssid_low = 15;
  int ssid_high = 0;

  getSSIDrange(&ssid_low, &ssid_high);

  return ssid_high;
}

/* TEST DG9OBU */
/*----------------------------------------------------------------------*/
/* Bestimmt, ob die uebergebene SSID im SSID-Bereich des Knotens liegt. */
/*----------------------------------------------------------------------*/
BOOLEAN SSIDinrange(int ssid)
{
  int ssid_low = 15;
  int ssid_high = 0;

  getSSIDrange(&ssid_low, &ssid_high);

  if ((ssid >= ssid_low) && (ssid <= ssid_high))
    return TRUE;

  return FALSE;
}

#ifdef AUTOROUTING
static int UpdateLink(char *call, int port, UWORD pid, PEER *pp)
{
  switch(pid)
  {
    case L2CTEXNET:                            /* THENET, NETROM oder INP-TYP */
    case L2CNETROM:
#ifdef AXIPR_HTML
          SetHTML(port,                /* Protokoll fuer HTML-Ausgabe setzen. */
                  call,
                  pp,
                  TRUE);
#endif /* AXIPR_HTML */

      switch (portpar[port].poAuto)                                 /* Modus. */
      {
        case L_THENET  :                                       /* THENET-TYP. */
          return(THENET);                               /* THENET-TYP setzen. */

        case L_INP     :                                          /* INP-TYP. */
          return(INP);                                     /* INP-TYP setzen. */

        case L_INPFLEX :                                       /* Voll-Modus. */
          return(NETROM);                      /* Standard NETROM-Typ setzen. */

        default        :
          return(EOF);                               /* Kein Eintrag liefern. */
      }

     break;


    case L2CFLEXNET:                                           /* FLEXNET-TYP */
#ifdef AXIPR_HTML
          SetHTML(port,                /* Protokoll fuer HTML-Ausgabe setzen. */
                  call,
                  pp,
                  TRUE);
#endif /* AXIPR_HTML */

    switch (portpar[port].poAuto)                                   /* Modus. */
    {
      case L_FLEXNET :
      case L_INPFLEX :
        return(FLEXNET);                               /* FLEXNET-TYP setzen. */

      default:
        return(EOF);                                 /* Kein Eintrag liefern. */
    }
  } /* Switch */

  return(EOF);                                       /* Kein Eintrag liefern. */
}

/********************************************************/
/*                                                      */
/* Neues Segment anlegen.                               */
/* Je nach PID wird Link-Typ gesetzt.                   */
/*                                                      */
/********************************************************/
static PEER *AddLink(UWORD pid)
{
  PEER *pp = NUL;
  char  digil[L2VLEN + 1] = "";
  WORD  typs = EOF;

  if ((pp = ispeer()) == NULL)
  {
#ifdef LINKSMODINFO
    char Info[INFOSIZE + 1] = "AutoRoute";
#endif /* LINKSMODINFO */

    /* Segment Updaten und Typ setzen. */
    if ((typs = UpdateLink(rxfhdr + L2IDLEN, rxfprt, pid, pp)) == EOF)
      return(NULL);

    cpyid(digil, dheardcall(rxfhdr));

    if (cmpid(rxfhdr + L2IDLEN, digil))
      digil[0] = 0;

    /* Einen Neuen Link-Nachbar eintragen. */
    pp = register_neigb(rxfhdr + L2IDLEN, digil, rxfhdr + L2IDLEN, rxfprt, typs
#ifdef LINKSMODINFO
                       , Info
#endif /* LINKSMODINFO */
                       , AUTO_ROUTE);

    /* Linkeintragung fehlgeschlagen. */
    if (pp == NULL)
      /* Kein Eintrag liefern. */
      return(NULL);

    /* Den aktuellen Nachbarn als connected eintragen.*/
    newnbr(pp);

    /* Nachbar ist ein Flexnet. */
    if (pp->typ == FLEXNET)
      /* Maxtime auf default stellen. */
      pp->maxtime = 30000U;

    /* Neues Segment liefern. */
    return(pp);
  }

  /* Segment Updaten. */
  UpdateLink(rxfhdr + L2IDLEN, rxfprt, pid,  pp);

  /* Segment liefern. */
  return(pp);
}

/*****************************************************/
/*                                                   */
/* Pruefen, ob sich jemand Automtisch anbinden will. */
/* Anhand der PID wird das Segment ermittelt.        */
/*                                                   */
/*****************************************************/
static PEER *ReadLink(UBYTE pid)
{
  PEER *pp;

  if ((pp = AddLink(pid)) == NULL)
    /* Kein Segment gefunden. */
    return(NULL);

  /* Aktuelles Segment liefern. */
  return(pp);
}
#endif /* AUTOROUTING */

/* End of src/l3misc.c */
