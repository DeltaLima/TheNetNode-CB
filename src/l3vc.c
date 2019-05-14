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
/* File src/l3vc.c (maintained by: ???)                                 */
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

/* definitionen */
/* FlexNet 0er Frames */

/* BYTE 0              */
#define SOFTKENNUNG  0x23
/* Bit 0..2  Software  */
/* 000 Flex Net        */
/* 001 BayCom Node     */
/* 010 Digi Ware       */
/* 011 TheNetNode      */
/* 100 SNet            */
/* 101 XNet            */
/* 110 .. 111  reserve */
#define S_FLEXNET  0
#define S_BAYCOM   1
#define S_DIGIWARE 2
#define S_THENET   3
#define S_SNET     4
#define S_XNET     5
/* Bit 3  Virtuelle Adressierung, 0 = disable, 1 = enable  */
#define S_VFLAG    8
/* Bit 4  frei , 0     */
/* Bit 5  frei , 1     */
/* Bit 6  frei , 0     */
/* Bit 7  frei , 0     */

/* BYTE 1 Version      */
#define SOFTVERSION  0x11

#define TOKEN_ER        0
#define TOKEN_ER_REQ    1
#define TOKEN_ICH       2
#define TOKEN_ICH_REQ   3

#define LOWER   -1
#define HIGHER   1
#define SAME     0

/* SSID fuer FlexNet aufbereiten */
#define FLEXSSID(id) (((id[L2IDLEN-1] & 0x1E) >> 1) + '0')

/* Messzeit in Sekunden, hat beim FlexNet nur einen Sinn,
 * wenn noch kein Connect steht, ansonsten gibt diese
 * Zeit nur die Pausen zwischen zwei Local-Tests an
 */

static int      callcompare(char *, char *);
static void     flex_tx_rtt_query(PEER *pp);
static void     flex_tx_init(PEER *pp);
static void     flex_tx_routes(PEER *pp);
static void     flex_route_reply(char *);
static MBHEAD  *getfbp(UBYTE, LNKBLK *, const char *format, ...);
static void     sendfbp(MBHEAD *);
static void     flex_tx_rtt_reply(PEER *);

void
local_flex_srv(void)
{
  PEER   *pp;
  int     i;
  int     max_peers = netp->max_peers;

  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
    if (pp->used)
    {
      if (pp->typ == LOCAL_M)
      {
        if (pp->rttstart)               /* Messung unterwegs?           */
          continue;
        if (pp->nbrl2l)                 /* ... Leichen abwerfen         */
                {
                        /* Disconnect einleiten. */
                        discnbp(pp);
                        /* Deaktiv markieren. */
                        pp->nbrl2l = NULL;
                }
        else
          if (pp->rtt_time == 0)        /* Messzeit abgelaufen?         */
          {
            if (connbr(pp))
              pp->rttstart = tic10;     /* Start Zeitmessung            */
          }
          else
            pp->rtt_time--;
      }
      else
        if (pp->typ == FLEXNET)
        {
          if (pp->nbrl2l == NULL)       /* gibt es einen Link?          */
          {
#ifdef PORT_L2_CONNECT_TIME
          if (pp->l2link->sabmtime == 0)
          {
#endif
#ifdef PORT_L2_CONNECT_RETRY

            if (connbr(pp) == FALSE)        /* sonst neu connecten          */
             continue;

                        pp->nbrl2l->tries = (UBYTE)portpar[pp->nbrl2l->liport].retry - (UBYTE)portpar[pp->nbrl2l->liport].l2_connect_retry;
#else
            connbr(pp);                 /* sonst neu connecten          */
#endif
          }
#ifdef PORT_L2_CONNECT_TIME
                   else
                      pp->l2link->sabmtime--;
                  continue;
                  }
#endif
          if (pp->nbrl2l->state < L2SIXFER)
            continue;                           /* und ist er aktiv?    */
          if (pp->rttstart)                     /* Laufzeitmessung      */
          {                                     /* unterwegs?           */
#ifndef FLEXTIMEFIX
            if (tic10 - pp->rttstart >= 6000L)  /* maximal 60 Sekunden  */
#else
            if (tic10 - pp->rttstart >= 18000L)  /* maximal 180 Sekunden  */
#endif /* FLEXTIMEFIX */
              discnbp(pp);                      /* sonst abwerfen       */
            continue;
          }

          if (pp->rtt_time == 0)        /* Zeit fuer neue Messung       */
            flex_tx_rtt_query(pp);
          else
            pp->rtt_time--;             /* sonst weiterwarten           */

          if (pp->quality)
          {
            if (pp->brotim == 0)        /* Zeit fuer neue Meldung?      */
              flex_tx_routes(pp);       /* Meldungen uebertragen        */
            else
              pp->brotim--;
          }
        }
    }
}

/*----------------------------------------------------------------------*/
/* eine Status-Aenderung fuer die LOCALS (gemessene) behandeln          */
/*----------------------------------------------------------------------*/
void local_status(PEER *pp, WORD status)
{
  ULONG rtt;
  INDEX index;

  if (pp->typ == LOCAL_M) {
    switch (status) {
      case L2MCONNT :
      case L2MLRESF :
      case L2MLREST :
        discnbp(pp);
      case L2MBUSYF :
        if (pp->rttstart) {
          rtt = (tic10 - pp->rttstart) + 1;
          update_peer_quality(pp, rtt, DONT_CHANGE_QUAL);
          if ((index = add_route(pp, pp->l2link->call,
                       (unsigned)pp->quality)) != NO_INDEX) {
            update_lt(pp, index, 1);
            update_alias(index, pp->l2link->alias);
          }
          rtt_metric(pp, (long)rtt);
        }
        pp->rttstart = 0;
        pp->rtt_time = MESSTIME;
        break;
      case L2MFAILW :
        if (pp->rttstart) {
          update_peer_quality(pp, 0L, DONT_CHANGE_QUAL);
          add_route(pp, pp->l2link->call, 0);
        }
      case L2MDISCF :
        pp->rttstart = 0;
        pp->rtt_time = MESSTIME;
        break;
    }
  }
}

/*----------------------------------------------------------------------*/
/* eine Status-Aenderung fuer die FLEXNET behandeln                     */
/*----------------------------------------------------------------------*/
void
flex_status(PEER *pp, WORD status)
{
#ifdef PORT_L2_CONNECT_TIME
  UWORD port = 0;
#endif
  if (pp->typ == FLEXNET)
  {
    switch (status)
    {
      case L2MCONNT :
      case L2MLRESF :
      case L2MLREST :
        pp->token = (callcompare(pp->l2link->call, myid) == LOWER)?
                    TOKEN_ER:TOKEN_ICH;
        connect_peer(pp);
        flex_tx_init(pp);           /* Init-Frame schicken              */
#ifndef FLEX_TX_RTT_FIX
        flex_tx_rtt_query(pp);      /* und Laufzeitmessung              */
#endif
        pp->maxtime = 30000U;       /* maximale Laufzeit                */
        break;

      case L2MFAILW :
      case L2MDISCF :
#ifdef PORT_L2_CONNECT_TIME
        port = pp->l2link->port;
        pp->l2link->sabmtime = portpar[port].l2_connect_time;
#endif  /* PORT_L2_CONNECT_TIME */

        /* Route als ausgefallen melden. */
        update_peer_quality(pp, 0L, DONT_CHANGE_QUAL);
        /* Alle Nodes von der Route loeschen. */
        check_all_destot();

#ifdef AUTOROUTING
        /* Link ist eine Auto-Route. */
        if (pp->l2link->ppAuto == AUTO_ROUTE)
        {
#ifdef AXIPR_HTML
            /* Protokoll fuer HTML-Ausgabe setzen. */
            SetHTML(pp->l2link->port, pp->l2link->call, NULL, FALSE);
#endif /* AXIPR_HTML */

          /* Link austragen. */
          unregister_neigb(pp->l2link->call, pp->l2link->digil, pp->l2link->port);
          return;
        }

        update_peer_quality(pp, 0, DONT_CHANGE_QUAL);
#endif /* AUTOROUTING */
        break;

      default :
        update_peer_quality(pp, 0, DONT_CHANGE_QUAL);
        disconnect_peer(pp);
        break;
    }
#ifdef __WIN32__
    pp->brotim =   0;               /* mit Broadcast beginnen           */
    pp->rttstart = 0;               /* Messung zuruecksetzen            */
#else
    pp->brotim =                    /* mit Broadcast beginnen           */
    pp->rttstart =                  /* Messung zuruecksetzen            */
#endif /* WIN32 */
    pp->rtt_time = 0;
  }
}

/*
 * Ein Frame mit einer bestimmten PID anlegen und Verweis auf den
 * zugehoerigen Link speichern.
 */
static MBHEAD *getfbp(UBYTE pid, LNKBLK *lnkpoi, const char *format, ...)
{
  va_list  arg_ptr;
  char     str[256];
  MBHEAD  *fbp;

  va_start(arg_ptr, format);        /* String formatieren               */
  vsprintf(str, format, arg_ptr);
  va_end(arg_ptr);

  (fbp = (MBHEAD *) allocb(ALLOC_MBHEAD))->l2fflg = pid;
  fbp->l2link = lnkpoi;             /* zugehoerigen Link speichern      */
  putstr(str, fbp);                 /* Text in das Frame speichern      */
  return(fbp);
}

/*
 * Ein Frame an den Level 2 senden (muss mit getfbp erzeugt worden sein)
 */
static void sendfbp(MBHEAD *fbp)
{
  rwndmb(fbp);                      /* Frame zurueckspulen              */
  i3tolnk(fbp->l2fflg, fbp->l2link, fbp);
}

/*
 * Flexnet Initialisierungs-Frame senden, dort sind die maximale SSID und
 * diverse Kennungen enthalten
 */
static void
flex_tx_init(PEER *pp)
{
  sendfbp(getfbp(L2CFLEXNET, pp->nbrl2l,
          "0"                       /* Frame-Kennung                    */
          "%c"                      /* maximale SSID                    */
          "%c%c",                   /* Software-Name und Version        */
          (maxSSID() + '0'),        /* TEST DG9OBU : max. SSID melden   */
          SOFTKENNUNG,
          SOFTVERSION));
}

/*
 * Flexnet Laufzeitmessungs-Antwort senden, es enthaelt unsere eigene
 * Laufzeit*2 (Flexnet uebertraegt Hin-und-Rueck-Laufzeiten).
 */
static void
flex_tx_rtt_reply(PEER *pp)
{
  sendfbp(getfbp(L2CFLEXNET, pp->nbrl2l,
          "1"                       /* Frame-Kennung                    */
          "%lu",                    /* unsere Laufzeit                  */
          pp->my_quality ? pp->my_quality/10L : (ULONG)(pp->nbrl2l->SRTT/10L)));
}

/*
 * Flexnet Laufzeitmessungs-Frame senden
 */
static void
flex_tx_rtt_query(PEER *pp)
{
  MBHEAD *fbp;

  fbp = getfbp(L2CFLEXNET, pp->nbrl2l, "2");/* Frame-Kennung            */
  while (fbp->mbpc < 201)                   /* auf 201 Bytes auffuellen */
    putchr(' ', fbp);
  sendfbp(fbp);
  pp->rttstart = tic10;             /* Startzeit der Messung merken     */
}

/************************************************************************/
/*                                                                      */
/* Flexnet Routing-Informations-Frame senden, hier wird die Token-      */
/* uebergabe und die Sendung von Routing-Informationen durchgefuehrt.   */
/*                                                                      */
/************************************************************************/
static void
flex_tx_routes(PEER *pp)
{
  MBHEAD   *fbp = NULL;
  INDEX     index;
  int       max_nodes = netp->max_nodes;
  NODE     *np = netp->nodetab;
  ROUTE    *rp = pp->routes;
  PEER     *bestpp;
  UWORD     quality;
  unsigned  reported_quality;
  unsigned  diff;

  switch (pp->token)                /* je nach Token-Status             */
  {
    case TOKEN_ICH_REQ:             /* ich warte auf Token und es kommt */
      discnbp(pp);                  /* nicht                            */
#if MAX_TRACE_LEVEL > 0
      notify(1, "token lost %6.6s", pp->l2link->call);
#endif
      return;

    case TOKEN_ER:
    case TOKEN_ER_REQ:
    case TOKEN_ICH:
    /* Wir koennen mit der Sendung der Informationen beginnen, da wir das */
    /* Token haben.                                                       */
      for (index = 0; index < max_nodes; index++, np++, rp++)
      {
        /* Leere Eintraege uebergehen */
        if (!np->id[0])
          continue;
        /* Keine versteckten Ziele melden */
        if (np->alias[0] == '#')
          continue;
        /* Dem Nachbarn nicht sich selbst melden */
        if (cmpid(np->id, pp->l2link->call))
          continue;
        /* Lokale Links nicht melden, die sind schon in unserer SSID-  */
        /* Range mit beruecksichtigt. Locals, die nicht das Knotencall */
        /* haben, muessen aber gemeldet werden, da sie nicht von der   */
        /* SSID-Range erfasst werden. */
        if (cmpcal(myid, np->id) && SSIDinrange(SSID(np->id)))
          continue;

#ifdef LINKSMOD_LOCALMOD
        if (CheckLocalLink(np->id) == TRUE)      /* Ist ein LOCAL-Link. */
          continue;                           /* Zum naechsten Eintrag. */
#endif /* LINKSMOD_LOCALMOD */

        quality = (find_best_qual(index, &bestpp, FLEX_MASK) + 9) / 10;
        reported_quality = rp->reported_quality / 10; /* gemeldete Zeit */

        if (bestpp == pp)           /* Partner ist der beste Weg        */
        {
          if (!reported_quality)    /* war er schon der Downstream?     */
            continue;               /* dann keine Aenderung             */
/* Den Partner informieren, das er jetzt unser Upstream wird, indem wir */
/* 0 melden.                                                            */
          quality = 0;
        }
        else                    /* bester Weg geht wo anders            */
        {                       /* zu meldende Qualitaet berechnen      */
          if (quality)
            quality += (unsigned)(pp->quality / 10L);
          if (quality == reported_quality)      /* keine Aenderung      */
            continue;
          diff = (reported_quality / 8);        /* 12.5% Schwelle       */
          diff = max(diff, 2);                  /* minimum 200ms        */
          if (quality && reported_quality)      /* beide gut            */
            if (   (quality < reported_quality) /* nicht schlechter     */
                && (reported_quality-quality <= diff))
              continue;                      /* und unwesentlich besser */
        }

        if (pp->token == TOKEN_ER)            /* erst nach Token fragen */
        {
          sendfbp(getfbp(L2CFLEXNET, pp->nbrl2l, "3+"));
          pp->token = TOKEN_ICH_REQ;
          pp->brotim = 120;         /* falls er nicht in 120s antwortet */
          return;                   /* ohne Token nichts senden         */
        }

        if (fbp == NULL)            /* eventuell neuen Buffer anlegen   */
          fbp = getfbp(L2CFLEXNET, pp->nbrl2l, "3");
/* wenn ich dem Nachbarn etwas melde, ist er automatisch mein Upstream, */
/* seine Qualitaet verwerfe ich also. Dem Downstream melde ich nie      */
/* etwas (quality = 0), deshalb werfe ich seine Qualitaet nicht weg.    */
        if (quality)
          update_route(pp, index, 0);
/* Maxtime des Nachbarn beruecksichtigen                                */
        if (quality > pp->maxtime / 10L)
          continue;
        putprintf(fbp, "%6.6s%c%c%u ",
                  np->id,           /* Rufzeichen des Ziels             */
                  FLEXSSID(np->id), /* untere SSID                      */
                  np->ssid_high+'0',/* obere SSID                       */
                  quality);         /* Quality (in 100ms, siehe oben)   */
        rp->reported_quality = quality * 10; /* merken, was gemeldet    */
                                             /* wurde                   */
        if (fbp->mbpc >= 256 - (6 /*id*/
                                + 2 /*2 ssids*/
                                + 5 /*4 quality+space*/))
        {
          sendfbp(fbp);
          fbp = NULL;               /* Frame senden wenn Buffer voll    */
        }
      }
      if (pp->token == TOKEN_ER_REQ)   /* er will das Token             */
      {
        if (!fbp)
          fbp = getfbp(L2CFLEXNET, pp->nbrl2l, "3");
        putchr('-', fbp);           /* ihm das Token geben              */
        pp->token = TOKEN_ER;       /* nun hat er das Token             */
      }
      if (fbp)
        sendfbp(fbp);               /* eventuell Reste senden           */
      break;
  }
  pp->brotim = 10;                  /* in 10s wieder melden             */
}

/* Einen Buffer in einen String umkopieren. CR wird entfernt.           */
static void
mbp2str(MBHEAD *mbp, char *str)
{
  int ch;

  while (mbp->mbgc < mbp->mbpc)
    if ((ch = getchr(mbp)) != '\r')
      *str++ = ch;
  *str = 0;
}

/************************************************************************/
/*                                                                      */
/* Ein Routentest-Frame empfangen und weiterleiten / oder beantworten   */
/* Routentest weiterleiten Format: 6HQQQQQSSSSSS XXXXXX DDDDDD          */
/*   H TTL-Zaehler. Beginnt bei ! (ASCII $21) und zaehlt hinauf !"#$%&/ */
/*   Q QSO-Nummer   ist erstes Zeichen davon  = ' (96dez | 60hex) oder  */
/*                  entsprechendes Bit gesetzt, wird zusaetzlich ein    */
/*                  7er Frame erzeugt und zurueckgeschickt.             */
/*   S Source-Digi                                                      */
/*   X Digipeater ....                                                  */
/*   D Ziel-Digi                                                        */
/*                                                                      */
/************************************************************************/
void
flex_route_query(char *buf)
{
  char    *bp, *p;                  /* Buffer fuer die Bearbeitung      */
  char    dstid[L2IDLEN];           /* Zielrufzeichen                   */
  char    dstcal[15];               /* ASCII: Rufzeichen des Zielnode   */
  char    nbrcal[15];               /* ASCII: Rufzeichen des Nachbarn   */
  char    linkzeit[10];
  char   *insert;                   /* Das soll eingefuegt werden.      */
  PEER   *pp;
  MBHEAD *fbp;
  int     len;
  int     mask_flag = 0;

  buf[0] = '7';                     /* Default: Frame geht zurueck      */
  bp = buf + 7;

  while ((p = strchr(bp, ' ')) != NULL)
    bp = p + 1;                     /* zum letzten Call gehen           */

  if (strlen(bp) > 9)               /* XXXXXX-99 darf noch drinstehen   */
    return;
  strcpy(dstcal, bp);               /* auch als ASCII merken            */
  str2call(dstid, bp);              /* Zielrufzeichen lesen             */

  insert = "";                      /* Default: nichts einfuegen        */
  linkzeit[0] = NUL;

  if (++buf[1] < '!' + 20)          /* noch Hops uebrig?                */
  {
    if (iscall(dstid, NULL, &pp, FLEX_MASK))
    {
/* Wenn es der Linkpartner selber ist, dann schicken wir das einfach    */
/* zurueck, sonst basteln wir sein Call da rein und schicken ihm das    */
/* Frame.                                                               */
/* (durch iscall(..,VC|VC_FAR) ist sichergestellt, das wir hier nur     */
/* Flexnets haben, die ferne Ziele anbieten)                            */
      if (!cmpid(dstid, pp->l2link->call))              /* WEITERLEITEN */
      {
        call2str(insert = nbrcal, pp->l2link->call);
        strcat(nbrcal, " ");     /* Leerzeichen als Trennung            */
        buf[0] = '6';            /* doch weiterleiten                   */
      }                          /* nichts einfuegen, nur zuerueck      */
      mask_flag = pp->options;   /* merken was es war. VC | VC_FAR | DG */
    }
    else
      if (iscall(dstid, NULL, &pp, DG))
        insert = "<tnngate> ";
      else
       insert = "??? ";
  }
  else
    insert = "... ";                /* Hops abgelaufen                  */

  len = (int)(strlen(buf) + strlen(insert)) + 1/*CR*/;
  if (len >= 230)                   /* Frame wird zulang, kuerzen       */
  {
    insert = len < 250 ? ">>> " : "";
    buf[0] = '7';                   /* zulange an den Absender zurueck  */
  }
/* hier angekommen haben wir genuegend Platz, um insert einzufuegen,    */
/* oder insert ist leer.                                                */
  p = bp;
  strcpy(bp, insert);
  strcat(bp, dstcal);

  if (buf[0] == '6')                /* 6er leiten wir selber weiter     */
  {
    fbp = getfbp(L2CFLEXNET, pp->nbrl2l, "");
    putstr(buf, fbp);               /* String ins Frame                 */
    putchr('\r', fbp);              /* CR hinterher                     */
    sendfbp(fbp);                   /* und senden...                    */
  }
  if (buf[2] >= 0x60)               /* erweiterter Routentest ?         */
  {
    if (mask_flag & VC_FAR)         /* und auch ein Flex?               */
    {
      bp = p;
      sprintf(linkzeit,"(%lu) ", (unsigned)pp->quality / 10L);
      strcpy(bp, linkzeit);
      strcat(bp, insert);
      strcat(bp, dstcal);
      buf[0] = '7';
    }
  }
  if (buf[0] == '7')
    flex_route_reply(buf);          /* 7er erledigt flex_route_reply    */
  return;
}

/************************************************************************/
/*                                                                      */
/* Eine Routentest-Antwort auswerten oder weiterleiten.                 */
/* Routentest-Antwort Format: 7HQQQQQSSSSSS XXXXXX DDDDDD               */
/*                            01234567890123                            */
/*   H TTL-Zaehler. Beginnt bei ! (ASCII $21) und zaehlt hinauf !"#$%&/ */
/*   Q QSO-Nummer                                                       */
/*   S Source-Digi                                                      */
/*   X Digipeater ....                                                  */
/*   D Ziel-Digi                                                        */
/*                                                                      */
/* Wir waehlen einen beliebigen Rueckweg, nicht unbedingt den in der    */
/* Anfrage festgestellten (eventuell gehts zurueck anders).             */
/*                                                                      */
/************************************************************************/
static void
flex_route_reply(char *buf)
{
  char    *p;                       /* Buffer fuer die Bearbeitung      */
  char    srcid[L2IDLEN];           /* Zielrufzeichen                   */
  char    srccal[15];               /* ASCII: Rufzeichen des Zielnode   */
  PEER   *pp;
  MBHEAD *fbp;
  USRBLK *up;
  unsigned int uid;

  if ((p = strchr(buf+7, ' ')) == NULL)
    return;
  *p = 0;                           /* Rufzeichen isolieren             */
  if (strlen(buf + 7) > 9)          /* Absender-Rufzeichen zu lang      */
    return;
  strcpy(srccal, buf + 7);          /* und kopieren                     */
  str2call(srcid, srccal);          /* in internes Format wandeln       */
  *p = ' ';                         /* ... wieder Space daraus machen   */

  if (cmpid(srcid, myid))           /* etwa fuer mich?                  */
  {
    if(buf[2] > 0x60)               /* das ' entfernen                  */
      buf[2] = buf[2] & 0xbf;
    else
      buf[2] = 0x20;

    sscanf(buf + 2, "%5u", &uid); /* User-ID lesen                    */
    if (uid > 0 && uid < NUMPAT)  /* nur wenn die ID gueltig ist      */
      if ((up = ptctab[uid].ublk) != NULL)
      {
#ifdef SEND_ASYNC_RESFIX
        /* Sicher ist sicher. */
        buf[256] = 0;
#endif /* SEND_ASYNC_RESFIX */
        send_async_response(up, "Route (VC): ", buf + 7);
        return;
      }
  }
  else                                          /* WEITERLEITEN         */
    if (++buf[1] < '!' + 40)                    /* noch Hops uebrig?    */
      if (iscall(srcid, NULL, &pp, FLEX_MASK))
      {
        if (pp->nbrl2l)
        {
          fbp = getfbp(L2CFLEXNET, pp->nbrl2l, "");
          putstr(buf, fbp);                     /* String ins Frame     */
          putchr('\r', fbp);                    /* CR hinterher         */
          sendfbp(fbp);                         /* und senden...        */
        }
      }
}

/************************************************************************/
/*                                                                      */
/* Auswertung eines empfangenen Flexnet-Frames                          */
/*                                                                      */
/************************************************************************/
void
flex_rx(PEER *pp, MBHEAD * mbp)
{
  char       call[L2IDLEN];
  int        ssidoben;
  long       myrtt;
  long       hisrtt;
  unsigned   quality;
  UBYTE      data;
  char       buf[280];
  INDEX      index;
#if MAX_TRACE_LEVEL > 6
  char       notify_call[10];
#endif

  if (pp->nbrl2l == NULL)
    return;

  if (mbp->l2link)                  /* Info Frame ?                      */
  {
    switch ((buf[0] = getchr(mbp)))
    {
      case '0':
        if (mbp->mbpc - mbp->mbgc < 3)
          return;                   /* Minimum 3 Bytes im Frame          */
        pp->l2link->ssid_high =
          getchr(mbp)-'0';          /* Get max SSID                      */
        data = getchr(mbp);         /* Version lesen                     */
        pp->version = data +        /* Versionskennung zusammenbauen     */
          (getchr(mbp)<<8);         /* Softwareversion dazuaddieren      */
        break;

/* Beantwortung unseres 2er Frames                                      */
      case '1':
        myrtt  = (tic10 - pp->rttstart);
        myrtt += (myrtt / 8 + 1);       /* ca. 12,5% Aufschlag          */
        myrtt  = max(myrtt, 10);
        mbp2str(mbp, buf);              /* Frame in ein String kopieren */
        hisrtt = atoi((char *) buf) * 10;/* Flex gibt in 100ms an       */
        hisrtt = max(hisrtt, 10);  /* er sendetet RTT, wir wollen Delay */
        update_peer_quality(pp, (ULONG)myrtt, (ULONG)hisrtt);
        if ((index = add_route(pp, pp->l2link->call,
                               (unsigned)pp->quality)) != NO_INDEX)
        {
          update_lt(pp, index, 1);
          update_alias(index, pp->l2link->alias);
          update_ssid(index, pp->l2link->ssid_high);
        }
        rtt_metric(pp, myrtt);
        pp->rttstart = 0;

/* festes Messinterval                                                  */
        pp->rtt_time = MESSTIME;
        break;

/* Partner schickt Testframe, Antwort mit unserer Laufzeit              */
      case '2':
        flex_tx_rtt_reply(pp);      /* Antwort schicken                 */
        break;

/* Routinginfo / Tokenuebergabe                                         */
      case '3':
        while (mbp->mbgc < mbp->mbpc)
        {
          if (ge6chr(call, mbp))    /* Rufzeichen komplett?             */
          {
            if (mbp->mbpc-mbp->mbgc < 2/*ssid*/+1/*Zeit*/)
              return;
            call[L2IDLEN-1] = ((getchr(mbp)-'0') << 1) | 0x60;
            ssidoben = getchr(mbp) - '0';
            quality = 0;
            while (isdigit(data = getchr(mbp)) && mbp->mbgc < mbp->mbpc)
              quality = 10 * quality + (data - '0');
            if (quality >= 6000)
              quality = 0;
            if (quality)
              quality += quality / 5; /* 20% Aufschlag als Hop-Horizont */
            if ((index = add_route(pp, call, quality * 10)) != NO_INDEX)
            {
              update_lt(pp, index, DEFAULT_LT);
              update_ssid(index, ssidoben);
            }
          }
          else
          {
            switch (call[0])
            {
              case '+' :            /* er will das Token haben          */
                pp->token = TOKEN_ER_REQ;
                flex_tx_routes(pp); /* Routen uebertragen, Token halten */
                break;
              case '-' :            /* er gibt uns das Token            */
                pp->token = TOKEN_ICH;
                flex_tx_routes(pp); /* Routen uebertragen, Token halten */
                break;
            }
            return;
          }
        }
        break;

/* 4xxxx  sagt die maximal erlaubte Ziel-Laufzeit fuer Broadcast.       */
/*        Wenn sie kleiner wird, muessen groessere ausgetragen werden.  */
/* 4xxxx! Flex hat irgendwie seine Dest's zermarmelt. Alles neu melden. */
      case '4':                    /* Choke fuer die Laufzeit der Ziele */
        mbp2str(mbp, buf);
#ifdef __WIN32__
        pp->maxtime = (unsigned short)atoi ((char *) buf) * (unsigned short)10L;
#else
        pp->maxtime = atoi ((char *) buf) * 10L;
#endif /* WIN32 */
#if MAX_TRACE_LEVEL > 6
        call2str(notify_call, pp->l2link->call);
        notify(7, "%s maxtime -> %u0 ms", notify_call, pp->maxtime);
#endif
        if (strchr(buf, '!') != 0)   /* Nachbar hat Uebersicht verloren,*/
          flex_tx_routes(pp);        /* will Dest's                     */
        break;

      case '5':
        break;

/* Routentest hin, weiterleiten                                         */
      case '6':
        mbp2str(mbp, buf + 1);          /* Frame in ein String kopieren */
        flex_route_query(buf);
        break;

/* Routentest zurueck, weiterleiten                                     */
      case '7':
        mbp2str(mbp, buf + 1);          /* Frame in ein String kopieren */
        flex_route_reply(buf);
        break;

      default:
        break;
    } /* switch       */
  } /* if infoframe */
}

static int
callcompare(char * call1, char * call2)
{
  register int i;

  /* Rufzeichen zeichenweise miteinander vergleichen */
  for (i = 0; i < L2IDLEN - 1; ++i)
  {
    if (call1[i] != call2[i])
    {
      if (call1[i] < call2[i])
        return(LOWER);
      if (call1[i] > call2[i])
        return(HIGHER);
    }
  }

  /* Rufzeichen waren gleich, dann muss die SSID entscheiden */
  if ((call1[L2IDLEN-1] & 0x1e) > (call2[L2IDLEN-1] & 0x1e))
    return(HIGHER);
  else
    return(LOWER);
}

/************************************************************************\
*                                                                        *
* Informationstransfer von Level2 zum FlexNet-Router                     *
* Fuer jedes empfangene Packet wird die Flexnet-Behandlungsroutine       *
* aus src/l3c.c aufgerufen.                                              *
*                                                                        *
\************************************************************************/
void
flexnet_rx(PEER *pp)
{
  MBHEAD *mbp;

  /* Zunaechst empfangene Frames verarbeiten, bis Liste leer...         */
  while (   (mbp = (MBHEAD *)(pp->rxfl.head))
         != (MBHEAD *)&(pp->rxfl))
  {
    ulink((LEHEAD *)mbp);           /* Packet aus der Liste aushaengen  */
    flex_rx(pp, mbp);               /* Auswertung durchfuehren          */
    dealmb(mbp);                    /* fertig, Buffer freigeben         */
  }
}

/* End of src/l3vc.c */
