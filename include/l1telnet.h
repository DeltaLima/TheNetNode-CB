#ifdef L1TCPIP

#define L1TELNET                                        /* TELNET-Interface.  */

#define KISS_TELNET            22                     /* Telnet KISS-Types    */
#define TEL_ID                  0                     /* Interface-ID TELNET. */

#define DEFAULT_TELNET_PORT 10023                     /* Default Port 10023.  */
#define TELNET_TXT   "telnet.txt"                     /* Fuer Login-Prompt.   */
#define TELNETLOG    "telnet.log"           /* Logdatei zum mit schreiben von */
                                            /* System- und Errormeldungen.    */


extern TRILLIAN GetContensTEL(char);          /* Aktuelle Zeichen auswerten.  */
extern void     ccptelnet(void); /* TELNET-Server Einstellung aendern/setzen. */

#endif /* L1TCPIP */

/* End of include/l1telnet.h */
