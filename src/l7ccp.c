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
/* File src/l7ccp.c (maintained by: ???)                                */
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

#include "tnn.h"

#ifdef ALIASCMD
extern CMDALIAS *aliaslist;
#endif

static BOOLEAN  abort_sbin(MBHEAD *);
static void     ccpcmd(void);
static BOOLEAN  loadprm(void);
static void     put_kMG(MBHEAD *, ULONG);
static void     put_pktnum(MBHEAD *, ULONG, ULONG);
static void     putptcinfo(PTCENT *, MBHEAD *);
static void     l2user(MBHEAD *, WORD, const char *);
static void     l4user(MBHEAD *, WORD, const char *);
static void     ptcuser(MBHEAD *, WORD, const char *);
static void     hostuser(MBHEAD *, WORD, const char *);
static BOOLEAN  talk_to(char *, WORD);
static void     percent(ULONG, ULONG, int, int, MBHEAD *);

#ifdef L1TCPIP
static void     TcpipUser(MBHEAD *, WORD, const char *);
#endif /* L1TCPIP */

#ifdef MAKRO_FILE
static int update_userpo_file(USRBLK *user);
#endif

#ifdef __LINUX__
extern UBYTE    console_type;
#endif

/************************************************************************/
/* Pruefen, ob die Autobinaer-Uebertragung abgebrochen werden soll      */
/*----------------------------------------------------------------------*/
static BOOLEAN abort_sbin(MBHEAD *mbp)
{
  if ((mbp->mbpc - mbp->mbgc) >= 4)
    return(get32(mbp) != 0x234f4b23L);  /* BC ist zu bloede fuer '#OK#' */
  return(FALSE);
}

static void ccpcmd(void)
{
  TRILLIAN result;
#ifdef ALIASCMD
  CMDALIAS *ap;
  char     cmdtmp[256], *clipoi2;
  int      i;
#endif

  cpyid(usrcal, calofs(UPLINK, userpo->uid));
#ifdef CONNECTMOD_SET_NODE
  switch(g_utyp(userpo->uid))               /* Den User-Typ eines User anhand */
  {                                         /* seiner UID feststellen.        */
    LNKBLK *l2_link;                                  /* Zeiger auf L2-Ebene. */
    CIRBLK *l4_link;                                  /* Zeiger auf L4-Ebene. */

    case L2_USER :
                                           /* Den User-Link eines User anhand */
      l2_link = g_ulink(userpo->uid);      /* seiner UID feststellen.         */
      SetMyNode(l2_link->viaidl);          /* Den Einstiegsknoten liefern,    */
     break;

    case L4_USER:
                                           /* Den User-Link eines User anhand */
      l4_link = g_ulink(userpo->uid);      /* seiner UID feststellen.         */
      cpyid(updigi,l4_link->upnod);       /* Den Einstiegsknoten liefern,     */
                                      /* dafuer nehmen wir den Uplink Knoten. */
     break;

    /* damit der Compiler auch bei Optimierungen zufrieden ist ... */
    default:
      updigi[0] = '\0';
     break;
  }
#endif /* CONNECTMOD_SET_NODE */

  if (ismemr()) {                   /* keine Arbeit ohne Speicher       */
    if (   userpo->sysflg           /* Sysop oder kein Sysop-Port?      */
        || g_utyp(userpo->uid) != L2_USER
        || !sysoponly(((LNKBLK*)g_ulink(userpo->uid))->liport)
#ifdef USER_PASSWORD
        || userpo->pwdtyp != PW_NOPW
#endif
       )
    {
#ifdef ALIASCMD
      clipoi2 = clipoi;
      i = 0;

      /* Kommando extrahieren und in Grossbuchstaben konvertieren */
      while (*clipoi2 && *clipoi2 != ' ' && i < 255)
        cmdtmp[i++] = toupper(*clipoi2++);
      cmdtmp[i] = '\0';

      if (i > 0)
      {
        for (ap = aliaslist; ap != NULL; ap = ap->next)
          if (!strcmp(cmdtmp, ap->alias))
            break;

        if (ap != NULL)
        {
          strcpy(cmdtmp, ap->cmd);  /* neues Kommando einsetzen         */
          clicnt = strlen(cmdtmp);
          if (*clipoi2 == ' ')      /* wenn Pars da waren               */
          {
            while (*clipoi2 && clicnt < 255)  /*     diese anfuegen     */
              cmdtmp[clicnt++] = *clipoi2++;
            cmdtmp[clicnt] = NUL;
          }
          strcpy(clilin, cmdtmp);
          clipoi = clilin;
        }
      }
#endif
      result = intern_command(cmdtab);  /* interner Befehl              */
      if (result == ERRORS)             /* Sonderzeichen im Befehl?     */
       {
        inv_cmd();
        return;
       }
      if (result == YES             /* kein Interner Befehl             */
          && read_txt()             /* kein lesbarer Text               */
          && extern_command()       /* kein externer Befehl             */
         )
        inv_cmd();                  /* unbekannter Befehl               */
      else                          /* Befehl war OK                    */
        userpo->errcnt = 0;
    } else {                        /* SYSOP-Port                       */
      if (intern_command(syscmdtab))
        inv_cmd();                  /* einfach nur Invalid Command      */
    }
  }
}

void l7rx(void) {
  FILE           *prot;
  char huge      *usnxtc;
  WORD            usget;
  WORD            zeichen;
  MBHEAD         *mhdp;
  MBHEAD         *usrmhd;
  MBHEAD         *mbp;
  UID             uid;
  UID             p_uid;
  PTCENT         *ptcp;
  CQBUF          *cqp;

  /*=== eingelaufene Info Frames fuer den CCP verarbeiten ===*/
  while ((mhdp = (MBHEAD *) userhd.head) != (MBHEAD *) &userhd.head)
  {
     ulink((LEHEAD *)mhdp);
     uid = mhdp->type;              /* User ID lesen                    */
     ptcp = ptctab+uid;
     userpo = ptcp->ublk;           /* Userblock-Zeiger lesen           */
     if (userpo == NULL) {          /* nicht im CCP?                    */
       dealmb(mhdp);
       continue;
     }

     /*==================================================================*/
     /*=== eingelaufene Info Frames fuer User verarbeiten             ===*/
     /*------------------------------------------------------------------*/

     if ((usrmhd = userpo->mbhd) == NULL)
       userpo->mbhd = mhdp;
     else {
       usnxtc = usrmhd->mbbp;
       usget = usrmhd->mbgc;
       while (usrmhd->mbgc < usrmhd->mbpc)
         getchr(usrmhd);
       while (mhdp->mbgc < mhdp->mbpc)
         putchr(getchr(mhdp), usrmhd);
       usrmhd->mbbp = usnxtc;
       usrmhd->mbgc = usget;
       dealmb(mhdp);
     }

     while (userpo && (mhdp = userpo->mbhd) != NULL && getlin(mhdp))
         {
       if (l7tosh(mhdp))            /* Shell hat verarbeitet        */
         continue;

        if (userpo->fp != NULL)     /* Laeuft Datei-Uebertragung?   */
        {
          if (   userpo->status == US_SBIN  /* binaersenden?                */
              && abort_sbin(mhdp) == FALSE) /* kam #OK#?, ignorieren        */
          {
            dealmb(mhdp);
            userpo->mbhd = NULL;
            continue;
          }

          fclose(userpo->fp);               /* Ja, dann abbrechen           */
          userpo->fp = NULL;

          if (userpo->fname != NULL)
          {
             xremove(userpo->fname);
             free(userpo->fname);
             userpo->fname = NULL;
          }

          mbp = getmbp();
#ifdef SPEECH
          putstr(speech_message(141), mbp);
#else
          putstr("\r- Aborted -\r\r", mbp);
#endif
          prompt(mbp);
          seteom(mbp);
          dealmb(mhdp);                    /* Abbruchzeile ignorieren */
          userpo->mbhd = NULL;
          userpo->status = US_CCP;
          continue;
        }

        if (userpo->status == US_RBIN)     /* BIN-Load                     */
        {
          program_load(mhdp);
          if (mhdp->mbgc == mhdp->mbpc)
          {
             dealmb(mhdp);
             userpo->mbhd = NULL;
          }
          continue;
        }

        if (userpo->convers != NULLCONNECTION)
          if (convers_input(mhdp))
          {
             if (mhdp->mbgc == mhdp->mbpc)
             {
               dealmb(mhdp);
               userpo->mbhd = NULL;
             }
             continue;
          }

        clipoi = clilin;
        clicnt = 0;

#ifdef AUTOBINAERFIX
        if (userpo->status == US_WBIN)
        {
          if ((zeichen = getchr(mhdp)) == CR)
            clicnt = 0;
          else
            {
              *clipoi++ = (char)zeichen;
               clicnt++;
            }
        }
#endif /* AUTOBINAERFIX */

        while ((mhdp->mbgc < mhdp->mbpc)
               && ((zeichen = getchr(mhdp)) != CR))
        {
          if ((zeichen == BS) || (zeichen == DEL))
          {
             if (clicnt != 0)
             {
                --clipoi;
                --clicnt;
             }
                }
          else
          {
             if ((zeichen != LF) && (clicnt < (WORD)(sizeof(clilin) - 5)))
             {
#ifdef __WIN32__
                *clipoi++ = (char)zeichen;
#else
                *clipoi++ = zeichen;
#endif /* WIN32 */
                ++clicnt;
             }
          }
        }
        *clipoi = NUL;                      /* und mit NUL terminieren! */

        if (mhdp->mbgc == mhdp->mbpc)
        {
          dealmb(mhdp);
          userpo->mbhd = NULL;
        }

        /*=== Zeile vom User auswerten ===*/

        switch (userpo->status) {
          case US_WBIN :
            start_autobin();
            continue;
          case US_RTXT :
            load_text();
            continue;
        }

#ifdef PACSAT
        if (userpo->pacsat != NULL) {
          l7_to_pacsat();
          continue;
        }
#endif
        clipoi = clilin;

        switch (userpo->status) {
          case US_WPWD :
          case US_WUPW :
            get_password();
            mbp = getmbp();
            prompt(mbp);
            seteom(mbp);
            continue;

#ifdef USERPROFIL
          case US_UPWD :
            if (CheckPasswd())
            {
              mbp = getmbp();

              putstr("Invalid Password\r", mbp);
              seteom(mbp);

              SendPasswdStringProfil();
              continue;
            }

            userpo->status = US_CCP;
            send_ctext();
            continue;
#endif /* USERPROFIL */

          case US_TALK :
            if (*clipoi == '/' && tolower(*(clipoi+1)) == 'q')
            {
              mbp = getmbp();
              prompt(mbp);
              seteom(mbp);
              userpo->talkcall[0] = NUL;
              userpo->status = US_CCP;
            }
            else
              talk_to(userpo->talkcall, 0);
            continue;
          case US_CREQ :
            p_uid = ptcp->p_uid;
            disusr(p_uid);
            ptcp->p_uid = NO_UID;
            ptctab[p_uid].p_uid = NO_UID;
            userpo->status = US_CCP;
            break;
          case US_CQ :
            for (cqp = (CQBUF *)cq_user.head;
                 cqp != (CQBUF *)&cq_user;
                 cqp  = cqp->next)
             {
              if (cqp->uid == userpo->uid)
               {
                dealoc((MBHEAD *)ulink((LEHEAD *)cqp));
                break;
               }
             }
            userpo->status = US_CCP;
            ptcp = ptctab + uid;
            ptcp->p_uid = NO_UID;
            break;
        }

        clipoi = clilin;

        /* hier werden alle Eingaben in einer Datei protokolliert */
        /* wenn dies gewuenscht wird !                            */

        if ( (proto == 2) ||
             ((proto == 1) && (issyso () )))
        {
          if ((prot = xfopen("COMMAND.LOG", "at")) != NULL)
          {
            fprintf(prot, "(%u) %.6s %s > %s\n\n",
                          uid,
                          calofs(UPLINK, uid),
                          ctime(&sys_time),
                          clipoi);
            fclose(prot);
          }
        }

        if (skipsp(&clicnt, &clipoi))
          ccpcmd();                       /* Zeile mit Inhalt             */
        else
        {                                 /* leere Zeile                  */
          mbp = getmbp();
          prompt(mbp);
          seteom(mbp);
        } /* else */
    } /* while */
  } /* while */
}

/************************************************************************/
/*                                                                      */
/* Wenn noch Reste aus einer Datei zu senden sind, diese an den User    */
/* senden                                                               */
/*                                                                      */
/************************************************************************/
void
l7tx(void)
{
  MBHEAD *mbp;
  LONG    pos;
  WORD    c;
  WORD    i;
#ifdef PORT_MANUELL
  LNKBLK *link;
  UWORD   PacLen = FALSE;
#endif /* PORT_MANUELL */

  for (userpo  = (USRBLK *) usccpl.head;
       userpo != (USRBLK *) &usccpl;
       userpo  = (USRBLK *) userpo->unext)
  {
    if (userpo->status == US_DIG)
    {
      gateway();
      if (userpo->status == US_DIG)
        userpo->status = US_CCP;
      continue;
    }

    if (userpo->fp != NULL)
    {
#ifdef PORT_MANUELL
      switch (g_utyp(userpo->uid))
      {
        case L2_USER:
          link = g_ulink(userpo->uid);
          PacLen = portpar[link->liport].paclen;
         break;

        default:
          PacLen = 236;
         break;
      }
#endif /* PORT_MANUELL */
        do
      {
        pos = ftell(userpo->fp);
        mbp = getmbp();
        if (userpo->status == US_SBIN)
        {
#ifdef PORT_MANUELL
          for (i = 0; i < PacLen; ++i)
#else
          for (i = 0; i < paclen; ++i)
#endif /* PORT_MANUELL */
          {
            if ((c = fgetc(userpo->fp)) == EOF)
              break;    /* for ... */
#ifdef __WIN32__
            putchr((char)c, mbp);
#else
            putchr(c, mbp);
#endif /* WIN32 */
          }
        }
        else
          {
#ifdef MAKRO_FILE
            /* USERPO->FILE nach Makros untersuchen */
            /* und neue USER->FILE erstellen.       */
            /* Gibt es einen Fehler in der funktion */
            /* update_userpo_file(userpo); wird die */
            /* alte userpo->fp genommen!            */
            update_userpo_file(userpo);
#endif
#ifdef PORT_MANUELL
            for (i = 0; i < PacLen; ++i)
#else
            for (i = 0; i < paclen; ++i)
#endif /* PORT_MANUELL */
            {
              if ((c = fgetc(userpo->fp)) == EOF)
                break;    /* for ... */

              if (c != CR)
#ifdef __WIN32__
                putchr((char)(c == '\n' ? CR : c), mbp);
#else
                putchr(c == '\n' ? CR : c, mbp);
#endif /* WIN32 */
            }
        }
      }
      while (i && send_msg(FALSE, mbp));        /* sind wir losgeworden */

      if (i)                                    /* spaeter versuchen    */
      {
        dealmb((MBHEAD *)ulink((LEHEAD *) mbp));
        fseek(userpo->fp, pos, SEEK_SET);
      }
      else                                      /* Ende erreicht        */
      {
#ifdef MAKRO_FILE
        /* Alle Daten verarbeitet, */
        /* dies markieren.         */
        userpo->read_ok = FALSE;
#endif
        putchr('\r', mbp);
        prompt(mbp);
        send_msg(TRUE, mbp);
        fclose(userpo->fp);
        userpo->fp = NULL;
        if (userpo->fname != NULL)
        {
           xremove(userpo->fname);
           free(userpo->fname);
           userpo->fname = NULL;
        }
        userpo->status = US_CCP;
      }
    }
  }
  convers_output();
}

/**************************************************************************/
/* BEACON                                                                 */
/*------------------------------------------------------------------------*/
void ccpbea(void)
{
  MBHEAD *mbp;
  BEACON *beapoi;
  char call[L2IDLEN];
  char digil[L2VLEN+1];
  WORD port;
  WORD interval;
  WORD telemetrie;
  WORD chaptr;

  if (issyso())
  {
    skipsp(&clicnt, &clipoi);
    if (clicnt)
      if ((port = (WORD) (nxtnum(&clicnt, &clipoi) & 0x7F)) < L2PNUM) {
        beapoi = &beacon[port];
        skipsp(&clicnt, &clipoi);
        if (*clipoi == '=') {
          ++clipoi;
          --clicnt;
          skipsp(&clicnt, &clipoi);
          chaptr = 0;
          while ((clicnt-- >0) && chaptr < 79)
            beapoi->text[chaptr++] = *clipoi++;
          beapoi->text[chaptr] = NUL;
          if (beapoi->text[0] == '.')
            beapoi->text[0] = NUL;
        } else {
          interval = nxtnum(&clicnt, &clipoi);
          telemetrie = nxtnum (&clicnt, &clipoi);
          if (getcal(&clicnt, &clipoi, FALSE, call) == YES) {
            cpyid(beapoi->beades, call);
            getdig(&clicnt, &clipoi, TRUE, digil);
            cpyidl(beapoi->beadil, digil);
            beapoi->interval = interval;
            beapoi->telemetrie = telemetrie;
            if (beapoi->interval != 0)
            {
              beapoi->beatim = interval; /* vorzeitige Aussendung erwirken */
              beacsv();
            }
          }
        }
        userpo->sysflg = 2;
      }
  }

  mbp = putals("Beacons:\r");
  for (port = 0, beapoi = beacon; port < L2PNUM; ++port, ++beapoi) {
    if (beapoi->interval != 0) {
      if (userpo->sysflg == 2)
      beapoi->beatim = 0;
      putnum(port, mbp);
      putchr(' ', mbp);
      putnum(beapoi->interval, mbp);
      putchr(' ', mbp);
      putnum(beapoi->telemetrie, mbp);
      putchr(' ', mbp);
      putid(beapoi->beades, mbp);
      putdil(beapoi->beadil, mbp);
      putchr('\r', mbp);
      putstr(beapoi->text, mbp);
      putchr('\r', mbp);
    }
  }
  prompt(mbp);
  seteom(mbp);
}

/**************************************************************************/
/* READ - Datei lesen                                                     */
/*------------------------------------------------------------------------*/
void ccpread(char *text)                 /* READ - Befehl (Dateien lesen) */
{
  char file[MAXPATH+1];

  MBHEAD *mbp;
#ifdef __WIN32__
  int a;
#endif

/* wenn text == NULL, dann war es ein Readbefehl
   ansonsten enthaelt text entweder den Namen einer Infodatei
   oder einer Tempdatei
   ob die Datei geloescht werden muss, erfaehrt man,
    wenn userpo->fname != NULL ist
*/
  if (text != NULL)
  {
    normfname(text);
    if (strpbrk(text, ":\\/") == NULL) {
      strcpy(file, textpath);
      strcat(file, text);
    }
    else
      strcpy(file, text);
    userpo->fp = xfopen(file, "rt");
  }
  else
  {
    if (issyso())
     {
      strncpy(file, (char*)clipoi, MAXPATH);
      file[MAXPATH] = NUL;
#ifdef WIN32
        a = strlen((char*)clipoi);
        if (a == 0) {
    mbp = getmbp();
#ifdef SPEECH
    putstr(speech_message(142), mbp);
#else
    putstr("Sri, no text available!\r", mbp);
#endif
    prompt(mbp);
    seteom(mbp);
   return;
          }
#endif

     }
    userpo->fp = xfopen(file, "rt");
  }

  if (userpo->fp == NULL) {
    mbp = getmbp();
    if (text != NULL)
      if (strcmp(text, "CMD.TMP") == 0) {
#ifdef SPEECH
          putstr(speech_message(143), mbp);
#else
          putstr("CLI failed!\r", mbp);
#endif
        prompt(mbp);
        seteom(mbp);
        return;
      }
    if (userpo->fname != NULL)
    {
      free(userpo->fname);
      userpo->fname = NULL;
    }
#ifdef SPEECH
    putstr(speech_message(142), mbp);
#else
    putstr("Sri, no text available!\r", mbp);
#endif
    prompt(mbp);
    seteom(mbp);
  }
}

/**
 * Parameter-Auswertung fuer LOAD/EDIT.
 * Es kann immer nur ein EDIT/LOAD zur Zeit laufen, dafuer aber mit
 * vollen Pfaden. Dies ist wohl keine echte Einschraenkung, es sollten
 * sowieso nicht zwei Sysops auf einmal schrauben, das bringt nur Chaos.
 */
static BOOLEAN loadprm(void) {
  char name[MAXPATH], *c;

   if (issyso()) {
    if (!*loadname)
    {
      clipoi[clicnt] = NUL;
      if (sscanf((char *) clipoi, "%s", name) == 1)
      {
        /* TMP-File auf dem Pfad des Zielfiles wegen Rename */

        if (strpbrk(name, ":/\\") == NULL)
        {
          strcpy(loadname, textpath);
          strcpy(loadtmp, textpath);
        }
        else
        {
          strcpy(loadtmp, name);
          if ((c = strrchr(loadtmp, '/')) == NULL)
            if ((c = strrchr(loadtmp, '\\')) == NULL)
              c = strchr(loadtmp, ':');
          c++;
          *c = NUL;
        }
        strcat(loadname, name);
        strcat(loadtmp, "LOAD.TMP");
        return(TRUE);
      } else
#ifdef SPEECH
          putmsg(speech_message(188));
#else
          putmsg("Invalid filename!\r");
#endif
    } else
#ifdef SPEECH
        putmsg(speech_message(189));
#else
        putmsg("EDIT/LOAD in use by other Sysop\r");
#endif
  }
  else
    invmsg();

  return(FALSE);
}

/**************************************************************************/
/* EDIT                                                                   */
/*------------------------------------------------------------------------*/
void ccpedi(void)
{
  MBHEAD *mbp;
#ifdef EDITOR

  if (loadprm())
  {
#ifndef MC68302
    if ((loadfp = xfopen(loadtmp, "wt")) != NULL)
#else
    xremove(loadname);
    if ((loadfp = xfopen(loadname, "wt")) != NULL)
#endif
    {
      userpo->status = US_RTXT;
      mbp = getmbp();
#ifdef SPEECH
      putprintf(mbp, speech_message(175), loadname);
#else
      putprintf(mbp, "editing>%s\r"
                     "Enter text. End with '.' in a new line.\r", loadname);
#endif
      seteom(mbp);
    }
    else {
#ifdef SPEECH
        putmsg(speech_message(190));
#else
        putmsg("Open error!\r");
#endif
      loadname[0] = loadtmp[0] = NUL;
    }
  }
#else
  if (loadprm()) {
#ifndef MC68302
    if ((loadfp = xfopen(loadtmp, "wt")) != NULL)
#else
    xremove(loadname);
    if ((loadfp = xfopen(loadname, "wt")) != NULL)
#endif
    {
      userpo->status = US_RTXT;
      mbp = getmbp();
#ifdef SPEECH
      putprintf(mbp, speech_message(175), loadname);
#else
      putprintf(mbp, "editing>%s\r"
                     "Enter text. End with '.' in a new line.\r", loadname);
#endif
      seteom(mbp);
    }
    else {
#ifdef SPEECH
        putmsg(speech_message(190));
#else
        putmsg("Open error!\r");
#endif
      loadname[0] = loadtmp[0] = NUL;
    }
  }
#endif /* EDITOR */
}

/**************************************************************************/
/* LOAD                                                                   */
/*------------------------------------------------------------------------*/
void ccpload(void)
{
  MBHEAD *mbp;

  if (loadprm()) {
#ifndef MC68302
    if ((loadfp = xfopen(loadtmp, "wb")) != NULL)
#else
    xremove(loadname);
    if ((loadfp = xfopen(loadname, "wb")) != NULL)
#endif
    {
      userpo->status = US_WBIN;
      mbp = getmbp();
#ifdef SPEECH
      putstr(speech_message(144), mbp);
#else
      putstr("Waiting for AUTOBIN-Transfer...\r", mbp);
#endif
      seteom(mbp);
      checksum = 0L;
      crc = 0;
    }
    else {
#ifdef SPEECH
        putmsg(speech_message(191));
#else
        putmsg("File error!\r");
#endif
      loadname[0] = loadtmp[0] = NUL;
    }
  }
}

/**************************************************************************/
/* SysOp                                                                  */
/*------------------------------------------------------------------------*/
void ccpsys(void)                  /* SYSOP - Befehl (Als Sysop anmelden) */
{
#ifndef USER_PASSWORD
  WORD num;
  WORD i, j;
  MBHEAD *mbp;

  if (paswle != 0)
  {
    mbp = putals("");
    srand((UWORD)tic10);
    for (i = 0; i < 5; ++i) {
      do {
    do;
    while (((num = (rand()%256)) >= paswle) || (paswrd[num] == ' '));
    for (j = 0; j < i; ++j) {
      if ((userpo->paswrd[j] & 0xFF) == num)
      break;
    }
      } while (i != j);
#ifdef __WIN32__
      userpo->paswrd[i] = (unsigned char)num;
#else
      userpo->paswrd[i] = num;
#endif /* WIN32 */
      putchr(' ', mbp);
      putnum((num + 1), mbp);
    }
    putchr('\r', mbp);
    seteom(mbp);
    for (i = 0; i < 5; ++i)                     /* Antwort aufbauen */
      userpo->paswrd[i] = paswrd[userpo->paswrd[i]];
    userpo->status = US_WPWD;
  }
#else
  WORD     num;
  WORD     i, j;
  MBHEAD  *mbp;
  FILE    *fp;
  BOOLEAN  found = FALSE;
  char    *pwd = paswrd;
  int      pwl = paswle;
  char     userpwd[100];
  char     call1[L2IDLEN];
  char     call2[L2IDLEN];
  char     line[256];
  char     fn[MAXPATH];
  char    *c;

  if (userpo->pwdtyp == PW_NOPW)
  {
    cpyid(call1, calofs(UPLINK, userpo->uid));
    if ((fp = xfopen("PERMS.TNN", "rt")) != NULL)
    {
      while (fgets(line, 256, fp) != NULL)
      {
        if (line[0] == '#')             /* Kommentarzeile ignorieren    */
          continue;
        str2call(call2, line);          /* Call steht am Zeilenanfang   */
        if (cmpcal(call1, call2))       /* User gefunden?               */
        {
          sscanf(line, "%*s %s", fn);   /* Filename steht an 2. Stelle  */
          found = TRUE;                 /* Password-File gefunden       */
          break;
        }
      }
      fclose(fp);
    }
    if (found)                          /* wenn Password-File angegeben */
    {
      if ((fp = xfopen(fn, "rt")) != NULL)      /* existiert PWD-File?  */
      {
        fgets(userpwd, 81, fp);                 /* Password lesen       */
        if ((c = strchr(userpwd, '\n')) != NULL)
          *c = NUL;
        if ((i = strlen(userpwd)) >= 5)         /* min. 5 Zeichen!      */
        {
          pwd = userpwd;
          pwl = i;
        }
        else
          found = FALSE;                /* war nix - also globales PWD  */
        fclose(fp);
      }
    }
  }

  if (pwl >= 5)
  {
    mbp = putals("");
    srand((UWORD)tic10);
    for (i = 0; i < 5; ++i)
    {
      do
      {
        do; while (((num = (rand()%256)) >= pwl) || (pwd[num] == ' '));
        for (j = 0; j < i; ++j)
        {
          if ((userpo->paswrd[j] & 0xFF) == num)
            break;
        }
      } while (i != j);
#ifdef __WIN32__
      userpo->paswrd[i] = (UBYTE)num;
#else
      userpo->paswrd[i] = num;
#endif
      putchr(' ', mbp);
      putnum((num + 1), mbp);
    }
    putchr('\r', mbp);
    seteom(mbp);
    for (i = 0; i < 5; ++i)                     /* Antwort aufbauen     */
      userpo->paswrd[i] = pwd[userpo->paswrd[i]];
    userpo->status = US_WPWD;
    if (found)                                  /* war User-Password    */
      userpo->status = US_WUPW;
   }
#endif
}

/************************************************************************/
/* USER                                                                 */
/*----------------------------------------------------------------------*/
void ccpuse(void)
{
  WORD    port;
  MBHEAD *mbp;
  #define USE_ALL   255
  #define USE_MASK  254
  #define USE_CALL  253
  #define USE_CONV  252
  #define USE_HOST  251
  char    call[L2IDLEN+1];
  char    mask[MAXMASK];
  int     i;

/* Titelzeile in neuen Buffer                                           */
  mbp = putals(" ");
  putprintf(mbp, "%s%d)", signon, nmbfre);
  i = mbp->mbpc;

/* Ueberpruefen, ob Befehl mit Parametern eingegeben wurde...           */
  if (skipsp(&clicnt, &clipoi))
  {
    if (getport(&clicnt, &clipoi, &port)) {
      /* Bei U Port alle L2-Uses dieses Ports zeigen */
      l2user(mbp, port, "");
      if (i == mbp->mbpc) putchr('\r', mbp);
      prompt(mbp);
      seteom(mbp);
      return;
    }

    if (strchr(clipoi, '+'))
    {
      l2user(mbp, USE_ALL, "");   /* Level 2 User in Tabellenform anzeigen */
      l4user(mbp, USE_ALL, "");   /* Level 4 User in Tabellenform anzeigen */
#ifdef L1TCPIP
      TcpipUser(mbp, USE_ALL, "");/* Tcpip-User in Tabellenform anzeigen.  */
#endif /* L1TCPIP */
      hostuser(mbp, USE_ALL, ""); /* Host-User in Tabellenform anzeigen    */
    } else
    if (toupper(*clipoi) == 'C' && clicnt == 1)
    {
      l4user(mbp, USE_ALL, "");  /* Bei U C nur die L4 User ausgeben         */
    } else
    if (toupper(*clipoi) == 'L' && clicnt == 1)
    {
      l2user(mbp, USE_ALL, "");  /* Bei U L alle L2 user anzeigen            */
    } else
    if (toupper(*clipoi) == 'H' && clicnt == 1)
    {
      ptcuser(mbp, USE_CONV, "");
      ptcuser(mbp, USE_HOST, "");
#ifdef L1TCPIP
      TcpipUser(mbp, USE_ALL, ""); /* Telnet-User in Tabellenform anzeigen*/
#endif /* L1TCPIP */
      hostuser(mbp, USE_ALL, "");
    } else
    if (getcal(&clicnt, &clipoi, TRUE, call) == YES) {
      ptcuser(mbp, USE_CALL, call);
      l2user(mbp, USE_CALL, call);
      l4user(mbp, USE_CALL, call);
#ifdef L1TCPIP
      TcpipUser(mbp, USE_CALL, call);/* Telnet-User in Tabellenform anzeigen */
#endif /* L1TCPIP */
      hostuser(mbp, USE_CALL, call);
    } else
    if (mhprm(clipoi, clicnt, mask) == TRUE) {
      ptcuser(mbp, USE_MASK, mask);
      l2user(mbp, USE_MASK, mask);
      l4user(mbp, USE_MASK, mask);
#ifdef L1TCPIP
      TcpipUser(mbp, USE_MASK, mask); /* Telnet-User in Tabellenform anzeigen*/
#endif /* L1TCPIP */
      hostuser(mbp, USE_MASK, mask);
    }
  if (i == mbp->mbpc) putchr('\r', mbp);
  }
  else
    ptcuser(mbp, USE_ALL, "");
  prompt(mbp);
  seteom(mbp);
}

/*
 * Formatierte Ausgabe einer Zahl 7stellig plus Suffix (k, M, G)
 */
static void put_kMG(MBHEAD *mbp, ULONG num) {
  const char *kMG = " kMG";
  int   suffix = 0;

  while ((num > 999999L) && (suffix < 3)) {
    num /= 1024L;
    suffix++;
  }
  putprintf(mbp, " %6lu%c", num, kMG[suffix]);
}

/************************************************************************/
/*                                                                      */
/* Formatierte Ausgabe evtl. sehr grosser Zahlen mit Punkten.           */
/* Die tatsaechliche Zahl besteht aus den Teilen "num" und "millions",  */
/* die zusammengerechnet werden muessen zu "num + millions * 1000000".  */
/*                                                                      */
/************************************************************************/
static void
put_pktnum(MBHEAD *mbp, ULONG millions, ULONG num)
{
  char  str[20];
  char *p;
  int   len;
  int   i;

  sprintf(str, "%lu%06lu", millions, num);
  p = str;
  while (*p == '0')
    p++;
  for (i = len = (int) strlen(p); i > 0; i--)
  {
    if ((i % 3) == 0)
      if (i != len)
        putchr(',', mbp);
    putchr(*p++, mbp);
  }
}

/*
 * Baudrate, Counter und Connect-Zeit aus der Patchcord-Tabelle ausgeben.
 */
static void putptcinfo(PTCENT *ptcp, MBHEAD *mbp) {
  ULONG Baud = (ptcp->rxbps + ptcp->txbps) * 8;
#ifndef CONNECTTIME
  ULONG d, h, m, s;
  /* Die Ausgabe aller Werte erfolgt 7stellig plus Suffix (k, M, G) */
#endif /* CONNECTTIME */

  put_kMG(mbp, ptcp->inforx);            /* empfangene Bytes     */
  put_kMG(mbp, ptcp->infotx);            /* gesendete Bytes      */
  put_kMG(mbp, Baud);                    /* errechnete Baudrate  */

#ifndef CONNECTTIME
  d = ptcp->contime;
  s = d % 60L; d /= 60L;
  m = d % 60L; d /= 60L;
  h = d % 24L; d /= 24L;
  if (d < 1L)
    putprintf(mbp, " %2lu:%02lu:%02lu", h, m, s); /* hh:mm:ss */
  else
  if (d < 99L)
    putprintf(mbp, " %2lu/%02lu:%02lu", d, h, m); /* dd/hh:mm */
  else
    putprintf(mbp, "%5lu/%02lu", d, h);          /* ddddd/hh */
#else /* CONNECTTIME */
  putprintf(mbp, "  %s", ConnectTime(ptcp->contime));
#endif /* CONNECTTIME */
}

/*------------------------------------------------------------------------*/
/*
 *   Level 2 User in Tabellenform anzeigen:
 *
 *   Po SrcCall   DstCall   LS  Rx Tx Tr SRTT    RxkB     TxkB   Baud   ConTime Pri
 *   -------------------------------------------------------------------------
 *    0 DD1FR     DB0KH     IXF  2 10  3 1234     3456   72345  122   0:45:16   0
 *    1 DB0KH     DF7ZE     REJ  0  0  0  254      652   52345  345   1:23:01  10
 *   /  /        /        /   /   /  /    /        /       /    /        /   /
 * 0)  1)      2)       3)  4)  5) 6)   7)       8)      9)  10)      11) 12)
 *   0)  Port
 *   1)  Quellrufzeichen des L2-QSOs
 *   2)  Zielrufzeichen des L2-QSOs
 *   3)  L2-Link-Status:
 *         SET = Link-Setup
 *         FMR = Frame Reject
 *         DRQ = Disconnect Request
 *         IXF = Info Transfer
 *         REJ = REJ Sent
 *         WAK = Waiting Ackknowledge
 *         DBS = Device Busy
 *         RBS = Remote Busy
 *         BBS = Both Busy
 *         WDB = Waiting Ack And Device Busy
 *         WRB = Waiting Ack And Remote Busy
 *         WBB = Waiting Ack And Both Busy
 *         RDB = REJ Sent and Device Busy
 *         RRB = REJ Sent and Remote Busy
 *         RBB = REJ Sent and Both Busy
 *         HTH = HTH waiting
 *   4)  Anzahl der empfangenen Frames in der Warteschlange fuer diesen
 *       Link
 *   5)  Anzahl der noch zu sendenden Frames in der Warteschlange fuer
 *       diesen Link
 *   6)  Anzahl Retries
 *   7)  Stand des 'Smoothed Round Trip Timers'
 *   8)  Anzahl empfangender Bytes seit Bestehen des Links
 *   9)  Anzahl gesendetet Bytes seit Bestehen des Links
 *   10) Aus 8) + 9) errechnete effektive Baudrate fuer diesen Link
 *   11) Connectzeit
 *   12) Bei DAMA-Netzeinstiegen: aktuelle Prioritaet des Users
 *       (0 = hoechste Prioritaet)
 *  */

static void l2user(MBHEAD *mbp, WORD what, const char *pstr)
{
  LNKBLK *lp;
  char    lsts[] = {"DISSETFMRDRQIXFREJWAKDBSRBSBBSWDBWRBWBBRDBRRBRBBHTH"};
  char    tmp1[10],
          tmp2[10];
  LHEAD  *actlp;
  PTCENT *ptcp;
  int     port;
  BOOLEAN first = TRUE;

  for (port = 0, actlp = &l2actl[0]; port < L2PNUM; port++, actlp++) {
    for (lp  = (LNKBLK *) actlp->head;
         lp != (LNKBLK *) actlp;
         lp  = lp->next) {
      switch (what) {
        case USE_ALL  : break;
        case USE_MASK : if (   !c6mtch(lp->dstid, pstr)
                            && !c6mtch(lp->srcid, pstr)) continue;
                        break;
        case USE_CALL : if (   !cmpid(lp->dstid, pstr)
                            && !cmpid(lp->srcid, pstr)) continue;
                        break;
        default       : if (what < L2PNUM && port != what) continue;
      }
      ptcp = ptctab + g_uid(lp, L2_USER);
      mbp->l4time = mbp->mbpc;
      call2str(tmp1, lp->srcid);
      call2str(tmp2, lp->dstid);
      if (first) {
        putstr("\rL2 - User:\r", mbp);
        putstr("Po SrcCall   DstCall   LS  Rx Tx Tr SRTT    RxB     TxB    Baud   ConTime Pr Da\r", mbp);
        putstr("-------------------------------------------------------------------------------\r", mbp);
        first = FALSE;
      }
      putprintf(mbp, "%2d %-9.9s %-9.9s %3.3s%3u%3u%3u%5u",
                     lp->liport,                /* Port nr user         */
                     tmp1,                      /* Quell-Rufzeichen     */
                     tmp2,                      /* Ziel-Rufzeichen      */
                     &lsts[lp->state * 3],      /* Link-Status          */
                     lp->rcvd,                  /* Frames in RX-Queue   */
                     lp->tosend,                /* Frames in TX-Queue   */
                     lp->tries,                 /* Link-Retries         */
                     lp->SRTT);                 /* Round Trip Timer     */

      putptcinfo(ptcp, mbp);

      if (dama(lp->liport))
        putprintf(mbp, " %2d %2d\r",   /* DAMA-Prioritaet       */
                  lp->damapm,
                  portpar[lp->liport].dch + 1);
      else
        putstr("  -\r", mbp);
    }
  }
}

/*------------------------------------------------------------------------*/

/*
 *   Level 4 User in Tabellenform anzeigen:
 *
 *   Call      Node              S   Rx  Tx Tr Win    RxB      TxB   Baud   ConTime
 *   ------------------------------------------------------------------------------
 *   DL9HCJ    HHOST :DB0HHO    IXF   0   1  0  10     4321    45621  153  01:33:12
 *   DG9FU     KS    :DB0EAM    IXF   0   0  0  10    87554    12874  743  01:59:03
 *    /         /                /   /   /  /   /        /        /    /        /
 *    1)        2)               3)  4)  5) 6)  7)       8)       9)  10)      11)
 *
 *   1)  Rufzeichen des Users
 *   2)  Ident und Call des Knotens an dem der User eingeloggt ist
 *   3)  L4-Circuit-Status:
 *         SET = Circuit-Setup
 *         IXF = Info-Transfer
 *         DRQ = Disconnect-Request
 *   4)  Anzahl der empfangenen Frames in der Warteschlange fuer diesen
 *       Circuit
 *   5)  Anzahl der noch zu sendenden Frames in der Warteschlange fuer
 *       diesen Circuit
 *   6)  Anzahl Transport-Retries
 *   7)  Transport Fenstergroesse
 *   8)  Anzahl empfangender Bytes seit Bestehen des Circuits
 *   9)  Anzahl gesendetet Bytes seit Bestehen des Circuits
 *   10) Aus 8) + 9) errechnete effektive Baudrate fuer diesen Circuit
 *   11) Connectzeit
 *
 */

static void l4user(MBHEAD *mbp, WORD what, const char *pstr)
{
  CIRBLK *p;
  WORD    i;
  char    lsts[] = {"-SID"};  /* {"---SETIXFDRQ"}; */
  char    tmp1[10],
          tmp3[10];
  PTCENT *ptcp;
  BOOLEAN first = TRUE;

  /*
   *    Circuit-Tabelle durchgehen und fuer alle nicht disconnecteten
   *    Circuits Info anzeigen
   */

  for (p = cirtab, i = 0; i < NUMCIR; ++p, ++i) {
    if (p->state != 0) {                        /* nur aktive Circuits  */
      switch (what) {
        case USE_ALL  : break;
        case USE_MASK : if (!c6mtch(p->upcall, pstr)) continue;
                        break;
        case USE_CALL : if (!cmpid(p->upcall, pstr)) continue;
                        break;
      }
      ptcp = ptctab + g_uid(p, L4_USER);
      mbp->l4time = mbp->mbpc;
      call2str(tmp1, p->upcall);

      call2str(tmp3, p->l3node);
      /*find_best_qual((int)(np-netp->nodetab), &bestpp, DG);
      call2str(tmp2, bestpp->l2link->call);*/

      if (first) {
#ifdef SPEECH
        putstr(speech_message(146), mbp);
#else
        putstr("\rL4 - User:\r", mbp);
        putstr("Call      Node       S  Rx  Tx Tr Win SRTT     RxB     TxB    Baud   ConTime\r", mbp);
#endif
        putstr("-----------------------------------------------------------------------------\r", mbp);
        first = FALSE;
      }

      putprintf(mbp, "%-9.9s %-9.9s %c%c%c%3u %3u %2u %2u %5u ",
                     tmp1,                      /* User-Call            */
                     tmp3,                      /* Node                 */
                     ((p->l4flag & L4FBUSY) ? '.':' '),
                     lsts[(int)p->state],       /* Circuit Status       */
                     ((p->l4flag & L4FPBUSY) ? '.':' '),
                     p->numrx,                  /* Frames in RX-Queue   */
                     p->numtx,                  /* Frames in TX-Queue   */
                     p->l4try,                  /* Transport-Retries    */
                     p->window,                 /* Transport-Window     */
                     p->SRTT);

      putptcinfo(ptcp, mbp);

      putchr('\r', mbp);
    }
  }
}


/*----------------------------------------------------------------------*/
/*                                                                      */
/*----------------------------------------------------------------------*/
void viaput(LINKTYP seite, UID uid, MBHEAD *mbp)
{
  UBYTE       typ = g_utyp(uid);
  LNKBLK     *lp;
  CIRBLK     *cp;
  const char *p;
#ifdef USER_AUSGABE
  int         index;
  NODE       *np;
#endif /* USER_AUSGABE */

  if (typ == L2_USER) {
    lp = g_ulink(uid);
    p = ndigipt(lp->viaidl);
    if ((*p != NUL) || (!updmheard(lp->liport)))
    {
#ifndef USER_AUSGABE
      putstr(seite == UPLINK ? "\r Uplink" : "\r Downlink", mbp);
      if (*p == NUL)         /* ohne Digis            */
        putprintf(mbp, " %s", portpar[lp->liport].name);
      else
        putdil(p, mbp);
#else
      if (*p != NUL)         /* Ausgabe nur mit Digis            */
      {
        putstr(seite == UPLINK ? "\r Uplink" : "\r Downlink", mbp);
        putdil(p, mbp);
      }
#endif /* USER_AUSGABE */
     }
  } else {
#ifdef USER_AUSGABE
      if (typ == L4_USER)
#else
      if (typ == L4_USER && seite == UPLINK)     /* User ist Circuit     */
#endif /* USER_AUSGABE */
    {
/************************************************************************\
*                                                                        *
* Die Uplinkinformation wird nur angezeigt, wenn entweder der letzte     *
* Absenderknoten nicht der Uplinkknoten ist oder aber wenn beim Uplink   *
* Digis verwendet wurden. Dann steht in einer eigenen Zeile:             *
* ' Uplink @ uplinkknoten via digikette'                                 *
*                                                                        *
\************************************************************************/
#ifdef USER_AUSGABE
        cp = g_ulink(uid);
        if (  (!cmpid(cp->downca, cp->upnod))
            ||(*(cp->upnodv) != NUL))
        {
          putstr(seite == UPLINK ? "\r Uplink @ " : "\r Downlink @ ", mbp);

          index = find_node_this_ssid(cp->downca);

          if (index != -1)
          {
            np = netp->nodetab+index;
            putalt(np->alias, mbp);
        }

        putid(cp->downca, mbp);
#else
        cp = g_ulink(uid);
        if (  (!cmpid(cp->upnod, cp->downca))
            ||(*(cp->upnodv) != NUL))
        {
          putstr("\r Uplink @ ", mbp);
          putid(cp->upnod, mbp);
          putdil(cp->upnodv, mbp);
#endif /* USER_AUSGABE */
      }
    }
  }
}

#ifdef USER_MONITOR
void puttraceu(USRBLK *u, MBHEAD *mbp)
{
  UID    uid = u->uid;

    putuse(UPLINK, uid, mbp);
    putspa(38, mbp);
    putstr("<--> (", mbp);
    putid(calofs(UPLINK, uid), mbp);
    putspa(53,mbp);
    putstr("<> Tracen)", mbp);
    putspa(65,mbp);
    putstr(" PO ", mbp);
    putprintf(mbp,"%d",u->monitor->Mport);
}
#endif /* USER_MONITOR */

/* Ausgeben der Patchcordliste, eventuell auch nur fuer ein Call oder eine */
/* Menge von Calls.                                                        */
static void ptcuser(MBHEAD *mbp, WORD what, const char *pstr)
{
  LINKTYP updown;
  PTCENT *ptcp;
  UID     uid;
  WORD    cvsflg = 0;

  if (what == USE_CONV) {
    cvsflg = 1;
    what = USE_ALL;
  }

  /*
   * Ausgegeben werden alle Uplinks aus der Patchcord mit ihren Downlinks
   * und die CCP-User (Convers, echte User usw)
   */
  if (what != USE_HOST)
    putchr('\r', mbp);
  for (uid = 0, ptcp = ptctab; uid < NUMPAT; ++uid, ptcp++)
  {
    if (ptcp->state == 0) continue;
    switch (what) {
      case USE_ALL  : break;
      case USE_MASK : if (   (!c6mtch(calofs(UPLINK, uid), pstr))
                          && (!c6mtch(calofs(DOWNLINK, uid), pstr)))
                        continue;
                      break;
      case USE_CALL : if (   (!cmpid(calofs(UPLINK, uid), pstr))
                          && (!cmpid(calofs(DOWNLINK, uid), pstr)))
                        continue;
                      break;
      case USE_HOST : if (   (g_utyp(uid) != HOST_USER)
                          && (g_utyp(ptcp->p_uid) != HOST_USER))
                        continue;
                      break;
    }
    mbp->l4time = mbp->mbpc;
    switch (ptcp->state) {
      case UPLINK :               /* kommt vom CCP (Convers)          */
                  if (ptcp->ublk) {         /* ist der im CCP?                  */
          if (   (!cvsflg && ptcp->ublk->convflag == TRUE)
              || (cvsflg && !ptcp->ublk->convflag == TRUE))
                continue;
          if (ptcp->ublk->convers != NULLCONNECTION) {
            putcvsu(ptcp->ublk, mbp);    /* Convers-Verb.             */
            putchr('\r', mbp);
            continue;
          }
#ifdef USER_MONITOR
          if (ptcp->ublk->status == US_TRAC)
          {
            puttraceu(ptcp->ublk, mbp);    /* Trace-Verb.             */
            putchr('\r', mbp);
            continue;
          }
#endif /* USER_MONITOR */
        }
        break;
#ifndef USER_AUSGABE
      case D_IPLINK :
#endif /* USER_AUSGABE */
      case U_IPLINK :
        updown = ptcp->state == D_IPLINK ? DOWNLINK : UPLINK;
        putuse(updown, uid, mbp);
        putspa(38, mbp);
#ifdef USER_AUSGABE
        putstr("<--> (", mbp);
        putid(calofs(UPLINK, uid), mbp);
        putspa(53,mbp);
        putstr("<> IP-Router)", mbp);
#else /* USER_AUSGABE */
        putstr("<-->  IP-Router", mbp);
#endif /* USER_AUSGABE */
        viaput(updown, uid, mbp);
        putchr('\r', mbp);
        continue;

#ifdef L1IPCONV
      case C_IPLINK :
        updown = ptcp->state == D_IPLINK ? DOWNLINK : UPLINK;

        if (ptcp->ublk->convers == NULL)             /* Login fehlgeschlagen. */
          continue;                                 /* zum naechsten Eintrag. */

        putuse(updown, uid, mbp);
        putspa(38, mbp);
#ifdef USER_AUSGABE
        putstr("<--> (", mbp);
        putid(calofs(UPLINK, uid), mbp);
        putspa(53,mbp);
        putstr("<> Convers)", mbp);
        putspa(65,mbp);
        putstr("Ch ", mbp);

#ifndef L1IRC
        if (ptcp->ublk != NULL)
          putlong(ptcp->ublk->convers->channel, 0, mbp);
#else
        if (ptcp->ublk != NULL)
        {
          if (ptcp->ublk->convers->channel == EOF)
            putstr(" no Channel", mbp);
          else
            putlong(ptcp->ublk->convers->channel, 0, mbp);
        }
#endif /* L1IRC */
#else /* USER_AUSGABE */
        putstr("<-->  Convers", mbp);
#endif /* USER_AUSGABE */
        putchr('\r', mbp);
        continue;
#endif /* L1IPCONV / L1IRC */

      default:
        continue;
    }
    if (cvsflg)
      continue;
    putuse(UPLINK, uid, mbp);
    if (ptcp->p_uid != NO_UID) {  /* gibts einen Partner-Link?        */
      putspa(38, mbp);
      putstr(ptcp->ublk ? "<..> " : "<--> ", mbp);
      if (ptcp->ublk)
        if (ptcp->ublk->status == US_CQ)
         {
          putuse(CQ_LINK, uid, mbp);
          continue;
         }
        putuse(DOWNLINK, ptcp->p_uid, mbp);
    }
    viaput(UPLINK, uid, mbp);
    if (ptcp->p_uid != NO_UID)
      viaput(DOWNLINK, ptcp->p_uid, mbp);
    putchr('\r', mbp);
  }
}


#ifdef L1TCPIP
static void TcpipUser(MBHEAD *mbp, WORD what, const char *pstr)
{
  TCPIP  *tpoi;
  int     i;
  char    tmp1[10];
  char    tmp2[10];
  PTCENT *ptcp;
  BOOLEAN first = TRUE;
  static char *Interface[] = {"Telnet","Httpd ","IPConv"};

  for (i = 0, tpoi = tcptbl; i < MAXTCPIP; ++i, ++tpoi)
  {
    if (   tpoi->activ == TRUE
                && tpoi->login == TRUE)
    {
    switch (what)
     {
      case USE_ALL  : break;
      case USE_MASK : if (!c6mtch(tpoi->Upcall, pstr)) continue;
                      break;
      case USE_CALL : if (!cmpid(tpoi->Upcall, pstr)) continue;
                      break;
     }
        call2str(tmp1, tpoi->Upcall);
        call2str(tmp2, tpoi->Downcall);

    ptcp = ptctab + g_uid(tpoi, TCP_USER);
    if (first)
     {
      putstr("\rTCPIP-User:\r"
             "Interf  SrcCall   DstCall      T3     RxB     TxB    Baud   ConTime\r"
             "-------------------------------------------------------------------\r", mbp);
      first = FALSE;
        }
    putprintf(mbp, "%6s: %-9.9s %-9.9s %5u ",
                    Interface[tpoi->Interface - KISS_TCPIP]
                                        ,tmp1
                                        ,tmp2
                                        ,tpoi->noacti);

    putptcinfo(ptcp, mbp);
    putchr('\r', mbp);
        }
   }
}
#endif /* L1TCPIP */

/*
 *
 *   Host-User in Tabellenform anzeigen:
 *
 *   CH Call      F     NT    RX    TX    ST     RxB     TxB    Baud   ConTime
 *   -------------------------------------------------------------------------
 *   03 DB0IL-3   CD  7199     1     0     0      65    1494     120   0:02:05
 *   05 DB0IL-5   C   7086     0     0     0      51     595      56   0:02:05
 *   06 OZ5BBS-1  C   7068     0     0     0    7620    1598      96   0:09:37
 *    /  /        /      /     /     /     /       /       /       /         /
 *   1) 2)       3)     4)    5)    6)    7)      8)      9)      10)       11)
 *
 *   1)  Hostmode-Kanal
 *   2)  Rufzeichen des Users (Connect vom User) oder eigenes Call (Connect
 *       vom Host zum Knoten)
 *   3)  Hostmode-Flags
 *         C = Connected
 *         D = Disconnecten wenn Info uebertragen
 *   4)  Noactivity-Timer
 *   5)  empfangene Frames in Warteschlange
 *   6)  zu sendende Frames in Warteschlange
 *   7)  Statusmeldungen in Warteschlange
 *   8)  Anzahl empfangender Bytes seit Bestehen des Links
 *   9)  Anzahl gesendetet Bytes seit Bestehen des Links
 *   10) Aus 8) + 9) + 11) errechnete effektive Baudrate fuer diesen Link
 *   11) Connectzeit
 *
 */

static void
hostuser(MBHEAD *mbp, WORD what, const char *pstr)
{
  HOSTUS  *hp;
  int     i;
  char    str[10];
  PTCENT *ptcp;
  BOOLEAN first = TRUE;

  for (hp = hstubl + 1, i = 1; i < MAXHST; ++hp, ++i)
   {
    if (   (!hp->conflg)
        && (!hp->disflg)
        && (cmpid(hp->call, hostid))
       )
      continue;
    switch (what)
     {
      case USE_ALL  : break;
      case USE_MASK : if (!c6mtch(hp->call, pstr)) continue;
                      break;
      case USE_CALL : if (!cmpid(hp->call, pstr)) continue;
                      break;
     }
    call2str(str, hp->call);
    if (first)
     {
#ifdef SPEECH
     putstr(speech_message(149), mbp);
#else
     putstr("\rHost-User:\r"
             "CH Call      F     NT    RX    TX    ST     RxB     TxB    Baud   ConTime\r"
             "-------------------------------------------------------------------------\r", mbp);
#endif
      first = FALSE;
     }
    putprintf(mbp, "%02ld %-9.9s %c%c %5u %5u %5u %5u ",
                    hp - hstubl, str, (hp->conflg ? 'C':' '),
                    (hp->disflg ? 'D':' '), hp->noacti, hp->inlin,
                    hp->outlin, hp->outsta);
    ptcp = ptctab + g_uid(hp, HOST_USER);
    putptcinfo(ptcp, mbp);
    putchr('\r', mbp);
   }
}

/************************************************************************/
/* Time                                                                 */
/*----------------------------------------------------------------------*/
void ccptim(void)
{
#ifdef MC68K
  UWORD t1, t2, t3;
  char t4;
  struct tm *tim;
#endif
  MBHEAD *mbp;

#ifdef MC68K
  if (issyso()) {
    t1 = nxtnum(&clicnt, &clipoi);
    t4 = *clipoi++;
    clicnt--;
    t2 = nxtnum(&clicnt, &clipoi);
    clipoi++;
    clicnt--;
    t3 = nxtnum(&clicnt, &clipoi);
    if (t4 == '.') {
      if (t1 <= 31 && t2 <= 12 && t3 <= 99) {
    if (t3 < 92)
         t3 += 100;
    tim = localtime(&sys_time);
    tim->tm_mday = t1;
    tim->tm_mon = t2 - 1;
    tim->tm_year = t3;
    sys_time = mktime(tim);
    stime(&sys_time);
      }
    }
    if (t4 == ':') {
      if (t1 <= 23 && t2 <= 59 && t3 <= 59)
          {
                tim = localtime(&sys_time);
                tim->tm_hour = t1;
                tim->tm_min = t2;
                tim->tm_sec = t3;
                sys_time = mktime(tim);
                stime(&sys_time);
      }
    }
  }
#endif
  mbp = putals("Time: ");
  puttim(&sys_time, mbp);
  putstr(" (", mbp);
  putlong(tic10, FALSE, mbp);
  putstr(")\r", mbp);
  prompt(mbp);
  seteom(mbp);
}

/************************************************************************/
/* CLEAR : Loeschen der Statistiken                                     */
/*----------------------------------------------------------------------*/
void
ccpclr(void)
{
  register int  i;
  STAT          *statp;
  char          call[L2IDLEN];
  char          viacall[L2IDLEN];

  if (issyso())
  {
    memset(portstat, 0, sizeof(portstat));

    for (i = 0, statp = mh; i < MAXSTAT; ++i, statp++)
    {
/* Rufzeichen und via-Rufzeichen beibehalten, Start-Zeit setzen         */
      cpyid(call, statp->call);
      cpyid(viacall, statp->viacall);
      memset(statp, 0, sizeof(STAT));
      cpyid(statp->call, call);
      cpyid(statp->viacall, viacall);
      statp->hfirst = sys_time;
    }
    for (i = 3; i <= 19; ++i)
      if (i != 13)                      /* Timeout nicht ruecksetzen    */
        Ip_mib[i].value.integer = 0;

    l1sclr("*");

    time(&clear_time);
#ifdef SPEECH
    putmsg(speech_message(192));
#else
    putmsg("Statistic-table cleared!\r");
#endif
    userpo->sysflg = 2;
  }
  else
    invmsg();
}

/************************************************************************/
/* percent                                                              */
/* int comma: gibt an wieviele Nachkommastellen berechnet werden sollen */
/*----------------------------------------------------------------------*/
static void
percent (ULONG zaehler, ULONG nenner, int comma, int space, MBHEAD *mbp)
{
   int  i, zaehlernull;
   char resultstr[32];

   resultstr[0] = NUL;
   zaehlernull = 0;
   if (nenner == 0)
   {
     zaehler = 0L;
     nenner = 1L;
   }

   if (zaehler / nenner != 0)
     sprintf(resultstr, "%s%lu", resultstr, zaehler / nenner);
   else
     zaehlernull = 1;
   zaehler = (zaehler % nenner) * 10;

   if (zaehlernull != 1 || zaehler / nenner != 0)
     sprintf(resultstr, "%s%lu", resultstr, zaehler / nenner);
   zaehler = (zaehler % nenner) * 10;

   sprintf(resultstr, "%s%lu", resultstr, zaehler / nenner);
   zaehler = (zaehler % nenner) * 10;
   if (comma > 0)
     strcat(resultstr, ".");

   for (i = 0 ; i < comma; i++)
   {
     sprintf(resultstr, "%s%lu", resultstr, zaehler / nenner);
     zaehler = (zaehler % nenner) * 10;
   }
/* String mit space Angabe formatieren                                  */
   putprintf(mbp, "%*s%%", space, resultstr);
   return;
}

/************************************************************************/
/* Statistik-Befehl                                                     */
/* Syntax: S + <call> <viacall> nimmt ein neues Call in die Statistik   */
/*                              auf                                     */
/*         S - <call> <viacall> loescht ein Call aus der Statistik      */
/*         S p                  zeigt nur die Port-Statistik            */
/*         S e                  zeigt nur die Fehler-Statistik          */
/*         S l                  zeigt nur die Link-Statistik            */
/*         S h                  zeigt nur die Hardware-Statistik        */
/*         S i                  zeigt nur die IP-statistik              */
/*         S                    zeigt nur die System-Statistik          */
/*         S *                  zeigt alles                             */
/*----------------------------------------------------------------------*/
void
ccpsta(void)
{
  MBHEAD   *mbp;
  STAT     *statp;
  WORD      i, j;
  char      call[L2IDLEN], viacall[L2IDLEN];
  ULONG     summe,                 /* Gesamter Datendurchsatz in KBytes */
            offset;
#ifndef CONNECTTIME
  LONG      upt, upd, uph;
#endif /* CONNECTTIME */
  PORTSTAT *pstatp;
  PORTINFO *portp;
  ULONG     rxoverhead = 0L, txoverhead = 0L;
  ULONG     trxframes[2];
  int       ch;
#define PORTSTATFLAG  1             /* Die Reihenfolge muss mit chars   */
#define ERRORSTATFLAG 2             /* uebereinstimmen                  */
#define LINKSTATFLAG  4
#define HARDWAREFLAG  8
#define IPSTATFLAG    16
#define KERNELFLAG    32
#define STATFLAG      64            /* kein oder falscher parameter     */
  int       flag = 0;
#ifndef KERNELIF
  const char *chars = "PELHI", *cp;
#else
  const char *chars = "PELHIK", *cp;
#endif

  mbp = getmbp();

  skipsp(&clicnt, &clipoi);
  ch = toupper(*clipoi);

  for (cp = chars, i = 1; *cp; cp++, i <<= 1)
    if (ch == *cp) flag |= i;

  if (ch == '*')
    flag = (~0);                   /* Monats-Statistik (alles)         */

  if (flag == 0)
    flag |= STATFLAG;

  if (issyso())
  {
/******* Delete Call from Statistics ************************************/
    if (ch == '-')
    {
      clipoi++;
      clicnt--;

      if (getcal(&clicnt, &clipoi, FALSE, call) == YES)
      {
        if (getcal(&clicnt, &clipoi, TRUE, viacall) == YES)
        {
          for (statp = mh, j = 0; j < MAXSTAT; statp++, j++)
            if (   cmpid(statp->call, call)
                && cmpid(statp->viacall, viacall))
            {
              statp->call[0] = NUL;
              statp->viacall[0] = NUL;
            }
        }
        else                       /* Kein oder falsches via Rufzeichen */
        {
          for (statp = mh, j = 0; j < MAXSTAT; statp++, j++)
            if (   cmpid(statp->call, call)
                && !*statp->viacall)
            {
              statp->call[0] = NUL;
              statp->viacall[0] = NUL;
            }
        }
      }
      flag |= LINKSTATFLAG;
    }

/******** Add Call to Statistics ****************************************/
    if (*clipoi == '+')
    {
      clipoi++;
      clicnt--;

/************************************************************************/
/* Wenn das Call schon in der Statistik steht, dann tragen wir es nicht */
/* nochmal ein. Rufzeichen und Via Rufzeichen duerfen nicht gleich      */
/* sein. Das Zeichen "*" ist nur erlaubt, wenn ein korrektes Via-       */
/* Rufzeichen mitangegeben wird.                                        */
/************************************************************************/

      if (getcal(&clicnt, &clipoi, FALSE, call) == YES)
/* Es ist ein "*" oder ein gueltiges Rufzeichen                         */
        if (cmpid(call, anycall) || fvalca(call) == YES)
        {
          if (   getcal(&clicnt, &clipoi, TRUE, viacall) == YES
              && !cmpid(call, viacall))
          {
            for (statp = mh, j = 0; j < MAXSTAT; statp++, j++)
              if (   cmpid(statp->call, call)
                  && cmpid(statp->viacall, viacall))
                break;
          }
          else                     /* Kein oder falsches Via-Rufzeichen */
          {
            viacall[0] = NUL;
            for (statp = mh, j = 0; j < MAXSTAT; statp++, j++)
            {
/* "*" ohne Via-Rufzeichen nicht zulassen                               */
              if (cmpid(call, anycall))
                break;
/* Rufzeichen (ohne via) schon gefunden, kein Eintrag noetig            */
              if (cmpid(statp->call, call) && !*statp->viacall)
                break;
            }
          }

          if (j >= MAXSTAT)
          {
            for (statp = mh, j = 0; j < MAXSTAT; statp++, j++)
              if (!*statp->call)
              {
                memset(statp, 0, sizeof(STAT));
                cpyid(statp->call, call);
                cpyid(statp->viacall, viacall);
                statp->hfirst = sys_time;
                statp->hlast = sys_time;
                userpo->sysflg = 2;
                break;
              }
          }
        }
      flag |= LINKSTATFLAG;
    }
  }
  summe = 0L;

/********** Show System Statistics **************************************/
#ifdef SPEECH
  putstr(speech_message(151), mbp);       /* aktuelle Uhrzeit      */
#else
  putstr("\r System Statistics: ", mbp);       /* aktuelle Uhrzeit      */
#endif
  puttim(&clear_time, mbp);
  putstr(" - ", mbp);
  puttim(&sys_time, mbp);
#ifdef SPEECH
  putstr(speech_message(152), mbp);       /* Startzeit             */
#else
  putstr("\r           Startup: ", mbp);       /* Startzeit             */
#endif
  puttim(&start_time, mbp);

#ifndef CONNECTTIME
  upt = sys_time - start_time;                 /* Uptime in seconds     */
  upd = upt / SECONDS_PER_DAY;                 /* Uptime days           */
  upt %= SECONDS_PER_DAY;
  uph = upt / SECONDS_PER_HOUR;                /* Uptime hours          */
  upt %= SECONDS_PER_HOUR;

  putprintf(mbp,  "\r            Uptime:%9ld/%02ld:%02ld\r",
                  upd, uph, upt / SECONDS_PER_MIN);
#else /* CONNECTTIME */
  putprintf(mbp, "\r           Runtime: %s", ConnectTime(tic10 / 100));
#endif /* CONNECTTIME */

  if (flag & STATFLAG)
  {
#ifdef SPEECH
    putprintf(mbp, speech_message(177),
                   rounds_min_sec, rounds_pro_sec, rounds_max_sec);
#else
    putprintf(mbp, "\r                       (min)    (now)    (max)\r"
                   "        Rounds/sec: %8lu %8lu %8lu\r",
                   rounds_min_sec, rounds_pro_sec, rounds_max_sec);
#endif
#ifdef SPEECH
    putprintf(mbp,  speech_message(178),
                        nmbfre_min, nmbfre, nmbfre_max,
                        thbps * 8L, thbps_max * 8L,
                        nmblks, nmblks_max,
                        nmbcir, nmbcir_max,
                        netp->num_nodes,
                        num_nodes_max);
#else
    putprintf(mbp,  "      Free Buffers: %8u %8u %8u\r"
                    "Overall Throughput:%18lu %8lu Baud\r"
                    "   Active L2-Links:%18u %8u\r"
                    "   Active Circuits:%18u %8u\r"
                    "      Active Nodes:%18u %8u\r",
                        nmbfre_min, nmbfre, nmbfre_max,
                        thbps * 8L, thbps_max * 8L,
                        nmblks, nmblks_max,
                        nmbcir, nmbcir_max,
                        netp->num_nodes,
                        num_nodes_max);
#endif
#ifdef L1TCPIP
    putprintf(mbp,   "\r     Active Socket:%18u %8u\r"
                         ,nmbtcp, nmbtcp_max);
#endif /* L1TCPIP */

#ifdef __LINUX__
    print_load (mbp);
    if (nmbfre_max)
#ifdef SPEECH
      putprintf(mbp, speech_message(180),
          100 - (((ULONG)nmbfre) * 100L) / ((ULONG)nmbfre_max));
#else
        putprintf(mbp, "\r      Buffer usage: %lu%%",
          100 - (((ULONG)nmbfre) * 100L) / ((ULONG)nmbfre_max));
#endif
#ifdef SPEECH
    putprintf(mbp,   speech_message(181),
          sizeof(NETWORK) + (ULONG)netp->num_peers * (sizeof(PEER) +
          (ULONG)netp->max_nodes * sizeof(ROUTE)));
#else
    putprintf(mbp,   "\r      Network Heap: %lu Bytes",
          sizeof(NETWORK) + (ULONG)netp->num_peers * (sizeof(PEER) +
          (ULONG)netp->max_nodes * sizeof(ROUTE)));
#endif
    putprintf(mbp,   "\r           Console: ");
    switch (console_type)
    {
        case CONS_NO_CONSOLE:       putprintf(mbp, "no console\r"); break;

        case CONS_TERM_DO_SETUP:
        case CONS_TERM_RUNNING:     putprintf(mbp, "terminal\r"); break;

        case CONS_SOCKET_DO_SETUP:  putprintf(mbp, "unix socket (setting up)\r"); break;
        case CONS_SOCKET_WAITING:   putprintf(mbp, "unix socket (waiting)\r"); break;
        case CONS_SOCKET_CONNECTED: putprintf(mbp, "unix socket (connected)\r"); break;

        default: putprintf(mbp, "unknown\r"); break;
    }
#else
    if (rounds_max_sec)
#ifdef SPEECH
      putprintf(mbp, speech_message(182),
          100 - (((ULONG)rounds_pro_sec) * 100L) / ((ULONG)rounds_max_sec));
#else
      putprintf(mbp, "\r          CPU load: %lu%%",
          100 - (((ULONG)rounds_pro_sec) * 100L) / ((ULONG)rounds_max_sec));
#endif
    if (nmbfre_max)
#ifdef SPEECH
      putprintf(mbp, speech_message(180),
          100 - (((ULONG)nmbfre) * 100L) / ((ULONG)nmbfre_max));
#else
      putprintf(mbp, "\r      Buffer usage: %lu%%",
          100 - (((ULONG)nmbfre) * 100L) / ((ULONG)nmbfre_max));
#endif
#ifdef SPEECH
      putprintf(mbp,   speech_message(181),
          sizeof(NETWORK) + (ULONG)netp->num_peers * (sizeof(PEER) +
          (ULONG)netp->max_nodes * sizeof(ROUTE)));
#else
      putprintf(mbp,   "\r      Network Heap: %lu Bytes\r",
          sizeof(NETWORK) + (ULONG)netp->num_peers * (sizeof(PEER) +
          (ULONG)netp->max_nodes * sizeof(ROUTE)));
#endif
#endif
  }

/********** Show Port Statistics ****************************************/
  if (flag & PORTSTATFLAG)
  {

/******************** number of byte rx/tx per port *********************/
    summe =
    offset = 0L;

#ifdef SPEECH
    putstr(speech_message(153), mbp);
#else
    putstr("\r\r Port-Statistics:\r\r"
           "              Links      RxB      TxB   RxBaud"
           "   TxBaud  RxOver TxOver\r", mbp);
#endif

    for (i = 0, pstatp = portstat, portp = portpar;
         i < L2PNUM;
         i++, pstatp++, portp++)
    {
      if (portenabled(i))
      {
        putprintf(mbp, "%2u:%-10s%3u/%-3u ", i, portp->name,
                                              portp->nmblks,
                                              portp->nmbstn);
        put_kMG(mbp, pstatp->rx_bytes);
        putchr(' ', mbp);
        put_kMG(mbp, pstatp->tx_bytes);
        putchr(' ', mbp);
        put_kMG(mbp, pstatp->rx_baud*8L);
        putchr(' ', mbp);
        put_kMG(mbp, pstatp->tx_baud*8L);
        if (pstatp->rx_bytes / 100L)
        {
          rxoverhead = pstatp->rx_overhead / (pstatp->rx_bytes / 100L);
          if (rxoverhead > 99)
            rxoverhead = 100;
          putprintf(mbp, "   %3lu%%", rxoverhead);
        }
        else
          if (pstatp->tx_bytes / 100L)
            putstr("       ", mbp);
        if (pstatp->tx_bytes / 100L)
        {
          txoverhead = pstatp->tx_overhead / (pstatp->tx_bytes / 100L);
          if (txoverhead > 99)
            txoverhead = 100;
          putprintf(mbp, "   %3lu%%", txoverhead);
        }
        putchr(CR, mbp);
        summe += (pstatp->rx_bytes + pstatp->tx_bytes);
        while (summe > 999999L)
        {
          offset++;
          summe -= 1000000L;
        }
      }
    }
#ifdef SPEECH
         putstr(speech_message(154), mbp);
#else
         putstr("\rTotal = ", mbp);
#endif
/* wird noch erweitert, gilt noch nicht fuer die einzelnen Ports, die   */
/* koennen weiterhin nur bis 4.2GB zaehlen                              */
    put_pktnum(mbp, offset, summe);
#ifdef SPEECH
    putstr(speech_message(155), mbp);
#else
    putstr(" Bytes\r", mbp);
#endif
}

/************* Fehler-Statistik *****************************************/
  if (flag & ERRORSTATFLAG)
  {
#ifdef SPEECH
  putstr(speech_message(156), mbp);
#else
  putstr("\r Error-Statistics:\r\r"
           "                RxID  RxLen  RxCtl Resets\r", mbp);
#endif

    for (i = 0, pstatp = portstat, portp = portpar;
         i < L2PNUM;
         i++, pstatp++, portp++)
    {
      if (portenabled(i))
      {
        putprintf(mbp, "%2u:%-10s ", i, portp->name);
        for (j = 0; j < 3; j++)
           putprintf(mbp, "%6u ", pstatp->invalid[j]);
        putprintf(mbp, "%6u\r", pstatp->reset_count);
      }
    }
  }

  if (flag & HARDWAREFLAG)
    l1stat("*", mbp);

#ifdef KERNELIF
/************* Show Kernel Link Statistics ********************************/
  if (flag & KERNELFLAG)
    ifip_dispstat(mbp);
#endif

/************* Show Link Statistics *************************************/
  if (flag & LINKSTATFLAG)
  {
#ifdef SPEECH
  putstr(speech_message(157), mbp);
#else
    putstr("\rLink-Statistics:\r", mbp);
#endif
    for (i = MAXSTAT, statp = mh; i--; statp++)
    {
      if (*statp->call)
      {
        mbp->l4time = mbp->mbpc;
#ifdef SPEECH
        putstr(speech_message(158), mbp);
#else
        putstr("\rLink to ", mbp);
#endif
        putid(statp->call, mbp);
        if (*statp->viacall)
        {
#ifdef SPEECH
          putstr(speech_message(159), mbp);
#else
          putstr(" via ", mbp);
#endif
          putid(statp->viacall, mbp);
        }
        putspa(35, mbp);
        puttim(&statp->hfirst, mbp);
        if (statp->hlast > 0)
        {
          putstr(" - ", mbp);
          puttim(&statp->hlast, mbp);
        }
#ifdef SPEECH
        putstr(speech_message(160), mbp);
#else
        putstr("\rFrames:      I         UI         RR       REJ      "
               "RNR  SABM/UA  DISC/DM FRMR\r", mbp);
#endif
        for (j = 0; j < 2; j++)
          putprintf(mbp, "%s:%11lu%11lu%11lu%10lu%9lu%9lu%9lu%5lu\r",
                    (j == 0) ? "RX" : "TX",
                    statp->Ino[j],
                    statp->UIno[j],
                    statp->RRno[j],
                    statp->REJno[j],
                    statp->RNRno[j],
                    statp->SABMno[j] + statp->UAno[j],
                    statp->DISCno[j] + statp->DMno[j],
                    statp->FRMRno[j]);
#ifdef SPEECH
        putstr(speech_message(161), mbp);
#else
        putstr("Bytes:   Total       Info     Header  Overhead      %I     "
               "%RR    %REJ    %RNR\r", mbp);
#endif
        for (j = 0; j < 2; j++)
        {
          trxframes[j] = statp->Ino[j] + statp->UIno[j] + statp->RRno[j]
                       + statp->REJno[j] + statp->RNRno[j] + statp->SABMno[j]
                       + statp->UAno[j] + statp->DISCno[j] + statp->DMno[j]
                       + statp->FRMRno[j];
          putprintf(mbp, "%s:%11lu%11lu%11lu",
                    (j == 0) ? "RX" : "TX",
                    statp->Bytetotal[j],
                    statp->Bytetotal[j] - statp->Byteheader[j],
                    statp->Byteheader[j]);
          percent(statp->Byteheader[j], statp->Bytetotal[j], 1, 9, mbp);
          percent(statp->Ino[j], trxframes[j], 1, 7, mbp);
          percent(statp->RRno[j], trxframes[j], 1, 7, mbp);
          percent(statp->REJno[j], trxframes[j], 1, 7, mbp);
          percent(statp->RNRno[j], trxframes[j], 1, 7, mbp);
          putchr(CR, mbp);
        }
        txoverhead = (statp->Bytetotal[1] - statp->Byteheader[1])
                     - statp->txByterepeated;
#ifdef SPEECH
        putprintf(mbp, speech_message(183),
                  txoverhead,
                  statp->txByterepeated);
#else
        putprintf(mbp, "TX:      Once:%11lu  Repeated:%10lu  IQual:",
                  txoverhead,
                  statp->txByterepeated);
#endif
        percent(txoverhead, statp->Bytetotal[1] - statp->Byteheader[1],
                1, 7, mbp);
#ifdef SPEECH
        putstr(speech_message(162),mbp);
#else
        putstr("  TQual:",mbp);
#endif
        percent(txoverhead,statp->Bytetotal[1], 1, 7, mbp);
        putchr(CR, mbp);
      }
    }
  }

#ifdef IPROUTE
/************* Show IP-Gateway Statistics *******************************/
  if (flag & IPSTATFLAG)
  {
#ifdef SPEECH
    putstr(speech_message(163), mbp);
#else
    putstr("\rIP-Gateway-Statistics:\r\r", mbp);
#endif
    for (i = 1; i <= NUMIPMIB; )
    {
      mbp->l4time = mbp->mbpc;
      for (j = 0; j < 3 && i <= NUMIPMIB; j++, i++)
      {
#ifdef __WIN32__
        putspa((char)(26 * j), mbp);
#else
        putspa(26 * j, mbp);
#endif /* WIN32 */
        putstr(Ip_mib[i].name, mbp);
        putchr(':', mbp);
#ifdef __WIN32__
        putspa((char)(26 * j + 19), mbp);
#else
        putspa(26 * j + 19, mbp);
#endif /* WIN32 */
        putnum(Ip_mib[i].value.integer, mbp);
      }
      putchr(CR, mbp);
    }
    putchr(CR, mbp);
  }
#endif

  putchr(CR, mbp);
  prompt(mbp);
  seteom(mbp);
}

/************************************************************************/
/* Test-Command                                                         */
/*----------------------------------------------------------------------*/
void ccptst(void)
{
  MBHEAD *mbp;
  WORD   port;

  if (issyso())                               /* Bin ich Sysop? */
  {
    if ((port = (char) nxtnum(&clicnt, &clipoi)) >= L2PNUM)
    {
#ifdef SPEECH
    putmsg(speech_message(193));
#else
    putmsg("\rInvalid Port\r");
#endif
      return;
    }

     if (!portenabled(port)) /* Port nicht eingeschaltet */
     {
#ifdef SPEECH
     putmsg(speech_message(345));
#else
     putmsg("\rCan't send, Port is disabled\r");
#endif
       return;
     }

     l1ctl(L1CTST, port);
     mbp = putals("Sending test on Port ");
     putnum(port, mbp);
     putchr('\r', mbp);
     prompt(mbp);
     seteom(mbp);
  }
  else
    invmsg();
}

/************************************************************************/
/* Version-Command                                                      */
/*----------------------------------------------------------------------*/
void ccpver(void)
{
  MBHEAD *mbp;                          /* message-buffer-pointer       */

  mbp = putals(version);                /* Versionskennung zeigen       */
#ifdef SPEECH
    putprintf(mbp,speech_message(184), L2PNUM, LINKNMBR, NUMCIR,author);
#else
    putprintf(mbp,"     Copyright by NORD><LINK, free for non-commercial"
                " usage.\r         See www.nordlink.org for further"
                " information.\r  This version compiled for %d Ports, %d"
                " L2-Links and %d Circuits.\r"
                                "       %s\r", L2PNUM, LINKNMBR, NUMCIR,author);
#endif

  if (clicnt != 0 && (*clipoi == '*' || *clipoi == '+')) {
    putstr("\rCompiled options:\r"
#ifdef IPROUTE
    "   * IP-Router\r"
#endif
#ifdef PACSAT
    "   * PACSAT Server\r"
#endif
#ifdef PADDLE
    "   * Paddle\r"
#endif
#ifdef FLEXHOST
    "   * Local-Hosting\r"
#endif
#ifdef L2PROFILER
    "   * L2-Profiling\r"
#endif
#ifdef PROFILING
    "   * Profiling\r"
#endif
#ifdef GRAPH
    "   * Graph\r"
#endif
#ifdef USER_PASSWORD
    "   * User-Password\r"
#endif
#ifdef USERMAXCON
    "   * User-MaxConnection\r"
#endif
#ifdef DAMASLAVE
    "   * DAMA-Slave\r"
#endif
#ifdef KERNELIF
    "   * Kernel-Interface\r"
#endif
#ifdef ALIASCMD
    "   * Command-Aliasing\r"
#endif
#ifdef EAX25
    "   * Extended-AX.25\r"
#endif
#ifdef SPEECH
    "   * Speech Support\r"
#endif
#ifdef USERPROFIL
    "   * User-Profil\r"
#endif /* USERPROFIL */
    "\r", mbp);
    putstr("Hardware:\r", mbp);
    l1enum(mbp);
    putchr('\r', mbp);
  }

  prompt(mbp);
  seteom(mbp);
}

/************************************************************************/
/* Links-Command                                                        */
/*----------------------------------------------------------------------*/
void ccplnk(void)
{
  MBHEAD *mbp;
  char    call[L2IDLEN];
  char    ident[L2CALEN];
  char    digil[L2VLEN+1];
  UWORD   port;
  char    mode;
  char    typ[] = {"  "};
#ifdef PROXYFUNC
  BOOLEAN typproxy = FALSE;
#endif
  char    typs = 0;
  char   *via;
  PEER   *pp;
  int     i;
  int     max_peers = netp->max_peers;
  const char *cp;
#ifdef LINKSMODINFO
  char    info[INFOSIZE + 1];
#endif /* LINKSMODINFO */

  mbp = getmbp();

  /* nur Neueintraege als Sysop und wenn noch was in der Kommandozeile da ist */
  if (issyso() && clicnt > 0)
  {
    mode = *clipoi;             /* Modus bestimmen (+ oder -) */
    clipoi++;
    clicnt--;
    skipsp(&clicnt, &clipoi);
    typ[0] = toupper(*clipoi);  /* Linktyp holen (N I F usw.) */
    clipoi++;
    clicnt--;
    if (*clipoi != ' ')         /* Linkzusatz holen (+ oder -) wenn vorhanden */
    {
      typ[1] = *clipoi++;
      clicnt--;
    }

#ifdef PROXYFUNC
    skipsp(&clicnt, &clipoi);
    if ( (typproxy = ((*clipoi == 'P' || *clipoi == 'p') && *(clipoi+1) == ' ' ) ? TRUE : FALSE) == TRUE) {
      clipoi++;
      clicnt--;
    }
#endif
    if ((port = (UWORD) nxtnum(&clicnt, &clipoi)) < L2PNUM)
    {
      skipsp(&clicnt, &clipoi);

      if (strchr((char *)clipoi, ':'))
        *strchr((char *)clipoi, ':') = ' ';

      if (getide(&clicnt, &clipoi, ident) == YES)
      {
        skipsp(&clicnt, &clipoi);
        if ((getcal(&clicnt, &clipoi, TRUE, call)) == YES)
        {
          getdig(&clicnt, &clipoi, TRUE, digil);
          digil[2*L2IDLEN] = NUL;

#ifdef LINKSMODINFO
          /* Frischer Buffer. */
          memset(info, 0, sizeof(info));

          /* Pruefe, ob es weiter Zeichen gibt. */
          if (skipsp(&clicnt, &clipoi))
          {
            char *infotext;

            if ((infotext = strchr(clipoi, '=')) != NULL)
            {
              /* Zeichen "=" loeschen. */
              infotext++;

              /* Stations-Beschreibung speichern. */
              strncpy(info, infotext, INFOSIZE);
            } /* kein "=" enthalen. */
          } /* keine weiteren Zeichen im Buffer. */
#endif /* LINKSMODINFO */

          for (cp = typtbl; *cp; cp += 2)
            if ((cp[0] == typ[0]) && (cp[1] == typ[1])) /* Typ stimmt */
              typs = ((int)(cp-typtbl))/2;

#ifdef LINKSMODROUTINGTYP
          if (  (typs == NETROM)                                /* NETROM-TYP */
              &&(typs == (mode == '+')))      /* und soll eingetragen werden, */
          {                                              /* nicht zulassen!!! */
            putstr("None Routing-Typ indicated.\r", mbp);
            prompt(mbp);                               /* Zum Prompt zurueck. */
            seteom(mbp);
            return;
          }
#endif /* LINKSMODROUTINGTYP */
          if (mode == '+')
          {
#ifdef PROXYFUNC
            typs = typproxy ? typs+PROXYMASK : typs;
#endif
            pp = register_neigb(call, digil, ident, port, typs
#ifdef LINKSMODINFO
                               ,info
#endif /* LINKSMODINFO */
#ifdef AUTOROUTING
                               ,FIXED_ROUTE /* Link ist immer Fest. */
#endif /* AUTOROUTING */
);
            if (!portenabled(port))
#ifdef SPEECH
              putstr(speech_message(166), mbp);
#else
              putstr("Warning - Port not active.\r", mbp);
#endif
#ifdef LINKSMOD_MSG
            if (pp != NULL)           /* Linkpartner erfolgreich eingetragen. */
            {
              putstr("Link is registered.\r", mbp);
              prompt(mbp);                             /* Zum Prompt zurueck. */
              seteom(mbp);
              return;
            }
#endif /* LINKSMOD_MSG. */
          }

          if (mode == '-')
#ifdef LINKSMOD_MSG
          {
            if (unregister_neigb(call, digil, port))       /* Link austragen. */
            {
#ifdef AXIPR_HTML
              /* Protokoll fuer HTML-Ausgabe setzen. */
              SetHTML(port, call, NULL, FALSE);
#endif /* AXIPR_HTML */

              putstr("Link is deleted.\r", mbp);
              prompt(mbp);                           /* Und zum Prompt zurueck. */
              seteom(mbp);
              return;
            }
          }
#else
            unregister_neigb(call, digil, port);
#endif /* LINKSMOD_MSG. */

        } /* if (getcal(&clicnt ... */
        else /* Call war nicht ok */
        {
#ifdef SPEECH
          putstr(speech_message(167), mbp);
#else
          putstr("Invalid callsign.\r", mbp);
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }
      }  /* if (ident_ok... */
      else /* Ident ist nicht ok (enthaelt ungueltige Zeichen etc.) */
      {
#ifdef SPEECH
        putstr(speech_message(168), mbp);
#else
        putstr("Invalid ident.\r", mbp);
#endif
        prompt(mbp);
        seteom(mbp);
        return;
      }
    }  /* if (port = nxtnum(&clicnt, &clipoi) < L2PNUM) */
      /* putstr("Syntax: L +/- TYP PORT ALIAS:CALL[-*] [Digi1 [Digi2]]\r", mbp);*/
  }  /* if (sysop...  */

#ifdef LINKSMODSYNTAXFIX
  /* Nur Sysop's duerfen die Syntax sehen. */
  if (issyso())
    putstr("Syntax: L +/- TYP PORT ALIAS:CALL[-*] [Digi1 [Digi2]] [INFO=<text>]\r", mbp);
#endif /* LINKSMODSYNTAXFIX */

  putstr("Links of ", mbp);     /* Konfiguration zeigen */
  putalt(alias, mbp);
  putid(myid, mbp);
  putprintf(mbp, " (%d/%d)\r", netp->num_peers, netp->max_peers);

#ifndef LINKSMODINFO
  putstr("Type-Port--Alias:Call------Route--------------\r", mbp);
#else
  putstr("Type-Port--Alias:Call------Route--------------Info------------------------\r", mbp);
#endif /* LINKSMODINFO */

  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
    if (pp->used) {
      mbp->l4time = mbp->mbpc;     /* Zaehler merken fuer putspa()      */

#ifdef LINKSMOD_LOCALMOD
   if (  (pp->typ == LOCAL_V)                  /* Local-Link versteckt. */
       &&(!issyso()))                                /* und kein Sysop. */
     continue;                                /* Zum naechsten Eintrag. */
#endif /* LINKSMOD_LOCALMOD */

#ifdef PROXYFUNC
      putprintf (mbp, "%2.2s %c", typtbl + pp->typ*2 , pp->proxy ? 'P' : ' ');
#else
      putspa(2, mbp);
      putprintf (mbp, "%2.2s", typtbl + pp->typ*2);
#endif
      putspa(6, mbp);
      if (pp->l2link->port < 10) putchr(' ', mbp);
      putnum(pp->l2link->port, mbp);
      putstr("  ", mbp);
      putide(pp->l2link->alias, mbp);
      putchr(':', mbp);
      putid(pp->l2link->call, mbp);
      via = pp->l2link->digil;   /* nicht putdil() wegen "via" */
      if (via[0])
      {
        putspa(27, mbp);
        putid(via, mbp);
        if (via[L2IDLEN])
        {
          putchr(' ', mbp);
          putid(via+L2IDLEN, mbp);
        }
      }
#ifdef LINKSMODINFO
      /* Ab spalte 46. */
      putspa(46, mbp);
      /* Stations-Beschreibung ausgeben. */
      putprintf(mbp, "%s",pp->l2link->info, mbp);
#endif /* LINKSMODINFO */
      putchr('\r', mbp);
    }

  prompt(mbp);
  seteom(mbp);
}

/************************************************************************/
/* Quit-Command                                                         */
/*----------------------------------------------------------------------*/
void ccpquit(void)
{
  MBHEAD *mbp;
  char   name[MAXPATH];

  mbp = getmbp();

  /* Gibt es eine QUIT.TXT ? */
  if (xaccess("QUIT.TXT", 0) == 0)
  {
    /* Markiere Path/Datei. */
    sprintf(name, "%sQUIT.TXT", textpath);
    /* Gibt es Makros, diese ausfuehren. */
    out_ctext(name, mbp);
  }
  else
  {
     putstr("\r73 de ", mbp);
     putid(myid, mbp);
   }

   putchr('\r', mbp);
   seteom(mbp);
   disusr(userpo->uid);
}


/************************************************************************/
/*                                                                      */
/* ECHO Befehl                                                          */
/*                                                                      */
/* Funktion : Rest der Eingabezeile des Users zurueck schicken.         */
/*            Anwendung z.B. fuer RTT Messung.                          */
/*                                                                      */
/* Syntax   : //E <text>                                                */
/*                                                                      */
/*----------------------------------------------------------------------*/
void ccpecho(void) {
  MBHEAD *mbp;

  mbp = getmbp();
  if (clicnt > 0 && !strnicmp(clilin, "//E", 3)) {
     putchr('\r', mbp);
     while (clicnt > 0) {
    putchr(*clipoi++, mbp);
    clicnt--;
     }
     putchr('\r', mbp);
  }
  prompt(mbp);
  seteom(mbp);
}

/************************************************************************/
/*                                                                      */
/* TALK Befehl                                                          */
/*                                                                      */
/* Funktion : Nachricht an einen anderen User schicken, der im CCP und  */
/*            nicht anderweitig connected ist!                          */
/* Syntax   : TALK <user> <text>                                        */
/* Ausgabe  : Msg from <call>: <text>                                   */
/*----------------------------------------------------------------------*/
void ccptalk(void)
{
  char   tmp[8];
  WORD   suc;
  MBHEAD *mbp;
  char call[L2IDLEN];

  if (skipsp(&clicnt, &clipoi)) {                      /* steht da was? */
    if (!strnicmp((char *)clipoi, "ALL", 3)) {         /* an alle?      */
      nextspace(&clicnt, &clipoi);
      if (skipsp(&clicnt, &clipoi) == FALSE) {         /* kein Text?    */
#ifdef SPEECH
          putmsg(speech_message(195));
#else
          putmsg("No Text\r");
#endif
        return;
      }
      call[0] = '*';
      suc = talk_to(call, 0);                          /* weiterpetzen  */
     } else
     if (getcal(&clicnt, &clipoi, TRUE, call) == YES)  { /* ein call ?  */
       if (skipsp(&clicnt, &clipoi)) {                 /* kommtn Text?  */
         suc = talk_to(call, 0);                       /* weiterpetzen  */
       } else {
         suc = talk_to(call, 1);                       /* gibs den?     */
         mbp = getmbp();
         if (suc) {
           cpyid(userpo->talkcall, call);              /* call merken   */
           userpo->status = US_TALK;
           callss2str(tmp, call);
#ifdef SPEECH
           putprintf(mbp, speech_message(185), tmp);
#else
           putprintf(mbp, "You are now talking with %s. Leave this mode with /q\r", tmp);
#endif
         } else {
#ifdef SPEECH
           putstr(speech_message(172), mbp);
#else
           putstr("No such User!\r", mbp);
#endif
           prompt(mbp);                                /* Prompt dran   */
         }
         seteom(mbp);                                  /* fertig        */
         return;
       }
     } else {
#ifdef SPEECH
         putmsg(speech_message(196));
#else
         putmsg("Invalid Call\r");
#endif
       return;
     }
#ifdef SPEECH
     putmsg(suc ? speech_message(197) : speech_message(198));
#else
     putmsg(suc ? "Msg sent\r" : "No User\r");
#endif
  } else
#ifdef SPEECH
      putmsg(speech_message(199));
#else
      putmsg("No Arguments\r");
#endif
}

static BOOLEAN talk_to(char *call, WORD test)
{
  char   tmp[8];
  WORD   cnt = 0;
  USRBLK *save_userpo = userpo;
  MBHEAD *mbp;

  callss2str(tmp, calofs(UPLINK, userpo->uid));
  strlwr(tmp);

  for (userpo  = (USRBLK *) usccpl.head;
       userpo != (USRBLK *) &usccpl;
       userpo  = (USRBLK *) userpo->unext)
  {
     if (       userpo->convers == NULLCONNECTION
         && (   userpo->status == US_CCP
             || userpo->status == US_TALK)
         &&     userpo != save_userpo
         && (   cmpcal(calofs(UPLINK, userpo->uid), call)
             || call[0] == '*')) {
       if (test) {
         userpo = save_userpo;
         return(TRUE);
       }
       mbp = getmbp();

       /* String zu lang? */
       if (clicnt > 214)
       {
         /* Aktuellen user setzen. */
         userpo = save_userpo;
         /* Der String ist zu lang, abbruch. */
         return(TRUE);
       }

#ifdef SPEECH
       putprintf(mbp, speech_message(186), tmp, clipoi);
#else
       putprintf(mbp, "Msg from %s (use TALK to reply): %s\r", tmp, clipoi);
#endif
       seteom(mbp);
       if (call[0] != '*') {
         userpo = save_userpo;
         return(TRUE);
       }
       cnt++;
     }
  }
  userpo = save_userpo;
  return(cnt ? TRUE : FALSE);
}

/************************************************************************/
/* Mailbox-Befehl                                                       */
/************************************************************************/
void ccpmail(void)
{
  if (issyso() && skipsp(&clicnt, &clipoi)) {
    ccp_call(boxid);               /* Rufzeichen setzen                 */
  } else {                         /* Connect zur Mailbox               */
    if (boxid[0]) {
      call2str((char *)(clipoi = clilin), boxid);
      clicnt = strlen((char *)clipoi);
      ccpcon(NULL);
    } else
#ifdef SPEECH
      putmsg(speech_message(200));
#else
      putmsg("No mailbox!\r");
#endif
  }
}

/************************************************************************/
/* DXCluster-Command                                                    */
/*----------------------------------------------------------------------*/
void ccpdxc(void)
{
  if (issyso() && skipsp(&clicnt, &clipoi)) {
    ccp_call(dxcid);                /* Rufzeichen setzen                 */
  } else {                          /* Connect zur Mailbox               */
    if (dxcid[0]) {
      call2str((char *)(clipoi = clilin), dxcid);
      clicnt = strlen((char *)clipoi);
      ccpcon(NULL);
    } else
#ifdef SPEECH
      putmsg(speech_message(201));
#else
      putmsg("No DX-Cluster!\r");
#endif
  }
}

#ifdef HOSTMYCALL
/************************************************************************/
/*                                                                      */
/* Consolen-Mycall setzen.                                              */
/*                                                                      */
/************************************************************************/
void ccpmyhost(void)
{
  MBHEAD *mbp;

  if (issyso())                                    /* Nur Sysop darf aendern. */
  {
    if (skipsp(&clicnt, &clipoi))
    {
      ccp_call(hostuserid);                        /* Neues rufzeichen setzen */
      return;
    }

    mbp = putals("HostMyCall:\r\r");

    putstr("Consolen MyCall:", mbp);
    putid(hostuserid, mbp);
    putstr("\r", mbp);

    prompt(mbp);
    seteom(mbp);
  }
  else
    invmsg();
}
#endif /* HOSTMYCALL */

#ifdef SYSOPPASSWD
/*************************************************************************/
/*                                                                       */
/* Sysop-Passwort im laufenden Betrieb aendern.                          */
/* Das neue Passwort wird in der TNN179.TNB gespeichert und beim         */
/* naechsten neustart eingelesen.                                        */
/*                                                                       */
/* SYNTAX: PASS dasistmeinpasswort                                       */
/* (Passwortstring muss genau 80Zeichen haben)                           */
/*                                                                       */
/*************************************************************************/
void ccppasswd(void)
{
  MBHEAD *mbp;

  if (issyso())                                    /* Nur Sysop darf aendern. */
  {
    if (skipsp(&clicnt, &clipoi))
    {
      if (clicnt == 80)                           /* Passwort hat 80 Zeichen. */
      {
        strncpy(paswrd, clipoi, 80);               /* Neues Passwort sichern. */
        mbp = putals("Sysop Passwort:\r\r");
#ifdef SPEECH
        putstr(speech_message(173),mbp);
#else
        putstr("Das Sysop Passwort wurde geaendert!\r",mbp);
#endif
        prompt(mbp);
        seteom(mbp);
        return;
      }
      else /* Passwort hat keine 80 Zeichen. */
        {
          mbp = putals("Sysop Passwort:\r\r");
#ifdef SPEECH
          putstr(speech_message(174),mbp);
#else
          putstr("Fehler: Das Passwort muss 80 Zeichen enthalten!\r",mbp);
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }
    }

    mbp = putals("Sysop Passwort:\r\r");
    putprintf(mbp,"%s\r",paswrd);
    prompt(mbp);
    seteom(mbp);
  }
  else
    invmsg();
}
#endif /* SYSPASSWD */

/************************************************************************/
/* Paddle-Command                                                       */
/*----------------------------------------------------------------------*/
#ifdef PADDLE           /* spezielle Hardware fuer AtariST */
char *padread(WORD i);
extern WORD paddles[8];
void ccppaddle(void)
{
  int i;
  MBHEAD *mbp;

  mbp = putals("Analog inputs:\r");

  for (i = 0; i < 6; i++)
    putstr(padread(i), mbp);

  putchr(CR, mbp);
  prompt(mbp);
  seteom(mbp);
}
#endif


/************************************************************************/
/*                                                                      */
/*----------------------------------------------------------------------*/

void send_ctext(void)
{
  BOOLEAN   ok = FALSE;
  UWORD     port = 0;
  MBHEAD   *mbp;
  char      file[14];
  char      name[MAXPATH];
  UBYTE     typ = g_utyp(userpo->uid);
  CIRBLK   *cp;
  LNKBLK   *lp;
  int       i;
  PERMLINK *p;

  /*
   * Wenn eine Verbindung als Digipeating reinkommt, setzen wir den
   * Status entsprechend und senden kein CTEXT.
   */

  if (typ == L2_USER) {
    lp = g_ulink(userpo->uid);
    if (lp->state == L2SHTH) {
      userpo->status = US_DIG;
      return;
    }
  } else
  if (typ == L4_USER) {

    cp = g_ulink(userpo->uid);
    if (cmpid(cp->destca, myid) == FALSE) {
      userpo->status = US_DIG;
      return;
    }
  }

#ifdef L1TCPIP
  {
    TCPIP *tc;

    if (typ == TCP_USER)
    {
      tc = g_ulink(userpo->uid);

      switch(tc->Interface)
      {
#ifdef L1HTTPD
        case KISS_HTTPD :
          return;
#endif /* L1HTTPD */

#ifdef L1IPCONV
        case KISS_IPCONV :
          return;
#endif /* L1IPCONV */

#ifdef L1IRC
        case KISS_IRC :
          return;
#endif /* L1IRC */

        default :
         break;
      }
    }
  }
#endif /* L1TCPIP */

  /*
   * Der CTEXT wird abhaengig vom Port gesendet.
   */
  port = userport(userpo);

#ifndef NOCTEXT
  /* Jedes Rufzeichen, das wir in der Nodes-Liste finden, begruessen
   * wir ueberhaupt nicht. */
  if (find_node_ssid_range(calofs(UPLINK, userpo->uid)) != -1)
    return;
#else
  {
    PEER *pp;
    int   i;
    int   max_peers = netp->max_peers;

    for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
    {
      if (pp->used)
      {
        if (cmpid(pp->l2link->call, calofs(UPLINK, userpo->uid)))
        {
          if (pp->typ <= FLEXNET)         /* Routing-Protokolle kein CTEXT. */
            return;
        }
      }
    }
  }
#endif /* NOCTEXT */

/* Keinen Ctext fuer eingetragene Convers-Hosts                         */
  for (i = 0; i < MAXCVSHOST; i++)
  {
    if ((p = permarray[i]) != NULLPERMLINK)
      if (cmpcal(calofs(UPLINK, userpo->uid), p->cname))
      return;
  }

  if (port == L2PNUM)                   /* HOST                         */
    ok = TRUE;
  else
    if (ctextenabled(port))
      ok = TRUE;

  mbp = getmbp();

  if (ok) {
#ifndef MAKRO_NOLOGINSTR
    putstr(loginstr, mbp);
#endif
    sprintf(name, "%sCTEXT.TXT", textpath);
    out_ctext(name, mbp);

    /* sende nun  CTEXT.Port  */
    sprintf(name, "%sCTEXT.%u", textpath, port);
    out_ctext(name, mbp);

    /*  Digimail Text(e), falls File "CALL.MSG" vorhanden */
    callss2str(file, calofs(UPLINK, userpo->uid));
    sprintf(name, "%s%s.MSG", msgpath, file);
    if (xaccess(name, 0) == 0) {
      putstr("\r", mbp);
      seteom(mbp);
      ccpread(name);
      return;                       /* nicht nochmal Prompt             */
    }
  }  /* kein CTEXT gewuenscht .. dann nur Prompt  */

  putchr('\r', mbp);
  prompt(mbp);
  seteom(mbp);
}

#ifdef MAKRO_FILE
/* USERPO->FILE nach Makros untersuchen */
/* und neue USER->FILE erstellen.       */
static int update_userpo_file(USRBLK *user)
{
  MBHEAD *usermbp;
  FILE   *fp;
  char   *tmpfile;
  char    zeichen;
  char    inbuf[255],
         *inread = inbuf;

  if (user->read_ok == FALSE)
          {
          /* Buffer besorgen. */
          usermbp = getmbp();

          /* Wir lesen userpo->fp komplett ein. */
          while(fgets(inread,255,user->fp) != NULL)
                  /* Jede Zeile nach Makros pruefen */
                  /* und gegebenfalls auswerten.    */
                  prompt2str(usermbp,inread);

          /* Buffer zurueck spulen. */
          rwndmb(usermbp);

          /* Markiere, das wir die funktion */
          /* durchlaufen haben.             */
          user->read_ok = TRUE;

      /* Ermitteln eines Namens fuer eine temporaere Datei. */
          if ((tmpfile = tempnam(textpath, "tmp")) == NULL)
                  /* da ging was schief. */
                  return(TRUE);

          /* Temp-File oeffenen. */
          if ((fp = xfopen(tmpfile, "wt")) == NULL)
                  {
                  putmsg("Sri, can't open tempfile...\r");
                  free(tmpfile);
                  return(TRUE);
                  }

          /* Keine Fehler, jetzt koennen wir */
          /* die alte Temp-File Schliessen!. */
          fclose(user->fp);
          user->fp = NULL;

          if (user->fname != NULL)
                  {
                  xremove(user->fname);
                  free(user->fname);
                  user->fname = NULL;
                  }

          while(usermbp->mbpc != usermbp->mbgc)
                  /* Alle Zeichen in das Temp-File schreiben.*/
                  fprintf(fp,"%c",(zeichen = getchr(usermbp)));

          /* Buffer leeren/entsorgen. */
          dealmb(usermbp);
          /* Temporaeres File schliessen */
          fclose(fp);
          /* Temporaeres File setzen. */
          user->fname = tmpfile;
          /* NEUE (mit eventuelle Makros) */
          /* Temporaeres File einlesen.   */
          ccpread(tmpfile);
          return(FALSE);
  }
  return(FALSE);
}
#endif
/* End of src/l7ccp.c */
