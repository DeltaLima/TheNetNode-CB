#define _STINGS_C

#ifndef strcasecmp
int      strcasecmp(const char *, const char *);
#endif

#ifdef _MSC_VER /* Nur fuer VC */
int      strncasecmp(const char *, const char *, int);
#endif

/* End of os/win32/sys/stings.h */
