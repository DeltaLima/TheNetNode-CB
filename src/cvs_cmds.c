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
/*    *****                      *****     Software                    */
/*                                                                      */
/* File src/cvs_cmds.c (maintained by: DL1XAO)                          */
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

/*
 * $Log$
 */

/*
 * This is Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@hpfcmdd.fc.hp.com>
 *
 * Modifications by Fred Baumgarten <dc6iq@insl1.etec.uni-karlsruhe.de>
 * $Revision: 3.12 $$Date: 1996/03/03 10:09:47 $
 *
 * Modifications by Oliver Kern <daa531@gmx.de>
 * $Revision: 3.13 $$Date: 17.08.2004 22:22 $
 */

/* zusammengefuehrtes USER.C und HOST.C, damit alle Kommandos samt
   Kommandotabelle mit statischen Funktionen auskommen                    */

#include "cvs_cmds.h"
#ifdef PPCONVERS

/*---------------------------------------------------------------------------*/

#ifdef CONVNICK
/* Den Nickname und/oder nur das Call ausgeben */
void makeName(CONNECTION* pCon, char* pBuffer)
{
    memset(pBuffer, 0, sizeof(pBuffer));

    if (pCon->nickname[0] != '\0')
      sprintf(pBuffer, "%s:%s", pCon->name, pCon->nickname);
    else
      sprintf(pBuffer, "%s", pCon->name);
}
#endif

void process_input(CONNECTION *cp)
{
  char *arg;
  WORD arglen;
  CMDTABLE *cmdp;
  CHANNEL *ch;
  CONNECTION *c;
#ifdef CONVNICK
  char buf[2 * NAMESIZE + 2];   /* Name + ":" + Nickname + 1 */
#endif

  clear_locks();
  cp->locked = 1;
  ts2();

  if (cp->type == CT_USER) {
    arg = convertin(cp->charset_in, cnvinbuf);
    if (arg != cnvinbuf)
      strcpy(cnvinbuf, arg);
  }

#ifdef USERPROFIL
  if (userpo->status == US_UPWD)               /* User ist im Passwort-Modus. */
  {
    cp->type = US_UPWD;                             /* Passwort-Modus setzen. */

    if (!strncmp(cnvinbuf, "/NAME", 5))/* Beim 1.Login Kanal ermitteln/setzen */
    {
      char *Channel;

      Channel = strrchr(cnvinbuf, ' ');                   /* Kanal ermitteln. */
      cp->channel = atoi(Channel);                           /* Kanal setzen. */
      return;
    }

    strncpy(clipoi, cnvinbuf, 6);         /* Passwortstring vom User sichern. */
    clicnt = 5;                                             /* laenge setzen. */

    if (CheckPasswd())                     /* Pruefe Passwortstring vom User. */
    {
      SendPasswdStringProfil();                  /* String war nicht korrekt, */
      return;                         /* neuen Passwortstring an User senden. */
    }
                                      /* Passwortstring vom User ist korrekt. */
    userpo->status = US_CCP;                /* Markiere, kein Passwort-Modus. */
    cp->type = CT_UNKNOWN;                         /* Typ ist noch ungekannt. */

    sprintf(cnvinbuf, "/NAME %s %d", cp->name, cp->channel);
  }
#endif /* USERPROFIL */

  cnvinbuf[2000] = '\0';             /* Zulange Antworten vermeiden */

#ifdef L1IRC
  if (cp->IrcMode == TRUE)
  {
    ProcessIrcInput(cnvinbuf, cp);
    return;
  }
#endif /* L1IRC */

  if (*cnvinbuf == '/') {
    if (!strncmp(cnvinbuf, "/\377\200", 3) && tolower(*(cnvinbuf+3)) != 'c')
      send_proto("cmds", "RX fm %s %s", cp->name, cnvinbuf+3);
    arglen = (WORD)strlen(arg = getarg(cnvinbuf + 1, GET_NXTLC));
    for (cmdp = cmdtable; cmdp->name; cmdp++)
      if (!strncmp(cmdp->name, arg, arglen)) {
        if (cmdp->states & (1 << cp->type)) {
          (*cmdp->fnc)(cp);
          return;
        }
      }
    if (cp->type == CT_USER) {
      if (!strncmp(arg, "\377\200", 2))      /* unangemeldete Hosts */
        return;                              /* werden ignoriert    */
#ifdef SPEECH
      appenddirect(cp, speech_message(28));
#else
      appenddirect(cp, "*** Unknown command '/");
#endif
      appendstring(cp, arg);
#ifdef SPEECH
      appenddirect(cp, speech_message(29));
#else
      appenddirect(cp, "'. Type /HELP for help.\r");
#endif
      appendprompt(cp, 0);
    }
    else if (cp->type == CT_HOST && !strncmp(arg, "\377\200", 2)) {
      if (strncmp("\377\200host", arg, 6)) {
        *strchr(cnvinbuf, 0) = ' '; /* durch getarg verursachte \0 loeschen */
        h_unknown_command(cp);
      }
    }
    return;
  }

  if (cp->type == CT_USER) {
      if (cp->away)
#ifdef SPEECH
          appenddirect(cp, speech_message(30));
#else
          appenddirect(cp, "*** You are away, aren't you ? :-)\r");
#endif
    if (cp->query[0] == '\0') {
      for (ch = channels; ch; ch = ch->next) {
         if (ch->chan == cp->channel) break;
      }
      if (cp->operator || cp->channelop || !(ch->flags & M_CHAN_M))
      {
#ifdef CONVNICK
        makeName(cp, buf);
        send_msg_to_channel(buf, cp->channel, cnvinbuf);
#else
        send_msg_to_channel(cp->name, cp->channel, cnvinbuf);
#endif
      } else {
#ifdef SPEECH
          appenddirect(cp, speech_message(31));
#else
          appenddirect(cp, "*** This is a moderated channel. Only channel operators may write.\r");
#endif
      }
    } else {
      for (c = connections; c; c = c->next)
#ifdef CONVNICK
        /* Typ User. */
        if (    c->type == CT_USER
            /* Callvergleich */
            && (!strcasecmp(c->name, cp->query)
            /* Nicknamevergleich */
            ||  !strcasecmp(c->nickname, cp->query)))
#else
        if (c->type == CT_USER && !strcmp(c->name, cp->query))
#endif
          break;
      if (!c) {
#ifdef SPEECH
        appenddirect(cp, speech_message(32));
#else
        appenddirect(cp, "*** Queried user left convers.\r");
#endif
      } else {
#ifdef CONV_NICK
        makeName(cp, buf);
        send_msg_to_user(buf, cp->query, cnvinbuf);
#else
        send_msg_to_user(cp->name, cp->query, cnvinbuf);
#endif
      }
    }
  }
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/
/* User-Kommandos                                                            */
/*---------------------------------------------------------------------------*/

static void all_command(CONNECTION *cp)
{
  CHANNEL *ch;
  char *s;
#ifdef CONVNICK
  char buf[2 * NAMESIZE + 2];   /* Name + ":" + Nickname + 1 */
#endif

  s = getarg(NULL, GET_ALL);

  for (ch = channels; ch; ch = ch->next) {
    if (ch->chan == cp->channel) break;
  }
  if (cp->operator || cp->channelop || !(ch->flags & M_CHAN_M)) {
    if (*s) {
#ifdef CONVNICK
      makeName(cp, buf);
      send_msg_to_channel(buf, cp->channel, s);
#else
      send_msg_to_channel(cp->name, cp->channel, s);
#endif
    }
  } else {
#ifdef SPEECH
      appenddirect(cp, speech_message(31));
#else
      appenddirect(cp, "*** This is a moderated channel. Only channel operators may write.\r");
#endif
  }
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void away_command(CONNECTION *cp)
{
  char *s;
  char buffer[128];
  CONNECTION *p;

  s = getarg(NULL, GET_ALL);
  if (*s || cp->away) {
    if (*s)
#ifdef SPEECH
      sprintf(buffer, speech_message(33), timestamp);
#else
      sprintf(buffer, "%sYou are marked as being away.\r", timestamp);
#endif
    else
#ifdef SPEECH
      sprintf(buffer, speech_message(34), timestamp);
#else
      sprintf(buffer, "%sYou are no longer marked as being away.\r", timestamp);
#endif
    for (p = connections; p; p = p->next) {
      if (p->type == CT_USER && !p->via && !Strcmp(p->name, cp->name)) {
        setstring(&(p->away), s, 256);
        p->atime = currtime;
        p->locked = 1;
      }
    }
#ifndef CONVNICK
    send_awaymsg(cp->name, myhostname, currtime, s);
#else
    send_awaymsg(cp->name, cp->nickname, myhostname, currtime, s);
#endif /* CONVNICK */
  } else
#ifdef SPEECH
      sprintf(buffer, speech_message(35), timestamp);
#else
      sprintf(buffer, "%sActually you were marked as being here :-)\r", timestamp);
#endif

  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void beep_command(CONNECTION *cp)
{
  char buffer[32], *c;

  if (cp->prompt[3] == '\0') {
    cp->prompt[3] = '\007';
#ifdef SPEECH
  c = speech_message(83);
#else
    c = "en";
#endif
  }
  else {
    cp->prompt[3] = '\0';
#ifdef SPEECH
  c = speech_message(84);
#else
    c = "dis";
#endif
  }
#ifdef SPEECH
  sprintf(buffer, speech_message(36), c);
#else
  sprintf(buffer, "*** Beep mode %sabled\r", c);
#endif
  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void bye_command2(CONNECTION *cp, char *reason)
{
  CONNECTION *p;
  WORD users_left;
  CLIST *cl = NULLCLIST;
  WORD channel;

  switch (cp->type) {
  case CT_UNKNOWN:
    cp->type = CT_CLOSED;
    break;

#ifdef USERPROFIL
  case US_UPWD:                               /* User ist im Passwort-Modus. */
#endif /* USERPROFIL */
  case CT_USER:
    cp->type = CT_CLOSED;
    cl = cp->chan_list;
    while (cl) {
      users_left = count_user(cl->channel);
      if (!users_left) destroy_channel(cl->channel);
      clear_locks();
#ifndef CONVNICK
      send_user_change_msg(cp->name, cp->host, cl->channel, -1, reason, currtime);
#else
      send_user_change_msg(cp->name, cp->nickname, cp->host, cl->channel, -1, reason, currtime);
#endif /* CONVNICK */
      cp->chan_list = cl->next;
      free (cl);
      cl = cp->chan_list;
    }
    break;
  case CT_HOST:
    cp->type = CT_CLOSED;
    update_permlinks(cp->name, NULLCONNECTION, 0);
    for (p = connections; p; p = p->next)
      if (p->via == cp) {
        p->type = CT_CLOSED;
        clear_locks();
        channel = p->channel;
#ifndef CONVNICK
        send_user_change_msg(p->name, p->host, p->channel, -1, reason, currtime);
#else
        send_user_change_msg(p->name, p->nickname, p->host, p->channel, -1, reason, currtime);
#endif /* CONVNICK */
        users_left = count_user(channel);
        if (!users_left) destroy_channel(channel);
      }
    break;
  case CT_CLOSED:
    break;
  }
}

/*---------------------------------------------------------------------------*/

void bye_command(CONNECTION *cp)
{
  char *s;
  char buffer[256];

  s = getarg(NULL, GET_ALL);
  if (!s || !*s) {
    bye_command2(cp, "/quit");
  } else {
    sprintf(buffer, "\"%.252s\"", s);
    bye_command2(cp, buffer);
  }
}

/*---------------------------------------------------------------------------*/

static CHANNEL *ins_channel(WORD chan)
{
  CHANNEL *ch, *ch1;

  ch = (CHANNEL *) calloc(1, sizeof(CHANNEL));

  if (ch) {
    ch->chan = chan;

    if (!channels || channels->chan > chan) {
      ch->next = channels;
      channels = ch;
    } else {
      ch1 = channels;
      while (ch1->next) {
        if (ch1->next->chan > chan) {
          ch->next = ch1->next;
          ch1->next = ch;
          return(ch);
        }
        ch1 = ch1->next;
      }
      if (!ch1->next) {
        ch->next = ch1->next;
        ch1->next = ch;
      }
    }
  }
  return(ch);
}

/*---------------------------------------------------------------------------*/

static BOOLEAN has_ChOp(WORD chan)
{
  CONNECTION *p;
  CLIST *cl;

  for (p = connections; p; p = p->next) {
    if (p->type == CT_USER) {
      if (p->channel == chan && p->channelop)
        return(1);

      for (cl = p->chan_list; cl; cl = cl->next)
        if (cl->channel == chan && cl->channelop)
        return(1);
    }
  }
  return(0);
}

/*---------------------------------------------------------------------------*/

static void channel_command(CONNECTION *cp)
{
  char *s;
  char buffer[256];
  WORD newchannel;
  LONG nc;
  CHANNEL *ch;
  CONNECTION *p;
  WORD printit, first;
  CLIST *cl;

  s = getarg(NULL, GET_NXTLC);

  if (!*s) {
#ifdef SPEECH
      sprintf(buffer, speech_message(37),
            timestamp, cp->channel, count_user(cp->channel));
#else
      sprintf(buffer, "%sYou are talking to channel %d. There are %d users.\r",
            timestamp, cp->channel, count_user(cp->channel));
#endif
    appenddirect(cp, buffer);
    for (ch = channels; ch; ch = ch->next) {
      if (ch->chan == cp->channel) break;
    }

    if (ch->topic)
    {
#ifdef CONV_TOPIC
#ifdef SPEECH
      sprintf(buffer, speech_message(38), ch->name, ts(ch->time));
#else
      sprintf(buffer, "*** current Topic by: %s (%s):\r               "
                    , ch->name
                    , ts(ch->time));
#endif /* SPEECH */
#else
      appenddirect(cp, "*** current Topic is:\r               ");
#endif /* CONV_TOPIC */
      appendstring(cp, ch->topic);
      appenddirect(cp, "\r");
    }
    printit = 0;
    first = 1;
    for (cl = cp->chan_list; cl; cl = cl->next) {
      if (cl->channel != cp->channel) {
        if (first) {
          first = 0;
#ifdef SPEECH
          appenddirect(cp, speech_message(39));
#else
          appenddirect(cp, "*** Also attached:");
#endif
        } else {
            appenddirect(cp, ",");
        }
        printit = count_user(cl->channel);
        if (printit == 1) {
#ifdef SPEECH
            sprintf(buffer, speech_message(40), cl->channel);
#else
            sprintf(buffer, " channel %d (alone)", cl->channel);
#endif
        } else {
#ifdef SPEECH
          sprintf(buffer, speech_message(41), cl->channel, printit);
#else
          sprintf(buffer, " channel %d (%d users)", cl->channel, printit);
#endif
        }
        appenddirect(cp, buffer);
      }
    }
    if (printit) appenddirect(cp, ".\r");
    appendprompt(cp, 0);
    return;
  }
  nc = atol(s); newchannel = (WORD)nc;
  if (nc < 0L || nc > MAXCHANNEL) {
#ifdef SPEECH
    sprintf(buffer, speech_message(42), timestamp, MAXCHANNEL);
#else
    sprintf(buffer, "%sChannel numbers must be in the range 0..%d.\r", timestamp, MAXCHANNEL);
#endif
    appenddirect(cp, buffer);
    appendprompt(cp, 0);
    return;
  }
  if (newchannel == cp->channel) {
#ifdef SPEECH
      sprintf(buffer, speech_message(43), timestamp, cp->channel);
#else
      sprintf(buffer, "%sChannel %d is already default.\r", timestamp, cp->channel);
#endif
    appenddirect(cp, buffer);
    appendprompt(cp, 0);
    return;
  }

  for (ch = channels; ch; ch = ch->next) {
    if (ch->chan == newchannel) break;
  }
  for (cl = cp->chan_list; cl; cl = cl->next) {
    if (cl->channel == newchannel) break;
  }
  if (!((cp->invitation_channel == newchannel) || !(ch) || !(ch->flags & M_CHAN_P)) && !cl && !cp->operator) {
#ifdef SPEECH
      sprintf(buffer, speech_message(44), timestamp, newchannel);
#else
      sprintf(buffer, "%sYou need an invitation to join the private channel %d.\r", timestamp, newchannel);
#endif
    appenddirect(cp, buffer);
    appendprompt(cp, 0);
#ifdef SPEECH
    sprintf(buffer, speech_message(45), timestamp, cp->name, cp->host);
#else
    sprintf(buffer, "%s%s@%s try to join your privat channel.", timestamp, cp->name, cp->host);
#endif
    send_msg_to_channel("conversd", newchannel, buffer);
    return;
  }
  if (!cl) {
    cl = (CLIST *)calloc(1, sizeof(CLIST));
    if (!cl || (!ch && NULL == ins_channel(newchannel)) ) {
#ifdef SPEECH
      sprintf(buffer, speech_message(46), timestamp, newchannel);
#else
      sprintf(buffer, "%scannot join channel %d, no more space.\r", timestamp, newchannel);
#endif
      appenddirect(cp, buffer);
      appendprompt(cp, 0);
      if (cl)
        free(cl);
      return;
    }
    cp->locked = 1;
#ifndef CONVNICK
    send_user_change_msg(cp->name,cp->host, -1, newchannel, cp->pers, cp->time);
#else
    send_user_change_msg(cp->name,cp->nickname, cp->host, -1, newchannel, cp->pers, cp->time);
#endif /* CONVNICK */
    cl->time = currtime;
    cp->mtime = currtime;
    if (!ch || !has_ChOp(newchannel))
      cl->channelop = 2;
    cl->channel = newchannel;
    cl->next = cp->chan_list;
    cp->chan_list = cl;
    cp->locked = 0;
  }
  cp->channel = newchannel;
#ifdef SPEECH
  sprintf(buffer, speech_message(47), timestamp, cp->channel);
#else
  sprintf(buffer, "%sYou are now talking to channel %d. ", timestamp, cp->channel);
#endif
  appenddirect(cp, buffer);
  if (ch) {
    if ((first = count_user(cp->channel)) == 1) {
#ifdef SPEECH
      sprintf(buffer, speech_message(48));
#else
      sprintf(buffer, "You're alone.\r");
#endif
    } else {
#ifdef SPEECH
      sprintf(buffer, speech_message(49), first);
#else
      sprintf(buffer, "There are %d users.\r", first);
#endif
    }
    appenddirect(cp, buffer);
    if (ch->topic)
    {
#ifdef CONV_TOPIC
#ifdef SPEECH
      sprintf(buffer, speech_message(38), ch->name, ts(ch->time));
      appenddirect(cp, buffer);
#else
      sprintf(buffer, "*** current Topic by: %s (%s):\r               "
                    , ch->name
                    , ts(ch->time));
      appenddirect(cp, buffer);
#endif /* SPEECH */
#else
      appenddirect(cp, "*** current Topic is:\r               ");
#endif /* CONV_TOPIC */
      appendstring(cp, ch->topic);
      appenddirect(cp, "\r");
    }
  } else {
#ifdef SPEECH
    appenddirect(cp, speech_message(48));
#else
    appenddirect(cp, "You're alone.\r");
#endif
  }
  if (cl->channelop == 2) {
    for (p = connections; p; p = p->next) {
      if (p->type == CT_HOST) {
        sprintf(buffer, "/\377\200OPER conversd %d %s\r", cp->channel, cp->name);
        appenddirect(p, buffer);
        send_proto("cmds", "TX to %s %s", p->name, buffer+3);
      }
    }
  }
  cp->channelop = cl->channelop;
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void charset_command(CONNECTION *cp)
{
  char *s1, *s2;
  char buffer[300];
  WORD charset_in, charset_out;

  s1 = getarg(NULL, GET_NXTLC);

  if (!*s1) {
#ifdef SPEECH
    sprintf(buffer, speech_message(50),
                        get_charset_by_ind(cp->charset_in),
                        get_charset_by_ind(cp->charset_out));
#else
    sprintf(buffer, "*** Charset in/out is %s/%s.\r",
                        get_charset_by_ind(cp->charset_in),
                        get_charset_by_ind(cp->charset_out));
#endif
    appenddirect(cp, buffer);
    appendprompt(cp, 0);
    return;
  }

  charset_in = get_charset_by_name(s1);

  s2 = getarg(NULL, GET_NXTLC);
  if (*s2) charset_out = get_charset_by_name(s2);
  else charset_out = charset_in;

  if (charset_in < 0 || charset_out < 0) {
    cp->charset_out = ISO;
#ifdef SPEECH
    sprintf(buffer, speech_message(51),
                        (charset_in < 0) ? s1 : s2, list_charsets());
#else
    sprintf(buffer, "Unknown charset: '%s'.  You may use one of them:\r%s***\r",
                        (charset_in < 0) ? s1 : s2, list_charsets());
#endif
    appenddirect(cp, buffer);
    appendprompt(cp, 0);
    return;
  }

  cp->charset_in = charset_in;
  cp->charset_out = charset_out;

#ifdef SPEECH
  sprintf(buffer, speech_message(52),
                        get_charset_by_ind(cp->charset_in),
                        get_charset_by_ind(cp->charset_out));
#else
  sprintf(buffer, "*** Charset in/out set to %s/%s.\r",
                        get_charset_by_ind(cp->charset_in),
                        get_charset_by_ind(cp->charset_out));
#endif
  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void cstat_command(CONNECTION *cp)
{
  cp->verbose = 1;            /* Flag wird beim Aendern der Links geloescht */
  if (*getarg(NULL, GET_ALL) != '#') {
    links_command(cp);
    getarg("", GET_NXTLC);    /* damit hosts_command keine Argumente kriegt */
  }
  if (cp->verbose)
    hosts_command(cp);
}

/*---------------------------------------------------------------------------*/

static void filter_command(CONNECTION *cp)
{
  ed_list(cp, 1);
}

/*---------------------------------------------------------------------------*/
/*  Damit convers der sprachlichen Umgebung angepasst werden kann ist
 *  die Hilfe extern organisiert. Die Hilfedatei besteht aus den
 *  jeweiligen Hifstexten, die am Anfang ein Stichwort zum Suchen
 *  haben und zwar @@Stichwort. Der allgemeine Hilfetext sind die
 *  ersten Zeilen in der Datei, bis zum @@ des ersten Stichworts.
 */
static WORD read_xhelp(char *topic, CONNECTION *cp)
{
  FILE *fp;
  char tmp[128], ts[16];
  char *c;
#ifdef L1IRC
  char buffer[2048];
#endif /* L1IRC */

   if (!topic)
       return(0);

   sprintf(tmp, "%sconversd.xhf", textpath);
   if ((fp = xfopen(tmp, "rt")) == NULL)
       return(0);

   if (*topic) {              /* topic gesetzt, suchen... */
     sprintf(ts, "@@%s", topic);
     do {
       if (fgets(tmp, 120, fp) == NULL) {
         fclose(fp);
         return(0);
       }
       if ((c = strpbrk(tmp, "\r\n")) != NULL)
         *c = NUL;
     }
     while (strcmp(ts,tmp));
   }
   if (fgets(tmp, 120, fp) == NULL) {
     fclose(fp);
     return(0);
   }
   do {
#ifdef L1IRC
     if (cp->IrcMode == TRUE)
       sprintf(buffer, ":%s 372 %s :"
                     , myhostname
                     , cp->nickname);
#endif /* L1IRC */
     if ((c = strpbrk(tmp, "\r\n")) != NULL)
     {
       *c++ = '\r';
       *c = NUL;
     }
#ifdef L1IRC
     if (cp->IrcMode == TRUE)
       appendstring(cp, buffer);
#endif /* L1IRC */
     appendstring(cp, tmp);
     topic = fgets(tmp, 120, fp);
   }
   while (topic && (*tmp != '@' || *(tmp+1) != '@'));
   fclose(fp);
#ifdef L1IRC
   if (cp->IrcMode == TRUE)
   {
     sprintf(buffer, "\n:%s 376 %s :\n"
                  , myhostname
                  , cp->nickname);
     appendstring(cp, buffer);
   }
#endif /* L1IRC */
   return(1);
}

#ifndef L1IRC
static void help_command(CONNECTION *cp)
#else
void help_command(CONNECTION *cp)
#endif /* L1IRC */
{
  char *s1;
  CMDTABLE *cmdp;

  s1 = getarg(NULL, GET_NXTLC);
  if (*s1 == '/')
    s1++;

  if (*s1 == '\0')
    read_xhelp("", cp);
  else {
    for (cmdp = cmdtable; cmdp->name; cmdp++)
      if ((cmdp->states & (1 << cp->type)) && !strnicmp(cmdp->name, s1, strlen(s1)))
        break;
    if (cmdp->name)
    {
      if (!read_xhelp(cmdp->help, cp))
#ifdef SPEECH
        appenddirect(cp, speech_message(53));
#else
        appenddirect(cp, "Help on this command not yet implemented...\rWrite it - or try another one :-)\r");
#endif
    }
    else
#ifndef L1IRC
#ifdef SPEECH
      appenddirect(cp, speech_message(54));
#else
      appenddirect(cp, "No such command...\r");
#endif /* SPEECH */
#else
      {
        char buffer[128];

#ifdef SPEECH
        sprintf(buffer, speech_message(54));
#else
        sprintf(buffer, "No such command...\r");
#endif /* SPEECH */
        if (cp->IrcMode == TRUE)
          sprintf(buffer, ":%s 421 %s No such command...\n"
                        , myhostname
                        , (*cp->nickname ? cp->nickname : cp->name));

        appenddirect(cp, buffer);
      }
#endif /* L1RC */
  }
  appendprompt(cp, 1);
}

/*---------------------------------------------------------------------------*/

static void hosts_command(CONNECTION *cp)
{
  char buffer[256], tmp[64];
  DESTINATION *d;
  WORD i = 1;
  char *dest;

  dest = getarg(NULL, GET_NXTCS);

  if (*dest == '\0') {
    for (d = destinations; d; d = d->next) {
      if (d->rtt) {
        sprintf(buffer, "%-9.9s (%-8.8s) %3s", d->name, d->rev, ts3(d->rtt,tmp));
        appenddirect(cp, buffer);
        if ((i == 3) || !(d->next)) {
          appenddirect(cp, "\r");
          i = 0;
        } else {
          appenddirect(cp, "   ");
        }
        i++;
      }
    }
    if ((i != 1)) {
      appenddirect(cp, "\r");
    }
    if (cp->type == CT_USER) {
      appendprompt(cp, 1);
    }
  } else if (*dest == '#') {
    for (d = destinations; d; d = d->next) {
      if (d->rtt)
        sprintf(buffer, "%-12.12s (%-8.8s) %5ld ->%-6s", d->name, d->rev, (long)d->rtt, d->link->cname);
       else
        sprintf(buffer, "%-12.12s                0         ", d->name);
      appenddirect(cp, buffer);
      if ((i == 2) || !(d->next)) {
        appenddirect(cp, "\r");
        i = 0;
      } else {
        appenddirect(cp, "   ");
      }
      i++;
    }
    if ((i != 1)) {
      appenddirect(cp, "\r");
    }
    if (cp->type == CT_USER) {
      appendprompt(cp, 1);
    }
  } else {
    for (d = destinations; d; d = d->next) {
      if (d->rtt && d->link && !strcmp(d->name, dest)) {
        clear_locks();
        sprintf(buffer, "*** Host : %s (%-8.8s) T=%lds\r", dest, d->rev, (long)d->rtt);
        appenddirect(cp, buffer);
        sprintf(buffer, "*** route: %s (%ld) %s -> %s\r", myhostname,
                        (long)((d->link->rxtime + d->link->txtime) / 2L),
                        d->link->cname, dest);
        appenddirect(cp, buffer);
        if (strcmp(dest, d->link->name)) {
          sprintf(buffer, "/\377\200ROUT %s %s 99\r", dest, cp->name);
          appenddirect(d->link->connection, buffer);
          send_proto("cmds", "TX to %s %s", d->link->connection->name, buffer+3);
        }
        break;
      }
    }
    if (d == NULLDESTINATION) {
#ifdef SPEECH
      sprintf(buffer, speech_message(55), dest);
#else
      sprintf(buffer, "*** no route to %s\r", dest);
#endif
      appenddirect(cp, buffer);
    }
    appendprompt(cp, 0);
  }
}

/*---------------------------------------------------------------------------*/

static void imsg_command(CONNECTION *cp)
{
  char *toname, *text;
  CONNECTION *p, *q;
#ifdef CONVNICK
  char buf[2 * NAMESIZE + 2];   /* Name + ":" + Nickname + 1 */
#endif

  toname = getarg(NULL, GET_NXTLC);
  text = getarg(NULL, GET_ALL);
  if (!*text) {
    appendprompt(cp, 0);
    return;
  }
  for (p = connections; p; p = p->next)
  {
#ifdef CONVNICK
      makeName(cp, buf);

    /* Typ User. */
    if (   p->type    == CT_USER
        /* Channelvergleich */
        && p->channel == cp->channel
        /* Callvergleich */
        && (strcasecmp(p->name, toname)
        /* Nicknamevergleich */
        ||  strcasecmp(p->nickname, toname)) != 0)
    {
      send_msg_to_user(buf, p->name, text);
#else
    if (p->type == CT_USER && p->channel == cp->channel && strcmp(p->name, toname) != 0) {
      send_msg_to_user(cp->name, p->name, text);
#endif
      q = p->next;
      while (q) {
        if (!strcmp(p->name, q->name))
           q->locked = 1;
        q = q->next;
      }
    }
    if (p->via)
      p->via->locked = 0;
  }
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void invite_command(CONNECTION *cp)
{
  char *toname;

  toname = getarg(NULL, GET_NXTLC);
  if (*toname) send_invite_msg(cp->name, toname, cp->channel);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

#ifndef L1IRC
static void leave_command(CONNECTION *cp)
#else
void leave_command(CONNECTION *cp)
#endif /* L1IRC */
{
  WORD chan;
  WORD users_left;
  CLIST *cl, *cl2;
  char *arg;
  char buffer[128];

  arg = getarg(NULL, GET_NXTLC);
#ifdef L1IRC
  if (arg && *arg == '#')
    arg++;
#endif /* L1IRC */
  if (*arg) {
    chan = atoi(arg);
  } else {
    chan = cp->channel;
  }
  if ((chan == cp->channel) && (cp->chan_list->next == NULLCLIST)) {
#ifndef L1IRC
    bye_command(cp);
    return;
#else
    if (cp->IrcMode == FALSE)
    {
      bye_command(cp);
      return;
    }
#endif /* L1IRC */
  }
  cl2 = cp->chan_list;
#ifdef L1IRC
  if (cl2 == NULL)
  {
    if (cp->IrcMode == TRUE)
      return;
  }
#endif /* L1IRC */
  if (cl2->channel == chan) {
    cp->chan_list = cl2->next;
    free(cl2);
    users_left = count_user(chan);
    if (!users_left) destroy_channel(chan);
    cp->locked = 1;
#ifndef CONVNICK
    send_user_change_msg(cp->name, cp->host, chan, -1, "/leave", cp->time);
#else
    send_user_change_msg(cp->name, cp->nickname, cp->host, chan, -1, "/leave", cp->time);
#endif /* CONVNICK */
    cp->locked = 0;
    if (chan == cp->channel) {
#ifndef L1IRC
      cp->channel = cp->chan_list->channel;
      cp->channelop = cp->chan_list->channelop;
#else
      cp->channel = (cp->chan_list) ? cp->chan_list->channel : -1;
      cp->channelop = (cp->chan_list) ? cp->chan_list->channelop : 0;

      if (cp->IrcMode == TRUE)
      {
        sprintf(buffer,":%s!%s@%s PART #%d\n"
                      ,(*cp->nickname ? cp->nickname : cp->name)
                      ,cp->name
                      ,cp->host
                      ,chan);
        appenddirect(cp, buffer);
        appendprompt(cp, 0);
        return;
      }
#endif /* L1IRC */
#ifdef SPEECH
      sprintf(buffer, speech_message(56), timestamp, cp->channel);
#else
      sprintf(buffer, "%sDefault channel is now %d.\r", timestamp, cp->channel);
#endif
      appenddirect(cp, buffer);
      appendprompt(cp, 0);
    } else {
#ifdef SPEECH
      sprintf(buffer, speech_message(57), timestamp, chan);
#else
      sprintf(buffer, "%sLeft channel %d.\r", timestamp, chan);
#endif
#ifdef L1IRC
      if (cp->IrcMode == TRUE)
        sprintf(buffer,":%s!%s@%s PART #%d\n"
                      ,(*cp->nickname ? cp->nickname : cp->name)
                      ,cp->name
                      ,cp->host
                      ,chan);
#endif /* L1IRC */
      appenddirect(cp, buffer);
      appendprompt(cp, 0);
    }
    return;
  }
  cl = cp->chan_list;
  while (cl) {
    if (cl->channel == chan) {
      cl2->next = cl->next;
      free (cl);
      cl = cl2;
      users_left = count_user(chan);
      if (!users_left) destroy_channel(chan);
      cp->locked = 1;
#ifndef CONVNICK
      send_user_change_msg(cp->name, cp->host, chan, -1, "/leave", cp->time);
#else
      send_user_change_msg(cp->name, cp->nickname, cp->host, chan, -1, "/leave", cp->time);
#endif /* CONVNICK */
      cp->locked = 0;
      if (chan == cp->channel) {
#ifndef L1IRC
        cp->channel = cp->chan_list->channel;
        cp->channelop = cp->chan_list->channelop;
#else
        cp->channel = (cp->chan_list) ? cp->chan_list->channel : -1;
        cp->channelop = (cp->chan_list) ? cp->chan_list->channelop : 0;
#endif /* L1IRC */
#ifdef SPEECH
        sprintf(buffer, speech_message(56), timestamp, cp->channel);
#else
        sprintf(buffer, "%sDefault channel is now %d.\r", timestamp, cp->channel);
#endif
#ifdef L1IRC
        if (cp->IrcMode == TRUE)
          sprintf(buffer,":%s!%s@%s PART #%d\n"
                        ,(*cp->nickname ? cp->nickname : cp->name)
                        ,cp->name
                        ,cp->host
                        ,chan);
#endif /* L1IRC */
        appenddirect(cp, buffer);
        appendprompt(cp, 0);
        return;
      } else {
#ifdef SPEECH
       sprintf(buffer, speech_message(57), timestamp, chan);
#else
       sprintf(buffer, "%sLeft channel %d.\r", timestamp, chan);
#endif
        appenddirect(cp, buffer);
        appendprompt(cp, 0);
        return;
      }
    } else {
      cl2 = cl;
    }
    cl = cl->next;
  }
#ifdef SPEECH
  sprintf(buffer, speech_message(58), timestamp, chan);
#else
  sprintf(buffer, "%sYou were not on channel %d.\r", timestamp, chan);
#endif
#ifdef L1IRC
  if (cp->IrcMode == TRUE)
    sprintf(buffer, ":%s 442 %s #%d :You were not on channel\n"
                  , myhostname
                  , (*cp->nickname ? cp->nickname : cp->name)
                  , chan);
#endif /* L1IRC */
  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/
static BOOLEAN scan_linksarg(char *arg, char *hostname, WORD *act,
#ifndef OS_IPLINK
                             char *call, char *via, WORD *prt)
#else
                             char *call, char *via, WORD *prt, unsigned char *ip)
#endif /* OS_IPLINK */
{
  WORD port, bcnt;
  char buffer[256], *bp;

  if (*arg == '-' || *arg == '+') {
    if (*arg == '-')
      *act = 1;
    do {
      arg++;
    } while (*arg == ' ');
  }

  bcnt = (WORD) min(strlen(arg), 255);
  if (!bcnt)
      return(FALSE);

  arg[bcnt] = NUL;
  bp = buffer;
  strcpy(buffer, arg);

#ifndef OS_IPLINK
  if (getcal(&bcnt, &bp, TRUE, call) != YES)           /* Call ok ? */
      return(FALSE);

  callss2str(hostname, call);           /* hostname: Call ohne SSID */
#else
  /* Pruefe ob Servername korrekt ist. */
  if (IPConvGetName(&bcnt, &bp, FALSE, hostname) != TRUE)
    /* Nicht korrekt, abbruch. */
    return(FALSE);

  /* Pruefe das AX25-CALL. */
  if (getcal(&bcnt, &bp, TRUE, call) == YES)       /* Call ok ? */
    *ip = '\0';
  else
    {
      int TcpPort = 0;

      /* IP-Adresse pruefen. */
      if (IPConvGetIP(&bcnt, &bp, ip) == YES)       /* Call ok ? */
      {
        struct hostent *he;

        /* Hostname oder IP-Adresse pruefen. */
        he = gethostbyname((char *)ip);

        if (he == NULL)
          return(FALSE);

        strncpy(call, hostname, L2CALEN);
        call[6] = NUL;
        strlwr(hostname);
        *prt = 255;
        *via = '\0';

        /* keine weiteren Angaben */
        if (skipsp(&bcnt, &bp) == FALSE)
          return(FALSE);

        TcpPort = nxtlong(&bcnt, &bp);
        *prt = TcpPort;

        /* Falscher Port */
        if (  (TcpPort <= 0)
            ||(TcpPort > 65535))
          return(FALSE);
        else
          IPConvAddTBL(call, ip, he, (unsigned short)TcpPort, TRUE);

        return(TRUE);
      }
      else
        {
          /* Es ist kein IP-Link. */
          *ip = '\0';
          /* Es wuerde kein AX25-CALL gesetzt! */
          cpyid(call, hostname);
          /* hostname: Call ohne SSID */
          callss2str(hostname, call);

          if (*act)
          {
            strlwr(hostname);
            IPConvDelTBL(hostname);
            return(TRUE);
          }
        }
  }
#endif /* OS_IPLINK */

  strlwr(hostname);
  if (*act)                      /* Beim Loeschen reicht das soweit */
      return(TRUE);

  *prt = 255;
  *via = '\0';

  if (skipsp(&bcnt, &bp) == FALSE)        /* keine weiteren Angaben */
    return(TRUE);

  if (bcnt && !isdigit(*bp))                    /* Keine Portangabe */
    return(FALSE);

  port = nxtnum(&bcnt, &bp);
  if (port >= L2PNUM && port != 254)              /* Falscher Port */
    return(FALSE);

  *prt = port;
  if (port != 254)
    getdig(&bcnt, &bp, 0, via);

 return(TRUE);
}


/*  /links - host                  loeschen (egal, ob L2 oder circuit)
 *  /links [+] host                circuit eintragen
 *  /links [+] host port [vialist] L2 Verbindung eintragen
 *  /links @ host                  Fernabfrage versenden
 */

#ifndef L1IRC
static void links_command(CONNECTION *cp)
#else
void links_command(CONNECTION *cp)
#endif /* L1IRC */
{
  char buffer[128];
#ifndef CONVERS_HOSTNAME
  char hn[L2CALEN+1], hc[L2IDLEN], hv[L2VLEN+1];
#else
  char hn[NAMESIZE + 1], /* Server-Name */
       hc[L2IDLEN],      /* AX25-Call   */
       hv[L2VLEN+1];     /* Digipeater  */
#endif /* CONVERS_HOSTNAME */
  char *arg;
  WORD pl, remove = 0, hp;
  PERMLINK *p;
  CONNECTION *c;
#ifdef CONVERS_LINKS
  char          name[NAMESIZE + 1];
#endif
#ifdef OS_IPLINK
  unsigned char Hostname[HNLEN + 1];

  Hostname[0] = 0;
#endif /* OS_IPLINK */

  arg = getarg(NULL, GET_ALL);
  if (*arg == '@' && cp->type == CT_USER) { /* Fernabfrage, aehnlich tpp */
    arg = getarg(NULL, GET_NXTCS);
    if (*arg == '@') arg++;
    if (!*arg) arg = getarg(NULL, GET_NXTCS);
    if (!Strcmp(myhostname, arg)) {
      disp_links(cp, NULL);
      appendprompt(cp, 1);
    } else {
      sprintf(buffer, "/\377\200LINK %s %s\r", cp->name, arg);
      for (c = connections; c; c = c->next)
        if (c->type == CT_HOST) {
          appenddirect(c, buffer);
          send_proto("cmds", "TX to %s %s", c->name, buffer+3);
        }
#ifdef SPEECH
    sprintf(buffer, speech_message(59), arg);
#else
    sprintf(buffer, "*** Request sent to %s.\r", arg);
#endif
      appenddirect(cp, buffer);
      appendprompt(cp, 0);
    }
    return;
  }
  if (*arg) {
    if (cp->operator != 2) {
#ifdef SPEECH
        appenddirect(cp, speech_message(60));
#else
        appenddirect(cp, "You must be an operator to set up new links\r");
#endif
    } else {
#ifndef OS_IPLINK
      if (scan_linksarg(arg, hn, &remove, hc, hv, &hp) == TRUE) {
#else
      if (scan_linksarg(arg, hn, &remove, hc, hv, &hp, Hostname) == TRUE) {
#endif /* OS_IPLINK */
        for (pl = 0; pl < MAXCVSHOST; pl++) {
          p = permarray[pl];
#ifdef CONVERS_LINKS
          /* es sind kein eintrage vorhanden. */
          if (p == NUL)
              /* zum Schleifenkopf. */
              continue;
          /* kopie vom AX25 Rufzeichen. */
          strcpy(name,hn);
          /* In Grossbuchstaben umwandeln. */
          strupr(name);
          /* Vergleiche Conversname und AX25 Rufzeichen. */
          if ((!strcasecmp(p->cname, hn)) || (cmpcal(p->call, name))) {
#else
          if (p && (!strcmp(p->cname, hn) || !cmpcal(p->call, hn))) {
#endif
              if (p->connection)
              bye_command2(p->connection, "link killed");
            if (permarray[pl] != NULLPERMLINK) {
              permarray[pl] = NULLPERMLINK;
              free(p);
            }
            break;
          }
        }
        for (c = connections; c; c = c->next)
          if (!strcmp(c->name, hn))
            bye_command2(c, "link killed");
        if (!remove) {
          p = update_permlinks(hn, NULLCONNECTION, 1);
          if (p) {
            strncpy(p->cname, hn, NAMESIZE);
            p->port        = hp;                              /* port  */
            cpyid(p->call,   hc);                             /* call  */
            cpyidl(p->via,   hv);                             /* digis */
#ifdef OS_IPLINK
            if (Hostname[0] != FALSE)
            {
              strncpy((char *)p->HostName, (char *)Hostname, HNLEN);
              p->TcpLink = TRUE;
            }
            else
              p->TcpLink = FALSE;
#endif /* OS_IPLINK */
          } else {
#ifdef SPEECH
              appenddirect(cp, speech_message(61));
#else
              appenddirect(cp, "Link table full !\r");
#endif
          }
        }
        cp->verbose = 0; /* bei cstat keine Destinations mehr anzeigen */
        userpo->sysflg = 2;
      } else {
#ifdef SPEECH
            appenddirect(cp, speech_message(62));
#else
            appenddirect(cp, "Argument error !\r");
#endif
      }
    }
  }
  disp_links(cp, NULL);
  if (cp->type == CT_USER) {
    appendprompt(cp, 1);
  }
}

static void disp_links(CONNECTION *cp, char *user)
{
  char buffer[128], *bp, tmp[10];
  WORD pl;
  PERMLINK *p;

  strcpy(buffer, "Host      State        Quality Revision  Since NextTry Tries Queue    RX    TX");
  if (user) {
    clear_locks();
    send_msg_to_user ("conversd", user, buffer);
  } else {
    strcat(buffer, "\r");
    appenddirect(cp, buffer);
  }
  for (pl = 0; pl < MAXCVSHOST; pl++) {
    p = permarray[pl];
    if (p) {
      bp = buffer;
      bp += sprintf(bp, "%-9.9s ", p->cname);
      if (!p->connection) {
        bp += sprintf(bp, "%s   ---            %s",
                      (p->locked)?"Disc./locked":"Disconnected", ts(p->statetime));
        if (p->port != 254)
          bp += sprintf(bp, " %s %5d", ts(p->retrytime), p->tries);
      } else {
        if (p->connection->type == CT_HOST) {
          bp += sprintf(bp, "Connected    ");
          if (p->txtime || p->rxtime) {
            if (p->rxtime == -1) {
              bp += sprintf(bp, "  %3s  ", ts3(p->txtime, tmp));
            } else {
              bp += sprintf(bp, "%3s/", ts3(p->txtime, tmp));
              bp += sprintf(bp, "%-3s", ts3(p->rxtime, tmp));
            }
          } else {
            bp += sprintf(bp, "  ---  ");
          }
          bp += sprintf(bp, " %-8.8s %s               %5ld %4ldK %4ldK",
                  p->connection->rev, ts(p->connection->time),
                  queuelength(p->connection), p->connection->received/1024L,
                  p->connection->xmitted/1024L);
        } else {
          bp += sprintf(bp, "Connecting     ---            %s", ts(p->statetime));
          bp += sprintf(bp, " %s %5d", ts(p->retrytime), p->tries);
        }
      }
      if (user) {
        clear_locks();
        send_msg_to_user ("conversd", user, buffer);
      } else {
        strcpy(bp, "\r");
        appenddirect(cp, buffer);

        if (cp->operator) {
          call2str(tmp, p->call);
          bp = buffer;
          if (p->port == 254) {
            bp += sprintf(bp, "(trusted host");
          } else {
#ifdef OS_IPLINK
              if (p->TcpLink)
                bp += sprintf(bp, "(%s", p->HostName);
              else
#endif /* OS_IPLINK */
            bp += sprintf(bp, "(%s", tmp);
            if (p->port != 255) {
              bp += sprintf(bp, " on port %d", p->port);
              if (*p->via) {
               char *liste = p->via;
                bp += sprintf(bp, " via");
                while (*liste) {
                  call2str(tmp, liste);
                  bp += sprintf(bp, " %s", tmp);
                  liste += L2IDLEN;
                }
              }
            }
          }
          strcpy(bp, ")\r");
          appenddirect(cp, buffer);
        }
      }
      if (p->nlcks) {
        sprintf(buffer, " %d loops detected.", p->nlcks);
        if (user) {
          clear_locks();
          send_msg_to_user ("conversd", user, buffer);
        } else {
          strcat(buffer, "\r");
          appenddirect(cp, buffer);
        }
      }
    }
  }
  if (user) {
    clear_locks();
    send_msg_to_user ("conversd", user, "***");
  }
}

/*---------------------------------------------------------------------------*/

static void list_command(CONNECTION *cp)
{
  char buffer[2048];
  char *flags, *bp;
  register CHANNEL *ch;
  WORD hide, showit, isonchan, n;
  CONNECTION *p;
  CLIST *cl;
#ifdef CONVNICK
  char buf[(NAMESIZE * 2) + 2];
#endif

  appenddirect(cp, "Channel Flags  Topic\r        Users\r");
  for (ch = channels; ch; ch = ch->next) {
    isonchan = 0;
    hide = 0;
    if (ch->chan == cp->channel)
      isonchan++;
    for (cl = cp->chan_list; cl && !isonchan; cl = cl->next) {
      if (cl->channel == ch->chan) {
        isonchan++;
      }
    }
    flags = getflags(ch->flags);
    if (ch->flags & M_CHAN_S)
      hide |= 1;
    if (ch->flags & M_CHAN_I)
      hide |= 2;
    showit = 0;
    bp = buffer;
    if (!hide || isonchan || cp->operator) {
      if (!ch->flags && (!ch->topic)) {
        bp += sprintf(bp, "%7d", ch->chan);
      } else {
        bp += sprintf(bp, "%7d %-6.6s %s\r       ", ch->chan, flags, ch->topic?ch->topic:"");
      }
      showit++;
    } else {
      if (hide == 1) {
        bp += sprintf(bp, "------- %-6.6s %s\r       ", flags, ch->topic?ch->topic:"");
        showit++;
      }
    }
    n = 9;
    if (showit) {
      for (p = connections; p; p = p->next) {
        if (p->type == CT_USER) {
          if (p->via) {
            if (p->channel == ch->chan) {
              if (n > cp->width - 12) {
                n = 9;
                bp += sprintf(bp, "\r       ");
              }
#ifdef CONVNICK
              /* Name:Nickname zusammen basteln. */
              makeName(p, buf);
              bp += sprintf(bp, " %s(", buf);
              n += 2 + (WORD)strlen(buf);
#else
              bp += sprintf(bp, " %s(", p->name);
              n += 2 + (WORD)strlen(p->name);
#endif /* CONVNICK */
              if (p->operator) {
                *bp++ = '!';
                n++;
              }
              else if (p->channelop) {
                *bp++ = '@';
                n++;
              }
              if (p->away) {
                *bp++ = 'G';
                n++;
              }
              if (*(bp-1) == '(') {
                bp--;
                n--;
              } else {
                *bp++ = ')';
                n++;
              }
            }
          } else {
            for (cl = p->chan_list; cl; cl = cl->next) {
              if (cl->channel == ch->chan) {
                if (n > cp->width - 12) {
                  n = 9;
                  bp += sprintf(bp, "\r       ");
                }
#ifdef CONVNICK
                /* Name:Nickname zusammen basteln. */
                makeName(p, buf);
                bp += sprintf(bp, " %s(", buf);
                n += 2 + (WORD)strlen(buf);
#else
                bp += sprintf(bp, " %s(", p->name);
                n += 2 + (WORD)strlen(p->name);
#endif /* CONVNICK */
                if (p->operator) {
                  *bp++ = '!';
                  n++;
                }
                else if (cl->channelop) {
                  *bp++ = '@';
                  n++;
                }
                if (p->away) {
                  *bp++ = 'G';
                  n++;
                }
                if (*(bp-1) == '(') {
                  bp--;
                  n--;
                } else {
                  *bp++ = ')';
                  n++;
                }
              }
            }
          }
        }
      }
      *bp++ = '\r';
      *bp = '\0';
      appendstring(cp, buffer);
    }
  }
  if (cp->type == CT_USER) {
    appendprompt(cp, 1);
  }
}

/*---------------------------------------------------------------------------*/

static void me_command(CONNECTION *cp)
{
  char *text;
  char buffer[2048];
  CHANNEL *ch;

  text = getarg(NULL, GET_ALL);

  if (*text) {
    for (ch = channels; ch; ch = ch->next) {
      if (ch->chan == cp->channel) break;
    }
    if (cp->operator || cp->channelop || !(ch->flags & M_CHAN_M)) {
      cp->locked = 1;
#ifdef CONVNICK
      if (cp->nickname[0] != '\0')
        sprintf(buffer, "*** %s:%s@%s %s", cp->name, cp->nickname, cp->host, text);
      else
        sprintf(buffer, "*** %s@%s %s", cp->name, cp->host, text);
#else
       sprintf(buffer, "*** %s@%s %s", cp->name, cp->host, text);
#endif
      send_msg_to_channel("conversd", cp->channel, buffer);
    } else {
#ifdef SPEECH
        appenddirect(cp, speech_message(31));
#else
        appenddirect(cp, "*** This is a moderated channel. Only channel operators may write.\r");
#endif
    }
  }
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

#ifndef L1IRC
static void msg_command(CONNECTION *cp)
#else
void msg_command(CONNECTION *cp)
#endif /* L1IRC */
{
  char *toname, *text;
  char buffer[600];
  CONNECTION *p;
  WORD chan;
  CHANNEL *ch;
  CLIST *cl;

  toname = getarg(NULL, GET_NXTLC);
  text = getarg(NULL, GET_ALL);
  if (!*text) {
    appendprompt(cp, 0);
    return;
  }
  if (toname[0] == '#') {
    chan = atoi(toname+1);
    for (cl = cp->chan_list; cl; cl = cl->next) {
      if (cl->channel == chan) break;
    }
    if (!cl) {
#ifdef SPEECH
      sprintf(buffer, speech_message(63), timestamp, chan);
#else
      sprintf(buffer, "%sYou have not joined channel %d.\r", timestamp, chan);
#endif
#ifdef L1IRC
      if (cp->IrcMode == TRUE)
        sprintf(buffer, ":%s 405 %s #%d :Du hast den Kanal nicht betreten!\n"
                      , myhostname
                      , cp->nickname
                      , chan);
#endif /* L1IRC */
      appenddirect(cp, buffer);
    } else {
      for (ch = channels; ch; ch = ch->next) {
        if (ch->chan == chan) break;
      }
      if (cl->channelop || !(ch->flags & M_CHAN_M)) {
#ifdef CONVNICK
        makeName(cp, buffer);
        send_msg_to_channel(buffer, chan, text);
#else
        send_msg_to_channel(cp->name, chan, text);
#endif
          } else {
#ifdef L1IRC
        if (cp->IrcMode == TRUE)
        {
          sprintf(buffer, ":%s 481 %s :Permission Denied- You're not an IRC operator\n"
                        , myhostname
                        , cp->name);
          appendstring(cp, buffer);
        }
        else
#endif /* L1IRC */
#ifdef SPEECH
        appenddirect(cp, speech_message(31));
#else
        appenddirect(cp, "*** This is a moderated channel. Only channel operators may write.\r");
#endif
      }
    }
  } else {
    for (p = connections; p; p = p->next)
#ifdef CONVNICK
      /* Typ User. */
      if (   p->type == CT_USER
          /* Callvergleich */
          && (!strcasecmp(p->name, toname)
          /* Nickamevergleich */
          || !strcasecmp(p->nickname, toname)))
        break;
#else
      if (p->type == CT_USER && !strcmp(p->name, toname)) break;
#endif
    if (!p) {
#ifdef SPEECH
      sprintf(buffer, speech_message(64), timestamp, toname);
#else
      sprintf(buffer, "%sNo such user: %s.\r", timestamp, toname);
#endif
#ifdef L1IRC
        if (cp->IrcMode == TRUE)
          sprintf(buffer, ":%s 401 %s %s :No such nick\n"
                        , cp->host
                        , cp->nickname
                        , toname);
#endif /* L1IRC */
      appendstring(cp, buffer);
    } else {
      if (p->away) {
#ifdef SPEECH
          sprintf(buffer, speech_message(65), timestamp, toname, p->away);
#else
          sprintf(buffer, "%s%s is away: %s\r", timestamp, toname, p->away);
#endif
#ifdef L1IRC
         if (cp->IrcMode == TRUE)
           sprintf(buffer, ":%s 306 %s :User ist Abwesend!\n"
                         ,cp->host
                         ,cp->nickname);
#endif /* L1IRC */
        appendstring(cp, buffer);
      }
#ifdef CONVNICK
      makeName(cp, buffer);
      send_msg_to_user(buffer, toname, text);
#else
      send_msg_to_user(cp->name, toname, text);
#endif
    }
  }
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void mode_command(CONNECTION *cp)
{
  char *arg, *c;
  WORD remove = 0;
  WORD channel;
  CONNECTION *p;
  register CHANNEL *ch;
  WORD oldflags = 0;
  CLIST *cl;
  WORD op = 0;

  if (cp->type == CT_USER) {
    arg = getarg(NULL, GET_ALL);
    if (isdigit(*arg))
      channel = atoi(getarg(NULL, GET_NXTLC));
    else
      channel = cp->channel;
    arg = getarg(NULL, GET_ALL);
    for (cl = cp->chan_list; cl; cl = cl->next) {
      if ((cl->channel == channel) && (cl->channelop)) {
        op++;
        break;
      }
    }
  }
  else {
    channel = atoi(getarg(NULL, GET_NXTLC));
    arg = getarg(NULL, GET_ALL);
  }

  for (ch = channels; ch; ch = ch->next)
    if (channel == ch->chan)
      break;

  if (!ch && cp->type == CT_USER) {
#ifdef SPEECH
      appenddirect(cp, speech_message(66));
#else
      appenddirect(cp, "*** non existing channel !\r");
#endif
    appendprompt(cp, 0);
    return;
  }

  /* Kanal 0 soll keine setzbaren Flags haben.
     Flags von Hosts werden durchgereicht.
     Sysop loescht fern, bei Aufruf von /mo                      */
  if (channel == 0) {
    if (cp->type == CT_USER) {    /* bei uns eingegeben          */
      if (!cp->operator) {        /* User duerfen nix            */
#ifdef SPEECH
          appenddirect(cp, speech_message(67));
#else
          appenddirect(cp, "*** no modes on channel 0 !\r");
#endif
        appendprompt(cp, 0);
        return;
      }
      else {                      /* Sysop setzt andere Hosts    */
        arg = "-sptiml";          /* zurueck (arg ist egal)      */
        ch->flags = (WORD)0xFFFFU;/* alle Flags an, zum Loeschen */
      }
    }
  } /* Flags werden unten nochmals (nach Aussendung) geloescht */

  if (*arg) {
    if (op || cp->operator || (cp->type == CT_HOST)) {
      if (ch)
        oldflags = ch->flags;
      while (*arg) {
        switch (toupper(*arg)) {
        case '+':
          remove = 0;
          arg++;
          break;

        case '-':
          remove = 1;
          arg++;
          break;

        case 'I':
          if (ch) {
            if (remove) ch->flags &= ~(M_CHAN_I);
              else      ch->flags |= M_CHAN_I;
          }
          arg++;
          break;

        case 'L':
          if (ch) {
            if (remove) ch->flags &= ~(M_CHAN_L);
              else      ch->flags |= M_CHAN_L;
          }
          arg++;
          break;

        case 'M':
          if (ch) {
            if (remove) ch->flags &= ~(M_CHAN_M);
              else      ch->flags |= M_CHAN_M;
          }
          arg++;
          break;

        case 'O':
          arg++;
          while (*arg) {
            while (*arg == ' ') arg++;
            c = arg;
            while (*c != '\0' && *c != ' ') {
              *c = tolower(*c);
              c++;      /* damit gcc zufrieden ist :( */
            }
            if (*c != '\0')
              *c++ = '\0';
            for (p = connections; p; p = p->next) {
              if (p->type == CT_USER && !Strcmp(p->name, arg)) {
                if ((channel == -1) && (cp->type == CT_HOST)) {
                  p->operator = 1;
#ifndef L1IRC
                  send_opermsg(arg, p->host, cp->name, -1);
#else
                  send_opermsg(arg, p->host, cp->name, -1, 0);
#endif /* L1IRC */
                } else {
                  if (p->channel == channel) {
                    p->channelop = 1;
                  }
                  for (cl = p->chan_list; cl; cl = cl->next) {
                    if (cl->channel == channel) cl->channelop = 1;
                  }
#ifndef L1IRC
                  send_opermsg(arg, p->host, cp->name, -1);
#else
                  send_opermsg(arg, p->host, cp->name, -1, 0);
#endif /* L1IRC */
                }
              }
            }
            arg = c;
          }
          break;

        case 'P':
          if (ch) {
            if (remove) ch->flags &= ~(M_CHAN_P);
              else      ch->flags |= M_CHAN_P;
          }
          arg++;
          break;

        case 'S':
          if (ch) {
            if (remove) ch->flags &= ~(M_CHAN_S);
              else      ch->flags |= M_CHAN_S;
          }
          arg++;
          break;

        case 'T':
          if (ch) {
            if (remove) ch->flags &= ~(M_CHAN_T);
              else      ch->flags |= M_CHAN_T;
          }
          arg++;
          break;

        default:
          arg++;
          break;
        }
      }
      if (ch && ch->flags != oldflags) {
        if (cp->type == CT_HOST)
          cp->locked = 1;
#ifndef L1IRC
        send_mode(ch);
#else
        send_mode(ch, cp, oldflags);
#endif /* L1IRC */
        if (channel == 0)            /* KEINE Flags auf Kanal 0 ! */
          ch->flags = 0;
      }
    } else {
#ifdef SPEECH
      appenddirect(cp, speech_message(68));
#else
      appenddirect(cp, "*** You are not an operator !\r");
#endif
    }
  }
  if (cp->type == CT_USER) {
    appenddirect(cp, "*** Flags: ");
    appenddirect(cp, getflags(ch->flags));
    appenddirect(cp, "\r");
    appendprompt(cp, 0);
  }
}


/*---------------------------------------------------------------------------*/

static void name_command(CONNECTION *cp)
{
  char buffer[2048], *pers;
  WORD newchannel;
  LONG nc;
  CHANNEL *ch;
  CLIST *cl;
  WORD cnt;

  getarg(NULL, GET_NXTLC);                                  /* name */
  cp->type = CT_USER;

#ifdef CONVERS_CTEXT
  read_xhelp("CTEXT", cp);
#endif
#ifdef SPEECH
  sprintf(buffer, speech_message(69), convtype, myhostname, strchr(REV, ':')+2);
#else
  sprintf(buffer, "%s @ %s PingPong-Release %5.5s (TNN) - Type /HELP for help.\r", convtype, myhostname, strchr(REV, ':')+2);
#endif
  appenddirect(cp, buffer);

  nc = atol(getarg(NULL, GET_NXTLC)); newchannel = (WORD)nc;

  if (nc < 0L || nc > MAXCHANNEL) {
#ifdef SPEECH
        sprintf(buffer, speech_message(42), timestamp, MAXCHANNEL);
#else
        sprintf(buffer, "%sChannel numbers must be in the range 0..%d.\r", timestamp, MAXCHANNEL);
#endif
    appenddirect(cp, buffer);
    newchannel = 0;
  }
  cp->channel = newchannel;
  for (ch = channels; ch; ch = ch->next) {
    if (ch->chan == cp->channel) break;
  }
  if (ch && (ch->flags & M_CHAN_P) && !cp->operator) {
#ifdef SPEECH
      sprintf(buffer, speech_message(44), timestamp, newchannel);
#else
      sprintf(buffer, "%sYou need an invitation to join channel %d.\r", timestamp, newchannel);
#endif
    appenddirect(cp, buffer);
#ifdef CONCNICK
    if (cp->nickname[0] != '\0')
      sprintf(buffer, "%s%s:%s@%s try to join your privat channel.", timestamp, cp->name, cp->nickname, cp->host);
    else
      sprintf(buffer, "%s%s@%s try to join your privat channel.", timestamp, cp->name, cp->host);
#else
     sprintf(buffer, "%s%s@%s try to join your privat channel.", timestamp, cp->name, cp->host);
#endif
    send_msg_to_channel("conversd", cp->channel, buffer);
    clear_locks();
    cp->locked = 1;
    cp->channel = 0;
    newchannel = 0;
    for (ch = channels; ch; ch = ch->next) {
      if (ch->chan == cp->channel) break;
    }
  }
  pers = personalmanager(GET, cp, NULL);

  cl = (CLIST *) calloc(1, sizeof(CLIST));
  if (!cl || (!ch && NULL == ins_channel(newchannel)) ) {
#ifdef SPEECH
      sprintf(buffer, speech_message(46), timestamp, newchannel);
#else
      sprintf(buffer, "%scannot join channel %d, no more space.\r", timestamp, newchannel);
#endif
    appenddirect(cp, buffer);
    appendprompt(cp, 0);
    if (cl)
      free(cl);
    cp->type = CT_CLOSED;
    return;
  }
  if (ch) {
    cnt = count_user(newchannel);
#ifdef SPEECH
    sprintf(buffer, speech_message(47), timestamp, newchannel);
#else
    sprintf(buffer, "%sYou are now talking to channel %d. ", timestamp, newchannel);
#endif
    appenddirect(cp, buffer);
    if (!cnt) {
#ifdef SPEECH
      sprintf(buffer, speech_message(48));
#else
      sprintf(buffer, "You're alone.\r");
#endif
    } else {
#ifdef SPEECH
      sprintf(buffer, speech_message(49), cnt+1);
#else
      sprintf(buffer, "There are %d users.\r", cnt+1);
#endif
    }
    appenddirect(cp, buffer);
    if (ch->topic) {
#ifdef CONV_TOPIC
#ifdef SPEECH
      sprintf(buffer, speech_message(125), ch->name, ts(ch->time), ch->topic);
#else
      sprintf(buffer, "*** current Topic by: %s (%s):\r               %s\r"
                    , ch->name
                    , ts(ch->time)
                    , ch->topic);
#endif /* SPEECH */
#else
      sprintf(buffer, "*** current Topic is:\r               %s\r", ch->topic);
#endif /* CONV_TOPIC */
      appendstring(cp, buffer);
    }
    cp->channelop = (has_ChOp(newchannel)) ? 0 : 1;
  } else {
#ifdef SPEECH
      sprintf(buffer, speech_message(72), cp->channel);
#else
      sprintf(buffer, "*** You created a new channel %d.\r", cp->channel);
#endif
    appenddirect(cp, buffer);
    cp->channelop = 1;
  }
  cl->next = NULLCLIST;
  cl->channelop = cp->channelop;
  cl->channel = cp->channel;
  cl->time = currtime;
  cp->mtime = currtime;
  cp->chan_list = cl;
#ifndef CONVNICK
  send_user_change_msg(cp->name, cp->host, -1, cp->channel, pers, currtime);
#else
  send_user_change_msg(cp->name, cp->nickname, cp->host, -1, cp->channel, pers, currtime);
#endif /* CONVNICK */
  if (cp->channelop) {
    clear_locks();
#ifndef L1IRC
    send_opermsg(cp->name, cp->host, "conversd", cp->channel);
#else
    send_opermsg(cp->name, cp->host, "conversd", cp->channel, 1);
#endif /* L1IRC */
  }
  if (cp->operator) {        /* TNN Sysop-Status wird durchgereicht */
    clear_locks();
#ifndef L1IRC
    send_opermsg(cp->name, cp->host, "conversd", cp->channel);
#else
    send_opermsg(cp->name, cp->host, "conversd", cp->channel, 1);
#endif /* L1IRC */
  }

#ifdef CONVNICK
  /* ggf. Nickname setzen. */
  if (GetNickname(cp))
  {
    /* Ausgabe vorbereiten. */
#ifdef SPEECH
    sprintf (buffer,speech_message(113),cp->nickname);
#else
    sprintf(buffer, "*** Nickname set up: %s\r", cp->nickname);
#endif /* SPEECH. */
    send_uaddmsg(cp);              /* Nickname an andere HOST's weiterleiten. */
    appendstring(cp, buffer);
  }
#endif /* CONVNICK */
#ifdef L1IRC
    SendIrcNick(cp);
#endif /* L1IRC */

  if (pers == NULL)
  {                /* freshmeat, jummy    XAO*/
    read_xhelp("FAID", cp);
  }
  else
    if (*pers)
    {
#ifdef SPEECH
      sprintf(buffer, speech_message(73), pers);
#else
      sprintf(buffer, "*** Personal text and data set.\rHello, %s\r", pers);
#endif
      appendstring(cp, buffer);
#ifdef CONVNICK
      appendprompt(cp, 0);
#else
      appendprompt(cp, 1);
#endif
      setstring(&(cp->pers), pers, 256);
    } else
#ifdef SPEECH
        appenddirect(cp, speech_message(74));
#else
        appenddirect(cp, "*** Please set your personal text. ( /H PERS )\r");
#endif
}

/*---------------------------------------------------------------------------*/

#ifdef CONVNICK
static void nickname_command(CONNECTION *cp)
{
  char *s;
  char buffer[128], *bp;

  s = getarg(NULL, GET_ALL);

  bp = buffer + sprintf(buffer, "%sYour nickname is ", timestamp);

  if (!*s) {
    if (cp->nickname[0] != '\0') {
      sprintf(bp, "\"%s\".\r", cp->nickname);
    }
    else {
      appenddirect(cp, "*** No nickname set.\r");
      appendprompt(cp, 0);
      return;
    }
  }
  else {
    if (!strcmp(s, "@")) {
      s = "\0";
      strcpy(bp, "deleted.\r");
    }
    else {
#ifndef CONVNICK_FIX
      strcpy(cp->nickname, s, min(strlen(s));
#else
      /* Alten Nickname sichern. */
      strcpy(cp->OldNickname, cp->nickname);
      /* Neuen Nickname sichern. */
      strcpy(cp->nickname, s);
#endif /* CONVNICKFIX */
      sprintf(bp, "\"%s\".\r", cp->nickname);
    }
  }

  /* Aenderungen merken */
  if (s != cp->nickname)
  {
    /* Alten Nickname sichern. */
    strncpy(cp->OldNickname, cp->nickname, NAMESIZE);
    /* Neuen Nickname sichern. */
    strncpy(cp->nickname, s, NAMESIZE);

    /* Melden, wenn was gesetzt ist, abmelden ist anscheinend nicht moeglich */
    if (cp->nickname[0] != '\0')
      send_uaddmsg(cp);

    appenddirect(cp, buffer);
    appendprompt(cp, 0);

#ifdef L1IRC
    SendIrcNick(cp);
#endif /* L1IRC */
#ifdef USERPROFIL
   /* Aktualisiere Profil-Daten. */
   ProfilService(cp);
#endif /* USERPROFIL */
   return;
  }

  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void nonickname_command(CONNECTION *cp)
{
  if (cp->nickname[0] != '\0') {
    memset(cp->nickname, 0, NAMESIZE);
    /* Nickname abmelden (nicht moeglich, nicht vorgesehen !) */
    /* send_uaddmsg(cp); */
    appenddirect(cp, "*** Nickname locally deleted.\r");
  }
  else
    appenddirect(cp, "*** No nickname set.\r");

  appendprompt(cp, 0);
}

#endif

/*---------------------------------------------------------------------------*/

static void notify_command(CONNECTION *cp)
{
  ed_list(cp, 0);
}

/*---------------------------------------------------------------------------*/

/*
 * Bearbeiten von Userlisten
 * which = 0 modifiziert die /notify-, sonst /filter-Liste
 */
static void ed_list(CONNECTION *cp, WORD which)
{
  char *p, *q, *toname;
  CONNECTION *pc;
  char buffer[256], tmp[512], wbuf[512];
  WORD action;

  toname = getarg(NULL, GET_NXTLC);
  strcpy(tmp, toname);
  toname = tmp;

  p = (which == 0) ? cp->notify : cp->filter;
  if (p)
    strcpy(wbuf, p);
  else
    *wbuf = '\0';

  if (*toname == '\0') {
    sprintf(buffer,
#ifdef SPEECH
        (which == 0) ?  speech_message(75)
                      : speech_message(76),
#else
        (which == 0) ? "%sYou are notified if one of the following users sign on/off:\r"
                       : "%sYou filter the messages of the following users:\r",
#endif

    timestamp);
    appenddirect(cp, buffer);
    appendstring(cp, wbuf);
    appenddirect(cp, "\r");
  }

  action = 0;
  while (*toname) {
    while ((*toname == '+') || (*toname == '-')) {
      while (*toname == '+') {
        action = 0;
        toname++;
        if (*toname == '\0') {
          toname = getarg(NULL, GET_NXTLC);
          strcpy(tmp, toname);
          toname = tmp;
          if (*toname == '\0') {
            break;
          }
        }
      }
      while (*toname == '-') {
        action = 1;
        toname++;
        if (*toname == '\0') {
          toname = getarg(NULL, GET_NXTLC);
          strcpy(tmp, toname);
          toname = tmp;
          if (*toname == '\0') {
            break;
          }
        }
      }
    }
    strcat(toname, " ");
    p = wbuf;
    p = strstr(p, toname);
    while (p) {
      p--;
      if (*p != ' ') {
        p++;
        p++;
      } else {
        q = ++p;
        while (*q != ' ') q++;
        while (*q == ' ') q++;
        while (*q != '\0') *p++ = *q++;
        *p = *q;
        p = wbuf;
      }
      p = strstr(p, toname);
    }
    if (action == 0 && strcmp(toname, "conversd ")) {
      if (wbuf[0] == '\0') {
        strcpy(wbuf, " ");
      }
      strcat(wbuf, toname);
      if (which == 0) {
        for (pc = connections; pc; pc = pc->next) {
          sprintf(buffer, "%s ", pc->name);
          if ((pc->type == CT_USER) && !strncmp(tmp, buffer, strlen(buffer))) {
#ifdef SPEECH
              sprintf(buffer, speech_message(76), pc->name);
#else
              sprintf(buffer, "*** %s is online.\r", pc->name);
#endif
#ifdef L1IRC
            if (cp->IrcMode == TRUE)
              sprintf(buffer, ":%s 303 %s :\n", myhostname, cp->nickname);
#endif /* L1IRC */
            appendstring(cp, buffer);
            break;
          }
        }
      }
    }
    toname = getarg(NULL, GET_NXTLC);
    strcpy(tmp, toname);
    toname = tmp;
  }
  appendprompt(cp, 0);
  setstring((which == 0) ? &cp->notify : &cp->filter, wbuf, 256);
}

/*---------------------------------------------------------------------------*/

static void personal_command(CONNECTION *cp)
{
  char *s;
  char buffer[128], *bp;

  s = getarg(NULL, GET_ALL);

  bp = buffer + sprintf(buffer, "%sPersonal text ", timestamp);

  if (!*s) {
    if (cp->pers) {
      s = cp->pers;
#ifdef SPEECH
      strcpy(bp, speech_message(78));
#else
      strcpy(bp, "and data saved.\r");
#endif
    } else {
#ifdef SPEECH
      appenddirect(cp, speech_message(79));
#else
      appenddirect(cp, "*** No personal text to save.\r");
#endif
      appendprompt(cp, 0);
      return;
    }
  } else if (!strcmp(s, "@")) {
    s = "";
#ifdef SPEECH
    strcpy(bp, speech_message(80));
#else
    strcpy(bp, "deleted.\r");
#endif
  } else {
#ifdef SPEECH
      strcpy(bp, speech_message(81));
#else
      strcpy(bp, "and data set.\r");
#endif
  }
  personalmanager(SET, cp, s);
  if (s != cp->pers) {
    setstring(&(cp->pers), s, 256);
    cp->mtime = currtime;
#ifndef CONVNICK
    send_persmsg(cp->name, myhostname, cp->channel, s, cp->time);
#else
    send_persmsg(cp->name, cp->nickname, myhostname, cp->channel, s, cp->time);
#endif /* CONVNICK */
  }
  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void prompt_command(CONNECTION *cp)
{
  char buffer[42];
  char *args, *p;

#ifdef L1IRC
  if (cp->IrcMode == TRUE)
  {
    char buffer[2048];

    sprintf(buffer, ":%s 421 %s PROMPT :Unknown command\n", myhostname, cp->nickname);
    appendstring(cp, buffer);
    return;
  }
#endif /* L1IRC */

  args = getarg(NULL, GET_ALL);
  p = cp->prompt;
  p[2] = '\0';
  p[3] = '\0';
  p[0] = '\0';

  if ((p[1] = args[0]) != '\0')
    if ((p[2] = args[1]) != '\0')
      if ((p[3] = args[2]) != '\0')
        p[0] = args[3];
  if (p[3] == '\b' && p[0] == '\0') {
    p[0] = '\b';
    p[3] = '\0';
  }

#ifdef SPEECH
  sprintf(buffer, speech_message(82), (*args) ? speech_message(83) : speech_message(84));
#else
  sprintf(buffer, "*** Prompting mode %sabled\r", (*args) ? "en" : "dis");
#endif
  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/
/* die loop-lock Loesung soll anders geschehen */
/* restart loest die loop-locks,
   mit dem Komando ist noch mehr geplant
*/
static void restart_command(CONNECTION *cp)
{
  char     buffer[128];
  WORD     pl;
  PERMLINK *p;

  if (cp->operator != 2) {
#ifndef L1IRC
#ifdef SPEECH
      appenddirect(cp, speech_message(85));
#else
      appenddirect(cp, "You must be an operator to restart!\r");
#endif
#else /* L1IRC */
    if (cp->IrcMode == FALSE)
#ifdef SPEECH
      appenddirect(cp, speech_message(85));
#else /* SPEECH */
      appenddirect(cp, "You must be an operator to restart!\r");
#endif /* SPEECH */
    else
    {
      sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
      appendstring(cp, buffer);
    }
#endif /* L1IRC */
    }
    else {
    for (pl = 0; pl < MAXCVSHOST; pl++) {
      p = permarray[pl];
      if (p) {
        if (p->locked) {
          p->locked = 0;
          p->statetime = currtime; /* Aufbauzeit auf 1min spaeter setzen */
          p->tries = 0;
          p->waittime = 60;
          p->rxtime = 0;
          p->txtime = 0;
          p->testwaittime = currtime;
          p->testnexttime = currtime + 60;
          p->retrytime = currtime + p->waittime;
#ifdef SPEECH
          sprintf(buffer, speech_message(86), p->cname);
#else
          sprintf(buffer, "Link to %s delocked.\r", p->cname);
#endif
          appenddirect(cp, buffer);
        }
      }
    }
  }
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void query_command(CONNECTION *cp)
{
  char *toname;
  char buffer[128];
  CONNECTION *p;

#ifdef L1IRC
  if (cp->IrcMode == TRUE)
  {
    sprintf(buffer, ":%s 421 %s QUERY :Unknown command\n", myhostname, cp->nickname);
    appendstring(cp, buffer);
    return;
  }
#endif /* L1IRC */

  toname = getarg(NULL, GET_NXTLC);

  if (*toname) {
    for (p = connections; p; p = p->next)
#ifdef CONVNICK
      /* Typ User. */
      if (   p->type == CT_USER
          /* Callvergleich. */
          && (!strcasecmp(p->name, toname)
          /* Nicknamevergleich. */
          ||  !strcasecmp(p->nickname, toname)))
        break;
#else
      if (p->type == CT_USER && !strcmp(p->name, toname)) break;
#endif
    if (!p) {
#ifdef SPEECH
        sprintf(buffer, speech_message(87), timestamp, toname);
#else
        sprintf(buffer, "%sNo such user: %.20s.\r", timestamp, toname);
#endif
      appendstring(cp, buffer);
    }
    else {
      strcpy(cp->query, toname);
#ifdef SPEECH
      sprintf(buffer, speech_message(88), timestamp, cp->query);
#else
      sprintf(buffer, "%sStarting private conversation with %s.\r", timestamp, cp->query);
#endif
      appendstring(cp, buffer);
    }
  }
  else if (cp->query[0] != '\0') {
#ifdef SPEECH
      sprintf(buffer, speech_message(89), timestamp, cp->query);
#else
      sprintf(buffer, "%sEnding private conversation with %s.\r", timestamp, cp->query);
#endif
    appendstring(cp, buffer);
    cp->query[0] = '\0';
  }
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void topic_command(CONNECTION *cp)
{
  CHANNEL *ch;
  CONNECTION *p;
  register char *topic;
  char buffer[2048];
  time_t time;
  WORD channel;

  channel = cp->channel;
  topic = getarg(NULL, GET_ALL);
  if (*topic == '#') {
    topic++;
    while (*topic == ' ') topic++;
    channel = atoi(topic);
    while (*topic && *topic != ' ') topic++;
    while (*topic == ' ') topic++;
  }
#ifdef L1IRC
  if (cp->IrcMode == TRUE)
  {
    if (channel < 0)
    {
      sprintf(buffer, ":%s 442 %s * :No channel joined. Try /join #<channel>\n", myhostname, cp->nickname);
      appendstring(cp, buffer);
      return;
    }

    if (*topic == ':')
    {
      if (!*(topic+1))
        *topic = '@';   // mark deleted
      else
        topic++;
    }
  }
#endif /* L1IRC */
  for (p = connections; p; p = p->next)
    if (p->type == CT_USER && !strcmp(p->name, cp->name) && (p->channel == channel)) break;
  time = currtime;
  for (ch = channels; ch; ch = ch->next) {
    if (ch->chan == channel) break;
  }
  if (ch) {
    if (ch->time > time) time = ch->time+1L;
    if (*topic) {
      if (p && (((ch->flags & M_CHAN_T) == 0) || cp->operator || p->channelop)) {
        if (*topic == '@') {
          *topic = '\0';
#ifdef CONV_TOPIC
#ifdef SPEECH
          sprintf(buffer, speech_message(90), timestamp, channel, ch->name, ts(ch->time));
#else
          sprintf(buffer, "%sChannel topic on channel %d removed from %s (%s).\r"
                        , timestamp
                        , channel
                        , ch->name
                        , ts(ch->time));
#endif /* SPEECH */
#else
          sprintf(buffer, "%sChannel topic on channel %d removed.\r", timestamp, channel);
#endif /* CONV_TOPIC */
        } else {
#ifdef CONV_TOPIC
             /* Topic sichern. */
             setstring(&(ch->topic), topic, 512);
             /* Call sichern. */
             strncpy(ch->name, cp->name, NAMESIZE + 1);
             /* Nullzeichen setzen. */
             ch->name[sizeof(ch->name)-1] = 0;
             /* Zeit sichern. */
             ch->time = time;
#ifdef SPEECH
            sprintf(buffer, speech_message(91), timestamp, channel, ch->name, ts(ch->time));
#else
            sprintf(buffer, "%sChannel topic set on channel %d from %s (%s).\r"
                          , timestamp
                          , channel
                          , ch->name
                          , ts(ch->time));
#endif /* SPEECH */
#else
            sprintf(buffer, "%sChannel topic set on channel %d.\r", timestamp, channel);
#endif /* CONV_TOPIC */
#ifdef L1IRC
          if (cp->IrcMode == TRUE)
            sprintf(buffer, ":%s TOPIC #%d :%s\n", cp->nickname, channel, topic);
#endif /* L1IRC */
        }
#ifndef CONVNICK
        send_topic(cp->name, cp->host, time, channel, topic);
#else
        send_topic(cp->name, cp->nickname, cp->host, time, channel, topic);
#endif /* CONVNICK */
      } else {
#ifdef SPEECH
          sprintf(buffer, speech_message(68), timestamp);
#else
          sprintf(buffer, "%sYou are not an operator.\r", timestamp);
#endif
#ifdef L1IRC
        if (cp->IrcMode == TRUE)
          sprintf(buffer, ":%s 482 %s #%d :%s\n", myhostname, cp->name, channel, speech_message(68));
#endif /* L1IRC */
      }
    } else {
      if (ch->topic)
      {
#ifdef CONV_TOPIC
#ifdef SPEECH
        sprintf(buffer, speech_message(92), timestamp, channel, ch->name, ts(ch->time));
#else
        sprintf(buffer, "%sCurrent channel topic on channel %d from %s (%s) is\r               "
                      , timestamp
                      , channel
                      , ch->name
                      , ts(ch->time));
#endif /* SPEECH */
#else
        sprintf(buffer, "%sCurrent channel topic on channel %d is\r               "
                      , timestamp
                      , channel);
#endif /* CONV_TOPIC */
#ifndef L1IRC
        appenddirect(cp, buffer);
        appendstring(cp, ch->topic);
        strcpy(buffer, "\r");
#else
        if (cp->IrcMode == TRUE)
        {
          sprintf(buffer, ":%s 332 %s #%d :%s",cp->host,*cp->nickname ? cp->nickname : cp->name, cp->channel, ch->topic);
          buffer[2048] = 0;
          appendstring(cp, buffer);
          appendstring(cp, "\n");
          sprintf(buffer, ":%s 333 %s #%d %ld\n",cp->host,*cp->nickname ? cp->nickname : cp->name, channel, ch->time);
        }
        else
        {
          appenddirect(cp, buffer);
          appendstring(cp, ch->topic);
          strcpy(buffer, "\r");
        }
#endif /* L1IRC */
      } else {
#ifdef SPEECH
          sprintf(buffer, speech_message(93), timestamp, channel);
#else
          sprintf(buffer, "%sNo current channel topic on channel %d.\r", timestamp, channel);
#endif
#ifdef L1IRC
        if (cp->IrcMode == TRUE)
          sprintf(buffer, ":%s 331 %s #%d :There isn't a topic.\n", myhostname, cp->nickname, channel);
#endif /* L1IRC */
      }
    }
  } else {
#ifdef SPEECH
    sprintf(buffer, speech_message(94), timestamp, channel);
#else
    sprintf(buffer, "%sChannel channel %d non existent.\r", timestamp, channel);
#endif
#ifdef L1IRC
    if (cp->IrcMode == TRUE)
      sprintf(buffer, ":%s 442 %s #%d :%s\n", myhostname, cp->nickname, channel, speech_message(94));
#endif /* L1IRC */
  }
  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void uptime_command(CONNECTION *cp)
{
  char buffer[128];

#ifdef SPEECH
  sprintf(buffer, speech_message(95), convtype, myhostname, ts4(currtime - boottime));
#else
  sprintf(buffer, "*** %s@%s is up for %s\r", convtype, myhostname, ts4(currtime - boottime));
#endif
#ifdef L1IRC
  if (cp->IrcMode == TRUE)
  {
    sprintf(buffer, ":%s 242 %s :Server Up %s\n", myhostname, cp->name, ts4(currtime - boottime));
    appendstring(cp, buffer);
    appendprompt(cp, 0);
    return;
  }
#endif /* L1IRC */
  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void verbose_command(CONNECTION *cp)
{
  char buffer[32];

  cp->verbose = 1 - cp->verbose;
#ifdef SPEECH
  sprintf(buffer, speech_message(96), (cp->verbose) ? speech_message(83) : speech_message(84));
#else
  sprintf(buffer, "*** Verbose mode %sabled\r", (cp->verbose) ? "en" : "dis");
#endif
#ifdef L1IRC
  if (cp->IrcMode == TRUE)
  {
    sprintf(buffer, ":%s MODE %s %cws\n", myhostname, cp->name, cp->verbose ? '+' : '-');
    appendstring(cp, buffer);
    appendprompt(cp, 0);
    return;
  }
#endif /* L1IRC */
  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void version_command(CONNECTION *cp)
{
  char buffer[128];

  sprintf(buffer, "*** conversd PingPong-Release %5.5s (TNN)\r", strchr(REV, ':')+2);
  appenddirect(cp, buffer);
#ifdef SPEECH
  appenddirect(cp, speech_message(97));
  appenddirect(cp, speech_message(98));
#else
  appenddirect(cp, "  This conversd implementation was originally written by Dieter Deyke\r"
                   "  <deyke@mdddhd.fc.hp.com>. It was modified and maintained up to version\r"
                   "  3.11 by Fred Baumgarten, dc6iq. This implementation is partly rewritten,\r"
                   "  enhanced and maintained by Odo Roscher <dl1xao@db0hhb.#hh.deu.eu>\r"
                   "  for TheNetNode and Xnet.\r");
#endif
  appendprompt(cp, 1);
}

/*---------------------------------------------------------------------------*/

static void who_command(CONNECTION *cp)
{
  char buffer[2048];
  char tmp[20];
  WORD flags;
  WORD thischan = -1;
  WORD full = 0;
  WORD aways = 0;
  WORD user = 0;
  char channelstr[16];
  char *options;
  char *athost = NULL;
  CONNECTION *p;
  CHANNEL *ch;
  CLIST *cl;
  WORD showit;
  WORD opstat;
  WORD isop;
  WORD width, w;
#ifdef CONVERS_USERANZAHL
  WORD usercount = FALSE;
#endif

  options = getarg(NULL, GET_NXTLC);
  if (   *options == '\0'
      || strchr("*aln@u", *options) == NULL) {
    list_command(cp);
    return;
  }
#ifdef CONVNICK
  appenddirect(cp, "User    Nickname Host         Via    Chan.  ");
#else
   appenddirect(cp, "User    Host   Via    Chan.  ");
#endif
  width = cp->width;
  if (width < 66)
    width = 80;
  switch (*options) {
    case '*':
#ifdef SPEECH
      appenddirect(cp, speech_message(99));
#else
      appenddirect(cp, " Idle Personal\r");
#endif
      thischan = cp->channel;
      break;
    case 'a':
#ifdef SPEECH
        appenddirect(cp, speech_message(100));
#else
        appenddirect(cp, "Login State\r");
#endif
      aways = 1;
      break;
    case 'u':
      user = 1;
      options++;
      if (!*options)
        options = getarg(NULL, GET_ALL);
    case 'l':
      appenddirect(cp, "Login Queue     RX      TX\r");
      full = 1;
      break;
    case '@':
      athost = ++options;
      if (!*athost)
        athost = getarg(NULL, GET_NXTCS);
    case 'n':
#ifdef SPEECH
        appenddirect(cp, speech_message(101));
#else
        appenddirect(cp, "Login Personal\r");
#endif
  }
  if (!user && athost == NULL) {            /* optional bei *,a,l,n */
    options = getarg(NULL, GET_NXTLC);      /* Kanalfilter setzen   */
    if (*options)
      thischan = atoi(options);
  }

  for (ch = channels; ch; ch = ch->next) {
    flags = ch->flags;
    for (p = connections; p; p = p->next) {
      if (p->type != CT_USER)
        continue;
      showit = 0;
      opstat = 0;
      isop = 0;
      if (p->channel == ch->chan) {
        opstat = p->channelop;
        showit = 1;
      }
      isop = cp->operator;
      for (cl = cp->chan_list; cl; cl = cl->next) {
        if (ch->chan == cl->channel) {
          isop |= cl->channelop;
          break;
        }
      }
      for (cl = p->chan_list; cl; cl = cl->next) {
        if (ch->chan == cl->channel) {
          opstat = cl->channelop;
          showit = 1;
          break;
        }
      }
      if (   ((flags & M_CHAN_I) && !(isop || ch->chan == cp->channel))
          || (thischan != -1 && thischan != ch->chan)
          || (athost && strncmp(athost, p->host, strlen(athost)))
          || (user && strstr(options, p->name) == NULL))
        showit = 0;

      if (showit) {
        if (!(flags & M_CHAN_S) || (isop) || (ch->chan == cp->channel)) {
          sprintf(channelstr, "%5d", ch->chan);
        } else {
          strcpy(channelstr, "-----");
        }
        sprintf(buffer, full ?
#ifdef CONVNICK
                "%-6.6s%c %-8.8s %-12.12s %-6.6s %5s %6s %5ld %7ld %7ld" :
                "%-6.6s%c %-8.8s %-12.12s %-6.6s %5s %6s",
                p->name, p->operator ? '!' : opstat ? '@' : ' ',
                (p->nickname[0] != '\0' ? p->nickname : ""), p->host,
#else
                 "%-6.6s%c %-6.6s %-6.6s %5s %6s %5ld %7ld %7ld" :
                 "%-6.6s%c %-6.6s %-6.6s %5s %6s",
                 p->name, p->operator ? '!' : opstat ? '@' : ' ', p->host,
#endif
                p->via ? p->via->name : "", channelstr,
                (thischan != -1) ? ts3(currtime - p->mtime, tmp) : ts(p->time),
                queuelength(p), p->received, p->xmitted);
        w = width;
        if (p->pers || aways) {
          if (full) {
            strcat(buffer, "\r        Personal: ");
            strcat(buffer, p->pers?p->pers:"");
          } else {
            strcat(buffer, " ");
            if (aways) {
              if (p->away) {
                strncat(buffer, p->away, w - 51);
#ifdef SPEECH
                strcat(buffer, speech_message(104));
#else
                strcat(buffer, " (since ");
#endif
                strcat(buffer, ts(p->atime));
                strcat(buffer, ")");
              } else {
#ifdef SPEECH
                strcat(buffer, speech_message(102));
#else
                strcat(buffer, "(here)");
#endif
              }
            } else {
              if (p->away)
                w -= 7;
              if (p->pers)
#ifdef CONVNICK
                 strncat(buffer, p->pers,22);
#else
                 strncat(buffer, p->pers, w - 36);
#endif
            }
          }
        }
        if (!aways) {
          if (p->away) {
            if (full) {
#ifdef SPEECH
              strcat(buffer, speech_message(103));
#else
              strcat(buffer, "\r        Away: ");
#endif
              strcat(buffer, p->away);
#ifdef SPEECH
              strcat(buffer, speech_message(104));
#else
              strcat(buffer, " (since ");
#endif
              strcat(buffer, ts(p->atime));
              strcat(buffer, ")");
            } else {
#ifdef SPEECH
                strcat(buffer, speech_message(105));
#else
                strcat(buffer, " (AWAY)");
#endif
            }
         } else {
            if (full) {
              if (p->mtime) {
#ifdef SPEECH
                  strcat(buffer, speech_message(106));
#else
                  strcat(buffer, "\r        Last Activity: ");
#endif
                strcat(buffer, ts(p->mtime));
              }
            }
          }
        }
        strcat(buffer, "\r");
#ifdef CONVERS_USERANZAHL
        usercount++;
#endif
        appendstring(cp, buffer);
      }
    }
  }
  if (cp->type == CT_USER) {
#ifdef CONVERS_USERANZAHL
if (usercount != FALSE)
    {
    if (thischan == EOF)
#ifdef SPEECH
      sprintf(buffer, speech_message(351),usercount);
#else
      sprintf(buffer,"\r*** Insgesamt %d User auf allen Kanaelen ***\r",usercount);
#endif
else
#ifdef SPEECH
      sprintf(buffer, speech_message(126),usercount, channelstr);
#else
      sprintf(buffer,"\r*** %d User auf Kanal%s ***\r",usercount,channelstr);
#endif
      appendstring(cp, buffer);
    } else
        {
#ifdef SPEECH
        sprintf(buffer, speech_message(172));
#else
        sprintf(buffer,"No such User!\r");
#endif
        appendstring(cp, buffer);
       }
#else
     appendprompt(cp, 1);
#endif
  }
}

/*---------------------------------------------------------------------------*/

static void width_command(CONNECTION *cp)
{
  WORD neww;
  char buffer[128];

#ifdef L1IRC
  if (cp->IrcMode == TRUE)
  {
    sprintf(buffer, ":%s 421 %s WIDTH :Unknown command\n", myhostname, cp->nickname);
    appendstring(cp, buffer);
    return;
  }
#endif /* L1IRC */

  neww = atoi(getarg(NULL, GET_NXTLC));

  if (neww == 0)
#ifdef SPEECH
      sprintf(buffer, speech_message(107), cp->width);
#else
    sprintf(buffer, "*** Current screen width is %d\r", cp->width);
#endif
  else if (neww < 32 || neww > 255)
#ifdef SPEECH
    sprintf(buffer, speech_message(108));
#else
    sprintf(buffer, "*** Range 32 to 255\r");
#endif
  else {
    cp->width = neww;
#ifdef SPEECH
    sprintf(buffer, speech_message(109), cp->width);
#else
    sprintf(buffer, "*** Screen width set to %d\r", cp->width);
#endif
  }
  appenddirect(cp, buffer);
  appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static void wall_command(CONNECTION *cp)
{
  char       buffer[128];
  CONNECTION *p;
  char       *text;

  if (cp->operator != 2) {
#ifdef SPEECH
    appenddirect(cp, speech_message(68));
#else
    appenddirect(cp, "*** You are not an operator !\r");
#endif
    appendprompt(cp, 0);
    return;
  }

  text = getarg(NULL, GET_ALL);
  if (!*text)
      return;

  for (p = connections; p; p = p->next)
    if (   p->type == CT_USER
        && !p->via
        && p != cp) {
#ifdef SPEECH
      sprintf(buffer, speech_message(110), cp->name);
#else
      sprintf(buffer, "*** Urgent message from operator (%s):\r    ", cp->name);
#endif
      appenddirect(p, buffer);
      appendstring(p, text);
      appenddirect(p, "\r");
      appendprompt(p, 0);
    }
}

/*---------------------------------------------------------------------------*/

#ifdef CVS_ZAPPING
static void zap_command(CONNECTION *cp)
{
  char buffer[256], *arg;

  arg = getarg(NULL, GET_ALL);
  if (cp->operator != 2 || !*arg)
      return;

  sprintf(buffer, "/\377\200%.245s\r", arg);
  for (cp = connections; cp; cp = cp->next)
    if (cp->type == CT_HOST) {
      appenddirect(cp, buffer);
      send_proto("cmds", "TX to %s %s", cp->name, buffer+3);
    }
}
#endif

/*---------------------------------------------------------------------------*/
/* Host-Kommandos                                                            */
/*---------------------------------------------------------------------------*/

static void h_away_command(CONNECTION *cp)
{
  char *fromname, *hostname, *text;
  time_t time;
  CONNECTION *p;
  int changed = 1;

  fromname = getarg(NULL, GET_NXTLC);
  hostname = getarg(NULL, GET_NXTCS);
  time = atol(getarg(NULL, GET_NXTLC));
  if (time <= MAXCHANNEL)                           /* altes AWAY erkennen */
      return;

  text = getarg(NULL, GET_ALL);
  cp->locked = 1;
  for (p = connections; p; p = p->next) {
    if (   p->type == CT_USER
        && !Strcmp(p->name, fromname)
        && !Strcmp(p->host, hostname)) {
      if (p->away && !strcmp(p->away, text)) {
        changed = 0;
        continue;
      }
      setstring(&(p->away), text, 256);
      p->atime = time;
      break;
    }
  }
  if (changed)
#ifndef CONVNICK
    send_awaymsg(fromname, hostname, time, text);
#else
    send_awaymsg(fromname, p->nickname, hostname, time, text);
#endif /* CONVNICK */
}

/*---------------------------------------------------------------------------*/

#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn -par
#endif
static void h_cmsg_command(CONNECTION *cp)
{
  char *text;
  char *name;
  WORD channel;
#ifdef CONVNICK
  CONNECTION *p;
  char        name2[2 * NAMESIZE + 2];                 /* Rufzeichen:Nickname */
  char        Nickname[NAMESIZE + 1],
             *c;
#endif

  name = getarg(NULL, GET_NXTLC);
#ifdef CONVNICK
  memset(Nickname, 0, sizeof(Nickname));
  memset(name2, 0, sizeof(name2));

  strncpy(name2, name, 2 * NAMESIZE + 1); /* String fuer Nickname Auswertung. */
  name2[sizeof(name2)-1] = 0;                           /* Sicher ist sicher. */

  if ((c = strchr(name2, ':')) != NULL)              /* Pruefe nach Nickname. */
  {
    c++;                                               /* Loesche zeichen ':'. */

    strncpy(Nickname, c, NAMESIZE);                      /* Nickname sichern. */
    Nickname[sizeof(Nickname)-1] = 0;                   /* Sicher ist sicher. */
    *c = NUL;
  }

  if (Nickname[0] != FALSE)                              /* Nickname gefunden. */
  {
    for (p = connections; p; p = p->next)
    {
      if (!strncasecmp(p->name, name2, L2CALEN))             /* Callvergleich. */
      {
        if (!strncasecmp(p->OldNickname  /*Ist alter und neuer Nickname gleich,*/
                       , p->nickname
                       , NAMESIZE))
          break;                              /* Keine Aenderung durchfuehren. */

        strncpy(p->OldNickname, p->nickname, NAMESIZE); /* Alten Nick sichern. */
        strncpy(p->nickname, Nickname, NAMESIZE);           /* Neuen Nick sichern. */
        p->nickname[sizeof(p->nickname)-1] = 0;         /* Nullzeichen setzen. */
#ifdef L1IRC
        SendIrcNick(p);         /* Nickname an alle IRC-Client's weiterleiten. */
#endif /* L1IRC */
#ifdef USERPROFIL
        ProfilService(p);                        /* Aktualisiere Profil-Daten. */
        break;
#endif /* USERPROFIL */
      }
    }
  }
#endif /* CONVNICK */

  if (strlen(name) > 64)  /* 1. Schutz vor Nicknames */
    name[64] = NUL;
  channel = atoi(getarg(NULL, GET_NXTLC));
  text = getarg(NULL, GET_ALL);
  if (*text)
    send_msg_to_channel(name, channel, text);
}
#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn .par
#endif
/*---------------------------------------------------------------------------*/

static WORD is_looped(PERMLINK *l, char *host)
{
  DESTINATION *d;

  /* Schleifenerkennung 1:
     Nachbar will uns was ueber uns erzaehlen */
  if (!Strcmp(myhostname, host))
      return(1);

  /* Schleifenerkennung 2:
     host schon aus anderer Richtung bekannt */
  if (l) {
    for (d = destinations; d; d = d->next)
      if (!Strcmp(d->name, host))
        break;
    if (d && d->link != l && d->rtt)
        return(1);
  }

  return(0);
}

/*---------------------------------------------------------------------------*/

static void h_dest_command(CONNECTION *cp)
{
  char *name, *rev;
  LONG rtt;
  PERMLINK *l;

  name = getarg(NULL, GET_NXTCS);
  if (!*name)
      return;

  rtt = atol(getarg(NULL, GET_NXTLC));
  rev = getarg(NULL, GET_NXTLC);

  l = permlink_of(cp);

  if (is_looped(l, name)) {
    if (l)
      l->locked = 1;                               /*   verriegeln */
    bye_command2(cp, "loop detect");               /* und abwerfen */
    return;
  }

  if (l && rtt >= 0L)
    update_destinations(l, name, rtt, rev);
}

/*---------------------------------------------------------------------------*/

static void h_host_command(CONNECTION *cp)
{
  char        *name;
  char        *rev;
#ifdef CONVNICK
  char        *features;
#endif
  char        buffer[1024];
  WORD        pl;
  char        *flags;
  CONNECTION  *c;
  PERMLINK    *p;
  DESTINATION *d;
  CHANNEL     *ch;
  CLIST       *cl;

  if (*cp->name == NUL)
      return;

  name = getarg(NULL, GET_NXTCS);
  if (!*name)
      return;

  rev = getarg(NULL, GET_NXTLC);
  if (!*rev)
    rev = "?";

#ifdef CONVNICK
  features = getarg(NULL, GET_NXTCS);

  cp->features = 0;
  if (strchr(features, 'n') != NULL)
    cp->features |= FEATURE_NICK;
#endif

  for (pl = 0; pl < MAXCVSHOST; pl++) {
    p = permarray[pl];
    if (p && !Strcmp(p->cname, cp->name)) {
      if (p->connection && (p->connection != cp))
        bye_command2(p->connection, "link reorg");
    }
  }
  for (c = connections; c; c = c->next)        /* sicher ist sicher */
    if (!Strcmp(c->name, cp->name) && c != cp && !c->via)
      bye_command2(c, "link reorg");

  cp->type = CT_HOST;
  strcpy(cp->host, "-");
  Strcpy(cp->rev, rev);
  p = update_permlinks(cp->name, cp, 0);
#ifdef CONVERS_HOSTNAME
  Strcpy(p->cname, name);
#else
  Strcpy(p->name, name);
#endif
  if (cp->up->convflag == 1) { /* nur senden, wenn connect von aussen */
    sprintf(buffer, "/\377\200HOST %s %s %s\r", myhostname, myrev, myfeatures);
    appenddirect(cp, buffer);
    send_proto("cmds", "TX to %s %s", cp->name, buffer+3);
  }
  send_proto("cmds", "sending user, modes... to %s", cp->name);
  /* DEST's zuerst aussenden (fuer loop-detect) */
  for (d = destinations; d; d = d->next) {
    if (d->rtt) {
      sprintf(buffer, "/\377\200DEST %s %ld %s\r", d->name, (long)d->rtt + 99L, d->rev);
      appenddirect(cp, buffer);
    }
  }
  /* Aussenden eines DEST zum Bekanntmachen (fuer loop-detect) */
  update_destinations(p, name, 1, rev);
  for (c = connections; c; c = c->next) {
    if (c->type == CT_USER) {
      if (!c->via) {
        for (cl = c->chan_list; cl; cl = cl->next) {
          sprintf(buffer, "/\377\200USER %s %s %ld -1 %d %s\r", c->name, c->host, (long)cl->time, cl->channel, c->pers?c->pers:"");
          appenddirect(cp, buffer);
          if (cl->channelop) {
            sprintf(buffer, "/\377\200OPER conversd %d %s\r", cl->channel, c->name);
            appenddirect(cp, buffer);
          }
        }

#ifdef CONVNICK
        /* Nickname des Users senden wenn er einen hat */
        if ((cp->features & FEATURE_NICK) && (c->nickname[0] != '\0')) {
          sprintf(buffer, "/\377\200UADD %s %s %s %d %s\r", c->name, c->host, c->nickname, -1, c->pers ? c->pers : "~~");
          appenddirect(cp, buffer);
        }
#endif
        if (c->away) {
          sprintf(buffer, "/\377\200AWAY %s %s %ld %s\r", c->name, c->host, (long)c->atime, c->away);
          appenddirect(cp, buffer);
        }
        if (c->operator) {
          sprintf(buffer, "/\377\200OPER conversd -1 %s\r", c->name);
          appenddirect(cp, buffer);
        }
      } else {
        sprintf(buffer, "/\377\200USER %s %s %ld -1 %d %s\r", c->name, c->host, (long)c->time, c->channel, c->pers?c->pers:"");
        appenddirect(cp, buffer);
        if (c->away) {
          sprintf(buffer, "/\377\200AWAY %s %s %ld %s\r", c->name, c->host, (long)c->atime, c->away);
          appenddirect(cp, buffer);
        }
        if (c->channelop) {
          sprintf(buffer, "/\377\200OPER conversd %d %s\r", c->channel, c->name);
          appenddirect(cp, buffer);
        }
        if (c->operator) {
          sprintf(buffer, "/\377\200OPER conversd -1 %s\r", c->name);
          appenddirect(cp, buffer);
        }
      }
    }
  }
  for (ch = channels; ch; ch = ch->next) {
    if (ch->topic)
    {
#ifdef CONV_TOPIC
      sprintf(buffer, "/\377\200TOPI %s %s %ld %d %s\n"
                    /* Wenn kein Call gesetzt, conversd setzen. */
                    , (*ch->name ? ch->name : "conversd")
                    /* User Hostname. */
                    , myhostname
                    /* Zeit. */
                    , (long)ch->time
                    /* Kanal. */
                    , ch->chan
                    /* Topic. */
                    , ch->topic);
#else
      sprintf(buffer, "/\377\200TOPI conversd %s %ld %d %s\r", myhostname, (long)ch->time, ch->chan, ch->topic);
#endif /* CONV_TOPIC */
      appenddirect(cp, buffer);
    }
    /* Dies wuerde zu 2 unterschiedlichen Modes zwischen den Teilnetzen
       der sich verbindenden Hosts fuehren
    sprintf(buffer, "/\377\200MODE %d -sptiml+%s\r", ch->chan, flags);
       folglich senden wir nur die gesetzten Flags, wenn welche gesetzt sind
    */
    flags = strlwr(getflags(ch->flags));
    if (*flags) {
      sprintf(buffer, "/\377\200MODE %d +%s\r", ch->chan, flags);
      appenddirect(cp, buffer);
    }
  }
}

/*---------------------------------------------------------------------------*/

#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn -par
#endif
static void h_invi_command(CONNECTION *cp)
{

  char *fromname, *toname;
  WORD channel;

  fromname = getarg(NULL, GET_NXTLC);
  toname = getarg(NULL, GET_NXTLC);
  channel = atoi(getarg(NULL, GET_NXTLC));
  send_invite_msg(fromname, toname, channel);
}

#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn .par
#endif
/*---------------------------------------------------------------------------*/

static void h_link_command(CONNECTION *cp)
{
  char buffer[256];
  char *user, *host;

  user = getarg(NULL, GET_NXTLC);
  host = getarg(NULL, GET_NXTCS);
  if (!Strcmp (myhostname, host)) {
    if (!*user)
        return;

    clear_locks();
#ifdef SPEECH
    sprintf(buffer, speech_message(111), myhostname);
#else
    sprintf(buffer, "*** Links at %s", myhostname);
#endif
    send_msg_to_user("conversd", user, buffer);
    disp_links(NULLCONNECTION, user);
  } else {                                  /* dies macht tpp nicht */
    sprintf(buffer, "/\377\200LINK %s %s", user, host);
    strcpy(cnvinbuf, buffer);
    h_unknown_command(cp);
  }
}

/*---------------------------------------------------------------------------*/

static void h_oper_command(CONNECTION *cp)
{
  char       *toname, *fromname;
  WORD       channel;
  CONNECTION *p;
  CLIST      *cl;

  fromname = getarg(NULL, GET_NXTLC);
  channel  = atoi(getarg(NULL, GET_NXTLC));
  toname   = getarg(NULL, GET_NXTLC);
  cp->locked = 1;

  for (p = connections; p; p = p->next) {
    if (Strcmp(p->name, toname))
      continue;

    if (channel == -1) {
      if (p->type == CT_USER) {
        p->operator = 1;
#ifndef L1IRC
        send_opermsg(toname, p->host, fromname, channel);
#else
        send_opermsg(toname, p->host, fromname, channel, 0);
#endif /* L1IRC */
      }
    } else {
      if (p->channel == channel) {
        p->channelop = 1;
      }
      for (cl = p->chan_list; cl; cl = cl->next) {
        if (cl->channel == channel) cl->channelop = 1;
      }
#ifndef L1IRC
      send_opermsg(toname, p->host, cp->name, channel);
#else
      send_opermsg(toname, p->host, cp->name, channel, 0);
#endif /* L1IRC */
    }
  }
}

/*---------------------------------------------------------------------------*/

static void h_ping_command(CONNECTION *cp)
{
  PERMLINK *l;
  char buffer[128];

  l = permlink_of(cp);
  if (l) {
    sprintf(buffer, "/\377\200PONG %ld\r", (long)l->txtime);
    appenddirect(cp, buffer);
    send_proto("cmds", "TX to %s %s", cp->name, buffer+3);
  }
}

/*---------------------------------------------------------------------------*/

static void h_pong_command(CONNECTION *cp)
{
  PERMLINK *l;

  l = permlink_of(cp);
  if (l) {
    l->rxtime = atol(getarg(NULL, GET_NXTLC));
    l->txtime = max(currtime - l->testwaittime, 1L);
    if (l->rxtime == 0L) l->rxtime = l->txtime;
    if ((labs(l->rxtime) > 20001L) || (labs(l->txtime) > 20001L)) {
      l->rxtime = 0;
      l->txtime = 0;
    }
    l->testnexttime = l->testwaittime + max(min(60L * (l->txtime), 7200L), 120L);
    /* I hacked this because of it's nasty behave - rewrite of the whole stuff is in progress */
    update_destinations(l, l->name,
                        (l->rxtime == -1L) ? l->txtime : (l->rxtime + l->txtime) / 2L,
                        cp->rev);
  }
}

/*---------------------------------------------------------------------------*/

#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn -par
#endif
static void h_rout_command(CONNECTION *cp)
{
  char *dest, *user;
  DESTINATION *d;
  char buffer[128];
  WORD ttl;

  dest = getarg(NULL, GET_NXTCS);
  user = getarg(NULL, GET_NXTLC);
  ttl = atoi(getarg(NULL, GET_NXTLC));
  clear_locks();
  for (d = destinations; d; d = d->next) {
    if (d->rtt && d->link && !Strcmp(d->name, dest)) {
      sprintf(buffer, "*** route: %s (%ld) %s -> %s", myhostname,
                      (long)((d->link->rxtime + d->link->txtime) / 2L),
                      d->link->cname, dest);
      send_msg_to_user("conversd", user, buffer);
      if (ttl && Strcmp(d->link->name, dest)) {
        ttl--;
        sprintf(buffer, "/\377\200ROUT %s %s %d\r", dest, user, ttl);
        appenddirect(d->link->connection, buffer);
        send_proto("cmds", "TX to %s %s", d->link->connection->name, buffer+3);
      }
    }
  }
}
#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn .par
#endif
/*---------------------------------------------------------------------------*/

static void h_topi_command(CONNECTION *cp)
{
  char *fromname, *hostname, *text;
  WORD channel;
  time_t time;

  fromname = getarg(NULL, GET_NXTLC);
  hostname = getarg(NULL, GET_NXTCS);
  time = atol(getarg(NULL, GET_NXTLC));
  channel = atoi(getarg(NULL, GET_NXTLC));
  text = getarg(NULL, GET_ALL);
  cp->locked = 1;
#ifndef CONVNICK
  send_topic(fromname, hostname, time, channel, text);
#else
  send_topic(fromname, fromname, hostname, time, channel, text);
#endif /* CONVNICK */
}

/*---------------------------------------------------------------------------*/

#ifdef CONVNICK
static void h_uadd_command(CONNECTION *cp)
{
  char *name, *host, *data, *nickname, *channel;
  CONNECTION *p;

  name = getarg(NULL, GET_NXTLC);
  host = getarg(NULL, GET_NXTLC);
  nickname = getarg(NULL, GET_NXTCS);
  channel = getarg(NULL, GET_NXTLC);
  data = getarg(NULL, GET_ALL);

  if (*data == '~' && data[1] == '~')
    *data = 0;

  for (p = connections; p ; p = p->next)
  {
    if (  (!strncasecmp(p->name, name, NAMESIZE))
        &&(!strncasecmp(p->host, host, NAMESIZE)))
    {
      if (!strncasecmp(p->OldNickname /* Ist alter und neuer Nickname gleich. */
                      , nickname
                      , NAMESIZE))
        break;                               /* Keine Aenderung durchfuehren. */

      /* Alten Nickname sichern. */
      strncpy(p->OldNickname, p->nickname, NAMESIZE);
      strncpy(p->nickname, nickname, NAMESIZE);
      setstring(&(p->pers), data, 256);
#ifdef L1IRC
      SendIrcNick(p);
#endif /* L1IRC */
      break;
    }
  }
}
#endif

/*---------------------------------------------------------------------------*/

static void h_udat_command(CONNECTION *cp)
{
  char *fromname, *hostname, *text;
  CONNECTION *p;

  fromname = getarg(NULL, GET_NXTLC);
  hostname = getarg(NULL, GET_NXTCS);
  text = getarg(NULL, GET_ALL);
  if (!strcmp(text, "@"))              /* wir verwenden keinen @ mehr */
    text = "";
  cp->locked = 1;
  for (p = connections; p; p = p->next) {
    if (p->type == CT_USER && !Strcmp(p->name, fromname) &&
        !Strcmp(p->host, hostname) && strcmp(p->pers?p->pers:"", text)) {
      setstring(&(p->pers), text, 256);
#ifndef CONVNICK
      send_persmsg(fromname, hostname, p->channel, text, currtime);
#else
      send_persmsg(fromname, (*p->nickname ? p->nickname : fromname), hostname, p->channel, text, currtime);
#endif /* CONVNICK */
    }
  }
}

/*---------------------------------------------------------------------------*/

#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn -par
#endif
static void h_umsg_command(CONNECTION *cp)
{
  char *fromname, *toname, *text;

  fromname = getarg(NULL, GET_NXTLC);
  if (strlen(fromname) > 64)  /* 1. Schutz vor Nicknames */
    fromname[64] = NUL;
  toname = getarg(NULL, GET_NXTLC);
  text = getarg(NULL, GET_ALL);
  if (*text) send_msg_to_user(fromname, toname, text);
}
#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn .par
#endif

/*---------------------------------------------------------------------------*/

/* this command simply passes on any host requests that we don't understand,
   since our neighbors MIGHT understand them (von tpp uebernommen) */

static void h_unknown_command(CONNECTION *cp)
{
  CONNECTION *p;

  for (p = connections; p; p = p->next)
    if (p->type == CT_HOST && p != cp) {
       appenddirect(p, cnvinbuf);
       appenddirect(p, "\r");
       send_proto("cmds", "TX to %s %s", p->name, cnvinbuf+3);
    }
}

/*---------------------------------------------------------------------------*/
#ifdef CONVERS_NO_NAME_OK
#else
static BOOLEAN name_ok(char *call)
{
  char tmp[L2IDLEN], buffer[12], *bp;
  WORD n, bcnt;

  n = (WORD) strlen(call);
  if (n < 4 || n > 9)
      return(FALSE);

  strcpy(buffer, call);
  if ((bp = strchr(buffer, '-')) != NULL) /* auch Buchstaben-SSID kuerzen(-u) */
    *bp = '\0';
  bp = buffer;
  bcnt = (WORD) strlen(bp);
  return(getcal(&bcnt, &bp, TRUE, tmp) == YES);
}
#endif

static BOOLEAN host_ok(char *call)
{
  char *c;

  if ((WORD)strlen(call) < 4)
      return(FALSE);

  c = call;
  while (*c) {                         /* nur 0...9 A...Z a...z . - */
    if (!(isalnum(*c) || *c == '.' || *c == '-'))
        return(FALSE);

    c++;
  }

  c = call;
  while (*c)                            /* mindestens ein Buchstabe */
    if (isalpha(*c++))
        return(TRUE);

    return(FALSE);
}

/*---------------------------------------------------------------------------*/

static void h_user_command(CONNECTION *cp)
{
  char *host;
  char *name;
  char *pers;
  WORD newchannel;
  WORD oldchannel;
  CONNECTION *p;
  CHANNEL *ch;
  PERMLINK *l;
  WORD users_left;
  time_t time;
#ifdef CONVNICK
  char nick[NAMESIZE + 1];
#endif

  name = getarg(NULL, GET_NXTLC);
  host = getarg(NULL, GET_NXTCS);

#ifdef CONVERS_NO_NAME_OK
  if (host_ok(host) != TRUE)
#else
  if (   name_ok(name) != TRUE
      || host_ok(host) != TRUE)
#endif
      return;

  if (   (l = permlink_of(cp)) != NULL            /* Looperkennung */
      && is_looped(l, host)) {
    l->locked = 1;                                 /*   verriegeln */
    bye_command2(cp, "loop detect");               /* und abwerfen */
    return;
  }

  time = atol(getarg(NULL, GET_NXTLC));
  if (time == 0) time = currtime;
  oldchannel = atoi(getarg(NULL, GET_NXTLC));
  newchannel = atoi(getarg(NULL, GET_NXTLC));
  if (   oldchannel < -1
      || newchannel < -1
      || (oldchannel == -1 && newchannel == -1)) /* einige WAMPEn tun dies */
      return;

  pers = getarg(NULL, GET_ALL);

#ifdef CONVNICK
  memset(nick, 0, sizeof(nick));
#endif /* CONVNICK */

  for (p = connections; p; p = p->next)
  {
    if (p->type == CT_USER)
    {
      if ((p->via == cp) &&
          !Strcmp(p->name, name) &&
          !Strcmp(p->host, host))
      {
        strncpy(nick, p->nickname, NAMESIZE);
        nick[NAMESIZE] = 0;

        if (p->channel == oldchannel)
          break;
      }
    }
  }
  if (!p) {
    p = (CONNECTION *) calloc(1, sizeof(CONNECTION));
    if (p) {
      p->type = CT_USER;
      Strcpy(p->name, name);
#ifdef CONVNICK
      if (nick[0] == FALSE)
      {
        memset(p->nickname, 0, NAMESIZE);
        memset(p->OldNickname, 0, NAMESIZE);
      }
#endif
      Strcpy(p->host, host);
      p->via = cp;
      p->next = connections;
      connections = p;
     }
  }
  if (p) {
    p->time = time;
    p->mtime = currtime;
    if (*pers && strcmp(pers, "@"))
      setstring(&(p->pers), pers, 256);
    if ((p->channel = newchannel) < 0) {
      p->type = CT_CLOSED;
    }
  }

#ifdef CONVNICK
  if (*p->nickname)
  {
    strncpy(nick, p->nickname, NAMESIZE);
    nick[NAMESIZE] = 0;
  }
  else
  {
    /* ggf Nickname aus Profil laden. */
    if (GetNickname(p))
    {
      /* Nickname sichern. */
      strncpy(nick, p->nickname, NAMESIZE);
      nick[NAMESIZE] = 0;
    }
  }
#endif /* CONVNICK */

 users_left = count_user(oldchannel);
  if (!users_left) destroy_channel(oldchannel);
  for (ch = channels; ch; ch = ch->next) {
    if (ch->chan == newchannel) break;
  }
  if (p && !ch && (newchannel != -1)) {
    ch = ins_channel(newchannel);
  }
#ifndef CONVNICK
  send_user_change_msg(name, host, oldchannel, newchannel, pers, time);
#else
  send_user_change_msg(name, nick, host, oldchannel, newchannel, pers, time);
#endif /* CONVNICK */
}

/*---------------------------------------------------------------------------*/

#else
#ifndef __LINUX__
#pragma warn -par
#endif
void bye_command2(CONNECTION *cp, char *reason)
{
}

#ifndef __LINUX__
#pragma warn .par
#endif
#endif


#ifdef CONVERS_SYSINFO
/* Sich als Sysop anmelden */
static void sysinfo_command(CONNECTION *cp)
{
  char buffer[255];

  sprintf(buffer, "*** conversd PingPong-Release %5.5s (TNN)\r"
                  , strchr(REV, ':')+2);
 appenddirect(cp, buffer);
 read_xhelp("SYSI", cp);
#ifdef CONNECTTIME
#ifdef SPEECH
  sprintf(buffer, speech_message(342), ConnectTime(tic10 / 100));
#else
  sprintf(buffer,  "Convers rennt seit: %s\r", ConnectTime(tic10 / 100));
#endif /* SPEECH */
#endif /* CONNECTTIME */
 appenddirect(cp, buffer);
}
#endif

#ifdef CONVERS_HOSTNAME
static void hostname_command(CONNECTION *cp)
{
  char buffer[255];
  char *arg;
  static char HostName[10 + 1];

  arg = getarg(NULL, GET_ALL);

  /* Hostname aendern darf nur der Sysop! */
  if (cp->operator != 2)
  {
    /* User bekommt eine Meckermeldung. */
#ifdef SPEECH
    appenddirect(cp, speech_message(68));
#else
    appenddirect(cp, "*** You must be an operator!\r");
#endif
  }

  if (arg[0] == FALSE)
  {
#ifdef SPEECH
    sprintf(buffer, speech_message(344),myhostname);
#else
    sprintf(buffer,"*** Hostname is %s.\r",myhostname);
#endif
    appenddirect(cp, buffer);
    return;
  }
  else
    {
      if (strlen(arg) > 9)
      {
#ifdef SPEECH
        sprintf(buffer, speech_message(3));
#else
        sprintf(buffer,"*** is too long. the Convers hostname (max.9 indications)!\r");
#endif
        appenddirect(cp, buffer);
        return;
      }
      strncpy(HostName, arg, 10);
      myhostname = HostName;
#ifdef SPEECH
      sprintf(buffer, speech_message(4), myhostname);
#else
      sprintf(buffer,"*** Convers hostname successfully changed in %s.\r",myhostname);
#endif
      appenddirect(cp, buffer);
      return;
    }
}

#endif

/* End of $RCSfile$ */
