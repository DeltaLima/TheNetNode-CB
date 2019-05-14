#include <io.h>
#include <sys/types.h>
#include <errno.h>
#include "fnmatch.h"
#include <stdlib.h>
#include <string.h>

struct dirent
{
  long d_ino;                 /* inode (always 1 in WIN32) */
  off_t d_off;                /* offset to this dirent */
  unsigned short d_reclen;    /* length of d_name */
  char d_name[_MAX_FNAME+1];  /* filename (null terminated) */
};

typedef struct
{
  long handle;                  /* _findfirst/_findnext handle */
  short offset;                 /* offset into directory */
  short finished;               /* 1 if there are not more files */
  struct _finddata_t fileinfo;  /* from _findfirst/_findnext */
  char *dir;                    /* the dir we are reading */
  struct dirent dent;           /* the dirent to return */
} DIR;

DIR           *opendir(const char *);
int            closedir(DIR *);
struct dirent *readdir(DIR *);
void           rewinddir(DIR *);

/* End of os/win32/dirent.h */
