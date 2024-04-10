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
/* File src/callstr.c (maintained by: ???)                              */
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

static BOOLEAN  pnmtch(const char *, char *);

/************************************************************************/
/*                                                                      */
/* "get frame id"                                                       */
/*                                                                      */
/* Ein Rufzeichen in AX.25-Notation aus einem Buffer lesen.             */
/*                                                                      */
/************************************************************************/
BOOLEAN
getfid(char *dest, MBHEAD *mbhd)
{
  char  c;
  WORD  i;

  if (mbhd->mbpc - mbhd->mbgc < L2IDLEN)
    return (FALSE);
  for (i = 0; i < L2CALEN; ++i)
  {
    if (((c = getchr(mbhd)) & L2CEOA) != 0)
      return (FALSE);
    *dest++ = (c >> 1) & 0x7F;
  }
  *dest = '\0';
  if (is_down_suspended((dest - L2CALEN), 253))
  {
    return (FALSE);
  }
  *dest = getchr(mbhd);
  return (TRUE);
}

/* TEST DG9OBU */

/************************************************************************/
/*                                                                      */
/* "get frame id complete"                                              */
/*                                                                      */
/* Ein Rufzeichen in AX.25-Notation aus einem Buffer lesen. Es wird     */
/* immer ein komplettes Call gelesen um im Eingangsbuffer eine          */
/* definierte Position zu erreichen, erst danach wird das Call          */
/* ueberprueft.                                                         */
/************************************************************************/
BOOLEAN
getfidc(char *dest, MBHEAD *mbhd)
{
  BOOLEAN bRetVal = TRUE;

  char c;

  WORD i;

  /* Noch ein Call im Buffer ? */
  if (mbhd->mbpc - mbhd->mbgc < L2IDLEN)
    return (FALSE);

  /* Call lesen */
  for (i = 0; i < L2CALEN; ++i)
  {
    /* EOA-Bit darf nicht vorzeitig kommen ! */
    if (((c = getchr(mbhd)) & L2CEOA) != 0)
      bRetVal = FALSE;

    *dest++ = (c >> 1) & 0x7F;
  }

  /* SSID ausblenden */
  *dest = '\0';

  /* Check */
  if (is_down_suspended((dest - L2CALEN), 253))
    bRetVal = FALSE;

  /* SSID hinzufuegen */
  *dest = getchr(mbhd);

  return (bRetVal);
}

#ifndef MC68K
/************************************************************************/
/*                                                                      */
/* "compare call"                                                       */
/*                                                                      */
/* Rufzeichen vergleichen. EOA und H-Bit werden ignoriert.              */
/*                                                                      */
/************************************************************************/
BOOLEAN
cmpcal(const char *id1, const char *id2)
{
  return (strncmp(id1, id2, L2CALEN) == 0);
}

/************************************************************************/
/*                                                                      */
/* "compare id"                                                         */
/*                                                                      */
/* Rufzeichen und SSID vergleichen. EOA und H-Bit werden ignoriert.     */
/*                                                                      */
/************************************************************************/
BOOLEAN
cmpid(const char *id1, const char *id2)
{
  return (   cmpcal(id1, id2)
          && (id2[L2CALEN] & 0x1E) == (id1[L2CALEN] & 0x1E));
}

/************************************************************************/
/*                                                                      */
/* "compare id list"                                                    */
/*                                                                      */
/* Zwei Rufzeichenlisten werden verglichen. Die H-Bits muessen in allen */
/* Rufzeichen uebereinstimmen.                                          */
/*                                                                      */
/************************************************************************/
BOOLEAN
cmpidl(const char *idl1, const char *idl2)
{
  while (*idl2 != '\0')
  {
    if (memcmp(idl1, idl2, L2IDLEN))
      return (FALSE);
    idl2 += L2IDLEN;
    idl1 += L2IDLEN;
  }

  return ((*idl1 == '\0') ? TRUE : FALSE);
}

/************************************************************************/
/*                                                                      */
/* "copy id"                                                            */
/*                                                                      */
/* Rufzeichen kopieren. End-of-Address und Has-been-digipeated Bit      */
/* werden geloescht.                                                    */
/*                                                                      */
/************************************************************************/
void
cpyid(char *dest, const char *source)
{
  memcpy(dest, source, L2CALEN);
  dest[L2IDLEN - 1] = source[L2IDLEN - 1] & ~(L2CEOA | L2CCR);
}

/************************************************************************/
/*                                                                      */
/* "copy id list"                                                       */
/*                                                                      */
/* Rufzeichenliste kopieren. EOA und H Bit bleiben erhalten.            */
/*                                                                      */
/************************************************************************/
void
cpyidl(char *dest, const char *source)
{
  while (*source != '\0')
  {
    memcpy(dest, source, L2IDLEN);
    source += L2IDLEN;
    dest += L2IDLEN;
  }
  *dest = '\0';
}
#endif
/************************************************************************/
/*                                                                      */
/* "add id"                                                             */
/*                                                                      */
/* An eine Rufzeichenliste ein Call anhaengen.                          */
/*                                                                      */
/************************************************************************/
void
addid(char *dest, const char *source)
{
  char *cp;

  cp = (char *)strchr((char *)dest, 0);
  if (&cp[0] < &dest[L2VLEN])
  {
    memcpy(cp, source, L2IDLEN);
    cp[L2IDLEN] = 0;
  }
}

/************************************************************************/
/*                                                                      */
/* "first not digipeated"                                               */
/*                                                                      */
/* Das erste Call aus einer Rufzeichenliste lesen, das noch nicht       */
/* gedigipeated hat. Es darf weder unser Call sein noch ein gesetztes   */
/* H-Bit haben.                                                         */
/*                                                                      */
/************************************************************************/
const char *
ndigipt(const char *digis)
{
  const char *viap;

  for (viap = digis; *viap; viap += L2IDLEN)
    if ((viap[L2IDLEN - 1] & L2CH) == 0)
      break;
  if (*viap)
    if (istome(viap))
      viap += L2IDLEN;
  return (viap);
}

#ifdef MH_LISTE

const char *
ydigipt(const char *viap)
{

  for (viap = rxfhdr + L2ILEN; *viap; viap += L2IDLEN)
      if ((viap[L2IDLEN - 1] & L2CH) != 0)
        if (!viap[L2IDLEN] || !(viap[L2ILEN - 1] & L2CH))
        break;
  if (*viap)
    if (istome(viap))
      viap += L2IDLEN;
  return (viap);
}
#endif

/************************************************************************/
/*                                                                      */
/* "put via"                                                            */
/*                                                                      */
/* Eine Rufzeichenliste in AX.25 in einen Frame schreiben. Das letzte   */
/* Rufzeichen erhaelt das EOA-Bit.                                      */
/*                                                                      */
/************************************************************************/
void
putvia(const char *idl, MBHEAD *mbhd)
{
  while (*idl != '\0')
  {
    putfid(idl, mbhd);
    idl += L2IDLEN;
  }
  *(mbhd->mbbp - 1) |= L2CEOA;
}

/************************************************************************/
/*                                                                      */
/* "put frame id"                                                       */
/*                                                                      */
/* Ein Rufzeichen in AX.25-Notation in einen Buffer schreiben.          */
/*                                                                      */
/************************************************************************/
void
putfid(const char *id, MBHEAD *mbhd)
{
  WORD i;

  for (i = 0; i < L2CALEN; ++i)
    putchr((char)(*id++ << 1), mbhd);
  putchr(*id, mbhd);
}

/************************************************************************/
/*  Convert Type CALL to string                                         */
/*----------------------------------------------------------------------*/
void
call2str(char *d, char *call)
{
  char c;
  int  i;

  for (i = 0; i < L2IDLEN - 1; i++)
  {
    c = *call++;
    if (c > ' ')
      *d++ = c;
    else if (c < ' ')
      *d++ = ' ';
  }

  c = (*call >> 1) & 0x0f;
  if (c != 0)
  {
    *d++ = '-';
    if (c >= 10)
    {
      *d++ = '1';
      c -= 10;
    }
    *d++ = '0' + c;
  }

  *d = '\0';
}

/************************************************************************/
/*  Convert Type CALL to string and strip SSID                          */
/*----------------------------------------------------------------------*/
void
callss2str(char *d, char *call)
{
  WORD i;

  for (i = 0; i < L2IDLEN - 1 && *call != ' '; i++)
    *d++ = *call++;

  *d = '\0';
}

/************************************************************************/
/*  Convert string to CALL                                              */
/*----------------------------------------------------------------------*/
void
str2call(char *d, char *call)
{
  WORD l = strlen(call);

  getcal(&l, (char **)&call, TRUE, d);
}


/*-----------------------------------------------------------------------*/
/* diese funktion hab ich 'gefunden' und etwas abgemagert                */
  /* wer ist ich???   ^^^                              */
/*   Author: James R. Van Zandt                                          */
/*           27 Spencer Dr.                                              */
/*           Nashua NH 03062                                             */
/*           <jrv@mitre-bedford.arpa>                                    */
/*                                                                       */
/*-----------------------------------------------------------------------*/
static BOOLEAN
pnmtch(const char *pat, char *nam)
{
  const char **a,
        **n,
         *s;
  const char  *alive[MAXMASK + 2];
  const char  *next[MAXMASK + 2];

  next[0] = pat;
  next[1] = 0;
  while (next[0])
  {
    a = alive;
    n = next;

    while (a <= alive + MAXMASK && ((*a++ = *n) != 0))
    {
      if (**n == '*')
      {
        for (s = *n; *++s == '*';);
        if (s != n[1] && a <= alive + MAXMASK)
          *a++ = s;
      }
      n++;
    }
    if (*nam == 0)
      return ((*a[-2] == 0) ? TRUE : FALSE);
    a = alive;
    n = next;
    while (*a)
    {
      switch (**a)
        {
          case '?':
            *n++ = *a + 1;
            break;
          case '*':
            if (n[-1] != *a)
              *n++ = *a;

           break;

          default:
            if (**a == *nam)
              *n++ = *a + 1;
        }
      a++;
    }
    *n = 0;
    nam++;
  }
  return (FALSE);
}

/* c6mtch() - 6 zeichen matschen...                                     */
/* TRUE wenn string 's' auf muster 'pat' passt                          */
/* nicht dass es umstaendlich waere... der string wird umkopiert und    */
/* NULterminiert, um eine vorgefundene Funktion zu benutzen             */
BOOLEAN
c6mtch(const char *s, const char *pat)
{
  char c6[6 + 1];
  WORD  i;

  for (i = 0; i < 6; i++)
  {
    if (s[i] == ' ')
      break;
    c6[i] = isascii(s[i]) ? toupper(s[i]) : s[i];
  }
  c6[i] = '\0';
  return ((i > 0) ? pnmtch(pat, c6) : FALSE);
}

/************************************************************************/
/* "direct heard callsign"                                              */
/* Das Call aus einer Rufzeichenliste lesen, das direkt gehoert wurde.  */
/************************************************************************/
const char *
dheardcall(const char *viap)
{
  for (viap += 2 * L2IDLEN; *viap; viap += L2IDLEN)
    if ((viap[L2IDLEN - 1] & L2CH) == 0)
      break;
  return (viap-L2IDLEN);
}

/* End of src/callstr.c */
