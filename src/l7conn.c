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
/* File src/l7conn.c (maintained by: DF6LN)                             */
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

static char     usrvia[2 * L2VLEN + L2IDLEN + 1];

static BOOLEAN  is_cq(const char *, DEST *);
static WORD     conprm2(DEST *, DEST *);
static WORD     conprm(WORD *, char **, DEST *);
static void     conerr(int, DEST *);
static void     setupmsg(DEST *);
static void     setptc(void *, UBYTE);
static void     conhst(void);
static void     conl4(DEST *);
static void     conl2(DEST *);
static void     con_cq(DEST *);
static void     conusr(int, DEST *);

#define CP_L2CON   2
#define CP_L4CON   4
#define CP_HOSTCON 7
#define CP_CQ      42

#define CP_PARERR  10
#define CP_NODIG   11
#define CP_BADQUAL 12
#define CP_INTERN  13
#define CP_UNKNOWN 14

#define NOPORT     (-1)

/************************************************************************/
/* Schaun, ob wir ein Call in der MH-Liste haben und dest fuellen.      */
/*----------------------------------------------------------------------*/
BOOLEAN
isheard(char *id, DEST *dest)
{
  MHEARD         *mhp;
  BOOLEAN         exact = FALSE;
  ULONG           heard = 0L;

  /* Es wird die L2-MHeardliste von hinten durchsucht */
  for (mhp = (MHEARD *)l2heard.heardl.tail;
       mhp != (MHEARD *)&l2heard.heardl;
       mhp = mhp->prev)
  {
    if (cmpcal(id, mhp->id))                    /* Call in MH-Liste     */
    {
      if (cmpid(id, mhp->id))                   /* SSID auch passend ?  */
        exact = TRUE;
      else
      {
        /* Gefundener Eintrag ist aelter als gespeicherter Eintrag */
#ifndef __WIN32__
        if (mhp->heard < heard)
#else
        if ((ULONG)mhp->heard < heard)
#endif /* WIN32 */
          continue;
        else
          heard = mhp->heard;
      }

      dest->port = mhp->port;                   /* Port aus MH          */
#ifdef EAX25
      dest->eax = mhp->eax_link;
#endif
      dest->typ = 'U';
      *dest->via = 0;

      if (exact == TRUE)             /* wenn SSID auch passt, fertig!   */
        return (TRUE);
    }
  }

  /* Nichts exaktes gefunden */
  return (FALSE);
}

#ifdef EAX25
/* Ermittelt fuer eine Port/Call-Kombination die Verwendung von EAX.25 */
BOOLEAN useEAX(char* call, WORD port)
{
  DEST tmpDest;

  /* Zuerst die Porteinstellungen ansehen */
  if (port != NOPORT)
  {
    switch (portpar[port].eax_behaviour)
    {
      case 0: return (FALSE);
              break;

      case 1:
      case 2: if (isheard(call, &tmpDest) == TRUE)
                return (tmpDest.eax);
              else
                return (FALSE);
              break;

      case 3: return (TRUE);
              break;

      default: break;
    }
  }

  return (FALSE);
}
#endif

/************************************************************************/
/*                                                                      */
/* Feststellen, ob das gewuenschte Call ein CQ-Rufer ist.               */
/*                                                                      */
/************************************************************************/
static BOOLEAN
is_cq(const char *id, DEST *dest)
{
  CQBUF          *cqp;

  for (cqp = (CQBUF *)cq_user.head;             /* CQ-Liste durchsuchen */
       cqp != (CQBUF *)&cq_user;
       cqp = cqp->next)
  {
    if (cmpid(id, cqp->id))                     /* Call stimmt?         */
    {
      dest->uid = cqp->uid;                     /* UID eintragen        */
      cpyid(dest->call, id);                    /* Call auch            */
      dealoc((MBHEAD *)ulink((LEHEAD *)cqp));   /* CQ-Eintrag loeschen  */
      return (TRUE);
    }
  }
  return (FALSE);
}

/************************************************************************/
/*                                                                      */
/* Connectwunsch bewerten und analysieren                               */
/*                                                                      */
/* user enthaelt die Daten des Connectwunsches:                         */
/*  call    enthaelt das Ziel                                           */
/*  via     enthaelt den Repeaterpfad (optional)                        */
/*  port    enthaelt den Sendeport    (optional)                        */
/*                                                                      */
/* dest nimmt die Ergebnisse der Analysen auf. dest->call muss immer    */
/* gesetzt werden, damit dem User eine Fehlermeldung mit Rufzeichen     */
/* angezeigt werden kann !                                              */
/*                                                                      */
/************************************************************************/
static WORD
conprm2(DEST *user, DEST *dest)
{
  int    l3route;                  /* L3-Erreichbarkeit             */
  char*  call;
  char*  tmp;

  /* rudimentaere Pruefung, ohne Ziel gehts nicht */
  if (user->call[0] == 0)
    return (CP_PARERR);         /* Parameter-Fehler                 */

  /* Connect an Hostkonsole ? */
  if (cmpid(user->call, hostid))
    return (CP_HOSTCON);        /* Connect geht an Host             */

  /* Haben wir Repeater ? */
  if (user->via[0] != 0)
  { /* Ja, haben wir */
/*    printf("\nmit VIA\n"); */
    tmp = (char*)ndigipt(user->via);

    /* keiner mehr nach uns */
    if (*tmp == 0)
      call = user->call;
    else
      call = tmp;
  }
  else
  {
    /* Nein, keine Repeater */
/*    printf("\nohne VIA\n"); */
    call = user->call;

    /* gleich auf CQ-User pruefen */
    if (is_cq(call, dest))
      return (CP_CQ);
  }

#ifdef OS_IPLINK
  /* Ist es ein IPConv-Rufzeichen. */
  if (IPConvIS(user->call, dest))
    return(CP_IPCONV);
#endif /* OS_IPLINK */

  /* Eine passende Route suchen */
  switch (l3route = l3_find_route(call, dest))
  {
    /* Ziel unerreichbar */
    case NODE_DOWN:
      /* Wenn der Node ausgefallen ist, duerfen auch User einen Umweg   */
      /* probieren wenn sie den Port angeben.                           */

/*      printf("\nNODE_DOWN\n"); */

      cpyid(dest->call, user->call);

      if (user->via[0] != 0)
        cpyidl(dest->via, user->via);

      /* Ohne Port gibt es einen Fehler */
      if (user->port == NOPORT)
      {
/*        printf("\nkein port angegeben\n"); */
        return (CP_BADQUAL);
      }

      dest->typ = 'U';  /* User */
      dest->port = user->port;
#ifdef EAX25
      dest->eax = useEAX(dest->call, dest->port);
#endif
      return (CP_L2CON);
      break;

    /* Ziel erreichbar */
    case NODE_AVAILABLE:
/*      printf("\nNODE_AVAILABLE\n"); */

      cpyid(dest->call, user->call);

#ifdef CONNECTMOD_GOPORT
      /* User hat ein Port angegeben, */
      /* dann gehen wir ueber L2.     */
      if (user->port != NOPORT)
      {
        dest->typ = 'U';  /* User */
        dest->port = user->port;
        cpyidl(dest->via, user->via);
#ifdef EAX25
        dest->eax = useEAX(dest->call, dest->port);
#endif
        return (CP_L2CON);
      }
#endif /* CONNECTMOD_GOPORT */

      /* Geht der Weg ueber NETROM-Transport ? (N- oder I-Link) */
      if (dest->typ <= NETROM)
      {
        if (call == user->via)
        {
/*          printf("\nVIA im L2 oder explizit L2 !!!\n"); */
          if (user->via[0] != 0)
            cpyidl(dest->via, user->via);

          dest->typ = 'U';
#ifdef EAX25
          dest->eax = useEAX(dest->call, dest->port);
#endif
          return (CP_L2CON);
        }

        if (call == user->call)
        {
/*          printf("\nL4 CONNECT\n"); */
          return (CP_L4CON);
        }
      }
      /* Weg geht ueber L2 (F- oder L-Link) weiter */
      if (user->via[0] != 0)
        cpyidl(dest->via, user->via);

/*      printf("\nL2-connect\n"); */
#ifdef EAX25
      dest->eax = useEAX(call, dest->port);
#endif

      return (CP_L2CON);
      break;

    /* Ziel unbekannt */
    case NODE_UNKNOWN:
/*      printf("\nNODE_UNKNOWN\n"); */
      break; /* Durchfallen zum MHeard-Check */

    default:
      break;
  }

  /* Naechstes Ziel ist kein Nachbar und nicht im Routing */
  /* Ist das Call in MHeard ? */
  if (isheard(call, dest) == TRUE)
  {
    /* Ja */
/*    printf("\nin MH\n"); */

    cpyid(dest->call, user->call);

    if (user->via[0] != 0)
      cpyidl(dest->via, user->via);

    /* Hat der User einen Port angegeben, dann nehmen wir den und */
    /* nicht den, den MHeard gespeichert hat. */

    if (user->port != NOPORT)
      dest->port = user->port;

    dest->typ = 'U';
    return (CP_L2CON);
  }
  else
  {
/*    printf("\nNICHT in MH\n"); */

    if (user->port == NOPORT)
    {
/*      printf("\nkein userport angegeben\n"); */

      cpyid(dest->call, user->call);

      if (user->via[0] != 0)
        cpyidl(dest->via, user->via);

      return (CP_UNKNOWN);
    }
    else /* User hat einen Port angegeben, weiter. */
    {
/*      printf("\nuserport angegeben\n"); */
    }
  }

  if (user->port != NOPORT)
  {
    dest->port = user->port;
    dest->via[0] = '\0';
    dest->typ = 'U';
  }

  if (user->via[0] != 0)
  {
    cpyidl(dest->via, user->via);
    dest->typ = 'U';
  }

  cpyid(dest->call, user->call);

#ifdef EAX25
  dest->eax = useEAX(user->call, user->port);
#endif

  /* Per L2 zum Ziel */
  return (CP_L2CON);
}

/************************************************************************/
/*                                                                      */
/* Parameter fuer den Connect-Befehl auswerten und in dest speichern.   */
/* conprm() erhaelt eine Kopie von clipoi/clicnt und arbeitet damit.    */
/*                                                                      */
/************************************************************************/
static WORD
conprm(WORD *n,                 /* Restlaenge der Parameterzeile        */
       char **p,                /* Parameter des Connectbefehls         */
       DEST *dest)
{
  int             index;
  char            ident[L2CALEN];
  DEST            user;

  user.typ = 'U';
  user.call[0] = 0;
  user.port = NOPORT;           /* Port setzen                          */
  user.via[0] = 0;

  if (!*n)
  {
    /* Keine Parameter - Verbindung zur Konsole gewuenscht              */
    return (CP_HOSTCON);
  }

  /* TEST DG9OBU */
  /* Ist das Ziel ein Call ? Beim Sysop ist das egal, der darf alles    */
  if (getcal(n, p, issyso() ? FALSE : TRUE, user.call) != YES)
  {
/*    printf("\nein alias ?\n"); */

    /* Connect-Parameter ist kein Rufzeichen - ist es ein ALIAS?        */
    if (getide(n, p, ident) != ERRORS)
    {
/*      printf("pruefe auf alias !\n"); */

      /* Gueltiger ALIAS eingegeben - in der Nodes-Liste suchen         */
      if ((index = find_alias(ident)) != -1)
      {
/*        printf("gefunden !\n"); */

        /* ALIAS in der Nodesliste gefunden, Call kopieren              */
        cpyid(user.call, netp->nodetab[index].id);
      }
    }
    else
    {
/*
    else
      printf("kein alias\n");
*/
    }
  }

  getdig(n, p, TRUE, user.via); /* Digi-Liste uebernehmen               */
  getport(n, p, &user.port);    /* Port-Angabe geht auch am Ende        */

#ifdef PACSAT
  if (cmpid(user.call, pacsatid))
  {
    ccpbox();
    return (CP_INTERN);
  }
  else
#endif
    return (conprm2(&user, dest));
}

#define CE_HTFULL  0
#define CE_CTFULL  1
#define CE_LTFULL  2
#define CE_BADQUAL 3
#define CE_SUSPEND 4
#define CE_BUSY    5
#define CE_PORTOFF 6
#define CE_INVCAL  7
#define CE_UNKNOWN 8
#define CE_INVLEN  9
#define CE_NODIG   10

/************************************************************************/
/*                                                                      */
/* (xxx table full) Fehler ausgeben und Fernconnects ablehnen.          */
/*                                                                      */
/************************************************************************/
static void
conerr(int error, DEST *dest)
{
  static const char *fullerr[] = {"Host",
                                  "Circuit",
                                  "Link"};
  static const char *failerr[] = {" (not available)",
                                  " (Callsign is restricted)"};
  static const char *busyerr[] = {" (already connected)"};
  MBHEAD            *mbp;
  int                port;
  char               dstcal[10];

  if (ptctab[userpo->uid].local == PTC_LOCAL)
  {
    switch (error)
    {
      case CE_HTFULL:
      case CE_BUSY:
        if (g_utyp(userpo->uid) == L2_USER)
          rejlnk(g_ulink(userpo->uid));                 /* durchfallen  */
      default:
        disusr(userpo->uid);
    }
  }
  else
  {
    switch (error)
    {
      case CE_HTFULL:
      case CE_CTFULL:
      case CE_LTFULL:
        puttfu(fullerr[error]);
        return;

      case CE_BADQUAL:
      case CE_SUSPEND:
        mbp = putals(failmsg);
        putid(dest->call, mbp);
        putstr(failerr[error - CE_BADQUAL], mbp);
        break;

          case CE_BUSY:
        mbp = putals(dmmsg);
        putid(dest->call, mbp);
        putstr(busyerr[error - CE_BUSY], mbp);
        break;

      case CE_PORTOFF:
        call2str(dstcal, (dest->via[0] != 0 ? dest->via : dest->call));
        mbp = putals("Port not in use\r");
        putprintf(mbp, "(Routing shows %s reachable on port %u, but "
                       "port is disabled)", dstcal, dest->port);
        break;

      case CE_INVCAL:
        mbp = putals(invcalmsg);
        break;

      case CE_UNKNOWN:
        call2str(dstcal, (dest->via[0] != 0 ? dest->via : dest->call));
        mbp = putals(failmsg);
#ifdef SPEECH
        putprintf(mbp, speech_message(239), dstcal, dstcal);
#else
        putprintf(mbp, "%s\rNode / User unknown! If %s is a L2-user, "
                       "try one of these commands:", dstcal, dstcal);
#endif
        for (port = 0; port < L2PNUM; port++)
        {
        if (  (portenabled(port) && updmheard(port))
#ifdef L1TCPIP
            /* Keine TCPIP-Interface auflisten. */
            &&(!CheckPortTCP((UWORD)port))
#endif /* L1TCPIP */
           )
            putprintf(mbp, "\rCONNECT %s %s", dstcal, portpar[port].name);
        }
        break;

      case CE_INVLEN:
        mbp = putals("Too many repeaters (max. 8 repeaters)");

      case CE_NODIG:
        mbp = putals("Can't route");
        break;

#ifdef L1TCPIP
      case CE_TCPIP:
        mbp = putals(failmsg);
        putid(dest->call, mbp);
        putstr(" (Port is suspended!)", mbp);
        break;
#endif /* L1TCPIP */

      default:
        mbp = putals("Can't connect");
        break;
    }
    putchr(CR, mbp);
    prompt(mbp);
    seteom(mbp);
  }
}

/************************************************************************/
/*                                                                      */
/* Nachricht ueber den Linkaufbau ausgeben.                             */
/*                                                                      */
/************************************************************************/
static void
setupmsg(DEST *dest)
{
  MBHEAD         *mbp;

  if (ptctab[userpo->uid].local == PTC_NORMAL)
  {
    switch (dest->typ)
    {
        case NETROM:
        case TNN:
        case INP:
        case THENET:
          mbp = putals("Interlink setup (");
          putstr(portpar[dest->port].name, mbp);
          break;

        case FLEXNET:
          mbp = putals("Interlink setup (via ");
          putid(dest->nbrcal, mbp);
          break;

        case LOCAL_M:
          mbp = putals("Link setup (");
          putstr(portpar[dest->port].name, mbp);
          break;

        case LOCAL:
#ifdef LINKSMOD_LOCALMOD
        case LOCAL_N:
        case LOCAL_V:
#endif /* LINKSMOD_LOCALMOD */
          mbp = putals("Local setup (");
          putstr(portpar[dest->port].name, mbp);
          break;

        default:
          mbp = putals("Downlink setup (");
          putstr(portpar[dest->port].name, mbp);
          putstr(") ...\r", mbp);
          /*
           * Bei einem Downlink soll keine LOOP-Warnung angezeigt werden.
           */
          seteom(mbp);
        return;
        }

    putstr(") ...\r", mbp);
#ifdef _INTERN
    if (userport(userpo) == dest->port)
      putstr("WARNING: Loop detected (HELP LOOP)\r", mbp);
#endif
    seteom(mbp);
  }
}

/************************************************************************/
/*                                                                      */
/* Connect-Partner in der Patchcord-Liste eintragen.                    */
/*                                                                      */
/************************************************************************/
static void
setptc(void *link, UBYTE typ)
{
  PTCENT         *ptcp;
  PTCENT         *p_ptcp;
  UID             uid = userpo->uid;

  ptcp = ptctab + uid;
  ptcp->p_uid = g_uid(link, typ);
  p_ptcp = ptctab + ptcp->p_uid;
  p_ptcp->p_uid = uid;
}

/************************************************************************/
/*                                                                      */
/* Connect an die Host-Console herstellen.                              */
/*                                                                      */
/************************************************************************/
static void
conhst(void)
{
  int             i;

  for (i = 1, hstusr = hstubl + 1; i < MAXHST; ++i, ++hstusr)
    if ((!hstusr->conflg) && (cmpid(hstusr->call, hostid)))
      break;

  if (i != MAXHST)
  {
    cpyid(hstusr->call, usrcal);
    if (hstreq())
    {
      setptc(hstusr, HOST_USER);
      userpo->status = US_CREQ;
      return;
    }
  }

  conerr(CE_HTFULL, NULL);
}

/*
 * Level 4 Connect (Circuit) herstellen.
 */
static void
conl4(DEST *dest)
{
  CIRBLK         *cirp;
  LNKBLK         *frelnk;
  int             i;

  for (i = 0, cirpoi = cirtab; i < NUMCIR; ++i, ++cirpoi)
    if (cirpoi->state == L4SDSCED)
      break;

  if (i != NUMCIR)
  {
    cpyid(cirpoi->downca, dest->np->id);
    cpyid(cirpoi->upcall, usrcal);
    switch (g_utyp(userpo->uid))
      {
        case L4_USER:                           /* User ist Circuit     */
          cirp = (CIRBLK *)g_ulink(userpo->uid);
          cpyid(cirpoi->upnod, cirp->upnod);    /* Uplinkknoten setzen  */
          cpyidl(cirpoi->upnodv, cirp->upnodv); /* Digikette auch       */
          break;

        case L2_USER:                           /* User ist L2-Link     */
          frelnk = (LNKBLK *)g_ulink(userpo->uid);
          cpyid(cirpoi->upnod, myid);           /* Uplink hier          */
          cpyidl(cirpoi->upnodv, frelnk->viaidl); /* und Digikette      */
          break;

        default:                                /* User vom Host        */
          cpyid(cirpoi->upnod, myid);           /* Uplink hier          */
          *cirpoi->upnodv = '\0';               /* kein Digi            */
          break;
      }
    cpyid(cirpoi->l3node, dest->np->id);
    userpo->status = US_CREQ;
    setptc(cirpoi, L4_USER);
    setupmsg(dest);                     /* "Interlink setup ..."        */
    newcir();
  }
  else                                  /* Circuit aufbauen             */
    conerr(CE_CTFULL, dest);            /* Circuit Table full           */
}

/*
 * Level 2 Connect herstellen.
 */
static void
conl2(DEST *dest)
{
#ifdef L1TCPIP
    /* TCPIP-Frames haben hier nix zu suchen. */
    if (CheckPortTCP(dest->port))
    {
      conerr(CE_TCPIP, dest);
      return;
    }
#endif /* L1TCPIP */

  /* User-Suspendierung pruefen */
  if (dest->typ == 'U')
    if (is_down_suspended(dest->call, dest->port))
    {
       conerr(CE_SUSPEND, dest);
       return;
    }

    if (portenabled(dest->port))
    {
#ifndef CONNECTMOD_SET_NODE
      if (   (dest->typ != FLEXNET)       /* bei FLEXNET Pfadweitergabe  */
          && (dest->typ != LOCAL)         /* bei Locals */
          && (dest->typ != LOCAL_M)       /* bei Locals mit Messung */
          && (dest->typ != 'U')           /* und auch bei User-Connect */
        )
      {
        /* uns als Absender ins Via-Feld eintragen */
        cpyid(usrvia, myid);
        usrvia[L2IDLEN - 1] |= L2CH;
        usrvia[L2IDLEN] = 0;
      }
#else
      /* usrvia leeren */
      memset(usrvia, 0, sizeof(usrvia));

      /* ist kein Einstiegsknoten definiert */
      /* setzen wir unser Mycall.            */
      if (updigi[0] == FALSE)
      {
        cpyid(usrvia, myid);
        usrvia[L2IDLEN - 1] |= L2CH;
        usrvia[L2IDLEN] = 0;
      }
      else
        {
          cpyid(usrvia, updigi);
          usrvia[L2IDLEN - 1] |= L2CH;
          usrvia[L2IDLEN] = 0;

          cpyid(usrvia + L2IDLEN, myid);
          usrvia[L2IDLEN + L2IDLEN - 1] |= L2CH;
          usrvia[L2IDLEN + L2IDLEN] = 0;

          memset(updigi, 0, sizeof(updigi));
        }
#endif /* CONNECTMOD_SET_NODE */

      if ((strlen(dest->via) + strlen(usrvia)) > L2VLEN)
      {
        conerr(CE_INVLEN, dest);
        return;
      }
      else
        strcat(usrvia, dest->via);

      /* wenn wir diesen Link schon aktiv haben, melden wir BUSY          */
#ifdef __WIN32__
      lnkpoi = getlnk((unsigned char)dest->port, usrcal, dest->call, usrvia);
#else
      lnkpoi = getlnk(dest->port, usrcal, dest->call, usrvia);
#endif /* WIN32 */

      if (lnkpoi)
      {
        if (lnkpoi->state == L2SDSCED)
        {
          /* Standard: Bitmaske fuer AX.25 */
#ifndef EAX25
          lnkpoi->bitmask = 0x07;
#else
          /* Behandlung bei EAX.25 */
          /* Mode 2: standardmaessig EAX25 versuchen, antwortet die */
          /* Gegenstelle nicht, dann erfolgt spaeter ein Fallback   */
          /* auf AX.25. */
          if (portpar[dest->port].eax_behaviour == 2)
            lnkpoi->bitmask = 0x7F;
          else
            /* EAX nach MHeard-Infos */
            lnkpoi->bitmask = (dest->eax == TRUE) ? 0x7F : 0x07;
#endif
          userpo->status = US_CREQ;
          setptc(lnkpoi, L2_USER);
          setupmsg(dest);                 /* "Downlink setup" ...         */
          newlnk();                       /* Neuen Link aufbauen          */
        }
        else
          conerr(CE_BUSY, dest);          /* Busy (Already connected)     */
      }
      else
#ifndef CONl2FIX
      conerr(CE_LTFULL, dest);          /* Link table full              */
#else
      conerr(CE_INVCAL, dest);          /* Link table full              */
#endif /* CONl2FIX */
    }
    else
      conerr(CE_PORTOFF, dest);           /* Port ist abgeschaltet        */
}

static void
con_cq(DEST *dest)
{
  UID             uid = userpo->uid;
  PTCENT         *ptcp;
  PTCENT         *p_ptcp;
  CQBUF          *cqp;

  ptcp = ptctab + uid;
  ptcp->p_uid = dest->uid;
  p_ptcp = ptctab + ptcp->p_uid;
  p_ptcp->p_uid = uid;

  cqp = (CQBUF *)allocb(ALLOC_CQBUF);
  cqp->uid = uid;
  cqp->p_uid = dest->uid;
  relink((LEHEAD *)cqp, (LEHEAD *)cq_statl.tail);
}

#ifdef OS_IPLINK
/* TCPIP Connect herstellen. */
void conIP(DEST *dest)
{
  /* Ein Connect zum TCPIP-Nachbarn aufbauen. */
  if (IPConvConnect(dest->call, usrcal, FALSE))
  {
    /* Connect misslungen.  */
    /* Server nicht online. */
    conerr(CE_BUSY, dest);
    return;
  }

#ifdef CONl3LOCAL
  /* Partner kommt via L4?        */
  if (g_utyp(userpo->uid) == L4_USER)
    /* ja? mit CACK bestaetigen     */
    ackcir(g_ulink(userpo->uid));
#endif /* CONl3LOCAL */

  /* Markiere den l2port vom TCPIP. */
  dest->port     = tcppoi->port;
  /* Markiere Connectversuch. */
  userpo->status = US_CREQ;
  /* Connect-Partner in der Patchcord-Liste eintragen. */
  setptc(tcppoi, TCP_USER);
  /* Nachricht ueber den Linkaufbau ausgeben. */
  setupmsg(dest);
  /* Connect melden. */
  tcppoi->state = L2MCONNT;

  tcppoi->Intern = TRUE;

  /* Ziel Rufzeichen fuer Connectmeldung setzen. */
  cpyid(tcppoi->Upcall, dest->call);
  /* Ziel Rufzeichen setzen. */
  cpyid(tcppoi->Downcall, dest->call);
}
#endif /* OS_IPLINK */

static void
conusr(int type, DEST *dest)
{
  switch (type)
    {
      case CP_CQ:                       /* CQ-User connecten            */
        con_cq(dest);
        break;

      case CP_HOSTCON:                  /* Connect an die Console       */
        conhst();                       /* nicht an Kanal 0 connecten   */
        break;

      case CP_L4CON:                    /* Layer4 (NET/ROM) Connect?    */
        conl4(dest);                    /* Level 4 Connect              */
        break;

      case CP_L2CON:                    /* Level2 connect?              */
        conl2(dest);
        break;

#ifdef OS_IPLINK
      case CP_IPCONV:                   /* TCPIP connect?                */
        conIP(dest);
        break;
#endif /* OS_IPLINK */

      case CP_BADQUAL:
        conerr(CE_BADQUAL, dest);
        break;

      case CP_INTERN:
        break;

      case CP_UNKNOWN:
        conerr(CE_UNKNOWN, dest);
        break;

      case CP_NODIG:
        conerr(CE_NODIG, dest);
        break;

      default:                          /* Fehler aufgetreten?          */
        conerr(CE_INVCAL, dest);
        break;
    }
}

/************************************************************************/
/* Einleiten einer Weiterverbindung bei Gateway-Connects.               */
/* Hierzu wird aus dem L2-via-Feld gelesen, wohin der Link gehen        */
/* soll. Bei L4-Links wird entsprechend das Zielcall gelesen.           */
/*----------------------------------------------------------------------*/
void
gateway(void)
{
  UID             uid = userpo->uid;
  PTCENT         *ptcp = ptctab + uid;
  char           *viap;
  char           *p;
  DEST            user;
  DEST            dest;
  /* TEST DG9OBU */
  BOOLEAN         invia = FALSE; /* Merker: war ich in den vias dabei ? */

#ifdef PROXYFUNC
  user.via[0] = 0;
#endif

  cpyid(usrcal, calofs(UPLINK, userpo->uid));  /* Mycall                */

  switch (g_utyp(userpo->uid))
  {
      case L2_USER:
        lnkpoi = g_ulink(userpo->uid);
        cpyid(user.call, lnkpoi->srcid);        /* Ziel-Rufzeichen      */

#ifdef CONNECTMOD_SET_NODE
        SetMyNode(lnkpoi->viaidl);           /* Einstiegsknoten setzen. */
#endif /* CONNECTMOD_SET_NODE */

/* Jetzt stellen wir das Addressfeld des eingehenden Links zusammen.      */
/* In usrvia wird der bisherige Weg gespeichert, in user.via der          */
/* restliche.                                                             */
/* Der Weg wird nur soweit kopiert, bis wir den ersten Digi kennen, der   */
/* Rest ist dann unwichtig.                                               */

        /* Achtung, die Via-Liste ist hier umgedreht, hinten stehen die,  */
        /* die schon gedigipeated haben */

        /* hinter letztes Via */
        for (viap = lnkpoi->viaidl; *viap; viap += L2IDLEN);

        /* usrvia leeren */
        memset(usrvia, 0, sizeof(usrvia));

        /* Via-Liste durchgehen, Arbeitszeiger auf usrvia-Liste */
        for (p = usrvia; viap > lnkpoi->viaidl;)
        {
          viap -= L2IDLEN;          /* zum ersten Via */
          memcpy(p, viap, L2IDLEN); /* in die schon-gedigipeated-Liste damit */

          /* Merken, ob ich in den VIAs mit dabei bin. */
          if (cmpid(myid, viap))
            invia = TRUE;

          p[L2IDLEN - 1] ^= L2CH;

          /* H-Bit gesetzt ? */
          if (!(p[L2IDLEN - 1] & L2CH))
          {
            p[L2IDLEN - 1] |= L2CH;
            p[L2IDLEN] = 0;

            for (p = user.via; viap > lnkpoi->viaidl;)
            {
              viap -= L2IDLEN;          /* den Rest regulaer kopieren   */
              cpyid(p, viap);           /* die muessen alle noch        */
              p += L2IDLEN;
            }
            break;
          }
          p += L2IDLEN;
        }
        *p = 0;

        /* TEST DG9OBU */
        /* Waren wir *nicht* in den VIAs dabei, dann war der Link auch */
        /* urspruenglich nicht an uns, sondern an einen unserer Locals */
        /* und wurde stellvertretend fuer diesen angenommen.           */
        /* (FlexNet braucht das so wenn wir einen SSID-Bereich melden) */
        /* Beim Downlink baut conl2() das VIA-Feld neu zusammen, wir   */
        /* muessen ihm noch anzeigen, dass es keine weiteren VIAs gibt */
        /* die er noch nehmen muss.                                    */
        if ((invia == FALSE) && (islocal(user.call)))
          user.via[0] = NUL;

        break;

      case L4_USER:
        cirpoi = g_ulink(userpo->uid);

        cpyid(user.call, cirpoi->destca);       /* Ziel-Rufzeichen      */

        user.via[0] = 0;

/* hier werden wir spaeter mal aus cirpoi->upnod/upnodv den alten und   */
/* mit einer L4-modif den weitern weg rausfinden.                       */

#ifndef CONNECTMOD_SET_NODE
        cpyid(usrvia, myid);                    /* eigenes Call         */
        usrvia[L2IDLEN - 1] |= L2CH;          /* wir haben gedigipeated */
        usrvia[L2IDLEN] = 0;                              /* und fertig */
#else
        cpyid(updigi, cirpoi->upnod);        /* Einstiegsknoten setzen. */
        updigi[L2IDLEN - 1] |= L2CH;                    /* Flag setzen. */
#endif /* CONNECTMOD_SET_NODE */
        break;

      default:
        return;
    }

  user.port = NOPORT;                           /* Port ist uns egal    */
#ifdef EAX25
  user.eax = FALSE;
#endif
  user.typ = 0;

  ptcp->local = PTC_LOCAL;

  /* Zur Sicherheit */
  dest.call[0] = 0;
  dest.via[0] = 0;
  dest.eax = FALSE;
  dest.port = NOPORT;

  dest.typ = 0;

  conusr(conprm2(&user, &dest), &dest);
  ptcp->recflg = FALSE;                         /* nie Reconnect        */
}

/**************************************************************************/
/* CONNECT                                                                */
/*------------------------------------------------------------------------*/
void
ccpcon(char *Direct)
{
  DEST dest;

  /* Zur Sicherheit */
  dest.call[0] = 0;
  dest.via[0] = 0;
  dest.eax = FALSE;
  dest.port = NOPORT;

  cpyid(usrvia, myid);
  usrvia[L2IDLEN - 1] |= L2CH;
  usrvia[L2IDLEN] = 0;
  conusr(conprm(&clicnt, &clipoi, &dest), &dest);
  ptctab[userpo->uid].recflg = (Direct == NULL);
}

/**************************************************************************/
/* CQ                                                                     */
/*------------------------------------------------------------------------*/
void
ccpcq(void)
{
  MBHEAD         *mbp;
  WORD            prt;
  CQBUF          *cqp;
  PTCENT         *ptcp;
  UID             uid = userpo->uid;

  /* CQ-Buffer besorgen, User-ID und User-Pfad speichern und einhaengen */
  cqp = (CQBUF *)allocb(ALLOC_CQBUF);
  cqp->uid = uid;
  cpyid(cqp->id, calofs(UPLINK, uid));
  relink((LEHEAD *)cqp, (LEHEAD *)cq_user.tail);
  userpo->status = US_CQ;

  /* wir rufen CQ */
  ptcp = ptctab + uid;
  ptcp->p_uid = uid;
  ptcp->recflg = TRUE;

  skipsp(&clicnt, &clipoi);

  /* Text kopieren */
  mbp = getmbp();
  mbp->l2fflg = L2CPID;
  while (clicnt-- > 0)
    putchr(*clipoi++, mbp);
  putchr(CR, mbp);

  /* Frame vorbereiten */
  cpyid(usrvia, myid);
  usrvia[L2IDLEN - 1] |= L2CH;
  usrvia[L2IDLEN] = 0;

  /* auf allen aktiven Ports mit aktivem MHeard als UI senden */
  for (prt = 0; prt < L2PNUM; prt++)
  {
    if (!(updmheard(prt) && portenabled(prt)))
      continue;

    rwndmb(mbp);
#ifdef __WIN32__
    sdui(usrvia, cqdest, usrcal, (char)prt, mbp);
#else
    sdui(usrvia, cqdest, usrcal, prt, mbp);
#endif /* WIN32 */
  }

  /* Frame wegschmeissen */
  dealmb(mbp);

  /* Meldung an User */
  mbp = putals("Waiting ...\r");
  seteom(mbp);
}

/************************************************************************/
/*                                                                      */
/* Fuer via-Connects feststellen, ob das gewuenschte Ziel bekannt ist.  */
/* Diese Funktion wird vom L2 aufgerufen. Daher gibt es keinen aktuell  */
/* zu bearbeitenden User!                                               */
/*                                                                      */
/************************************************************************/
BOOLEAN
conn_ok(const char *rxhdr)
{
  DEST  dest1;
  DEST  dest2;

  userpo = NULL;
  dest1.typ = 'U';
  dest1.port = NOPORT;
  cpyid(dest1.call, rxhdr);
  cpyidl(dest1.via, rxhdr + 2 * L2IDLEN);

  switch (conprm2(&dest1, &dest2))
  {
    case CP_L2CON:
    case CP_L4CON:
    case CP_HOSTCON:
#ifdef L1IPCONV
    case CP_IPCONV:
#endif /* L1IPCONV */
    case CP_CQ:
      return(TRUE);
  }
  return(FALSE);
}

#ifdef  CONNECTMOD_SET_NODE
/* Einstiegsknoten setzen. */
void SetMyNode(char *viaidl)
{
  char *viap;

  updigi[0] = 0;

  if (*viaidl == NUL)                             /* Es gibt keine Digiliste. */
    return;                                               /* Keine Aenderung. */

  for(viap = viaidl; *viap; viap += L2IDLEN);        /* Durchsuche Digiliste. */
   viap-=L2IDLEN;                                    /* naechstes Rufzeichen .*/

  cpyid(updigi, viap);                             /* Einstiegsknoten setzen. */
  updigi[L2IDLEN - 1] |= L2CH;                                /* Flag setzen. */
  return;
}
#endif /* CONNECTMOD_SET_NODE */


/* End of src/l7conn.c */
