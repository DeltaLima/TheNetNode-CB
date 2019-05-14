
extern LHEAD rxSegment;                                    /* RX TCP-Segmente */
extern LHEAD txSegment;                                    /* TX TCP-Segment. */
extern LHEAD rxDaten;                                           /* RX-Buffer. */

#define TCP_HEADER        20                            /* TCP-Headerlaenge.  */
#define TCP_OPTION         8                            /* TCP-Optionslaenge. */

/* TCP-Option-Flags */
#define TFIN         0x01                       /* Socket schliessen.         */
#define TSYN         0x02                       /* Verbindungsaufbau starten. */
#define TRST         0x04                       /* Verbindung zurueck setzen. */
#define TPSH         0x08                       /* Daten senden.              */
#define TACK         0x10                       /* Bestaetigungen.            */
#define TURGE        0x20                       /* Flag ignored               */

#define WINSIZE      512                       /* Fenstergroesse festlegen.   */

/* TCP-Options */
#define OPTEND         516                           /* End of Option List.   */
#define OPTNOO         257                           /* No-Operation.         */
#define OPTMSS         512                           /* Maximum Segment Size. */
#define OPTPA3        1026

#define TCP_TIMEOUT     30         /* Timer-Wert fuer Segment Wiederholungen. */
#define TCP_MAX_RETRY    7         /* Maximal Wiederholen.                    */

typedef struct datenrx               /* Struktur fuer RX-Bufferung.           */
{
  struct datenrx *next;
  struct datenrx *prev;
#ifdef BUFFER_DEBUG
  UBYTE           owner;             /* Muss an 9. Bytestelle stehen          */
#endif
  MBHEAD         *Data;              /* RX-Buffer eines Segment.              */
  int             Sock;              /* Socket vom Segment.                   */
} DATENRX;

typedef struct stackrx               /* Struktur RX-Segment Empfang.          */
{
  struct stackrx *next;
  struct stackrx *prev;
#ifdef BUFFER_DEBUG
  UBYTE           owner;             /* Muss an 9. Bytestelle stehen          */
#endif
  MBHEAD         *Data;              /* Buffer eines Segment  .               */
  IP             *IpHdr;             /* IP-Header.                            */
} STACKRX;

typedef struct stacktx               /* Struktur TX-Segment senden.           */
{
  struct stacktx *next;
  struct stacktx *prev;
#ifdef BUFFER_DEBUG
  UBYTE           owner;             /* Muss an 9. Bytestelle stehen          */
#endif
  MBHEAD         *Data;              /* Buffer.                               */
  int             Sock;              /* Socket.                               */
  unsigned short  Flags;             /* TCP-Flags.                            */
  unsigned char   TState;            /* TCP-Statusaenderung.                  */
  int             Timer;             /* Timer.                                */
  time_t          TimeLast;          /* */
  int             Retry;             /* Retry-Zaehler.                        */
} STACKTX;


extern void     StackInitTCP(void);                     /* TCP Initialisieren */
extern void     StackSRV(void)                               ; /* TCP-Service */
extern void     StackTimer(void);         /* Timer fuer Frame-Wiederholungen. */

extern void     SendTcpFlag(register int,/* Frame fuer IP-Router vorbereiten. */

                            MBHEAD  *,
                            unsigned int,
                            unsigned short,
                            unsigned short,
                            unsigned short);

extern void     TCPIPProcess(register IP     *,/* Eingehende TCP-Daten weiter-*/
                             register MBHEAD *);/* leiten in die RX-Seg.-liste*/

extern void     PutTXStack(register int,     /* Neues TX-Segment in die Sende-*/
                           MBHEAD      *,         /* liste anlegen/anhaengen. */
                           int,
                           unsigned short,
                           unsigned char);

extern void     DelSock(int);             /* Entsorge alle Buffer vom Socket. */
extern DATENRX *GetBuffer(TSOCKET *); /* Einen Eintrag aus der RX-Liste holen.*/

/* End of include/l3tcp.h. */
