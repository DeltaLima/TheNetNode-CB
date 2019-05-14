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
/* File src/l7showl3.c (maintained by: ???)                             */
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

static void     putnod(NODE *, int, MBHEAD *);
static void     show_nodes(const char *, int);
static WORD     nodprm(char *, WORD, char *, UWORD *, char *);
static void     dump_options(NODE *, MBHEAD *);
static void     putquality(MBHEAD *, ULONG, int);
static void     putrou(PEER *, MBHEAD *, BOOLEAN);

static void
dump_options(NODE *np, MBHEAD *mbp)
{
  MBHEAD *op;
  int     len,
          i;
  UBYTE   tag;
  char    buf[256];

  if (np->options == NULL)
    return;
  op = np->options;
  rwndmb(op);
  putstr("\rINP-Options:", mbp);
  while (op->mbpc > op->mbgc)
  {
    len = getchr(op);
    tag = getchr(op);
    for (i = 0; i < len - 2; i++)
      buf[i] = getchr(op);
    buf[i] = NUL;
    putprintf(mbp, "\rlen = %d, tag = 0x%02x\r", len, tag);
    for (i = 0; i < len - 2; i++)
      putprintf(mbp, "%02x%c", buf[i], ((i % 16) == 0) ? CR : ' ');
  }
  putchr(CR, mbp);
}

/* Rufzeichen eines Node ausgeben, wahlweise Alias und SSID-Bereich     */
static void
putnod(NODE *np, int options, MBHEAD *mbp)
{
  char id[L2IDLEN];
  int  i;

  cpyid(id, np->id);
  if (np->options != NULL)
  {
    for(i = 0; i < L2CALEN; i++)
      id[i] = tolower(id[i]);
  }
  if (!(options & OPT_SSID_RANGE))      /* Anzeige als Node             */
  {
    if (*np->alias != ' ')              /* Alias ist vorhanden, dann    */
      options |= OPT_ALIAS;             /* auch anzeigen                */
  }
  if (options & OPT_ALIAS)
  {
    putide(np->alias, mbp);
    putchr(':', mbp);              /* ":" als Trennung zum Rufzeichen   */
  }
  if (options & OPT_SSID_RANGE)
  {
   putprintf(mbp, "%6.6s %2d-%-2d", id, SSID(np->id), np->ssid_high);
   }
  else
    putid(id, mbp);
}

/*----------------------------------------------------------------------*/
/*  NODES                                                               */
/*  -----------                                                         */
/*  clipoi zeigt auf das naechste Zeichen der Komandozeile.             */
/*  Wenn clicnt == 0 ist diese zu ende.                                 */
/*  nodprm() operiert nur auf einer KOPIE von clicnt/clipoi !           */
/*----------------------------------------------------------------------*/
static void
show_nodes(const char *name,     /* Name (Nodes,Destinations,Locals)    */
           int options)          /* Optionen fuer die Ausgabe           */
{
  char      newcal[L2IDLEN];     /* neues Call                          */
  char      niden[L2CALEN];      /* neuer Alias                         */
  char     *cpoisa;              /* temp fuer clipoi                    */
  BOOLEAN   alles;               /* versteckte Nodes auch zeigen        */
  BOOLEAN   plus_quality;        /* mit Qualitaet etc. zeigen           */
  TRILLIAN  callok;              /* Ergebnis Test auf gueltiges Call    */
  TRILLIAN  isnode;              /* Ergebnis Test auf gueltigen Alias   */
  WORD      ccntsa;              /* temp fuer clicnt                    */
  MBHEAD   *mbp;                 /* Buffer fuer Meldung an User         */
  WORD      is_mask_or_qual;     /* Qualitaet / Wildcards angegeben     */
  char      mask[MAXMASK];
  UWORD     qual;
  char      nbrcal[L2IDLEN];
  int       j;
  INDEX     index;
  int       max_peers = netp->max_peers;
  unsigned  route_quality;
  unsigned  quality = 0;
  PEER     *pp;
  PEER     *bestpp;
  NODE     *np;
  ROUTE    *rp;
  int       width;
  int       len;
  BOOLEAN   all_routes;
  char      buf[40];
  char      call1[15];
  char      call2[15];
  int       maske = OPTIONS_MASK;

  if (!ismemr())                 /* Nicht genuegend Buffer !!!          */
    return;

  alles =                        /* default: keine versteckten Nodes    */
    plus_quality = FALSE;        /* default: ohne Qualitaet etc.        */

/* Wenn Befehlszeile leer, Qualitaet nicht beruecksichtigen; bei Aufruf */
/* mit Parametern, pruefen, ob Qualitaet / Wildcards angegeben          */

  is_mask_or_qual = clicnt ? nodprm(clipoi, clicnt, mask, &qual, nbrcal) : 0;

  if ((!is_mask_or_qual) && clicnt)         /* nur 1 Node gefragt       */
  {
    cpoisa = clipoi;                        /* Befehlszeile merken      */
    ccntsa = clicnt;
    if (options & OPT_ALIAS)
    {
      isnode = getide(&clicnt, &clipoi, niden);  /* Test auf gueltigen  */
                                                 /* Alias               */
      clipoi = cpoisa;                       /* Befehlszeile zurueck    */
      clicnt = ccntsa;
    }
    else
      isnode = NO;
    callok = getcal(&clicnt, &clipoi, TRUE, newcal);  /* Rufzeichen?   */

    if (callok == YES || isnode != ERRORS) /* einzelner Eintrag gefragt */
    {
      if (   !(options & OPT_ALIAS)
          || !(isnode)
          || ((index = find_alias(niden)) == NO_INDEX))
      {

/* Bei Nodes gibt es nur eine SSID. Nur Flex kann mehrere haben. Darum   */
/* erstmal auf genaues Ziel testen.                                      */
/* Wenn das genaue Ziel nicht gefunden wurde, koennen wir noch nach dem  */
/* SSID-Bereich ueber Flexnet-Nachbarn suchen                            */

        if (callok == YES)
        {
          if ((index = find_node_this_ssid(newcal)) == NO_INDEX)
          {
/* Da das genaue Ziel nicht gefunden wurde, koennen wir noch nach dem   */
/* SSID-Bereich ueber Flexnet-Nachbarn suchen                           */
            if ((index = find_node_ssid_range(newcal)) != NO_INDEX)
            {
/* Wir haben einen Flexnet-Weg gefunden - dann duerfen wir nachher auch */
/* nur einen Flexnet-Weg anzeigen                                       */
                                maske = VC | VC_FAR;
            }
          }
        }
        else
          index = NO_INDEX;
      }

      if (index != NO_INDEX)
      {
        np = netp->nodetab + index;
        find_best_qual(index, &bestpp, options & maske);

#ifdef NONODESFIX
        if (bestpp == NULL)
        {
          mbp = putals("No entry for: ");

          putid(newcal, mbp);
          putstr("\r", mbp);
          prompt(mbp);
          seteom(mbp);
          return;
        }
#endif /* NONODESFIX */

        mbp = getmbp();             /* Buffer holen fuer Antwort        */
        putstr("Routes to ", mbp);

        if (!(maske & DG))              /* Flex-Ziel ueber SSID-Bereich */
          putid(newcal, mbp);
        else
        {
          putnod(np, (options & ~OPT_ALIAS), mbp);
/* bei "n alias" wird kein Call uebergeben                              */
          cpyid(newcal,np->id);
          if (np->ipa != 0L)
            putprintf(mbp, " (%u.%u.%u.%u/%u)",
                           (unsigned)(np->ipa >> 24),
                           (unsigned)((np->ipa >> 16) & 0xff),
                           (unsigned)((np->ipa >> 8) & 0xff),
                           (unsigned)(np->ipa & 0xff),
                           (unsigned)np->bits);
        }
        putstr("\r---T[ms]----RxT----TxT--LT-Mode-Obc-----"
               "RTT-Po-Route------------------------", mbp);

        all_routes = (strchr((char *)clipoi, '*') != NULL);

        for (pp = netp->peertab, j = 0;       /* alle Wege anzeigen     */
             j < max_peers; j++, pp++)
        {
          if (!pp->used)
            continue;
          if (!(pp->options & (options & OPTIONS_MASK)))
            continue;
          if (!(maske & DG))
            if (pp->typ != FLEXNET)
              continue;

          mbp->l4time = mbp->mbpc;

          rp = pp->routes + index;
          route_quality = rp->quality;
          quality = getquality(route_quality, pp);

          if (!quality && !all_routes)
            continue;

          if (pp->typ == FLEXNET)
            putstr(rp->reported_quality ? "\r- " : "\r> ", mbp);
          else
            putstr(quality ? pp == bestpp ? "\r> " : "\r  " : "\r- ", mbp);

          putspa(2, mbp);
          putprintf(mbp, "%6lu %6lu ",
                    ((ULONG)quality) * 10L,
                    ((ULONG)route_quality) * 10L);
          if (rp->reported_quality != DIRTY)
            putprintf(mbp, "%6lu ",
                      ((ULONG)rp->reported_quality) * 10L);
          else
            putstr("UPDATE ", mbp);
          putprintf(mbp, "%3u %4s %3u %7lu %2u",
                    (int)rp->lt,
                    pp->typ <= NETROM ? "DG" : "VC",
                    rp->timeout,
                    ((ULONG)pp->quality * 10),
                    pp->l2link->port);

          putspa(48, mbp);
          putid(pp->l2link->call, mbp);         /* Nachbarcall fuer Weg */
          putdil(pp->l2link->digil, mbp);       /* Digiweg zum Nachbarn */
        }
        putchr('\r', mbp);            /* Antwort abschliessen           */
        dump_options(np, mbp);
        call2str(call1, myid);
        if (quality && bestpp->typ >= LOCAL)
        {
          putchr('\r', mbp);
          putalt(alias, mbp);
          putid(myid, mbp);
          putprintf(mbp, "> %s <local>\r\r", call1);
        }
        prompt(mbp);
        seteom(mbp);                  /* Antwort abschicken             */

/* Routentest abhaengig vom Typ des besten Weges (LOCAL / LOCAL_M wurde */
/* schon vorher behandelt)                                              */
        if (bestpp->typ <= NETROM)
          request_nrr(np->id, userpo->uid);
        else
          if (bestpp->typ == FLEXNET)
          {
            call2str(call2, newcal);            /* das gesuchte Call    */
            sprintf(buf, "6!%5u%s %s", userpo->uid, call1, call2);
            if (clipoi[clicnt-1] == '>')/* Routentest um Laufzeitangabe */
              buf[2] = buf[2] | 0x60;   /* der Einzellinks erweitert    */
            flex_route_query(buf);
          }
        return;                       /* fertig                         */
      }
      else
      {
        mbp = putals("No entry for: ");
        strupr((char *)cpoisa);
        putstr((char *)cpoisa, mbp);
        putchr('\r', mbp);
        prompt(mbp);
        seteom(mbp);
        return;
      }
    }
  }

/* ========================== Nodestabelle anzeigen =================== */

  mbp = putals(name);               /* Kopfzeile                        */
  putnum(netp->num_nodes, mbp);     /* Zahl Nodeseintraege anzeigen     */
  putchr('/', mbp);                 /* Maximalanzahl ausgeben           */
  putnum(num_nodes_max, mbp);
  putstr("):\r", mbp);              /* Zeile abschliessen               */

  width = 0;

  if (is_mask_or_qual)              /* wenn Wildcards, alle untersuchen */
    alles = plus_quality = TRUE;

  for (np = (NODE *)netp->nodelis.head;
       np != (NODE *)&netp->nodelis;/* sortierte Nodes-Liste durchgehen */
       np = np->next)
  {
    index = (INDEX)(np - netp->nodetab);        /* Index berechnen      */
    if ((np->alias[0] != '#') || (alles == TRUE))
    {                        /* Eintrag kein #-Node oder alles anzeigen */
      quality = find_best_qual(index, &bestpp, options & OPTIONS_MASK);
      if (bestpp == NULL)
        continue;
      rp = bestpp->routes + index;
      if (is_mask_or_qual)              /* nur eine Auswahl bitte ...   */
      {

/* wird nach Routes eines bestimmten Nachbarn gesucht?                  */

        if (   (is_mask_or_qual & ISNBRCALL)
            && (!cmpid(bestpp->l2link->call, nbrcal)))
          continue;

        if (is_mask_or_qual & ISCALLMASK)        /* Call mit Wildcards? */
        {

/* wenn Call mit Wildcards nicht passt und nicht Alias mit Wildcards    */
/* gefragt wurde oder auch nicht passt, naechsten Eintrag testen        */

          if (   !c6mtch(np->id, mask)
              && (   !(is_mask_or_qual & ISIDENTMASK)
                  || !c6mtch(np->alias, mask)))
            continue;
        }

/* wenn Alias mit Wildcards gefragt, aber nicht passt, naechsten        */
/* Eintrag                                                              */

        else if (   (is_mask_or_qual & ISIDENTMASK)
                 && !c6mtch(np->alias, mask))
          continue;

        if (quality > 0)
        {                                      /* aktiver Weg zum Node? */

/* wenn untere Qualitaetsgrenze angegeben, aber die Qualitaet fuer den  */
/* aktiven Weg zu gering ist, naechsten Eintrag testen                  */

          if (   (is_mask_or_qual & ISMINQUAL)
              && (quality < qual))
            continue;

/* wenn obere Qualitaetsgrenze angegeben, aber die Qualitaet fuer den   */
/* aktiven Weg zu hoch ist, naechsten Eintrag testen                    */

          if (   (is_mask_or_qual & ISMAXQUAL)
              && (quality > qual))
            continue;
        }
      }
      else if (quality == 0)
        continue;
    }

#ifdef LINKSMOD_LOCALMOD
    if (CheckLocalLink(np->id) == TRUE)          /* Ist ein LOCAL-Link. */
      continue;                               /* Zum naechsten Eintrag. */
#endif /* LINKSMOD_LOCALMOD */

    mbp->l4time = mbp->mbpc;         /* Zaehler merken fuer putspa()    */
    putnod(np, options, mbp);
    if (options & OPT_SSID_RANGE)
      putspa(12, mbp);
    else
    {
      if (options & OPT_ALIAS)
        putspa(16, mbp);
      else
        putspa(9, mbp);
    }

    if (plus_quality == TRUE)        /* Qualitaet etc. auch anzeigen?   */
    {
      if (quality > 0)
      {                              /* wenn aktiver, diesen anzeigen   */
        if (quality > 6000)          /* mehr als eine Minute?           */
          putprintf(mbp, " %2umin", quality / 6000);
        else
          putprintf(mbp, " %5lu", ((ULONG)quality) * 10L);
        if (bestpp)
        {
          putchr('/', mbp);
          putprintf(mbp, "%2d ", bestpp->l2link->port);
        }
        else
          putstr("    ", mbp);
      }
      else                           /* kein aktiver Weg                */
        putstr("     -/ - ", mbp);
    }

    len = (mbp->mbpc - mbp->l4time); /* Laenge eines Eintrages          */
    width += len;
    if (width + len < 79)
    {                                /* einer passt noch in die Zeile   */
      len = (79 % len) / (79 / len);
      while (len--)
        putchr(' ', mbp);
    }
    else
    {
      putchr('\r', mbp);
      width = 0;
    }
  }
  putchr('\r', mbp);             /* Antwort abschliessen                */
  prompt(mbp);
  seteom(mbp);                   /* und abschicken                      */
}

/*----------------------------------------------------------------------*/
/* nodprm() - Parameter des erweiterten NODE Befehls auswerten          */
/* testet ob p ein Wort mit Wildcard enthaelt, kopiert dieses nach mp   */
/* Case-Conversion! Nod*E wird NOD*E                                    */
/* n MUSS > 0 sein                                                      */
/*----------------------------------------------------------------------*/
static WORD
nodprm(char *p,                        /* parameter des node befehls    */
       WORD n,                         /* restlaenge der parameterzeile */
       char  *mp,                      /* nimmt die maske auf           */
       UWORD *qp,                      /* nimmt die quality auf         */
       char *nc)                      /* nimmt das nachbarcall auf     */
{
  WORD i;
  WORD ret = ISMINQUAL;
  WORD matchok;
  WORD c;

  if (*p == '-')
  {
    n--;
    p++;
    ret = ISMAXQUAL;
  }
  if ((*qp = (UWORD)(nxtlong(&n, &p) / 10L)) == 0)
  {
    ret = 0;
  }

  if (*p == '<')
  {
    n--;
    p++;
    if (getcal(&n, &p, TRUE, nc) == YES)
      ret |= ISNBRCALL;
  }

  if (skipsp(&n, &p))
  {
    matchok = FALSE;
    for (i = 0; i < (MAXMASK - 1);)
    {
      if (!n || ((c = *p++) == ' '))
        break;
      n--;
      if (c == ':')
      {
        if (i == 0)
        {
          ret |= ISCALLMASK;
          continue;
        }
        else
        {
          ret &= ~ISCALLMASK;
          ret |= ISIDENTMASK;
          break;
        }
      }
      if ((c == MATCHMANY) || (c == MATCHONE))
      {
        matchok = TRUE;
      }
      mp[i++] = isascii(c) ? toupper(c) : MATCHONE;
    }
    mp[i] = MATCHEND;
    if (matchok)
    {
      if (!(ret & (ISCALLMASK | ISIDENTMASK)))
      {
        ret |= (ISCALLMASK | ISIDENTMASK);
      }
    }
    else
    {
      ret &= ~(ISCALLMASK | ISIDENTMASK);
    }
  }
  return (ret);
}

/*----------------------------------------------------------------------*/
void
ccpnod(void)                     /* Nodes ausgeben                      */
{
#ifndef SHOW_DESTNODES
  show_nodes("Nodes (", OPT_ALIAS | OPT_DGTEST | VC | VC_FAR | DG);
#else
  show_nodes("Nodes (", OPT_ALIAS | OPT_DGTEST | VC | DG);
#endif /* SHOW_DESTNODES */
}

/*----------------------------------------------------------------------*/
void
ccpdest(void)                    /* Destinations ausgeben               */
{
#ifndef SHOW_DESTNODES
        show_nodes("Destinations (", OPT_SSID_RANGE | OPT_VCTEST | VC | VC_FAR | DG);
#else
        show_nodes("Destinations (", OPT_SSID_RANGE | OPT_VCTEST | VC | VC_FAR);
#endif /* SHOW_DESTNODES */
}

/* Die Antwort wird hier etwas leserlicher gemacht und an den User      */
/* geschickt                                                            */
static const char *reason[] =
{"no route", "local", "flexgate", "", "loop"};

void
nrr2usr(NRRLIST *l, char time_to_live)
{
  char    buffer[512],
         *bp,
          call[10];
  int     llt,
          lt,
          err;
  UID     uid;
  USRBLK *up;

  bp = buffer;
  llt = l->lt & LT_MASK;

  while (*l->id)
  {                                         /* Liste abarbeiten         */
    *bp++ = ' ';
    lt = l->lt & LT_MASK;
    while (llt > lt)
    {                                       /* fehlende Digis markieren */
      *bp++ = '?';
      *bp++ = ' ';
      llt--;
    }
    call2str(call, l->id);
    bp += sprintf(bp, "%s", call);
    if (l->lt & ECHO_FLAG)
    {                                       /* Echoflag anzeigen        */
      *bp++ = '*';
      err = l->id[L2CALEN] >> 5;            /* Fehlercode extrahieren   */
      if (err < 5 && err != 3)              /* und anzeigen             */
        bp += sprintf(bp, "<%s>", reason[err]);
    }
    llt--;
    l++;
  }
  *bp++ = ' ';
  time_to_live++;
  while (llt > time_to_live)
  {
    *bp++ = '?';
    *bp++ = ' ';
    llt--;
  }
  call2str(call, myid);
  bp += sprintf(bp, "%s", call);

  uid = (l4hdr2 << 8) | l4hdr3;                /* User suchen           */
  if (uid > 0 && uid < NUMPAT)
    if ((up = ptctab[uid].ublk) != NULL)
#ifndef SEND_ASYNC_RESFIX
      send_async_response(up, "Route (DG):", buffer);
#else
    {
      /* Sicher ist sicher. */
      buffer[256] = 0;
      send_async_response(up, "Route (DG):", buffer);
    }
#endif /* SEND_ASYNC_RESFIX. */

}

/*
 * Qualitaet links oder rechtsbuendig ausgeben
 */
static void putquality(MBHEAD *mbp, ULONG qual, int align)
{
#define QA_LEFT  0
#define QA_RIGHT 1
  if (align == QA_LEFT) {
#ifndef ROUTESMOD_L3RTTSHOW
    if (qual) putprintf(mbp, "%-6ld ", qual*10L);
#else
    if (qual) putprintf(mbp, "%-6ld ", qual/10L);
#endif /* ROUTESMOD_L3RTTSHOW */
    else putstr("----   ", mbp);
  } else {
#ifndef ROUTESMOD_L3RTTSHOW
    if (qual) putprintf(mbp, " %6ld", qual*10L);
#else
    if (qual) putprintf(mbp, " %6ld", qual/10L);
#endif /* ROUTESMOD_L3RTTSHOW */
    else putstr("   ----", mbp);
  }
}

/**************************************************************************/
/* Anzeigen des Routing und der Links                                     */
/*------------------------------------------------------------------------*/
static void putrou(PEER *pp, MBHEAD *mbp, BOOLEAN printver)
{
  const char *c_state[] = {"      ", "setup ", "conn. ", "active"};
  int   state;
#define S_UNUSED 0
#define S_SETUP  1
#define S_CONN   2
#define S_ACTIVE 3
  int   typ = pp->typ;
  char *c;
  const char *cp;
  NODE *np;
  PEER *bestpp;
  INDEX index;
  unsigned int numroutes = 0;
  unsigned int qual;

#ifdef LINKSMOD_LOCALMOD
   if (  (pp->typ == LOCAL_V)     /* Segment ist eine versteckte LOCAL-Route. */
       &&(!issyso()))                                          /* Kein Sysop. */
     return;                                        /* Eintrag geheim halten. */
#endif /* LINKSMOD_LOCALMOD */

  mbp->l4time = mbp->mbpc;
  putid(pp->l2link->call, mbp);        /* Node Call ausgeben              */
  if (pp->primary != pp)
    putchr('*', mbp);
  putspa(10, mbp);                     /* Qua-Po-Dst Werte ausgeben       */

  for (np = (NODE *)netp->nodelis.head;
       np != (NODE *)&netp->nodelis;/* sortierte Nodes-Liste durchgehen */
       np = np->next)
  {
    if (np->id[0])              /* nur benutzte Eintraege interessieren */
    {
      index = (INDEX)(np - netp->nodetab);      /* Index berechnen      */
      qual = find_best_qual(index, &bestpp, OPTIONS_MASK); /* suche besten Weg */

      if ((bestpp == NULL) || (qual == 0)) /* geloeschte Nodes abfangen */
        continue;

      if (cmpid(bestpp->l2link->call, pp->l2link->call))  /* Callvergleich */
        ++numroutes;
    }
  }

  /* TEST DG9OBU */
  /* SSID bzw. SSID-Bereich ausgeben */
  if (pp->typ == FLEXNET)
  {
    putprintf(mbp, "%2d-%-2d", SSID(pp->l2link->call), pp->l2link->ssid_high);
  }
  else
  {
    putspa(11, mbp);
    putprintf(mbp, "%2d", SSID(pp->l2link->call));
  }

  putspa(17, mbp);

  putprintf(mbp, "%2.2s %2d %4d/%-4d", typtbl + pp->typ*2,
                                      pp->l2link->port,
                                      pp->num_routes,
                                      numroutes);

  if (pp->nbrl2l != NULL) {
    if (pp->nbrl2l->state < L2SIXFER)
      state = S_SETUP; /* setup */
    else
      state = S_CONN; /* connected */
  } else {
    if (pp->typ == LOCAL_M) {
      if (pp->quality > 0)
        state = S_ACTIVE;   /* active */
      else
        state = S_UNUSED; /* unused */
    } else
      state = S_UNUSED; /* nicht connected */
  }

#ifndef LINKSMOD_LOCALMOD
  if (typ != LOCAL)
#else
  if (typ < LOCAL)
#endif /* LINKSMOD_LOCALMOD */
  {
      if (typ == THENET && pp->quality != 0)
          putprintf(mbp,"   Qual:%d",pp->quality);
      else {
          putquality(mbp, pp->my_quality, QA_RIGHT);
          putchr('/', mbp);
          if (pp->my_quality)
              putquality(mbp, pp->his_quality, QA_LEFT);
          else
              putquality(mbp, 0, QA_LEFT);
          }
      }
      putspa(40, mbp);

  if (typ == INP || typ == FLEXNET)
    putprintf(mbp, "%8lu ", (ULONG)pp->maxtime * 10L);
  else
    putstr("         ", mbp);

  putspa(52,mbp);
  putstr(c_state[state], mbp);

  putchr(' ', mbp);

  if (typ == LOCAL)
    putstr("    ", mbp);

  if (!printver) {
    if (*(c = pp->l2link->digil))
    {
      while (*c)
      {
        putid(c, mbp);
        putchr(' ', mbp);
        c += L2IDLEN;
      }
    }
  }
  else
  {
     switch (pp->typ)
     {
       case NETROM :
         cp = "NET/ROM (UI)";
         break;
       case THENET :
         cp = "THENET (UI)";
         break;
       case TNN :
         putprintf(mbp, "TNN V%d.%d (I) ", (pp->version/100),
                                           (pp->version%100));
         cp = "";
         break;
       case INP :
         cp = "INP Node";
         break;
       case FLEXNET :
         switch (pp->version & 0x07)
         {
           case 0 : cp = "FlexNet"; break;
           case 1 : cp = "BayCom"; break;
           case 2 : cp = "Digiware"; break;
           case 3 : cp = "TheNetNode"; break;
           case 4 : cp = "SNet"; break;
           case 5 : cp = "(X)Net"; break;
           default: cp = "unknown"; return;
         }
         putspa(58,mbp);
         putstr(cp, mbp);
         if (pp->version)
           putprintf (mbp, " V%d.%d", ((pp->version>>8) / 10),
                                      ((pp->version>>8) % 10));
       default :
         cp = "";
         break;
     }
     putspa(65,mbp);
     putstr(cp, mbp);
  }

#ifdef CONNECTTIME
  if (!printver)                         /* Nur wenn keine erweiterte Ausgabe */
  {                                                  /* Connectzeit ausgeben. */
    putspa(72, mbp);
    putprintf(mbp, "%s", ConnectTime(pp->contime));
  }
#endif /* CONNECTTIME */

  putchr('\r', mbp);
}

#ifdef ROUTESMODVIANODES
/**************************************************************************/
/*                                                                        */
/*  Segment suchen und bereitstellen.                                     */
/*                                                                        */
/**************************************************************************/
static PEER *isSegment(const char *call)
{
  PEER *pp;
  int   max_peers = netp->max_peers;
  int   i;

  if (call[0] == FALSE)
    return(NULL);

  /* durchsuche alle Segmente. */
  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
  {
    /* Nur benutzte Eintraege. */
    if (  (pp->used)
      /* Callvergleich (SSID beachten). */
        &&(cmpid(call, pp->l2link->call)))
      /* Segment gefunden. */
      return(pp);
  }

  /* Kein Eintrag gefunden. */
  return(NULL);
}

/**************************************************************************/
/*                                                                        */
/*  Ausgabe vorbereiten.                                                  */
/*                                                                        */
/**************************************************************************/
static MBHEAD *RoutesMBP(void)
{
  MBHEAD *mbp;

  mbp = getmbp();               /* frischen Puffer besorgen */
  putstr("Routes of ", mbp);    /* Konfiguration zeigen */
  putalt(alias, mbp);
  putid(myid, mbp);

  putprintf(mbp, " (%d/%d)\r", netp->num_peers, netp->max_peers);
#ifndef CONNECTTIME
  putstr("Node------SSID--Typ-Po--Dst/Rou---L3SRTT[ms]---MaxT[ms]-State--Route-\r", mbp);
#else
  putstr("Node------SSID--Typ-Po--Dst/Rou---L3SRTT[ms]---MaxT[ms]-State--Route----Contime\r", mbp);
#endif /* CONNECTTIME */
  return(mbp);
}

/**************************************************************************/
/*                                                                        */
/*  ROUTES COMMAND mit Erweiterung.                                       */
/*  Ein Routes-Eintrag ausgeben und dazu alle Noden                       */
/*  die ueber diesen Routes-Eintrag geroutet werden.                      */
/*                                                                        */
/*  THENETMOD                                                               */
/*  Qualitaet einer Route (nur THENET-Typ) setzen/aendern.                */
/**************************************************************************/
void putnodes(PEER *pp, MBHEAD *mbp)
{
  PEER        *bestpp;
  NODE        *np;
  INDEX        index;
  char         call[10];
  int          len;
  int          width = 0;
  unsigned int numroutes = 0;
  unsigned int qual;

  call2str(call, pp->l2link->call);
#ifdef SPEECH
  putprintf(mbp,speech_message(273), call);
#else
  putprintf(mbp, "\rAll Nodes over %s to be geroutet to show:\r", call);
#endif

  for (np = (NODE *)netp->nodelis.head;
       np != (NODE *)&netp->nodelis;/* sortierte Nodes-Liste durchgehen */
       np = np->next)
  {
    if (np->id[0])              /* nur benutzte Eintraege interessieren */
    {
      index = (INDEX)(np - netp->nodetab);      /* Index berechnen      */
      qual = find_best_qual(index, &bestpp, OPTIONS_MASK); /* suche besten Weg */

      if ((bestpp == NULL) || (qual == 0)) /* geloeschte Nodes abfangen */
        continue;

      if (cmpid(bestpp->l2link->call, pp->l2link->call))  /* Callvergleich */
      {
        ++numroutes;

        /* Rufzeichen ausgeben. */
        putid(np->id,mbp);
        /* Konvertiere Rufzeichen. */
        call2str(call, np->id);
        /* Laenge eines Eintrages. */
        len = strlen(call) + 16;
        width += len;

        if (width + len < 169)
        {
          /* einer passt noch in die Zeile   */
          len = (79 % len) / (79 / len);

          while (len--)
            putchr(' ', mbp);
        }
        else
          {
            putchr('\r', mbp);
            width = 0;
          }
      }
    }
  }

  putchr('\r', mbp);
}

/**************************************************************************/
/*                                                                        */
/*  ROUTES COMMAND mit Erweiterung.                                       */
/*  Ein Routes-Eintrag ausgeben und dazu alle Noden                       */
/*  die ueber diesen Routes-Eintrag geroutet werden.                      */
/*                                                                        */
/*  THENETMOD                                                               */
/*  Qualitaet einer Route (nur THENET-Typ) setzen/aendern.                */
/**************************************************************************/
static void RoutesViaNodes(void)
{
  MBHEAD *mbp = NULL;
  char    call[L2IDLEN];

  skipsp(&clicnt,&clipoi);                     /* evl. Leerzeichen entfernen. */

  /* Pruefe Rufzeichen. */
  if ((getcal(&clicnt, &clipoi, TRUE, call)) == YES)
  {
    PEER        *pp;

    /* Aktueller Linkblock ist ein Segment. */
    if ((pp = isSegment(call)) != NULL)
    {
#ifdef THENETMOD
      if (pp->typ == THENET)      /* L4QUALI, Aenderungen nur am Typ  THENET. */
      {
        if (RoutesL4Para(mbp, pp))
          return;
      }
#endif /* THENETMOD */

      mbp = RoutesMBP();

      putrou(pp, mbp, FALSE);
      putnodes(pp, mbp);
      prompt(mbp);
      seteom(mbp);
    }
    else
      /* Es gibt kein Segment mit den angegeben Rufzeichen. */
      {
        mbp = RoutesMBP();
#ifdef SPEECH
        putstr(speech_message(276),mbp);
#else
        putstr("Call does not stand in the links List!\r",mbp);
#endif
        prompt(mbp);
        seteom(mbp);
        return;
      }
  }
  else
    /* Ungueltiges Rufzeichen . */
    {
      mbp = RoutesMBP();
#ifdef SPEECH
      putstr(speech_message(196),mbp);
#else
      putstr("Invalid Call!\r",mbp);
#endif
      prompt(mbp);
      seteom(mbp);
      return;
    }
}
#endif /* ROUTESMODVIANODES */

/*------------------------------------------------------------------------*/
/*  ROUTES COMMAND      Ausgabe einer formatierten Routes-Liste an User.  */
/*                      Eingabe von Routes durch SYSOP.                   */
/*------------------------------------------------------------------------*/
void ccprou(void)
{
  MBHEAD *mbp;
  BOOLEAN printver = FALSE;
  PEER   *i_pp, *j_pp;
  int     i, j;
  int     flag[MAX_PEERS];
  const int max_peers = netp->max_peers;

  memset(flag, 0, sizeof(flag));

  if (clicnt != 0)
  {
    if (   *clipoi == '*'
        || *clipoi == '+')
      printver = TRUE;          /* Version mit ausgeben */
#ifdef ROUTESMODVIANODES
    else
      {
        RoutesViaNodes();
        return;
      }
#endif /* ROUTESMODVIANODES */
  }

  mbp = getmbp();               /* frischen Puffer besorgen */
  putstr("Routes of ", mbp);    /* Konfiguration zeigen */
  putalt(alias, mbp);
  putid(myid, mbp);

  putprintf(mbp, " (%d/%d)\r", netp->num_peers, netp->max_peers);
  putstr("Node------SSID--Typ-Po--Dst/Rou---L3SRTT[ms]---MaxT[ms]-State--", mbp);
#ifdef CONNECTTIME
  putstr(printver ? "Software/Version\r"
                  : "Route----Contime\r", mbp);
#else
  putstr(printver ? "Software/Version\r"
                  : "Route-----------\r", mbp);
#endif /* CONNECTTIME */

  for (i = 0, i_pp = netp->peertab; i < max_peers; ++i, ++i_pp)
    if (i_pp->used)
      if (i_pp->primary == i_pp)
      {
        putrou(i_pp, mbp, printver);

        for (j = 0, j_pp = netp->peertab; j < max_peers; ++j, ++j_pp)
          if (j_pp->used)
            if (j_pp->primary == i_pp && j_pp != i_pp)
              putrou(j_pp, mbp, printver);
      }

  prompt(mbp);
  seteom(mbp);
}

/* End of src/l7showl3.c */
