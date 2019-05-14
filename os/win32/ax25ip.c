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
/* File os/linux/ax25ip.c (maintained by: DG9OBU)                       */
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
/*                                                                      */
/* AX25IP.C V1.05 01.9.98 by DG1KWA / Andreas                           */
/*                        extended by DG9OBU / Marc                     */
/************************************************************************/

#include "tnn.h"

#ifdef AX25IP
#include "ax25ip.h"

/*************************************************************************/
/* AX25IP-Hausaufgaben : RX, TX und Routingtabellenpflege                */
/*************************************************************************/
void ax25ip(void)
{
  register UBYTE uEntry = 0;

  /* nur wenn Interface aktiv ist */
  if (!ax25ip_active)
    return;

  /* empfangene Frames abholen */
  ax25ip_recv();

  /* IP-Adressen der Routentabelle pflegen */
  if ((uNamesUpdate != 0) && (tic10 % uNamesUpdate == 0))
  {
    for (; uEntry <= TABLE_SIZE; ++uEntry)
      route_update(&route_tbl[uEntry]);

    route_update(&default_route);
  }

  /* ausstehende Frames senden wenn vorhanden */
  if (kick[ax25ip_port])
    ax25ip_send();
}

/*************************************************************************/
/* Einen Eintrag aus der Routingtabelle ausgeben                         */
/*************************************************************************/
void show_rt_entry(struct route_table_entry rp, MBHEAD *bufpoi)
{
  /* erst mal merken, wo wir sind */
  bufpoi->l4time = bufpoi->mbpc;

  /* Check auf Eintrag fuer die default-Route */
  if (rp.callsign[0] != 1)
    putprintf(bufpoi, "%s", call_to_a(&rp.callsign[0]));
  else
    putstr("default", bufpoi);

  putspa(11, bufpoi);

  /* IP-Adresse ausgeben */
  show_ip_addr(ntohl(rp.ip_addr), bufpoi);

  putspa(29, bufpoi);

  /* UDP-Port ausgeben wenn UDP-Port gesetzt */
  if (rp.udp_port != 0)
  {
    /* Eine UDP-Route */
    putstr("UDP", bufpoi);
    putspa(35, bufpoi);
    putnum(ntohs(rp.udp_port), bufpoi);
  }
  else
    /* Eine IP-Route */
    putstr("IP", bufpoi);

  /* Routen-Timeout anzeigen */
  if (rp.timeout != 0)
  {
    ULONG d = rp.timeout;
    ULONG h, m, s;                  /* Stunden, Minuten, Sekunden */

    s = d % 60L;
    d /= 60L;
    m = d % 60L;
    d /= 60L;
    h = d % 24L;

    putspa(40, bufpoi);
    putprintf(bufpoi, " %02lu:%02lu:%02lu", h, m, s); /* hh:mm:ss */
  }

  if (rp.hostname[0] != 0)
  {
    putspa(50, bufpoi);
    putprintf(bufpoi, "%s", rp.hostname);
  }

  putstr("\r", bufpoi);
}

/* Anzahl von IP- oder UDP-Routen in der Routingtabelle feststellen */
/* (IP-Routen haben keinen UDP-Port gesetzt) */
/* UDP = FALSE -> IP-Routen zaehlen, UDP = TRUE -> UDP-Routen zaehlen */
/* Default-Route wird beachtet */
unsigned int count_routes(BOOLEAN udp)
{
  register unsigned int i;
  register unsigned int j = 0;

  /* Gesamte Routingtabelle durchgehen */
  for (i = 0; i < (unsigned int)route_tbl_top; ++i)
  {
    if (   ((udp == TRUE) && (route_tbl[i].udp_port != 0))    /* UDP-Route */
        || ((udp == FALSE) && (route_tbl[i].udp_port == 0)))  /* IP-Route */
      ++j;
  }

  /* Default-Route */
  if (default_route.callsign[0] != 0)
  {
    if (   ((udp == TRUE) && (default_route.udp_port != 0))    /* UDP-Route */
        || ((udp == FALSE) && (default_route.udp_port == 0)))  /* IP-Route */
      ++j;
  }

  /* Anzahl Routen melden */
  return (j);
}

/* Timeout fuer gelernte Routen */
void route_age(void)
{
  register unsigned int i = 0;

#ifdef AXIPR_HTML
  /* htmlstatistik-timer. */
  h_timer();
#endif

  /* Gesamte Routingtabelle durchgehen */
  for (; i < (unsigned int)route_tbl_top; ++i)
  {
    /* Route mit laufendem Timer */
    if (route_tbl[i].timeout != 0)
    {
      /* Timer laeuft jetzt ab */
      if (--route_tbl[i].timeout == 0)
        /* Route loeschen */
        route_del(route_tbl[i].callsign);
    }
  }
}

/* Hostname -> IP-Adressen aufloesen */
void route_update(struct route_table_entry *rtentry)
{
  struct hostent *hstent;

  /* Kein Hostname, dann auch keine Umwandlung moeglich */
  if (rtentry->hostname[0] == 0)
    return;

  /* Hostname im IP-Adresse konvertieren */
  if ((hstent = gethostbyname((char*)&rtentry->hostname[0])) != NULL)
  {
    /* Bei Aenderung uebernehmen */
    if (memcmp(&rtentry->ip_addr, &hstent->h_addr_list[0][0], 4) != 0)
    {
      /* IP-Adresse hat sich geaendert */
      memcpy(&rtentry->ip_addr, &hstent->h_addr_list[0][0], 4);
      LOGL1("IP-adress for %s changed !", rtentry->hostname);
    }
  }

#ifdef AXIPR_UDP
  /* Original UDP-Port setzen. */
  rtentry->udp_port = rtentry->org_udp_port;
#endif /* AXIPR_UDP */
}

/* Partielle Frame-Analyse fuer dyn. Eintraege */
BOOLEAN route_canlearn(MBHEAD *fbp)
{
  char *p;                              /* Zeiger im Header             */
  register int n;
  UBYTE ctl;

  rwndmb(fbp);                          /* Frame von vorne              */

  for (p = rxfhdr, n = 1; n <= L2INUM + L2VNUM; ++n)
  {
    if (!getfid(p, fbp))                /* naechstes Call lesen         */
      return (FALSE);

    p += L2IDLEN;
    if (*(p - 1) & L2CEOA)
      break;                            /* Ende des Addressfeldes       */
  }
  *(p - 1) &= ~L2CEOA;
  *p = NUL;
  ctl = getchr(fbp);                    /* Control-Byte extrahieren     */
  ctl &= ~L2CPF;                        /* Poll-Flag loeschen           */
  /* Nur UI, UA und SABM/SABME erzeugen einen dynamischen Eintrag       */
  if (   ctl == L2CUI                   /* UI                           */
      || ctl == L2CUA                   /* UA                           */
      || ctl == L2CSABM                 /* SABM                         */
#ifdef EAX25
      || ctl == L2CSABME                /* SABME (EAX.25)               */
     )
#endif
    return (TRUE);

#ifndef AXIPR_UDP
  /* Alle anderen Kontrollcodes landen hier und bewirken keinen Eintrag */
  return (FALSE);
#else
  return (TRUE);
#endif
}

/*************************************************************************/
/* Kommandointerface fuer Operationen auf die AX25IP-Routentabelle       */
/*************************************************************************/

/* Eine Route anhand eines Calls loeschen, liefert TRUE wenn erfolgreich */
BOOLEAN route_del(unsigned char *call)
{
  register int i;
  register int j;

  /* Soll die Default-Route geloescht werden ? */
  if ((call == NULL) || (call[0] == NUL))
  {
    /* Als unbenutzt markieren */
    default_route.callsign[0] = NUL;

    /* Pruefen, ob Sockets noch notwendig */
    ax25ip_check_down();
    return (TRUE);
  }

  /* Uebergebenes Call in der Tabelle suchen */
  for (i = 0; i < route_tbl_top; ++i)
  {
    if (addrmatch(call, route_tbl[i].callsign))
    {
    /* BINGO ! */
    /* Problem: die Tabelle hat keinen Merker, ob ein Eintrag gueltig ist, */
    /*          die Eintraege haengen einfach hintereinander. Deshalb alle */
    /*          Eintraege oberhalb des gefundenen Eintrages einen nach     */
    /*          unten kopieren und den "table-top" einen herabsetzen.      */
      for (j = i; j < route_tbl_top; ++j)
        route_tbl[j] = route_tbl[j + 1];

      /* Ein Tabelleneintrag weniger */
      route_tbl_top--;

      /* Pruefen, ob Sockets noch notwendig */
      ax25ip_check_down();
      return(TRUE);
    }
  }

  return(FALSE);
}

static const char inv_mode[] = "Invalid Mode\r";
static const char inv_call[] = "Invalid Callsign\r";
static const char inv_ip[]   = "Invalid IP adress\r";
static const char inv_par[]  = "Invalid Parameter\r";

void
ccpaxipr(void)
{
  MBHEAD         *mbp;
  int             port = 0;
  register int    i;
  int             tmp_fd = -1;
  int             new_udp = 0;
  unsigned char   call[L2IDLEN];
  char            hostname[HNLEN + 1];
  struct          hostent *host = NULL;
  int             uNewTimeout;
#ifdef AXIPR_UDP
  char           *Hostname = hostname;
  int             NewLogLevel = FALSE;
#endif
#ifdef AXIPR_HTML
  char           *timeset  = set_time();
  int             NewHtmlStat = FALSE;
#endif
  char            cBuf[BUFLEN + 1];
  unsigned int    uCmd = OP_NONE;
  unsigned int    uMode = NO_MODE;

#ifdef AXIPR_HTML
#define OP_HTMLSTAT 10
#endif

  memset(hostname, 0, sizeof(hostname));

  /* AX25IP ueberhaupt aktiv ? */
  if (ax25ip_active == FALSE)
  {
    putmsg("No AX25IP-interface present, nothing to configure or show !\r");
    return;
  }

  /* neuen Parser verwenden ? */
  if (new_parser == TRUE)
  {

  /* Sysop will aendern und noch was in der Zeile da ? */
  if (issyso() && skipsp(&clicnt, &clipoi))
  {
    /* Frischer Buffer */
    memset(cBuf, 0, sizeof(cBuf));

    /* Operation lesen (add, delete) */
    for (i = 0; i < BUFLEN; ++i)
    {
      if ((!clicnt) || (*clipoi == ' '))
        break;
      clicnt--;
      cBuf[i] = toupper(*clipoi++);
    }

    /* Zum alten Parser zurueckschalten */
    if (   (strcmp(cBuf, "OLD") == 0)
        || (cBuf[0] == 'O')
       )
    {
      new_parser = FALSE;
      putmsg("Switched to old axipr-parser !!!\r");
      return;
    }

    /* Hinzufuegen (add oder +) */
    if (   (strcmp(cBuf, "ADD") == 0)
        || (cBuf[0] == '+')
       )
      uCmd = OP_ADD;

    /* Loeschen (delete, del oder -) */
    if (   (strcmp(cBuf, "DELETE") == 0)
        || (strcmp(cBuf, "DEL") == 0)
        || (cBuf[0] == '-')
       )
      uCmd = OP_DEL;

    /* eigenen UDP-Port aendern */
    if (strcmp(cBuf, "MYUDP") == 0)
      uCmd = OP_MYUDP;

    /* Namensaufloesungsintervall einstellen */
    if (strcmp(cBuf, "LOOKUP") == 0)
      uCmd = OP_HNUPD;

    /* Loglevel */
    if (   (strcmp(cBuf, "LOGLEVEL") == 0)
        || (strcmp(cBuf, "LOG") == 0)
       )
      uCmd = OP_LOG;

    /* Timeout fuer dynamische Routen */
    if (strcmp(cBuf, "TIMEOUT") == 0)
      uCmd = OP_TIMEOUT;

#ifdef AXIPR_HTML
    /* htmlstatistik */
    if (   (strcmp(cBuf, "HTMLSTAT") == 0)
        || (strcmp(cBuf, "H") == 0))

      uCmd = OP_HTMLSTAT;
#endif

    /* Fehler bei der Auswertung ? */
    if (uCmd == OP_NONE)
    {
      putmsg(inv_mode);
      return;
    }

    /* Hier rein, wenn der Timeout fuer dynamische Routen geaendert werden soll */
    if (uCmd == OP_TIMEOUT)
    {
      int uNewTimeout;

      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        mbp = getmbp();
        putprintf(mbp, "the actual timeout for dynamic routes is : %u\r", uDynTimeout);
        /* und ab die Post ... */
        prompt(mbp);
        seteom(mbp);
        return;
      }

      /* Timeout lesen */
      uNewTimeout = nxtlong(&clicnt, &clipoi);

      if (  (uNewTimeout < 0)
          ||(uNewTimeout > (86400)))  /* max. 1 Tag */
      {
        putmsg("error: timeout out of range (0 - 86400) !\r");
        return;
      }

      /* Wert uebernehmen */
      uDynTimeout = uNewTimeout;

      /* und ab die Post ... */
      mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
      return;
    }

    /* Hier rein, wenn das Intervall der Namensaufloesung geaendert werden soll */
    if (uCmd == OP_HNUPD)
    {
      UWORD uNewInterval;

      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        mbp = getmbp();

        putprintf(mbp, "Hostname-update ");

        if (uNamesUpdate != 0)
          putprintf(mbp, "occurs every %u seconds.\r", uNamesUpdate / 100);
        else
          putprintf(mbp, "is disabled at the moment.\r");

        /* und ab die Post ... */
        prompt(mbp);
        seteom(mbp);
        return;
      }

      /* Timeout lesen */
      uNewInterval = nxtnum(&clicnt, &clipoi);

      if (uNewInterval > 3600)  /* max. eine Stunde */
      {
        putmsg("error: value out of range (0 - 3600 seconds) !\r");
        return;
      }

      /* Wert uebernehmen */
      uNamesUpdate = uNewInterval * 100;

      /* und ab die Post ... */
      mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
      return;
    }

    /* Hier rein, wenn der Loglevel geaendert werden soll */
    if (uCmd == OP_LOG)
    {
      int new_loglevel;

      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        mbp = getmbp();
        putprintf(mbp, "my actual loglevel is : %u\r", loglevel);
        /* und ab die Post ...                                            */
        prompt(mbp);
        seteom(mbp);
        return;
      }

      /* neues Loglevel lesen */
      new_loglevel = nxtnum(&clicnt, &clipoi);

      if (  (new_loglevel < 0)
          ||(new_loglevel > 4))
      {
        putmsg("error: loglevel out of range (0 - 4) !!!\r");
        return;
      }

      /* Wert uebernehmen */
      loglevel = new_loglevel;

      /* und ab die Post ... */
      mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
      return;
    }

#ifdef AXIPR_HTML
    if (uCmd == OP_HTMLSTAT)
    {
      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        mbp = getmbp();
#ifdef SPEECH
        putprintf(mbp, speech_message(347),HtmlStat);
#else
        putprintf(mbp, "HTML-Statistic is %d\r", HtmlStat);
#endif
        /* und ab die Post ... */
        prompt(mbp);
        seteom(mbp);
        return;
      }

      /* Timeout lesen */
      NewHtmlStat = nxtnum(&clicnt, &clipoi);

      if (  (NewHtmlStat < 0)
          ||(NewHtmlStat > 1))
      {
#ifdef SPEECH
        putmsg(speech_message(300));
#else
        putmsg("errors: Log level worth from 0 to 1!\r");
#endif
        return;
      }

      /* Wert uebernehmen */
      HtmlStat = NewHtmlStat;

      /* und ab die Post ... */
      mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
      return;
    }
#endif /* AXIPR_HTML */

    /* Hier nur rein, wenn wir als naechstes eine UDP-Portnummer erwarten */
    if (uCmd == OP_MYUDP)
    {
      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        mbp = getmbp();
        putprintf(mbp, "my actual UDP-port is : %u\r", ntohs(my_udp));
        /* und ab die Post ...                                            */
        prompt(mbp);
        seteom(mbp);
        return;
      }

      /* neue UDP-Portnummer lesen */
      new_udp = nxtlong(&clicnt, &clipoi);

      if (  (new_udp <= 0)
          ||(new_udp > 65535))
      {
        putmsg("error: UDP-port number not valid (0 - 65535) !!!\r");
        return;
      }

#ifdef AXIPR_UDP
      /* Neuer UDP-Port gleich MY-UDP?   */
      /* Keine Aenderungen durchfuehren. */
      if (my_udp == htons((unsigned short)new_udp))
      {
        /* und ab die Post ...           */
        mbp = getmbp();
        prompt(mbp);
        seteom(mbp);
        return;
      }
#endif /* AXIPR_UDP */

#ifndef AXIPR_UDP
      /* Laufendes UDP aendern */
      if (fd_udp != -1)
      {
#endif
        /* Versuchen, neuen UDP-Port anzulegen */
        if ((tmp_fd = setup_udp(htons((unsigned short)new_udp))) != -1)
        {
#ifndef AXIPR_UDP
          /* Neuen Descriptor eintragen und alten schliessen */
          close(fd_udp);
#else
          /* Nur wenn alter Socket noch lebt,  */
          if (fd_udp != EOF)
            /* dann schliessen wir den Socket. */
            close(fd_udp);
#endif /* AXIPR_UDP */

          fd_udp = tmp_fd;
          /* Gibt es einen Eintrag. */
          if (l1pp)
            /* Check, ob neuer Filedescriptor groesser ist */    
            l1pp->kisslink = max(fd_ip, fd_udp);

          /* Neuen UDP-Port merken */
          my_udp = htons((unsigned short)new_udp);
        }
        else
        {
          putmsg("error: changing the UDP-port failed !!!\r");
          return;
        }
#ifndef AXIPR_UDP
      }
      else
      {
        /* neuen UDP-Port merken */
        my_udp = htons(new_udp);
      }
#endif
      /* und ab die Post ...                                                  */
      mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
      return;
    }

    /* Noch was da in der Kommandozeile ? */
    if (!skipsp(&clicnt, &clipoi))
    {
      putmsg("syntax error: callsign missing\r");
      return;
    }

    /* Frischer Buffer */
    memset(cBuf, 0, sizeof(cBuf));

    /* Call lesen */
    for (i = 0; i < BUFLEN; ++i)
    {
      if ((!clicnt) || (*clipoi == ' '))
        break;

      clicnt--;
      cBuf[i] = toupper(*clipoi++);
    }

    /* Operation auf Default-Route ? */
    if (strcmp(cBuf, "DEFAULT") == 0)
      call[0] = NUL;    /* Leere Callstring als Indikator fuer Defaultroute */
    else
    {
      /* Call konvertieren und pruefen */
      if (a_to_call(cBuf, call) != 0)
      {
        putmsg(inv_call);
        return;
      }
    }

    /* Wenn wir loeschen wollen, brauchen wir nichts mehr */
    if (uCmd != OP_DEL)
    {
      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        putmsg("syntax error: IP-adress missing\r");
        return;
      }

      /* Frische Buffer */
      memset(hostname, 0, sizeof(hostname));

      /* Hostnamen / IP-Adresse lesen */
      for (i = 0; i < HNLEN; ++i)
      {
        if (!clicnt || *clipoi == ' ')
          break;

        clicnt--;
        hostname[i] = *clipoi++;
      }

      /* eventuellen Hostnamen aufloesen */
      if ((host = gethostbyname(hostname)) == NULL)
      {
        mbp = getmbp();
        putprintf(mbp, "Warning: can't resolve IP for host %s ! I keep trying ...\r", hostname);
        prompt(mbp);
        seteom(mbp);
      }
      else
      {
        strncpy(hostname, host->h_name, sizeof(hostname));
      }
    } /* if (uCmd != OP_DEL) */

    /* Optionale UDP-Parameter lesen */
    skipsp(&clicnt, &clipoi);

    /* Frischer Buffer */
    memset(cBuf, 0, sizeof(cBuf));

    /* Mode lesen (IP, UDP) */
    for (i = 0; i < BUFLEN; ++i)
    {
      if ((!clicnt) || (*clipoi == ' '))
        break;

      clicnt--;
      cBuf[i] = toupper(*clipoi++);
    }

    /* Mode bestimmen (UDP) */
    /* Bei der Initialisierung wurde bereits IP eingestellt */
    if (   (strcmp(cBuf, "UDP") == 0)
        || (cBuf[0] == 'U')
       )
      uMode = UDP_MODE;

    /* Noch was da in der Kommandozeile ? */
    if (!skipsp(&clicnt, &clipoi))
      port = DEFAULT_UDP_PORT;
    else
    {
      /* neue UDP-Portnummer lesen */
      port = nxtlong(&clicnt, &clipoi);

      if (  (port <= 0)
          ||(port > 65535))
      {
        putmsg("error: UDP-port number not valid\r");
        return;
      }
    }

    /* Operation ausfuehren */
    switch (uCmd)
    {
      /* Route hinzufuegen */
      case OP_ADD :
#ifndef AXIPR_UDP
        route_add((unsigned char*)&hostname, host, call, (uMode == UDP_MODE ? port : 0), (call[0] == 0 ? TRUE : FALSE)
        break;
#else
        if (route_add((unsigned char*)&hostname
                                 , host
                                 , call
                                 , (uMode == UDP_MODE ? (int)port : 0)
                                 , (call[0] == 0 ? TRUE : FALSE)
                                 , 0
                                 , hostname
#ifdef AXIPR_HTML
                                 , timeset
                                 , P_USER
#endif /* AXIPR_HTML */
                                 ))
                      putmsg("Call is registerd.\r");

                    return;
#endif /* AXIPR_UDP */

      /* Route loeschen */
      case OP_DEL :
#ifndef AXIPR_UDP
                    route_del(call);
                    break;
#else

                    if (route_del(call))
                      putmsg("Call is deleted!\r");
                    else
                      putmsg("Call no deleted!\r");

                    return;
#endif /* AXIPR_UDP. */


      default     : break;
    }

    mbp = getmbp();
    /* und ab die Post ... */
    prompt(mbp);
    seteom(mbp);
    return;
  }

  }
  else
  {
  /* ALTE SYNTAX */

  /* Sysop will aendern und noch was in der Zeile da ? */
  if (issyso() && skipsp(&clicnt, &clipoi))
  {
    clicnt--;

    switch (toupper(*clipoi++))
    {
      case 'P':                                 /* Parser aendern       */
        /* wir koennen nur in den neuen Parser umschalten */
        new_parser = TRUE;
        putmsg("Switched to new axipr-parser !!!\r");
        return;
        break;

      case 'R':                                 /* Routen-Eintrag       */
        /* Kommandozeile pruefen */
        if (!skipsp(&clicnt, &clipoi))
        {
          putmsg(inv_par);
          return;
        }

        clicnt--;

        switch (*clipoi++)
        {
          case '-':                             /* Route loeschen       */
            /* Call lesen */
            if (getcal(&clicnt, &clipoi, TRUE, (char *)call) != YES)
            {
              invcal();
              return;
            }

            /* Call konvertieren */
            for (i = 0; i < L2CALEN; ++i)
              call[i] = call[i] << 1;

#ifdef AXIPR_UDP
            /* und ab die Post ...                                                  */
            mbp = getmbp();

            if (route_del(call))
              putstr("Call is delete!\r", mbp);
            else
              putstr("Call no delete!\r", mbp);

            prompt(mbp);
            seteom(mbp);
            return;
#else
           /* Route loeschen */
            route_del(call);
            break;
#endif /* AXIPR_UDP. */

          case '+':                             /* Route hinzufuegen    */
            /* Call lesen */
            if (getcal(&clicnt, &clipoi, TRUE, (char *)call) != YES)
            {
              invcal();
              return;
            }

            /* Call linksschieben   */
            for (i = 0; i < L2CALEN; ++i)
              call[i] = call[i] << 1;

            /* Noch was da in der Kommandozeile ? */
            if (!skipsp(&clicnt, &clipoi))
            {
              putmsg("IP-Adress/Hostname missing\r");
              return;
            }

            /* Frische Buffer */
            memset(hostname, 0, sizeof(hostname));

            /* Hostnamen / IP-Adresse lesen */
            for (i = 0; i < HNLEN; ++i)
            {
              if (!clicnt || *clipoi == ' ')
                break;

              clicnt--;
              hostname[i] = *clipoi++;
            }

            /* Hostname / IP-Adresse aufloesen */
            if ((host = gethostbyname(hostname)) == NULL)
            {
              putmsg(inv_ip);
              return;
            }

            /* Mode-Parameter vorhanden ? */
            if (skipsp(&clicnt, &clipoi))
            {
              --clicnt;
              switch (toupper(*clipoi++))
              {
                case 'I':       /* IP: Port steht fest */
                  break;
                case 'U':       /* UDP: abweichenden Port lesen wenn vorhanden */
                  nextspace(&clicnt, &clipoi);
                  if (!skipsp(&clicnt, &clipoi))
                    port = DEFAULT_UDP_PORT;
                  else
                    {
                      port = nxtlong(&clicnt, &clipoi);

                      if (  (port <= 0)
                          ||(port > 65535))
                      {
                        putmsg("error: UDP-port number not valid\r");
                        return;
                      }
                    }
                  break;

                default:        /* unbekannter Modus */
                  putmsg(inv_mode);
                  return;
              }
            }
            /* Route hinzufuegen */
#ifndef AXIPR_UDP
            route_add((unsigned char*)&hostname, host, call, (int) port, FALSE, 0);
            break;
#else
            if (route_add((unsigned char*)&hostname
                         , host
                         , call
                         , (int) port
                         , FALSE
                         , 0
                         , hostname
#ifdef AXIPR_HTML
                         , timeset
                         , P_USER
#endif /* AXIPR_HTML */
                         ))
              putmsg("Call is registerd.\r");

            return;
#endif /* AXIPR_UDP */

        }
        break;

      case 'D':                         /* Default Route aendern        */

        nextspace(&clicnt, &clipoi);

        /* Noch Eingaben da ? */
        if (!skipsp(&clicnt, &clipoi))
        {
          putmsg(inv_mode);
          return;
        }

        clicnt--;

        /* Operation bestimmen */
        switch (*clipoi++)
        {
          case '-':                     /* Default Route loeschen       */
#ifdef AXIPR_UDP
            /* und ab die Post ...                                                  */
            mbp = getmbp();

            if (route_del((unsigned char *)""))
              putstr("Call is delete!\r", mbp);
            else
              putstr("Call no delete!\r", mbp);

            prompt(mbp);
            seteom(mbp);
            return;
#else
            route_del((unsigned char *)"");
            break;
#endif /* AXIPR_UDP. */

          case '+':                     /* Default Route eingeben       */

            /* IP-Adresse / Hostname lesen */
            if (!skipsp(&clicnt, &clipoi))
            {
              putmsg(inv_ip);
              return;
            }

            /* Frische Buffer */
            memset(hostname, 0, sizeof(hostname));

            /* IP-Adresse / Hostname kopieren */
            for (i = 0; i < HNLEN; ++i)
            {
              if (!clicnt || *clipoi == ' ')
                break;
              clicnt--;
              hostname[i] = *clipoi++;
            }

            /* aufloesen */
            if ((host = gethostbyname(hostname)) == NULL)
            {
              putmsg(inv_ip);
              return;
            }

            /* Modus-Kennzeichner lesen falls vorhanden */
            if (skipsp(&clicnt, &clipoi))
            {
              --clicnt;
              switch (toupper(*clipoi++))
              {
                case 'I':   /* IP: Port steht fest */
                  break;
                case 'U':   /* UDP: eventuell abweichenden Port lesen */
                  nextspace(&clicnt, &clipoi);

                  if (!skipsp(&clicnt, &clipoi))
                    port = DEFAULT_UDP_PORT;
                  else
                    {
                      port = nxtlong(&clicnt, &clipoi);

                      if (  (port <= 0)
                          ||(port > 65535))
                      {
                        putmsg("error: UDP-port number not valid\r");
                        return;
                      }
                    }
                  break;
                default:    /* unbekannter Modus */
                  putmsg(inv_mode);
                  return;
              }
            }
            /* Route eintragen */
#ifndef AXIPR_UDP
            route_add((unsigned char*)&hostname, host, NULL, (int) port, TRUE, 0);
            break;
#else
            if (route_add((unsigned char*)&hostname
                         , host
                         , NULL
                         , (int) port
                         , TRUE
                         , 0
                         , hostname
#ifdef AXIPR_HTML
                         , timeset
                         , P_USER
#endif /* AXIPR_HTML */
                         ))
              putmsg("Call is registerd.\r");

            return;
#endif /* AXIPR_UDP */

          default:
            putmsg(inv_mode);
            return;
        }
        break;

      /* Eigenen UDP-Port aendern */
      case 'U':
        /* neuer Port muss vorhanden sein */
        if (!skipsp(&clicnt, &clipoi))
        {
          putmsg(inv_par);
          return;
        }

        /* Port ermitteln. */
        new_udp = nxtlong(&clicnt, &clipoi);

        if (  (new_udp <= 0)
            ||(new_udp >= 65535))
        {
          putmsg("UDP-Port not valid, not changed !!!\r");
          return;
        }

#ifdef AXIPR_UDP
        /* Neuer UDP-Port gleich MY-UDP?   */
        /* Keine Aenderungen durchfuehren. */
        if (my_udp == htons((unsigned short)new_udp))
        {
          /* und ab die Post ...           */
          mbp = getmbp();
         prompt(mbp);
         seteom(mbp);
         return;
        }
#endif /* AXIPR_UDP */

#ifndef AXIPR_UDP
        /* Laufendes UDP aendern */
        if (fd_udp != -1)
        {
#endif
          /* Versuchen, neuen UDP-Port anzulegen */
          if ((tmp_fd = setup_udp(htons((unsigned short)new_udp))) != -1)
          {
#ifndef AXIPR_UDP
          /* Neuen Descriptor eintragen und alten schliessen */
          close(fd_udp);
#else
          /* Nur wenn alter Socket noch lebt,  */
          if (fd_udp != EOF)
            /* dann schliessen wir den Socket. */
            close(fd_udp);
#endif /* AXIPR_UDP */

            fd_udp = tmp_fd;
            /* Gibt es einen Eintrag. */
            if (l1pp)
              /* Check, ob neuer Filedescriptor groesser ist */
              l1pp->kisslink = max(fd_ip, fd_udp);

            /* Neuen UDP-Port merken */
            my_udp = htons((unsigned short)new_udp);
          }
          else
          {
            putmsg("ERROR: Changing UDP-Port failed, Port not changed !!!\r");
            return;
          }
#ifndef AXIPR_UDP
        }
        else
        {
          /* neuen UDP-Port merken */
          my_udp = htons(new_udp);
        }
#endif

        putmsg("UDP-Port successfully changed\r");
#ifdef AXIPR_UDP
        return;
#else
        break;
#endif /* AXIPR_UDP. */

      /* Timeout fuer dynamische Routen aendern. */
      case 'T':

      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        mbp = getmbp();
        putprintf(mbp, "the actual timeout for dynamic routes is : %u\r", uDynTimeout);
        /* und ab die Post ... */
        prompt(mbp);
        seteom(mbp);
        return;
      }

      /* Timeout lesen */
      uNewTimeout = nxtlong(&clicnt, &clipoi);

      if (  (uNewTimeout < 0)
          ||(uNewTimeout > (86400)))  /* max. 1 Tag */
      {
        putmsg("error: timeout out of range (0 - 86400) !\r");
        return;
      }

      /* Wert uebernehmen */
      uDynTimeout = uNewTimeout;

      /* und ab die Post ... */
      mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
      return;

#ifdef AXIPR_UDP
      /* Loglevel aendern. */
      case 'L':

      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        mbp = getmbp();
#ifdef SPEECH
        putprintf(mbp, speech_message(299),loglevel);
#else
        putprintf(mbp, "My LogLevel: %d\r",loglevel);
#endif
        /* und ab die Post ... */
        prompt(mbp);
        seteom(mbp);
        return;
      }

      /* Loglevel lesen */
      NewLogLevel = nxtnum(&clicnt, &clipoi);

      if (  (NewLogLevel < 0)
          ||(NewLogLevel > 4))
      {
#ifdef SPEECH
         putmsg(speech_message(300));
#else
         putmsg("errors: Log level worth from 0 to 4!\r");
#endif
         return;
      }

      /* Wert uebernehmen */
      loglevel = NewLogLevel;

      /* und ab die Post ... */
      mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
      return;
#endif /* AXIPR_UDP */

#ifdef AXIPR_HTML
      /* htmlstatistik aendern. */
      case 'H':

      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        mbp = getmbp();
#ifdef SPEECH
        putprintf(mbp, speech_message(347),HtmlStat);
#else
        putprintf(mbp, "HTML-Statistic is %d\r", HtmlStat);
#endif
        /* und ab die Post ... */
        prompt(mbp);
        seteom(mbp);
        return;
      }

      /* Loglevel lesen */
      NewHtmlStat = nxtnum(&clicnt, &clipoi);

      if (  (NewHtmlStat < 0)
          ||(NewHtmlStat > 1))
      {
#ifdef SPEECH
         putmsg(speech_message(348));
#else
         putmsg("Error: HTML-Statistic worth from 0 to 1!\r\r");
#endif
         return;
      }

      /* Wert uebernehmen */
      HtmlStat = NewHtmlStat;

      /* und ab die Post ... */
      mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
      return;
#endif

      /* Unbekannte Kommandos */
      default:
        putmsg(inv_par);
        return;
    }
  }
  }

  mbp = putals("AXIP-Routes:\rCall-------IP----------------Mode--Port--Timeout--IP/Hostname----\r");

  /* Alle Routingtabelleneintraege durchgehen und ausgeben */
  for (i = 0; i < route_tbl_top; ++i)
    show_rt_entry(route_tbl[i], mbp);

  /* Default-Route ausgeben wenn vorhanden */
  if (default_route.callsign[0] == 1)
    show_rt_entry(default_route, mbp);

  putprintf(mbp, "-----------------------------------------------------------------\r");

  /* Eigenen UDP-Port ausgeben */
  putprintf(mbp, "UDP-port (%u): ", ntohs(my_udp));
  putprintf(mbp, "%s", (fd_udp != -1 ? "active" : "not active"));

  /* IP-Status ausgeben */
  putprintf(mbp, ", IP-protocol (family %u): ", IPPROTO_AX25);
  putprintf(mbp, "%s", (fd_ip != -1 ? "active" : "not active"));

  /* Timeout des dynamischen Routenlerners ausgeben */
  putprintf(mbp, "\rTimeout for dynamically learned routes is %u seconds.\r", uDynTimeout);

  /* Hostname -> IP-Adresse Aktualisierungsintervall ausgeben */
  putprintf(mbp, "Hostname-to-IP-adress conversion ");

  if (uNamesUpdate != 0)
    putprintf(mbp, "occurs every %u seconds.\r", uNamesUpdate / 100);
  else
    putprintf(mbp, "is disabled.\r");

#ifdef AXIPR_UDP
#ifdef SPEECH
  putprintf(mbp, speech_message(299), loglevel);
#else
  putprintf(mbp, "My LOGLevel is %u.\r", loglevel);
#endif /* SPEECH */
#endif /* AXIPR_UDP */

#ifdef AXIPR_HTML
#ifdef SPEECH
  putprintf(mbp, speech_message(347), HtmlStat);
#else
  putprintf(mbp, "My HTML-Statistic is %d.\r", HtmlStat);
#endif /* SPEECH */
#endif /* AXIPR_HTML */

  /* und ab die Post ... */
  prompt(mbp);
  seteom(mbp);
}

/* Routeneintraege fuer parms.tnb ausgeben */
void dump_ax25ip(MBHEAD* mbp)
{
  register int i;

  putstr(";\r; AX25IP Routes\r;\r", mbp);

  if (ax25ip_active == FALSE)
    return;

  if (new_parser == TRUE)
  {
    putstr("; WARNING: this section is written using the new syntax of the AXIPR-command,\r", mbp);
    putstr(";          DO NOT USE with older TNNs compiled only for old style syntax !!!\r;\r", mbp);

    /* Falls wir noch im alten Parser sind, dann umschalten */
    putprintf(mbp, "AXIPR P\r");
    /* eigenen UDP-Port ausgeben */
    putprintf(mbp, "AXIPR MYUDP %u\r", ntohs(my_udp));
    /* Timeout fuer dynamische Routen ausgeben */
    putprintf(mbp, "AXIPR TIMEOUT %u\r", uDynTimeout);
    /* Intervall fuer Hostnamen-Update ausgeben */
    putprintf(mbp, "AXIPR LOOKUP %u\r", uNamesUpdate);
  }

#ifdef AXIPR_UDP
  /* eigenen UDP-Port ausgeben */
  putprintf(mbp, "AXIPR U %u\r", ntohs(my_udp));
  /* Timeout fuer dynamische Routen ausgeben */
  putprintf(mbp, "AXIPR T %u\r", uDynTimeout);

  putprintf(mbp, "AXIPR L %d\r",loglevel);

#ifdef AXIPR_HTML
  putprintf(mbp, "AXIPR H %d\r",HtmlStat);
#endif /* AXIPR_HTML */
#endif /* AXIPR_UDP */

  /* alle Routingtabelleneintraege durchgehen und ausgeben */
  for (i = 0; i < route_tbl_top; ++i)
  {
    /* Dynamisch gelernte Routen werden inaktiv zur Information geschrieben ! */
    if (route_tbl[i].timeout != 0)
      continue;

/*    putprintf(mbp, "; ");*/

    /* Eintrag einer Route erzeugen */
    putprintf(mbp, (new_parser == FALSE ? "AXIPR R + %s " : "AXIPR ADD %s ")
                 , call_to_a(&route_tbl[i].callsign[0]));

    /* Den Hostnamen ausgeben */
    putprintf(mbp, "%s", route_tbl[i].hostname);

    /* Mode und ggf. Port ausgeben wenn UDP */
    if (route_tbl[i].udp_port != 0)
    {
      putstr((new_parser == FALSE ? " U " : " UDP "), mbp);
      putnum(ntohs(route_tbl[i].udp_port), mbp);
    }

    putstr("\r", mbp);
  }

  /* Die Defaultroute ausgeben wenn gesetzt */
  if (default_route.callsign[0] == 1)
  {
    /* Kommandokopf schreiben */
    putstr((new_parser == FALSE ? "AXIPR D + " : "AXIPR ADD DEFAULT "), mbp);

    /* Den Hostnamen ausgeben */
    putprintf(mbp, "%s", default_route.hostname);

    /* Mode und ggf. Port ausgeben wenn UDP */
    if (default_route.udp_port != 0)
    {
      putstr((new_parser == FALSE ? " U " : " UDP "), mbp);
      putnum(ntohs(default_route.udp_port), mbp);
    }
    putstr("\r", mbp);
  }
}

/************************************************************************/
/* AX25IP Initialisierung                                               */
/************************************************************************/
BOOLEAN ax25ip_l1init(int l2port)
{
  if (ax25ip_active == TRUE)
    return(TRUE);

  /* Socketstrukturen initialisieren */
  memset((char*)&to, 0, sizeof(to));
  memset((char*)&from, 0, sizeof(from));

  ax25ip_port = l2port;
  l1pp = &l1port[l1ptab[l2port]];
  l2flp = (LHEAD *) &txl2fl[l2port];

  /* Routingtabelle initialisieren */
  route_init();

  /* Konfiguration einlesen */
  if (config_read() == FALSE)
    return(FALSE);

  /* Notwendige Sockets anlegen */
#ifndef AXIPR_UDP
  ax25ip_check_up();
#endif

  /* Wir sind fertig */
  ax25ip_active = TRUE;
  return(TRUE);
}

/* Einen Filedescriptor fuer IP anlegen */
int setup_ip(void)
{
  /* Temporaerer Filedescriptor */
  int tmp_fd = -1;

  /* Adressstruktur loeschen */
  memset((char*)&ipbind, 0, sizeof(ipbind));

  /* Rohen IP-Socket anlegen */
  if ((tmp_fd = socket(AF_INET, SOCK_RAW, IPPROTO_AX25)) < 0)
  {
    xprintf("AX25IP : cannot create ip raw-socket: %s\n", strerror(errno));
    LOGL2("AX25IP : cannot create ip raw-socket: %s\n", strerror(errno));
    return(-1);
  }

  /* Nonblocking-IO */
  if (fcntl(tmp_fd, F_SETFL, FNDELAY) < 0)
  {
    xprintf("AX25IP : cannot set non-blocking I/O on ip raw-socket\n");
    LOGL2("AX25IP : cannot set non-blocking I/O on ip raw-socket\n");
    close(tmp_fd);
    return(-1);
  }

  /* Adressstruktur mit notwendigen Werten fuellen */
  ipbind.sin_family = AF_INET;
  ipbind.sin_addr.s_addr = htonl(INADDR_ANY);

  return (tmp_fd);
}

/* Einen Filedescriptor fuer UDP anlegen */
/* Es muss der UDP-Port angegeben werden, auf dem wir hoeren wollen */
/* (Achtung: UDP-Port in Network-Byteorder) */
int setup_udp(unsigned short udp_port)
{
  /* Temporaerer Filedescriptor */
  int tmp_fd = -1;

  /* Port-Check */
  if (udp_port == 0)
    return(-1);

  /* Adressstruktur loeschen */
  memset((char*)&udpbind, 0, sizeof(udpbind));

  /* UDP-Socket anlegen */
  if ((tmp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    xprintf("AX25IP : cannot create socket: %s\n", strerror(errno));
    LOGL2("AX25IP : cannot create socket: %s\n", strerror(errno));
    return(-1);
  }

  /* Nonblocking-IO */
  if (fcntl(tmp_fd, F_SETFL, FNDELAY) < 0)
  {
    xprintf("AX25IP : cannot set non-blocking I/O on udp socket\n");
    LOGL2("AX25IP : cannot set non-blocking I/O on udp socket\n");
    close(tmp_fd);
    return(-1);
  }

  /* Socket-Optionen fuer UDP einstellen */
  /* SO_KEEPALIVE einschalten */
  if (sockopt_keepalive == TRUE)
  {
    int iFlag = 1;

    if (setsockopt(tmp_fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&iFlag, sizeof(int)) < 0)
    {
      xprintf("AX25IP : cannot set SO_KEEPALIVE for udp : %s\n", strerror(errno));
      LOGL2("AX25IP : cannot set SO_KEEPALIVE for udp : %s\n", strerror(errno));
    }
  }

  /* Socket auf max. Durchsatz */
  if (sockopt_throughput == TRUE)
  {
    int iFlag = IPTOS_THROUGHPUT;

    if (setsockopt(tmp_fd, IPPROTO_IP, IP_TOS, (const char *)&iFlag, sizeof(IPTOS_THROUGHPUT)) < 0)
    {
      xprintf("AX25IP : cannot set IPTOS_THROUGHPUT for udp : %s\n", strerror(errno));
      LOGL2("AX25IP : cannot set IPTOS_THROUGHPUT for udp : %s\n", strerror(errno));
    }
  }

  /* Adressstruktur fuellen */
  udpbind.sin_addr.s_addr = htonl(INADDR_ANY);
  udpbind.sin_port = udp_port;
  udpbind.sin_family = AF_INET;

  /* Adresse und Port binden */
  if (bind(tmp_fd, (struct sockaddr *)&udpbind, sizeof(udpbind)) < 0)
  {
    xprintf("AX25IP : cannot bind udp socket: %s\n", strerror(errno));
    LOGL2("AX25IP : cannot bind udp socket: %s\n", strerror(errno));
    close(tmp_fd);
    return(-1);
  }

  return(tmp_fd);
}

/* Pruefen, ob schon Sockets angelegt sind, wenn nicht, dann notwendige */
/* Sockets oeffnen */
void ax25ip_check_up(void)
{
  /* IP notwendig ? */
  if ((count_routes(FALSE) != 0) && (fd_ip == -1))
    fd_ip = setup_ip(); /* IP starten */

  /* UDP notwendig ? */
  if ((count_routes(TRUE) != 0) && (fd_udp == -1))
    fd_udp = setup_udp(my_udp); /* UDP starten */

#ifdef AXIPR_UDP
  if (l1pp != NULL)
#endif
    /* Markieren, ob IP oder UDP laufen */
    l1pp->kisslink = max(fd_ip, fd_udp);
}

/* Pruefen, ob Sockets angelegt sind und diese noch benoetigt werden. */
/* Falls nicht, Sockets schliessen */
void ax25ip_check_down(void)
{
  /* IP notwendig ? */
  if ((count_routes(FALSE) == 0) && (fd_ip != -1))
  {
    close(fd_ip);
    fd_ip = -1;
  }

  /* UDP notwendig ? */
  if ((count_routes(TRUE) == 0) && (fd_udp != -1))
  {
    close(fd_udp);
    fd_udp = -1;
  }

  /* Gibt es einen Eintrag. */
  if (l1pp)
    /* Markieren, ob IP oder UDP laufen */
    l1pp->kisslink = max(fd_ip, fd_udp);
}

/*************************************************************************/
/* AX25IP Pakete empfangen                                               */
/*************************************************************************/
void ax25ip_recv(void)
{
  struct       iphdr *ipptr;
  struct       timeval tv;
  int          l = 0;
  int          iDescriptorsReady = 0;
  register int i = 0;
  register int max_fd = -1;
  socklen_t    fromlen = sizeof(from);
  int          hdr_len = 0;
  UBYTE        buf[MAX_FRAME+1];
  UBYTE        *bufptr;
  MBHEAD       *rxfhd;
  BOOLEAN      UDP_Frame = FALSE;
  fd_set       rmask;

#ifdef AXIPR_UDP
  if (LookAX25IP)
    return;
#endif /* AX25IP_UDP */

  FD_ZERO(&rmask);

  /* IP-Filedescriptor eintragen */
  if (fd_ip != -1)
  {
    FD_SET((unsigned int)fd_ip, &rmask);
    if (fd_ip > max_fd - 1)
      max_fd = fd_ip + 1;
  }

  /* UDP-Filedescriptor eintragen */
  if (fd_udp != -1)
  {
    FD_SET((unsigned int)fd_udp, &rmask);
    if (fd_udp > max_fd - 1)
      max_fd = fd_udp + 1;
  }

  /* ist was aktiv ? */
  if (max_fd == -1)
    return;

  tv.tv_usec = 0;
  tv.tv_sec = 0;

  iDescriptorsReady = select(max_fd, &rmask, NULL, NULL, &tv);

  /* nix da */
  if (iDescriptorsReady == 0)
    return;
 
  /* Fehler */
  if (iDescriptorsReady == -1)
  {
    LOGL2("select()-Error %i: %s", errno, strerror(errno));
    return;
  }

  /* RX fuer IP */
  if ((fd_ip != -1) && (FD_ISSET(fd_ip, &rmask)))
  {
    l = recvfrom(fd_ip, (UBYTE *)(bufptr = buf), MAX_FRAME, 0, (struct sockaddr *) &from, &fromlen);

    if (l > 0)
    {
      if (l > (signed)sizeof(struct iphdr*))
      {
        ipptr = (struct iphdr*)buf;
        hdr_len = 20;

        if (!ok_crc(buf + hdr_len, l - hdr_len))
        { /* stimmt die CRC ? */
          LOGL2("IP-RX: CRC-Error, frame dropped");
          return; /* Fehler */
        }
      }
      else
      {
        LOGL2("IP-RX: frame too short, frame dropped");
        return; /* Fehler */
      }
    }
    i = hdr_len;
  }

  /* RX fuer UDP, aber nur, wenn nicht schon TCP was empfangen hat */
  if ((fd_udp != -1) && (FD_ISSET(fd_udp, &rmask) && (l == 0)))
  {
    l = recvfrom(fd_udp, (UBYTE *)(bufptr = buf), MAX_FRAME, 0, (struct sockaddr *) &from, &fromlen);

    if (l > 0)
    {
      if (!ok_crc(buf, l))
      { /* stimmt die CRC ? */
        LOGL2("UDP-RX: CRC-Error, frame dropped");
        return; /* Fehler */
      }
    }
    UDP_Frame = TRUE;
  }

  /* Ab hier gemeinsame Behandlung */
  /* Erst mal gucken, ob was gekommen ist */
  if (l == 0)
    return; /* nix da */

  l -= 2; /* CRC abschneiden */

  if (l < 15)
  {
    LOGL2("AX25IP: RX frame dropped, length wrong, frame is too short !");
    return;
  }

  /* Hier angekommen haben wir (hoffentlich) ein gueltiges Frame. Die */
  /* weitere Behandlung ist fuer IP und UDP nahezu identisch.         */

  /* Frame in Buffer umkopieren und Empfangsport eintragen */
  rxfhd = cpyflatmb((char*)&buf[i], l - i);
  rxfhd->l2port = ax25ip_port;

  /* Frame in die L2-RX-Kette einhaengen */
  relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);

  /* Jetzt noch das Frame an den Routenlerner zur Analyse */
  if (route_canlearn(rxfhd))
  {
    /* Frame kann zum Lernen benutzt werden */
    /* Absenderrufzeichen ermitteln und konvertieren      */
    /* Wir brauchen hier nicht den eigentlichen Absender, */
    /* sondern den, der als letztes gedigipeated hat.     */
    struct in_addr ipaddr;
#ifndef AXIPR_UDP
    struct route_table_entry* rtptr;
#endif /* AXIPR_UDP */
    unsigned char srccall[L2IDLEN + 1];

    cpyid((char *)srccall, dheardcall(rxfhdr));

    /* Call linksschieben */
    for (i = 0; i < L2CALEN; ++i)
      srccall[i] = srccall[i] << 1;

    /* Gibt es schon eine IP fuer dieses Call, dann kann sich die IP  */
    /* geaendert haben. Falls ja und es sich um einen dynamischen Eintrag */
    /* handelt, dann loeschen und neuen Eintrag anlegen. */
#ifndef AXIPR_UDP
    if ((rtptr = call_to_ip(srccall)) != NULL)
    {
      if ((   (rtptr->ip_addr != from.sin_addr.s_addr)
           || (rtptr->udp_port != (UDP_Frame == TRUE ? ntohs(from.sin_port) : 0)))
           && (rtptr->timeout != 0))
        route_del(srccall); /* Route loeschen */
      else
      {
        if (rtptr != &default_route)
          return;             /* Adresse unveraendert */
      }
    }
#endif /* AXIPR_UDP */

    ipaddr.s_addr = from.sin_addr.s_addr;

#ifndef AXIPR_UDP
    /* Route eintragen */
    route_add((unsigned char*)inet_ntoa(ipaddr), NULL, srccall,
              (UDP_Frame == TRUE ? ntohs(from.sin_port) : 0), FALSE, uDynTimeout);
#else
    route_analyse(ipaddr, srccall,
                  (UDP_Frame == TRUE ? ntohs(from.sin_port) : 0));
  }
#endif /* AXIPR_UDP */
}

/*************************************************************************/
/* AX25IP Pakete senden                                                  */
/*************************************************************************/
void ax25ip_send(void)
{
  unsigned char  buf[MAX_FRAME];
  int            len = 0;
  int            fd = -1;
  MBHEAD        *txfhdl;

  struct route_table_entry *rt;
  struct hostent *hstent;

  unsigned char *call;

  if (kick[ax25ip_port]) /* haben wir was zu senden ? */
  {
    /* Sicherheitsabfrage, da kick[] auch manipuliert werden kann */
    if (l2flp->head == l2flp)
    {
      kick[ax25ip_port] = FALSE;
      return;
    }

    ulink((LEHEAD *)(txfhdl = (MBHEAD *) l2flp->head));/*Zeiger holen*/

    /* Daten aus den internen Puffern in einen linearen Puffer uebertragen */
    len = cpymbflat((char*)&buf[0], txfhdl);

    relink((LEHEAD *)txfhdl,          /* als gesendet betrachten und in */
           (LEHEAD *)stfl.tail);      /* die gesendet Liste umhaengen   */

    kick[ax25ip_port] = ((LHEAD *)l2flp->head != l2flp);

    /* Naechstes Call der Digipeaterkette holen */
    call = next_addr(buf);

    /* IP-Adresse des naechsten Digipeaters ermitteln */
    rt = call_to_ip(call);

    if (rt == NULL) /* wir kennen das Ziel nicht */
    {
#ifdef AXIPR_UDP
       if ((rt = search_route(call)) == NULL)
       {
#endif
         LOGL2("no route for %s, can't send", call_to_a(call));
         return;
       }
#ifdef AXIPR_UDP
    }
#endif

    /* Ein Host, zu dem bisher keine IP-Adresse aufgeloest werden konnte */
    if (rt->ip_addr == 0L)
    {
      /* Versuchen, jetzt eine IP-Adresse zu bestimmen */
      if ((hstent = gethostbyname((char*)rt->hostname)) != NULL)
         memcpy((unsigned char*)&rt->hostname, (unsigned char *)&hstent->h_addr_list[0][0], 4);
    }

    /* Checksumme anhaengen */
    add_ax25crc(buf, len);
    /* CRC-Bytes sind dazugekommen */
    len += 2;
    /* Ziel-IP-Adresse in Sendestruktur eintragen */
    memcpy((char *) &to.sin_addr, (char *)&rt->ip_addr, 4);
    /* Ziel-Port nur bei UDP eintragen */
    if (rt->udp_port != 0)
      memcpy((char *) &to.sin_port, (char *)&rt->udp_port, 2);
    else
      to.sin_port = 0;  /* bei IP kein Port */

    to.sin_family = AF_INET;

    /* Unterscheidung IP oder UDP */
    if (to.sin_port == 0)
      fd = fd_ip;
    else
      fd = fd_udp;

    if (fd == -1)
    {
      LOGL2("error: no descriptor, can't send !!!");
      return;
    }

    if (sendto(fd, (char *)buf, len, 0, (struct sockaddr *)&to, sizeof(to)) < 0)
      LOGL2("error: sendto failed, fd=%u error=%s", fd, strerror(errno));
  }
}

/*************************************************************************/
/* AX25IP Deinitialisierung, Port und Sockets schliessen                 */
/*************************************************************************/
void ax25ip_l1exit(void)
{
  /* Nur wenn aktiv */
  if (ax25ip_active == FALSE)
    return;

  /* IP-Socket schliessen wenn offen */
  if (fd_ip != -1)
  {
    close(fd_ip);
    fd_ip = -1;
  }

  /* UDP-Socket schliessen wenn offen */
  if (fd_udp != -1)
  {
    close(fd_udp);
    fd_udp = -1;
  }

  /* AX25IP nicht mehr aktiv */
  ax25ip_active = FALSE;

  /* Gibt es einen Eintrag. */
  if (l1pp)
    l1pp->kisslink = -1;
}

void ax25ip_l1ctl(int req, int port)
{
  /* nur eigene Ports bearbeiten */
  if ((ax25ip_active == FALSE) || (kissmode(port) != KISS_AXIP))
    return;

  switch (req)
  {
    /* Testpattern auf dem Port senden (wird unterdrueckt) */
    case L1CTST : testflag[port] = FALSE;
                  kick[port] = FALSE;
                  break;

    default : break;
  }
}

void ax25ip_hwstr(int port, MBHEAD *mbp)
{
#ifdef AXIPR_UDP
    putstr(" ", mbp);
    /* My UDP-Port ausgeben. */
    putprintf(mbp,"%u", ntohs(my_udp));
#endif
}

BOOLEAN ax25ip_dcd(int port)
{
  return (FALSE);
}


/* routing.c    Routing table manipulation routines
 *
 * Copyright 1991, Michael Westerhof, Sun Microsystems, Inc.
 * This software may be freely used, distributed, or modified, providing
 * this header is not removed.
 *

 * Modified for use in TNN by DG1KWA & DG9OBU

 */



/* Initialize the routing table */
void route_init(void)
{
  /* Defaultroute initialisieren */
  memset(&default_route, 0 , sizeof(struct route_table_entry));

  /* Routen initialisieren */
  memset(&route_tbl, 0, (TABLE_SIZE * sizeof(struct route_table_entry)));
}

/* Add a new route entry */
#ifndef AXIPR_UDP
static void route_add(unsigned char* hostname, struct hostent *hstent, unsigned char *call, int udpport,
                      int default_rt, int timeout)
#else
static BOOLEAN route_add(unsigned char *host, struct hostent *hstent, unsigned char *call, int udpport,
                         int default_rt, int timeout, char *hostname
#ifdef AXIPR_HTML
                      , char *timeset, UWORD protokoll
#endif /* AXIPR_HTML */
                      )
#endif /* AXIPR_UDP */
{
   register unsigned int i = 0;
   struct route_table_entry *rtptr;
   struct hostent *hst;

   /* Eintrag soll Defaultroute sein */
   if (default_rt)
   {
      for (i = 1; i < 7 ;++i)
        default_route.callsign[i] = 0;

      if (hstent != NULL)
        memcpy(&default_route.ip_addr, (unsigned char *)&hstent->h_addr_list[0][0], 4);
      else
        memset(&default_route.ip_addr, 0, 4);

      default_route.udp_port = htons((unsigned short)udpport);
      default_route.timeout = 0;                /* Defaultroute ohne Timeout */

      if (hostname != NULL)
        strncpy((char*)&default_route.hostname, (char*)hostname, HNLEN);

      default_route.callsign[0] = 1;            /* mark valid */

      /* Socket ggf. installieren */
      ax25ip_check_up();

#ifndef AXIPR_UDP
      return;
#else
      return(TRUE);
#endif /* AXIPR_UDP */
   }

   /* Normale Routen nur wenn Call vorhanden */
   if (call == NULL)
#ifndef AXIPR_UDP
     return;
#else
     return(FALSE);
#endif /* AXIPR_UDP */

   /* Check auf doppelte Eintraege, Eintrag wird nur angenommen wenn */
   /* es noch noch keinen Eintrag unter diesem Call gibt. */
   if (((rtptr = call_to_ip(call)) != NULL) && (rtptr->ip_addr != default_route.ip_addr))
   {
#ifndef AXIPR_UDP
     putmsg("A route to this callsign is already set up, delete it first.\r");
     return;
#else
#ifndef AXIPR_HTML
     UpdateRoute(call, udpport, timeout, hostname);
#else
     UpdateRoute(call, udpport, timeout, hostname, timeset, protokoll);
#endif /* AXIPR_HTML */
     return(FALSE);
#endif /* AXIPR_UDP */
   }

   /* Passt noch ein Eintrag in die Tabelle ? */
   if (route_tbl_top >= TABLE_SIZE)
   {
     xprintf("Routing table is full; entry ignored.\n");
     LOGL2("Routing table is full; entry ignored");
     putmsg("Routing table is full, entry ignored\r");
#ifndef AXIPR_UDP
     return;
#else
     return(FALSE);
#endif /* AXIPR_UDP */
   }

   /* Call eintragen */
   for (i = 0; i < 6; ++i)
     route_tbl[route_tbl_top].callsign[i] = call[i] & 0xfe;

   /* SSID eintragen */
   route_tbl[route_tbl_top].callsign[6] = (call[6] & 0x1e) | 0x60;

   if (hstent != NULL && hstent->h_length != 0)
   {
     memcpy((unsigned char*)&route_tbl[route_tbl_top].ip_addr, (unsigned char *)&hstent->h_addr_list[0][0], 4);
   }
   else
     memset(&route_tbl[route_tbl_top].ip_addr, 0, sizeof(route_tbl[route_tbl_top].ip_addr));

   route_tbl[route_tbl_top].udp_port = htons((unsigned short)udpport);
   route_tbl[route_tbl_top].timeout = timeout;

   if (hostname != NULL)
   {
     memcpy(&route_tbl[route_tbl_top].hostname, hostname, strlen((char*)hostname));
     /* Keine Adressaufloesung bisher, dann probieren wir es noch einmal */
     if (hstent == NULL)
     {
       if ((hst = gethostbyname((char*)hostname)) != NULL)
         memcpy((unsigned char*)&route_tbl[route_tbl_top].ip_addr, &hst->h_addr_list[0][0], 4);
     }
   }

#ifdef AXIPR_UDP
   /* Original UDP-Port eintragen. */
   route_tbl[route_tbl_top].org_udp_port = htons((UWORD)udpport);
#endif

#ifdef AXIPR_HTML
   /* Hostname eintragen. */
   (void)memcpy(route_tbl[route_tbl_top].timeset, timeset, 17);
   /* Protokoll eintragen. */
   route_tbl[route_tbl_top].protokoll = P_USER;
   /* Route als offline markieren. */
   route_tbl[route_tbl_top].online = FALSE;
#endif

   /* Ein Routeneintrag mehr */
   route_tbl_top++;

   /* Socket ggf. installieren */
   ax25ip_check_up();

#ifndef AXIPR_UDP
   return;
#else
   return(TRUE);
#endif /* AXIPR_UDP */
}

/*
 * Return an IP address and port number given a callsign.
 * We return a pointer to the route structure for this callsign
 * or a NULL-pointer if no route was found.
 */

struct route_table_entry* call_to_ip(unsigned char *call)
{
   register int i;
   unsigned char mycall[7];

   /* Leere Calls koennen nicht verarbeitet werden */
   if (call == NULL)
     return (NULL);

   /* Call-Bytes kopieren, letztes Bit abschneiden (entspricht >> 1 & 7F) */
   for (i = 0; i < 6; ++i)
     mycall[i] = call[i] & 0xfe;

   /* SSID kopieren */
   mycall[6] = (call[6] & 0x1e) | 0x60;

   /* Routingtabelle durchsuchen */
   for (i = 0; i < route_tbl_top; ++i)
   {
     if (addrmatch(mycall , route_tbl[i].callsign))
     {
       /* Aktivitaet auf diesem Eintrag, dann Timeout erneuern */
       if (route_tbl[i].timeout)
         route_tbl[i].timeout = uDynTimeout;

       return (&route_tbl[i]); /* Eintrag gefunden */
     }
   }

   /* Kein Routingeintrag vorhanden, Default-Route melden wenn vorhanden */
   if (default_route.callsign[0])
      return (&default_route);

   /* Kein Routingeintrag gefunden, keine Defaultroute */
   return (NULL);
}

/*
 * tack on the CRC for the frame.  Note we assume the buffer is long
 * enough to have the two bytes tacked on.
 */
void add_ax25crc(unsigned char *buf, int l)
{
   unsigned short int u = compute_crc(buf, l);

   buf[l] = u & 0xff;              /* lsb first */
   buf[l + 1] = (u >> 8) & 0xff;   /* msb next */
}


/*
 **********************************************************************
 * The following code was taken from Appendix B of RFC 1171
 * (Point-to-Point Protocol)
 *
 * The RFC credits the following sources for this implementation:
 *
 *   Perez, "Byte-wise CRC Calculations", IEEE Micro, June, 1983.
 *
 *   Morse, G., "Calculating CRC's by Bits and Bytes", Byte,
 *   September 1986.
 *
 *   LeVan, J., "A Fast CRC", Byte, November 1987.
 *
 *
 * The HDLC polynomial: x**0 + x**5 + x**12 + x**16
 */

/*
 * u16 represents an unsigned 16-bit number.  Adjust the typedef for
 * your hardware.
 */
/*typedef unsigned short u16;*/


/*
 * FCS lookup table as calculated by the table generator in section 2.
 */
static UWORD fcstab[256] = {
   0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
   0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
   0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
   0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
   0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
   0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
   0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
   0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
   0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
   0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
   0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
   0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
   0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
   0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
   0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
   0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
   0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
   0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
   0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
   0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
   0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
   0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
   0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
   0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
   0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
   0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
   0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
   0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
   0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
   0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
   0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
   0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

#define PPPINITFCS      0xffff  /* Initial FCS value */
#define PPPGOODFCS      0xf0b8  /* Good final FCS value */

/*
 * Calculate a new fcs given the current fcs and the new data.
 */
UWORD pppfcs(register UWORD fcs, register unsigned char *cp, register int len)
{
    while (len--)
       fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff];

    return(fcs);
}

/*
 * End code from Appendix B of RFC 1171
 **********************************************************************
 */

/*
 *  The following routines are simply convenience routines...
 *  I'll merge them into the mainline code when suitably debugged
 */

/* Return the computed CRC */
UWORD compute_crc(unsigned char *buf, int l)
{
   UWORD fcs = (pppfcs(PPPINITFCS, buf, l) ^ 0xFFFF);

   return (fcs);
}

/* Return true if the CRC is correct */
BOOLEAN ok_crc(unsigned char *buf, int l)
{
   UWORD fcs = pppfcs(PPPINITFCS, buf, l);

   return (fcs == PPPGOODFCS);
}

/* return true if the addresses supplied match */
int addrmatch(unsigned char *a, unsigned char *b)
{
   if ((*a == '\0') || (*b == '\0')) return(0);

   if ((*a++ ^ *b++) & 0xfe) return(0);   /* "K" */
   if ((*a++ ^ *b++) & 0xfe) return(0);   /* "A" */
   if ((*a++ ^ *b++) & 0xfe) return(0);   /* "9" */
   if ((*a++ ^ *b++) & 0xfe) return(0);   /* "W" */
   if ((*a++ ^ *b++) & 0xfe) return(0);   /* "S" */
   if ((*a++ ^ *b++) & 0xfe) return(0);   /* "B" */
   if ((*a++ ^ *b++) & 0x1e) return(0);   /* ssid */
   return(1);
}

/* return pointer to the next station to get this packet */
unsigned char *next_addr(unsigned char *f)
{
   unsigned char *a;

   /* If no digis, return the destination address */
   if (NO_DIGIS(f)) return(f);

   /* check each digi field.  The first one that hasn't seen it is the one */
   a = f + 7;
   do {
      a += 7;
      if (NOTREPEATED(a)) return(a);
   } while (NOT_LAST(a));

/* all the digis have seen it.  return the destination address */
   return(f);
}

/* Open and read the config file */
int config_read(void)
{
   FILE *cf;
   char buf[256], cbuf[256];
   int errflag = 0;
   int e = 0;
   int lineno = 0;
   char cfgfile[256];

   strcpy(cfgfile, confpath);
   strcat(cfgfile, CONFIG_FILE);

   if ((cf = fopen(cfgfile, "r")) == NULL)
   {
#ifdef INIPATH
     strcpy(cfgfile, INIPATH);
     strcat(cfgfile, CONFIG_FILE);

     if ((cf = fopen(cfgfile, "r")) == NULL)
     {
#endif
#ifdef AXIPR_UDP
       /* AX25IP.CFG ist nicht mehr noetig,   */
       /* kann aber weiterhin benutzt werden. */
       return(TRUE);
#endif
       xprintf("AX25IP: Config file %s not found or could not be opened\n", CONFIG_FILE);
       return(FALSE);
#ifdef INIPATH
     }
#endif
   }

   while (fgets(buf, 255, cf) != NULL)
   {
     (void)strcpy(cbuf, buf);
     ++lineno;

     if ((e = parse_line(buf)) < 0)
     {
       xprintf("Config error at line %d: ", lineno);

       switch (e)
       {
         case -1 : xprintf("Missing argument\n"); break;
         case -2 : xprintf("Bad callsign format\n"); break;
         case -3 : xprintf("Bad option - on/off\n"); break;
         case -4 : xprintf("Bad option - tnc/digi\n"); break;
         case -5 : xprintf("Host not known\n"); break;
         case -6 : xprintf("Unknown command\n"); break;
         case -7 : xprintf("Text string too long\n"); break;
         case -8 : xprintf("Bad option - every/after\n"); break;
         case -9 : xprintf("Bad option - ip/udp\n"); break;

         default: xprintf("Unknown error\n"); break;
       }
       xprintf("%s", cbuf);
       ++errflag;
     }
   }

   if (errflag)
     exit(1);

   if (mode == NO_MODE)
   {
     xprintf("Must specify ip and/or udp sockets\n");
     return(FALSE);
   }

   fclose(cf);

   return(TRUE);
}

/* Process each line from the config file.  The return value is encoded. */
int parse_line(char *buf)
{
   char *p;
   char *q;
   char hostname[HNLEN];
   unsigned char tcall[7];
   struct hostent hstent;
   struct hostent *he;
   int i = 0;
   int uport = 0;
   int dfalt = 0;
#ifdef AXIPR_HTML
   char *timeset = set_time();
#endif

   p = strtok(buf, " \t\n\r");

   memset(&hstent, 0, sizeof(hstent));

   /* Leere Zeilen */
   if (p == NULL)
     return(0);

   /* Kommentarzeilen nicht auswerten */
   if (*p == '#')
     return(0);

   if (strcmp(p, "socket") == 0)
   {
     q = strtok(NULL, " \t\n\r");

     if (q == NULL)
       return(-1);

     if (strcmp(q, "ip") == 0)
       mode = IP_MODE;
     else if (strcmp(q, "udp") == 0)
     {
        mode = UDP_MODE;
        my_udp = htons(DEFAULT_UDP_PORT);

        /* Bei UDP optionale Portnummer lesen */
        q = strtok(NULL, " \t\n\r");

        /* Nummer war vorhanden, auswerten */
        if (q != NULL)
        {
           i = atoi(q);
           /* Nummer ist gueltig, uebernehmen */
           if (i > 0)
             my_udp = htons((unsigned short)i);
        }
     }
     else
       return(-9);

     return(0);
   }
   else if (strcmp(p, "socketoption") == 0)
     {
       q = strtok(NULL, " \t\n\r");

       /* Pruefung auf leere Zeile */
       if (q == NULL)
         return(-1);

       if (strcmp(q, "SO_KEEPALIVE") == 0)
       {
          sockopt_keepalive = TRUE;
       }
       else if (strcmp(q, "IPTOS_THROUGHPUT") == 0)
       {
          sockopt_throughput = TRUE;
       }
       else return(-9);

     return(0);

    } else if (strcmp(p, "loglevel") == 0) {
      q = strtok(NULL, " \t\n\r");
      if (q == NULL) return(-1);
      loglevel = atoi(q);
      return(0);

   } else if (strcmp(p, "route") == 0) {
      uport = 0;
      dfalt = 0;

      q = strtok(NULL, " \t\n\r");

      /* Pruefung auf leere Zeile */
      if (q == NULL)
        return(-1);

      /* Pruefung auf default-Route */
      if (strcmp(q, "default") == 0)
        dfalt = 1;
      else
      {
         /* Ziel der Route pruefen */
         if (a_to_call(q, tcall) != 0)
           return(-2);
      }

      /* Hostnamen holen, dies kann auch eine IP-Adresse sein */
      q = strtok(NULL, " \t\n\r");
      if (q == NULL)
        return(-1);

      he = gethostbyname(q);

      /* Keine gueltige IP-Adresse ? */
      if (he == NULL)
      {
        /* Da route_add() eine hostent-Struktur braucht, bauen wir ihm eine. */
        /* Den Hostname sichern und den Zeiger auf den Puffer im hostent eintragen. */
        strncpy(hostname, q, HNLEN);

         he = &hstent;
         he->h_name = hostname;
      }

      q = strtok(NULL, " \t\n\r");

      if (q != NULL)
      {
        if (strcmp(q, "udp") == 0)
        {
          uport = DEFAULT_UDP_PORT;

          q = strtok(NULL, " \t\n\r");
          if (q != NULL)
          {
            i = atoi(q);
            if (i > 0)
               uport = i;
          }
        }
      }
#ifndef AXIPR_UDP
      route_add((unsigned char*)he->h_name, he, tcall, uport, dfalt, 0);
#else
      route_add((unsigned char*)he->h_name, he, tcall, uport, dfalt, 0, (char*)he->h_name
#ifdef AXIPR_HTML
               , timeset, P_USER
#endif /* AXIPR_HTML */
               );
#endif /* AXIPR_UDP */
      return(0);
   }
   return(-999);
}


/* Convert ascii callsign to internal format */
int a_to_call(char *text, unsigned char *tcall)
{
   register size_t  i = 0;
   int           ssid = 0;
   unsigned char c;

   if (strlen(text) == 0)
     return(-1);

   for (i = 0; i < 6; ++i)
     tcall[i] = (' ' << 1);

   tcall[6] = '\0';

   for (i = 0; i < strlen(text); ++i)
   {
     c = text[i];
     if (c == '-')
     {
       ssid = atoi(&text[i + 1]);
       if (ssid > 15)
         return(-1);
       tcall[6] = (ssid << 1);
       return(0);
     }

     if (islower(c))
       c = toupper(c);

     if (i > 5)
       return(-1);

     tcall[i] = (c << 1);
   }
   return(0);
}

/* Convert internal callsign to printable format */
char *call_to_a(unsigned char *tcall)
{
   int i = 0;
   int ssid = 0;
   char *tptr;
   static char t[10];

   for (i = 0, tptr = t; i < 6; ++i)
   {
     if (tcall[i] == (' ' << 1))
       break;
     *tptr = tcall[i] >> 1;
     tptr++;
   }

   ssid = (tcall[6] >> 1) & 0x0f;
   if (ssid > 0)
   {
     *tptr = '-';
     tptr++;

     if (ssid > 9)
     {
       *tptr = '1';
       tptr++;
       ssid -= 10;
     }
     *tptr = '0' + ssid;
     tptr++;
   }

   *tptr = '\0';
   return(&t[0]);
}

void write_log(const char *format, ...)
{
  FILE *fp;
  va_list arg_ptr;
  struct tm *lt;

  fp = fopen("ax25ip.log", "a+");

  if (fp != NULL)
  {
    lt = localtime(&sys_time);
    fprintf(fp, "%16.16s:", ctime(&sys_time));
    va_start(arg_ptr, format);
    vfprintf(fp, format, arg_ptr);
    va_end(arg_ptr);
    fprintf(fp, "\n");
    fclose(fp);
  }
  else
    loglevel = 0;   /* Logging aus da Fehler mit der Logdatei */
}

#ifdef AXIPR_UDP
/* AX25IP-Frame untersuchen, wenn Aenderungen     */
/* z.B. UDP-Port, IP-Adresse in TBL Aktualisieren.*/
void route_analyse(struct in_addr ip, unsigned char *call, int uport)
{
  register int   i;
  char           mycall[7];

#ifdef AXIPR_HTML
  char        *timeset = set_time();
#endif /* AXIPR_HTML */

  /* Leere Calls koennen nicht verarbeitet werden */
  if (call[0] == FALSE)
    return;

   /* Call-Bytes kopieren, letztes Bit abschneiden (entspricht >> 1 & 7F) */
   for (i = 0; i < 6; ++i)
     mycall[i] = call[i] & 0xfe;

   /* SSID kopieren */
   mycall[6] = (call[6] & 0x1e) | 0x60;

   /* Routingtabelle durchsuchen */
   for (i = 0; i < route_tbl_top; ++i)
   {
      if (addrmatch((unsigned char *)mycall , route_tbl[i].callsign))
      {
        /* Bei Aenderung uebernehmen */
        if (memcmp(&ip, &route_tbl[i].ip_addr, 4) != 0)
          /* IP-Adresse hat sich geaendert */
          memcpy(&route_tbl[i].ip_addr, &ip, 4);

        /* UDP-Port unterschiedlich? */
        if (route_tbl[i].udp_port != htons((UWORD)uport))
           /* UDP-Port updaten. */
           route_tbl[i].udp_port = htons((UWORD)uport);

#ifdef AXIPR_HTML
        /* Route ggf. auf online setzen. */
        if (!route_tbl[i].online)
          route_tbl[i].online = TRUE;
#endif
        /* Aktivitaet auf diesem Eintrag, dann Timeout erneuern */
        if (route_tbl[i].timeout)
          route_tbl[i].timeout = uDynTimeout;

        /* Aktualisierung beendet. */
        return;
      }
   }

#ifndef AXIPR_HTML
   /* Auto-Route eintragen */
   route_add((unsigned char*)inet_ntoa(ip), NULL, call,uport, FALSE, uDynTimeout, (char*)inet_ntoa(ip));
#else
   /* Auto-Route eintragen */
   route_add((unsigned char*)inet_ntoa(ip), NULL, call,uport, FALSE, uDynTimeout, (char*)inet_ntoa(ip), timeset, P_USER);
#endif /* AXIPR_HTML */
}

/* Per Rufzeichen, ohne die SSID zubeachten, einen */
/* Routeneintrag suchen. Gibt es keinen Eintrag,   */
/* wird sofort abgebrochen, ansonsten werden die   */
/* routeneintraege IP-Adresse und UDP-Port ueber-  */
/* nommen und das neue Rufzeichen wird eingetragen.*/
struct route_table_entry *search_route(unsigned char *call)
{
  register int i;
  char         srccall[10];
#ifdef AXIPR_HTML
  char        *timeset = set_time();
#endif /* AXIPR_HTML */

  /* Konvertiere Rufzeichen. */
  callss2str((char *)srccall, call_to_a(call));

  /* TB durchgehen. */
  for (i = 0; i < route_tbl_top; ++i)
  {
    /* Suche Rufzeichen. */
    if (cmpcal(srccall, call_to_a(&route_tbl[i].callsign[0])))
    {
#ifndef AXIPR_HTML
      /* Auto-Route eintragen */
      route_add((unsigned char *)&route_tbl[i].ip_addr, NULL, (unsigned char *)call, htons(route_tbl[i].org_udp_port), FALSE, uDynTimeout, (char *)route_tbl[i].hostname);
#else
      /* Auto-Route eintragen */
      route_add((unsigned char *)&route_tbl[i].ip_addr, NULL, (unsigned char *)call, htons(route_tbl[i].org_udp_port), FALSE, uDynTimeout, (char *)route_tbl[i].hostname, timeset, P_USER);
#endif /* AXIPR_HTML */
      /* Aktuellen Eintrag weiterreichen. */
       return (&route_tbl[i]); /* Eintrag gefunden */
    }
  }
 /* Kein Rufzeichen gefunden. */
 return (NULL);
}

/* Eine AX-Route Updaten. */
void UpdateRoute(unsigned char *srccall,
                  int           udpport,
                  int           timeout,
                  char         *hostname
#ifdef AXIPR_HTML
                , char          *timeset
                , UWORD          protokoll
#endif /* AXIPR_HTML */
)
{
  MBHEAD      *mbp;
  register int i;

  /* TB durchgehen. */
  for (i = 0; i < route_tbl_top; ++i)
  {
    /* Suche Rufzeichen. */
    if (addrmatch((unsigned char *)srccall , route_tbl[i].callsign))
    {
      route_tbl[i].org_udp_port = route_tbl[i].udp_port = htons((unsigned short)udpport);
      /* Hostname eintragen. */
      (void)memcpy(route_tbl[i].hostname, hostname, HNLEN);
      /* ggf. Timeout setzen. */
      route_tbl[i].timeout = timeout;

#ifdef AXIPR_HTML
      /* Hostname eintragen. */
      (void)memcpy(route_tbl[i].timeset, timeset, 17);
      /* Protokoll eintragen. */
      route_tbl[i].protokoll = P_USER;
#endif /* AXIPR_HTML */

      mbp = getmbp();

      /* und ab die Post ... */
      prompt(mbp);
      seteom(mbp);
      return;
    }
  }
}

#ifdef AXIPR_HTML

/* Protokoll fuer HTML-Ausgabe setzen. */
void SetHTML(int port, char *call, PEER *pp, BOOLEAN flag)
{
  register int  i;
  char          srccall[10 + 1];

  call2str(srccall, call);

  /* Routingtabelle durchsuchen */
  for (i = 0; i < route_tbl_top; ++i)
  {
    if (!strcmp(srccall , call_to_a(&route_tbl[i].callsign[0])))
    {
      /* Protokoll setzen. */
      set_status(i, port, pp);

      /* Route auf online/offline stellen. */
      route_tbl[i].online = flag;

      break;
    }
  }
}

/* Aktualisiere Loginzeit fuer TB */
char *set_time(void)
{
  time_t      go;
  struct tm  *zeit;
  static char c[17];
  char       *timeset = c;

  /* aktuelle Kalenderzeit */
  time(&go);
  /* konvertiert Datum und Uhrzeit. */
  zeit = localtime(&go);
  /* Formatiere eine Zeit-/Datumsangabe */
  strftime(c,17,"%d-%b-%H:%M:%S",zeit);

/* Aktuelle Login Date/Time */
return(timeset);
}

/* Protokoll setzen. */
void set_status(register int a, int port, PEER *pp)
{
  if (pp == NULL)                                  /* Route wird ausgetragen. */
  {
    route_tbl[a].protokoll = P_OFFLINE;          /* Route auf OFFLINE setzen. */
    route_tbl[a].online    = FALSE;
    return;
  }

  switch(pp->typ)                                            /* Protokol-TYP. */
  {
    case INP    :                                                 /* INP-TYP. */
     route_tbl[a].protokoll = P_INP;                       /* INP-TYP setzen. */
    break;

    case TNN    :                                                  /* TNN-TYP */
    case THENET :                                              /* THENET-TYP. */
      route_tbl[a].protokoll = P_THENET;            /* TNN/THENET-TYP setzen. */
     break;

    case FLEXNET:                                             /* FLEXNET-TYP. */
      route_tbl[a].protokoll = P_FLEXNET;              /* FLEXNET-TYP setzen. */
     break;

    default:
      route_tbl[a].protokoll = P_OFFLINE;        /* Route auf OFFLINE setzen. */
      route_tbl[a].online    = FALSE;
     return;
  }

  route_tbl[a].online    = TRUE;
}

/* Timer fuer htmlstatistik. */
void h_timer(void)
{
  static LONG h_Timer = 0L;

  /* Timer abgelaufen. */
  if (h_Timer <= 0L)
  {
    /* Timer neu setzen. */
    /* fest auf 60 sec.  */
    h_Timer = 60L;
    /* html auf Platte schreiben. */
    w_statistik();
  }
  /* Timer runter zaehlen. */
  h_Timer--;
}

/* Hiermit checken wir ob die rstat.css existiert. */
/* FALSE = keine rstat.css gefunden                */
/* TRUE  = es existiert eine rstat.css             */
static int check_css_file(void)
{
  char cssfile[255];

  /* Path kopieren. */
  strcpy(cssfile, htmlpath);
  /* CSS-Dateiname anhaengen.  */
  strcat(cssfile, CSS_RFILE);

  /* Pruefe ob Datei existiert.*/
  if (!access(cssfile,FALSE))
      /* Datei existiert. */
          return(TRUE);
  else
      /* Nix gefunden! */
          return(FALSE);
}

/* Eine default RSTAT.CSS schreiben */
static void rstat_css_default_file(void)
{
  FILE *fp;
  char cfgfile[255];

  if (HtmlStat == FALSE)
      return;

  strcpy(cfgfile, htmlpath);
  strcat(cfgfile, CSS_RFILE);
  if ((fp = fopen(cfgfile, "w+t")) == NULL)
      return;

  fprintf(fp,"BODY {\n"
             " BACKGROUND-IMAGE: url('bg.jpg');\n"
             " BACKGROUND-COLOR: #000080;\n"
             " COLOR: #FFFFFF;\n"
             " FONT-FAMILY: Verdana, Arial;\n"
             " FONT-SIZE: 12;\n"
             "}\n");

  fprintf(fp,"H1 {\n"
             " FONT-SIZE: 18; COLOR: #FFCC00;\n"
             "}\n");

  fprintf(fp,"H2 {\n"
             " FONT-SIZE: 12; COLOR: #FFFFFF;\n"
             "}\n");

  fprintf(fp,"a:link, a:visited {\n"
             " COLOR: #FFFFFF;\n"
             " text-weight: bold;\n"
             " text-decoration: none;\n"
             "}\n");

  fprintf(fp,"a:hover {\n"
             " COLOR: #FF6600;\n"
             " text-weight: bold;\n"
             " text-decoration: none;\n"
             "}\n"
             ".info {\n"
             " FONT-SIZE: 12; COLOR: #99CCFF;\n"
             "}\n");

  fprintf(fp,"table.box {\n"
             " background-color: #ffbb00;\n"
             " backcolor: #220000;\n"
             " backcolorlight: #00ff00;\n"
             " backcolordark: #000000;\n"
             " border-width: 0px;\n"
             " width: 95%%;\n"
             " border-style: outset;\n"
             " border-spacing: 2px;\n"
             " border-padding: 8px;\n"
             "}\n");
  
fprintf(fp,".status {\n"
             " FONT-SIZE: 10; BACKGROUND: #000000; COLOR: #FFCC00; TEXT-ALIGN: center;\n"
             "}\n");

  fprintf(fp,".off {\n"
             " FONT-SIZE: 10; BACKGROUND: #000000; COLOR: #ffffff;\n"
             "}");

  fprintf(fp,".offline {\n"
             " FONT-SIZE: 10; BACKGROUND: #9f00FF; COLOR: #ffffff;\n"
             "}");

  fprintf(fp,".user {\n"
             " FONT-SIZE: 10; BACKGROUND: #E06060; COLOR: #ffffff;\n"
             "}\n");

  fprintf(fp,".thenet {\n"
             " FONT-SIZE: 10; BACKGROUND: #E00060; COLOR: #ffffff;\n"
             "}\n");

  fprintf(fp,".inp {\n"
             " FONT-SIZE: 10; BACKGROUND: #0070C0; COLOR: #ffffff;\n"
             "}\n");

  fprintf(fp,".Flexnet {\n"
             " FONT-SIZE: 10; BACKGROUND: #006060; COLOR: #ffffff;\n"
             "}\n");

  fprintf(fp,".about {\n"
             " FONT-SIZE: 13; COLOR: #ffffff;\n"
             "}\n");

  fclose(fp);
}

/* Wie viel Ports sind nich frei ? */
static int free_ports(void)
{
  int free = FALSE;

  return(free = TABLE_SIZE - route_tbl_top);
}

/* Wie viel L2-Links (USER) - sind aktiv ? */
static int activ_user(void)
{
  register int a;
  int          user = FALSE;

  /* TB durchgehen */
  for (a = 0; a < route_tbl_top; a++)
    /* nur die als User markiert sind */
    if (   route_tbl[a].protokoll == P_USER
        /* und online sind !!!        */
        && route_tbl[a].online > 0)
        /* Alle USER zaehlen */
        user++;

  return(user);
}

/* Wie viel L2/L4_links sind aktiv ? */
static int activ_links(void)
{
  register int a;
  int          links = FALSE;

  /* TB durchgehen */
  for (a = 0; a < route_tbl_top; a++)
    /* L2/L4-Link online ?      */
    if (   route_tbl[a].online > 0
    /* wir zaehlen alle Protokolle, ausser USER */
        && route_tbl[a].protokoll != 1)
        links++;

  return(links);
}

/* Die Aktuelle Uhrzeit fuer HTML ausgeben */
static char *puttim_stat(void)
{
  struct      tm *p;
  static char tim[20];
  time_t      timet;
  char       *atime = tim;

  time(&timet);

  p = localtime(&timet);
  sprintf(tim,"%02i.%02i.%02i %02i:%02i:%02i",
              p->tm_mday,  p->tm_mon+1, p->tm_year % 100,
              p->tm_hour, p->tm_min, p->tm_sec);

  return(atime);
}

static char *set_class(UWORD flag, register int a)
{
  static char    o[8];
  char *protokoll = o;

  switch(flag)
  {
    case P_OFFLINE :
     if (!route_tbl[a].online)
       strcpy(o, "offline");

    break;

    case P_USER :
    if (route_tbl[a].online)
       strcpy(o, "user");
     else
       strcpy(o, "offline");
     break;

    case P_THENET :
    if (route_tbl[a].online)
       strcpy(o, "thenet");
     else
       strcpy(o, "offline");
     break;

    case P_INP :
     if (route_tbl[a].online)
       strcpy(o, "inp");
     else
       strcpy(o, "offline");

     break;

    case P_FLEXNET :
     if (route_tbl[a].online)
       strcpy(o, "flexnet");
     else
       strcpy(o, "offline");

     break;

    default:
      strcpy(o, "unknown");
     break;
  }

 return(protokoll);
}

static char *set_pname(register int a)
{
  static char p[8];
  char *protokoll = p;

  switch(route_tbl[a].protokoll)
  {
    case P_USER :
      strcpy(p, "L2");
     break;

    case P_THENET :
      strcpy(p, "THENET");
     break;

    case P_INP :
      strcpy(p, "INP");
     break;

    case P_FLEXNET :
      strcpy(p, "FLEXNET");
     break;

    default:
      strcpy(p, "UNKNOWN");
     break;
  }

return(protokoll);
}

static void all_route(FILE *fp, UWORD flag, register int a)
{
  char *pclass = set_class(flag, a);
  char *protokoll = set_pname(a);

  /* Nummer einer Route. */
  fprintf(fp,"  <td class=\"%s\"><center>%d</td>\n"
            , pclass, a);

  /* Rufzeichen. */
  fprintf(fp,"  <td class=\"%s\">%s</td>\n"
            , pclass
            , call_to_a(&route_tbl[a].callsign[0]));

  /* IP-Adresse. */
  fprintf(fp,"  <td class=\"%s\">"
             "  <a href=\"http://%s\" target=\"_http\">%s</a></td>\n"
            , pclass
            , route_tbl[a].hostname
            , route_tbl[a].hostname);

  /* Hostname/IP-Adresse. */
  fprintf(fp,"  <td class=\"%s\"><center>%s</td>\n"
            , pclass
            , route_tbl[a].hostname);

  /* UDP-Port */
  fprintf(fp,"  <td class=\"%s\"><center>%d</td>\n"
            , pclass
            , htons(route_tbl[a].udp_port));

  /* Loginzeit */
  fprintf(fp,"  <td class=\"%s\"><center>%s</td>\n"
            , pclass
            , route_tbl[a].timeset);

  if (route_tbl[a].timeout)
    /* Timeout */
    fprintf(fp,"  <td class=\"%s\"><center>%d</td>\n"
              , pclass
              , route_tbl[a].timeout);
  else
    /* Statische Route */
    fprintf(fp,"  <td class=\"%s\"><center>Statisch</td>\n"
              , pclass);

  /* Protokoll */
  fprintf(fp,"  <td class=\"%s\"><center>%s</td></tr>\n"
            , pclass
            , protokoll);
}

static void show_route(FILE *fp, UWORD flag, register int a)
{
  switch(flag)
  {
    case P_OFFLINE :
      if (!route_tbl[a].online)
         fprintf(fp," <tr class=\"offline\">\n");

      break;

    case P_USER :
      if (   route_tbl[a].protokoll == flag
          && route_tbl[a].online)
         fprintf(fp," <tr class=\"user\">\n");
      else
         fprintf(fp," <tr class=\"offline\">\n");

      break;

    case P_THENET :
      if (   route_tbl[a].protokoll == flag
          && route_tbl[a].online)
         fprintf(fp," <tr class=\"thenet\">\n");
      else
         fprintf(fp," <tr class=\"offline\">\n");

      break;


    case P_INP :
      if (   route_tbl[a].protokoll == flag
          && route_tbl[a].online)
         fprintf(fp," <tr class=\"inp\">\n");
      else
         fprintf(fp," <tr class=\"offline\">\n");

      break;

    case P_FLEXNET :
      if (   route_tbl[a].protokoll == flag
          && route_tbl[a].online)
         fprintf(fp," <tr class=\"flexnet\">\n");
      else
         fprintf(fp," <tr class=\"offline\">\n");

      break;


    default:
      if (   route_tbl[a].protokoll == flag
          && route_tbl[a].online)
         fprintf(fp," <tr class=\"unknow\">\n");
      else
         fprintf(fp," <tr class=\"offline\">\n");

     break;
  }
  all_route(fp, flag, a);
}

/* Schreibe htmlstatistik auf Platte. */
void w_statistik(void)
{
  FILE         *fp;
  char          cfgfile[255],mycall[10];
  char         *atime = puttim_stat();
  register int  a;
  char          runtime[8 + 1];
#ifndef CONNECTTIME
  unsigned long upt,
                upd,
                uph;
#endif /* CONNECTTIME */

  /* Pruefe, ob htmlstatistik eingeschaltet ist. */
  if (HtmlStat == FALSE)
      /* Ist nicht eingeschaltet, abbruch. */
      return;
  else
  {
    /* Pruefe ob CSS-Datei exisiert. */
    if (!check_css_file())
       /* Nein, dann neuschreiben.  */
       rstat_css_default_file();

  }

  strcpy(cfgfile,htmlpath );
  strcat(cfgfile, HTML_RFILE);
  if ((fp = fopen(cfgfile,"w+t")) == NULL)
      return;

  call2str(mycall,myid);

#ifndef CONNECTTIME
  upt = sys_time - start_time;                 /* Uptime in seconds     */
  upd = upt / SECONDS_PER_DAY;                 /* Uptime days           */
  upt %= SECONDS_PER_DAY;
  uph = upt / SECONDS_PER_HOUR;                /* Uptime hours          */
  upt %= SECONDS_PER_HOUR;

  if (upd > 0)
    sprintf(runtime, "%2lud,%2luh", upd, uph);
  else
    if (uph > 0)
      sprintf(runtime, "%2luh,%2lus", uph, upt);
    else
      if (upt > 0)
        sprintf(runtime, "    %2lus", upt);
      else
        sprintf(runtime, "       ");
#else
  sprintf(runtime, "%s", ConnectTime(tic10 / 100));
#endif /* CONNECTTIME */

  fprintf(fp,"<html>"
             "<head>\n"
             "<meta http-equiv=\"refresh\" content=\"%d\">\n"
             "<title>AXIPR-Statistik</title>\n"
             "<link rel=stylesheet type=text/css href=rstat.css>\n"
             "</head>\n<body>\n",HTML_INV);

  fprintf(fp,"<DIV ALIGN=\"CENTER\"><CENTER>\n"
             "<H1><U>%s%s)</U></H1>\n",signon,mycall);

  fprintf(fp,"<H2>Date/Time: %s Runtime: %s, %d Frei</H2>\n\n",atime, runtime, free_ports());
  fprintf(fp,"<FONT class=\"info\"><B> %d L2/L4-LINKS</b></FONT>\n",activ_links());
  fprintf(fp,"<table class=\"box\" cellspacing=\"2\" cellpadding=\"2\" style=\"border-collapse: collapse\">\n");
  fprintf(fp," <tr class=\"status\">\n"
             "  <td class=\"status\">Nr.</td>\n"
             "  <td class=\"status\">Rufzeichen</td>\n"
             "  <td class=\"status\">IP-Adresse</td>\n"
             "  <td class=\"status\">DYNDNS</td>\n"
             "  <td class=\"status\">UDP</td>\n"
             "  <td class=\"status\">Login</td>\n"
             "  <td class=\"status\">Timeout</td>\n"
             "  <td class=\"status\">Protokoll</td>\n"
             " </tr>\n");

   /* TB durchgehen. */
   for (a = 0; a < route_tbl_top; a++)
   {
     /* Nur aktive Routen zeigen. */
     if (route_tbl[a].online == FALSE)
       /* zum naechsten Eintrag. */
       continue;

     /* Nur INP- oder FLEXNET-Routen bearbeiten */
     if (route_tbl[a].protokoll > P_USER)
       /* und anzeigen. */
       show_route(fp, route_tbl[a].protokoll, a);
   }

   fprintf(fp,"</table><br><br>\n");

   fprintf(fp,"<FONT class=\"info\"><B>%d USER</B></FONT>\n",activ_user());
   fprintf(fp,"<table class=\"box\" cellspacing=\"2\" cellpadding=\"2\" style=\"border-collapse: collapse\">\n");
   fprintf(fp," <tr class=\"status\">\n"
              "  <td class=\"status\">Nr.</td>\n"
              "  <td class=\"status\">Rufzeichen</td>\n"
              "  <td class=\"status\">IP-Adresse</td>\n"
              "  <td class=\"status\">DYNDNS</td>\n"
              "  <td class=\"status\">UDP</td>\n"
              "  <td class=\"status\">Login</td>\n"
              "  <td class=\"status\">Timeout</td>\n"
              "  <td class=\"status\">Protokoll</td>\n"
              " </tr>\n");

   /* TB durchgehen */
   for (a = 0; a < route_tbl_top; a++)
   {
     /* Nur aktive Routen zeigen. */
     if (route_tbl[a].online == FALSE)
       /* zum naechsten Eintrag. */
       continue;

     /* Nur USER-Routen bearbeiten */
     if (route_tbl[a].protokoll == P_USER)
       /* und anzeigen. */
       show_route(fp, route_tbl[a].protokoll, a);
   }

   fprintf(fp,"</table>\n\n");
   fprintf(fp,"<BR><FONT class=\"info\"><B>My UDP-Port: %u</B></FONT>\n",htons(my_udp));
   fprintf(fp,"</body></html>\n");
   fclose(fp);
}

#endif /* AXIPR_HTML */
#endif /* AXIPR_UDP */

#endif

/* End of os/win32/ax25ip.c */
