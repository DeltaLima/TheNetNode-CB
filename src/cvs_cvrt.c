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
/* File src/cvs_cvrt.c (maintained by: DL1XAO)                          */
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

/* Dies ist eine stark geaenderte Version des convert, welches von
   Thomas Osterried, DL9SAU geschrieben wurde.
   Die Verwaltung ist noch erhalten, die Konvertierung laeuft aber
   ganz anders ab. Hoffentlich schneller und speichersparender.
   Diese Version ist von DL1XAO.
*/
#include "tnn.h"

#ifdef PPCONVERS

#define  BIN  255

#define  ISO    0
#define  dumb   1
#define  TeX    2
#define  IBM7   3
#define  ROMAN8 4
#define  IBMPC  5
#define  ATARI  6
#define  MAC    7

#define  CHARSETS  8  /* # of the lines above  defined charsets */
#define  CHARS    96  /* 96 ISO char's are defined */

static int iso2asc(int);
static void cvitexfn(char *, char *);
static void cvitabfn(char *, char *);
static void cvodmbfn(char *, char *);
static void cvotexfn(char *, char *);
static void cvotabfn(char *, char *);

typedef struct {
  char *samples;
  void *par;
  void (*ifunc)(char *, char *);
  void (*ofunc)(char *, char *);
 } CONVERT;


struct charsets {
  char *name;
  WORD ind;
};

static struct charsets charsets[] = {
  {"iso-8859-1",  ISO},
  {"ansi",        ISO},
  {"8bit",        ISO},
  {"dumb",        dumb},
  {"ascii",       dumb},
  {"none",        dumb},
  {"us",          dumb},
  {"tex",         TeX},
  {"ibm7bit",     IBM7},
  {"7bit",        IBM7},
  {"commodore",   IBM7},
  {"c64",         IBM7},
  {"digicom",     IBM7},
  {"roman8",      ROMAN8},
  {"ibmpc",       IBMPC},
  {"pc",          IBMPC},
  {"at",          IBMPC},
  {"xt",          IBMPC},
  {"atari",       ATARI},
  {"macintosh",   MAC},  /* by HB9JNX */
  {"mac",         MAC},  /* by HB9JNX */
  {"binary",      BIN},
  {"image",       BIN},
  {(char *) NULL, -1}
};

#define MAXBUF  2048

#define isupperalpha(x)  (isalpha(x) && isupper(x))

static char  outbuf[MAXBUF];

static char ibm7[] = {
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x5B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x5C\x00\x00\x00\x00\x00\x5D\x00\x00\x7E"
  "\x00\x00\x00\x00\x7B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x00\x00\x7C\x00\x00\x00\x00\x00\x7D\x00\x00\x00" };

static char roman8[] = {
  "\xFF\xB8\xBF\xBB\xBA\xBC\x00\xBD\xAB\x00\xF9\xFB\x00\xF6\x00\xB0"
  "\xB3\xFE\x00\x00\xA8\x00\x00\x00\x00\x00\xFA\xFD\xF7\xF8\x00\xB9"
  "\xA1\xE0\xA2\xE1\xD8\xD0\xD3\xB4\xA3\xDC\xA4\xA5\xE6\xE5\xA6\xA7"
  "\xE3\xB6\xE8\xE7\xDF\xE9\xDA\x00\xD2\xAD\xED\xAE\xDB\x00\xF0\xDE"
  "\xC8\xC4\xC0\xE2\xCC\xD4\xD7\xB5\xC9\xC5\xC1\xCD\xD9\xD5\xD1\xDD"
  "\xE4\xB7\xCA\xC6\xC2\xEA\xCE\x00\xD6\xCB\xC7\xC3\xCF\x00\xF1\xEF" };

static char stibm2[] = {
  "\xFF\xAD\x9B\x9C\x00\x9D\x00\x00\x00\x00\xA6\xAE\xAA\xC4\x00\x00"
  "\xF8\xF1\xFD\x00\x00\xE6\x00\xF9\x00\x00\xA7\xAF\xAC\xAB\x00\xA8"
  "\x00\x00\x00\x00\x8E\x8F\x92\x80\x90\x00\x00\x00\x00\x00\x00\x00"
  "\x00\xA5\x00\x00\x00\x00\x99\x00\x00\x00\x00\x00\x9A\x00\x00\xE1"
  "\x85\xA0\x83\x00\x84\x86\x91\x87\x8A\x82\x88\x89\x8D\xA1\x8C\x8B"
  "\xE5\xA4\x95\xA2\x93\x00\x94\xF6\xED\x97\xA3\x96\x81\x00\x00\x98" };

/* This charset converts the characters from 0240 to 0377 to the
 * character set used in Apple's Macintosh class computers
 * (or those computers that run MacOS)
 * This was derived from the Table T-40 (p.248) of "Inside Macintosh
 * Volume I", which is quite old and may have been extended since...
 * Added by Tom Sailer, HB9JNX, 18.6.95
 * leider sind einige Zeichen doppelt definiert gewesen, der andre Wert
 * ist: mac[0x0A]=0xd2; mac[0x1A]=0xd3;         XAO
 */
static char mac[] = {
  "\x00\xc1\xa2\xa3\x00\xb4\x00\xa4\xac\xa9\xbb\xc7\xc2\x00\xa8\x00"
  "\xa1\xb1\x00\x00\xab\xb5\xa6\x00\x00\x00\xbc\xc8\x00\x00\x00\xc0"
  "\xcb\x00\x00\xcc\x80\x81\xae\x82\x00\x83\x00\x00\x00\x00\x00\x00"
  "\x00\x84\x00\x00\x00\xcd\x85\x00\xaf\x00\x00\x00\x86\x00\x00\xa7"
  "\x88\x87\x89\x8b\x8a\x8c\xbe\x8d\x8f\x8e\x90\x91\x93\x92\x94\x95"
  "\x00\x96\x98\x97\x99\x9b\x9a\xd6\xbf\x9d\x9c\x9e\x9f\x00\x00\xd8" };


CONVERT cnvrtinf[CHARSETS] = {
  { "\xc4\xd6\xdc\xe4\xf6\xfc\xdf", NULL,   NULL,     NULL     },
  { "AeOeUeaeoeuess",               NULL,   NULL,     cvodmbfn },
  { "\"A\"O\"U\"a\"o\"u\"s",        NULL,   cvitexfn, cvotexfn },
  { "\x5b\x5c\x5d\x7b\x7c\x7d\x7e", ibm7,   cvitabfn, cvotabfn },
  { "\xd8\xda\xdb\xcc\xce\xcf\xde", roman8, cvitabfn, cvotabfn },
  { "\x8e\x99\x9a\x84\x94\x81\xe1", stibm2, cvitabfn, cvotabfn },
  { "\x8e\x99\x9a\x84\x94\x81\x9e", stibm2, cvitabfn, cvotabfn },
  { "\x80\x85\x86\x8a\x9a\x9f\xa7", mac,    cvitabfn, cvotabfn }
 };

/*---------------------------------------------------------------------------*/

WORD get_charset_by_name(char *buf)
{
  WORD len;
  struct charsets *p_charset;

  if ((len = (WORD)strlen(buf)) == 0) return(ISO);

  for (p_charset = charsets; p_charset->name; p_charset++)
    if (!strncmp(p_charset->name, buf, len))
      return(p_charset->ind);

  return(-1);
}

/*---------------------------------------------------------------------------*/

char *get_charset_by_ind(WORD ind)
{
  struct charsets *p_charset;

  for (p_charset = charsets; p_charset->name; p_charset++)
    if (p_charset->ind == ind)
      return(p_charset->name);

  return((char *) NULL);
}

/*---------------------------------------------------------------------------*/

char *list_charsets(void)
{
  char buf[256];
  char tmp[64];
  WORD i;
  struct charsets *p_charset;

  static char *p = (char *) NULL;

  if (p) return(p);

  *buf = '\0';
  for (i = 0; i < CHARSETS; i++) {
    *tmp = '\0';
    for (p_charset = charsets; p_charset->name; p_charset++) {
      if (i != p_charset->ind) continue;
      if (*tmp) strcat(tmp, ", ");
      strcat(tmp, p_charset->name);
    }
    strcat(tmp, " : ");
    strcat(tmp, cnvrtinf[i].samples);
    strcat(tmp, "\r");
    strcat(buf, tmp);
  }

  /* No good solution - but BIN is not part of CHARSERTS */
  *tmp = '\0';
  for (p_charset = charsets; p_charset->name; p_charset++) {
    if (p_charset->ind != BIN) continue;
      if (*tmp) strcat(tmp, ", ");
      strcat(tmp, p_charset->name);
  }
  strcat(tmp, "\r");
  strcat(buf, tmp);

  return(p = strdup(buf));
}

/*---------------------------------------------------------------------------*/

char *convertin(WORD in, char *string)
{
 CONVERT *cnvrt;

  if (in <= CHARSETS-1) {
    cnvrt = &cnvrtinf[in];
    if (cnvrt->ifunc != NULL) {
      stibm2[63] = (in == ATARI) ?  0x9E : 0xE1;
      cnvrt->ifunc(cnvrt->par, string);
      return(outbuf);
    }
  }
  return(string);
}

char *convertout(WORD out, char *string)
{
 CONVERT *cnvrt;

  if (out <= CHARSETS-1) {
    cnvrt = &cnvrtinf[out];
    if (cnvrt->ofunc != NULL) {
      stibm2[63] = (out == ATARI) ?  0x9E : 0xE1;
      cnvrt->ofunc(cnvrt->par, string);
      return(outbuf);
    }
  }
  return(string);
}

/*---------------------------------------------------------------------------*/

static void cvitexfn(char *d, char *string)
{
 char *s;
 int c;

  for (s = string, d = outbuf; *s; s++ ) {
    c = *s;
    if (c == '\\' && *(s+1) == '\"') {
      s++;
      *d++ = *s++;
     }
     else {
      if (c == '\"') {
        s++;
        c = *s;
        switch (c) {
          case 'A': c = 160+36; break;
          case 'O': c = 160+54; break;
          case 'U': c = 160+60; break;
          case 'a': c = 160+68; break;
          case 'o': c = 160+86; break;
          case 's': c = 160+63; break;
          case 'u': c = 160+92; break;
          default:  *d++ = '\"';
        }
      }
      *d++ = c;
    }
  }
  *d = '\0';
}

static void cvitabfn(char *tab, char *string)
{
 char *s, *d, *tp;
 int c, i;

  for (s = string, d = outbuf; *s; s++ ) {
    c = *s;
    for (i = 160, tp = tab; i < 256; i++, tp++) {
      if (*tp == c) {
        c = i;
        break;
      }
    }
    *d++ = c;
  }
  *d = '\0';
}

/*---------------------------------------------------------------------------*/

static int iso2asc(int c)                   /* wegkucken! */
{
  if (c == 160+36) return('A');
  if (c == 160+54) return('O');
  if (c == 160+60) return('U');
  if (c == 160+63) return('s');
  if (c == 160+68) return('a');
  if (c == 160+86) return('o');
  if (c == 160+92) return('u');
  return(c);
}

static void cvodmbfn(char *d, char *string)
{
 char *s, *ep;
 int c;

  for (s = string, d = outbuf, ep = &outbuf[MAXBUF-2];
       *s && (d < ep);
       s++ ) {

    c = *s;
    if (c >= 160) {
      c = iso2asc(c);
      if (c < 160) {
        *d = c;

        if (c != 's')
          c = 'e';

        if (   isupperalpha(s[1])
            || (   (   d == outbuf
                    || (    d > outbuf
                         && isupperalpha(*d)
                         && isupperalpha(*(d-1))))
                && !isalnum(s[1])))
          c = toupper(c);

        if (c == 'S')
          *d = c;

        d++;
      }
    }
    *d++ = c;
  }
  *d = '\0';
}

static void cvotexfn(char *d, char *string)
{
 char *s, *ep;
 int c;

  for (s = string, d = outbuf, ep = &outbuf[MAXBUF-2];
       *s && (d < ep);
       s++ ) {

    c = *s;
    if (c >= 160) {
      c = iso2asc(c);
      if (c < 160)
        *d++ = '\"';
    }
    *d++ = c;
  }
  *d = '\0';
}

static void cvotabfn(char *tab, char *string)
{
 register char *s, *d;
 int c, x;

  for (s = string, d = outbuf; *s; s++ ) {
    c = *s;
    if (c >= 160)
      if ((x = tab[c - 160]) != 0)
       c = x;
    *d++ = c;
  }
  *d = '\0';
}

/*---------------------------------------------------------------------------*/
/* End of $RCSfile$ */
#endif
