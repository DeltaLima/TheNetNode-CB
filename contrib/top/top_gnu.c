/*
** TOP.EXE fuer TNN 1.72 und groesser (c) DG8BR
** Eigentlicher Titel "MHTNN.EXE"
** Ein Programm um die MHEARD.TAB in TNN direkt auszuwerten
**
**
** Zum Compilieren BITTE die TOP.MAK benutzen. Turbo-C.
** Eventuell die Pfade aendern.
** Falls die Datei nicht mitgegeben wurde, hier nochmal die Parameter fuer
** den Compiler (TC1/3.0++)
** TCC -2 -mc -f- -a -d -G -O
**
** Fuer Linux habe ich kein Makefile.
** Einfach gcc -Wall top_gnu.c -o top.exe .
**
**
** Aenderungen BITTE an mich weitergeben!!
** Mein Copyright darf NICHT entfernt werden!!
**
** Dieses Programm ist PUBLIC DOMAIN, mit den Einschraenkungen durch
** die ALAS (Allgemeine Lizenz fuer Amateurfunk Software), entweder
** Version 1, veroeffentlicht von Hans Georg Giese (DF2AU),
** am 13.Oct.1992, oder (wenn gewuenscht) jede spaetere Version.
******************************************************************************
** Beginn 10.02.1995
** Anpassung auf neue MHEARD.TAB am 13.02.97 by DG1KWA
**
** neues Format der MHEARD.TAB jetzt reiner ASCII
** Aufbau : Zeit im PC-Format , RX-Bytes , TX-Bytes , Port
**          rx-rej , tx-rej , dama-verstoesse , Call
**
** Beispiel: 855398713 1083577 170775 2 1 8 2 DG9FU-2
**
** Endgueltige Anpassung auf das neue Format.
** Externe Hilfe rausgeworfen.
** Rennt nun unter GNU32 und Linux
**
** Seit der 1.73 ist Max_MH = 5000. Darum habe ich auf das
** Speichermodul COMPACT umgestellt.
**
** Ein struct line *liste benoetigt 52 Bytes. Das bedeutet es muss 240000
** Bytes freier Speicher fuer die Daten vorhanden sein. Dazu kommt noch das
** Programm.
*/

#define DATUM "28.12.2000"                        /* Letzte Aenderung DG8BR */
#define EOS '\0'
#ifdef __MSDOS__
#define Seperator '\\'
#endif
#ifdef __linux__
#define Seperator '/'
#endif
/*#define DEBUG*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#ifdef __MSDOS__
  #include <dir.h>
  #include <sys\stat.h>
#endif
#if defined(__linux__) || defined(__GO32__)
  #include <sys/dir.h>
  #include <unistd.h>
  #include <sys/stat.h>
#endif

#ifdef WIN32
#include <direct.h>
#include <sys/stat.h>

#define Seperator '\\'
#endif /* WIN32 */

/* prototypen */

int get_ftime (int);                      /* Dateizeit der MHEARD.TAB holen */
void config_lesen (void);                           /* config-file einlesen */
char grossbuchstabe (char);                        /* meine toupper-Version */
void prozent (unsigned long, unsigned long, char*);    /* Prozentberechnung */
void ssid_weg ( char* );                     /* die SSiD von Call entfernen */
void benutzung (void);                                /* Kurzhilfe ausgeben */
void einles (int, int);                                 /* Hauptleseroutine */
void addier (void);                  /* Bytes der einzelnen Call's addieren */
void sortier (int, int); /* nach Menge sortieren, Plaetze vergeben, Ausgabe */
char* punkte (char*, unsigned long);              /* Zahl mit 100ter Punkte */
void callraus (char*);                            /* Einzelausgabe von Call */
int callcmp (char*, char*, int);               /* Call mit Call vergleichen */

/* globale Variablen */

char VERSION[] = "TOP v1.06g ("DATUM") (c) DG8BR";

struct line {           /* Struktur fuer die Daten. Speicherbedarf 52 Bytes */
             struct line    *naechster;                /* naechster Eintrag */
             unsigned int   platz;
             unsigned long  gesamt;
             time_t         heard;                               /* Uhrzeit */
             unsigned long  tx;                          /* gesendete Bytes */
             unsigned long  rx;                         /* empfangene Bytes */
             unsigned int   d;                            /* Damaverstoesse */
             unsigned int   t;                         /* gesendete Rejects */
             unsigned int   r;                        /* empfangene Rejects */
             unsigned int   mhport;                             /* Digiport */
             unsigned int   flag;                             /* Call aktiv */
             char           call[10];
             char           via[10];
             int            dummi;           /* fuer die verschiedene Via's */
            } *liste;
struct line *listenanfang;          /* Eintrag der ersten Station der Liste */
time_t Dateianfang;                  /* Wann die MHEARD.TAB gegonnen wurde. */
time_t filetime;                               /* entspricht dem Listenende */
char zeitzone[3] = "UTC";                           /* Zeitzonenbezeichnung */
unsigned long max;                               /* Gesamtbytes ueber alles */
int max_user;                                   /* alle gezaehlten Benutzer */
int max_nenner;
char filename[12];
struct kanal {                         /* hier wird die config.top abgelegt */
               char name[11];
               unsigned long gesamt;
             } port[16];                                   /* Port0 -Port15 */

/*----------------------------------------------------------------------------
* Zeit der MHEARD.TAB holen. Wird als bis-Zeit genommen. Damit sollte egal
* in welcher Reihenfolge die Daten stehen, bzw. wie durcheinander sie sind.
*/
int get_ftime(int handle)
{
#ifdef WIN32
#else
        struct stat filestat;

  if (fstat(handle, &filestat) == -1)
    return(-1);
  filetime = filestat.st_mtime;                  /* filetime ist 32bit-Zahl */
#endif
  return(0);
}

/*----------------------------------------------------------------------------
** Config-File einlesen
** Wird von main aufgerufen. Rueckgabewert: Keiner
**
** Diese Datei sollte in dem TNN-Pfad stehen.
**
** Config sollte folgendermassen aussehen:
** #Beispiel fuer den Einstieg. Alle Zeilen ohne # werden eingelesen
** P0 = 70cm
*/

void config_lesen (void)
{
  FILE *ein;
  char buffer[81];
  char standard[] = "P 0";
  int a;

  if ((ein = fopen("config.top","rt")) != NULL)
  {
    while(!feof(ein))
    {
      fgets(buffer,80,ein);
      if((buffer[0] != '#') && (strlen(buffer) > 1))    /* nur lesen wenn # */
      {                                                    /* und Zeile > 1 */
        a = atoi(&buffer[1]);                           /* Portnummer holen */
        strtok(buffer," =");
        if ( buffer[0] == 'P' )
          strcpy(port[a].name ,strtok(NULL," =\n\r"));         /* Portnamen */
      }
    }
  }
  for ( a = 0; a < 16; a++)          /* PORT wird nun von 0-15 eingerichtet */
  {
    if (port[a].name[0] == EOS)       /* wenn der Port nicht von der Config */
     strcpy(port[a].name,standard);                       /* versorgt wurde */

    if ( standard[2] == '9' )           /* wenn 9 dann auf 0 setzen und das */
    {                                               /* vorherige Byte auf 1 */
      standard[1] = '1';
      standard[2] = '0';
    }
    else
      standard[2]++;                       /* wenn nicht, dann nur erhoehen */
  }
  #ifdef DEBUG
    printf("config erfolgreich.\n");
  #endif
}
/*----------------------------------------------------------------------------
** Diese Routine wurde aus dem TNN-Packet entnommen.
** Erdacht hat sie DG9AML. Von mir an TOP angepasst.
*/
void prozent (unsigned long zaehler, unsigned long nenner, char *resultstr)
{
   int  i, zaehlernull;
   char tmp[32];
   tmp[0] = EOS;
   *resultstr = EOS;
   zaehlernull = 0;
   if (nenner == 0)          /* dividiert durch 0 ergibt eine Fehlermeldung */
   {
     zaehler = 0L;
     nenner = 1L;
   }

   if (zaehler / nenner != 0)                               /* 100er Stelle */
   {
     sprintf(tmp, "%s%lu", resultstr, zaehler / nenner);
     strcpy(resultstr,tmp);
   }
   else
     zaehlernull = 1;
/* Solange der zaehler kleiner nenner ist wird der zaehler um eine 10-potenz
** hoeher. Da bei 4,3G grossen Zahlen ein Ueberlauf stattfinden wuerde,
** werden beide Zahlen um eine 10er-potenz gekuerzt.
*/
   if ( zaehler > 430000000L)
   {
     zaehler = zaehler / 10;
     nenner = nenner / 10;
   }
   zaehler = (zaehler % nenner) * 10;
   if (zaehlernull != 1 || zaehler / nenner != 0)            /* 10er Stelle */
   {
                 sprintf(tmp, "%s%lu",resultstr, zaehler / nenner);
           strcpy(resultstr,tmp);
         }
   if ( zaehler > 430000000L)
   {
     zaehler = zaehler / 10;
     nenner = nenner / 10;
   }
   zaehler = (zaehler % nenner) * 10;
   sprintf(tmp, "%s%lu", resultstr, zaehler / nenner);        /* 1er Stelle */
         strcpy(resultstr,tmp);
   if ( zaehler > 430000000L)
   {
     zaehler = zaehler / 10;
     nenner = nenner / 10;
   }
         zaehler = (zaehler % nenner) * 10;
   strcat(resultstr, ".");

   for (i = 0 ; i < 2; i++)                               /* 2 Kommastellen */
   {
     sprintf(tmp, "%s%lu", resultstr, zaehler / nenner);
     if ( zaehler > 430000000L)
     {
       zaehler = zaehler / 10;
       nenner = nenner / 10;
     }
     zaehler = (zaehler % nenner) * 10;
     strcpy(resultstr,tmp);
   }
   return;
}


/*----------------------------------------------------------------------------
**  meine Version von 'toupper'
*/

char grossbuchstabe ( char buchstabe )
{
  if ( buchstabe > 'Z' )
    buchstabe -= ' ';
  return ( buchstabe );
}
/*----------------------------------------------------------------------------
** Die SSID vom call entfernen.
** Return ist ein String ohne SSID
** Es gibt kein eigentliches Return. Es wird direkt der Eintrag via Zeiger
** geaendert.
*/
void ssid_weg (char *callmit)
{
  char *callohne;

  for (callohne = callmit; *callohne; callohne++)
    if ( *callohne == '-' )
      *callohne = EOS;
}
/*-----------------------------------------------------------------------------
** Kurzanleitung ausgeben
*/

void benutzung (void)
{
  printf("\n");
  printf("   Befehlssyntax         Bedeutung                      Beispiel\n\n");
  printf("   TOP                   alle Port, 10 Plaetze          TOP\n");
  printf("   TOP Anzahl <*>        soviel Plaetze, alle Ports     TOP 45\n");
  printf("   TOP -Port             einen Port, 10 Plaetze         TOP -0 \n");
  printf("   TOP Anzahl -Port      soviel Plaetze, ein Port       TOP 30 -0\n");
  printf("   TOP -Port Anzahl      ein Port, soviel Plaetze       TOP -0 30 \n");
  printf("   TOP Call              nur ein Rufzeichen             TOP dg8br\n");
  printf("   TOP Ca*               Alle Call's mit DL             TOP dl*\n");
  printf("   TOP -L3               Die L3HEARD auswerten\n");
  printf("   TOP -v                Version\n");
  printf("   TOP -h                dieses hier\n\n");
  printf("Weitere Hilfe gibt es mit 'Help Top'\n");            /* Hinweis auf Help */
}


/*----------------------------------------------------------------------------
** Vergleicht 2 Calls miteinander.
** Beide calls werden ohne SSId geliefert.
** Returnwert : bei Gleichheit 0, sonst 1
** call1 == das Call das gesucht wird.
** call2 == mit dem wird verglichen
** wird von addier und callraus aufgerufen
*/

int callcmp ( char *call1 , char *call2, int len )
{
  while ( len-- )                      /* Call wird zeichenweise verglichen */
    if (*call1++ != *call2++ )               /* bei Gleichheit return mit 0 */
      return (1);
    return (0);
}

/*----------------------------------------------------------------------------
** Die Gesamt-Zahl der Endsummen werden zwecks besserer Lesbarkeit
** mit 100ter Punkte versehen.
** Wird von sortier aufgerufen.
** Rueckgabe : gepunkteter String
*/

char* punkte (char *string, unsigned long all)
{
  char temp[20];                                             /* Hilfsstring */
  char *tp = temp;                                       /* Zeiger auf temp */
  int a = 0;                                         /* Zaehler fuer Punkte */
  int b = 0;                                        /* Zaehler Returnstring */

  while ( all )                  /* Solange das Ende von all nicht erreicht */
  {
#ifdef WIN32
    *tp++ = (char)((all % 10) + '0');    /* Zahl/10 und den Rest verasscien */
#else
    *tp++ = (all % 10) + '0';            /* Zahl/10 und den Rest verasscien */
#endif /* WIN32 */
    all = all / 10;                      /* Quelle muss auch geteilt werden */

    a++;
    if ( a == 3 && all != EOS)                           /* Punkt einfuegen */
    {
      *tp++ = '.';
      a = 0;
    }
  }
  *tp = EOS;

  a = strlen(temp);

  while ( a )               /* und nun kopieren, aber von hinten nach vorne */
    string[b++] = temp[--a];

  string[b] = EOS;
  return(string);
}

/*----------------------------------------------------------------------------
** Generelle Einleseroutine. Wird von main aufgerufen.
** Es wird immer fuer einen Eintrag dynamisch Speicher allociert.
** Und auch nur ein Eintrag zur Zeit gelesen.
** Der erste Speicherplatz hat auch schon gueltige Daten. Der letze Eintrag
** hat im "naechster" ein NULL stehen!! Es geht bis zum NULL-Eintrag!!
** User die im TX oder RX nichts haben, werden eingelesen aber wenn keine
** Einzelausgabe gewuenscht sofort wieder geloescht.
** Wird von main aufgerufen.
**
** Der CONFPATH wird nun vom Programm ermittelt. Es wird also bei der Linux-
** Version kein globaler CONFPATH-Eintrag mehr benoetigt. Bei der DOS(GNU32)
** wird es aber gesetzt, aber nicht genutzt. Bei der Linux-Version wird der
** Eintrag auch gesetzt, gilt aber nur fuer den TNN-Task.
**
** neues Format der MHEARD.TAB jetzt reiner ASCII
** Erster Eintrag ist der neuste Eintrag.
** Aufbau : Zeit im PC-Format , RX-Bytes , TX-Bytes , Port
**          rx-rej , tx-rej , dama-verstoesse , Call
**
** Bespiel: 855398713 1083577 170775 2 1 8 2 DG9FU-15
*/

void einles (int alles, int Gesamt)
{
  FILE *ein;
  int a = 0;
  char buffer[81];
  char file[81];
  char *fp;
  struct line *indexalt = NULL;    /* Hilfszeiger um alten Eintrag zufinden */
  int kontrolle = 0;                /* Zaehler der eingelesenen Datensaetze */
  *file = EOS;

  if(getcwd(file,80) == NULL)                           /* Pfad feststellen */
  {
    printf("\nKonnte den Pfad nicht ermitteln !\n");
    exit(0);
  }
  if(*(fp = file + strlen(file)) != Seperator)
  {
    *fp++ = Seperator;                               /* Seperator anhaengen */
    *fp = EOS;                                                /* und schulz */
  }
  strcat(file,filename);

  if (( ein = fopen (file , "rb")) == NULL)
  {
    printf ("\n%s nicht gefunden!\n", file);
    exit (0);
  }

  if (get_ftime(fileno(ein)) != 0)
    printf("\nEndezeit kann nicht ermittelt werden !\n");

  while (!feof(ein))
  {
    if ((liste = malloc(sizeof(struct line))) == NULL)
    {
      printf("Kein Speicher!!! %d Datensaetze eingelesen.\n",kontrolle);
      kontrolle = sizeof(struct line);
      printf("Eine Datenzeile benoetigt %d Bytes.\n",kontrolle);
      printf("Bitte diese Zeilen mit Digicall mir, DG8BR, zuschicken!!");
      exit(0);
    }
    kontrolle++;
    if (( fgets (buffer, 80, ein )) != NULL)
    {
      if ( buffer[0] == ':' )             /* dann wurde die MHEARD begonnen */
      {                                           /* ist ab der 1.74xx drin */
        sscanf ( buffer+1, "%lu", (unsigned long *)&Dateianfang );
        continue;
      }
      liste->via[0] = EOS;
      liste->call[0] = EOS;

      sscanf (buffer ,"%lu %lu %lu %u %u %u %u %u %s %s",
                       (unsigned long *)&liste->heard ,&liste->rx ,&liste->tx ,&liste->mhport,
                       &liste->d, &liste->r ,&liste->t ,&liste->flag,liste->call,
                       liste->via);
      if(Gesamt == 0)
      {
        if(alles == 1 && ((liste ->tx == 0L) || (liste->rx == 0L)))
        {
          free(liste);
          continue;     /* Bei Summenausgabe, nur Leute mit gueltigen Daten */
        }
      }
      if ( alles == 1 )                  /* Bei Summenausgabe, die SSId weg */
        ssid_weg (liste->call);

      liste->dummi = ' ';
      liste->gesamt = 0L;

      if ( a == 0 )
      {
        listenanfang = liste;       /* Damit wird der 1.te Eintrag gefunden */
        indexalt = liste;    /* beim 1.ten Mal muss hier gespeichert werden */
        a++;
      }
      indexalt->naechster = liste;   /* im vorherigen Eintrag den Naechsten */
      indexalt = liste;     /* eintragen. Und dann den Alten auf neu setzen */
    }
  }
  indexalt->naechster = NULL; /* Sonst wird das Ende der Liste nicht bemerkt*/
  fclose(ein);
  #ifdef DEBUG
    printf("%d Datensaetze eingelesen.\n",kontrolle);
    printf("einles erfolgreich.\n");
  #endif
}

/*----------------------------------------------------------------------------
** Alle Bytes eines Call's zusammenzaehlen. Es werden bis auf einen Listen-
** eintrag alle ungueltig gemacht.
** Ermittelt fuer die globalen Variablen max und port.gesamt die Werte.
** Wird von main aufgerufen.
**
*/

void addier (void)
{
  struct line *index = NULL;                                 /* Zieleintrag */
  struct line *indexalt = NULL;      /* vorheriger Eintrag beim Durchsuchen */

  liste = listenanfang;                               /* Und nu geht es los */
   while ( liste != NULL )
   {
     index = liste;                                      /* call uebernehmen */
     indexalt = liste;                                     /* aktuell merken */
     liste = liste->naechster;                         /* und weiterschalten */

     while ( liste != NULL)
     {
       if (!callcmp (index->call, liste->call, 6 ))        /* call gleich ?? */
       {
         if (index->mhport == liste->mhport)           /* und Port gleich ?? */
         {
              index->tx += liste->tx;                         /* a weng rechnen */
              index->rx += liste->rx;
              index->t += liste->t;
              index->r += liste->r;
              index->d += liste->d;
              indexalt->naechster = liste->naechster;    /* Listenplatz aendern */
              free ( liste );                             /* und den Alten wech */
              liste = indexalt->naechster;       /* zurueck auf aktuellen Stand */
             }
         else                              /* Call gleich, aber anderer port */
         {
           if (index->dummi != '#')        /* wenn noch nicht gekennzeichnet */
           {
             index->dummi = liste->dummi = '#';
             max_user--;              /* Jedes Call wird nur einmal gezaehlt */
           }
           indexalt = liste;
           liste = liste->naechster;                   /* und weiterschalten */
         }
       }
       else                                             /* call nicht gleich */
       {
         indexalt = liste;                                 /* aktuell merken */
         liste = liste->naechster;                     /* und weiterschalten */
       }
     }
     index->gesamt = index->tx + index->rx;          /* Ergebnisse speichern */
     index->platz = 0;

     max_user++;
     max += index->gesamt;                     /* Gesamtdurchsatz des Digi's */
     port[index->mhport].gesamt += index->gesamt;           /* Portdurchsatz */
     liste = index->naechster;                                    /* zurueck */
   }
  #ifdef DEBUG
    printf("addier erfolgreich.\n");
  #endif
}

/*----------------------------------------------------------------------------
** Suche nach der groessten Zahl und zwar nehme 1.te Zahl und vergleiche mit
** der naechsten. Ist sie groesser, nehmen, sonst naechsten Platz. Wenn Zahl
** gefunden, wird ein Platz vergeben und es beginnt von vorne. Die Liste
** wird nur nach gueltigen Listenplaetzen durchsucht. D.h. nur wenn Platz = 0
** ist.
** Hier ist nun auch die Ausgabe. Ausgegeben wird bis Anzahl erreicht. Die
** Liste wird trotzdem bis zum Ende durchsucht um die Anzahl der Benutzer
** zubekommen.
** Wird von main aufgerufen.
**
*/

void sortier (int anzahl, int ports)
{
  int a;
  int c = 0;                                                /* Ausgabeplatz */
  char f[32];
  unsigned long aktu;
  unsigned long alleszusammen = 0L;
  char tmp[15];                /* Rueckgabevariable fuer gepunkteten String */
  struct line *indexplatz = NULL;                /* hat zur Zeit das meiste */
                                                     /* und wird ausgegeben */
  struct tm *ts;

  liste = listenanfang;                                 /* von Vorne suchen */

  while ( liste != NULL )
  {
    a = 0;
    aktu = 0L;      /* und wieder auf 0, damit wieder richtig gefunden wird */

    while ( liste != NULL )   /* Es wird immer die ganze Tabelle durchsucht */
    {
#ifdef WIN32
      if((liste->platz == 0) && (liste->mhport == (unsigned int)ports || ports == 'a'))
#else
      if((liste->platz == 0) && (liste->mhport == ports || ports == 'a'))
#endif /* WIN32 */
                {                         /* platz gueltig, dann mit aktu vergleichen */
        if ((liste->gesamt) >= aktu)                  /* wenn aktu > gesamt */
        {                                    /* dann neuen aktu uebernehmen */
          aktu = liste->gesamt;                /* um die Plaetze zuerhalten */
          indexplatz = liste;           /* der hat die meisten, uebernehmen */
          a = 1;                       /* Flag setzen fuer fuendig geworden */
        }
      }
      liste = liste->naechster;
    }
    if (a == 1)                                    /* Es wurde was gefunden */
    {
      indexplatz->platz = 5000;              /* Pseudo-Platznummer vergeben */
      if ( c == 0 )                                    /* nur beim 1ten mal */
      {
        ts = localtime(&Dateianfang);              /* Zeit von Anfang holen */
        if (!strcmp(filename, "mheard.tab"))
          printf("\n\n      MHEARD-Auswertung");
        else
          printf("\n\n     L3MHEARD-Auswertung");

        printf(" vom %02d.%02d.%02d %02d:%02d %.3s",
                                                    ts->tm_mday,
                                                    ts->tm_mon+1,
                                                    ts->tm_year%100,
                                                    ts->tm_hour,
                                                    ts->tm_min,
                                                    zeitzone);

        ts = localtime(&filetime);                   /* Zeit von Listenende */

        printf(" bis %02d.%02d.%02d %02d:%02d %.3s\n",ts->tm_mday,
                                                      ts->tm_mon+1,
                                                      ts->tm_year%100,
                                                      ts->tm_hour,
                                                      ts->tm_min,
                                                      zeitzone);
        printf("\nNr. Call   Port               Rx         Tx      "
               "Summe     %%");

        if (!strcmp( filename,"mheard.tab"))
          printf(" RxRej TxRej Dama\n");

        c++;                                            /* nun Platzzaehler */
      }
      if ( ports == 'a' )                          /* Alle Ports ausgeben ? */
        prozent(indexplatz->gesamt,max, f);
      else
        prozent(indexplatz->gesamt ,port[ports].gesamt,f);
                                         /* Prozentsatz des Users berechnen */
      printf("\n%3u %-6s %-10s %10lu %10lu %10lu %5s",
      c,indexplatz->call,port[indexplatz->mhport].name,indexplatz->tx,
      indexplatz->rx,indexplatz->gesamt,f);

      if (!strcmp( filename , "mheard.tab"))
      {
        printf(" %5u %5u %4u %c",indexplatz->t,indexplatz->r,
                                  indexplatz->d,indexplatz->dummi);
        if ( indexplatz->via[0] != EOS )          /* via gibt es nicht mehr */
          printf("*");
      }
      else                                                      /* L3MHEARD */
        printf(" %2c",indexplatz->dummi);

      alleszusammen += indexplatz->gesamt;
      c++;                                               /* Anzahl erhoehen */
      liste = listenanfang;                   /* und wieder an Anfang gehen */
    }
    if ( c > anzahl )
      break;
  }
  if (c == 0 )                                            /* was gefunden ? */
  {
    if ( ports != 'a' )
      printf("\nFuer diesen Port sind keine Daten in der MHEARD-Tabelle!\n");
    else
      printf("\nKeine Daten in der MHEARD-Tabelle!\n");
  }
  else
  {
    if ( ports ==  'a' )
    {
      prozent(alleszusammen,max,f);
      printf("\n\nDiese User haben auf allen Ports %s Bytes = %s %%"
             " uebertragen.\n",punkte( tmp,alleszusammen ), f);
    }
    else
    {
      prozent(alleszusammen,port[ports].gesamt,f);
      printf("\n\nDiese User haben auf Port %d (%s) %s Bytes = %s%%"
             " uebertragen.\n",ports,port[ports].name,punkte( tmp,
               alleszusammen ),f );
    }
    printf("Insgesamt wurden %s Bytes von %d Benutzer bewegt.\n",punkte
                                       (tmp,max), (max_user));
  }
  #ifdef DEBUG
    printf("sortiert erfolgreich.\n");
  #endif
}

/*----------------------------------------------------------------------------
** Einzelaugabe eines Calls
** Seit der 1.75 gibt es im L2 keine Via-Eintraege mehr in der MHEARD.TAB.
*/

void callraus(char *suchcall)
{
  int a;
  int calllen = 0;
  char vergleichcall[10] = "0x00";
  struct line *liste;
  struct tm *ts;

  calllen = strlen (suchcall);

  for ( a = 0; a < calllen; a++ )        /* call in Grossbuchstaben wandeln */
  {
    suchcall[a] = grossbuchstabe ( suchcall[a] );
  }

  if ( suchcall[calllen -1] =='*' )  /* Wenn * vorhanden, calllen 1 weniger */
    calllen-- ;                             /* sonst wird das * mitgesucht. */
  else
    calllen = 6;

  a = 0;

  liste = listenanfang;
  while (liste != NULL)
  {
    strcpy(vergleichcall, liste->call);
    ssid_weg ( vergleichcall );
    if (!callcmp(suchcall,vergleichcall,calllen))       /* und guggen ob da */
    {
      if ( a == 0 )                                   /* nur beim erstenmal */
      {
        printf("\nDatum          Port              Rx        Tx RxRej "
               "TxRej   D Call\n");
      }
      ts = localtime ( &liste->heard ) ;
      printf("\n%02d.%02d.%02d %02d:%02d ", ts->tm_mday,
                                            ts->tm_mon+1,
                                            ts->tm_year%100,
                                            ts->tm_hour,
                                            ts->tm_min);

      printf("%-10s %9lu %9lu %5u %5u %3u %-9s",
              port[liste->mhport].name,liste->tx,liste->rx,liste->t,liste->r,
              liste->d,liste->call);
      a = 1;
    }
    liste = liste->naechster;
  }
  if ( a == 0 )
    printf("\n%s nicht gehoert!",suchcall);

  ts = localtime ( &filetime );               /* wann das letze Mal gesaved */
  printf("\n\nStand: %02d.%02d.%02d %02d:%02d %.3s\n", ts->tm_mday,
                                                       ts->tm_mon+1,
                                                       ts->tm_year%100,
                                                       ts->tm_hour,
                                                       ts->tm_min,
                                                       zeitzone);
  #ifdef DEBUG
    printf("callraus erfolgreich\n");
  #endif
}

/*----------------------------------------------------------------------------
** Aufruf ohne Parameter ergibt eine Ausgabe von 10 Eintraege von allen Ports.
** Es koennen ein bestimmter Port oder eine bestimmte Anzahl uebergeben
** werden. Oder beides gemeinsam. Reihenfolge, egal.
** Dann kann man noch ein Rufzeichen eingeben. Aber dann nur das!!
*/

int main (int argc, char *argv[])
{
  int uni;                               /* Uebergabevariable fuer max_user */
  int Ausgabe = 10;
  int Gesamt = 0;
  char Port  = 'a';                        /* Dummi fuer Ausgabe alle Ports */
  char call[10] = "\x0";
  char *port_z;                                         /* Zeiger fuer Port */

  #ifdef DEBUG
    clock_t ende;
  #endif

  strcpy (filename, "mheard.tab");

  for ( argc--, uni = 1; uni < argc; uni++ )
  {
    if (argv[uni][0] == '-')           /* erstmal die -Parameter bearbeiten */
    {
      switch (argv[uni][1] )
      {
        case 'V' :
        case 'v' : printf("\n%s\n",VERSION);
                   exit(0);
                   break;
        case 'H' :
        case 'h' :
        case '?' : benutzung ();
                   exit(0);
        case 'L' :
        case 'l' : strcpy(filename, "l3heard.tab");
                   break;
        default  : if (isdigit( argv[uni][1]))
                   {
                     port_z = argv[uni];           /* auf zeiger umkopieren */
                     port_z++;  /* einen weiterdrehen, damit - verschwindet */
                     Port = atoi(port_z);      /* aus Asci "echte" Hexwerte */
                     break;               /*machen. Kann dann nur Port sein */
                   }
                   else
                   {
                     benutzung();                     /* Kurzhilfe ausgeben */
                     exit(0);
                   }
      }
    }
    if ( isdigit (argv[uni][0] ))                                /* Zahl ?? */
      Ausgabe = atoi (argv[uni]);                     /* Anzahl der Ausgabe */

    if ( isalpha (argv [uni][0]))                           /* Buchstabe ?? */
    {
      strncpy ( call, argv[uni],9 );
      uni = argc;                        /* Bei Call wird ALLES ignoriert!! */
      if ( strlen ( call ) < 3 )     /* Es sollte mehr als 1 Buchstabe sein */
      {
        printf("\nMindestens 2 Buchstaben und eine Zahl oder * eingeben!\n");
        exit (0);
      }
    }
    if(*argv[uni] == '*')
      Gesamt = 1;                          /* Alles einlesen, aber SSId weg */
  }

  config_lesen();

  if ((port_z = getenv("TZ")) != NULL)
    strcpy(zeitzone,port_z);

  if( call[0] )                    /* wenn call angegeben, alles ignorieren */
  {
    einles(0,Gesamt);              /* MHEARD.TAB einlesen mit Nullbyteleute */
    callraus(call);                                  /* Einzelcall ausgeben */
  }
  else
  {
    einles(1,Gesamt);             /* MHEARD.TAB einlesen OHNE Nullbyteleute */
    addier();
    sortier(Ausgabe, Port);                        /* sortiert und gibt aus */
  }
  #ifdef DEBUG
    ende = clock();
    printf("Laufzeit = %ld \n",(ende*100) / 182);
  #endif
  exit(EXIT_SUCCESS);
}
