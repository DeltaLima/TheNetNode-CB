#include "tnn.h"
#ifdef USERPROFIL

#include "prof_pwd.c"
#include "prof_nick.c"

PROFTAB proftab;

static TRILLIAN GetName(WORD *, char **, int, char *);

/* Initialisiere User-Profil. */
void InitProfilTAB(void)
{
  proftab.act  = 0;               /* Aktuelle Eintraege.         */
  proftab.max  = 100;             /* Max. Eintraege.             */
  proftab.name = "PROFIL.TAB";    /* Dateiname setzen.           */
  inithd(&proftab.heardl);        /* Initialisiere Profil-Liste. */
  LoadTableProfil(&proftab);      /* Lade Profil-TBL ein.        */
}

#define PO_NAME   1
#define PO_NICK   2
#define PO_SETPW  3
#define PO_PASSWD 4

PROFCMD profcmd[] =
{
  {"NAME",   PO_NAME   },
  {"NICK",   PO_NICK   },
  {"SETPW",  PO_SETPW  },
  {"PASSWD", PO_PASSWD },
  {NULL,     0         }
};

void call3str(char *id1, const char *id2)
{
  int    i,
         j = 0;

  /* Frischen Buffer besorgen. */
  memset(id1, 0, sizeof(id1));

  /* Rufzeichen ohne SSID einlesen. */
  for (i = 0; i < L2IDLEN - 1; i++, j++)
  {
    /* Kein Zeichen. */
    if (id2[i] == FALSE)
    {
      /* Rest fuellen wir mit Leerzeichen aus. */
      while (i++ < L2IDLEN - 1)
        id1[j++] = ' ';

      break;
    }

    /* Zeichen setzen. */
    id1[j] = toupper(id2[i]);
  }

  /* Nullzeichen setzen. */
  id1[L2IDLEN] = 0;
}

/* Lade Profil-TBL "profil.tab" ein. */
void LoadTableProfil(PROFTAB *nic)
{
  FILE     *fp;
  PRFHEARD *nicp;
  PROFCMD  *tbl;
  char      file[128],
            buf[80];
  char     *ptr,
           *bps;
  WORD      cnt;
  WORD      bcs,
            found;
  int       Len = PWSETLEN,
            i   = 0;
  char      passwort[80 + 1];
  int       Error;

  /* Path setzen. */
  strcpy(file, confpath);
  /* Dateiname setzen. */
  strcat(file, nic->name);

  /* Oeffne Datei. */
  if ((fp = xfopen(file, "rt")) != NULL)
  {
    /* Zeile einlesen (Max. 120 Zeichen). */
    while (fgets(file, 120, fp) != NULL)
    {
      /* Buffer beseorgen. */
      nicp = (PRFHEARD *)allocb(ALLOC_USEPROF);
      /* Buffer uebergeben. */
      ptr = file;
      /* Zeilenlaenge uebergeben. */
      cnt = strlen(ptr) - 1;

      /* Frischer Buffer. */
      memset(nicp->name, 0, sizeof(nicp->name));

      do
      {
        *buf = NUL;
        bps = buf;
        if (skipsp(&cnt, &ptr))
        {
          while (*ptr && isalnum(*ptr))
          {
            *bps++ = toupper(*ptr++);
            --cnt;
          }
          *bps = NUL;
        }
        if (!*buf)
        {
          cnt = 0;
          break;
        }
        bcs = (WORD)strlen(buf);

        for (tbl = profcmd, found = 0; !found && tbl->str != NULL; ++tbl)
        {
          if (!strncmp(tbl->str, buf, (size_t)bcs))
            found = tbl->par;
        }

        /* Befehl gefunden. */
        if (found)
        {
          switch (found)
          {
            /* Rufzeichen einlesen. */
            case PO_NAME :
              ptr++;
              cnt--;

              /* Pruefe, ob Rufzeichen korrekt ist. */
              if (getcal(&cnt, &ptr, TRUE, nicp->name) == ERRORS)
                /* Ungueltiges Rufzeichen. */
                nicp->name[0] = 0;

              break;


            /* Nickname einlesen. */
            case PO_NICK :
              ptr++;
              cnt--;

              /* Pruefe, ob Nickname korrekt ist. */
              if (GetName(&cnt, &ptr, NAMESIZE, nicp->nick) == ERRORS)
                /* Ungueltiger Nickname. */
                nicp->name[0] = 0;

              nicp->name[L2IDLEN] = 0;
              break;


            /* Passwort-Einstellungen einlesen. */
            case PO_SETPW :
              ptr++;
              cnt--;

              while (Len--)
              {
                /* Bei leerstelle, */
                if (  (*ptr == ' ')
                    /* keine 0 */
                    ||((*ptr != '0')
                    /* oder kein 1 */
                    &&(*ptr != '1')))
                {
                  /* Eintrag ungueltig. */
                  nicp->name[0] = 0;
                  break;
                }

                /* Einstellung setzen. */
                nicp->setpw[i++] = *ptr;

                /* Zur naechsten Einstellung. */
                ptr++;
                cnt--;
              }

              if (nicp->name[0] == FALSE)
                break;

              /* Nullzeichen setzen. */
              nicp->setpw[i] = 0;
              /* Laenge neu setzen, fuer naechsten Eintrag. */
              Len = PWSETLEN;
              /* Zuruecksetzen. */
              i   = 0;
              break;


            /* Passwort einlesen. */
            case PO_PASSWD :
              ptr++;
              cnt--;

              /* Pruefe Passwort . */
              if ((Error = GetPasswd(&cnt, &ptr, 80 + 1, passwort)) == YES)
              {
                /* Passwort Dekodieren. */
                PasswdDekodierung(passwort, nicp->passwd);
                break;
              }
                /* Kein Passwort gesetzt. */
              else
                nicp->passwd[0] = 0;

             break;


            default :
             break;
          }
        }
      } while (cnt > 0);

      /* Nur wenn Rufzeichen gesetzt. */
      if (nicp->name[0] != FALSE)
      {
        /* Eintrag in die List haengen. */
        relink((LEHEAD *)nicp, (LEHEAD *)(nic->heardl.tail));
        /* Ein Eintrag mehr. */
        nic->act++;
      }
       /* Kein Rufzeichen gesetzt. */
      else
        /* Eintrag entsorgen. */
        dealoc((MBHEAD *)nicp);

    }

    /* Datei schliessen. */
    fclose(fp);
    return;
  }
}

/* Profil-TBL auf Platte sichern. */
void SaveTableProfil(char *path, PROFTAB *nictab)
{
  FILE          *fp;
  PRFHEARD huge *nicp;
  char           file[128];
  char           passwd[80 + 1];

  /* Path setzen. */
  strcpy(file, path);
  /* Dateiname setzen. */
  strcat(file, nictab->name);

  /* Datei oeffnen. */
  if ((fp = xfopen(file, "wt")) != NULL)
  {
    /* Alle Eintraege sichern. */
    for (nicp  = (PRFHEARD *)nictab->heardl.head;
         nicp != (PRFHEARD *)&nictab->heardl;
         nicp  = nicp->next)
    {
      /* Kein Rufzeichen gesetzt (sicher ist sicher). */
      if (nicp->name[0] == FALSE)
        /* Zum naechsten Eintrag. */
        continue;

      /* Frischen Buffer. */
      memset(passwd, 0, sizeof(passwd));
      /* Passwortvariable zuruecksetzen. */
      passwd[0] = 0;
      /* Evl. Passwort kodieren. */
      PasswdKodierung(nicp->passwd, passwd);

      nicp->name[L2CALEN] = 0;

      /* Eintrag in Datei schreiben. */
      fprintf(fp, "NAME=%s NICK=%s SETPW=%s PASSWD=%s\n"
                , nicp->name
                , nicp->nick
                , nicp->setpw
                , passwd);
    }

    /* Datei schliessen. */
    fclose(fp);
  }
}

/* Profil-Liste sichern. */
void SaveProfil(void)
{
#ifdef ST
  char *mcp;
#endif

  /* Profil-TBL auf Platte sichern. */
  SaveTableProfil(confpath, &proftab);
#ifdef ST
  if ((mcp = getenv("MHTOPPATH")) != NULL)
    SaveTableProfil(mcp, &proftab);
#endif
}

/* Eintrag suchen/bereitstellen. */
PRFHEARD *LookupProfil(PROFTAB *nic, const char *id)
{
  PRFHEARD *nicp;
  LHEAD    *heardl = &nic->heardl;
  char      call[L2IDLEN];

  call3str(call, id);

  /* Durchsuche alle Eintraege. */
  for (nicp  = (PRFHEARD *)heardl->tail;
       nicp != (PRFHEARD *)heardl;
       nicp  = nicp->prev)
  {
    /* Callvergleich. */
    if (cmpcal(nicp->name, call))
      /* Aktueller Eintrag. */
      return(nicp);
  }

  /* Kein Eintrag gefunden. */
  return(NULL);
}

/* Neues Profil anlegen. */
PRFHEARD *AddProfil(PROFTAB *nic, const char *id)
{
  PRFHEARD *nicp;
  char      call[L2IDLEN];
  int       i;

  call3str(call, id);

  /* Keine Eintraege anlegen. */
  if (nic->max == 0)
    /* Abbrechen. */
    return(NULL);

  /* Max. Profil-Eintraege erreicht. */
  while (nic->act >= nic->max)
  {
    /* Den letzten Profil-Eintrag loeschen. */
    dealoc((MBHEAD *)ulink((LEHEAD *)nic->heardl.head));
    /* Einen Eintrag weniger. */
    nic->act--;
  }
  /* Neues Profil anlegen. */
  relink((LEHEAD *)(nicp = ((PRFHEARD *)allocb(ALLOC_USEPROF))),
         (LEHEAD *)(nic->heardl.head));
  /* Einen Eintrag mehr. */
  nic->act++;

  nicp->nick[0]   = 0;
  nicp->passwd[0] = 0;

  strncpy(nicp->name, call, L2CALEN);

  /* Passwort-Einstellungen auf deaktiv stellen. */
  for (i = 0; i < PWSETLEN; i++)
    nicp->setpw[i] = '0';

  /* Nullzeichen. */
  nicp->setpw[i] = 0;

  ulink((LEHEAD *)nicp);
  relink((LEHEAD *)nicp, (LEHEAD *)(nic->heardl.tail));

  return(nicp);
}

/* Profil-Eintrag loeschen. */
void DeleteProfil(char *name)
{
  PRFHEARD *prf;
  PROFTAB  *ptab = &proftab;

  /* Durchsuche alle Eintraege. */
  for (prf  = (PRFHEARD *)ptab->heardl.head;
       prf != (PRFHEARD *)&ptab->heardl;
       prf  = prf->next)
  {
    /* Callvergleich. */
    if (!cmpcal(prf->name, name))
      /* Zum naechsten Eintrag. */
      continue;

    /* Loesche Profil-Eintrag. */
    dealoc((MBHEAD *)ulink((LEHEAD *)prf));
    /* Einen Eintrag weniger. */
    ptab->act--;
    return;
  }
}

/* Nickname einlesen. */
static TRILLIAN
GetName(WORD *laenge, char **inbuf, int Len, char *outbuf)
{
  char  Buffer[NAMESIZE + 1];
  char *bufpoi = Buffer;
  char  zeichen;
  WORD  i;
  int   a = 0,
        b = 0;
  char *p = *inbuf;
  WORD  n = *laenge;

  /* Frischer Buffer. */
  memset(Buffer, 0, sizeof(Buffer));
  memset(outbuf, 0, sizeof(outbuf));

  i = 0;
  while (n > 0)
  {
    if (((zeichen = (char)(*p)) == ' ' ))
      break;

    if (zeichen < ' ')
    {
      /* Umlaute zulassen. */
      if (  (zeichen != (char)0xe4) /* ae */
          &&(zeichen != (char)0xf6) /* oe */
          &&(zeichen != (char)0xfc) /* ue */
          &&(zeichen != (char)0xc4) /* Ae */
          &&(zeichen != (char)0xd6) /* Oe */
          &&(zeichen != (char)0xdc) /* Ue */
          &&(zeichen != (char)0xdf)) /* ss */
        return(ERRORS);
    }

    if (i++ == Len - 1)
      return(ERRORS);

    *bufpoi++ = zeichen;
    ++p;
    --n;
  }

  if (i == 0)
    return(NO);

  Buffer[i] = 0;

  while (i--)
    outbuf[a++] = Buffer[b++];

  outbuf[a] = 0;

  *laenge = n;
  *inbuf  = p;

  return(YES);
}

/* Pruefe Passwortstring vom User. */
BOOLEAN CheckPasswd(void)
{
  char pwd[5 + 1];

  memcpy(pwd, userpo->paswrd, 5);
  pwd[5] = 0;

  if (strlen((char *)clipoi) > 80)                     /* maximal 80 Zeichen  */
    *(clipoi + 80) = NUL;
  if (clicnt >= 5 && strstr((char *)clipoi, pwd) != NULL)
    return(FALSE);                                   /* Passwort war korrekt. */
  else
    return(TRUE);                                     /* Passwort war falsch. */
}

/* Passwortstring senden. */
void SendPasswdStringProfil(void)
{
  PRFHEARD *prf = NULL;
  MBHEAD   *mbp;
  WORD      num;
  WORD      a,
            b;
  int       i;
  char      buffer[255];
  char     *pwd;
  int       pwl;

  /* Profil-Eintrag suchen. */
  if ((prf = SearchProfil(calofs(UPLINK, userpo->uid))) == NULL)
    return;

  pwd = prf->passwd;
  pwl = strlen(prf->passwd);

  mbp = getmbp();

  putstr("*** ", mbp);

  /* Zufallsgenerator starten. */
  srand((UWORD)tic10);

  memset(buffer, 0, sizeof(buffer));

  for (a = 0; a < 5; ++a)
  {
    do
    {
      do; while (((num = (rand()%256)) >= pwl) || (pwd[num] == ' '));

      for (b = 0; b < a; ++b)
      {
        if ((userpo->paswrd[b] & 0xFF) == num)
          break;
      }
    } while (a != b);

    /* Passwortstring merken. */
    userpo->paswrd[a] = (UBYTE)num;
    sprintf(buffer," %d",num+1);
    putprintf(mbp, "%s", buffer);
  }

  putstr(">\r", mbp);
  seteom(mbp);

  for (i = 0; i < 5; ++i)
    userpo->paswrd[i] = pwd[userpo->paswrd[i]];
}

/* Ein Profil-Eintrag suchen. */
PRFHEARD *SearchProfil(const char *call)
{
  PRFHEARD *prf = NULL;
  PROFTAB  *ptab = &proftab;

  /* Durchsuche alle Eintraege. */
  for (prf  = (PRFHEARD *)ptab->heardl.head;
       prf != (PRFHEARD *)&ptab->heardl;
       prf  = prf->next)
  {
    if (call[0] == FALSE)
      /* Kein Eintrag gefunden. */
      return(NULL);

    /* Callvergleich. */
    if (cmpcal(call, prf->name))
    {
      ulink((LEHEAD *)prf);
      relink((LEHEAD *)prf, (LEHEAD *)(ptab->heardl.tail));
      /* Eintrag gefunden. */
      return(prf);
    }
  }

  /* Kein Eintrag gefunden. */
  return(NULL);
}

/* Profil-Eintrag mit Passwortabfrage suchen. */
BOOLEAN SearchPasswdProfil()
{
  PRFHEARD *prf = NULL;
  LNKBLK   *link;
#ifdef L1TCPIP
  TCPIP    *tpoi;
#endif /* L1TCPIP */

  /* Profil-Eintrag suchen. */
  if ((prf = SearchProfil(calofs(UPLINK, userpo->uid))) != NULL)
  {
    /* Kein Passwortabfrage bei Host-User. */
    if (g_utyp(userpo->uid) == HOST_USER)
      return(TRUE);

    /* User-Typ anhand der UID feststellen. */
    switch(g_utyp(userpo->uid))
    {
      /* L2-Link */
      case L2_USER :
        link = g_ulink(userpo->uid);

        /* Keine Passwortabfrage. */
        if (prf->setpw[link->liport] == '0')
         return(TRUE);

       break;

#ifdef L1TCPIP
      /* TCPIP */
      case TCP_USER :
        tpoi = g_ulink(userpo->uid);

        /* Keine Passwortabfrage. */
        if (prf->setpw[(int)tpoi->port] == '0')
          return(TRUE);

       break;
#endif /* L1TCPIP */

    }

    /* Passwort ist gesetzt. */
    if (prf->passwd[0] != FALSE)
    {
      /* Passwort-Modus setzen. */
      userpo->status = US_UPWD;
      /* Passwortstring senden. */
      SendPasswdStringProfil();
      return(FALSE);
    }
  }

  /* Kein Eintrag oder kein Passwort gesetzt. */
  return(TRUE);
}

/* Passwort-Einstellungen ausgeben. */
static void GetPortPW(MBHEAD *mbp,   PRFHEARD *prf)
{
  PORTINFO *p;
  int       port,
            None = 0;


  putstr("Activ Password of Port:\r", mbp);

  for (port = 0, p = portpar; port < L2PNUM; p++, port++)
  {
    if (prf->setpw[port] == '1')
    {
      putprintf(mbp, "P%02u (%s)\r", port, p->name);
      ++None;
    }
  }

  if (None == FALSE)
    putstr("None\r", mbp);
}

/* Einen Eintrag ausgeben. */
static void ShowProfil(MBHEAD *mbp, const char *id)
{
  PRFHEARD *prf = NULL;
  char      passwd[80 + 1],
            call[10];
  int       i = 0;

  /* Profil-Eintrag suchen. */
  if ((prf = SearchProfil(id)) != NULL)
  {
    /* Passwort ist gesetzt. */
    if (prf->passwd[0] != FALSE)
    {
      /* Frischer Buffer. */
      memset(passwd, 0, sizeof(passwd));

      /* Passwort bleibt Geheim!. */
      for (i = 0; i < (signed)strlen(prf->passwd); ++i)
        passwd[i] = '*';
    }

    /* Nullzeichen setzen. */
    passwd[i] = 0;

    memset(call, 0, sizeof(call));
    call2str(call, prf->name);
    /* Profil-Eintrag ausgeben. */
    putprintf(mbp, "Call %s is registered\r"
                   "Nick  : %s\r"
                   "Passwd: %s\r\r"
                   , call
                   , (prf->nick[0] == FALSE ? "No set Nickname" : prf->nick)
                   , (passwd[0] == FALSE ? "No set Passwd" : passwd));

    /* Passwort-Einstellungen ausgeben. */
    GetPortPW(mbp, prf);
    return;
  }

  /* Kein Eintrag gefunden! */
  putstr("Call is not registerd!\r", mbp);
}

/* Neues Profil anlegen. */
static void NewProfil(MBHEAD *mbp, const char *call)
{
  PRFHEARD *prf = NULL;

  /* Wenn es keinen Eintrag gibt. */
  if ((prf = SearchProfil(call)) == NULL)
  {
    /* Neues Profil anlegen. */
    if ((prf = AddProfil(&proftab, call)) != NULL)
    {
      putprintf(mbp, "Call successfully registerd\r");
      return;
    }

    putprintf(mbp, "Invalid Call!\r", call);
    return;
  }

  /* Eintrag gibt es schon. */
  putprintf(mbp, "Call existed already\r");
}

/* Profil loeschen. */
static void DelProfil(MBHEAD *mbp, char *call)
{
  PRFHEARD *prf;

  /* Eintrag suchen der geloescht werden soll. */
  if ((prf = SearchProfil(call)) != NULL)
  {
    /* Profil loeschen. */
    DeleteProfil(call);
    putprintf(mbp, "Call was exitinguished.\r");
    return;
  }

  /* Kein Eintrag gefunden. */
  putstr("Call is not registerd!\r", mbp);
}

/* Alle Profil-Eintraege auflisten. */
static void ListProfil(MBHEAD *mbp)
{
  PRFHEARD *prf;
  PROFTAB  *ptab = &proftab;
  char      call[10];

  /* Wenn kein Sysop, */
  if (!issyso())
  {
    /* ist hier schluss. */
    putstr("No Sysop!\r", mbp);
    return;
  }

  /* Wenn keine Eintraege, */
  if (ptab->act == FALSE)
  {
    /* ist hier ebenfalls schluss. */
    putstr("No entries found!\r", mbp);
    return;
  }

  putstr("--Call----Nick-Passwd-\r", mbp);

  /* Alle Eintraege. */
  for (prf  = (PRFHEARD *)ptab->heardl.head;
       prf != (PRFHEARD *)&ptab->heardl;
       prf  = prf->next)
  {
    /* Frischer Buffer. */
    memset(call, 0, sizeof(call));
    /* Konvertiere Rutzeichen. */
    call2str(call, prf->name);
    /* Eintrag auslisten. */
    putprintf(mbp, "%-9s %-3s  %-3s\r"
                 , call
                 , (prf->nick[0] == FALSE ? "NO" : "YES")
                 , (prf->passwd[0] == FALSE ? "NO" : "YES"));
  }
}

/* Passwort setzen/loeschen. */
static void SetPasswd(MBHEAD *mbp, const char *call)
{
  PRFHEARD *prf;
  char      passwd[80 + 1];
  int       i;

  /* Pruefe ob der User direkt kommt. */
  if (  (strlen(call) > L2IDLEN + 1)
      &&(!cmpcal(call + L2IDLEN, myid)))
  {
    putstr("They may not change the password!\r", mbp);
    return;
  }

  /* Suche Eintrag. */
  if ((prf = SearchProfil(call)) != NULL)
  {
    /* Eventuelle leerzeile loeschen. */
    skipsp(&clicnt, &clipoi);

    /* Passwortstring. */
    if (clicnt)
    {
      /* Pruefe Passwortstring. */
      if (  (clicnt < 5)
          ||(clicnt > 81))
      {
        putstr("Invalid password!\r", mbp);
        return;
      }

      /* Frischer Buffer. */
      memset(passwd, 0, sizeof(passwd));

      /* Max. 80 zeichen einlesen. */
      for (i = 0; i < 80; i++)
      {
        /* Keine Zeichen mehr. */
        if (clicnt == 0)
          /* Abbrechen. */
          break;

        /* Leerzeichen. */
        if (*clipoi == ' ')
          /* Abbrechen. */
          break;

        /* Zeichen setzen. */
        passwd[i] = *clipoi;

        /* Naechstes Zeichen. */
        ++clipoi;
        --clicnt;
      }

      /* Max. 80 Zeichen. */
      for (i = 0; i < 80; i++)
      {
        /* Passwort string zu ende. */
        if (passwd[i] == FALSE)
          /* Abbrechen. */
          break;

        /* Zeichen setzen. */
        prf->passwd[i] = passwd[i];
      }

      /* Nullzeichen setzen. */
      prf->passwd[i] = 0;

      putstr("Password is set.\r", mbp);
      return;
    }
      /* Kein Passwort angegeben. */
    else
      {
        putstr("Syntax: PROF P S Passwordstring\rNo password indicated!\r", mbp);
        return;
      }
  }

  /* Kein Eintrag gefunden. */
  putstr("Call is not registerd!\r", mbp);
}

/* Passwort loeschen. */
static void DelPasswd(MBHEAD *mbp, const char *call)
{
  PRFHEARD *prf;

  /* Pruefe ob der User direkt kommt. */
  if (  (strlen(call) > L2IDLEN + 1)
      &&(!cmpcal(call + L2IDLEN, myid)))
  {
    putstr("They may not change the password!\r", mbp);
    return;
  }

  /* Suche Eintrag. */
  if ((prf = SearchProfil(call)) != NULL)
  {
    /* Kein Passwort gesetzt. */
      if (prf->passwd[0] == FALSE)
      {
        /* Dann brauchen wir auch nix loeschen. */
        putstr("No password indicated!\r", mbp);
        return;
      }

        /* Passwort ist gesetzt. */
      else
        {
          int i;

          /* Loeschen Passwort. */
          prf->passwd[0] = 0;
          putstr("Password is delete!\r", mbp);

          /* Passwort-Modus zuruecksetzen. */
          for (i = 0; i < PWSETLEN; i++)
            prf->setpw[i] = '0';

          /* Nullzeichen setzen. */
          prf->setpw[i] = 0;
          return;
        }
    }
}

/* Passwort-Modus ein/ausschalten. */
static void SetPortPW(MBHEAD *mbp, const char *call)
{
  PRFHEARD *prf;
  int       Port,
            Opt;

  /* Suche Eintrag. */
  if ((prf = SearchProfil(call)) != NULL)
  {
    /* Passwortstring. */
    if (clicnt)
    {
      skipsp(&clicnt, &clipoi);

      /* Modus ermitteln (0 = aus, 1 = ein). */
      Opt = atoi(clipoi);
      /* Wenn Keine Ziffer, */
      if (  (isalpha(*clipoi))
          ||(Opt < 0)
          ||(Opt > 1))
      {
        /* Abbrechen. */
        putstr("Invalid Option!\r", mbp);
        return;
      }

      clipoi++;
      clicnt--;

      /* Eventuelle leerzeile loeschen. */
      skipsp(&clicnt, &clipoi);
      /* Port ermitteln. */
      Port = atoi(clipoi);

      /* Wenn Keine Ziffer, */
      if (  (isalpha(*clipoi))
          ||(Port < 0)
          ||(Port >= PWSETLEN))
      {
        /* Abbrechen. */
        putstr("Invalid Port!\r", mbp);
        return;
      }

      /* Wenn kein Passwort gesetzt, */
      if (prf->passwd[0] == FALSE)
      {
        /* abbrechen. */
        putstr("No set password!\r", mbp);
        return;
      }

      /* Passwort-Modus einschalten. */
      if (Opt == 1)
      {
        /* Modus setzen. */
        prf->setpw[Port] = '1';
        putprintf(mbp, "Password is of Port %d enabled.\r", Port);
      }
        /* Passwort-Modus ausschalten. */
      else
        {
          /* Modus setzen. */
          prf->setpw[Port] = '0';
          putprintf(mbp, "Port %d disabled.\r", Port);
        }

        return;
    }
    else
      {
        putstr("Syntax: PROF P P 0..1 0..15\rNo Port!\r\r", mbp);
        /* Passwort-Einstellungen ausgeben. */
        GetPortPW(mbp, prf);
        return;
      }
  }

  /* Kein Eintrag gefunden. */
  putstr("Call is not registerd!\r", mbp);
}

/* Nickname setzen. */
static void SetNick(MBHEAD *mbp, const char *call)
{
  PRFHEARD *prf;
  char      nickname[NAMESIZE + 1];
  int       i;

  /* Pruefe ob der User direkt kommt. */
  if (  (strlen(call) > L2IDLEN + 1)
      &&(!cmpcal(call + L2IDLEN, myid)))
  {
    putstr("They may not change the nickname!\r", mbp);
    return;
  }

  /* Suche Eintrag. */
  if ((prf = SearchProfil(call)) != NULL)
  {
    /* Eventuelle leerzeile loeschen. */
    skipsp(&clicnt, &clipoi);

    /* Passwortstring. */
    if (clicnt)
    {
      /* Pruefe Passwortstring. */
      if (clicnt > NAMESIZE)
      {
        putstr("Invalid nickname!\r", mbp);
        return;
      }

      /* Frischer Buffer. */
      memset(nickname, 0, sizeof(nickname));

      /* Max. 16 zeichen einlesen. */
      for (i = 0; i < NAMESIZE; i++)
      {
        /* Keine Zeichen mehr. */
        if (clicnt == 0)
          /* Abbrechen. */
          break;

        /* Leerzeichen. */
        if (*clipoi == ' ')
          /* Abbrechen. */
          break;

        /* Zeichen setzen. */
        nickname[i] = *clipoi;

        /* Naechstes Zeichen. */
        ++clipoi;
        --clicnt;
      }

      /* Max. 16 Zeichen. */
      for (i = 0; i < NAMESIZE; i++)
      {
        /* Passwort string zu ende. */
        if (nickname[i] == FALSE)
          /* Abbrechen. */
          break;

        /* Zeichen setzen. */
        prf->nick[i] = nickname[i];
      }

      /* Nullzeichen setzen. */
      prf->nick[i] = 0;

      putstr("Nickname is set.\r", mbp);
      return;
    }
      /* Kein Nickname angegeben. */
    else
      {
        putstr("Syntax: PROF I S nickname\rNo nickname indicated!\r", mbp);
        return;
      }
  }

  /* Kein Eintrag gefunden. */
  putstr("Call is not registerd!\r", mbp);
}

/* Nickname loeschen. */
static void DelNick(MBHEAD *mbp, const char *call)
{
  PRFHEARD *prf;

  /* Pruefe ob der User direkt kommt. */
  if (  (strlen(call) > L2IDLEN + 1)
      &&(!cmpcal(call + L2IDLEN, myid)))
  {
    putstr("They may not change the nickname!\r", mbp);
    return;
  }

  /* Suche Eintrag. */
  if ((prf = SearchProfil(call)) != NULL)
  {
    /* Kein Passwort gesetzt. */
      if (prf->nick[0] == FALSE)
      {
        /* Dann brauchen wir auch nix loeschen. */
        putstr("No nickname indicated!\r", mbp);
        return;
      }

        /* Nickname ist gesetzt. */
      else
        {
          /* Loeschen Nickname. */
          prf->nick[0] = 0;
          putstr("Nickname is delete!\r", mbp);
          return;
        }
    }
}

/* Nick setzen/loeschen. */
static void NickProfil(MBHEAD *mbp, const char *call)
{
  skipsp(&clicnt, &clipoi);

  switch (*clipoi)
  {
    /* Passwort setzen. */
    case 'S' :
    case 's' :
      clipoi++;
      clicnt--;

      SetNick(mbp, call);
     return;

    /* Passwort loeschen. */
    case 'D' :
    case 'd' :
      clipoi++;
      clicnt--;

      DelNick(mbp, call);
     return;

    default :
     break;
  }

  putstr("(S)et Nickname (D)el Nickname\r", mbp);
}

/* Passwort setzen/loeschen. */
static void PasswdProfil(MBHEAD *mbp, const char *call)
{
  skipsp(&clicnt, &clipoi);

  switch (*clipoi)
  {
    /* Passwort setzen. */
    case 'S' :
    case 's' :
      clipoi++;
      clicnt--;

      SetPasswd(mbp, call);
     return;

    /* Passwort loeschen. */
    case 'D' :
    case 'd' :
      clipoi++;
      clicnt--;

      DelPasswd(mbp, call);
     return;

    /* Passwort-Modus ein/ausschalten. */
    case 'P' :
    case 'p' :
      clipoi++;
      clicnt--;

      SetPortPW(mbp, call);
     return;

    default :
     break;
  }

  putstr("(S)et Password (D)el Password (P)ort activ/deactiv\r", mbp);
}

#define     OP_NONE     0
#define     OP_SHOW     1
#define     OP_NEW      2
#define     OP_NICK     3
#define     OP_DEL      4
#define     OP_LIST     5
#define     OP_PASS     6
#define     BUFLEN     32

void ccpprofil(void)
{
  MBHEAD      *mbp;
  PROFTAB     *ptab = &proftab;
  char         cBuf[BUFLEN + 1];
  int          i;
  unsigned int uCmd = OP_NONE;

  if (clicnt)
  {
    /* Frischer Buffer */
    memset(cBuf, 0, sizeof(cBuf));

    /* Buffer einlesen. */
    for (i = 0; i < BUFLEN; ++i)
    {
      if ((!clicnt) || (*clipoi == ' '))
        break;
      clicnt--;
      cBuf[i] = toupper(*clipoi++);
    }

    /* Befehl: Eigenes Profil zeigen. */
    if (   (strcmp(cBuf, "SHOW") == 0)
        || (cBuf[0] == 'S')
       )
      uCmd = OP_SHOW;

    /* Befehl: Neues Profil anlegen. */
    if (   (strcmp(cBuf, "NEW") == 0)
        || (cBuf[0] == 'N')
       )
      uCmd = OP_NEW;

    /* Befehl: Nickname setzen/loeschen. */
    if (   (strcmp(cBuf, "NICK") == 0)
        || (cBuf[0] == 'I')
       )
      uCmd = OP_NICK;

    /* Befehl: Profil loeschen. */
    if (   (strcmp(cBuf, "DEL") == 0)
        || (cBuf[0] == 'D')
       )
      uCmd = OP_DEL;

    /* Befehl: Alle Profil-Eintraege auflisten. */
    if (   (strcmp(cBuf, "LIST") == 0)
        || (cBuf[0] == 'L')
       )
      uCmd = OP_LIST;

    /* Befehl Passwort setzen. */
    if (   (strcmp(cBuf, "PASSWD") == 0)
        || (cBuf[0] == 'P')
       )
      uCmd = OP_PASS;

    /* Befehl. */
    switch (uCmd)
    {
      /* Profil zeigen. */
      case OP_SHOW :
        mbp = putals("");
        putprintf(mbp, "Profil->Show (%d:%d):\r\r", ptab->act, ptab->max);

        ShowProfil(mbp, calofs(UPLINK, userpo->uid));
        prompt(mbp);
        seteom(mbp);
       return;

      /* Neues Profil anlegen. */
      case OP_NEW :
        mbp = putals("");
        putprintf(mbp, "Profil->New (%d:%d):\r\r", ptab->act, ptab->max);

        NewProfil(mbp, calofs(UPLINK, userpo->uid));
        prompt(mbp);
        seteom(mbp);
       return;

      /* Neues Profil anlegen. */
      case OP_NICK :
        mbp = putals("");
        putprintf(mbp, "Profil->Nick (%d:%d):\r\r", ptab->act, ptab->max);

        NickProfil(mbp, calofs(UPLINK, userpo->uid));
        prompt(mbp);
        seteom(mbp);
       return;

      /* Profil loeschen. */
      case OP_DEL :
        mbp = putals("");
        putprintf(mbp, "Profil->Delete (%d:%d):\r\r", ptab->act, ptab->max);

        DelProfil(mbp, calofs(UPLINK, userpo->uid));
        prompt(mbp);
        seteom(mbp);
        return;

      /* Alle Profil-Eintraege auflisten. */
      case OP_LIST :
        mbp = putals("");

        putprintf(mbp, "Profil->List (%d:%d):\r\r", ptab->act, ptab->max);
        ListProfil(mbp);
        prompt(mbp);
        seteom(mbp);
        return;

      /* Passwort setzen/loeschen. */
      case OP_PASS :
        mbp = putals("");
        putprintf(mbp, "Profil->Passwd (%d:%d):\r\r", ptab->act, ptab->max);

        PasswdProfil(mbp, calofs(UPLINK, userpo->uid));
        prompt(mbp);
        seteom(mbp);
        return;

      default:
        mbp = putals("");
        putprintf(mbp, "User-Profil (%d:%d):\r\r", ptab->act, ptab->max);

        putstr("Invalid command\r", mbp);
        prompt(mbp);
        seteom(mbp);
       return;
    }
  }

  mbp = putals("");
  putprintf(mbp, "User-Profil (%d:%d):\r\r", ptab->act, ptab->max);

  if (uCmd == OP_NONE)
    putprintf(mbp, "(S)how (N)ew (D)el N(I)ckname (P)asswd %s\r", (issyso() == TRUE ? "(L)ist" : ""));

  prompt(mbp);
  seteom(mbp);
}

#endif /* USERPROF. */

/* End of src/profil.c. */
