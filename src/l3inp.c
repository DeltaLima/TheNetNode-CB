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
/* File src/l3inp.c (maintained by: ???)                                */
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


static BOOLEAN update_options(INDEX, const char *, ULONG, UBYTE, MBHEAD *);
UWORD  max_lt = 30;                  /* maximaler HopCounter            */

/************************************************************************/
/*                                                                      */
/* Wenn sich die Info zu einem Node aendert, werden alle INP-Routes zur */
/* Aktualisierung ausgeschrieben.                                       */
/*                                                                      */
/************************************************************************/
void
propagate_node_update(INDEX index)
{
  PEER  *pp;
  ROUTE *rp;
  unsigned int max_peers = netp->max_peers;
  unsigned int i = 0;

  for (pp = netp->peertab; i < max_peers; i++, pp++)
  {
    if (!pp->used)                      /* nur benutzte Eintraege */
      continue;
    if (pp->typ != INP)                 /* nur INP-Links */
      continue;
    rp = pp->routes + index;            /* Zeiger auf Route */

    if (rp->reported_quality)           /* nur wenn wir gemeldet hatten */
      rp->reported_quality = DIRTY;
  }
}

/************************************************************************/
/*                                                                      */
/* Auswertung von empfangenen INP-Routinginformationen                  */
/*                                                                      */
/************************************************************************/
BOOLEAN
rx_inp_broadcast(PEER *rxpp, MBHEAD *mbp)
{
  char      desnod[L2IDLEN];    /* Callsign */
  char      beaide[L2CALEN],    /* Alias */
           *bp;

  UBYTE     uHopCounter;        /* Hops */
  UWORD     uTransportTime;     /* Laufzeit */
  UBYTE     uOptionCode;        /* Optionscode */
  UBYTE     uOptionLength;      /* Optionslaenge */
  UBYTE     uOptionData[256];   /* Daten der INP-Option */
  MBHEAD   *pOptions;           /* Optionszeiger */

  ULONG     uIP_Adr;            /* IP-Adresse */
  UBYTE     uBits;              /* Subnetz-Bits */

  int       i;
  UBYTE     ch;
  PEER     *pp;
  INDEX     index;

  BOOLEAN   bValid;             /* Eintrag verwendbar ? */

#if MAX_TRACE_LEVEL > 0
  char     notify_call1[10];
#if MAX_TRACE_LEVEL > 2
  char     notify_call2[10];
#endif
#endif

  if (*mbp->mbbp != INP_RIF)            /* Kennung INP-Routing-Paket?   */
    return (FALSE);         /* Nein, dann Puffer ungelesen zurueckgeben */

  getchr(mbp);                                 /* RIF-Kenner uebergehen */

  /* Ist noch mind. ein RIP (ohne Optionen) im RIF ? */
  /* (Call + Hop + Time + EOP = 7 + 1 + 2 + 1 = 11) */
  while ((mbp->mbpc - mbp->mbgc) > 10)
  {
    /* Nodeseintrag lesen ggf. incl. Alias, IP-Nr./Subnet und anderen   */
    /* INP-Options. Fehler werden zunaechst ignoriert.                  */
    bValid = TRUE;                              /* Eintrag gueltig      */
    cpyals(beaide, DONT_CHANGE_ALIAS);          /* Default: kein Alias  */
    uIP_Adr = 0L;                               /* und keine IP-Adresse */
    uBits = 0;                                  /* keine IP-Subnetzbits */
    pOptions = NULL;                            /* sowie keine Options  */
    uHopCounter = 0;                            /* Hop Counter          */
    uTransportTime = 0;                         /* Laufzeit             */
    memset(desnod, 0, sizeof(desnod));          /* Noch kein Zielknoten */

    if (!getfidc(desnod, mbp))                  /* Call holen           */
      bValid = FALSE;                           /* Eintrag ungueltig    */

    if (valcal(desnod) == ERRORS)               /* Call gueltig ?       */
      bValid = FALSE;                           /* Eintrag ungueltig    */

    uHopCounter = getchr(mbp);                  /* Hop-Counter lesen    */
    uTransportTime = get16(mbp);                /* Laufzeit lesen       */

    if (uHopCounter == 0)       /* HopCount 0 darf nie gesendet werden, */
      uHopCounter = DEFAULT_LT; /* korrigieren                          */
    else
      ++uHopCounter;            /* Hop-Counter erhoehen                 */

    /* Endekriterium fuer den Nodeseintrag ist das EOP-Byte (0x00)      */
    /* des RIP-Blocks. Danach koennen weitere Nodeseintraege folgen.    */
    while (mbp->mbpc > mbp->mbgc)
    {
      /* Einzelnen Options-Eintrag lesen und auswerten                  */
      /* Laenge der Option holen */
      uOptionLength = getchr(mbp);

      if (uOptionLength == INP_EOP)     /* EOP ? */
        break;                          /* alle Optionen gelesen */

      --uOptionLength;  /* Laengenbyte von der Optionslaenge abziehen   */

      /* Sind noch genug Zeichen da ? */
      if ((mbp->mbpc - mbp->mbgc) < uOptionLength)      /* INP-Fehler!  */
      {
        /* Der Fehler wird gemeldet, die fehlerhaften Options werden    */
        /* komplett verworfen, damit wir keinen Muell weitermelden.     */
#if MAX_TRACE_LEVEL > 0
        call2str(notify_call1, rxpp->l2link->call);
        notify(1, "INP: frame error received from %s, "
                  "len = %u, left = %u", notify_call1, uOptionLength,
                  (UWORD)(mbp->mbpc - mbp->mbgc));
#endif
        /* Unbekannte Optionen ? Dann diese loeschen */
        if (pOptions != NULL)
        {
          dealmb(pOptions);
          pOptions = NULL;
        }

        /* nicht zu gebrauchen */
        bValid = FALSE;

        break;
      }

      uOptionCode = getchr(mbp);    /* Optionstyp holen */
      --uOptionLength;  /* Typenfeld von der Optionslaenge abziehen */

      /* Daten der Option holen */
      memset(uOptionData, 0, sizeof(uOptionData));
      for (i = 0; i < uOptionLength; ++i)
         uOptionData[i] = getchr(mbp);

      /* Option bestimmen */
      switch (uOptionCode)
      {
        /* Alias */
        case INP_ALIAS:

          /* Laengencheck */
          if (uOptionLength > L2CALEN)
          {
#if MAX_TRACE_LEVEL > 2
              call2str(notify_call1, rxpp->l2link->call);
              notify(3, "alias from %s too long (cut)", notify_call1);
#endif
          }

          /* Alias kopieren und checken */
          for (i = 0, bp = beaide; (i < uOptionLength) && (i <= L2CALEN); ++i)
          {
            ch = uOptionData[i];

            /* Pruefung auf ungueltige Zeichen */
            if ((ch < ' ') || (ch > 127))
            {
#if MAX_TRACE_LEVEL > 2
              call2str(notify_call1, rxpp->l2link->call);
              notify(3, "invalid alias from %s (flushed)", notify_call1);
#endif
              /* Unbekannte Optionen loeschen wenn vorhanden */
              if (pOptions != NULL)
              {
                dealmb(pOptions);
                pOptions = NULL;
              }

              /* nicht zu gebrauchen */
              bValid = FALSE;
            }

            /* Zeichen gueltig, uebernehmen */
            *bp++ = ch;
          }

          /* Der Nachbar posaunt seine Secrets durch die Gegend ... */
          if (beaide[0] == '#')
          {
#if MAX_TRACE_LEVEL > 2
            call2str(notify_call1, rxpp->l2link->call);
            call2str(notify_call2, desnod);
            notify(3, "%s sent secret node %s", notify_call1, notify_call2);
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
              i = L2CALEN;        /* brauchen wir nicht mehr auffuellen */
#if MAX_TRACE_LEVEL > 2
              notify(3, "not real secret node, corrected");
#endif
            }
            else
            {
              /* Ein echter Secret Node, normal behandeln (loeschen) */
              if (pOptions != NULL)
              {
                dealmb(pOptions);
                pOptions = NULL;
              }
#if MAX_TRACE_LEVEL > 2
              notify(3, "secret node flushed");
#endif
              /* Eintrag ist nicht zu gebrauchen */
              bValid = FALSE;
            }
          }

          for (; i < L2CALEN; ++i)    /* auffuellen mit ' '           */
            *bp++ = ' ';

          break;

        /* IP-Adresse */
        case INP_IPA:

          /* Option lang genug ? */
          if (uOptionLength == 5)
          {
            /* IP-Adresse lesen */
            uIP_Adr  = uOptionData[0] << 24;
            uIP_Adr |= uOptionData[1] << 16;
            uIP_Adr |= uOptionData[2] << 8;
            uIP_Adr |= uOptionData[3];

            /* Hostbits lesen */
            uBits = uOptionData[4];
          }
          else
          {
#if MAX_TRACE_LEVEL > 2
            call2str(notify_call1, rxpp->l2link->call);
            notify(3, "invalid ipa-length %u from %s (flushed)", uOptionLength, notify_call1);
#endif
            if (pOptions != NULL)
            {
              dealmb(pOptions);
              pOptions = NULL;
            }

            bValid = FALSE;
          }

          /* Rudimentaerer Check der Subnetz-Bits, sind diese falsch */
          /* wird die IP fuer diesen Node nicht uebernommen, die IP  */
          /* wird an anderer Stelle je nach Einstellung ueberprueft  */
          if ((uBits == 0) || (uBits > 32))
          {
            uIP_Adr = 0L;
            uBits = 0;
#if MAX_TRACE_LEVEL > 2
            call2str(notify_call1, desnod);
            notify(3, "invalid subnet-bits for %s, ip not stored", notify_call1);
#endif
          }
          break;

        /* unbekannte Option - in Buffer speichern */
        default:
          /* Buffer besorgen wenn nicht schon geschehen */
          if (pOptions == NULL)
            pOptions = (MBHEAD *)allocb(ALLOC_INPOPT);

          /* Option speichern */
#ifdef __WIN32__
          putchr((char)(uOptionLength + 2), pOptions);  /* Laenge */
#else
          putchr(uOptionLength + 2, pOptions);  /* Laenge */
#endif /* WIN32 */
          putchr(uOptionCode, pOptions);                /* Typ */

          for (i = 0; i < uOptionLength; ++i)   /* Daten */
            putchr(uOptionData[i], pOptions);
      } /* switch (uOptionCode) */
    } /* while (mbp->mbpc > mbp->mbgc) */

    /* Hat der Nachbar mich selbst gemeldet ? */
    if (cmpid(myid, desnod) || iscall(desnod, NULL, NULL, VC))
    {
#if MAX_TRACE_LEVEL > 2
      call2str(notify_call1, myid);
      call2str(notify_call2, rxpp->l2link->call);
      notify(3, "destination %s received from %s (ignored)",
             notify_call1, notify_call2);
#endif
      if (pOptions != NULL)
      {
        dealmb(pOptions);
        pOptions = NULL;
      }
      continue;
    }

#if MAX_TRACE_LEVEL > 8
    call2str(notify_call1, rxpp->l2link->call);
    call2str(notify_call2, desnod);
    notify(9, "%-9.9s>%-6.6s:%-9.9s Time:%u Hops:%u Valid:%u",
           notify_call1, beaide, notify_call2, uTransportTime, uHopCounter,
           (bValid == TRUE ? 1 : 0));

    /* IP-Adresse */
    if (uIP_Adr != 0L)
      notify(9, "IP:%u.%u.%u.%u/%u",
              (uIP_Adr >> 24) & 0xFF,
              (uIP_Adr >> 16) & 0xFF,
              (uIP_Adr >> 8) & 0xFF,
              uIP_Adr & 0xFF,
              uBits);

    if (pOptions)
      notify(9, "unknown options: %u bytes", pOptions->mbpc);
#endif

    /* Erhaltenen Eintrag pruefen */
    if (uHopCounter > max_lt)           /* Nach Lifetime runterzwingen  */
    {
      uTransportTime = 0;               /* Knoten nicht annehmen        */
      bValid = FALSE;
    }

    if (cmpid(rxpp->l2link->call, desnod))      /* der Nachbar selbst   */
    {
      uTransportTime = 1;                  /* 10ms auf die Linklaufzeit */
      uHopCounter = 1;                        /* immer ein Hop entfernt */
    }

    /* Den Node in der Nodetabelle suchen, ist er schon vorhanden dann  */
    /* wird der Eintrag geupdated, ansonsten neu angelegt.              */
    if ((index = add_route(rxpp, desnod, uTransportTime)) != NO_INDEX)
    {
      /* Ziele mit diesen Laufzeiten sollen abgemeldet werden */
      if ((uTransportTime == 60000) || (uTransportTime == 0))
        update_lt(rxpp, index, 0);
      else
        /* alle anderen Laufzeiten normaler HopCounter */
        update_lt(rxpp, index, uHopCounter);

      /* Optionen nehmen wir nur vom primaeren Partner an, */
      /* haben wir noch keine, dann nehmen wir von jedem.  */
      /* Haben wir schon optionale Infos ueber den Node ?  */
      /* (Alias, IP-Adresse/Subnetzbits, sonstiges)        */
      if (   (!cmpcal(netp->nodetab[index].alias, nulide))
          || (netp->nodetab[index].ipa != 0L)
         )
      {
        if (find_best_qual(index, &pp, DG) > 0)     /* beste Qualitaet */
        {
          if (pp != rxpp)               /* bester Nachbar ist dieser ? */
          {
            /* Nein, nicht bester Nachbar */
            if (pOptions != NULL)   /* eventuelle Optionen loeschen    */
            {
              dealmb(pOptions);
              pOptions = NULL;
            }
            bValid = FALSE;
          }
        }
      }

      /* Der primaere Weg hat sich geaendert, wir uebernehmen Node-Info   */
      /* sofern vorhanden (Alias, IP-Nr., Options). Wird keine Zusatzinfo */
      /* gemeldet, bleibt die bisher bekannte Info erhalten.              */
      if (    (bValid == TRUE)
           && (update_options(index, beaide, uIP_Adr, uBits, pOptions))
         )
      {
        propagate_node_update(index);
        pOptions = NULL;
      }
    } /* if (index = add_route(...) ... )*/
  } /* while (...) bis Frameende */

  /* Wir waren erfolgreich */
  return (TRUE);
}

/************************************************************************/
/*                                                                      */
/* Uebernehmen neuer Zusatzinfos zu einem Node (Alias, IP-Nr./Subnet,   */
/* weitere unbekannte Options). Wird nichts gemeldet, bleiben bekannte  */
/* Daten gespeichert. Wird etwas neu gemeldet, muessen ALLE Daten neu   */
/* gemeldet werden. Alles was nicht mehr gemeldet wird, wird geloescht. */
/*                                                                      */
/************************************************************************/
static BOOLEAN
update_options(INDEX index, const char *alias,
               ULONG ip_adr, UBYTE bits, MBHEAD *options)
{
  NODE    *np;
  BOOLEAN  res = FALSE;
  BOOLEAN  ip_valid = TRUE;
  int      len;
  char     temp[1];
  ipaddr   host;

  temp[0] = 0;                         /* damit arp_add() zufrieden ist */

  if (   !cmpcal(alias, nulide)         /* nur wenn ueberhaupt INP-     */
      || ip_adr != 0L                   /* Options uebertragen wurden   */
      || options != NULL)               /* kann sich etwas aendern      */
  {
/* Es wurden Options uebertragen - also muessen alle Options auf den    */
/* neuen Wert gesetzt werden, oder geloescht, falls nicht mehr gemeldet */
    np = netp->nodetab + index;
    res = !cmpcal(np->alias, alias);
    cpyals(np->alias, alias);

    if (   ip_adr != np->ipa
        || bits != np->bits)
      res = TRUE;

/* ARP- und IPR-Eintrag erneuern                                        */
/* nur austragen wenn schon was eingetragen gewesen ist und die         */
/* automatische Modifikation erlaubt                                    */
    if (np->ipa != 0L && autoipr != 0)
    {
      arp_drop(np->ipa, NETROM_PORT, TRUE);
      rt_drop(np->ipa, np->bits, TRUE);
    }

/* neue Eintraege nur machen wenn nicht leer gemeldet und wir selber    */
/* eine IP-Adresse haben und wir das ueberhaupt duerfen (Param. 12)     */
    if (ip_adr != 0L && my_ip_addr != 0L && autoipr != 0)
    {
/* bevor wir die IP uebernehmen erst mal pruefen wenn wir das sollen    */
      if (autoipr >= 2)
      {
/* elementare Pruefung immer machen                                     */
        if (((ip_adr & 0xFF) == 0L) || ((ip_adr & 0xFF) == 0xFF))
          ip_valid = FALSE;

/* nur IPs uebernehmen, die im gleichen Netz sind                       */
        if (   (autoipr >= 3)
            && ((ip_adr & 0xFF000000L) != (my_ip_addr & 0xFF000000L)))
          ip_valid = FALSE;
      }

      if (ip_valid)
      {
        arp_add(ip_adr, NETROM_PORT, np->id, temp, 0, 0, FALSE, TRUE);
        host = ip_adr;
        host >>= (32 - bits);
        host <<= (32 - bits);
        rt_add(host, bits, ip_adr, NETROM_PORT, 0, 0, 0, TRUE);
      }
    }

    np->ipa = ip_adr;
    np->bits = bits;

    if (options == NULL)                /* keine unbekannten Options?   */
    {
      if (np->options != NULL)          /* war aber was bekannt?        */
      {
        dealmb(np->options);
        np->options = NULL;
        res = TRUE;
      }
      return (res);
    }

    if (np->options == NULL)            /* bisher nix bekannt           */
    {
      np->options = options;
      return (TRUE);
    }

    if ((len = options->mbpc) == np->options->mbpc)
    {
/* alte und neue Options sind gleich lang - also vergleichen, ob sich   */
/* etwas geaendert hat (wir gehen mal davon aus, dass die Reihenfolge   */
/* unveraendert bleibt)                                                 */
      rwndmb(options);
      rwndmb(np->options);
      while (--len >= 0)
      {
        if (getchr(options) != getchr(np->options))
          break;
      }

      if (len < 0)
      {
        dealmb(options);                  /* keine neuen Options          */
        return (res);
      }
    }
    dealmb(np->options);                  /* alte Options vergessen       */
    np->options = options;                /* neue merken                  */
    return (TRUE);                        /* Aenderung                    */
  }

  return (res);
}

/************************************************************************/
/*                                                                      */
/* "add internode protocol route information"                           */
/*                                                                      */
/* Einen Weg zu dem Routing-Info-Frame fuer einen Nachbarn hinzufuegen. */
/* Das Frame wird gesendet, sobald es voll ist oder in brosrv() wenn    */
/* der Timeout abgelaufen ist.                                          */
/*                                                                      */
/*----------------------------------------------------------------------*/
void
add_inp_info(MBHEAD **mbpp, NODE *node, PEER *topp, unsigned qualit,
             unsigned last_qualit, int lt)
{
  char buf[256],
       *bp,
       *bp2;
  int   len;

  bp = buf;

  /* Laufzeit 0 entspricht einer Abmeldung */
  if (qualit == 0)
    qualit = HORIZONT;

  *bp++ = lt;               /* Hops eintragen */
  *bp++ = qualit >> 8;      /* Laufzeit eintragen */
  *bp++ = qualit & 0xFF;

  /* Wurde dieser Node schon mal gemeldet ? Falls ja, dann    */
  /* die Optionen nicht noch einmal melden. Ausnahme ist nur, */
  /* wenn sich Daten geaendert haben.                         */
  if (   (qualit && last_qualit == 0)
      || last_qualit == DIRTY)
  {
    /* Laenge des Alias feststellen */
    for (len = 0; len < L2CALEN; ++len)
      if (node->alias[len] == ' ')
        break;

    /* Alias uebernehmen */
    if (len != 0)
    {
      *bp++ = len + 2;                      /* Laenge */
      *bp++ = INP_ALIAS;                    /* Typ    */

      memcpy(bp, node->alias, (size_t)len); /* Daten  */
      bp += len;
    }

    /* IP-Adresse und Subnetzbits uebernehmen */
    if (node->ipa != 0L)
    {
      *bp++ = 7;                            /* Laenge */
      *bp++ = INP_IPA;                      /* Typ    */

      /* IP-Adresse und Subnetzbits eintragen */
#ifdef __WIN32__
      *bp++ = (unsigned char)(node->ipa >> 24);
      *bp++ = (unsigned char)((node->ipa >> 16) & 0xff);
      *bp++ = (unsigned char)((node->ipa >> 8) & 0xff);
      *bp++ = (unsigned char)(node->ipa & 0xff);
#else
      *bp++ = node->ipa >> 24;
      *bp++ = (node->ipa >> 16) & 0xff;
      *bp++ = (node->ipa >> 8) & 0xff;
      *bp++ = node->ipa & 0xff;
#endif /* WIN32 */
      *bp++ = node->bits;
    }

    /* uns unbekannte Optionen anhaengen */
    if (node->options != NULL)
    {
      rwndmb(node->options);
      while (node->options->mbpc > node->options->mbgc)
        *bp++ = getchr(node->options);
    }
  }

  *bp++ = 0;                             /* Ende der Meldung (EOP)      */
  len = (int)(bp - buf) + L2IDLEN;       /* Laenge berechnen            */

  if (*mbpp)                             /* noch ein Buffer in Arbeit ? */
    if ((*mbpp)->mbpc + len > 256)       /* passt nicht mehr ?          */
      brosnd(mbpp, topp);                /* dann senden                 */

  /* war der Buffer voll so wurde er gesendet und entsorgt, */
  /* *mbpp ist jetzt NULL, das hat brosnd() erledigt        */

  if (*mbpp == NULL)                     /* noch kein Buffer besorgt    */
  {                                      /* neues I-Frame holen         */
    (*mbpp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l3_typ = L2CI;
#ifdef __WIN32__
    putchr((char)0xFF, *mbpp);                 /* Kennung (RIF)               */
#else
    putchr(0xFF, *mbpp);                 /* Kennung (RIF)               */
#endif /* WIN32 */
  }

  putfid(node->id, *mbpp);
  for (bp2 = buf; bp2 < bp; bp2++)
    putchr(*bp2, *mbpp);
}

/************************************************************************/
/*                                                                      */
/* "send own node's inp beacon"                                         */
/*                                                                      */
/* Den direkten Nachbarn unsere Daten per INP mitteilen, z.B. nach      */
/* einem Linkaufbau, oder wenn sich unsere Daten (IP-Adresse oder       */
/* Subnetzbits) aendern.                                                */
/*                                                                      */
/*----------------------------------------------------------------------*/
void send_inp_nodebeacon(PEER* pPeer)
{
  NODE    node;
  MBHEAD* inpmbp = NULL;

  /* nur INP-Nachbarn duerfen hier rein */
  if (pPeer->typ != INP)
    return;

  /* uns selber melden */
  cpyid(node.id, myid);                        /* unsere ID */
  cpyals(node.alias, alias);                   /* unser Alias */

  node.ipa = my_ip_addr;                       /* usere IP */
  node.bits = my_ip_bits;                      /* unsere Subnetzbits */
  node.options = NULL;                         /* keine Options */

  add_inp_info(&inpmbp, &node, pPeer, 1, 0, 1); /* INP-Frame erstellen */
  brosnd(&inpmbp, pPeer);                       /* und senden */
}

/* End of src/l3inp.c */
