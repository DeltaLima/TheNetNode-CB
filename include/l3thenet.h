
extern PARAM L4partab[];
extern int   L4partablen;

extern UWORD obcini;   /* Anfangswert fuer Knoten Lebensdauer                 */
extern UWORD obcbro;   /* min. Restlebensdauer fuer Rundspruch                */

/* L4TIMEOUT */
extern UWORD L4Timeout;                         /* L4-Link no activity Timer. */

/* NOROUTE */
extern UWORD NoRoute;  /* 0 = Link-Nachbar mit # im Alias NICHT zulassen.     */
                       /* 1 = Link-Nachbar mit # im Alias zulassen.           */
                       /* Parameter 0 ist standard.                           */

extern UWORD   l4_beta3;              /* BUSY/REQ-TIMEOUT (T3) = SRTT * BETA3 */

typedef struct L4param                /* L4-Parameter                         */
{
  UWORD *paradr;                      /* Adresse des Parameters               */
  UWORD  minimal;                     /* Minimalwert                          */
  UWORD  maximal;                     /* Maximalwert                          */
  const char *parstr;                 /* Name des Parameters                  */
} L4PARAM;

extern           void ccpL4par(void);         /* L4-Parameter setzen/aendern. */
extern           void dump_l4parms(MBHEAD *); /* Param auf Festplatte sichern.*/
                                                        /* L4-Parameter unter */
extern        BOOLEAN RoutesL4Para(MBHEAD *, PEER *);       /* ROUTES aendern.*/
extern           void BroadcastBakeClear(PEER *); /* Broadcast-Timer zurueck- */
                                                  /* setzen.                  */
extern        BOOLEAN OBSinitTimer(PEER *pp); /* Alterungs-Zaehler minimieren.*/
extern           void BroadCastBake(void);    /* Broadcast-Nodes Bake senden. */
extern           void bcast(MBHEAD **, char *, UWORD);

/* L4TIMEOUT */
extern PEER          *SearchTHENET(void); /* Suche THENET-Typ.                */
extern unsigned short SetL4Timeout(void); /* Wenn THENET L4-Timeout setzen,   */
                                       /* ansonsten normal L2-Timeout setzen. */

/* L4QUALI */
extern           void dump_routes(MBHEAD *); /* Qualitaet einer Route (nur    */
                                             /* vom Typ THENET) speichern.    */

/* End of include/l3thenet.h. */
