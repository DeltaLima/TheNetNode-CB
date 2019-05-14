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
/* File src/l7utils.c (maintained by: ???)                              */
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

static char *search_file(char *, char *);

#ifdef MAKRO_USER
static UWORD Interlinks_zaehlen(void);
#endif

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void invcal(void)
{
#ifdef SPACHE
  putmsg(speech_message(167));
#else
  putmsg("Invalid Call\r");
#endif
}

#ifdef CONNECTMOD_MSG
/**************************************************************************/
/*                                                                        */
/* Status-Meldungen "*** connected to" bzw. "*** reconnected to"          */
/* ausgeben.                                                              */
/*                                                                        */
/*------------------------------------------------------------------------*/
MBHEAD *PutStatusMSG(const char *string)
{
  MBHEAD *mbp;

  mbp = getmbp();

  if (strncasecmp(string, "R" ,1) == FALSE)             /* Reconnect-Meldung. */
    putstr("\r", mbp);                            /* Zeilenumbruch einfuegen. */

  putprintf(mbp, "*** %s", string);
  return(mbp);
}
#endif /* CONNECTMOD_MSG */

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void msgfrm(LINKTYP uplink, UID uid, char *msg)
{
  MBHEAD *mbp;
  UBYTE   user = g_utyp(uid);
  int     index;
  NODE   *np;

#ifndef CONNECTMOD_MSG
  mbp = putals(msg);
#else
  mbp = PutStatusMSG(msg);
#endif /* CONNECTMOD_MSG */

  if (uid != NO_UID)
   {
      if ((user == L4_USER) && (uplink != CQ_LINK))
       {
        index = find_node_this_ssid(((CIRBLK *)g_ulink(uid))->downca);
        if (index != -1)
         {
          np = netp->nodetab+index;
          putalt(np->alias, mbp);
         }
       }
    putid(calofs(uplink, uid), mbp);
  } else {
    putalt(alias, mbp);
    putid(myid, mbp);
  }
  putchr('\r', mbp);
  if (msg != conmsg)
    prompt(mbp);
  seteom(mbp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void puttfu(const char *name)
{
  MBHEAD *mbp;

#ifdef SPEECH
  putstr(speech_message(279), (mbp = putals(name)));
#else
  putstr(" table full\r", (mbp = putals(name)));
#endif
  prompt(mbp);
  seteom(mbp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putmsg(const char *string)
{
  MBHEAD *mbp;

  mbp = putals(string);
  if (userpo->convflag == 0)
    prompt(mbp);
  seteom(mbp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
MBHEAD *putals(const char *string)
{
  MBHEAD *mbp;

  mbp = getmbp();
  putalt(alias, mbp);
  putid(myid, mbp);
  putstr("> ", mbp);
  putstr(string, mbp);
  return(mbp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
MBHEAD *getmbp(void)
{
  MBHEAD *mbp;

  mbp = (MBHEAD *) allocb(ALLOC_MBHEAD);
  mbp->l2link = g_ulink(userpo->uid);
  mbp->type   = g_utyp(userpo->uid);

  return(mbp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putuse(LINKTYP seite, UID uid, MBHEAD *mbp)
{
  CIRBLK *cp;
  LNKBLK *lp;
  HOSTUS *hp;
#ifdef L1TCPIP
  TCPIP  *tc;
#endif /* L1TCPIP */
#ifndef USER_AUSGABE
  int     index;
  NODE   *np;
#endif /* USER_AUSGABE */

  if (seite == CQ_LINK)
   {
    putstr("CQ(", mbp);
    switch (g_utyp(uid))
     {
      case L4_USER:
        putid(((CIRBLK *)g_ulink(uid))->upcall, mbp);
        break;

      case L2_USER:
        putid(((LNKBLK *)g_ulink(uid))->dstid, mbp);
        break;

#ifdef L1TCPIP
      case TCP_USER:
        putid(((TCPIP *)g_ulink(uid))->Upcall, mbp);
        break;
#endif /* L1TCPIP */

      default:
        putid(((HOSTUS *)g_ulink(uid))->call, mbp);
        break;
     }
    putstr(")\r", mbp);
    return;
   }

  switch (g_utyp(uid)) {
    case L4_USER:
      cp = g_ulink(uid);
#ifdef USER_AUSGABE
      if (seite == UPLINK)
      {
        putstr("Uplink(", mbp);
        putid(cp->upcall, mbp);
        putchr(')', mbp);
        putspa(18,mbp);
        putstr("N: ",mbp);
        putid(cp->downca, mbp);
      }
      else
        {
          putstr("(", mbp);
          putid(cp->upcall, mbp);
          putspa(52,mbp);
          putstr(" <> ", mbp);
          putid(cp->downca, mbp);
          putchr(')', mbp);
          putspa(65,mbp);
        }
#else /* USER_AUSGABE */
       putstr("Circuit(", mbp);
       index = find_node_this_ssid(cp->downca);

       if (index != -1)
       {
         np = netp->nodetab+index;
         putalt(np->alias, mbp);
       }

       putid(cp->downca, mbp);
       putchr(' ', mbp);
       putid(cp->upcall, mbp);
       putchr(')', mbp);
#endif /* USER_AUSGABE */
      break;

    case L2_USER:
      lp = g_ulink(uid);
#ifdef USER_AUSGABE
      if (seite == UPLINK)
      {
        putstr("Uplink(", mbp);
        putid(lp->dstid, mbp);
        putchr(')', mbp);
        putspa(18,mbp);
        putprintf(mbp,"P%-2u ",lp->liport);
        putprintf(mbp,"%-10s ",portpar[lp->liport].name);
      }
      else
        {
          putspa(40,mbp);
          putstr("(", mbp);
          putid(lp->srcid, mbp);
          putspa(52,mbp);
          putstr(" <> ", mbp);
          putid(lp->dstid, mbp);
          putchr(')', mbp);
          putspa(65,mbp);
          putprintf(mbp,"P%-2u ",lp->liport);
          putprintf(mbp,"%-10s",portpar[lp->liport].name);
#else /* USER_AUSGABE */

      if (seite == UPLINK) {
        putstr("Uplink(", mbp);
        putid(lp->dstid, mbp);
        putchr(')', mbp);
      }
      else {
        putstr("Downlink(", mbp);
        putid(lp->srcid, mbp);
        putchr(' ', mbp);
        putid(lp->dstid, mbp);
        putchr(')', mbp);
#endif /* USER_AUSGABE */
      }
      break;

#ifdef L1TCPIP
    case TCP_USER:
      tc = g_ulink(uid);
      if (seite == UPLINK) {
        putstr("Uplink(", mbp);
        putid(tc->Upcall, mbp);
        putchr(')', mbp);
        putspa(18,mbp);
        putprintf(mbp,"P%-2u ",tc->port);
        putprintf(mbp,"%-15s ",tc->ip);
          }
          else
          {
          putspa(40,mbp);
          putstr("(", mbp);
          putprintf(mbp,"%s",tc->ip);
          putchr(')', mbp);
          putspa(65,mbp);
          putprintf(mbp,"P%-2u ",tc->port);
          putprintf(mbp,"%-10s",portpar[tc->port].name);
          }
          break;
#endif /* L1TCPIP */


    default:
      hp = g_ulink(uid);
      putstr("Host(", mbp);
      if (cmpid(hostid, myid))
       {
        putalt(alias, mbp);
        putid(myid, mbp);
       }
      else
       {
        if (seite == UPLINK) putid(hp->call, mbp);
        else
         {
          putid(hp->call, mbp);
          putchr(' ', mbp);
          putid(hostid, mbp);
         }
       }
      putchr(')', mbp);
      break;
  }
}

static BOOLEAN issecret(WORD channel)
{
  CHANNEL *ch;

  for (ch = channels; ch; ch = ch->next)
    if (ch->chan == channel)
      break;
  if (ch->flags & (M_CHAN_S|M_CHAN_I))
    return(TRUE);
  return(FALSE);
}

/************************************************************************/
/* putcvsu                                                              */
/* Da die Verbindungen des Conversmodus so verdreht aufgebaut werden,   */
/* hier nun eine entsprechend verdrehte Ausgaberoutine         DL1XAO   */
/*----------------------------------------------------------------------*/
void putcvsu(USRBLK *u, MBHEAD *mbp)
{
  UID         uid = u->uid;
  CONNECTION *cp  = u->convers;

  if (u->convflag == 2) {                /* ausgehender Convershost */
#ifdef USER_AUSGABE
    putstr("Uplink", mbp);
        putstr("(", mbp);
    putid(myid, mbp);
    putchr(')', mbp);
    putspa(18,mbp);
    putstr("Convers", mbp);
#else
    putstr("Convers(", mbp);
    putid(myid, mbp); /* SSID egal? ja! */
    putchr(')', mbp);
#endif /* USER_AUSGABE */

    if (cp->type == CT_HOST)
      putstr("  Host", mbp);
    putspa(38, mbp);
    putstr(u->status == US_CHST ? "<..> "            /* im Aufbau? */
                                : "<--> ", mbp);

    putuse(DOWNLINK, uid, mbp);
    viaput(DOWNLINK, uid, mbp);
  }
  else {                             /* User und eingehende CVSHOST */
    putuse(UPLINK, uid, mbp);
    putspa(38, mbp);
#ifdef USER_AUSGABE
    putstr("<--> (", mbp);
    putid(calofs(UPLINK, uid), mbp);
    putspa(53,mbp);
    putstr("<> Convers)", mbp);
#else

    putstr("<-->  Convers(", mbp);
    putid(calofs(UPLINK, uid), mbp);
    putchr(')', mbp);
#endif /* USER_AUSGABE */
    if (   cp->type == CT_USER
        && (issecret(cp->channel) == FALSE || userpo->sysflg)) {
#ifdef USER_AUSGABE
    putspa(65,mbp);
#endif /*USER_AUSGABE */
#ifndef L1IRC
      putstr("Ch", mbp);
      putlong((LONG)cp->channel, 0, mbp);
#else
      if (cp->channel == EOF)
        putstr("IRC", mbp);
      else
      {
        putstr("Ch", mbp);
        putlong((LONG)cp->channel, 0, mbp);
      }
#endif /* L1IRC */
    }
    if (cp->type == CT_HOST)
      putstr("  Host", mbp);
    viaput(UPLINK, uid, mbp);
  }
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putdil(const char *liste, MBHEAD *mbp)
{
  BOOLEAN mark = TRUE;

  if (*liste) {
    putstr(" via", mbp);
    while (*liste) {
      putchr(' ', mbp);
      putid(liste, mbp);
      if (mark)
        if ((liste[L2IDLEN - 1] & L2CH) != 0)
          if (!liste[L2IDLEN] || !(liste[L2ILEN - 1] & L2CH)) {
            putchr('*', mbp);
            mark = FALSE;
        }
      liste += L2IDLEN;
    }
  }
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putid(const char *call, MBHEAD *mbp)
 {
  char   ssid;
  char   c;
  WORD   i;

  for (i = 0; i < L2IDLEN-1; ++i)
   {
    c = *call++;

    if (c > ' ') putchr(c, mbp);
    else
     {
      if (c < ' ')
       {
        putchr('^', mbp);
#ifdef __WIN32__
        putchr((char)(c + '@'), mbp);
#else
        putchr((c + '@'), mbp);
#endif /* WIN32 */
          }
     }
   }
  ssid = (*call >> 1) & 0x0f;
  if (ssid != 0) {
    putchr('-', mbp);
    putnum(ssid, mbp);
  }
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putalt(char *ident, MBHEAD *mbp)
{
  if (*ident != ' ') {
    putcal(ident, mbp);
    putchr(':', mbp);
  }
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putcal(char *call, MBHEAD *mbp)
{
  char *cp;
  WORD i;

  for (i = 0, cp = call; i < L2CALEN; ++cp, ++i) {
    if (*cp == ' ')
      break;
    putchr(*cp, mbp);
  }
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void puttim(time_t *time, MBHEAD *mbp)
{
  struct tm *p;

  p = localtime(time);
  putprintf(mbp,"%02d.%02d.%02d %02d:%02d:%02d",
          p->tm_mday,  p->tm_mon+1, p->tm_year % 100,
          p->tm_hour, p->tm_min, p->tm_sec);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putnum(int zahl, MBHEAD *mbp)
{
  char buf[10];

  sprintf(buf, "%d", zahl);
  putstr(buf, mbp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putlong(ULONG zahl, BOOLEAN justify, MBHEAD *mbp)
{
  char buf[20];

  sprintf(buf,justify ? " %10lu" : " %lu",zahl);
  putstr(buf, mbp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putstr(const char *string, MBHEAD *mbp)
{
  while (*string)
    putchr(*string++, mbp);
}

/**************************************************************************/
/* printf fuer mbp-Fuellen                                       DL1XAO   */
/*------------------------------------------------------------------------*/
#define VSPRINTFBUFFER 256
void putprintf(MBHEAD *mbp, const char *format, ...)
{
  va_list arg_ptr;
  char    str[VSPRINTFBUFFER];
  int     n = 0;

  va_start(arg_ptr, format);
  n = vsnprintf(str, VSPRINTFBUFFER, format, arg_ptr);
  va_end(arg_ptr);

  if (n > -1 && n < VSPRINTFBUFFER)
    putstr(str, mbp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void putspa(WORD stop, MBHEAD *mbp)
{
  WORD i;

  i = mbp->l4time + stop - mbp->mbpc;
  while (i-- > 0)
    putchr(' ', mbp);
}

/**************************************************************************/
/* VIA-Pfad extrahieren und pruefen                                       */
/*                                                                        */
/* YES = Check ohne Fehler                                                */
/* NO  = Check mit Fehler, Pfad zu kurz                                   */
/* ERRORS = Check mit Fehler, Calls konnten nicht extrahiert werden       */
/*------------------------------------------------------------------------*/
TRILLIAN
getdig(WORD *laenge, char **inbuf, BOOLEAN pflag, char *outbuf)
{
  char      *lispoi;
  WORD       i;
  char      *p;
  WORD       n;

  *outbuf = NUL;

  skipsp(laenge, inbuf);

  /* Laengenpruefung */
  if ((n = *laenge) < 4)
    return (NO); /* zu kurz */

  p = *inbuf;

  /* optionales "VIA" im Pfad ueberspringen */
  if (!strnicmp((char*)p, "VIA", 3))
  {
    p += 3;
    n -= 3;
  }

  skipsp(&n, &p);

  /* solange wir gueltige Calls finden */
  for (i = 0, lispoi = outbuf; n > 0 && i < L2VNUM; ++i, lispoi += L2IDLEN)
    if (getcal(&n, &p, pflag, lispoi) != YES)
      break;

  *lispoi = NUL;

  /* mindestens ein gueltiges Call gefunden */
  if (i > 0)
  {
    *laenge = n;
    *inbuf = p;
    return(YES);
  }

  return(ERRORS);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
TRILLIAN
getcal(WORD *laenge, char **inbuf, BOOLEAN pflag, char *outbuf)
{
  char  call[L2IDLEN];
  char  binsid = 0;
  char *bufpoi;
  char  zeichen;
  WORD  i;
  char *p = *inbuf;
  WORD  n = *laenge;

  cpyid(call,nullid);

  skipsp(&n, &p);

  bufpoi = call;
  i = 0;
  while (n > 0) {
    if (((zeichen = (char) toupper(*p)) == ' ' ) || (zeichen == ','))
      break;
    if (zeichen < ' ')
      return(ERRORS);
    if (zeichen == '-') {
      if (n <= 0)
        return(ERRORS);
      ++p;
      --n;
      if (n <= 0)
        return(ERRORS);
      zeichen = *p;
      if ((zeichen < '0') || (zeichen > '9'))
        return(ERRORS);
      ++p;
      --n;
      binsid = (zeichen - '0');
      if (n > 0) {
        zeichen = *p;
        if ((zeichen >= '0') && (zeichen <= '9')) {
          binsid *= 10;
          binsid += (zeichen - '0');
          if (binsid > 15)
            return(ERRORS);
          ++p;
          --n;
        }
      }
      call[L2IDLEN-1] = (binsid << 1) | 0x60;
      break;
    }
    else {
      if (i++ == L2IDLEN-1)
        return(ERRORS);
      *bufpoi++ = zeichen;
      ++p;
      --n;
    }
  }

  while (n > 0) {
    zeichen = *p;
    if ((zeichen != ' ') && (zeichen != ','))
      break;
    ++p;
    --n;
    if (zeichen == ',') break;
  }

  /* haben wir nur eine SID bekommen, dann ergaenzen wir mit unserem Call */
  if ((call[0] == ' ') && (binsid <= 15) && (binsid > 0))
  {
    cpyid(call, myid);
    call[L2IDLEN-1] = (binsid << 1) | 0x60;
    i = 1;
  }

  if (i == 0)
    return(NO);

  if (pflag && (fvalca(call) == ERRORS))
    return(ERRORS);

  cpyid(outbuf, call);
  *laenge = n;
  *inbuf  = p;

  return(YES);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOLEAN getport(WORD *laenge, char **inbuf, WORD *port)
{
  WORD i;
  WORD tmp_port;
  char *save_inbuf = *inbuf;
  WORD  save_laenge = *laenge;
  char pn[12];
  char *inbp;
  char ch;

  skipsp(laenge, (char **) inbuf);

  if (*laenge <= 0) return(FALSE);

  inbp = *inbuf;
  for (i = 0; i < min(*laenge, 11); i++)
   {
    ch = *inbp++;
    if (ch == ' ')
      break;
    pn[i] = ch;
   }
  pn[i] = NUL;

  for (i = 0; i < L2PNUM; i++) {    /* erst Portnamen vergleichen */
    if (stricmp(portpar[i].name, pn) == 0) {
      nextspace(laenge, inbuf);
      return(portenabled(*port = i));
    }
  }

  if (isdigit(**inbuf)) {           /* sonst Portnummer pruefen   */
    tmp_port = nxtnum(laenge, inbuf);
    if (   (tmp_port >= 0)
        && (tmp_port < L2PNUM)) {   /* Port gueltig?              */
      return(portenabled(*port = tmp_port));
    }
  }

  *inbuf = save_inbuf;              /* Position restaurieren            */
  *laenge = save_laenge;

  return(FALSE);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
TRILLIAN
getide(WORD *laenge, char *(*inbuf), char *outbuf)
{
  char   ident[L2CALEN];
  char   c;
  WORD   i;
  WORD   n = *laenge;
  char  *p = *inbuf;

  memcpy(ident, nulide, L2CALEN);

  for (i = 0; (i < L2CALEN) && (n > 0); ++i) {
    --n;
    c = *p++;
    if (c != ' ') {
      if (c != '*') {
        ident[i] = c;
        continue;
      }
      if (i != 0 || c != '*')
        return(ERRORS);
    }
    break;
  }

  if ((i == L2CALEN) && (n > 0) && (*p != ' '))
    return(ERRORS);

  if (valcal(ident) == YES)
    return(ERRORS);

  memcpy(outbuf, ident, L2CALEN);

  if (ident[0] != ' ')            /* Ident korrekt gelesen            */
  {
    *laenge = n;
    *inbuf = p;
    return(YES);
  }

  return(NO);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
UWORD nxtnum(WORD *laenge, char **buffer)
{
  UWORD temp;

  skipsp(laenge, buffer);
  temp = 0;
  while (*laenge > 0 && **buffer >= '0' && **buffer <= '9') {
    --*laenge;
    temp *= 10;
    temp += (*(*buffer)++ - '0');
  }
  return(temp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
LONG nxtlong(WORD *laenge, char **buffer)
{
  LONG temp;

  skipsp(laenge, buffer);
  temp = 0;
  while (*laenge > 0 && **buffer >= '0' && **buffer <= '9') {
    --*laenge;
    temp *= 10;
    temp += (*(*buffer)++ - '0');
  }
  return(temp);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
TRILLIAN
fvalca(char *call)
{
  if (*call == ' ' )
    return(NO);
  return(valcal(call));
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
TRILLIAN
valcal(char *call)
{
  char *numpos = 0;
  char *actual;
  char zeichen;
  WORD zahl = 0;
  WORD i = 0;

  for (actual = call; i < L2IDLEN-1; ++i, ++actual)
  {
    if ((zeichen = *actual) == ' ')
      break;
    if (!((zeichen >= 'A') && (zeichen <= 'Z')))
    {
      if ((zeichen >= '0') && (zeichen <= '9'))
      {
        zahl += 1;
        numpos = actual;
      }
      else
        return(ERRORS);
    }
  }

#ifndef CALLCHECK
  if (   ((actual - call) < 4) || zahl == 0 || zahl > 2 || (numpos == call)
      || (numpos == (actual - 1)) || ((numpos - call) >= 3))
    return(ERRORS);
#else
  /* Hat Rufzeichen weniger als 4 Zeichen, */
  if (i < 4)
    /* ist es ungueltig. */
    return(ERRORS);
#endif /* CALLCHECK */

  return(YES);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOLEAN ismemr(void)
{
  if (nmbfre < 300 && !userpo->sysflg)
    return(FALSE);

  return(TRUE);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
char *calofs(LINKTYP uplink, UID uid)
{
  void *link;

  if (uid == NO_UID) return(myid);

  link = g_ulink(uid);
  switch (g_utyp(uid)) {
    case L2_USER :
      return(((LNKBLK *)link)->dstid);
    case L4_USER :
      return(  ((uplink == UPLINK) || (uplink == CQ_LINK))
             ? ((CIRBLK *)link)->upcall
             : ((CIRBLK *)link)->downca);
#ifdef L1TCPIP
    case TCP_USER :
      return(((TCPIP *)link)->Upcall);
#endif /* L1TCPIP */
  }

  if (tnnb_aktiv) return(myid);
  if (uplink == DOWNLINK) return(hostid);
  return(hstusr->call);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOLEAN getlin(MBHEAD *mbp)
{
  char huge *nextch;
  WORD getcou;
  WORD found;
  WORD laenge;
  WORD mlaenge = 81;

  if (userpo->status == US_WBIN || userpo->status == US_RBIN)
    return(TRUE);

  if (userpo->convers) {
    if (userpo->convers->type == CT_UNKNOWN)
      return(TRUE);
    mlaenge = 2047;
  }
  nextch = mbp->mbbp;
  getcou = mbp->mbgc;
  found = FALSE;
  laenge = 0;
  while (mbp->mbgc < mbp->mbpc) {
    if ( ((getchr(mbp)) == CR) || (++laenge == mlaenge)) {
      found = TRUE;
      break;
     }
  }
  mbp->mbbp = nextch;
  mbp->mbgc = getcou;
  return(found);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOLEAN issyso(void)
{
  return(userpo->sysflg != 0);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOLEAN skipsp(WORD *cnt, char **cpp)
{
  while ((*cnt > 0) && **cpp == ' ') {
    ++*cpp;
    --*cnt;
  }
  return((*cnt > 0) ? TRUE : FALSE);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
WORD getparam(WORD *cnt, char **cpp, WORD min, WORD max, WORD def)
{
  int par;
  if ((*cnt) > 1) {
    (*cpp)++;
    (*cnt)--;
    if (toupper(*(*cpp)) == 'O') { /* BOOLEAN */
      if (toupper((*cpp)[1]) == 'N') par = 1;
      else par = 0;
      nextspace(cnt, cpp);
      return(par);
    }
    par = (nxtnum(cnt, cpp));
    if (par > max) par = max;
    if (par < min) par = min;
    return(par);
  }
  return(def);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOLEAN nextspace(WORD *cnt, char **cpp)
{
  while ((*cnt > 0) && **cpp != ' ') {
    ++*cpp;
    --*cnt;
  }
  return((*cnt > 0) ? TRUE : FALSE);
}

/* Ermittelt das Filedatum von AKTUELL.TXT und stellt es zur Verfuegung
 */
static char *aktuell(void)
{
  char file[128];
  struct ffblk fb;
  static char akt[10] = "?";
#ifdef MC68302
  struct tm *ftime;
#endif

  strcpy(file, textcmdpath);
#ifdef MC68302
  strcat(file, "aktuell.txc");
#else
  strcat(file, "aktuell.txt");
#endif
  if (xfindfirst(file, &fb, 0) == 0) {
#ifdef MC68302
    ftime = localtime(&fb.ff_ftime);
    sprintf(akt, "%02d.%02d.%02d", ftime->tm_mday,
                                  (ftime->tm_mon)+1,
                                  (ftime->tm_year)%100);
#else
    sprintf(akt,"%02d.%02d.%02d", fb.ff_fdate & 0x1f,
                                 (fb.ff_fdate >> 5) & 0xf,
                                 ((fb.ff_fdate >> 9) + 80) % 100);
#endif
  }
  return(akt);
}

/*------------------------------------------------------------------------*/
/* Prompt generieren und an Message-Buffer anhaengen                      */
/*------------------------------------------------------------------------*/
/* Prompt aufbauen, im Convers-Modus gibt es keinen Prompt, sondern */
/* stattdessen  einen String '***\r', der das Ende der Ausgabe des  */
/* Digis anzeigt.                                                   */
void prompt(MBHEAD *mbp)
{
  if (userpo->convers == NULLCONNECTION)
    prompt2str(mbp, promptstr);
  else
    putstr("***\r", mbp);
}

  /* String nach '%' durchsucht und gegebenenfalls ergaenzt.    */
  /*     '%a' wird durch den Digipeater-Ident ersetzt,          */
  /*     '%A' wird durch das Datum des aktuell.txt ersetzt,     */
  /*     '%c' durch das Call des Users,                         */
  /*     '%C' durch das User-Call mit SSID,                     */
  /*     '%d' durch das Call des Digipeaters,                   */
  /*     '%D' durch das Call des Digipeaters mit SSID,          */
  /*     '%f' durch den Inhalt der angegebenen Datei            */
  /*     '%l' durch die Anzahl aktiver L2-Links                 */
  /*     '%p' durch die Portnummer                              */
  /*     '%P' durch den Pseudo-Name des Ports                   */
  /*     '%r' durch Carriage-Return                             */
  /*     '%s' durch Datum                                       */
  /*     '%t' durch die aktuelle Uhrzeit (HH:MM).               */
  /*     '%u' durch die Anzahl der User auf dem aktuellen Port  */
  /*     '%%' durch %                                           */
  /*     '%0' unterdrueckt die Aussendung eines Prompts.        */
void prompt2str(MBHEAD *mbp, char *str)
{
  char      *cp;
  char      buf[128];
  struct tm *lt;
  LNKBLK    *link;       /* L2-Link des Users */
  FILE      *fp;
  char      *c;
  char      filename[128];
#ifdef MAKRO_USER
  UWORD     user_anzahl = 0;   /* die echte Useranzahl im Knoten         */
  UWORD     interlinks  = 0;   /* alle Interlinks im knoten              */
  UWORD     _nmblks     = 0;   /* Sicherungskopie fuer das echte nmblks  */
#endif
#ifdef L1TCPIP
  TCPIP     *tpoi;
#endif /* L1TCPIP */

  while (*str) {
    for (cp = str; *cp && *cp != '%'; ++cp)
      ;
    if (*cp == '%') {
      *cp = NUL;
      putstr(str, mbp);
      *cp++ = '%';
      switch (*cp) {
        case 'a':  putalt(alias, mbp);
                   break;

        case 'A':  putstr(aktuell(), mbp);
                   break;

        case 'c':  putcal(calofs(UPLINK, userpo->uid), mbp);
                   break;

        case 'C':  putid(calofs(UPLINK, userpo->uid), mbp);
                   break;

        case 'd':  putcal(myid, mbp);
                   break;

        case 'D':  putid(myid, mbp);
                   break;

        case 'f':  cp++;
                   strcpy(filename, cp); /* Dateiname extrahieren */

                   if ((c = strchr(filename, ' ')) != NULL)
                     *c = '\0';

                   if ((fp = xfopen(filename, "rt")) != NULL) {

                     while (fgets(buf, 126, fp) != NULL) {

                       if ((c = strchr(buf, '\n')) != NULL)
                         *c = CR;
                         putprintf(mbp, buf);
                     }

                     fclose(fp);

                     cp += strlen(filename) - 1; /* Ein Zeichen vor dem Ende */

                     if (*(cp + 1) == ' ')
                       cp++; /* Sonst Probs bei Text am Strinende */
                   }
                   break;

        case 'l':  putprintf(mbp, "%01u", nmblks); /* Anzahl L2-Links */
                   break;

        case 'p':  /* Portnummer */
                   switch(g_utyp(userpo->uid))
                   {
                     case L2_USER :
                       link = g_ulink(userpo->uid);    /* Userlink ermitteln */
                       putprintf(mbp, "%u", link->liport); /*  -> Portnummer */
                       break;

#ifdef L1TCPIP
                     case TCP_USER :
                       tpoi = g_ulink(userpo->uid);    /* Userlink ermitteln */
                       putprintf(mbp, "%u", tpoi->port); /*  -> Portnummer */
                       break;
#endif /* L1TCPIP */

                     default :
                       /* Host-Console   */
                        putstr("C", mbp);                   /*  -> "C"        */
                        break;
                   }
                   break;

        case 'P':  /* Portname */
                   switch(g_utyp(userpo->uid))
                   {
                     case L2_USER :
                       link = g_ulink(userpo->uid);     /* Userlink ermitteln */
                       /* Portname ausgeben, wenn User nicht von Console kommt */
                       putprintf(mbp, "%s", portpar[link->liport].name);
                       break;

#ifdef L1TCPIP
                     case TCP_USER :
                       tpoi = g_ulink(userpo->uid);    /* Userlink ermitteln */
                       /* Portname ausgeben, wenn User nicht von Console kommt */
                       putprintf(mbp, "%s", portpar[tpoi->port].name);
                       break;
#endif /* L1TCPIP */

                     default :
                       /* Host-Console   */
                       putstr("Console", mbp);         /* -> "Console" */
                       break;
                   }
                   break;

        case 'r':  putchr(CR, mbp);
                   break;

        case 's':  lt = localtime(&sys_time);
                   putprintf(mbp, "%02u.%02u.%02u",
                                  lt->tm_mday, lt->tm_mon+1, lt->tm_year%100);
                   break;

        case 't':  sprintf(buf,"%16.16s",ctime(&sys_time));
                   putstr(&buf[11], mbp);
                   break;

#ifdef MAKRO_USER
        case 'u':  interlinks = Interlinks_zaehlen();
                   /* wir zaehlen erstmal alle Interlinks im Knoten */
                    _nmblks = nmblks;
                    /* Sicherungskopie vom Original nmblks           */
                    putprintf(mbp, "%u", user_anzahl = (_nmblks - interlinks) + nmbcir
#ifdef L1TCPIP
                              + nmbtcp
#endif /* L1TCPIP */
                             );
                    /* L2-Links - Interlinks + L4-User = echte Useranzahl   */
                    break;
#else
        case 'u':  /* Useranzahl auf aktuellem L2-Port */
                   if (mbp->type == L2_USER)
                   {
                     lnk = g_ulink(userpo->uid);    /* Userlink ermitteln */
                     /* Anzahl ausgeben, wenn User nicht von Console kommt */
                     putprintf(mbp, "%u", portpar[lnk->liport].nmbstn);
                   }
                   else                   /* fuer Host-Console und L4 nicht */
                     putstr("N/A", mbp);  /* verfuegbar */
                   break;
#endif
#ifdef MAKRO_NOLOGINSTR
        case 'v':  putprintf(mbp,"%s",loginstr);
#endif
        case '0':  break;

        case '%':  putchr('%', mbp);
                   break;
      }
      str = ++cp;
    }
    else
    {
      putstr(str, mbp);
      str = cp;
    }
  }
}

/************************************************************************/
/*                                                                      */
/*----------------------------------------------------------------------*/
void putide(char *ID, MBHEAD *mbp)
{
  int i;

  for (i = 5; (i >= 0) && (ID[i] == ' '); i--)       putchr(' ', mbp);
  for (i = 0; (i <  L2CALEN) && (ID[i] != ' '); i++) putchr(ID[i],mbp);
}

/************************************************************************/
/*                                                                      */
/*----------------------------------------------------------------------*/
void invmsg(void)
{
#ifdef SPEECH
  putmsg(speech_message(277));
#else
  putmsg("Invalid command. Try HELP.\r");
#endif
}

/************************************************************************/
/* Beacons                                                              */
/*----------------------------------------------------------------------*/
void dump_beacn(MBHEAD *mbp)
{
  BEACON *beapoi;
  int port;

  putstr(";\r; Stat/Text-Broadcast\r;\r", mbp);
  for (port = 0, beapoi = beacon; port < L2PNUM; ++port, ++beapoi) {
    if (*beapoi->text)
      putprintf(mbp, "BEACON %d = %s\r", port, beapoi->text);
    putprintf(mbp, "BEACON %d %d %d ", port, beapoi->interval, beapoi->telemetrie);
    putid(beapoi->beades, mbp);
    putdil(beapoi->beadil, mbp);
    putchr('\r', mbp);
  }
}

/************************************************************************/
/* Convers-Links                                                        */
/*----------------------------------------------------------------------*/
void dump_convc(MBHEAD *mbp)
{
  int  pl;
  char *c;
  PERMLINK *p;

  putstr(";\r; Convers Interlinks\r;\r", mbp);
#ifdef CONVERS_HOSTNAME
  putprintf(mbp,"CONV HOSTNAME %s\r",myhostname);
#endif
  for (pl = 0; pl < MAXCVSHOST; pl++) {
    p = permarray[pl];
    if (p ) {
      putstr("CONV C ", mbp);
#ifdef L1IPCONV
      putprintf(mbp,"%s ", p->cname);
      if (p->TcpLink)
        putprintf(mbp,"%s ", p->HostName);
      else
#endif /* L1IPCONV */
      putid(p->call, mbp);
      if (p->port != 255) {
    putchr(' ', mbp);
    putnum(p->port, mbp);
    if (*(c = p->via)) {
      while (*c) {
        putchr(' ', mbp);
        putid(c, mbp);
        c += L2IDLEN;
      }
    }
      }
      putchr('\r', mbp);
    }
  }
}


/************************************************************************/
/* Links                                                                */
/*----------------------------------------------------------------------*/
void dump_links(MBHEAD *mbp)
{
  char   *c;
  L2LINK *p;
  PEER   *pp;
  int     i;
  int     max_peers = netp->max_peers;
#ifdef ALIASSAVEMOD
  char    alias[L2IDLEN];
#endif /* ALIASSAVEMOD */

  putstr(";\r; Links\r;\r", mbp);

  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
    if (pp->used)
    {
      p = pp->l2link;
      if (p->port != L2PNUM)          /* Sonderbehandlung Hostmode-Link */
      {
#ifdef AUTOROUTING
        /* Link ist eine Auto-Route, */
        if (p->ppAuto == AUTO_ROUTE)
          /* zum naechsten Eintrag.  */
          continue;
#endif /* AUTOROUTING */
           putstr("LINK + ", mbp);
           putprintf(mbp, "%2.2s ",typtbl + pp->soll_typ * 2);

#ifdef PROXYFUNC
           if (pp->proxy == TRUE)
             putstr("P ", mbp);
#endif /* PROXYFUNC */
           putnum(p->port, mbp);
           putchr(' ', mbp);
#ifdef ALIASSAVEMOD
           /* Ist der Alias komplett in Grossbuchstaben, wird der Linkeintrag */
           /* beim naechsten mal NICHT eingelesen!!! Darum muessen wir den    */
           /* in kleinbuchstaben umwandeln.                                   */
           cpyid(alias, p->alias);
           /* Ein Alias kann max. 6 Zeichen haben. */
           alias[L2CALEN] = 0;

           /* Alias in kleinbuchstaben umwandeln und in den Buffer schreiben. */
           putcal(strlwr(alias), mbp);
#else
           putcal(p->alias, mbp);
#endif /* ALIASSAVEMOD */
           putchr(' ', mbp);
           putid (p->call, mbp);
           putchr(' ', mbp);

           if (*(c = p->digil ))
           {
             while (*c)
             {
               putchr(' ', mbp);
               putid(c, mbp);
               c += L2IDLEN;
             }
           }

#ifdef LINKSMODINFO
           putchr(' ', mbp);
           putstr("INFO=",mbp);
           putprintf(mbp,"%s",p->info, mbp);
#endif /* LINKSMODINFO */
      }
      else
        {
          putstr("ESC I ", mbp);
          putid(p->call, mbp);
          putchr(' ', mbp);
          putcal(p->alias, mbp);

#ifdef PROXYFUNC
          if (pp->proxy == TRUE)
            putstr(" P", mbp);
#endif
        }

      putchr('\r',mbp);
    }
}

/************************************************************************/
/* Parameter                                                            */
/*----------------------------------------------------------------------*/
void dump_parms(MBHEAD *mbp)
{
  UWORD  num;
  PARAM  *p;

  putstr(";\r; Parameters\r;\r", mbp);
  for (p = partab, num = 1; num <= partablen; p++, num++)
    if (p->paradr && strncmp(p->parstr, "unu", 3))
      putprintf(mbp, "PAR %s %u\r", p->parstr, *(p->paradr));
}

/************************************************************************/
/* Ports                                                                */
/*----------------------------------------------------------------------*/
void
dump_ports(MBHEAD *mbp)
{
  int        port;
  PORTINFO  *pp;
  L1MODETAB *mtp;
  char       mode[16], *cp;

  putstr(";\r; Port Configuration (Level 1)\r;\r", mbp);
  for (port = 0, pp = portpar; port < L2PNUM; port++, pp++)
  {
    putprintf(mbp, "PORT %d NAME=%s ", port, pp->name);
    l1hwcfg(port, mbp);
    cp = mode;
    for (mtp = l1modetab; mtp->ch; mtp++)
      if (pp->l1mode & mtp->mode)
        *cp++ = mtp->ch;
    *cp = NUL;

    putprintf(mbp, " MODE=%u00%s TXD=%u MAX=%u%s ",
                   pp->speed,
                   mode,
                   pp->txdelay,
                   pp->maxframe,
                   automaxframe(port) ? "a" : "");
#ifdef SETTAILTIME
    putprintf(mbp, "TAIL=%u ", pp->tailtime);
#endif

#ifdef DAMASLAVE
    if (pp->l2mode & MODE_ds)
      strcpy(mode, "s");
    else
#endif
      sprintf(mode, "%2u", pp->l2mode & MODE_a ? pp->dch + 1 : 0);

    putprintf(mbp, "DAMA=%s CTEXT=%u SYSOP=%u MH=%u",
                   mode,
                   pp->l2mode & MODE_x ? 1 : 0,
                   pp->l2mode & MODE_s ? 1 : 0,
                   pp->l2mode & MODE_h ? 1 : 0);
#ifdef USERMAXCON
    putprintf(mbp, " MAXCON=%u", pp->maxcon);
#endif
#ifdef EAX25
    putprintf(mbp, " EAXMAXF=%u EAXMODE=%u", pp->maxframe_eax, pp->eax_behaviour);
#endif

#ifdef EXPERTPARAMETER
    putprintf(mbp, " PERS=%u%c SLOT=%u%c IRTT=%u%c T2=%u%c RETRY=%u%c",
                             pp->persistance,
                             (pp->l2autoparam & MODE_apers) ? 'a' : ' ',
                             pp->slottime,
                             (pp->l2autoparam & MODE_aslot) ? 'a' : ' ',
                             pp->IRTT,
                             (pp->l2autoparam & MODE_aIRTT) ? 'a' : ' ',
                             pp->T2,
                             (pp->l2autoparam & MODE_aT2) ? 'a' : ' ',
                             pp->retry,
                             (pp->l2autoparam & MODE_aretry) ? 'a' : ' ');
#endif

#ifdef PORT_MANUELL
    putprintf(mbp, "\rPORT %d", port);

    putprintf(mbp, " PACLEN=%u", pp->paclen);
    putprintf(mbp, " T3=%u", pp->T3);
#endif /* PORT_MANUELL */

#ifdef IPOLL_FRAME
    putprintf(mbp, " IPACLEN=%u IRETRY=%u", pp->ipoll_paclen,pp->ipoll_retry);
#endif /* IPOLL_FRAME */
#ifdef PORT_L2_CONNECT_TIME
    putprintf(mbp, " L2TIME=%u", pp->l2_connect_time);
#endif
#ifdef PORT_L2_CONNECT_RETRY
    putprintf(mbp, " L2RETRY=%u", pp->l2_connect_retry);
#endif
#ifdef AUTOROUTING
    /* Fixed/Auto-Route eintragen. */
    putprintf(mbp, " L4AUTO=%u", pp->poAuto);
#endif /* AUTOROUTING */
    putchr('\r', mbp);
  }
}

/************************************************************************/
/* SUSPEND                                                              */
/*----------------------------------------------------------------------*/
void dump_suspd(MBHEAD *mbp)
{
  SUSPEND *suspoi;
  int     i;

  putstr(";\r; Suspended Users\r;\r", mbp);
  for (suspoi = sustab, i = 0; i < MAXSUSPEND; suspoi++, i++)
    if (suspoi->call[0]) {
      putprintf(mbp, "SUSPEND + %d ", suspoi->port);
      putcal(suspoi->call, mbp);
      if (suspoi->port == 255) {
        putchr(' ', mbp);
        putnum(suspoi->okcount, mbp);
      }
      putchr('\r', mbp);
   }
}

/************************************************************************/
/* ip                                                                   */
/*----------------------------------------------------------------------*/
#ifdef IPROUTE
void dump_ipr(MBHEAD *mbp)
{
 IP_ROUTE *ipr;
 ARP_TAB  *arp;
 const char *dgmode_tab[] = {"", "DG", "VC"};

  putstr(";\r;IP-Config\r;\rIPA ", mbp);
  show_ip_addr(my_ip_addr, mbp);               /* My IP-Adress */
  putprintf(mbp, "/%d\r", my_ip_bits);

  for (ipr =  (IP_ROUTE *)IP_Routes.head;         /* IP-Routen */
       ipr != (IP_ROUTE *)&IP_Routes;
       ipr =  (IP_ROUTE *)ipr->nextip) {
    putstr("IPR ", mbp);
    show_ip_addr(ipr->dest, mbp);          /* Adresse anzeigen */
    putprintf(mbp, "/%d + %s",
                   ipr->bits, /* Bitmaske zeigen  */
                   ipr->flags & RTDYNAMIC ? "D " : " ");
    switch (ipr->port)
    {
        case NETROM_PORT: putstr("NET/ROM ", mbp); break;
#ifdef KERNELIF
        case KERNEL_PORT: putstr("KERNEL ", mbp); break;
#endif
        default         : putprintf(mbp, "%s ", portpar[ipr->port].name);
    }
    if (ipr->gateway != 0)
      show_ip_addr(ipr->gateway, mbp);     /* Gateway          */
    putchr(' ', mbp);
    if (ipr->metric != 0)                  /* if metric set,   */
        putnum(ipr->metric, mbp);          /* show metric      */
    putchr('\r', mbp);
  }

  for (arp =  (ARP_TAB *)Arp_tab.head;                  /* ARP */
       arp != (ARP_TAB *)&Arp_tab;
       arp =  (ARP_TAB *)arp->nextar) {
    if (arp->timer == 0) {              /* nur feste Eintraege */
      putstr("ARP ", mbp);
      show_ip_addr(arp->dest, mbp);
      putprintf(mbp, " + %c ", arp->publish_flag ? 'P' : ' ');
      switch (arp->port)
      {
        case NETROM_PORT: putstr("NET/ROM ", mbp); break;
#ifdef KERNELIF
        case KERNEL_PORT: putstr("KERNEL ", mbp); break;
#endif
        default         : putprintf(mbp, "%s ", portpar[arp->port].name);
      }
      putprintf(mbp, "%s ", dgmode_tab[(int)arp->dgmode]);
      putid(arp->callsign, mbp);
      putchr(' ', mbp );
      putdil(arp->digi, mbp);
      putchr('\r', mbp );
    }
  }
}
#endif

#ifdef ALIASCMD
/************************************************************************/
/* print aliasses                                                       */
/*----------------------------------------------------------------------*/
void dump_alias(MBHEAD *mbp)
{
  CMDALIAS *ap;

  putstr(";\r; Commando-Aliasses\r;\r", mbp);

  for (ap = aliaslist; ap != NULL; ap = ap->next)
    putprintf(mbp, "ALIAS %s %s\r", ap->alias, ap->cmd);

}
#endif

/************************************************************************/
/* Versch. noch eintragen                                               */
/*----------------------------------------------------------------------*/
void dump_divrs(MBHEAD *mbp)
{
#ifdef PACSAT
  WORD x;

  /* PACSAT-BROADCAST */

  putstr(";\r; Pacsat Broadcast\r;\r", mbp);
  for (x = 0; x < L2PNUM; x++)
  {
    if (pacsat_enabled[x])
    {
      putprintf(mbp, "PACSAT + %u\r", x);
    }
  }
  putstr("PACSAT C ", mbp);
  if (!cmpid(pacsatid, nullid))
    putid(pacsatid,mbp);
  else
    putchr('-', mbp);

  putprintf(mbp, "\rPACSAT P 1 %u\r",pacsat_timer);
  putprintf(mbp, "PACSAT P 2 %u\r",pacsat_frames);
  putprintf(mbp, "PACSAT P 3 %u\r",pacsat_free);
#endif
  /* versch. */

  putstr(";\r; Mailbox and Cluster\r;", mbp);
  if (boxid[0])
   {
    putstr("\rMAILBOX ", mbp);
    putid(boxid, mbp);
   }
  if (dxcid[0])
   {
    putstr("\rDXCLUSTER ", mbp);
    putid(dxcid, mbp);
   }

#ifdef HOSTMYCALL
  if (hostuserid[0])
   {
    putstr("\rMYHOST ", mbp);
    putid(hostuserid, mbp);
   }
#endif /* HOSTMYCALL */

#ifdef SYSOPPASSWD
  if (paswrd[0])
  {
    putstr("\rSYSPASS ", mbp);
    putprintf(mbp,"%s",paswrd);
  }
#endif /* SYSOPPASSWD */

  putstr("\r;\r; MHeard, Prompt\r;\r", mbp);
  putprintf(mbp, "MH = %d\r", l2heard.max);
  putprintf(mbp, "L3MH = %d\r", l3heard.max);
  putprintf(mbp, "PROMPT =%s\r", promptstr);
#ifndef __LINUX__                       /* steht bei Linux in tnn.ini   */
  if (tkcom > -1) {
    /* Tokenring-Speed */
    putstr(";\r; Tokenspeed\r;\r", mbp);
    putprintf(mbp, "#T %u00",tkbaud);
  }
#endif
}

void save_parms(void)
{
  MBHEAD    *mbp;
  FILE      *fp;
  UBYTE      ch;
  char       call[20];

#ifdef SAVEPARAMFIX
  char konfigfile[128];

  strcpy(konfigfile,cfgfile);
  strcat(konfigfile,".TNB");

  if ((fp = xfopen(konfigfile, "wt+")) == NULL) return;
#else /* SAVEPARAMFIX */
  if ((fp = xfopen("PARMS.TNB", "wt+")) == NULL) return;
#endif /* SAVEPARAMFIX */
#ifdef ST
    setvbuf(fp, NULL, _IOFBF, 4096L); /* speedup */
#endif

  mbp = (MBHEAD *) allocb(ALLOC_MBHEAD);

  /* Versionskennung und Datum speichern */

  putprintf(mbp, "; ----------------------------------------------\r"
                 ";\r"
                 "; THIS FILE MAY BE OVERWRITTEN - DO NOT CHANGE!!\r"
                 ";               (use %s.TNB)\r;\r", cfgfile);
  putprintf(mbp, "; ----------------------------------------------\r"
                 "; TheNetNode AutoConfiguration Batch File\r"
                 ";\r"
                 "; Created: ");
  puttim(&sys_time, mbp);
  call2str(call,myid);
  putprintf(mbp, "\r; Version: %s;\r"
                 "; Node Ident : %s\r"
                 "; Node MyCall: %s\r;\r", version, alias, call);

#ifdef ATTACH
  dump_attach(mbp);
#endif
  /* die ganzen Parameter ausgeben, nach DL1XAO */
  dump_ports(mbp);
  dump_beacn(mbp);
  dump_convc(mbp);
  dump_links(mbp);
  dump_parms(mbp);
#ifdef THENETMOD
  dump_l4parms(mbp);                               /* L4-Parameter speichern. */
  /* L4QUALI */
  dump_routes(mbp);          /* Qualitaet einer Route (nur THENET) speichern. */
#endif /* THENETMOD */
  dump_suspd(mbp);
#ifdef ALIASCMD
  dump_alias(mbp);
#endif
#ifdef IPROUTE
  dump_ipr(mbp);
#endif
#ifdef KERNELIF
  dump_kernel(mbp);
#endif
#ifdef AX25IP
  dump_ax25ip(mbp);
#endif
#ifdef SPEECH
  dump_speech(mbp);
#endif
#ifdef L1TCPIP
  DumpTCP(mbp);
#endif /* L1TCPIP */
  dump_divrs(mbp);
  rwndmb(mbp);

  while (mbp->mbgc < mbp->mbpc)
  {
    ch = getchr(mbp);
    if (ch == CR) fprintf(fp, "\n");
    else fputc(ch, fp);
  }

  dealmb(mbp);
  fclose(fp);
}

/************************************************************************/
/* Ein Ereignis protokollieren, jeder Sysop, der einen Trace mit dem    */
/* notwendigen Level eingestellt hat, bekommt die Meldung zugeschickt.  */
/*----------------------------------------------------------------------*/
void notify(int level, const char *format, ...)
{
#if MAX_TRACE_LEVEL > 0
#define NOTIFYVSNPFBUFF 1024
  va_list    arg_ptr;
  char       str[NOTIFYVSNPFBUFF];
  USRBLK    *sav_userpo;
  MBHEAD    *mbp;
  struct    tm *p;
  int       n;

  p = localtime(&sys_time);

  va_start(arg_ptr, format);
  n = vsnprintf(str, NOTIFYVSNPFBUFF, format, arg_ptr);
  va_end(arg_ptr);

  if (n == -1 || n > NOTIFYVSNPFBUFF)
    return;

  sav_userpo = userpo;

  for (userpo =  (USRBLK *) usccpl.head;
       userpo != (USRBLK *) &usccpl;
       userpo =  (USRBLK *) userpo->unext)
  {
    if (userpo->status == US_CCP && userpo->auditlevel >= level) {
      mbp = getmbp();
      putprintf(mbp, "(%u) %02d.%02d.%02d %02d:%02d:%02d ",
                     level,
                     p->tm_mday, p->tm_mon+1, p->tm_year % 100,
                     p->tm_hour, p->tm_min, p->tm_sec);
      putstr(str, mbp);
      putchr('\r', mbp);
      if (!send_msg(FALSE,mbp))
        dealmb((MBHEAD *)ulink((LEHEAD *)mbp)); /* der Link ist voll */
      break;
    }
  }

  userpo = sav_userpo;
#endif
}

/************************************************************************/
/* Feststellen, ob ein User einen Downlink auf einem Port machen darf.  */
/*----------------------------------------------------------------------*/
BOOLEAN is_down_suspended(char *user_call, int user_port)
{
  LNKBLK  *link;
  SUSPEND *suspended;
  WORD    user_links;
  WORD    i, j;

  /* Suspended-Liste nach dem User-Rufzeichen durchsuchen */
  for (suspended = sustab, i = 0; i < MAXSUSPEND; suspended++, i++)
    if (strnicmp(user_call, suspended->call, L2CALEN) == 0)
      break;

  if (i == MAXSUSPEND)                    /* User nicht in der Liste       */
     return(FALSE);                        /* er darf..                     */

  if (user_port == suspended->port)       /* User gesperrt auf diesem Port */
     return(TRUE);                         /* also abwerfen..               */

  if (suspended->port == 254)             /* darf ueberhauptnicht          */
     return(TRUE);

  if (suspended->port != 255)             /* Falls 255 dann zaehlen...     */
     return(FALSE);

  /* Anzahl der Links dieses Users zum Knoten ermitteln */
  for (link = lnktbl, j = 0, user_links = 0; j < LINKNMBR; j++, link++)
    if (cmpcal(link->dstid, user_call) && link->state != L2SDSCED )
      user_links++;

  return(user_links > suspended->okcount);
}

/************************************************************************/
/*--- Test, ob User gesperrt ist                                     ---*/
/*                                                                      */
/* Parameter im Aufruf:                                                 */
/*      *u_block: Pointer auf User Kontrollblock                        */
/*                                                                      */
/* Rueckgabe:                                                           */
/*      TRUE: User ist gesperrt                                         */
/*     FALSE: User hat noch mindestens einen Kanal frei                 */
/*                                                                      */
/* Es wird auf PORT und OKCOUNT geprueft. OKCOUNT ist die Maximalzahl   */
/* der zulaessigen Links, unabhaengig vom Port. PORT ist immer gesperrt.*/
/* Um einen User nur fuer einen Port zu sperren ist also OKCOUNT auf    */
/* 255 zu setzen und PORT auf den Sperrport. Um einen USER auf eine     */
/* Maximalzahl von Links zu begrenzen, ist PORT auf 255 zu setzen und   */
/* OKCOUNT auf die gewuenschte Zahl an Links.                           */
/* Der Port 254 entspricht einem L4-User                                */
/*----------------------------------------------------------------------*/
BOOLEAN is_suspended(USRBLK *u_block)
{
  LNKBLK  *link;
  SUSPEND *suspended;
  char    *user_call;
  UBYTE   user_port;
  WORD    user_links;
  WORD    i, j;
#ifdef L1TCPIP
  TCPIP  *tc;
#endif /* L1TCPIP */


  switch (g_utyp(u_block->uid))
  {
    case HOST_USER:
             return(FALSE);                     /* User am Host-Terminal   */

                                                /* User im Level-2 Uplink  */
    case L2_USER:
            link = g_ulink(u_block->uid);
            user_call = link->dstid;
#ifdef __WIN32__
            user_port = (unsigned char)link->liport;
#else
            user_port = link->liport;
#endif /* WIN32 */
            break;

                                                /* User kommt per Circuit  */
    case L4_USER:
            user_call = ((CIRBLK *)g_ulink(u_block->uid))->upcall;
            user_port = 254;
            break;

#ifdef L1TCPIP
    case TCP_USER:
            tc = g_ulink(u_block->uid);
            user_call = tc->Upcall;
            user_port = (unsigned char)tc->port;
            break;
#endif /* L1TCPIP */

    /* damit der Compiler auch bei Optimierungen zufrieden ist ... */
    default:
            user_call = 0;
            user_port = 254;
            break;
  }

  /* Suspended-Liste nach dem User-Rufzeichen durchsuchen */
  for (suspended = sustab, i = 0; i < MAXSUSPEND; ++suspended, ++i)
    if (cmpcal(user_call, suspended->call))
      break;

  if (i == MAXSUSPEND)                    /* User nicht in der Liste       */
    return(FALSE);                        /* er darf..                     */

  if (user_port == suspended->port)       /* User gesperrt auf diesem Port */
    return(TRUE);                         /* also abwerfen..               */

  /* Anzahl der Links dieses Users zum Knoten ermitteln */
  for (link = lnktbl, j = 0, user_links = 0; j < LINKNMBR; ++j, ++link)
    if (cmpcal(link->dstid, user_call) && link->state != L2SDSCED )
      ++user_links;

  if (user_links > suspended->okcount)  /* Anzahl der Links ueberschritten */
    return(TRUE);                       /* also abwerfen..                 */

  return(FALSE);                        /* User darf..                     */
}

#ifdef PORT_SUSPEND
BOOLEAN is_port_suspended(USRBLK *u_block)
{
  LNKBLK  *link;
  char    *user_call = NUL;
  char    *p;
  UWORD    user_port = FALSE;
  PEER    *pp;
  int      max_peers = netp->max_peers;
  int      i;

  switch (g_utyp(u_block->uid))
  {
  case HOST_USER:
      return(FALSE);                 /* Host ist nie gesperrt !!! */

    case L2_USER:
            link = g_ulink(u_block->uid);
            /* In den Linkblock wird ein Zeiger auf das Rufzeichen abgelegt, das in */
            /* Wirklichkeit der Ansprechpartner dieses Linkes ist. Es ist das Erste */
            /* Rufzeichen im via-Feld ohne H-Bit oder das Ziel-Rufzeichen selbst.   */
            for (p = link->viaidl; *p; p += L2IDLEN)
              if ((p[L2IDLEN - 1] & L2CH) == 0)
                break;
               link->realid = *p ? p : link->dstid;
            user_call = link->realid;
            user_port = link->liport;
            break;
    case L4_USER:
        return(FALSE);

    /* damit der Compiler auch bei Optimierungen zufrieden ist ... */
    default:
            break;
  }

for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
    if (pp->used)
        {
     if ((cmpcal(user_call,pp->l2link->call) || (cmpcal(user_call,pp->l2link->digil) == TRUE)))
       if ((user_port == pp->l2link->port) == TRUE)
          return (FALSE);
        }
  return(TRUE);
}
#endif
/************************************************************************/
/*--- CRC-Tabelle berechnen                                          ---*/
/*                                                                      */
/*----------------------------------------------------------------------*/
void init_crctab(void)
{
    UWORD n;
    UWORD m;
    UWORD r;
    UWORD mask;

    static UWORD bitrmdrs[] = { 0x9188, 0x48C4, 0x2462, 0x1231,
                                0x8108, 0x4084, 0x2042, 0x1021};

    for (n = 0; n < 256; ++n)
      {
        for (mask = 0x0080, r = 0, m = 0; m < 8; ++m, mask >>= 1)
          if (n & mask)
            r = bitrmdrs[m] ^ r;
        crctab[n] = r;
      }
}

/*----------------------------------------------------------------------*/
static char *search_file(char *dir_name, char *text_name) {
/*                                                                      */
/* Das erste Wort in *clipoi als Filenamen annehmen. Dabei sind         */
/* Abkuerzungen erlaubt. Nach einem passenden File im Directory dir_name*/
/* suchen und den Namen bei Bedarf ergaenzen.                           */
/*                                                                      */
/* Rueckgabe: char * auf den Filenamen ohne Extention, wenn gefunden    */
/*            NULL  : kein passendes File gefunden                      */
/*                                                                      */
/* 18/02/95 DL9HCJ Extention nicht zurueckgeben, damit mehr Platz in der*/
/*                 Eingabezeile ist.                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/

  char         *cli_save;
  WORD          cli_cnt;
  struct ffblk  file_block;
  WORD          i;

  cli_save = clipoi;
  cli_cnt = clicnt;
  strcpy(text_name, dir_name);

  i = 0;
  while (isalnum(*clipoi) && (i < 8) && (clicnt > 0)) {
    strncat(text_name, (char *) clipoi++, 1);
    clicnt--;
    i++;
  }

  if (i == 0) return(NULL);

  if (i < 8)
    strcat(text_name, "*");

/*****************************************************************************/
/*** Da es beim MC68302-Betriebssystem keine Verzeichnisse gibt, unter-    ***/
/*** scheiden wir anhand der Dateiendung um was es sich hier handelt...    ***/
#ifdef MC68302
  if (dir_name == userexepath)
    strcat(text_name, ".APU");
  else if (dir_name == sysopexepath)
    strcat(text_name, ".APS");
  else
    strcat(text_name, ".TXC");
#endif
#if defined(__GO32__) || defined(__WIN32__)
  if (dir_name == textcmdpath)
    strcat(text_name, ".TXT");
  else
    strcat(text_name, ".EXE");
#endif
#ifdef ST
  if (dir_name == textcmdpath)
    strcat(text_name, ".TXT");
  else
    strcat(text_name, ".TTP");
#endif
  if (xfindfirst(text_name, &file_block, 0) == 0)
  {
    strcpy(text_name, dir_name);
    strcat(text_name, file_block.ff_name);

    return(text_name);
   }
  clipoi = cli_save;
  clicnt = cli_cnt;
  return(NULL);
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
BOOLEAN read_txt(void) {
/*                                                                      */
/* Das erste Wort *clipoi als Filenamen annehmen. Dabei sind            */
/* Abkuerzungen erlaubt. Nach einem passenden File im Directory "TEXT"  */
/* suchen und den Namen bei Bedarf ergaenzen. Das File lesen und an den */
/* User userpo schicken.                                                */
/*                                                                      */
/* Rueckgabe:                                                           */
/*       TRUE: Fehler, File nicht gefunden.                             */
/*      FALSE: kein Fehler.                                             */
/*                                                                      */
/*----------------------------------------------------------------------*/

  char    file_path[128];

  if (search_file(textcmdpath, file_path) != NULL)
  {
    ccpread(file_path);
    return(FALSE);
  }
  return(TRUE);
}


/*----------------------------------------------------------------------*/
BOOLEAN do_file(char *directory) {
/*                                                                      */
/* Sucht in Directory directory nach einem zu *clipoi passenden File    */
/* und fuehrt es bei Erfolg aus. Der Rest von clilin wird als Argument  */
/* uebergeben.                                                          */
/* Wenn die auszufuehrende Kommandozeile nicht zu lang ist, dann wird   */
/* die Ausgabe auf ein File umgeleitet und an den User ueber ccpread    */
/* geschickt.                                                           */
/* Als zusaetzliches Argument wird das User Call+SSID uebergeben        */
/*                                                                      */
/* Rueckgabe: TRUE : Fehler, kein File gefunden                         */
/*            FALSE: kein Fehler aufgetreten                            */
/*                                                                      */
/*----------------------------------------------------------------------*/

  /* Puffergroesse je nach OS */
#if !defined(__LINUX__) && !defined(__WIN32__)
#define MAXCMDLINE 512             /* so wie bisher */
#else
#define MAXCMDLINE MAXPATH         /* Linux (mehr hat keinen Sinn, siehe unten) */
#endif
  char    command_line[MAXCMDLINE];
  char    call[10];
  char   *tmpfile;
#ifdef __GO32__
  int     i;
#endif

  if (search_file(directory, command_line) != NULL)
  {
    if (clicnt > 0)
        {
                /* Vorerst case abfangen -> nur Sonderzeichen */
                /* zusaetzlich noch das, was spaeter noch maximal angehaengt */
                /* werden kann (Call, Umleitung und temp. Datei) */
                if ( 2*(clicnt-1) + strlen(command_line) + 21 <= MAXCMDLINE)
                        strncat(command_line, (char *) clipoi, clicnt);
                else
                {
                        putmsg("Commandline too long..\r");
                        return(FALSE);
                }
      }

#ifdef __LINUX__
    security_check(command_line);
#else
    command_line[strcspn(command_line,"<>|")] = NUL;
#endif
    strcat(command_line, " ");

    tmpfile = tempnam(textpath, "do");
    if (tmpfile == NULL) return(FALSE);
    call2str(call, usrcal);
    strcat(command_line, call);     /* Call anhaengen */
#ifndef MC68302
    strcat(command_line, " > ");    /* Ausgabe umleiten */
#else
    strcat(command_line, " ");
#endif
    strcat(command_line, tmpfile);  /* Zieldatei */
#ifdef __LINUX__
    if (strlen(command_line) > 127)        /* max. Laenge der Kommando-  */
#else
    if (strlen(command_line) > MAXPATH)    /* max. Laenge der Kommando-  */
#endif
    {                                     /* Zeile ist 127 bei DOSE     */
#ifdef SPEECH
      putmsg(speech_message(278));
#else
      putmsg("Commandline too long..\r");
#endif
      free(tmpfile);
    }
    else
    {
      xchdir(textpath);
      system(command_line);               /* Ausgaben ins Temporaerfile */
#ifdef __GO32__
      for (i = 0; i < MAXCOMS; i++)
        clear_rs232(i);                   /* DOS schaltet IRQs ab???    */
#endif
      xchdir(NULL);
#ifdef MAKRO_FILE
      /* Markiere, userpo->fp NICHT updaten. */
          userpo->read_ok = TRUE;
#endif
      userpo->fname = tmpfile;
      ccpread(tmpfile);
    }
    return(FALSE);                        /* Kommandoauswertung beenden */
  }
  return(TRUE);
}


/*----------------------------------------------------------------------*/
BOOLEAN extern_command(void) {
/*                                                                      */
/* Sucht in Directory "USEREXE" nach einem *clipoi passenden File       */
/* und fuehrt es bei Erfolg aus. Der Rest von clilin wird als Argument  */
/* uebergeben. Die Ausgabe wird auf EXTCMD.TMP umgeleitet und den User  */
/* ueber ccpread geschickt.                                             */
/* Als zusaetzliches Argument wird das User Call uebergeben             */
/* Falls der User Sysop Status hat, auch in "SYSEXE" nachsehen          */
/*                                                                      */
/* Rueckgabe: TRUE : Fehler, kein File gefunden                         */
/*            FALSE: kein Fehler aufgetreten                            */
/*                                                                      */
/*----------------------------------------------------------------------*/

  if (!do_file(userexepath))
    return(FALSE);

  if (issyso())
     return(do_file(sysopexepath));
  return(TRUE);

 }

/*----------------------------------------------------------------------*/
TRILLIAN intern_command(COMAND *tabelle) {
/*                                                                      */
/* Das erste Wort in clilin als Befehl annehmen und nach einem          */
/* passenden Befehl suchen und ihn bei Erfolg ausfuehren.               */
/*                                                                      */
/* Rueckgabe: ERRORS: Befehl mit Sonderzeichen                          */
/*            YES   : Fehler, nichts gefunden                           */
/*            NO    : kein Fehler, Befehl wurde ausgefuehrt.            */
/*                                                                      */
/*----------------------------------------------------------------------*/
  COMAND        *cmdpoi;
  const char    *cmdnam;
  char          *command_line_save;
  WORD           command_length_save;

  command_line_save = clipoi;
  command_length_save = clicnt;
  for (cmdpoi = tabelle;
       cmdpoi->cmdstr != 0;
       ++cmdpoi) {
    clipoi = command_line_save;
    clicnt = command_length_save;
    cmdnam = cmdpoi->cmdstr;
    while ((clicnt != 0) && (*clipoi != ' ')) {
      if ((char) toupper(*clipoi) != *cmdnam)
        break;
      ++clipoi;
      --clicnt;
      ++cmdnam;
    }
    if (clicnt == 0 || *clipoi == ' ')
      break;
  }
  if (cmdpoi->cmdfun != NULL) {
    skipsp(&clicnt, &clipoi);
    (*cmdpoi->cmdfun)(cmdpoi->cmdpar);
     return(NO);
  }
  clipoi = command_line_save;
  clicnt = command_length_save;
  while ((clicnt != 0) && (*clipoi != ' '))
   {
    if (!isalnum(*clipoi))
      return(ERRORS);
    ++clipoi;
    --clicnt;
   }
  clipoi = command_line_save;
  clicnt = command_length_save;
  return(YES);
}

/************************************************************************/
/* Unbekanntes Kommando, Anzahl Fehler zaehlen                          */
/*----------------------------------------------------------------------*/
void inv_cmd(void)
{
  invmsg();
  if (g_ulink(userpo->uid) != hstubl)
    if (++userpo->errcnt >= 5)
      disusr(userpo->uid);
}

/************************************************************************/
/* Binaerfile einladen, Pruefsumme berechnen.                           */
/*----------------------------------------------------------------------*/
void program_load(MBHEAD *mhdp)
{
  MBHEAD *mbp;
#ifndef AUTOBINAERFIX
  char  c;
#else
  UBYTE c;
#endif /* AUTOBINAERFIX */

  while (mhdp->mbgc < mhdp->mbpc) {
    c = getchr(mhdp) & 0xFF;
    fputc(c, loadfp);
    checksum += c;
    crc = crctab[crc >> 8] ^ ((crc << 8) | (UWORD)c);
    bytecnt--;
    if (bytecnt == 0L) {
      userpo->status = US_CCP;
      fclose(loadfp);
      mbp = getmbp();
#ifndef MC68302
      if (xrename(loadtmp, loadname) == 0)
#endif
        strcpy(loadtmp, loadname);
#ifndef AUTOBINAERFIX
      putprintf(mbp,"%s ok. Checksum: %ld  CRC: %u\r", loadtmp, checksum, crc);
#else
      putprintf(mbp,"%s ok.\rChecksum: %ld  CRC: %u\r", loadtmp, checksum, crc);
#endif /* AUTOBINAERFIX */
      prompt(mbp);
      seteom(mbp);
      checksum = 0L;
      crc = 0;
      loadname[0] = loadtmp[0] = NUL;
    }
  }
}

/*----------------------------------------------------------------------*/
/* Binaerfileuebertragung nach dem THP-AUTOBIN-Format starten.          */
/*----------------------------------------------------------------------*/
void start_autobin(void)
{
  MBHEAD *mbp;

  if (strnicmp(clilin, "#BIN#", 5) == 0)
  {
    clipoi = clilin + 5;
    clicnt -= 5;

    if ((bytecnt = nxtlong(&clicnt,&clipoi)) != 0L)
    {
      userpo->status = US_RBIN;
      xremove(loadname);
      mbp = getmbp();
      putstr("#OK#\r", mbp);
      seteom(mbp);
      return;
    }
  }

  fclose(loadfp);
  loadname[0] = loadtmp[0] = NUL;
  userpo->status = US_CCP;
}

/*----------------------------------------------------------------------*/
/* Text-Eingabe starten. Der Text wird zunaechst in die Datei TMP.TXT   */
/* geschrieben. Wird ein '.' als erstes Zeichen einer Zeile empfangen,  */
/* wird TMP.TXT umbenannt in den vom Sysop angegebenen Dateinamen.      */
/* Da nur ein temporaeres File TMP.TXT zur gleichen Zeit vorhanden      */
/* sein kann, darf bei mehreren eingelogten Sysops immer nur einer      */
/* Dateien editieren.                                                   */
/*----------------------------------------------------------------------*/
void load_text(void)
{
  MBHEAD *mbp;

  *clipoi = NUL;
  clipoi = clilin;
  if (*clipoi == '.' || *clipoi == 0x1A) {
    userpo->status = US_CCP;
    fclose(loadfp);
    mbp = getmbp();
#ifndef MC68302
#ifdef __WIN32__
    remove(loadname);
#endif
    if (xrename(loadtmp, loadname) == 0)
#endif
      strcpy(loadtmp, loadname);
    putprintf(mbp, "%s ok!\r", loadtmp);
    prompt(mbp);
    seteom(mbp);
    loadname[0] = loadtmp[0] = NUL;
  }
  else {
    fputs(clilin, loadfp);
    fputc('\n', loadfp);
  }
}

/************************************************************************/
/* Vom User eingegebene Zeichen mit aktuellem Passwort vergleichen und  */
/* bei korrekter Eingabe SYSOP-Flag setzen. In jedem Fall den Versuch   */
/* in die Datei SYSOP-PRO aufnehmen.                                    */
/*----------------------------------------------------------------------*/
void get_password(void)
{
  WORD i;
  char file[128];
  FILE *fp;
  char pwd[6];                            /* ala BayBox   DL1XAO */

  memcpy(pwd, userpo->paswrd, 5);
  pwd[5] = NUL;
  i = FALSE;
  if (strlen((char *)clipoi) > 80)        /* maximal 80 Zeichen  */
    *(clipoi + 80) = NUL;
  if (clicnt >= 5 && strstr((char *)clipoi, pwd) != NULL) {
    userpo->errcnt = 0;
    i = TRUE;
#ifndef USER_PASSWORD
    userpo->sysflg = 1;
#else
    if (userpo->status == US_WPWD)
      userpo->sysflg = 1;
    userpo->pwdtyp = PW_USER;
#endif
  }
  else
    if (++userpo->errcnt >= 5)
     {
      disusr(userpo->uid);
      return;
     }

  /* Wenn SYSOP-Protokoll gefuehrt werden soll, Rufzeichen, Datum, */
  /* Zeit und ob SYSOP-Befehl erfolgreich in eine Datei mit Namen  */
  /* SYSOP.PRO schreiben. Diese Datei kann spaeter vom SYSOP       */
  /* ausgelesen und ausgewertet werden.                            */

#ifndef USER_PASSWORD
  if (syspro_flag == TRUE)
#else
  if (syspro_flag == TRUE && userpo->status == US_WPWD)
#endif
  {
    strcpy(file, confpath);
    strcat(file, "SYSOP.PRO");
    if ((fp = xfopen(file,"at")) != NULL) {
      fprintf(fp, "%24.24s %6.6s: %s\n",
                  ctime(&sys_time),
                  calofs(UPLINK, userpo->uid),
                  i ? "accepted" : "rejected");
      fclose(fp);
    }
  }
  userpo->status = US_CCP;
}

/*
 * CTEXT mit mehr Moeglichkeiten (wie prompt)
 */
void out_ctext(char *name, MBHEAD *mbp)
{
  FILE *fp;
  char line[128];
  char *c;

  if ((fp = xfopen(name, "rt")) != NULL)
  {
    while (fgets(line, 126, fp) != NULL)
    {
      if ((c = strchr(line, '\n')) != NULL)
        *c = CR;
      prompt2str(mbp, line);
    }
    fclose(fp);
  }
}

/*
 * Einstiegsport des User feststellen fuer LOOP-Warning
 */
int userport(USRBLK *u)
{
  CIRBLK *cirp;
  LNKBLK *lnkp;
#ifdef L1TCPIP
  TCPIP  *tpoi;
#endif /* L1TCPIP */
  PEER   *pp;

  /* Einstiegsport des Users feststellen (fuer LOOP-Warning) */
  switch (g_utyp(u->uid)) {
    case L4_USER:  /* User ist Circuit    */
      cirp = (CIRBLK *) g_ulink(u->uid);
      if (find_best_qual(find_node_this_ssid(cirp->l3node), &pp, DG) > 0)
        return(pp->l2link->port);
      break;

    case L2_USER:  /* User ist L2-Link    */
      lnkp = (LNKBLK *) g_ulink(u->uid);
      return(lnkp->liport);

#ifdef L1TCPIP
    case TCP_USER:  /* User ist TCPIP     */
      tpoi = (TCPIP *) g_ulink(u->uid);
      return(tpoi->port);
#endif /* L1TCPIP */
  }
  return(L2PNUM); /* Defaultport */
}

/*
 * Manche Befehle erhalten die Anwort erst ein bischen spaeter (PING,
 * DEST, NRR usw), deshalb mit besonderer Vorsicht ausgeben.
 */
void send_async_response(USRBLK *ublk, const char *title, const char *text) {
  MBHEAD *mbp;
  USRBLK *up;
  LHEAD   mbhd;

  /* Antwort generieren und an den User senden                      */
  for (up = (USRBLK *) usccpl.head;
       (USRBLK *) &usccpl != up;
       up = (USRBLK *) up->unext)
  {
    if (up == ublk)
      /* das ist der gesuchte User, darf er die Antwort bekommen? */
      if ((up->status == US_CCP || up->status == US_TALK))
      {
        (mbp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2fflg = L2CPID; /* tnx DB2OS */
        putchr('\r', mbp);
        putalt(alias, mbp);
        putid(myid, mbp);
        putprintf(mbp, "> %s%s\r", title, text);
        rwndmb(mbp);
        inithd(&mbhd);
        relink((LEHEAD *)mbp, (LEHEAD *)mbhd.tail);
        if (!itousr(up->uid, NO_UID, FALSE, mbp))
          dealmb(mbp);
      }
  }
}

/************************************************************************/
/* Parameternummer aus Namen ermitteln                                           */
/*----------------------------------------------------------------------*/
static UWORD getparnum(PARAM *partab, int len)
{
  size_t l;
  UWORD  num;
  PARAM  *p;

  for (p = partab, num = 1; num <= len; p++, num++)
    if (p->paradr) {
      l = strlen(p->parstr);
      if (!strnicmp(p->parstr, clipoi, l)) {
        clipoi += l;
        clicnt -= (WORD)l;
        return(num);
      }
    }
  return(0);
}

/* Interpreter fuer einfache Parametertabellen.
 */
void ccp_par(const char *name, PARAM *partab, int len)
{
  int     i;
  UWORD   parnum;
  MBHEAD *mbp;
  PARAM  *parpoi;
  UWORD   nummer;
  UWORD   wert;

  nummer = 0;
  if (issyso())
  {
    if (!isdigit(*clipoi))
      nummer = getparnum(partab, len);
    else
      nummer = nxtnum(&clicnt, &clipoi);
  }

  if (nummer != 0 && nummer <= len)
  {
    parpoi = partab + nummer-1;
    if (   clicnt != 0
        && ((wert = nxtnum(&clicnt, &clipoi)) >= parpoi->minimal)
        && (wert <= parpoi->maximal)
       )
      *(parpoi->paradr) = wert;
    mbp = putals(name);
    putprintf(mbp, "%u: %s = %u (%u...%u)\r", nummer, parpoi->parstr,
          *(parpoi->paradr), parpoi->minimal, parpoi->maximal);
  } else {
    mbp = putals(name);

    mbp->l4time = mbp->mbpc;
    for (i = 0, parnum = 1, parpoi = partab;
         parnum <= len;  ++i, ++parnum, ++parpoi)
    {
      if (i >= 3) {
        putchr('\r', mbp);
        mbp->l4time = mbp->mbpc;
        i = 0;
      } else
#ifdef __WIN32__
        putspa((char)(25 * i), mbp);
#else
        putspa(25 * i, mbp);
#endif /* WIN32 */
      putprintf(mbp, "%02u:%-12s %5u", parnum, parpoi->parstr, *(parpoi->paradr));
    }
    putchr('\r', mbp);
  }
  prompt(mbp);
  seteom(mbp);
}

/* Rufzeichen eintragen/Austragen.
 */
void ccp_call(char *id)
{
  if (!issyso())
    invmsg();

  if (*clipoi == '-')             /* Call austragen? */
  {
    id[0] = NUL;
    putmsg("ok\r");
  }
  else
    if (getcal(&clicnt, &clipoi, TRUE, id) != YES) { /* ein Call ? */
#ifdef SPEECH
      putmsg(speech_message(167));
#else
      putmsg("Invalid call\r");
#endif
    } else {
      MBHEAD *mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
    }
}

/* Fuer den L4 feststellen, wie gross fragmentierte Frames sein duerfen */

int
ptc_p_max(void *link, UBYTE typ)
{
  UID     p_uid;

  p_uid = (ptctab + g_uid(link, typ))->p_uid;
  if (p_uid == NO_UID) return(256);
  if (g_utyp(p_uid) == L4_USER)
    return(236);
  return(256);
}

#ifdef MAKRO_USER
/* Wir zaehlen alle Interlinks und */
/* Convers-Host's die aktiv sind.  */
static UWORD Interlinks_zaehlen(void)
{
  PEER     *pp;
  PERMLINK *p;
  int       i;
  int       max_peers = netp->max_peers;
  UWORD     interlinks = 0;

  /* Interlinks zaehlen ausser    */
  /* LOCAL ,LOCAL_N, LOCAL_V.      */
  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
  {
    /* Nur benutzte eintraege. */
    if (pp->used)
    {
      /* Rufzeichen? */
      if (pp->l2link->call)
      {
        /* Pruefe ob das Rufzeichen ein LOCAL ist. */
        if (pp->typ >= LOCAL)
          continue;

        /* Ist der PEER aktiv? */
        if (pp->nbrl2l == NULL)
          /* Nein! */
          continue;

        /* Kein LOCAL/N/V und aktiv.*/
        interlinks++;
      } /* kein Rufzeichen gefunden. */
    } /* freier eintrag. */
  }

  /* Zaehlen der Convers-Host's. */
  for (i = 0; i < MAXCVSHOST; i++)
  {
    p = permarray[i];
    /* Nur eingetragene Host's zaehlen. */
    if (p != NULL)
    {
      /* Ist der Host aktiv? */
      if (p->connection)
        /* Dann wird gezaehlt. */
        interlinks++;

      /* Host ist nicht aktiv. */
      continue;
    }

    /* Kein Host gefunden. */
    /* Weiter suchen.      */
    break;
  }

  /* Gesamtanzahl der Interlinks und Convers-Host's. */
  return(interlinks);
}
#endif

#ifdef CONNECTTIME
/*************************************************************************/
/*                                                                       */
/* Connect-Zeit in Flexnet-Stil ausgeben.                                */
/*  1 CB0RIE    CB1GLA    IXF  0  0  0   13 136  138     96   13h,21m  - */
/*                                                            =======    */
/*************************************************************************/
char *ConnectTime(unsigned long contime)
{
  static char   runtime[8 + 1];
  char         *time = runtime;
  register int  zeit;
  unsigned long sec,
                min,
                std;

  zeit = contime;
  sec = zeit % 60L; zeit /= 60L;
  min = zeit % 60L; zeit /= 60L;
  std = zeit % 24L; zeit /= 24L;

  if (zeit > 0)
    sprintf(time, "%2dd,%2luh",zeit, std);                 /* Tage, Stunden. */
  else
    if (std > 0)
      sprintf(time, "%2luh,%2lum",std, min);             /* Stunden, Minuten. */
    else
      if (min > 0)
        sprintf(time, "%2lum,%2lus", min, sec);         /* Minuten, Sekunden. */
      else
        {
          if (sec > 0)
            sprintf(time, "    %2lus", sec);          /* Ausgabe in Sekunden. */
          else
            sprintf(time, " ");
        }

  return(time);
}
#endif /* CONNECTTIME */

/* Ende src/l7utils.c */
