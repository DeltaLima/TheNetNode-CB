#include "tnn.h"
#ifdef L1IRC
#include "conversd.h"

static T_INTERFACE *ifpp;               /* Zeiger auf das Aktuelle Interface.*/

void IrcAwayCommand(CONNECTION *cp);                  /* Username aufloesen. */
void IrcChannelCommand(CONNECTION *cp);                 /* CHANNEL-Kommando. */
void IrcHelpCommand(CONNECTION *cp);                        /* Help Kommand. */
void IrcLinksCommand(CONNECTION *cp);                     /* Links-Kommando. */
void IrcListCommand(CONNECTION *cp);                       /* List-Kommando. */
void IrcNickCommand(CONNECTION *cp);                  /* Nickname aufloesen. */
void IrcModeCommand(CONNECTION *cp);                       /* Mode-Kommando. */
void IrcNamesCommand(CONNECTION *cp);                     /* Names-Kommando. */
void IrcPartCommand(CONNECTION *cp);                  /* Channel schliessen. */
void IrcPersonalCommand(CONNECTION *cp);   /* Personal-Text setzen/loeschen. */
void IrcPingCommand(CONNECTION *cp);
void IrcPongCommand(CONNECTION *cp);
void IrcPrivmsgCommand(CONNECTION *cp);                     /* MSG-Kommando. */
void IrcQuitCommand(CONNECTION *cp);                       /* Quit-Kommando. */
void IrcUserHostCommand(CONNECTION *cp);                                  /* */
void IrcSquitCommand(CONNECTION *cp);
void IrcUserCommand(CONNECTION *cp);
void IrcWhoCommand(CONNECTION *cp);                         /* Who-Kommando. */
void IrcWhoisCommand(CONNECTION *cp);                 /* Userdaten ausgeben. */


typedef struct cmdtable_irc
{
  char *name;
  void (*fnc)(CONNECTION *);
  char *help;
} CMDTABLE_IRC;

CMDTABLE_IRC cmdtable_irc[] =
{
 {"away",        IrcAwayCommand,      "AWAY"},
 {"channel",     IrcChannelCommand,   "CHAN"},
 {"help",        IrcHelpCommand,      "HELP"},
 {"nick",        IrcNickCommand,      "NICK"},
 {"join",        IrcChannelCommand,   "JOIN"},
 {"links",       IrcLinksCommand,     "LINKS"},
 {"list",        IrcListCommand,      "LIST"},
 {"mode",        IrcModeCommand,      "MODE"},
 {"names",       IrcNamesCommand,     "NAMES"},
 {"part",        IrcPartCommand,      "PART"},
 {"personal",    IrcPersonalCommand,  "PERS"},
 {"ping",        IrcPingCommand,      "PING"},
 {"pong",        IrcPongCommand,      "PONG"},
 {"privmsg",     IrcPrivmsgCommand,   "PRIVMSG"},
 {"quit",        IrcQuitCommand,      "QUIT"},
 {"user",        IrcUserCommand,      "USER"},
 {"userhost",    IrcUserHostCommand,  "USERHOST"},
 {"squit",       IrcSquitCommand,     "SQUIT"},
 {"who",         IrcWhoCommand,       "WOH"},
 {"whois",       IrcWhoisCommand,     "WHOIS"},
 {NULL,          0,                    NULL}
};

/* IRC-Server Einstellung aendern/setzen. */
void ccpirc(void)
{
  MBHEAD         *mbp;
  char            ch;
  int             tmp_fd       = EOF;
  int             newloglevel  = 0;
  int             new_tcp_port = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(ccpirc)";
#endif /* DEBUG_MODUS */

  ifpp = &ifp[IRC_ID];                  /* Dann Zeiger auf das IRC-Interface. */

  if (issyso() && skipsp(&clicnt, &clipoi))           /* Sysop will aendern.  */
  {
    clicnt--;
    ch = toupper(*clipoi++);

    switch (ch)
    {
      case 'P':                             /* Sysop will IRC-Port aéndern */
        if (!skipsp(&clicnt, &clipoi))
        {
          putmsg("Invalid Parameter\r");
          return;
        }

        new_tcp_port = nxtlong(&clicnt, &clipoi);      /* Neuen Port ermitteln */

        if (  (new_tcp_port < 1)
            ||(new_tcp_port > 65535))
        {
          putmsg("TCP-Port not valid, not changed !!!\r");
          return;
        }

        if (ifpp->actively == FALSE)                    /* IRC ist deaktiv.*/
        {
          putmsg("TCP-Port haven not switched on!\r");
          return;
        }
        /* Wenn NEUER Port und ALTER Port gleich sind, nicht neu Initialisieren.*/
        if (ifpp->tcpport == Htons((unsigned short)new_tcp_port))
        {
          putmsg("TCP-Port successfully changed\r");
          return;
        }

                                           /* Neuen TCP-Port Initialisieren. */
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


        ifpp->tcpport = Htons((unsigned short)new_tcp_port);

        putmsg("TCP-Port successfully changed\r");
        return;

      case 'L':                                /* Sysop will LOGLevel aendern */
       if (!skipsp(&clicnt, &clipoi))
       {
         mbp = putals("IRC-Server:\r");
         putprintf(mbp, "My LogLevel: %d\r",ifpp->log); /* Loglevel anzeigen. */

         prompt(mbp);
         seteom(mbp);
         return;
       }

       newloglevel = nxtnum(&clicnt, &clipoi);   /* neuen Loglevel ermitteln. */

       if (  (newloglevel < 0)
           ||(newloglevel > 3))                     /* Pruefe neuen Loglevel. */
       {
         mbp = putals("IRC-Server:\r");
         putprintf(mbp, "Error: Log level worth from 0 to 3!\r");
         prompt(mbp);
         seteom(mbp);
         return;
       }

       ifpp->log = newloglevel;         /* Neuen Loglevel setzen und zeigen */
       mbp = putals("IRC-Server:\r");
       putprintf(mbp, "My Loglevel: %d\r", ifpp->log);

       prompt(mbp);
       seteom(mbp);
       return;
      break;


      default:                                      /* Ungueltiger Parameter. */
        putmsg("Invalid Parameter\r");
       return;
     }
  }

  mbp = putals("IRC-Server:\r");            /* Aktuelle Einstellungen zeigen. */

  putprintf(mbp, "My TCP-Port: %u\r", Ntohs(ifpp->tcpport));
  prompt(mbp);
  seteom(mbp);
}

char *get_mode_flags2irc(int flags)
{
  static char mode[16];
  char *p = mode;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(get_mode_flags2irc)";
#endif /* DEBUG_MODUS */

  p = mode;
  if (flags & M_CHAN_I)
    *p++ = 's';

  if (flags & M_CHAN_M)
    *p++ = 'm';

  if (flags & M_CHAN_T)
    *p++ = 't';

  if (flags & M_CHAN_P)
    *p++ = 'i';

  if (flags & M_CHAN_S)
    *p++ = 'p';

  // always +n
  *p++ = 'n';
  //if (flags & M_CHAN_L)
  //*p++ = 'l';
  //*p++ = ' ';

  *p = 0;

  return mode;
}

/******************************************************************************/
/*                                                                            */
/* Eingehende Befehle bearbeiten.                                             */
/*                                                                            */
/******************************************************************************/
void ProcessIrcInput(char *cnvinbuf, CONNECTION *cp)
{
  CMDTABLE_IRC *IrcCmdp;
  char         *cmd;
  char          buffer[2048];
  int           cmdlen;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(ProcessIrcInput)";
#endif /* DEBUG_MODUS */

  cnvinbuf[2048] = 0;

  cmd = getarg(cnvinbuf, 0);                              /* Befehl einlesen. */

  if (cmd[0] == FALSE)                                        /* kein befehl, */
    return;                                                /* brechen wir ab. */

  cmdlen = strlen(cmd);                            /* Befehlslange ermitteln. */
                                                 /* durchsuche Befehls-Liste. */
  for (IrcCmdp = cmdtable_irc; IrcCmdp->name; IrcCmdp++)
  {
    if (!strncmp(IrcCmdp->name, cmd, cmdlen))          /* Befehl vergleichen. */
    {
      (*IrcCmdp->fnc) (cp);                             /* Befehl ausfuehren. */
      return;
    }
  }
                                                       /* Ungueltiger Befehl. */
  sprintf(buffer, ":%s 421 %s *** Unknown command '/%s'. Type /HELP for help.\n"
                , myhostname
                , (*cp->nickname ? cp->nickname : cp->name)
                , cmd);
  appenddirect(cp, buffer);
}

/******************************************************************************/
/*                                                                            */
/* Away (Ab/Anwesend) setzen.                                                 */
/*                                                                            */
/******************************************************************************/
void IrcAwayCommand(CONNECTION *cp)
{
  char *AwayText;
  char buffer[2048];
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcAwayCommand)";
#endif /* DEBUG_MODUS */

  AwayText = getarg(NULL, GET_ALL);              /* akt. Away-Text ermitteln. */

  if (*AwayText == ':')                                     /* ':' im String. */
    AwayText++;                                              /* ':' loeschen. */

  if (AwayText == NULL)                                         /* Kein Text. */
    return;                                               /* Keine Aenderung. */

  cp->away = AwayText;               /* Aktualisiere den aktuellen Away-Text. */

  if (*AwayText != '\0')                                         /* Abmelden. */
    sprintf(buffer, ":%s 301 %s %s : %s\n"        /* IRC-Meldung vorbereiten, */
    , myhostname
    , cp->name
    , (*cp->nickname ? cp->nickname : cp->name)
    , AwayText);
  else                                                           /* Anmelden. */
    sprintf(buffer, ":%s 320 %s %s : is back\n"   /* IRC-Meldung vorbereiten. */
    , myhostname
    , cp->name
    , (*cp->nickname ? cp->nickname : cp->name));

  appendstring(cp, buffer);

#ifndef CONVNICK
  send_awaymsg(cp->name,    /* Neuen Away-Text an andere Host's weiterleiten. */
               cp->hosthostname,
               currtime,
               AwayText);
#else
  send_awaymsg(cp->name,    /* Neuen Away-Text an andere Host's weiterleiten. */
               cp->nickname,
               cp->host,
               currtime,
               AwayText);
#endif /* CONVNICK */
}

/******************************************************************************/
/*                                                                            */
/* Persoenlichen Text setzen.                                                 */
/*                                                                            */
/******************************************************************************/
void IrcPersonalCommand(CONNECTION *cp)
{
  char *pers;
  char  buffer[2048];
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcPersonalCommand)";
#endif /* DEBUG_MODUS */

  pers = getarg(NULL, GET_ALL);                  /* akt. Pers-Text ermitteln. */

  if (*pers)
  {
    if (!strcmp(pers, "@"))               /* Pers-Text soll geloescht werden. */
      pers = "";                                       /* Pers-Text loeschen. */

    personalmanager(SET, cp, pers);               /* Datenbank aktualisieren. */

    if (pers != cp->pers)
    {
      setstring(&(cp->pers), pers, 256);           /* Neuen Pers-Text setzen. */
      cp->mtime = currtime;                          /* Aktuelle Zeit merken. */

      sprintf(buffer, ":%s 311 %s %s %s %s * :%s\n"
                    , myhostname
                    , cp->nickname
                    , cp->name
                    , (*cp->nickname ? cp->nickname : cp->name)
                    , cp->host
                    , (*pers && *pers != '@') ? pers : "");
      appendstring(cp, buffer);

#ifndef CONVNICK
      send_persmsg(cp->name, /* Neuen Pers-Text an andere HOST's weiterleiten.*/
                   myhostname,
                   cp->channel,
                   pers,
                   cp->time);
#else
      send_persmsg(cp->name, /* Neuen Pers-Text an andere HOST's weiterleiten.*/
                   cp->nickname,
                   myhostname,
                   cp->channel,
                   pers,
                   cp->time);
#endif /* CONVNICK */
    }
  }
}

void IrcNoticeCommand(CONNECTION *cp)
{
  char     buffer[2048];
  int      channel;
  char    *toname, *text;
  CHANNEL *ch;
  CLIST   *cl;
  char    *q;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcNoticeCommand)";
#endif /* DEBUG_MODUS */

  *buffer = 0;

  toname = getarg(0, 0);
  if (*toname && (*toname == '#' || *toname == '~'))
    toname++;

  // notice commands should not be answered
  if (!*toname)
    return;

  text = getarg(NULL, GET_ALL);

  cp->locked = 1;

  if (*text == ':')
    text++;
  // dcc mess

  if ((q = strchr(text, '\001')))
  {
    text = q+1;
    if ((q = strchr(text, '\001')))
      *q = 0;

    if (!strncasecmp(text, "AWAY ", 5))
    {
      text = text+5;

      while (*text && isspace(*text & 0xff))
        text++;

      if (strcmp(cp->away, text))
        send_awaymsg(cp->name, cp->nickname, myhostname, currtime, text);

      return;
    }

    if (!strncasecmp(text, "ACTION ", 7))
    {
      text = text+7;

      while (*text && isspace(*text & 0xff))
        text++;

      if (strlen(text) > 384)
        text[384] = 0;
    }
    else
    {
      while (*text && isspace(*text & 0xff))
        text++;

      if (strlen(text) > 384)
        text[384] = 0;

      sprintf(buffer, "*** %s@%s ack :%s:", cp->name, cp->host, text);
    }
  }

  if (!*buffer)
    sprintf(buffer, "*** %s@%s notice: %s", cp->name, cp->host, text);

  if (isdigit(*toname & 0xff) && ((channel = atoi(toname)) > 0 || (channel == 0 && !strcmp(toname, "0"))))
  {
    // to channel. special care must be taken
    for (ch = channels; ch; ch = ch->next)
    {
      if (ch->chan == channel)
        break;
    }

    if (!ch)
      return;

    // only channels we are on (and have permission to talk)
    if (cp->operator == 2)
    {
      ;
    }
    else
      if  (cp->channel == ch->chan)
      {
        if ((ch->flags & M_CHAN_M) && !cp->channelop)
          return;
      }
      else
      {
        for (cl = cp->chan_list; cl; cl = cl->next)
          if (cl->channel == ch->chan)
            break;

          if (!cl)
            return;

          if ((ch->flags & M_CHAN_M) && !cl->channelop)
            return;
      }

      send_msg_to_channel("conversd", (short)channel, buffer);
  }
  else
  {
    send_msg_to_user("conversd", toname, buffer);
  }
}

void IrcChannelCommand(CONNECTION *cp)
{
  char           *chan;
  char            buffer[2048];
  WORD            newchannel;
  struct channel *ch;
  struct clist   *cl,
                 *cl2;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcChannelCommand)";
#endif /* DEBUG_MODUS */

  /* Channel holen. */
  chan = getarg(NULL, GET_NXTLC);

  if (*chan == '#')
    chan++;

  newchannel = atoi(chan);

  for (ch = channels; ch; ch = ch->next)
  {
    if (ch->chan == newchannel)
      break;
  }

  for (cl = cp->chan_list; cl; cl = cl->next)
  {
    if (cl->channel == newchannel)
      break;
  }

  /* Gibt es einen Eintrag? */
  if (cl)
    /* Ja, */
    return;

#ifdef CONVNICK
  send_user_change_msg(cp->name,cp->nickname, cp->host, -1, newchannel, cp->pers, cp->time);
#else
  send_user_change_msg(cp->name,cp->host, -1, newchannel, cp->pers, cp->time);
#endif /* CONV_NICKNAME */

  if (!ch)
    cp->locked = 0;

  cl = (CLIST *)calloc(1, sizeof(CLIST));
  cl->time = currtime;
  cp->mtime = currtime;

  if (!ch)
    cl->channelop = 1;
  else
    cl->channelop = 0;

  cl->channel = newchannel;

  if (!cp->chan_list || cp->chan_list->channel > cl->channel)
  {
    cl->next = cp->chan_list;
    cp->chan_list = cl;
  }
  else
    for (cl2 = cp->chan_list; cl2; cl2 = cl2->next)
    {
      if (cl2->next)
      {
        if (cl2->next->channel > atoi(chan))
        {
          cl->next = cl2->next;
          cl2->next = cl;
          break;
        }
      }
      else
        {
          cl2->next = cl;
          cl->next = NULLCLIST;
          break;
        }
    }

  cp->channel = atoi(chan);

  sprintf(buffer,":%s!%s@%s JOIN :#%d\n"
                , (*cp->nickname ? cp->nickname : cp->name)
                , cp->name
                , cp->host
                , cp->channel);
  appendstring(cp, buffer);

  sprintf(buffer, ":%s MODE #%d +%s\n"
                 ,myhostname
                 ,cp->channel
                 ,get_mode_flags2irc((ch ? ch->flags : 0)));
  appendstring(cp, buffer);

  IrcNamesCommand(cp);

  if (ch)
  {
    if (ch->topic)
    {
      sprintf(buffer, ":%s 332 %s #%d :%s\n"
                    ,cp->host
                    ,(*cp->nickname ? cp->nickname : cp->name)
                    ,cp->channel
                    ,ch->topic);
      appendstring(cp, buffer);

      sprintf(buffer, ":%s 333 %s #%d %s %ld\n"
                    , cp->host
                    , (*cp->nickname ? cp->nickname : cp->name)
                    , cp->channel
                    , ch->name
                    , ch->time);
      appendstring(cp,buffer);
    }
    cp->channelop = cl->channelop;
  }
}

/* Mode einstellen. */
void IrcModeCommand(CONNECTION *cp)
{
  CONNECTION *p;
  CHANNEL    *ch;
  CLIST      *cl;
  char        buffer[2048];
  char       *arg,
             *c;
  int         remove   = 0;
  int         channel  = 0;
  int         oldflags = 0;
  int         op       = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcModeCommand)";
#endif /* DEBUG_MODUS */

  arg = getarg(0, 0);

  if (*arg == ':')
    arg++;

  if (*arg == '#')
    arg++;
  else
  {
    if (cp->IrcMode)
    {
      if (!strcmp(arg, "*"))
        return;

      if (cp->type == CT_USER)
      {
        char *arg2 = getarg(0, 0);

        if (!*arg)
        {
          sprintf(buffer, ":%s 461 %s MODE :Not enough parameters\n"
                        , cp->host
                        , (cp->nickname ? cp->nickname : cp->name));
          appendstring(cp, buffer);
          return;
        }

        if (strcasecmp(arg, cp->nickname))
        {
          sprintf(buffer, ":%s 403 %s %s :No such channel\n"
                        , cp->host
                        , (cp->nickname ? cp->nickname : cp->name)
                        , arg);
          appendstring(cp, buffer);
          return;
        }

        *buffer = 0;

        if (!*arg2)
        {
          sprintf(buffer, ":%s 211 %s %s\n"
                        , myhostname
                        , cp->nickname
                        , (cp->verbose) ? "+sw" : "");
        }
        else
        {
          if (strstr(arg2, "+s") || strstr(arg2, "+w"))
            cp->verbose = 1;
          else
            if (strstr(arg2, "-s") || strstr(arg2, "-w"))
              cp->verbose = 0;
            else
            {
              sprintf(buffer, ":%s 501 %s :Unknown MODE flag: %s\n"
                            , cp->host
                            , (cp->nickname ? cp->nickname : cp->name)
                            , arg2);
            }

            if (!*buffer)
              sprintf(buffer, ":%s MODE %s %csw\n"
                            , myhostname
                            , cp->nickname
                            , (cp->verbose ? '+' : '-'));

            appendstring(cp, buffer);
        }
        return;
      }
    }
  }

  channel = atoi(arg);

  if (*arg && !strcmp(arg, "*"))
  {
    channel = cp->channel;
  }
  else
  {
    if (!*arg || !(isdigit(*arg & 0xff) && (channel > 0 || (!channel && !strcmp(arg, "0")))))
    {
      if (cp->type != CT_HOST)
      {
        if (*arg)
        {
          sprintf(buffer, ":%s 403 %s #%s :No such channel\n"
                        , cp->host
                        , (cp->nickname ? cp->nickname : cp->name)
                        , arg);
        }
        else
        {
          sprintf(buffer, ":%s 461 %s MODE :Not enough parameters\n"
                        , cp->host
                        , (cp->nickname ? cp->nickname : cp->name));
        }

        appendstring(cp, buffer);
        appendprompt(cp, 0);
      }

      return;
    }

    arg = getarg(0, 1);

    if (!*arg && cp->type == CT_USER)
    {
      for (ch = channels; ch; ch = ch->next)
      {
        if (ch->chan == channel)
          break;
/*
        {
          sprintf(buffer, ":%s 324 %s %s #%d +%s\n"
                        , cp->host
                        , cp->name
                        , (*cp->nickname ? cp->nickname : cp->name)
                        , channel
                        , get_mode_flags2irc(ch->flags));
          appendstring(cp, buffer);

          sprintf(buffer, ":%s 329 %s %s #%d %d\n"
                        , cp->host
                        , cp->name
                        , (*cp->nickname ? cp->nickname : cp->name)
                        , channel
                        , 0);
          appendstring(cp, buffer);
          appendprompt(cp, 0);
          break;
        }*/
      }

      if (!ch)
      {
        sprintf(buffer, ":%s 403 %s #%d :No such channel\n"
                      , cp->host
                      , (cp->nickname ? cp->nickname : cp->name)
                      , channel);
        appendstring(cp, buffer);
        appendprompt(cp, 0);
      }

      return;
    }

    for (cl = cp->chan_list; cl; cl = cl->next)
    {
      if (cl->channel == channel)
      {
        if (cl->channelop)
          op++;
        break;
      }
    }

    for (ch = channels; ch; ch = ch->next)
    {
      if (channel == ch->chan)
        break;
    }

    if (op || cp->operator == 2 || (cp->type == CT_HOST))
    {
      if (!ch || (cp->type != CT_HOST && channel == 0 && cp->operator != 2))
      {
        if (cp->type == CT_USER)
        {
          if (channel == 0)
          {
            sprintf(buffer, ":%s 477 %s #%d :Channel doesn't support modes\n"
                          , cp->host
                          , (cp->nickname ? cp->nickname : cp->name)
                          , channel);
            appendstring(cp, buffer);
          }
          else
          {
            sprintf(buffer, ":%s 403 %s #%d :No such channel\n"
                          , cp->host
                          , (cp->nickname ? cp->nickname : cp->name)
                          , channel);
            appendstring(cp, buffer);
          }
        }
      }

      return;
    }
    else
    {
      if (cp->type == CT_USER)
        return;

      oldflags = ch->flags;

      while (*arg)
      {
        switch (*arg)
        {
        case '+':
          {
            remove = 0;
            arg++;
            break;
          }

        case '-':
          {
            remove = 1;
            arg++;
            break;
          }

        case 'S':
        case 's':
          {
            if (remove && ch)
            {
              if (!cp->IrcMode)
              {
                ch->flags -= (ch->flags & M_CHAN_S);
              }
              else
              {
                ch->flags -= (ch->flags & M_CHAN_I);
              }
            }
            else
            {
              if (!cp->IrcMode)
              {
                ch->flags |= M_CHAN_S;
              }
              else
              {
                ch->flags |= M_CHAN_I;
              }
            }

            arg++;
            break;
          }

        case 'P':
        case 'p':
          {
            if (remove && ch)
            {
              if (!cp->IrcMode)
              {
                ch->flags -= (ch->flags & M_CHAN_P);
              }
              else
              {
                ch->flags -= (ch->flags & M_CHAN_S);
              }
            }
            else
            {
              if (!cp->IrcMode)
              {
                ch->flags |= M_CHAN_P;
              }
              else
              {
                ch->flags |= M_CHAN_S;
              }
            }

            arg++;
            break;
          }

        case 'T':
        case 't':
          {
            if (remove && ch)
            {
              ch->flags -= (ch->flags & M_CHAN_T);
            }
            else
            {
              ch->flags |= M_CHAN_T;
            }

            arg++;
            break;
          }

        case 'I':
        case 'i':
          {
            if (remove && ch)
            {
              if (!cp->IrcMode)
              {
                ch->flags -= (ch->flags & M_CHAN_I);
              }
              else
              {
                ch->flags -= (ch->flags & M_CHAN_P);
              }
            }
            else
            {
              if (!cp->IrcMode)
              {
                ch->flags |= M_CHAN_I;
              }
              else
              {
                ch->flags |= M_CHAN_P;
              }
            }

            arg++;
            break;
          }

        case 'L':
        case 'l':
          {
            if (!cp->IrcMode)
            {
              if (remove && ch)
              {
                ch->flags -= (ch->flags & M_CHAN_L);
              }
              else
              {
                ch->flags |= M_CHAN_L;
              }
            }
            else
            {
              sprintf(buffer, ":%s 472 %s %c :is unknown mode char to me\n"
                            , cp->host
                            , (cp->nickname ? cp->nickname : cp->name)
                            , *arg);
              appendstring(cp, buffer);
            }

            arg++;
            break;
          }

        case 'M':
        case 'm':
          {
            if (remove && ch)
            {
              ch->flags -= (ch->flags & M_CHAN_M);
            }
            else
            {
              ch->flags |= M_CHAN_M;
            }

            arg++;
            break;
          }

        case 'O':
        case 'o':
          {
            arg++;
            while (*arg)
            {
              CONNECTION *p_found = 0;
              int         status_really_changed = 0;
              char       *fromname = cp->name;
              char       *fromnickname = (cp->type == CT_HOST ? cp->name : cp->nickname);
              int         user_found = 0;

              while (*arg == ' ')
                arg++;

              if (*arg == ':')
                arg++;

              c = arg;

              while (*c != ' ')
              {
                if (*c != '\0')
                {
                  c++;
                }
                else
                  break;
              }

              if (*c != '\0')
              {
                *c++ = '\0';
              }

              if (cp->type != CT_HOST && channel < 0 )
                continue;

              clear_locks();

              if (cp->type == CT_HOST)
                cp->locked = 1;

              for (p = connections; p; p = p->next)
              {
                if (p->type != CT_USER)
                  continue;

                if (!(!strcasecmp(p->name, arg) || (cp->type == CT_USER && !strcasecmp(p->nickname, arg))))
                  continue;

                user_found = 1;

                if (channel == -1)
                {
                  if (!p_found || !p->operator)
                    p_found = p;

                  if (!status_really_changed)
                  {
                    status_really_changed = (p->operator == 0);
                  }
                }
                else
                {
                  if (p->channel == channel)
                  {
                    if (!p_found || !p->channelop)
                      p_found = p;

                    if (!status_really_changed)
                      status_really_changed = (p->channelop == 0);

                    p->channelop = 1;
                  }

                  for (cl = p->chan_list; cl; cl = cl->next)
                  {
                    if (cl->channel == p->channel)
                    {
                      if (!p_found || !p->channelop)
                        p_found = p;

                      if (!status_really_changed)
                        status_really_changed = (cl->channelop == 0);

                      cl->channelop = 1;
                    }
                  }
                }
              }

              if (p_found)
              {
                send_opermsg(fromnickname, p_found->host, fromname, (WORD)channel, 0);
              }
              else
              {
                if (cp->type == CT_USER)
                {
                  if (cp->IrcMode)
                  {
                    if (user_found)
                      sprintf(buffer, ":%s 441 %s %s #%d :They aren't on that channel\n"
                                    , cp->host
                                    , cp->nickname
                                    , arg
                                    , channel);
                    else
                      sprintf(buffer, ":%s 401 %s %s :No such nick\n"
                                    , cp->host
                                    , cp->nickname
                                    , arg);

                    appendstring(cp, buffer);
                  }
                  else
                  {
                    if (user_found)
                      sprintf(buffer, "*** User not in channel: %s.\n"
                                    , arg);
                    else
                      sprintf(buffer, "*** No such user: %s.\n"
                                    , arg);

                    appendstring(cp, buffer);
                  }
                }
              }

              cp->locked = 1;
              arg = c;
            }
            break;

          }

        case 'N':
        case 'B':
        case 'b':
        case 'V':
        case 'v':
        case 'K':
        case 'k':
        case 'W':
        case 'w':
          {
            if (cp->IrcMode)
            {
              sprintf(buffer, ":%s 472 %s :Unknown MODE flag (%c).\n"
                            , cp->host
                            , (cp->nickname ? cp->nickname : cp->name)
                            , *arg);
              appendstring(cp, buffer);
            }

            arg++;
            break;
          }

        case 'n':
        default:
          {
            arg++;
            break;
          }
        }
      }

      if (ch && (ch->flags != oldflags))
      {
        clear_locks(); // send_opermsg may have locked it..
        cp->locked = 1;

        if (cp->type == CT_USER && !cp->IrcMode)
        {
          appendstring(cp, "*** Flags: ");
          // appendstring(cp, get_mode_flags(ch->flags));
          appendstring(cp, "\n");
        }

        //send_mode(cp, ch, oldflags);
      }
    }
  }
//else
  {
    if (!ch || ((ch->flags & M_CHAN_S || ch->flags & M_CHAN_I) && !(cp->operator == 2 || cl)))
    {
      sprintf(buffer, ":%s 403 %s #%d :No such channel\n"
                    , cp->host
                    , (cp->nickname ? cp->nickname : cp->name)
                    , channel);
      appendstring(cp, buffer);
    }
    else
    {
      sprintf(buffer, ":%s 482 %s #%d :You're not channel operator\n"
                    , cp->host
                    , (cp->nickname ? cp->nickname : cp->name)
                    , channel);
      appendstring(cp, buffer);
    }
  }

  if (cp->type == CT_USER)
  {
    appendprompt(cp, 0);
  }
}

/* Private MSG verschicken. */
void IrcPrivmsgCommand(CONNECTION *cp)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcPrivmsgCommand)";
#endif /* DEBUG_MODUS */

  msg_command(cp);
}

/* Channel schliessen. */
void IrcPartCommand(CONNECTION *cp)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcPartCommand)";
#endif /* DEBUG_MODUS */

  leave_command(cp);
}

/* Help Kommando */
void IrcHelpCommand(CONNECTION *cp)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcHelpCommand)";
#endif /* DEBUG_MODUS */

  help_command(cp);
}

/* Quit Kommando */
void IrcQuitCommand(CONNECTION *cp)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcQuitCommand)";
#endif /* DEBUG_MODUS */

  bye_command(cp);
}

void IrcPingCommand(CONNECTION *cp)
{
  char *arg;
  char buffer[2048];
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcPingCommand)";
#endif /* DEBUG_MODUS */

  arg = getarg(0, 0);

  arg[cp->width] = 0;

  sprintf(buffer, ":%s PONG %s :%s\n", myhostname, myhostname, arg);
  appendstring(cp, buffer);
}

void IrcPongCommand(CONNECTION *cp)
{
  char   buffer[1024];
  char  *line;
  time_t response_time = 0L;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcPongCommand)";
#endif /* DEBUG_MODUS */

  line = getarg(0, 0);

  if (*line == ':')
    line++;

  if (isalpha(*line))
  {
    sprintf(buffer, "PING :%ld\n", currtime);
    appendstring(cp, buffer);
    return;
  }

  sscanf(line, "%ld", &response_time);

  if (response_time < 1L)
  {
    sprintf(buffer, ":%s 375 %s bad answer, sorry\n"
                  , myhostname
                  , (*cp->nickname ? cp->nickname : cp->name));
    appendstring(cp, buffer);
  }
  else
  {
    ts2();
    sprintf(buffer, ":%s 375 %s *** rtt %s <-> %s is %s\n"
                  , myhostname
                  , (*cp->nickname ? cp->nickname : cp->name)
                  , myhostname
                  , (*cp->name ? cp->name : "you")
                  , ts4(currtime - response_time));
    appendstring(cp, buffer);
  }
  appendprompt(cp, 0);
}

/* Alle User auf den aktuellen Channel bereitstellen. */
void IrcNamesCommand(CONNECTION *cp)
{
  CONNECTION *p;
  CHANNEL    *ch;
  CLIST      *cl2;
  char        buffer[BUFSIZ];
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcNamesCommand)";
#endif /* DEBUG_MODUS */

  /* IRC-Meldung vorbereiten. */
  sprintf(buffer,":%s 353 %s = #%d :"
                ,myhostname
                ,cp->nickname
                ,cp->channel);
  /* Und abschicken. */
  appendstring(cp,buffer);

  for (ch = channels; ch; ch = ch->next)
  {
    /* Aktueller Channel? */
    if (ch->chan != cp->channel)
      /* Nein, zum naechsten. */
      continue;

    for (p = connections; p; p = p->next)
    {
      /* Nur angemeldete User. */
      if (p->type == CT_USER)
      {
       /* if (ch->chan != cp->channel)
          break;*/
        /* User im gesuchten Channel. */
        if (p->channel != ch->chan)
        {
          /* Nein, dann suchen wir in der Channel-List. */
          for (cl2 = p->chan_list; cl2; cl2 = cl2->next)
          {
            /* User im gesuchten Channel. */
            if (cl2->channel == ch->chan)
              /* Eintrag gefunden. */
              break;
          }

          /* Kein Eintrag gefunden? */
          if (!cl2)
            /* Dann zum naechsten. */
            continue;
        }

        /* Username - IRC-Meldung vorbereiten. */
        sprintf(buffer, "%s%s "
                      ,(p->channelop ? "@" : "")
                      , (*p->nickname ? p->nickname : p->name));
        /* Und abschicken. */
        appendstring(cp,buffer);
      }
    }
  }
  /* LISTENDE - IRC-Meldung vorbereiten. */
  sprintf(buffer, "\n:%s 366 %s #%d\n"
                ,myhostname
                ,cp->nickname
                ,cp->channel);
  /* Und abschicken. */
  appendstring(cp, buffer);
}

/* Auflistung aller User im angegebenen Kanal. */
void IrcWhoCommand(CONNECTION *cp)
{
  CONNECTION *p;
  CHANNEL    *ch;
  CLIST      *cl2;
  char        buffer[BUFSIZ];
  char       *chan;
  WORD        Channel;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcWhoCommand)";
#endif /* DEBUG_MODUS */

  /* Gesuchten Channel holen. */
  chan = getarg(NULL, GET_NXTLC);

  /* 1. Zeichen eine "#"? */
  if (*chan == '#')
    /* "#" entfernen. */
    chan++;

  Channel = atoi(chan);

  for (ch = channels; ch; ch = ch->next)
  {
    for (p = connections; p; p = p->next)
    {
      /* Nur angemeldete User. */
      if (p->type == CT_USER)
      {
        /* Unser Channel? */
        if (ch->chan != Channel)
          /* Zum naechsten. */
          break;

        /* Hat unser User den gesuchten Channel? */
        if (p->channel != ch->chan)
        {
          /* Nein, dann schauen wir noch in die Channel-Liste. */
          for (cl2 = p->chan_list; cl2; cl2 = cl2->next)
          {
            /* Hat unser User den gesuchten Channel? */
            if (cl2->channel == ch->chan)
              /* Channel gefunden. */
              break;
          }
          /* Eintrag gefunden. */
          if (!cl2)
            /* Nein, weitersuchen. */
            continue;
        }

        /* IRC-Meldung vorbereiten. */
        sprintf(buffer,":%s 352 %s %d %s%s %s %s %s %s%s%s :%d %s\n"
                      ,myhostname
                      ,cp->nickname
                      ,p->channel
                      ,""
                      ,p->name
                      ,p->host
                      ,p->host
                      ,p->nickname
                      ,(p->away ? "G" : "H")
                      ,""
                      ,(p->operator ? "@" : "")
                      ,0
                      ,(p->pers ? p->pers : ""));
        /* Und abschicken. */
        appendstring(cp, buffer);
      }
    }
  }

  /* LISTENDE - IRC-Meldung vorbereiten. */
  sprintf(buffer, ":%s 315 %s #%s\n"
                ,myhostname
                ,cp->nickname
                ,chan);
  /* Und abschicken. */
  appendstring(cp,buffer);
}

void IrcLinksCommand(CONNECTION *cp)
{
  DESTINATION *d;
  char         buffer[2048];
  char         *dest = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcLinksCommand)";
#endif /* DEBUG_MODUS */

  if (cp->type == CT_USER)
    dest = getarg(0, 0);
  else
    dest = "";

  if (*dest == '\0')
  {
    for (d = destinations; d; d = d->next)
    {
      if (d->rtt)
      {
        if (d->name[0] == FALSE)
          continue;

        sprintf(buffer, ":%s 364 %s %s %s :%ld %s\n"
                      , cp->host
                      , (cp->nickname ? cp->nickname : cp->name)
                      , d->name
                      , (d->link->name ? d->link->name : myhostname)
                      , d->rtt
                      , d->rev);
        appendstring(cp, buffer);
      }
    }
  }

  sprintf(buffer, ":%s 364 %s %s %s :0 %s\n"
                , cp->host
                , (cp->nickname ? cp->nickname : cp->name)
                , cp->host
                , cp->host
                , "pp-3.14c");
  appendstring(cp, buffer);

  sprintf(buffer, ":%s 365 %s * :End of /LINKS list.\n"
                , cp->host
                , (cp->nickname ? cp->nickname : cp->name));
  appendstring(cp, buffer);
  appendprompt(cp, 0);
}

/* Auflisten aller aktuellen Channels, Kanal, Useranzahl und Topic. */
void IrcListCommand(CONNECTION *cp)
{
  CHANNEL    *ch;
  CONNECTION *p;
  CLIST      *cl;
  char        buffer[BUFSIZ];
  int         n;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcListCommand)";
#endif /* DEBUG_MODUS */

  for (ch = channels; ch; ch = ch->next)
  {
    /* Userzaehler auf 0 setzen. */
    n = 0;
    for (p = connections; p; p = p->next)
    {
      /* Nur angemeldete User. */
      if (p->type == CT_USER)
      {
        if (p->channel == ch->chan)
          /* Userzaehler um eins hoeher. */
          n++;
        else
          {
            for (cl = p->chan_list; cl; cl = cl->next)
              /* Hat unser User den gesuchten Channel? */
              if (cl->channel == ch->chan)
                /* Channel gefunden. */
                break;

            /* Eintrag gefunden? */
            if (cl)
              /* Userzaehler um eins hoeher. */
              n++;
          }
      }
    }

    /* IRC-Meldung vorbereiten. */
    sprintf(buffer, ":%s 322 %s #%d %d :%s\n"
                  ,cp->host
                  ,(*cp->nickname ? cp->nickname : cp->name)
                  ,ch->chan
                  ,n
                  ,(ch->topic ? ch->topic : ""));

    /* Und abschicken. */
    appendstring(cp, buffer);
  }

  /* LISTENDE - IRC-Meldung vorbereiten. */
  sprintf(buffer, ":%s 323 %s\n"
                ,cp->host
                ,(*cp->nickname ? cp->nickname : cp->name));

  /* Und abschicken. */
  appendstring(cp,buffer);
}

/* Informationen eines Users. */
void IrcWhoisCommand(CONNECTION *cp)
{
  char buffer[BUFSIZ];
  char *username;
  char *revision;
  CONNECTION  *p, *c;
  CHANNEL     *ch;
  DESTINATION *d;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcWhoisCommand)";
#endif /* DEBUG_MODUS */

  /* Username holen. */
  username = getarg(0, 1);

  for (p = connections; p; p = p->next)
  {
    /* User angemeldet. */
    if (p->type == CT_USER)
    {
      /* Gesuchter Username? */
      if (  (strcmp(p->name, username) == FALSE)
          ||(strcmp(p->nickname, username) == FALSE))
      {
        /* IRC-Meldung vorbereiten. */
        sprintf(buffer, ":%s 311 %s %s %s%s %s * :%s%s\n"
                      ,myhostname
                      ,cp->nickname
                      ,p->nickname
                      ,""
                      ,p->name
                      ,p->host
                      ,(p->pers ? p->pers : "")
                      ,(p->pers == "~" ? " [NonAuth]" : ""));
        /* Und abschicken. */
        appendstring(cp,buffer);

        /* User als Sysop angemeldet. */
        if (p->operator)
        {
          /* IRC-Meldung vorbereiten. */
          sprintf(buffer, ":%s 313 %s %s :is an IRC operator\n"
                        ,myhostname
                        ,cp->nickname
                        ,p->nickname);
          /* Und abschicken. */
          appendstring(cp,buffer);
        }

        for (c = connections; c; c = c->next)
        {
          /* Nur angemeldete User. */
          if (c->type != CT_USER)
            /* Ein unangemeldeter oder HOST, zum naechsten. */
            continue;

          /* Vergleiche Username. */
          if (strcmp(p->name, c->name))
            /* Nicht unser gesuchter Username, zum naechten. */
            continue;

            /* Aktuellen Channel suchen. */
            for (ch = channels; ch; ch = ch->next)
              if (ch->chan == c->channel)
                /* gefunden. */
                break;

            /* IRC-Meldung vorbereiten. */
            sprintf(buffer, ":%s 319 %s %s :%s %d \n"
                          ,myhostname
                          ,cp->nickname
                          ,p->nickname
                          ,(c->channelop ? "@" : "")
                          ,c->channel);
            /* Und abschicken. */
            appendstring(cp, buffer);
          }

        /* User eigner Knoten. */
        if (!strcasecmp(p->host, myhostname))
          /* Markieren wir die Revision, */
          revision = strchr(REV, ':')+2;

        /* Pruefe alle Destinationen. */
        for (d = destinations; d; d = d->next)
          /* Vergleiche Hostname. */
          if (!strcasecmp(d->name, p->host))
           /* gefunden. */
           break;

        /* Revision, IRC-Meldung vorbereiten. */
        sprintf(buffer, ":%s 312 %s %s %s :%s\n"
                      ,myhostname
                      ,cp->nickname
                      ,p->nickname
                      ,p->host
                      ,(d ? d->rev : revision));
        /* Und abschicken. */
        appendstring(cp,buffer);

        /* Online, IRC-Meldung vorbereiten. */
        sprintf(buffer, ":%s 317 %s %s %ld %ld :seconds idle, signon time\n"
                      ,myhostname
                      ,cp->nickname
                      ,p->nickname
                      ,(currtime - p->mtime), p->time);
        /* Und abschicken. */
        appendstring(cp,buffer);
      }
    }
  }

  /* LISTENDE - IRC-Meldung vorbereiten. */
  sprintf(buffer, ":%s 318 %s %s\n"
                ,myhostname
                ,cp->nickname
                ,username);
  /* Und abschicken. */
  appendstring(cp,buffer);
}

/* Username aufloesen. */
void IrcUserCommand(CONNECTION *cp)
{
  char *user, *pers;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcUserCommand)";
#endif /* DEBUG_MODUS */

  /* User erstmal sperren .*/
  cp->locked = 1;
  /* Kanal wird spaeter gesetzt. */
  cp->channel = EOF;

  /* Ist es ein Host, ist der hier verkehrt. */
  if (cp->type == CT_HOST)
    /* Und weg damit. */
    return;

  /* Username holfen. */
  user = getarg(0, 0);

  /* Personal Text einlesen & speichern. */
  personalmanager(SET, cp, (pers = getarg(NULL, GET_ALL)));
  /* Personal Text setzen, max. zeichen 256!. */
  setstring(&(cp->pers), pers, 256);

  /* Username sichern. */
  strncpy(cp->name, user, NAMESIZE + 1);

  /* Verbose, defaultwert setzen. */
  cp->verbose = 0;
}

/******************************************************************************/
/*                                                                            */
/* Gibt Informationen ueber die Anzahl von Verbindungen, Global und Lokal.     */
/*                                                                            */
/******************************************************************************/
static void IrcLusersCommand(CONNECTION *cp)
{
  char         buffer[2048];
  DESTINATION *d;
  CHANNEL     *ch;
  CONNECTION  *p;
  int          n_users = 0;                                    /* Useranzahl. */
  int          n_dest,                                 /* Destination/Routen. */
               n_channels,                                    /* Kanalanzahl. */
               n_clients,                                      /* IRC-Client. */
               n_servers,                                      /* IRC-Server. */
               n_operators,                                /* Operatoranzahl. */
               n_typeunknown;                                   /* Unbekannt. */
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcLusersCommand)";
#endif /* DEBUG_MODUS */

  for (n_dest = 0, d = destinations; d; d = d->next)  /* Dest/Routen zaehlen. */
  {
    if (d->rtt && d->link)                            /* Dest/Route gefunden. */
      n_dest++;                                          /* um eins erhoehen. */
  }

  for (n_channels = 0, ch = channels; ch; ch = ch->next)  /* Kanaele zaehlen. */
  {
    if (!(ch->flags & M_CHAN_I))       /* keine unsichtbaren kanaele zaehlen. */
      n_channels++;                                      /* um eins erhoehen. */
  }

  for (n_clients = n_servers = n_typeunknown = n_operators = 0, p = connections; p; p = p->next)
  {
    switch (p->type)
    {
      case CT_HOST:
        n_servers++;
        break;

      case CT_USER:
        n_users++;

        if (!p->via)
          n_clients++;

        if (p->operator)
          n_operators++;

        break;

      case CT_UNKNOWN:
      default:
        n_typeunknown++;
    }
  }

  sprintf(buffer, ":%s 251 %s :Hier sind %d User auf %d Server\n", myhostname, cp->nickname, n_users, (n_dest) ? n_dest+1 : 1) ;
  appendstring(cp, buffer);

  if (n_operators)
  {
    sprintf(buffer, ":%s 252 %s %d Sysop%s online\n", myhostname, cp->nickname, n_operators, (n_operators != 1) ? "s" : "");
    appendstring(cp, buffer);
  }

  if (n_typeunknown)
  {
    sprintf(buffer, ":%s 253 %s %d :Ungueltige Verbindung%s\n", myhostname, cp->nickname, n_typeunknown, (n_typeunknown != 1) ? "en" : "");
    appendstring(cp, buffer);
  }

  sprintf(buffer, ":%s 254 %s %d :Kanael%s\n", myhostname, cp->nickname, n_channels, (n_channels != 1 ? "e" : ""));
  appendstring(cp, buffer);

  sprintf(buffer, ":%s 255 %s :Ich habe %d client%s auf %d Server\n", myhostname, cp->nickname, n_clients, (n_clients != 1) ? "s" : "", n_servers);
  appendstring(cp, buffer);
}

/******************************************************************************/
/*                                                                            */
/* Zusaetzlich Text an IRC-Client senden.                                     */
/*                                                                            */
/******************************************************************************/
static void IrcMotdCommand(CONNECTION *cp)
{
  FILE *fp;
  char  motdfile[MAXPATH + 1];
  char  buffer[80 + 1];
  char  buffer2[128];
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcMotdCommand)";
#endif /* DEBUG_MODUS */

  strcpy(motdfile, confpath);
  strcat(motdfile, IRC_TXT);

  if ((fp = xfopen(motdfile, "r")) == NULL)           /* Fehler beim oeffnen. */
    return;                                                     /* Abbrechen. */

  sprintf(buffer2, ":%s 372 %s :"
                , myhostname
                , cp->nickname);

  while (fgets(buffer, 80, fp))               /* Maximal 80 Zeichen einlesen. */
  {
    appendstring(cp, buffer2);                                /* Code-String. */
    appendstring(cp, buffer);                              /* Datei einlesen. */
  }

  sprintf(buffer, "\n:%s 376 %s :\n"
                , myhostname
                , cp->nickname);
  appendstring(cp, buffer);

  fclose(fp);                                            /* Datei schliessen. */
}

/******************************************************************************/
/*                                                                            */
/* IRC-Client anmelden.                                                       */
/*                                                                            */
/******************************************************************************/
static void IrcLogin(CONNECTION *cp)
{
  char buffer[2048];
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcLogin)";
#endif /* DEBUG_MODUS */

  cp->charset_in = cp->charset_out = ISO_STRIPED;

  sprintf(buffer, ":%s 001 %s :%s @ %s PingPong-Release %5.5s (TNN) - Type /HELP for help.\n"
                , myhostname
                , cp->nickname
                , convtype
                , myhostname
                , strchr(REV, ':')+2);
  appendstring(cp, buffer);

  sprintf(buffer, ":%s 004 %s %s :\n"
                , myhostname
                , cp->nickname
                , myhostname);

  appendstring(cp, buffer);

  IrcLusersCommand(cp);        /* Anzahl User, Server, Kanaele etc. ausgeben. */
  IrcMotdCommand(cp);               /* Zusaetzlichen Text ausgeben (irc.txt). */

  cp->width = 80;                                     /* Zeilengrenze setzen .*/
}

/******************************************************************************/
/*                                                                            */
/* Nickname aufloesen.                                                        */
/*                                                                            */
/******************************************************************************/
void IrcNickCommand(CONNECTION *cp)
{
  char *arg;
  char buffer[2048];
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcNickCommand)";
#endif /* DEBUG_MODUS */

  arg = getarg(0, 0);                                   /* Nickname einlesen. */

  if (cp->type == CT_HOST)                                          /* Host ? */
    return;                                                 /* Und weg damit. */

  if (cp->away != NULL)                    /* Wenn ein Away-Text gesetzt ist, */
  {
    cp->away = NULL;                                        /* zuruecksetzen. */

#ifndef CONVNICK
  send_awaymsg(cp->name,    /* Neuen Away-Text an andere Host's weiterleiten. */
               cp->hosthostname,
               currtime,
               cp->away);
#else
  send_awaymsg(cp->name,    /* Neuen Away-Text an andere Host's weiterleiten. */
               cp->nickname,
               cp->host,
               currtime,
               cp->away);
#endif /* CONVNICK */
    return;
  }

  if (*arg == ':')                                  /* Gibt es ":" im string? */
    arg++;                                                   /* ":" loeschen. */

  if (!*arg)                                                  /* String leer? */
  {
    sprintf(buffer, ":%s 431 * :Kein Nickname angegeben!\n"
                  , myhostname);
    appendstring(cp, buffer);
    return;
  }

#ifdef CONV_CHECK_USER
  {
    CONNECTION *pP;

    if ((pP = CheckUserCVS(arg)) != NULL)       /* Pruefung auf Doppel-Login. */
    {                                                 /* Kein Login moeglich. */
      sprintf(buffer, ":%s 433 %s %s :Nickname already in use\n"
                    , myhostname
                    , (*cp->nickname ? cp->nickname : "*")
                    , arg);
      appendstring(cp, buffer);
      return;
    }
  }
#endif /* CONV_CHECK_USER */

  if (*cp->nickname)                                     /* Schon eingeloggt. */
  {
    sprintf(buffer, ":%s!%s@%s NICK :%s\n"             /* Meldung abschicken. */
                  , cp->nickname
                  , cp->name
                  , myhostname
                  , arg);
    appendstring(cp, buffer);                          /* Meldung abschicken. */

    strncpy(cp->OldNickname, cp->nickname, NAMESIZE);/* Alten Nick eintragen. */
    cp->nickname[NAMESIZE] = 0;

    strncpy(cp->nickname, arg, NAMESIZE);            /* Neuen Nick eintragen. */
    cp->nickname[NAMESIZE] = 0;
    return;
  }

  strncpy(cp->nickname, arg, NAMESIZE);                    /* Nick eintragen. */
  cp->nickname[NAMESIZE] = 0;

  if (!*cp->name)                              /* Noch kein Username bekannt? */
  {
    strncpy(cp->name, arg, NAMESIZE + 1);/* Nehmen wir den Nickn als Username.*/
    cp->name[NAMESIZE + 1] = 0;
  }

  sprintf(buffer, ":%s!%s@%s NICK :%s\r"          /* IRC-Meldung vorbereiten. */
                , cp->nickname
                , cp->name
                , myhostname
                , arg);
  appendstring(cp, buffer);                                /* Und abschicken. */

  strncpy(cp->host, myhostname, NAMESIZE);             /* Hostname eintragen. */
  cp->host[sizeof(cp->host)-1] = 0;

  cp->type = CT_USER;                                  /* Als User markieren. */

  IrcLogin(cp);                                             /* IRC-Logintext. */
}

/******************************************************************************/
/*                                                                            */
/* Nickname an andere IRC-Client's weiterleiten.                              */
/*                                                                            */
/******************************************************************************/
void SendIrcNick(CONNECTION *p)
{
  CONNECTION *p2;
  CLIST      *cl,
             *cl2;
  char        buffer[2048];
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(SendIrcNick)";
#endif /* DEBUG_MODUS */

  for (p2 = connections; p2; p2 = p2->next)
  {
    if (p2->locked)                                  /* Sperre eingeschaltet. */
      continue;                                     /* zum naechsten Eintrag. */

    if (p2->type == CT_USER)                                    /* Type User. */
    {
      if (p2 == p)                          /* mich selber nicht weitersagen. */
        continue;                                   /* zum naechsten eintrag. */

      if (p2->channel != p->channel)                /* Kanal unterschiedlich. */
      {
        for (cl2 = p2->chan_list; cl2; cl2 = cl2->next)
        {
          if (cl2->channel == p->channel)
            break;
          else
          {
            for (cl = p->chan_list; cl; cl = cl->next)
            {
              if (cl->channel == cl2->channel)
                break;
            }
          }
        }
      }

      if (p2->IrcMode == FALSE)                            /* Kein IRC-Client */
        continue;                                   /* zum naechsten Eintrag. */
                                                          /* Keine Aenderung, */
      if (  (!strncasecmp(p->OldNickname, p->nickname, NAMESIZE))
          ||(!strncasecmp(p->name, p->nickname, NAMESIZE)))
        continue;                                   /* zum naechsten Eintrag. */

                                               /* Aktuellen Nick weitersagen. */
      sprintf(buffer, ":%s!%s@%s NICK %s\n"
                    , (p->OldNickname[0] == FALSE ? p->name : p->OldNickname)
                    , p->name
                    , p->host
                    , p->nickname);
      appenddirect(p2, buffer);
      break;
    }
  }
}

/******************************************************************************/
/*                                                                            */
/* Informationen vom User ausgeben.  .                                        */
/*                                                                            */
/******************************************************************************/
void IrcUserHostCommand(CONNECTION *cp)
{
  CONNECTION *p;
  char        buffer[2048];
  char        buffer2[2048];
  int         found = 0;
  char       *user;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcUserHostCommand)";
#endif /* DEBUG_MODUS */

  user = getarg(0, 0);                                      /* User einlesen. */

  if (!*user)                                         /* Kein User angegeben. */
  {                                               /* IRC-Meldung vorbereiten  */
    sprintf(buffer, ":%s 461 %s USERHOST :Not enough parameters\n"
                  , myhostname
                  , cp->nickname);
    appendstring(cp, buffer);                              /* und abschicken. */
    return;
  }

  sprintf(buffer2, ":%s 302 %s :"                 /* IRC-Meldung vorbereiten  */
                 , myhostname
                 , cp->nickname);

  while (*user)
  {
    p = 0;
    if (  (!strcasecmp(cp->nickname, user))   /* Uservergleich mit Nick/Name. */
        ||(!strcasecmp(cp->name,     user)))
      p = cp;                                            /* Eintrag gefunden. */
    else                                            /* Kein Eintrag gefunden. */
    {
      for (p = connections; p; p = p->next)        /* durchsuche convers-TBL. */
      {
        if (  (!strcasecmp(p->nickname, user))/* Uservergleich mit Nick/Name. */
            ||(!strcasecmp(p->name,     user)))
          break;                                         /* Eintrag gefunden. */
      }
    }

    if (!p)                                            /* Kein User gefunden. */
    {
      sprintf(buffer, ":%s 401 %s %s :No such nick\n"
                    , myhostname
                    , (*cp->nickname ? cp->nickname : cp->name)
                    , user);
      appendstring(cp,buffer);
    }
    else                                                    /* User gefunden. */
    {
      sprintf(buffer, "%s%s%s=%c%s%s@%s"
                    , (found) ? " " : ""
                    , (*p->nickname ? p->nickname : p->name)
                    , (p->operator ? "*" : "")
                    , (p->away ? '-' : '+')
                    , ""
                    , p->name
                    , p->host);

      if (strlen(buffer) + strlen(buffer2) > 510) /* Buf-laenge ueberschritten*/
        break;                                                  /* Abbrechen. */

      strcat(buffer2, buffer);                    /* Informationen anhaengen. */

      if (!found)                                /* Gibt noch keinen Eintrag. */
        found++;                                           /* min. 1 Eintrag. */
    }

    user = getarg(0, 0);                          /* Naechsten User einlesen. */
  }

  if (found)                                             /* Eintrag gefunden. */
  {
    appendstring(cp, buffer2);
    appendstring(cp, "\n");
  }
}

/******************************************************************************/
/*                                                                            */
/* Beendet die Verbindung eines Server's vom Netzwerk.                        */
/*                                                                            */
/******************************************************************************/
void IrcSquitCommand(CONNECTION *cp)
{
  char  buffer[2048];
  char *arg;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcSquitCommand)";
#endif /* DEBUG_MODUS */

  arg = getarg(0, 1);                                     /* Server einlesen. */

  if (!*arg)                                        /* kein Server angegeben. */
  {
    sprintf(buffer, ":%s 461 %s SQUIT :Not enough parameters\n"
                  , myhostname
                  , cp->nickname);
    appendstring(cp, buffer);
    return;
  }

  if (cp->operator != 2)                      /* Kein Sysop, keine Aenderung. */
  {
    sprintf(buffer, ":%s 481 %s :Permission Denied- You're not an IRC operator\n"
                  , myhostname
                  , cp->name);
    appendstring(cp, buffer);
    return;
  }

  sprintf(buffer, "/link reset %s", arg);        /* Reset-Befehl vorbereiten. */
  getarg(buffer, 0);

  links_command(cp);                                        /* Server killen. */
}

/******************************************************************************/
/*                                                                            */
/* Username an IRC-Server weiterleiten.                                       */
/*                                                                            */
/******************************************************************************/
void IrcLinkUser(char *user)
{
  char        *scall;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcLinkUser)";
#endif /* DEBUG_MODUS */

  if (tcppoi->Interface != KISS_IRC)                   /* Kein IRC-Interface. */
  {
    user[0] = 0;                                      /* Rufzeichen loeschen. */
    tcppoi->disflg |= 0x80;                         /* Verbindung schliessen. */
    return;
  }

  if ((scall = strchr(user, ' ')) != FALSE)       /* Zum Rufzeichen springen. */
  {
    scall++;                                        /* Leerzeichen entfernen. */
    strncpy(user, scall, L2CALEN);                     /* Rufzeichen sichern. */
    user[6] = 0;                                        /* Sicher ist sicher. */
  }
}

/******************************************************************************/
/*                                                                            */
/* Nickname an IRC-Server weiterleiten.                                       */
/*                                                                            */
/******************************************************************************/
void IrcLinkNick(char *header)
{
  READRX      *SRx;
  register int i;
#ifdef DEBUG_MODUS
  lastfunc = "l1irc(IrcLinkNick)";
#endif /* DEBUG_MODUS */

  if (header[0] == FALSE)                             /* Kein Nick angegeben. */
    return;                                                     /* Abbrechen. */

  if ((SRx = (READRX *)allocb(ALLOC_L1TCPIP)) == NULL)    /* RX-Seg. besorgen */
    return;                                    /* Kein Segment frei, abbruch. */

  if ((SRx->Data = SetBuffer()) == NULL)                  /* Buffer besorgen. */
  {
    dealoc((MBHEAD *)ulink((LEHEAD *)SRx));          /* RX-Segment entsorgen. */
    return;                                      /* Buffer ist voll, abbruch. */
  }

  SRx->Sock      = tcppoi->sock;                            /* Socket setzen. */
  SRx->Interface = tcppoi->Interface;                    /* Interface setzen. */
  SRx->Mode      = tcppoi->mode;                        /* Stack-Mode setzen. */

  for (i = 0; i < (signed)strlen(header); ++i)         /* Nickname eintragen. */
    putchr(header[i], SRx->Data);

  relink((LEHEAD *) SRx, (LEHEAD *)rxflRX.tail);       /* RX-Liste umhaengen. */
}

#endif /* L1IRC */

/* End of src/l1irc.c */
