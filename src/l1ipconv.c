#include "tnn.h"

#ifdef L1IPCONV

/* Zeiger auf das Aktuelle Interface. */
static T_INTERFACE *ifpp;

/* Anzahl der aktuellen IP-CONVERS-Routen auf 0 setzen. */
int ip_tbl_top = 0;


#ifdef OS_IPLINK
/******************************************************************************/
/*                                                                            */
/* IPC-Routen ausgeben.                                                       */
/*                                                                            */
/******************************************************************************/
static void ShowIpcEntry(struct iptbl iPC, MBHEAD *buf)
{
  char TmpCall[10];
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(ShowIpcEntry)";
#endif /* DEBUG_MODUS */

  buf->l4time = buf->mbpc;                    /* erst mal merken, wo wir sind */

  call2str(TmpCall, iPC.name);                     /* Konvertiere Rufzeichen. */
  putprintf(buf, "%s", TmpCall);

  putspa(10, buf);
  show_ip_addr(Ntohl(iPC.ipaddr), buf);               /* IP-Adresse ausgeben. */

  putspa(27, buf);
  putnum(iPC.port, buf);                                /* TCP-Port ausgeben. */

  putspa(33, buf);
  putprintf(buf, "%s\r", (iPC.linkflag ? "LINK" : "USER")); /* Status ausgeben*/
}
#endif /* OS_IPLINK */

/* IP-CONVERS-Server Einstellung aendern/setzen. */
void ccpipconv(void)
{
  MBHEAD         *mbp;
#ifdef OS_IPLINK
  struct          hostent *he;
  UBYTE           tip[4];
  unsigned char   tmpip[IPADDR + 1];
  unsigned char   tmphost[HNLEN];
  unsigned int    tmptport;
  register int    i;
  char            tmpcall[L2IDLEN];
  BOOLEAN         tmplink = FALSE;
#endif /* OS_IPLINK */
  char            ch;
  int             tmp_fd         = EOF;
  int             newloglevel    = 0;
  unsigned int    new_tcp_port   = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(ccpipconv)";
#endif /* DEBUG_MODUS */

  ifpp = &ifp[CVS_ID];                    /* Zeiger auf das IPCONV-Interface. */

  /* Sysop will aendern?  */
  if (issyso() && skipsp(&clicnt, &clipoi))
  {
    clicnt--;
    ch = toupper(*clipoi++);
    switch (ch)
    {
      /* Sysop will IPCONV-Port aendern */
      case 'P':
       if (!skipsp(&clicnt, &clipoi))
       {
         putmsg("Invalid Parameter\r");
         return;
       }

       /* Neuen TCP Port einlesen. */
       new_tcp_port = nxtlong(&clicnt, &clipoi);

       if (  (new_tcp_port <= 0)
           ||(new_tcp_port > 65535))
       {
#ifdef SPEECH
         putmsg(speech_message(328));
#else
         putmsg("IPCONV-Port not valid, not changed !!!\r");
#endif
         return;
       }

       /* Wenn NEUER TCP-Port und ALTER TCP-Port */
       /* GLEICH sind, brauchen wir nix aendern. */
       if (ifpp->tcpport == Htons((unsigned short)new_tcp_port))
       {
         putmsg("TCP-Port successfully changed\r");
         return;
       }

       /* Ist IPCONV-Server aktiv ? */
       if (ifpp->actively == FALSE)
       {
#ifdef SPEECH
         putmsg(speech_message(327));
#else
         putmsg("TCP-Port haven not switched on!\r");
#endif
         return;
       }

       if ((tmp_fd = SetupTCP(ifpp->name, (unsigned short)new_tcp_port)) != EOF)
       {
#ifdef OS_STACK
         int tmp_fd_OS;

         if ((tmp_fd_OS = SetupOS(ifpp, (unsigned short)new_tcp_port)) != EOF)
         {
           /* alten Socket schliessen */
           close(ifpp->OsSock);
           /* Neuen Socket merken */
           ifpp->OsSock = tmp_fd_OS;
         }
         else
           {
             Close(tmp_fd);
             putmsg("ERROR: Changing UDP-Port failed, Port not changed !!!\r");
             return;
           }
#endif /* OS_STACK */

         Close(ifpp->ISock);                   /* Alten Socket schliessen. */
         ifpp->ISock = tmp_fd ;                     /* Neuen Socket merken */
       }
       else
         {              /* Neuen TCP-Port liess sich nicht Initialisieren. */
           putmsg("ERROR: Changing UDP-Port failed, Port not changed !!!\r");
           return;
         }

       /* TCP-Port OK, dann markieren wir neuen TCP-Port. */
       ifpp->tcpport = Htons((unsigned short)new_tcp_port);

#ifdef SPEECH
       putmsg(speech_message(330));
#else
       putmsg("TCP-Port successfully changed\r");
#endif
       return;

      /* Sysop will LOGLevel aendern */
      case 'L':
        if (!skipsp(&clicnt, &clipoi))
        {
          /* Keine Parameter angegeben, */
          /* status Loglevel anzeigen.  */
          mbp = putals("IPCONVER-Server:\r");
#ifdef SPEECH
          putprintf(mbp, speech_message(299),ifpp->log);
#else
          putprintf(mbp, "My LogLevel: %d\r",ifpp->log);
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }

        /* Markiere neuen Loglevel */
        newloglevel = nxtnum(&clicnt, &clipoi);

        if (  (newloglevel < 0)
            ||(newloglevel > 3))
        {
          mbp = putals("IPCONVERS-Server:\r");
#ifdef SPEECH
          putprintf(mbp, speech_message(325));
#else
          putprintf(mbp, "Fehler: Loglevel werte von 0 bis 3!\r");
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }

        /* Neuen Loglevel Sysop setzen. */
        ifpp->log = newloglevel;

        mbp = putals("IPCONVERS-Server:\r");
#ifdef SPEECH
        putprintf(mbp, speech_message(299), ifpp->log);
#else
        putprintf(mbp, "My Loglevel: %d\r", ifpp->log);
#endif
        prompt(mbp);
        seteom(mbp);
        return;
       break;

#ifdef OS_IPLINK
      /* Route eintragen/loeschen. */
      case 'R':
       /* Steht noch was im Buffer? */
       if (!skipsp(&clicnt, &clipoi))
       {
         putmsg("Invalid Parameter\r");
         return;
       }

       /* Leerzeile loeschen. */
       clicnt--;
       switch (*clipoi++)
       {
         /* Route loeschen? */
         case '-':
          /* Steht noch was im Buffer? */
          if (!skipsp(&clicnt, &clipoi))
          {
            putmsg("Invalid Parameter\r");
            return;
          }

          /* IPConver-Route loeschen. */
          IPConvDelTBL(clipoi);
          break;

          /* Neue Route eintragen. */
         case '+':
           /* Steht noch was im Buffer? */
           if (!skipsp(&clicnt, &clipoi))
           {
              putmsg("Invalid Parameter\r");
              return;
           }

            /* Call lesen */
            if (getcal(&clicnt, &clipoi, TRUE, (char *)tmpcall) != YES)
            {
              invcal();
              return;
            }

           /* Steht noch was im Buffer? */
           if (!skipsp(&clicnt, &clipoi))
           {
             putmsg("Invalid Parameter\r");
             return;
           }

           /* Hostname/IP-Adresse einlesen. */
           for (i = 0; i < HNLEN; ++i)
           {
             /* Keine Zeichen oder leerzeichen? */
             if (!clicnt || *clipoi == ' ')
               /* Schleife abbrechen. */
               break;

             clicnt--;
             /* Aktuelles zeichen in TBL schreiben. */
             tmphost[i] = *clipoi++;
           }
           tmphost[i] = NUL;
           /* Hostname oder IP-Adresse pruefen. */
           he = gethostbyname((char *)tmphost);

           /* IP-Adresse OK? */
           if (he == NULL)
           {
             /* Hostname nicht bekannt oder */
             /* IP-Adresse ungueltig.       */
             putmsg("Invalid Parameter\r");
             return;
           }

           (void)memcpy(tip, he->h_addr_list[0], 4);
           /* Kopiere IP-Adresse in TBL. */
           sprintf((char *)tmpip, "%d.%d.%d.%d"
                                ,tip[0]
                                ,tip[1]
                                ,tip[2]
                                ,tip[3]);

           /* Steht noch was im Buffer? */
           if (!skipsp(&clicnt, &clipoi))
           {
             putmsg("Invalid Parameter\r");
             return;
           }

           /* TCP-Port einlesen. */
           tmptport = nxtlong(&clicnt, &clipoi);

           /* Pruefen ob der TCP-Port gueltig ist. */
           if (  tmptport <= 0
               ||tmptport >  65535)
           {
             putmsg("Invalid Parameter\r");
             return;
           }

           /* Steht noch was im Buffer? */
           if (!skipsp(&clicnt, &clipoi))
             tmplink = FALSE;
           else
             if (!strncasecmp(clipoi, "LINK", clicnt))
               tmplink = TRUE;
             else
               tmplink = FALSE;
                                               /* Neue IPC-Route hinzufuegen. */
           IPConvAddTBL(tmpcall,                               /* Rufzeichen. */
                        tmphost,                      /* Hostname/IP-Adresse. */
                        he,                                    /* IP-Adresse. */
                        (unsigned short)tmptport,                /* TCP-Port. */
                        tmplink);                 /* Status: USER/LINK-Modus. */
         }
         break;
#endif /* OS_IPLINK */

         default:
           putmsg("Invalid Parameter\r");
          return;
       }
   }
   mbp = putals("IPCONVERS-Server");
   putprintf(mbp,":(%d/%d):\r", ip_tbl_top, MAX_ROUTEN);

#ifdef OS_IPLINK
   putstr("Name------IP---------------Port--Status-\r", mbp);

   for (i = 0; i < ip_tbl_top; ++i)
    ShowIpcEntry(ip_tbl[i], mbp);

   putstr("\r", mbp);
#endif /* OS_IPLINK */
#ifdef SPEECH
   putprintf(mbp, speech_message(299), ifpp->log);
   putprintf(mbp, speech_message(326), Ntohs(ifpp->tcpport));
#else
   putprintf(mbp, "My Loglevel: %u\r", ifpp->log);
   putprintf(mbp, "My TCP-Port: %u\r", Ntohs(ifpp->tcpport));
#endif
   prompt(mbp);
   seteom(mbp);
}

/******************************************************************************/
/*                                                                            */
/* Im Convers anmelden.                                                       */
/*                                                                            */
/******************************************************************************/
void IPConvLogin(void)
{
  PTCENT *ptcp;
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(IPConvLogin)";
#endif /* DEBUG_MODUS */

  cpyid(usrcal, tcppoi->Upcall);                 /* Usercall setzen fuer CVS. */

  clipoi = tcppoi->cmd;                               /* ggf. Channel setzen. */
  clicnt = tcppoi->cmdlen;                                  /* laenge setzen. */

  ptcp = ptctab + g_uid(tcppoi, TCP_USER);/* User aus der patchcord ermitteln.*/
  ptcp->state = C_IPLINK;                 /* Option fuer USER-Ausgabe setzen. */

  userpo = ptcp->ublk;                             /* Userblock-Zeiger lesen. */
  userpo->convflag = 1;                               /* Connect von aussen!. */
#ifdef L1IRC
  userpo->IrcMode = tcppoi->IrcMode;
#endif /* L1IRC */

  ccpcvs();                                          /* Login in den Convers. */

  ifpp = SetInterface(tcppoi->Interface);           /* akt. Interface setzen. */
                                                     /* ggf. Logbuch fuehren. */
  T_LOGL2(TRUE, "(IPConvogin):%s\n.Login in den Convermodus.\n"
                , tcppoi-> ip);
  return;
}

/* Aktuelle Zeichen auswerten. */
TRILLIAN GetContensCVS(char contens)
{
  int   i = 0;
  char s[RXLEN];
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(GetContensCVS)";
#endif /* DEBUG_MODUS */

  /* Linkpartner? */
  if (tcppoi->CVSlink)
  {
    /* Alles auf 0 setzen. */
    tcppoi->cmdlen = 0;
    tcppoi->rxc = 0;

    /* Alle Zeichen vom Socket holen. */
    while (tcppoi->rxc < tcppoi->RecvLen)
    {
      /* Aktuelle Zeichen. */
      s[i] = tcppoi->rxbuf[tcppoi->rxc++];

      /* Zeichen ein Return? */
      if (s[i] == CR)
        /* zum naechsten Zeichen. */
        continue;

      /* Zeichen ein Return? */
      if (s[i] == LF)
      {
        tcppoi->cmd[tcppoi->cmdlen++] = CR;
        /* zum naechsten Zeichen. */
        continue;
      }

      /* Aktuelles Zeichen setzen. */
      tcppoi->cmd[tcppoi->cmdlen++] = s[i++];
    }
    /* Nullzeichen setzen. */
    tcppoi->cmd[tcppoi->cmdlen] = 0;
    /* Frame ist komplett. */
    return(YES);
  }
  else
  /* User */
  {
    /* Zeichen ein Return? */
    if (contens == CR)
      /* Markiere es das es ein Return gibt. */
      tcppoi->cr = TRUE;

    /* Zeichen ein LF-Return? */
    if (contens == LF)
      /* LF moegen wir nicht.   */
      /* zum naechsten Zeichen. */
      return(NO);

    /* Alle Zeichen durch. */
    if (contens == (int)NULL)
    {
      /* War ein Return dabei? */
      if (tcppoi->cr)
      {
        tcppoi->cr = FALSE;
        /* Frame ist komplett. */
        return(YES);
      }
      else
        /* Return fehlt, */
        return(ERRORS);
    }

    /* User noch nicht eingeloggt? */
    if (!tcppoi->login)
    {
      /* Zeichen auf Gueltigkeit pruefen. */
      if (CheckContens(contens))
      {
        /* Ist es ein Linkpartner? */
        if (!strncmp(tcppoi->cmd, "ppconvers", 9))
          tcppoi->CVSlink = TRUE;
        else
          /* Ungueltiges Zeichen.   */
          /* zum naechsten Zeichen. */
          return(NO);
      }
    }

    /* Aktuelle Zeichen setzen. */
    tcppoi->cmd[tcppoi->cmdlen++] = contens;
    /* Zum naechsten Zeichen. */
    return(NO);
  }

  return(ERRORS);
}

#ifdef OS_IPLINK
/* Speichern der IPCONV-Routen. */
void IPConvDump(MBHEAD *mbp)
{
  int   i;
  char  tmp[10];
  char *linkstatus[] = {"User","Link"};
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(IPConvDump)";
#endif /* DEBUG_MODUS */

  putstr("; IPConv Routen\r;\r",mbp);

  for (i = 0; i < ip_tbl_top; i++)
  {
    call2str(tmp, (char *)ip_tbl[i].name);
    putprintf(mbp,"IPC R + %s %s %u %s\r"
                 ,tmp
                 ,ip_tbl[i].hostname
                 ,ip_tbl[i].port
                 ,ip_tbl[i].linkflag ? linkstatus[ip_tbl[i].linkflag] : linkstatus[ip_tbl[i].linkflag]);
  }
  putstr(";\r", mbp);
}

/******************************************************************************/
/*                                                                            */
/* IPC-Route in TBL suchen.                                                   */
/*                                                                            */
/******************************************************************************/
int IPConvIS(char *name, DEST *dest)
{
  register int i;
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(IPConvIS)";
#endif /* DEBUG_MODUS */

  for (i = 0; i < ip_tbl_top; i++)             /* Alle Eintraege durch gehen. */
  {
    if (cmpid(name, (char *)ip_tbl[i].name))          /* Rufzeichenvergleich. */
    {
      cpyid(dest->call, (char *)ip_tbl[i].name);        /* Rufzeichen setzen. */
      dest->port = ip_tbl[i].l2port;                       /* L2-Port setzen. */

      dest->via[0] = 0;                               /* defaultwerte setzen. */
      dest->typ    = 'U';
      dest->eax    = 0;

      return(TRUE);                                      /* Eintrag gefunden. */
    }
  }

  return(FALSE);                                    /* Kein eintrag gefunden. */
}

/* Rufzeichen in der IPC-TBL suchen. */
int IPConvSearch(char *call)
{
  register int i = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(IPConvSearch)";
#endif /* DEBUG_MODUS */

  /* Alle Eintraege durch gehen. */
  for (i = 0; i < ip_tbl_top; ++i)
  {
    /* Vergleiche Call/SSID. */
    if (cmpid(call, (char *)ip_tbl[i].name))
     /* i = TBL-Nummer vom CALL. */
     return(i);
  }
  /* Es gibt keinen Eintrag mit dem */
  /* gesuchten Rufzeichen.          */
  return(EOF);
}

/*****************************************************************************/
/*                                                                           */
/* Neue IPC-Route hinzufuegen                                                */
/*                                                                           */
/*****************************************************************************/
void IPConvAddTBL(char           *call,
                  unsigned char  *host,
                  struct hostent *hstent,
                  unsigned short  tport,
                  BOOLEAN         link)
{
  register int i;
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(IPConvAddTBL)";
#endif /* DEBUG_MODUS */
                                              /* Suche Rufzeichen in der TBL. */
  if ((i = IPConvSearch(call)) != EOF)
  {                                               /* IPC-Route Aktualisieren. */
    (void)memcpy(ip_tbl[i].hostname, host, HNLEN);        /* Hostname setzen. */

    ip_tbl[i].l2port   = ifp[CVS_ID].l2port;               /* L2-Port setzen. */
    ip_tbl[i].port     = tport;                           /* TCP-Port setzen. */
    ip_tbl[i].linkflag = link;                   /* Status: User/Link setzen. */
  }
  else                                              /* IPC-Route hinzufuegen. */
  {
    cpyid(ip_tbl[ip_tbl_top].name, call);                 /* Rufzeichen setzen*/

    (void)memcpy(ip_tbl[ip_tbl_top].hostname, host, HNLEN);/* Hostname setzen.*/
    (void)memcpy((unsigned char*)&ip_tbl[ip_tbl_top].ipaddr, (unsigned char *)&hstent->h_addr_list[0][0], 4);

    ip_tbl[ip_tbl_top].l2port   = ifp[CVS_ID].l2port;   /* L2-Port eintragen. */
    ip_tbl[ip_tbl_top].port     = tport;               /* TCP-Port eintragen. */
    ip_tbl[ip_tbl_top].linkflag = link;          /* Status: User/Link setzen. */

    ip_tbl_top++;                                 /* Ein Tabelleneintrag mehr */
  }
}

/* IPCONV-Route loeschen. */
BOOLEAN IPConvDelTBL(char *name)
{
  register int i;
  register int j;
  char         tmp[10];
  char        *tmppoi = name;
  WORD         tmpcnt = strlen(name);
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(IPConvDelTBL)";
#endif /* DEBUG_MODUS */

  getcal(&tmpcnt, &tmppoi, TRUE, tmp);

  for (i = 0; i < ip_tbl_top; ++i)
  {
    if (cmpid(tmp, (char *)ip_tbl[i].name))
    {
      for (j = i; j < ip_tbl_top; ++j)
        ip_tbl[j] = ip_tbl[j+1];

      /* Ein Tabelleneintrag weniger */
      ip_tbl_top--;
      return(TRUE);
    }
  }
  return(FALSE);
}

/* IPConvers-Hostname lesen. */
BOOLEAN IPConvGetName(WORD *len, char **inbuf, BOOLEAN flag, char *outbuf)
{
  char  name[NAMESIZE + 1];
  char *nampoi;
  char  zeichen;
  char *p = *inbuf, *save_inbuf = *inbuf;
  WORD  n = *len,    save_len   = *len;
  int   plen = FALSE;
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(IPConvGetName)";
#endif /* DEBUG_MODUS */

  skipsp(&n, &p);
  nampoi = name;

  if (n)
  {
    while (n > 0)
    {
      zeichen = (char) toupper(*p);
      if (zeichen == ' ')
        break;

      if (   ((zeichen >= '0') && (zeichen <= '9'))
          || ((zeichen >= 'A') && (zeichen <= 'Z')))
      {
        *nampoi++ = zeichen;
        p++;
        n--;
        plen++;

        if (plen > NAMESIZE)
          return(FALSE);

        continue;
      }
      n--;
      p++;
    }

    name[plen] = 0;
    strncpy(outbuf, name, NAMESIZE);

    if (flag)
    {
      *len    = save_len;
      *inbuf  = save_inbuf;
    }
    else
      {
        *len    = n;
        *inbuf  = p;
      }
     return(TRUE);
  }
  return(FALSE);
}

/* IPConvers IP-Adresse lesen,. */
BOOLEAN IPConvGetIP(WORD *laenge, char **inbuf, unsigned char *outbuf)
{
  char  ipadresse[16 + 1];
  char *ipaddrpoi;
  char  zeichen;
  char *p = *inbuf;
  WORD  n = *laenge;
  int   len = FALSE;
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(IPConvGetIP)";
#endif /* DEBUG_MODUS */

  skipsp(&n, &p);
  ipaddrpoi = ipadresse;

  if (n)
  {
  while (n > 0)
  {
    zeichen = (char) toupper(*p);
    if (zeichen == ' ')
      break;

    if (   (zeichen == '.')
       || ((zeichen >= '0') && (zeichen <= '9')))
    {
      *ipaddrpoi++ = zeichen;
      p++;
      n--;
      len++;

      if (len > 16)
        return(FALSE);

      continue;
    }
    p++;
    n--;
  }

  ipadresse[len] = 0;
  strncpy((char *)outbuf, (char *)ipadresse, 16);

  *laenge = n;
  *inbuf  = p;
  return(TRUE);
 }
 return(FALSE);
}

int IPConvConnect(char *call, char *upcall, BOOLEAN flag)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1ipconv(IPConvConnect)";
#endif /* DEBUG_MODUS */

  if (IPConvConnectOS(call, upcall, flag))
    return(TRUE);

  return(FALSE);
}
#endif /* OS_IPLINK */
#endif /* L1IPCONV */

/* End of os/win32/ipconv.c */
