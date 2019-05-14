#include "tnn.h"
#ifdef L1TCPIP

T_INTERFACE ifp[MAXINTERFACE];         /* Interface-Liste.                    */

LHEAD tcpfrel;                         /* Liste der freien Linkbloecke        */
LHEAD tcpactl;                         /* Liste der aktiven Linkbloecke.      */
LHEAD tcptatl;

LHEAD  rxflRX;                         /* Empfangsliste                       */
LHEAD  rxflTX;                         /* Sendeliste                          */

TCPIP *tcppoi;                         /* "link pointer", globaler Zeiger auf */
                                       /* den gerade aktuellen Linkblock      */
TCPIP *tcptbl;                         /* "link table", fuer jeden moeglichen */
                                       /* TCPIP Eintrag                       */

struct Sockaddr_in Myaddr_in;
struct Sockaddr_in Peeraddr_in;

static T_INTERFACE *ifpp;              /* Zeiger auf das aktuelle Interface   */

UWORD  nmbtcp;                         /* Anzahl aktiver TCPIP-Links          */
UWORD  nmbtcp_max;                     /* Maximale anzahl der TCPIP-Links     */

int tcp_tbl_top  = 0;                  /* Zaehler fuer aktive TCP-User.       */

static BOOLEAN CheckSocket(void);             /* Pruefe, ob Socket aktiv ist. */

void InitTCP(void)                     /* TCPIP initialisieren.               */
{
  int i;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(InitTCP)";
#endif /* DEBUG_MODUS */

  inithd(&tcptatl);
  inithd(&tcpactl);
  inithd(&tcpfrel);
  inithd(&rxflRX);
  inithd(&rxflTX);

  for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi)
  {
    tcppoi->ip[0]         = 0;
    tcppoi->rxbuf[0]      = 0;
    tcppoi->txbuf[0]      = 0;
    tcppoi->cmd[0]        = 0;
    tcppoi->sock          = 0;
    tcppoi->Upcall[0]     = 0;
    tcppoi->Downcall[0]   = 0;
    tcppoi->disflg        = 0;
    tcppoi->port          = 0;
    tcppoi->noacti        = 0;
    tcppoi->T3            = 0;
    tcppoi->inlin         = 0;
    tcppoi->outlin        = 0;
    tcppoi->activ         = 0;
    tcppoi->login         = 0;
    tcppoi->cmdlen        = 0;
    tcppoi->RecvLen       = 0;
    tcppoi->rxc           = 0;
    tcppoi->txc           = 0;
    tcppoi->sum           = 0;
    tcppoi->LoginPrompt   = 0;
    tcppoi->state         = L2MNIX;
    tcppoi->cr            = 0;
    tcppoi->txstatus      = 0;
    tcppoi->Interface     = 0;
#ifdef L1HTTPD
    tcppoi->status        = 0;
    tcppoi->http          = 0;
    tcppoi->fp            = NULL;
#endif /* L1HTTPD */
#ifdef L1IPCONV
    tcppoi->CVSlink       = FALSE;
    tcppoi->Intern        = FALSE;
#ifdef L1IRC
    tcppoi->IrcMode       = FALSE;
#endif /* IRC */
#endif /* L1IRC */

    inithd(&tcppoi->inbuf);
    inithd(&tcppoi->outbuf);

    resptc(g_uid(tcppoi, TCP_USER));
    relink((LEHEAD *)tcppoi, (LEHEAD *)tcpfrel.tail);
  }

  nmbtcp = nmbtcp_max = FALSE;         /* TCPIP User/link zaehler.            */
}

void InitIFC(void)                     /* Interface Initialisierung           */
{
  register int Interface;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(InitIFC)";
#endif /* DEBUG_MODUS */
                                       /* alle Interface durchgehen und       */
  for (Interface = 0; Interface < MAXINTERFACE; Interface++)
  {                                    /* Initialisieren.                     */
    ifpp = &ifp[Interface];            /* Zeiger auf auf das akt. Interface.  */
    ifpp->actively  = FALSE;           /* Interface deaktiv                   */
    ifpp->l2port    = EOF;             /* L2-Port.                            */
    ifpp->OsSock    = 0;               /* OS-Socket     .                     */
    ifpp->ISock     = 0;               /* Interne Socket.                     */
    ifpp->log       = 0;               /* Loglevel.                           */

#ifdef L1TELNET
    if (Interface == TEL_ID)
    {
      ifpp->Interface = KISS_TELNET;
      ifpp->tcpport = Htons(DEFAULT_TELNET_PORT); /* Default TCP-Port setzen. */
      strncpy(ifpp->name, "TELNET", 10);/* Interface Name setzen.             */
    }
#endif /* L1TELNET */
#ifdef L1HTTPD
    if (Interface == HTP_ID)
    {
      ifpp->Interface = KISS_HTTPD;                       /* HTTPD-Interface. */
      ifpp->tcpport   = Htons(DEFAULT_HTTPD_PORT);/* Default TCP-Port setzen. */
      strncpy(ifpp->name, "HTTPD", 10);             /* Interface Name setzen. */
    }
#endif /* L1HTTPD */
#ifdef L1IPCONV
    if (Interface == CVS_ID)
    {
      ifpp->Interface = KISS_IPCONV;
      ifpp->tcpport = Htons(DEFAULT_IPCONV_PORT); /* Default TCP-Port setzen. */
      strncpy(ifpp->name, "IPCONV", 10);/* Interface Name setzen.             */
    }
#endif /* L1IPCONV */
#ifdef L1IRC
    if (Interface == IRC_ID)
    {
      ifpp->Interface = KISS_IRC;
      ifpp->tcpport = Htons(DEFAULT_IRC_PORT);    /* Default TCP-Port setzen. */
      strncpy(ifpp->name, "IRC", 10);   /* Interface Name setzen.             */
    }
#endif /* L1IRC */
  }
}

T_INTERFACE *SearchIf(UWORD Interface) /* Das gesuchte Interface ermitteln    */
{                                      /* und den Interfacezeiger setzen.     */
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(SearchIf)";
#endif /* DEBUG_MODUS */

  switch(Interface)
  {
#ifdef L1TELNET
    case KISS_TELNET:                  /* TELNET Interface.                   */
      ifpp = &ifp[TEL_ID];             /* Interfacezeiger setzen.             */
      break;
#endif /* L1TELNET */

#ifdef L1HTTPD
    case KISS_HTTPD:                   /* HTTPD Interface.                    */
      ifpp = &ifp[HTP_ID];             /* Interfacezeiger setzen.             */
      break;
#endif /* L1HTTPD */

#ifdef L1IPCONV
    case KISS_IPCONV:                  /* TELNET Interface.                   */
      ifpp = &ifp[CVS_ID];             /* Interfacezeiger setzen.             */
      break;
#endif /* L1IPCONV */

#ifdef L1IRC
    case KISS_IRC:                     /* IRC Interface.                      */
      ifpp = &ifp[IRC_ID];             /* Interfacezeiger setzen.             */
      break;
#endif /* L1IPCONV */

    default:                           /* kein Interface gefunden.            */
      return(NULL);
  }

 return(ifpp);                         /* Aktuelles Interface liefern.        */
}

/* Schliesse aktuellen Socket. */
static BOOLEAN CloseSockTCP(BOOLEAN ALL, WORD Interface)
{
  int i,
      j = FALSE;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(CloseSockTCP)";
#endif /* DEBUG_MODUS */

#ifdef OS_STACK
  if (tcppoi->mode == OS_MODE)
    return(CloseSockOS(ALL, Interface));
#endif /* OS_STACK */

  if (ALL == TRUE)                    /* ALLE Socket-Verbindungen schliessen. */
  {
    for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi)/* TBL durchgehen*/
    {
      if (   Interface == tcppoi->Interface             /* Interfacevergleich */
          && tcppoi->sock > TRUE)                       /* Socket ist aktiv.  */
      {
        Close(tcppoi->sock);                           /* Socket schliessen.  */
        DiscTCP();               /* TCPIP-User/link aus der Liste raushaengen */
      }                          /* bzw. Parameter wieder auf 0 setzen.       */

      if (  ((tcppoi->activ)        /* Sind alle aktiven Sockets durchlaufen, */
          && (j++ >= tcp_tbl_top))
          || (j == tcp_tbl_top))
        return(FALSE);                                     /* brechen wir ab. */
    }
  }

  Close(tcppoi->sock);                                 /* Socket schliessen.  */

  DiscTCP();                     /* TCPIP-User/Link aus der Liste raushaengen */
  return(FALSE);                 /* bzw. Parameter wieder auf 0 setzen.       */
}

void L1ExitTCP(WORD Interface)         /* TCPIP-Interface schliessen.         */
{
  T_INTERFACE *ifpoi;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(L1ExitTCP)";
#endif /* DEBUG_MODUS */

#ifdef OS_STACK
  L1ExitOS(Interface);
#endif /* OS_STACK */

  if ((ifpoi = SearchIf(Interface)) == NULL) /* Zeiger auf das akt. Interface.*/
    return;                            /* Aktuelles Interface ist deaktiv,    */
                                       /* dann zum naechsten Interface.       */

  if (ifpoi->actively == TRUE)         /* Nur wenn Interface aktiv.           */
    CloseSockTCP(TRUE, ifpoi->Interface); /* Alle TCPIP-Verbind. schliessen.*/

  if ((signed)ifpoi->ISock > FALSE)    /* TCPIP-Socket schliessen wenn offen  */
    Close(ifpoi->ISock);               /* Schliesse Socket.                   */

  ifpoi->ISock = EOF;

  ifpoi->actively = FALSE;             /* Markiere TCPIP-Interface als deaktiv*/

  T_LOGL1(TRUE, "(L1ExitTCP):\nInterface ist deaktiviert!\n");
}

                                              /* Aktive Socket's setzen.      */
static void PutSocketTCP(Fd_set      *Rmask,  /* Socket-liste.                */
                         int         *maxI)   /* Socket-Wert.                 */
{
  register int i;
  int          j = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(PutSocketTCP)";
#endif /* DEBUG_MODUS */

  /* TCPIP-User/Links durchgehen. */
  for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi)
  {
    if (tcppoi->sock)                  /* Nur wenn Socket aktiv!              */
    {
      if (CheckSocket())                      /* Pruefe, ob Socket aktiv ist. */
      {
        CloseSockTCP(FALSE, tcppoi->Interface);          /* Schliesse Socket. */
        continue;                                    /* Zum naechsten Socket. */
      }

      if (tcppoi->mode)                                    /* Interner Stack. */
      {
        FD_SET_T((unsigned)tcppoi->sock, Rmask);            /* Socket setzen. */
        if (tcppoi->sock > *maxI - 1)
          *maxI = tcppoi->sock + 1;
      }
    }

    if (  ((tcppoi->activ)          /* Sind alle aktiven Sockets durchlaufen, */
        && (j++ >= tcp_tbl_top))
        || (j == tcp_tbl_top))
      break;                                               /* brechen wir ab. */
  }
}

                                      /* Portinfo-String fuer den PORT-Befehl.*/
void HwstrTCP(UWORD   Interface,      /* Aktuelle Interface.                  */
              int     port,           /* L2Port                               */
              MBHEAD *mbp)            /* Buffer                               */
{
  T_INTERFACE *ifpoi;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(HwstrTCP)";
#endif /* DEBUG_MODUS */

  if ((ifpoi = SearchIf(Interface)) == NULL) /* Zeiger auf das akt. Interface.*/
    return;                                         /* Ungueltiges Interface. */

  putstr(" ", mbp);
  putprintf(mbp,"%u", Ntohs(ifpoi->tcpport));
}

                                              /* TCPIP-Port Initialisieren.   */
BOOLEAN L1InitTCP(UWORD Interface,            /* Aktuelle Interface.          */
                  int   port,                 /* L2Port                       */
                  int   TCPPort)              /* TCP-Port.                    */
{
  T_INTERFACE *ifpoi;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(L1InitTCP)";
#endif /* DEBUG_MODUS */

  if ((ifpoi = SearchIf(Interface)) == NULL)   /* Zeiger auf das akt.Interface.*/
    return(FALSE);                             /* Interface ist belegt.        */

  if (ifpoi->actively == TRUE)                 /* Interface ist altiv          */
    return(FALSE);                       /* Kann Filedescriptor nicht anlegen. */

  if (  (TCPPort < 1)
      ||(TCPPort > 65535))
      return(FALSE);
                                       /* Einen neuen Filedescriptor anlegen. */
  if ((ifpoi->ISock = SetupTCP(ifpoi->name, Htons((unsigned short)TCPPort))) == EOF)
    return(FALSE);                      /* Kann Filedescriptor nicht anlegen. */

#ifdef OS_STACK
  if (L1InitOS(Interface, port, (unsigned short)TCPPort) == FALSE)
    return(FALSE);
#endif /* OS_STACK */

  ifpoi->l2port   = port;                /* L2-Port setzen.                    */
  ifpoi->actively = TRUE;                /* Markiere Interface als aktiv.      */
  ifpoi->tcpport  = TCPPort;             /* Markiere TCP-Port.                 */

  return(TRUE);                         /* Socket wurde erfolgreich erstellt. */
}

                                        /* Einen Filedescriptor anlegen.      */
int SetupTCP(char          *name,       /* Interface-Name.                    */
             unsigned short tcp_port)   /* Neuer TCP-Port.                    */
{
  int tmp_fd = EOF;                     /* Temporaerer Filedescriptor         */
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(SetupTCP)";
#endif /* DEBUG_MODUS */

  if (tcp_port == FALSE)                /* Kein Port angegeben.               */
  {
    T_LOGL1(TRUE, "(SetupTCP):\nKein TCP-Port angegeben.\n");
    return(EOF);                        /* Abbruch.                           */
  }

  memset(&Myaddr_in, 0, sizeof(Myaddr_in)); /* Adressstruktur loeschen        */
  memset(&Peeraddr_in, 0, sizeof(Peeraddr_in));
  Myaddr_in.sin_family = AF_INET;             /* Adressstruktur fuellen       */
  Myaddr_in.sin_port = Htons(tcp_port);       /* TCP-Port setzen.             */
  Myaddr_in.sin_addr._S_addr = INADDR_ANY;  /* Jedes Netzwerk Geraet abfragen. */

                                           /* Socket erstellen.               */
  if ((tmp_fd = Socket(AF_INET, SOCK_STREAM, 0)) == EOF)
  {
    printf("Error %s: Socket cannot be constructed!\r", ifpp->name);

    T_LOGL1(TRUE, "(SetupTCP):\nNeuer Socket kann nicht geoeffnet werden.\n");
    return(EOF);                           /* Keine freien Socket's.          */
  }

                                           /* Bind Initialisieren.            */
  if (Bind (tmp_fd, (struct Sockaddr *) &Myaddr_in, sizeof (struct Sockaddr_in)) == EOF)
  {
    printf("Error %s: Bind cannot be initialized!\n", ifpp->name);
    Close(tmp_fd);                         /* Schliesse Socket.               */

    T_LOGL1(TRUE, "(SetupTCP):\nBind Initialisierung fehlgeschlagen.\n");
    return(EOF);                           /* Fehler beim Bind     .          */
  }

  if (Listen (tmp_fd, 3) == EOF)           /* Listen Initialisieren.          */
  {
    printf("Error %s: listen cannot be initialized!\n", ifpp->name);
    Close(tmp_fd);                         /* Schliesse Socket.               */

    T_LOGL1(TRUE, "(SetupTCP):\nListen Initialisierung fehlgeschlagen.\n");
    return(EOF);                           /* Fehler beim Listen.             */
  }

  T_LOGL1(TRUE, "(SetupTCP):\nTCP-Port (%d) erfolgreich gesetzt.\n"
                , Ntohs(tcp_port));

  return(tmp_fd);                          /* Aktuelle Filedescriptor.        */
}

void L1ctlTCP(int req, int port)           /* Level 1 Kontrolle               */
{
  testflag[port] = 0;                      /* Kein TEST-Frame zulassen.       */
  kick[port]     = 0;
}

BOOLEAN TcpDCD(int l2port)           /* DCD-Status bei TCPIP ist immer FALSE. */
{
  return(FALSE);
}

/* Defaultwerte setzen. */
void SetDefaultWorthTCP(unsigned  sock,               /* Neuer Socket.        */
                        char     *ip,                 /* IP-Adresse.          */
                        int       l2port,             /* L2-Port.             */
                        UWORD     Interface,          /* TCP-Interface.       */
                        BOOLEAN   Mode)               /* Interner TCP-Stack   */
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(SetDefaultWorthTCP)";
#endif /* DEBUG_MODUS */

  strncpy(tcppoi->ip, ip, IPADDR);       /* IP-Adresse eintragen.             */
  tcppoi->port        = l2port;          /* L2-Port setzen.                   */
  tcppoi->activ       = TRUE;            /* Markiere aktiv.                   */
  tcppoi->sock        = sock;            /* Aktuellen Socket setzen.          */
  tcppoi->mode        = Mode;            /* Mode setzen. (Int/Ext.)           */
  tcppoi->cmdlen      = 0;               /* Zaehler fuer cmd-Buffer.          */
  tcppoi->inlin       = 0;               /* eingelaufene Zeilen auf 0 setzen. */
  tcppoi->outlin      = 0;               /* auszugebende Zeilen auf 0 setzen. */
  tcppoi->RecvLen     = 0;               /* Rueckgabewert fuer recv.          */
  tcppoi->rxc         = 0;               /* Zaehler fuer RX-BUFFER.           */
  tcppoi->LoginPrompt = FALSE;           /* Markiere Prompt senden.           */
  tcppoi->T3          = T3PARA;          /* T3-Timer setzen.                  */
  tcppoi->noacti      = ininat;          /* Timeout setzen                    */
  tcppoi->Interface   = Interface;       /* Interface ID setzen.              */
  tcppoi->state       = L2MNIX;          /* State setzen.                     */
  tcppoi->cr          = 0;               /* Kein RETRUN empfangen.            */
  tcppoi->txstatus    = TCP_TX_FREE;     /* Sender ist frei.                  */
  tcppoi->Upcall[0]   = 0;
  tcppoi->Downcall[0] = 0;
#ifdef L1HTTPD
  tcppoi->status      = TCP_NULL;
  tcppoi->http        = TCP_NULL;
  tcppoi->fp          = NULL;
#endif /* L1HTTPD */
#ifdef L1IPCONV
  tcppoi->CVSlink     = FALSE;
  tcppoi->Intern      = FALSE;
#ifdef L1IRC
  if (Interface == KISS_IRC)
    tcppoi->IrcMode   = TRUE;
  else
    tcppoi->IrcMode   = FALSE;
#endif /* L1IRC */
#endif /* L1IPCONV */

  tcp_tbl_top++;                         /* TBL-Zaehler um 1 erhoehen.        */

  relink(ulink((LEHEAD *)tcppoi),        /* Link aus der frei-Liste nehmen    */
               (LEHEAD *)tcpactl.tail);
}

int ReadSockTCP(void)                    /* Empfangene TCPIP Packete.         */
{
  Fd_set          fdsI;
  int             ret = EOF;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(ReadSockTCP)";
#endif /* DEBUG_MODUS */

  if (tcppoi->rxc >= tcppoi->RecvLen)       /* Sind alle Zeichen verarbeitet. */
  {
    if (tcppoi->mode == INT_STACK)             /* Interner Socket.            */
    {
      FD_ZERO_T(&fdsI);                        /* Inhalt loeschen.            */
      FD_SET_T((unsigned)tcppoi->sock, &fdsI); /* Socket setzen.              */

                                               /* Pruefe auf aktivitaet.      */
      ret = Select(tcppoi->sock + 1, &fdsI, NULL , NULL ,NULL);

      if (ret)
        /* Zeichen vom Socket empfangen. */
        tcppoi->RecvLen = Recv (tcppoi->sock, tcppoi->rxbuf, RXLEN,0);
      else
        return((int)NULL);               /* Keine aktivitaet auf den Socket.  */
    }

    if (tcppoi->RecvLen < 1)             /* Fehler beim Empfang.              */
    {
      if (tcppoi->RecvLen == EOF)        /* Socket wurde mittlerweile geloes. */
      {
        ifpp = SetInterface(tcppoi->Interface);     /* akt. Interface setzen. */
                                                     /* ggf. Logbuch fuehren. */
        T_LOGL1(TRUE, "(ReadSockTCP):%s\n (Recv) Fehler beim empfang, Segment wird auf DISC gesetzt!\n"
                  , tcppoi->ip);

        tcppoi->disflg |= 0x80;          /* Segment auf DISC stellen.         */
        return((int)NULL);
      }

      if (  (ret)
          &&(tcppoi->RecvLen == 0))
      {
        ifpp = SetInterface(tcppoi->Interface);     /* akt. Interface setzen. */
                                                     /* ggf. Logbuch fuehren. */
        T_LOGL1(TRUE, "(ReadSockTCP):%s\n(Recv) Keine Zeichen, Segment wird auf DISC gesetzt!\n"
                  , tcppoi->ip);

        tcppoi->disflg |= 0x80;          /* Segment auf DISC stellen.         */
        return((int)NULL);
      }
    }

    tcppoi->rxc                    = 0;  /* RX-Zaehler auf 0 setzen.          */
    tcppoi->rxbuf[tcppoi->RecvLen] = 0;  /* Nullzeichen setzen.               */
                                         /* ggf. Logbuch fuehren.             */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL3(TRUE, "(ReadSockTCP):%s\nInput:\r%s\r"
                  , tcppoi->ip
                  , tcppoi->rxbuf);
  }

  ret = tcppoi->rxbuf[tcppoi->rxc++];    /* Zeichen vom rxbuf holen.          */

  return(ret);                           /* Aktuelle Zeichen weiterreichen.   */
}

static int ReadIndicationsSockTCP(void)       /* Zeichen vom Socket einlesen. */
{
  TRILLIAN ok = ERRORS;
  char     s;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(ReadIndicationsSockTCP)";
#endif /* DEBUG_MODUS */

  while(1)                               /* Alle Zeichen von Socket einlesen. */
  {
    s = ReadSockTCP();                   /* Zeichen vom Socket.               */

    if (tcppoi->cmdlen >= RXLEN)         /* Maximale Buffergroesse erreicht,  */
    {
      ifpp = SetInterface(tcppoi->Interface);
      T_LOGL1(TRUE, "(ReadIndicationsSockTCP):%s\nMaximale Buffergroesse erreicht!\n"
                    , tcppoi->ip);
       return(TRUE);
    }

    switch(tcppoi->Interface)            /* Interface.                        */
    {
#ifdef L1TELNET
      case KISS_TELNET :                        /* Telnet.                    */
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
#endif /* L1IPCONV */

      default :                                /* Ungueltiges Interface.      */
        break;                                 /* Hier werden wir nie kommen. */
    }

    return(FALSE);                             /* Frame noch nicht komplett.  */
  }

  return(FALSE);                               /* Frame noch nicht komplett.  */
}

                                             /* Zusaetzlichen CTEXT einlesen. */
void LoginTextTCP(char   *filename,          /* CTEXT-Datei.                  */
                  MBHEAD *tmpmbp)            /* Buffer.                       */
{
  FILE *fp;
  char  buffer[80 + 1];
  char  convfile[MAXPATH];
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(LoginTextTCP)";
#endif /* DEBUG_MODUS */

  strcpy(convfile, confpath);                    /* Basis-Verzeichnis setzen. */
  strcat(convfile, filename);                    /* CTEXT-Datei setzen.       */

  ifpp = SetInterface(tcppoi->Interface);              /* aktuelles Interface */
                                                      /* setzen fuer Logging. */
  if ((fp = fopen(convfile, "r")) == NULL)            /* CTEXT-Datei oeffnen. */
  {                                                  /* ggf. Logbuch fuehren. */
    T_LOGL1(TRUE, "(LoginTextTCP):%s\nDatei (%s) kann nicht geoeffnet werden!\n"
                , tcppoi->ip
                , convfile);
    return;                                      /* Laest sich nicht oeffnen. */
  }
                                                     /* ggf. Logbuch fuehren. */
  T_LOGL2(TRUE, "(LoginTextTCP):%s\nDatei (%s) ist geoeffnet!\n"
              , tcppoi->ip
              , convfile);

  while (fgets(buffer, 80, fp) != NULL)    /* String mit 80 Zeichen einlesen. */
    putprintf(tmpmbp, "%s\r", buffer);

  fclose(fp);                                            /* Datei schliessen. */
                                                     /* ggf. Logbuch fuehren. */
  T_LOGL2(TRUE, "(LoginTextTCP):%s\nDatei (%s) ist geschlossen!\n"
              , tcppoi->ip
              , convfile);
}

static void PromptTCP(UWORD Interface)          /* Login-Prompt senden.       */
{
  SENDTX *STx = NULL;
  char    call[10];
  char    file[128];
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(PromptTCP)";
#endif /* DEBUG_MODUS */

  switch(Interface)                             /* Interface.                 */
  {
#ifdef L1TELNET
    /* TELNET-Server */
    case KISS_TELNET :                          /* Telnet.                    */
      strcpy(file, TELNET_TXT);                 /* Markiere Datei (telnet.txt)*/
      break;
#endif /* L1TELNET */

#ifdef L1IPCONV
    /* IPCONVERS-Server */
    case KISS_IPCONV :                          /* IPConvers                  */
      strcpy(file, IPCONV_TXT);                 /* Markiere Datei (ipconv.txt)*/
      break;
#endif /* L1IPCONV */

    default :
      tcppoi->LoginPrompt = TRUE;               /* Markiere Prompt als gesend.*/
      return;
  }

  if ((STx = (SENDTX *)allocb(ALLOC_L1TCPIP)) == NULL)/* TX-Segment besorgen. */
  {
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(PromptTCP):%s\nSpeicher (%d) ist voll!\n"
                  , tcppoi->ip
                  , nmbfre);

    tcppoi->disflg |= 0x80;                 /* Segment auf Disconnect setzen. */
    return;                                    /* Kein Segment frei, abbruch. */
  }

  if ((STx->Data = SetBuffer()) == NULL)             /* Buffer besorgen.      */
  {
    dealoc((MBHEAD *)ulink((LEHEAD *)STx));           /* TX-Segment entsorgen.*/

    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(PromptTCP):%s\nSpeicher (%d) ist voll!\n"
                  , tcppoi->ip
                  , nmbfre);

    tcppoi->disflg |= 0x80;                 /* Segment auf Disconnect setzen. */
    return;                                      /* Buffer ist voll, abbruch. */
  }

  call2str(call,myid);                             /* Konvertiere das MYCALL. */
  putprintf(STx->Data, "%s - %s\n", call, version); /* Standardtext einfuegen.*/
  LoginTextTCP(file, STx->Data);         /* evl. erweiterten CTEXT einfuegen. */

  putprintf(STx->Data, "\nLogin:");                    /* Und zum schluss den */
                                                   /* Login Prompt einfuegen. */

  rwndmb(STx->Data);                                  /* Buffer zurueckspulen.*/
  STx->Sock      = tcppoi->sock;                      /* Socket setzen.       */
  STx->Interface = tcppoi->Interface;                 /* Interface setzen.    */
  STx->Mode      = tcppoi->mode;                      /* Stack-Mode setzen.   */

  relink((LEHEAD *) STx, (LEHEAD *)rxflTX.tail);/* Umhaengen in die Sendeliste*/
  tcppoi->LoginPrompt = TRUE;         /* Markiere, Login-Prompt als gesendet. */
}

void ServTCP(void)                    /* Empfangene Zeichen in Frames packen. */
{
  READRX      *SRx;
  register int i;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(ServTCP)";
#endif /* DEBUG_MODUS */

  if (ReadIndicationsSockTCP())            /* Empfangene Zeichen analysieren. */
  {
    if(tcppoi->cmdlen > 0)                       /* Es sind Zeichen im Buffer.*/
    {
      if ((SRx = (READRX *)allocb(ALLOC_L1TCPIP)) == NULL)/* RX-Seg. besorgen */
      {
        ifpp = SetInterface(tcppoi->Interface);
        T_LOGL1(TRUE, "(ServTCP):%s\nSpeicher (%d) ist voll!\n"
                  , tcppoi->ip
                  , nmbfre);
        return;                                /* Kein Segment frei, abbruch. */
      }

      if ((SRx->Data = SetBuffer()) == NULL)              /* Buffer besorgen. */
      {
        ifpp = SetInterface(tcppoi->Interface);
        T_LOGL1(TRUE, "(ServTCP):%s\nSpeicher (%d) ist voll!\n"
                  , tcppoi->ip
                  , nmbfre);

        dealoc((MBHEAD *)ulink((LEHEAD *)SRx));      /* RX-Segment entsorgen. */
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
          ifpp = SetInterface(tcppoi->Interface);
          T_LOGL1(TRUE, "(ServTCP):%s\nTCP-Segment nicht mehr aktiv!\n"
                      , tcppoi->ip);

          if (SRx->Data != NULL)
            dealmb(SRx->Data);                        /* Buffer entsorgen.    */

          dealoc((MBHEAD *)ulink((LEHEAD *)SRx));     /* RX-Segment entsorgen.*/
        }                                     /* Socket ist nicht mehr aktiv. */
    }                                              /* kein Zeichen im buffer. */
  }                                         /* Frame ist noch nicht komplett. */
}

/* Neue User hinzufuegen, vorrausgesetzt es sind noch freie Sockets vorhanden.*/
int AddUserTCP(T_INTERFACE *ifpoi,                    /* TCP-Interface.       */
               unsigned     NewSocket,                /* Neuer Socket.        */
               char        *ip)                       /* IP-Adresse.          */
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(AddUserTCP)";
#endif /* DEBUG_MODUS */

  if ((tcppoi = (TCPIP *)tcpfrel.head) == NULL)   /* freien Eintrag besorgen. */
  {
    ifpp = SetInterface(ifpoi->Interface);
    T_LOGL1(TRUE, "(AddUserTCP):%s\nKann kein neues TCPIP-Segment anlegen !\n"
                , ip
                , tcp_tbl_top);

    return(TRUE);                      /* es gibt keinen freien Eintrag mehr! */
  }


  SetDefaultWorthTCP(NewSocket,         /* Defaultwerte setzen, neuer Socket. */
                       ip,                             /* IP-Adresse.         */
                       ifpoi->l2port,                  /* L2-Port.            */
                       ifpoi->Interface,               /* TCPIP-Interface.    */
                       INT_STACK);                     /* Interner TCP-STack. */

  ServTCP();                          /* Empfangene Zeichen in Frames packen. */

  return(FALSE);
}


/* Alle Parameter auf default zuruecksetzen und den User aus der Liste nehmen.*/
void DiscTCP(void)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(DiscTCP)";
#endif /* DEBUG_MODUS */

  if (tcppoi->activ == FALSE)                 /* User ist deaktiv.            */
  {
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(DiscTCP):%s\nTCP-Segment wurde schon zurueckgesetzt!\n"
                , tcppoi->ip);
    return;                                   /* Abbruch.                     */
  }
  else                                        /* User ist aktiv.              */
    tcppoi->activ = FALSE;                    /* Markiere, User als deaktiv.  */

  if (tcppoi->login)                          /* User war eingeloggt.         */
    nmbtcp--;                                 /* Ein TCPIP User weniger.      */

  dealml((LEHEAD *)&tcppoi->inbuf);           /* Buffer leeren.               */
  dealml((LEHEAD *)&tcppoi->outbuf);


#ifdef L1HTTPD
  if (tcppoi->fp != NULL)
  {
    fclose(tcppoi->fp);

    /* ggf. Logbuch fuehren. */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL2(TRUE, "(DiscTCP):%s\nDatei wird geschlossen.\n"
                , tcppoi->ip);
  }
#endif /* L1HTTPD */

  /* ggf. Logbuch fuehren. */
  ifpp = SetInterface(tcppoi->Interface);
  T_LOGL2(TRUE, "(DiscTCP):\nVerbindung zur IP-Adresse %s wurde geschlossen.\n"
                , tcppoi->ip);

  tcppoi->port        = 0;              /* Parameter auf defaultwerte setzen. */
  tcppoi->sock        = 0;
  tcppoi->mode        = 0;
  tcppoi->cmdlen      = 0;
  tcppoi->rxc         = 0;
  tcppoi->RecvLen     = 0;
  tcppoi->login       = 0;
  tcppoi->noacti      = 0;
  tcppoi->T3          = 0;
  tcppoi->inlin       = 0;
  tcppoi->outlin      = 0;
  tcppoi->disflg      = 0;
  tcppoi->LoginPrompt = 0;
  tcppoi->state       = L2MNIX;
  tcppoi->cr          = 0;
  tcppoi->txstatus    = TCP_TX_FREE;
  tcppoi->Upcall[0]   = 0;
  tcppoi->Downcall[0] = 0;
  tcppoi->cmd[0]      = 0;
  tcppoi->rxbuf[0]    = 0;
  tcppoi->txbuf[0]    = 0;
  tcppoi->ip[0]       = 0;
#ifdef l1HTTPD
  tcppoi->status      = 0;
  tcppoi->http        = 0;
  tcppoi->fp          = NULL;
#endif /* L1HTTPD */
#ifdef L1IPCONV
  tcppoi->CVSlink     = FALSE;
  tcppoi->Intern      = FALSE;
#ifdef L1IRC
  tcppoi->IrcMode     = FALSE;
#endif /* L1IRC */
#endif /* L1IPCONV */

  tcp_tbl_top--;                         /* TBL-Zaehler um 1 runter.          */

  l2tol7(2, tcppoi, TCP_USER);           /* Statusmeldung an L7 Weiterleiten. */
  resptc(g_uid(tcppoi, TCP_USER));       /* patchcord-Liste zuruecksetzen     */
  relink(ulink((LEHEAD *)tcppoi),        /* aus der aktiv-Liste nehmen        */
               (LEHEAD *)tcpfrel.tail);  /* in die Freiliste haengen          */
}

                                 /* Rufzeichen in der MH-Liste Aktualisieren. */
void MhUpdateTCP(MBHEAD *tbp,                       /* Buffer                 */
                 BOOLEAN rxtx)                      /* Flag fuer RX/TX-Bytes. */
{
  MHEARD *mhp;
  char    Upcall[10 + 1];
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(MhUpdateTCP)";
#endif /* DEBUG_MODUS */

  if (tcppoi->Upcall[0] == FALSE)
    return;

  if (updmheard((int)tcppoi->port))       /* Pruefen ob die MH-Liste          */
  {                                       /* auf den L2-Port aktiv ist.       */
    if (valcal(tcppoi->Upcall) == ERRORS) /* Rufzeichen auf Gueltigkeit pruef.*/
    {
      ifpp = SetInterface(tcppoi->Interface);
      call2str(Upcall, tcppoi->Upcall);
      T_LOGL1(TRUE, "(MhUpdateTCP):%s\nRufzeichen (%s) ist ungueltig!\n"
                , tcppoi->ip
                , Upcall);
      return;                             /* Ungueltiges Rufzeichen.          */
    }

                                          /* Rufzeichen in der MHListe aktual.*/
    if ((mhp = mh_lookup_port(&l2heard, tcppoi->Upcall, tcppoi->port, FALSE)) == NULL)
      mhp = mh_add(&l2heard);             /* Neuen MH-Eintrag taetigen.       */

    if (mhp)                              /* Eintrag gefunden.        .       */
                                          /* User/Link Aktualisieren.         */
        mh_update(&l2heard, mhp, tcppoi->Upcall, tcppoi->port);

      if (tbp == NULL)                    /* Kein Eintrag fuer RX/TX-Bytes.   */
        return;                           /* Kein Aktualisierung moeglich.     */

      if (rxtx)                           /* RX/TX-Bytes Aktualisieren.       */
        mhp->rx_bytes += (tbp->mbpc);     /* RX-Bytes Aktualiseren.           */
      else
        mhp->tx_bytes += (tbp->mbpc);     /* TX-Bytes Aktualisieren.          */
  }
}

void SendTCP(void)                      /* Frame vorbereiten fuer Sendeliste. */
{
  MBHEAD *Data   = NULL;
  SENDTX *NewSTx = NULL;
  char    c;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(SendTCP)";
#endif /* DEBUG_MODUS */

                                         /* Zeiger auf den Inhalt des Frames. */
  Data = (MBHEAD *)ulink((LEHEAD *)tcppoi->outbuf.head);
    --tcppoi->outlin;                                 /* Ein Frame weniger.   */

  if ((NewSTx = (SENDTX *)allocb(ALLOC_L1TCPIP)) == NULL) /* Neues Seg.anlegen*/
  {
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(SendTCP):%s\nSpeicher (%d) ist voll!\n"
                , tcppoi->ip
                , nmbfre);

    tcppoi->disflg |= 0x80;                 /* Segment auf Disconnect setzen. */
    return;                                           /* Kein TX-Segment frei.*/
  }

  if ((NewSTx->Data = SetBuffer()) == NULL)           /* Buffer besorgen.     */
  {
    dealoc((MBHEAD *)ulink((LEHEAD *)NewSTx));        /* TX-Segment entsorgen.*/

    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(SendTCP):%s\nSpeicher (%d) ist voll!\n"
                , tcppoi->ip
                , nmbfre);

    tcppoi->disflg |= 0x80;                 /* Segment auf Disconnect setzen. */
    return;                                           /* Buffer voll, abbruch.*/
  }

  NewSTx->Sock      = tcppoi->sock;                   /* Socket setzen.       */
  NewSTx->Interface = tcppoi->Interface;              /* Interface setzen.    */
  NewSTx->Mode      = tcppoi->mode;                   /* Stack-Mode setzen.   */

  while (Data->mbgc != Data->mbpc)       /* Alle Zeichen im Buffer lesen und  */
  {                                      /* vorbereiten zum Senden.           */
    c = getchr(Data);                    /* Zeichen aus den Buffer lesen.     */

    switch (tcppoi->Interface)                       /* Interface.            */
    {
#ifdef L1TELNET
      case KISS_TELNET :                             /* TELNET-Interface.     */
        if (  (c == CR)                              /* CR-Return             */
            ||(c == LF))                             /* oder LF-Return.       */
          putchr(LF, NewSTx->Data);                  /* Setze nur LF-Return.  */
       break;
#endif /* L1TELNET */
#ifdef L1HTTPD
      case KISS_HTTPD :                              /* HTTPD-Interface.      */
        if (tcppoi->http == TCP_CMD)                     /* Befehl via HTTPD. */
        {
          /* Schreibe Zeichen in Buffer. */
          putv(NewSTx->Data, c);
          /* zum naechsten Zeichen. */
          continue;
        }
       break;
#endif /* L1HTTPD */

#ifdef L1IPCONV
      case KISS_IPCONV :                             /* IPCONV-Interface.     */
        if (  (c == CR)                              /* CR-Return             */
            ||(c == LF))                             /* oder LF-Return.       */
          putchr(LF, NewSTx->Data);                  /* Setze nur LF-Return.  */
       break;
#endif /* L1IPCONV */

#ifdef L1IRC
      case KISS_IRC :                                /* IRC-Interface.        */
        if (  (c == CR)                              /* CR-Return             */
            ||(c == LF))                             /* oder LF-Return.       */
          putchr(LF, NewSTx->Data);                  /* Setze nur LF-Return.  */
       break;
#endif /* L1IRC */

      default :                                     /* Unbekanntes Interface. */
       break;
    }

    putchr(c, NewSTx->Data);                     /* Zeichen in Buffer setzen. */
  }

  rwndmb(NewSTx->Data);                             /* Buffer zurueckspulen.  */
  relink((LEHEAD *) NewSTx, (LEHEAD *)rxflTX.tail); /* In Sendeliste umhaengen*/

  dealmb(Data);                             /* Alten Buffer leeren/entsorgen. */

  tcppoi->cmdlen = 0;                                 /* Komandozeile leeren. */
  tcppoi->cmd[0] = 0;                                 /* Nullzeichen setzen.  */
}

void SenTCP(void)            /* Informationstransfer von TCPIP nach Layer X */
{
  MBHEAD         *mbp;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(SenTCP)";
#endif /* DEBUG_MODUS */

  while (tcppoi->inlin != 0)                           /* Frame fuer Layer X? */
  {
    mbp = (MBHEAD *)tcppoi->inbuf.head;                        /* Hole Frame. */
    mbp->l2link = (LNKBLK *)tcppoi;
    mbp->type = TCP_USER;
    mbp->l2fflg = L2CPID;

    if (!fmlink(FALSE, mbp))             /* Frame fuer Layer X weiterreichen. */
      break;                                  /* Link ist verstopft, abbruch. */

    --tcppoi->inlin;                                    /* Ein Frame weniger. */
  }
}

/* Frame aus der Sendeliste senden. */
static BOOLEAN PutflushSockTCP(void)
{
  int ok  = FALSE;
  int ret = FALSE;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(PutflushSockTCP)";
#endif /* DEBUG_MODUS */

#ifdef OS_STACK
  if (tcppoi->mode == OS_MODE)
    return(PutflushSockOS());
#endif /* OS_STACK */

  if (tcppoi->txc)                                  /* Gibt es was zu senden. */
  {
    while (1)                                       /* Gibt es was zu senden. */
    {
      Fd_set wfdsI;

      if (tcppoi->mode)                                /* Interner TCP-Stack. */
      {
        FD_ZERO_T(&wfdsI);                                 /* Loesche inhalt. */
        FD_SET_T((unsigned)tcppoi->sock, &wfdsI);           /* Socket setzen. */

        if (Select(tcppoi->sock + 1, NULL, &wfdsI, NULL ,NULL))/* Sender frei.*/
        {
          if (tcppoi->txstatus == TCP_TX_BUSY)        /* Sender ist auf Busy. */
            tcppoi->txstatus = TCP_TX_FREE;       /* Sender auf frei stellen. */

                                                              /* Frame senden */
          if ((ok = Send(tcppoi->sock, tcppoi->txbuf + tcppoi->sum, tcppoi->txc - tcppoi->sum,0)) < 0)
          {
            ifpp = SetInterface(tcppoi->Interface);
            T_LOGL1(TRUE, "(PutflushSockTCP):%s\nFrame senden fehlgeschlagen!\n"
                          , tcppoi->ip);

            tcppoi->disflg |= 0x80;         /* Segment auf Disconnect setzen. */
            return(TRUE);                              /* Abbruch.            */
          }

                                                     /* ggf. Logbuch fuehren. */
          ifpp = SetInterface(tcppoi->Interface);
          T_LOGL3(TRUE, "(PutflushSockTCP):%s\nOutput:\r%s\r"
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
    }

    tcppoi->sum = 0;                                        /* Zuruecksetzen. */
    tcppoi->txc = 0;
    tcppoi->txbuf[0] = 0;
  }

  return(FALSE);
}

/* Pruefen ob noch Frame in der Warteschlage haengen. */
static int NotContensTCP(void)
{
  BOOLEAN result = TRUE;                          /* Keine weiteren Aufgaben. */
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(NotContensTCP)";
#endif /* DEBUG_MODUS */

  switch(tcppoi->Interface)
  {
#ifdef L1TELNET
    case KISS_TELNET :
      if (!tcppoi->LoginPrompt)                      /* Noch kein Prompt gesendet */
        PromptTCP(tcppoi->Interface);                       /* Login-Text senden. */

      break;
#endif /* L1TELNET */

#ifdef L1IPCONV
    case KISS_IPCONV :
      if (!tcppoi->LoginPrompt)                      /* Noch kein Prompt gesendet */
        PromptTCP(tcppoi->Interface);                       /* Login-Text senden. */

      break;
#endif /* L1IPCONV */

    default :
      break;
  }

#ifdef L1HTTPD
  /* HTTPD, Uri laden? Nur wenn Sender frei. */
  if (  (tcppoi->Interface == KISS_HTTPD)
      &&(tcppoi->status == TCP_URI)
      &&(tcppoi->txstatus != TCP_BUSY))
  {
    /* uri laden. */
    if (load_uri())
    {
      tcppoi->T3     = T3PARA;
      tcppoi->noacti = ininat;
      /* Aufgabe bearbeitet. */
      result = FALSE;
    }
  }
#endif /* L1HTTPD */

  while (tcppoi->outlin != 0)     /* Es sind noch Frames in der Warteschlange */
  {
    SendTCP();                                /* Frame vorbereiten zum senden */
    result = FALSE;                                    /* Aufgabe bearbeitet. */
#ifdef L1HTTPD
    if (tcppoi->Interface == KISS_HTTPD)
    {
      if (tcppoi->outlin == 0)
      {
        tcppoi->disflg |= 0x80;
        PutHtmlEnd();
      }
    }
  #endif /* L1HTTPD */
  }

  if (tcppoi->inlin != 0)                              /* Frame fuer Layer X. */
  {
    SenTCP();                            /* Frame fuer Layer X weiterreichen. */
    result = FALSE;                                    /* Aufgabe bearbeitet. */
  }

  if (tcppoi->txstatus == TCP_TX_BUSY)                    /* Sender ist Busy. */
  {
    PutflushSockTCP();                              /* Frame erneuert senden. */
    result = FALSE;                                  /* Aufgabe bearbeitet. */
  }

  if (  (tcppoi->disflg)                            /* Steht Segment auf Disc */
      &&(result))                   /* und liegen keine weiteren Aufgaben an, */
    return(TRUE);                                       /* Socket Schliessen. */

  return(FALSE);
}

/* Pruefe Logindaten. */
static WORD CheckData(MBHEAD *tbp,                  /* Buffer mit Logindaten. */
                      char   *scall,               /* Zeiger fuer Rufzeichen. */
                      char   *contens)   /* Zeiger fuer weiterte Login-Daten. */
{
  char         ch;
  UWORD        len = 0;
  register int j   = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(CheckData)";
#endif /* DEBUG_MODUS */

  rwndmb(tbp);                                  /* Bufferdaten zurueckspulen. */

  if ((tbp->mbpc - tbp->mbgc) > 256)        /* Maximale Logindaten 256 Zeichen. */
  {
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(CheckData):%s\nMaximale Logindaten (%d) ueberschritten!\n"
                  , tcppoi->ip
                  , tbp->mbgc - tbp->mbpc);
    return(FALSE);                                     /* Spielmatz, abbruch. */
  }

  while (tbp->mbgc != tbp->mbpc)        /* Alle Zeichen aus den Buffer lesen. */
  {
    ch = getchr(tbp);                                       /* Zeichen holen. */

    if (  (ch == CR)                                /* Zeichen ein CR-Return, */
        ||(ch == LF)                                       /* oder LF-Return, */
        ||(ch == '#'))                                         /* oder Route. */
       break;                                              /* Brechen wir ab. */

    scall[len++] = ch;                                  /* Zeichen eintragen. */
  }

  scall[len] = '\0';                                   /* Nullzeichen setzen. */

  while (tbp->mbgc != tbp->mbpc)   /* Restliche Zeichen aus den Buffer lesen. */
    contens[j++] = getchr(tbp);                             /* Zeichen holen. */

  contens[j] = '\0';                                   /* Nullzeichen setzen. */

#ifdef L1IRC
  if (tcppoi->IrcMode)
  {
    IrcLinkUser(scall);
    IrcLinkNick(contens);
    len = L2CALEN;
  }
#endif /* L1IRC */

  return(len);                                           /* Rufzeichenlaenge. */
}

/***********************************************/
/* Rufzeichen suchen.                          */
/* ERRORS - Es wurde kein Rufzeichen gefunden. */
/* NO     - Rufzeichen ohne SSID-Vergleich     */
/*          gefunden.                          */
/* YES    - Rufzeichen mit SSID-Vergleich      */
/*          gefunden.                          */
/***********************************************/
static TRILLIAN SearchCall(char  *call, BOOLEAN ssid)
{
  TCPIP   *tpoi;
  int      i     = FALSE;
  int      j     = FALSE;
  TRILLIAN found = ERRORS;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(SearchCall)";
#endif /* DEBUG_MODUS */

  for (i = 0, tpoi = tcptbl; i < MAXTCPIP; ++i, ++tpoi)    /* TBL durchgehen. */
  {
    if (tpoi->activ == TRUE)                               /* Nur wenn aktiv! */
    {
      if (  (cmpcal(tpoi->Upcall, call)                     /* Callvergleich. */
          &&(tpoi->port == tcppoi->port)))                  /* Portvergleich. */
      {
        if (ssid == TRUE)                            /* SSID-Bereich pruefen. */
        {
          if (cmpid(tpoi->Upcall, call))           /* Callvergleich mit SSID. */
          {
            found = YES;                                         /* Markiere, */
            break;                                       /* Eintrag gefunden. */
          }

          found = NO;                       /* Rufzeichen ohne SSID gefunden. */
        }
        else
          found = NO;                       /* Rufzeichen ohne SSID gefunden. */
      }

      if (  ((tcppoi->activ)        /* Sind alle aktiven Sockets durchlaufen, */
          && (j++ >= tcp_tbl_top))
          || (j == tcp_tbl_top))
        break;                                             /* brechen wir ab. */
    }
  }

  return(found);
}

/* Pruefen der SSID. */
static int CheckSSID(char *scall)
{
  char ssid = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(CheckSSID)";
#endif /* DEBUG_MODUS */

  while (SearchCall(scall, TRUE) == TRUE)/* Suche Rufzeichen mit SSID-Bereich */
  {
    ssid++;      /* Es gibt SCHON ein Rufzeichen mit der SSID, um 1 erhoehen. */

    if (ssid > 15)                               /* SSID ist groesser als 15. */
      return(EOF);                                                /* Abbruch! */

    scall[L2IDLEN-1] = (ssid << 1) | 0x60;                    /* SSID setzen. */
  }

  return((int)ssid);                         /* SSID ist noch nicht vergeben. */
}

/***********************************************/
/* Ist das Rufzeichen schon mindestens einmal  */
/* eingeloggt, vergeben wir anhand des letzten */
/* Call ,mit der hoehsten SSID, die SSID.      */
/***********************************************/
char *CheckCallSSID(char *scall)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(CheckCallSSID)";
#endif /* DEBUG_MODUS */

  if (SearchCall(scall,FALSE) == NO)    /* Suche Rufzeichen ohne Beachtung    */
  {                                                              /* der SSID. */
    if (CheckSSID(scall) != EOF)        /* Pruefe Rufzeichen auf SSID,        */
                                        /* gegebenfalls neue SSID vergeben.   */
      return(scall);                    /* Rufzeichen mit aktueller SSID.     */
    else
      {
        ifpp = SetInterface(tcppoi->Interface);
        T_LOGL1(TRUE, "(CheckCallSSID):%s\nAlle SSID-Nummern vergeben!\n"
                      , tcppoi->ip);
        return(NULL);                     /* Alle SSID-Nummern sind belegt.   */
      }
  }
  else                                      /* Rufzeichen ohne SSID gefunden. */
    return(scall);                          /* Rufzeichen setzen.             */
}

/* Pruefe Loginrufzeichen. */
static BOOLEAN CheckCall(MBHEAD *tbp,               /* Buffer mit Logindaten. */
                         char *contens)  /* Zeiger fuer erweitere Logibdaten. */
{
  char  scall[256];
  char *call = scall;
  char  logincall[L2IDLEN];
  char *pcall = logincall;
  WORD  len = FALSE;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(CheckCall)";
#endif /* DEBUG_MODUS */

  if ((len = CheckData(tbp, scall, contens)) == FALSE)   /* Rufzeichen holen. */
    return(TRUE);                                  /* Ungueltiges Rufzeichen. */

  if (getcal(&len,&call,FALSE,logincall) == YES)        /* Rufzeichen pruefen */
  {
    if (valcal(logincall) == ERRORS)    /* Pruefe Rufzeichen auf Gueltigkeit. */
    {
      ifpp = SetInterface(tcppoi->Interface);
      T_LOGL1(TRUE, "(CheckCall):%s\n%s : Ungueltiges Rufzeichen\n"
                    , tcppoi->ip
                    , pcall);
      return(TRUE);                                  /* Rufzeichen ungueltig. */
    }

                       /* Pruefen ob das Rufzeichen als suspend markiert ist. */
    if (is_down_suspended(logincall, tcppoi->port) == TRUE)
    {
      ifpp = SetInterface(tcppoi->Interface);
      T_LOGL1(TRUE, "(CheckCall):%s\n%s : Rufzeichen ist gesperrt!\n"
                    , tcppoi->ip
                    , pcall);
      return(TRUE);                               /* Rufzeichen ist gesperrt. */
    }

    if (cmpcal(logincall, myid))     /* Pruefen ob Rufzeichen unser Node ist. */
    {
      ifpp = SetInterface(tcppoi->Interface);
      T_LOGL1(TRUE, "(CheckCall):%s\n%s : Rufzeichen ist user NODE!\n"
                    , tcppoi->ip
                    , pcall);
      return(TRUE);                            /* Rufzeichen wird Missbrauch. */
    }

    /***********************************************/
    /* Ist das Rufzeichen schon mindestens einmal  */
    /* eingeloggt, vergeben wir anhand des letzten */
    /* Call, mit der hoehsten SSID, die SSID oder  */
    /* die angegebene SSID im Rufzeichen.          */
    /***********************************************/
    if ((pcall = CheckCallSSID(logincall)) != NULL)
    {
      cpyid(tcppoi->Upcall, pcall);                       /* Quellrufzeichen. */
      cpyid(usrcal, pcall);                    /* Aktuelle Rufzeichen setzen. */
      cpyid(tcppoi->Downcall, myid);                       /* Zielrufzeichen. */
      return(FALSE);                        /* Gueltiges Rufzeichen gefunden. */
    }
    else
    {
      ifpp = SetInterface(tcppoi->Interface);
      T_LOGL1(TRUE, "(CheckCall):%s\nSSID ausserhalb des Gueltigkeitsbereiches.\n"
                  , tcppoi->ip);

      return(TRUE);               /* SSID ausserhalb des Gueltigkeitsbereich. */
    }
  }

  ifpp = SetInterface(tcppoi->Interface);
  T_LOGL1(TRUE, "(CheckCall):%s\n%s : Ungueltiges Rufzeichen\n"
                , tcppoi->ip
                , call);
  return(TRUE);                                    /* Ungueltiges Rufzeichen. */
}

/* Rufzeichen anmelden. */
BOOLEAN LoginTCP(MBHEAD *tbp)
{
  char buffer[256 + 1];                 /* Buffer fuer erweiterte Logindaten. */
  char Upcall[10 + 1];
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(LoginTCP)";
#endif /* DEBUG_MODUS */

  buffer[0] = 0;

  if (CheckCall(tbp, buffer))           /* Pruefe Loginrufzeichen.            */
  {
    dealmb(tbp);                        /* Loginrufzeichen ist ungueltig,     */
    return(TRUE);                       /* Buffer entsorgen.                  */
  }

  /* ggf. Logbuch fuehren. */
  ifpp = SetInterface(tcppoi->Interface);
  call2str(Upcall, tcppoi->Upcall);
  T_LOGL2(TRUE, "(LoginTCP):%s\nEingeloggt als %s.\n"
                , tcppoi->ip
                , Upcall);

  tcppoi->login = TRUE;                 /* Markiere, User als eingeloggt.     */

  if (++nmbtcp > nmbtcp_max)            /* Maximalwert fuer die Statistik     */
    nmbtcp_max = nmbtcp;

  tcppoi->state = L2MCONNT;             /* Statusmeldung an den L7 Melden.    */

#ifdef L1IPCONV
  /* Ist Interface IPConvers? */
  if (tcppoi->Interface == KISS_IPCONV)
  {
    int  i = 0,
         j = 0;

    if (buffer[0] != FALSE)
    {
      buffer[256] = 0;                              /* Maximale Buffergrenze. */

      tcppoi->cmdlen = strlen(buffer);                      /* Laenge setzen. */

      if (tcppoi->cmdlen > 0)                                /* Kanalangabe.  */
      {
        while(tcppoi->cmdlen--)
          tcppoi->cmd[i++] = buffer[j++];                  /* Channel setzen. */

        tcppoi->cmd[i++] = 0;                          /* Nullzeichen setzen. */

        ifpp = SetInterface(tcppoi->Interface);     /* akt. Interface setzen. */
        T_LOGL2(TRUE, "(LoginTCP):%s\nMit Channelangabe (%s).\n"
                , tcppoi->ip
                , tcppoi->cmd);
      }
    }
  }
#endif /* L1IPCONV */

 dealmb(tbp);                           /* Buffer entsorgen.                  */
 return(FALSE);
}

/* Packet umhaengen. */
void RelinkTCP(MBHEAD *tbp)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(RelinkTCP)";
#endif /* DEBUG_MODUS */

  if (tbp->mbpc)                                   /* Es gíbt daten.          */
  {
    rwndmb(tbp);                                   /* Buffer zurueckspulen.   */
                                                   /* Packet umhaengen.       */
    relink((LEHEAD *)tbp, (LEHEAD *)tcppoi->inbuf.tail);
    ++tcppoi->inlin;                               /* Eingabebuffer hat Daten */

    if (tcppoi->login == TRUE)
      MhUpdateTCP(tbp, TRUE);                      /* MH-Liste Aktualisieren. */

    tcppoi->noacti = ininat;                       /* Timeout neu aufziehen.  */
    tcppoi->T3 = T3PARA;                           /* T3-Timer neu aufziehen. */
    tcppoi->cmdlen = 0;                            /* Zaehler zuruecksetzen.  */
  }
}

/* Anhand des Sockets den User aus der Liste holen. */
TCPIP *FdReadTCP(Fd_set Rmask)
{
  int i, j = FALSE;
  LHEAD  *h_llp;
  h_llp = &tcpactl;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(FdReadTCP)";
#endif /* DEBUG_MODUS */

  for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi) /* TBL durchgehen.*/
  {
    if (tcppoi->mode != INT_STACK)
      continue;

    if (tcppoi->activ)                                     /* Nur wenn aktiv. */
    {
      if (CheckSocket())                      /* Pruefe, ob Socket aktiv ist. */
      {
        CloseSockTCP(FALSE, tcppoi->Interface);          /* Schliesse Socket. */
        return(NULL);            /* Socket deaktiv, Segment auf DISC stellen. */
      }

      if (FD_ISSET_T(tcppoi->sock, &Rmask))               /* Socketvergleich. */
      {
        ulink((LEHEAD *)tcppoi);            /* wir haben einen Link gefunden. */
        relink((LEHEAD *)tcppoi, (LEHEAD *)h_llp->tail);
        return(tcppoi);
      }
    }

    if (  ((tcppoi->activ)                 /* Socket ist aktiv,               */
        && (j++ >= tcp_tbl_top))           /* merken und pruefen,             */
        || (j == tcp_tbl_top))             /* alle aktiven Socket's bearbeitet*/
      break;                               /* koennen wir abbrechen.          */
  }

  return(NULL);                                   /* Keinen Eintrag gefunden. */
}

/* Anhand des Sockets den User aus der Liste holen. */
TCPIP *FdReadSockTCP(int Sock, UWORD Interface, UWORD Mode)
{
  int i, j = FALSE;
  LHEAD  *h_llp;
  h_llp = &tcpactl;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(FdReadSockTCP)";
#endif /* DEBUG_MODUS */

  for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi) /*TBL durchgehen. */
  {
    if (tcppoi->activ)                                     /* Nur wenn aktiv. */
    {
      if (  (tcppoi->sock      == Sock)                   /* Socketvergleich. */
          &&(tcppoi->Interface == Interface)
          &&(tcppoi->mode      == Mode))
      {
        if (CheckSocket())                    /* Pruefe, ob Socket aktiv ist. */
        {
          CloseSockTCP(FALSE, tcppoi->Interface);        /* Schliesse Socket. */
          return(NULL);          /* Socket deaktiv, Segment auf DISC stellen. */
        }

        ulink((LEHEAD *)tcppoi);            /* wir haben einen Link gefunden. */
        relink((LEHEAD *)tcppoi, (LEHEAD *)h_llp->tail);
        return(tcppoi);
      }
    }

    if (  ((tcppoi->activ)                 /* Socket ist aktiv,               */
        && (j++ >= tcp_tbl_top))           /* merken und pruefen,             */
        || (j == tcp_tbl_top))             /* alle aktiven Socket's bearbeitet*/
      break;                               /* koennen wir abbrechen.          */
  }

  return(NULL);                                   /* Keinen Eintrag gefunden. */
}

/* Sender bis max. TXLEN fuellen. */
static BOOLEAN PutvSockTCP(int c)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(PutvSockTCP)";
#endif /* DEBUG_MODUS */

  if (tcppoi->txc >= TXLEN)                       /* Maximale TXLEN erreicht. */
  {
    tcppoi->txbuf[tcppoi->txc] = 0;

    if (PutflushSockTCP())                                /* Frame senden. */
      return(TRUE);
  }

  tcppoi->txbuf[tcppoi->txc++] = c;                       /* Fuelle txbuffer. */
  return(FALSE);
}

/* Eingehende Frames bearbeiten. */
void TcpipRX(void)
{
  READRX *SRx;
  READRX *SRxPrev;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(TcpipRX)";
#endif /* DEBUG_MODUS */

  for (SRx  = (READRX *)rxflRX.head;          /* Alle RX-Segmente durchgehen. */
       SRx != (READRX *)&rxflRX;
       SRx  = SRxPrev)
  {
    SRxPrev = SRx->next;

                                       /* Aktuellen User aus der Liste holen. */
    if ((tcppoi = FdReadSockTCP(SRx->Sock, SRx->Interface, SRx->Mode)) == NULL)
    {
      /* ggf. Logbuch fuehren. */
      ifpp = SetInterface(SRx->Interface);
      T_LOGL1(TRUE, "(TcpipRX):\nKein User gefunden (Sock:%d, Interface:%d, Mode:%d\n"
                    "RX-Segment entsorgen.\n"
                    , SRx->Sock
                    , SRx->Interface
                    , SRx->Mode);

      if (SRx->Data != NULL)                         /* Wenn es daten gibt,   */
        dealmb(SRx->Data);                           /* entsorgen.            */

      dealoc((MBHEAD *)ulink((LEHEAD *)SRx));        /* RX-Segment entsorgen. */
      continue;
    }

    switch(tcppoi->Interface)
    {
#ifdef L1HTTPD
      case KISS_HTTPD :
        /* HTTPD-Service. */
        TcpipHttpd(SRx->Data);

        /* Buffer entsorgen. */
        if (SRx->Data != NULL)
          dealmb(SRx->Data);

        dealoc((MBHEAD *)ulink((LEHEAD *)SRx));        /* TX-Segment entsorgen. */
        /* naechstes Frame aus der Liste holen. */
       break;
#endif /* L1HTTPD */

      default :
        if (!tcppoi->login)                              /* Nicht angemeldet.     */
        {
          if (LoginTCP(SRx->Data))                     /* User/Link anmelden.   */
            tcppoi->disflg |= 0x80;           /* Anmeldung war nicht erfolgreich. */
        }
        else                                          /* User-Link ist angemeldet.*/
          RelinkTCP(SRx->Data);                     /* Frame weiterleiten.      */

        dealoc((MBHEAD *)ulink((LEHEAD *)SRx));           /* RX-Segment entsorgen. */
       break;
    }
  }
}

/* Ausgehende Frames senden. */
void TcpipTX(void)
{
  SENDTX *STx;
  SENDTX *STxPrev;
  char    buf;
  int     len = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(TcpipTX)";
#endif /* DEBUG_MODUS */

  for (STx  = (SENDTX *)rxflTX.head;          /* Alle TX-Segmente durchgehen. */
       STx != (SENDTX *)&rxflTX;
       STx  = STxPrev)
  {
    STxPrev = STx->next;

                                       /* Aktuellen User aus der Liste holen. */
    if ((tcppoi = FdReadSockTCP(STx->Sock, STx->Interface, STx->Mode)) == NULL)
    {
      /* ggf. Logbuch fuehren. */
      ifpp = SetInterface(STx->Interface);
      T_LOGL1(TRUE, "(TcpipRX):\nKein User gefunden (Sock:%d, Interface:%d, Mode:%d\n"
                    "TX-Segment entsorgen.\n"
                    , STx->Sock
                    , STx->Interface
                    , STx->Mode);

      if (STx->Data != NULL)                           /* Wenn es daten gibt, */
        dealmb(STx->Data);                                      /* entsorgen. */

      dealoc((MBHEAD *)ulink((LEHEAD *)STx));        /* TX-Segment entsorgen. */
      continue;                                     /* zum naechsten Segment. */
    }

    if (tcppoi->txstatus == TCP_TX_BUSY)            /* Sender steht auf Busy. */
        continue;                                     /* zum naechsten eintrag. */

    if (tcppoi->login == TRUE)
      MhUpdateTCP(STx->Data, FALSE);                      /* MH-Liste updaten */

                                              /* solange Daten vorhanden sind */
    while (STx->Data->mbgc != STx->Data->mbpc)
    {
      if (PutvSockTCP(buf = getchr(STx->Data)))  /* Zeichen in sendebuffer. */
      {
        if (STx->Data != NULL)                         /* Wenn es daten gibt, */
          dealmb(STx->Data);                                    /* entsorgen. */

        dealoc((MBHEAD *)ulink((LEHEAD *)STx));      /* TX-Segment entsorgen. */
        continue;                                   /* zum naechsten eintrag. */
      }
    }

    tcppoi->txbuf[tcppoi->txc] = 0;
    PutflushSockTCP();                                       /* Frame senden. */

    if (STx->Data != NULL)                             /* Wenn es daten gibt, */
      dealmb(STx->Data);                                        /* entsorgen. */

    dealoc((MBHEAD *)ulink((LEHEAD *)STx));          /* TX-Segment entsorgen. */

    len = 0;                                                /* Zuruecksetzen. */
  }
}

T_INTERFACE *SetInterface(UWORD Interface)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(SetInterface)";
#endif /* DEBUG_MODUS */

  switch(Interface)
  {
    case KISS_TELNET :
      ifpp = &ifp[TEL_ID];
     break;

    case KISS_HTTPD :
      ifpp = &ifp[HTP_ID];
     break;

    case KISS_IPCONV :
      ifpp = &ifp[CVS_ID];
     break;

    default :
     break;
  }

  return(ifpp);
}

/*********************************/
/* Statusaenderungen,            */
/* Frame vorbereiten zum Senden. */
/* evl. Socket schliessen.       */
/*********************************/
void TcpipSV(void)
{
  int   i, j = FALSE;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(TcpipSV)";
#endif /* DEBUG_MODUS */

  for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi) /*TBL durchgehen. */
  {
    if (tcppoi->activ)                                   /* Socket ist aktiv. */
    {
      if (tcppoi->state != L2MNIX)                       /* Statusaenderung.  */
      {
        l2tol7(tcppoi->state, tcppoi, TCP_USER);   /* Statusmeldungen an L7.  */
        tcppoi->state = L2MNIX;                    /* Status auf 0 setzen.    */
        cpyid(tcppoi->Upcall, usrcal);             /* Quellrufzeichen setzen. */

#ifdef L1IPCONV
        if (  (tcppoi->Interface == KISS_IPCONV)
            &&(tcppoi->Intern    == FALSE))
          IPConvLogin();
#endif /* L1IPCONV */
#ifdef L1IRC
        if (  (tcppoi->Interface == KISS_IRC)
            &&(tcppoi->Intern    == FALSE))
          IPConvLogin();
#endif /* L1IRC */
      }

      if (NotContensTCP())                   /* Frame vorbereiten zum Senden. */
        CloseSockTCP(FALSE, tcppoi->Interface);        /* Socket schliessen. */
    }

    if (  ((tcppoi->activ)                /* Socket ist aktiv,                */
        && (j++ >= tcp_tbl_top))          /* merken und pruefen,              */
        || (j == tcp_tbl_top))            /* alle aktiven Socket's bearbeitet.*/
      break;                              /* koennen wir abbrechen.           */
  }
}


/* Lausche auf TCP-Stack. */
void ListenTCP(void)
{
  UWORD        Interface = KISS_TCPIP;
  int          count,
               max_fdI = 0,
               i;
  Fd_set       Rmask;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(ListenTCP)";
#endif /* DEBUG_MODUS */

  FD_ZERO_T(&Rmask);

  /* fuer alle Interfaces die benutzten Filedescriptoren fuer select() */
  /* ermitteln und eintragen */
  for (i = 0; i < MAXINTERFACE; i++, Interface++)
  {                                     /* Zeiger auf das aktuelle Interface. */
    if ((ifpp = SearchIf(Interface)) == NULL)
      continue;                                   /* Zum naechsten Interface. */

    if (ifpp->actively == FALSE)                 /* Interface ist nicht aktiv.*/
      continue;                                   /* Zum naechsten Interface. */

    FD_SET_T((unsigned)ifpp->ISock, &Rmask);                /* Socket setzen. */

    if (ifpp->ISock > max_fdI - 1)
      max_fdI = ifpp->ISock + 1;
  }

  PutSocketTCP(&Rmask, &max_fdI);                           /* Socket setzen. */

  /* Pruefe auf Aktivitaet, Int.Stack. */
  count = Select(max_fdI, &Rmask, NULL, NULL, NULL);

  if (!count)                                        /* Keine Aktivitaet.     */
    return;                                          /* Zur Hauptschleife.    */

  Interface = KISS_TCPIP;                       /* 1. TCPIP-Interface setzen. */

  for (i = 0; i < MAXINTERFACE; i++, Interface++)
  {                                     /* Zeiger auf das aktuelle Interface. */
    if ((ifpp = SearchIf(Interface)) == NULL)
      continue;                                   /* Zum naechsten Interface. */

    if (ifpp->actively == FALSE)                /* Interface ist nicht aktiv. */
      continue;                                   /* Zum naechsten Interface. */

    if (FD_ISSET_T(ifpp->ISock, &Rmask))
    {
      char       ip[IPADDR + 1];
      int        NewSock   = EOF;
      Socklen_t  addrlen   = EOF;
      ULONG      peerip    = 0;
      char      *p         = (char *) &peerip;

      addrlen = sizeof Peeraddr_in;                  /* Neuen Socket anlegen. */
      if ((NewSock = Accept(ifpp->ISock, (struct Sockaddr *) &Peeraddr_in, &addrlen)) != EOF)
      {
        peerip = Peeraddr_in.sin_addr._S_addr;       /* IP-Adresse ermitteln. */
        sprintf (ip, "%d.%d.%d.%d",(p[3] & 255)
                                  ,(p[2] & 255)
                                  ,(p[1] & 255)
                                  ,(p[0] & 255));
      }

      if (AddUserTCP(ifpp, NewSock, ip))            /* Neuen User hinzufuegen */
      {
        Close(NewSock);
        continue;                                 /* zum naechsten Interface. */
      }

      T_LOGL2(TRUE, "(RecvTCP):\n"
                    "Verbindung zur IP-Adresse %s angenommen.\n"
                    "Neu erzeugter Socket ist (%d).\n"
                    , ip
                    , NewSock);
      continue;                                   /* zum naechsten Interface. */
    }                                             /* Interface-Schnittstelle. */

    if ((tcppoi = FdReadTCP(Rmask)) != NULL)  /* Den User aus der Liste Holen */
    {
      T_LOGL3(TRUE, "(ListenTCP):%s\nAktuellen Socket (%d) in der Liste gefunden.\n"
                    , tcppoi->ip
                    , tcppoi->sock);

      ServTCP();                     /* Eingehende TCPIP Packete verarbeiten. */
      continue;                                   /* zum naechsten Interface. */
    }                                                       /* Socket suchen. */
  }                                                          /* for-schleife. */
}

/* TCPIP-Service. */
void TcpipSRV(void)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(TcpipSRV)";
#endif /* DEBUG_MODUS */

#ifdef OS_STACK
  ListenTCP_OS();
#endif /* OS_STACK */
  ListenTCP();                                      /* Lausche auf TCP-Stack. */
  TcpipTX();               /* ausstehende Frames senden wenn vorhanden        */
  TcpipRX();               /* eingehende Frame analysieren.                   */
                           /* Frames in der Warteschlange umhaengen.          */
  TcpipSV();               /* Pruefe auf Status, Informationstransfer und ggf */
}

/* Aktuelle TCPIP-Daten sichern. */
void DumpTCP(MBHEAD* mbp)
{
  T_INTERFACE *ifpoi;
  UWORD        Interface = KISS_TCPIP;
  int          i;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(DumpTCP)";
#endif /* DEBUG_MODUS */

  for (i = 0; i < MAXINTERFACE; i++, ifpoi++)    /* INTERFACE-TBL durchgehen. */
  {
    if ((ifpoi = SearchIf(Interface++)) == NULL) /* Zeiger auf das akt. Inter.*/
      continue;                                   /* Zum naechsten Interface. */

    if (ifpoi->actively == TRUE)                 /* Nur wenn Interface aktiv. */
    {
      putprintf(mbp,";\r; %s-Server\r;\r", ifpoi->name);
      putprintf(mbp,"%s P ", ifpoi->name);
      putnum(Ntohs(ifpoi->tcpport), mbp);
      putprintf(mbp,"\r%s L %u",ifpoi->name, ifpoi->log);

      putstr("\r;\r",mbp);
    }
  }

#ifdef L1IPCONV
  IPConvDump(mbp);
#endif /* L1IPCONV */
}

/* Pruefe Loginzeichen. */
BOOLEAN CheckContens(char contents)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(CheckContens)";
#endif /* DEBUG_MODUS */

  if (   !(contents >= 'A' && contents <= 'Z')
      && !(contents >= 'a' && contents <= 'z')
      && !(contents >= '0' && contents <= '9')
      && !(contents == '-')
      && !(contents == '/')
#ifdef L1TELNET
      && !((contents == '#') && (tcppoi->Interface != KISS_TELNET))
#endif /* L1TELNET. */
#ifdef L1IPCONV
      && !((contents == ' ') && (tcppoi->Interface == KISS_IPCONV))
#endif /* L1IPCONV */
#ifdef L1IRC
      && !((contents == ' ') && (tcppoi->Interface == KISS_IRC))
#endif /* L1IRC */
      && !(contents == CR )
      && !(contents == LF ))
    return(TRUE);
   else
    return(FALSE);
}

/* Info vom L7 an TCPIP-Interface senden */
BOOLEAN itoTCP(BOOLEAN conflg, MBHEAD *ublk)
{
  TCPIP *tcppoi = (TCPIP *)ublk->l2link;
  int    mem  = (nmbfre_max / 2);
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(itoTCP)";
#endif /* DEBUG_MODUS */

  if (  (conflg == TRUE)
      ||(tcppoi->outlin < conctl))
  {
    if ((ublk->mbpc - ublk->mbgc) == 0)       /* Frames ohne Info ignorieren  */
    {
      T_LOGL1(TRUE, "(itoTCP):%s\nFrames ohne Info ignorieren.\n"
                  , tcppoi->ip);

      dealmb((MBHEAD *)ulink((LEHEAD *)ublk));             /* Frame entsorgen.*/
      return (TRUE);                                    /* Frame verarbeitet. */
    }

    if (  (nmbfre < 300)
        ||(mem < (nmbfre_max - nmbfre)))
    {
      dealmb((MBHEAD *)ulink((LEHEAD *)ublk));             /* Frame entsorgen.*/

      /* ggf. Logbuch fuehren. */
      ifpp = SetInterface(tcppoi->Interface);
      T_LOGL1(TRUE, "(itoTCP):%s\nSpeicher (%d) ist voll!\n"
                  , tcppoi->ip
                  , nmbfre);

      tcppoi->disflg |= 0x80;               /* Segment auf Disconnect setzen. */
      return (TRUE);                                    /* Frame verarbeitet. */
    }
                                        /* Markiere das es was zu senden gibt */
      relink(ulink((LEHEAD *)ublk), (LEHEAD *)tcppoi->outbuf.tail);
      ublk->type = L2MNIX;                      /* Info-Frame (keine Meldung)   */
      ublk->l4type = HMRINFO;
      ++tcppoi->outlin;                    /* Frame in die Warteschlange haengen. */
      return (TRUE);                                      /* Frame verarbeitet. */
  }

  /* ggf. Logbuch fuehren. */
  ifpp = SetInterface(tcppoi->Interface);
  T_LOGL1(TRUE, "(itoTCP):%s\nLink ist verstopft!\n"
              , tcppoi->ip);

  return (FALSE);                                      /* Link ist verstopft. */
}

/* User hat ein Disconnect eingeleitet. */
void SetDiscTCP(void)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(SetDiscTCP)";
#endif /* DEBUG_MODUS */

  /* ggf. Logbuch fuehren. */
  ifpp = SetInterface(tcppoi->Interface);
  T_LOGL2(TRUE, "(SetDiscTCP):%s\nTCP-Segment auf Disconnect setzen!\n"
              , tcppoi->ip);

  tcppoi->disflg |= 0x80;                   /* Segment auf Disconnect setzen. */
}

/* TCPIP-Timer */
void TimerTCP(void)
{
  int    i,
         j  = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(TimerTCP)";
#endif /* DEBUG_MODUS */

  for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi)/* TBL durchgehen. */
  {
    if (tcppoi->activ)                                     /* Nur wenn aktiv. */
    {
      if (  (tcppoi->noacti != FALSE)          /* Noactivity-Timer reduzieren */
          &&(--tcppoi->noacti == FALSE))       /* und ggf. User disconnecten. */
      {
        ifpp = SetInterface(tcppoi->Interface);
                                                     /* ggf. Logbuch fuehren. */
        T_LOGL1(TRUE, "(TimerTCP):%s\nNoactivity-Timer abgelaufen, Segment auf DISC stellen!\n"
              , tcppoi->ip);

        tcppoi->disflg |= 0x80;             /* Segment auf Disconnect setzen. */
        continue;
      }

      if (tcppoi->T3 == 0)                            /* T3-Timer abgelaufen. */
      {
        MhUpdateTCP(NULL, TRUE);                         /* MH-Liste updaten. */
        tcppoi->T3 = T3PARA;                            /* T3-Timer neusetzen */
      }
      else
        tcppoi->T3--;                  /* T3-Timer ist noch nicht abgelaufen, */
                                       /* weiter runterzaehlen.               */
    }

    if ( ((tcppoi->activ)           /* Sind alle aktiven Sockets durchlaufen, */
        && (j++ >= tcp_tbl_top))
        || (j == tcp_tbl_top))
      break;                                               /* brechen wir ab. */
  }
}

/* System- und Errormeldungen in einer Logdatei schreiben. */
void WriteLogTCP(BOOLEAN flag, const char *format, ...)
{
  FILE        *fp = NUL;
  va_list      arg_ptr;
  struct tm   *lt;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(WriteLogTCP)";
#endif /* DEBUG_MODUS */

  if (ifpp == NULL)                           /* Kein Interface angegeben!!!. */
  {
    printf("(WriteLogTCP): ACHTUNG, kein Interface angegeben!!!\n");
    return;                                       /* keine Meldung schreiben. */
  }

  switch(ifpp->Interface)
  {
#ifdef L1TELNET
    case KISS_TELNET:
      {
        if (!ifpp->log)                  /* Loglevel ist nicht eingeschaltet. */
          return;                        /* Keine Logmeldungen schreiben.     */

        fp = fopen(TELNETLOG, "a+");                        /* Datei oeffnen. */
          break;
      }
#endif /* L1TELNET */

#ifdef L1HTTPD
    case KISS_HTTPD:
      {
        if (!ifpp->log)                      /* Loglevel ist nicht eingeschaltet. */
          return;                            /* Keine Logmeldungen schreiben.     */

          fp = fopen(HTTPDLOG, "a+");                           /* Datei oeffnen. */
        break;
      }
#endif /* L1HTTPD */

#ifdef L1IPCONV
    case KISS_IPCONV:
      {
        if (!ifpp->log)                      /* Loglevel ist nicht eingeschaltet. */
          return;                            /* Keine Logmeldungen schreiben.     */

        fp = fopen(IPCONVLOG, "a+");                            /* Datei oeffnen. */
         break;
      }
#endif /* L1IPCONV */

#ifdef L1IRC
    case KISS_IRC:
      {
        if (!ifpp->log)                      /* Loglevel ist nicht eingeschaltet. */
          return;                            /* Keine Logmeldungen schreiben.     */

        fp = fopen(IRCLOG, "a+");                               /* Datei oeffnen. */
         break;
      }
#endif /* L1IPCONV */

    default :
      break;
  }

  if (fp == NULL)                           /* Fehler beim oeffnen der Datei. */
  {
    printf("(WriteLogTCP):\nLOG-Datei kann nicht geoeffnet werden!\n");
    return;                                 /* Abbruch.                       */
  }

  lt = localtime(&sys_time);

  if (flag)                                          /* Datum/Zeit schreiben. */
    fprintf(fp, "%16.16s ", ctime(&sys_time));

  va_start(arg_ptr, format);
  vfprintf(fp, format, arg_ptr);
  va_end(arg_ptr);
  fprintf(fp, "\n");
  fclose(fp);                                            /* Datei schliessen. */
}

/* TCPIP-Sockets killen. */
int KillTCP(UWORD port, char *call, WORD what)
{
  int      i            = FALSE;
  UWORD    kill_zaehler = FALSE;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(KillTCP)";
#endif /* DEBUG_MODUS */

  /* Alle Aktive Sockets durchlaufen. */
  for (i = 0, tcppoi = tcptbl; i < MAXTCPIP; ++i, ++tcppoi) /* TBL durchgehen */
  {
    if (  (tcppoi->activ)                                  /* Nur wenn aktiv. */
        &&((tcppoi->port == port)                             /* Port gleich. */
        || (port == 255)))                               /* oder alle Port's. */
    {
      switch(what)
      {
        case 1 :                                  /* Rufzeichen disconnecten. */
           if (!cmpid (call, tcppoi->Upcall))     /* Callvergleich.           */
             continue;                            /* zum naechsten eintrag.   */
          break;

        case 3 :                            /* Alle TCPIP-Links disconnecten. */
          break;

        default:                                        /* Ungueltige option. */
          continue;                                 /* Zum naechsten eintrag. */
      }

      ifpp = SetInterface(tcppoi->Interface);       /* akt. Interface setzen. */
                                                     /* ggf. Logbuch fuehren. */
      T_LOGL1(TRUE, "(KillTCP):%s\nSegment wird auf DISC gesetzt!\n"
                  , tcppoi->ip);

      tcppoi->disflg |= 0x80;                  /* Segment auf DISC stellen.   */
      kill_zaehler++;                          /* Killzaehler um eins hoeher. */
    }
  }

  return(kill_zaehler);
}

/* Buffer besorgen. */
MBHEAD *SetBuffer(void)
{
  MBHEAD *tbp = NULL;
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(SetBuffer)";
#endif /* DEBUG_MODUS */

  if (nmbfre < 300)                /* Buffer liegt unterhalb des Grenzbereich. */
    return(NULL);                              /* Kein Buffer zur Verfuegung. */

  if ((tbp = (MBHEAD *) allocb(ALLOC_L1TCPIP)) == NULL)   /* Buffer besorgen. */
    return(NULL);                     /* Es steht kein buffer zur Verfuegung. */

  return(tbp);
}

/* Pruefe, ob Socket aktiv ist. */
static BOOLEAN CheckSocket(void)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1tcpip(CheckSocket)";
#endif /* DEBUG_MODUS */

#ifdef OS_STACK
  if (tcppoi->mode == OS_MODE)
    return(CheckSocketOS());
#endif /* OS_STACK */

  if (Send(tcppoi->sock, "", 0, 0) < 0)
  {
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(CheckSocket):%s\nSocket ist nicht mehr aktiv!\n"
              , tcppoi->ip);

    /* Segment auf DISC setzen. */
    tcppoi->disflg |= 0x80;
    return(TRUE);
  }
  else
    return(FALSE);

  return(TRUE);
}

#endif /* L1TCPIP */

/* End of src/l1tcpip.c */
