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
/* File contrib/onlhelp/oh.c (maintained by: DL1XAO)                    */
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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#ifdef WIN32
#define MODI "(01.08.04) Modifiziert fuer Win32 von DAA531 Oliver Kern."
#endif

#define VERSION "1.1"
#define VDATUM "Mar 01 2000"

char *helpfn;
char *idxfn;
char  progname[128];

#if defined(WIN32) && defined(_MSC_VER)
int strcasecmp(const char *Sstr, const char *Dstr)
{
  char _Sstr[255];
  char _Dstr[255];

  if (  (Sstr == NULL)
      ||(Dstr == NULL))
    return(1);

  strcpy(_Sstr, Sstr);
  strcpy(_Dstr, Dstr);
  strupr(_Sstr);
  strupr(_Dstr);

  if (strcmp(_Sstr, _Dstr) == 0)
    return(0);
  else
    return(1);
}

int strncasecmp(const char *Sstr, const char *Dstr, int len)
{
  char _Sstr[255];
  char _Dstr[255];

  if (  (Sstr == NULL)
      ||(Dstr == NULL))
    return(1);

  strcpy(_Sstr, Sstr);
  strcpy(_Dstr, Dstr);
  strupr(_Sstr);
  strupr(_Dstr);

  if (strncmp(_Sstr, _Dstr, len) == 0)
    return(0);
  else
    return(1);
}

#endif /* WIN32 */


int gettopic(char *in, char *topic) /* prueft auf Zeile mit Befehl*/
{
  if (*in == '(' && strchr(in, ')')) {
    sscanf(in, "%s", topic);
    if (strstr(in, "externer Befehl"))
      return 2;
    return 1;
  }
  return 0;
}

void getcmd(char *cmd) /* liest den Befehl ohne Klammern */
{
 char *cp = cmd;

  if (*cmd == '(')
    cmd++;

  while (*cmd && *cmd != ')')
    *cp++ = *cmd++;

  if (*cmd == ')')
    cmd++;

  while (*cmd)
    *cp++ = *cmd++;

  *cp = '\0';
}

int genindex(void) /* legt Index-File an */
{
 FILE *fpt, *fpi;
 char in[128], c_cmd[32], tmp[32];
#ifdef WIN32
 int state,
     i,
     lines = 0;
#else
 int state, i, lines;
#endif /* WIN32 */

 long pos;

  if ((fpt = fopen(helpfn, "r")) == NULL) {
    puts("Konnte Hilfedatei nicht oeffnen.");
    return 0;
  }
  if ((fpi = fopen(idxfn, "w")) == NULL) {
    puts("Konnte Index nicht anlegen.");
    fclose(fpt);
    return 0;
  }

  state = 1;
  pos = 0;
  while (fgets(in, 120, fpt) != NULL) {
    if (state == 1) {              /* fetch version */
      if (!strncmp(in, "TNN-Doku-Version", 16)) {
        fprintf(fpi, "%s", in);
        state++;
      }
    }
    else if (state == 2) {         /* fetch first topic */
      if ((i = gettopic(in, tmp)) != 0) {
        fprintf(fpi, "%s %d %ld ", tmp, i - 1, pos);
        strcpy(c_cmd, tmp);
        lines = 1;
        state++;
      }
    }
    else if (state == 3) {         /* fetch next topics */
      if (   (i = gettopic(in, tmp)) != 0
          && strcasecmp(c_cmd, tmp)) {
        fprintf(fpi, "%d\n%s %d %ld ", lines, tmp, i - 1, pos);
        strcpy(c_cmd, tmp);
        lines = 1;
      }
      else
        lines++;
    }
    pos = ftell(fpt);
  }

  fprintf(fpi, "%d\n", lines);

  fclose(fpt);
  fclose(fpi);

  return 1;
}

int checkindex(void)  /* prueft, ob Indexfile neu angelegt werden muss */
{
 struct stat statbuf1, statbuf2;

  if (stat(helpfn, &statbuf1)) {
    puts("Hilfe-Datei fehlt. Bitte Sysop verstaendigen !");
    return 0;
  }
  if (stat(idxfn, &statbuf2))
    statbuf2.st_ctime = 0;

  if (statbuf1.st_ctime > statbuf2.st_ctime) {
    puts("Generiere Index.");
    return genindex();
  }

  return 1;
}

void overview(void)  /* Alle Befehle in der Uebersicht */
{
 FILE *fpi;
 char in[128], cmd[32];
 int left, ext, lines;
 long pos;

  if ((fpi = fopen(idxfn, "r")) != NULL) {
    puts("Programm-"VERSION" vom "VDATUM" by DL1XAO");
    fgets(in, 100, fpi);
    printf("%s", in);
#ifdef WIN32
        puts(""MODI"\n");
#endif
    puts("\nFolgende Befehle sind laut Dokumentation verfuegbar :");
    puts("\nBefehl              Hilfe-Seiten         Befehl              Hilfe-Seiten");

    left = 1;
    while (fgets(in, 120, fpi) != NULL) {
      sscanf(in, "%s %d %ld %d\n", cmd, &ext, &pos, &lines);
      lines = (lines + 18) / 20;

      printf("%-15s%s", cmd, ext ? "(extern) " : "         ");

      if (left)
        printf("%2d               ", lines);
       else
        printf("%2d\n", lines);
      left = 1 - left;           /* Seitenwechsel */

    }
    puts("\nExterne Befehle sind nicht bei jedem Digi vorhanden !");
    fclose(fpi);
  }
}

/* Topic im Index suchen */
int findtopic(char *topic, char *fcmd, long *pos, int *lines)
{
 FILE *fpi;
 char in[128], cmd[32];
 int  ext;

  if ((fpi = fopen(idxfn, "r")) != NULL) {
    fgets(in, 100, fpi);

    while (fgets(in, 120, fpi) != NULL) {
      sscanf(in, "%s %d %ld %d\n", cmd, &ext, pos, lines);
      strcpy(fcmd, cmd);
      getcmd(cmd);
      if (!strncasecmp(topic, cmd, strlen(topic))) {
        fclose(fpi);
        return 1;
      }
    }
    fclose(fpi);
  }
  return 0;
}

void showhelp(char *topic, int startpage) /* Hilfe zu topic anzeigen */
{
 FILE *fpt;
 char in[128], cmd[32] ,*cp;
 int  lines, pages, n;
 long pos;

  if (!findtopic(topic, cmd, &pos, &lines)) {
    printf("Keine Hilfe zu %s gefunden.\n", topic);
    return;
  }

  pages = (lines + 18) / 20;

  if (startpage > pages) {
    printf("Keine Seite %d zu %s vorhanden.\n", startpage, topic);
    return;
  }

  if ((fpt = fopen(helpfn, "r")) != NULL) {
    fseek(fpt, pos, SEEK_SET);

    if (startpage == -1) {               /* display all */
      while (lines && fgets(in, 120, fpt) != NULL) {
        printf("%s", in);
                lines--;
      }
    }
    else {                               /* show one page */
      fgets(in, 120, fpt);
      lines--;
      if ((cp = strchr(in, '\n')) != NULL)
        *cp = '\0';
      printf("%-70.70s Seite %d\n", in, startpage);

      n = (startpage - 1) * 20;            /* skip n lines */
      while (n-- && fgets(in, 120, fpt) != NULL)
        lines--;

      n = (lines > 20) ? 20 : lines;
      while (n-- && fgets(in, 120, fpt) != NULL) { /* show page */
        printf("%s", in);
        lines--;
      }

    if (lines)
      printf("Weiter mit \'%s %s %d\'\n", progname, topic, startpage + 1);
     else
      printf("Ende Hilfe zu %s!\n", cmd);
    }

    fclose(fpt);
  }
}

void sysopcheck(char *pname)   /* sind wir SYSHELP ? */
{

 char *cp;

  cp = strrchr(pname, '/');
  if (!cp)
    cp = strrchr(pname, '\\');
  if (!cp)
    cp = pname  - 1;

  strcpy(progname, cp + 1);

  if ((cp = strchr(progname, '.')) != NULL)
    *cp = '\0';

  if (!strncasecmp(progname, "SYSH", 4)) {

    helpfn = "ohs.txt";
    idxfn = "ohs.idx";
  }
  else {
    helpfn = "ohu.txt";
    idxfn = "ohu.idx";
  }
}

int main(int argc, char **argv)
{
 char topic[32];
 int start;

  puts("\nHilfe fuer TNN");

  sysopcheck(argv[0]);

  if (checkindex()) {

    if (argc < 2) {
      puts("Zuwenig Parameter angegeben !");
    }
    else if (argc == 2) { /* oh dl1xao */
      overview();
    }
    else if (argc == 3 || argc == 4) { /* oh st [2|*] dl1xao */
      strncpy(topic, argv[1], 30);
      topic[30] ='\0';
      getcmd(topic);
      start = 1;
      if (argc == 4) {
        if (*argv[2] == '*')
          start = -1;
        else {
          start = atoi(argv[2]);
          if (start < 1 || start > 20) {
            puts("Nummer der Folgeseite muss zwischen 1 und 20 liegen !");
            return 1;
          }
        }
      }
      showhelp(argv[1], start);
    }
    else {
      puts("Zuviele Parameter angegeben !");
    }
  }
  return 0;
}
