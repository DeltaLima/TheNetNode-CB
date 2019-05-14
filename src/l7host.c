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
/* File src/l7host.c (maintained by: DF6LN)                             */
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

WORD        startup_running = FALSE;    /* laeuft Start-Batch?          */
int         console_login_status = 0;   /* Host-Console eingeloggt      */

WORD        last_hour;       /* letzte Stunde (fuer Batch-Verarbeitung) */
WORD        last_min;        /* letzte Minute merken                    */
FILE       *tnnb_file;       /* aktives Batch-File                      */
FILE       *pro_file = NULL; /* Protokoll_File                          */
BOOLEAN     ishmod = FALSE;  /* HOSTMODE aktiv?                         */
int         numhsts = 0;     /* Aktive Host-Connects                    */
int         Ypar = 0;        /* Maximalzahl der zulaessigen Host-Verb.  */
int         Ypar2 = MAXHST - 1;  /* Max. Zahl verwendbarer Hostkanaele  */
LHEAD       smonfl;          /* Liste der Monitor-Daten (Header,Info)   */
LHEAD       statml;          /* Liste der Statusmeldungen (Monitor)     */
int         monlin;          /* Zahl gespeicherte Monitor-Daten         */
int         stalin;          /* Zahl gespeicherte Meldungen (Monitor)   */

MBHEAD     *mifmbp = NULL;   /* angefangener Monitor-Buffer (Hostmode)  */

WORD        actch;           /* momentan bearbeiteter Host-Channel      */
BOOLEAN     tnb_ch;          /* TRUE = TNB-Kanal, FALSE sonst           */

char        hinbuf[256];     /* Zeile vom Hostinterface                 */
BOOLEAN     hlixfl;          /* X-on / X-off Flag fuer Hostinterface    */
int         incnt;           /* Eingabezaehler                          */
char       *inbufp;          /* Pointer fuer die Eingabezeile           */

char       *blipoi;          /* Pointer                                 */
WORD        blicnt;          /* zaehlt Zeichen                          */

HOSTUS     *hstubl;          /* Kontrollblock fuer Host-User            */
HOSTUS     *hstusr;          /* Pointer auf aktuellen Host User         */

LHEAD       hstatl;
MBHEAD     *hstmbp;

/* Zielcall, Digi-Liste und Port fuer UI-Frames Hostmode Kanal 0 (z.B.  */
/* fuer MAIL-Beacons einer Mailbox)                                     */
char hmdest[L2IDLEN] = {' ', ' ', ' ', ' ', ' ', ' ', '\140'};
char hmdigil[L2VLEN+1] = "";
WORD hmport = 0;

static const char *tmp_tnb = "TNB.$$$";
extern char    *hm_err[];

void            cputmb(MBHEAD *);

static void     hstol7(WORD, void *);
static void     trpmbb(MBHEAD *);
static BOOLEAN  termle(unsigned);
static void     tohost(void);
static void     srvbch(void);
static void     srvhch(void);
static void     sttoch(unsigned);

/************************************************************************/
/*                                                                      */
/* TheNetNode Hostinterface                                             */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*                                                                      */
/* Die vorliegende Version des Hostinterface beinhaltet grundlegende    */
/* Veraenderungen am urspruenglichen Host-Konzept, um eine bessere      */
/* Ausnutzung insbesondere fuer die Singelboard-Loesungen (TNC3 und     */
/* TNC4) zu ermoeglichen. Dabei wird der Host-Kanal 0 gemeinsam         */
/* verwendet fuer den Monitor an der Konsole und fuer die Ausfuehrung   */
/* von Batch-Files. Die Konsole kann daher nicht mehr auf dem Kanal 0   */
/* verwendet werden. An sonsten ist das Interface vom Verhalten im      */
/* Terminal-Mode kompatibel zu TheFirmware, abgesehen von den verfueg-  */
/* baren Befehlen.                                                      */
/*                                                                      */
/* - Meldungen (Status) werden pro Kanal gepuffert                      */
/* - die wichtigen Konsolen-Befehle werden unterstuetzt:                */
/*   C, D, I, K, L, M, S, V, Y, @B                                      */
/* - zusaetzliche Befehle an der Konsole:                               */
/*   B      = anzeigen Rounds/s                                         */
/*   @E     = Protokoll_File ein/aus                                    */
/*   LOGOUT = Konsole ausloggen                                         */
/*   QUIT   = Programmende                                              */
/*   T      = Tokenring-Baudrate                                        */
/*   @Y     = max. verwendete Konsolenkanaele                           */
/*   @C     = Port, Ziel-Call + Digiliste fuer UI-Frames setzen         */
/*                                                                      */
/* Weiterhin wird der Hostmode unterstuetzt. Dafuer wird die Konsole    */
/* bei der DOS/GO32-Version auf eine serielle Schnittstelle umgeleitet, */
/* bei Linux wird ueber eine Socket-Schnittstelle gearbeitet. Der Host- */
/* mode unterstuetzt die Befehle C, D, G, I, K, L, M, V, Y, @B, @C, @S, */
/* @Y. Der Befehl E wird ignoriert (fuehrt also nicht zu einer Fehler-  */
/* meldung). Der Monitor ist voll funktionsfaehig.                      */
/*                                                                      */
/* Alle Konsolenbefehle schreiben die Ausgaben in einen Buffer          */
/* (hstmbp). Je nach Aufruf von der Konsole, vom Hostmode oder ueber    */
/* einen Batch wird der Buffer-Inhalt weitergeleitet. Damit sind die    */
/* Konsole bzw. Hostmode und Batches voneinander unabhaengig. Die       */
/* Ausgaben von Batchbefehlen koennen in einem File protokolliert       */
/* werden.                                                              */
/*                                                                      */
/************************************************************************/


/************************************************************************/
/*                                                                      */
/* Initialisieren der Host-Variablen                                    */
/*                                                                      */
/************************************************************************/
void
init_host(void)
{
  char            startup[20];
  WORD            i;

  tnb_ch = FALSE;
  actch = 1;
  init_console();

  inithd(&smonfl);
  inithd(&statml);
  inithd(&hstatl);

  for (i = 0, hstusr = hstubl; i < MAXHST; ++i, ++hstusr)
  {
#ifdef __WIN32__
      hstusr->conflg = 0;
      hstusr->disflg = 0;
      hstusr->inlin = 0;
      hstusr->outlin = 0;
      hstusr->outsta = 0;
#else
      hstusr->conflg =
      hstusr->disflg =
      hstusr->inlin =
      hstusr->outlin =
      hstusr->outsta =
#endif /* WIN32 */
      hstusr->direction = 0;
    inithd(&hstusr->inbuf);
    inithd(&hstusr->outbuf);
    cpyid(hstusr->call, hostid);
  }
  hlixfl = FALSE;
  incnt = 0;
  monlin = stalin = 0;
  tnnb_aktiv = FALSE;
  last_hour = 30;
  last_min = 70;

  xremove(tmp_tnb);             /* temporaer-File loeschen              */
  strcpy(startup, cfgfile);
  strcat(startup, ".TNB");
  if ((startup_running = runbatch(startup)) == TRUE)
  {
#ifndef MC68302
    pro_file = xfopen("STARTUP.LOG", "wt+");
#else
    pro_file = NULL;            /* Platz sparen und Zeit                */
#endif
#ifdef ST
    setvbuf(pro_file, NULL, _IOFBF, 4096L);  /* speedup                 */
#endif
  }
  else
    xprintf("*** WARNING: %s not found!\r", startup);
}

/************************************************************************/
/*                                                                      */
/*                                                                      */
/*                                                                      */
/************************************************************************/
void
exit_host(void)
{
  exit_console();
}

/************************************************************************/
/*                                                                      */
/* "host to level 7"                                                    */
/*                                                                      */
/* Hier begegnet uns mal wieder das Problem mit dem globalen userpo     */
/* usw., wir duerfen hier nicht l2tol7 aufrufen, weil wir selber aus    */
/* l7tx() aufgerufen wurden und eventuell die Userliste oder userpo     */
/* geaendert werden. Das Prinzip Bottom-Up (Meldungen nur von unten     */
/* nach oben) muss gewahrt bleiben, also melden wir spaeter.            */
/*                                                                      */
/************************************************************************/
static void
hstol7(WORD status, void *link)
{
  MBHEAD         *mbhd;

  (mbhd = (MBHEAD *)allocb(ALLOC_MBHEAD))->type = status;
  mbhd->l2link = link;
  relink((LEHEAD *)mbhd, (LEHEAD *)hstatl.tail);
}

/************************************************************************/
/*                                                                      */
/* "terminal mode response message buffer bell"                         */
/*                                                                      */
/************************************************************************/
static void
trpmbb(MBHEAD *mbp)
{
  hputs("\r* ");
  rwndmb(mbp);
  hputmb(mbp);
  if (stamp)
  {
    hputs(" - ");
    hputbt(&mbp->btime);
  }
  hputs(" *\007\r");
}

/************************************************************************/
/*                                                                      */
/* "terminal mode line editor"                                          */
/*                                                                      */
/************************************************************************/
static BOOLEAN
termle(unsigned ch)
{
  if (incnt == 0)
    inbufp = hinbuf;            /* leere Zeile: Pointer = Start         */

  switch (ch)                   /* ueber Eingabe verzweigen             */
    {
      case CR:
        hputc(CR);              /* abspeichern                          */
        if (hinbuf[0] != ESC)   /* Textzeilen mit CR                    */
        {
          *inbufp++ = CR;
          incnt++;
        }
        *inbufp = NUL;          /* Ende markieren                       */
        blipoi = hinbuf;        /* blipoi auf Eingabezeile              */
        blicnt = incnt;         /* Laenge merken                        */
        inbufp = hinbuf;        /* wieder auf Start                     */
        incnt = 0;              /* neue Zeile beginnt                   */

        if (console_login_status)       /* ist Console eingeloggt ?     */
          return (TRUE);                /* eine Zeile ist fertig        */
        else
        {
          if (strnicmp((char *)blipoi, "\x1BJHOST", 6) == 0)
          {
            blipoi += 2;        /* ESC uebergehen                       */
            blicnt -= 2;
            Jcmd();             /* Umschaltung ermoeglichen             */
            break;
          }
          if (strstr((char *)blipoi, pass) == (char *)blipoi)
          {
            console_login_status = 1;   /* die Console einloggen        */
#ifdef SPEECH
            hprintf(speech_message(242), signon);
#else
            hprintf("Welcome to %s", signon);
#endif
            hputid(myid);
            hputs(")\r");
          }
          else
            hputs("password: ");
        }
        break;

      case BS:                  /* BS und DEL werden gleich             */
      case DEL:                 /* behandelt und loeschen               */
        if (incnt)              /* destruktiv. Sie werden aber          */
          bliloe();             /* ignoriert.                           */
        break;

      case 0x15:                /* ctl-U und                            */
      case 0x18:                /* ctl-X loeschen Eingabezeile          */
        while (incnt)
          bliloe();
        break;

      case XON:                 /* XON loescht Flag                     */
        hlixfl = FALSE;
        break;

      case XOFF:                /* XOFF setzt Flag                      */
        hlixfl = TRUE;
        break;

      default:
        if ((incnt < 255)
            && ((ch >= ' ') || (ch == ESC) || (ch == 26)))
        {
          blieco(ch);
          *inbufp++ = ch;
          ++incnt;
        }
        else
          hputc(BELL);          /* Zeile voll: meckern                  */
    }
  return (FALSE);
}

/************************************************************************/
/*                                                                      */
/* "to host"                                                            */
/*                                                                      */
/************************************************************************/
static void
tohost(void)
{
  MBHEAD         *mbp;

  if (nmbfre < 14)              /* kein Speicher                        */
  {
    if (!ishmod)
      hprintf("* %s *\007\n", hm_err[HMELIG]);
    else
    {
#ifdef __WIN32__
      hputc((char)actch);
#else
      hputc(actch);
#endif /* WIN32 */
      hputs(hm_err[HMELIG]);
    }
    return;
  }
  if (!hstusr->conflg)          /* nicht connected?                     */
  {
    if (!ishmod)
    {
#ifdef SPEECH
      hprintf(speech_message(243));
#else
      hprintf("* CHANNEL NOT CONNECTED *\007\n");
#endif
      return;
    }
  }
  else
  {
    if (!(hstusr->disflg & 0x80))       /* schon DISCed                 */
    {
      mbp = (MBHEAD *)allocb(ALLOC_MBHEAD);
      while (blicnt--)
        putchr(*blipoi++, mbp);
      rwndmb(mbp);
      relink((LEHEAD *)mbp, (LEHEAD *)hstusr->inbuf.tail);
      ++hstusr->inlin;
      hstsen(FALSE);                    /* Info an Rest des Programms   */
    }
  }
  if (ishmod && !tnb_ch)
  {
#ifdef __WIN32__
    hputc((char)actch);
#else
    hputc(actch);
#endif /* WIN32 */
    hputc(0);
  }
}

/************************************************************************/
/*                                                                      */
/* "service batch channel"                                              */
/*                                                                      */
/************************************************************************/
static void
srvbch(void)
{
  MBHEAD         *mbp;
  char            bline[256];   /* nicht die Host-Buffer!               */
  USRBLK         *user;

  hstusr = hstubl;              /* auf den System-Kanal                 */

/* eingelaufene Infos ausgeben, Statusmeldungen ignorieren              */

  while ((LHEAD *)hstusr->outbuf.head != &hstusr->outbuf)
  {
    mbp = (MBHEAD *)ulink((LEHEAD *)hstusr->outbuf.head);
    if (mbp->type == L2MNIX)            /* Info-Frame                   */
    {
      --hstusr->outlin;                 /* ein Frame weniger            */
      cputmb(mbp);                      /* Frame ausgeben               */
    }
    else                                /* Meldung                      */
      --hstusr->outsta;                 /* eine Meldung weniger         */
    dealmb(mbp);                        /* Buffer dann auf den Muell    */
  }

/* abarbeiten von Batch-Files                                           */

  if (tnnb_aktiv)
  {

/* ggf. erstmal zum Knoten connecten                                    */

    if (!hstusr->conflg)
    {
      hstcon(1);
      return;
    }

/* der Befehl muss erst komplett ausgefuehrt sein, bevor der naechste   */
/* gestartet wird, d.h. saemtliche Ausgaben muessen beendet sein.       */

    user = (ptctab + g_uid(hstusr, HOST_USER))->ublk;
    if (hstusr->outlin
        || hstusr->outsta
        || (user->status > US_CCP)
        || (user->fp != NULL))
      return;

/* Naechsten Befehl holen und in Protokollfile schreiben (wenn          */
/* verwendet). Befehle werden mit CR terminiert.                        */

    if (fgets(bline, 255, tnnb_file))
    {
      bline[255] = NUL;                 /* auf jeden Fall terminieren   */
      if ((blipoi = strchr(bline, '\n')) != NULL)
      {
        if (pro_file != NULL)       /* ggf. Befehl in ein Protokollfile */
          fputs(bline, pro_file);
        *blipoi = CR;               /* '\n' durch CR ersetzen           */
      }
      blipoi = bline;
      blicnt = strlen((char *)blipoi);  /* Laenge ermitteln             */
      tohost();                         /* Info an den L7               */
    }
    else
    {
      if (hstusr->inlin || hstusr->outlin)/* noch Reste zu verarbeiten? */
        return;
      fclose(tnnb_file);                /* File wieder schliessen       */
      xremove(tmp_tnb);                 /* temporaer-File loeschen      */
      tnnb_aktiv = FALSE;
      if ((hstusr->conflg)
          && (user->auditlevel == 0)
          && (user->monitor == NULL))
        hstdis();                       /* Kanal rauswerfen             */
    }
  }
  else
  {
    if (startup_running)
    {
      if (pro_file != NULL)
      {
        fclose(pro_file);
        pro_file = NULL;
      }
      startup_running = FALSE;
    }
  }
}

/************************************************************************/
/*                                                                      */
/* "service host channel"                                               */
/*                                                                      */
/************************************************************************/
static void
srvhch(void)
{
  MBHEAD         *mbp;
  unsigned        ch;
  static int      hmstat = 0;           /* Hostmode Status              */
  static int      hmch;                 /* Hostmode Kanal               */
  static int      hmcmd;                /* Hostmode Befehl              */
  static int      hmlen;                /* Hostmode Datenbytes          */

/* Kanaele disconnecten, wenn disco gefordert und alle Info abgeholt    */

  for (hstusr = &hstubl[1]; hstusr < &hstubl[MAXHST]; hstusr++)
    if (((hstusr->disflg & 0x80) != 0)
        && (hstusr->outlin == 0)
        && (hstusr->outsta == 0))
      hstdis();

  hstusr = &hstubl[actch];              /* auf aktuellen Kanal          */
  if (!ishmod)                          /* kein Hostmode?               */
  {
    if (!ishput())                      /* keine Reste auszugeben       */
      if (!incnt && !hlixfl)            /* wenn am Zeilenanfang und     */
      {                                 /* kein XOFF Status             */
        while (stalin)                  /* solange noch Meldungen       */
        {
          mbp = (MBHEAD *)ulink((LEHEAD *)statml.head);
          trpmbb(mbp);
          dealmb(mbp);
          stalin--;
        }
        while (monlin)                  /* solange Frames fuer Monitor  */
        {
          mbp = (MBHEAD *)ulink((LEHEAD *)smonfl.head);
          rwndmb(mbp);
          hputmb(mbp);
          hputc('\r');
          dealmb(mbp);
          monlin--;
        }

/* eingelaufene Infos ausgeben                                          */

        while ((LHEAD *)hstusr->outbuf.head != &hstusr->outbuf)
        {
          mbp = (MBHEAD *)ulink((LEHEAD *)hstusr->outbuf.head);
          if (mbp->type == L2MNIX)              /* Info-Frame           */
          {
            --hstusr->outlin;                   /* ein Frame weniger    */
            hputmb(mbp);                        /* Frame ausgeben       */
            if (mbp->l4type != HMRINFO)
              hputs(" *\r");
          }
          else                                  /* Meldung              */
          {
            --hstusr->outsta;                   /* eine Meldung weniger */
            trpmbb(mbp);                        /* Meldung ausgeben     */
          }
          dealmb(mbp);                  /* Buffer dann auf den Muell    */
        }
      }

    if (monlin > 50)                    /* damit der Knoten nicht voll- */
    {                                   /* laeuft                       */
      dealml((LEHEAD *)&smonfl);
      monlin = 0;                       /* Monitor leeren               */
    }

    if (ishget())               /* nun Eingaben bearbeiten:             */
    {
      ch = hgetc();             /* Zeichen holen, wenn vorhanden        */
      if (termle(ch))           /* Zeile fertig?                        */
      {
        if (*blipoi == ESC)     /* Befehl eingegeben?                   */
        {
          *blipoi++ = 0x00;
          --blicnt;
          skipsp(&blicnt, &blipoi);
          hstcmd((MBHEAD *)NULL);       /* Befehl ausfuehren            */
        }
        else                            /* Info fuer Kanal              */
          tohost();
      }
    }
  }
  else                                  /* Hostmode                     */
  {
    while (ishget())
    {
      ch = hgetc();                     /* ein Zeichen lesen            */
      switch (hmstat)
        {
          case 0:                       /* CHANNEL                      */
            hmch = ch;
            ++hmstat;
            break;

          case 1:                       /* COMMAND                      */
            hmcmd = ch;
            ++hmstat;
            break;

          case 2:                       /* LENGTH                       */
            hmlen = ch;
            ++hmstat;
            inbufp = hinbuf;
            incnt = 0;
            break;

          case 3:
            *inbufp++ = ch;
            ++incnt;
            if (hmlen != 0)
              --hmlen;
            else                        /* Hostmode-Packet komplett da  */
            {
              hmstat = 0;
              actch = hmch;
              if ((actch < MAXHST)
                  || ((actch == 0xFF)
                      && (hmcmd == TRUE)
                      && (toupper(*hinbuf) == 'G')))
              {
                if ((actch != 0)
                    && (actch != 0xFF))
                  hstusr = &hstubl[actch];
                blipoi = hinbuf;
                blicnt = incnt;
                if (hmcmd)                /* Befehl?                    */
                {
                  skipsp(&blicnt, &blipoi);
                  hstcmd((MBHEAD *)NULL); /* Konsolen-Befehl ausfuehren */
                }
                else                      /* Info                       */
                  if (actch)
                    tohost();             /* an den L7 senden           */
                else
                {
                  if (hmdest[0] != ' ')
                  {
                    (mbp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2fflg = L2CPID;
                    inbufp = hinbuf;
                    while (incnt--)
                      putchr(*inbufp++, mbp);
                    rwndmb(mbp);
#ifdef __WIN32__
                    sdui(hmdigil, hmdest, hostid, (char)hmport, mbp);
#else
                    sdui(hmdigil, hmdest, hostid, hmport, mbp);
#endif /* WIN32 */
                    dealmb(mbp);
                  }
#ifdef __WIN32__
                  hputc((char)actch);           /* Info an Monitor-Kanal      */
#else
                  hputc(actch);           /* Info an Monitor-Kanal      */
#endif /* WIN32 */
                  hputc(0);               /* ignorieren                 */
                }
              }
              else                        /* Fehler                     */
              {
                hmputr(HMRFMSG);
                hputs("INVALID CHANNEL NUMBER");
                hputc(0);
              }
              actch = 1; /* Schutzmassnahme XAO 050100 */
            }
            break;
        }
    }
  }
  hstsen(FALSE);                        /* Info an Rest des Programms   */
}

/************************************************************************/
/*                                                                      */
/* Host_Service                                                         */
/*                                                                      */
/* Statusmeldungen an L7 weiterleiten, Batches ausfuehren, Konsole      */
/* bedienen.                                                            */
/*                                                                      */
/************************************************************************/
void
hostsv(void)
{
  MBHEAD         *mbhd;

  while ((LHEAD *)hstatl.head != &hstatl)
  {
    mbhd = (MBHEAD *)ulink((LEHEAD *)hstatl.head);
    l2tol7(mbhd->type, mbhd->l2link, HOST_USER);
    dealoc(mbhd);
  }

  tnb_ch = TRUE;
  srvbch();                             /* Batch-Kanal behandeln        */
  tnb_ch = FALSE;
  srvhch();                             /* Host-Kanaele auch            */
}

/************************************************************************/
/*                                                                      */
/* Batches ausfuehren.                                                  */
/*                                                                      */
/************************************************************************/
BOOLEAN
runbatch(char *batchname)
{
  FILE           *batch_file;
  char            line[256];
  char           *lp;
  LONG            oldpos = 0L;

  strcpy(line, batchname);
  if (strchr(line, '.') == NULL)
    strcat(line, ".TNB");
  batch_file = xfopen(line, "rt");
  if (!batch_file)
    return (FALSE);                     /* nicht gefunden!              */
  if (tnnb_aktiv)                       /* laeuft schon ein Batch?      */
  {
    oldpos = ftell(tnnb_file);          /* Position merken              */
    fclose(tnnb_file);                  /* ... das ist zum lesen!       */
  }
  tnnb_file = xfopen(tmp_tnb, "at");
  if (!tnnb_file)                       /* Temp-File geht nicht auf?    */
  {
    fclose(batch_file);
    tnnb_aktiv = FALSE;
    return (FALSE);                     /* Fehler!                      */
  }
#ifdef ST
  setvbuf(tnnb_file, NULL, _IOFBF, 4096L);      /* speedup              */
#endif

/* Eine Zeile aus Batchfile einlesen, Zeile beim ersten ";" oder "\n"   */
/* beenden. Tabs entfernen, sonst kommt der CPP durcheinander. Leer-    */
/* zeichen am Zeilenende werden ebenfalls entfernt.                     */

  while (fgets(line, 255, batch_file) != NULL)
  {
    line[strcspn(line, ";\n\r")] = NUL;
    while ((lp = strchr(line, 9)) != NULL)
      *lp = 32;
    lp = strchr(line, 0) - 1;                           /* Zeilenende   */
    while ((lp > line) && (*lp == 32))
      *lp-- = NUL;

/* Zeilen mit "#" am Zeilenanfang als ESC-Befehl in Temporaer-File,     */
/* andere Zeilen unveraendert ins Temporaer-File. Leerzeilen ignorieren */

    if (line[0])
    {
      if (line[0] == '#')
        fprintf(tnnb_file, "ESC %s\n", &line[1]);
      else
        fprintf(tnnb_file, "%s\n", line);
    }
  }

  fclose(tnnb_file);                    /* File wieder schliessen       */
  fclose(batch_file);                   /* auch Batch File              */
  tnnb_file = xfopen(tmp_tnb, "rt");    /* ... und zum Lesen oeffnen    */
  if (tnnb_file)
  {
    fseek(tnnb_file, oldpos, SEEK_SET);
    tnnb_aktiv = TRUE;                  /* Markieren -> Batch laeuft    */
  }
  return (tnnb_aktiv);
}

/************************************************************************/
/*                                                                      */
/* Host-Timer                                                           */
/*                                                                      */
/* Noactivity-Timer fuer Konsolen-Kanaele reduzieren und ggf. Kanal     */
/* disconnecten. Zur vollen Stunde Batches starten.                     */
/*                                                                      */
/************************************************************************/
void
hostim(void)
{
  WORD            i;
  char            tnnb_d_name[15];
  char            tnnb_w_name[15];
  WORD            fertig;
  struct ffblk    tnnb_f;

  if (!tnnb_aktiv)
    for (i = 1, hstusr = hstubl + 1; i < MAXHST; i++, hstusr++)
    {
      if (hstusr->conflg)
      {
        if ((hstusr->noacti != 0) && (--hstusr->noacti == 0))
          hstdis();
      }
    }
  hstusr = &hstubl[actch];              /* auf aktuellen Kanal          */

  if (sys_localtime->tm_min != last_min)
  {
    last_min = sys_localtime->tm_min;         /* aktuelle Minute merken */
    if (xaccess("NOW.TNB", 0) == 0)
    {
      runbatch("NOW.TNB");
      xremove("NOW.TNB");
    }
  }

  if (sys_localtime->tm_hour != last_hour)
  {

/* Stuendlich kontrollieren, ob ein passendes TNB-File existiert. Dazu  */
/* die gesuchten Filenamen fuer taeglichen und woechentlichen Job       */
/* aus aktuellem Datum und aktueller Zeit generieren.                   */

    last_hour = sys_localtime->tm_hour;       /* aktuelle Stunde merken */
    sprintf(tnnb_d_name, "%02d%02d%02d%02d.TNB",
            sys_localtime->tm_year % 100, sys_localtime->tm_mon + 1,
            sys_localtime->tm_mday, sys_localtime->tm_hour);
    sprintf(tnnb_w_name, "%02d%02dW%1.1d%02d.TNB",
            sys_localtime->tm_year % 100, sys_localtime->tm_mon + 1,
            sys_localtime->tm_wday, sys_localtime->tm_hour);

/* Nun nach allen passenden Files suchen, Wildcards beachten.           */

    fertig = xfindfirst("*.TNB", &tnnb_f, 0);
    while (!fertig)
    {
      for (i = 0; i < 8; i += 2)                /* Beide Buffer testen  */
      {
        if (tnnb_f.ff_name[i] == '#')
          continue;
        if (strnicmp(&tnnb_f.ff_name[i], &tnnb_d_name[i], 2)
            && strnicmp(&tnnb_f.ff_name[i], &tnnb_w_name[i], 2))
          break;        /* Abbruch beim ersten nicht passenden Zeichen  */
      }
      if (i == 8)                      /* Name stimmt?                  */
        runbatch(tnnb_f.ff_name);      /* Batch ausfuehren              */
      fertig = xfindnext(&tnnb_f);     /* nichts gefunden, weitersuchen */
    }
  }
}

/************************************************************************/
/*                                                                      */
/* Connect-Wunsch zum Host bearbeiten                                   */
/*                                                                      */
/************************************************************************/
BOOLEAN
hstreq(void)
{
  MBHEAD         *mbp;

  if (numhsts < Ypar)
  {
    hstcon(0);
  }
  else
  {
#ifdef SPEECH
    putstr(speech_message(246), mbp = (MBHEAD *)allocb(ALLOC_MBHEAD));
#else
    putstr("CONNECT REQUEST fm ", mbp = (MBHEAD *)allocb(ALLOC_MBHEAD));
#endif
    putid(hstusr->call, mbp);
    mbp->btime = sys_time;
    mbp->type = L2MBUSYT;
    relink((LEHEAD *)mbp, (LEHEAD *)statml.tail);
    stalin++;
    hstol7(L2MBUSYF, hstusr);
    cpyid(hstusr->call, hostid);
    return (FALSE);
  }
  return (TRUE);
}

/************************************************************************/
/*                                                                      */
/* Host-User wurde vom CCP disconnected.                                */
/*                                                                      */
/************************************************************************/
void
hstout(void)
{
  dealml((LEHEAD *)&hstusr->inbuf);
  hstusr->inlin = 0;
  hstusr->disflg |= 0x80;
}

/************************************************************************/
/*                                                                      */
/* Info vom L7 an Host                                                  */
/*                                                                      */
/************************************************************************/
BOOLEAN
itohst(BOOLEAN conflg, MBHEAD *ublk)
{
  HOSTUS         *hostp;

  hostp = (HOSTUS *)ublk->l2link;

  if (hostp == hstubl)                  /* im Batch-Modus keine         */
    conflg = TRUE;                      /* Erstickungskontrolle.        */

  if ((conflg == TRUE)
      || (hostp->outlin < conctl))
  {
    if ((ublk->mbpc - ublk->mbgc) == 0) /* Frames ohne Info ignorieren  */
    {
      dealmb((MBHEAD *)ulink((LEHEAD *)ublk));
      return (TRUE);
    }
    relink(ulink((LEHEAD *)ublk), (LEHEAD *)hostp->outbuf.tail);
    ublk->type = L2MNIX;                /* Info-Frame (keine Meldung)   */
    ublk->l4type = HMRINFO;
    ++hostp->outlin;
    hostp->noacti = ininat;
    return (TRUE);
  }
  return (FALSE);
}

/************************************************************************/
/*                                                                      */
/* Echo fuer Konsole                                                    */
/*                                                                      */
/************************************************************************/
void
blieco(int zeichen)
{
  if (console_login_status)
  {
    if (incnt == 0 && zeichen == ESC)
      hputs("* ");
    else
#ifdef __WIN32__
      hputcc((char)zeichen);
#else
      hputcc(zeichen);
#endif /* WIN32 */
  }
  else                                  /* Password als ***** anzeigen */
    hputc('*');
}

/************************************************************************/
/*                                                                      */
/* 1 eingegebenes Zeichen im Buffer und in der Anzeige loeschen         */
/*                                                                      */
/************************************************************************/
void
bliloe(void)
{
  char            ch;

  if ((ch = *(--inbufp)) != BELL)
  {
    bputbs();
    if (ch < ' ')
      bputbs();
  }
  else
    hputc(ch);
  --incnt;
}

/************************************************************************/
/*                                                                      */
/* 1 Zeichen in der Anzeige loeschen                                    */
/*                                                                      */
/************************************************************************/
void
bputbs(void)
{
  hputs("\010 \010");
}

/************************************************************************/
/*                                                                      */
/* Konsolenbefehle bearbeiten, auch ESC-Befehl im CCP                   */
/*                                                                      */
/************************************************************************/
void
hstcmd(MBHEAD *mbpoi)
{
  HOSTCMD        *cmdpoi;

  if (skipsp(&blicnt, &blipoi))
  {

/* Befehl in Befehlsliste suchen                                        */

    for (cmdpoi = hostcmdtab; cmdpoi->cmdfun != NULL; ++cmdpoi)
    {
      if (toupper(*blipoi) == *cmdpoi->cmdstr)
      {
        ++blipoi;
        --blicnt;
        break;
      }
    }
    if (cmdpoi->cmdfun != NULL)         /* Befehl gefunden              */
    {
      hstmbp = mbpoi;
      if (hstmbp == NULL)             /* Aufruf von Terminal / Hostmode */
        hstmbp = (MBHEAD *)allocb(ALLOC_MBHEAD);
      else
        tnb_ch = TRUE;                /* Aufruf vom CCP                 */
      hstmbp->type = L2MNIX;
      hstmbp->l4type = HMRINFO;

      (*cmdpoi->cmdfun) ();           /* Befehl ausfuehren              */

      if (mbpoi == NULL)              /* Aufruf von Terminal / Hostmode */
      {
        hstusr->noacti = ininat;      /* Timeout zuruecksetzen          */
        rwndmb(hstmbp);
        if (!ishmod)                  /* Terminalmode                   */
        {
          if (hstmbp->mbpc != 0)      /* Antwort vorhanden              */
          {
            hputmb(hstmbp);
            if (hstmbp->l4type != HMRINFO)
              hputs(" *\n");
            if (hstmbp->l4type == HMRFMSG)
              hputc('\007');
          }
        }
        else                            /* Hostmode                     */
        {
          while (hstmbp->mbpc > hstmbp->mbgc)
            hputc(getchr(hstmbp));
          if (hstmbp->l4type < HMRMONI) /* Meldung mit '\0' beenden     */
            hputc(0);
        }
        dealmb(hstmbp);
      }
      else                                    /* Aufruf vom CCP         */
        if (hstmbp->mbpc > hstmbp->mbgc)      /* wenn Antwort im Buffer */
          putchr('\r', hstmbp);               /* <CR> hinterher         */
      return;
    }
    if (mbpoi != NULL)
#ifdef SPEECH
      putstr(speech_message(247), hstmbp);
#else
      putstr("INVALID COMMAND\r", hstmbp);
#endif
    else
    {
      hmputr(HMRFMSG);
      if (!ishmod)
        hputs("* ");
      hputs("INVALID COMMAND: ");
      hputcc(*blipoi);
      if (!ishmod)
        hputs(" *\007\r");
      else
        hputc(0);
    }
    return;
  }
  if (ishmod)
    hmputr(0);
}

/************************************************************************/
/*                                                                      */
/* "status to channel"                                                  */
/*                                                                      */
/************************************************************************/
static void
sttoch(unsigned msg)
{
  MBHEAD     *mbp;
  CIRBLK     *cp;
  UID         uid;
  const char *vp;

  if (tnb_ch)                           /* keine Meldung fuer TNBs      */
    return;

  putprintf(mbp = (MBHEAD *)allocb(ALLOC_MBHEAD),
            "(%u) ", (unsigned)(hstusr - hstubl));
#ifdef SPEECH
  putstr(msg == L2MCONNT ? speech_message(244) : speech_message(245), mbp);
#else
  putstr(msg == L2MCONNT ? "CONNECTED to " : "DISCONNECTED fm ", mbp);
#endif

  if (hstusr->direction)
          putid(myid, mbp);
  else
  {
    putid(hstusr->call, mbp);
    if (msg == L2MCONNT)
    {
      uid = userpo->uid;
      switch (g_utyp(uid))
      {
        case L2_USER:
          putdil(((LNKBLK *)g_ulink(uid))->viaidl, mbp);
        break;

        case L4_USER:
          cp = g_ulink(uid);
          if ((!cmpid(cp->upnod, cp->downca))
              || (*(cp->upnodv) != NUL))
          {
            putstr(" via ", mbp);
            putid(cp->upnod, mbp);
            vp = cp->upnodv;
            while (*vp != NUL)
            {
              putchr(' ', mbp);
              putid(vp, mbp);
              vp += L2IDLEN;
            }
          }
        break;
      }
    }
  }

  mbp->btime = sys_time;
  mbp->type = msg;
  if (!ishmod)
  {
    relink((LEHEAD *)mbp, (LEHEAD *)statml.tail);
    stalin++;
  }
  else
  {
    relink((LEHEAD *)mbp, (LEHEAD *)hstusr->outbuf.tail);
    hstusr->outsta++;
  }
}

/************************************************************************/
/*                                                                      */
/* Hostmode-Kanal Connecten                                             */
/*                                                                      */
/************************************************************************/
void
hstcon(char direction)
{
  hstusr->direction = direction;
  sttoch(L2MCONNT);
  hstol7(L2MCONNT, hstusr);
  hstusr->conflg = 1;
  hstusr->noacti = ininat;

#ifdef HOSTMYCALL
  cpyid(hstusr->call, hostuserid);
#endif /* HOSTMYCAL */

  resptc(g_uid(hstusr, HOST_USER));
  numhsts++;
}

/************************************************************************/
/*                                                                      */
/* Host-Kanal disconnecten                                              */
/*                                                                      */
/************************************************************************/
void
hstdis(void)
{
  dealml((LEHEAD *)&hstusr->inbuf);
  dealml((LEHEAD *)&hstusr->outbuf);
  hstusr->inlin =
    hstusr->outlin =
    hstusr->outsta = 0;
  sttoch(L2MDISCF);
  hstol7(L2MDISCF, hstusr);
  resptc(g_uid(hstusr, HOST_USER));

  hstusr->conflg =
    hstusr->disflg = 0;
  cpyid(hstusr->call, hostid);

  numhsts--;
}

/************************************************************************/
/*                                                                      */
/*      Informationstransfer von Host nach Layer X                      */
/*      ------------------------------------------                      */
/*                                                                      */
/*      Solange noch empfangene Pakete vorhanden sind, werden diese     */
/*      an andere Layer durch Aufruf von fmlink() uebertragen. Bei ge-  */
/*      setztem Ueberfuellungskontroll-Flag (conctl == TRUE) wird die   */
/*      Uebertragung abgebrochen, wenn der andere Layer keine weiteren  */
/*      Daten mehr aufnehmen kann.                                      */
/*                                                                      */
/*      Nach erfolgter Uebertragung wird der No-Activity-Timer neu      */
/*      gesetzt.                                                        */
/*                                                                      */
/************************************************************************/
void
hstsen(BOOLEAN conctl)
{
  MBHEAD         *mbp;

  if ((hstusr - hstubl) >= MAXHST)
    return;
  while (hstusr->inlin != 0)
  {
    mbp = (MBHEAD *)hstusr->inbuf.head;
    mbp->l2link = (LNKBLK *)hstusr;
    mbp->type = HOST_USER;
    mbp->l2fflg = L2CPID;
    if (!fmlink(conctl, mbp))
      break;
    hstusr->noacti = ininat;
    --hstusr->inlin;
  }
}

/************************************************************************/
/*                                                                      */
/* Rufzeichen mit SSID an der Konsole anzeigen.                         */
/*                                                                      */
/************************************************************************/
void
hputid(char *id)
{
  WORD            ssid;
  WORD            i;
  char            ch;

  for (i = 0; i < L2CALEN; ++i)
    if ((ch = *id++ & 0xFF) != ' ')
      hputcc(ch);
  if ((ssid = (*id >> 1) & 0x0F) != 0)
    hprintf("-%d", ssid);
}

/************************************************************************/
/*                                                                      */
/* Zeichen an der Konsole anzeigen, Steuerzeichen mit "^" vorweg.       */
/*                                                                      */
/************************************************************************/
void
hputcc(char c)
{
  if (c >= ' ')
    hputc(c);
  else
  {
    hputc('^');
#ifdef __WIN32__
    hputc((char)(c + '@'));
#else
    hputc(c + '@');
#endif /* WIN32 */
  }
}

/************************************************************************/
/*                                                                      */
/* Uhrzeit aus Buffer an der Konsole anzeigen.                          */
/*                                                                      */
/************************************************************************/
void
hputbt(time_t * t)
{
  struct tm      *p;

  p = localtime(t);
  hprintf("%02u.%02u.%02u %02u:%02u:%02u",
          p->tm_mday, p->tm_mon + 1, p->tm_year % 100,
          p->tm_hour, p->tm_min, p->tm_sec);
}

/************************************************************************/
/*                                                                      */
/* String an der Konsole formatiert anzeigen.                           */
/*                                                                      */
/************************************************************************/
void
hprintf(const char *format,...)
{
  va_list         arg_ptr;
  char            str[256];

  va_start(arg_ptr, format);
  vsprintf(str, format, arg_ptr);
  va_end(arg_ptr);
  hputs(str);
}

/************************************************************************/
/*                                                                      */
/* Text auf die Console ausgeben, nicht unbedingt an das Host-Interface.*/
/* xprintf() wird fuer Status-Ausgaben genutzt, die den Hostmode durch- */
/* einander bringen koennten.                                           */
/* Der TNC3 muss xprintf() im Hostmode unterdruecken!                   */
/*                                                                      */
/************************************************************************/
void
xprintf(const char *format,...)
{
        va_list         arg_ptr;

  if (consfile == NULL)
    return;
  va_start(arg_ptr, format);
  vfprintf(consfile, format, arg_ptr);
  va_end(arg_ptr);
}

/************************************************************************/
/*                                                                      */
/* String an der Konsole anzeigen.                                      */
/*                                                                      */
/************************************************************************/
void
hputs(const char *str)
{
  while (*str)
    hputc(*str++);
}

/************************************************************************/
/*                                                                      */
/* Inhalt eines Buffers an der Konsole anzeigen, Steuerzeichen mit "^"  */
/* vorweg.                                                              */
/*                                                                      */
/************************************************************************/
void
hputmb(MBHEAD *mbp)
{
  UBYTE           c;

  if (tnb_ch)
  {
    cputmb(mbp);
    return;
  }
  while (mbp->mbgc < mbp->mbpc)
  {
    c = getchr(mbp);
    if (c >= ' ' || c == BELL || c == TAB || c == LF || c == CR)
      hputc(c);
    else
      hputcc(c);
  }
}

/************************************************************************/
/*                                                                      */
/* Inhalt eines Buffers in das Protokollfile fuer Batches schreiben,    */
/* Steuerzeichen mit "^" vorweg.                                        */
/*                                                                      */
/************************************************************************/
void
cputmb(MBHEAD *mbp)
{
  UBYTE           c;

  if (pro_file == NULL)
    return;
  while (mbp->mbgc < mbp->mbpc)
  {
    c = getchr(mbp);
    if (c >= ' ')
      fputc(c, pro_file);
    else
    {
      if (c == CR)
        fputs("\n", pro_file);
      else if (c != LF)
      {
        fputc('^', pro_file);
        fputc(c + '@', pro_file);
      }
    }
  }
}

/* End of src/l7host.c */
