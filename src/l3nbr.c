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
/* File src/l3nbr.c (maintained by: ???)                                */
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

static BOOLEAN  isneig(const char *, const char *, int, PEER *);

/*----------------------------------------------------------------------*/
/* Nachbarn disconnecten                                                */
/* Wir brauchen nicht den disconnect weiterzumelden, das macht der L2,  */
/* wenn alle Infos gesendet sind.                                       */
/*----------------------------------------------------------------------*/
void
discnbp(PEER *nbpx)
{
  LNKBLK         *save_lp;

  save_lp = lnkpoi;
  if ((lnkpoi = nbpx->nbrl2l) != NULL)
    dsclnk();                           /* disconnect link, dealml ...  */
  lnkpoi = save_lp;
}

/* einen Nachbarn austragen                                             */
#ifndef LINKSMOD_MSG
void
unregister_neigb(const char *id, const char *via, int port)
#else
BOOLEAN unregister_neigb(const char *id, const char *via, int port)
#endif /* LINKSMOD_MSG */
{
  char  digi_list[2 * L2IDLEN + 1];
  PEER *pp;                        /* Buffer fuer Eintrag               */

  cpyidl2(digi_list, via);         /* nur maximal 2 Digipeater!         */
  pp = getnei(digi_list, id, port);
  if (!pp)
#ifndef LINKSMOD_MSG
    return;
#else
    return(FALSE);
#endif /* LINKSMOD_MSG */

  update_peer_quality(pp, 0L, 0L); /* als ausgefallen melden            */

  if (pp->nbrl2l)                  /* noch connected?                   */
    discnbp(pp);

  dealoc((MBHEAD *)pp->l2link);
  unregister_peer(pp);
#ifdef LINKSMOD_MSG
  return(TRUE);
#endif /* LINKSMOD_MSG */
}

/* einen neuen Nachbarn eintragen                                       */
PEER *
register_neigb(const char *id, const char *via, const char *alias,
               int port, int typ
#ifdef LINKSMODINFO
               ,const char *info
#endif /* LINKSMODINFO */
#ifdef AUTOROUTING
               , UWORD Status
#endif /* AUTOROUTING */
)

{
  L2LINK *p;
  char    digi_list[2 * L2IDLEN + 1];
  PEER   *pp;
  INDEX   index;

#ifdef PROXYFUNC
  BOOLEAN ppproxy;
  ppproxy = (typ & PROXYMASK) ? TRUE : FALSE;
  typ &= PROXYMASK - 1;
#endif

  cpyidl2(digi_list, via);        /* nur maximal 2 Digipeater!          */
  pp = getnei(digi_list, id, port);

  if (pp != NULL)
  {                                /* wir kennen den Nachbarn           */
    index = find_node_this_ssid(id);
    if (update_alias(index, alias))     /* Alias hat sich geaendert     */
    {
      p = pp->l2link;
      cpyals(p->alias, alias);
      propagate_node_update(index);
    }
    return (pp);                   /* Eintrag liefern                   */
  }

  if ((pp = register_peer()) == NULL)
    return (NULL);

  p = pp->l2link = (L2LINK *)allocb(ALLOC_L2LINK); /* Platz f. L2-Link  */

/* Diese beiden Eintraege werden sofort von aussen neu gesetzt.         */
/* Bei NET/ROMs wird der alias selbstaendig bestimmt, die Qualitaet     */
/* ist nur fuer ungemessene Links der Vorgabe-Wert.                     */

  cpyid(p->call, id);              /* Call eintragen                    */
  cpyidl2(p->digil, digi_list);    /* maximal 2 Digis eintragen         */
  cpyals(p->alias, alias);         /* Alias eintragen                   */
  p->port = port;                  /* Port eintragen                    */
  p->ssid_high = SSID(id);         /* noch keine Grenze bekannt         */

#ifdef LINKSMODINFO
  memset(p->info, 0, sizeof(p->info));
  strncpy(p->info, info, INFOSIZE);
#endif /* LINKSMODINFO */

#ifdef AUTOROUTING
  p->ppAuto = Status;              /* Fixed/Auto-Route eintragen.       */
#endif

#ifdef PORT_L2_CONNECT_TIME
  p->sabmtime = 0;
#endif

  pp->soll_typ = typ;              /* soll ein Link dieses Typs werden  */
  if (typ == TNN || typ == INP)
    typ = NETROM;                  /* werden automatisch erkannt        */

#ifdef PROXYFUNC
  pp->proxy = ppproxy;
#endif
  set_peer_typ(pp, typ);           /* ist derzeit ein Link dieses Typs  */

  pp->nbrl2l = NULL;               /* kein L2 Link vorhanden            */
  pp->version = 0;                 /* unbekannte Version                */
  pp->rttstart = 0;                /* keine Messung unterwegs           */
  pp->rtt_time = 0;                /* schnell messen                    */
  pp->brotim = 0;                  /* Broadcast Timer                   */

#ifdef CONNECTTIME
  pp->contime = 0;                 /* Connectzeit zuruecksetzen.        */
#endif /* CONNECTTIME */

#ifdef THENETMOD
  if (typ == THENET)                        /* Nur wenn THENET-TYP ist, */
  {
    pp->brotim  = broint_ui;                 /* Broadcast-Timer setzen. */
    pp->obscnt  = 0;  /* Restlebensdauer fuer Rundspruch zuruecksetzen. */
  }
#endif /* THENETMOD */

  pp->options = (typ <= NETROM) ?  /* Datagramm oder VirtualConnect     */
                DG :
                (typ == FLEXNET) ? /* FlexNet oder Local?               */
                VC_FAR : VC;

  inithd(&(pp->rxfl));             /* Empfangsliste                     */

  if (typ == LOCAL)
  {                                /* erste Messung simulieren          */
#ifndef LOCAL_ROUTEFIX
    if ((index = add_route(pp, id, 400)) != NO_INDEX)
#else
    if ((index = add_route(pp, id, 10)) != NO_INDEX)    /* 10ms setzen. */
#endif /* LOCAL_ROUTEFIX */
    {
      update_alias(index, alias);
      update_lt(pp, index, 1);
    }
    pp->quality = 1;               /* Messung simulieren                */
  }

#ifdef LINKSMOD_LOCALMOD
  if (typ == LOCAL_N)
  {                                /* erste Messung simulieren          */
    if ((index = add_route(pp, id, 10)) != NO_INDEX)
    {
      update_alias(index, alias);
      update_lt(pp, index, 1);
    }
    pp->quality = 1;               /* Messung simulieren                */
  }

 if (typ == LOCAL_V)
  {                                /* erste Messung simulieren          */
    if ((index = add_route(pp, id, 10)) != NO_INDEX)
    {
      update_alias(index, alias);
      update_lt(pp, index, 1);
    }
    pp->quality = 1;               /* Messung simulieren                */
  }
#endif /* LINKSMOD_LOCALMOD */

  update_primary_peer(pp->l2link->call);

  return (pp);                     /* mit Pointer auf Eintrag zurueck   */
}

/************************************************************************/
/*                                                                      */
/* Eine neue Verbindung zum Nachbarn herstellen.                        */
/*                                                                      */
/************************************************************************/
BOOLEAN
connbr(PEER *pp)
{
  char id[L2IDLEN];
#if MAX_TRACE_LEVEL > 2
  char notify_call[10];
#endif

  cpyid(id, myid);
  if (pp->typ == LOCAL_M)
    id[L2IDLEN - 1] = (testid << 1) | 0x60;

#ifdef __WIN32__
  lnkpoi = getlnk((unsigned char)pp->l2link->port, id, pp->l2link->call, pp->l2link->digil);
#else
  lnkpoi = getlnk(pp->l2link->port, id, pp->l2link->call, pp->l2link->digil);
#endif /* WIN32 */

  if (lnkpoi)
  {                                 /* gabs noch einen freien Eintrag?  */
    if (lnkpoi->state == L2SDSCED)
    {
      newlnk();                     /* Verbindung erstellen             */
      pp->nbrl2l = lnkpoi;
      return (TRUE);
    }
#if MAX_TRACE_LEVEL > 2
    call2str(notify_call, pp->l2link->call);
    notify(3, "%s: dsclnk in connbr", notify_call);
#endif
    dsclnk();
  }
  return (FALSE);
}

/*----------------------------------------------------------------------*/
/* Den aktuellen Nachbarn als disconnected eintragen.                   */
/*----------------------------------------------------------------------*/
void
disnbr(PEER *pp)
{
  if (pp->nbrl2l == NULL)
    return;
  ptctab[g_uid(pp->nbrl2l, L2_USER)].state = 0;
  if (pp->nbrl2l)
  {
    pp->nbrl2l = NULL;
    dealml((LEHEAD *)&pp->rxfl);            /* nur rx loeschen, den     */
                                            /* Rest macht die Abmeldung */
  }
}

/*----------------------------------------------------------------------*/
/* Nachbarn in Nachbarliste suchen                                      */
/*----------------------------------------------------------------------*/
PEER *
getnei(const char *digil, const char *call, int port)
{
  PEER *pp;
  int   i;
  int   max_peers = netp->max_peers;

  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
    if (pp->used)
      if (isneig(digil, call, port, pp))
        return (pp);
  return (NULL);
}

/*----------------------------------------------------------------------*/
/* Stimmen Digipeaterliste (digis), Rufzeichen (call) und Port mit dem  */
/* Eintrag in die Nachbarnliste (nachp) ueberein?                       */
/* Rueckgabe: TRUE=ja,  FALSE=nein                                      */
/*----------------------------------------------------------------------*/
static BOOLEAN
isneig(const char *digis, const char *call, int port, PEER *pp)
{
  if (pp->l2link->port != port)
    return (FALSE);
  if (cmpid(pp->l2link->call, call) == FALSE)
    return (FALSE);
  if (cmpidl(digis, pp->l2link->digil) == FALSE)
    return (FALSE);

  return (TRUE);
}

/*----------------------------------------------------------------------*/
/* Ein Frame an einen Nachbarn senden.                                  */
void
toneig(PEER *pp, MBHEAD *mbp)
{
#ifdef THENETMOD
  if (pp->nbrl2l == NULL)                      /* Link ist nicht aktiv. */
    connbr(pp);        /* Eine neue Verbindung zum Nachbarn herstellen. */
#endif /* THENETMOD */

  if ((lnkpoi = pp->nbrl2l) != NULL)     /* L2-Link vorhanden?          */
    i3tolnk(L2CNETROM, pp->nbrl2l, mbp);
  else
    dealmb(mbp);                         /* sonst wegwerfen             */
}

/*----------------------------------------------------------------------*/
/* Den aktuellen Nachbarn als connected eintragen.                      */
/*----------------------------------------------------------------------*/
void
newnbr(PEER *pp)
{

  pp->nbrl2l = lnkpoi;                      /* Link eintragen           */

#ifdef THENETMOD
  if (pp->nbrl2l != NULL)                /* Nur wenn Segment aktiv ist. */
    ptctab[g_uid(pp->nbrl2l,L2_USER)].state = PEERLINK;
#else
  ptctab[g_uid(lnkpoi, L2_USER)].state = PEERLINK;
#endif /* THENETMOD */
}

/*----------------------------------------------------------------------*/
/* Ist der neue Link ein erlaubter Nachbar ?                            */
/* Wenn ja: Pointer auf die Nachbarstruktur zurueck, sonst NULL.        */
/*----------------------------------------------------------------------*/
PEER *
ispeer(void)
{
  L2LINK *p;
  PEER   *pp;
  int     i;
  int     max_peers = netp->max_peers;
  char   *srcid;

  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
    if (pp->used)
    {
      p = pp->l2link;             /* Pointer auf den eingetragenen Link */
      if (lnkpoi)
      {
        if (pp->nbrl2l == lnkpoi)
          return (pp);
        if (   !cmpid(p->call, lnkpoi->dstid)
            || !cmpidl(p->digil, lnkpoi->viaidl)
            || (p->port != lnkpoi->liport))
          continue;
      }
      else
      {
        if (   !cmpid(p->call, &rxfhdr[L2IDLEN])
            || !cmpidl(p->digil, &txfhdr[L2ILEN])
            || (rxfprt != p->port))
          continue;

        return ((pp->typ <= NETROM) ? pp : NULL);
      }

      srcid = (lnkpoi == NULL) ? rxfhdr : lnkpoi->srcid;

#ifndef LINKSMOD_LOCALMOD
      if (pp->typ != LOCAL)
#else
      if (pp->typ >= LOCAL)
#endif /* LINKSMOD_LOCALMOD */
      {
        if (cmpcal(myid, srcid))
        {
          if (pp->typ == LOCAL_M)
          {
            if (srcid[L2IDLEN - 1] == ((testid << 1) | 0x60))
              return (pp);
          }
          else
          {
            if ((srcid[L2IDLEN - 1] & 0x1E) == (myid[L2IDLEN - 1] & 0x1E))
              return (pp);
          }
        }
      }
    }
  return (NULL);
}

/************************************************************************\
*                                                                        *
* "rx frame to neigbour"                                                 *
*                                                                        *
* Ein L3-Frame wurde empfangen und wird hier an den entsprechenden       *
* Nachbarpointer zur weiteren verarbeitung angehaengt.                   *
*                                                                        *
\************************************************************************/
void
rxneig(PEER *pp, MBHEAD *fbp)
{
  relink((LEHEAD *)fbp, (LEHEAD *)pp->rxfl.tail); /* ok->weiter         */
}

#ifdef LINKSMOD_LOCALMOD
BOOLEAN CheckLocalLink(const char *call)
{
  PEER   *pp;
  int     i;
  int     max_peers = netp->max_peers;

  if (call[0] == FALSE)
    return(FALSE);

                                             /* Durchsuche die Segment-Liste. */
  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
  {
    if (pp->used)                     /* Nur benutze Eintraege interessieren. */
    {
      if (cmpid(pp->l2link->call, call))                    /* Callvergleich. */
      {
        if (  (pp->typ == LOCAL_N)
            ||(pp->typ == LOCAL_V))
          return(TRUE);                /* Ist eine LOCAL-Route "L-" oder "L#" */
      }
    } /* unbenutzter Eintrag. */
  } /* for */

  return(FALSE);                                /* Rufzeichen ist kein LOCAL. */
}
#endif /* LINKSMOD_LOCALMOD */

/* End of src/l3nbr.c */
