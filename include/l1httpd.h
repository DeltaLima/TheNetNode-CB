#ifdef L1TCPIP

#define L1HTTPD                                          /* HTTPD-Interface.  */

#define KISS_HTTPD             23                      /* HTTPD KISS-Types    */
#define HTP_ID                  1                      /* Interface-ID HTTPD. */

#define DEFAULT_HTTPD_PORT  10080                     /* Default Port 10080.  */
#define HTTPDLOG      "httpd.log"           /* Logdatei zum mit schreiben von */
                                            /* System- und Errormeldungen.    */

#define HEADLEN 1024                            /* Maximale HTML-Headerlaenge */

#define TCP_NULL    0              /* leer/dummy                        */
#define TCP_URI     1              /* html laden.                       */
#define TCP_BUSY    2              /* Sender Busy.                      */
#define TCP_CMD     3              /* Befehl ausfuehren                 */
#define TCP_BIN     4              /* Binaertransfer  .                 */

#define MIME_JPG "image/jpeg"
#define MIME_GIF "image/gif"
#define MIME_TXT "text/html"
#define MIME_BIN "application/x-binary"
#define MIME_WAV "audio/wav"
#define MIME_MP3 "audio/x-mpeg"

extern TRILLIAN GetContensHTP(char);          /* Aktuelle Zeichen auswerten.  */
extern void     ccphttpd(void);   /* HTTPD-Server Einstellung aendern/setzen. */
extern void     TcpipHttpd(MBHEAD *);
extern void     putv( MBHEAD *, int);              /* Konvertierung IBM/HTML. */
extern BOOLEAN  load_uri(void);                  /* Externe HTML-Datei laden. */
extern void     PutHtmlEnd(void);

#endif /* L1TCPIP */

/* End of include/l1httpd.h */
