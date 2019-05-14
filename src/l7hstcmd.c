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
/* File src/l7hstcmd.c (maintained by: DF6LN)                           */
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

extern int      console_login_status;
extern int      Ypar2;
extern MONBUF   consmon;
extern MBHEAD  *hstmbp;
extern FILE    *pro_file;
extern char     hmdest[L2IDLEN];
extern char     hmdigil[L2VLEN+1];
extern WORD     hmport;

static void     xEcmd(void);
static void     xGcmd(void);
static void     listch(WORD);
static void     xYcmd(void);
static void     rspic(unsigned);
static void     rspiec(unsigned);
static void     rspiv(unsigned);
static void     rspcs(void);
static void     rsppar(ULONG);
static void     putcc(char, MBHEAD *);

/************************************************************************/
/*                                                                      */
/*                                                                      */
/*                                                                      */
/************************************************************************/
void
Bcmd(void)
{
  rsppar((ULONG)rounds_pro_sec);
}

/************************************************************************/
/*                                                                      */
/* Connect von der Konsole zum Knoten bzw. Konfiguration des UI-Pfads   */
/*                                                                      */
/************************************************************************/
void
Ccmd(void)
{
  if (actch == 0 || tnb_ch)             /* Konfiguration UI-Pfad        */
  {
    if (!blicnt)                        /* derzeitigen UI-Pfad anzeigen */
    {
      rspini(HMRSMSG);
      if (hmdest[0] == ' ')
#ifdef SPEECH
        putstr(speech_message(252), hstmbp);
#else
        putstr("not available", hstmbp);
#endif
      else
      {
        putnum(hmport, hstmbp);
        putchr(' ', hstmbp);
        putid(hmdest, hstmbp);
        putdil(hmdigil, hstmbp);
      }
    }
    else                                /* UI-Pfad setzen               */
    {
      if (!getport(&blicnt, &blipoi, &hmport))
      {
        rsperr(HMEPOR);
        return;
      }
      if (getcal(&blicnt, &blipoi, FALSE, hmdest) != YES)
      {
        rsperr(HMEICS);
        return;
      }
      if (getdig(&blicnt, &blipoi, FALSE, hmdigil) == ERRORS)
      {
        rsperr(HMEICS);
        return;
      }
      rspsuc();
    }
  }
  else                          /* nicht Konfiguration UI-Pfad          */
  {
    if (ishmod && !blicnt)      /* Hostmode, ohne Parameter ->          */
    {                           /* aktuelles Call auf dem Host-Kanal    */
      rspini(HMRSMSG);
      if (hstusr->conflg)
        putid(hstusr->direction ? myid : hstusr->call, hstmbp);
      else
#ifdef SPEECH
        putstr(speech_message(253), hstmbp);
#else
        putstr("CHANNEL NOT CONNECTED", hstmbp);
#endif
    }
    else
    {                                   /* mit Parameter                */
      if (hstusr->conflg == 0)          /* schon connected?             */
      {
        hstcon(1);
        rspsuc();
      }
      else
        rsperr(HMEALC);                   /* bereits connected            */
    }
  }
}

/************************************************************************/
/*                                                                      */
/* Konsole disconnecten                                                 */
/*                                                                      */
/************************************************************************/
void
Dcmd(void)
{
  if (hstusr->conflg != 0)
  {
    hstsen(TRUE);
    hstdis();
    rspsuc();
  }
  else
    rspcs();
}

/************************************************************************/
/*                                                                      */
/* Echo-Befehl nur fuer Kompatibilitaet                                 */
/*                                                                      */
/************************************************************************/
void
Ecmd(void)
{
  rspsuc();
}

/************************************************************************/
/*                                                                      */
/* @E-Befehl - Protokoll-File fuer TNBs ein- und ausschalten            */
/*                                                                      */
/************************************************************************/
static void
xEcmd(void)
{
  char            out_file_name[MAXPATH+1];
  struct tm      *sys_t;
  UWORD           arg;

  if (!blicnt)
    rsppar(pro_file == NULL ? 0 : 1);
  else
    switch ((arg = nxtnum(&blicnt, &blipoi)))
    {
      case 0:
        if (pro_file != NULL)
        {
          fclose(pro_file);
          pro_file = NULL;
          rspsuc();
        }
        else
          rsperr(HMEFNO);
        break;
      case 1:
        if (pro_file == NULL)
        {
          skipsp(&blicnt, &blipoi);
          if (blicnt)
          {
            strncpy(out_file_name, (char*)blipoi, MAXPATH);
            out_file_name[MAXPATH] = NUL;
          }
          else
          {

/* Aus Datum und Uhrzeit den Filenamen generieren                       */

            sys_t = sys_localtime;
            sprintf(out_file_name, "%02d%02d%02d%02d.PRO",
                    sys_t->tm_year % 100, sys_t->tm_mon + 1,
                    sys_t->tm_mday, sys_t->tm_hour);
          }
          pro_file = xfopen(out_file_name, "at+");
          rspsuc();
        }
        else
          rsperr(HMEFAO);
        break;
      default:
        rspiv(arg);
    }
}

/************************************************************************/
/*                                                                      */
/* action     : G-Kommando (nur fuer Hostmode).                         */
/*                                                                      */
/*              Infoframedaten, Linkstatus-Meldungen, Monitorheader     */
/*              und Monitorframeinfodaten im Hostmode abholen.          */
/*                                                                      */
/*              Kanal 0   :  G  - Linkstatus/Monitorheader/Monitorinfo  */
/*                           G0 - Monitorheader/Monitorinfos            */
/*                           G1 - Linkstatus                            */
/*                                                                      */
/*              Kanal > 0 :  G  - Linkstati/Infoframedaten              */
/*                           G0 - Infoframedaten                        */
/*                           G1 - Linkstati                             */
/*                                                                      */
/*              Im Kanal 0 ist die einzig moegliche Linkstatusmeldung   */
/*              ein nicht angenommener Connect-Request, in den anderen  */
/*              Kanaelen werden keine Connect-Request-Meldungen         */
/*              ausgegeben.                                             */
/*                                                                      */
/*              Es wird eine der Anforderung entsprechende Hostmode-    */
/*              antwort ausgegeben.                                     */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* parameter  : -                                                       */
/*                                                                      */
/* r/o globals: ishmod -  TRUE = wir sind im Hostmode, Terminalmode     */
/*                        sonst                                         */
/*              incnt  -  Anzahl Zeichen hinter 'G' im Eingabebuffer    */
/*              inbufp -  Zeiger hinter 'G' in Eingabebuffer            */
/*              actch  -  Kanal des Hostmodekommandos                   */
/*                                                                      */
/* r/w globals: mifmbp -  Zeiger auf Framekopf eines I/UI-Frames, von   */
/*                        dem der Rumpf (die Daten) noch gemonitort     */
/*                        werden muessen                                */
/*              statml -  Liste der auszugebenden Statusmeldungen fuer  */
/*                        Kanal 0 (Hostmode nur Connect-Requests)       */
/*              smonfl -  Liste der zu monitorenden Frames (Vorauswahl) */
/*                                                                      */
/* locals     : s.u.                                                    */
/*                                                                      */
/* returns    : -                                                       */
/*                                                                      */
/************************************************************************/

void
Gcmd(void)
{
  static MBHEAD  *mbp;          /* Zeiger auf Kopf des aktuellen Frames */
  static unsigned n;
  unsigned        par;
  FILE           *fd;
  char            tmpcall[10];

  if (ishmod)                                   /* im Hostmode ?        */
  {
    if (actch == 0xFF)                          /* extended G           */
    {
      xGcmd();
      return;
    }
    if (blicnt)                                 /* ja, Parameter da ?   */
    {
      if ((par = *blipoi++) > 1)                /* ja, holen            */
      {
        rspiv(par);                             /* ... falsch angegeben */
        return;
      }
      blicnt++;
    }
    else                                        /* kein Parameter, alle */
      par = MBALL;                              /* Typen sind gemeint   */

/* G : par = MBALL   G0 : par = MBINFO   G1 : par = MBSTATUS            */

    if (!actch)                                 /* Monitorkanal ?       */
    {
      if (mifmbp != NULL                        /* ja, noch alter Rest  */
          && par != MBSTATUS)                   /* kein Status ?        */
      {
        rspini(HMRMONI);

        n = mifmbp->mbpc - mifmbp->mbgc;       /* Datenlaenge ermitteln */
        if (n == 0)
        {
          putchr(15, hstmbp);
#ifdef SPEECH
          putstr(speech_message(254), hstmbp);
#else
          putstr("[FRAME EMPTY?!?]", hstmbp);
#endif
        }
        else
        {
          if (n <= 257)                /* Max. 256 Datenbytes zulaessig */
          {
#ifdef __WIN32__
            putchr((char)(n - 1), hstmbp);     /* Zahl der folgenden Datenbytes */
#else
            putchr(n - 1, hstmbp);     /* Zahl der folgenden Datenbytes */
#endif /* WIN32 */
            while (mifmbp->mbgc < mifmbp->mbpc)
              putchr(getchr(mifmbp), hstmbp);
          }
          else                          /* stattdessen Fehlermeldung    */
          {
            putchr(15, hstmbp);
#ifdef SPEECH
            putstr(speech_message(255), hstmbp);
#else
            putstr("[FRAME TOO LONG]", hstmbp);
#endif
          }
        }
        dealmb(mifmbp);
        mifmbp = NULL;                  /* (kein Rest mehr)             */
      }
      else                              /* ... oder                     */
      {
        if ((par != MBINFO)             /* Status/Alles angefordert und */
            && (stalin))                /* Status da?                   */
        {
          mbp = (MBHEAD *)ulink((LEHEAD *)statml.head);    /* ja, holen */
          stalin--;                                  /* eine weniger    */
          rwndmb(mbp);                               /* und ausgeben    */
          rspini(HMRSTAT);                           /* (Antworttyp)    */
          while (mbp->mbpc > mbp->mbgc)
            putchr(getchr(mbp), hstmbp);
          if (stamp)
          {
            putstr(" - ", mbp);
            puttim(&mbp->btime, mbp);
          }
          dealmb(mbp);                               /* (freigeben)     */
        }
        else
        {
          if ((par != MBSTATUS)         /* ... oder Info/Alles          */
              && (monlin))              /* angefordert und Frames da?   */
          {
            mbp = (MBHEAD *)ulink((LEHEAD *)smonfl.head);
            monlin--;                                /* eine weniger    */
            rspini(mbp->type);                       /* Typ ausgeben    */
            while (mbp->mbpc > mbp->mbgc)
              putchr(getchr(mbp), hstmbp);
            if ((mbp->type == HMRMONIH)              /* mit Infofeld?   */
                && monlin)                           /* und Frames da?  */
            {
              if (((MBHEAD *)smonfl.head)->type == HMRMONI)
              {
                mifmbp = (MBHEAD *)ulink((LEHEAD *)smonfl.head);
                monlin--;                            /* noch 1 weniger  */
              }
            }
            dealmb(mbp);                             /* Buffer weg      */
          }
          else
            rspsuc();                                /* nix da          */
        }
      }
    }
    else  /* if (!actch)     */
    {
      if (hstusr->outsta                             /* Status da und   */
          && (par != MBINFO))                        /* gewuenscht      */
      {
        for (mbp = (MBHEAD *)hstusr->outbuf.head;
             mbp != (MBHEAD *)&hstusr->outbuf;
             mbp = (MBHEAD *)mbp->nextmh)
        {
          if (mbp->type > L2MNIX)                    /* Status-Frame!   */
          {
            hstusr->outsta--;                        /* einen weniger   */
            rwndmb((MBHEAD *)ulink((LEHEAD *)mbp));  /* zurueckspulen   */
            rspini(HMRSTAT);                         /* Antwort starten */
            while (mbp->mbpc > mbp->mbgc)
              putchr(getchr(mbp), hstmbp);
            if (stamp)                               /* Zeit dazu       */
            {
              putstr(" - ", hstmbp);
              puttim(&mbp->btime, hstmbp);
            }
            dealmb(mbp);                             /* Buffer weg      */
            return;                                  /* fertig          */
          }
        }
      }
      else
      {
        if (hstusr->outlin                           /* Meldungen und   */
            && (par != MBSTATUS))                    /* gewuenscht      */
        {
          for (mbp = (MBHEAD *)hstusr->outbuf.head;
               mbp != (MBHEAD *)&hstusr->outbuf;
               mbp = (MBHEAD *)mbp->nextmh)
            if (mbp->type == L2MNIX)                 /* Info-Frame!     */
            {
              rspini(HMRCONI);
              n = mbp->mbpc - mbp->mbgc;
              if (n > 256)
              {
                call2str(tmpcall, hstusr->call);
                fd = xfopen("hostmode.err", "a");
#ifdef SPEECH
                fprintf(fd, speech_message(248), n, tmpcall);
#else
                fprintf(fd, "Laenge = %d; Call = %s\n", n, tmpcall);
#endif
                fclose(fd);
                n = 256;
              }
              putchr((BYTE)(n - 1), hstmbp);                 /* Laenge ausgeben */
              while (n-- > 0)                        /* Info ausgeben   */
                putchr(getchr(mbp), hstmbp);
              if (mbp->mbpc - mbp->mbgc > 0)
                return;
              mbp = (MBHEAD *)ulink((LEHEAD *)mbp);
              hstusr->outlin--;                      /* einen weniger   */
              dealmb(mbp);                           /* Buffer weg      */
              return;
            }
        }
        else
          rspsuc();
      }
    }
  }
  else
    rspic('G');                           /* Terminal-Mode: G ungueltig */
}

/************************************************************************/
/*                                                                      */
/* EXTENDED HOSTMODE COMMAND    nach DG3DBI, ueberarbeitet von DB2OS    */
/*                                                                      */
/* Der Rechner sendet ein 'G'-Kommando an den Kanal 255.                */
/* Handelt es sich um einen Slave ohne Erweiterung, so antwortet dieser */
/* mit der Fehlermeldung "INVALID CHANNEL NUMBER".                      */
/* Bei dem erweiterten Hostmode wird stattdessen ein Null-terminierter  */
/* String zurueckgeliefert, der eine Liste von Kanaelen enthaelt, bei   */
/* denen noch Infos in den Buffern abholbereit sind. Dies gilt auch     */
/* fuer den Monitor-Kanal und Statusdaten. Die Kanalnummern im String   */
/* sind wegen der Null-Terminierung um 1 erhoeht.                       */
/*                                                                      */
/* Beispiele fuer Antwort des TNC:                                      */
/*       0xFF 0x01 0x00   - Keine Daten verfuegbar.                     */
/*       0xFF 0x01 0x01 0x00 - Es sind Daten im Monitor-Buffer.         */
/*       0xFF 0x01 0x01 0x03 0x04 0x00 - Es sind Daten im Monitor und   */
/*                                       in den Kanaelen 2 und 3.       */
/*                                                                      */
/************************************************************************/
static void
xGcmd(void)
{
  static unsigned n;                    /* Index auf Kanaele            */

  rspini(HMRSMSG);                      /* Success with Response        */

  if ((mifmbp != NULL)                  /* Ist was im Monitor-Buffer?   */
      || ((LHEAD *)statml.head != &statml)
      || ((LHEAD *)smonfl.head != &smonfl))
    putchr(1, hstmbp);                  /* Ja, melden...                */

  for (n = 1; n < MAXHST; ++n)        /* Haben wir etwas auf den Links? */
    if (hstubl[n].outlin || hstubl[n].outsta)
#ifdef __WIN32__
      putchr((char)(n + 1), hstmbp);          /* Ja, diesen Kanal melden..      */
#else
      putchr(n + 1, hstmbp);          /* Ja, diesen Kanal melden..      */
#endif /* WIN32 */
}

/************************************************************************/
/*                                                                      */
/* Host-Call anzeigen bzw. setzen. Ohne Parameter wird das globale      */
/* Hostcall ausgegeben (TNT/DP haette das gerne so). Mit Parameter wird */
/* das Hostcall gesetzt: Auf Kanal 0 wird das globale Hostcall (hostid) */
/* gesetzt, das beim Connect-Befehl verwendet wird, um eine Verbindung  */
/* zum Host herzustellen. Ausserdem werden die temporaeren Hostcalls    */
/* fuer alle nicht connecteten Kanaele gesetzt (hstusr->call). Dies     */
/* gilt auch fuer die anderen Hostkanaele, falls das Host-Call das      */
/* gleiche Rufzeichen ist, wie das Knoten-Call (myid). Sonst wird auf   */
/* allen anderen Kanaelen nur das temporaere Hostcall des entsprechen-  */
/* den Hostkanals (hstusr->call) gesetzt, wenn der Kanal nicht          */
/* connected ist.                                                       */
/*                                                                      */
/************************************************************************/
void
Icmd(void)
{
  char            tempcall[L2IDLEN];
  char            tempalias[L2CALEN];
  HOSTUS         *tmpusr;

  if (!blicnt)
  {
    rspini(HMRSMSG);
    putid(hstusr->direction ? hstusr->call : hostid, hstmbp);
  }
  else
  {
/* Es soll ein Call auf dem aktiven Kanal gesetzt werden                */
    if (getcal(&blicnt, &blipoi, TRUE, tempcall) == YES)
    {
      if ((actch == 0) || tnb_ch)
      {
/* Auf dem Monitorkanal bzw. ueber den CCP-Befehl "ESC I" wird ausser   */
/* dem Host-Call auch der Alias fuer den Hostmode gesetzt. Wenn kein    */
/* Alias angegeben wird, ist es eine Fehlbedienung, die ignoriert wird. */
        skipsp(&blicnt, &blipoi);
        if (getide(&blicnt, &blipoi, tempalias) == YES)
        {
/* bevor neu eingetragen wird, muss der alte Eintrag geloescht werden   */
          if (!cmpid(hostid, myid))
            unregister_neigb(hostid, "", L2PNUM);
/* Das neue Host-Call wird festgelegt und fuer alle nicht connecteten   */
/* Hostkanaele verwendet.                                               */
          cpyid(hostid, tempcall);
          for (tmpusr = &hstubl[1]; tmpusr < &hstubl[MAXHST]; tmpusr++)
          {
            if (!tmpusr->conflg)
              cpyid(tmpusr->call, hostid);
          }
/* Wenn Host-Call und Knoten-Call unterschiedlich sind, wird ein Local- */
/* Link-Eintrag vorgenommen, um das Call im Netz bekanntzugeben.        */
          if (!cmpid(hostid, myid))
#ifdef PROXYFUNC
            skipsp(&blicnt, &blipoi);
#endif
            register_neigb(hostid, "", tempalias, L2PNUM, LOCAL
#ifdef PROXYFUNC
                          | (((blicnt > 0) && (*blipoi == 'p' || *blipoi == 'P')) ? PROXYMASK : 0)
#endif
#ifdef LINKSMODINFO
                          ,"Local-Route"
#endif /* LINKSMODINFO */
#ifdef AUTOROUTING
                          , FIXED_ROUTE /* Route ist immer Fest. */
#endif /* AUTOROUTING */
                          );
        }
      }
      else                     /* Call fuer einzelnen Host-Kanal setzen */
      {
        if (hstusr->conflg)
        {
          rsperr(HMENWC);
          return;
        }
        else
          cpyid(hstusr->call, tempcall);
      }
      rspsuc();
    }
    else
      rsperr(HMEICS);
  }
}

/************************************************************************/
/*                                                                      */
/* Umschalten vom / zum Hostmode                                        */
/*                                                                      */
/************************************************************************/
void
Jcmd(void)
{
  UWORD           par;

  if (blicnt >= 4)
  {
    if (strnicmp((char *)blipoi, "HOST", 4) == 0)
    {
      blicnt -= 4;
      blipoi += 4;
      if (!skipsp(&blicnt, &blipoi))
        rsppar(ishmod);
      else if ((par = nxtnum(&blicnt, &blipoi)) <= 1)
      {
        rspsuc();
        ishmod = par;
      }
      else
        rspiv(par);
      return;
    }
  }
  rspic('J');
}

/************************************************************************/
/*                                                                      */
/* K-Befehl (Uhrzeit bei Monitor / Status-Meldungen)                    */
/*                                                                      */
/************************************************************************/
void
Kcmd(void)
{
  UWORD           par;

  if (!blicnt)
  {
    rspini(HMRSMSG);
    putprintf(hstmbp, "%d ", stamp);
    puttim(&sys_time, hstmbp);
  }
  else
  {
    if ((par = nxtnum(&blicnt, &blipoi)) <= 2)
      if (!blicnt)
        stamp = par;
    rspsuc();                   /* Zeit kann hier nicht gesetzt werden  */
  }
}

/************************************************************************/
/*                                                                      */
/* "list channel" - fuer den Kanal chnl anzeigen, wer connected ist und */
/* wieviele Frames noch zu bearbeiten sind (rx / tx)                    */
/*                                                                      */
/************************************************************************/
static void
listch(WORD chnl)
{
  unsigned        mbnmbr;
  HOSTUS         *hostp;
  int             cstate;

  putprintf(hstmbp, "%c(%u) ", chnl == actch ? '+' : ' ', chnl);

  if (chnl != 0)
  {
    hostp = &hstubl[chnl];
    cstate = hostp->conflg;

    if (cstate)                                 /* CONNECTED            */
      putid(myid, hstmbp);

    if ((mbnmbr = hostp->outlin + hostp->outsta) != 0
        || cstate)
#ifdef SPEECH
      putprintf(hstmbp, speech_message(249), mbnmbr);
#else
      putprintf(hstmbp, "\r      receive %u", mbnmbr);
#endif
    if (cstate)
#ifdef SPEECH
      putprintf(hstmbp, speech_message(250), hostp->inlin);
#else
      putprintf(hstmbp, "   send %u   unacked 0", hostp->inlin);
#endif
  }
  putchr('\r', hstmbp);
}

/************************************************************************/
/*                                                                      */
/* L-Befehl - Logout an der Konsole (nur Terminalmode) und anzeigen     */
/* wieviele Frames noch zu bearbeiten sind, fuer einen oder alle Host-  */
/* kanaele                                                              */
/*                                                                      */
/************************************************************************/
void
Lcmd(void)
{
  WORD            n;
  UWORD           par;

  if (!ishmod)
  {
    if (strnicmp((char *)blipoi, "OGOUT", 5) == 0)
      console_login_status = 0;
    else
    {
      if (!blicnt)
        for (n = 1; n <= Ypar2; n++)
          listch(n);
      else
      {
        if ((par = nxtnum(&blicnt, &blipoi)) <= Ypar2)
          listch(par);
        else
          rspiv(par);
      }
    }
  }
  else
  {
    rspini(HMRSMSG);
    if (!actch)
      putprintf(hstmbp, "%u %u", stalin, monlin);
    else
      putprintf(hstmbp, "%u %u %u %u 0 %u",
                hstusr->outsta, hstusr->outlin,
                hstusr->inlin, hstusr->inlin,
                hstusr->conflg ? L2SIXFER : L2SDSCED);
  }
}

/************************************************************************/
/*                                                                      */
/* M-Befehl - einstellen / abfragen der Monitor-Parameter               */
/*                                                                      */
/************************************************************************/
void
Mcmd(void)
{
  moncmd(NULL, &consmon, blipoi, blicnt);
}

/************************************************************************/
/*                                                                      */
/* QUIT-Befehl - Programm beenden                                       */
/*                                                                      */
/************************************************************************/
void
Qcmd(void)
{
  if ((strnicmp((char *)blipoi, "UIT", 3) == 0) && !ishmod)
    quit_program(0);
  else
    rspic('Q');
}

/************************************************************************/
/*                                                                      */
/* R-Befehl - Anzeige von Token-Recoveries ein- und ausschalten         */
/*                                                                      */
/************************************************************************/
void
Rcmd(void)
{
  UWORD           arg;

  if (blicnt == 0)
    rsppar(show_recovery);
  else
  {
    if ((arg = nxtnum(&blicnt, &blipoi)) <= 1)
    {
      show_recovery = arg;
      rspsuc();
    }
    else
      rspiv(arg);
  }
}

/************************************************************************/
/*                                                                      */
/* S-Befehl - Kanal-Umschaltung (nur Terminal-Modus)                    */
/*                                                                      */
/************************************************************************/
void
Scmd(void)
{
  WORD            arg;

  if (!ishmod)
  {
    if (!blicnt)
      rsppar(actch);
    else
    {
      if ((arg = nxtnum(&blicnt, &blipoi)) <= Ypar2 && arg > 0)
      {
        hstusr = &hstubl[actch = arg];
        rspcs();
      }
      else
        rspiv(arg);
    }
  }
  else
    rspic('S');
}

/************************************************************************/
/*                                                                      */
/* Tokenring-Baudrate anzeigen / setzen                                 */
/*                                                                      */
/************************************************************************/
void
Tcmd(void)
{
#if !defined(__LINUX__) && !defined(__WIN32__)
  ULONG           arg;

  if (!ishmod)                          /* im Hostmode sperren          */
  {
    if (!blicnt)
      rsppar(tkbaud * 100L);
    else
    {
      arg = nxtlong(&blicnt, &blipoi);
      if (tkcom > -1)
        tkbaud = setbaud(tkcom, (WORD)(arg / 100L));
      rspsuc();
    }
  }
  else
#endif
    rspic('T');
}

/************************************************************************/
/*                                                                      */
/* Version anzeigen                                                     */
/*                                                                      */
/************************************************************************/
void
Vcmd(void)
{
  rspini(HMRSMSG);
  putprintf(hstmbp, "%sXHOST)", signon);
}

/************************************************************************/
/*                                                                      */
/* Y-Befehl - zulaessige Zahl von connects anzeigen / setzen            */
/*                                                                      */
/************************************************************************/
void
Ycmd(void)
{
  UWORD           arg;

  if (blicnt == 0)
  {
    rspini(HMRSMSG);
    putprintf(hstmbp, "%d (%d)", Ypar, numhsts);
  }
  else
  {
    if ((arg = nxtnum(&blicnt, &blipoi)) <= Ypar2)
    {
      Ypar = arg;
      rspsuc();
    }
    else
      rspiv(arg);
  }
}

/************************************************************************/
/*                                                                      */
/* @Y-Befehl - Anzahl der zu verwendenden Hostkanaele anzeigen / setzen */
/*                                                                      */
/************************************************************************/
static void
xYcmd(void)
{
  UWORD           arg;
  HOSTUS         *tmpusr;

  if (blicnt == 0)
  {
    rspini(HMRSMSG);
    putprintf(hstmbp, "%d (%d)", Ypar2, MAXHST - 1);
  }
  else
  {
    arg = nxtnum(&blicnt, &blipoi);
    if ((arg < MAXHST) && (arg > 0) && (arg >= actch))
    {
/* alle Kanaele disconnecten, die kuenftig ungueltig sind               */
      if (arg < Ypar2)
      {
        tmpusr = hstusr;
        for (hstusr = &hstubl[arg + 1]; hstusr < &hstubl[MAXHST]; hstusr++)
        {
          if (hstusr->conflg)
            hstdis();
        }
        hstusr = tmpusr;
      }
/* Zahl der erlaubten Connect-Ports auf erlaubte Ports begrenzen        */
      if (arg < Ypar)
        Ypar = arg;
      Ypar2 = arg;
      rspsuc();
    }
    else
      rspiv(arg);
  }
}

/************************************************************************/
/*                                                                      */
/* "extended commands"                                                  */
/*                                                                      */
/* Erweiterte Befehle abarbeiten.                                       */
/*                                                                      */
/************************************************************************/
void
extcmd(void)
{
  char  ch;

  if (blicnt)
  {
    ch = *blipoi++;
    blicnt--;
    skipsp(&blicnt, &blipoi);
    switch (toupper(ch))
    {
      case 'B':
        rsppar(min(nmbfre, 1999));    /* max 1999 - wer weiss, ob sonst */
        break;                        /* das Terminalprg. verwirrt wird */
      case 'S':
        if ((actch) && (!tnb_ch))
          rsppar(hstusr->conflg ? L2SIXFER : L2SDSCED);
        else
          rspiec('S');
        break;
      case 'E':
        xEcmd();
        break;
      case 'Y':
        if (!actch || tnb_ch)
          xYcmd();
        else
          rspiec('Y');
        break;
      default:
        rspiec(ch);
    }
  }
  else
    rspsuc();
}

/************************************************************************/
/*                                                                      */
/* "host mode put response"                                             */
/*                                                                      */
/************************************************************************/
void
hmputr(unsigned r)
{
#ifdef __WIN32__
  hputc((char)actch);
  hputc((char)r);
#else
  hputc(actch);
  hputc(r);
#endif /* WIN32 */
}

/************************************************************************/
/*                                                                      */
/* "response init"                                                      */
/*                                                                      */
/************************************************************************/
void
rspini(unsigned r)
{
  if (tnb_ch)
    return;
  if (!ishmod)
    putstr("* ", hstmbp);
  else
  {
#ifdef __WIN32__
    putchr((char)actch, hstmbp);
    putchr((char)r, hstmbp);
#else
    putchr(actch, hstmbp);
    putchr(r, hstmbp);
#endif /* WIN32 */

  }
  hstmbp->l4type = r;
}

/************************************************************************/
/*                                                                      */
/* "response success"                                                   */
/*                                                                      */
/************************************************************************/
void
rspsuc(void)
{
  if (tnb_ch)
    return;
  if (ishmod)
#ifdef __WIN32__
    putchr((char)actch, hstmbp);
#else
    putchr(actch, hstmbp);
#endif /* WIN32 */
}

/************************************************************************/
/*                                                                      */
/* "response invalid command"                                           */
/*                                                                      */
/************************************************************************/
static void
rspic(unsigned c)
{
  rspini(HMRFMSG);
#ifdef SPEECH
  putstr(speech_message(256), hstmbp);
#else
  putstr("INVALID COMMAND: ", hstmbp);
#endif
#ifdef __WIN32__
  putcc((char)c, hstmbp);
#else
  putcc(c, hstmbp);
#endif /* WIN32 */
}

/************************************************************************/
/*                                                                      */
/* "response invalid extended command"                                  */
/*                                                                      */
/************************************************************************/
void
rspiec(unsigned c)
{
  rspini(HMRFMSG);
#ifdef SPEECH
  putstr(speech_message(257), hstmbp);
#else
  putstr("INVALID EXTENDED COMMAND: ", hstmbp);
#endif
#ifdef __WIN32__
  putcc((char)c, hstmbp);
#else
  putcc(c, hstmbp);
#endif /* WIN32 */
}

/************************************************************************/
/*                                                                      */
/* "response invalid value"                                             */
/*                                                                      */
/************************************************************************/
static void
rspiv(unsigned value)
{
  if (tnb_ch)
    return;
  rspini(HMRFMSG);
#ifdef SPEECH
  putprintf(hstmbp, speech_message(251), value);
#else
  putprintf(hstmbp, "INVALID VALUE: %u", value);
#endif
}

/************************************************************************/
/*                                                                      */
/* Fehlermeldungen                                                      */
/*                                                                      */
/************************************************************************/
const char     *hm_err[] =
{
  "CHANNEL ALREADY CONNECTED",
  "INVALID PARAMETER",
  "INVALID CALLSIGN",
  "TNC BUSY - LINE IGNORED",
  "ALREADY OPEN",
  "FILE NOT OPEN",
  "NOT WHILE CONNECTED",
  "INVALID PORT"
};

/************************************************************************/
/*                                                                      */
/* "response error"                                                     */
/*                                                                      */
/************************************************************************/
void
rsperr(int i)
{
  rspini(HMRFMSG);
  putstr(hm_err[i], hstmbp);
}

/************************************************************************/
/*                                                                      */
/* "response channel status"                                            */
/*                                                                      */
/************************************************************************/
static void
rspcs(void)
{
  rspini(HMRSMSG);
  if (!actch)
    putid(myid, hstmbp);
  else
    if (hstusr->conflg != 0)
      putid(hstusr->direction ? myid : hstusr->call, hstmbp);
    else
#ifdef SPEECH
      putstr(speech_message(253), hstmbp);
#else
      putstr("CHANNEL NOT CONNECTED", hstmbp);
#endif
}

/************************************************************************/
/*                                                                      */
/* "response parameter"                                                 */
/*                                                                      */
/************************************************************************/
static void
rsppar(ULONG par)
{
  rspini(HMRSMSG);
  putprintf(hstmbp, "%lu", par);
}

/************************************************************************/
/*                                                                      */
/* Zeichen in Buffer schreiben, Steuerzeichen mit '^' vorweg            */
/*                                                                      */
/************************************************************************/
static void
putcc(char c, MBHEAD *mbp)
{
  if (c < ' ')
  {
    putchr('^', mbp);
#ifdef __WIN32__
    putchr((char)(c + '@'), mbp);
#else
    putchr(c + '@', mbp);
#endif /* WIN32 */
  }
  else
    putchr(c, mbp);
}

/* End of src/l7hstcmd.c */
