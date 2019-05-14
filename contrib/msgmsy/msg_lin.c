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
/* File msg_lin.c (maintained by: DG8BR)                                */
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

/*****************************************************************************
Dies ist ein Remake von MSG.PAS.
Erstellt wurde die Originalversion von DG3AAH und DF7BZ in Pascal.

Ihnen ist die Entstehung des Programmpaares MSG und MSY zuverdanken.

Da ich einige Sachen aus DF6LN's Linuxpacket fuer TNN entnommen habe,
moechte und muss ich diese Ausgabe unter ALAS stellen.
Dieses Programm ist in der Bedienung und in der Ausgabe weitestgehend
kompatibel zum Original-MSG.

Viele Gruesse Bernd DG8BR
*****************************************************************************/

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
#endif /* MSC_VER */
#include <sys/stat.h>
#include "sys/dirent32.h"
#include "sys/fnmatch.h"
#endif /* WIN32 */

/* Definitionen */
#define MaxZeilen       200
#define MaxLT           30
#define FALSE           0
#define TRUE            (!FALSE)
#define DefaultLT       " 7"
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
int Gruppe;                                      /* True, wenn Call==Gruppe */
char Usercall[10];                              /* Aufrufender des MSG-pgms */
char msgpath[MAXPATH];
char ZeitZone[5];
#ifndef WIN32
struct ffblk ffblk;
#endif /* WIN32 */

/* Prototypen */
void ListMsg (int,int,char**);
void GroupMsg ( int, char** );
void SendMsg ( int, char** );
void HelpMsg ( void );
void VerMsg ( void );

int Suche_Datei ( char* );
void Stop (char* );
int Calltest ( char* );
int speichern ( char*, char*, char* );         /* Unterfunktion von SendMsg */

/* Alles unter dem nachfolgenden ifdef __linux__ wurde aus dem TNN-Packet   */
/* fuer LinuX von der NORD><LINK - Gruppe bzw. aus dem DJGPP GNU-Paket fuer */
/* DOS von D.J. Delorie entnommen.  */

#if defined(__linux__) || defined(WIN32)
#ifndef WIN32
int xfindfirst (const char*, struct ffblk *, int);
int xfindnext (struct ffblk *);
void setfiletime (struct ffblk * );
#endif /* WIN32 */
char* strlwr (char*);
char* strupr (char*);


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
                    + ((filetime->tm_mon & 0x0f) << 5)
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

#endif                                                        /* Linux-Teil */

/*============================================================================
                                 VerMsg.c
============================================================================*/
#define MsgVersion      "3.04"
#define MsgDatum        "14.05.1997 von DF7BZ & DG3AAH"
#define MsgLinux        "(20.06.99) nach 'C' portiert von DG8BR"
#ifdef WIN32
#define MsgWin32        "(17.01.04) Modifiziert fuer Win32 von DAA531 Oliver Kern."
#endif

void VerMsg ( void )
{
#ifdef WIN32
  printf("\nMSG-Programm %s von %s \n%s\n%s\n",MsgVersion, MsgDatum,MsgLinux,MsgWin32);
#else
  printf("\nMSG-Programm %s von %s \n%s\n",MsgVersion, MsgDatum,MsgLinux);
#endif
  Status = TRUE;
}
/*============================================================================
                                 SendMsg.c
============================================================================*/
/* Parameterreihenfolge ist:
Programmname Befehl Empfaenger (Lifetime) Nachricht Absender */
/* 1           2       3          4         5         6
   0           1       2          3         4         5   */
void SendMsg ( int argc, char *argv[] )
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
    sprintf(dname,"%s%s.mbr",msgpath,name);
         /* Gruppendatei oeffnen */
    if((fp = fopen(dname,"rt")) == NULL)
    {
      sprintf(dname,"Fehler #S1 Konnte die Datei %s nicht oeffnen.",name);
      Stop(dname);
    }
    while(!feof(fp))
    {
      *name = '\0';
      fscanf(fp,"%s",&(*name));
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
  char Nachricht[MaxZeilen][80];
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
    fprintf(fp,"\nMessage from %-6s - %s  Lifetime %2s\n%s",
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

/*============================================================================
                                 ListMsg.c
============================================================================*/
/* Parameterreihenfolge ist: Programmname Befehl von-bis Rufzeichen Usercall
              Argument(argv)     0          1       2         3       4
              Argument(argc)     1          2       3         4       5

Argument 2 (von-bis) und 3 (Rufzeichen) sind optional.
Wenn 2 nicht angegeben, dann alles ausgeben.
Wenn 3 angegeben, beim Loeschen pruefen ob Usercall der Absender war ?
Ansonsten ist es egal was fuer ein Call angegeben wurde!
*/

void ListMsg (int Befehl,int argc,char *argv[])
{
  int von = 1;                 /* Nur Nachricht von * ausgeben. 1 = default */
  int bis = 99;               /* Nur Nachricht bis * ausgeben. 99 = default */
  int Nr = 0;                  /* Nummer der Nachricht. Von 1 bis MaxZeilen */
  int a;
  int aus;                 /* es wurde was ausgegeben, also nicht speichern */
  int seine = 0;                                       /* user's eigene MSG */
  int Ruecknr;                    /* wieviel Zeilen noch zurueckgeschreiben */
  char trenner = '\0';                    /* wird nur fuer sscanf benoetigt */
  char AnzNr[3] = "00";                             /* Das wird ausgegeben. */
  char dname[MAXPATH];                                /* Pfad und Dateiname */
  char name[7];                                                /* Dateiname */
  char Nachricht[MaxZeilen][80];
  FILE *fp;

  *name = '\0';                                    /* erstmal den Namen weg */
/*----------------------------------------------------------------------------
                     von-bis und eventuell das call pruefen.
----------------------------------------------------------------------------*/
  if (argc == 4 || argc == 5)                       /* Nummer und/oder Call */
  {
    for ( a = 2; a < argc-1; a++)
    {
      if((isdigit(argv[a][0])) != FALSE)
        Nr = sscanf(argv[a],"%02d%c%02d",&von, &trenner, &bis);
      if (Nr == 1)                  /* wenn nur eine Zahl eingegeben wurde. */
        bis = von;
      if((argv[a][0] == '-') == TRUE)
        Nr = sscanf (argv[a],"%c%02d",&trenner, &bis);
      if (Nr == 0)                                     /* Argument ein Call */
        sscanf(argv[a],"%s",&(*name));
      Nr = 0;     /* name nach von-bis wird nicht gefunden, wenn Nr gesetzt */
    }
    if( von > bis )                /* Ev. sollte man das einfach verbessern */
      Stop("Ende liegt vor Anfang!");
  }
  if(*name == FALSE)                      /* wenn kein Call angegeben wurde */
  {
    strcpy(name,Usercall);                              /* Sind seine MSG's */
  }
  else
  {
    if (Calltest(name) == FALSE)    /* Gucken ob der User richtig schreiben */
      Stop("Das angegebene Rufzeichen ist ungueltig !");    /* Sonst Mecker */
  }
/*--------------------------------------------------------------------------*/
  strlwr(name);
  sprintf(dname,"%s%s.msg",msgpath,name);
  strupr(name);
  Nr = 0;

  if ((fp = fopen(dname,"rt")) == NULL)
  {
    printf("Keine MSG fuer %s gefunden.\n",name);
    exit(0);
  }

  printf("\n%s :\n",name);             /* Empfaenger der Nachricht ausgeben */

  while(!feof(fp))                                   /* erstmal alles lesen */
  {
    if((fgets(Nachricht[Nr],79,fp)) != NULL)
    {
      if (Nachricht[Nr][0] == 0x0A)                           /* LF-Return, */
        continue;                               /* naechste Zeile einlesen. */

      Nr++;
    }

    if ( Nr > MaxZeilen )                        /* Sollte nie vorkommen !! */
    {
      fclose(fp);
      sprintf(dname,"Fehler #L1 %s ist zu gross! ",ffblk.ff_name);
      Stop(dname);
    }
  }
  fclose(fp);

/*----------------------------------------------------------------------------
Die gewuenschten Nachrichten ausgeben und wenn geloescht werden soll, die
entsprechenden Zeilen markieren.
----------------------------------------------------------------------------*/
  aus = 0;
  Ruecknr = Nr;

  for (a = 0; a < Nr; a++)
  {
    if(strstr(Nachricht[a],"Message from "))           /* Kopfzeile von MSG */
    {
      if ( AnzNr[1] == '9' )            /* wenn 9 dann auf 0 setzen und das */
      {                                             /* vorherige Byte auf 1 */
        AnzNr[0]++;
        AnzNr[1] = '0';
      }
      else
        AnzNr[1]++;                        /* wenn nicht, dann nur erhoehen */

      aus++;                               /* zaehlt die nachrichtennummern */

      if(( aus >= von) && ( aus <= bis) == TRUE)
      {
        if (Befehl == 'e')       /* feststellen ob es seine oder ob von ihm */
        {
          if(!stricmp(name,Usercall) || strstr(Nachricht[a],Usercall))
          {
            printf("MSG %s: %s",AnzNr,Nachricht[a]);
            *Nachricht[a] = '\0';                     /* Loeschen markieren */
            Ruecknr--;
            seine = TRUE;                     /* fuer den Nachrichteninhalt */
          }
          else
            seine = FALSE;
        }
        if(Befehl == 'r' || Befehl == 'l')
          printf("MSG %s: %s",AnzNr,Nachricht[a]);
      }
    }
    else                                        /* ist kein Nachrichtenkopf */
    {
      if(( aus >= von) && ( aus <= bis) == TRUE)    /* aber gueltige Nummer */
      {
        if( Befehl == 'e' && seine == TRUE)
        {
          printf("%s",Nachricht[a]);
          *Nachricht[a] = '\0';
          Ruecknr--;
        }
        if(Befehl == 'r')
          printf("%s",Nachricht[a]);
      }
    }
  }
/*----------------------------------------------------------------------------
                         Und nun wird geloescht.
----------------------------------------------------------------------------*/

  if(Befehl == 'e')
  {
    if(Ruecknr == 0)                   /* ist nix mehr zum zurueckschreiben */
      unlink(dname);
    else
    {
      unlink(dname);
      if ((fp = fopen(dname,"wt")) == NULL)
      {
        Stop("Fehler #L2 Fehler beim Zurueckschreiben der Datei!");
      }
      for (a = 0; a < Nr; a++)
      {
        if(*Nachricht[a] != '\0')
          fprintf(fp,"%s",Nachricht[a]);
      }
      fclose(fp);
    }
    if(Ruecknr < Nr)                  /* wenn ja, dann wurde was geloescht. */
      printf("\nDiese MSG('s) wurde(n) soeben geloescht!\n");
    else
      printf("\nEs wurde nichts geloescht!\n");
  }
  Status = TRUE;
}
/*============================================================================
                                  HelpMsg.c
============================================================================*/
void HelpMsg ( void )
{
  char filename[MAXPATH];
  FILE *fp;

  sprintf(filename,"%smsg.usr",msgpath);
  if (( fp = fopen(filename,"rt")) == NULL)
  {
    Stop("Fehler #H1 Konnte Hilfedatei nicht oeffnen.");
  }
  while (!feof(fp))
  {
    if((fgets(filename,79,fp)) != NULL)
      printf("%s",filename);
  }
  fclose (fp);
  Status = TRUE;
}
/*============================================================================
                                    GroupMsg.c
============================================================================*/
/* Parameterreihenfolge ist: Programmname Befehl Gruppe Usercall
             Argument(argv)     0          1       2       3
             Argument(argc)     1          2       3       4
   Wobei Gruppe optional ist. Wenn keine Gruppe angegeben wurde, dann alle
   Gruppen auflisten.
*/

void GroupMsg ( int argc, char *argv[] )
{
  int a;
  int gefunden;
  char dname[MAXPATH];
  FILE *fp;

  if(argc == 3)
    sprintf(dname,"%s*.mbr",msgpath);
  if(argc == 4)
    sprintf(dname,"%s%s.mbr",msgpath,strlwr(argv[2]));
  if(argc > 4)
    Stop("Zuviele Parameter !");

  gefunden = xfindfirst( dname, &ffblk, 0);
  if (gefunden != FALSE)               /* wenn nicht gefunden, dann meckern */
    Stop("Gruppe nicht gefunden...");

  printf("\n");

  while (!gefunden)
  {
    sprintf(dname,"%s%s",msgpath,ffblk.ff_name);

    if (( fp = fopen(dname,"rt")) == NULL)
      Stop("Fehler #G1 Konnte Datei nicht oeffnen.");

    strcpy(dname,ffblk.ff_name);
    dname[strlen(dname)-4] = '\0';                   /* Standard IMMER .EXT */
    printf("%s :\n",strupr(dname));

    for(a = strlen(dname)+2;a > 0;a--)          /* Laenge des Gruppennamens */
    {
       printf("-");                                       /* unterstreichen */
    }
    printf("\n");                              /* und dann ..... return !!! */

    do                                                    /* Datei ausgeben */
    {
      fgets(dname,79,fp);
      printf("%s",dname);
      *dname = '\0';
    }
    while (!feof(fp));

    printf("\n");

    fclose (fp);
    gefunden = xfindnext(&ffblk);
  }
  Status= TRUE;
}

/*============================================================================
                                    MSG_LIN.C
============================================================================*/

/*--------------------------------------------------------------------------*/
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
      printf("Bitte Hilfe anfordern mit 'MSG H'.\n");
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
  for (i = 0; i < a; i++)            /* AeAe pruefen ob Zeichen in Call Ok AeAe */
  {
    Call[i] = tolower(Call[i]);                   /* fuer die Ascii-Ausgabe */

      /* Ae alle Zeichen die in der ASCII-Tabelle ueber Z, zwischen 9 und A
                            sowie unter 0 liegen, machen Call ungueltig ! Ae */

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
  if ((xfindfirst (dname, &ffblk,0)) == 0)               /* einwenig Arbeit */
    Gruppe = TRUE;

  if (!Gruppe && (strlen(Call) < 4 || strlen(Call) > 6 || Zahl < 1
                  || Zahl > 2 || falschesZ > 0))
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

  sprintf ( hilfstring,"%smsg.usr",msgpath);            /* Hilfedatei da ?? */

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
    strcpy ( Usercall, Call);

  strupr(Usercall);

  printf("\nDigimail fuer TheNetNode!\n");

  switch (Befehl)
  {
    case 'e':
    case 'l':
    case 'r': ListMsg(Befehl,argc,argv);
              break;
    case 's': if ( argc < 4 )
                Stop("Zuwenig Parameter!\n");
              else
                SendMsg(argc,argv);
              break;

    case 'g': GroupMsg(argc,argv);
              break;

    case 'h': HelpMsg();
              break;

    case 'v': VerMsg();
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
