#ifdef TCP_STACK

#define TCP_SOCKET          1             /* Status: Socket erstellen         */
#define TCP_BIND            2             /* Status: Socket binden.           */
#define TCP_LISTEN          3             /* Status: Socket auf Listen setzen.*/
#define TCP_CONNECTED       4             /* Status: Es gibt eine Verbindung. */

#define TCP_CLOSED          0      /* Socket leeren/entsorgen.                */
#define TCP_SYNCON          1      /* Verbindungsaufbau bestaetigen.          */
#define TCP_ESTABLISHED     2      /* Status Stabil        .                  */
#define TCP_ACKSENT         3      /* ACK-Bestaetigung senden.                */
#define TCP_PSHSENT         4      /* Frame an den Nachbarn schicken.         */
#define TCP_ACKWAIT         5      /* Warte auf ACK-Bestaetigung.             */
#define TCP_CLOSE           6      /* Status schliessen. (7 bis 15)           */
#define TCP_FINSENT         7      /* Socket sofort schliessen.               */
#define TCP_FINWAIT         8      /* Warte auf FIN & ACK-Bestaetigung.       */
#define TCP_FINACKWAIT      9      /* Warte auf ACK-Bestaetigung.             */
#define TCP_ACKWAITFIN     10      /* Warte auf ACK, danach FIN senden.       */
#define TCP_PSHWAITFIN     11      /* Sende alle offenstehende Packete.       */
#define TCP_PSHSENTFIN     12      /* Naechstes Packet senden.                */
#define TCP_FINSENTCLOSED  13      /* Nach ACK-Bestaetigung, FIN & ACK-Senden.*/
#define TCP_ACKWAITCLOSED  14      /* Warte auf ACK-Bestaetigung, Socket entf.*/
#define TCP_RSTSENT        15      /* Verbindung zuruecksetzen.               */

#ifndef SOCK_STREAM
#define SOCK_STREAM         1                                /* stream socket */
#endif

#ifndef AF_INET
#define AF_INET             2                 /* internetwork: UDP, TCP, etc. */
#endif

#ifndef INADDR_ANY
#define INADDR_ANY          (ULONG)0x00000000
#endif

#define NUM_SOCKETS         32                               /* Max. Socket's */

typedef unsigned int Socklen_t;

typedef struct _fd_set
{
  unsigned int fd_count;                               /* how many are SET?   */
  Socklen_t    fd_array[FD_SETSIZE];                   /* an array of SOCKETs */
} Fd_set;

#define FD_SET_T(fd, set) do { \
    Socklen_t __i; \
    for (__i = 0; __i < ((Fd_set *)(set))->fd_count; __i++) { \
        if (((Fd_set *)(set))->fd_array[__i] == (fd)) { \
            break; \
        } \
    } \
    if (__i == ((Fd_set *)(set))->fd_count) { \
        if (((Fd_set *)(set))->fd_count < FD_SETSIZE) { \
            ((Fd_set *)(set))->fd_array[__i] = (fd); \
            ((Fd_set *)(set))->fd_count++; \
        } \
    } \
} while(0)

#define FD_ZERO_T(set) (((Fd_set *)(set))->fd_count=0)

extern BOOLEAN  GetSocket(Socklen_t, Fd_set *);
#define FD_ISSET_T(fd, set) GetSocket((Socklen_t)(fd), (Fd_set *)(set))

struct In_addr {
        union {
                struct { UBYTE s_b1,s_b2,s_b3,s_b4; } S_un_b;
                struct { UWORD s_w1,s_w2; } S_un_w;
                ULONG _S_addr;
        } _S_un;
#define _S_addr  _S_un._S_addr
                                        /* can be used for most tcp & ip code */
};

struct Sockaddr_in
{
  short   sin_family;
  UWORD   sin_port;
  struct  In_addr sin_addr;
  char    sin_zero[8];
};

struct Sockaddr
{
  UWORD   sa_family;                                        /* address family */
  char    sa_data[14];                    /* up to 14 bytes of direct address */
};

struct Timeval
{
  long    tv_sec;                                                  /* seconds */
  long    tv_usec;                                        /* and microseconds */
};

typedef struct tsocket                 /* Struktur Socketliste                */
{
  struct tsocket *next;
  struct tsocket *prev;
#ifdef BUFFER_DEBUG
  UBYTE           owner;               /* Muss an 9. Bytestelle stehen        */
#endif
  ACK             RecvNext;            /* Naechstes Frame Empfangen.          */
  SEQ             SendNext;            /* Naechstes Frame Senden.             */
  ACK             SendUnacked;         /* Noch nicht gesende Frames.          */
  unsigned long   IpDest;              /* IP-Adresse vom Nachbarn.            */
  int             Socket;              /* Socket.                             */
  int             Domain;              /* Domain.                             */
  int             Type;                /* Protokoll-Typ (TCP oder UDP).       */
  int             PacNum;              /* Frame-Zaehler.                      */
  /* TCP-Header. */
  unsigned        DestPort;            /* Destination-Port                    */
  unsigned        LocalPort;           /* Local-Port setzen.                  */
  unsigned short  MaxListen;           /* Groesse der Warteschlange.          */
  unsigned short  Listen;              /* Momentan in der Warteschlange.      */
  unsigned char   UrgPointer;          /* Urgent Pointer                      */
  UBYTE           State;               /* Socket-Status.                      */
  UBYTE           TState;              /* Socket-Status.                      */
  UBYTE           tos;                 /* Type of service                     */
  BOOLEAN         RecvEvent;           /* Select markieren, Empfang.          */
  BOOLEAN         SendEvent;           /* Select markieren, Senden.           */
} TSOCKET;

extern TSOCKET sockets[NUM_SOCKETS];

extern int      Socket(int,                            /* Ein socket anlegen. */
                       int,
                       int);

extern int      Bind  (int,                                 /* Socket binden. */
                       struct Sockaddr *,
                       Socklen_t);

extern int      Listen(int,                            /* Lausche auf Socket. */
                       int);

extern int      Select(int,             /* Den Socket auf Aktivitaet pruefen. */
                       Fd_set         *,
                       Fd_set         *,
                       Fd_set         *,
                       struct Timeval *);

extern int      Accept(int ,             /* Verbindungsbau annehmen/ablehnen. */
                       struct Sockaddr *,
                       Socklen_t       *);

extern int      Recv  (int ,                              /* Daten Empfangen. */
                       char *,
                       int ,
                       int);

extern int      Send  (int ,                                 /* Daten Senden. */
                       char *,
                       int,
                       int);

extern void     Close (int);                            /* Socket schliessen. */


extern   int    SearchSock(int);    /* Den Socket aus der Socketliste suchen. */
extern  void    DelSocket(int);            /* Socket als unbenutzt markieren. */

/* konvertiert die Kurzganzzahl hostshort Rechner- nach Netzwerk-Byteordnung. */
extern unsigned short Htons(unsigned short);
/* konvertiert die Kurzganzzahl netshort von Netzwerk-nach Rechner-Byteordnung*/
extern unsigned short Ntohs(unsigned short);
/* konvertiert die Langganzzahl hostlong von Rechner-nach Netzwerk-Byteordnung*/
extern unsigned long  Htonl(unsigned long);
/* konvertiert die Langganzzahl netlong von Netzwerk- nach Rechner-Byteordnung*/
extern unsigned long  Ntohl(unsigned long);

#endif /* TCP_STACK. */

/* End of include/l3sock.h. */
