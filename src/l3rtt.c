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
/* File src/l3rtt.c (maintained by: ???)                                */
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


/************************************************************************/
/* L3-RTT-Service                                                       */
/* erzeugt einen Frame mit L3-Header, bei dem Ziel und Absender         */
/* der eigene Knoten ist. Dieses Frame wird dann an den Nachbarn        */
/* gesendet. Die vorhergehende Messung wird dem Nachbarn mitgeteilt.    */
/* im Frame wird die vergangene Zeit in 10ms-Schritten seit Programm-   */
/* start mitgegeben.                                                    */
/* Automatische Qualitaetsberechnung aus dem L3SRTT, DB2OS 06/04/93     */
/*----------------------------------------------------------------------*/
void
l3rtt_service(void)
{
  PEER   *pp;
  int     i;
  int     max_peers = netp->max_peers;
  MBHEAD *mbp;
  INDEX   index;
#if MAX_TRACE_LEVEL > 2
  char    notify_call[10];
#endif

  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
  {
    if (!pp->used)
      continue;
    if (pp->typ > NETROM)
      continue;

#ifdef THENETMOD
    if (  (pp->typ       == THENET)                   /* THENET-Typ und */
        &&(pp->constatus == TRUE))     /* kein Dauerconnect gewuenscht. */
      continue;                               /* Zum naechsten Segment. */
#endif /* THENETMOD */

#ifdef L4NOBAKE
    if (pp->soll_typ == NETROM)/* Wenn noch kein Routing-TYP fest steht,*/
      continue;                               /* zum naechsten Segment. */
#endif /* L4NOBAKE */

    if (pp->nbrl2l == NULL)
    {
/* Der Nachbar, auf den pp zeigt, bekommt eine L3-Mini-Broadcast-Bake   */
/* von uns. Diese ist notwendig, da einige aeltere Versionen von TheNet */
/* und andere NET/ROM-Implementierungen den Nachbarn nur annehmen, wenn */
/* vorher bereits eine L3-Bake empfangen wurde. Anschliessend wird eine */
/* Verbindung zum Nachbarn hergestellt.                                 */
/* Wollen wir einen INP-Link, so senden wir keine NODES-Bake vorher da  */
/* wir mit NETROM ja nix machen wollen und wir wegen INP annehmen, dass */
/* die Gegenseite keine Uralt-Soft hat, die diese Minibake braucht.     */
#ifndef THENETMOD
      if (pp->soll_typ != INP)
#else
      if (pp->typ == TNN)      /* L3-Mini Bake nur fuer TNN-Typ zulassen. */
#endif /* THENETMOD */
      {
        mbp = (MBHEAD *)allocb(ALLOC_MBHEAD);     /* Buffer besorgen      */
#ifdef __WIN32__
        putchr((char)0xFF, mbp);
#else
        putchr(0xFF, mbp);
#endif /* WIN32 */
        pu6chr(alias, mbp);         /* Ident in Buffer                    */
        mbp->l2fflg = L2CNETROM;    /* PID = Level 3                      */
        rwndmb(mbp);                /* Pointer aufziehen                  */
        sdl3ui(pp, mbp);            /* UI-Broadcast senden                */
      }

#ifdef PORT_L2_CONNECT_TIME
      if (pp->l2link->sabmtime == 0)
      {
#endif
#ifdef PORT_L2_CONNECT_RETRY
        if (connbr(pp))
          pp->nbrl2l->tries = (UBYTE)portpar[pp->nbrl2l->liport].retry - (UBYTE)portpar[pp->nbrl2l->liport].l2_connect_retry;

        continue;
#else
        if (connbr(pp))
          pp->nbrl2l->tries = portpar[pp->nbrl2l->liport].retry - 1;

        continue;
#endif
#ifdef PORT_L2_CONNECT_TIME
      }
      else
        {
          pp->l2link->sabmtime--;
          continue;
        }
#endif
    }
    if (pp->nbrl2l->state < L2SIXFER)
      continue;
    if (pp->rttstart)
    {
      if (tic10 - pp->rttstart >= 18000L)  /* maximal 180 Sekunden      */
      {
#if MAX_TRACE_LEVEL > 2
        call2str(notify_call, pp->l2link->call);
        notify(3, "%s: l3rtt too high: %ld", notify_call,
               tic10 - pp->rttstart);
#endif
        discnbp(pp);
      }
      continue;
    }
    if (pp->rtt_time == 0)
    {                             /* Messung ist faellig                */
      if (pp->typ == THENET)
      {                           /* der kann nicht messen              */
        update_peer_quality(pp, pp->nbrl2l->SRTT * 4L, DONT_CHANGE_QUAL);
        /* Falls das Timeout auf einer Route mal abfaellt,    */
        /* melden wir hier wieder die Routen-Qualitaet hoch.  */
        if ((index = add_route(pp, pp->l2link->call, 1)) != NO_INDEX)
          update_lt(pp, index, 1);
        rtt_metric(pp, pp->nbrl2l->SRTT * 4L);
        pp->rtt_time = L3_RTT_TIME;

#ifdef THENETMOD
        if (pp->constatus == FALSE)/* Link soll staendig Aktiv bleiben. */
          pp->nbrl2l->noatou = ininat;            /* L2-Timeout setzen. */
#endif /* THENETMOD */
      }
      else
#ifndef L4NOBAKE
        send_l3srtt_frame(pp);    /* Neue Messung starten               */
#else
       {
         if (pp->soll_typ != NETROM) /* Nur wenn Routing-TYP fest steht,*/
           send_l3srtt_frame(pp); /* Neue Messung starten               */
       }
#endif /* L4NOBAKE */
    }
    else
      pp->rtt_time--;             /* Messintervall runterzaehlen        */
  }
}

/*----------------------------------------------------------------------*/
void
send_l3srtt_frame(PEER *pp)
{
  MBHEAD *mbp;
  int     mtu;

  if (pp->typ > NETROM)                   /* nur bei NET/ROM erlaubt    */
    return;

  if (pp->nbrl2l)
  {                                       /* nur aktiver mit L2-Link..  */
    mbp = (MBHEAD *)allocb(ALLOC_MBHEAD); /* Buffer besorgen            */
    putfid(myid, mbp);                    /* von eigenes Call           */
    putfid("L3RTT \140", mbp);            /* an Pseudo-Destination      */
    *(mbp->mbbp - 1) |= 1;                /* EOA setzen                 */
    putchr(0x02, mbp);                    /* Lifetime = 2               */
    putchr(0x00, mbp);                    /* Circuit-Index              */
    putchr(0x00, mbp);                    /* Circuit-ID                 */
    putchr(0x00, mbp);                    /* TX-Sequence-Number         */
    putchr(0x00, mbp);                    /* RX-Sequence-Number         */
    putchr(0x05, mbp);                    /* OpCode & Flags             */
    putstr("L3RTT:", mbp);                /* Framekennung               */
    pp->rttstart = tic10;                 /* Startzeit merken           */
    putlong(tic10, TRUE, mbp);            /* 10ms Ticks seit Softstart  */
    putlong((ULONG)pp->quality, TRUE, mbp);  /* aktueller L3-SRTT Wert  */
    putlong((ULONG)pp->my_quality, TRUE, mbp);  /* letzter L3-RTT Wert  */
    putlong((ULONG)pp, TRUE, mbp);        /* Pointer als ID             */
    putchr(' ', mbp);
    pu6chr(alias, mbp);
    putstr(" LEVEL3_V2.1 ", mbp);         /* L3-Version                 */
    putstr(infostr, mbp);                 /* TNN-Version                */

    /* nur wenn wir einen INP-Link wollen auch dem Nachbarn sagen dass  */
    /* wir INP koennen und ggf. maxtime melden, ansonsten nix sagen     */
    if (pp->soll_typ == INP)
    {
      if (mymaxtime != 0)
        putprintf(mbp, " $M%u", mymaxtime);

      putstr(" $N\r", mbp);
    }

    mtu = portpar[pp->l2link->port].mtu;
    while (mbp->mbpc < mtu)               /* Buffer voll machen         */
      putchr(' ', mbp);
    rwndmb(mbp);                          /* Frame aufziehen            */
    toneig(pp, mbp);                      /* Frame an den Nachbarn      */
  }
}

/************************************************************************\
*                                                                        *
* Das Frame mit der gewuenschten Kennung vergleichen. Wenn ja,           *
* wird sie ueberlesen, sonst wird mbbp/mbgc restauriert.                 *
*                                                                        *
\************************************************************************/
BOOLEAN
match(MBHEAD *fbp, const char *text)
{
  char huge *mbbp;                  /* Sicherung von mbbp               */
  UWORD      mbgc;                  /* Sicherung von mbgc               */

  mbbp = fbp->mbbp;                 /* Position im Frame (mbbp,mbgc)    */
  mbgc = fbp->mbgc;                 /* merken, falls kein Erfolg        */
  while (*text)
  {                                 /* die Kennung durchgehen           */
    if (getchr(fbp) != *text++)
    {                               /* Abweichung!                      */
      fbp->mbbp = mbbp;             /* auf die alte Position im Frame   */
      fbp->mbgc = mbgc;             /* zurueckgehen                     */
      return (FALSE);               /* Kennung stimmt nicht             */
    }
  }
  return (TRUE);                    /* Kennung stimmt                   */
}

/************************************************************************/
/*                                                                      */
/* Eine Bake senden mit Informationen zur erfolgten Laufzeitmessung     */
/*                                                                      */
/************************************************************************/
void
rtt_metric(PEER *pp, long rtt)
{
  BEACON    *beapoi;            /* Zeiger auf einen Baken-Eintrag       */
  int        i;
  MBHEAD    *mbp;
  struct tm *p;
  LNKBLK    *lp;
  PTCENT    *ptcp;
  int        telemetrie;

  p = localtime(&sys_time);

  /* fuer alle Ports, bei denen die Bake freigegeben ist.. */
  for (i = 0, beapoi = beacon; i < L2PNUM; ++beapoi, ++i)
  {
#ifdef BEACON_STATUS
  if ((telemetrie = beapoi->telemetrie) == 2)
#else
  if ((telemetrie = beapoi->telemetrie) >= 2)
#endif
    {
      (mbp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2fflg = L2CPID;
      lp = pp->nbrl2l;
      ptcp = ptctab + g_uid(lp, L2_USER);
      if (telemetrie != 2)       /* Format fuer automatische Auswertung */
      {
        mbp->l4time = mbp->mbpc;
        putid(pp->l2link->call, mbp);         /* Node Call              */
        putspa(10, mbp);
        putprintf(mbp, "%2d %02d.%02d.%02d %02d:%02d:%02d",
                  lp->liport,
                  p->tm_mday, p->tm_mon + 1, p->tm_year % 100,
                  p->tm_hour, p->tm_min, p->tm_sec);
        putprintf(mbp, " %7lu", rtt * 10L);

        if (pp->quality > 9999)
        {
          putprintf(mbp, " %2lumin", pp->quality / 6000);
        }
        else
          putprintf(mbp, " %5lu", pp->quality * 10L);

        if (pp->my_quality > 9999)
        {
          putprintf(mbp, " %2lumin", pp->my_quality / 6000);
        }
        else
          putprintf(mbp, " %5lu", pp->my_quality * 10L);

        if (pp->his_quality > 9999)
        {
          putprintf(mbp, " %2lumin", pp->his_quality / 6000);
        }
        else
          putprintf(mbp, " %5lu", pp->his_quality * 10L);

        putprintf(mbp, " %7lu %lu\r",
                  lp->SRTT * 10L,
                  ptcp->inforx + ptcp->infotx);
      }
      else
      {
        putprintf(mbp, "%02d.%02d.%02d %02d:%02d:%02d ",
                  p->tm_mday, p->tm_mon + 1, p->tm_year % 100,
                  p->tm_hour, p->tm_min, p->tm_sec);
        putid(pp->l2link->call, mbp);         /* Node Call              */
        putprintf(mbp, "(%02d)\rL3RTT=%lums L3SRTT=%lums (%lums/%lums) "
                       "L2SRTT=%lums SUM=%lu\r",
                  lp->liport,
                  rtt * 10L,
                  pp->quality * 10L,
                  pp->my_quality * 10L,
                  pp->his_quality * 10L,
                  lp->SRTT * 10L,
                  ptcp->inforx + ptcp->infotx);
      }
      rwndmb(mbp);
#ifdef __WIN32__
      sdui(beapoi->beadil, "METRIC\140", myid, (char)i, mbp);
#else
      sdui(beapoi->beadil, "METRIC\140", myid, i, mbp);
#endif /* WIN32 */
      dealmb(mbp);                   /* Telemetrie-Frame kann jetzt weg */
    } /* Telemetrie */
  } /* alle Ports */
}

/************************************************************************/
/*                                                                      */
/* L3RTT-Frames auswerten: Entweder sind es Messframes fuer Laufzeit-   */
/* messungen oder aber NODES-Meldungen bei ON5ZS-Routing                */
/*                                                                      */
/************************************************************************/
void
rx_l3rtt_frame(PEER *rxpp, MBHEAD *mbp, char huge *savmbbp, WORD savmbgc)
{
  char        rx_alias[L2IDLEN]; /* Ident des Nachbarn (wenn bekannt)   */
  char        ver[12];           /* Version des Nachbarn                */
  WORD        i,
              j;                /* Zaehler                              */
  char        buffer[150];      /* Speicher fuer RTT String-Operationen */
  char       *bp;
  char        version[4];       /* TNN-Version                          */
  ULONG       prev_tic10;       /* Zeitpunkt der RTT Messung in 10ms    */
  ULONG       prev_l3srtt;      /* letzter SRTT (smoothed RTT)          */
  ULONG       prev_l3rtt;       /* letzter gemessener RTT-Wert          */
  PEER       *rtt_pp;           /* Pointer auf Nachbarliste             */
  char        buf2[8];
  ULONG       rtt;
  INDEX       index;
  ULONG       maxtime;

  if (l4opco == L4INFTRA)               /* Info-Transfer                */
  {
    if (match(mbp, "BROAD"))            /* BROAD Kennung ?              */
    {
      if (cmpid(rxpp->l2link->call, orgnod) == TRUE)
        if (rxpp->typ == TNN)           /* kann der andere Info?        */
          rx_broadcast(rxpp, mbp);      /* dann auswerten               */
        dealmb(mbp);                    /* Buffer entsorgen             */
        return;
    }

    if (match(mbp, "L3RTT:"))           /* L3RTT Kennung ?              */
    {
      for (i = 0; ((i < 110) && (mbp->mbgc < mbp->mbpc)); i++)
        buffer[i] = getchr(mbp);

          /* zur Sicherheit */
          memset(rx_alias, 0, L2IDLEN);
      strcpy(&buffer[i], " 1 2 3 4 5 6 \n\0");

      sscanf(&buffer[0], "%lu %lu %lu %lu %6s %11s %6s",
             &prev_tic10,
             &prev_l3srtt,
             &prev_l3rtt,
             (unsigned long *)&rtt_pp,
             rx_alias,
             ver,
             buf2);
      *version = NUL;
      if (!strncmp(buf2, "TNN", 3))
        strcpy(version, &buf2[3]);

      for (i = j = 0; i < L2CALEN; i++)
        if (rx_alias[i] == 0x00 || j == 1)
        {
          rx_alias[i] = ' ';            /* mit Leerzeichen fuellen      */
          j = 1;
        }

/************************************************************************/
/* Eigenes Frame auswerten                                              */
/************************************************************************/
      if (cmpid(orgnod, myid))               /* selbst Absender         */
      {
        if ((rtt_pp == rxpp))                /* passender Nachbar       */
          if (   tic10 > prev_tic10          /* Uhr nicht uebergelaufen */
              && rxpp->rttstart == prev_tic10   /* richtige Messung     */
              && ((tic10 - prev_tic10) < 10000))/* nicht zu lang!       */
          {                                     /* gemessene L3-RTT     */
            rtt = (tic10 - prev_tic10) + 2;
            update_peer_quality(rxpp, rtt / 2, DONT_CHANGE_QUAL);

/* Falls das Timeout auf einer Route mal abfaellt, melden wir hier      */
/* wieder die Routen-Qualitaet hoch.                                    */
            if ((index = add_route(rxpp, rxpp->l2link->call, 1))
                       != NO_INDEX)
              update_lt(rxpp, index, 1);
            rtt_metric(rxpp, (long)rtt);
            rxpp->rttstart = 0L;
            rxpp->rtt_time = L3_RTT_TIME;

/* erste Bake ueberhaupt, dem Nachbarn wurde noch nicht die */
/* Laufzeit uebertragen, das machen wir jetzt gleich mit einer */
/* zweiten Bake. Dies fuehrt dazu, dass der Link sofort benutzbar */
/* wird. Den Nachbarn merken wir selber provisorisch mit schlechtester */
/* Laufzeit, bei der ersten regulaeren Bake wird das durch die echte */
/* Laufzeitmeldung korrigiert. Der provisorische Wert wird nicht */
/* an andere Nodes weiterverbreitet. */

            if ((prev_l3srtt == 0) && (prev_l3rtt == 0))
              send_l3srtt_frame(rxpp);

          }
        dealmb(mbp);
        return;
      } /* eigenes L3RTT-Frame */

/* Fremdes L3RTT-Frame auswerten, und an Nachbarn zurueck               */
      if (cmpid(rxpp->l2link->call, orgnod) == TRUE)
      {
        if (time_to_live >= 1)
        {                               /* noch Restlebenszeit..        */
          mbp->mbbp = savmbbp;
          mbp->mbgc = savmbgc;
          /* Frame sofort zurueck an Absender */
          toneig(rxpp, mbp);

          /* Frame auswerten */
          if (strnicmp("LEVEL3_V2.1", ver, 11) == 0)
          {
            if (rxpp->typ == NETROM)
              set_peer_typ(rxpp, TNN);
            rxpp->version = atoi(version);
            cpyals(rxpp->l2link->alias, rx_alias);
          }
          else
            cpyals(rx_alias, rxpp->l2link->alias);

          /* meldet der Nachbar INP-Faehigkeit ? */
          if (strstr(buffer, "$N"))
          {
#ifndef L4NOBAKE
            /* Partner ist noch nicht I und wir wollen auch einen I-Link */
            if ((rxpp->typ != INP) && (rxpp->soll_typ == INP))
#else
            /* Partner ist noch nicht I und wir wollen auch einen I-Link */
            if (  ((rxpp->typ != INP) && (rxpp->soll_typ == INP))
                ||(rxpp->soll_typ == NETROM))  /* Routing-Typ steht noch nicht*/
                                               /* fest, Nachbar will I, dann  */
                                               /* stellen wir auf INP um.     */
#endif /* L4NOBAKE */
            {
              set_peer_typ(rxpp, INP);      /* Typ aendern */
#ifdef L4NOBAKE
              rxpp->soll_typ = INP; /* Typ auf INP setzen. */
#endif /* L4NOBAKE */
              send_inp_nodebeacon(rxpp);    /* uns selber melden */
            }
          }

          /* MaxTime fuer INP-Nachbarn */
          if (rxpp->typ == INP)
          {
            /* Nachbar sagt uns eine MaxTime ? */
            if ((bp = strstr(buffer, "$M")) != NULL)
            {
              /* Ja, dann uebernehmen */
              sscanf(bp + 2, "%lu", &maxtime);
              rxpp->maxtime = (UWORD)maxtime;
            }
            else
              rxpp->maxtime = 0;    /* Nein, dann keine MaxTime */
          }

          if (prev_l3srtt >= 5L)
            update_peer_quality(rxpp, DONT_CHANGE_QUAL, prev_l3srtt);

          if ((index = add_route(rxpp, rxpp->l2link->call, 1)) != NO_INDEX)
          {
            update_lt(rxpp, index, 1);
            if (update_alias(index, rx_alias))
              propagate_node_update(index);
          }
          return;
        }
      }
    } /* L3RTT: */
  } /* L4-Info-Frame */
  dealmb(mbp);
}

/* End of src/l3rtt.c */
