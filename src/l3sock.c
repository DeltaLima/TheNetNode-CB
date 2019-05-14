#include "tnn.h"

#ifdef TCP_STACK

TSOCKET sockets[NUM_SOCKETS];

static int      SocketAlloc(void);                     /* Ein socket anlegen. */
static BOOLEAN  SearchPort(ULONG);       /* Pruefen, ob LocalPort belegt ist. */


/* Ein socket anlegen. */
int Socket(int domain, int type, int protocol)
{
  register int i;

  if ((i = SocketAlloc()) == EOF)                   /* Neuen Socket erstellen.*/
    return(EOF);                                    /* Kein Socket mehr frei. */

  sockets[i].Socket        = i;            /* Socket setzen.                  */
  sockets[i].Domain        = domain;       /* Domain setzen.                  */
  sockets[i].Type          = type;         /* Protokoll-Typ setzen.           */
  sockets[i].State         = TCP_SOCKET;   /* Socket-Status setzen.           */
  sockets[i].tos           = 0;            /* Typ of Service.                 */
  sockets[i].TState        = TCP_CLOSED;   /* TCP-Statusaenderungen.          */
  sockets[i].IpDest        = 0;            /* IP-Adresse vom Nachbarn.        */
  sockets[i].RecvNext      = 0;            /* Next-Frame.                     */
  sockets[i].SendNext      = 0;            /* Sequence Number                 */
  sockets[i].SendUnacked   = 0;            /* Sequence Number                 */
  sockets[i].LocalPort     = 0;            /* Local-Port setzen.              */
  sockets[i].PacNum        = 0;            /* Frame-Zahler.                   */
  sockets[i].MaxListen     = 0;            /* Groesse der Warteschlange.      */
  sockets[i].Listen        = 0;            /* Momentan in der Warteschlange.  */
  sockets[i].RecvEvent     = FALSE;        /* Select Signalisieren - Empfang. */
  sockets[i].SendEvent     = FALSE;        /* Select Signalisieren - Senden.  */
  sockets[i].DestPort      = 0;            /* Nachbar-Port.                   */
  sockets[i].UrgPointer    = 0;            /* Urgent Pointer.                 */

  return(i);                                                 /* Neuer Socket. */
}

/* Verbindungsbau annehmen/ablehnen. */
int Accept(int s, struct Sockaddr *addr, Socklen_t *addrlen)
{
  TSOCKET *sock = &sockets[s];
  int      NewSock;
  struct   Sockaddr_in sin;

  if (!sock)  /* Angegebener Socket wurde nicht in der Socket-Liste gefunden. */
    return(EOF);                                                /* Abbrechen. */

  memset(&sin, 0, sizeof(sin));                     /* Frischer Buffer holen. */
  sin.sin_family      = AF_INET;                              /* Type setzen. */
  sin.sin_port        = Htons((unsigned short)sock->LocalPort);  /* Local-Port*/
                                                                 /* setzen.   */
  sin.sin_addr._S_addr = sock->IpDest;                  /* IP-Adresse setzen. */

  memcpy(addr, &sin, *addrlen);

  sock->RecvEvent = FALSE;                   /* Select - Empfang ausschalten. */
  sock->SendEvent = FALSE;                   /* Select - Sender ausschalten.  */

  if ((NewSock = Socket(sock->Domain, sock->Type, 0)) == EOF)  /* Neuen Socket*/
                                                               /* anlegen.    */
    return(EOF);                  /* Neuer Socket kann nicht erstellt werden. */

  sockets[NewSock].Socket      = NewSock;
  sockets[NewSock].State       = TCP_CONNECTED;        /* Socket-Status auf   */
                                                         /* CONNECTED setzen. */
  sockets[NewSock].TState      = TCP_SYNCON;/* Verbindungsaufbau bestaetigen. */
  sockets[NewSock].IpDest      = sock->IpDest;          /* IP-Adresse setzen. */
  sockets[NewSock].RecvNext    = sock->RecvNext;
  sockets[NewSock].SendNext    = sock->SendNext;
  sockets[NewSock].SendUnacked = sock->SendUnacked;
  sockets[NewSock].tos         = sock->tos;        /* Type of Service setzen. */
  sockets[NewSock].LocalPort   = sock->LocalPort;       /* Local-Port setzen. */
  sockets[NewSock].DestPort    = sock->DestPort;      /* Nachbar-Port setzen. */
  sockets[NewSock].UrgPointer  = sock->UrgPointer;  /* Urgent Pointer setzen. */

  sock->Listen--;

  return(NewSock);                                           /* Neuer Socket. */
}

/* Socket schliessen. */
void Close(int Sock)
{
  register int i;

  for (i = 1; i < NUM_SOCKETS; ++i)           /* Durchsuche die Socket-Liste. */
  {
    if (sockets[i].Socket != Sock)                        /* Socketvergleich. */
      continue;                                     /* Zum naechsten Eintrag. */

    if (sockets[i].State == TCP_CONNECTED)    /* Socket mit Status CONNECTED. */
    {
      switch (sockets[i].TState)                               /* TCP-Status. */
      {
        case TCP_ACKWAIT :      /* Status Stabil: Warte auf ACK-Bestaetigung. */
          if (sockets[i].PacNum)                 /* Es liegen noch Frames an. */
            sockets[i].TState = TCP_PSHWAITFIN;  /* Neuer Status: Sende alle  */
          else                                   /* offenstehende Packete.    */
            sockets[i].TState = TCP_ACKWAITFIN;/* Neuer Status: Warte auf ACK,*/
         return;                               /* danach FIN senden.          */

        case TCP_ESTABLISHED :                      /* Verbindung ist stabil. */
          if (sockets[i].PacNum)                 /* Es liegen noch Frames an. */
            sockets[i].TState = TCP_PSHSENTFIN;    /* Neuer Status:Naechstes  */
          else                                     /* Packet senden.          */
            sockets[i].TState = TCP_FINSENT;   /* Neuer Status: Socket sofort */
         return;                               /* schliessen.                 */

        default :
          return;
      }
    }

    DelSock(sockets[i].Socket);                          /* Socket entsorgen. */
    return;
  }
}

/* Socket binden. */
int Bind(int s, struct Sockaddr *name, Socklen_t namelen)
{
  TSOCKET       *sock = &sockets[s];

  if (!sock)  /* Angegebener Socket wurde nicht in der Socket-Liste gefunden. */
    return(EOF);                                                /* Abbrechen. */

  if (sock->State != TCP_SOCKET) /* Socket-Status muss auf TCP_SOCKET stehen. */
    return(EOF);                             /* Status stimmt nicht ueberein. */

  if (SearchPort(((struct Sockaddr_in *)name)->sin_port))  /* Pruefe, ob der  */
                                                          /* Port belegt ist. */
    return(EOF);                                    /* Abbrechen.. */

  sock->LocalPort = Ntohs(((struct Sockaddr_in *)name)->sin_port);/* Port und */
  sock->State     = TCP_BIND;                   /* Socket-Status BIND setzen. */
  return(FALSE);
}

/* Lausche auf Socket. */
int Listen(int s, int Number)
{
  TSOCKET *sock = &sockets[s];

  if (!sock)  /* Angegebener Socket wurde nicht in der Socket-Liste gefunden. */
      return(EOF);                                              /* Abbrechen. */

  if (sock->State != TCP_BIND)                            /* Kein Status BIND */
    return(EOF);                                                /* Abbrechen. */

  sock->MaxListen = Number;              /* Groesse der Warteschlange setzen. */
  sock->State  = TCP_LISTEN;              /* Socket-Status auf LISTEN setzen. */
  return(FALSE);
}

/* Daten Empfangen. */
int Recv(int Sock, char *Buffer, int Len, int flag)
{
  TSOCKET *sock = &sockets[Sock];
#ifdef L1TCPIP
  DATENRX *Drx;
#endif /* L1TCPIP */
  int      DataLen = 0;
  int      i       = 0;

  if (!sock)  /* Angegebener Socket wurde nicht in der Socket-Liste gefunden. */
    return(EOF);                                                /* Abbrechen. */

  if (sock->TState >= TCP_CLOSE)            /* Socket soll geschlossen werden. */
  {
    sock->TState = TCP_CLOSED;
    return(EOF);                                     /* Dann machen wir das.. */
  }

#ifdef L1TCPIP
  if ((Drx = GetBuffer(sock)) == NULL) /* Buffer aus der EmpfangsListe holen. */
    return(FALSE);                                  /* Kein eintrag gefunden. */

  DataLen = Drx->Data->mbpc - Drx->Data->mbgc;     /* Gesamtlaenge ermitteln. */

  while (DataLen--)                           /* Zeichen in Buffer schreiben. */
  {
    if (Len == i)                                  /* Buffergroesse beachten. */
      break;                                     /* Wenn zu gross, abbrechen. */

    Buffer[i++] = getchr(Drx->Data);                   /* Schreibe in Buffer. */
  }

  if (Drx->Data->mbpc == Drx->Data->mbgc)        /* Alle zeichen verarbeitet. */
  {
    sock->RecvEvent = FALSE;                   /* Select Empfang ausschalten. */
    dealmb((MBHEAD *)Drx->Data);                         /* Buffer entsorgen. */
    dealoc((MBHEAD *)ulink((LEHEAD *)Drx));          /* TX Segment entsorgen. */
  }
  else
    {
      relink((LEHEAD *) Drx, ((LEHEAD *)rxDaten.tail)); /* Umhaengen in die RX*/
                                                        /* Buffer-Liste.      */
      sock->RecvEvent = TRUE;           /* Select siginalisieren Empfang ein. */
    }
#endif /* TCP_L1TCPIP */

  return(i);                                     /* Anzahl zeichen im Buffer. */
}

/* Daten Senden. */
int Send(int Sock, char *Buffer, int DataLen, int Flag)
{
  TSOCKET *sock = &sockets[Sock];
  MBHEAD  *Data = NULL;
  int      i    = 0;

  if (!sock)  /* Angegebener Socket wurde nicht in der Socket-Liste gefunden. */
    return(EOF);                                                /* Abbrechen. */

  if (sock->TState >= TCP_CLOSE)            /* Socket soll geschlossen werden. */
  {
    sock->TState = TCP_CLOSED;
    return(EOF);                              /* Keine weiteren Daten senden. */
  }

  if (DataLen)                                       /* Es gibt was zusenden. */
  {
    if ((Data = (MBHEAD *)allocb(ALLOC_TCPSTACK)) == NULL)/* Buffer besorgen. */
      return(FALSE);                                      /* Buffer ist voll. */

    while (DataLen--)                    /* Alle zeichen in Buffer schreiben. */
    {
      if (i == 214)                         /* Maximum Segment Size erreicht. */
      {
        rwndmb(Data);                               /* Buffer zurueck spulen. */
        PutTXStack(sock->Socket,               /* In die Sende-Liste haengen. */
                   Data,
                   Data->mbpc - Data->mbgc,
                   (TACK + TPSH),
                   TCP_PSHSENT);

        sock->SendEvent = FALSE;          /* Select signalisieren Sender aus. */
        sock->SendNext += i;                /* Sequence Number aktualisieren. */
        return(i);                               /* Anzahl zeichen im Buffer. */
      }

      putchr(Buffer[i++], Data);               /* Schreibe zeichen in Buffer. */
    }

    rwndmb(Data);                                    /* Buffer zurueckspulen. */
    PutTXStack(sock->Socket,                   /* In die Sende-Liste haengen. */
               Data,
               Data->mbpc - Data->mbgc,
               (TACK + TPSH),
               TCP_PSHSENT);

    sock->SendEvent = FALSE;              /* Select signalisieren Sender aus. */
    sock->SendNext += i;                    /* Sequence Number aktualisieren. */
  }

  return(i);                                     /* Anzahl zeichen im Buffer. */
}

/* Den Socket auf Aktivitaet pruefen. */
int SelScan(int max_fd, Fd_set *readset, Fd_set *writeset, Fd_set *exceptset)
{
  int          nready = 0;
  register int i;
  Fd_set       lreadset,
               lwriteset,
               lexceptset;

  FD_ZERO_T(&lreadset);                        /* Filedescriptoren entleeren. */
  FD_ZERO_T(&lwriteset);
  FD_ZERO_T(&lexceptset);

  for (i = 1; i < NUM_SOCKETS; ++i)           /* Durchsuche die Socket-Liste. */
  {
    if (FD_ISSET_T(sockets[i].Socket, readset))    /* Socketvergleich - Read. */
    {
      if (sockets[i].RecvEvent)      /* Es liegen daten in der Empfangsliste. */
      {
        FD_SET_T((unsigned)sockets[i].Socket, &lreadset);   /* Socket setzen. */
        nready++;                         /* Markiere, es wird was empfangen. */
      }
    }

    if (FD_ISSET_T(sockets[i].Socket, writeset))  /* Socketvergleich - Write. */
    {
      if (sockets[i].SendEvent)         /* Es liegen daten in der Sendeliste. */
      {
        FD_SET_T((unsigned)sockets[i].Socket, &lwriteset);  /* Socket setzen. */
        nready++;                          /* Markiere, es wird was gesendet. */
      }
    }
  }

  *readset  = lreadset;                               /* Aktion weiterleiten. */
  *writeset = lwriteset;
  FD_ZERO_T(exceptset);                        /* Filedescriptoren entleeren. */

  return nready;
}

/* Den Socket auf Aktivitaet pruefen. */
int Select(int max_fd,
               Fd_set         *readset,
               Fd_set         *writeset,
               Fd_set         *exceptset,
               struct Timeval *timeout)
{
  int    nready;
  Fd_set lreadset,
         lwriteset,
         lexceptset;

  if (readset)                          /* Es soll auf Lesen geprueft werden. */
    lreadset = *readset;                           /* Socket-liste uebergeben.*/
  else
    FD_ZERO_T(&lreadset);                      /* Filedescriptoren entleeren. */

  if (writeset)                     /* Es soll auf Schreiben geprueft werden. */
    lwriteset = *writeset;                         /* Socket-liste uebergeben.*/
  else
    FD_ZERO_T(&lwriteset);                     /* Filedescriptoren entleeren. */

  FD_ZERO_T(&lexceptset);                      /* Filedescriptoren entleeren. */

  nready = SelScan(max_fd,              /* Den Socket auf Aktivitaet pruefen. */
                   &lreadset,
                   &lwriteset,
                   &lexceptset);

  if (readset)                          /* Es soll auf lesen geprueft werden. */
    *readset = lreadset;                     /* Neue Socket-liste uebergeben. */

  if (writeset)                     /* Es soll auf schreiben geprueft werden. */
    *writeset = lwriteset;                   /* Neue Socket-liste uebergeben. */

  return nready;
}

/* Einen neuen Socket anlegen. */
static int SocketAlloc(void)
{
  register int i;

  for(i = 1; i < NUM_SOCKETS; i++)             /* Durchsuche die Socketliste. */
  {
    if (sockets[i].State == TCP_CLOSED)                    /* Eintrag ist frei. */
      return(i);                                             /* Neuer Socket. */
  }

  return(EOF);                              /* Kein freier Eintrag vorhanden. */
}

/* Pruefe, ob der Socket in der Filedescriptoren-Liste steht. */
BOOLEAN GetSocket(Socklen_t Sock, Fd_set *rmask)
{
  unsigned int i;

  for (i = 0; i < rmask->fd_count; i++)      /* Durchsuche alle FD-Eintraege. */
  {                                            /* Neuen Eintrag ermitteln und */
    if ((Socklen_t)rmask->fd_array[i] == Sock)    /* vergleichen. */
      return(TRUE);                                       /* Socket gefunden. */
  }

  return(FALSE);                                     /* Kein Socket gefunden. */
}

/* Pruefe, ob der Local-Port belegt ist. */
static BOOLEAN SearchPort(ULONG LocalPort)
{
  register int i;

  for (i = 1; i < NUM_SOCKETS; ++i)            /* Durchsuche die Socketliste. */
  {
    if (sockets[i].LocalPort == LocalPort)                  /* Portvergleich. */
      return(TRUE);                                 /* Port ist schon belegt. */
  }

  return(FALSE);                                       /* Kein Port gefunden. */
}

/* Den Socket aus der Socketliste suchen. */
int SearchSock(int Sock)
{
  register int i;

  for (i = 1; i < NUM_SOCKETS; ++i)            /* Durchsuche die Socketliste. */
  {
    if (sockets[i].Socket == Sock)                        /* Socketvergleich. */
      return(sockets[i].Socket);                          /* Socket gefunden. */
  }

  return(EOF);            /* Kein Socket gefunden oder mittlerweile entsorgt. */

}

/* Socket als unbenutzt markieren. */
void DelSocket(int Sock)
{
  register int i;

  for (i = 1; i < NUM_SOCKETS; ++i)            /* Durchsuche die Socketliste. */
  {
    if (sockets[i].Socket != Sock)                        /* Socketvergleich. */
      continue;                                      /* Zum naechsten socket. */

    sockets[i].State = TCP_CLOSED;          /* Markiere socket als unbenutzt. */
    return;
  }
}

/* konvertiert die Kurzganzzahl hostshort Rechner- nach Netzwerk-Byteordnung.*/
unsigned short
Htons(unsigned short n)
{
  return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

/* konvertiert die Kurzganzzahl netshort von Netzwerk- nach Rechner-Byteordnung. */
unsigned short
Ntohs(unsigned short n)
{
  return Htons(n);
}

/* konvertiert die Langganzzahl hostlong von Rechner- nach Netzwerk-Byteordnung. */
unsigned long
Htonl(unsigned long n)
{
  return ((n & 0xff) << 24) |
    ((n & 0xff00) << 8) |
    ((n & 0xff0000) >> 8) |
    ((n & 0xff000000) >> 24);
}

/* konvertiert die Langganzzahl netlong von Netzwerk- nach Rechner-Byteordnung. */
unsigned long
Ntohl(unsigned long n)
{
  return Htonl(n);
}

#endif /* TCPSTACK */

/* End of src/l3sock.c */
