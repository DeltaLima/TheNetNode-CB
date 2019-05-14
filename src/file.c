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
/* File src/file.c (maintained by: ???)                                 */
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

extern WORD magic_number;


char confpath[80];

static void fgetpath(char *, FILE *);
static void fgetcall(char *, FILE *);

/*----------------------------------------------------------------------*/
/* Aktuelle Konfiguration speichern.                                    */
/*                                                                      */
/* Rueckgabe:   TRUE:  Abspeichern erfolgreich                          */
/*              FALSE: Fehler beim Abspeichern                          */
/*----------------------------------------------------------------------*/
BOOLEAN save_configuration(void)
{
  FILE *fp;                     /* File-Pointer                         */
  char file[128];               /* Puffer fuer Filenamen                */
  char call[10];

  sprintf(file, "%s%s.PAS", confpath, cfgfile);

  if ((fp = xfopen(file,"wt")) != NULL)      /* File oeffnen            */
  {
                        /* Configuration in einem Block speichern       */
    fprintf(fp,"; TheNetNode Configuration File\n"
               ";\n"
               "; DO NOT CHANGE THE ORDER OF THE CONFIGURATION LINES !\n"
               "; DO NOT CLEAR ANY LINES !\n"
               ";\n"
               "; NET/ROM-Sysop-Password, 80 Characters (01234567890123...)\n"
               "%s\n", paswrd);
    fprintf(fp,";\n"
               "; Console Password\n"
               "%s\n", pass);
    fprintf(fp,";\n"
               "; Node Ident (Test)\n"
               "%s\n", alias);
    call2str(call,myid);
    fprintf(fp,";\n"
               "; Node MyCall (XX0XX)\n"
               "%s\n", call);
    fprintf(fp,";\n"
               "; Workpath, Path to the Help-Files (" TEXTPATH ")\n"
               "; TNN should be started from this path.\n"
               "%s\n", textpath);
    fprintf(fp,";\n"
               "; Path to the ""executable"" Text-Files (" TEXTCMDPATH ")\n"
               "%s\n", textcmdpath);
    fprintf(fp,";\n"
               "; Path to the extern Programs for User (" USEREXEPATH ")\n"
               "%s\n", userexepath);
    fprintf(fp,";\n"
               "; Path to the extern Programs only for Sysop (" SYSEXEPATH ")\n"
               "%s\n", sysopexepath);
#ifdef PACSAT
    fprintf(fp,";\n"
               "; Path to the PACSAT-Files (" PACSATPATH ")\n"
               "%s\n", pacsatpath);
#endif
#ifdef AXIPR_HTML
    fprintf(fp,";\n"
               "; Path fuer HTML (""/usr/local/httpd/htdocs/"")\n"
               "%s\n", htmlpath);
#endif
    fclose(fp);                 /* File schliessen                      */
    return(TRUE);
  }
  else

  return(FALSE);
}

typedef struct {
  void  *ptr;                   /* Zeiger auf den Wert                  */
  size_t size;                  /* Groesse des Bereiches                */
} STATAB;

STATAB statab[] =               /* Werte die in der STA stehen          */
{ {mh,               sizeof(STAT)      * MAXSTAT },
  {portstat,         sizeof(PORTSTAT)  * L2PNUM  },
  {&clear_time,      sizeof(time_t)              },
  {NULL,             0                           }
};

/*----------------------------------------------------------------------*/
/* Aktuelle Statistik-Daten speichern.                                  */
/*                                                                      */
/* Rueckgabe:   TRUE:  Abspeichern erfolgreich                          */
/*              FALSE: Fehler beim Abspeichern                          */
/*----------------------------------------------------------------------*/
BOOLEAN save_stat(void)
{
  FILE *fp;                     /* File-Pointer                         */
  BOOLEAN ret = TRUE;           /* Return-Wert                          */
  char file[128];               /* Puffer fuer Filenamen                */
  STATAB *st;

  sprintf(file, "%s%s.STA", confpath, cfgfile);

  if ((fp = xfopen(file,"wb")) != NULL)             /* File oeffnen     */
  {
    for (st = statab; st->ptr; st++)
      if (fwrite(st->ptr, st->size, 1, fp) != 1) {
        ret = FALSE;
        break;
      }

    if (ret == FALSE)
        xprintf("*** %s.STA write error ***\n",cfgfile);
    fclose(fp);                 /* File schliessen                      */
    return(ret);
  }
  xprintf("*** %s.STA open error ***\n",cfgfile);
  return(FALSE);
}

void fgetstr(char *str, int n, FILE *fp)
{
  char line[200+1], *c;
  int  i = min(n, 200);

  do if (fgets(line, 200, fp) == NULL) break;
  while (*line == ';');

  if ((c = strchr(line, '\n')) == NULL)
    c = &line[i];
  *c = 0;
  strcpy(str, line);
}

static void fgetpath(char *path, FILE *fp)
{
  char line[MAXPATH+1];

  fgetstr(line, MAXPATH, fp);
  sscanf(line, "%s", path);
  addslash(path);
}

static void fgetcall(char *id, FILE *fp)
{
  char str[15];
  char *callp;
  WORD cnt;

  fgetstr(str, 12, fp);
  cnt = (WORD)strlen(str);
  callp = (char *)&str;
  getcal(&cnt, &callp, TRUE, id);
}

/*----------------------------------------------------------------------*/
/* Aktuelle Konfiguration laden.                                        */
/*                                                                      */
/* Rueckgabe:   TRUE bei erfolgreichem Einladen                         */
/*              FALSE bei Fehler                                        */
/*----------------------------------------------------------------------*/
BOOLEAN
load_configuration(void)
{
  FILE   *fp;
  char    line[200];
#ifdef AUTO_UPDATE
  FILE   *ofp;
  int     c;
  BOOLEAN save_new = FALSE;
#endif

  sprintf(line, "%s%s.PAS", confpath, cfgfile);
  fp = xfopen(line,"rt");       /* erstmal neue PAS versuchen           */
#ifdef AUTO_UPDATE
  if (fp == NULL)               /* existiert nicht?, also alte Version  */
  {                             /* lesen und neue Version schreiben     */
    save_new = TRUE;
    sprintf(line, "%s%s.PAS", confpath, oldcfgfile);
    fp = xfopen(line,"rt");
  }
#endif
  if (fp != NULL)                               /* PAS File gefunden    */
  {
    fgetstr(line, 120, fp);                     /* Main Password        */
    paswle = (UWORD)strlen(line);

    if (paswle > 5) {
      paswle = min(paswle, 80);
      xprintf("--- Length of Password: %d\n", paswle);
      strncpy(paswrd, line, paswle);
    } else
        xprintf("*** WARNING: Password rejected (too short)\n");

    fgetstr(pass, 80, fp);                          /* login password   */

    fgetstr(alias, L2CALEN, fp);                    /* ALIAS Node-ID    */
    strncat(alias, "     ", L2CALEN-strlen(alias));

    fgetcall(myid, fp);                             /* Node callsign    */
    cpyid(hostid, myid);

    fgetpath(textpath, fp);                         /* Text Path        */

#ifdef CONFPATHFIX
    strcpy(confpath,textpath);        /* textpath -> confpath zuweisen. */
#endif /* CONFPATHFIX */

    fgetpath(textcmdpath, fp);                      /* TextCommandPath  */
    fgetpath(userexepath, fp);                      /* UserEXE-Path     */
    fgetpath(sysopexepath, fp);                     /* SysopEXE-Path    */
#ifdef PACSAT                                       /* PACSAT-Path      */
    fgetpath(pacsatpath, fp);
#endif
#ifdef AXIPR_HTML                                   /* HTML-Path        */
    fgetpath(htmlpath, fp);
#endif

#ifdef MSGPATHFIX
    strcpy(msgpath,textpath);
    strcat(msgpath,"msg");
    addslash(msgpath);
#endif /* MSGPATHFIX */

    fclose(fp);                                     /* Datei schliessen */
#ifdef AUTO_UPDATE
    if (save_new)
      save_configuration();   /* neue PAS schreiben, alte Konfiguration */

    sprintf(line, "%s%s.TNB", textpath, cfgfile);   /* neue TNB suchen  */
    ofp = xfopen(line,"rt");
    if (ofp != NULL)            /* neue TNB existiert schon - so lassen */
      fclose(ofp);
    else                        /* neue TNB existiert noch nicht        */
    {
      ofp = xfopen(line,"wt");  /* neue TNB wird geschrieben            */
      if (ofp != NULL)          /* File offen                           */
      {
        sprintf(line, "%s%s.TNB", textpath, oldcfgfile);  /* alte TNB   */
        fp = xfopen(line,"rt");
        if (fp == NULL)         /* keine alte TNB gefunden!             */
          fclose(ofp);
        else
        {
          LOOP                  /* ok, TNB kopieren                     */
          {
            c = fgetc(fp);
            if (c == EOF)
              break;
            fputc(c, ofp);
          }
          fclose(fp);
          fclose(ofp);
        }
      }
    }
#endif
    return(TRUE);
  }
  save_configuration();       /* PAS ganz neu erzeugen                  */

  return(FALSE);
}

/*----------------------------------------------------------------------*/
/* Aktuelle Statistik-Daten laden.                                      */
/*                                                                      */
/* Rueckgabe:   TRUE bei erfolgreichem Einladen                         */
/*              FALSE bei Fehler                                        */
/*----------------------------------------------------------------------*/
BOOLEAN load_stat(void)
{
  FILE *fp;             /* File-Pointer                                 */
  LONG filelength;      /* Laenge von CONFIG.NET                        */
  BOOLEAN ret = TRUE;   /* Returnwert                                   */
  char file[MAXPATH+1]; /* Puffer fuer Filenamen                        */
  STATAB *st;
  long l = 0;

  STAT     *statp;

  sprintf(file, "%s%s.STA", confpath, cfgfile);

  if ((fp = xfopen(file,"rb")) != NULL)             /* File oeffnen     */
  {
    for (st = statab; st->ptr; st++)
      l += st->size;

    fseek(fp, 0L, SEEK_END);            /* Dateilaenge ermitteln        */
    filelength = ftell(fp);
    rewind(fp);

    if (filelength != (LONG) l) {       /* Stimmt Dateilaenge?          */
      ret = FALSE;
      xprintf("*** %s.STA has wrong size ***\n", cfgfile);
    }
    else {                              /* Laenge stimmt                */
    for (st = statab; st->ptr; st++)
      if (fread(st->ptr, st->size, 1, fp) != 1) {
        ret = FALSE;
        break;
      }
    }
    for (statp = mh; statp < mh + MAXSTAT; statp++)
      if (statp->call[0] == ' ')
        statp->call[0] = '\0';
    fclose(fp);                         /* Datei schliessen             */
    return(ret);
  }
                        /* TNN???.STA konnte nicht geoeffnet werden     */
  xprintf("*** %s.STA - open error ***\n", cfgfile);
  return(FALSE);
}

/*----------------------------------------------------------------------*/
/*                                                                      */
/* An einen Verzeichnisnamen das letzte Slash anfuegen. Es wird das     */
/* fuer das jeweilige Dateisystem passende Symbol genommen. Der Name    */
/* wird gleichzeitig normiert.                                          */
/*                                                                      */
/*----------------------------------------------------------------------*/
void addslash(char *path)
{
  char *p = path;

  normfname(path);                  /* den Namen normieren              */

  /* jetzt koennen wir einfach FILE_SEP verwenden, da normfname()       */
  /* einen abweichenden Syntax bereits korrigiert hat.                  */
  if (!*path || *((p = path + strlen(path)) - 1) != FILE_SEP) {
    *p++ = FILE_SEP;
    *p   = '\0';
  }
}

/**************************************************************************/
/* Sonst kriegt DOS die Panik ...                                         */
/*------------------------------------------------------------------------*/
#ifdef PC
BOOLEAN good_file_name(const char *file)
 {
  if ( (strnicmp(file, "CON", 3) == 0)
    || (strnicmp(file, "COM1", 4) == 0)
    || (strnicmp(file, "COM2", 4) == 0)
    || (strnicmp(file, "COM3", 4) == 0)
    || (strnicmp(file, "COM4", 4) == 0)
    || (strnicmp(file, "LPT1", 4) == 0)
    || (strnicmp(file, "LPT2", 4) == 0)
    || (strnicmp(file, "LPT3", 4) == 0)
    || (strnicmp(file, "LPT4", 4) == 0)
    || (strnicmp(file, "AUX", 3) == 0)
    || (strnicmp(file, "PRN", 3) == 0)
    || (strnicmp(file, "NUL", 3) == 0)
    || (strnicmp(file, "CLOCK$", 6) == 0)
      )
    return(FALSE);
  else return(TRUE);
 }
#endif

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Einen Dateinamen normieren, d.h. er wird den Gegenheiten des         */
/* verwendeten Dateisystems angepasst. Die Informationen hierfuer       */
/* werden in ALL.H festgelegt.                                          */
/*                                                                      */
/*----------------------------------------------------------------------*/
char *normfname(char *filename)
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
/* Wechelt das aktuelle Arbeitsverzeichniss. Der Name wird normiert.    */
/*                                                                      */
/*----------------------------------------------------------------------*/
#ifndef MC68302
int xchdir(const char *path)
{
   char tmppath[MAXPATH+1];
   static char oldp[MAXPATH+1];
#ifndef NO_DISKDRIVE
   static WORD odrv;
#endif

   if (path) {
        strcpy(tmppath, path);
        normfname(tmppath);
        if (tmppath[strlen(tmppath)-1] == FILE_SEP)
          tmppath[strlen(tmppath)-1] = '\0';
#ifndef NO_DISKDRIVE
        odrv = getdisk();
        if (*(tmppath+1) == ':')
            setdisk((*tmppath & 0xDF) - 'A');
        getcwd(oldp, MAXPATH);
#endif
        chdir(tmppath);
    }
    else {
        chdir(oldp);
#ifndef NO_DISKDRIVE
        setdisk(odrv);
#endif
    }
    return(0);
}
#endif

#ifndef MC68K                      /* der ATARI kocht da anders         */
/*----------------------------------------------------------------------*/
/*                                                                      */
/* Feststellen, ob eine Datei existiert.                                */
/*                                                                      */
/*----------------------------------------------------------------------*/
int xaccess(const char *filename, int amode)
{
 char fname[MAXPATH+1];

  strcpy(fname, filename);         /* umkopieren, den Originalnamen     */
  normfname(fname);                /* nicht anfassen                    */
  return(access(fname, amode));
}
#else
int xaccess(const char *filename, int amode)
{
 struct ffblk ffblk;
 char fname[MAXPATH+1];

  strcpy(fname, filename);         /* umkopieren, den Originalnamen     */
  normfname(fname);                /* nicht anfassen                    */
  return(findfirst(fname, &ffblk, 0));
}
#endif

/* wird in os/linux/linux.c erledigt */
#if !defined(__LINUX__) && !defined(__WIN32__)
/*----------------------------------------------------------------------*/
/*                                                                      */
/* Die Suche nach einer bestimmten Datei beginnen. Fortsetzen mit       */
/* xfindnext().                                                         */
/*                                                                      */
/*----------------------------------------------------------------------*/
int xfindfirst(const char *pathname, struct ffblk *ffblk, int attrib)
{
 char fname[MAXPATH+1];

  strcpy(fname, pathname);         /* umkopieren, den Originalnamen     */
  normfname(fname);                /* nicht anfassen                    */
  return(findfirst(pathname, ffblk, attrib));
}

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Die Suche nach einer bestimmten Datei fortsetzen.                    */
/*                                                                      */
/*----------------------------------------------------------------------*/
int xfindnext(struct ffblk *ffblk)
{
  return(findnext(ffblk));
}

#endif /* WIN32 */

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Eine Datei oeffnen, es wird darauf geachtet, dass einige Betriebs-   */
/* systeme zwischen Textdateien und Binaerdateien unterscheiden.        */
/* normfname() wird zur Normierung des Dateinamens benutzt.             */
/*                                                                      */
/*----------------------------------------------------------------------*/
FILE *xfopen(const char *filename, const char *mode)
{
  char fmode[4], *fm;              /* der angepasste Datei-Mode         */
  char fname[MAXPATH+1];

#ifndef MC68K
  if (!good_file_name(filename))
    return(NULL);
#endif

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

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Eine Datei loeschen, es werden ALLE Dateien geloescht (auch RDONLY)  */
/* normfname() wird zur Normierung des Dateinamen benutzt.              */
/*                                                                      */
/*----------------------------------------------------------------------*/
int xremove(const char *filename)
{
 char fname[MAXPATH+1];

  strcpy(fname, filename);         /* umkopieren, den Originalnamen     */
  normfname(fname);                /* nicht anfassen                    */
  return(remove(fname));
}

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Eine Datei umbenennen.                                               */
/* normfname() wird zur Normierung des Dateinamens benutzt.             */
/*                                                                      */
/*----------------------------------------------------------------------*/
int xrename(const char *oldname, const char *newname)
{
 char fname[MAXPATH+1];
 char fname2[MAXPATH+1];

  strcpy(fname, oldname);         /* umkopieren, den Originalnamen     */
  normfname(fname);               /* nicht anfassen                    */
  strcpy(fname2, newname);        /* umkopieren, den neuen Namen       */
  normfname(fname2);              /* nicht anfassen                    */
  return(rename(fname,fname2));
}

/* End of src/file.c */
