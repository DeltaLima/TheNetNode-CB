#ifdef L1TCPIP

#define KISS_IPCONV            24                     /* IPConv KISS-Types    */
#define CVS_ID                  2                     /* Interface-ID IPCONV. */

#define CP_IPCONV              85

#define DEFAULT_IPCONV_PORT 13600                           /* Default Port.  */
#define IPCONV_TXT   "ipconv.txt"                     /* Fuer Login-Prompt.   */
#define IPCONVLOG    "ipconv.log"           /* Logdatei zum mit schreiben von */
                                            /* System- und Errormeldungen.    */
#define MAX_ROUTEN             10                       /* Max. 10 Eintraege. */


struct iptbl                                    /* TBL fuer IP-Convers-Links. */
{
  char    name[L2IDLEN];
  ULONG   ipaddr;
  UBYTE   hostname[64 + 1];
  UWORD   port;
  UWORD   l2port;
  BOOLEAN linkflag;
};

struct iptbl ip_tbl[MAX_ROUTEN];

/* Anzahl der aktuellen IP-CONVERS-Routen auf 0 setzen. */
extern int ip_tbl_top;

extern TRILLIAN GetContensCVS(char);          /* Aktuelle Zeichen auswerten.  */
extern void     ccpipconv(void); /* IPConv-Server Einstellung aendern/setzen. */
extern void     IPConvLogin(void);

#ifdef OS_IPLINK
extern int      IPConvSearch(char *);    /* Rufzeichen in der IPC-TBL suchen. */

extern void     IPConvAddTBL(char *,             /* IPConvers-Link eintragen. */
                             unsigned char  *,
                             struct hostent *,
                             unsigned short,
                             BOOLEAN);


extern int      IPConvConnect(char *,/* Ein Connect zum TCPIP-Nachbarn aufbauen. */
                              char *,
                              BOOLEAN);

extern BOOLEAN  IPConvGetName(WORD *,
                              char **,
                              BOOLEAN,
                              char *);

extern BOOLEAN  IPConvGetIP(WORD *,
                            char **,
                            unsigned char *);

extern int      IPConvIS(char *,
                         DEST *);

extern int      IPConvConnectOS(char *,
                                char *,
                                BOOLEAN);

extern void     IPConvDump(MBHEAD *);

extern BOOLEAN  IPConvDelTBL(char *);
#endif /* OS_IPLINK */
#endif /* L1TCPIP */

/* End of include/l1ipconv.h */
