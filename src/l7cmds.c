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
/* File src/l7cmds.c (maintained by: ???)                               */
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


static void put_pcnf(WORD, MBHEAD *);

#ifdef ALIASCMD
CMDALIAS *aliaslist;

/************************************************************************/
/* alias-management                                                     */
/*----------------------------------------------------------------------*/
void ccpalias(void)
{
  int i;
  char aliastmp[MAXALIASLEN + 1];
  CMDALIAS *ap = aliaslist, *ap2;
  MBHEAD *mbp;

  if (issyso())       /* only Sysop may use this */
  {
    mbp = putals("");

    if (clicnt == 0) /* no parameters given, then show all aliasses*/
    {
      if (ap != NULL)
      {
        /* print all elements in the list */
#ifdef SPEECH
        putstr(speech_message(217), mbp);
#else
        putstr("Currently defined aliasses:\r", mbp);
#endif

        for ( ; ap != NULL; ap = ap->next)
          putprintf(mbp, "%s -> %s\r", ap->alias, ap->cmd);
      }
      else
#ifdef SPEECH
        putstr(speech_message(218), mbp);
#else
        putstr("No aliasses defined\r", mbp);
#endif
    }
    else  /* handling if called with parameters */
    {
      /* read alias first */
      i = 0;
      while (clicnt > 0 && *clipoi != ' ' && i < MAXALIASLEN)
      {
        aliastmp[i++] = toupper(*clipoi++);
        clicnt--;
      }
      aliastmp[i] = '\0';

      /* check its length */
      if (clicnt > 0 && *clipoi != ' ')
      {
#ifdef SPEECH
        putprintf(mbp, speech_message(202), MAXALIASLEN);
#else
        putprintf(mbp, "Alias too long, max. %u characters.\r", MAXALIASLEN);
#endif
        prompt(mbp);
        seteom(mbp);
        return;
      }

      for (ap2 = NULL ; ap != NULL; ap = ap->next)
      {
        if (!strcmp(aliastmp, ap->alias))     /* aendern */
          break;

        ap2 = ap;
      }

      /* are there any more characters ? */
      if (skipsp(&clicnt, &clipoi))
      {
        if (clicnt > MAXALIASCMDLEN)
        {
#ifdef SPEECH
         putprintf(mbp, speech_message(203), MAXALIASCMDLEN);
#else
         putprintf(mbp, "Command too long, max. %u characters.\r", MAXALIASCMDLEN);
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }

        /*strupr(clipoi); wenns denn unbedingt noetig ist */

        if (ap != NULL)     /* aendern */
        {
          strcpy(ap->cmd, clipoi);
#ifdef SPEECH
          putstr(speech_message(219), mbp);
#else
          putstr("Alias stored.\r", mbp);
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }

        /* nicht gefunden, also neu */
        ap = (CMDALIAS *)calloc(1, sizeof(CMDALIAS));
        if (ap == NULL)
        {
#ifdef SPEECH
          putstr(speech_message(220), mbp);
#else
          putstr("Can't store, no memory.\r", mbp);
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }

        strcpy(ap->alias, aliastmp);
        strcpy(ap->cmd, clipoi);
        if (aliaslist == NULL)     /* erster Eintrag */
          aliaslist = ap;
        else                       /* in Liste */
          ap2->next = ap;

#ifdef SPEECH
        putstr(speech_message(219), mbp);
#else
        putstr("Alias stored.\r", mbp);
#endif
      }
      else /* no more parameters than alias-name */
      {
        if (ap != NULL)           /* loeschen */
        {
          if (ap == aliaslist)
            aliaslist = ap->next;
          else
            ap2->next = ap->next;

          free(ap);

#ifdef SPEECH
          putstr(speech_message(221), mbp);
#else
          putstr("Alias deleted.\r", mbp);
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }

        /* no match */
#ifdef SPEECH
        putstr(speech_message(222), mbp);
#else
        putstr("No such alias defined, can't delete.\r", mbp);
#endif
      }
    }
    prompt(mbp);
    seteom(mbp);
  }
  else
    invmsg();
}

/* Loeschen der gesamten Alias-Liste */
void clean_aliaslist(void)
{
  CMDALIAS *ap = aliaslist, *ap2;

  /* Aliaslist gefuellt ? */
  if (ap == NULL)
    return;

  /* alle Elemente der Liste durchgehen und loeschen */
  do
  {
    ap2 = ap->next;         /* Zeiger auf naechstes Alias retten       */
    free(ap);               /* aktuelles Alias loeschen                */
    ap = ap2;               /* geretteten Zeiger wiederherstellen      */
  } while (ap != NULL);     /* so lange die Liste nicht terminiert ist */
}

#endif

/************************************************************************/
/* Programm starten                                                     */
/*----------------------------------------------------------------------*/
void ccpstart(void)
{
  if (issyso()) {
    tnnexec((char *)clipoi);
#ifndef MC68302                     /* da gibs mehrere Fehlermeldungen */
#ifdef SPEECH
    putmsg(speech_message(237));
#else
    putmsg("Invalid program!\r");
#endif
#endif
  }
}

/************************************************************************/
/* Parameter anzeigen/aendern                                           */
/*----------------------------------------------------------------------*/
void ccppar(void)
{
  ccp_par("Parms:\r", partab, partablen);
}

/**************************************************************************/
/* RESET                                                                  */
/*------------------------------------------------------------------------*/
void ccpres(void)
{
  MBHEAD *mbp;
  WORD    port;

  if (issyso()) {
    if (strnicmp((char *)clipoi,"SYSTEM",6) == 0)
      HALT("sysop");
    if (getport(&clicnt, &clipoi, &port)) {
      mbp = putals("RESET Port ");
      putnum(port, mbp);
      putchr('\r', mbp);
      prompt(mbp);
      seteom(mbp);
      l1ctl(L1CRES, port);
    } else
#ifdef SPEECH
      putmsg(speech_message(193));
#else
      putmsg("Invalid Port!\r");
#endif
  } else
    invmsg();
}

/************************************************************************/
/* PROMPT                                                               */
/*----------------------------------------------------------------------*/
void ccpprompt(void)
{
  MBHEAD *mbp;
  WORD i;

  if (issyso() && *clipoi == '=') {
    if (clicnt > 0)
    {
      clicnt--;
      clipoi++;
    }
    for (i = 0; i < 79 && clicnt--; promptstr[i++] = *clipoi++);
    promptstr[i] = 0;
  }
  mbp = putals("Prompt: ");
  putstr(promptstr, mbp);
  putchr('\r', mbp);
  prompt(mbp);
  seteom(mbp);
}

#if defined(MC68302) || defined(__WIN32__)
/************************************************************************/
/* DELETE - Befehl (Dateien loeschen)                                   */
/*----------------------------------------------------------------------*/
void ccpdelete(void)
{
  MBHEAD *mbp;
  char file[128];

  if (issyso()) {
    strcpy(file, clipoi);
    mbp = putals(file);
    if (remove(file))
#ifdef SPEECH
      putstr(speech_message(223), mbp);
#else
      putstr(" not deleted!\r", mbp);
#endif
    else
    {
#ifndef __WIN32__
      compact();
#endif /* WIN32 */
#ifdef SPEECH
      putstr(speech_message(224), mbp);
#else
      putstr(" deleted.\r", mbp);
#endif
    }
    prompt(mbp);
    seteom(mbp);
  }
  else
    invmsg();
}

/************************************************************************/
/* copy                                                              */
/************************************************************************/
void ccpcopy(void)
{
  FILE *f1;
  FILE *f2;
  size_t n;
  char buf[512];
  char buf2[40];
  char file1[80];
  char file2[80];
  MBHEAD *mbp;

  if (issyso())
  {
    if (clicnt > 0)
    {
      skipsp(&clicnt, &clipoi);
      *file1 = *file2 = '\0';
      sscanf(clipoi, "%s %s", file1, file2);

      if (*file1 && *file2)
      {
        mbp = getmbp();
        strcpy(buf2, "OK\r");
        if ((f1 = fopen(file1, "rb")) != NULL)
        {
          if ((f2 = fopen(file2, "wb")) != NULL)
          {
            while ((n = fread(buf, 1, 512, f1)) > 0)
            {
              if (fwrite(buf, 1, n, f2) != n)
              {
                strcpy(buf2, "write error !\r");
                remove(file2);
                break;
              }
            }
            fclose(f1);
            fclose(f2);
#ifndef __WIN32__
            compact();
#endif /* WIN32 */
          }
          else
          {
#ifdef SPEECH
          sprintf(buf2, speech_message(204), file2);
#else
          sprintf(buf2, "can't create %s !\r", file2);
#endif
          }
        }
        else
        {
#ifdef SPEECH
          sprintf(buf2, speech_message(205), file1);
#else
          sprintf(buf2, "can't open %s !\r", file1);
#endif
        }
        putstr(buf2, mbp);
        prompt(mbp);
        seteom(mbp);
      }
    }
  }
  else
    invmsg();
}

/************************************************************************/
/* ccpdir                                                               */
/************************************************************************/
void ccpdir(void)
{
  char file[128];
  MBHEAD *mbp;
  struct ffblk fblock;
  int lastentry;
  struct tm *ftime;
  int einfach, cnt;

  einfach = FALSE;
  if (userpo->sysflg != 0) {            /* Benutzer ist Sysop           */
    strcpy(file, textpath);             /* Verzeichnis der Texte        */
    if (clicnt > 0) {                   /* Parameter folgen             */
      skipsp(&clicnt, &clipoi);         /* Nachfolgende Leerzeichen     */
      if (strncmp(strlwr(clipoi), "/w", 2) == 0) { /* Einfache Ausgabe  */
      nextspace(&clicnt, &clipoi);
      skipsp(&clicnt, &clipoi);         /* Nachfolgende Leerzeichen     */
      if (clicnt > 0)                   /* vorhandene Dateimaske        */
        strcat(file, clipoi);           /* an Pfad dranhaengen          */
      else                              /* sonst:                       */
        strcat(file, "*.*");            /* alles auswaehlen             */
      einfach = TRUE;
      }
      else {
        strcat(file, clipoi);
      }
    }
    else {
      strcat(file,"*.*");
    }
    mbp = getmbp();
    cnt = 0;
    lastentry = xfindfirst(file, &fblock, 0);
    while (!lastentry) {
      if (einfach) {
        putprintf(mbp,"%15s", fblock.ff_name);
        if (++cnt == 5) {
          putchr('\r', mbp);
          cnt = 0;
        }
      } else {
        ftime = localtime((const long *)&fblock.ff_ftime);
        putprintf(mbp,"%12s %8ld Bytes  %2.2d.%02d.%02d %2.2d:%02d:%02d\r",
            fblock.ff_name, fblock.ff_fsize,
            ftime->tm_mday, (ftime->tm_mon)+1, ftime->tm_year%100,
            ftime->tm_hour, ftime->tm_min, ftime->tm_sec);
      }
      lastentry = xfindnext(&fblock);
    }
    if (einfach) putstr("\r", mbp);
#ifndef __WIN32__
    compact();
#endif /* WIN32 */
#ifdef SPEECH
    putprintf(mbp,speech_message(206), coreleft());
#else
    putprintf(mbp,"%lu Bytes free\r", coreleft());
#endif
    prompt(mbp);
    seteom(mbp);
  }
  else
    invmsg();
}
#endif /* WIN32 */

#ifndef MC68302
/************************************************************************/
/* SHELL/DOS/TOS                                                        */
/*----------------------------------------------------------------------*/
void ccpshell(void)
{
  char sysline[MAXPATH+1];

  if (issyso())                         /* darf nur ein Sysop           */
  {
    strncpy(sysline, (char *)clipoi, MAXPATH);
    sysline[MAXPATH] = 0;
    if (tnnshell(sysline) == TRUE) return;      /* und ausfuehren       */
    /* tnnshell() liefert in sysline den Namen der temporaeren Datei,   */
    /* die die Ausgabe der Shell enthaelt.                              */
    if (*sysline) {
      userpo->fname = strdup(sysline);
      ccpread(sysline);                 /* Ergebnis dem User sagen      */
    }
    else
      invmsg();
  }
  else
    invmsg();
}
#endif

/* Commandotabelle fuer die Porteinstellung, alles was hier nicht erscheint,
   wird als Geraetename interpretiert und an l1attach() uebergeben. */
#define PO_DETACH 1
#define PO_NAME   2
#define PO_TXD    3
#define PO_MODE   4
#define PO_MAXF   5
#define PO_CTEXT  6
#define PO_SYSOP  7
#define PO_MH     8
#define PO_DAMA   9
#define PO_MAXC  10
#define PO_TAIL  11
#define PO_EAXMF 12
#define PO_EAXBH 13
#ifdef EXPERTPARAMETER
#define PO_PERS  14
#define PO_SLOT  15
#define PO_IRTT  16
#define PO_T2    17
#define PO_RETRY 18
#endif

#ifdef PORT_MANUELL
#define PO_PACL  19
#define PO_T3    20
#endif /* PORT_MANUELL */

#ifdef IPOLL_FRAME
#define PO_IPAC  21
#define PO_IRET  22
#endif /* IPOLL_FRAME */
#ifdef PORT_L2_CONNECT_TIME
#define PO_L2_CONNECT_TIME 23
#endif
#ifdef PORT_L2_CONNECT_RETRY
#define PO_L2_CONNECT_RETRY 24
#endif
#ifdef AUTOROUTING
#define PO_RAUTO 25
#endif /* AUTOROUTING */

PORTCMD portcmd[] = {    /* nicht aktivierte Pars werden ueberlesen */
  {"OFF",             PO_DETACH },
  {"NAME",            PO_NAME   },
  {"TXDELAY",         PO_TXD    },
  {"MODE",            PO_MODE   },
  {"MAXFRAME",        PO_MAXF   },
  {"CTEXT",           PO_CTEXT  },
  {"SYSOP",           PO_SYSOP  },
  {"MH",              PO_MH     },
  {"DAMA",            PO_DAMA   },
  {"MAXCON",          PO_MAXC   },
  {"TAILTIME",        PO_TAIL   },
  {"EAXMAXFR",        PO_EAXMF  },
  {"EAXMODE",         PO_EAXBH  },
#ifdef EXPERTPARAMETER
  {"PERSISTANCE",     PO_PERS   },
  {"SLOTTIME",        PO_SLOT   },
  {"IRTT",            PO_IRTT   },
  {"T2",              PO_T2     },
  {"RETRY",           PO_RETRY  },
#endif

#ifdef PORT_MANUELL
  {"PACLEN",          PO_PACL   },
  {"T3",              PO_T3     },
#endif /* PORT_MANUELL */

#ifdef IPOLL_FRAME
  {"IPACLEN",         PO_IPAC   },
  {"IRETRY",          PO_IRET   },
#endif /* IPOLL_FRAME */
#ifdef PORT_L2_CONNECT_TIME
  {"L2TIME",           PO_L2_CONNECT_TIME },
#endif
#ifdef PORT_L2_CONNECT_RETRY
  {"L2RETRY",          PO_L2_CONNECT_RETRY },
#endif
#ifdef AUTOROUTING
  {"L4AUTO",          PO_RAUTO  },
#endif
  {NULL,              0         }
};

/************************************************************************/
/*                                                                      */
/*----------------------------------------------------------------------*/
#ifdef MC68302
void hsbus_stat(MBHEAD *);
#endif

void
ccpport(void)
{
  MBHEAD    *mbp;                       /* message-buffer-pointer       */
  WORD       port;                      /* Port-Nummer (0..L2PNUM-1)    */
  char       buf[80];
  PORTCMD   *pc;
  char      *bps;
  WORD       bcs, found;
  ULONG      lbaud;
  L1MODETAB *mtp;
  PORTINFO  *p;
  WORD       begport = 0;
  WORD       endport = L2PNUM - 1;

  skipsp(&clicnt, &clipoi);

  if (*clipoi == '*' || *clipoi == '+')    /* Ausgabe der Autoparameter */
  {
    mbp = putals("Port Parameters:\r");
#ifdef PORT_MANUELL
    putstr("              ",mbp);
#else
    putstr("              TX-                     Max-  L2-           ", mbp);
#endif /* PORT_MANUELL */
#ifdef SETTAILTIME
    putstr("  Tail", mbp);
#endif
#ifdef USERMAXCON
    putstr(" Max", mbp);
#endif
#ifdef EAX25
    putstr(" EAX- EAX-", mbp);
#endif
#ifdef IPOLL_FRAME
    putstr(" IPOLL  IPOLL ", mbp);
#endif /* IPOLL_FRAME */
#ifdef PORT_L2_CONNECT_TIME
    putstr("INTERLINK-", mbp);
#endif
#ifdef AUTOROUTING
    putstr("  L4-", mbp);
#endif /* AUTOROUTING */
#ifdef PORT_MANUELL
    putstr("\r"
           "-#-Port-------",mbp);
#else
    putstr("\r"
           "-#-Port-------Delay-Pers--Slot--IRTT--Frame-Retry--Timer2-", mbp);
#endif /* PORT_MANUELL */
#ifdef SETTAILTIME
    putstr("--Time", mbp);
#endif
#ifdef USERMAXCON
    putstr("-Con", mbp);
#endif
#ifdef EAX25
    putstr("-MaxF-Mode", mbp);
#endif
#ifdef IPOLL_FRAME
    putstr("-Paclen-Retry", mbp);
#endif /* IPOLL_FRAME */
#ifdef PORT_L2_CONNECT_TIME
    putstr("-Time", mbp);
#endif
#ifdef PORT_L2_CONNECT_RETRY
    putstr("-Retry", mbp);
#endif
#ifdef AUTOROUTING
    putstr("-Auto", mbp);
#endif /* AUTOROUTING */
#ifdef PORT_MANUELL
    putstr("-Hardware", mbp);
#endif /* PORT_MANUELL */
    putchr(CR, mbp);

    for (port = 0, p = portpar; port < L2PNUM; p++, port++)
    {
      if (!portenabled(port))
        continue;
#ifdef PORT_MANUELL
      putprintf(mbp, "%2u:%-10s",port,p->name);
#else
      putprintf(mbp, "%2u:%-10s  %3u   %3u%c %3u%c  %4u%c   "
                     "%1u%c   %3u%c   %4u%c ",
                     port,
                     p->name,
                     p->txdelay,
                     p->persistance,
                     (p->l2autoparam & MODE_apers) ? 'a' : ' ',
                     p->slottime,
                     (p->l2autoparam & MODE_aslot) ? 'a' : ' ',
                     p->IRTT,
                     (p->l2autoparam & MODE_aIRTT) ? 'a' : ' ',
                     p->maxframe,
                     automaxframe(port) ? 'a' : ' ',
                     p->retry,
                     (p->l2autoparam & MODE_aretry) ? 'a' : ' ',
                     p->T2,
                     (p->l2autoparam & MODE_aT2) ? 'a' : ' ');
#endif /* PORT_MANUELL */
#ifdef SETTAILTIME
      putprintf(mbp, " %5u", p->tailtime);
#endif
#ifdef USERMAXCON
      putprintf(mbp, "  %2u", p->maxcon);
#endif
#ifdef EAX25
      putprintf(mbp, "  %2u   %2u", p->maxframe_eax, p->eax_behaviour);
#endif
#ifdef IPOLL_FRAME
      putprintf(mbp, "  %6u   %2u", p->ipoll_paclen, p->ipoll_retry);
#endif /* IPOLL_FRAME */
#ifdef PORT_L2_CONNECT_TIME
      putprintf(mbp, "  %5u", p->l2_connect_time);
#endif
#ifdef PORT_L2_CONNECT_RETRY
      putprintf(mbp, "  %2u", p->l2_connect_retry);
#endif
#ifdef AUTOROUTING
      putprintf(mbp, "   %2u", p->poAuto);
#endif /* AUTOROUTING */
#ifdef PORT_MANUELL
      putstr("   ",mbp);
      l1hwstr(port, mbp);
#endif /* PORT_MANUELL */
      putchr(CR, mbp);
    }

#if defined(EXPERTPARAMETER) && !defined(PORT_MANUELL)
    putstr("(the little \"a\" means, this value is calculated automatically)\r", mbp);
#endif
    prompt(mbp);
    seteom(mbp);
    return;
  }
#ifdef MC68302
  if (*clipoi == '?')       /* Ausgabe der Inquiry (vorerst nur hskiss) */
  {
    mbp = putals("Port-Inquiry:\r"
                 "Po. Software-Version                 TxD "
                 "Per Slt  Tail Dup DAMA    Baud Duo\r");
    hsbus_stat(mbp);
    prompt(mbp);
    seteom(mbp);
    return;
  }
#endif

  if (issyso() && clicnt)
  {
    port = nxtnum(&clicnt, &clipoi);
    if (port >= 0 && port < L2PNUM && skipsp(&clicnt, &clipoi))
    {
      p = &portpar[port];
      l1ctl(L1CCMD, port);                      /* HB9PAE/DB7KG         */
      do
      {
        *buf = NUL;
        bps = buf;
        if (skipsp(&clicnt, &clipoi))
        {
          while (*clipoi && isalnum(*clipoi))
          {
            *bps++ = toupper(*clipoi++);
            --clicnt;
          }
          *bps = NUL;
        }
        if (!*buf)
        {
          clicnt = 0;
          break;
        }
        bcs = (WORD)strlen(buf);

        for (pc = portcmd, found = 0;
             !found && pc->cmdstr != NULL;
             ++pc)
        {
          if (!strncmp(pc->cmdstr, buf, (size_t)bcs))
            found = pc->cmdpar;
        }

        if (found)
        {
          switch (found)
          {
            case PO_DETACH:
              l1detach(port);
              break;

            case PO_NAME:
              if (clicnt > 1)
              {
                clipoi++;
                clicnt--;
                if (sscanf((char *) clipoi,"%10s", buf) == 1)
                {
                  strcpy(p->name, buf);
                  bcs = (WORD)strlen(buf);
                  clipoi += bcs;
                  clicnt -= bcs;
                }
              }
              break;

            case PO_TXD:
              p->txdelay = getparam(&clicnt, &clipoi, 0, 255, 25);
              l1ctl(L1CCMD, port);
              autopar(port);
              break;

            case PO_MODE:
              if (clicnt > 1)
              {
                clipoi++;
                clicnt--;
                *buf = 0;

                if (isdigit(*clipoi))
                {
                  lbaud = nxtlong(&clicnt, &clipoi);
                  if (lbaud > 4915200L)
                    p->speed = 49152U;
                  else if (lbaud < 300L)
                    p->speed = 3;
                  else
                    p->speed = (UWORD) (lbaud/100L);
                }

                *buf = 0;
                if (*clipoi && *clipoi != ' ')
                {
                  sscanf((char *) clipoi,"%8s", buf);
                  strlwr(buf);
                  nextspace(&clicnt, &clipoi);
                }
                CLR_L1MODE(port);
                for (mtp = l1modetab; mtp->ch; mtp++)
                  if (strchr(buf, mtp->ch))
                    SET_L1MODE(port, mtp->mode);

                l1ctl(L1CCMD, port);
              }
              break;

            case PO_MAXF:
              if (clicnt > 1)
              {
                p->maxframe = getparam(&clicnt, &clipoi, 1, 7, 2);
                if (clicnt != 0 && toupper(*clipoi) == 'A')
                  p->l2mode |= MODE_am;
                else
                  p->l2mode &= ~MODE_am;
                nextspace(&clicnt, &clipoi);
              }
              break;

            case PO_CTEXT:
              if (getparam(&clicnt, &clipoi, 0, 1, 0))
                p->l2mode |= MODE_x;
              else
                p->l2mode &= ~MODE_x;
              break;

            case PO_SYSOP:
              if (getparam(&clicnt, &clipoi, 0, 1, 0))
                p->l2mode |= MODE_s;
              else
                p->l2mode &= ~MODE_s;
              break;

            case PO_MH:
              if (getparam(&clicnt, &clipoi, 0, 1, 0))
                p->l2mode |= MODE_h;
              else
                p->l2mode &= ~MODE_h;
              break;

            case PO_DAMA:
/* Es darf nur auf DAMA-Master oder DAMA-Slave geschaltet werden.       */
/* DAMA-Slave wird mit "DAMA S" (oder "DAMA=S") gewaehlt.               */
              if (clicnt > 1 && toupper(clipoi[1]) == 'S')
              {
                clipoi++;
                clicnt--;
#ifdef DAMASLAVE
                p->l2mode |= MODE_ds;           /* DAMA-Slave ON        */
                p->dch = DAMA_CH;               /* DAMA-Master OFF      */
                p->l2mode &= ~MODE_a;
#endif
                nextspace(&clicnt, &clipoi);
                break;
              }
#ifdef DAMASLAVE
              p->l2mode &= ~MODE_ds;            /* DAMA-Slave OFF       */
#endif
              p->dch = getparam(&clicnt, &clipoi, 0, DAMA_CH, DAMA_CH);
              if (p->dch > 0)
              {
                p->dch -= 1;
                p->l2mode |= MODE_a;
              }
              else
              {
                p->dch = DAMA_CH;
                p->l2mode &= ~MODE_a;
              }
              break;

            case PO_MAXC:
#ifdef USERMAXCON
              p->maxcon = getparam(&clicnt, &clipoi, 0, 15, 0);
#else
              nextspace(&clicnt, &clipoi);
#endif
              break;

            case PO_TAIL:
#ifdef SETTAILTIME
              p->tailtime = getparam(&clicnt, &clipoi, 0, 32700, TAILTIME);
              l1ctl(L1CCMD, port);
#else
              nextspace(&clicnt, &clipoi);
#endif
              break;

            case PO_EAXMF:
#ifdef EAX25
              p->maxframe_eax = getparam(&clicnt, &clipoi, 1, 32, 16);
#else
              nextspace(&clicnt, &clipoi);
#endif
              break;

            case PO_EAXBH:
#ifdef EAX25
              p->eax_behaviour = getparam(&clicnt, &clipoi, 0, 3, 1);
#else
              nextspace(&clicnt, &clipoi);
#endif
           break;

#ifdef EXPERTPARAMETER
            case PO_PERS:
              if (clicnt > 1)
              {
                p->persistance = getparam(&clicnt, &clipoi, 32, 255, 128);
                if (clicnt != 0 && toupper(*clipoi) == 'A')
                  p->l2autoparam |= MODE_apers;
                else
                  p->l2autoparam &= ~MODE_apers;
                nextspace(&clicnt, &clipoi);
              }
              l1ctl(L1CCMD, port);
              autopar(port);
              break;

            case PO_SLOT:
              if (clicnt > 1)
              {
                p->slottime = getparam(&clicnt, &clipoi, 0, 255, 10);
                if (clicnt != 0 && toupper(*clipoi) == 'A')
                  p->l2autoparam |= MODE_aslot;
                else
                  p->l2autoparam &= ~MODE_aslot;
                nextspace(&clicnt, &clipoi);
                l1ctl(L1CCMD, port);
              }
              autopar(port);
              break;

            case PO_IRTT:
              if (clicnt > 1)
              {
                p->IRTT = getparam(&clicnt, &clipoi, 1, 400, 200);
                if (clicnt != 0 && toupper(*clipoi) == 'A')
                  p->l2autoparam |= MODE_aIRTT;
                else
                  p->l2autoparam &= ~MODE_aIRTT;
                nextspace(&clicnt, &clipoi);
                l1ctl(L1CCMD, port);
              }
              autopar(port);
              break;

            case PO_T2:
              if (clicnt > 1)
              {
                p->T2 = getparam(&clicnt, &clipoi, 1, 300, 200);
                if (clicnt != 0 && toupper(*clipoi) == 'A')
                  p->l2autoparam |= MODE_aT2;
                else
                  p->l2autoparam &= ~MODE_aT2;
                nextspace(&clicnt, &clipoi);
                l1ctl(L1CCMD, port);
              }
              autopar(port);
              break;

            case PO_RETRY:
              if (clicnt > 1)
              {
                p->retry = getparam(&clicnt, &clipoi, 1, 127, 30);
                if (clicnt != 0 && toupper(*clipoi) == 'A')
                  p->l2autoparam |= MODE_aretry;
                else
                  p->l2autoparam &= ~MODE_aretry;
                nextspace(&clicnt, &clipoi);
                l1ctl(L1CCMD, port);
              }
              autopar(port);
              break;
#endif

#ifdef PORT_MANUELL
            /* Packetlaenge setzen. */
            case PO_PACL:
              p->paclen = getparam(&clicnt, &clipoi, 32, 256, 128);
              l1ctl(L1CCMD, port);
             break;

            /* T3-Timer setzen. */
            case PO_T3:
              p->T3 = getparam(&clicnt, &clipoi, 1, 32700, 18000);
              l1ctl(L1CCMD, port);
             break;
#endif /* PORT_MANUELL */

#ifdef IPOLL_FRAME
            case PO_IPAC:
              p->ipoll_paclen = getparam(&clicnt, &clipoi, 32, 256, 128);
              l1ctl(L1CCMD, port);
                          break;
            case PO_IRET:
              p->ipoll_retry = getparam(&clicnt, &clipoi, 1, 6, 3);
              l1ctl(L1CCMD, port);
                          break;
#endif /* IPOLL_FRAME */
#ifdef PORT_L2_CONNECT_TIME
            case PO_L2_CONNECT_TIME:
              p->l2_connect_time = getparam(&clicnt, &clipoi, 1, 500, 120);
              l1ctl(L1CCMD, port);
                          break;
#endif
#ifdef PORT_L2_CONNECT_RETRY
            case PO_L2_CONNECT_RETRY:
              p->l2_connect_retry = getparam(&clicnt, &clipoi, 1, 127, 3);
              l1ctl(L1CCMD, port);
                          break;
#endif
#ifdef AUTOROUTING
            case PO_RAUTO:
              p->poAuto = getparam(&clicnt, &clipoi, 0, 4, 0);
              l1ctl(L1CCMD, port);
              break;
#endif /* AUTOROUTING */
          } /* switch */
        }   /* if     */
        else
          l1attach(port, buf);
      } while (clicnt > 0);
       /* Portnummer merken. */
       begport = endport = port;
       autopar(begport = endport = port);       /* Auto-Parameter setzen */
    }
  }

  mbp = putals("Link-Interface Ports:\r");     /* Konfiguration zeigen  */
#ifdef PORT_MANUELL
  putstr("-#-Name----------Speed/Mode-Max-TXD-", mbp);
  putstr("PAC-",mbp);
  putstr("PERS-",mbp);
  putstr("SLOT-",mbp);
  putstr("IRTT-",mbp);
  putstr("--T2-",mbp);
  putstr("---T3-",mbp);
  putstr("-RET-",mbp);
  putstr("DA-C-S-M", mbp);
  putchr('\r', mbp);
#else
  putstr("-#-Name----------Speed/Mode-Max-TXD-DA"
         "----------------Hardware--\r", mbp);
#endif /* PORT_MANUELL */
  for (port = begport; port <= endport; port++)
    put_pcnf(port, mbp);

  prompt(mbp);
  seteom(mbp);
}

/*----------------------------------------------------------------------*/
static void
put_pcnf(WORD port, MBHEAD *mbp)
{
  char          mode[16], *cp;
  ULONG         baud;
  L1MODETAB    *mtp;
  PORTINFO     *p = &portpar[port];

  if (!portenabled(port))
    return;

  putprintf(mbp, "%2d %-10s", port, p->name);

  cp = mode;
  for (mtp = l1modetab; mtp->ch; mtp++)
    if (p->l1mode & mtp->mode)
      *cp++ = mtp->ch;
  *cp = 0;

  baud = ((ULONG)p->speed) * 100L;
  putprintf(mbp, " %8lu%-5s  %1u%c %3u ", baud, mode, p->maxframe,
                 automaxframe(port) ? 'a' : ' ', p->txdelay);

#ifdef PORT_MANUELL
  putprintf(mbp, "%3u "   ,p->paclen);
  putprintf(mbp, "%3u%c ",p->persistance, (p->l2autoparam & MODE_apers) ? 'a' : ' ');
  putprintf(mbp, "%3u%c ",p->slottime, (p->l2autoparam & MODE_aslot) ? 'a' : ' ');
  putprintf(mbp, "%3u%c ",p->IRTT, (p->l2autoparam & MODE_aIRTT) ? 'a' : ' ');
  putprintf(mbp, "%3u%c ",p->T2, (p->l2autoparam & MODE_aT2) ? 'a' : ' ');
  putprintf(mbp, "%5u ",p->T3);
  putprintf(mbp, "%3u%c ",p->retry, (p->l2autoparam & MODE_aretry) ? 'a' : ' ');
#endif /* PORT_MANUELL */

#ifdef DAMASLAVE
  if ((p->l2mode & MODE_ds) || (p->l2mode & MODE_a))
  {
    if (p->l2mode & MODE_ds)
      putstr(" s ", mbp);
    else
      if (p->l2mode & MODE_a)
        putprintf(mbp, "%2u ", p->dch + 1);
  }
#else
  if (p->l2mode & MODE_a)
    putprintf(mbp, "%2u ", p->dch + 1);
#endif
  else
    putstr("   ", mbp);
#ifdef PORT_MANUELL
  putstr(p->l2mode & MODE_x ? "C " : "  ", mbp);
  putstr(p->l2mode & MODE_s ? "S " : "  ", mbp);
  putstr(p->l2mode & MODE_h ? "M" : " ", mbp);
#else
  putstr(p->l2mode & MODE_x ? "CTEXT " : "      ", mbp);
  putstr(p->l2mode & MODE_s ? "SYSOP " : "      ", mbp);
  putstr(p->l2mode & MODE_h ? "MH "    : "   ",    mbp);

  l1hwstr(port, mbp);
#endif /* PORT_MANUELL */

  putchr(CR, mbp);
}

/************************************************************************/
/* HELP                                                                 */
/*----------------------------------------------------------------------*/
void ccphelp(void)                              /* HELP - Befehl        */
{
  struct ffblk fb;
  char tmp[9];
  MBHEAD *mbp;
  char file[80];
  char *cp;
  WORD i,j;
  WORD next = 0;
  WORD       cnt;
  char      *poi;
#ifndef MC68K
  BOOLEAN found = FALSE;
#endif

  poi = clipoi;
  cnt = clicnt;

  clipoi = clilin;
  clicnt = strlen(clilin);

#ifdef __WIN32__
  strupr(poi);
#endif

#ifndef MC68K
  if (issyso())                /* HELP.EXE fuer Sysops (OHS) suchen        */
    found = !do_file(sysopexepath);
  if (!found)                  /* HELP.EXE fuer User (OHU) suchen          */
    found = !do_file(userexepath);

  if (found)                   /* wenn es HELP.EXE gibt, sind wir fertig   */
          return;
#endif

  clipoi = poi;                /* die alte Hilfe                           */
  clicnt = cnt;

  if (clicnt == 0)             /* Kein Argument angegegen, kurzer HELP.TXT */
  {
    strcpy(file,textpath);
    strcat(file,"HELP.TXT");
    ccpread(file);
  }
  else
  {
    if (strnicmp((char *)clipoi, "INDEX", 5) == 0) /* Hilfe-Index ausgeben */
    {
      mbp = putals("HELP - Index:\r");
      strcpy (file,textpath);
      strcat (file,"*.HLP");
      if (xfindfirst(file, &fb, 0) == 0)
      {
        while (next == 0)
        {
          for (i = 0; i < 7; i++)
          {
            for (cp = fb.ff_name, j = 0; j < 8 && *cp != '.'; cp++, j++)
            {
              tmp[j] = *cp;
            }
            tmp[j] = '\0';
            putstr(tmp, mbp);
            for ( ; j < 10; j++)
              putchr(' ', mbp);
            if ((next = xfindnext(&fb)) != 0)
              break;
          }
          putchr('\r', mbp);
        }
      }

      prompt(mbp);
      seteom(mbp);
    }
    else
    { /* HILFE-Info fuer angegebenen Befehl suchen und ausgeben */

      for (cp = tmp, i = 0; clicnt-- && i < 8 && isalnum(*clipoi); ++i)
        *cp++ = (char) toupper(*clipoi++);
      if (i < 8)
        *cp++ = '*';
      *cp = '\0';

      if (userpo->sysflg != 0)
      {
        sprintf(file, "%s%s.SLP", textpath, tmp);
        if (i && !xfindfirst(file, &fb, 0)) {
           strcpy(file, textpath);
           strcat(file, fb.ff_name);
           ccpread(file);
           return;
         }
      }

      sprintf(file, "%s%s.HLP", textpath, tmp);
      if (i && !xfindfirst(file, &fb, 0)) {
        strcpy(file, textpath);
        strcat(file, fb.ff_name);
        ccpread(file);
        return;
      }

      mbp = putals("No HELP available for \'");
      putstr(strtok(file,"*."), mbp);
      putstr("\'.\r",mbp);
      prompt(mbp);
      seteom(mbp);
    }
  }
}

/************************************************************************/
/* SUSPEND - Befehl                                                     */
/*----------------------------------------------------------------------*/
void ccpsusp(void)
 {
  SUSPEND *suspoi;
  MBHEAD  *mbp;
  char     call[L2IDLEN];
  WORD     i;
  WORD     port;
  int      mode;

  if (!issyso()) {                  /* nur der Sysop darf SUSPEND       */
    invmsg();
    return;
  }

  if (skipsp(&clicnt, &clipoi)) {
    mode = *clipoi++;
    clicnt--;
    port = nxtnum(&clicnt, &clipoi);
    if (getcal(&clicnt, &clipoi, FALSE, call) == YES)
      for (suspoi = sustab, i = 0; i < MAXSUSPEND; suspoi++, i++) {
        if (   suspoi->port == port
            && cmpcal(suspoi->call, call)) /* erstmal austragen */
          suspoi->call[0] = '\0';
        if (   mode == '+'
            && suspoi->call[0] == '\0') {
          memcpy(suspoi->call, call, L2CALEN);
          suspoi->port = port;
#ifdef __WIN32__
          suspoi->okcount = (unsigned char)nxtnum(&clicnt,&clipoi);
#else
          suspoi->okcount = nxtnum(&clicnt,&clipoi);
#endif /* WIN32 */
          break;
        }
      }
  }

  mbp = putals("Suspended are\r");
  for (suspoi = sustab, i = 0; i < MAXSUSPEND; suspoi++, i++)
  {
    if (*suspoi->call != '\0') {
      putcal(suspoi->call, mbp);
#ifdef SPEECH
      putstr(speech_message(225), mbp);
#else
      putstr(" is restricted ", mbp);
#endif
      switch (suspoi->port) {
        case 253:
#ifdef SPEECH
            putstr(speech_message(226), mbp);
#else
            putstr("to Access denied\r", mbp);
#endif
                  break;
        case 254:
#ifdef SPEECH
            putstr(speech_message(227), mbp);
#else
            putstr("to level-2 access\r", mbp);
#endif
                  break;
        case 255:
#ifdef SPEECH
            putstr(speech_message(228), mbp);
#else
            putstr("to max ", mbp);
#endif
                  putnum(suspoi->okcount,mbp);
#ifdef SPEECH
                  putstr(speech_message(229), mbp);
#else
                  putstr(" simultanous connections\r", mbp);
#endif
                  break;
        default:
#ifdef SPEECH
            putstr(speech_message(230), mbp);
#else
            putstr("from using Port ", mbp);
#endif
                  putnum(suspoi->port, mbp);
                  putchr('\r', mbp);
      }
    }
  }
  prompt(mbp);
  seteom(mbp);
}

/**************************************************************************/
/* READB - Datei lesen (binaer)                                  DL1XAO   */
/*------------------------------------------------------------------------*/
#ifdef MC68K
LONG swaplong(LONG)0x4840;     /* SWAP.L D0    */
#endif

void
ccpreadb(void)
{
  char    buf[1024];
  char    fn[14];
  char   *cp;
  WORD    i;
  UWORD   crc;
  LONG    pos;
  LONG    len;
  MBHEAD *mbp;

  if (issyso() && userpo->convers == NULLCONNECTION)    /* darf er ?    */
  {
    mbp = getmbp();
    i = 0;
    for (cp = buf; clicnt-- && i < 128; i++)
      *cp++ = (char) toupper(*clipoi++);
    *cp = NUL;
    userpo->fp = (i == 0) ? NULL : xfopen(buf, "rb");

    if (userpo->fp != NULL)
    {
#ifndef MC68302
      getftime(fileno(userpo->fp), (struct ftime *)&pos);
#else
      getftime(buf, (struct ftime *)&pos);
#endif
#ifdef MC68K
      pos = swaplong(pos);
#endif
      if ((cp = strrchr(buf, FILE_SEP)) != NULL)/* Dateinamen isolieren */
        cp++;
      else
        cp = buf;
      strncpy(fn, cp, 12);
      fn[12] = NUL;
      len = 0L;
      crc = 0;                 /* CRC errechnen und Laenge bestimmen */
      while ((i = (WORD)fread(buf, 1, 1024, userpo->fp)) > 0)
      {
        len += i;
        for (cp = buf; i--; cp++)
          crc = crctab[crc >> 8] ^ ((crc << 8) | (UWORD)*cp);
      }

      if (len != 0)
      {
        putprintf(mbp, "\r#BIN#%ld#|%u#$%08lX#%s\r", len, crc, pos, fn);
        send_msg(TRUE, mbp);
        fseek(userpo->fp, 0L, SEEK_SET);
        userpo->status = US_SBIN;
        return;
      }

#ifdef SPEECH
      putstr(speech_message(231), mbp);
#else
      putstr("File has no length!\r", mbp);
#endif
    }
    else
#ifdef SPEECH
      putstr(speech_message(232), mbp);
#else
      putstr("File not found!\r", mbp);
#endif
    prompt(mbp);
    seteom(mbp);
  }
  else
    invmsg();
}

/************************************************************************/
/*                                                                      */
/* ESC Befehl  (nach einer Idee von DL9HCJ)                             */
/*             (Implementation: DL2LAY)                                 */
/*                                                                      */
/* Funktion : Ermoeglicht den Sysops den Remote-Zugriff auf Konsolen-   */
/*            ESC-Befehle                                               */
/* Syntax   : ESC <esc-befehl>                                          */
/*----------------------------------------------------------------------*/
void ccpesc(void)
{
  MBHEAD *mbp;
  char    esctab[] = "@CITVY";

  if (issyso())
   {
    if (clicnt != 0)
     {
      if (strchr(esctab, toupper(*clipoi)))
       {
        blipoi = (char *)clipoi;
        blicnt = clicnt;
        hstcmd(mbp = (MBHEAD *) allocb(ALLOC_MBHEAD));
        mbp->l2link = g_ulink(userpo->uid);
        mbp->type   = g_utyp(userpo->uid);
        prompt(mbp);
        seteom(mbp);
       }
      else
#ifdef SPEECH
        putmsg(speech_message(238));
#else
        putmsg("Invalid Hostcommand\r");
#endif
     }
   }
  else
    invmsg();
}

/************************************************/
/* L2-QSO killen                                */
/************************************************/
void ccpkill(void) {
  #define ALLPORTS 255

  MBHEAD  *mbp, *msg;
  WORD    what = 0, i;
  char    call[L2IDLEN];
  char    syscall[L2IDLEN];
#ifdef __WIN32__
  char    mask[MAXMASK];
  WORD    port = 128;
#else
  char    mask[MAXMASK];
  UBYTE   port = 128;
#endif
  LNKBLK  *savlp = lnkpoi;
  BOOLEAN kill = 0;
  UWORD   kill_zaehler = 0;

  if (issyso())
  {
    mbp = getmbp();
    cpyid (syscall,calofs(UPLINK, userpo->uid));

    /* schaun ob ein Port angegeben wurde, wenn ja, dann diesen merken */

    if ((*clipoi >= '0') && (*clipoi <= '9' ))
    {
#ifdef __WIN32__
      if ((port = (unsigned char)nxtnum(&clicnt, &clipoi)) <= L2PNUM) what = 3; /* Port angegeben ? */
#else
      if ((port = nxtnum(&clicnt, &clipoi)) <= L2PNUM) what = 3; /* Port angegeben ? */
#endif /* WIN32 */
      else port = 128;
    }

    if (*clipoi == '*')
    {
      if (*clipoi != 0) clipoi++;
      if (*clipoi != 0) clipoi++;
      port = ALLPORTS;         /* alle Ports killen ? */
      what = 3;
    }

    if ((port == 128) && (strlen ((char *)clipoi) > 3)) /* Call killen ? */
    {
      getcal(&clicnt, &clipoi, FALSE, call);
      what = 1;
      port = ALLPORTS;        /* Call auf allen Ports abwerfen */
    }

    if (what)
    {
        skipsp(&clicnt, &clipoi);

        /* nun die L2-Liste durchsehen und User ggfs abwerfen */

        for (lnkpoi = lnktbl, i = 0;i < LINKNMBR; ++lnkpoi, ++i)
        {
          if  (lnkpoi->state  &&
              ((lnkpoi->liport == port) || (port == ALLPORTS)))
          {
              switch (what)
              {
                  case 1 : kill = (!strncmp (call, lnkpoi->srcid,7) ||
                                   !strncmp (call, lnkpoi->dstid,7));
                           break;
                  case 2 : kill = (c6mtch(lnkpoi->srcid, mask) ||
                                   c6mtch(lnkpoi->dstid, mask) );
                           break;
                  case 3 : if (!cmpid (lnkpoi->dstid,syscall))
                                               /* nicht den Sysop killen !  */
                                  kill = TRUE; /* jeden auf dem Port */
                              else
                                  kill = FALSE;
              }
              if (kill)
              {
                /*dealml((LEHEAD *)&lnkpoi->sendil);*/ /* clear send liste */
                /*lnkpoi->tosend = 0;               */ /* alles weg        */
                /*siehe Hinweis in L2DAMA           */
                if (   *clipoi
                    && lnkpoi->state >= L2SIXFER)   /* Msg ?            */
                {
                  msg = (MBHEAD *) allocb(ALLOC_MBHEAD); /* Buffer besorgen */
                  msg->l2link = lnkpoi;
                  msg->type = 2;
#ifdef SPEECH
                  putprintf(msg, speech_message(207), clipoi);
#else
                  putprintf(msg, "\r*** Msg from Sysop: %s ***\r", clipoi);
#endif
                  seteom(msg);
                }
                kill_zaehler++;    /* zaehlen      */
#ifdef DEBUG
                newlnk();
#else
                dsclnk();          /* und DISC     */
#endif
              }
           }
         }

#ifdef L4KILL
         kill_zaehler += KillL4(call, clipoi);        /* ggf. L4-User killen. */
#endif /* L4KILL */
#ifdef L1TCPIP
         kill_zaehler += KillTCP(port, call, what);  /* ggf. TCP-User killen. */
#endif /* L1TCPIP */

         if (!kill_zaehler)
#ifdef SPEECH
           putstr(speech_message(233), mbp);
#else
           putstr("No L2-link found.\r", mbp);
#endif
         else
#ifdef SPEECH
          putprintf(mbp, speech_message(208),kill_zaehler);
#else
          putprintf(mbp, "%3u link(s) disconnected.\r",kill_zaehler);
#endif
    }
    else
    {
      putstr("Syntax: KILL [Port] [Msg]\r", mbp);
      putstr("        KILL [Call] [Msg]\r", mbp);
      putstr("        KILL *      [Msg]\r", mbp);
    }

    lnkpoi = savlp;  /* Pointer restaurieren */
    prompt(mbp);     /* Prompt */
    seteom(mbp);     /* senden.... */
  }
  else invmsg();
}

/*----------------------------------------------------------------------*/
/* Save Befehl                                                          */
/*                                                                      */
/* Funktion : speichert die Parameter-Datei, generiert eine Text-Datei, */
/*            die als TNNxxx.TNB benutzt werden kann (PARMS.TNB)        */
/* Syntax   : SPARAM                                                    */
/*----------------------------------------------------------------------*/
void ccpsave(void)
{
  MBHEAD    *mbp;

  if (!issyso())
  {
     invmsg();
     return;
  }
  save_stat();   /* Stat speichern */
  save_parms();  /* Parameter      */
  save_mh();     /* und MH-Liste   */
#ifdef USERPROFIL
  SaveProfil();  /* und Profil sichern. */
#endif /* USERPROFIL. */
#ifdef GRAPH
  save_graph();
#endif

  mbp = getmbp();
#ifdef SAVEPARAMFIX
#ifdef SPEECH
   putprintf(mbp,speech_message(209), cfgfile);
#else
   putprintf(mbp,"%s.TNB saved...\r", cfgfile);
#endif /* SPEECH */
#else /* SAVEPARAMFIX */
  putstr("PARMS.TNB saved...\r", mbp);
#endif /* SAVEPARAMFIX */
  prompt(mbp);
  seteom(mbp);
}

/**************************************************************************/
/*                                                                        */
/* TRACE-Befehl                                                           */
/*                                                                        */
/* Funktion : Einloggen zur Systemueberwachung.                           */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ccptrace(void)
{
  MBHEAD  *mbp;
#ifdef USER_MONITOR
  PTCENT  *ptcp;
#endif /* USER_MONIOR */

  /* Bestehendes Monitoring wird ausgeschaltet */
  if (userpo->monitor) {
    moncmd(NULL, userpo->monitor, "N", 1); /* Monitor abschalten */
    dealoc((MBHEAD *)userpo->monitor);
    userpo->monitor = NULL;
  }
  userpo->auditlevel = 0;

#ifndef USER_MONITOR
  if (issyso()) {
    /* Audit-Level und Monitor setzen */
    userpo->auditlevel = nxtnum(&clicnt, &clipoi);
#else
  /* Sysop, */
  if (issyso())
    /* darf Tracen, */
    userpo->auditlevel = nxtnum(&clicnt, &clipoi);
  else
    /* aber User nicht ! */
    userpo->auditlevel = 0;

  /* Nur wenn Monitor ausgeschalten ist. */
  if (!userpo->monitor)
  {
#endif /* USER_MONITOR */
    if (skipsp(&clicnt, &clipoi)) {
      userpo->monitor = (MONBUF *)allocb(ALLOC_MONBUF);
      moncmd(NULL, userpo->monitor, clipoi, clicnt);
      if (!userpo->monitor->Mpar) { /* er ist nicht angegangen */
        dealoc((MBHEAD *)userpo->monitor);
        userpo->monitor = NULL;
      }
    }

    /* ... zur Kontrolle anzeigen */
    mbp = putals("Trace");
    if (userpo->auditlevel)
      putlong(userpo->auditlevel, FALSE, mbp);
    else
      putstr(" is off", mbp);

    if (userpo->monitor) /* Monitor-Status zeigen */
#ifdef USER_MONITOR
    {
      /* Sysop/User als Trace markieren. */
      userpo->status = US_TRAC;
      /* Die User Pid ins Patchcord Liste uebergeben */
      ptcp = ptctab + userpo->uid;
      /* User eigene PID ins Partner P_PID uebergeben */
      ptcp->p_uid = userpo->uid;
      /* wird TRACE beendet, wechseln wir auf CCP um  */
      ptcp->recflg = TRUE;
      moncmd(mbp, userpo->monitor, "", 0);
    }
    else
      {
        /* Vom TRACE-Modus in den CCP wechseln */
        userpo->status = US_CCP;
        /* User PID an die Patchcord Liste uebergeben */
        ptcp = ptctab + userpo->uid;
        /* Partner PID -> NO_UID uebergeben */
        ptcp->p_uid = NO_UID;
        /* Portangabe auf NUL setzen */
        putchr('\r', mbp);
      }
#else
      moncmd(mbp, userpo->monitor, "", 0);
    else
      putchr('\r', mbp);
#endif /* USER_MONITOR */
#if MAX_TRACE_LEVEL < 9
    if (userpo->auditlevel > MAX_TRACE_LEVEL)
      putprintf(mbp, "Warning: max. trace level of this TNN version is %d\r",
                     MAX_TRACE_LEVEL);
#endif
    prompt(mbp);
    seteom(mbp);     /* senden.... */
  } else
    invmsg();
}

/************************************************************************/
/*                                                                      */
/* RUNBATCH                                                             */
/*                                                                      */
/* Funktion : Batch ausfuehren (nur als Sysop)                          */
/*                                                                      */
/*----------------------------------------------------------------------*/
void ccprun(void)
{
  if (issyso()) {                   /* nur der Sysop darf               */
    if (skipsp(&clicnt, &clipoi))
#ifdef SPEECH
        putmsg(runbatch((char*)clipoi) ? "OK\r" : speech_message(232));
#else
        putmsg(runbatch((char*)clipoi) ? "OK\r" : "File not found!\r");
#endif
    else
      putmsg("Syntax: RUNBATCH FILENAME.TNB\r");
  } else
    invmsg();
}

/**************************************************************************/
/*                                                                      */
/* DCD                                                                  */
/*                                                                      */
/* Funktion : Status der DCD pro Port anzeigen (fuer alle)              */
/*                                                                      */
/*----------------------------------------------------------------------*/
void ccpdcd(void)
{
  MBHEAD  *mbp;
  int      port;
  PORTINFO *p;
  int      dcd;

  mbp = putals("Data carrier detect:\r");
  for (port = 0; port < L2PNUM; port++) {
    putprintf(mbp, "P%02u", port);
    if (port <= 15) putchr(' ', mbp);
  }
  putchr('\r', mbp);
  for (port = 0, p = portpar; port < L2PNUM; p++, port++) {
    mbp->l4time = mbp->mbpc;
    if (portenabled(port)) {
      if ((dcd = iscd(port)) & DCDFLAG)
            putstr(dcd & RXBFLAG ? "R" : "r", mbp);
      if (dcd & PTTFLAG)
            putstr(dcd & TXBFLAG ? "T" : "t", mbp);
    } else
      putstr("OFF", mbp);
    if (port <= 15) putspa(4, mbp);
  }
  putchr('\r', mbp);
  prompt(mbp);
  seteom(mbp);
}
#ifdef BUFFER_DEBUG
void ccpbuf(void)
{
  MBHEAD          *mbp;
  MAX_BUFFER huge *actbp;
  ULONG            i, n;
  ULONG            used[ALLOC_MAXELEMENTE + 1];

  mbp = putals("Buffer Usage:\r");
  for (i = 0; i < ALLOC_MAXELEMENTE + 1; i++)
    used[i] = 0;
  actbp = (MAX_BUFFER *)RAMBOT;
  n = (ULONG)((RAMTOP - RAMBOT) / sizeof(MAX_BUFFER));
#ifdef SPEECH
  putprintf(mbp, speech_message(213), n, nmbfre_max,
            sizeof(MAX_BUFFER));
#else
  putprintf(mbp, "\rAllBuffer : %lu (%lu) a %lu Bytes\r", n, nmbfre_max,
            sizeof(MAX_BUFFER));
#endif

  while (n--)
  {
    if ((UBYTE) ((USRBLK *)actbp)->owner >= ALLOC_MAXELEMENTE)
    {
      used[ALLOC_MAXELEMENTE]++;
#ifdef SPEECH
      putprintf(mbp,speech_message(214), n,
      (UBYTE) ((USRBLK *)actbp)->owner);
#else
      putprintf(mbp,"Wrong Owner: Buffer:%lu Wert:%d\r", n,
      (UBYTE) ((USRBLK *)actbp)->owner);
#endif
    }
    else
      used[(UBYTE) ((USRBLK *)actbp)->owner]++;
      actbp++;
  }
#ifdef SPEECH
  putprintf(mbp, speech_message(215), used[ALLOC_NO_OWNER], nmbfre);
#else
  putprintf(mbp, "FreeBuffer: %lu (%lu)\r", used[ALLOC_NO_OWNER], nmbfre);
#endif

  putprintf(mbp, "LEHEAD : %lu\r", used[ALLOC_LEHEAD]);
  putprintf(mbp, "MBHEAD : %lu\r", used[ALLOC_MBHEAD]);
  putprintf(mbp, "USRBLK1: %lu\r", used[ALLOC_USRBLK1]);
  putprintf(mbp, "USRBLK2: %lu\r", used[ALLOC_USRBLK2]);
  putprintf(mbp, "L2LINK : %lu\r", used[ALLOC_L2LINK]);
  putprintf(mbp, "MB     : %lu\r", used[ALLOC_MB]);
  putprintf(mbp, "MONBUF : %lu\r", used[ALLOC_MONBUF]);
  putprintf(mbp, "CQBUF  : %lu\r", used[ALLOC_CQBUF]);
  putprintf(mbp, "IPROUTE: %lu\r", used[ALLOC_IP_ROUTE]);
  putprintf(mbp, "ARPTAB : %lu\r", used[ALLOC_ARP_TAB]);
  putprintf(mbp, "MHEARD : %lu\r", used[ALLOC_MHEARD]);
  putprintf(mbp, "PACSAT : %lu\r", used[ALLOC_PACSATBLK]);
#ifdef USERPROFIL
  putprintf(mbp, "USEPROF: %lu\r", used[ALLOC_USEPROF]);
#endif /* USERPROFIL. */
#ifdef TCP_STACK
  putprintf(mbp, "TCPSTA : %lu\r", used[ALLOC_TCPSTACK]);
#endif /* TCP_STACK */
#ifdef L1TCPIP
  putprintf(mbp, "TCPIP  : %lu\r", used[ALLOC_L1TCPIP]);
#endif /* L1TCPIP */
#ifdef L1HTTPD
  putprintf(mbp, "HTTPDRX: %lu\r", used[ALLOC_L1HTTPD_RX]);
  putprintf(mbp, "HTTPDTX: %lu\r", used[ALLOC_L1HTTPD_TX]);
#endif /* L1HTTPD */
  putprintf(mbp, "INPOPT : %lu\r", used[ALLOC_INPOPT]);
  if (used[0] != 0L)
    putprintf(mbp, "???    : %lu\r", used[0]);
#ifdef SPEECH
  putprintf(mbp, speech_message(216), used[ALLOC_MAXELEMENTE]);
#else
  putprintf(mbp, "Errors : %lu\r", used[ALLOC_MAXELEMENTE]);
#endif
  prompt(mbp);
  seteom(mbp);
}
#endif

/* End of src/l7cmds.c */
