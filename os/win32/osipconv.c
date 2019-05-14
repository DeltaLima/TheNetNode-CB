
#include "tnn.h"
#ifdef OS_IPLINK

/* Zeiger auf das Aktuelle Interface. */
static T_INTERFACE *ifpp;


/* Struktur (sockaddr_in) fuer Server definieren. */
struct sockaddr_in myaddr_in;
/* Struktur (sockaddr_in) fuer client definieren. */
struct sockaddr_in peeraddr_in;


/* Ein Connect zum TCPIP-Nachbarn aufbauen. */
int IPConvConnectOS(char *call, char *upcall, BOOLEAN flag)
{
  SENDTX         *STx = NULL;
  int             fd = EOF;
  int             ch = FALSE;
  register int    i  = EOF;
  struct hostent *h;
  long            addr;
  char            tmp[10];
#ifdef DEBUG_MODUS
  lastfunc = "osipconv(IPConvConnectOS)";
#endif /* DEBUG_MODUS */

  ifpp = &ifp[CVS_ID];                    /* Zeiger auf das IPCONV-Interface. */

  /* IPConvers aktiv? */
  if (ifpp->actively == FALSE)
  {
    /* ggf. Logbuch fuehren. */
    T_LOGL1(TRUE, "(IPConvConnectOS):\nKein IPConvers-Port geoeffnet!\n");
    /* Nein, abbrechen. */
    return(TRUE);
  }

  /* Suche Rufzeichen in der IPC-TBL. */
  if ((i = IPConvSearch(call)) == EOF)
  {
    /* ggf. Logbuch fuehren. */
    T_LOGL1(TRUE, "(IPConvConnectOS):\nRufzeichen (%s) wurde nicht gefunden!\n"
                  , call);
    /* Nix gefunden, abbrechen. */
    return(TRUE);
  }

  /* Konvertiere Rufzeichen OHNE SSID!. */
  callss2str(tmp, upcall);

  /* Konvertiere IP-Adresse. */
  if ( (addr = inet_addr((const char *)ip_tbl[i].hostname)) == EOF)
  {
    /* Hostname aufloesen. */
    if ((h = gethostbyname((char *)ip_tbl[i].hostname)) == FALSE)
    {
      /* ggf. Logbuch fuehren. */
      T_LOGL1(TRUE, "(IPConvConnectOS):\nIP-Adresse/Hostname (%s) konnte nicht aufgeloest werden!\n"
                    , ip_tbl[i].hostname);
      /* Hostname/IP-Adresse ungueltig, abbrechen. */
      return(TRUE);
    }

    /* IP-Adresse zuweisen. */
    addr = ((struct in_addr*)h->h_addr)->s_addr;
  }

  /* Adressstruktur loeschen */
  memset(&myaddr_in,  0, sizeof(myaddr_in));

  /* Adressstruktur fuellen */
  myaddr_in.sin_family = AF_INET;
  /* Auf tcp_port lauschen. */
  myaddr_in.sin_port = htons(ip_tbl[i].port);
  /* Jedes netzwerk-taugliche Geraet abfragen. */
  myaddr_in.sin_addr.s_addr = addr;

  /* Socket erstellen. */
  if ((fd = socket (AF_INET, SOCK_STREAM, 0)) == EOF)
  {
    /* ggf. Logbuch fuehren. */
    T_LOGL1(TRUE, "(IPConvConnectOS):%s\nNeuer Socket konnte nicht erstellt werden!\n"
                  , ip_tbl[i].hostname);
#ifdef SPEECH
    printf(speech_message(335));
#else
    printf("Error: Socket cannot be constructed!\r");
#endif
    return(TRUE);
  }

  /* Connnect Initialisieren. */
  if (connect (fd, (struct sockaddr *) &myaddr_in, sizeof (struct sockaddr_in)) == EOF)
  {
    /* ggf. Logbuch fuehren. */
    T_LOGL1(TRUE, "(IPConvConnectOS):\nConnect nicht möglich (IP-Addr: %s)!\n"
                  , ip_tbl[i].hostname);

    /* Schliesse Socket. */
    close(fd);
    return(TRUE);
  }

  T_LOGL2(TRUE, "(IPConvConnectOS):\nMit IP-Adr./Hostname %s verbunden.\n"
                , ip_tbl[i].hostname);

  /* Neues TCPIP-Segment anlegen. */
  if (AddUserOS(ifpp, fd, (char *)ip_tbl[i].hostname))
  {
    /* Schliesse Socket. */
    close(fd);
    return(TRUE);
  }

  /* Markiere Route als Linkpartner. */
  tcppoi->CVSlink  = TRUE;
  /* Markiere das wir eingeloggt sind. */
  tcppoi->login    = TRUE;
  /* Markiere, kein prompt senden. */
  tcppoi->LoginPrompt   = TRUE;

  /* Maximalwert fuer die Statistik   */
  if (++nmbtcp > nmbtcp_max)
    nmbtcp_max = nmbtcp;

  /* Mit channel angabe? */
  if (clicnt)
    /* Hole channel. */
    ch = atoi(clipoi);

  /* Wenn Nachbar STATUS "User" hat, gleich durch connecten. */
  if (  (!flag)
      &&(ip_tbl[i].linkflag == TRUE))
  {
    /* ggf. Logbuch fuehren. */
    T_LOGL2(TRUE, "(IPConvConnectOS):\nVerbindung zur IP-Adresse %s hergestellt.\n"
                , ip_tbl[i].hostname);

    return(FALSE);
  }

  if ((STx = (SENDTX *)allocb(ALLOC_L1IPCONV)) == NULL)/* TX-Segment besorgen. */
  {
    /* ggf. Logbuch fuehren. */
    T_LOGL1(TRUE, "(IPConvConnectOS):%s\nSpeicher (%d) ist voll!\n"
                  , ip_tbl[i].hostname
                  , nmbfre);

    /* Schliesse Socket. */ 
    close(fd);

    tcppoi->disflg |= 0x80;               /* Segment auf Disconnect setzen. */
    return(TRUE);                              /* Kein Segment frei, abbruch. */
  }

  if ((STx->Data = SetBuffer()) == NULL)             /* Buffer besorgen.      */
  {
    /* ggf. Logbuch fuehren. */
    T_LOGL1(TRUE, "(IPConvConnectOS):%s\nSpeicher (%d) ist voll!\n"
                  , ip_tbl[i].hostname
                  , nmbfre);

    /* Schliesse Socket. */ 
    close(fd);

    dealoc((MBHEAD *)ulink((LEHEAD *)STx));           /* TX-Segment entsorgen.*/

    tcppoi->disflg |= 0x80;               /* Segment auf Disconnect setzen. */
    return(TRUE);                                /* Buffer ist voll, abbruch. */
  }

  STx->Sock      = tcppoi->sock;                      /* Socket setzen.       */
  STx->Interface = tcppoi->Interface;                 /* Interface setzen.    */
  STx->Mode      = tcppoi->mode;                      /* Stack-Mode setzen.   */

  /* Je nach Status von flag     */
  /* demenstsprechend reagieren. */
  if (flag)
    putprintf(STx->Data, "ppconvers/\377\200HOST %s %s %s\r", myhostname, myrev, myfeatures);
  else
    {
     /* Wenn Nachbar STATUS "User" hat, gleich durch connecten. */
     if (ip_tbl[i].linkflag == FALSE)
       /* Loginstring vorbereiten. */
       putprintf(STx->Data, "/na %s %d\r", tmp, ch);
    }

  rwndmb(STx->Data);
  /* Umhaengen in die sendesliste. */
  relink((LEHEAD *) STx, (LEHEAD *)rxflTX.tail);/* Umhaengen in die Sendeliste. */
  return(FALSE);
}

#endif /* OS_IPLINK */

/* End of os/win32/osipconv.c */
