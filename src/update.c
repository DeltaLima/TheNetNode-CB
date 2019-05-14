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
/* File src/update.c (maintained by: DF6LN)                             */
/*                                                                      */
/* This file is part of "TheNetNode" - Software Package                 */
/*                                                                      */
/* Copyright (C) 1998 - 2003 NORD><LINK e.V. Braunschweig               */
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

char  infile[20];
char  outfile[20];
FILE *ifd;
FILE *ofd;
int   oldver = 0;

void  open_files(const char *);
char *normfname(char *);
FILE *xfopen(const char *, const char *);
void  do_parms(const char *);
void  open_files(const char *);
void  copy(char *);
int   main(int, char *[]);
#ifdef __LINUX__
char *strlwr(char *);

/*
   String in Kleinbuchstaben umwandeln - aus DJGPP GNU-Paket fuer MS-DOS
   von D.J. Delorie
*/
char *
strlwr(char *s)
{
  char *p = s;

  while (*s)
  {
    if ((*s >= 'A') && (*s <= 'Z'))
      *s += 'a'-'A';
    s++;
  }
  return(p);
}
#endif

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Einen Dateinamen normieren, d.h. er wird den Gegenheiten des         */
/* verwendeten Dateisystems angepasst. Die Informationen hierfuer       */
/* werden in ALL.H festgelegt.                                          */
/*                                                                      */
/*----------------------------------------------------------------------*/
char *
normfname(char *filename)
{
  char *s;                         /* Zeiger innerhalb des Namens       */

  for (s = filename; *s; s++)      /* alle Zeichen im String durchgehen */
    if (strchr(SEPARATORS, *s))    /* ein Dateitrennungszeichen ?       */
      *s = FILE_SEP;               /* dann durch das richtige ersetzen  */
#if (FILE_FLAGS & FF_LWR)
  strlwr(filename);                /* eventuell in Kleinschreibung...   */
#endif
  return(filename);
}

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Eine Datei oeffnen, es wird darauf geachtet, dass einige Betriebs-   */
/* systeme zwischen Textdateien und Binaerdateien unterscheiden.        */
/* normfname() wird zur Normierung des Dateinamens benutzt.             */
/*                                                                      */
/*----------------------------------------------------------------------*/
FILE *
xfopen(const char *filename, const char *mode)
{
  char fmode[4], *fm;              /* der angepasste Datei-Mode         */
  char fname[MAXPATH+1];

  strcpy(fname, filename);         /* umkopieren, den Originalnamen     */
  normfname(fname);                /* nicht anfassen                    */

  fm = fmode;
  if (strchr(mode, 'w'))           /* zum Schreiben                     */
    *fm++ = 'w';
  else if (strchr(mode, 'a'))      /* zum Anhaengen                     */
    *fm++ = 'a';
  else
    *fm++ = 'r';                   /* sonst zum Lesen oeffnen           */
#if (FILE_FLAGS & FF_TXT)          /* b/t-Flag uebernehmen?             */
  if (strchr(mode, 'b'))           /* eine Binaer-Datei?                */
    *fm++ = 'b';                   /* ... Flag uebernehmen              */
#ifndef MC68K
  if (strchr(mode, 't'))           /* eine Text-Datei?                  */
    *fm++ = 't';                   /* ... Flag uebernehmen              */
#endif
#endif
  if (strchr(mode, '+'))           /* Ueberschreiben?                   */
    *fm++ = '+';

  *fm = NUL;                       /* String terminieren                */
  return(fopen(fname, fmode));     /* Datei oeffnen                     */
}

/************************************************************************/
/*                                                                      */
/* Parameter-Befehl bearbeiten bei Vorgaengerversion 1.76:              */
/*                                                                      */
/* Par 2 war frueher Timeout und ist nun L3-Maxtime. Wenn als Zahl      */
/* oder mit Namen Timeout angegeben, wird die Zeile auskommentiert.     */
/* Par 10 war frueher Downport und ist entfallen. Zeile wird aus-       */
/* kommentiert.                                                         */
/* Par 11 und 12 haben jetzt eine neue Nummer, weil Par 10 entfallen    */
/* ist. Mit Namen ausgeben.                                             */
/*                                                                      */
/************************************************************************/
void
do_parms(const char *str)
{
  char *s1;
  char *s2;
  char *s3;
  int   i;

  s1 = strchr(str, ' ');        /* Leerzeichen nach Befehl              */
  if (!s1)                      /* keine Parameter angegeben            */
  {
    fputs(str, ofd);
    return;
  }
  s1++;                                 /* 1. Parameter                 */
  if (sscanf(s1, "%d", &i) != 1)        /* Parameter als Nummer?        */
  {
    i = -1;
    if (strnicmp(s1, "timeout", min(strlen(s1), 7)) == 0)
      i = 2;
    if (strnicmp(s1, "downport", min(strlen(s1), 8)) == 0)
      i = 10;
  }
  switch (i)
  {
    case 11:
      s2 = "TestSSID";
      s3 = strchr(s1, ' ');
      fprintf(ofd, "PAR %s%s", s2, s3);
      return;
    case 12:
      s2 = "ConvSSID";
      s3 = strchr(s1, ' ');
      fprintf(ofd, "PAR %s%s", s2, s3);
      return;
    case 2:
    case 10:
      fputc(';', ofd);
      break;
  }
  fputs(str, ofd);
}

void
open_files(const char *ext)
{
#ifdef __LINUX__
  struct stat istat;
#endif

  sprintf(outfile, "tnn178.%s", ext);
  if (oldver != 176)
  {
    sprintf(infile, "tnn177.%s", ext);
    ifd = xfopen(infile, "rt");
    if (oldver == 177 && ifd == NULL)
    {
      printf("Input file %s not found!\n", infile);
      exit(1);
    }
    oldver = 177;
  }
  if (oldver != 177)
  {
    sprintf(infile, "tnn176.%s", ext);
    ifd = xfopen(infile, "rt");
    if (ifd == NULL)
    {
      printf("Input file %s not found!\n", infile);
      exit(1);
    }
    oldver = 176;
  }
  ofd = xfopen(outfile, "rt");
  if (ofd != NULL)
  {
    printf("Output file %s exists!\n", outfile);
    fclose(ifd);
    fclose(ofd);
    exit(1);
  }
  ofd = xfopen(outfile, "wt");
  if (ofd == NULL)
  {
    printf("Open error %s\n", outfile);
    fclose(ifd);
    exit(1);
  }
#ifdef __LINUX__
  if (fstat(fileno(ifd), &istat) == 0)
    fchmod(fileno(ofd), istat.st_mode);
#endif  
}

void
copy(char *ext)
{
  int c;

  open_files(ext);
  LOOP
  {
    c = fgetc(ifd);
    if (c == EOF)
      break;
    fputc(c, ofd);
  }
  fclose(ifd);
  fclose(ofd);
}

int
main(int argc, char *argv[])
{
  char str[256];
  char str2[256];

  copy("pas");
  copy("sta");
  open_files("tnb");
  while (!feof(ifd))
  {
    if (!fgets(str, 255, ifd))
      break;
    sscanf(str, "%s", str2);
    if (oldver == 176)
    {
      if (strnicmp(str2, "PARMS", min(strlen(str2), 5)) == 0)
      {
        do_parms(str);
        continue;
      }
    }
    fputs(str, ofd);
  }
  fclose(ifd);
  fclose(ofd);
  return(0);
}
