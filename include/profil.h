
#define PWSETLEN   17                       /* Gesamtlaenge fuer PW-Optionen. */

typedef struct ProfTab
{
  const char *name;
  LHEAD       heardl;
  UWORD       max;
  UWORD       act;
} PROFTAB;

extern PROFTAB proftab;


typedef struct  prfheard
{
  struct prfheard *next;               /* doppelt verkettete Liste             */
  struct prfheard *prev;
#ifdef BUFFER_DEBUG
  UBYTE          owner;                /* Muss an 9. Bytestelle stehen         */
#endif
  char           name[L2IDLEN + 1];    /* Rufzeichen.                          */
  char           nick[NAMESIZE + 1];   /* Nickname.                            */
  char           setpw[PWSETLEN + 1];  /* Aktiviere Passwortabfrage auf den    */
                                       /* gewuenschten Port.                   */
  char           passwd[80 + 1];       /* Passwort.                            */
} PRFHEARD;

typedef struct profcmd          /* Profil-Struktur              */
{
  const char *str;              /* Befehlsname                  */
  const char  par;              /* Zeiger auf Parameter         */
} PROFCMD;

extern void      InitProfilTAB(void);                    /* TBL Initialisieren.               */
extern void      SaveProfil(void);                       /* TBL-Liste auf Festplatte sichern. */
extern void      ProfilService(CONNECTION *);            /* Aktualisiere Profil-Daten.        */

extern void      LoadTableProfil(PROFTAB *);
extern void      LoadTableProfil(PROFTAB *);


extern void      ProfilLoad(CONNECTION *);
extern PRFHEARD *LookupProfil(PROFTAB *, const char *);
extern PRFHEARD *AddProfil(PROFTAB *, const char *);
extern void      ccpprofil(void);

extern PRFHEARD *SearchProfil(const char *);
extern BOOLEAN   SearchPasswdProfil(void);
extern void      SendPasswdStringProfil(void);

#define DEFAULT_PASS "12345678901234567890123456789012345678901234567890123456789012345678901234567890"
extern BOOLEAN   CheckPasswd(void);
extern TRILLIAN  GetPasswd(WORD *, char **, int, char *);
extern void      PasswdKodierung(const char *, char *);
extern void      PasswdDekodierung(const char *, char *);


#ifdef CONVNICK
extern void    UpdateNickProfil(PROFTAB *, PRFHEARD *, const char *, const char *);
extern BOOLEAN GetNickname(CONNECTION *);
#endif /* CONVNICK */

/* End of include/profil.h. */
