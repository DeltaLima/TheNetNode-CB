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
/* File include/typedef.h (maintained by: ???)                          */
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

#ifndef __TYPEDEF_H
#define __TYPEDEF_H

/* vorzeichenbehaftete Typen */
typedef signed char      BYTE;                                /*  8 Bit */
typedef signed short     WORD;                                /* 16 Bit */
typedef signed long      LONG;                                /* 32 Bit */

/* vorzeichenlose Typen */
typedef unsigned char   UBYTE;                                /*  8 Bit */
typedef unsigned short  UWORD;                                /* 16 Bit */
typedef unsigned long   ULONG;                                /* 32 Bit */

typedef unsigned short  UID;
typedef int             INDEX;
#define NO_INDEX        -1

typedef enum {FALSE = 0, TRUE} BOOLEAN;

typedef enum {ERRORS = -1, NO, YES} TRILLIAN;

typedef enum {EMPTY = 0, DOWNLINK, UPLINK, U_IPLINK, D_IPLINK,
              PEERLINK, CQ_LINK
#ifdef USER_MONITOR
             , TRACEN
#endif /* USER_MONITOR */
#ifdef L1IPCONV
             , C_IPLINK
#endif /* L1IPCONV */
             } LINKTYP;

typedef struct lhead        /* "list head", Listenkopf :                */
{
  struct lhead *head;       /* Zeiger auf ersten Eintrag in Liste       */
  struct lhead *tail;       /* Zeiger auf letzten Eintrag in Liste      */
} LHEAD;


typedef struct lehead       /* "list entry head", Kopf eines Eintrags   */
{                           /* in Liste :                               */
  struct lehead *nextle;    /*   Zeiger auf naechsten Listeneintrag     */
  struct lehead *prevle;    /*   Zeiger auf vorherigen Listeneintrag    */
#ifdef BUFFER_DEBUG
  UBYTE          owner;     /* Muss an 9. Bytestelle stehen             */
#endif
} LEHEAD;


typedef struct lnkblk       /* "link block", fuer jeden Level-2-Link :  */
{
  struct lnkblk *next;      /* Naechster Listeneintrag                  */
  struct lnkblk *prev;      /* Vorheriger Listeneintrag                 */
  UBYTE    state;           /* Link-State, s.o. L2S...                  */
  char     srcid[L2IDLEN];  /* "source id", eigenes Call/SSID oder      */
                            /* Ident/SSID                               */
  char     dstid[L2IDLEN];  /* "destination id", Call/SSID Gegenstation */
  char     viaidl[L2VLEN+1];/* "via id list", Digipeaterstrecke,        */
                            /* 0-terminiert, Weg zur Gegenstation       */
  char    *realid;          /* Zeiger auf Rufzeichen des "echten"       */
                            /* Senders                                  */
  UWORD    RTT;             /* Round-Trip-Timer (10 ms)                 */
  UWORD    SRTT;            /* Smoothed Round Trip Timer                */
                            /* SRTT = (Alpha x SRTT + RTT)/(Alpha + 1)  */
  UBYTE    RTTvs;           /* VS bei der die Messung gestartet wurde   */
  UWORD    T1;              /* Timer 1, "frame acknowledge interval",   */
                            /* Start :  SRTT,                           */
                            /* 0 = inaktiv, 10 msec Downcounter         */
  UWORD    T2;              /* Timer T2, "response delay timer",        */
                            /* 0 = inaktiv, 10 msec Downcounter         */
  UWORD    T3;              /* Timer T3, "inactive link timer",         */
                            /* 0 = inaktiv, 10 msec Downcounter         */
  UWORD    noatou;          /* "no activity timeout",                   */
                            /* nach Ablauf Link disconnecten,           */
                            /* 0 = inaktiv, 1 sec Downcounter           */
  UWORD    rcvd;            /* "received", Anzahl empfangener I-Frames  */
                            /* in rcvdil                                */
  UWORD    tosend;          /* Anzahl noch nicht gesendete oder         */
                            /* unbestaetigte Frames in sendil           */
  LHEAD    rcvdil;          /* "received info list", richtig            */
                            /* empfangene I-Frames, mit Header/PID      */
  LHEAD    sendil;          /* "send info list", zu sendende I-Frames,  */
                            /* ohne Header/PID, nur Info                */
  LHEAD    damail;          /* DAMA - Sendeframeliste                   */
  UBYTE    damapm;          /* DAMA - Prioritaetsmerker                 */
                            /* 0=hoechste Prioritaet, 10=niedrigste     */
  UWORD    damapc;          /* DAMA - Prioritaetszaehler in 10ms        */
  UWORD    liport;          /* "link port", 0..L2PNUM-1                 */
  UBYTE    VR;              /* "receive sequence variable", Sequenz-    */
                            /* nummer des naechsten zu empfangenden     */
                            /* I-Frames                                 */
  UBYTE    VS;              /* "send sequence variable", Sequenznummer  */
                            /* des naechsten zu sendenden I-Frames      */
  UBYTE    lrxdNR;          /* "last received N(R)", zuletzt            */
                            /* empfangenes N(R) = eigene gesendete      */
                            /* I-Frames bis lrxdNR-1 bestaetigt         */
  UBYTE    ltxdNR;          /* "last transmitted N(R)", zuletzt         */
                              /* gesendetes N(R) = empfangene I-Frames    */
                            /* bis ltxdNR-1 bestaetigt                  */
  UBYTE    tries;           /* aktuelle Anzahl Versuche (RETRY),        */
                            /* hochzaehlend                             */
  UBYTE    RStype;          /* "response supervisory frametype", nach   */
                            /* T2-Ablauf zu sendendes Antwortframe      */
                            /* (RR=0x01, RNR=0x05, REJ=0x09)            */
  UBYTE    frmr[5];         /* die 3 FRMR-Infobytes, Sendung u. Empfang */
                            /*   frmr[0] : zurueckgewies. Kontrollfeld  */
                            /*   frmr[1] : V(R) CR V(S) 0               */
                            /*   frmr[2] : 0000ZYXW                     */

                            /* bei EAX.25:                              */
                            /* die 5 FRMR-Infobytes, Sendung u. Empfang */
                            /*   frmr[0] : rueckgew. Kontrollbyte 1     */
                            /*   frmr[1] : rueckgew. Kontrollbyte 2     */
                            /*   frmr[2] : V(S) 0                       */
                            /*   frmr[3] : V(N) CR                      */
                            /*   frmr[4] : 0000ZYXW                     */

  UWORD    flag;            /* Flag (s. include/l2.h L2F...)            */
  UBYTE    busyrx;          /* angenommene Frames waehrend busy         */
  UWORD    indx;            /* MultiConnect: Indizes (nicht SSIDs)      */
  UWORD    anzl;            /* Anzahl der MultiConnects des Users..     */
  UWORD    zael;            /* MultiConnect: aktueller Zaehlerstand     */
  UBYTE    pollcnt;         /* Anzahl verbotener Polls vom DAMA-User    */
  WORD     priold;          /*<=== DAMA-Merker/Flag Link-Prioritaet     */

  struct mbhead *tmbp;      /* RX-Liste empfangene Fragmente            */
  UBYTE    maxframe;        /* fuer diesen Link (<= Port-Maxframe)      */
  UBYTE    bitmask;         /* Bitmaske Frameauswertung 0x07 oder 0x7F  */
} LNKBLK;

typedef struct mbhead       /* "message buffer head",                   */
{                           /* Datenbuffer-Liste, Kopf :                */
  struct mbhead *nextmh;    /* doppelt verkettete Liste                 */
  struct mbhead *prevmh;
#ifdef BUFFER_DEBUG
  UBYTE          owner;     /* Muss an 9. Bytestelle stehen             */
#endif
  LHEAD            mbl;     /*   "message buffer list", Zeiger auf      */
                            /*   ersten Infobuffer dieser Message       */
#ifdef __WIN32__
  unsigned char huge *mbbp; /*   "message buffer byte pointer",         */
#else
  char  huge      *mbbp;    /*   "message buffer byte pointer",         */
#endif
                            /*   Zeiger auf aktuelles Zeichen in Buffer */
  UWORD            mbpc;    /*   "message buffer put count",            */
                            /*   Einschreibzaehler, aufwaertszaehlend   */
  UWORD            mbgc;    /*   "message buffer get count",            */
                            /*   Lesezaehler, aufwaertszaehlend         */
  LNKBLK          *l2link;  /*   Zeiger auf assozierten Linkblock       */
  WORD             type;    /*   Typ des Buffers (User, Status)         */
                            /*     oder MB...                           */
                            /*     oder 0 User, 2 Level-2, 4 Level-4    */
  UBYTE            l2fflg;  /*   Level 2 Frameflag / PID :              */
                            /*     RX: PID                              */
                            /*     TX: PID / s.o. T2FT1ST/T2FUS         */
  UWORD            l2port;  /*   Level-2-Port (0..L2PNUM-1)             */
  UBYTE            morflg;  /*   "more follows flag" fuer Pakete, die   */
                            /*   durch zusaetzlichen Netzwerkheader zu  */
                            /*   lang wuerden und in 2 Frames           */
                            /*   aufgetrennt wurden                     */
                            /*     YES = das naechste Frame gehoert zu  */
                            /*           diesem Paket                   */
                            /*     NO  = sonst                          */
  UBYTE            l4seq;   /* Sequenznummer des Paketes in Level 4     */
  UWORD            l4time;  /* Timeout in Level 4                       */
  UBYTE            l4trie;  /* Versuche in Level 4                      */
  UBYTE            l4type;  /* Typ des L4-Paketes                       */
#define L4TNORMAL  0        /* Normales L4-Info-Frame                   */
#define L4TSTATUS  1        /* L3-Status-Frame, beschleunigt senden     */
#define L4TPRIOR   2        /* Eiliges L3-Status-Frame                  */
  UBYTE            tx;      /* 0 = empfangenes, 1 = gesendetes Frame    */
  UBYTE            repeated;/* I-Frame Wiederholungszaehler             */
  UBYTE            l3_typ;  /* Spezielle Frames des Level3 ab TNN V1.55 */
#define L3NORMAL 0x00       /* Standard L3-Frame                        */
#define L3LOCAL  0x01       /* Local-Connect                            */
#define L3RTTF   0xFF       /* L3RTT-Messframe                          */
  time_t           btime;   /* Zeit                                     */
  char             destcall[L2IDLEN];
#ifdef MAXFRAMEDEBUG
  UWORD            lnkflag; /* Flags des Links bei Aussendung           */
  UBYTE            lmf;     /* Link-Maxframe bei Aussendung             */
  UBYTE            pmf;     /* Port-Maxframe bei Aussendung             */
  UWORD            tosend;  /* zu sendende Frames                       */
#endif
} MBHEAD;

typedef struct mb           /* "message buffer",                        */
{                           /* allgemeiner Datenbuffer :                */
  struct mb *nextmb;        /*   naechster Eintrag in Liste             */
  struct mb *prevmb;        /*   vorheriger Eintrag in Liste            */
#ifdef BUFFER_DEBUG
  UBYTE          owner;     /* Muss an 9. Bytestelle stehen             */
#endif
  char       data[64];      /* wird bis MAX_BUFFERS-8 gefuellt          */
} MB;

/*---------------------conversd-Strukturen--------------------------*/

#define NAMESIZE 16

typedef struct clist {
  WORD channel;                 /* channel number */
  WORD channelop;               /* channel operator status */
  time_t time;                  /* join time */
  struct clist *next;           /* pointer to next entry */
} CLIST;

#define NULLCLIST ((CLIST *) NULL)

typedef struct connection {
  WORD type;                     /* Connection type */
#define CT_UNKNOWN      0
#define CT_USER         1
#define CT_HOST         2
#define CT_CLOSED       3
  char name[NAMESIZE + 1];      /* Name of user or host */
  char host[NAMESIZE + 1];      /* Name of host where user is logged on */
  char rev[NAMESIZE + 1];       /* revision of software (CT_HOST) */
#ifdef CONVNICK
  char nickname[NAMESIZE + 1];  /* Nickname of user */
  char OldNickname[NAMESIZE + 1];/* Old Nickname of user */
  WORD features;                /* Features of neighbor */
  /* alle anderen Features sind immer da, wir brauchen nur den hier */
#define FEATURE_NICK 32         /* Nickname-Feature */
#endif
  struct connection *via;       /* Pointer to neighbor host */
  WORD channel;                 /* current channel number */
  CLIST *chan_list;             /* linked list of joined channels */
  time_t time;                  /* Connect time */
  WORD locked;                  /* Set if mesg already sent */
  MBHEAD *mbout;                /* Output queue */
  LONG received;                /* Number of bytes received */
  LONG xmitted;                 /* Number of bytes transmitted */
  char *away;                   /* Away string */
  time_t atime;                 /* time of last "away" state change */
  time_t mtime;                 /* time of last message receive */
  char *pers;                   /* Personal string */
  WORD verbose;                 /* verbose mode */
  char prompt[4];               /* prompt mode */
  char query[NAMESIZE];         /* name of queried user */
  char *notify;                 /* List of calls you like to be notified */
  char *filter;                 /* List of calls you like to filter */
  WORD charset_in;              /* charset - default ISO (ansi) */
  WORD charset_out;             /* charset - default ISO (ansi) */
  WORD channelop;               /* channel operator of current channel ? */
  WORD operator;                /* convers operator */
  WORD invitation_channel;      /* last invitation was from this channel */
  WORD width;                   /* user screen width */
  struct usrblk *up;            /* parent-userblk of this connection */
  struct connection *next;      /* Linked list pointer */
#ifdef L1IRC
  BOOLEAN IrcMode;
#endif /* L1IRC */
} CONNECTION;

#define CM_UNKNOWN   (1 << CT_UNKNOWN)
#define CM_USER      (1 << CT_USER)
#define CM_HOST      (1 << CT_HOST)
#define CM_CLOSED    (1 << CT_CLOSED)

#define NULLCONNECTION  ((CONNECTION *) NULL)

typedef struct permlink {
#ifdef CONVERS_HOSTNAME
    char cname[NAMESIZE + 1];   /* Internal name of host */
#else
    char cname[8];              /* Internal name of host */
#endif
  char name[NAMESIZE + 1];      /* Name of host */
  CONNECTION *connection;       /* Pointer to associated connection */
  time_t statetime;             /* Time of last (dis)connect */
  WORD tries;                   /* Number of connect tries */
  time_t waittime;              /* Time between connect tries */
  time_t retrytime;             /* Time of next connect try */
  time_t testwaittime;          /* Time between tries */
  time_t testnexttime;          /* Time of next test */
  time_t rxtime;                /* rtt by other side */
  time_t txtime;                /* rtt found out by me */
  WORD locked;                  /* test for a loop blocker */
  WORD nlcks;                   /* lock-counter */
  UWORD port;                   /* Neighbour port                       */
  char  call[L2IDLEN];          /* Call of host                         */
  char  via[L2VLEN+1];          /* 0-term. via-list                     */
#ifdef L1IPCONV
  BOOLEAN TcpLink;
  UBYTE   HostName[64 + 1];
  ULONG   IpAddr;
#endif /* L1IPCONV */
} PERMLINK;

#define NULLPERMLINK  ((PERMLINK *) NULL)

typedef struct destination {
  char name[NAMESIZE];          /* destination name */
  char rev[NAMESIZE];           /* revision of software (CT_HOST) */
  PERMLINK *link;               /* link to this destination */
  time_t rtt;                   /* round trip time to this host */
  time_t last_sent_rtt;         /* last donwnstream sent rtt */
  struct destination *next;     /* a one dimansional list is ok for now :-) */
} DESTINATION;

#define NULLDESTINATION  ((DESTINATION *) NULL)

typedef struct channel {
  char *topic;                  /* topic of channel */
#ifdef CONV_TOPIC
  char  name[NAMESIZE + 1];     /* Username of channel. */
#endif
  time_t time;                  /* when was the topic set ? */
  WORD chan;                    /* integer value of channel (<32768 !) */
  WORD flags;                   /* channel flags */
#define M_CHAN_S 0x01           /* secret channel - number invisible */
#define M_CHAN_P 0x02           /* private channel - only join by invitation */
#define M_CHAN_T 0x04           /* topic settable by operator only */
#define M_CHAN_I 0x08           /* invisible channel - only sysops can see */
#define M_CHAN_M 0x10           /* moderated channel - operators may talk */
#define M_CHAN_L 0x20           /* local channel - no forwarding */
  struct channel *next;         /* linked list again */
} CHANNEL;

#define NULLCHANNEL  ((CHANNEL *) NULL)

/*---------------Ende der conversd-Strukturen-----------------------*/


typedef struct stentry      /* "state table entry",                     */
{                           /* ein Eintrag in der State-Table :         */
  void  (*func)(void);      /*   Zustandsuebergangsfunktion             */
  UBYTE newstate;           /*   neuer Zustand                          */
  UBYTE msg;                /*   Meldung                                */
} STENTRY;

typedef struct mfentry      /* "message function entry",                */
{                           /* ein Eintrag in der State-Table :         */
  void  (*func)(void);      /*   Meldungsfunktion                       */
} MFENTRY;

#ifdef PACSAT
typedef struct pacsatblk    /* PACSAT-Server V1.12                      */
{
#ifdef BUFFER_DEBUG
  void    *next;
  void    *prev;
  UBYTE    owner;           /* Muss an 9. Bytestelle stehen             */
#endif
  char    *tempfile;        /* Temporaer-Name beim Upload               */
  FILE    *tempfp;          /* File beim Upload                         */
  BOOLEAN  check_pwd;       /* bei BOX Passwort verlangen ?             */
  time_t   login;           /* Loginzeit fuer Passwort                  */
  WORD     row, col;        /*                                          */
} PACSATBLK;
#endif

typedef struct monbuf {             /* Struktur fuer einen Monitor      */
#ifdef BUFFER_DEBUG
  void    *next;
  void    *prev;
  UBYTE    owner;                   /* Muss an 9. Bytestelle stehen     */
#endif
  UBYTE Mpar;                       /* Flags fuer den Monitor           */
  UWORD Mport;                      /* Port fuer den Monitor            */
  UBYTE mftsel;                     /* Rufzeichen an/aus/alle?          */
  char  mftidl[L2VLEN+1];           /* Platz fuer 8 Rufzeichen          */
} MONBUF;

#define HOST_USER  0
#define L2_USER    2
#define L4_USER    4

#define NO_UID     (0xFFFFU)

typedef struct  usrblk
{
  struct usrblk *unext;   /* doppelt verkettete Liste                   */
  struct usrblk *uprev;
#ifdef BUFFER_DEBUG
  UBYTE       owner;      /* Muss an 9. Bytestelle stehen               */
#endif
  UID         uid;        /* eigene User-ID                             */
  UID         p_uid;      /* Partner User-ID                            */
  MBHEAD     *mbhd;       /* eingelaufene Frames fuer User              */
  FILE       *fp;         /* File fuer Read                             */
  char       *fname;      /* Pointer auf Dateiname+Pfad fuer Read       */
#ifdef MAKRO_FILE
  int         read_ok;    /* Wurde userp->fp geupdate?                  */
#endif
  CONNECTION *convers;    /* Pointer auf Convers-Struktur               */
#ifdef PACSAT
  PACSATBLK  *pacsat;     /* Zeiger auf den PACSAT-Block                */
#endif
  MONBUF     *monitor;    /* Zeiger auf den Monitor-Filter-Buffer       */
  int         auditlevel;
  char        talkcall[11];
  UBYTE       status;     /* USER-Status                                */
#define US_NULL 0         /* leer/dummy                                 */
#define US_CCP  1         /* CCP-User                                   */
#define US_CREQ 2         /* Connect-Request                            */
#define US_WPWD 3         /* Warte auf Sysop-Passwort                   */
#define US_RTXT 4         /* Empfange Textdatei                         */
#define US_WBIN 5         /* Warte auf #BIN#                            */
#define US_SBIN 6         /* Sende Binaerdaten                          */
#define US_RBIN 7         /* Empfange Binaerdaten                       */
#define US_CHST 8         /* ConversHost                                */
#define US_TALK 9         /* TALK-Mode                                  */
#define US_DIG  10        /* User muss weitergereicht werden            */
#define US_EXTP 12        /* externes Programm laeuft (Linux)           */
#define US_CQ   13        /* User ruft CQ                               */
#define US_WUPW 14        /* warte auf User-Password                    */
#ifdef USER_MONITOR
#define US_TRAC 15        /* Ein User Trace auf einen Port.             */
#endif /* USER_MONITOR */
#ifdef USERPROFIL
#define US_UPWD 16         /* Warte auf User-Passwort                   */
#endif /* USERPROFIL */

#if defined(__LINUX__) || defined(__WIN32__)
  pid_t   child_pid;      /* PID des Child-Prozesses                    */
  UWORD   child_timeout;  /* Timeout                                    */
  int     child_fd;       /* Filedescriptor des tty-Interfaces          */
  BOOLEAN child_iactive;  /* interaktive Shell oder nur Single-Command  */
#endif /* LINUX / WIN32 */
  UBYTE   sysflg;         /* SYSOP Flag                                 */
  UBYTE   errcnt;         /* CCP Fehlerzaehler                          */
  UBYTE   convflag;       /* wenn != 0 soll nicht in den ccp zurueck    */
                          /* 1 = CVSHOST von aussen 2= CVSHOST von uns  */
  UBYTE   paswrd[5];      /* gegebene Passwort Stellen                  */
#ifdef USER_PASSWORD
  UBYTE   pwdtyp;         /* User-Password Typ                          */
#define PW_NOPW 0         /* noch kein Password eingegeben              */
#define PW_USER 1         /* User hat Password eingegeben               */
#endif
#ifdef L1IRC
  UWORD   IrcMode;        /* IRC-Mode markieren.                        */
#endif /* L1IRC */
} USRBLK;

typedef struct            /* Zum Host connectete User                   */
{
  char    call[L2IDLEN];  /* Call des Users                             */
  char    conflg;         /* User ist connected Flag                    */
  char    disflg;         /* Flag: Verbindung trennen, wenn Info weg    */
  char    direction;      /* 0 = Conn. von aussen; 1 = Conn. zum Knoten */
  UWORD   noacti;         /* Timer fuer keine Aktivitaet                */
  UWORD   inlin;          /* eingelaufene Zeilen                        */
  UWORD   outlin;         /* auszugebende Zeilen                        */
  UWORD   outsta;         /* auszugebende Meldung                       */
  LHEAD   inbuf;          /* Listenkopf Eingabebuffer                   */
  LHEAD   outbuf;         /* Listenkopf Ausgabebuffer                   */
} HOSTUS;

typedef struct l2_link
{
#ifdef BUFFER_DEBUG
  void    *next;
  void    *prev;
  UBYTE    owner;               /* Muss an 9. Bytestelle stehen         */
#endif
  char call[L2IDLEN];           /* Rufzeichen                           */
  char alias[L2CALEN];          /* Alias                                */
  char digil[L2VLEN+1];         /* Digiliste                            */
/*  char digil[2*L2IDLEN+1];*/      /* Digiliste - NUR 2 DIGIS!             */
  UWORD port;                   /* L2-Port                              */
  UWORD ssid_high;              /* SSID oben fuer Flexnet               */
#ifdef LINKSMODINFO
#define INFOSIZE 28
  char  info[INFOSIZE + 1];     /* Stationsbeschreibung.                */

#endif /* LINKSMODINFO */
#ifdef AUTOROUTING
  UWORD ppAuto;                 /* Fixed/Auto-Route.                    */
#endif /* AUTOROUTING */

#ifdef PORT_L2_CONNECT_TIME
  UWORD sabmtime;               /* Intervall fuer SABM                  */
#endif
} L2LINK;

#define NOMODE -1       /* Linkmode ist noch nicht gesetzt/egal         */
#define INP     0       /* Knoten nach INP-Protokoll                    */
#define TNN     1       /* neuer TNN-Typ                                */
#define THENET  2       /* Nachbar, der nicht auf Messung reagiert      */
#define NETROM  3       /* unser guter alter Netrom                     */
                        /* alles <= NETROM wird als NETROM behandelt!   */
#define FLEXNET 4       /* FlexNet                                      */
#define LOCAL   5       /* Local                                        */
#define LOCAL_M 6       /* Local mit Messung                            */
#ifdef LINKSMOD_LOCALMOD
#define LOCAL_N 7       /* Local ohne Messung und ohne Weiterleitung an */
                        /* den Nachbarn.                                */

#define LOCAL_V 8       /* Local ohne Messung, ohne Weiterleitung und   */
                        /* versteckt, nur fuer den Sysop sichtbar.      */
#endif /* LINKSMOD_LOCALMOD */

#define SSID(x) ((x[L2CALEN]>>1)&0x0F)
#define MAX_SSIDs 16

typedef struct                  /* Weginformation                       */
{
  unsigned  reported_quality;   /* letzte gemeldete Qualitaet           */
  unsigned  quality;            /* letzte empfangene Qualitaet          */
  int       timeout;            /* Timeout (> 0 wenn aktiv)             */
  char      lt;                 /* Hops seit dem Ziel                   */
  char      ssid_high;          /* oberer SSID-Bereich                  */
} ROUTE;

typedef struct peer             /* ein Segment ("Nachbar")              */
{
  ROUTE  *routes;               /* Liste mit Weginformationen           */

                                /* alle Qualitaeten in ms Schritten     */
  ULONG quality;                /* geglaettete Qualitaet zum Segment    */
  ULONG my_quality;             /* meine Messung                        */
  ULONG his_quality;            /* seine Messung                        */

  BOOLEAN used;                 /* frei?                                */

  BOOLEAN locked;               /* feste Route, nicht loeschen          */
  int     options;

  int     num_routes;           /* Anzahl der Routen                    */

  struct peer *primary;         /* Flag: == 0 primaerer Weg             */
                                /*       != 0 der beste Weg dorthin     */

  L2LINK *l2link;               /* vorgegebener Link-Weg                */
  WORD    rtt_time;             /* Messintervalle fuer L3RTT            */
  ULONG   rttstart;             /* Laufzeitmessung, Startzeit           */

  LHEAD   rxfl;                 /* Neigbour Frame List                  */

  WORD    tosend;               /* Frames fuer den Nachbarn             */

  LNKBLK  *nbrl2l;              /* Querverweis zur Level2 Tabelle       */
  UBYTE   tries;                /* Anzahl der Connectversuche           */
  char    soll_typ;             /* Nachbar Typ (soll)                   */
  char    typ;                  /* Nachbar Typ (ist)                    */
#ifdef PROXYFUNC
#define PROXYMASK 64
  BOOLEAN proxy;                /* Proxyfunktion fuer diesen Link        */
#endif
  BOOLEAN secured;
  WORD    version;              /* TNN-Version                          */
  UBYTE   token;                /* Flexnet, wer hat das Token?          */
  UWORD   brotim;               /* Broadcast Timer                      */
  UWORD   maxtime;              /* max. gewuenschte Laufzeit            */
#ifdef CONNECTTIME
  ULONG   contime;              /* Connectzeit in Flexnet-Stil.         */
#endif /* CONNECTTIME */
#ifdef THENETMOD
  UWORD   obscnt;               /* Restlebensdauer fuer Rundspruch.     */
  BOOLEAN constatus;            /* Connect-Status.                      */
#endif /* THENETMOD */
} PEER;

typedef struct node             /* Nodes-Liste                          */
{
  struct node *next;            /* doppelt verkettete Liste             */
  struct node *prev;
  char         id[L2IDLEN];     /* Rufzeichen des Nodes                 */
  char         alias[L2CALEN];  /* Alias dieses Zieles                  */
  ULONG        ipa;             /* IP-Adresse                           */
  UBYTE        bits;            /* Bits fuer Subnet-Maske               */
  MBHEAD      *options;         /* unbekannte INP-Options in Buffer     */
  char         ssid_high;       /* oberer SSID-Bereich                  */
} NODE;

typedef struct network          /* ein Netzwerk                         */
{
  PEER   *peertab;              /* Segment-Tabelle                      */
  NODE   *nodetab;              /* Nodes-Tabelle                        */
  LHEAD   nodelis;              /* sortierte Nodes-Liste                */
  int     max_peers;            /* max. Anzahl der Segmente             */
  int     num_peers;            /* Anzahl der aktiven Segmente          */
  int     max_nodes;            /* max. Anzahl der Nodes                */
  int     num_nodes;            /* Anzahl der aktiven Nodes             */
} NETWORK;

typedef struct {                /* ein Netrom Route Record Eintrag      */
#ifdef __WIN32__
  UBYTE id[L2IDLEN];
  UBYTE lt;
#else
  char id[L2IDLEN];
  char lt;
#endif
} NRRLIST;

typedef struct ptcent      /* Patchcord Liste                           */
{
  ULONG    inforx;         /* Empfangene Info-Bytes                     */
  ULONG    infotx;         /* Gesendete Info-Bytes                      */
  ULONG    lastrx;         /* letzter Stand der Info-Bytes              */
  ULONG    lasttx;         /* letzter Stand der Info-Bytes              */
  ULONG    contime;        /* Connect-Zeit                              */
  ULONG    rxbps;          /* Empfangsbaudrate                          */
  ULONG    txbps;          /* Sendebaudrate                             */
  BOOLEAN  recflg;         /* Nach einem DISCONNECT reconnect in CCP?   */
  UID      p_uid;          /* Partner User-ID                           */
  USRBLK  *ublk;           /* Zugehoeriger User-Block (p_uid==CCP_USER) */
  LINKTYP  state;          /* Ist das der Uplink?                       */
  UBYTE    local;
#define PTC_NORMAL 0
#define PTC_LOCAL  1
} PTCENT;                  /* es gehoeren immer 2 Eintraege zusammen    */

typedef struct cirblk      /* Level 3 Kontrollblock                     */
{
  struct cirblk *head;     /* doppelt verkettete Liste                  */
  struct cirblk *tail;
  char    state;           /* Status: 0=leer, 1=ConReq, 2=Con, 3=DisReq */
#ifdef __WIN32__
  unsigned char    idxpar; /* Partner Index                             */
  unsigned char    ideige; /* eigener ID                                */
  unsigned char    idpart; /* Partner ID                                */
#else
  char    idxpar;          /* Partner Index                             */
  char    ideige;          /* eigener ID                                */
  char    idpart;          /* Partner ID                                */
#endif /* WIN32 */
  char    destca[L2IDLEN]; /* Zielrufzeichen (wenn != myid -> LOCAL)    */
  char    downca[L2IDLEN]; /* Downlink Call                             */
  char    upcall[L2IDLEN]; /* Uplink Call                               */
  char    window;          /* Fenstergroesse                            */
  char    l4rxvs;          /* letzte bestaetigte Framenummer            */
  char    l4vs;            /* letzte gesendete Framenummer              */
  char    l4vr;            /* letzte erhaltene Framenummer              */
  char    l4rs;            /* notwendige Antwort: 0=ACK, 1=NAK, 2=NAKweg*/
  UBYTE   l4try;           /* Transport Versuche                        */
  char    l4flag;          /* DISC-req, selbst choked, Partner choked   */
  UWORD   RTT;             /* Round-Trip-Timer (10 ms)                  */
  UWORD   SRTT;            /* Smoothed Round Trip Timer                 */
                           /* SRTT = (Alpha x SRTT + RTT)/(Alpha + 1)   */
  char    RTTvs;           /* RTT gestartet bei dieser Framenummer      */
  UWORD   traout;          /* Transport Timeout                         */
  UWORD   acktim;          /* Acknowledge Timer                         */
  UWORD   tranoa;          /* no-activity-Timeout                       */
  UWORD   numrx;           /* empfangene Frames                         */
  UWORD   numtx;           /* zu sendende Frames                        */
  MBHEAD *fragme;          /* Fragment eines kommenden Frames           */
  char    l3node[L2IDLEN]; /* Node-Call fuer diesen Circuit             */
  LHEAD   mbhdrx;          /* Listenkopf empfangene Frames              */
  LHEAD   mbhdtx;          /* Listenkopf zu sendende Frames             */
  LHEAD   mbhdos;          /* Listenkopf: Frames ausserhalb der Folge   */
  UWORD   ll4txNR;         /* last level 4 tx NR  (on5zs)               */
  char    upnod[L2IDLEN];  /* Uplinkknoten                              */
  char    upnodv[L2VLEN+1];/* Digipeaterkette beim Uplink               */
#ifdef NEW_L4
  UBYTE   pid;             /* aktuelle PID des Links                    */
#endif
} CIRBLK;

typedef struct param            /* Parameter                            */
{
  UWORD *paradr;                /* Adresse des Parameters               */
  UWORD  minimal;               /* Minimalwert                          */
  UWORD  maximal;               /* Maximalwert                          */
  const char *parstr;           /* Name des Parameters                  */
} PARAM;

typedef struct command          /* Befehls-Struktur                     */
{
  const char *cmdstr;           /* Befehlsname                          */
  void (*cmdfun)(const char *); /* auszufuehrende Funktion              */
  const char *cmdpar;           /* Zeiger auf Parameter                 */
} COMAND;
/* WICHTIG!: bei ST gibts eine Struktur COMMAND schon (os-headers) */

typedef struct hostcomand       /* Funktionen nur fuer Console          */
{
  const char *cmdstr;           /* Funktionsname                        */
  void (*cmdfun)(void);         /* auszufuehrende Funktion              */
} HOSTCMD;

typedef struct statistik        /* Fuer Statistik                       */
{
  char    call[L2IDLEN];        /* Rufzeichen                           */
  char    viacall[L2IDLEN];     /* falls Call nur via gefuehrt wird     */
  time_t  hfirst;               /* zuerst gehoert                       */
  time_t  hlast;                /* zuletzt gehoert                      */
                                /* rx = 0, tx = 1                       */
  ULONG   Bytetotal[2];         /* Anzahl Info+Protokoll Bytes          */
  ULONG   Byteheader[2];        /* Anzahl Protokoll Bytes               */
  ULONG   Ino[2];               /* Anzahl I-Frames                      */
  ULONG   RRno[2];              /* Anzahl RR-Frames                     */
  ULONG   REJno[2];             /* Anzahl REJ-Frames                    */
  ULONG   RNRno[2];             /* Anzahl RNR-Frames                    */
  ULONG   SABMno[2];
  ULONG   DISCno[2];
  ULONG   UAno[2];
  ULONG   DMno[2];
  ULONG   FRMRno[2];
  ULONG   UIno[2];
  ULONG   txByterepeated;       /* Anzahl wiederholte Infobytes         */
} STAT;

typedef struct mhtab {          /* MHEARD-Tabelle                       */
    const char *name;
    time_t      mhstart;
    LHEAD       heardl;
    UWORD       max;
    UWORD       act;
} MHTAB;

typedef struct  mheard          /* Direkt am Digi-Standort gehoerte     */
{                               /* L2-Frames                            */
  struct mheard *next;          /* doppelt verkettete Liste             */
  struct mheard *prev;
#ifdef BUFFER_DEBUG
  UBYTE          owner;         /* Muss an 9. Bytestelle stehen         */
#endif
  time_t         heard;         /* Zeitpunkt des Empfangs               */
  ULONG          tx_bytes;      /* Gesendete Info-Bytes zum User        */
  ULONG          rx_bytes;      /* Empfangene Info-Bytes vom User       */
  ULONG          tx_rej;        /* wieviele rej gesendet ?              */
  ULONG          rx_rej;        /* wieviele empfangen                   */
  UWORD          port;          /* TNC, an dem das Frames gehoert wurde */
  char           id[L2IDLEN];   /* Rufzeichen                           */
  char           via[L2IDLEN];  /* via einer Station?                   */
  UWORD          damawarn;      /* Meckermeldungen/DISC's vom Knoten    */
#ifdef EAX25
  BOOLEAN        eax_link;      /* Merker: Link unterstuetzt EAX.25     */
#endif
#ifdef MH_LISTE
  WORD           flag;          /* zuletzt gehoertes Rufzeichen/Port    */
#endif                          /* anzeigen.                            */
} MHEARD;

typedef struct cqbuf            /* Eintrag fuer User im CQ-Modus        */
{
  struct cqbuf *next;           /* doppelt verkettete Liste             */
  struct cqbuf *prev;
#ifdef BUFFER_DEBUG
  UBYTE          owner;         /* Muss an 9. Bytestelle stehen         */
#endif
  char          id[7];          /* Rufzeichen                           */
  UID           uid;            /* User ID                              */
  UID           p_uid;
} CQBUF;

typedef struct portinfo         /* Alle L1 Parameter fuer einen Port    */
{
  char  name[11];               /* Pseudoname des Ports                 */
  UWORD speed;                  /* Port Speed (z.Z. nur fuer Vanessa    */

  WORD  reset_port;             /* Diesen Port resetten ?               */
  UWORD nmblks;                 /* Anzahl Links auf dem Port            */
  UWORD nmbstn;                 /* Anzahl Stationen auf dem Port        */

  UWORD txdelay;                /* Txdelay auf diesem Port              */
  UWORD mtu;                    /* Maximum Transmition Units (I-Feld)   */

  UWORD persistance;            /* AUTOPAR: Persistance                 */
  UWORD slottime;               /* AUTOPAR: Slottime                    */

  UWORD IRTT;                   /* AUTOPAR: IRTT (T1)                   */
  UWORD T2;                     /* AUTOPAR: T2                          */
  UWORD retry;                  /* AUTOPAR: retry                       */

  UWORD maxframe;               /* MANUELL: maxframe                    */

#ifdef PORT_MANUELL
  UWORD paclen;                 /* MANUELL: Packetlaenge                */
  UWORD T3;                     /* MANUELL: T3_timer                    */
#endif /* PORT_MANUELL */

#ifdef IPOLL_FRAME
  UWORD ipoll_paclen;           /* MANUELL: IPOLL-Paecketlaenge         */
  UWORD ipoll_retry;            /* MANUELL: IPOLL-Retry                 */
#endif /* IPOLL_FRAME */

#ifdef PORT_L2_CONNECT_TIME
  UWORD l2_connect_time;        /* Intervall fuer SABM                  */
#endif

#ifdef PORT_L2_CONNECT_RETRY
  UWORD l2_connect_retry;       /* MANUELL: RETRY fuer LINKAUFBAU       */
#endif

#ifdef AUTOROUTING
#define L_NOROUTE    0          /* Auto-Routing ausgeschaltet.          */
#define L_THENET     1          /* THENET.                              */
#define L_INP        2          /* INP.                                 */
#define L_FLEXNET    3          /* FLEXNET.                             */
#define L_INPFLEX    4          /* INP/THENET/FLEXNET (Vollmodus).      */

  UWORD poAuto;                 /* MANUELL: Fixed/Auto-Routing.         */
#endif /* AUTOROUTING */

#ifdef THENETMOD
  UWORD broadcast;              /* MANUELL: fuer Broadcast-Nodes Bake.  */
#endif /* THENETMOD */

#ifdef EAX25
  UWORD maxframe_eax;           /* MANUELL: maxframe bei Extended-AX.25 */
  UWORD eax_behaviour;          /* MANUELL: Verhalten des EAX.25        */
#endif

#ifdef SETTAILTIME
  UWORD tailtime;               /* MANUELL: tailtime                    */
#endif

  UWORD l1mode;                 /* Mode-Flags                           */
/* Masken fuer den L1-Mode-Parameter */
#ifdef PORT_SYNRONATION
#define MODE_ss  0x0008       /* Port Synronation                     */
#endif
#ifdef PORT_SUSPEND
#define MODE_l   0x0004        /* Port sperren                         */
#endif
#define MODE_m   0x0200         /* Multibaud Flag (Kanalkopplung)       */
#define MODE_e   0x0100         /* externer Takt Vanessa                */
#define MODE_d   0x0080         /* Vollduplex                           */
#define MODE_r   0x0040         /* externer RX-Takt                     */
#define MODE_t   0x0020         /* externer TX-Takt                     */
#define MODE_z   0x0010         /* NRZ statt NRZI bei SCC               */
#define MODE_c   0x0002         /* CRC bei KISS, DCD bei SER12          */
#define MODE_off 0x0001         /* Special: Wenn 1, Kanal abgeschaltet  */
#define CLR_L1MODE(port) portpar[port].l1mode = 0
#define SET_L1MODE(port,mode) portpar[port].l1mode |= mode

  UWORD l2mode;                 /* Mode-Flags                           */
/* Masken fuer den L2-Mode-Parameter */
#define MODE_am  0x8000         /* Maxframe-Automatik                   */
#define MODE_ds  0x4000         /* DAMA-Slave                           */
#define MODE_h   0x2000         /* MH Flag (an=HEARD fuehren/aus)       */
#define MODE_s   0x1000         /* SYSOP Flag (an=nur mit PRIV/aus)     */
#define MODE_a   0x0800         /* DAMA Flag (an/aus)                   */
#define MODE_x   0x0400         /* CTEXT Flag (an/aus)                  */

#ifdef EXPERTPARAMETER
  UWORD l2autoparam;            /* mode flags                           */
  /* Masken fuer Autoparameter */
#define MODE_apers  0x8000      /* Persistence-Automatik                */
#define MODE_aslot  0x4000      /* Slottime-Automatik                   */
#define MODE_aIRTT  0x2000      /* IRTT-Automatik                       */
#define MODE_aT2    0x1000      /* T2-Automatik                         */
#define MODE_aretry 0x0800      /* Retry-Automatik                      */
#endif

  UWORD l1_tx_timer;            /* Verzoegerung fuer Sendertastung      */
  int   dch;                    /* Dama-Kanal                           */

  int     major;
  #define NO_MAJOR 0
  int     minor;
  #define NO_MINOR (-1)

#ifdef DAMASLAVE
  UWORD  damaok;                /* != 0 ->Port ist DAMA-Slave           */
  UWORD  sendok;
#endif

#ifdef USERMAXCON                /* != 0 ->max. Anzahl simultaner Connects     */
  int    maxcon;                 /*        eines Users (fuer alle User gleich) */
#endif
} PORTINFO;

#define portenabled(port)  (portpar[port].major != NO_MAJOR)

#define dama(port)         (portpar[port].l2mode & MODE_a)
#ifdef DAMASLAVE
#define damaslave(port)    (portpar[port].l2mode & MODE_ds)
#define damaslaveon(port)  (damaslave(port) && (portpar[port].damaok != 0))
#endif
#define updmheard(port)    (portpar[port].l2mode & MODE_h)
#define sysoponly(port)    (portpar[port].l2mode & MODE_s)
#define ctextenabled(port) (portpar[port].l2mode & MODE_x)
#define automaxframe(port) (portpar[port].l2mode & MODE_am)

#define fullduplex(port)   (portpar[port].l1mode & MODE_d)
#define extclock(port)     (portpar[port].l1mode & MODE_e)
#define multibaud(port)    (portpar[port].l1mode & MODE_m)
#ifdef PORT_SUSPEND
#define port_suspend_enabled(port) (portpar[port].l1mode & MODE_l)
#endif
#ifdef PORT_SYNRONATION
#define port_synronation_enabled(port) (portpar[port].l1mode & MODE_ss)
#endif


typedef struct l1modetab {
  char ch;
  int  mode;
} L1MODETAB;

#define DCDFLAG 0x1             /* Kanal-Zustaende fuer iscd()          */
#define PTTFLAG 0x2
#define RXBFLAG 0x4
#define TXBFLAG 0x8

#define L1CRES 1                /* Requests fuer l1_ctl()               */
#define L1CCMD 2
#define L1CTST 3

typedef struct portcmd          /* Port-Befehls-Struktur                */
{
  const char *cmdstr;           /* Befehlsname                          */
  const char  cmdpar;           /* Zeiger auf Parameter           DB7KG */
} PORTCMD;

typedef struct portstat
{
  UWORD reset_count;   /* Anzahl der Resets ?                           */

  ULONG rx_bytes;      /* Anzahl bisher empfangener Bytes               */
  ULONG tx_bytes;      /* Anzahl bisher gesendeter Bytes                */

  ULONG last_rx;       /* letzter Zaehlerstand empfangene Bytes         */
  ULONG last_tx;       /* letzter Zaehlerstand gesendeter Bytes         */

  ULONG rx_baud;       /* Empfangsbaudrate (MAC)                        */
  ULONG tx_baud;       /* Sendebaudrate (MAC)                           */

  ULONG rx_overhead;   /* empfangener Protokolloverhead                 */
  ULONG tx_overhead;   /* gesendeter Protokolloverhead                  */

  UWORD invalid[4];    /* Invalid Frames + 1 in Reserve                 */
} PORTSTAT;

#define INV_FRAME(port, type) portstat[port].invalid[type]++
#define INVF_ADR 0              /* Addressfehler (L2)                   */
#define INVF_ALN 1              /* Addressfeldlaenge (L2)               */
#define INVF_CTL 2              /* Controlfeldfehler (L2)               */

typedef struct major {          /* Layer 1 Geraetestruktur              */
  char         *name;
  int         (*istome)(int, char *);
  void        (*init)(void);
  void        (*exit)(void);
  void        (*handle)(void);
  void        (*ctl)(int, int);
  WORD        (*dcd)(PORTINFO *);
  int         (*attach)(int, int, BOOLEAN);
  int         (*detach)(int);
  void        (*info)(int, int, MBHEAD *);
  void        (*timer)(UWORD);
#define HW_INF_IDENT  1
#define HW_INF_INFO   2
#define HW_INF_STAT   3
#define HW_INF_CLEAR  4
} MAJOR;

typedef struct devtable {       /* Layer 1 Geraeteliste                 */
  char        *name;
  int          major;           /* Major des Treiber nach der reg.      */
  int         (*reg_func)(void);
} DEVTABLE;
#define REGISTER_DEVICE(a,b) {a,0,b}

typedef struct suspend          /* Liste gesperrter Rufzeichen          */
{
  char  call[L2CALEN];          /* gesperrtes Rufzeichen                */
  UWORD port;                   /* Port auf dem gesperrt ist            */
  UBYTE okcount;                /* Anzahl zulaessiger Links             */
} SUSPEND;

typedef struct bake
{
  char beades[L2IDLEN];         /* Zielrufzeichen fuer Bake             */
  char beadil[L2VLEN+1];        /* Digiliste fuer Bake                  */
  WORD interval;                /* Zeitinterval in Minuten              */
  WORD beatim;                  /* Minuten-Zaehler                      */
  WORD telemetrie;              /* Telemetrie mitsenden                 */
  char text[80];                /* Bakentext                            */
} BEACON;

typedef struct
{
  char   call[L2IDLEN];         /* Rufzeichen incl. SSID                */
  char   via[L2VLEN+1];         /* VIA-Pfad                             */
  WORD   port;                  /* Port                                 */
#ifdef EAX25
  BOOLEAN eax;                  /* Flag: EAX-Verbindung                 */
#endif
  char   typ;
  char   nbrcal[L2IDLEN];
  NODE  *np;
  UID    uid;
} DEST;

typedef union max_buffer        /* sizeof() -> groesster Buffer         */
{
  MB               mb;
  MBHEAD           mbhead;
  MHEARD           mheard;
  USRBLK           usrblk;
  L2LINK           l2link;
} MAX_BUFFER;

#ifdef BUFFER_DEBUG
#define sizeof_MBDATA (sizeof(MAX_BUFFER)-sizeof(UBYTE)-sizeof(void*)*2)
#else
#define sizeof_MBDATA (sizeof(MAX_BUFFER)-sizeof(void*)*2)
#endif

#ifdef GRAPH
typedef struct tgraphpeak
{
  ULONG max;
  ULONG ave;
  ULONG min;
} TGRAPHPEAK;

typedef struct tgraphelement
{
  TGRAPHPEAK hour[GRAPH_STD_ELEMENTS];
  TGRAPHPEAK day [GRAPH_DAY_ELEMENTS];
  TGRAPHPEAK week[GRAPH_WEK_ELEMENTS];
} TGRAPHELEMENT;

#ifdef PORTGRAPH
typedef struct tportgraphpeak
{
  ULONG rx;
  ULONG tx;
} TPORTGRAPHPEAK;

typedef struct tportgraphelement
{
  TPORTGRAPHPEAK hour[GRAPH_STD_ELEMENTS];
  TPORTGRAPHPEAK day [GRAPH_DAY_ELEMENTS];
  TPORTGRAPHPEAK week[GRAPH_WEK_ELEMENTS];
} TPORTGRAPHELEMENT;
#endif

typedef struct tgraph
{
  BOOLEAN enabled;
  int hour_pos;                     /* Position des aktuellen Elementes */
  int day_pos;
  int week_pos;
  int hour_slot;      /* Anzahl der gespeicherten Werte im akt. Element */
  int day_slot;
  int week_slot;
  TGRAPHELEMENT circuit;
  TGRAPHELEMENT freebuffer;
  TGRAPHELEMENT l2link;
  TGRAPHELEMENT node;
  TGRAPHELEMENT roundpsec;
  TGRAPHELEMENT throughput;
#ifdef PORTGRAPH
  TPORTGRAPHELEMENT sabm[L2PNUM];
  TPORTGRAPHELEMENT disc[L2PNUM];
  TPORTGRAPHELEMENT dm[L2PNUM];
  TPORTGRAPHELEMENT info[L2PNUM];
  TPORTGRAPHELEMENT reject[L2PNUM];
  TPORTGRAPHELEMENT frmr[L2PNUM];
#endif
} TGRAPH;
#endif                                                   /* ifdef graph */

#ifdef MC68302
struct ftime
{
    unsigned ft_tsec:   5;
    unsigned ft_min:    6;
    unsigned ft_hour:   5;
    unsigned ft_year:   7;
    unsigned ft_month:  4;
    unsigned ft_day:    5;
};
#endif

#ifdef ALIASCMD
#define MAXALIASLEN 8
#define MAXALIASCMDLEN 16

typedef struct cmdalias {
  char alias[MAXALIASLEN + 1];      /* Alias */
  char cmd[MAXALIASCMDLEN + 1];     /* auszufuehrendes Kommando */
  struct cmdalias *next;            /* pointer to next entry */
} CMDALIAS;
#endif

#endif
/* End of include/typedef.h */
