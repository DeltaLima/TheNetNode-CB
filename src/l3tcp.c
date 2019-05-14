#include "tnn.h"
#ifdef TCP_STACK

LHEAD rxSegment;                                           /* RX TCP-Segment. */
LHEAD txSegment;                                           /* TX TCP-Segment. */
LHEAD rxDaten;                                                  /* RX-Buffer. */


static void     StackRX(void);                     /* Eingehende RX Segmente. */
static void     StackState(void);                        /* Statusanderungen. */
static void     StackTX(void);                     /* Ausgehende TX Segmente. */


/* Initialisiere, */
void StackInitTCP(void)
{
  inithd(&rxSegment);                           /* Frame fuer Empfangs-Liste. */
  inithd(&txSegment);                              /* Frame fuer Sende-Liste. */
  inithd(&rxDaten);                                          /* RX-Bufferung. */
}

/* TCP-Service */
void StackSRV(void)
{
  StackRX();                                       /* Eingehende RX-Segmente. */
  StackState();                                         /* Statusaenderungen. */
  StackTX();                                       /* Ausgehende TX-Segmente. */
}


/* Neues TX-Segment in die Sende-Liste anlegen/anhaengen. */
void PutTXStack(register int  i    ,                /* SOCKET                 */
                MBHEAD        *Data,                /* Daten ohne TCP-Header. */
                int            DataLen,             /* Datenlaenge.           */
                unsigned short Flags,               /* TCP-Flag.              */
                unsigned char  TState)              /* Sendestatus.           */
{
  STACKTX *TSeq;

  if ((TSeq = (STACKTX *)allocb(ALLOC_TCPSTACK)) == NULL) /* Buffer besorgen. */
  {
    sockets[i].TState = TCP_RSTSENT; /* Kein Speicher,Verbindung zuruecksetzen*/
    return;
  }

  TSeq->Sock     = i;                                       /* Socket setzen. */
  TSeq->Data     = Data;                                     /* Daten setzen. */
  TSeq->Flags    = Flags;                                 /* TCP-Flag setzen. */
  TSeq->TState   = TState;                            /* Sende-Status setzen. */
  TSeq->TimeLast = time(NULL);
  TSeq->Timer    = 0;                                 /* Timer zuruecksetzen. */
  TSeq->Retry    = 0;                                 /* Retry zuruecksetzen. */

  ++sockets[i].PacNum;                  /* Ein Frame mehr in der Sende-Liste. */

  relink((LEHEAD *)TSeq, (LEHEAD *)txSegment.tail);     /* In die Sende-Liste */
}                                                       /* haengen.           */

/* Entsorge alle RX-Daten. */
static void DelDatenRX(int Sock)
{
  DATENRX *Drx;
  DATENRX *DrxPrev;

  for (Drx  = (DATENRX *)rxDaten.head;           /* Durchsuche RX-Datenliste. */
       Drx != (DATENRX *)&rxDaten;
       Drx  = DrxPrev)
  {
    DrxPrev = Drx->next;

    if (Drx->Sock != Sock)                                /* Socketvergleich. */
      continue;                                     /* Zum naechsten Eintrag. */

    if (Drx->Data != NULL)                             /* Wenn es Daten gibt, */
      dealmb(Drx->Data);                              /* entsorgen. */

    dealoc((MBHEAD *)ulink((LEHEAD *)Drx));          /* RX-Segment entsorgen. */
  }
}

/* Entsorge alle TX-Segment. */
static void DelTXSegment(int Sock)
{
  STACKTX *TSeq;

  for (TSeq  = (STACKTX *)txSegment.head;  /* Durchsuche die TX-Segmentliste. */
       TSeq != (STACKTX *)&txSegment;
       TSeq  = TSeq->next)
  {
    if (TSeq->Sock != Sock)                                /* Socketvegleich. */
      continue;                                     /* Zum naechsten Eintrag. */

    if (TSeq->Data != NULL)                            /* Wenn es daten gibt, */
      dealmb(TSeq->Data);                                       /* entsorgen. */

    dealoc((MBHEAD *)ulink((LEHEAD *)TSeq));         /* TX-Segment entsorgen. */
    return;
  }
}

/* Entsorge alle Buffer vom Socket. */
void DelSock(int Sock)
{
  DelDatenRX(Sock);                                    /* RX-Daten entsorgen. */
  DelTXSegment(Sock);                               /* TX-Segmente entsorgen. */
  DelSocket(Sock);                                       /* Socket entsorgen. */
}

/* Einen Eintrag aus der RX-Liste holen. */
DATENRX *GetBuffer(TSOCKET *Sock)
{
  DATENRX *Drx;

  for (Drx  = (DATENRX *)rxDaten.head;            /* Durchsuche die RX-Liste. */
       Drx != (DATENRX *)&rxDaten;
       Drx  = Drx->next)
  {
    if (Sock->Socket != Drx->Sock)                        /* Socketvergleich. */
      continue;                                      /* Zum naechsten Socket. */

    return(Drx);                                        /* Aktueller Eintrag. */
  }

  return(NULL);                                     /* Kein Eintrag gefunden. */
}

/* Eingehende TCP-Daten weiterleiten in die RX-Segmentliste. */
void TCPIPProcess(register IP     *ip,
                  register MBHEAD *bp)
{
  MBHEAD  *Data;
  STACKRX *RSeq;

  if ((Data = (MBHEAD *)allocb(ALLOC_MBHEAD)) == NULL)    /* Buffer besorgen. */
    return;          /* Kein Speicher, Empfangsbuffer entsorgt der IP-Router. */

  while (bp->mbgc != bp->mbpc)       /* Alle Zeichen in den Buffer schreiben. */
    putchr(getchr(bp), Data);

  rwndmb(Data);                                      /* Buffer zurueckspulen. */

  if ((RSeq = (STACKRX *)allocb(ALLOC_TCPSTACK)) == NULL)/* RX-Segment besorgen.*/
  {
    dealmb(Data);                         /* Kein Speicher, Buffer entsorgen. */
    return;
  }

  RSeq->Data  = Data;                                /* Empfang-Daten setzen. */
  RSeq->IpHdr = ip;                                  /* IP-Header setzen.     */

  relink((LEHEAD *)RSeq, (LEHEAD *)rxSegment.tail);       /* Umhaengen in die */
  return;                                                 /* Empfangs-Liste.  */
}

/* TCP-Header lesen. */
void ReadTcpHeader(TCP    *tcp,
                   IP     *ip,
                   MBHEAD *bp)
{
  int temp_flag;

  tcp->srcPort        = get16(bp);                 /* Port-Nummer der Quelle. */
  tcp->dstPort        = get16(bp);                 /* Port-Nummer des Ziel.   */
  tcp->seqnum         = get32(bp);         /* Sequenznummer des 1. Datenbytes */
                                           /* in diesem Segment.              */
  tcp->acknum         = get32(bp);                    /* Bestaetigungsnummer. */
  temp_flag           = get16(bp);                         /* Flags einlesen. */
  tcp->data_offset    = (temp_flag >> 12) & 0x0f;    /* Datenworte im header. */
  tcp->res            = (temp_flag >>  6) & 0x3f;       /* nicht benutzt -> 0.*/
  tcp->flags          = temp_flag & 0x3f;       /* URG,ACK,PSH,RST,SYN & FIN. */
  tcp->window         = get16(bp);    /* Groesse des Buffers fuer diese Verb. */
  tcp->checksum       = get16(bp);                           /* CRC-Checksum. */
  tcp->urgentPointer  = get16(bp);                    /* Fuer wichtige Daten. */

  if (tcp->flags == TSYN)                               /* Verbindungsaufbau. */
  {
    int DataLen = 0,
        options = 0,
        i       = 0;
                                                     /* Gesamtlaenge ermitteln */
    DataLen = ((tcp->data_offset * 4) + (ip->ihl * 4));/* (TCP-Header + Daten).*/
    options = (DataLen - TCP_HEADER);               /* Optionslaenge ermitteln */
                                              /* (Data - TCP-Header abziehen). */

    options = options / 2;

    for (i = 0; i < options; i++)
      tcp->options[i] = (char)get16(bp);                   /* Optionen setzen. */
  }
}

/* Entsorge alte TX-Segmente. */
void DelSegment(register int i,
                TCP         *tcp)
{
  STACKTX *TSeq;

  for (TSeq  = (STACKTX *)txSegment.head;     /* Durchsuche alle TX-Segmente. */
       TSeq != (STACKTX *)&txSegment;
       TSeq  = TSeq->next)
  {
    if (TSeq->Sock != sockets[i].Socket)                  /* Socketvergleich. */
      continue;                                     /* Zum naechsten Segment. */

    if (sockets[i].SendNext == tcp->acknum)             /* Frame uebertragen. */
    {
      --sockets[i].PacNum;                              /* Ein Frame weniger. */

      sockets[i].SendEvent = TRUE;   /* Select signalisieren - weiter senden. */

      if (TSeq->Data != NULL)                      /* Nur wenn es Daten gibt. */
        dealmb(TSeq->Data);                       /* Buffer leeren/entsorgen. */

      dealoc((MBHEAD *)ulink((LEHEAD *)TSeq));       /* TX Segment entsorgen. */
      return;
    }
    else                            /* Frame konnte nicht uebertragen werden. */
      {
        if (sockets[i].TState != TCP_RSTSENT)
          sockets[i].SendEvent = FALSE;       /* Select signalisieren - keine */
      }                                       /* weiteren Frame senden.       */
  }
}

/* Daten auslesen und fuer Funktion Recv bereitstellen. */
BOOLEAN ReadBuffer(register IP  *ip,
                   register TCP *tcp,
                   register int  i,
                   MBHEAD       *bp)
{
  DATENRX     *Drx;
  unsigned int DataLen = 0;

                              /* Gesamtlaenge ermitteln (TCP-Header + Daten). */
  DataLen = (ip->length) - ((tcp->data_offset * 4) + (ip->ihl * 4));

  if (DataLen >= TCP_HEADER)
    DataLen -= TCP_HEADER;                              /* TCP-Header abziehen. */
  else
    DataLen = 0;

  if (DataLen)                                              /* Es gibt Daten. */
  {
    if ((Drx = (DATENRX *)allocb(ALLOC_TCPSTACK)) == NULL) /* Besorge Buffer. */
      return(TRUE);                                     /* Speicher ist voll. */
                                                           /* Besorge Buffer. */
    if ((Drx->Data = (MBHEAD *)allocb(ALLOC_TCPSTACK)) == NULL)
    {
      dealoc((MBHEAD *)ulink((LEHEAD *)Drx));            /* Buffer entsorgen. */
      return(TRUE);                                     /* Speicher ist voll. */
    }

    Drx->Sock = sockets[i].Socket;                          /* Socket setzen. */
    sockets[i].RecvEvent = TRUE;                    /* Select Siginalisieren, */
                                                  /* wir haben was empfangen. */

    sockets[i].RecvNext += DataLen;       /* Daten an den Seq-Wert anhaengen. */

    while (DataLen--)                           /* Daten in Buffer schreiben. */
      putchr(getchr(bp), Drx->Data);

    rwndmb(Drx->Data);                               /* Buffer zurueckspulen. */
    relink((LEHEAD *) Drx, (LEHEAD *)rxDaten.tail);    /* RX-Liste umhaengen. */
  }

  return(FALSE);                                         /* Frame angenommen. */
}

/* Eingehende RX-Segment analysieren. */
int ReadRXStack(TCP *tcp, IP *ip, MBHEAD *bp)
{
  register int i;

  for (i = 1; i < NUM_SOCKETS; ++i)            /* Durchsuche die Socketliste. */
  {
    switch(sockets[i].State)                                /* Socket status. */
    {
      case TCP_LISTEN:                                       /* LISTEN-Modus. */
        if (tcp->flags != TSYN)        /* Nur annehmen, wenn neue Verbindung. */
          continue;                       /* Ansonsten zum naechsten Eintrag. */

        if (sockets[i].LocalPort != tcp->dstPort)           /* Portvergleich. */
          continue;                                 /* Zum naechsten Eintrag. */

        if (tcp->flags == TSYN)
        {
          if (sockets[i].Listen++ >= sockets[i].MaxListen)/* in die Warteschlange. */
            continue;         /* Warteschlange ist voll, zum naechsten eintrag. */
        }

                                                /* Verbindungsaufbau starten. */
        sockets[i].IpDest      = ip->source;/* Empfaenger, IP-Adresse setzen. */
        sockets[i].tos         = ip->tos;          /* Type of Service setzen. */
        sockets[i].RecvEvent   = TRUE;            /* Select signalisieren -   */
                                                  /* wir haben was empfangen. */
        sockets[i].DestPort    = tcp->srcPort;       /* Port-Nummer vom Ziel. */
        sockets[i].UrgPointer  = tcp->urgentPointer;       /* Flags einlesen. */

        sockets[i].RecvNext    = tcp->seqnum + 1;
        sockets[i].SendUnacked = 1;

        sockets[i].SendNext    = sockets[i].SendUnacked + 1;

       return(i);                                       /* Socket uebergeben. */

      case TCP_CONNECTED:                                 /* CONNECTED-Modus. */
        if (sockets[i].DestPort != tcp->srcPort)        /* Destportvergleich! */
          continue;

        return(i);                                 /* Unser gesuchter Socket. */
    }
  }

  return(EOF);                                               /* Nix gefunden. */
}

/* Verbindung nicht annehmen/zuruecksetzen. */
void SendRST(IP *ip, TCP *tcp)
{
  register int NewSock;

                                                     /* Neuen Socket anlegen. */
  if ((NewSock = Socket(AF_INET, SOCK_STREAM, 0)) == EOF)
    return;                       /* Neuer Socket kann nicht erstellt werden. */

  sockets[NewSock].IpDest     = ip->source;
  sockets[NewSock].LocalPort  = tcp->dstPort;
  sockets[NewSock].DestPort   = tcp->srcPort;
  sockets[NewSock].SendUnacked= tcp->acknum;
  sockets[NewSock].RecvNext   = tcp->seqnum + 1;

  SendTcpFlag(NewSock,                       /* Frame an den Nachbarn senden. */
              NULL,
              0,
              (TACK + TRST),
              0,
              0);

  DelSock(NewSock);                                      /* Socket entsorgen. */
}

/* ACK-Statusaendernungen. */
void StateACK(register int i, TCP *tcp)
{
  if (  (sockets[i].SendUnacked != tcp->acknum)   /* Noch nicht aktualisiert, */
      &&(tcp->window  != 0))             /* wenn noch was ins Fenster passt!. */
   sockets[i].SendUnacked = tcp->acknum;                /* ACK-Aktualisieren. */

  switch(sockets[i].TState)                              /* Aktueller Status. */
  {
                                                        /* Status schliessen: */
   case TCP_PSHWAITFIN :                 /* Sende alle offenstehende Packete. */
     if (tcp->window == 0)         /* Nachbar kann keine Daten mehr annehmen. */
       sockets[i].SendEvent = FALSE; /* Select signalisieren, keine weiteren Frame senden */
     else                                     /* Nachbar kann daten annehmen. */
       {
         if (sockets[i].PacNum)                        /* Es gibt noch Frame. */
         {                                        /* Neuer Status:            */
           sockets[i].TState = TCP_PSHSENTFIN;    /* Naechstes Packet senden. */
           sockets[i].SendEvent = TRUE;/* Select signalisieren, weiter senden.*/
         }
         else                                    /* Neuer Status:             */
           sockets[i].TState = TCP_FINSENT;      /* Socket sofort schliessen. */
       }
     break;

   case TCP_ACKWAIT :           /* Status Stabil: Warte auf ACK-Bestaetigung. */
     if (tcp->window == 0)        /* Nachbar kann keine Daten mehr annehmen . */
       sockets[i].SendEvent = FALSE;/* Select signalisieren, keine weiteren Frame senden. */
     else                                     /* Nachbar kann daten annehmen. */
       {                                                /* Neuer Status:      */
         sockets[i].TState = TCP_ESTABLISHED;           /* Verbindung stabil. */
         sockets[i].SendEvent = TRUE;               /* Select Signalisieren,  */
       }                                            /* weitere Frames senden. */
    break;

                                         /* Status schliessen:                */
   case TCP_ACKWAITFIN :                 /* Warte auf ACK, danach FIN senden. */
     sockets[i].TState = TCP_FINSENT;            /* Neuer Status:             */
                                                 /* Socket sofort schliessen. */
     sockets[i].SendEvent = TRUE;/* Select Signalisieren, weitere Frames senden. */
    break;

   case TCP_FINACKWAIT :    /* Status schliessen: Warte auf ACK-Bestaetigung. */
     sockets[i].TState = TCP_FINWAIT;    /* Neuer Status:                     */
                                         /* Warte auf FIN & ACK-Bestaetigung. */
     sockets[i].SendEvent = TRUE;/* Select Signalisieren, weitere Frames senden. */
    break;

                             /* Status schliessen:                            */
   case TCP_ACKWAITCLOSED :  /* Warte auf ACK-Bestaetigung, Socket entfernen. */
     sockets[i].TState = TCP_CLOSED;                      /* Entsorge Socket. */
    break;

  }
}

/* RX-Segmente analysieren. */
static void StackRX(void)
{
  STACKRX     *RSeq;
  register int i;
  static TCP   tcp;

  while ((RSeq = (STACKRX *)rxSegment.head) != (STACKRX *)&rxSegment)
  {
    ReadTcpHeader(&tcp, RSeq->IpHdr, RSeq->Data);        /* TCP-Header lesen. */

                                        /* Eingehende RX-Segment analysieren. */
    if ((i = ReadRXStack(&tcp, RSeq->IpHdr, RSeq->Data)) != EOF)
    {
      ReadBuffer(RSeq->IpHdr, &tcp, i, RSeq->Data);
      switch (tcp.flags)
      {
        case (TACK + TFIN) :          /* Nachbar will sofort getrennt werden. */
          sockets[i].RecvEvent = TRUE; /* Select signalisieren, wir haben was empfangen. */
          sockets[i].SendEvent = TRUE; /* Select signalisieren, weitere Frames senden. */

          if (  (sockets[i].TState == TCP_ESTABLISHED)  /* Verbindung stabil. */
              ||(sockets[i].TState == TCP_ACKWAIT)
              ||(sockets[i].TState == TCP_PSHSENTFIN))
          {
            sockets[i].RecvNext += 1;    /* Bei Bestaetigung um ein erhoehen. */
            SendTcpFlag(i,                  /* ACK & FIN mit ACK-bestaetigen. */
                        NULL,
                        0,
                        TACK,
                        WINSIZE,
                        0);

                                                  /* Neuer Status: ACK & FIN  */
            sockets[i].TState = TCP_FINSENTCLOSED;/* an den Nachbarn schicken.*/
          }

          if (sockets[i].TState == TCP_FINWAIT) /* FIN & ACK-Bestaetigung erhalten. */
          {
            sockets[i].RecvNext += 1;           /* Seq-Wert um eins erhoehen. */
            SendTcpFlag(i,                        /* ACK-Bestaetigung senden. */
                        NULL,
                        0,
                        TACK,
                        WINSIZE,
                        0);

            sockets[i].TState = TCP_CLOSED;/* Socket kann geschlossen werden. */
          }
         break;

        case (TACK + TPSH + TFIN) :   /* Nachbar will sofort getrennt werden. */
          sockets[i].RecvEvent = TRUE;/* Select signalisieren, wir haben was empfangen. */
          sockets[i].SendEvent = TRUE;/* Select signalisieren, weitere Frames senden. */

          SendTcpFlag(i,                         /* Verbindung zuruecksetzen. */
                      NULL,
                      0,
                      TRST,
                      WINSIZE,
                      0);

          sockets[i].TState = TCP_CLOSED;  /* Neuer Status: Socket entsorgen. */
         break;

        case (TACK) :                                    /* ACK-Bestaetigung. */
          StateACK(i, &tcp);
         break;


          case (TACK + TPSH) :                    /* Es sind Daten im Header. */
            if (sockets[i].SendUnacked != tcp.acknum)
              sockets[i].SendUnacked = tcp.acknum;

            sockets[i].SendEvent = TRUE;
            SendTcpFlag(i,                        /* ACK-Bestaetigung senden. */
                        NULL,
                        0,
                        TACK,
                        WINSIZE,
                        0);
                                                        /* Neuer Status:      */
            sockets[i].TState = TCP_ESTABLISHED;        /* Verbindung stabil. */
           break;


          case (TRST) :                          /* Verbindung zuruecksetzen. */
            sockets[i].TState = TCP_RSTSENT;              /* Socket entsorgen. */
            sockets[i].SendEvent = TRUE;
            DelSock(i);
           break;
        }

      DelSegment(i, &tcp);
    }
    else
      SendRST(RSeq->IpHdr, &tcp); /* Verbindung nicht annehmen/zuruecksetzen. */

    dealmb(RSeq->Data);                                  /* Buffer entsorgen. */
    dealoc((MBHEAD *)ulink((LEHEAD *)RSeq));         /* RX-Segment entsorgen. */
  }
}

/* Je nach Status reagieren. */
static void StackState(void)
{
  register int i;

  for (i = 1; i < NUM_SOCKETS; ++i)                /* durchsuche alle Socket. */
  {                                                      /* Socket mit Status */
    if (  (sockets[i].State == TCP_CLOSED)                  /* Socket unbenutzt */
        ||(sockets[i].State == TCP_LISTEN)                          /* Listen */
        ||(sockets[i].TState == TCP_ESTABLISHED)    /* Stabile Verbindung und */
        ||(sockets[i].TState == TCP_PSHSENT)) /* Frame senden nicht beachten. */
      continue;                                      /* Zum naechsten Socket. */

    switch(sockets[i].TState)                              /*  Socket-Status. */
    {
      case TCP_SYNCON:                      /* Verbindungsaufbau bestaetigen. */
        PutTXStack(i,                                     /* SYN & ACK senden.*/
                   NULL,
                   0,
                   (TSYN + TACK),
                   TCP_SYNCON);
       sockets[i].TState = TCP_PSHSENT;
       continue;                                     /* Zum naechsten Socket. */


      case TCP_FINSENT:       /* Status schliessen: Socket sofort schliessen. */
        PutTXStack(i,                                    /* FIN & ACK senden. */
                   NULL,
                   0,
                   (TACK + TFIN),
                   TCP_FINSENT);
        sockets[i].TState = TCP_PSHSENT;      /* Neuer Status: Frames senden. */
       continue;                                     /* Zum naechsten Socket. */

                                   /* Status schliessen:                      */
      case TCP_FINSENTCLOSED :     /* Nach ACK-Bestaetigung, FIN & ACK-Senden.*/
        PutTXStack(i,                                    /* FIN & ACK senden. */
                   NULL,
                   0,
                   (TACK + TFIN),
                   TCP_FINSENTCLOSED);
        sockets[i].TState = TCP_PSHSENT;      /* Neuer Status: Frames senden. */
       continue;                                     /* Zum naechsten Socket. */

                                                   /* Status schliessen:      */
      case TCP_PSHSENTFIN :                        /* Naechstes Frame senden. */
        if (!sockets[i].PacNum)                       /* Alle Frame gesendet. */
          sockets[i].TState = TCP_FINSENT; /* Neuer Status: FIN & ACK senden. */
       continue;                                     /* Zum naechsten Socket. */


      case TCP_RSTSENT :      /* Status schliessen: Verbindung zuruecksetzen. */
        PutTXStack(i,                           /* Frame mit Flag RST senden. */
                   NULL,
                   0,
                   TRST,
                   TCP_RSTSENT);
        sockets[i].TState = TCP_PSHSENT;      /* Neuer Status: Frames senden. */
       continue;                                     /* Zum naechsten Socket. */


      case TCP_CLOSED :                           /* Socket leeren/entsorgen. */
        DelSock(i);
       return;
    }
  }
}

/* Frame senden. */
static void StackTX(void)
{
  STACKTX     *TSeq;
  register int i;
  time_t       now;

  for (TSeq  = (STACKTX *)txSegment.head;          /* Durchsuche alle Socket. */
       TSeq != (STACKTX *)&txSegment;
       TSeq  = TSeq->next)
  {
    if ((i = SearchSock(TSeq->Sock)) == EOF)             /* Socket ermitteln. */
    {                               /* Socket wurde mittlerweile geschlossen. */
      if (TSeq->Data != NULL)                               /* Gibt es Daten. */
        dealmb((MBHEAD *)TSeq->Data);             /* Buffer leeren/entsorgen. */

      dealoc((MBHEAD *)ulink((LEHEAD *)TSeq));       /* TX Segment entsorgen. */
      return;
    }

    if (TSeq->Timer)                                         /* Timer laeuft. */
    {
      now = time(NULL);

      if (TSeq->TimeLast < now)
      {
        TSeq->TimeLast = time(NULL);

        if (TSeq->Timer  < 20)
        {
          ++TSeq->Timer;
          continue;
        }

        /* Max. versuche ueberschritten. */
        if (TSeq->Retry >= TCP_MAX_RETRY)
        {
          /* Socket soll geschlossen werden. */
          if (sockets[i].TState == TCP_CLOSE)
          {
            /* Socket schliessen. */
            sockets[i].TState = TCP_CLOSED;
            /* zum naechsten Frame. */
            continue;
          }

          /* Socket soll geschlossen werden. */
          sockets[i].TState = TCP_CLOSE;
          /* Select signalisieren. */
          sockets[i].RecvEvent = TRUE;
          sockets[i].SendEvent = TRUE;
          continue;
        }

        ++TSeq->Retry;
        TSeq->Timer = 0;

        if (TSeq->Data != NULL)
          rwndmb(TSeq->Data);

        sockets[i].TState = TCP_PSHSENT;
      }
    }

    if (  (sockets[i].TState == TCP_PSHSENT)  /* Frame an den Nachbarn senden */
        ||(sockets[i].TState == TCP_ESTABLISHED)   /* wenn Verbindung stabil. */
        ||(sockets[i].TState == TCP_PSHSENTFIN))  /* Socket soll geschloessen */
    {                                   /* werden, vorher alle Frames senden. */
      SendTcpFlag(i,                                         /* Frame senden. */
                  TSeq->Data,
                  (TSeq->Data == NULL ? 0 : TSeq->Data->mbpc - TSeq->Data->mbgc),
                  TSeq->Flags,
                  WINSIZE,
                  (unsigned short)(TSeq->Flags == (TSYN + TACK) ? TCP_OPTION : FALSE));

     if ((unsigned short)(TSeq->Flags == (TSYN + TACK)))
     {
       sockets[i].TState = TCP_ACKWAIT;     /* Warte auf ACK-Bestaetigung. */
       ++TSeq->Timer;                                    /* Timer starten. */
       continue;                                    /* Zum naechsten Frame. */
     }

      switch(TSeq->TState)                                  /* Statusaendung. */
      {
        case TCP_ESTABLISHED                 : /* Status: Stabile Verbindung. */
        case TCP_PSHSENT :                           /* Status: Frame senden. */
          if (sockets[i].TState == TCP_PSHSENTFIN)
          {
            sockets[i].TState = TCP_PSHWAITFIN;/* offenstehende Frames senden */
            ++TSeq->Timer;                                  /* Timer starten. */
            continue;                                 /* Zum naechsten Frame. */
          }

                                                 /* Neuer Status:               */
          sockets[i].TState = TCP_ACKWAIT;     /* Warte auf ACK-Bestaetigung. */
          ++TSeq->Timer;                                    /* Timer starten. */
          continue;                                    /* Zum naechsten Frame. */


        case TCP_PSHSENTFIN :  /* Status schliessen: Naechstes Packet senden. */
          sockets[i].TState = TCP_PSHWAITFIN;/* Status: Warte auf ACK-Meldung.*/
          ++TSeq->Timer;                                    /* Timer starten. */
          continue;                                    /* Zum naechsten Frame. */


        case TCP_FINSENT :/* Status schliessen: Socket soll geschlossen werden*/
          sockets[i].TState = TCP_FINACKWAIT; /* Neuer Status: Warte auf ACK. */
          ++TSeq->Timer;                                    /* Timer starten. */
          continue;                                    /* Zum naechsten Frame. */

                                     /* Status schliessen:                      */
        case TCP_FINSENTCLOSED :  /* Nach ACK-Bestaetigung, FIN & ACK-Senden.*/
          sockets[i].TState = TCP_ACKWAITCLOSED;/* Neuer Status: Warte auf ACK*/
          ++TSeq->Timer;                                    /* Timer starten. */
          continue;                                    /* Zum naechsten Frame. */


        case TCP_RSTSENT :                       /* Verbindung zuruecksetzen. */
          sockets[i].TState = TCP_CLOSED; /* Neuer Status: .Socket schliessen.*/
          ++TSeq->Timer;                                    /* Timer starten. */
          continue;                                    /* Zum naechsten Frame. */


        default:                               /* Fuer alle restlichen Status. */
          continue;                                    /* Zum naechsten Frame. */
      }
    }
  }
}


/* TCP-Header zusammenbasteln. */
MBHEAD *
htontcp(register int   i,                                /* Aktueller Socket. */
        MBHEAD        *Data,                             /* evl. Daten.       */
        unsigned short flags,                            /* TCP-Flags.        */
        unsigned short tcp_len,                          /* Headerlaenge.     */
        unsigned short WinSize,                          /* Fenstergroesse.   */
        BOOLEAN        options)                          /* evl. Optionen.    */
{
  register MBHEAD    *bp;
  PSEUDO_HEADER       pseudo_hdr;
  unsigned short      checksum;
  WORD                DataMbpc;                            /* Sicherung mbpc  */
  WORD                DataMbgc;                            /* Sicherung mbgc. */
  int                 SaveLen;

  bp = (MBHEAD *)allocb(ALLOC_MBHEAD);                    /* Buffer besorgen. */

  put16((unsigned short)sockets[i].LocalPort, bp);       /* Dest-Port´setzen. */
  put16((unsigned short)sockets[i].DestPort, bp);      /* Source-Port setzen. */

  put32((unsigned long)sockets[i].SendUnacked, bp);     /* Ack-Nummer setzen. */
  put32((unsigned long)sockets[i].RecvNext, bp);        /* Seq-Nummer setzen. */

  put16((unsigned short)flags, bp);  /* TCP-Flags setzen. (ACK, FIN, SYN...). */
  put16((unsigned short)WinSize, bp);               /* Fenstergroesse setzen. */
  put16((unsigned short)0, bp);             /* Checksum ERSTMAL auf 0 setzen. */
  put16((unsigned short)sockets[i].UrgPointer, bp);    /* Urgent-Flag setzen. */

  if (options)                   /* Nur wenn Optionen gewuenscht wird setzen. */
  {
    put16((unsigned short)OPTEND, bp);               /* End of Option List.   */
    put16((unsigned short)OPTMSS, bp);               /* Maximum Segment Size. */
    put16((unsigned short)OPTNOO, bp);               /* No-Operation.         */
    put16((unsigned short)OPTPA3, bp);
  }
  else                                              /* Keine Optionen setzen. */
    {
      if (tcp_len > TCP_HEADER)                             /* Es gibt daten. */
      {
        int i = 0;

        SaveLen = i = Data->mbpc - Data->mbgc;           /* Laenge ermitteln. */

        DataMbpc = Data->mbpc;                              /* Werte sichern. */
        DataMbgc = Data->mbgc;

        while (i--)                              /* Alle zeichen einlesen und */
          putchr(getchr(Data), bp);                   /* in Buffer schreiben. */

        rwndmb(Data);                                /* Buffer zurueckspulen. */
      }
    }

  rwndmb(bp);                                         /* Frame zurueckspulen. */

  /************************  Pseudo-Header ermitteln. *************************/
  pseudo_hdr.source   = my_ip_addr;                     /* Unsere IP-Adresse. */
  pseudo_hdr.dest     = sockets[i].IpDest;        /* IP-Adresse vom Nachbarn. */
  pseudo_hdr.protocol = TCP_PTCL;                           /* Protokoll-Typ. */
  pseudo_hdr.length   = tcp_len;                       /* TCP-Header + Daten. */
  checksum = cksum(&pseudo_hdr, bp, (unsigned short)tcp_len);/* Checksume ermitteln. */

  dealmb(bp);                                       /* Alten buffer loeschen. */

  bp = (MBHEAD *)allocb(ALLOC_MBHEAD);              /* Neuen buffer besorgen. */

  put16((unsigned short)sockets[i].LocalPort, bp);       /* Dest-Port´setzen. */
  put16((unsigned short)sockets[i].DestPort, bp);      /* Source-Port setzen. */

  put32((unsigned long)sockets[i].SendUnacked, bp);     /* Ack-Nummer setzen. */
  put32((unsigned long)sockets[i].RecvNext, bp);        /* Seq-Nummer setzen. */
  put16((unsigned short)flags, bp);   /* TCP-Flags setzen. (ACK, FIN, SYN...) */
  put16((unsigned short)WinSize, bp);               /* Fenstergroesse setzen. */
  put16((unsigned short)checksum, bp);                    /* Checksum setzen. */
  put16((unsigned short)sockets[i].UrgPointer, bp);    /* Urgent-Flag setzen. */

  if (options)                   /* Nur wenn Optionen gewuenscht wird setzen. */
  {
    put16((unsigned short)OPTEND, bp);               /* End of Option List.   */
    put16((unsigned short)OPTMSS, bp);               /* Maximum Segment Size. */
    put16((unsigned short)OPTNOO, bp);               /* No-Operation.         */
    put16((unsigned short)OPTPA3, bp);
  }
  else                                              /* Keine Optionen setzen. */
    {
      if (tcp_len > TCP_HEADER)                             /* Es gibt daten. */
      {
        int i = 0;

        i = Data->mbpc - SaveLen;                        /* Laenge ermitteln. */

        if (i != Data->mbpc)
        {
          while (i--)
            getchr(Data);
        }

        while (SaveLen--)                        /* Alle zeichen einlesen und */
          putchr(getchr(Data), bp);                   /* in buffer schreiben. */
      }
    }

  rwndmb(bp);                                         /* Daten zurueckspulen. */
  return (bp);
}

/* Frame fuer IP-Router vorbereiten. */
void
SendTcpFlag(register int   i,                        /* Aktueller Socket.     */
            MBHEAD        *Data,                     /* Reine Daten.          */
            unsigned int   DataLen,                  /* Bufferlaenge.         */
            unsigned short Flag,                     /* TCP-Flags.            */
            unsigned short WinSize,                  /* Fenstergroesse setzen.*/
            unsigned short Options)                  /* Optionen setzen.      */
{
  MBHEAD        *bp;
  unsigned short TcpLen;
  unsigned short TcpFlags;

                                        /* TCP-Headerlaenge + evl. Optionen + */
  TcpLen    = TCP_HEADER + Options + DataLen;       /* Reine Daten ermitteln. */

  if (Options)                   /* Nur wenn Optionen gewuenscht wird setzen. */
  {
    TcpFlags  = (TCP_HEADER + Options) << 10; /* TCP-Header + Optionen setzen.*/
    Options = TRUE;                                       /* Optionen setzen. */
  }
  else                                                     /* Keine Optionen. */
    {
     TcpFlags  = TCP_HEADER << 10;
     Options = FALSE;                                      /* Keine Optionen. */
    }

  TcpFlags |= Flag;

  bp = htontcp(i,                       /* TCP-Header + Daten zusammenfuegen. */
               Data,
               TcpFlags,
               TcpLen,
               WinSize,
               Options);

  ip_send(my_ip_addr,           /* IP-Header basteln und ab in den IP-Router. */
          sockets[i].IpDest,
          TCP_PTCL,
          sockets[i].tos,
          0,
          bp,
          0,
          0,
          0);
}

#endif /* TCPSTACK */

/* End of src/l3tcp.c. */
