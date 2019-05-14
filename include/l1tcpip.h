#ifdef L1TCPIP

#ifdef L1TELNET
#include "l1telnet.h"              /* Telnet-Interface.                       */
#endif /* L1TELNET */

#ifdef L1HTTPD
#include "l1httpd.h"               /* HTTPD-Interface.                        */
#endif /* L1HTTPD */

#ifdef L1IPCONV
#include "l1ipconv.h"              /* IPCONVERS-Interface.                    */
#endif /* L1IPCONV */

#ifdef L1IRC
#include "l1irc.h"                 /* IRC Chat-Server                         */
#endif /* L1IRC */

#define TXLEN      2048            /* Maximale TX-Packetlaenge                */
#define RXLEN      2048            /* Maximale.RX-Packetlaenge                */
#define CE_TCPIP     11            /* Fuer Meckermeldung.                     */
#define IPADDR       16            /* Max.-Laenge IP-Adresse.                 */
#define TCP_USER      8            /* TCPIP-User/Link.                        */
#define MAXTCPIP     32            /* Max: Anzahl der TCPIP-User/Link.        */
#define HNLEN        64            /* Maximale laenge eines Hostnamen.        */
#define T3PARA       60            /* T3-Timer abgelaufen, wuerd geprueft ob  */
                                   /* der Socket noch aktiv ist.              */
#define MAXINTERFACE  4            /* Maximale Anzahl aller TCPIP-Interface.  */

#define TCP_TX_FREE   0            /* Sender ist frei.                        */
#define TCP_TX_BUSY   1            /* Sender ist belegt.                      */

#define INT_STACK     1            /* Interner TCP-Stack.                     */

extern UWORD nmbtcp;               /* Anzahl aktiver TCPIP-Links.             */
extern UWORD nmbtcp_max;           /* Maximale anzahl der TCPIP-Links         */

typedef struct
{
  UWORD   Interface;               /* Nummer vom Interface.                   */
  BOOLEAN actively;                /* Interface aktiv/deaktiv.                */
  UWORD   tcpport;                 /* TCP-Port.                               */
  int     OsSock;                  /* OS-Socket.                              */
  int     ISock;                   /* Interner Socket.                        */
  int     l2port;                  /* L2-Port vom Interface.                  */
  int     log;                     /* Loglevel.                               */
  char    name[10 + 1];            /* Interfacename.                          */
} T_INTERFACE;

extern T_INTERFACE ifp[MAXINTERFACE];

typedef struct ReadRX              /* Empfangsbuffer.                         */
{
  struct ReadRX *next;             /* Naechster Listeneintrag                 */
  struct ReadRX *prev;             /* Vorheriger Listeneintra                 */
#ifdef BUFFER_DEBUG
  UBYTE          owner;            /* Muss an 9. Bytestelle stehen            */
#endif
  MBHEAD        *Data;             /* RX-Buffer.                              */
  int            Sock;
  UWORD          Interface;        /* TCPIP-Interface.                        */
  UWORD          Mode;             /* Stack-Mode.                             */
} READRX;

typedef struct SendTX              /* Sendebuffer.                            */
{
  struct SendTX *next;             /* Naechster Listeneintrag                 */
  struct SendTX *prev;             /* Vorheriger Listeneintra                 */
#ifdef BUFFER_DEBUG
  UBYTE          owner;            /* Muss an 9. Bytestelle stehen            */
#endif
  MBHEAD        *Data;             /* TX-Buffer.                              */
  int            Sock;
  UWORD          Interface;        /* TCPIP-Interface.                        */
  UWORD          Mode;             /* Stack-Mode.                             */
} SENDTX;


typedef struct TCPIP               /* TCPIP-User/Link                         */
{
  struct TCPIP *next;              /* Naechster Listeneintrag                 */
  struct TCPIP *prev;              /* Vorheriger Listeneintra                 */
#ifdef BUFFER_DEBUG
  UBYTE         owner;             /* Muss an 9. Bytestelle stehen            */
#endif
  char          ip[IPADDR + 1];    /* IP-Adresse                              */
  char          rxbuf[RXLEN + 1];  /* RX-Buffer recv                          */
  char          txbuf[TXLEN + 1];  /* TX-Buffer send                          */
  char          cmd[TXLEN + 1];    /* Befehlszeile vom User.                  */
  int           sock;              /* Socket.                                 */
  BOOLEAN       mode;              /* Socket-Mode. (OS-Socket / Inter. Socket)*/
  char          Upcall[L2IDLEN];   /* Updown Login Rufzeichen                 */
  char          Downcall[L2IDLEN]; /* Down   Login Rufzeichen                 */
  char          disflg;            /* Flag: Verbindung trennen.               */
  UWORD         port;              /* Einstiegsport (0..15)                   */
  UWORD         noacti;            /* Timer fuer keine Aktivitaet             */
  UWORD         T3;                /* Timer T3, "inactive link timer"         */
  UWORD         inlin;             /* eingelaufene Zeilen                     */
  UWORD         outlin;            /* auszugebende Zeilen                     */
  UWORD         Interface;         /* Aktuelle Interface.                     */
  int           activ;             /* Interface aktiv/deaktiv.                */
  int           login;             /* Ist User angemeldet.                    */
  int           cmdlen;            /* Laenge der Befehlszeile.                */
  int           RecvLen;           /* Rueckgabewert fuer recv                 */
  int           rxc;               /* Zaehler fuer RX-BUFFER                  */
  int           txc;               /* Zaehler fuer TX-BUFFER                  */
  int           sum;               /* Zaehler fuer gesendete.Zeichen.         */
#ifdef L1HTTPD
  UBYTE         status;            /* USER-Status                             */
  int           http;              /* Conversion IBM -> HTML.                 */
  FILE         *fp;                /* Datei laden.                            */
#endif /* L1HTTPD */
#ifdef L1IPCONV
  BOOLEAN       CVSlink;           /* Linkpartner.                            */
  BOOLEAN       Intern;            /* Connect Intern.                         */
#ifdef L1IRC
  BOOLEAN       IrcMode;           /* irc-client.                             */
#endif /* L1IRC */
#endif /* L1IPCONV */
  BOOLEAN       LoginPrompt;       /* Prompt senden.                          */
  BOOLEAN       cr;                /* Return durchlaufen.                     */
  WORD          state;             /* Connected, Disconnected setzen.         */
  LHEAD         inbuf;             /* Listenkopf Eingabebuffer                */
  LHEAD         outbuf;            /* Listenkopf Ausgabebuffer                */
  char          txstatus;          /* Sender frei/busy.                       */
} TCPIP;

extern TCPIP *tcptbl;              /* Zeiger auf die TBL.                     */
extern TCPIP *tcppoi;              /* Zeiger auf den aktuellen TCPIP-User.    */
extern LHEAD  tcpfrel;             /* Liste der freien Linkbloecke.           */
extern LHEAD  tcpactl;             /* Liste der aktiven Linkbloecke.          */
extern LHEAD  rxflRX;              /* Empfangsliste                           */
extern LHEAD  rxflTX;              /* Sendeliste                              */

extern int    tcp_tbl_top;         /* Anzahl der aktuellen TCP-User.          */

extern void    TcpipSRV(void);                              /* TCPIP-Service. */
extern void    InitTCP(void);                        /* TCPIP Initialisieren. */
extern void    InitIFC(void);                    /* Interface Initialisieren. */
extern void    L1ExitTCP(WORD);                /* TCPIP-Interface schliessen. */
extern void    L1ctlTCP(int, int);                       /* Level 1 Kontrolle */
extern BOOLEAN TcpDCD(int);                          /* DCD-Status liefern.   */
extern BOOLEAN L1InitTCP(UWORD, int, int);      /* TCPIP-Port Initialisieren. */
extern void    HwstrTCP(UWORD, int, MBHEAD *);/* Portinfo-String (PORT-Befehl)*/
extern int     CheckPortTCP(int);                  /* Pruefe, auf TCP-Port's. */
extern void    DumpTCP(MBHEAD *);             /* TCPIP-Einstellungen sichern. */
extern BOOLEAN itoTCP(BOOLEAN, MBHEAD *); /* Info vom L7 an TCP-Interf. senden*/
extern void    SetDiscTCP(void);      /* User hat ein Disconnect eingeleitet. */
extern void    TimerTCP(void); /* Noactivity-Timer fuer alle TCPIP Connect's. */
extern int     KillTCP(UWORD, char *,WORD);     /* TCPIP-User/Link(s) KILLEN. */
extern int     ReadSockTCP(void);                /* Zeichen vom Socket holen. */
extern BOOLEAN CheckContens(char);                    /* Pruefe Loginzeichen. */
extern void    TcpipRelink(MBHEAD *);       /* Eingehende Daten weiterleiten. */
extern MBHEAD *SetBuffer(void);                           /* Buffer besorgen. */
extern int     SetupTCP(char *, unsigned short);       /* Sock Initialisieren.*/
T_INTERFACE   *SearchIf(UWORD);          /* Das gesuchte Interface ermitteln  */
                                         /* und den Interfacezeiger setzen.   */
extern void    MhUpdateTCP(MBHEAD *,                /* Buffer                 */
                        BOOLEAN);                   /* Flag fuer RX/TX-Bytes. */
extern BOOLEAN LoginTCP(MBHEAD *);
extern void    RelinkTCP(MBHEAD *);                      /* Packet umhaengen. */



extern void    DiscTCP(void);     /* Alle Parameter auf default zuruecksetzen */
                                  /* und den User aus der Liste nehmen.       */

extern int     AddUserTCP(T_INTERFACE *,       /* Neue User hinzufuegen,      */
                          unsigned,            /* vorrausgesetzt es sind noch */
                          char *);             /* freie Sockets vorhanden.    */

extern void    SetDefaultWorthTCP(unsigned,           /* Defaultwerte setzen. */
                                  char *,
                                  int ,
                                  UWORD ,
                                  BOOLEAN);

extern void    WriteLogTCP(BOOLEAN,           /* System- und Errormeldungen   */
                           const char *,      /* in einer Logdatei schreiben. */
                           ...);

#define T_LOGL1 if(ifpp->log > 0)(void)WriteLogTCP     /* System-Meldungen in */
                                                 /* einer Logdatei schreiben. */
#define T_LOGL2 if(ifpp->log > 1)(void)WriteLogTCP      /* System- und Error- */
                                    /* meldungen in einer Logdatei schreiben. */
#define T_LOGL3 if(ifpp->log > 2)(void)WriteLogTCP      /* ALLE Log-Meldungen */
                                                                /* schreiben. */

extern T_INTERFACE *SetInterface(UWORD);

#define KISS_TCPIP             22                      /* 1. TCPIP-Interface  */
                                                       /* 22 Telnet           */
                                                       /* 23 Httpd            */
                                                       /* 24 IPConv           */
                                                       /* 25 IRC              */

#define KISS_MAX               25                     /* letztes Interface    */

#endif /* L1TCPIP */

/* End of include/l1tcpip.h */
