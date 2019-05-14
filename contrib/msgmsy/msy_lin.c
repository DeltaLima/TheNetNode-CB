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
/* File msy_lin.c (maintained by: DG8BR)                                */
/*                                                                      */
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef __MSDOS__
  #include <dir.h>
#endif

#ifdef __GO32__
  #include <unistd.h>
#endif

#ifdef __linux__
  #include <errno.h>
  #include <unistd.h>
  #include <fnmatch.h>
  #include <dirent.h>
  #include <sys/stat.h>
#endif

#ifdef WIN32
#ifdef _MSC_VER
/* Wird nur im VC benoetig. */
#include <direct.h>
#endif /* WIN32 */
#include <sys/stat.h>
#include <errno.h>
#include "sys/dirent32.h"
#include "sys/fnmatch.h"
#endif /* WIN32 */

/* Definitionen */
#define MaxZeilen       200
#define MaxLT           99
#define FALSE           0
#define TRUE            (!FALSE)
#define DefaultLT       " 7"
#define MAXGROUP        50
#ifdef __MSDOS__
  #define Seperator '\\'
  #define xfindfirst findfirst
  #define xfindnext findnext
#endif

#ifdef WIN32
  #define MAXPATH 255
  #define NAME_MAX 260
  #define findfirst _findfirst
  #define findnext  _findnext
  #define Seperator '\\'
#endif /* WIN32 */

#ifdef __linux__
  #define Seperator '/'
  #define MAXPATH         128          /* ist unter TC3.1 auf 80 festgelegt */
  #define stricmp strcasecmp
#endif

/* Globale Variablen */
int Status;                             /* True, wenn procedure erfolgreich */
int Gruppe;                                       /* True, wenn Call=Gruppe */
char Usercall[10];                              /* Aufrufender des MSG-pgms */
char msgpath[MAXPATH];
char Nachricht[MaxZeilen][80];      /* wird von mehreren Funktionen genutzt */
#ifndef WIN32
struct ffblk ffblk;
#endif /* WIN32 */
struct ZEILE{                                                /* fuer DirMsy */
              char From[7];
              char To[7];
              char Date[14];
              char Time[7];
              int  LT;
              char Return[7];
              int  Byte;
            };

/* Prototypen */
void CheckMsy (void);
void DirMsy (int, char**);
void ListMsy (int, char**);
void SendMsy ( int, char** );
void GetMsy (void);
void GroupMsy ( int, char** );
void GroupList ( char* );
void GroupSys ( char** );
void HelpMsy (void);
void VerMsy (void);

int Kopie (int, char, struct ZEILE *, char, char*, int, char);
int Calltest ( char* );
int speichern ( char*, char*, char* );         /* Unterfunktion von SendMsg */
void Stop (char* );

/* Alles unter dem nachfolgenden ifdef __linux__ wurde aus dem TNN-Packet   */
/* fuer LinuX von der NORD><LINK - Gruppe bzw. aus DJGPP GNU-Paket fuer     */
/* DOS von D.J. Delorie entnommen.  */

#if defined(__linux__) || defined(WIN32)
#ifndef WIN32
int xfindfirst (const char*, struct ffblk *, int);
int xfindnext (struct ffblk *);
void setfiletime (struct ffblk * );
#endif /* WIN32 */
char* strupr (char*);
char* strlwr (char*);
char* itoa (int, char*, int);

struct ffblk
{
  char     ff_path[MAXPATH];
  char     ff_find[NAME_MAX];
  char     ff_name[NAME_MAX];
  unsigned ff_fdate;
  unsigned ff_ftime;
};

#ifdef WIN32
struct ffblk ffblk;

int xfindfirst (const char*, struct ffblk *, int);
int xfindnext (struct ffblk *);
void setfiletime (struct ffblk * );
#endif /* WIN32 */

struct ftime
{
  unsigned ft_tsec      : 5;
  unsigned ft_min       : 6;
  unsigned ft_hour      : 5;
  unsigned ft_day       : 5;
  unsigned ft_month     : 4;
  unsigned ft_year      : 7;
};

/************************************************************************/
/*                                                                      */
/* Als Ersatz fuer die Funktionen "findfirst" und "findnext" (bei DOS)  */
/* werden die Funktionen "xfindfirst" und "xfindnext" verwendet. Hier   */
/* wird nur der fuer TNN benoetigte Teil in die Struktur ffblk einge-   */
/* tragen. Der zuletzt gefundene Filename muss in ffblk->ff_name erhal- */
/* ten bleiben, weil dieser beim Aufruf von "xfindnext" gesucht wird.   */
/*                                                                      */
/************************************************************************/

int xfindfirst(const char *pathname, struct ffblk *ffblk, int attrib)
{
  char          *fnpoi;
  DIR           *dp;
  struct dirent *dirp;
  int           retval;

  strcpy(ffblk->ff_path, pathname);                 /* Filename incl. Pfad  */
#ifdef WIN32
  fnpoi = strrchr(ffblk->ff_path, Seperator);       /* Pfad angegeben?      */
#else
  fnpoi = strrchr(ffblk->ff_path, '/');             /* Pfad angegeben?      */
#endif /* WIN32 */
  if (fnpoi == NULL)                                /* - nein ..            */
  {
    strcpy(ffblk->ff_find, ffblk->ff_path);         /* Filename kopieren    */
    strcpy(ffblk->ff_path, msgpath);                /* default: msgpath     */
  }
  else                                              /* mit Pfad             */
  {
    if (fnpoi == ffblk->ff_path+strlen(ffblk->ff_path))     /* ohne Name    */
      return(-1);                                           /* Unsinn       */
    strcpy(ffblk->ff_find, ++fnpoi);        /* nur Filename                 */
    *fnpoi = '\0';                          /* Filename im Pfad loeschen    */
  }

  if ((dp = opendir(ffblk->ff_path)) == NULL)     /* Directory vorhanden?   */
    return(-1);
  retval = -1;                              /* default: nix gefunden        */
  while ((dirp = readdir(dp)) != NULL)      /* Eintrag vorhanden?           */
  {
    if ((fnmatch(ffblk->ff_find, dirp->d_name,
                 FNM_PATHNAME|FNM_PERIOD) != 0))
      continue;
    strcpy(ffblk->ff_name, dirp->d_name);
    setfiletime(ffblk);
    retval = 0;
    break;
  }
  closedir(dp);
  return(retval);
}

/****************************************************************************/
/*                                                                          */
/* Erst den zuletzt gefundenen Eintrag suchen (steht in ffblk->ff_name)     */
/* und den darauffolgenden passenden Eintrag zurueckmelden.                 */
/*                                                                          */
/****************************************************************************/

int xfindnext(struct ffblk *ffblk)
{
  DIR           *dp;
  struct dirent *dirp;
  int           retval;

  if ((dp = opendir(ffblk->ff_path)) == NULL)
    return -1;
  retval = -1;                              /* default: nix gefunden        */
  while ((dirp = readdir(dp)) != NULL)
  {
    if ((fnmatch(ffblk->ff_name, dirp->d_name,
                 FNM_PATHNAME|FNM_PERIOD) != 0))
      continue;
    retval = 1;
    break;
  }
  if (retval == 1)
  {
    retval = -1;                              /* default: nix gefunden      */
    while ((dirp = readdir(dp)) != NULL)
    {
      if ((fnmatch(ffblk->ff_find, dirp->d_name,
                   FNM_PATHNAME|FNM_PERIOD) != 0))
        continue;
      strcpy(ffblk->ff_name, dirp->d_name);
      setfiletime(ffblk);
      retval = 0;
      break;
    }
  }
  closedir(dp);
  return(retval);
}

/****************************************************************************/
/*                                                                          */
/* Bei xfindfirst und xfindnext Datum und Uhrzeit im Fileblock setzen       */
/*                                                                          */
/****************************************************************************/

void setfiletime(struct ffblk *ffblk)
{
  struct stat  filestat;
  struct tm   *filetime;
  char         fn[MAXPATH];

  sprintf(fn, "%s%s", ffblk->ff_path, ffblk->ff_name);
  stat(fn, &filestat);
  filetime = gmtime(&filestat.st_mtime);
  ffblk->ff_ftime =   ((filetime->tm_sec / 2) & 0x1f)
                    + ((filetime->tm_min & 0x3f) << 5)
                    + ((filetime->tm_hour & 0x1f) << 11);
  ffblk->ff_fdate =   (filetime->tm_mday & 0x1f)
                    + (((filetime->tm_mon + 1) & 0x0f) << 5)
                    + (((filetime->tm_year - 80) & 0x7f) << 9);
}

/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

char* strlwr(char *_s)
{
  char *rv = _s;
  while (*_s)
  {
    *_s = tolower(*_s);
    _s++;
  }
  return rv;
}

/*
   String in Grossbuchstaben umwandeln - aus DJGPP GNU-Paket fuer DOS
   von D.J. Delorie
*/

char *strupr(char *s)
{
  char *p = s;
  while (*s)
  {
    if ((*s >= 'a') && (*s <= 'z'))
      *s += 'A'-'a';
    s++;
  }
  return p;
}

/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

char *itoa(int value, char *string, int radix)
{
  char tmp[33];
  char *tp = tmp;
  int i;
  unsigned v;
  int sign;
  char *sp;

  if (radix > 36 || radix <= 1)
  {
    errno = EDOM;
    return 0;
  }

  sign = (radix == 10 && value < 0);
  if (sign)
    v = -value;
  else
    v = (unsigned)value;
  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+'0';
    else
      *tp++ = i + 'a' - 10;
  }

  if (string == 0)
    string = (char *)malloc((tp-tmp)+sign+1);
  sp = string;

  if (sign)
    *sp++ = '-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;
  return string;
}

#endif                                                        /* Linux-Teil */

/*----------------------------------------------------------------------------
                         VerMsy.C
----------------------------------------------------------------------------*/
#define MsyVersion      "3.02"
#define MsyDatum        "16.10.1996 von DF7BZ & DG3AAH"
#define MsyLinux        "(20.06.99) nach 'C' portiert von DG8BR"
#ifdef WIN32
#define MsyWin32        "(11.01.04) Modifiziert fuer Win32 von DAA531 Oliver Kern."
#endif

void VerMsy ( void )
{
 #ifdef WIN32
  printf("\nMSG-Programm %s von %s \n%s\n%s\n",MsyVersion, MsyDatum,MsyLinux,MsyWin32);
#else
  printf("\nMSG-Programm %s von %s \n%s\n",MsyVersion, MsyDatum,MsyLinux);
#endif
  Status = TRUE;
}

/*----------------------------------------------------------------------------
                         HelpMsy.C
----------------------------------------------------------------------------*/
void HelpMsy ( void )
{
  char filename[MAXPATH];
  FILE *fp;
  sprintf(filename,"%smsg.sys",msgpath);
  if (( fp = fopen(filename,"rt")) == NULL)
    Stop("Fehler #H1 Konnte Hilfedatei nicht oeffnen.");

  while (!feof(fp))
  {
    if((fgets(filename,79,fp)) != NULL)
      printf("%s",filename);
  }
  fclose (fp);
  Status = TRUE;
}

/*----------------------------------------------------------------------------
                         CheckMsy.C
Funktion prueft anhand des Datums ob es aeltere MSG's gibt. Wenn ja, dann
die Lifetime ermitteln und um einen reduzieren. Bei "0", Empfaenger und
Absender vertauschen und ein RETURN in den Header packen und auf DefaultLT
setzen. Wenn Lifetime "0" und RETURN oder Empfaenger und Absender gleich,
dann loeschen.
----------------------------------------------------------------------------*/
void CheckMsy (void)
{
  int Anzahl = 0;                         /* Anzahl der eingelesenen Zeilen */
  int Nr;                        /* Zeilennummer die gerade bearbeitet wird */
  int LTz;                                               /* Lifetimezaehler */
  int loeschen = 0;
  int RueckNr = 0;              /* Zaehler fuer zurueckzuschreibende Zeilen */
  int gefunden;
  unsigned int Datum;
  char LT[3];
  char Empfaenger[7];
  char Absender[7];
  char dname[MAXPATH];
  time_t t;
  struct tm *ts;
  FILE *fp, *aus;

  time(&t);
  ts = localtime(&t);     /* aktuelles Datum in Form ffblk.ff_fdate bringen */
  Datum = (ts->tm_mday & 0x1f)
          + (((ts->tm_mon + 1) & 0x0f) << 5)
          + (((ts->tm_year - 80) & 0x7f) << 9);

  sprintf(dname,"%s*.msg",msgpath);
  if (( gefunden = xfindfirst(dname,&ffblk,0)) != FALSE)
  {
    printf("Keine MSG-Dateien gefunden.\n");
    exit(0);
  }

  while(!gefunden)
  {
    if(Datum != ffblk.ff_fdate )                 /* nur wenn Unterschied !! */
    {
      sprintf(dname,"%s%s",msgpath,ffblk.ff_name);
      fp = fopen(dname,"rt");
      Anzahl = 0;
      do
      {
        if((fgets(Nachricht[Anzahl],80,fp)) != NULL)
          Anzahl++;
        if ( Anzahl > MaxZeilen )                /* Sollte nie vorkommen !! */
        {
          fclose(fp);  /* nachfolgendes wird nur auf der Konsole ausgegeben */
          printf("\nIn %s sind zuviele Nachrichten !"
                 " Bitte Sysop verstaendigen !\n",ffblk.ff_name);
        }
      }
      while(!feof(fp));
      fclose(fp);            /* Ursprungsdatei schliessen und nun auswerten */
      RueckNr = Anzahl;
  /* Die  Auswertung folgt hier */

      for (Nr = 0;Nr < Anzahl;)
      {
        if(strstr(Nachricht[Nr],"Message from "))
        {
          loeschen = 0;
          strncpy (LT,strstr(Nachricht[Nr],"Lifetime ")+9,2);
          LT[2] = '\0';
          LTz = atoi(LT);
          LTz--;
          if (LTz == 0)
          {
            if(strstr(Nachricht[Nr],"RETURN"))         /* Return schon da ? */
            {
              Nachricht[Nr][0] = '\0';                          /* Loeschen */
              RueckNr--;
              loeschen = 1;
            }
            else
            {
              strncpy (Absender,strstr(Nachricht[Nr],"from ")+5,6);
              strncpy (Empfaenger,ffblk.ff_name,6);
              if (Empfaenger[5] == '.')                  /* Punkt entfernen */
                Empfaenger[5] = ' ';
              Absender[6] = Empfaenger[6] = '\0';      /* Stringende setzen */

              if(!stricmp(Empfaenger,Absender))           /* beide gleich ? */
              {
                Nachricht[Nr][0] = '\0';        /* dann gibt es kein Return */
                RueckNr--;
                loeschen = 1;
              }
              else                              /* nun beide Calls tauschen */
              {
                strncpy(strstr(Nachricht[Nr],"from ")+5,strupr(Empfaenger),6);
                strcpy(strstr(Nachricht[Nr],"Lifetime ")+9,DefaultLT);
                strcat(Nachricht[Nr]," RETURN\n");
                if(Absender[5] == ' ')
                Absender[5] = '\0';
                sprintf(dname,"%s%s.msg",msgpath,strlwr(Absender));
                aus = fopen(dname,"at");     /* an das andere Call schicken */
                fprintf(aus,"%s",Nachricht[Nr]);
                Nachricht[Nr][0] = '\0';                    /* und loeschen */
                Nr++;                                       /* einen weiter */
                RueckNr--;
                while (Anzahl > Nr)
                {
                  if(!strstr(Nachricht[Nr],"Message from "))
                  {
                    fprintf(aus,"%s",Nachricht[Nr]);
                    Nachricht[Nr][0] = '\0';
                    Nr++;
                    RueckNr--;
                  }
                  else
                  {
                    Nr--;
                    break;
                  }
                }
                fclose(aus);
              }
            }
          }
          else
          {
            itoa(LTz,LT,10);  /* Wenn nur Lifetime ein weniger. Aendern und */
            if(strlen(LT) == 1 )
            {
              LT[1] = LT[0];
              LT[0] = ' ';
            }
            strncpy(strstr(Nachricht[Nr],"Lifetime ")+9,LT,2);    /*zurueck */
            Nr++;
          }
        }                                                   /* Ende Message */
        else
        {
          if (loeschen == 1)
          {
            Nachricht[Nr][0] = '\0';
            RueckNr--;
          }
        }
        Nr++;
      }                                                         /* Ende for */
      sprintf(dname,"%s%s",msgpath,ffblk.ff_name);

      unlink(dname);                                 /* alte Datei loeschen */
    }                                                         /* Ende Datum */
                    /* und nun die alte(neue) Datei wieder zurueckschreiben */

    if (RueckNr > 0)
    {
      fp = fopen(dname,"at");                       /* und wieder aufmachen */
      for (Nr = 0; Anzahl > Nr; Nr++)
      {
        if(Nachricht[Nr][0] != '\0')
        {
          fprintf(fp,"%s",Nachricht[Nr]);
        }
      }
      fclose(fp);
      Anzahl = 0;
      RueckNr = 0;
      Nr = 0;
    }
    gefunden = xfindnext(&ffblk);                         /* naechste Datei */
  }
  printf("Checkmsg am %02d.%02d.%02d um %02d:%02d durchgefuehrt.\n",
            ts->tm_mday,ts->tm_mon+1,ts->tm_year,ts->tm_hour,ts->tm_min);
  Status = TRUE;
}
/*----------------------------------------------------------------------------
                         DirMsy.C
----------------------------------------------------------------------------*/
/* Diese Funktion listet alle vorhandenen Nachrichten in kompremierter Form
auf. Der Nachrichteninhalt wird nur als Laenge dargestellt.
Sondersache: mit # wird nach Lifetime gesucht und mit * nach Datum
*/

void DirMsy (int argc, char* argv[])
{
  int gefunden;
  int Anzahl = 0;
  int a;
  int b;
  int c;
  int SuchTage;
  int Tag;
  int Mon;
  int Jahr;
  char was;                            /* was wird gewuenscht? LT oder Tage */
  char Vorzeichen;                                          /* '+' oder '-' */
  char Befehl;
  char Hilf[60];
  char Suchdatum[10];
  char dname[MAXPATH];                     /* kompletter Pfad mit Dateiname */
  char buffer[MaxZeilen][80];
  time_t t;
  struct tm *ts;
  struct ZEILE Zeile;
  FILE *fp;
  char Monat[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if(argc > 4 )
    Stop("Zuviele Parameter!\n");

  Befehl = ' ';                                         /* als Vorbesetzung */

  if(argc == 3)                              /* MSY D CALL == MSY D TO CALL */
    Befehl = 'T';

  if (argc == 4)
  {
    strupr(argv[2]);
    {
      if(!strcmp(argv[2], "TO"))
      {
        Befehl = 'T';
      }
        if(!strcmp(argv[2],"FROM"))
      {
        Befehl = 'F';
      }
      if(!strcmp(argv[2],"BYTE"))
      {
        Befehl = 'B';
      }
      if(!strcmp(argv[2],"DATE"))
      {
        Befehl = 'D';
      }
      if(!strcmp(argv[2],"TIME"))
      {
        Befehl = 'H';
      }
      if(!strcmp(argv[2],"LT"))
      {
        Befehl = 'L';
      }
      if(!strcmp(argv[2],"RETURN"))
      {
        Befehl = 'R';
      }
      if(*argv[2] == '-'|| *argv[2] == '+')
      {
        if(sscanf(argv[2],"%c%c%d",&Vorzeichen,&was,&SuchTage) != 3)
          Stop("Parameter falsch!\n");
        if(SuchTage < 1 || SuchTage > MaxLT)
          Stop("Tage falsch angegeben!\n");
        if(was == '*')
        {
          time(&t);
          ts = localtime(&t);                      /* aktuelles Datum holen */
          if((Tag = ts->tm_mday - SuchTage) > 0)     /* ist in diesem Monat */
            sprintf(Suchdatum,"%02d%02d%02d",ts->tm_year,ts->tm_mon+1,Tag);
          else                                        /* doch anderer Monat */
          {
            Tag = ts->tm_mday - SuchTage;      /* ist nicht in diesem Monat */
            Mon = ts->tm_mon + 1;    /* 1 mehr. localtime liefert 1 weniger */
            Jahr = ts->tm_year + 1900;        /* Muss wegen Schaltjahr sein */
            while (Tag < 1)
            {
              Mon--;
              if (Mon < 1)                               /* Jahreswechsel ? */
                Mon = 12;
              if (Mon == 2 && (Jahr%4) == (Jahr/4) && Jahr != 0)
                Monat[Mon - 1] = 29;                          /* Schaltjahr */
              Tag += Monat[Mon - 1];
            }
            if(Mon > ts->tm_mon)                     /* War Jahreswechsel ? */
              Jahr -= 1901;
            else
              Jahr -= 1900;
            sprintf(Suchdatum,"%02d%02d%02d",Jahr,Mon,Tag);
          }
        }
        Befehl = 'X';        /* Es wird nach der TO-Sortierung ausgegeben */
      }
    }
  }
  if(Befehl == ' ')
    Stop("Falscher Parameter!\n");
/*----------------------Datei suchen-------------------------------*/
  sprintf(dname,"%s*.msg",msgpath);

  if (( gefunden = xfindfirst(dname,&ffblk,0)) != FALSE)
  {
    printf("Keine MSG-Dateien gefunden.\n");
    exit(0);
  }
/*--------------------Datei einlesen-------------------------------*/
  while(!gefunden)
  {
    sprintf(dname,"%s%s",msgpath,ffblk.ff_name);
    if((fp = fopen(dname,"rt")) == NULL)
    {
      sprintf(dname,"Fehler #D! beim Oeffnen der Datei %s\n",ffblk.ff_name);
      Stop(dname);
    }
    a = 0;

    while(!feof(fp))                      /* Datei wird komplett eingelesen */
    {
      if((fgets(buffer[a],79,fp)) != NULL)
        a++;
    }
    fclose(fp);
/*------------------Buffer auswerten--------------------------*/
    for(b = 0,c = 0;b < a; b++)
    {
      if(strstr(buffer[b],"Message from "))  /* erst den Header verarbeiten */
      {
        if(c > 0)             /* wenn c > 0, dann gab es schon einen Header */
          Anzahl = Kopie(Anzahl,Befehl,&Zeile,was,Suchdatum,SuchTage,
          Vorzeichen);
        strncpy(Zeile.To,ffblk.ff_name,6);                            /* To */
        if(Zeile.To[5] ==  '.')
          Zeile.To[5] = ' ';                     /* Call ist immer 6stellig */
        Zeile.To[6] = '\0';
        c = sscanf(buffer[b],"%*s %*s %6c %*s %*s %13c %*s %s %*s %i %s",
                   Zeile.From,Zeile.Date,Zeile.Time,&Zeile.LT,
                   Zeile.Return);
        Zeile.Date[13] = '\0';                 /* muss stringabschluss sein */
        Zeile.From[6] = '\0';
        if(c == 4)                                           /* kein Return */
          strcpy(Zeile.Return,"      \0");   /* fuer Sortierung nach Return */

        Zeile.Byte = 0;          /* auf 0 setzen, der vorherige hat gesetzt */
      }
      else
        Zeile.Byte += (strlen(buffer[b])-2);     /* eingeruecktes Blank und */
    }                                                    /* (cr)lf entfernt */
    Anzahl = Kopie(Anzahl,Befehl,&Zeile,was,Suchdatum,SuchTage,Vorzeichen);

    gefunden = xfindnext(&ffblk);                         /* naechste Datei */
  }
/*----------------------Daten ausgeben--------------------------------*/
  if(Anzahl == 0)
  {
    printf("\nKeine passende MSG-Datei gefunden !\n");
    Status = TRUE;
    return;
  }
  printf("\n");
  switch(Befehl)
  {
    case 'T' :
    case 'X' :
    {
      printf("  TO     FROM   BYTE       DATE       TIME   LT  RETURN\n");
      printf("------  ------  ----  -------------  ------  --  ------\n");
      break;
    }
    case 'F' :
    {
      printf(" FROM     TO    BYTE       DATE       TIME   LT  RETURN\n");
      printf("------  ------  ----  -------------  ------  --  ------\n");
      break;
    }
    case 'B' :
    {
      printf("BYTE    TO     FROM        DATE       TIME   LT  RETURN\n");
      printf("----  ------  ------  -------------  ------  --  ------\n");
      break;
    }
    case 'D' :
    {
      printf("     DATE        TO     FROM   BYTE   TIME   LT  RETURN\n");
      printf("-------------  ------  ------  ----  ------  --  ------\n");
      break;
    }
    case 'H' :
    {
      printf(" TIME     TO     FROM   BYTE       DATE      LT  RETURN\n");
      printf("------  ------  ------  ----  -------------  --  ------\n");
      break;
    }
    case 'L' :
    {
      printf("LT    TO     FROM   BYTE       DATE       TIME   RETURN\n");
      printf("--  ------  ------  ----  -------------  ------  ------\n");
      break;
    }
    case 'R' :
    {
      printf("RETURN    TO     FROM   BYTE       DATE       TIME   LT\n");
      printf("------  ------  ------  ----  -------------  ------  --\n");
      break;
    }
  }
  for (a = 0; a < Anzahl; a++)
  {
    for (b = Anzahl - 1; b >= a; b--)
    {
      if (strcmp(Nachricht[a], Nachricht[b]) > 0)              /* sortieren */
      {
        strcpy(Hilf, Nachricht[a]);
        strcpy(Nachricht[a], Nachricht[b]);
        strcpy(Nachricht[b], Hilf);
      }
    }
  }

  for(a = 0; a < Anzahl; a++)                                   /* ausgeben */
  {
    printf("%s\n",Nachricht[a]);
  }
  Status = TRUE;
}
int Kopie (int Anzahl, char Befehl, struct ZEILE *Zeile, char was,
           char *Suchdatum,int SuchTage,char Vorzeichen )
{
  int a;
  int erdarf = 0;
  int Tag;
  int Mon;
  int Jahr;
  char laenge[5];
  char hilf[3];
  char Hilf[10];
  char Abstand[] = "  \0";

  if(Befehl == 'X')
  {
    if(was == '*')
    {
      sscanf(Zeile->Date,"%*s%d.%d.%d",&Tag,&Mon,&Jahr);   /* Datum von MSG */
      sprintf(Hilf,"%02d%02d%02d",Jahr,Mon,Tag);
      a = strcmp(Suchdatum,Hilf);              /* aelter == +, juenger == - */
    }
    else                                                 /* dann ist es `#` */
    {
      if(Zeile->LT < SuchTage)               /* kleiner == -, groesser == + */
        a = 1;
      else
        a = -1;
    }
    erdarf = 0;
    if(Vorzeichen == '-' && a > 0)
      erdarf = 1;
    if(Vorzeichen == '+' && a < 0)
      erdarf = 1;
    if(erdarf == 0)
      return(Anzahl);
  }
  itoa(Zeile->Byte,hilf,10);                 /* aus Zahl eine string machen */
  sprintf(laenge,"%4s",hilf);
  itoa(Zeile->LT,hilf,10);     /* bei einstelliger LT, muss 2stellig werden */
  sprintf(Hilf,"%2s",hilf);
  switch(Befehl)    /* String nach gewuenschter Reihenfolge zusammenstellen */
  {
    case 'T' :
    case 'X' :
    {
      strcpy(Nachricht[Anzahl],strupr(Zeile->To));
      *Zeile->To ='\0';
      break;
    }
    case 'F' :
    {
      strcpy(Nachricht[Anzahl],Zeile->From);
      *Zeile->From ='\0';
      break;
    }
    case 'B' :
    {
      strcpy(Nachricht[Anzahl],laenge);
      *laenge ='\0';
      break;
    }
    case 'D' :
    {
      strcpy(Nachricht[Anzahl],Zeile->Date);
      *Zeile->Date ='\0';
      break;
    }
    case 'H' :
    {
      strcpy(Nachricht[Anzahl],Zeile->Time);
      *Zeile->Time ='\0';
      break;
    }
    case 'L' :
    {
      strcpy(Nachricht[Anzahl],Hilf);
      *Hilf ='\0';
      break;
    }
    case 'R' :
    {
      strcpy(Nachricht[Anzahl],Zeile->Return);
      *Zeile->Return ='\0';
      break;
    }
  }
  strcat(Nachricht[Anzahl],Abstand);     /* Der Abstand nach der Sortierung */

  if(*Zeile->To)         /* und nun den Rest. nix kann nicht kopiert werden */
  {
    strcat(Nachricht[Anzahl],strupr(Zeile->To));
    strcat(Nachricht[Anzahl],Abstand);
  }
  if(*Zeile->From)
  {
    strcat(Nachricht[Anzahl],Zeile->From);
    strcat(Nachricht[Anzahl],Abstand);
  }
  if(*laenge)
  {
    strcat(Nachricht[Anzahl],laenge);
    strcat(Nachricht[Anzahl],Abstand);
  }
  if(*Zeile->Date)
  {
   strcat(Nachricht[Anzahl],Zeile->Date);
   strcat(Nachricht[Anzahl],Abstand);
  }
  if(*Zeile->Time)
  {
   strcat(Nachricht[Anzahl],Zeile->Time);
   strcat(Nachricht[Anzahl],Abstand);
  }
  if(*Hilf)
  {
    strcat(Nachricht[Anzahl],Hilf);
    strcat(Nachricht[Anzahl],Abstand);
  }
  if(*Zeile->Return)
    strcat(Nachricht[Anzahl],Zeile->Return);
  Anzahl++;
  return(Anzahl);
}
/*----------------------------------------------------------------------------
                         ListMsy.C
----------------------------------------------------------------------------*/
/* ListMsy ist eigentlich falsch!! Es wird zwar was gelistet, aber auch
meistens geloescht. Ich habe aber den Funktionsnamen aus dem Pascalsource
uebernommen.
Diese Teil prueft die Msg-Nummern bzw das Call, listet sie auf und loescht
das Ganze. Der Rest wird wieder gespeichert.

Parameterreihenfolge ist: Programmname Befehl Argument  Argument Sysopcall
              Argument(argv)     0          1       2          3       4
              Argument(argc)     1          2       3          4       5
                                NSY         E     DG8BR      DG8BR
Diese Variante ist nicht drin.  MSY         E 01.01.98/15:02 DG8BR
                                MSY         E     DG8BR        3     DG8BR
                                MSY         E      ALL       DG8BR   DG8BR

Als Sysop darf er alles loeschen. Darum wird argc 5 nicht ueberprueft.
*/

void ListMsy (int argc,char *argv[])
{
  int von = 1;                 /* Nur Nachricht von * ausgeben. 1 = default */
  int bis = 99;               /* Nur Nachricht bis * ausgeben. 99 = default */
  int Nr = 0;                  /* Nummer der Nachricht. Von 1 bis MaxZeilen */
  int a;
  int aus;                 /* es wurde was ausgegeben, also nicht speichern */
  int Ruecknr = 0;                /* wieviel Zeilen noch zurueckgeschreiben */
  int alle = 0;                /* Flag fuer alle MSG's von/an Call loeschen */
  int gefunden;                                           /* Datei gefunden */
  int loeschen = 0;                                         /* loeschenflag */
  char trenner = '\0';                    /* wird nur fuer sscanf benoetigt */
  char AnzNr[3] = "00";                             /* Das wird ausgegeben. */
  char dname[MAXPATH];                                /* Pfad und Dateiname */
  char name[7];                                      /* gesuchter Dateiname */
  char ptr[7];                                       /* Absendercall in MSG */
  char Ucall[10];                                   /* gekuerzter Dateiname */
  FILE *fp;

  *name = '\0';                                    /* erstmal den Namen weg */
/*----------------------------------------------------------------------------
                     von-bis und eventuell das call pruefen.
----------------------------------------------------------------------------*/
  if (argc == 4 || argc == 5)                       /* Nummer und/oder Call */
  {
    for ( a = 2; a < argc-1; a++)       /* eigenes Call wird mit uebergeben */
    {
      if((isdigit(argv[a][0])) != FALSE)
        Nr = sscanf(argv[a],"%02d%c%02d",&von, &trenner, &bis);
      if (Nr == 1)                  /* wenn nur eine Zahl eingegeben wurde. */
        bis = von;
      if((argv[a][0] == '-') == TRUE)                   /* nur -X angegeben */
        Nr = sscanf (argv[a],"%c%02d",&trenner, &bis);
      if (Nr == 0)                                     /* Argument ein Call */
      {
        sscanf(argv[a],"%s",&(*name));
        if (stricmp(name,"ALL") == FALSE)                /* all angegeben ? */
        {
          alle = TRUE;
          *name = '\0';           /* name fuer die callpruefung frei machen */
        }
      }
      Nr = 0;     /* name nach von-bis wird nicht gefunden, wenn Nr gesetzt */
    }
    if( von > bis )                /* Ev. sollte man das einfach verbessern */
      Stop("Ende liegt vor Anfang.");
  }
  if(*name == FALSE)                      /* wenn kein Call angegeben wurde */
  {
    Stop("Kein Rufzeichen angegeben!");   /* Es MUSS immer ein Call da sein */
  }
  else
  {
    if (Calltest(name) == FALSE)    /* Gucken ob der User richtig schreiben */
      Stop("Das angegebene Rufzeichen ist ungueltig.");     /* Sonst Mecker */
  }
  trenner = FALSE;          /* wird nun fuer Call schon ausgegeben benutzt. */
/*--------------------------------------------------------------------------*/

  if( alle == TRUE)              /* wenn all, dann alle Dateien durchsuchen */
  {
    sprintf(dname,"%s*.msg",msgpath);
    loeschen = 1;                                       /* loeschen sperren */
  }
  else                                              /* Einzelcall angegeben */
    sprintf(dname,"%s%s.msg",msgpath,name);

  if (( gefunden = xfindfirst(dname,&ffblk,0)) != FALSE)
    Stop("Keine MSG-Dateien gefunden !");

  while(!gefunden)                      /* Solange es noch Msg-Dateien gibt */
  {
    sprintf(dname,"%s%s",msgpath,ffblk.ff_name);
    if ((fp = fopen(dname,"rt")) == NULL)
    {
      printf("Keine MSG fuer %s gefunden.\n",name);
      exit(0);
    }
    strcpy(Ucall,ffblk.ff_name);                   /* Usernamen selektieren */
    Ucall[strlen(Ucall)-4] = '\0';                              /* .MSG weg */
    strupr(Ucall);
    Nr = 0;
    AnzNr[0] = AnzNr[1] = '0';

    while(!feof(fp))                                 /* erstmal alles lesen */
    {
      if((fgets(Nachricht[Nr],79,fp)) != NULL)
        Nr++;
      if ( Nr > MaxZeilen )                      /* Sollte nie vorkommen !! */
      {
        fclose(fp);
        sprintf(dname,"%s ist zu gross! ",ffblk.ff_name);
        Stop(dname);
      }
    }
    fclose(fp);
         /* das nachfolgende xfindnext MUSS hier stehen, weil xfindnext mit */
                                  /*  geloeschte Dateien nicht funktioniert */
    gefunden = xfindnext(&ffblk);                         /* naechste Datei */


/*----------------------------------------------------------------------------
Die gewuenschten Nachrichten ausgeben und wenn geloescht werden soll, die
entsprechenden Zeilen markieren.
----------------------------------------------------------------------------*/
    aus = 0;
    Ruecknr = Nr;

    for (a = 0; a < Nr; a++)
    {
      if(strstr(Nachricht[a],"Message from "))         /* Kopfzeile von MSG */
      {
        loeschen = 1;                                   /* loeschen sperren */
        if ( AnzNr[1] == '9' )          /* wenn 9 dann auf 0 setzen und das */
        {                                           /* vorherige Byte auf 1 */
          AnzNr[0]++;
          AnzNr[1] = '0';
        }
        else
          AnzNr[1]++;                      /* wenn nicht, dann nur erhoehen */

        aus++;                             /* zaehlt die nachrichtennummern */
        if (alle == 1)
        {
          if(stricmp(Ucall,name) == 0)       /* Dateiname gleich Suchcall?? */
            loeschen = 0;
          else
          {
            sscanf(Nachricht[a],"%*s %*s %s",ptr);    /* MSG von Suchcall?? */
            if(!stricmp(ptr,name))
              loeschen = 0;
          }
        }
        else
          loeschen = 0;                     /* kein all, loeschen freigeben */
        if(( aus >= von) && ( aus <= bis) && (loeschen == 0) == TRUE)
        {
          if(trenner == FALSE)
          {
            printf("\n%s :\n",Ucall);  /* Empfaenger der Nachricht ausgeben */

            for (Nr = strlen(Ucall)+2; Nr > 0; Nr--)
              printf("-");
            printf("\n");
            trenner = TRUE;
            Nr = Ruecknr; /* Nr wieder zuruechschreiben. Wurde missbraucht. */
          }
          printf("MSG %s: %s",AnzNr,Nachricht[a]);
          *Nachricht[a] = '\0';                       /* Loeschen markieren */
          Ruecknr--;
        }
      }
      else                                      /* ist kein Nachrichtenkopf */
      {
        if(( aus >= von) && ( aus <= bis) && (loeschen == 0)== TRUE)
        {                                           /* aber gueltige Nummer */
          printf("%s",Nachricht[a]);
          *Nachricht[a] = '\0';
          Ruecknr--;
        }
      }
    }
/*----------------------------------------------------------------------------
                         Und nun wird geloescht.
----------------------------------------------------------------------------*/

    if(Ruecknr < Nr)                  /* Wenn es denn was zuloeschen gibt.. */
    {
      if(Ruecknr == 0)                 /* ist nix mehr zum zurueckschreiben */
        unlink(dname);
      else
      {
        unlink(dname);
        if ((fp = fopen(dname,"wt")) == NULL)
        {
          Stop("Fehler #L1 Fehler beim Zurueckschreiben der Datei!");
        }
        for (a = 0; a < Nr; a++)
        {
          if(*Nachricht[a] != '\0')
            fprintf(fp,"%s",Nachricht[a]);
        }
        fclose(fp);
      }
    }
    trenner = FALSE;
  }
  if(Ruecknr < Nr)                    /* wenn ja, dann wurde was geloescht. */
    printf("\nDiese MSG('s) wurde(n) soeben geloescht!\n");
  else
    printf("\nEs wurde nichts geloescht!\n");

  Status = TRUE;
}
/*============================================================================
                                 SendMsy.c
============================================================================*/
/* Parameterreihenfolge ist:
Programmname Befehl Empfaenger (Lifetime) Nachricht Absender */

void SendMsy ( int argc, char *argv[] )
{
  int a;
  char LT[3] = DefaultLT;                   /* LifeTime im Header einer MSG */
  char buffer[80];                                       /* Nachrichtentext */
  char dname[MAXPATH];
  char name[9];                              /* Gruppen oder Empfeangername */
  FILE *fp;                                       /* Empfaenger-Dateizeiger */

  a = 2;                                                    /* Empfangscall */

  sscanf (argv[a], "%s", &(*name));               /* Empfaenger feststellen */
  if ( Calltest(name) == FALSE)
    Stop("Fehler im Empfaengerrufzeichen.");

  a++;                                           /* Lifetime oder Nachricht */

  if(argv[a][0] == '#')                              /* Lifetime vorhanden? */
  {
    strcpy (LT,&(argv[a][1]));                                  /* Lifetime */
    if (( atoi(LT) < 1 ) || (atoi(LT) > MaxLT))
    {
      printf("ACHTUNG ! Lifetime muss zwischen 1 und %d sein!\n\n",MaxLT);
      strcpy(LT,DefaultLT);
    }
    a++;                                        /* Nur wenn lifetime da war */
  }

  buffer[0] = ' ';           /* Nachricht einruecken. Sieht doch besser aus */
  buffer[1] = '\0';

  for(;a < argc-1; a++)                 /* die komplette Nachricht einlesen */
  {
    sprintf (buffer+(char)strlen(buffer),"%s ", argv[a]);
  }
  if(buffer[1] == '\0')                             /* wenn nix dann mecker */
    Stop("Nachricht fehlt !");

  buffer[strlen(buffer)-1] = '\n';                      /* return einfuegen */
  a = 0;                                             /* wird noch benoetigt */
/*--------------------------------------------------------------------------*/
  if ( Gruppe == TRUE )                        /* wird in Calltest geprueft */
  {
    sprintf(dname,"%s%s.mbr",msgpath,name);         /* Gruppendatei oeffnen */
    if((fp = fopen(dname,"rt")) == NULL)
    {
      sprintf(dname,"Fehler #S1 Konnte die Datei %s nicht oeffnen.",name);
      Stop(dname);
    }
    while(!feof(fp))
    {
      *name = '\0';
      fscanf(fp,"%7s",name);                      /* Call aus der MBR holen */
      if(name[0] != '\0')                   /* fscanf findet trotz feof was */
      {
        strlwr(name);
        speichern(LT,name,buffer);
        a = TRUE;
      }
    }
    fclose(fp);
  }
  else
    a = speichern(LT,name,buffer);                 /* Speichern ohne Gruppe */

  if (a == TRUE)
  {
    printf("\nInhalt der Nachricht :\n");              /* das bekommen alle */
    printf("----------------------\n");
    printf("%s",buffer);
  }

  Status = TRUE;
}
/*--------------------------------------------------------------------------*/
int speichern (char *LT,char *name,char *buffer)
{
  int a = 0;                                          /* Nachrichtenzaehler */
  int b = 0;                                           /* Umspeicherzaehler */
  int ist = 0;                          /* Flag fuer eingefuegte Nachricht. */
  char dname[MAXPATH];
  char MsgZeit[40];
  char Zbuffer[80];                      /* Zwischenbuffer beim umspeichern */
  char Datum[9];
  FILE *fp;
  time_t t;
  struct tm *ts;
  char *Tage[] = { "Sun", "Mon", "Thu", "Wed", "Thu", "Fri", "Sat" };

  time(&t);
  ts = localtime(&t);
  sprintf(Datum,"%02d.%02d.%02d",ts->tm_mday,ts->tm_mon +1,ts->tm_year%100);
  sprintf(MsgZeit,"Time: %s, %s / %02d:%02dZ",Tage[ts->tm_wday],
                                              Datum,
                                              ts->tm_hour,
                                              ts->tm_min);

  sprintf(dname,"%s%s.msg",msgpath,name);
  strupr(name);                   /* nun wird name nur noch GROSS gebraucht */

  if ((fp = fopen(dname,"rt")) == NULL)                 /* Datei vorhanden? */
  {
    if((fp = fopen(dname,"wt")) == NULL)        /* Nein, schreibend oeffnen */
    {
      printf("Fehler beim Oeffnen von %s !\n",dname);
      return(FALSE);
    }
    fprintf(fp,"Message from %-6s - %s  Lifetime %2s\n%s",
                Usercall,MsgZeit,LT,buffer);

    fclose(fp);
  }
  else   /* Datei ist da. Alles einlesen und aktuelle Nachricht einschieben */
  {
    fgets(Nachricht[a], 79,fp);
    while(!feof(fp))
    {
      if(strstr(Nachricht[a],"Message from "))         /* Kopfzeile von MSG */
      {
        if( strstr(Nachricht[a],Usercall))   /* hat er schon eine von call? */
        {                                           /* und auch von heute ? */
          if(strstr(Nachricht[a],Datum) && !strstr(Nachricht[a],"RETURN"))
          {                                          /* LT aus Datei nehmen */
            if(Gruppe == FALSE)         /* Bei Gruppe die LT ueberschreiben */
              strncpy(LT,strstr(Nachricht[a],"Lifetime ")+9,2);
            else                                    /* sonst LT uebernehmen */
            {
              strncpy (strstr(Nachricht[a],"Lifetime ")+9,LT,2);
            }
            fgets(Nachricht[++a],79,fp);   /* 1 String kommt auf jeden Fall */
            do
            {
              if(fgets(Zbuffer,79,fp))                   /* kommt noch mehr */
              {
                if(strstr(Zbuffer,"Message from "))
                  break;                         /* Nee. War nur eine Zeile */
                strcpy(Nachricht[++a],Zbuffer);           /* sonst kopieren */
              }
              else                         /* sicherheitshalber leer machen */
                *Zbuffer = '\0';
            }
            while(!feof(fp));

            strcpy(Nachricht[++a],buffer);  /* aktuelle Nachricht einfuegen */
            if(Zbuffer[0] != '\x0')     /* die geholte Zeile auch einfuegen */
              strcpy(Nachricht[++a],Zbuffer);
            ist = TRUE;                                  /* habe eingefuegt */
          }
        }
      }
      a++;
      fgets(Nachricht[a], 79,fp);
    }
    fclose(fp);

    if ( a >= MaxZeilen )
    {
      printf("%s hat zuviele Nachrichten ! Nichts gespeichert!\n",name);
      return(FALSE);                  /* Speichertext wird nicht ausgegeben */
    }

    unlink(dname);

    if ((fp = fopen(dname,"wt")) == NULL)
      Stop("Fehler #S2 Problem beim Zurueckschreiben der Datei!");
    while (b < a)
    {
      fprintf(fp,"%s",Nachricht[b++]);
    }
    if(ist == FALSE)               /* habe nicht eingefuegt, also anhaengen */
      fprintf(fp,"Message from %-6s - %s  Lifetime %2s\n%s",
                  Usercall,MsgZeit,LT,buffer);

    fclose(fp);
  }

  printf("MSG wurde fuer %-6s mit einer Lifetime von %s Tag(en)"
         " gespeichert.\n",name,LT);
  return(TRUE);                                        /* war alles richtig */
}


/*----------------------------------------------------------------------------
                         GetMsy.C

Dieser Teil listet einfach alle Msg-Dateien auf. Header und Inhalt.
----------------------------------------------------------------------------*/
void GetMsy (void)
{
  int a;
  int gefunden;
  char dname[MAXPATH];
  char Datei[10];
  char buffer[80];
  FILE *fp;

  sprintf(dname,"%s*.msg",msgpath);

  if (( gefunden = xfindfirst(dname,&ffblk,0)) != FALSE)
    Stop("Keine MSG-Dateien gefunden.");

  while(!gefunden)
  {
    strcpy(Datei,ffblk.ff_name);
    printf("\n%s\n",strupr(Datei));
    for (a = strlen(Datei); a > 0; a--)
      printf("-");
    printf("\n");
    sprintf(dname,"%s%s",msgpath,ffblk.ff_name);
    fp = fopen(dname,"rt");
    while(!feof(fp))
    {
      if((fgets(buffer,79,fp)) != NULL)
        printf("%s",buffer);
    }
    fclose(fp);
    gefunden = xfindnext(&ffblk);
  }
  Status = TRUE;
}
/*----------------------------------------------------------------------------
                         GroupMsy.C
----------------------------------------------------------------------------*/
/* Parameterreihenfolge ist: Programmname Befehl [Gruppe  -/+ Call] Usercall
             Argument(argv)     0          1        2      3    4      5
             Argument(argc)     1          2        3      4    5      6
   Wobei Gruppe, -/+ und call optional sind. Wenn keine Gruppe angegeben
   wurde, dann alle Gruppen auflisten. Das letze Argument wird nicht aus-
   gewertet.
*/

void GroupMsy ( int argc, char *argv[] )
{
  char dname[MAXPATH];

  if(argc == 3)                                   /* Keine Gruppe angegeben */
  {
    sprintf(dname,"%s*.mbr",msgpath);
    GroupList(dname);
  }
  if(argc == 4)                                         /* Gruppe angegeben */
  {
    sprintf(dname,"%s%s.mbr",msgpath,strlwr(argv[2]));
    GroupList(dname);
  }
  if(argc == 5)
    Stop("Zuviele oder zuwenige Parameter !");
  if(argc == 6)
    GroupSys(argv);
}
void GroupList(char *dname)
{
  int a;
  int gefunden;
  FILE *fp;

  gefunden = xfindfirst( dname, &ffblk, 0);
  if (gefunden != FALSE)               /* wenn nicht gefunden, dann meckern */
    Stop("Gruppe nicht gefunden...");

  printf("\n");

  while (!gefunden)
  {
    sprintf(dname,"%s%s",msgpath,ffblk.ff_name);

    if (( fp = fopen(dname,"rt")) == NULL)
      Stop("Fehler #G1 Konnte Datei nicht oeffnen!");

    strcpy(dname,ffblk.ff_name);
    dname[strlen(dname)-4] = '\0';                   /* Standard IMMER .EXT */
    printf("%s :\n",dname);

    for(a = strlen(dname)+2;a > 0;a--)          /* Laenge des Gruppennamens */
    {
       printf("-");                                       /* unterstreichen */
    }
    printf("\n");                              /* und dann ..... return !!! */

    do                                                    /* Datei ausgeben */
    {
      if(( fgets(dname,79,fp)) != NULL)
        printf("%s",dname);
    }
    while (!feof(fp));

    printf("\n");

    fclose (fp);
    gefunden = xfindnext(&ffblk);
  }
  Status= TRUE;
}
/*----------------------------------------------------------------------------
                         GroupSys.C
----------------------------------------------------------------------------*/
void GroupSys ( char *argv[])
{
  int a;
  int Anzahl;
  int Nr = 0;
  int was;                                       /* Flag fuer - oder + call */
  int gefunden = FALSE;
  char dname[MAXPATH];
  char call[MAXGROUP][8];
  FILE *fp;

  sprintf(dname,"%s%s.mbr",msgpath,strlwr(argv[2]));
  was = *argv[3];                             /* Was wird den gewuenscht ?? */

  if (was > '.' )                           /* simpelpruefung ob es + - ist */
    Stop("Falsche Parameterreihenfolge!");

  if((fp = fopen(dname,"rt")) == NULL)            /* Nun kommt es drauf an. */
  {
    if(was == '-')                       /* Loeschen? Und ist da auch was ? */
      Stop("Gruppe ist nicht vorhanden!");                 /* Noeh. Ist nix */
    else
      printf("\n%s.mbr erzeugt.\n",argv[2]);                  /* Was Neues. */
  }
  else                                                            /* Dann + */
  {
    while(!feof(fp))                                    /* Erstmal einlesen */
    {
      *call[Nr] = '\0';
      fscanf(fp,"%7s",&(*call[Nr]));                /* fscanf findet zuviel */
      if(call[Nr][0] != '\0')
        Nr++;
      if(Nr > MAXGROUP)
        Stop("Zuviele Calls in der Gruppe!");
    }
    fclose(fp);
  }
  strupr(argv[4]);
  strupr(argv[2]);
  Anzahl = Nr;

  for( a = 0; a < Nr; a++ )                           /* Gruppe durchsuchen */
  {
    if((strcmp(call[a],argv[4])) == 0)
    {
      gefunden = TRUE;
      if(was == '+')                                     /* Call schon da ? */
        Stop("Das Call gibt es schon in der Gruppe!");
      else                                                   /* Loeschen !! */
      {
        printf("\n%s wurde aus %s entfernt.\n",call[a],argv[2]);
        (call[a][0] = '\0');
        Anzahl--;
      }
    }
  }
  if(gefunden == FALSE)                                  /* Nichts gefunden */
  {
    if(was == '-')                                 /* und sollte loeschen.. */
    {
      sprintf(dname,"%s ist nicht in der Gruppe %s !",argv[4],argv[2]);
      Stop(dname);
    }
    else                                                  /* also eintragen */
    {
      strcpy(call[Nr],argv[4]);
      Nr++;
      Anzahl++;
      if(Nr > MAXGROUP)
        Stop("Zuviele Calls in der Gruppe!");
      printf("\n%s wird der Gruppe %s hinzugefuegt.\n",call[a],argv[2]);
    }
  }
  unlink(dname);                          /* die eingelesene Datei loeschen */

  if (Anzahl == 0)
  {
    printf("Das Call war der letzte Eintrag. Gruppe geloescht.\n");
    exit(0);
  }

  if((fp = fopen(dname,"wt")) == NULL)
    Stop("Fehler #G2 Probleme beim Oeffnen der Datei !");

  for ( a = 0,Anzahl = 0; a < Nr; a++)
  {
    if(*call[a] != '\0')
    {
      fprintf(fp,"%-6s ",call[a]);
      Anzahl++;           /* Anzahl nur erhoehen wenn auch geschrieben wird */
    }
    if(Anzahl > 10)                                    /* ist eigentlich 11 */
    {
      fprintf(fp,"\n");
      Anzahl = 0;
    }
  }
  fprintf(fp,"\n");                         /* Return nach dem letztem Call */
  fclose(fp);
  Status = TRUE;
}

/*----------------------------------------------------------------------------
                         MSY_LIN.C
----------------------------------------------------------------------------*/
void Stop(char *S)
{
  if (strchr(S, '#'))                                       /* Systemfehler */
  {
    printf("\n%s\n", S);
    printf("Bitte Beschreibung des Fehlers an Sysop schicken !\n");
  }
  else
  {
    if (strchr(S, '!'))                                 /* Bedienungsfehler */
    {
      printf("\n%s\n", S);
      printf("Bitte Hilfe anfordern mit 'MSY H'.\n");
    }
    else
    {
      printf("\n%s\n", S);                        /* nur ne dumme Bemerkung */
    }
  }
  exit(0);
}
/*--------------------------------------------------------------------------*/
int Calltest( char *Call )
{
  int falschesZ, Zahl, i, a;
  char dname[MAXPATH];
  Zahl = 0;
  falschesZ = 0;                                        /* falsches Zeichen */

  if(!Gruppe)
  {
    if (strchr(Call, '-') > (char *)NULL)
      *(char *) (strchr(Call, '-')) = '\0';
  }
  a = strlen(Call);
  for (i = 0; i < a; i++)                  /* pruefen ob Zeichen in Call Ok */
  {
    Call[i] = tolower(Call[i]);                   /* fuer die Ascii-Ausgabe */

      /* alle Zeichen die in der ASCII-Tabelle ueber Z, zwischen 9 und A
                              sowie unter 0 liegen, machen Call ungueltig ! */

    if (isdigit(Call[i]))
      Zahl++;
    if (Call[i] > 'z')
      falschesZ++;
    if (Call[i] < 'a' && Call[i] > '9')
      falschesZ++;
    if (Call[i] < '0')
      falschesZ++;
  }

  sprintf(dname,"%s%s.mbr",msgpath,Call); /* auch auf Gruppe testen, erspart*/
  if ((xfindfirst(dname, &ffblk,0)) == 0)                /* einwenig Arbeit */
    Gruppe = TRUE;

  if (!Gruppe && (strlen(Call) < 4 || strlen(Call) > 6 || Zahl < 1 || Zahl > 2
                                   || falschesZ > 0))
    return(TRUE);
  return(TRUE);
}
/*--------------------------------------------------------------------------*/

int main ( int argc, char *argv[] )
{
  char hilfstring[MAXPATH];
  char Call[11];
  char Befehl = '\0';                                 /* Befehl ist argv[1] */
  char *fnpoi;

  Status = FALSE;  /* ist immer FALSE, muss von der Funktion gesetzt werden */

  *msgpath = '\0';

  if (getcwd(msgpath,MAXPATH)== NULL)                  /* Wo sind wir denn? */
  {
    printf("\nKonnte Pfad nicht ermmitteln!\n");
    exit(0);
  }

  if(*(fnpoi = msgpath + strlen(msgpath)) != Seperator) /* / oder \\ dran ? */
    *fnpoi++ = Seperator;
  strcat(fnpoi, "msg");      /* Hier sollten immer MSG`s unter TNN sein !!! */
  *(fnpoi+3) = Seperator;                     /* msgpath ist nun festgelegt */

  sprintf ( hilfstring,"%smsg.sys",msgpath);             /* Hilfedatei da ??*/

  if (xfindfirst( hilfstring, &ffblk, 0) != FALSE)
    Stop ( "\nFehler #M1 Fehler im MSGPATH ! Hilfe-Datei nicht gefunden !" );


  if (argc < 3)
    Stop("\nEs wurde kein Befehl angegeben !");

  if ( strlen(argv[1]) > 1 )
    Stop( "\nDer Befehl wird nur mit dem Anfangsbuchstaben angegeben !");
  else
    Befehl = tolower( argv[1][0] );

  strcpy(Call, argv[argc - 1]);                       /* Absender ermitteln */
  if(Calltest(Call) == FALSE)             /* und pruefen ob es ein call ist */
    Stop("\nFehler in Deinem Rufzeichen !?");
  else
  {
    strcpy ( Usercall, Call);
    strupr(Usercall);
  }

  printf("\nDigimail fuer SysOp's\n");

  switch (Befehl)
  {
    case 'c': CheckMsy();
              break;

    case 'd': DirMsy(argc, argv);
              break;

    case 'e': ListMsy(argc, argv);
              break;

    case 's': if (argc < 4)
                Stop("Zuwenig Parameter!\n");
              else
                SendMsy(argc,argv);
              break;

    case 'l': GetMsy();
              break;

    case 'g': GroupMsy(argc,argv);
              break;

    case 'h': HelpMsy();
              break;

    case 'v': VerMsy();
              break;

    default:  sprintf(hilfstring, "Befehl %c ungueltig !", Befehl);
              Stop(hilfstring);
              break;
  }
  if (!Status)
  {
    sprintf(hilfstring, "#Fehler bei Ausfuehrung des '%c'-Befehls !",Befehl);
    Stop(hilfstring);
  }
  return(0);
}
