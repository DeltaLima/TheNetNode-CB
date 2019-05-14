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
/* File src/cvs_cvsd.c (maintained by: DL1XAO)                          */
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
/* modified for use with TheNetNode by DL1XAO                       */
/* hierdrin befinden sich alle Senderoutinen und sonstiges,
   welches sich nicht mit Datenstrukturen von TNN befasst           */
#include "tnn.h"

#ifdef PPCONVERS

#include "conversd.h"

static char *persfile;
static WORD isonchannel __ARGS((CONNECTION  *cp, char *user, char *host));

/*---------------------------------------------------------------------------*/

void appenddirect(CONNECTION *cp, const char *string)
{
  if (cp->up)
    putcvsstr(cp, (char *)string);
}

void appendformline(CONNECTION *cp, char *prefix, char *text)
{
  char buf[258], *d, *s1, *s2;
  WORD prefixlen, l, linelen;

  if (cp->up) {
    text = convertout(cp->charset_out, (char *)text);
    linelen = cp->width - 1;
    l = prefixlen = (WORD)strlen(prefix);
    strcpy(buf, prefix);
    d = buf + prefixlen;
    *d++ = ' ';

    while (*text) {

      while (isspace(uchar(*text)))
        text++;

      if (*text) {
        while (*text && !isspace(uchar(*text)) && l < linelen) {
          *d++ = *text++;
          l++;
        }
        while (isspace(uchar(*text)) && l < linelen) {
          s1 = text;
          while (isspace(uchar(*s1)))
           s1++;
          s2 = s1;
          while (*s2 && !isspace(uchar(*s2)))
           s2++;
          if (l + (WORD)(s2 - text) < linelen) {
            while (text < s2) {
              *d++ = *text++;
              l++;
            }
          }
          else if ((WORD)(s2 - s1) > (linelen - prefixlen)) {
            while (l < linelen) {
              *d++ = *text++;
              l++;
            }
          }
          else
            text = s1;
        }
        if (*(d-1) != '\r')
          *d++ = '\r';
        *d = '\0';
        putcvsstr(cp, buf);
        for (l = 0, d = buf; l < prefixlen; l++)
          *d++ = ' ';
      }
    }
  }
}

void appendstring(CONNECTION *cp, const char *string)
{
  char *p_string;

  if (cp->up && *string) {

    if (cp->type == CT_USER)
      p_string = convertout(cp->charset_out, (char *)string);
    else
      p_string = (char *)string;

    putcvsstr(cp, (char *)p_string);
  }
}

/*---------------------------------------------------------------------------*/

/* zusammengefasste Variante von appendprompt/appendc */

void appendprompt(CONNECTION *cp, const WORD ast)
{
  char x[3], *p;

#ifdef L1IRC
  if (cp->IrcMode)
    return;
#endif /* L1IRC */

  if (ast == 2) {
    p = x;
    if (cp->prompt[0])
      *p++ = cp->prompt[0];
    if (cp->prompt[3])
      *p++ = cp->prompt[3];
    *p = '\0';
    if (p != x)
      appenddirect(cp, x);
  }
  else if ((*x = cp->prompt[(*cp->query) ? 1 : 2]) != 0) {
    x[1] = '\0';
    appenddirect(cp, x);
  }
  else if (ast)
    appenddirect(cp, "***\r");
}

/*---------------------------------------------------------------------------*/

void destroy_channel(WORD number)
{
  CHANNEL *ch, *ch1;

  ch1 = NULLCHANNEL;
  for (ch = channels; ch; ch = ch->next) {
    if (ch->chan == number) {
      if (ch1) {
        ch1->next = ch->next;
      } else {
        channels = ch->next;
      }
      if (ch->topic)
        free(ch->topic);

#ifdef CONV_TOPIC
      /* Call loeschen. */
      ch->name[0] = 0;
#endif
      free(ch);
      break;
    }
    ch1 = ch;
  }
}

/*---------------------------------------------------------------------------*/

PERMLINK *permlink_of(CONNECTION *cp)
{
  WORD pl;
  PERMLINK *l;

  for (pl = 0; pl < MAXCVSHOST; pl++) {
    l = permarray[pl];
    if (l && l->connection == cp)
     return(l);
  }
  return(NULL);
}

/*---------------------------------------------------------------------------*/

void free_closed_connections()
{
  CONNECTION *cp, *p;
  PERMLINK *l;
  WORD to;

  for (p = NULLCONNECTION, cp = connections; cp; ) {
    l = cp->via ? NULL : permlink_of(cp);
    to = 0;
    if (l && cp->type == CT_UNKNOWN && cp->time + l->waittime - 5 < currtime) {
      send_proto("cvsd", "timeout for %s", cp->name);
      to = 1;
    }

    if (   cp->type == CT_CLOSED
        || to
        ||(!l && cp->type == CT_UNKNOWN))
    {
      if (p)
      {
        p->next = cp->next;
        free_connection(cp);
        cp = p->next;
      }
      else
        {
          connections = cp->next;
          free_connection(cp);
          cp = connections;
        }
    }
    else
      {
        p = cp;
        cp = cp->next;
      }
  }
}

/*---------------------------------------------------------------------------*/

char *getarg(char *line, WORD mode)
{
  char *arg;
  WORD c;
  static char *p;

#ifdef CONVNICK
  int mke = FALSE;
#endif

  if (line) p = line;
  while (isspace(uchar(*p))) p++;
  if (mode == GET_ALL) return(p);
  arg = p;
  if (mode == GET_NXTCS)
    while (*p && !isspace(uchar(*p)))
      p++;
  else
    while (*p && !isspace(uchar(*p)))
    {
#ifdef CONVNICK
      if (mke == TRUE || (*p == ':'))
      {
        c = (uchar(*p));
        mke = TRUE;
      }
      else
#endif
        c = tolower(uchar(*p));
#ifdef __WIN32__
      *p++ = (char)c;
#else
      *p++ = c;
#endif /* WIN32 */
    }

  if (*p)
    *p++ = '\0';

  return(arg);
}

/*---------------------------------------------------------------------------*/

char *ts(time_t gmt)
{
  static char buffer[80];
  static char monthnames[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  struct tm *tm;

  tm = localtime(&gmt);
  if (gmt + 24L * 60L * 60L > currtime)
    sprintf(buffer, " %2d:%02d", tm->tm_hour, tm->tm_min);
  else
    sprintf(buffer, "%-3.3s %2d", monthnames + 3 * tm->tm_mon, tm->tm_mday);
  return(buffer);
}

/*---------------------------------------------------------------------------*/

void ts2(void)
{
  struct tm *tm;

  tm = localtime(&currtime);
  sprintf(timestamp, "*** (%2d:%02d) ", tm->tm_hour, tm->tm_min);
}

/*---------------------------------------------------------------------------*/

char *ts3(time_t seconds, char *buffer)
{
  if (seconds < 100L) {
    sprintf(buffer, "%ds", (WORD)seconds);
  } else if (seconds <6000L) {
    sprintf(buffer, "%dm", (WORD)(seconds/60L));
  } else if (seconds <360000L) {
    sprintf(buffer, "%dh", (WORD)(seconds/3600L));
  } else {
    sprintf(buffer, "%dd", (WORD)(seconds/86400L));
  }
  return(buffer);
}

/*---------------------------------------------------------------------------*/

char *ts4(time_t seconds)
{
  time_t days, hours, minutes;
  static char buffer[64];
  char *bp;

  days = seconds / 86400L;
  seconds -= days * 86400L;
  hours = seconds / 3600L;
  seconds -= hours * 3600L;
  minutes = seconds / 60L;
  seconds -= minutes * 60L;

  bp = buffer;
  if (days)
    bp += sprintf(bp, "%d days, ", (WORD)days);
  if (days+hours)
    bp += sprintf(bp, "%d hours, ", (WORD)hours);
  if (days+hours+minutes)
    bp += sprintf(bp, "%d minutes, ", (WORD)minutes);
  sprintf(bp, "%d seconds.", (WORD)seconds);

  return(buffer);
}

/*---------------------------------------------------------------------------*/

WORD count_user(WORD channel)
{
  CONNECTION *p;
  CLIST *cl;
  WORD n = 0;

  for (p = connections; p; p = p->next) {
    if (p->type == CT_USER) {
      if (p->via && (p->channel == channel)) {
        n++;
      } else {
        for (cl = p->chan_list; cl; cl = cl->next) {
          if (cl->channel == channel) {
            n++;
            break;
          }
        }
      }
    }
  }
  return(n);
}

/*---------------------------------------------------------------------------*/

void clear_locks()
{
  CONNECTION *p;

  for (p = connections; p; p = p->next) p->locked = 0;
}

/*---------------------------------------------------------------------------*/

static WORD isonchannel(CONNECTION  *cp, char *user, char *host)
{
  CONNECTION *p;
  CLIST      *cl, *cl1;

  for (p = connections; p; p = p->next)
    if (p->type == CT_USER && p != cp)
      if (!Strcmp(p->name, user) && !Strcmp(p->host, host))
        for (cl = cp->chan_list; cl; cl = cl->next) {
          if (p->channel == cl->channel)
            return(1);
          for (cl1 = p->chan_list; cl1; cl1 = cl1->next)
            if (cl1->channel == cl->channel)
              return(1);
        }
  return(0);
}

/*---------------------------------------------------------------------------*/

#ifndef CONVNICK
void send_awaymsg(char *fromname,.char *hostname,
#else
void send_awaymsg(char *fromname, char *fromnickname, char *hostname,
#endif /* CONVNICK */
                  time_t time, char *text)
{
  char buffer[2048];
  char namefilt[68], *c;
  register CONNECTION *p;

  sprintf(namefilt, " %s ", fromname);
  if ((c = strchr(namefilt, ':')) != NULL) {
    *c++ = ' ';
    *c = NUL;
  }
  for (p = connections; p; p = p->next) {
#ifdef L1IRC
    if (p->IrcMode)
    {
      if (!p->locked && (p->type == CT_HOST || (p->type == CT_USER && !p->via && p->verbose && isonchannel(p, fromname, hostname))))
      {
        sprintf(buffer, ":%s!%s@%s MODE %s %ca%s%s\n", fromname, fromname, hostname, fromname, *text ? '+' : '-', (*text ? " :" : ""), (*text ? text : ""));
        appendstring(p, buffer);
        p->locked = 1;
        continue;
      }
    }
#endif /* L1IRC */
    if (p->type == CT_HOST) {
      if (!p->locked) {
        sprintf(buffer, "/\377\200AWAY %s %s %ld %s\r", fromname, hostname, (long)time, text);
        appenddirect(p, buffer);
        p->locked = 1;
        send_proto("cvsd", "TX to %s %s", p->name, buffer+3);
      }
    } else {
      if (!p->via && !p->locked) {
        if (isonchannel(p, fromname, hostname)) {
          if (!(p->filter && strstr(p->filter, namefilt))) {
            appendprompt(p, 2);
            if (*text != '\0')
#ifdef SPEECH
                sprintf(buffer, speech_message(6), timestamp, fromname, hostname, text);
#else
                sprintf(buffer, "%s%s@%s has gone away:\r    %s\r", timestamp, fromname, hostname, text);
#endif
            else
#ifdef SPEECH
              sprintf(buffer, speech_message(7), timestamp, fromname, hostname);
#else
              sprintf(buffer, "%s%s@%s is back again.\r", timestamp, fromname, hostname);
#endif
#ifdef L1IRC
            if (p->IrcMode == TRUE)                            /* IRC-Client. */
            {
              if (*text != '\0')                                 /* Abmelden. */
                sprintf(buffer, ":%s 301 %s %s : %s\n"         /* IRC-Meldung */
                              , myhostname                    /* vorbereiten. */
                              , fromname
                              , (*fromnickname ? fromnickname : fromname)
                              , text);
              else                                               /* Anmelden. */
                sprintf(buffer, ":%s 320 %s %s : is back\n"    /* IRC-Meldung */
                              , myhostname                    /* vorbereiten. */
                              , fromname
                              , (*fromnickname ? fromnickname : fromname));
            }
#endif /* L1IRC */
            appendstring(p, buffer);
            appendprompt(p, 0);
          }
        }
        p->locked = 1;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

#ifndef L1IRC
void send_mode(CHANNEL *ch)
#else
void send_mode(CHANNEL *ch, CONNECTION *cp, WORD oldflags)
#endif /* L1IRC */
{
  register CONNECTION *p;
  char buffer[2048];
  char *flags;
#ifdef L1IRC
  char flags_irc[16];
  char oldflags_irc[16];

  flags_irc[0]    = '\0';
  oldflags_irc[0] = '\0';

  if (ch->flags & M_CHAN_S)
  {
    if (!(oldflags & M_CHAN_S))
      strcat(flags_irc, "p");
  }

  if (ch->flags & M_CHAN_P)
  {
    if (!(oldflags & M_CHAN_P))
      strcat(flags_irc, "i");
  }

  if (ch->flags & M_CHAN_T)
  {
    if (!(oldflags & M_CHAN_T))
      strcat(flags_irc, "t");
  }

  if (ch->flags & M_CHAN_I)
  {
    if (!(oldflags & M_CHAN_I))
      strcat(flags_irc, "s");
  }

  if (ch->flags & M_CHAN_M)
  {
    if (!(oldflags & M_CHAN_M))
      strcat(flags_irc, "m");
  }

  if ((oldflags & M_CHAN_S) && !(ch->flags & M_CHAN_S))
    strcat(oldflags_irc, "p");

  if ((oldflags & M_CHAN_P) && !(ch->flags & M_CHAN_P))
    strcat(oldflags_irc, "i");

  if ((oldflags & M_CHAN_T) && !(ch->flags & M_CHAN_T))
    strcat(oldflags_irc, "t");

  if ((oldflags & M_CHAN_I) && !(ch->flags & M_CHAN_I))
    strcat(oldflags_irc, "s");

  if ((oldflags & M_CHAN_M) && !(ch->flags & M_CHAN_M))
    strcat(oldflags_irc, "m");
#endif /* L1IRC */

  flags = getflags(ch->flags);
  for (p = connections; p; p = p->next) {
#ifdef L1IRC
    if (p->IrcMode == FALSE)
    {
#endif /* L1IRC */
    if ((p->type == CT_HOST) && !p->locked) {
      sprintf(buffer, "/\377\200MODE %d -sptiml+%s\r", ch->chan, flags);
      appenddirect(p, buffer);
      send_proto("cvsd", "TX to %s %s", p->name, buffer+3);
    }
#ifdef L1IRC
    }
    else
    {
      struct clist *cl2;
      char         *fromname;

      if ((p->type == CT_HOST || p->type == CT_USER))
      {
        if (p->type == CT_USER && !(p->operator == 2 && p->verbose))
        {
          if (p->channel != ch->chan)
          {
            for (cl2 = p->chan_list; cl2; cl2 = cl2->next)
            {
              if (cl2->channel == ch->chan)
                break;
            }

            if (!cl2)
              continue;
          }
        }

        fromname = ((cp) ? ((cp->type == CT_HOST) ? cp->name : cp->nickname) : myhostname);
        sprintf(buffer, ":%s MODE #%d %s%s%s%s\n", fromname, ch->chan, (*oldflags_irc) ? "-" : "", oldflags_irc, (*flags_irc) ? "+" : "", flags_irc);
        appendstring(p, buffer);
      }
    }

    p->locked = 1;
#endif /* L1IRC */
  }
}

/*---------------------------------------------------------------------------*/

#ifndef L1IRC
void send_opermsg(char *toname, char *hostname, char *fromname, WORD channel)
#else
void send_opermsg(char *toname, char *hostname, char *fromname, WORD channel, int status_really_changed)
#endif /* L1IRC */
{
  char buffer[2048];
  register CONNECTION *p;
  CLIST *cl;

  for (p = connections; p; p = p->next) {
    if (!p->locked) {
#ifdef L1IRC
      int ison = 0;

      if (channel >= 0)
      {
        ison = 0;

        if (p->channel == channel)
          ison = 1;
        else
        {
          for (cl = p->chan_list; cl; cl = cl->next)
          {
            if (cl->channel == channel)
            {
              ison = 1;
              break;
            }
          }
        }
      }

      if (p->IrcMode == TRUE)
      {
        if (channel == EOF)
        {
          if (p->type == CT_HOST || (p->type == CT_USER && (p->verbose || !strcasecmp(toname, p->name)) && status_really_changed))
          {
            sprintf(buffer, ":%s MODE %s +o\n", fromname, toname);
            appendstring(p, buffer);
          }
        }
        else
        {
          if (p->type != CT_USER && p->type != CT_HOST)
            continue;

          if (p->type == CT_USER)
          {
            if (p->via || !status_really_changed)
              continue;

            if (!ison)
            {
              if (!p->verbose)
                continue;
            }
          }

          sprintf(buffer, ":%s MODE #%d +o %s\n", fromname, channel, toname);
          appendstring(p, buffer);
        }
      }
#endif /* L1IRC */
      if (p->type == CT_HOST) {
        sprintf(buffer, "/\377\200OPER %s %d %s\r", fromname, channel, toname);
        appenddirect(p, buffer);
        p->locked = 1;
        send_proto("cvsd", "TX to %s %s", p->name, buffer+3);
      } else {
        if (p->type == CT_USER) {
          if (!p->via && (channel != -1)) {
            if (!Strcmp(p->name,toname)) {
              for (cl = p->chan_list; cl; cl = cl->next) {
                if (cl->channel == channel) {
                  appendprompt(p, 2);
#ifdef SPEECH
                  sprintf(buffer, speech_message(8),
                          timestamp, fromname, channel);
#else
                  sprintf(buffer, "%s%s made you a channel operator for channel %d\r",
                          timestamp, fromname, channel);
#endif
                  appenddirect(p, buffer);
                  appendprompt(p, 0);
                  break;
                }
              }
              p->locked = 1;
            } else {
              if (p->verbose) {
                appendprompt(p, 2);
#ifdef SPEECH
                sprintf(buffer, speech_message(9),
                        timestamp, toname, hostname, channel);
#else
                sprintf(buffer, "%s%s@%s is now a channel operator for channel %d\r",
                        timestamp, toname, hostname, channel);
#endif
                appenddirect(p, buffer);
                appendprompt(p, 0);
                p->locked = 1;
              }
            }
          }
        }
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

#ifndef CONVNICK
void send_persmsg(char *fromname, char *hostname, WORD channel,
#else
void send_persmsg(char *fromname, char *fromnickname, char *hostname, WORD channel,
#endif /* CONVNICK */
                  char *text, time_t time)
{
  char buffer[2048];
  char namefilt[68], *c;
  register CONNECTION *p;
  CHANNEL *ch;
  char chan[128];
  WORD mychannel;
  CLIST *cl;

  if (!text)                        /* DL1XAO NULL Pointer abfangen */
    text = "";
  for (ch = channels; ch; ch = ch->next) {
    if (ch->chan == channel) break;
  }
#ifdef SPEECH
  sprintf(chan, speech_message(10), channel);
#else
  sprintf(chan, "channel %d", channel);
#endif
  if (ch) {
    if (ch->flags & M_CHAN_S) strcpy(chan, "secret channel");
    if (ch->flags & M_CHAN_I) strcpy(chan, "this invisible channel");
  }
  sprintf(namefilt, " %s ", fromname);
  if ((c = strchr(namefilt, ':')) != NULL) {
    *c++ = ' ';
    *c = NUL;
  }
  for (p = connections; p; p = p->next) {
    if (p->type == CT_HOST) {
      if (!p->locked) {
#ifndef L1IRC
        sprintf(buffer, "/\377\200USER %s %s %ld %d %d %s\r", fromname, hostname, (long)time, channel, channel, text);
#else
        if (p->IrcMode == FALSE)
          sprintf(buffer, "/\377\200USER %s %s %ld %d %d %s\r", fromname, hostname, (long)time, channel, channel, text);
        else
        {
          sprintf(buffer, "NICK %s %d %s %s 0 + :%s", fromname, (strcasecmp(hostname, myhostname) ? 1 : 0), fromname, hostname, text);
          buffer[2048] = 0;
        }
#endif /* L1IRC */
        appenddirect(p, buffer);
        p->locked = 1;
        send_proto("cvsd", "TX to %s %s", p->name, buffer+3);
      }
    } else {
      mychannel = -1;
      for (cl = p->chan_list; cl; cl = cl->next) {
        if (cl->channel == channel) {
          mychannel = channel;
          break;
        }
      }
      if ((p->type == CT_USER) && !p->locked && !p->via &&
          (p->verbose || (mychannel == channel))) {
        if (!(p->filter && strstr(p->filter, namefilt))) {
          appendprompt(p, 2);
          if (*text != '\0') {
#ifdef SPEECH
            sprintf(buffer, speech_message(11), timestamp, fromname, hostname, chan, text);
#else
            sprintf(buffer, "%s%s@%s on %s set personal text:\r    %s\r", timestamp, fromname, hostname, chan, text);
#endif
          } else {
#ifdef SPEECH
            sprintf(buffer, speech_message(12), timestamp, fromname, hostname, chan);
#else
            sprintf(buffer, "%s%s@%s on %s removed personal text.\r", timestamp, fromname, hostname, chan);
#endif
          }
#ifdef L1IRC
          if (p->IrcMode == TRUE)
          {
            sprintf(buffer, ":%s 311 %s %s %s %s * :%s\n"
                          , myhostname
                          , p->nickname
                          , fromname
                          , (*fromnickname ? fromnickname : fromname)
                          , hostname
                          , (*text && *text != '@') ? text : "");
            buffer[2048] = 0;
          }
#endif /* L1IRC */
          appendstring(p, buffer);
          appendprompt(p, 0);
        }
      }
      p->locked = 1;
    }
  }
}

/*---------------------------------------------------------------------------*/

#ifndef CONVNICK
void send_topic(char *fromname, char *hostname, time_t time,
#else
void send_topic(char *fromname, char *fromnickname, char *hostname, time_t time,
#endif /* CONVNICK */
                WORD channel, char *text)
{
  char buffer[2048];
  char namefilt[68], *c;
  register CONNECTION *p;
  register CHANNEL *ch;
  char chan[128];
  WORD mychannel, flags = 0;
  CLIST *cl;

  for (ch = channels; ch; ch = ch->next)
    if (ch->chan == channel) break;
  if (ch) {
    sprintf(namefilt, " %s ", fromname);
    if ((c = strchr(namefilt, ':')) != NULL) {
      *c++ = ' ';
      *c = NUL;
    }
#ifdef SPEECH
    sprintf(chan, speech_message(10), channel);
#else
    sprintf(chan, "channel %d", channel);
#endif
    flags = ch->flags;
    if (flags & M_CHAN_S) strcpy(chan, "secret channel");
    if (flags & M_CHAN_I) strcpy(chan, "this invisible channel");
#ifndef CONVTOPIC_FIX
    if (ch->time < time) {
#else
    if (ch->time <= time) {
#endif /* CONVTOPIC_FIX */
      setstring(&(ch->topic), text, 512);
      ch->time = time;
#ifdef CONV_TOPIC
      /* Call sichern. */
      strncpy(ch->name, fromname, NAMESIZE + 1);
      /* Nullzeichen setzen. */
      ch->name[sizeof(ch->name)-1] = 0;
#endif

      for (p = connections; p; p = p->next) {
        if (p->type == CT_HOST) {
          if (!p->locked) {
#ifndef L1IRC
            sprintf(buffer, "/\377\200TOPI %s %s %ld %d %s\r", fromname, hostname, (long)time, channel, text);
#else
            if (p->IrcMode == FALSE)
              sprintf(buffer, ":%s!%s@%s TOPIC #%d :%s\n", fromname, fromname, hostname, channel, text);
          else
            sprintf(buffer, "/\377\200TOPI %s %s %ld %d %s\r", fromname, hostname, (long)time, channel, text);
#endif /* L1IRC */
            appenddirect(p, buffer);
            p->locked = 1;
            send_proto("cvsd", "TX to %s %s", p->name, buffer+3);
          }
        } else {
          mychannel = -1;
          for (cl = p->chan_list; cl; cl = cl->next) {
            if (cl->channel == channel) {
              mychannel = channel;
              break;
            }
          }
          if ((p->type == CT_USER) && !p->locked && !p->via &&
                ((!(ch->flags & M_CHAN_I) && p->verbose) || (mychannel == channel) )) {
            if (!(p->filter && strstr(p->filter, namefilt))) {
              appendprompt(p, 2);
              if (*text != '\0')
#ifdef SPEECH
                  sprintf(buffer, speech_message(13),
                                timestamp, fromname, hostname, chan, text);
#else
                  sprintf(buffer, "%s%s@%s on %s set channel topic:\r               %s\r",
                                timestamp, fromname, hostname, chan, text);
#endif
              else
#ifdef SPEECH
                sprintf(buffer, speech_message(14),
                                timestamp, fromname, hostname, chan);
#else
                  sprintf(buffer, "%s%s@%s on %s removed channel topic.\r",
                                timestamp, fromname, hostname, chan);
#endif
#ifdef L1IRC
              if (p->IrcMode == TRUE)
              {
                char IrcChannel[128];

                sprintf(IrcChannel, "#%d", channel);
                sprintf(buffer, ":%s!%s@%s TOPIC %s :%s\n", fromname, fromname, hostname, IrcChannel, text);
              }
#endif /* L1IRC */
              appendstring(p, buffer);
              appendprompt(p, 0);
            }
          }
          p->locked = 1;
        }
      }
    }
  }
}

#ifdef CONVNICK
/* Allen Convershosts den geaenderten Nickname mitteilen */
void send_uaddmsg(CONNECTION *cp)
{
  CONNECTION *p;
  char buffer[1024];

  /* Nur, wenn wir auch einen Nickname haben */
  if (cp->nickname[0] == '\0')
    return;

  for (p = connections; p; p = p->next) {
    /* Ist es ein Host ? */
    if (p->type == CT_HOST) {
      /* Wenn keine Sperre, dann senden wir den Nickname an den Host. */
      if (!p->locked) {
        /* Nickname nur senden wenn der Host das Flag gesendet hat. */
        if (p->features & FEATURE_NICK) {
          sprintf(buffer, "/\377\200UADD %s %s %s %d %s\r", cp->name, cp->host, cp->nickname, -1, cp->pers ? cp->pers : "~~");
          appenddirect(p, buffer);
          send_proto("cvsd", "TX to %s %s", p->name, buffer + 3);
        }
      }
    }
  }
}
#endif

/*---------------------------------------------------------------------------*/
#ifndef CONVNICK
void send_user_change_msg(char *name, char *host,
                          WORD oldchannel, WORD newchannel,
                          char *pers, time_t time)
#else
void send_user_change_msg(char *name, char *nick, char *host,
                          WORD oldchannel, WORD newchannel,
                          char *pers, time_t time)
#endif /* CONVNICK */
{
  char buffer[2048];
  CONNECTION *p;
  char oldchan[24], newchan[24], name2[64 + 1], *c;
  CHANNEL *ch;
  WORD oldflags = 0, newflags = 0;
  WORD mychannel;
  CLIST *cl;

  if (!pers)                       /* DL1XAO: NULL Pointer abfangen */
    pers = "";
#ifdef SPEECH
  sprintf(newchan, speech_message(10), newchannel);
  sprintf(oldchan, speech_message(10), oldchannel);
#else
  sprintf(newchan, "channel %d", newchannel);
  sprintf(oldchan, "channel %d", oldchannel);
#endif
  sprintf(name2, " %s ", name);
  if ((c = strchr(name2, ':')) != NULL) {
    *c++ = ' ';
    *c = NUL;
  }
  for (ch = channels; ch; ch = ch->next)
    if (ch->chan == newchannel) break;
  if (ch) {
    newflags = ch->flags;
    if (newflags & M_CHAN_S) {
#ifdef SPEECH
        strcpy(newchan, speech_message(15));
#else
        strcpy(newchan, "secret channel");
#endif
    }
    if (newflags & M_CHAN_I) {
#ifdef SPEECH
        strcpy(newchan, speech_message(16));
#else
        strcpy(newchan, "this invisible channel");
#endif
    }
  }
  for (ch = channels; ch; ch = ch->next)
    if (ch->chan == oldchannel) break;
  if (ch) {
    oldflags = ch->flags;
    if (oldflags & M_CHAN_S) {
#ifdef SPEECH
        strcpy(oldchan, speech_message(15));
#else
        strcpy(oldchan, "secret channel");
#endif
    }
    if (oldflags & M_CHAN_I) {
#ifdef SPEECH
      strcpy(oldchan, speech_message(16));
#else
      strcpy(oldchan, "this invisible channel");
#endif
    }
  }
  for (p = connections; p; p = p->next) {
    if (p->type == CT_USER && !p->via && !p->locked) {
      mychannel = -1;
      for (cl = p->chan_list; cl; cl = cl->next) {
        if (cl->channel == oldchannel) {
          mychannel = oldchannel;
          break;
        }
      }
      if ((newchannel == oldchannel) && (newchannel != -1)) {
        if ((p->verbose && !(newflags & M_CHAN_I)) || (mychannel == newchannel)) {
          if (!(p->filter && strstr(p->filter, name2))) {
            appendprompt(p, 2);
#ifdef L1IRC
            if (p->IrcMode == FALSE)
            {
#endif /* L1IRC */
            if (*pers && strcmp(pers, "@"))
#ifdef CONVNICK
                {
                if (*nick != NUL)
#ifdef SPEECH
                    sprintf(buffer, speech_message(17), timestamp, name, nick, host, newchan, pers);
#else
                    sprintf(buffer, "%s%s:%s@%s on %s set personal text:\r    %s\r", timestamp, name, nick, host, newchan, pers);
#endif
#ifdef SPEECH
            sprintf(buffer, speech_message(11), timestamp, name, host, newchan, pers);
#else
            sprintf(buffer, "%s%s@%s on %s set personal text:\r    %s\r", timestamp, name, host, newchan, pers);
#endif
                }
#else
            sprintf(buffer, "%s%s@%s on %s set personal text:\r    %s\r", timestamp, name, host, newchan, pers);
#endif
            else
#ifdef SPEECH
                sprintf(buffer, speech_message(12), timestamp, name, host, newchan);
#else
                sprintf(buffer, "%s%s@%s on %s removed personal text.\r", timestamp, name, host, newchan);
#endif
#ifdef L1IRC
            }
            else
              sprintf(buffer, ":%s 311 %s %s %s %s * :%s\n"
                            , myhostname
                            , name
                            , (*nick ? nick : name)
                            , name
                            , host
                            , (*pers && *pers != '@') ? pers : "");
#endif /* L1IRC */
            appendstring(p, buffer);
            appendprompt(p, 0);
          }
          p->locked = 1;
        }
      } else {
        if (oldchannel >= 0) {
          if ((!(oldflags & M_CHAN_I) && p->verbose) || (mychannel == oldchannel)) {
            appendprompt(p, 2);
            if (strlen(pers) < 2) {
#ifdef CONVNICK
                if (*nick != NUL)
#ifdef SPEECH
                    sprintf(buffer, speech_message(18), timestamp, name, nick, host, oldchan);
#else
                    sprintf(buffer, "%s%s:%s@%s left %s.\r", timestamp, name, nick, host, oldchan);
#endif
                else
#endif
#ifdef SPEECH
                    sprintf(buffer, speech_message(19), timestamp, name, host, oldchan);
#else
                    sprintf(buffer, "%s%s@%s left %s.\r", timestamp, name, host, oldchan);
#endif
            } else {
#ifdef SPEECH
                    sprintf(buffer, speech_message(20), timestamp, name, host, oldchan, pers);
#else
                    sprintf(buffer, "%s%s@%s left %s (%s).\r", timestamp, name, host, oldchan, pers);
#endif
            }
#ifdef L1IRC
            if (p->IrcMode == TRUE)
            {
              char IrcChannel[128];

              sprintf(IrcChannel, "#%d", oldchannel);
              sprintf(buffer, ":%s!%s@%s PART %s :%s\n"
                            , (*nick ? nick : name)
                            , name
                            , host
                            , IrcChannel
                            , "/leave");
            }
#endif /* L1IRC */
            appendstring(p, buffer);
            appendprompt(p, 0);
            p->locked = 1;
          }
        }
        for (cl = p->chan_list; cl; cl = cl->next) {
          if (cl->channel == newchannel) {
            break;
          }
        }
        if (newchannel >= 0) {
          if ((cl) || (!(newflags & M_CHAN_I) && (p->verbose || (p->notify && strstr(p->notify, name2))))) {
            appendprompt(p, 2);
#ifdef CONVNICK
           if (*nick != NUL)
#ifdef SPEECH
            sprintf(buffer, speech_message(21), timestamp, name, nick,host, newchan);
#else
            sprintf(buffer, "%s%s:%s@%s joined %s\r", timestamp, name, nick,host, newchan);
#endif
           else
#ifdef SPEECH
            sprintf(buffer, speech_message(22), timestamp, name, host, newchan);
#else
            sprintf(buffer, "%s%s@%s joined %s\r", timestamp, name, host, newchan);
#endif
#else
#ifdef SPEECH
            sprintf(buffer, speech_message(22), timestamp, name, host, newchan);
#else
            sprintf(buffer, "%s%s@%s joined %s\r", timestamp, name, host, newchan);
#endif
#endif
#ifdef L1IRC
           if (p->IrcMode == TRUE)
           {
             char IrcChannel[128];

             sprintf(IrcChannel, "#%d", newchannel);
             sprintf(buffer, ":%s!%s@%s JOIN :%s\n", (*nick ? nick : name), name, host, IrcChannel);
             appendstring(p, buffer);

             sprintf(buffer, ":%s 303 %s :\n", myhostname, p->nickname);

             appendstring(p, buffer);
             sprintf(buffer, ":%s MODE #%d +t\n"
                             , host
                             , newchannel);
           }
#endif /* L1IRC */
           appendstring(p, buffer);
            if (!(p->filter && strstr(p->filter, name2))) {
              if (*pers && strcmp(pers, "@")) {
#ifndef L1IRC
                sprintf(buffer, "    (%s)\r", pers);
                appendstring(p, buffer);
#else
                sprintf(buffer, "    (%s)\r", pers);

                if (p->IrcMode == TRUE)
                  sprintf(buffer, ":%s 311 %s %s %s %s * :%s\n"
                                , myhostname
                                , name
                                , (*nick ? nick : name)
                                , name
                                , host
                                , (*pers && *pers != '@') ? pers : "");

                  appendstring(p, buffer);
#endif /* L1IRC */
              }
            }
            appendprompt(p, 0);
            p->locked = 1;
          }
        }
      }
    }
    if (p->type == CT_HOST && !p->locked) {
      if (time == currtime) {
        time++;
      }
      sprintf(buffer, "/\377\200USER %s %s %ld %d %d %s\r", name, host, (long)time, oldchannel, newchannel, pers);
      appenddirect(p, buffer);
      p->locked = 1;
      send_proto("cvsd", "TX to %s %s", p->name, buffer+3);
    }
  }
}

/*---------------------------------------------------------------------------*/

void send_msg_to_local_user(char *fromname, char *toname, char *text)
{

  char buffer[2048];
  CONNECTION *p;
#ifdef CONVNICK
  char buf[(NAMESIZE * 2) + 2];
#endif

  for (p = connections; p; p = p->next)
    if (p->type == CT_USER && (!strcasecmp(p->name, toname)
#ifdef CONVERS_NICKNAME
     || !strcasecmp(p->nickname, toname)
#endif
     ) && !p->locked)
      if (!p->via) {
        if (!p->locked) {
          appendprompt(p, 2);
          if (strcmp(fromname, "conversd")) {
#ifndef L1IRC
            sprintf(buffer, "<*%s*>:", fromname);
            appendformline(p, buffer, text);
#else
            if (p->IrcMode == TRUE)
            {
              sprintf(buffer,":%s PRIVMSG %s : %s\n", fromname, p->nickname, text);
              appendstring(p, buffer);
            }
            else
            {
              sprintf(buffer, "<*%s*>:", fromname);
              appendformline(p, buffer, text);
            }
#endif /* L1IRC */
            if (p->away && text[0] != '*') {
#ifdef CONVNICK
                makeName(p, buf);
#ifdef SPEECH
                sprintf(buffer,speech_message(23), timestamp, buf, p->away);
#else
                sprintf(buffer,"%s%s is away: %s", timestamp, buf, p->away);
#endif /* SPEECH */
#else
#ifdef SPEECH
                sprintf(buffer,speech_message(23), timestamp, p->name, p->away);
#else
                sprintf(buffer,"%s%s is away: %s", timestamp, p->name, p->away);
#endif /* SPEECH */
#endif /* CONVNICK */
#ifdef L1IRC
              if (p->IrcMode == TRUE)
                sprintf(buffer, ":%s 301 %s %s :#%s"
                              , myhostname
                              , fromname
                              , toname
                              , p->away);
#endif /* L1IRC */
                send_msg_to_local_user("conversd",fromname,buffer);
                }
          } else {
#ifndef L1IRC
            appendstring(p, text);
            appenddirect(p, "\r");
#else
            if (p->IrcMode == TRUE)
            {
              appendstring(p, text);
              appendstring(p, "\n");
            }
            else
            {
              appendstring(p, text);
              appenddirect(p, "\r");
            }
#endif /* L1IRC */
          }
          appendprompt(p, 0);
          p->locked = 1;
        }
      }
}

/*---------------------------------------------------------------------------*/

void send_msg_to_user(char *fromname, char *toname, char *text)
{

  char buffer[2048];
  char namefilt[68], *c;
  WORD filtered = 0;
  CONNECTION *p;
#ifdef CONVNICK
  char buf[(NAMESIZE * 2) + 2];
#endif

  sprintf(namefilt, " %s ", fromname);
  if ((c = strchr(namefilt, ':')) != NULL) {
    *c++ = ' ';
    *c = NUL;
  }
  for (p = connections; p; p = p->next)
    if (p->type == CT_USER && (!strcasecmp(p->name, toname)
#ifdef CONVERS_NICKNAME
     || !strcasecmp(p->nickname, toname)
#endif
     ) && !p->locked) {
      if (p->via) {
        if (!p->via->locked) {
          sprintf(buffer, "/\377\200UMSG %s %s %s\r", fromname, toname, text);
          appenddirect(p->via, buffer);
          p->via->locked = 1;
          p->locked = 1;
          send_proto("cvsd", "TX to %s %s", p->via->name, buffer+3);
        }
      } else {
        if (!p->locked) {
          if (p->filter && strstr(p->filter, namefilt)) {
            filtered = 1;
            continue;
          }
          appendprompt(p, 2);
          if (strcmp(fromname, "conversd")) {
#ifndef L1IRC
            sprintf(buffer, "<*%s*>:", fromname);
            appendformline(p, buffer, text);
#else
            if (p->IrcMode == TRUE)
            {
              if ((c = strchr(fromname, ':')) != NULL)
                c++;

              sprintf(buffer,":%s PRIVMSG %s : %s\n"
                            , (c[0] == FALSE ? fromname : c)
                            , p->nickname, text);
              appendstring(p, buffer);
            }
            else
            {
              sprintf(buffer, "<*%s*>:", fromname);
              appendformline(p, buffer, text);
            }
#endif /* L1IRC */
            if (p->away && text[0] != '*') {
#ifdef CONVNICK
            makeName(p, buf);
#ifdef SPEECH
            sprintf(buffer,speech_message(23), timestamp, buf, p->away);
#else
            sprintf(buffer,"%s%s is away: %s", timestamp, buf, p->away);
#endif /* SPEECH */
#else
#ifdef SPEECH
            sprintf(buffer,speech_message(23), timestamp, p->name, p->away);
#else
            sprintf(buffer,"%s%s is away: %s", timestamp, p->name, p->away);
#endif /* SPEECH */
#endif /* CONVNICK */
#ifdef L1IRC
              if (p->IrcMode == TRUE)
                sprintf(buffer, ":%s 301 %s %s :#%s\n"
                              ,p->host
                              ,p->nickname
                              ,p->name
                              ,p->away);
#endif /* L1IRC */
              send_msg_to_local_user("conversd",fromname,buffer);
            }
          } else {
#ifndef L1IRC
            appendstring(p, text);
            appenddirect(p, "\r");
#else
            if (p->IrcMode == TRUE)
            {
              appendstring(p, text);
              appendstring(p, "\n");
            }
            else
            {
              appendstring(p, text);
              appenddirect(p, "\r");
            }
#endif /* L1IRC */
          }
          appendprompt(p, 0);
          p->locked = 1;
        }
      }
    }

  if (filtered) {
    clear_locks();
#ifdef SPEECH
    sprintf(buffer,speech_message(24), timestamp, toname);
#else
    sprintf(buffer,"%sYour messages are ignored by %s", timestamp, toname);
#endif
#ifdef L1IRC
    if (p->IrcMode == TRUE)
      sprintf(buffer,":%s 465 %s :Nachricht wird gefiltert!\n"
                    , p->host
                    , (p->nickname ? p->nickname : p->name));
#endif /* L1IRC */
    send_msg_to_user("conversd",fromname,buffer);
  }
}

/*---------------------------------------------------------------------------*/

void send_msg_to_channel(char *fromname, WORD channel, char *text)
{
  char buffer[2048];
  char namefilt[68], *c;
  CONNECTION *p;
  CHANNEL *ch;
  char addchan[16];
  CLIST *cl;
  WORD printit;
  WORD cvd;
#ifdef CONVNICK
  char Nickname[NAMESIZE + 1];
#endif /* CONVNICK */

  cvd = strcmp(fromname, "conversd");
  if (!cvd && !strncmp(text, "*** ", 4)) {          /* /me abfangen */
    sscanf(text + 4, "%s", buffer);           /* Absender ermitteln */
    buffer[64] = '\0';
    if ((c = strchr(buffer, '@')) != NULL)
      *c = '\0';
    sprintf(namefilt, " %s ", buffer);
#ifdef CONVNICK
    memset(Nickname, 0, sizeof(Nickname));          /* String Initialisieren. */

    if ((c = strchr(buffer, ':')) != NULL)        /* ggf. Nickname ermitteln. */
    {
      c++;                                           /* ':' zeichen loeschen. */
      strncpy(Nickname, c, NAMESIZE);                    /* Nickname sichern. */
    }
#endif /* CONVNICK */
  } else {
     sprintf(namefilt, " %s ", fromname);
    if ((c = strchr(namefilt, ':')) != NULL) {
      *c++ = ' ';
      *c = NUL;
    }
  }
  for (ch = channels; ch; ch = ch->next) {
    if (ch->chan == channel) break;
  }
  for (p = connections; p; p = p->next) {
#ifdef CONVNICK
    /* Wir pruefen auf Rufzeichen *und* Nickname */
    if (p->type == CT_USER && (!strcasecmp(p->name, fromname) || !strcasecmp(p->nickname, fromname))) {
#else
    if (p->type == CT_USER && !Strcmp(p->name, fromname)) {
#endif
      p->mtime = currtime;
    }
  }
  for (p = connections; p; p = p->next) {
    if (p->type == CT_USER) {
      printit = 0;
      if (p->channel == channel) {
        printit = 1;
        addchan[0] = '\0';
      } else {
        for (cl = p->chan_list; cl ; cl = cl->next) {
          if (cl->channel == channel) {
            printit = 1;
            sprintf(addchan, "%d:", channel);
          }
        }
      }
      if (printit) {
        if (p->via) {
          if (!p->via->locked && !(ch->flags & M_CHAN_L)) {
            sprintf(buffer, "/\377\200CMSG %s %d %s\r", fromname, channel, text);
            appenddirect(p->via, buffer);
            p->via->locked = 1;
          }
        } else {
          if (!p->locked) {
            if (p->filter && strstr(p->filter, namefilt))
              continue;
            appendprompt(p, 2);
            if (cvd) {
#ifndef L1IRC
              sprintf(buffer, "<%s%s>:", addchan, fromname);
              appendformline(p, buffer, text);
#else
              if (p->IrcMode == TRUE)
              {
                sprintf(buffer, ":%s PRIVMSG #%d :%s\n",fromname, channel, text);
                appendstring(p, buffer);
              }
              else
              {
                sprintf(buffer, "<%s%s>", addchan, fromname);
                appendformline(p, buffer, text);
              }
#endif /* L1IRC */
            } else {
#ifndef L1IRC
              appendstring(p, text);
              appenddirect(p, "\r");
#else
              if (p->IrcMode == TRUE)                          /* IRC-Client. */
              {
                char buffer2[2048];
                                                  /* IRC-Meldung vorbereiten. */
                sprintf(buffer2, ":%s PRIVMSG #%d :%s\n"
                              , (Nickname[0] != FALSE ? Nickname : buffer)
                              , channel
                              , text);
                appendstring(p, buffer2);              /* IRC-Meldung senden. */
              }
              else
              {
                appendstring(p, text);
                appenddirect(p, "\r");
              }
#endif /* L1IRC */
            }
            appendprompt(p, 0);
            p->locked = 1;
          }
        }
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
/* DL1XAO Protokollausgabe, lokal auf Kanal 32767, egal welchen Status
 * er sonst noch hat
 */
void send_proto(const char *modul, const char *format, ...)
{
  char       buffer[16];
  CONNECTION *p;
  CHANNEL    *ch;
  CLIST      *cl;
  WORD       printit;
  va_list    arg_ptr;
  char       str[1024];

  va_start(arg_ptr, format);
  vsprintf(str, format, arg_ptr);
  va_end(arg_ptr);

  if (!cvs_pc)
    return;
  for (ch = channels; ch; ch = ch->next)
    if (ch->chan == 32767)
      break;
  if (!ch)                                        /* nobody online */
    return;

  for (p = connections; p; p = p->next) {
    if (p->type == CT_USER && !p->via) {
      printit = 0;
      if (p->channel == 32767)
        printit = 1;
      else {
        for (cl = p->chan_list; cl ; cl = cl->next)
          if (cl->channel == 32767)
            printit = 1;
      }
      if (printit) {
        appendprompt(p, 2);
        sprintf(buffer, "[%s]:", modul);
        appendformline(p, buffer, str);
        appendprompt(p, 0);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

void send_invite_msg(char *fromname, char *toname, WORD channel)
{
  CONNECTION *p;
  char buffer[2048];
#ifdef SPEECH
  static char invitetext[128];
  static char responsetext[128];

  strcpy(invitetext,speech_message(25));
  strcpy(responsetext,speech_message(26));
#else
  static char invitetext[] = "\r\007\007%sMessage from %s...\rPlease join %s channel %d.\r\007\007\r";
  static char responsetext[] = "%s%s Invitation sent to %s @ %s.";
#endif


  for (p = connections; p; p = p->next)
    if (p->type == CT_USER && !Strcmp(p->name, toname)) {
      if (p->channel == channel) {
        clear_locks();
#ifdef SPEECH
        sprintf(buffer, speech_message(27), timestamp, toname);
#else
        sprintf(buffer, "%sUser %s is already on this channel.", timestamp, toname);
#endif
        send_msg_to_user("conversd", fromname, buffer);
        return;
      }
      if (!p->via && !p->locked) {
        sprintf(buffer, invitetext, timestamp, fromname, convtype, channel);
        p->invitation_channel = channel;
        appendstring(p, buffer);
        clear_locks();
        sprintf(buffer, responsetext, timestamp, convtype, toname, myhostname);
        send_msg_to_user("conversd", fromname, buffer);
        return;
      }
      if (p->via && !p->via->locked) {
        sprintf(buffer, "/\377\200INVI %s %s %d\r", fromname, toname, channel);
        appenddirect(p->via, buffer);
        send_proto("cvsd", "TX to %s %s", p->via->name, buffer+3);
        return;
      }
    }

  sprintf(buffer, invitetext, timestamp, fromname, convtype, channel);
  if (invite_ccp(toname, buffer)) {
    clear_locks();
    sprintf(buffer, responsetext, timestamp, convtype, toname, myhostname);
    send_msg_to_user("conversd", fromname, buffer);
    return;
  }

  for (p = connections; p; p = p->next)
    if (p->type == CT_HOST && !p->locked) {
      sprintf(buffer, "/\377\200INVI %s %s %d\r", fromname, toname, channel);
      appenddirect(p, buffer);
      send_proto("cvsd", "TX to %s %s", p->name, buffer+3);
    }

}

/*---------------------------------------------------------------------------*/

void update_destinations(PERMLINK *p, char *name, time_t rtt, char *rev)
{
  PERMLINK *l;
  DESTINATION *d, *d1;
  WORD pl;
  char buffer[256];

  for (d = destinations; d; d = d->next)
    if (!Strcmp(d->name, name)) break;

  if (!d) {
    d = (DESTINATION *)calloc(1, sizeof(DESTINATION));
    if (d) {
      d->link = p;
      Strcpy(d->name, name);
      Strcpy(d->rev, rev);
      if (!destinations || (! destinations->name) || (Strcmp(destinations->name, name) > 0)) {
        d->next = destinations;
        destinations = d;
      } else {
        d1 = destinations;
        while (d1->next) {
          if (Strcmp(d1->next->name, name) > 0) {
            d->next = d1->next;
            d1->next = d;
            break;
          }
          d1 = d1->next;
        }
        if (!d1->next) {
          d->next = d1->next;
          d1->next = d;
        }
      }
    }
  } else {
    Strcpy(d->rev, rev);
    d->link = p;
  }

  if (!d)
    return;

  d->rtt = rtt;
  if (labs(rtt - d->last_sent_rtt) > (d->last_sent_rtt / 8L)) {
    for (pl = 0; pl < MAXCVSHOST; pl++) {
      l = permarray[pl];
      if (   l != NULL
          && l != d->link
          && l->connection
          && l->connection->type == CT_HOST
          ) {
        if (rtt)
          sprintf(buffer, "/\377\200DEST %s %ld %s\r", name, rtt+(l->txtime+l->rxtime)/2L, rev);
         else
          sprintf(buffer, "/\377\200DEST %s 0\r", name);

        d->last_sent_rtt = rtt;
        appenddirect(l->connection, buffer);
        send_proto("cvsd", "TX to %s %s", l->connection->name, buffer+3);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn -par
#endif
PERMLINK *update_permlinks(char *name, CONNECTION *cp, WORD isperm)
{
  DESTINATION *d;
  PERMLINK *p;
  WORD pl;
#ifdef CONVERS_LINKS
  char NAME[NAMESIZE + 1];
#endif

  if (*name == NUL)
    return(NULLPERMLINK);

#ifdef CONVERS_LINKS
  strncpy(NAME, name, NAMESIZE);
  strupr(NAME);
#endif

  for (pl = 0; pl < MAXCVSHOST; pl++) {
    p = permarray[pl];
    if (p == NUL)
        continue;
#ifdef CONVERS_LINKS
    if ((cmpcal(p->call,NAME)) || (!strcasecmp(p->cname, name)))
#else
    if (!strcmp(p->cname, name))
#endif
        { /* Eintrag gefunden,aendern */
      for (d = destinations; d; d = d->next) {
        if (d->rtt && (d->link == p)) {
          update_destinations(p, d->name, 0, "");
        }
      }
      if (cp) {                                       /* eintragen */
        p->connection = cp;
        p->statetime = currtime;
        p->tries = 0;
        p->waittime = 9;
        p->rxtime = 0;
        p->txtime = 0;
        p->testwaittime = currtime;
        p->testnexttime = currtime + 60;
        p->retrytime = currtime + p->waittime;
      } else {                                        /* austragen */
        p->statetime = currtime;
        p->tries = 0;
        if (p->locked) {
          p->waittime = 3600;
          p->nlcks++;
        }
        else
          p->waittime = 9;
        p->rxtime = 0;
        p->txtime = 0;
        p->testwaittime = currtime;
        p->testnexttime = currtime + 60;
        p->retrytime = currtime + p->waittime;
      }
      return(p);
    }
  }
  for (pl = 0; pl < MAXCVSHOST; pl++) {         /* neuer Eintrag */
    if (!permarray[pl]) break;
  }
  if (pl < MAXCVSHOST) {
    p = (PERMLINK *) calloc(1, sizeof(PERMLINK));
    if (p) {
      strncpy(p->cname, name, NAMESIZE);
      strcpy(p->call, name);
      p->connection = cp;
      p->statetime = currtime;
      p->waittime = 9;
      p->testnexttime = currtime + 60;
      p->testwaittime = currtime;
      p->retrytime = currtime + p->waittime;
    }
    permarray[pl] = p;
  } else {
    return(NULLPERMLINK);
  }
  return(p);
}
#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn .par
#endif

/*---------------------------------------------------------------------------*/

void connect_permlinks()
{

#define MAX_WAITTIME   (60L*60L*3L)

  WORD pl;
  PERMLINK    *p;
  DESTINATION *d;

  for (pl = 0; pl < MAXCVSHOST; pl++) {
    p = permarray[pl];
    if (p && p->connection && p->connection->type == CT_HOST) {
      if (p->testnexttime < currtime) {
        if ((p->testwaittime + 7300L) < currtime) {
          p->rxtime = 0;
          p->txtime = 0;
          for (d = destinations; d; d = d->next) {
            if (d->link == p) {
              update_destinations(p, d->name, 0, "");
            }
          }
        }
        appenddirect(p->connection, "/\377\200PING\r");
        send_proto("cvsd", "TX to %s PING", p->cname);
        p->testwaittime = currtime;
        p->testnexttime = currtime + 7300L;
      }
    }
  }
  for (pl = 0; pl < MAXCVSHOST; pl++) {
    p = permarray[pl];
    if (p && !p->connection) {
      if (p->retrytime < currtime) {
        p->tries++;
        if (p->locked) { /* Sperrzeit abgelaufen */
          p->waittime = 75;
          p->locked = 0;
        }
        if (p->waittime == 9) p->waittime = 75; /* *2 = 2.5 minutes */
        p->waittime <<= 1; /* 2.5,5,10,20,40,80,160,180,180,... min */
        if (p->waittime > MAX_WAITTIME) p->waittime = MAX_WAITTIME;
        p->retrytime = currtime + p->waittime;
        if (p->port != 254)
          connect_cvshost(p);
      }
    }
  }
}

/*------------------------------------------------------------------------*/

/* lokal eingegebene Personalbeschreibungen werden in einer Datenbank
 * gespeichert. Deim Einloggen von bekannten Personen wird der Text
 * automatisch gesetzt. Die Datenbank laeuft im textpath (ramdisk), die
 * Daten koennen im confpath gesichert werden. Die Textlaenge wird auf
 * 118 Zeichen begrenzt, das sollte ausreichen.
 * Zusaetzlich werden die Zeilenbreite, sowie Zeichensaetze gespeichert
 * und ein Timestamp, der zum Loeschen von VOM's dient.
 */
char *personalmanager(WORD func, CONNECTION *cp, char *pers)
{
 char buffer[1024];
 static char tmp[128];
 ULONG n;
 FILE *fp1;
#ifndef MC68302
 FILE *fp2;
#endif

  if (cp && *cp->name) {
    sprintf(tmp, "%-6.6s", cp->name);
    strupr(tmp);
  }
  else
    cp = NULL;
  switch (func) {
    case INIT:
        persfile = strdup(pers);
#ifndef MC68302
        strcpy(tmp, confpath);
        strcat(tmp, "convers.prs");
        if (!stricmp(persfile, tmp))           /* Pfade sind gleich */
          break;
        if ((fp1 = xfopen(tmp, "rb")) == NULL) /* noch keine Datei  */
          break;
        if ((fp2 = xfopen(persfile, "wb")) == NULL) { /*Schreibfehler*/
          fclose(fp1);
          break;
        }                                      /* Daten umkopieren  */
        while ((n = fread(buffer, 1, 1024, fp1)) > 0)
          fwrite(buffer, 1, (size_t)n, fp2);
        fclose(fp1);
        fclose(fp2);
#endif
        break;

    case SAVE:
#ifndef MC68302
        strcpy(tmp, confpath);
        strcat(tmp, "convers.prs");
        if (!stricmp(persfile, tmp))           /* Pfade sind gleich */
          break;
        if ((fp1 = xfopen(persfile, "rb")) == NULL)/*noch keine Datei*/
          break;
        if ((fp2 = xfopen(tmp, "wb")) == NULL) { /* Schreibfehler    */
          fclose(fp1);
          break;
        }                                       /* Daten umkopieren */
        while ((n = fread(buffer, 1, 1024, fp1)) > 0)
          fwrite(buffer, 1, (size_t)n, fp2);
        fclose(fp1);
        fclose(fp2);
#endif
        break;

    case SET:
        if (!cp || !pers)
          break;
        strncpy(tmp+6, pers, 118);
        tmp[124] = (char)(currtime >> 24); /* Timestamp, ca 194 Tage */
        tmp[125] = (char)cp->width;
        tmp[126] = (char)cp->charset_in;
        tmp[127] = (char)cp->charset_out;
        if ((fp1 = xfopen(persfile, "rb+")) == NULL) {/*Datei oeffnen*/
          if ((fp1 = xfopen(persfile, "wb")) != NULL) { /* dann neu  */
            fwrite(tmp, 1, 128, fp1);          /* der erste Eintrag */
            fclose(fp1);
          }
          break;
        }
        func = 0;
        n = 0;
        while (!func && fread(buffer, 1, 128, fp1)) {/* suchen      */
          if (!strncmp(buffer, tmp, 6)) {
            func = 1;
            break;
          }
          n += 128;
        }
        if (func)                             /* zum editieren zu-  */
          fseek(fp1, n, SEEK_SET);            /* rueckpositionieren */
        fwrite(tmp, 1, 128, fp1);             /* Daten schreiben    */
        fclose(fp1);
        break;

    case GET:
        if (!cp)
          break;
        if ((fp1 = xfopen(persfile, "rb+")) == NULL)
          break;
        n = 0;
        while (fread(buffer, 1, 128, fp1)) {
          if (!strncmp(buffer, tmp, 6)) {       /* suchen           */
            if (buffer[124] != (char)(currtime >> 24)) { /* neuen   */
              buffer[124] = (char)(currtime >> 24);    /* Timestamp */
              fseek(fp1, n, SEEK_SET);          /*rueckpositionieren*/
              fwrite(buffer, 1, 128, fp1);      /* Daten schreiben  */
            }
            fclose(fp1);
            if ((cp->width = buffer[125]) == 0)
              cp->width = 80;
            cp->charset_in = buffer[126];
            cp->charset_out = buffer[127];
            buffer[124] = '\0';
            strcpy(tmp, buffer+6);
            return(tmp);                        /* zurueckgeben     */
          }
          n += 128;
        }
        fclose(fp1);
  }
  return(NULL);
}

/*---------------------------------------------------------------------------*/

char *getflags(WORD flag)
{
  static char f[8];
  char        *p;

  p = f;
  if (flag & M_CHAN_S)
    *p++ = 'S';
  if (flag & M_CHAN_P)
    *p++ = 'P';
  if (flag & M_CHAN_T)
    *p++ = 'T';
  if (flag & M_CHAN_I)
    *p++ = 'I';
  if (flag & M_CHAN_M)
    *p++ = 'M';
  if (flag & M_CHAN_L)
    *p++ = 'L';
  *p = '\0';
  return(f);
}

/*---------------------------------------------------------------------------*/

/* aehnlich strncpy, speziell zum Eintragen in die Namensfelder */
void Strcpy(char *dst, char *src)
{
 WORD max;

  for (max = NAMESIZE-1; max-- && *src; )
    *dst++ = *src++;
  *dst = '\0';
}

/* aehnlich strncmp, prueft mit auf Namensfeldbreite gekuerztem b */
WORD Strcmp(char *a, char *b)
{
 char tmp[NAMESIZE];

  Strcpy(tmp, b);
  return(strcmp(a, tmp));
}

/*---------------------------------------------------------------------------*/

/* Da einige Felder zuviel Platz belegen und es einfach Verschwendung
   ist, dafuer soviel Platz zu belegen, obwohl man diese nicht immer
   nutzt, verwalten wir das ganze mit Pointern und dynamisch.
   Einige andere Funktionen mussten diesem Verhalten angepasst werden.

   Eingabe:
     adr : Zeiger auf den Zeiger auf den String
     str : neuer String, leer zum Loeschen
     max : maximale ehemalige Feldbreite
   Resultat:
     Ueber adr wird der Zeiger in der Struktur modifiziert          */

void setstring(char **adr, char *str, WORD max)
{
  if (!*str) {                     /* Kein String,also loeschen */
    if (*adr)                      /* schon ein String gesetzt, */
      free(*adr);                  /*  also Speicher freigeben  */
    *adr = NULL;                   /* markieren                 */
   }
   else {
#ifdef __WIN32__
    if (strlen(str) >= (unsigned short)max) /* String zulang,            */
#else
    if (strlen(str) >= max)        /* String zulang,            */
#endif /* WIN32 */
                   *(str + max-1) = '\0';       /*  also kuerzen             */
    if (*adr) {                    /* schon ein String gesetzt, */
      if (!strcmp(*adr, str))      /*  String ist derselbe      */
        return;                    /*   also nichts zutun       */
      free(*adr);                  /*  Speicher freigeben       */
    }
    *adr = strdup(str);            /* neuen String einfuegen     */
  }
}

/*---------------------------------------------------------------------------*/

#ifdef CONV_CHECK_USER
/************************************************************************/
/* Pruefung auf Doppel-Login                                            */
/*----------------------------------------------------------------------*/
CONNECTION *CheckUserCVS(char *user)
{
  CONNECTION *p;

  for (p = connections; p; p = p->next)
  {
    if (  ((!strcasecmp(p->name, user))         /* Rufzeichenvergleich. */
#ifdef CONVNICK
        ||(!strcasecmp(p->nickname, user))        /* Nicknamevergleich. */
#endif /* CONVNICK */
       )
       &&(p->type == CT_USER))                   /* Als User markiert. */
      return (p);
  }

  return(NULL);
}
#endif /* CONV_CHECK_USER. */

#else
#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn -par
#endif

void send_proto(const char *modul, const char *format, ...)
{
}

char *personalmanager(WORD func, CONNECTION *cp, char *pers)
{
  return NULL;
}

#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn .par
#endif
#endif

#ifdef BEACON_STATUS
char *convers_user(char *cuser)
{
  WORD newchannel;
  WORD printit;

  newchannel = 0;
#ifdef PPCONVERS
  printit = count_user(newchannel);
  sprintf(cuser,"%u",printit);
#else
  printit = FALSE;
#endif
  return(cuser);
}
#endif

/* End of $RCSfile$ */
