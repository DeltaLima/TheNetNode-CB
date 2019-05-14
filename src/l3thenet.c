#include "tnn.h"

#ifdef THENETMOD
#include "l3local.h"

UWORD obcini                            /* Anfangswert Knotenlebensdauer      */
    = 60;

UWORD obcbro                      /* min Wert Restlebensdauer fuer Rundspruch */
    = 60;

/* L4TIMEOUT */
UWORD L4Timeout                           /* L4-Timeout, nur fuer THENET-Typ. */
    = 900;                                /* Wenn keine Aktivitaet wird       */
                                          /* der Link getrennt.               */
/* NOROUTE */
UWORD NoRoute                 /* Standardwert fuer # im Alias nicht zulassen. */
    = 0;


PARAM L4partab[] = {                            /* L4 Parameter Tabelle       */
    {&worqua,       0,    255,"Min-Quality"},   /* min Qualiatet Autoupdate   */
    {&obcini,       1,    255,"OBS_Init"},  /* Anfangswert Knotenlebensdauer. */
    {&obcbro,       1,    255,"OBS_Min"},        /* min Wert Restlebensdauer  */
                                                 /* fuer Rundspruch           */
    {&broint_ui,    1,    240,"Broadcast"},   /* Rundspruchintervall in 1min. */
    {&l4_beta3,     1,   1000,"L4-Busytim"},    /* BUSY/REQ-TIMEOUT           */
                                                /* (T3) = SRTT * BETA3        */
    /* L4TIMEOUT */
    {&L4Timeout,   60,  54000,"L4-Timeout"},  /* L4-Link no activity Timer.   */

    /* NOROUTE */
    {&NoRoute,      0,      1,"#Alias"},      /* 0 = Link-Nachbar mit # im  . */
                                              /*     Alias NICHT zulassen     */
                                              /* 1 = Link-Nachbar mit # im    */
                                              /*     Alias zulassen.          */
                                              /* Parameter 0 ist standard.    */
};

int L4partablen = (int)(sizeof(L4partab)/sizeof(PARAM));

static void add_nodes_info(MBHEAD **, NODE *, char *, PEER *, unsigned, UWORD);

/************************************************************************/
/*                                                                      */
/* Parameter anzeigen/aendern                                           */
/*                                                                      */
/************************************************************************/
void ccpL4par(void)
{
 ccp_par("L4Parms:\r", L4partab, L4partablen);
}


/************************************************************************/
/*                                                                      */
/* L4-Parameter auf Festplatte sichern.                                 */
/*                                                                      */
/************************************************************************/
void dump_l4parms(MBHEAD *mbp)
{
  UWORD  num;
  PARAM  *p;

  putstr(";\r; L4-Parameters\r;\r", mbp);

  for (p = L4partab, num = 1; num <= L4partablen; p++, num++)
  {
    if (  (p->paradr)
        &&(strncmp(p->parstr, "unu", 3)))
      putprintf(mbp, "L4PAR %s %u\r", p->parstr, *(p->paradr));
  }
}

/************************************************************************/
/*                                                                      */
/* L4-Parameter unter ROUTES aendern.                                   */
/*                                                                      */
/* z.Z. ist folgendes moeglich,                                          */
/* Qualitaet einer Route (nur THENET-TYP) aendern:                      */
/* R CB0GLA QUAL=10..255                                                */
/*                                                                      */
/* Connect-Status, 0 = wenn keine Aktivitaet, Verbindung trennen (X1J4).*/
/*                 1 = Verbindung wird gehalten (Dauerconnect z.b.XNET).*/
/* R CB0GLA CONSTATUS=0..1                                              */
/*                                                                      */
/************************************************************************/
BOOLEAN RoutesL4Para(MBHEAD *mbp, PEER *pp)
{
#define        L4_NONE      0
#define        L4_QUAL      1
#define        L4_CONSTATUS 2
#define        BUFLEN      64

  char         pBuf[BUFLEN + 1];
  char         call[10];
  unsigned int pCmd = L4_NONE;
  unsigned int quality   = 0;
  int          constatus = 0;
  register int i;

  if (  (issyso())                                         /* Nur Sysop darf. */
          &&(clicnt > 0))                               /* Parameter aendern. */
  {
    do
    {
      memset(pBuf, 0, sizeof(pBuf));                       /* Frischer Buffer */
      skipsp(&clicnt, &clipoi);                /* ggf. Leerzeichen entfernen. */

      for (i = 0; i < BUFLEN; ++i)
      {
        if (!clicnt)
          break;

        if (  (*clipoi == ' ')                 /* Leerzeichen im Lese-Buffer. */
            ||(*clipoi == '='))                /* Zeichen '=' im Lese-Buffer. */
        {
           clicnt--;                                     /* Zeichen loeschen. */
           clipoi++;
          break;                               /* Zeichen einlesen abbrechen. */
        }

        clicnt--;                          /* Zeichen im Lese-Buffer weniger. */
        pBuf[i] = toupper(*clipoi++);         /* Zeichen in Buffer schreiben. */
      }

      if (  (strcmp(pBuf, "QUAL") == 0)                  /* Qualitaet aendern */
          ||(pBuf[0] ==   'Q')
         )
        pCmd = L4_QUAL;                                      /* Modus setzen. */

      if (  (strcmp(pBuf, "CONSTATUS") == 0)       /* Connect-Status aendern. */
          ||(pBuf[0] ==   'C')
         )
        pCmd = L4_CONSTATUS;                                 /* Modus setzen. */


      switch (pCmd)                                /* L4-Parameter auswerten. */
      {
        case L4_QUAL :                                  /* Qualitaet aendern. */

          quality = nxtlong(&clicnt, &clipoi);    /* Ermittle neue Qualitaet. */

          if (  (quality > 9)                        /* Grenzbereich pruefen. */
              &&(quality < 256))
            pp->quality = quality;                  /* Neue Qualitaet setzen. */
          else                           /* Liegt ausserhalb im Grenzbereich. */
            {
              if (pp->quality < 1)          /* Nur wenn ungueltige Qualitaet, */
                pp->quality = worqua;           /* minimale Qualitaet setzen. */
            }

         break;


        case L4_CONSTATUS :                        /* Connect-Status aendert. */

          constatus = nxtlong(&clicnt, &clipoi);  /* Ermittle Connect-Status. */

          if (  (constatus >= 0)                     /* Grenzbereich pruefen. */
              &&(constatus <= 1))
            pp->constatus = constatus;        /* Neuer Connect-Status setzen. */

         break;

        default                                : /* Unbekannter L4-Parameter. */
         break;
      }
    } while (clicnt > 0);                /* evl. naechsten Paramter einlesen. */
  }
  else
    return(FALSE);

  call2str(call, pp->l2link->call);                            /* Rufzeichen. */
  mbp = putals("L4-Parameter:\r");                        /* Buffer besorgen. */

  putstr("--Call----Quality---ConStatus--\r", mbp);

  putprintf(mbp,"%-9s %-3d       %-1d\r",
            call,
            pp->quality,
            pp->constatus);

  prompt(mbp);           /* L4-Parameter wurde geaendert, zurueck zum Prompt. */
  seteom(mbp);
  return(TRUE);                         /* Kein Sysop oder keine Aenderungen. */
}

/************************************************************************/
/*                                                                      */
/* Hat man mehrerer THENET-Links auf den gleichen Port,                 */
/* wird in der Regel fuer jeden THENET-Partner eine                     */
/* Broadcast-Bake geschickt,was aber unsinn ist - ausge-                */
/* nommen auf AX25IP-Ports. Fuer die Restlichen THENET-                 */
/* Partner den Broadcast-Timer zuruecksetzen.                           */
/*                                                                      */
/************************************************************************/
void BroadcastBakeClear(PEER *pp)
{
  PEER *p;
  int   i;
  int   max_peers = netp->max_peers;

                                                 /* durchsuche alle Segmente. */
  for (i = 0, p = netp->peertab; i < max_peers; i++, p++)
  {
    if (p->used)                                    /* Nur benutze Eintraege. */
    {
      if (pp->l2link->port == p->l2link->port) /* Link auf den gleichen Port. */
        p->brotim = 0;                      /* Broadcast-Timer zuruecksetzen. */
    }
  }
}

/************************************************************************/
/*                                                                      */
/* Alterungs-Zaehler minimieren.                                        */
/* ggf Segment entfernen wenn Auto-Route.                               */
/*                                                                      */
/************************************************************************/
BOOLEAN OBSinitTimer(PEER *pp)
{
  if (pp->typ != THENET)                         /* Segment ist kein THENET.  */
    return(FALSE);                                 /* hat hier nix zu suchen. */

  if (pp->obscnt == 0)         /* Restlebensdauer fuer Rundspruch abgelaufen. */
  {
    update_peer_quality(pp, 0L, DONT_CHANGE_QUAL);
    memset(pp->routes, 0, netp->max_nodes * sizeof(ROUTE));
    pp->num_routes = 0;                       /* Routen loeschen              */
    drop_unreachable_nodes();                 /* Routes/Nodes loeschen        */

#ifdef AUTOROUTING
    if (pp->l2link->ppAuto == AUTO_ROUTE)     /* Segment ist eine Auto-Route. */
    {
#ifdef AXIPR_HTML
                                /* Protokoll fuer HTML-Ausgabe aktualisieren. */
      SetHTML(pp->l2link->port, pp->l2link->call, NULL, FALSE);
#endif /* AXIPR_HTML */

      unregister_neigb(pp->l2link->call,                /* Segment austragen. */
                       pp->l2link->digil,
                       pp->l2link->port);
      return(TRUE);                 /* Segment wurde geloescht, weiter sagen. */
    } /* keine auto-route. */
#endif /* AUTOROUTING */

  }
  else               /* Restlebensdauer fuer Rundspruch noch nicht abgelaufen */
    --pp->obscnt;      /* Restlebensdauer fuer Rundspruch um eins minimieren. */

  return(FALSE);                                    /* Segment weiterpruefen. */
}

/************************************************************************/
/*                                                                      */
/* Suche ein Segment mit Typ THENET auf den angegebenen Port.           */
/*                                                                      */
/************************************************************************/
static PEER *SearchSegmentTHENET(UWORD port)
{
  PEER *pp;
  int   i;

                                                 /* Durchsuche alle Segmente. */
  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
  {
    if (   (pp->used)                              /* Nur benutzte eintraege. */
        && (pp->typ == THENET)                         /* Routing-Typ THENET. */
        && (pp->l2link->port == port))             /* auf den gesuchten port. */
      return(pp);                                  /* Segment ist ein THENET. */
  }

  return(NULL);            /* Kein Segment oder nicht auf den angebenen Port. */
}

/************************************************************************/
/*                                                                      */
/* Broadcast-Nodes Bake erstellen und auf den angegebenen Port senden.  */
/*                                                                      */
/************************************************************************/
static void sendui(UWORD port, char *NODES, MBHEAD *mbp)
{
  sdui("",                                               /* an Level 2 senden */
       NODES,
       myid,                                /* Absender ist unser Knotencall. */
       (char)port,                         /* an den angegebenen Port senden. */
       mbp);                                                    /* Alle Noden */

  dealmb(mbp);                               /* Buffer wieder freigeben       */
}

/************************************************************************/
/*                                                                      */
/* Broadcast-Nodes Bake erstellen und auf den angegebenen Port senden.  */
/*                                                                      */
/************************************************************************/
void bcast(MBHEAD **mbpp, char *call, UWORD port)
{
  if (*mbpp == NULL)                         /* Noch keine Daten in der Bake. */
  {                                                       /* Buffer besorgen. */
    (*mbpp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l3_typ = L2CUI;

    if (mbpp == NULL)                                    /* Kein Buffer frei. */
      return;                                                   /* Abbrechen. */

    putchr((char)0xFF, *mbpp);
    pu6chr(alias, *mbpp);
  }

  rwndmb(*mbpp);                      /* vor dem Senden Buffer zurueckspulen  */
  (*mbpp)->l2fflg = L2CNETROM;        /* Als UI-Bake rausschicken an "NODES"  */

  sendui(port, call, *mbpp);        /* Bake auf den gewuenschten Port senden. */

  *mbpp = NULL;
}

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

/************************************************************************/
/*                                                                      */
/* Einen Weg zu dem Infocast-Frame fuer einen Nachbarn hinzufuegen.     */
/* Das Frame wird gesendet, sobald es voll ist oder in BroadCastBake()  */
/* wenn der Timeout abgelaufen ist.                                     */
/*                                                                      */
/************************************************************************/
static void add_nodes_info(MBHEAD **mbpp,
                            NODE    *node,
                            char    *call,
                            PEER    *viapp,
                            unsigned qualit,
                            UWORD    port)
{
  int qual = rtt2qual(qualit);

  /* schlechte Wege werden unterdrueckt, abmelden kennt NETROM nicht    */
  if (qual <= worqua)
    return;

  if (*mbpp)
  {
    if (((*mbpp)->mbpc + 21) > 256)            /* bis Frame voll              */
      bcast(mbpp,                                   /* Broadcast-Bake senden. */
            call,
            port);
  }

  if (*mbpp == NULL)
  {                                            /* neues Frame holen           */
    (*mbpp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l3_typ = L2CUI;
    putchr((char)0xFF, *mbpp);
    pu6chr(alias, *mbpp);
  }

  putfid(node->id, *mbpp);                     /* Call des Zieles             */
  pu6chr(node->alias, *mbpp);                  /* Ident des Zieles            */
  if (qualit && viapp)
    putfid(viapp->l2link->call, *mbpp);        /* Nachbar des Zieles          */
  else
    putfid(myid, *mbpp);                       /* Nachbar geloescht           */
  putchr((char)rtt2qual(qualit), *mbpp);
}

/************************************************************************/
/*                                                                      */
/* LOCAL-Route hinzufuegen fuer Broadcast-Nodes Bake.                   */
/*                                                                      */
/************************************************************************/
static void AddLocal(PEER *pp, MBHEAD **mbp, UWORD port, char *call)
{
  PEER        *Segm;
  PEER        *BestSegm;
  NODE        *np;
  INDEX        index;
  register int i;
  unsigned int qual = 0;

                                                 /* Durchsuche alle Segmente. */
  for (i = 0, Segm = netp->peertab; i < max_peers; i++, Segm++)
  {
    if (!Segm->used)                                    /* Wenn kein Eintrag, */
      continue;                                     /* zum naechsten Segment. */

    if (  (Segm->typ != LOCAL)                                /* Local-Route. */
        &&(Segm->typ != LOCAL_M))                 /* Local-Route mit Messung. */
      continue;

                                                       /* Ermittle den index. */
    if ((index = find_node_this_ssid(Segm->l2link->call)) != NO_INDEX)
    {
      np = &netp->nodetab[index];                 /* Nodes-Eintrag ermitteln. */
      qual = find_best_qual(index,                    /* Qualitaet ermitteln. */
                            &BestSegm,
                            OPTIONS_MASK);

      if ((BestSegm == NULL) || (qual == 0))     /* geloeschte Nodes abfangen */
        continue;

      if (np->alias[0] == '#')                           /* versteckter Node. */
        continue;

      add_nodes_info(mbp,                        /* Local-Route hinzufuegen.  */
                     np,
                     call,
                     BestSegm,
                     qual,
                     port);
      continue;                                     /* zum naechsten Eintrag. */
    }
  } /* for */
}

/************************************************************************/
/*                                                                      */
/* Nodes hinzufuegen fuer Broadcast-Nodes Bake.                         */
/*                                                                      */
/************************************************************************/
static void AddNodes(char *ppcall, UWORD port, MBHEAD **mbp)
{
  PEER        *pp,
              *bestpp;
  NODE        *np;
  INDEX        index;
  BOOLEAN      addlocal = FALSE;
  register int i;
  unsigned int qual = 0;

                                                 /* Durchsuche alle Segmente. */
  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
  {
    if (!pp->used)                                      /* Wenn kein Eintrag, */
      continue;                                     /* zum naechsten Segment. */

    if (addlocal == FALSE)         /* LOCAL-Routen sind noch nicht behandelt. */
    {
      AddLocal(pp, mbp, port, ppcall);           /* LOCAL-Routen hinzufuegen. */
      addlocal = TRUE;                     /* LOCAL-Routen sind abgearbeitet. */
    }

    if (pp->typ != THENET)                        /* Kein Routing-Typ THENET. */
      continue;                                     /* Zum naechsten Segment. */

    for (np = (NODE *)netp->nodelis.head;
         np != (NODE *)&netp->nodelis;    /* sortierte Nodes-Liste durchgehen */
         np = np->next)
    {
      if (np->id[0])                  /* nur benutzte Eintraege interessieren */
      {
        index = (INDEX)(np - netp->nodetab);          /* Index berechnen      */
        qual = find_best_qual(index,                  /* Qualitaet ermitteln. */
                              &bestpp,
                              OPTIONS_MASK);

        if ((bestpp == NULL) || (qual == 0))     /* geloeschte Nodes abfangen */
          continue;                                 /* Zum naechsten Eintrag. */

        if (cmpid(bestpp->l2link->call, pp->l2link->call))   /* Callvergleich */
        {
          int    Timeout;
          ROUTE *rp = bestpp->routes + index;  /* Zeiger auf die Route. */

          if (np->alias[0] == '#')                       /* versteckter Node. */
            continue;

          Timeout = rp->timeout;                        /* Timeout ermitteln. */

          if (Timeout >= (obcini / 3))        /* mindestens 30% des Timeouts. */
            add_nodes_info(mbp,                         /* Node hinzufuegen.  */
                           np,
                           ppcall,
                           bestpp,
                           qual,
                           port);
          continue;                                 /* Zum naechsten Eintrag. */
        } /* Call war nicht gleich. */
      } /* nicht benutzer Eintrag. */
    } /* for NODE */
  } /* for PEER */
}

#ifdef AX25IP
/************************************************************************/
/*                                                                      */
/* Sende Broadcast-Nodes Bake zu jedem Segment mit Routing-TYP THENET.  */
/* Gilt nur fuer AX25IP-Port.                                           */
/*                                                                      */
/************************************************************************/
void bcastAXIP(void)
{
  PEER *pp;
  int   i;
                                                 /* Durchsuche alle Segmente. */
  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
  {
    if (   (pp->used)                              /* Nur benutzte eintraege. */
        && (pp->typ == THENET)                         /* Routing-Typ THENET. */
        && (kissmode(pp->l2link->port) == KISS_AXIP))       /* Portvergleich. */
    {
      MBHEAD *mbp = NUL;

      if (pp->obscnt)                    /* Wenn ein Link zum Nachbarn steht, */
        AddNodes(pp->l2link->call,                 /* ggf. Nodes hinzufuegen. */
                 pp->l2link->port,
                 &mbp);

      bcast(&mbp,                                   /* Broadcast-Bake senden. */
            pp->l2link->call,
            pp->l2link->port);

      continue;                                     /* Zum naechsten Segment. */
    }
  }
}

/************************************************************************/
/*                                                                      */
/* Wenn der Timer abgelaufen, ggf. Broadcast-Bake senden.               */
/* Gilt nur fuer AX25IP-Port.                                           */
/*                                                                      */
/************************************************************************/
void BroadCastBakeAXIP(void)
{
  PORTINFO *p;
  UWORD     port;

                                                   /* Alle Port's durchgehen. */
  for (port = 0, p = portpar; port < L2PNUM; p++, port++)
  {
    if (!portenabled(port))                          /* Port ist nicht aktiv. */
      continue;                                        /* Zum naechsten Port. */

#ifdef AX25IP
    if (kissmode(port) != KISS_AXIP)            /* Port ist kein AX25IP-Port, */
      continue;                                        /* zum naechsten Port. */
#endif /* AX25IP */

    if (++p->broadcast >= broint_ui)                     /* Timer abgelaufen. */
    {
      bcastAXIP();                             /* ggf. Broadcast-Bake senden. */

      p->broadcast = 0;                              /* Timer zurueck setzen. */
    } /* Timer noch nicht abgelaufen. */
  }
}
#endif /* AX25IP */

/************************************************************************/
/*                                                                      */
/* Broadcast-Nodes Bake senden.                                         */
/*                                                                      */
/************************************************************************/
void BroadCastBake(void)
{
  PORTINFO *p;
  PEER     *pp;
  UWORD     port;

#ifdef AX25IP
  BroadCastBakeAXIP();
#endif /* AX25IP */
                                           /* Durchsuche alle aktiven Port's. */
  for (port = 0, p = portpar; port < L2PNUM; p++, port++)
  {
    if (!portenabled(port))                          /* Port ist nicht aktiv. */
      continue;                                        /* Zum naechsten Port. */

#ifdef AX25IP
    if (kissmode(port) == KISS_AXIP)    /* AX25IP-Port hat hier nix zu suchen */
      continue;                                        /* Zum naechsten Port. */
#endif /* AX25IP */

#ifdef L1TCPIP
      /* Pruefe, ob Port ein TCPIP-Interface ist. */
      if (CheckPortTCP(port))
        /* Zum naechsten Eintrag. */
        continue;
#endif

    if (++p->broadcast >= broint_ui)                 /* Timer ist abgelaufen. */
    {
      MBHEAD *mbp = NUL;

                                              /* Suche auf den Port Segmente. */
      if ((pp = SearchSegmentTHENET(port)) != NULL)
      {
#ifdef AUTOROUTING
        if (  (p->poAuto != L_THENET)         /* kein THENET-TYP gesetzt bzw. */
            &&(p->poAuto != L_INPFLEX))            /* kein Vollmodus gestezt. */
        {
          AddNodes(pp->l2link->call,              /* ggf. Nodes hin zufuegen. */
                   port,
                   &mbp);
          bcast(&mbp,                              /* Bake an to CALL senden. */
          pp->l2link->call,
          port);

          p->broadcast = 0;                           /* Timer zuruecksetzen. */
          continue;                                    /* Zum naechsten Port. */
        }
        else                            /* THENET-TYP bzw. Vollmodus gesetzt. */
#endif /* AUTOROUTING */
          {
          AddNodes("NODES \140",                  /* ggf. Nodes hin zufuegen. */
                   port,
                   &mbp);
          bcast(&mbp,                             /* Bake an to NODES senden. */
                "NODES \140",
                port);

          p->broadcast = 0;                           /* Timer zuruecksetzen. */
          continue;                                    /* Zum naechsten Port. */
          }
      }
      else                                          /* kein Segment gefunden. */
        {
#ifdef AUTOROUTING
          if (  (p->poAuto == L_THENET)   /* Nur wenn THENET-TYP gesetzt bzw. */
              ||(p->poAuto == L_INPFLEX))               /* Vollmodus gestezt. */
          {
#endif /* AUTOROUTING */
            AddNodes("NODES \140",                /* ggf. Nodes hin zufuegen. */
                     port,
                     &mbp);
            bcast(&mbp,                           /* BAKE an to NODES senden. */
                  "NODES \140",
                  port);

            p->broadcast = 0;                        /* Timer zurueck setzen. */
            continue;                                  /* Zum naechsten Port. */
#ifdef AUTOROUTING
          }
#endif /* AUTOROUTING */
        } /* kein Segment gefunden. */
    } /* Timer noch nicht abgelaufen. */
  } /* for */
}


/************************************************************************/
/*                                                                      */
/* Suche Segment mit THENET-Typ.                                        */
/*                                                                      */
/* L4TIMEOUT                                                            */
/************************************************************************/
PEER *SearchTHENET(void)
{
  PEER *pp;

  if ((pp = ispeer()) != NULL)        /* Aktueller Linkblock ist ein Segment. */
  {
    if (pp->typ == THENET)                     /* Segment ist ein THENET-Typ. */
      return(pp);                                         /* THENET gefunden. */
  }
                                              /* Aktueller Linkblock ist KEIN */
  return(NULL);                               /*  Segment oder THENET-Typ.    */
}

/************************************************************************/
/*                                                                      */
/* Wenn THENET-Typ, L4-Timeout setzen, ansonsten L2-Timeout.            */
/*                                                                      */
/* L4TIMEOUT                                                            */
/************************************************************************/
unsigned short SetL4Timeout(void)
{
  PEER *pp;

  if ((pp = SearchTHENET()) != NULL) /* Suche nach THENET-Typ. Wenn gefunden, */
  {
    if (pp->constatus == FALSE)       /* Der Link soll nicht getrennt werden. */
      return(ininat);                                   /* L2-Timeout setzen. */
    else                  /* Link soll bei keiner Aktivitaet getrennt werden. */
      return(L4Timeout);                                /* L4-Timeout setzen. */
  }

  return(ininat);                              /* anonsten L2-Timeout setzen. */
}

/************************************************************************/
/*                                                                      */
/* Qualitaet einer Route (nur THENET) speichern.           .            */
/*                                                                      */
/* L4QUALI                                                              */
/************************************************************************/
void dump_routes(MBHEAD *mbp)
{
  PEER   *pp;
  int     i;
  int     max_peers = netp->max_peers;

  putstr(";\r; Routes\r;\r", mbp);

                                                 /* durchsuche alle Segmente. */
  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
  {
    if (pp->used)                                  /* Nur benutzte Eintraege. */
    {
      if (pp->typ == THENET)                           /* Nur vom Typ THENET. */
      {
        putprintf(mbp,"R ");
        putid(pp->l2link->call, mbp);                          /* Rufzeichen. */
        putprintf(mbp," QUAL=%d CONSTATUS=%d\r",
                  pp->quality,                                  /* Qualitaet. */
                  pp->constatus);                          /* Connect-Status. */
      } /* kein THENET. */
    } /* unbenutzer Eintrag. */
  } /* for */
}

#endif /* THENETMOD */

 /* End of src/l3thenet.c */
