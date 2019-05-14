/************************************************************************/
/*                                                                      */
/* Modifizierungen von DAA531 Oliver Kern                               */
/*                     DAC922 Stefan                                    */
/*                                                                      */
/************************************************************************/


/************************************************************************/
/*                                                                      */
/* AFU-Callcheck deaktiviert                                            */
/*                                                                      */
/************************************************************************/
#define CALLCHECK


#if defined(__WIN32__) || defined(__LINUX__)
/************************************************************************/
/*                                                                      */
/* Port oeffnen / schliessen.                                           */
/*                                                                      */
/************************************************************************/
#define ATTACH


/************************************************************************/
/*                                                                      */
/* AX25IP-Modul.                                                        */
/*                                                                      */
/* Dynamische DNS Verwalten.                                            */
/* Parameter Loglevel per Console einstellbar.                          */
/* Keine Frame verarbeiten beim einlesen der tnn179.tnb.                */
/* Zusaetzliche Meldungen bei ein - und  austragen von AXIPR-Routen.    */
/*                                                                      */
/************************************************************************/
#define AXIPR_UDP
#ifdef AXIPR_UDP

/************************************************************************/
/*                                                                      */
/* AXIPR-Liste in HTML-Format schreiben.                                */
/* Datei: rstat.html, rstat.css.                                        */
/* Verzeichnis wird in der TNN179.PAS angepasst.                        */
/*                                                                      */
/************************************************************************/
#define AXIPR_HTML

#endif /* AXIPR_UDP */
#endif /* WIN32/LINUX */


/************************************************************************/
/*                                                                      */
/* Allgemeiner Direktiv fuer alle Modifizierungen und Fixe.             */
/*                                                                      */
/************************************************************************/
#define MODIFIGLOBAL
#ifdef MODIFIGLOBAL

#define FUNCFIX              /* Fix in einzelnen funktionen.             */
#ifdef FUNCFIX

#define CONVTOPIC_FIX        /* Fix in funktion send_topic.              */

#define CONVNICK_FIX         /* Fix in funktion nickname_command.        */

#define MHEAX_LINKFIX        /* Variable eax_link wurde nicht immer      */
                             /* gesetzt, behoben.                        */

#define MHRXTXBYTESFIX       /* Nur "Info-Bytes" werden gezaehlt, der    */
                             /* AX25-Header wird abgezogen.              */

#define L4NOBAKE             /* Steht der Routing-TYP noch nicht fest,   */
                             /* senden wir zum Nachbarn noch keine Bake  */
                             /* irgendeiner Art.                         */

#define SEND_ASYNC_RESFIX    /* Null-Zeichen setzen - sicher ist sicher. */

#define FLEX_TX_RTT_FIX      /* Kein Doppelte Laufzeitmessung senden.    */

#define LOCAL_ROUTEFIX       /* hiermit setzen wir die LOCAL-Time runter */
                             /* von 400ms auf 10ms. Leider ist es vorge- */
                             /* kommen, das eine LOCAL-ROUTE im Netz bes-*/
                             /* ser geroutet wurde, als die von unseren  */
                             /* Node.                                    */

#define FLEXTIMEFIX          /* Mess-Timeout erhoeht.                    */

#define FLEX_ROUTINGFIX      /* Flex-GATE deaktivieren.                  */

#define AUTOBINAERFIX        /* Fehler in "Load" behoben. (WPP)          */
                             /* Paxcon funktioniert nun auch ab mh02.    */

#define SAVEPARAMFIX         /* Parameter in die TNN179.TNB speichern.   */

#define CONFPATHFIX          /* Der angegebene TEXTPATH in TNN179.PAS    */
                             /* auch CONFPATH zuweisen !!! Wichtig, wenn */
                             /* Verzeichnisse wie MSG und interne        */
                             /* Prozesse auf CONFPATH zugreifen.         */

#define MSGPATHFIX           /* Der MSG Path wird in der funktion main.c */
                             /* definiert von confpath. Nun ist es aber  */
                             /* moeglich, das der confpath, wo TNN.EXE   */
                             /* gestartet wurde, nicht stimmt, weil der  */
                             /* TEXTPATH in der TNN179.PAS ganz anders   */
                             /* definiert wurde. Als muessen wir das     */
                             /* korrigieren.                             */

#define PARMS_PORTMOD        /* den PORT Befehl mit dem PARMS Befehl     */
                             /* tauschen. Mit dem Befehl P werden die    */
                             /* Portparamter angezeigt.                  */
                             /* (Mehrfach gewuenscht!)                   */

/*************************************************************************/
/*                                                                       */
/* UI-Frames von einem Port auf einen anderen Port durchreichen.         */
/*                                                                       */
/* Aenderungen: 1. Digipeater-Call -> TNN-Call                           */
/*              2. Auch auf RX-Port wo die Bake gehoert wurde senden.    */
/*                                                                       */
/*************************************************************************/
#define UIDIGIMOD

/*************************************************************************/
/*                                                                       */
/* Gibt es keinen Weg zum Ziel (n call), Fehlermeldung ausgeben.         */
/*                                                                       */
/*************************************************************************/
#define NONODESFIX


/*************************************************************************/
/*                                                                       */
/* Flexnet-Ziele -> nur noch in der Destinations-Liste angezeigen.       */
/* Nodes-Ziele   -> nur noch in der Nodes-Liste angezeigen.              */
/*                                                                       */
/*************************************************************************/
#define SHOW_DESTNODES


/*************************************************************************/
/*                                                                       */
/* Nach erneuten SABM, Zeitmessung starten.                              */
/*                                                                       */
/*************************************************************************/
#define RTTSTART_MOD


/*************************************************************************/
/*                                                                       */
/* Standard-Baken Text deaktiviert.                                      */
/* TheNetNode (Win32) (CB), 1.79cb52 (TEST:CB1GRH)                       */
/*                                                                       */
/* Kann mit //BAKEFIX wieder aktiviert werden.                           */
/*                                                                       */
/*************************************************************************/
#define BAKEFIX


/*************************************************************************/
/*                                                                       */
/* UI-Frame mit POLL-Flag. .                                             */
/*                                                                       */
/*************************************************************************/
#define UIPOLLFIX

#endif /* FUNCFIX */


/*************************************************************************/
/*                                                                       */
/* Aktiviert das Auto-Routing.                                           */
/*                                                                       */
/*************************************************************************/
#define AUTOROUTING
#ifdef AUTOROUTING
#define AUTO_ROUTE  0        /* AutoRoute, keine Speicherung in TNN179.TNB    */
#define FIXED_ROUTE 1        /* FesteRoute, wird gespeichert in TNN179.TNB    */
#endif /* AUTOROUTING */


/*************************************************************************/
/*                                                                       */
/* Alias in kleinbuchstaben umwandeln. Wird ein Alias in GROSSBUCHSTABEN */
/* gespeichert, wird dieser Linkeintrag beim naechsten start NICHT       */
/* eingelesen !!!.                                                       */
/*                                                                       */
/*************************************************************************/
#define ALIASSAVEMOD


/*************************************************************************/
/*                                                                       */
/* BEACON/BAKE .                                                         */
/*                                                                       */
/* Beacon-Bake (Metric) erweitert auf 0..3.                              */
/*                                                                       */
/* Ausgabe (BE 0 60 3 IDNET):                                            */
/* DNO531 to STATUS UI^ pid F0 - 19.09.03 23:44:37                       */
/* Links: 31, Convers: 25, Dest/Nodes: 267, Runtime: 31d,15h             */
/* DNO530:39   CB0DLN:44                                                 */
/*                                                                       */
/* Beschreibung der Bake im einzelnen:                                   */
/* Links: 32       -> aktuelle L2-Links im Knoten.                       */
/* Convers: 25     -> aktuelle anzahl user im Convers.                   */
/* Dest/Nodes: 267 -> aktuelle anzahl der Destination und Nodes.         */
/* Runtime: 31d.15 -> Laufzeit der Knotensoftware.                       */
/*                                                                       */
/*************************************************************************/
#define BEACON_STATUS


/*************************************************************************/
/*                                                                       */
/* Modifizierung am Modul Convers                                        */
/*                                                                       */
/*************************************************************************/
#define CONVMOD
#ifdef CONVMOD

#define CONV_CHECK_USER      /* Pruefung auf Doppel-Login.               */

#define CONV_TOPIC           /* Topic-Call sichern.                      */

#define CONVERS_HOSTNAME     /* Convers-Hostname aendern. Es stehen 15   */
                             /* Zeichen zur Verfuegung.                  */

#define CONVERS_CTEXT        /* In der Datei conversd.xhf unter @@CTEXT  */
                             /* kann man einen zusaetzlichen CTEXT zu-   */
                             /* zusammen basteln.                        */

#define CONVERS_LINKS        /* Aenderungen an den Convers-Links.        */

#define CONVERS_SYSINFO      /* System-Ausgabe (wunsch DAD213)           */

#define CONVERS_NO_NAME_OK   /* Deaktivierung funktion name_ok.          */

#define CONVERS_USERANZAHL   /* Gesamtanzahl der Convers-User im Kanal.  */


#endif /* CONVMOD */

/*************************************************************************/
/*                                                                       */
/* Modifizierung am Modul CONNECT                                        */
/*                                                                       */
/*************************************************************************/
#define CONNECTMOD
#ifdef CONNECTMOD
/*************************************************************************/
/*                                                                       */
/* User hat Port angegeben, dann gehen wir ueber L2.                     */
/*                                                                       */
/*************************************************************************/
#define CONNECTMOD_GOPORT

/*************************************************************************/
/*                                                                       */
/* Status-Meldungen "*** connected to" bzw. "*** reconnected to"         */
/* eingebaut.                                                            */
/*                                                                       */
/*************************************************************************/
#define CONNECTMOD_MSG

/*************************************************************************/
/*                                                                       */
/* Auch wenn der Nachbar nicht erreichbar, wird ein L2-Link aufgebaut.   */
/*                                                                       */
/*************************************************************************/
#define CONNECTMOD_NODE_AVAI

/*************************************************************************/
/*                                                                       */
/* Einstiegsknoten setzen.                                               */
/* Das Rufzeichen was weitergeleitet wird, ist der Node, wo sich der     */
/* User eingeloggt hat. Ist eine weitere Anpassung an Flexnet und        */
/* Baycom-Systemen.                                                      */
/*                                                                       */
/*************************************************************************/
#define CONNECTMOD_SET_NODE

#endif /* CONNECTMOD */


/*************************************************************************/
/*                                                                       */
/* Connect-Zeit in Flexnet-Stil ausgeben.                                */
/*  1 CB0RIE    CB1GLA    IXF  0  0  0   13 136  138     96   13h,21m  - */
/*                                                            =======    */
/*************************************************************************/
#define CONNECTTIME


/*************************************************************************/
/*                                                                       */
/* System sauber runterfahren.                                           */
/* Erweiterte Fehlersuche                              .                 */
/* (Hat mir schon einige Arbeit erspart.)                                */
/*                                                                       */
/*************************************************************************/
#define DEBUG_MODUS


/*************************************************************************/
/*                                                                       */
/* Editor erweitert. (ist noch nicht eingebaut)                          */
/*                                                                       */
/*************************************************************************/
#define EDITOR


/*************************************************************************/
/*                                                                       */
/* Damit setzt man einmalig das Consolen-Mycall.                         */
/* Speicherung erfolgt in der TNN179.TNB. Bei jeden neustart wird das    */
/* neue Consolen-Mycall gesetzt.                                         */
/*                                                                       */
/*************************************************************************/
#define HOSTMYCALL


/*************************************************************************/
/*                                                                       */
/* Modifizierung am Modul IPROUTE                                        */
/*                                                                       */
/* IP-Adresse 0.0.0.0 unter ARP sperren.                                 */
/* Default IP-Adresse 0.0.0.0 unter IPR setzen.                          */
/* Ist eine default-route gesetzt, werden alle NICHT definierten         */
/* IP-Adressen an diese default-route geschicht.                         */
/*                                                                       */
/* default-route unter IPR eintragen:                                    */
/* 0.0.0.0 + NETROM 192.168.100.10                                       */
/* Wichtig ist die Gateway Adresse !!!                                   */
/*                                                                       */
/*************************************************************************/
#define IPROUTEMOD


/*************************************************************************/
/*                                                                       */
/* Ist der L4-Timeout abgelaufen, wird die Verbindung getrennt.          */
/* Vor der Trennung schicken wir den User eine Meldung.                  */
/*                                                                       */
/*************************************************************************/
#define L4TIMEOUTAUSGABE


/*************************************************************************/
/*                                                                       */
/* L4-User killen (ccpkill kann nur L2).                                 */
/*                                                                       */
/*************************************************************************/
#define L4KILL


/*************************************************************************/
/*                                                                       */
/* Modifizierung am Modul (L)INKS                                        */
/*                                                                       */
/*************************************************************************/
#define LINKSMOD
#ifdef LINKSMOD
/*************************************************************************/
/*                                                                       */
/* Syntax Link ein/austragen fuer Sysop ausgeben.                        */
/*                                                                       */
/*************************************************************************/
#define LINKSMODSYNTAXFIX

/*************************************************************************/
/*                                                                       */
/* Stations-Beschreibung.unter Links-Liste.                              */
/*                                                                       */
/*************************************************************************/
#define LINKSMODINFO

/*************************************************************************/
/*                                                                       */
/* Zusaetzliche Meldungen bei ein/austragen von Links.                   */
/*                                                                       */
/*************************************************************************/
#define LINKSMOD_MSG

/*************************************************************************/
/*                                                                       */
/* Wenn kein Routing-TYP angegeben, dann  gibt es eine Fehlermeldung.    */
/* Der Routing-TYP muss korrekt gesetzt werden !!!                       */
/*                                                                       */
/*************************************************************************/
#define LINKSMODROUTINGTYP

/*************************************************************************/
/*                                                                       */
/* Routing-TYP LOCAL ohne Messung, ohne Weiterleitung                    */
/* und ohne Raute '#' im Alias.                                          */
/*                                                                       */
/* Routing-Typ "L-"                                                      */
/*                                                                       */
/* Routing-TYP LOCAL ohne Messung, ohne Weiterleitung und versteckt.     */
/* Die Route ist nur fuer den Sysop sichtbar.                            */
/*                                                                       */
/* Routing-Typ "L#"                                                      */
/*                                                                       */
/*************************************************************************/
/*#define LINKSMOD_LOCALMOD */

#endif /* LINKSMOD */


/*************************************************************************/
/*                                                                       */
/* Jeder Link-Nachbar mit Routing-Protokoll bekommt keinen CTEXT von uns.*/
/*                                                                       */
/*************************************************************************/
/*#define NOCTEXT */


/*************************************************************************/
/*                                                                       */
/* Modifizierung am Modul MAKROS                                         */
/*                                                                       */
/*************************************************************************/
#define MAKROS
#ifdef MAKROS
/*************************************************************************/
/*                                                                       */
/* Echte Useranzahl ausgeben.                                            */
/* Dafuer Modifiziere ich das Makro "%u", da ich das eh fuer unnutze     */
/* halte. Wer es  im orignal zustand haben will, deaktiviert einfach     */
/* //#define MAKRO_USER                                                  */
/*                                                                       */
/*************************************************************************/
#define MAKRO_USER

/*************************************************************************/
/*                                                                       */
/* Makros in TXT-Datei (info.txt/map.txt) im Verzeichnis TEXTCMD         */
/* einsetzbar.                                                           */
/*                                                                       */
/*************************************************************************/
#define MAKRO_FILE

/*************************************************************************/
/*                                                                       */
/* Makro %v, Loginstring (TNN V1.79 (Win32).                             */
/* Code von DAC922 Stefan.                                               */
/*                                                                       */
/*************************************************************************/
#define MAKRO_NOLOGINSTR

#endif /* MAKROS */


/*************************************************************************/
/*                                                                       */
/* Modifizierung am Modul (MH)EARD                                       */
/*                                                                       */
/* Modifizierung der MH-Liste (Flexnet-Stil)                             */
/* Einzelne Port Listen.                                                 */
/* MH-Listenlaenge einstellbar.                                          */
/* Verschiedene MH-Listen Formate.                                       */
/*                                                                       */
/*************************************************************************/
#define MH_LISTE


/*************************************************************************/
/*                                                                       */
/* Modifizierung am Modul (R)ORTPARAMETER                                */
/*                                                                       */
/*************************************************************************/
#define PORTPARAMETER
#ifdef PORTPARAMETER
/*************************************************************************/
/*                                                                       */
/* Manuelle Portparameter.                                               */
/*                                                                       */
/* Paketlaenge, Persistance, Slottime, IRTT(Frack), T2, T3, Retry.       */
/*                                                                       */
/*************************************************************************/
#define PORT_MANUELL

/*************************************************************************/
/*                                                                       */
/* IPOLL-Frame (von TheFirmware uebernommen)                             */
/*                                                                       */
/* Manuelle Portparamter:                                                */
/* Paketlaenge: PO 0 I=128                                               */
/* (Ab welcher groesser soll IPOLL-Frame zuschlagen.)                    */
/*                                                                       */
/* Retry PO 0 IR=3                                                       */
/* (Wieviel mal das Frame Wiederholt werden soll).                       */
/*                                                                       */
/*************************************************************************/
#define IPOLL_FRAME

/*************************************************************************/
/*                                                                       */
/* Ist die L2_CONNECT_TIME abgelaufen, duerfen wir den Linkpartner Rufen.*/
/* Nach abgelaufener L2_CONNECT_RETRY wird die L2_CONNECT_TIME wieder    */
/* hochgesetzt. Damit erreichen wir immer eine gewisse Pause beim        */
/* LINKAUFBAU, wenn unsere Nachbar-Station grade mal nicht online ist.   */
/*                                                                       */
/*************************************************************************/
#define PORT_L2_CONNECT_TIME

/*************************************************************************/
/*                                                                       */
/* Wie viel mal soll nach dem Linkpartner gerufen werden bis zur         */
/* naechsten Pause.                                                      */
/*                                                                       */
/*************************************************************************/
#define PORT_L2_CONNECT_RETRY

/*************************************************************************/
/*                                                                       */
/* einzelne Ports sperren. Mode Parameter "l" (kleines "L")              */
/* Beispiel: PO 0 mode=1200l                                             */
/* Es duerfen nur LINK-Partner in der LINKS-LISTE Connecten, andere      */
/* bekommen Stationen bekommen eine kleine Meldung "INTERLINK" bzw.      */
/* man kann die Meldung selber in einer Datei festlegen (LOCK.TXT).      */
/*                                                                       */
/*************************************************************************/
#define PORT_SUSPEND

#endif /* PORTPARAMETER */


/*************************************************************************/
/*                                                                       */
/* Verhindert das gleichzeitige senden auf  mehreren Port's.             */
/* Mode-Parameter "s". (po 0 mode=1200s)                                 */
/*                                                                       */
/* Noch nicht vollstaendig.                                              */
/*                                                                       */
/*************************************************************************/
#define PORT_SYNRONATION


/*************************************************************************/
/*                                                                       */
/* Proxyfunktion (noch zu optimieren).                                   */
/*                                                                       */
/*************************************************************************/
#define PROXYFUNC


/*************************************************************************/
/*                                                                       */
/* Modifizierung am Modul (R)OUTES                                       */
/*                                                                       */
/*************************************************************************/
#define ROUTESMOD
#ifdef ROUTESMOD
/*************************************************************************/
/*                                                                       */
/* L3RTT-Anzeige / 10                                                    */
/*                                                                       */
/*************************************************************************/
#define ROUTESMOD_L3RTTSHOW

/*************************************************************************/
/*                                                                       */
/* Alle Nodes die ueber den Linkpartner geroutet werden anzeigen.        */
/* (Beispiel: R CB0GLA)                                                  */
/*                                                                       */
/*************************************************************************/
#define ROUTESMODVIANODES

#endif /* ROUTESMOD */


/*************************************************************************/
/*                                                                       */
/* System-Verzeichnisse anlegen.                                         */
/*                                                                       */
/*************************************************************************/
#define SETPATH


/*************************************************************************/
/*                                                                       */
/* Sprachauswahl deutsch / englich fuer System und Convers-Meldungen.    */
/*                                                                       */
/* Ist erweiterbar fuer weitere Sprachen.                                */
/*                                                                       */
/*************************************************************************/
#define SPEECH


/*************************************************************************/
/*                                                                       */
/* Sysop-Passwort im laufenden Betrieb aendern.                          */
/* Das neue Passwort wird in der TNN179.TNB gespeichert und beim         */
/* naechsten neustart eingelesen.                                        */
/*                                                                       */
/* SYNTAX: PASS dasistmeinpasswort                                       */
/* (Passwortstring muss genau 80 Zeichen haben)                          */
/*                                                                       */
/*************************************************************************/
#define SYSOPPASSWD


/*************************************************************************/
/*                                                                       */
/* TCPIP-Service                                                         */
/*                                                                       */
/* Interner TCP-Stack.                                                   */
/* TCPIP L1-Layer.                                                       */
/* OS-Stack (Win32 / Linux)                                              */
/* TELNET-Server                                                         */
/* HTTPD-Server                                                          */
/* IPCONVER-Server                                                       */
/* TNN <IP-LINK> SAUPP.                                                  */
/* IRC-Server (noch nicht vollstaendig!)                                 */
/*                                                                       */
/*************************************************************************/
#define TCP_STACK
#define L1TCPIP
#define OS_STACK
#define L1TELNET
#define L1HTTPD
#define L1IPCONV
/* #define L1IRC */
#define OS_IPLINK



/*************************************************************************/
/*                                                                       */
/* Modifizierung am Routing-TYP THENET.im zusammenhang mit X1J4-Knoten.  */
/*                                                                       */
/* L4TIMEOUT                  L4-Link no activity Timer (THENET-TYP).    */
/*                            Ist der Timer abgelaufen, wird der Link    */
/*                            getrennt, aber die Connects die ueber      */
/*                            diesen Link laufen werden nicht getrennt.  */
/*                            Bei Aktivitaet wird der Link wieder        */
/*                            automatisch aufgebaut.                     */
/*                                                                       */
/* L4QUALI                    Qualitaet einer Route setzen (THENET).     */
/*                            (Beispiel: R CB0GLA QUAL=128)              */
/*                                                                       */
/* NOROUTE                    Route "#" im Alias zulassen. Ist ueber     */
/*                            den L4PAR 7 0..1 einstellbar.              */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
#define THENETMOD


/*************************************************************************/
/*                                                                       */
/* Modifizierung am Modul "TIMER".                                       */
/*                                                                       */
/*************************************************************************/
#define TIMERMOD
#ifdef TIMERMOD
/*************************************************************************/
/*                                                                       */
/* Max. SRTT-Wert auf 250 festlegen.                                     */
/*                                                                       */
/*************************************************************************/
#define SRTTMAXMOD 250

/*************************************************************************/
/*                                                                       */
/* SRTT-Wert updaten bei schnellen Links.                                */
/*                                                                       */
/*************************************************************************/
#define T1TIMERMOD

/*************************************************************************/
/*                                                                       */
/* Anfangswert fuer Smoothed Round Trip Timer setzen.                    */
/*                                                                       */
/*************************************************************************/
#define SETISRTTMOD

#endif /* TIMERMOD */


/*************************************************************************/
/*                                                                       */
/* Alle TCPIP-Interface bei der Auflistung nicht mit angezeigt, da ein   */
/* connect auf den Port nicht moeglich ist.                              */
/* (Node / User unknown! Please specify port, if DD0DSD is a User:)      */
/*                                                                       */
/*************************************************************************/
#define TCPIP_NO_SHOW


/*************************************************************************/
/*                                                                       */
/* L2-Timeout Manuell setzen.                                            */
/*                                                                       */
/* PA Timeout 60..54000.                                                 */
/*                                                                       */
/*************************************************************************/
#define TIMEOUT_MANUELL


/*************************************************************************/
/*                                                                       */
/* USER-PROFIL .                                                         */
/*                                                                       */
/* Persoenliche Einstellungen, z.B. Passwort, Nickname verwalten.        */
/*                                                                       */
/*************************************************************************/
#define USERPROFIL


/*************************************************************************/
/*                                                                       */
/* USER duerfen Monitoring.                                              */
/*                                                                       */
/*************************************************************************/
#define USER_MONITOR


/*************************************************************************/
/*                                                                       */
/* Modifizierung der Userausgabe.                                        */
/*                                                                       */
/*************************************************************************/
#define USER_AUSGABE


#endif /* MODIFIGLOBAL */
