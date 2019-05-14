
#include "tnn.h"
#ifdef OS_STACK

static T_INTERFACE *ifpp;              /* Zeiger auf das aktuelle Interface   */

struct sockaddr_in myaddr_in;
struct sockaddr_in peeraddr_in;


/* Schliesse aktuellen Socket. */
BOOLEAN CloseSockOS(BOOLEAN ALL, WORD INTERFACE)
{
  int i,
      j = 0;
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(CloseSockOS)";
#endif /* DEBUG_MODUS */

  /* ALLE Socket-Verbindungen schliessen. */
  if (ALL == TRUE)
  {
    /* TCPIP-TBL durchgehen. */
    for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi)
    {
      /* Ist Socket noch offen? */
      if (   INTERFACE == tcppoi->Interface
          && tcppoi->sock > TRUE)
      {
        if (tcppoi->mode != OS_MODE)                /* Segment kein OS-STACK, */
          continue;                                 /* zum naechsten Segment. */

        /* Socket schliessen. */
        close(tcppoi->sock);

        /* TCPIP-User/link aus der Liste raushaengen */
        /* bzw. Parameter wieder auf 0 setzen.       */
        DiscTCP();
      }

      /* Sind alle aktiven Sockets durchlaufen, */
      if (((tcppoi->activ) && (j++ >= tcp_tbl_top)) || (j == tcp_tbl_top))
        /* brechen wir ab. */
        return(FALSE);
    }
  }

  /* Socket schliessen. */
  close(tcppoi->sock);

  /* TCPIP-User/Link aus der Liste raushaengen */
  /* bzw. Parameter wieder auf 0 setzen.       */
  DiscTCP();
  return(FALSE);
}

/* TCPIP-Port Initialisierung. */
BOOLEAN L1InitOS(UWORD Interface, int l2port, unsigned short TCPPort)
{
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(L1InitOS)";
#endif /* DEBUG_MODUS */

  if (l2port == EOF)                                  /* Keine Portangabe!!!. */
  {
    printf("(L1InitOS): Fehler, keine L2-Portangabe!!!\n");
    return(FALSE);
  }
                                        /* Zeiger auf das aktuelle Interface. */
  if ((ifpp = SearchIf(Interface)) == NULL)
  {
    printf("(L1InitOS): Fehler, Interface existiert nicht!!!\n");
    return(FALSE);                                              /* Abbrechen. */
  }

                                       /* Einen neuen Filedescriptor anlegen. */
  if ((ifpp->OsSock = SetupOS(ifpp, htons(TCPPort))) == EOF)
    return(FALSE);                                              /* Abbrechen. */

  ifpp->l2port  = l2port;                                  /* L2-Port setzen. */
  ifpp->tcpport = TCPPort;                           /* akt. TCP-Port setzen. */

  return(TRUE);                               /* Erfolgreich Socket erstellt. */
}

/* TCPIP-Interface schliessen. */
void L1ExitOS(UWORD Interface)
{
  T_INTERFACE *ifpoi;
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(L1ExitOS)";
#endif /* DEBUG_MODUS */

  /* Zeiger auf das aktuelle Interface. */
  if ((ifpoi = SearchIf(Interface)) == NULL)
    /* Aktuelles Interface ist deaktiv, */
    /* dann zum naechsten Interface. */
    return;

    /* Alle TCPIP-Verbindungen auf den Interface schliessen. */
    CloseSockOS(TRUE, ifpoi->Interface);

  /* Interface ist nicht aktiv, */
  if (ifpoi->actively == FALSE)
    /* damit ist hier schon schluss. */
    return;

  /* TCPIP-Socket schliessen. */
  close(ifpoi->OsSock);

  /* Markiere TCPIP-Interface als deaktiv. */
  ifpoi->actively = FALSE;
}

                                        /* Einen Filedescriptor anlegen.      */
int SetupOS(T_INTERFACE   *ifpp,       /* Zeiger auf das aktuelle Interface. */
             unsigned short tcp_port)   /* Neuer TCP-Port.                    */
{
  int tmp_fd = EOF;                     /* Temporaerer Filedescriptor         */
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(SetupOS)";
#endif /* DEBUG_MODUS */

  if (tcp_port == FALSE)                /* Kein Port angegeben.               */
  {
    T_LOGL1(TRUE, "(SetupOS):\nKein TCP-Port angegeben.\n");
    return(EOF);                        /* Abbruch.                           */
  }

  memset(&myaddr_in, 0, sizeof(myaddr_in)); /* Adressstruktur loeschen        */
  memset(&peeraddr_in, 0, sizeof(peeraddr_in));
  myaddr_in.sin_family = AF_INET;             /* Adressstruktur fuellen       */
  myaddr_in.sin_port = htons(tcp_port);       /* TCP-Port setzen.             */
  myaddr_in.sin_addr.s_addr = INADDR_ANY;  /* Jedes Netzwerk Geraet abfragen. */

                                           /* Socket erstellen.               */
  if ((tmp_fd = socket(AF_INET, SOCK_STREAM, 0)) == EOF)
  {
    printf("Error %s: Socket cannot be constructed!\r", ifpp->name);

    T_LOGL1(TRUE, "(SetupOS):\nNeuer Socket kann nicht geoeffnet werden.\n");
    return(EOF);                           /* Keine freien Socket's.          */
  }

                                           /* Bind Initialisieren.            */
  if (bind (tmp_fd, (struct sockaddr *) &myaddr_in, sizeof (struct sockaddr_in)) == EOF)
  {
    printf("Error %s: Bind cannot be initialized!\n", ifpp->name);
    close(tmp_fd);                         /* Schliesse Socket.               */

    T_LOGL1(TRUE, "(SetupOS):\nBind Initialisierung fehlgeschlagen.\n");
    return(EOF);                           /* Fehler beim Bind     .          */
  }

  if (listen (tmp_fd, 3) == EOF)           /* Listen Initialisieren.          */
  {
    printf("Error %s: listen cannot be initialized!\n", ifpp->name);
    close(tmp_fd);                         /* Schliesse Socket.               */

    T_LOGL1(TRUE, "(SetupOS):\nListen Initialisierung fehlgeschlagen.\n");
    return(EOF);                           /* Fehler beim Listen.             */
  }

  T_LOGL1(TRUE, "(SetupOS):\nTCP-Port (%d) erfolgreich gesetzt.\n"
                , ntohs(tcp_port));
  return(tmp_fd);                          /* Aktuelle Filedescriptor.        */
}


/* Anhand des Sockets den User aus der Liste holen. */
TCPIP *FdReadOS(fd_set *fds)
{
  int    i,
         j = 0;
  LHEAD *h_llp;

  h_llp = &tcpactl;
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(FdReadOS)";
#endif /* DEBUG_MODUS */

  /* TCPIP-TBL durchgehen. */
  for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi)
  {
    if (tcppoi->mode != OS_MODE)
      continue;

    /* Nur wenn aktiv! */
    if (tcppoi->activ)
    {
      /* Vergleiche die Sockets */
      if (FD_ISSET(tcppoi->sock, fds))
      {
        /* wir haben einen Link gefunden        */
        ulink((LEHEAD *)tcppoi);
        relink((LEHEAD *)tcppoi, (LEHEAD *)h_llp->tail);
        return(tcppoi);
      }
    }
    /* Sind alle aktiven Sockets durchlaufen, */
    if (((tcppoi->activ) && (j++ >= tcp_tbl_top)) || (j == tcp_tbl_top))
      /* brechen wir ab. */
      break;
  }

  /* Keinen Eintrag gefunden. */
  return(NULL);
}

int ReadSockOS(void)                     /* Empfangene TCPIP Packete.         */
{
  fd_set          fdsI;
  struct timeval  timevalue;
  int             ret = EOF;
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(ReadSockOS)";
#endif /* DEBUG_MODUS */

  if (tcppoi->rxc >= tcppoi->RecvLen)       /* Sind alle Zeichen verarbeitet. */
  {
    if (!tcppoi->mode)                                           /* OS-Stack. */
    {
      FD_ZERO(&fdsI);                          /* Inhalt loeschen.            */
      FD_SET((unsigned)tcppoi->sock, &fdsI);   /* Socket setzen.              */

      timevalue.tv_usec = 0;
      timevalue.tv_sec  = 0;
                                               /* Pruefe auf aktivitaet.      */
      ret = select(tcppoi->sock + 1, &fdsI, NULL , NULL ,&timevalue);

      if (ret <= 0)
        return((int)NULL);               /* Keine aktivitaet auf den Socket.  */
      else
        /* Zeichen vom Socket empfangen. */
        tcppoi->RecvLen = recv (tcppoi->sock, tcppoi->rxbuf, RXLEN,0);
    }

    if (tcppoi->RecvLen < 1)             /* Fehler beim Empfang.              */
    {
      if (tcppoi->RecvLen == EOF)        /* Socket wurde mittlerweile geloes. */
      {
        ifpp = SetInterface(tcppoi->Interface);     /* akt. Interface setzen. */
                                                     /* ggf. Logbuch fuehren. */
        T_LOGL1(TRUE, "(ReadSockOS):%s\n (Recv) Fehler beim empfang, Segment wird auf DISC gesetzt!\n"
                  , tcppoi->ip);


        tcppoi->disflg |= 0x80;          /* Segment auf DISC stellen.         */
        return((int)NULL);
      }

      if (  (ret)
          &&(tcppoi->RecvLen == 0))
      {
        ifpp = SetInterface(tcppoi->Interface);     /* akt. Interface setzen. */
                                                     /* ggf. Logbuch fuehren. */
        T_LOGL1(TRUE, "(ReadSockOS):%s\n(Recv) Keine Zeichen, Segment wird auf DISC gesetzt!\n"
                  , tcppoi->ip);

        tcppoi->disflg |= 0x80;          /* Segment auf DISC stellen.         */
        return((int)NULL);
      }
    }

    tcppoi->rxc                    = 0;  /* RX-Zaehler auf 0 setzen.          */
    tcppoi->rxbuf[tcppoi->RecvLen] = 0;  /* Nullzeichen setzen.               */
                                         /* ggf. Logbuch fuehren.             */

    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL3(TRUE, "(ReadSockOS):%s\nInput:\r%s\r"
                  , tcppoi->ip
                  , tcppoi->rxbuf);
  }

  ret = tcppoi->rxbuf[tcppoi->rxc++];    /* Zeichen vom rxbuf holen.          */

  return(ret);                           /* Aktuelle Zeichen weiterreichen.   */
}

static int OsStackReadIndicationsSock(void)   /* Zeichen vom Socket einlesen. */
{
  TRILLIAN ok = ERRORS;
  char     s;
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(OsStackReadIndicationsSock)";
#endif /* DEBUG_MODUS */

  while(1)                               /* Alle Zeichen von Socket einlesen. */
  {
    s = ReadSockOS();                    /* Zeichen vom Socket.               */

    if (tcppoi->cmdlen >= RXLEN)         /* Maximale Buffergroesse erreicht,  */
    {
      ifpp = SetInterface(tcppoi->Interface);
      T_LOGL1(TRUE, "(OsStackReadIndicationsSock):%s\nMaximale Buffergroesse erreicht!\n"
                    , tcppoi->ip);
       return(TRUE);
    }

    switch(tcppoi->Interface)            /* Interface.                        */
    {
#ifdef L1TELNET
      case KISS_TELNET :                        /* TELNET-Server.             */
        if ((ok = GetContensTEL(s)) == ERRORS)  /* Aktuelle Zeichen auswerten.*/
          break;                         /* Zeichen verarbeitet, kein Return. */
        else
          if (ok == YES)                   /* Zeichen verarbeitet und Return. */
          {
            tcppoi->cmd[tcppoi->cmdlen] = 0; /* Nullzeichen setzen.           */
            return(TRUE);                    /* Frame ist komplett.           */
          }
          else
            continue;                          /* Sind noch Zeichen im RXBUF, */
                                               /* zum naechsten Zeichen.      */
       break;
#endif /* L1TELNET. */

#ifdef L1HTTPD
      case KISS_HTTPD :                         /* HTTPD-Server.              */
        if ((ok = GetContensHTP(s)) == ERRORS)  /* Aktuelle Zeichen auswerten.*/
          break;                         /* Zeichen verarbeitet, kein Return. */
        else
          if (ok == YES)                   /* Zeichen verarbeitet und Return. */
          {
            tcppoi->cmd[tcppoi->cmdlen] = 0; /* Nullzeichen setzen.           */
            return(TRUE);                    /* Frame ist komplett.           */
          }
          else
            continue;                          /* Sind noch Zeichen im RXBUF, */
                                               /* zum naechsten Zeichen.      */
       break;
#endif /* L1HTTPD. */

#ifdef L1IPCONV
      /* IPCONV */
      case KISS_IPCONV :
        /* Aktuelle Zeichen auswerten. */
        if ((ok = GetContensCVS(s)) == ERRORS)
          /* Alle Zeichen verarbeitet, aber kein Return. */
          break;
        else
          /* Alle Zeichen verarbeitet und Return? */
          if (ok == YES)
          {
            /* User/Link noch kein Login. */
            if (!tcppoi->login)
            {
              /* Pruefe auf Login. */
              if (!strncmp(tcppoi->cmd, "/na", 3))
                /* Kein Prompt senden. */
                tcppoi->LoginPrompt = TRUE;
            }
            tcppoi->cmd[tcppoi->cmdlen] = 0;
            /* Frame komplett. */
            return(TRUE);
          }
          else
            /* Sind noch Zeichen im RXBUF,*/
            /* zum naechsten Zeichen. */
            continue;

       break;
#endif /* L1IPCONV */

#ifdef L1IRC
      /* IPCONV */
      case KISS_IRC :
        /* Aktuelle Zeichen auswerten. */
        if ((ok = GetContensCVS(s)) == ERRORS)
          /* Alle Zeichen verarbeitet, aber kein Return. */
          break;
        else
          /* Alle Zeichen verarbeitet und Return? */
          if (ok == YES)
          {
            tcppoi->cmd[tcppoi->cmdlen] = 0;
            /* Frame komplett. */
            return(TRUE);
          }
          else
            /* Sind noch Zeichen im RXBUF,*/
            /* zum naechsten Zeichen. */
            continue;

       break;
#endif /* L1IRC */

      default :                                /* Ungueltiges Interface.      */
        break;                                 /* Hier werden wir nie kommen. */
    }

    return(FALSE);                             /* Frame noch nicht komplett.  */
  }

  return(FALSE);                               /* Frame noch nicht komplett.  */
}

void ServOS(void)                     /* Empfangene Zeichen in Frames packen. */
{
  READRX      *SRx;
  register int i;
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(ServOS)";
#endif /* DEBUG_MODUS */

  if (OsStackReadIndicationsSock())        /* Empfangene Zeichen analysieren. */
  {
    if(tcppoi->cmdlen > 0)                       /* Es sind Zeichen im Buffer.*/
    {
      if ((SRx = (READRX *)allocb(ALLOC_L1TCPIP)) == NULL)/* RX-Seg. besorgen */
      {
        ifpp = SetInterface(tcppoi->Interface);
        T_LOGL1(TRUE, "(ServOS):%s\nSpeicher (%d) ist voll!\n"
                      , tcppoi->ip
                      , nmbfre);
      tcppoi->disflg |= 0x80;               /* Segment auf Disconnect setzen. */
      return;                                  /* Kein Segment frei, abbruch. */
      }

      if ((SRx->Data = SetBuffer()) == NULL)              /* Buffer besorgen. */
      {
        dealoc((MBHEAD *)ulink((LEHEAD *)SRx));      /* RX-Segment entsorgen. */

        ifpp = SetInterface(tcppoi->Interface);
        T_LOGL1(TRUE, "(ServOS):%s\nSpeicher (%d) ist voll!\n"
                      , tcppoi->ip
                      , nmbfre);
        tcppoi->disflg |= 0x80;             /* Segment auf Disconnect setzen. */
        return;                                  /* Buffer ist voll, abbruch. */
      }

      SRx->Sock      = tcppoi->sock;                     /* Socket setzen.    */
      SRx->Interface = tcppoi->Interface;                /* Inetrface setzen. */
      SRx->Mode      = tcppoi->mode;                  /* Stack-Mode setzen.   */

      for (i = 0; i < tcppoi->cmdlen; ++i)
        putchr(tcppoi->cmd[i], SRx->Data);                 /* Buffer fuellen. */

      tcppoi->cmdlen = 0;                    /* Buffer-Zaehler zuruecksetzen. */
      tcppoi->cmd[0] = 0;                              /* Nullzeichen setzen. */

      if (tcppoi->activ)                            /* Nur wenn Socket aktiv. */
        relink((LEHEAD *) SRx, (LEHEAD *)rxflRX.tail);/* Ab in die Empf.-liste*/
      else
        {
          ifpp = SetInterface(tcppoi->Interface);   /* akt. Interface setzen. */
          T_LOGL1(TRUE, "(ServOS):%s\nSegment ist nicht mehr aktiv!\n"
                      , tcppoi->ip);

          if (SRx->Data != NULL)
            dealmb(SRx->Data);                        /* Buffer entsorgen.    */

          dealoc((MBHEAD *)ulink((LEHEAD *)SRx));     /* RX-Segment entsorgen.*/
        }                                     /* Socket ist nicht mehr aktiv. */
    }                                              /* kein Zeichen im buffer. */
  }                                         /* Frame ist noch nicht komplett. */
}

/* Neue User hinzufuegen, vorrausgesetzt es sind noch freie Sockets vorhanden.*/
int AddUserOS(T_INTERFACE *ifpoi,                     /* TCP-Interface.       */
               unsigned     NewSocket,                /* Neuer Socket.        */
               char        *ip)                       /* IP-Adresse.          */
{
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(AddUserOS)";
#endif /* DEBUG_MODUS */

  while ((LHEAD *)tcpfrel.head != &tcpfrel)
  {
    tcppoi = (TCPIP *)ulink((LEHEAD *)tcpfrel.head);
    SetDefaultWorthTCP(NewSocket,         /* Defaultwerte setzen, neuer Socket. */
                         ip,                             /* IP-Adresse.         */
                         ifpoi->l2port,                  /* L2-Port.            */
                         ifpoi->Interface,               /* TCPIP-Interface.    */
                         OS_MODE);                                 /* OS-STack. */

    ServOS();                           /* Empfangene Zeichen in Frames packen. */
    return(FALSE);
  }

  ifpp = SetInterface(ifpoi->Interface);           /* akt. Interface setzen. */
                                                     /* ggf. Logbuch fuehren. */
  T_LOGL1(TRUE, "(AddUserOS):%s\nKann kein neues TCPIP-Segment anlegen !\n"
              , ip);
  return(TRUE);
}

void ListenTCP_OS(void)
{
  fd_set         rmask;
  struct timeval timevalue;
  UWORD          Interface = KISS_TCPIP;
  int            max_fd;
  register int   i;
  int            count,
                 j = 0;
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(ListenTCP_OS)";
#endif /* DEBUG_MODUS */

  max_fd = 0;
  FD_ZERO(&rmask);

  /* Interface-Socket's setzen. */
  for (i = 0; i < MAXINTERFACE; i++, Interface++)
  {
    /* Zeiger auf das aktuelle Interface. */
    if ((ifpp = SearchIf(Interface)) == NULL)
      continue;

    if (ifpp->actively == FALSE)
      continue;

    /* Interface-Socket setzen. */
    FD_SET((unsigned)ifpp->OsSock, &rmask);

    if (ifpp->OsSock > max_fd - 1)
      max_fd = ifpp->OsSock + 1;
  }

                                                     /* User-Socket's setzen. */
  for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi)
  {
    if (tcppoi->activ)                    /* Nur wenn Interface Socket aktiv! */
    {
      if (tcppoi->mode != OS_MODE)                          /* Kein OS-STACK, */
        continue;                                   /* zum naechsten Segment. */

      if (tcppoi->sock < 1)                           /* Kein Socket gesetzt, */
        continue;                                   /* zum naechsten Segment. */

      FD_SET((unsigned)tcppoi->sock, &rmask);                /* Socket setzen */

      if (tcppoi->sock > max_fd - 1)
        max_fd = tcppoi->sock + 1;
    }

    if (  ((tcppoi->activ)          /* Sind alle aktiven Sockets durchlaufen, */
        && (j++ >= tcp_tbl_top))
        || (j   == tcp_tbl_top))
      break;                                               /* brechen wir ab. */
  }

  timevalue.tv_usec = 0;
  timevalue.tv_sec  = 0;

  count = select(max_fd, &rmask, NULL, NULL, &timevalue);

  if (count < 1)
    return;

  Interface = KISS_TCPIP;

  for (i = 0; i < MAXINTERFACE; i++, Interface++)
  {
    if ((ifpp = SearchIf(Interface)) == NULL)       /* Ungueltiges Interface, */
      continue;                                   /* zum naechsten Interface. */

    if (ifpp->actively == FALSE)                    /* Interface nicht aktiv, */
      continue;                                   /* zum naechsten Interface. */


    if (FD_ISSET(ifpp->OsSock, &rmask))         /* Interface-Socketvergleich. */
    {
      char      ip[IPADDR];
      int       NewSock;
      socklen_t addrlen;
      ULONG     peerip = 0;
      char     *p = (char *)&peerip;

      addrlen = sizeof peeraddr_in;

      if ((NewSock = accept(ifpp->OsSock, (struct sockaddr *) &peeraddr_in, &addrlen)) != EOF)
      {
        peerip = peeraddr_in.sin_addr.s_addr;
        sprintf (ip, "%d.%d.%d.%d",(p[0] & 255)
                                  ,(p[1] & 255)
                                  ,(p[2] & 255)
                                  ,(p[3] & 255));
        /* Einen Neuen TCPIP-User/Link hinzufuegen. Bekannt  */
        /* ist das Interface, der Socket und die IP-Adresse. */
        if (AddUserOS(ifpp, NewSock, ip))
        {
          close(NewSock);
          return;
        }

        /* ggf. Logbuch fuehren. */
        ifpp = SetInterface(tcppoi->Interface);
        T_LOGL2(TRUE, "(ListenTCP_OS):\nVerbindung zur IP-Adresse %s angenommen.\n"
                      , ip);
        return;
      }
    }
  }

  /* Anhand des Sockets den User aus der Liste Holen */
  if ((tcppoi = FdReadOS(&rmask)) != NULL)
  {
    /* ggf. Logbuch fuehren. */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL3(TRUE, "(ListenTCP_OS):%s\nAktuellen Socket (%d) in der Liste gefunden.\n"
                  , tcppoi->ip
                  , tcppoi->sock);

    /* TCPIP Packete empfangen/bearbeiten/senden*/
    ServOS();
    return;
  }
}


/* Frame aus der Sendeliste senden. */
int PutflushSockOS(void)
{
  int ok  = FALSE;
  int ret = FALSE;
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(PutflushSockOS)";
#endif /* DEBUG_MODUS */

  if (tcppoi->txc)                                  /* Gibt es was zu senden. */
  {
    while (1)                                       /* Gibt es was zu senden. */
    {
      fd_set wfdsI;
      struct timeval Tv;

      FD_ZERO(&wfdsI);                                 /* Loesche inhalt. */
      FD_SET((unsigned)tcppoi->sock, &wfdsI);           /* Socket setzen. */

      if (select(tcppoi->sock + 1, NULL, &wfdsI , NULL ,&Tv))/* Sender frei.*/
      {
        if (tcppoi->txstatus == TCP_TX_BUSY)        /* Sender ist auf Busy. */
          tcppoi->txstatus = TCP_TX_FREE;       /* Sender auf frei stellen. */

                                                            /* Frame senden */
        if ((ok = send(tcppoi->sock, tcppoi->txbuf + tcppoi->sum, tcppoi->txc - tcppoi->sum,0)) < 0)
        {
          ifpp = SetInterface(tcppoi->Interface);
          T_LOGL1(TRUE, "(PutflushSockTCP):%s\nFrame senden fehlgeschlagen!\n"
                          , tcppoi->ip);

          tcppoi->disflg |= 0x80;         /* Segment auf Disconnect setzen. */
          return(TRUE);                              /* Abbruch.            */
        }

        ifpp = SetInterface(tcppoi->Interface);                     /* ggf. Logbuch fuehren. */
        T_LOGL3(TRUE, "(PutflushSockOS):%s\nOutput:\r%s\r"
                      , tcppoi->ip
                      , tcppoi->txbuf);

        tcppoi->sum += ok;                  /* Markiere, gesendete Zeichen. */

        if (tcppoi->sum == tcppoi->txc)           /* Alle Zeichen gesendet. */
          break;                                     /* Schleife verlassen. */

      }

      if (ret == FALSE)                           /* Sender ist nicht frei. */
      {
        tcppoi->txstatus = TCP_TX_BUSY;         /* Sender auf Busy stellen. */
        return(FALSE);
      }
    }

    tcppoi->sum = 0;                                        /* Zuruecksetzen. */
    tcppoi->txc = 0;
    tcppoi->txbuf[0] = 0;
  }

  return(FALSE);
}

BOOLEAN CheckSocketOS(void)
{
  int ok = EOF;
#ifdef DEBUG_MODUS
  lastfunc = "ostcpip(CheckSocketOS)";
#endif /* DEBUG_MODUS */

  if ((ok = send(tcppoi->sock, "", 0, 0)) < 0)          /* Socket noch aktiv. */
  {
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(CheckSocket):%s\nSocket ist nicht mehr aktiv!\n"
              , tcppoi->ip);

    tcppoi->disflg |= 0x80;                       /* Segment auf DISC setzen. */
    return(TRUE);                              /* Socket ist nicht mehr aktiv */
  }
  else
    return(FALSE);                                       /* Socket ist aktiv. */
}

#endif /* OS_STACK */

/* End of os/linux/ostcpip.h */
