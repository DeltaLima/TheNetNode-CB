#include <string.h>

int strcasecmp(const char *Dest, const char *Source)
{
  char tDest[1024 + 1];
  char tSource[1024 + 1];

  /* Leere Strings koennen wir nicht bearbeiten. */
  if (  (Source[0] == 0)
      ||(Dest[0]   == 0))
    return(1);

  /* Sicherung */
  strncpy(tSource, Source, 1024);
  strncpy(tDest, Dest, 1024);

  /* Umwandeln in Grossbuchstaben. */
  strupr(tSource);
  strupr(tDest);

  /* Sting vergleichen. */
  if (strcmp(tSource, tDest) == 0)
    /* Strings passt. */
    return(0);
  else
    /* String stimmt nicht ueberein. */
    return(1);
}

#ifdef _MSC_VER /* Nur fuer VC */
int strncasecmp(const char *Dest, const char *Source, int len)
{
  char tSource[1024 + 1];
  char tDest[1024 + 1];

  /* Leere Strings koennen wir nicht bearbeiten. */
  if (  (Source[0] == 0)
      ||(Dest[0]   == 0))
    return(1);

  /* Sicherung */
  strncpy(tSource, Source, 1024);
  strncpy(tDest, Dest, 1024);

  /* Umwandeln in Grossbuchstaben. */
  strupr(tSource);
  strupr(tDest);

  /* Sting vergleichen. */
  if (strncmp(tSource, tDest, len) == 0)
    /* Strings passt. */ 
   return(0);
  else
    /* String stimmt nicht ueberein. */
    return(1);
}
#endif

/* End of os/win32/strings.h */
