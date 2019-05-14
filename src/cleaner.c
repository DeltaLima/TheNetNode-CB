/************************************************************************/
/* Sourcen-Reiniger von DL1XAO                                          */
/*                                                                      */
/* expandiert in den gewuenschten Files die Tabulatoren, loescht        */
/* ueberfluessige Spaces und wandelt Umlaute                            */
/*                                                                      */
/* Aenderungen fuer Kommandozeilenparameter von DF6LN                   */
/*                                                                      */
/************************************************************************/

#include "tnn.h"

int     clean(char *);
void    usage(void);

int     tabs = 8;

int
main(int argc, char *argv[])
{
  char  file[MAXPATH];
  int   i = 1;
  char *sp;

  if (argc < 2)
  {
    usage();
    return (-1);
  }
  
  if (argc > 2)
  {
    if (!strcmp(argv[1], "-t"))
    {
      if (sscanf(argv[2], "%d", &tabs) != 1)
      {
        usage();
        return (-1);
      }
      if (tabs < 2 || tabs > 8)
      {
        fprintf(stderr, "TAB size out of range (2 .. 8).\n");
        return (-1);
      }
      i += 2;
      if (i == argc)
      {
        usage();
        return (-1);
      }
    }
  }

  for (; i < argc; i++)
  {
    strcpy(file, argv[i]);
    strlwr(file);
    sp = strstr(file, ".c");
    if (sp != file + strlen(file) - 2)
    {
      sp = strstr(file, ".h");
      if (sp != file + strlen(file) - 2)
      {
        fprintf(stderr, "%s has bad file extension.\n", argv[i]);
        return (-1);
      }
    }
    if (!clean(file))
      return (-1);
  }
  return (0);
}

void
usage(void)
{
  fprintf(stderr, "Usage: cleaner [-t <num>] filename [filename [...]]\n");
}

int
clean(char *fn)
{
  char  tempf[10];
  char  out[2048];
  char *co;
  int   changed = 0;
  int   n, c, lc;
  FILE *fpi;
  FILE *fpo;

  if ((fpi = xfopen(fn, "rb")) == NULL)
  {
    fprintf(stderr, "File not found: %s\n", fn);
    return (0);
  }
  strcpy(tempf, "temp");
  if ((fpo = xfopen(tempf, "w")) == NULL)
  {
    fprintf(stderr, "Can't open temp file.\n");
    fclose(fpi);
    return (0);
  }
  setvbuf(fpi, NULL, _IOFBF, 8192L);
  setvbuf(fpo, NULL, _IOFBF, 8192L);

  for (lc = 0, co = out; (c = fgetc(fpi)) != EOF; )
  {
#ifndef __LINUX__
    if (c == '\n' && lc != '\r')
      changed = 1;
#else
    if (c == '\r')
      changed = 1;
#endif

    switch (c)
    {
      case  9:    changed = 1;
                  c = ' ';
                  for (n = tabs -1 - ((co - out) % tabs); n; n--)
                    *co++ = c;
                  break;
      case  0x8e:
      case  0xc4: changed = 1;
                  *co++ = 'A';
                  c = 'e';
                  break;
      case  0x99:
      case  0xd6: changed = 1;
                  *co++ = 'O';
                  c = 'e';
                  break;
      case  0x9a:
      case  0xdc: changed = 1;
                  *co++ = 'U';
                  c = 'e';
                  break;
      case  0x84:
      case  0xe4: changed = 1;
                  *co++ = 'a';
                  c = 'e';
                  break;
      case  0x94:
      case  0xf6: changed = 1;
                  *co++ = 'o';
                  c = 'e';
                  break;
      case  0x81:
      case  0xfc: changed = 1;
                  *co++ = 'u';
                  c = 'e';
                  break;
      case  0x9e:
      case  0xdf:
      case  0xe1: changed = 1;
                  c = 's';
                  *co++ = c;
                  break;
    }

    lc = c;
    if (c != '\r' && c != '\n')
      *co++ = c;

    if (c == '\n')
    {
      while (co > out && *(co - 1) == ' ')
      {
        co--;
        changed = 1;
      }
      *co++ = '\n';
      *co = '\0';
      fputs(out, fpo);
      co = out;
    }
  }

  fclose(fpi);
  fclose(fpo);
  if (changed)
  {
    unlink(fn);
    rename(tempf, fn);
    fprintf(stderr, "Changed version of %s saved.\n", fn);
  }
  else
    unlink(tempf);
  return (-1);
}

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
  return (filename);
}

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Eine Datei oeffnen, es wird darauf geachtet, das einige Betriebs-    */
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
  else
    if (strchr(mode, 'a'))         /* zum Anhaengen                     */
      *fm++ = 'a';
    else
      *fm++ = 'r';                 /* sonst zum Lesen oeffnen           */
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

  *fm = 0;                         /* String terminieren                */
  return (fopen(fname, fmode));    /* Datei oeffnen                     */
}

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Eine Datei loeschen, es werden ALLE Dateien geloescht (auch RDONLY)  */
/* normfname() wird zur Normierung des Dateinamen benutzt.              */
/*                                                                      */
/*----------------------------------------------------------------------*/
int
xremove(const char *filename)
{
 char fname[MAXPATH+1];

  strcpy(fname, filename);         /* umkopieren, den Originalnamen     */
  normfname(fname);                /* nicht anfassen                    */
  return (remove(fname));
}

/*----------------------------------------------------------------------*/
/*                                                                      */
/* Eine Datei umbenennen.                                               */
/* normfname() wird zur Normierung des Dateinamens benutzt.             */
/*                                                                      */
/*----------------------------------------------------------------------*/
int
xrename(const char *oldname, const char *newname)
{
 char fname[MAXPATH+1];
 char fname2[MAXPATH+1];

  strcpy(fname, oldname);         /* umkopieren, den Originalnamen     */
  normfname(fname);               /* nicht anfassen                    */
  strcpy(fname2, newname);        /* umkopieren, den neuen Namen       */
  normfname(fname2);              /* nicht anfassen                    */
  return (rename(fname,fname2));
}
