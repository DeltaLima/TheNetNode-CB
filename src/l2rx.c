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
/* File src/l2rx.c (maintained by: DF6LN)                               */
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

#ifdef IPROUTE
static MBHEAD  *pid8frag(MBHEAD *);
#endif
static BOOLEAN  srxdNR(void);
static BOOLEAN  isnxti(void);
/* max. Anzahl anzunehmender Frames wenn BUSY */
#ifdef EAX25
#define LINKBUSY ((lnkpoi->flag & L2FBUSY) && (lnkpoi->busyrx >= (lnkpoi->bitmask == 0x7F) ? 16 : 7))
#else
#define LINKBUSY ((lnkpoi->flag & L2FBUSY) && (lnkpoi->busyrx >= 7))
#endif

/*----------------------------------------------------------------------*/
/*                                                                      */
/* "level 2 receiver"                                                   */
/*                                                                      */
/* Alle Frames aus der RX-Frameliste holen und analysieren. Kopie an    */
/* Monitorliste, digipeaten oder in Level-3-Liste, falls erforderlich.  */
/* Auf UI-Frames antworten, falls erforderlich.                         */
/*                                                                      */
/* Reaktion entsprechend Protokoll, siehe unten.                        */
/*                                                                      */
/*----------------------------------------------------------------------*/
void
l2rx(void)
{
  UBYTE   *source;              /* Zeiger auf Quellrufzeichen/SSID      */
  WORD    l2state;              /* aktueller Level 2 Linkstatus         */
  MBHEAD *fbp;                  /* Framebufferpointer lokal             */
  int     i;
  int     frmr_bytes = 3;       /* Anzahl Bytes bei FRMR                */

#ifdef USERMAXCON
  LNKBLK *lnk;
  WORD    user_links;
#endif


  /* solange empfangene Frames vorhanden                                  */
  while ((fbp = (MBHEAD *)rxfl.head) != (MBHEAD *)&rxfl)
  {
    ulink((LEHEAD *)fbp);       /* eins aus Liste holen                 */

    if (!takfhd(fbp))           /* Kopf analysieren                     */
    {
      dealmb(fbp);              /* nicht ok, dann wegwerfen             */
      continue;                 /* naechstes Frame                      */
    }

    fbp->type = 2;              /* wir sind im Level 2                  */
    fbp->tx = 0;                /* es ist ein empfangenes Frame         */

    monitor(fbp);               /* an den Monitor                       */

    if (rxfctl == L2CUI)        /* UI-Frame                             */
    {
/*      if (istome(rxfhdr) == TRUE)      * wenn an mich ..              */
/*        if (rxfPF != FALSE             * .. und Antwort gewuenscht .. */
/*        && rxfCR != FALSE)                                            */
/*      xdm();                           * beantworten mit DM           */
/*                                                                      */
/* Dies ist ein Protokollverstoss! Nach AX.25-Protokoll soll auf ein    */
/* UI-Frame mit Poll nur dann mit DM geantwortet werden, wenn kein Link */
/* besteht. Wenn dagegen ein Link besteht, soll entsprechend Linkzu-    */
/* stand geantwortet werden. Dadurch kann ein bestehender, gut funktio- */
/* nierender Link vorzeitig beendet werden. Jetzt wird der Fehler so    */
/* geaendert, dass auf eine Reaktion hier vollkommen verzichtet wird.   */
/* Manche Spielkinder hatten ausserdem bemerkt, dass man einen Knoten   */
/* abschiessen kann, wenn man ihm in einer Aussendung beliebig viele    */
/* UI-Frames mit Poll schickt. Dadurch werden naemlich fuer die Ant-    */
/* wort-Frames beliebig viele Buffer belegt, bis der Knoten einen Reset */
/* ausloest. Auch deshalb ist es guenstiger, hier auf eine Reaktion zu  */
/* verzichten. Wenn es jemanden stoeren sollte, darf er natuerlich die  */
/* vom AX.25-Protokoll geforderte Reaktion einbauen.                    */

      lnkpoi = NULL;   /* kein Linkpointer */

      /* PID des UI-Frames holen wenn noch Bytes da, sonst 0 eintragen */
      fbp->l2fflg = (fbp->mbgc < fbp->mbpc) ? getchr(fbp) : 0;

      /* Fuer uns interessantes UI-Frame ? */
      if (   (cmpid(rxfhdr, myid))          /* an mich direkt */
          || (cmpid(rxfhdr, "NODES \140"))  /* NETROM-Nodes   */
#ifdef IPROUTE
          || (istomev() == 4)               /* AX25-ARP an "QST" */
          || (istomev() == 5)               /* AX25-ARP an "QST" */
#endif
                  )
      {
        /* UI-Frame bearbeiten */
        if (!tol3sw(fbp))
          dealmb(fbp);
      }
      else /* UI-Frames, die gedigipeated werden koennen/muessen */
                  gateway_ui(rxfhdr + L2IDLEN, rxfhdr, rxfhdr + L2ILEN, fbp);

      continue;
    }

/* Haben wir einen zum Frame passenden Linkblock ?                      */
/*                                                                      */
/* Die Linkliste dieses Ports durchgehen und nach einem passenden Link- */
/* block ausschau halten. Wurde er gefunden, wird er an das Ende der    */
/* Liste gehaengt, weil die Liste im naechsten Durchlauf wieder von     */
/* hinten durchsucht wird.                                              */
/*                                                                      */
/* Wenn wir keinen passenden Link haben, aber das Framezielcall an mich */
/* (Call + SSID oder Ident mit beliebiger SSID) ist, oder das Block-    */
/* quellcall mit dem Framezielcall uebereinstimmt, dann einen neuen     */
/* Link aus der Freiliste holen.                                        */
/* Wichtig: Es wird der Header, den wir senden wuerden, mit der Link-   */
/*          liste verglichen (txf...)                                   */
/*    printf("\nistomev() = %u\n", istomev()); */
#ifdef __WIN32__
    lnkpoi = getlnk((unsigned char)rxfprt, txfhdr + L2IDLEN, txfhdr, txfhdr + L2ILEN);
#else
    lnkpoi = getlnk(rxfprt, txfhdr + L2IDLEN, txfhdr, txfhdr + L2ILEN);
#endif /* WIN32 */

/* Jetzt haben wir einen Pointer auf den Link, der entweder schon vorher*/
/* bestand, oder der jetzt neu angelegt wurde. Ist der Pointer gleich   */
/* NULL, dann ist was schiefgegangen und wir koennen nicht annehmen.    */

    if (lnkpoi == NULL) /* keinen passenden Linkblock gefunden und es   */
                        /* kann kein Link mehr angenommen werden.       */
    {
      if (istome(rxfhdr))               /* Verbindung fuer mich ?       */
      {
        if (   rxfctl == L2CSABM        /* SABM                         */
#ifdef EAX25
            || rxfctl == L2CSABME       /* SABME (EAX.25)               */
#endif
           )
        {
          l2tolx(L2MBUSYT);             /* hoeheren Leveln melden       */
          xdm();                        /* mit DM beantworten           */
        }
        else                            /* sonst nur antworten, wenn    */
          if (   rxfPF != 0             /* Command mit Poll, dann       */
              && rxfCR != 0)            /* mit DM antworten             */
          xdm();
        else                            /* oder wenn kein Command       */
          if (rxfctl == L2CDISC)        /* Poll, aber ein DISC, mit     */
            xua();                      /* UA antworten                 */
      }
      dealmb(fbp);                      /* empfangenes Frame wegwerfen  */
      continue;                         /* und zum naechsten            */
    }

    if (lnkpoi->state == L2SDSCED)     /* wir haben den Link noch nicht */
    {
      if (istomev() == 0)         /* nicht fuer mich und auch nicht via */
      {
          dealmb(fbp);              /* Frame wegwerfen   */
          continue;                 /* und zum naechsten */
      }
    }

/* Falls Timer 3 aktiv, diesen neu setzen, es ist wieder Aktivitaet auf */
/* dem Link.                                                            */
    if (lnkpoi->T3 != 0)
      setT3();

    l2state = lnkpoi->state;            /* Linkstatus zur Abfrage       */

#ifdef DAMASLAVE
    if (rxfDA)                          /* DAMA?                        */
    {
      if (damaslave(rxfprt))            /* DAMA-Slave auf Port erlaubt? */
      {
        portpar[rxfprt].damaok = 12000; /* 2 Minuten Timeout            */
        if (rxfPF && rxfCR)             /* wenn gepollt                 */
        {
          l2stma(stbl10a);                      /* DAMA-Poll receive    */
          portpar[rxfprt].sendok = 1;
          if (l2state)                               /* aktiver Link ?  */
            if (   (rxfctl & L2CNONRM) != L2CNONRM   /* Frame mit N(R)  */
                && srxdNR() == TRUE                  /* Zaehlerstand ok */
                && lnkpoi->VS != lnkpoi->lrxdNR      /* noch was offen? */
               )
            {
              ++lnkpoi->tries;                /* Retryzaehler erhoehen  */

#ifdef T1TIMERMOD
              /* Nach 3 erfolglosen Versuche,     */
              /* wird der SRTT-Wert vergroessert. */
              if (  (lnkpoi->tries > 2)
                  /* SRTT-Wert kleiner als 50. */
                  &&(lnkpoi->SRTT < 50))
              {
                lnkpoi->RTT = 10;
                clrRTT();
              }
#endif /* T1TIMERMOD */

              lnkpoi->flag |= L2FREPEAT;
/* da nicht alles bestaetigt, wird Maxframe um 1 reduziert              */
              change_maxframe(lnkpoi, -1);
            }
        }
      }
    }
#endif

    if (!(rxfctl & L2CNOIM))            /* I-Frame ?                    */
    {
/*----------------------------------------------------------------------*/
/* I-Frame :                                                            */
/*                                                                      */
/* Nur annehmen, wenn empfangene N(R) des Frames ok, srxdNR(), und das  */
/* I-Frame das naechste erwartete in der Sequenz ist, isntxi(). Wenn    */
/* alles ok, Laenge pruefen und ggf. auf falsche Laenge mit Frame-      */
/* Reject reagieren, sonst Antwort entsprechend Statetable und I-Frame  */
/* verarbeiten.                                                         */
/*----------------------------------------------------------------------*/
      if (   srxdNR()                             /* N(R) ok?           */
          && isnxti())                            /* erwartet?          */
      {
        if (fbp->mbpc - fbp->mbgc <= L2MILEN + 1) /* Info-Laenge ok?    */
        {
          lnkpoi->tries = 0;
/*----------------------------------------------------------------------*/
/* Bei DAMA-Betrieb auf dem Port eventuell erstes neues I-Poll          */
/* anmeckern. DAMA-Ack-Poll verzoegern, bis alle Infos verarbeitet..    */
/* Aktivitaetszaehler und -merker loeschen                              */
/*----------------------------------------------------------------------*/
          if (dama(rxfprt))             /* I-Frame auf DAMA-Port ?      */
          {
            if (rxfPF & L2CPF)          /* Wenn I-Poll..                */
            {
              polDAMA();                /* anmeckern..                  */
              l2stma(stbl00dama);
            }
            else                        /* kein Poll                    */
            {
              clrDAMA();                   /* DAMA clear !              */
              setT2(0);                    /* T2 setzen                 */
              lnkpoi->damapc = lnkpoi->T2; /* und fuer DAMA nehmen      */
              l2stma(stbl01dama);          /* DAMA braucht auch T2..    */
            }
          }
          else
            if (rxfPF & L2CPF)                  /* Poll                 */
              l2stma(stbl00);
            else                                /* kein Poll            */
              l2stma(stbl01);

/* Wenn kein I-Frame mehr kommen kann, T2 loeschen                      */
          if (((lnkpoi->VR - lnkpoi->ltxdNR) & lnkpoi->bitmask) == lnkpoi->bitmask)
          {
            lnkpoi->T2 = 0;   /* T2 loeschen */
            if (dama(rxfprt))
              lnkpoi->damapc = 0;
          }

/* Linkzustand I-Transfer moeglich und nicht busy ?                     */

          if (   (l2state >= L2SIXFER)
              && (l2state != L2SHTH)
              && !LINKBUSY)
          {
            if (lnkpoi->flag & L2FBUSY)
              lnkpoi->busyrx++;
            fbp->l2fflg = fbp->mbgc < fbp->mbpc ? getchr(fbp) : 0;
#ifdef IPROUTE
            if (istome (rxfhdr))
              if ((fbp = pid8frag(fbp)) == NULL)
                continue;
#endif
/*----------------------------------------------------------------------*/
/* wenn Level-3-I-Paket, dann in Level-3-RX-Liste einhaengen und Link   */
/* als Level-3-Link markieren, No-Activity-Timeout neu starten          */
/*----------------------------------------------------------------------*/
            if (tol3sw(fbp))
#ifndef THENETMOD
              lnkpoi->noatou = ininat;
#else                                          /* L4TIMEOUT */
              lnkpoi->noatou = SetL4Timeout(); /* L2/L4-Timeout setzen. */
#endif /* THENETMOD */
            else
            {
/*----------------------------------------------------------------------*/
/* wenn normales Level-2-I-Paket, wenn nicht Busy oder Level-3-Link, I  */
/* annehmen und in Linkempfangsliste einhaengen                         */
/*----------------------------------------------------------------------*/
              if (!(lnkpoi->flag & (L2FDSLE | L2FDIMM)))
              {
                relink((LEHEAD *)fbp, (LEHEAD *)lnkpoi->rcvdil.tail);
                ++lnkpoi->rcvd;
              }
              else                              /* sonst                */
                dealmb(fbp);                    /* Paket verwerfen      */
            }
            continue;                           /* naechstes Paket      */
          }
        }
        else                                    /* Frame zu lang        */
          sdfrmr(0x03);                         /* U/S-Frame invalid!   */
      }
    }                           /* durchfallen und Frame wegschmeissen  */
    else                        /* kein I-Frame :                       */
    {
      if (!(rxfctl & L2CNOSM))  /* "no S mask", kein S-Frame            */
      {
/*----------------------------------------------------------------------*/
/* S-Frame :                                                            */
/*                                                                      */
/* Nur annehmen, wenn empfangene N(R) des Frames ok, srxdNR(), und wenn */
/* das Frame kein Infofeld enthaelt.                                    */
/*                                                                      */
/* Auf RR, RNR, REJ entsprechend Statetable antworten, auf andere mit   */
/* Frame-Reject antworten.                                              */
/*----------------------------------------------------------------------*/

        if (srxdNR())                           /* N(R) ok?             */
        {
          if (fbp->mbgc == fbp->mbpc)           /* kein I-Feld?         */
          {
            lnkpoi->tries = 0;
            if (dama(rxfprt))                   /* Auf DAMA-Links:      */
              incDAMA();                        /* Prioritaet runter    */
            switch (rxfctl & 0x0c)
            {
              case L2CRR & 0x0c:                /* RR Frame             */
                l2stma(!rxfCR
                       ? (!rxfPF ? stbl11 : stbl10)
                       : (!rxfPF ? stbl03 : stbl02));
                if (dama(rxfprt))               /* wenn DAMA-Port       */
                  if (rxfCR && rxfPF)           /* und gepollt wird     */
                    polDAMA();                  /* DAMA-Warnung         */
                break;

              case L2CRNR & 0x0c:               /* RNR Frame            */
                l2stma(!rxfCR
                       ? (!rxfPF ? stbl15 : stbl14)
                       : (!rxfPF ? stbl07 : stbl06));
                lnkpoi->flag |= L2FREPEAT;
                break;

              case L2CREJ & 0x0c:               /* REJ Frame            */
                l2stma(!rxfCR
                       ? (!rxfPF ? stbl13 : stbl12)
                       : (!rxfPF ? stbl05 : stbl04));
                if ((l2state >= L2SIXFER) && (l2state != L2SHTH))
                {
/* bei REJ wird Maxframe um 1 reduziert - wer mies empfaengt, landet    */
/* irgendwann bei Maxframe 1                                            */
                  change_maxframe(lnkpoi, -1);
                  lnkpoi->flag |= L2FREPEAT;
#ifndef DAMASLAVE
                  if (!dama(rxfprt))
#else
                  if (   !dama(rxfprt)
                      && !damaslaveon(rxfprt))
#endif
                    sdoi();             /* ausstehende I-Frames senden  */
                }
                break;

              default:          /* Kontrollfeld falsch oder nicht       */
                sdfrmr(0x01);   /* implementiert!                       */
                break;
            } /* switch */
          }
          else                                  /* U/S-Frame mit        */
            sdfrmr(0x03);                       /* unerlaubtem I-Feld   */
        }
      } /* END S-Frame */
      else                     /* Kein S-Frame, dann Frame der U-Gruppe */
      {
        if ((rxfctl & 0xFF) != L2CFRMR)
        {
/*----------------------------------------------------------------------*/
/* Kein FRMR-Frame, Frame nur annehmen, wenn kein Infofeld vorhanden.   */
/*                                                                      */
/* Frame auswerten, reagieren, nach Statetable antworten.               */
/*----------------------------------------------------------------------*/
          clrT3();                              /* Timer 3 loeschen     */
          if (fbp->mbgc == fbp->mbpc)
          {
            switch (rxfctl)
              {
                case L2CSABM:                   /* neuer Link/-reset    */
#ifdef EAX25
                case L2CSABME:                  /* neuer EAX.25-Link    */
#endif
#ifdef USERMAXCON
                  /* Zaehlung auf Port aktiv und Call OK und SABM-Frame fuer/ueber mich ? */
                  if (   (portpar[rxfprt].maxcon != 0)
                      && (fvalca(rxfhdr + L2IDLEN) == YES)
                      && (istomev() != 0))
                  {
                    /* Anzahl der Connects dieses Users auf *diesem* Port ermitteln */
                    for (lnk = lnktbl, i = 0, user_links = 0; i < LINKNMBR; i++, lnk++)
                      if (   (cmpcal(lnk->dstid, (rxfhdr + L2IDLEN)))
                          && (lnk->state != L2SDSCED)
                          && (lnk->liport == rxfprt))
                        user_links++;

                    /* noch Connects frei ? */
                    if (user_links >= portpar[rxfprt].maxcon)
                    {
                      /* nein, also den Verbindungswunsch ablehnen */
                      xdm();                      /* mit DM antworten     */
                      dealmb(fbp);                /* Frame wegschmeissen  */
                      continue;
                    }
                  }
#endif
                  if (fvalca(rxfhdr + L2IDLEN) != YES)
                  {
                    l2tolx(L2MBUSYT);           /* nein, melden         */
                    xdm();                      /* mit DM antworten     */
                    dealmb(fbp);                /* Frame vergessen      */
                    continue;                   /* naechstes Paket      */
                  }

#ifdef EAX25
                  /* Ist kein EAX.25 auf diesem Port erlaubt, dann */
                  /* Verbindung abwerfen. Das Gleiche noch mal fuer den */
                  /* Fall dass nur EAX.25 erzwungen ist, Frame aber AX.25 ist */
                  if (   ((rxfctl == L2CSABME) && (portpar[rxfprt].eax_behaviour == 0))
                      || ((rxfctl == L2CSABM) && (portpar[rxfprt].eax_behaviour == 3)))
                  {
                    xdm();                      /* mit DM antworten     */
                    dealmb(fbp);                /* Frame vergessen      */
                    continue;                   /* naechstes Paket      */
                  }

                  /* Nur auf EAX.25 umschalten wenn SABME, EAX-Flag ignorieren */
                  if (rxfctl == L2CSABME)
                  {
                     /* EAX-Verbindung kennzeichnen und Maxframe aendern */
                    lnkpoi->bitmask = 0x7F;
#ifdef __WIN32__
                    lnkpoi->maxframe = (unsigned char)portpar[rxfprt].maxframe_eax;
#else
                    lnkpoi->maxframe = portpar[rxfprt].maxframe_eax;
#endif /* WIN32 */

                  }
                  else
                    lnkpoi->bitmask = 0x07; /* normale AX.25-Verbindung */
#endif
                  /* TEST DG9OBU */
                  /* Linkwunsch via uns, kennen wir einen Weg ?         */
                  /* ebenfalls: Linkwuensche an unsere Locals ?         */
                  /* (Wird gebraucht bei FlexNet SSID-Bereichsmeldung)  */


                 if (   (lnkpoi->state == L2SDSCED) /* nicht verbunden */
                      && (   (istomev() == 2)        /* an mich mit via */
                /* nicht an mich, aber einen Local oder in SSID-Range */
                          || ((istomev() == 3) && (islocal(rxfhdr) != NO))
#ifdef PROXYFUNC
                          || isproxy(rxfhdr)
#endif
                         )
                     )
                  {
                    if (conn_ok(rxfhdr))        /* Ziel bekannt?        */
                    {
                      l2stma(stbl08a);          /* ja, nicht reagieren  */
                                                /* bis Link steht       */
                    }
                    else                        /* nein - kein neuer    */
                    {                           /* Link moeglich        */
                      dealmb(fbp);              /* Frame vergessen      */
                      continue;                 /* naechstes Paket      */
                    }
                  }
                  else
                    l2stma(stbl08);
                  break;

                case L2CDISC:
                  if (!l2state)                 /* Link aktiv ?         */
                  {
                    if (   rxfPF != 0           /* nein, wenn Command   */
                        && rxfCR != 0)          /* mit Poll, dann mit   */
                      xdm();                    /* DM antworten         */
                    else
                      xua();                    /* sonst mit UA         */
                    dealmb(fbp);                /* Frame wegwerfen      */
                    continue;                   /* naechstes Paket      */
                  }
                  l2stma(stbl09);               /* DISC EITHER COMMAND  */
                  break;

                case L2CUA:
                  l2stma(stbl16);               /* UA EITHER RESPONSE   */
                  break;

                case L2CDM:
#ifdef EAX25
                  /* Wird unser SAMBE mit DM beantwortet, dann Rueckfall */
                  /* auf AX.25 */
                  if ((l2state == L2SLKSUP) && (lnkpoi->bitmask == 0x7F))
                  {
                    /* Rueckfall nur wenn Port nicht nur EAX.25 zulaesst */
                    if (portpar[rxfprt].eax_behaviour != 3)
                      lnkpoi->bitmask = 0x07;
                    dealmb(fbp);                /* Frame wegwerfen      */
                    continue;
                  }
#endif
                  l2stma(stbl17);               /* DM EITHER RESPONSE   */
                  break;

                default:                /* unbekanntes Kontrollfeld :   */
                  sdfrmr(0x01);         /* "Kontrollfeld falsch oder    */
                  break;                /* nicht implementiert"         */
              } /* end switch (rxfctl) */
          } /* if */
          else
            sdfrmr(0x03);  /* Frametyp unbekannt "U/S-Frame mit un-     */
                           /* erlaubtem Infofeld"                       */
        }
        else
        {
/* FRMR-Frame :                                                         */
/*                                                                      */
/* Wird nur im Frame-Reject-Zustand oder bei moeglichem Informations-   */
/* transfer angenommen. Es werden die FRMR-Infobytes gelesen, FRMR an   */
/* die hoeheren Level gemeldet, nach Statetable geantwortet.            */
          if (   (l2state >= L2SIXFER && l2state != L2SHTH)
              || l2state == L2SFRREJ)
          {
#ifdef EAX25
            /* FRMR ist bei EAX.25 laenger, neue Laenge setzen          */
            if (lnkpoi->bitmask == 0x7F)
              frmr_bytes = 5;
#endif
            for (source = lnkpoi->frmr, i = 0; i < frmr_bytes; ++i)
            {
              *source++ = (fbp->mbgc < fbp->mbpc) ? getchr(fbp) : 0;
            }
          }
          l2stma(stbl18);                       /* FRMR EITHER RESPONSE */
        }
      }
    }
    dealmb(fbp);              /* aktuelles Frame verarbeitet, wegwerfen */
  } /* end while ((fbp = rxfl.head) != &rxfl) */
}

/************************************************************************/
/*                                                                      */
/* "take frame head"                                                    */
/*                                                                      */
/* Adresskopf und Kontrollbyte des Frames aus dem Framebuffer, auf      */
/* dessen Kopf fbp zeigt, analysieren. Diese Funktion ist die erste,    */
/* die auf ein empfangenes Frame angewandt wird.                        */
/*                                                                      */
/*                                                                      */
/* Folgende Parameter werden bei der Analyse gesetzt:                   */
/*                                                                      */
/*    rxfhdr, rxfPF, rxfCR, rxfctl, rxfprt, rxfDA                       */
/*    (rxfEAX, rxfctlE wenn Extended-AX.25 aktiviert)                   */
/*                                                                      */
/* Folgende Parameter werden nach der Analyse gesetzt fuer ein          */
/* moegliches Antwortframe :                                            */
/*                                                                      */
/*   txfhdr  = Quell- und Zielcall aus rxfhdr, aber vertauscht, plus    */
/*             reverse via-Liste aus rxfhdr                             */
/*   txfPF   = rxfPF                                                    */
/*   txfCR   = 0, Response !                                            */
/*   txfprt  = rxfprt                                                   */
/*                                                                      */
/*                                                                      */
/* Return :  TRUE  - das Frame hat einen gueltigen AX.25-Framekopf      */
/*           FALSE - sonst                                              */
/*                                                                      */
/************************************************************************/
BOOLEAN
takfhd(MBHEAD *fbp)
{
  char *p;                              /* Zeiger im Header             */
  char *source;                         /* Quellzeiger Kopien           */
  char *dest;                           /* Zielzeiger Kopien            */
  int   n;

  rxfprt = fbp->l2port;                 /* Port uebernehmen             */

  if (!portenabled(rxfprt))             /* Port gesperrt                */
    return (FALSE);

  rwndmb(fbp);                          /* Frame von vorne              */
  for (p = rxfhdr, n = 1; n <= L2INUM + L2VNUM; n++)
  {
    if (!getfid(p, fbp))                /* naechstes Call lesen         */
    {
      INV_FRAME(rxfprt, INVF_ADR);
      return (FALSE);
    }
    p += L2IDLEN;
    if (*(p - 1) & L2CEOA)
      break;                            /* Ende des Addressfeldes       */
  }
  *(p - 1) &= ~L2CEOA;

  if (n < L2INUM)                       /* Absender und/oder Ziel fehlt */
  {
    INV_FRAME(rxfprt, INVF_ALN);
    return (FALSE);
  }

  if (n > L2VNUM + L2INUM)              /* Addressfeld zu lang          */
  {
    INV_FRAME(rxfprt, INVF_ALN);
    return (FALSE);
  }
  *p = NUL;                             /* Addressfeld terminieren      */

  if (fbp->mbgc == fbp->mbpc)           /* Control-Feld fehlt?          */
  {
    INV_FRAME(rxfprt, INVF_CTL);
    return (FALSE);
  }

  rxfctl = getchr(fbp);                 /* Control-Byte extrahieren     */

/* Framelaengencheck UI-Frames: jetzt duerfen noch max. 257 ungelesene  */
/* Bytes im Buffer sein! (PID + 256 Bytes Info)                         */
  if ((rxfctl == L2CUI) && (fbp->mbpc - fbp->mbgc > L2MILEN + 1))
    return (FALSE);

  rxfDA = !(rxfhdr[L2ILEN-1] & L2CDAMA); /* ist es ein DAMA-Frame?      */

  rxfhdr[L2ILEN-1] |= L2CDAMA;          /* DAMA-Flag zuruecksetzen      */

#ifdef EAX25
  /* EAX-Bit maskieren und invertieren, da im Frame geloescht gleich    */
  /* aktiviert bedeutet, hier aber "normale" Logik verwendet wird.      */
  rxfEAX = !(rxfhdr[L2ILEN-1] & L2CEAX); /* ist es ein Extended-Frame?  */

  rxfhdr[L2ILEN-1] |= L2CEAX;            /* EAX.25-Flag zuruecksetzen   */

  /* bei EAX-Frames zweites Control-Byte lesen wenn nicht U-Frame       */
  if (rxfEAX && !((rxfctl & 0x03) == 3)) /* kein U-Frame, dann zweites  */
    rxfctlE = getchr(fbp);               /* zweites Controlbyte lesen   */
  else
    rxfctlE = 0;                         /* zweites Controlbyte leer    */
#endif

  /* Unterscheidung AX.25 V1- oder V2-Frame, wir koennen nur V2         */
  if (((rxfhdr[L2IDLEN - 1] ^ rxfhdr[L2ILEN - 1]) & L2CCR) != 0)
  {
    /* Auswertung normale AX.25-Frames und EAX U-Frames */
    rxfCR = rxfhdr[L2IDLEN - 1];
    rxfCR &= L2CCR;

#ifdef EAX25
    /* Pollflag befindet sich an unterschiedlichen Positionen           */
    if (   !rxfEAX
        || (rxfEAX && ((rxfctl & 0x03) == 3)))
#endif
      rxfPF = rxfctl & L2CPF;          /* Auswertung Pollflag bei AX.25 */
#ifdef EAX25
    else
      rxfPF = (rxfctlE << 4) & L2CPF;  /* Auswertung Pollflag bei EAX25 */
#endif
  }
  else
    return (rxfctl == L2CUI);           /* V1-Frame, nur UI nehmen      */

#ifdef EAX25
  /* PF-Bit ruecksetzen */
  if (   !rxfEAX
      || (rxfEAX && ((rxfctl & 0x03) == 3)))
#endif
    rxfctl &= ~L2CPF;    /* AX.25 */
#ifdef EAX25
  else
    rxfctlE &= 0xFE;    /* EAX.25 */
#endif

  txfCR = 0;                        /* Command-Flag fuer TX-Frame  */
  txfPF = rxfPF;                    /* Poll-Flag bei TX wie bei RX */
  txfprt = rxfprt;                  /* TX-Port gleich RX-Port      */
  cpyid(txfhdr, rxfhdr + L2IDLEN);  /* Calls von Absender und Ziel */
  cpyid(txfhdr + L2IDLEN, rxfhdr);  /* vertauschen                 */

/* Jetzt wird das via-Feld so umgebaut, wie es bei der Sendung aussehen */
/* wird.                                                                */
  for (source = rxfhdr + L2ILEN; *source; source += L2IDLEN)
    ;
  for (dest = txfhdr + L2ILEN; source != rxfhdr + L2ILEN; dest += L2IDLEN)
  {
    source -= L2IDLEN;
    memcpy(dest, source, L2IDLEN);              /* Rufzeichen kopieren  */
    dest[L2IDLEN - 1] ^= L2CH;
  }
  *dest = NUL;

  return (TRUE);
}

/************************************************************************/
/*                                                                      */
/* "level 2 to level x"                                                 */
/*                                                                      */
/* Meldung msg (L2M...) an Layer 3 und hoehere Layer weitergeben.       */
/*                                                                      */
/************************************************************************/
void
l2tolx(WORD msg)
{
  if (!l2tol3(msg))                         /* Layer 2 -> Layer 3       */
    l2tol7(msg, lnkpoi, L2_USER);           /* Layer 2 -> Layer 7       */
}

/************************************************************************\
*                                                                        *
*       Informationstransfer von Layer 2 nach Layer X                    *
*       ---------------------------------------------                    *
*                                                                        *
*      Infopakete aus dem aktuellen Link (lnkpoi) an hoehere Level       *
*      weiterreichen. conctl gibt an, ob der hoehere Level die           *
*      Erstickungskontrolle" (hier = Beruecksichtigung der maximal       *
*      noch anzunehmenden I-Pakete) machen soll (FALSE) oder in jedem    *
*      Fall alle uebermittelten I-Pakete annehmen muss (TRUE). Falls     *
*      die I-Pakete vom hoeheren Level angenommen wurden, Empfangs-      *
*      zaehler rcvd und Aktivitaetstimer noatou entsprechend updaten.    *
*      Es wird l2link in den Framekoepfen der weitergereichten Pakete    *
*      auf lnkpoi gesetzt und type auf 2 fuer "Level 2".                 *
*                                                                        *
*      Solange noch empfangene Pakete vorhanden sind, werden diese       *
*      an andere Layer durch Aufruf von fmlink() uebertragen. Bei ge-    *
*      setztem Ueberfuellungskontroll-Flag (conctl == TRUE) wird die     *
*      Uebertragung abgebrochen, wenn der andere Layer keine weiteren    *
*      Daten mehr aufnehmen kann.                                        *
*                                                                        *
*      Nach erfolgter Uebertragung wird die Anzahl der uebertragenen     *
*      Zeichen fuer die Statistik gezaehlt und der No-Activity-Timer     *
*      neu gesetzt.                                                      *
*                                                                        *
\************************************************************************/
void
i2tolx(BOOLEAN conctrl)
{
  MBHEAD         *mbp;     /* Zeiger auf Framekopf weiterzureichendes I */

  if (   (lnkpoi->state == L2SDSCRQ)
      || (lnkpoi->flag & (L2FDSLE | L2FDIMM)))
  {
    dealml((LEHEAD *)&lnkpoi->rcvdil);
    lnkpoi->rcvd = 0;
    return;
  }

  while (lnkpoi->rcvd != 0)           /* solange I's aus Link vorhanden */
  {
    mbp = (MBHEAD *)lnkpoi->rcvdil.head;
    mbp->l2link = lnkpoi;             /* Linkzeiger                     */
    mbp->type = 2;                    /* Level 2 !                      */
    if (!fmlink(conctrl, mbp))        /* I an hoeheren Level geben      */
      return;                         /* Abbruch, wenn nicht angenommen */

#ifndef THENETMOD
    lnkpoi->noatou = ininat;
#else                                       /* L4TIMEOUT */
    lnkpoi->noatou = SetL4Timeout();        /* L2/L4-Timeout setzen,    */
#endif /* THENETMOD */

    --lnkpoi->rcvd;                   /* Empfangspaketezaehler updaten  */
  }
}


/************************************************************************\
*                                                                        *
* "serve received N(R)"                                                  *
*                                                                        *
* Aktuell empfangenes N(R) (rxfctl) des aktuellen Links (lnkpoi)         *
* auswerten und entsprechend verfahren (s.u.).                           *
*                                                                        *
* Return:  TRUE  - aktuell empfangenes N(R) ist okay oder                *
*                  Linkzustand laesst N(R)-Empfang nicht zu              *
*          FALSE - aktuell empfangenes N(R) ist falsch                   *
*                                                                        *
\************************************************************************/
static BOOLEAN
srxdNR(void)
{
#ifdef __WIN32__
  char            rxdNR;      /* emfangenes N(R)                        */
#else
  WORD            rxdNR;      /* emfangenes N(R)                        */
#endif
  WORD            newok;      /* Anzahl neu bestaetigte I's             */
  WORD            outstd;     /* Anzahl ausstehende (unbestaetigte) I's */
  WORD            l2state;    /* Link-Status                            */

  l2state = lnkpoi->state;
  if (l2state >= L2SIXFER && l2state != L2SHTH)  /* darf N(R) kommen?   */
  {
#ifdef EAX25
    /* EAX I- und S-Frames gesondert behandeln, U-Frames wie bei AX25   */
    if (   (lnkpoi->bitmask == 0x7F) && ((rxfctl & 0x03) <= 2))
      rxdNR = ((rxfctlE >> 1) & 0x7F);
    else
#endif
      rxdNR = ((rxfctl >> 5) & 0x07);     /* Sequenznummer lesen        */

    if (   (outstd = outsdI()) != 0     /* Wenn I's ausstehen und       */
        && (newok = ((rxdNR - lnkpoi->lrxdNR) & lnkpoi->bitmask)) != 0)
    {                                    /* neue bestaetigt wurden:      */
      if (newok <= outstd)              /* und nicht zuviel bestaet.    */
      {
        if (  (lnkpoi->lrxdNR)
            &&((lnkpoi->RTTvs - lnkpoi->lrxdNR) & lnkpoi->bitmask) >= newok)
          clrRTT();                     /* RTT-Messung ok, stoppen      */

#ifdef __WIN32__
        lnkpoi->lrxdNR = (char)rxdNR;   /* dann N(R) annehmen,          */
#else
        lnkpoi->lrxdNR = rxdNR;         /* dann N(R) annehmen,          */
#endif /* WIN32 */
        clrT1();                        /* T1 stoppen,                  */
                                        /* Sind alle ausstehenden I's   */
        if (newok == outstd)            /* bestaetigt worden ?          */
        {                               /* ja:                          */

/* Da alles bestaetigt, kann Maxframe erhoeht werden                    */
          change_maxframe(lnkpoi, +1);

          if ((signed char)lnkpoi->priold != -1)     /* bei Downlink-Aktivitaet:     */
#ifdef __WIN32__
            lnkpoi->damapm = (unsigned char)lnkpoi->priold;  /* Prioritaet reseten     */
#else
            lnkpoi->damapm = lnkpoi->priold;  /* Prioritaet reseten     */
#endif /* WIN32 */
          lnkpoi->priold = -1;          /* Priori-Flag loeschen         */
          clearDT(0);                   /* DAMA-Timer loeschen <<<<     */
        }
        else
          setT1();                      /* T1 neu starten               */

        while (newok-- != 0)            /* Alle bestaetigten Frames     */
        {                               /* aus der Liste entfernen      */
          dealmb((MBHEAD *)ulink((LEHEAD *)lnkpoi->sendil.head));
          --lnkpoi->tosend;
        }
      }
      else                              /* Mehr I's bestaetigt als      */
      {
        sdfrmr((char)0x08);             /* ausstehen. Das geht nicht.   */
        return (FALSE);                 /* FRMR erzeugen, N(R) falsch   */
      }
    }
    if (   l2state == L2SWA            /* Falls warten auf Bestaetigung */
        || l2state == L2SWADBS
        || l2state == L2SWARBS
        || l2state == L2SWABBS)
    {
      if (!rxfCR && rxfPF != 0)         /* Wenn RR mit Final war, dann  */
      {
        clrT1();                        /* T1 stoppen                   */
        clrRTT();                       /* RTT stoppen, Messung gueltig */
        if (lnkpoi->VS != lnkpoi->lrxdNR)
          lnkpoi->flag |= L2FREPEAT;
        clearDT(0);                     /* DAMA-Timer loeschen          */
      }
      else if (!lnkpoi->T1)             /* T1 neu starten               */
        setT1();
    }
  }

  return (TRUE);
}

/************************************************************************\
*                                                                        *
* "is next I"                                                            *
*                                                                        *
* Testen, ob das aktuell empfangene I-Frame (rxf...) das naechste fuer   *
* den aktuellen Linkblock (lnkpoi) erwartete I-Frame ist, wenn der       *
* Linkzustand Informationstransfer zulaesst. Bei nicht erwarteter        *
* Sequenznummer entsprechende Statetable abarbeiten.                     *
*                                                                        *
* Return :  TRUE  - I-Frame ist das naechste erwartete oder Linkzustand  *
*                   laesst keinen Informationstransfer zu                *
*           FALSE - sonst                                                *
*                                                                        *
\************************************************************************/
static BOOLEAN
isnxti(void)
{
  WORD            iseqno;                       /* I Sequence Number    */

  if (   lnkpoi->state >= L2SIXFER
      && lnkpoi->state != L2SHTH)               /* I-Transfer?          */
  {
    /* N(S) des Kontrollfeldes maskieren, diese Framenummer erwartet ?  */
    if ((iseqno = (rxfctl >> 1) & lnkpoi->bitmask) == lnkpoi->VR)  /* I erwartet ? */
    {
      /* Maximale Erhoehung des Sequenzzaehlers */
      if (((lnkpoi->ltxdNR + lnkpoi->bitmask) & lnkpoi->bitmask) != iseqno)  /* kein Ueberlauf ? */
      {
        if (!LINKBUSY)                            /* wenn nicht busy, neue */
          lnkpoi->VR = (iseqno + 1) & lnkpoi->bitmask;    /* V(R) setzen           */
      }
      else
      {
        sdfrmr((char)0x01);
        return (FALSE);
      }
    }
    else
    {                                         /* unerwartetes Info:     */
      l2stma(!rxfPF ? stbl26 : stb26b);       /* INVALID N(S) RECEIVED  */
      if (dama(lnkpoi->liport) && rxfPF)      /* I-Poll ?          <=== */
        polDAMA();                            /* ja: muss meckern! <=== */
      return (FALSE);
    }
  }
  return (TRUE);          /* I richtig oder Linkzustand ohne I-Transfer */
}

/************************************************************************\
*                                                                        *
* "info to layer x and clear link"                                       *
*                                                                        *
* Empfangsdaten ohne Flowcontrol in den Layer hochmelden und dann        *
* Link zuruecksetzen.                                                    *
*                                                                        *
\************************************************************************/
void
i2xclr(void)
{
  i2tolx(TRUE);
  clrlnk();
}

#ifdef IPROUTE
/************************************************************************\
*                                                                        *
* "pid 8 fragmentierung"                                                 *
*                                                                        *
* Ein PID 8 Frame empfangen und ggfs mit vorhergehenden Frames zusammen- *
* basteln.                                                               *
*                                                                        *
\************************************************************************/
static MBHEAD *
pid8frag(MBHEAD *fbp)
{
  BYTE           anz_frag;
  MBHEAD        *ret;
  int            len;

  if (fbp->l2fflg != L2CFRAG)           /* PID 08 ?                     */
    return (fbp);                       /* nein, dann Frame zurueck     */

  if ((len = fbp->mbgc - fbp->mbpc) == 0)       /* leeres Frame?        */
  {
    if (lnkpoi->tmbp != NULL)
      dealmb(lnkpoi->tmbp);
    dealmb(fbp);
    return(NULL);
  }
  anz_frag = getchr(fbp);               /* erstes Byte lesen            */
  if (anz_frag & 0x80)                  /* erstes PID08-Frame           */
  {
    if (lnkpoi->tmbp != NULL)           /* Fehler! Letztes Paket war    */
      dealmb(lnkpoi->tmbp);             /* noch nicht komplett          */
    if (len == 1)                       /* keine PID im 1. Frame?       */
    {
      dealmb(fbp);
      return(NULL);
    }
    anz_frag &= 0x7F;                   /* Anzahl der Folgefragmente    */
    lnkpoi->tmbp = (MBHEAD *)allocb(ALLOC_MBHEAD); /* Buffer besorgen   */
    lnkpoi->tmbp->l2fflg = getchr(fbp); /* PID des Originals lesen      */
  }
  else
    if (lnkpoi->tmbp == NULL)           /* Fehler! Folgefragment ohne   */
    {                                   /* vorheriges 1. Fragment       */
      dealmb(fbp);                      /* hier muesste eigentlich ein  */
      return (NULL);                    /* FRMR fuer PID08 folgen, gips */
    }                                   /* aber wohl nicht?             */
  while (fbp->mbgc < fbp->mbpc)
    putchr(getchr(fbp), lnkpoi->tmbp);
  dealmb(fbp);

  if (anz_frag == 0)                    /* letztes Frame!               */
  {
    rwndmb(lnkpoi->tmbp);               /* zurueckspulen                */
    ret = lnkpoi->tmbp;
    lnkpoi->tmbp = NULL;
    return (ret);                       /* und an L2 uebergeben         */
  }
  else
    return (NULL);                      /* es folgen weitere Teile      */
}
#endif

/* End of src/l2rx.c */
