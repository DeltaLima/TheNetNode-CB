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
/* File src/l3tab.c (maintained by: ???)                                */
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

/************************************************************************/
/* In diesem Modul werden die Verwaltungsstrukturen fuer den Level 3    */
/* und die dazugehoerigen Algorithmen implementiert.                    */
/* Alle Routinen in diesem Modul sind nicht abhaengig vom der ver-      */
/* wendeten Netzwerk-Klasse.                                            */
/************************************************************************/

#include "tnn.h"
#include "l3local.h"

static int hashid(const char *, int);
static void range(ULONG *, ULONG);
static void clear_peer(PEER *);
static void smooth(unsigned long *, unsigned long);

/* HASHING */
#if 0
static int hashid(                  /* Hash-Funktion fuer ein Rufzeichen   */
                  const char *id,   /* Rufzeichen ohne SSID                */
                  int hashsize)     /* Hash-Array Groesse                  */
{
  int prim[] = {3, 5, 7, 11, 13, 17}; /* tnx Prof. Dassow, UMD             */
  register int r, hashval = 0;
  for (r = 0; r < 6; r++)
    hashval += ((unsigned)(*id++)) * prim[r];
  return((unsigned)(hashval % hashsize));
}
#else                   /* nicht alle compiler koennen unroll-loops     */
/* Hash-Funktion fuer ein Rufzeichen                                    */
static int
hashid(const char *id,                          /* Rufzeichen ohne SSID */
       int hashsize)                            /* Hash-Array Groesse   */
{
  register int hashval;

  hashval = ((unsigned)(*id++)) * 3;
  hashval += ((unsigned)(*id++)) * 5;
  hashval += ((unsigned)(*id++)) * 7;
  hashval += ((unsigned)(*id++)) * 11;
  hashval += ((unsigned)(*id++)) * 13;
  hashval += ((unsigned)(*id)) * 17;
  return((unsigned)(hashval % hashsize));
}
#endif

/* ein neues Segment anlegen */
PEER *register_peer(void)
{
  PEER *peertab = netp->peertab;    /* Segment-Tabelle                     */
  PEER *pp;

  for (pp = peertab; pp < &peertab[netp->max_peers]; pp++)
    if (!pp->used) break;

  if (pp != &peertab[netp->max_peers]) { /* nur wenn Platz                 */
    if ((pp->routes = calloc((size_t)netp->max_nodes, sizeof(ROUTE))) == NULL)
      return(NULL);                 /* kein Speicher                       */
    pp->quality =                   /* Segment nicht erreichbar            */
    pp->my_quality =
    pp->his_quality = 0L;
    pp->locked = FALSE;
    pp->used = TRUE;
    pp->num_routes = 0;
    pp->primary = pp;
    pp->maxtime = 0;

#ifdef THENETMOD
             /* Kein Dauerconnect zum Nachbarn (nur THENET-TYP). Ist keine */
    pp->constatus = 1;        /* Aktivitaet, wird die Verbindung getrennt. */
#endif /* THENETMOD */

    netp->num_peers++;
    return(pp);
  }
  return(NULL);
}

void unregister_peer(               /* Segment austragen                   */
                     PEER *pp)      /* Zeiger auf das Segment              */
{
  clear_peer(pp);                   /* Routen ueber das Segment verwerfen  */
  free(pp->routes);                 /* Routentabelle freigeben             */
  netp->num_peers--;
  pp->used = FALSE;
}

/************************************************************************/
/*                                                                      */
/* Node id mit SSID-Bereich suchen                                      */
/*                                                                      */
/************************************************************************/
INDEX
find_node_ssid_range(const char *id)
{
  NODE *np;
  NODE *nodetab = netp->nodetab;
  int   max_nodes = netp->max_nodes;
  int   hash_tries;                     /* Anzahl Schritte bis Treffer  */
  int   ssid = SSID(id);
  INDEX index;

  if (id[0] == FALSE)
    return (NO_INDEX);

  index = hashid(id, max_nodes);
  if ((np = nodetab + index) == NULL)
    return (NO_INDEX);

  for (hash_tries = 0;                  /* Ab Hash-Position suchen      */
       hash_tries < max_nodes;          /* ueber die ganze Hash-Tabelle */
       hash_tries++)                    /* keine Nodes mehr uebrig?     */
  {
    if (np->id[0] == TRUE)
    {
      if (   (ssid >= SSID(np->id))     /* passt der SSID-Bereich?      */
          && (ssid <= np->ssid_high))
      {
        if (cmpcal(np->id, id))
          return (index);               /* Index liefern                */
      }
    }

    if (++index >= max_nodes)           /* np weiterruecken, Umbruch    */
    {                                   /* beachten                     */
      np = nodetab;
      index = 0;
    }
    else
      np++;
  }

  return (NO_INDEX);
}

/************************************************************************/
/*                                                                      */
/* Node id mit festem SSID suchen                                       */
/*                                                                      */
/************************************************************************/
INDEX
find_node_this_ssid(const char *id)
{
  NODE *np;
  NODE *nodetab = netp->nodetab;
  int   max_nodes = netp->max_nodes;
  int   hash_tries;                     /* Anzahl Schritte bis Treffer  */
  INDEX index;

  index = hashid(id, max_nodes);
  np = nodetab + index;

  for (hash_tries = 0;                  /* Ab Hash-Position suchen      */
       hash_tries < max_nodes;          /* ueber die ganze Hash-Tabelle */
       hash_tries++)                    /* keine Nodes mehr uebrig?     */
  {
    if (np->id[0])
      if (cmpid(np->id, id))            /* gefunden?                    */
        return (index);                 /* Index liefern                */
    if (++index >= max_nodes)           /* np weiterruecken, Umbruch    */
    {                                   /* beachten                     */
      np = nodetab;
      index = 0;
    }
    else
      np++;
  }
  return (NO_INDEX);
}

/************************************************************************/
/*                                                                      */
/* einen Node einsortieren                                              */
/*                                                                      */
/************************************************************************/
static void
sort_node(NODE *new_np)
{
  NODE *np;
  char *id = new_np->id;                /* Rufzeichen des neuen Nodes   */
  int   ssid = SSID(id);                /* SSID des neuen Nodes         */
  int   i;

  for (np  = (NODE *)netp->nodelis.head;
       np != (NODE *)&netp->nodelis;
       np  = np->next)                  /* richtige Stelle finden       */
    if ((i = strncmp(np->id, id, L2CALEN)) >= 0)
      if ((i > 0) || (SSID(np->id) > ssid))
        break;
  relink((LEHEAD *)new_np, (LEHEAD *)np->prev);
}

/************************************************************************/
/*                                                                      */
/* Node id zur Nodestabelle hinzufuegen + LINKTYP                       */
/*                                                                      */
/************************************************************************/
INDEX
add_node(const char *id)
{
  int   i;
  NODE *np;
  NODE *nodetab = netp->nodetab;
  int   max_nodes = netp->max_nodes;
  INDEX index;

  index = hashid(id, max_nodes);        /* Position vorraussagen        */
  np = nodetab + index;

  for (i = 0; i < max_nodes; i++)
  {
    if (!np->id[0])                     /* Platz gefunden               */
    {
      cpyid(np->id, id);                /* Call umkopieren              */
      cpyals(np->alias, nulide);        /* Alias loeschen               */
      np->ipa = 0L;                     /* IP-Adresse loeschen          */
      np->bits = 0;                     /* Subnet-Maskenbits loeschen   */
      np->options = NULL;               /* keine weiteren INP-Options   */
      np->ssid_high = SSID(id);         /* ohne SSID-Bereich            */
      netp->num_nodes++;                /* ein Node mehr                */
      if (netp->num_nodes > num_nodes_max)
        num_nodes_max = netp->num_nodes;
      sort_node(np);                    /* Node einsortieren            */
      return (index);                   /* Index liefern                */
    }
    if (++index >= max_nodes)           /* np weiterruecken, Umbruch    */
    {                                   /* beachten                     */
      np = nodetab;
      index = 0;
    }
    else
      np++;
  }
  return (NO_INDEX);
}

/************************************************************************/
/*                                                                      */
/* einen Node aus der Liste nehmen                                      */
/*                                                                      */
/************************************************************************/
void
del_node(INDEX index)
{
  NODE *np = netp->nodetab + index;

  destot(index);
  np->id[0] = NUL;                      /* Eintrag unbenutzt            */

  /* INP: Node hatte eine IP, deshalb ARP- und IPR-Eintrag loeschen     */
  /* aber nur wenn erlaubt                                              */
  if (np->ipa != 0L && autoipr != 0)
  {
    arp_drop(np->ipa, NETROM_PORT, TRUE);
    rt_drop(np->ipa, np->bits, TRUE);
  }

  if (np->options != NULL)    /* eventuell vorhandene Optionen loeschen */
  {
    dealmb(np->options);                           /* Optionen loeschen */
    np->options = NULL;                      /* hat keine Optionen mehr */
  }

  netp->num_nodes--;                    /* ein bekannter Node weniger   */

  ulink((LEHEAD*)np);                   /* aussortieren                 */
}

/************************************************************************/
/*                                                                      */
/* unerreichbare Nodes loeschen                                         */
/*                                                                      */
/************************************************************************/
void drop_unreachable_nodes(void)   /* Routes/Nodes loeschen            */
{
  ROUTE *rp;                        /* Zeiger auf eine Route            */
  PEER  *pp;                        /* Zeiger auf ein Segment           */
  NODE  *np;                        /* Zeiger auf einen Node            */
  NODE  *nodetab = netp->nodetab;   /* Zeiger auf die Node-Tabelle      */
  PEER  *peertab = netp->peertab;   /* Zeiger auf die Segment-Tabelle   */
  int    max_nodes = netp->max_nodes; /* Groesse der Nodes-Tabelle      */
  int    max_peers = netp->max_peers; /* Groesse der Segment-Tabelle    */
  INDEX  index;

#ifdef L3TABDEBUG
#if MAX_TRACE_LEVEL > 2
  char   notify_call1[10];
#endif
#endif

  /* die ganze Nodetabelle durchgehen */
  for (index = 0, np = nodetab; index < max_nodes; index++, np++)
  {
    if (!np->id[0])         /* Eintrag ist unbenutzt                    */
      continue;             /* braucht nicht bearbeitet zu werden       */

    /* alle Peers durchgehen */
    for (pp = peertab; pp < &peertab[max_peers]; pp++)
    {
      if (!pp->used)      /* Eintrag ist unbenutzt                    */
        continue;         /* braucht nicht bearbeitet zu werden       */

      rp = &pp->routes[index];  /* Zeiger auf den Routeneintrag       */

      if (   (rp->reported_quality != 0) /* wir haben Qualitaet gemeldet */
          || (rp->quality != 0))         /* und er auch                  */
        break;             /* noch Routen vorhanden, nicht loeschen   */
#ifdef L3TABDEBUG
#if MAX_TRACE_LEVEL > 2
      call2str(notify_call1, np->id);
      notify(3, "drop_unreachable_nodes(): node %s has no routing info",
             notify_call1);
#endif
#endif
    }

    /* steht nach dem Durchlauf der oberen Schleife der pp-Pointer      */
    /* auf dem letzten Eintrag, dann kann geloescht werden              */
    if (pp == &peertab[max_peers])  /* keine aktiven Routen mehr        */
    {
#ifdef L3TABDEBUG
#if MAX_TRACE_LEVEL > 2
      call2str(notify_call1, np->id);
      notify(3, "drop_unreachable_nodes(): no peer found for %s", notify_call1);
#endif
#endif
      del_node(index);              /* Node austragen                   */
    }
  }
}

/************************************************************************/
/*                                                                      */
/* eine Route hinzufuegen                                               */
/*                                                                      */
/************************************************************************/
INDEX
add_route(PEER *pp, const char *id, unsigned quality)
{
  INDEX index = find_node_this_ssid(id);

  if ((index == NO_INDEX) && quality)   /* nur bei guter Qualitaet neu  */
    index = add_node(id);

  if (index != NO_INDEX)                /* nur wenn noch Platz war      */
    update_route(pp, index, quality);
  return (index);
}

/************************************************************************/
/*                                                                      */
/* eine Route aktualisieren                                             */
/*                                                                      */
/************************************************************************/
void
update_route(PEER *pp, INDEX index, unsigned quality)
{
  ROUTE *rp = pp->routes + index;       /* Zeiger auf die Route         */

/* Empfangene Laufzeit aktualisieren                                    */

  if (quality >= HORIZONT)
    quality = 0;                        /* Route ausgefallen            */

  if (rp->quality && !quality)          /* faellt eine Route aus?       */
    pp->num_routes--;

  if (!rp->quality && quality)          /* neue Route?                  */
    pp->num_routes++;

  rp->quality = quality;                /* Qualitaet neu setzen         */

  rp->timeout = (pp->secured)
                ? (0)                   /* ohne Timeout                 */
#ifndef THENETMOD
                : ROUTE_TIMEOUT;
#else
                : obcini;      /* Lebensdauer einer Node aktualisieren. */
#endif /* THENETMOD */
}

/************************************************************************/
/*                                                                      */
/* Eintragen der L3-Lifetime fuer ein Ziel                              */
/*                                                                      */
/************************************************************************/
void
update_lt(PEER *pp, INDEX index, int lt)
{
  (pp->routes + index)->lt = lt;
}

/************************************************************************/
/*                                                                      */
/* Feststellen, ob sich der Alias aendert, und bei neuem Alias diesen   */
/* eintragen.                                                           */
/*                                                                      */
/************************************************************************/
BOOLEAN
update_alias(INDEX index, const char *alias)
{
  NODE *np = netp->nodetab + index;

  if (cmpcal(alias, nulide))            /* kein Alias gemeldet          */
    return (FALSE);
  if (cmpcal(np->alias, alias))         /* keine Aenderung              */
    return (FALSE);
  cpyals(np->alias, alias);             /* neuen Alias setzen           */
  return (TRUE);
}

BOOLEAN
update_ssid(INDEX index, int ssid_high)
{
  NODE *np = netp->nodetab+index;

  if (np->ssid_high == ssid_high)
    return (FALSE);                     /* keine Aenderung              */
  np->ssid_high = ssid_high;            /* obere SSID-Grenze nachtragen */
  return (TRUE);                        /* geaendert                    */
}

/* Grenzen pruefen                                                         */
static void range(ULONG *oldval, ULONG newval)
{
  if (newval < RTT_MIN)
    newval = RTT_MIN;
  if (newval <= RTT_MAX)            /* noch nicht am Horizont?             */
    *oldval = newval;
    else
    *oldval = 0;
}

/* Glaettung durchfuehren und Grenzen pruefen                              */
static void
smooth(ULONG *oldval, ULONG newval)
{
  if (newval < RTT_MIN)
    newval = RTT_MIN;
  if (newval <= RTT_MAX) {          /* noch nicht am Horizont?             */
    if (*oldval)                    /* nicht die erste Messung             */
      *oldval = ((*oldval + 1) * RTT_ALPHA1 - 1 + newval) / RTT_ALPHA2;
    else
      *oldval = (newval * RTT_BETA);
    if (*oldval < 1)
      *oldval = 1;
  } else
    *oldval = 0;                    /* Link ist tot, komplette Neumessung  */
                                    /* ist notwendig                       */
}

/* Die Qualitaet eines Segmentes aktualisieren, wir unterscheiden dabei
   seine und unsere Messung. Unbekannte/unveraenderte Werte werden
   durch IGNORE_RTT (==0) uebergeben.
   pp->his_quality und pp->my_quality koennen folgende Werte annehmen:

   0                     noch keine Messung in diese Richtung erfolgreich
   RTT_MIN...RTT_MAX     gueltiger Messwert
   groesser RTT_MAX      Peer nicht erreichbar

   pp->quality wird auf RTT_MAX+1 gesetzt
    - wenn pp->my_quality nicht gueltig *oder* unbekannt
    - wenn pp->his_quality nicht gueltig ist
*/

void update_peer_quality(
PEER *pp,                           /* fuer welches Segment?               */
ULONG my_quality,                   /* meine Messung                       */
ULONG his_quality)                  /* Nachbarqualitaet                    */
{
  if (his_quality != DONT_CHANGE_QUAL) {
    if (his_quality > 0)            /* hat er eine Messung?                */
      range(&pp->his_quality, his_quality);
    else
      if (his_quality == 0)
        pp->his_quality = 0;
  }

  if (my_quality != DONT_CHANGE_QUAL) {
    if (my_quality > 0)               /* unsere Messung gueltig?             */
      smooth(&pp->my_quality, my_quality);
    else
      if (my_quality == 0)
        pp->my_quality = 0;
  }

  if (pp->my_quality == 0)
#ifndef THENETMOD
    pp->quality = 0;
#else
  {
    if (pp->typ != THENET)  /* Alle Segmente ausser THENET Qualitaet updaten. */
      pp->quality = 0;
  }
#endif /* THENETMOD */
  else {
    if (pp->typ == FLEXNET)
      range(&pp->quality,
            (pp->my_quality + (pp->his_quality ? pp->his_quality : 6000))/2);
    else
#ifdef THENETMOD
      if (pp->typ != THENET) /* Alle Segmente ausser THENET Quali. updaten. */
#endif /* THENETMOD */
        pp->quality = pp->my_quality;
  }

  update_primary_peer(pp->l2link->call);
}

/************************************************************************/
/*                                                                      */
/* Routen ueber das Segment verwerfen                                   */
/*                                                                      */
/************************************************************************/
static void
clear_peer(PEER *pp)
{
#ifdef THENETMOD
  if (pp->typ == THENET)                  /* Segment ist ein THENET-Typ */
    return;                             /* keine Aenderungen vornehmen. */
#endif /* THENETMOD */

  memset(pp->routes, 0, netp->max_nodes * sizeof(ROUTE));
  pp->num_routes = 0;                   /* Routen loeschen              */
  drop_unreachable_nodes();             /* Routes/Nodes loeschen        */
}

/************************************************************************/
/*                                                                      */
/* Linkreset eines Nachbarn                                             */
/*                                                                      */
/************************************************************************/
void
reset_peer(PEER *pp)
{
  int    i;
  NODE  *np;
  ROUTE *rp;
  int    max_nodes = netp->max_nodes;

  if (pp->typ <= TNN)
    set_peer_typ(pp, NETROM);
  for (i = 0, np = netp->nodetab, rp = pp->routes;
       i < max_nodes;
       i++, np++, rp++)
  {
    if (!np->id[0])                     /* freier Eintrag?              */
      continue;
    rp->reported_quality = 0;           /* Eintrag muss gemeldet werden */
    if (rp->quality != 0)               /* Nachbar hatte gemeldet       */
      if (   rp->timeout == 0           /* bisher sichere Route oder    */
          || rp->timeout > 4)           /* Timeout zu gross?            */
      rp->timeout = 4;                  /* neues Timeout 3-4 Minuten    */
  }
  update_peer_quality(pp, 0, DONT_CHANGE_QUAL);
  drop_unreachable_nodes();
}

/************************************************************************/
/*                                                                      */
/* Verbindung zum Segment hergestellt                                   */
/*                                                                      */
/************************************************************************/
void
connect_peer(PEER *pp)
{
  clear_peer(pp);                   /* Alle Ziele loeschen (erstmal)    */
}

/************************************************************************/
/*                                                                      */
/* Verbindung zum Segment beendet                                       */
/*                                                                      */
/************************************************************************/
void
disconnect_peer(PEER *pp)
{
  clear_peer(pp);                       /* alle Routen loeschen         */
}

void
set_peer_typ(PEER *pp, int typ)
{
  int    i;
  NODE  *np;
  ROUTE *rp;
  int    max_nodes;

  pp->typ = typ;

#ifndef LINKSMOD_LOCALMOD
  pp->secured =    (typ == LOCAL_M)
                || (typ == LOCAL)
                || (typ == FLEXNET)
                || (typ == INP);
#else
  pp->secured =    (typ == LOCAL_V)
                || (typ == LOCAL_N)
                || (typ == LOCAL_M)
                || (typ == LOCAL)
                || (typ == FLEXNET)
                || (typ == INP);
#endif /* LINKSMOD_LOCALMOD */

  /* bei INP werden nach Linkaufbau alles Nodes an den Nachbarn gemeldet */
  if (typ == INP)
  {
    max_nodes = netp->max_nodes;
    for (i = 0, np = netp->nodetab, rp = pp->routes;
         i < max_nodes;
         i++, np++, rp++)
    {
      if (!np->id[0])                     /* freier Eintrag?              */
        continue;
      rp->reported_quality = 0;           /* Eintrag muss gemeldet werden */
    }
  }
}

/* Qualitaet des besten Weges (immer >= 1)                              */
/* oder 0 wenn es keinen Weg zum Ziel gibt */
unsigned
do_find_best_qual(INDEX index, PEER *notthis, PEER **retpp, int options)
{
  int       i;
  PEER     *pp = netp->peertab,
           *bestpp = NULL;
  ROUTE    *rp;
  unsigned  bestqual = 0;
  unsigned  quality = 0;

  for (i = netp->max_peers; i--; pp++)
  {
    if (!pp->used)
      continue;
    if (pp == notthis)
      continue;
    if ((pp->options & options) == 0)
      continue;
    rp = pp->routes + index;
    quality = getquality(rp->quality, pp);
    if ((quality && quality < bestqual) || !bestqual)
    {
      bestpp = pp;                             /* Route und Peer merken */
      bestqual = quality;
    }
  }
  if (retpp)
    *retpp = bestpp;                 /* besten Nachbarn liefern         */
  return (bestqual);                 /* und Qualitaet als Rueckgabewert */
}

unsigned
getquality(unsigned route_qual, PEER *pp)
{
  ULONG qual;

  if (route_qual == 0 || pp->quality == 0)
    return (0);
  if (pp->typ <= NETROM)
  {
    qual = ((ULONG)route_qual) + pp->quality;
    if (qual > HORIZONT)
      qual = HORIZONT;
    return ((unsigned)qual);
  }
  else
    return (route_qual);
}

/* find_route() wird vom L7 benutzt, um einen Connect zu einem Node
 * aufzubauen. Dies kann ein dummer L2 sein (Local), ein
 * L2 (Flexnet) oder ein L4 QSO (NetRom).
 * Wegen Flexnet sieht das Addressfeld bei einem Connect recht witzig
 * aus:
 * fm DB0SRC to DB0DST via DB0MY DB0DUM DB0NBR
 * DB0MY ist das eigene Rufzeichen (myid), DB0DUM ein schlichter L2-
 * Wassertraeger und DB0NBR der Nachbar. DB0DUM entfaellt in der Regel.
 * Besondere Vorkehrung wird fuer die ausgefallenen Nachbarn getroffen,
 * sie stehen eventuell nicht in der Nodes-Liste, aber wir kennen sie
 * ja noch als Nachbarn.
 * In dest.call steht das Ziel-Call, call ist eventuell nur ein Nachbar
 * auf dem Weg dorthin.
 */
int l3_find_route(char *call, DEST *dest)
{
  int          max_peers = netp->max_peers;
  INDEX        index;
  PEER        *bestpp;
  PEER        *pp;
  int          i;
  int          ssid;
  char        *id;
  int          maske = DG | VC | VC_FAR; /* erstmal alles erlaubt       */


  if ((index = find_node_this_ssid(call)) == NO_INDEX)
  {
    /* Kein Node, ist es vielleicht ein Alias ? */
    if ((index = find_alias(call)) == NO_INDEX)
    {
      /* Da das genaue Ziel nicht gefunden wurde, koennen wir noch nach dem   */
      /* SSID-Bereich ueber Flexnet-Nachbarn suchen                           */
      if ((index = find_node_ssid_range(call)) != NO_INDEX)
      {
        /* Wir haben einen Flexnet-Weg gefunden - dann duerfen wir nachher auch */
        /* nur ueber einen Flexnet-Weg connecten                                */
          maske = FLEX_MASK;
      }
    }
  }

  if (index != NO_INDEX)
  {
    if (find_best_qual(index, &bestpp, maske) > 0)
    {
      dest->via[0] = NUL;
      cpyid(dest->nbrcal, bestpp->l2link->call);

      if (bestpp->typ > NETROM)
        cpyidl(dest->via, bestpp->l2link->digil);
      /*
       * Wenn des Ziel nicht in den SSID-Bereich des Nachbarn passt,
       * muessen wir das Nachbarrufzeichen in des via-Feld aufnehmen.
       */
      id = netp->nodetab[index].id;

      ssid = SSID(id);
      if (   (!cmpcal(id, bestpp->l2link->call))
          || (ssid < SSID(bestpp->l2link->call))
          || (ssid > bestpp->l2link->ssid_high))
        addid(dest->via, bestpp->l2link->call);

      dest->port = bestpp->l2link->port;
      dest->typ  = bestpp->typ;
      dest->np   = netp->nodetab+index;
#ifdef THENETMOD
      if (  (bestpp->typ == THENET)             /* Segment ist ein THENET-Typ */
          &&(dest->typ != 'U')      /* wenn kein "direkt ('U')" markiert ist. */
          &&(bestpp->nbrl2l == NULL))               /* und kein aktiver Link. */
        connbr(bestpp);                                 /* Link neu aufbauen. */
#endif /* THENETMOD */

      return(NODE_AVAILABLE); /* Ziel ist erreichbar */
    }
    return(NODE_DOWN);
  }

  /* wenn direkter Nachbar nicht erreichbar, dies mitteilen */
  for (pp = netp->peertab, i = 0; i < max_peers; pp++, i++)
    if (pp->used)
      if (cmpid(pp->l2link->call, call))
#ifndef CONNECTMOD_NODE_AVAI
        if (!pp->quality) {
          dest->typ  = pp->typ;
          cpyid(dest->nbrcal, myid);
          return(NODE_DOWN);                 /* Nachbar unerreichbar */
        }
#else
      {
                               /* Auch wenn der Nachbar "unerreichbar" sein   */
                               /* sollte versuchen wir es ueber einen L2_link.*/
        dest->port = pp->l2link->port;                     /* L2-Port setzen. */
        dest->typ  = 'U';                                /* wir rufen direkt. */
        return(NODE_AVAILABLE);                        /* Ziel ist erreichbar */
      }
#endif /* CONNECTMOD_NODE_AVAI */

  return(NODE_UNKNOWN); /* Ziel nicht gefunden */
}

/*----------------------------------------------------------------------*/
/* Feststellen, ob zu einem Node noch ein Weg existiert. Wenn es keinen */
/* aktiven Weg mehr gibt, wird dies dem L4 gemeldet. Dieser traegt dann */
/* alle aktiven L4-Verbindungen aus.                                    */
/* Da wir die Wege nach der Qualitaet sortieren, reicht es zu pruefen,  */
/* ob der beste Weg ausgefallen ist. In diesem Fall erfolgt die Meldung */
/* an den L4.                                                           */
/* Diese Routine wird nach jeder Veraenderung der Qualitaet eines Weges */
/* aufgerufen.                                                          */
/*----------------------------------------------------------------------*/
BOOLEAN
check_destot(INDEX index)       /* ueberprueft, ob es noch verwendbare  */
{                               /* Wege zu einem Ziel gibt              */
  if (find_best_qual(index, NULL, DG) == 0)
  {
    destot(index);
    return (TRUE);
  }
  return (FALSE);
}

/*----------------------------------------------------------------------*/
void
check_all_destot(void)
{
  INDEX index;
  int   max_nodes = netp->max_nodes;

  for (index = 0; index < max_nodes; index++)
    check_destot(index);
}

/*----------------------------------------------------------------------*/
void
destot(INDEX index)             /* Ziel ist nicht mehr erreichbar       */
{
  MBHEAD *mbp,
         *nextmbp;
  NODE   *totnod = netp->nodetab + index;

  l3tol4(totnod);               /* an L4 melden: Knoten wird entfernt   */

  for (mbp = (MBHEAD *)l3txl.head;              /* L4 Sendeliste        */
       mbp != (MBHEAD *)&l3txl.head;            /* durchgehen           */
       mbp = nextmbp)
  {
    nextmbp = (MBHEAD *)mbp->nextmh;
    if ((NODE *)mbp->l2link == totnod)
    {                                           /* dieser Node?         */
      ulink((LEHEAD *)mbp);
      dealmb(mbp);
    }
  }
}

/* den besten Weg zu einem Segment feststellen (wenn es mehrere gibt)   */
void
update_primary_peer(char *id)
{
  PEER  *peertab = netp->peertab;       /* Segment-Tabelle              */
  PEER  *pp;
  PEER  *bestpp = NULL;
  ULONG  quality;
  ULONG  bestqual = 0;

  /* den primaeren Weg suchen und merken */
  for (pp = peertab; pp < &peertab[netp->max_peers]; pp++)
    if (pp->used)
    {
      if (cmpid(pp->l2link->call, id))
      {
        if (bestpp)
        {
          quality = pp->quality;
          if ((quality && quality < bestqual) || !bestqual)
          {
            bestpp = pp;
            bestqual = quality;
          }
        }
        else
        {
          bestpp = pp;
          bestqual = pp->quality;
        }
      }
    }

  /* und dann in allen Wegen (zu dem Nachbarn) eintragen */
  for (pp = peertab; pp < &peertab[netp->max_peers]; pp++)
    if (pp->used)
      if (cmpid(pp->l2link->call, id))
        pp->primary = bestpp;
}

/* Kennen wir das Ziel als Ident?                                       */
INDEX
find_alias(char *ident)
{
  NODE *np = netp->nodetab;            /* NET/ROM-Tabelle durchsuchen   */
  INDEX i;
  int   max_nodes = netp->max_nodes;

  for (i = 0; i < max_nodes; i++, np++)
    if (np->id[0])                      /* Eintrag existiert?           */
      if (cmpals(ident, np->alias))
        return (i);
  return (NO_INDEX);                    /* nix gefunden                 */
}

/* Rufzeichen suchen                                                    */
BOOLEAN
iscall(const char *id, NODE **retnp, PEER **bestpp, int options)
{
  INDEX index;

/* Call ueber genaues Ziel oder/und ueber SSId suchen                   */
  if ((index = find_node_this_ssid(id)) == NO_INDEX)
    index = find_node_ssid_range(id);
  if (index != NO_INDEX)
  {
    if (retnp)
      *retnp = netp->nodetab + index;
    return (find_best_qual(index, bestpp, options) != 0); /* beste Qual */
  }
  return (FALSE);                   /* nix gefunden                     */
}

/* End of src/l3tab.c */
