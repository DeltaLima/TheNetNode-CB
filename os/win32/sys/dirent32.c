#include "dirent32.h"

/* Verzeichniss offnen. */
DIR *opendir(const char *dir)
{
  DIR *dp;
  char *filespec;
  long handle;
  int index;

  filespec = malloc(strlen(dir) + 2 + 1);
  strcpy(filespec, dir);
  index = strlen(filespec) - 1;
  if (index >= 0 && (filespec[index] == '/' || filespec[index] == '\\'))
    filespec[index] = '\0';

  strcat(filespec, "/*");

  dp = (DIR *)malloc(sizeof(DIR));
  dp->offset = 0;
  dp->finished = 0;
  dp->dir = strdup(dir);

  if ((handle = _findfirst(filespec, &(dp->fileinfo))) < 0)
  {
    if (handle == -1)
    {
      closedir(dp);
      return NULL;
    }

    if (errno == ENOENT)
      dp->finished = 1;
    else
    {
      closedir(dp);
      return NULL;
    }
  }

  dp->handle = handle;
  free(filespec);

  return dp;
}

/* Dateien aus Verzeichnis lesen. */
struct dirent *readdir(DIR *dp)
{
  if (!dp || dp->finished)
    return NULL;

  if (dp->offset != 0)
  {
    if (_findnext(dp->handle, &(dp->fileinfo)) < 0)
    {
      dp->finished = 1;
      return NULL;
    }
  }
  dp->offset++;

  strncpy(dp->dent.d_name, dp->fileinfo.name, _MAX_FNAME);
  dp->dent.d_ino = 1;
  dp->dent.d_reclen = strlen(dp->dent.d_name);
  dp->dent.d_off = dp->offset;

  return&(dp->dent);
}

/* Verzeichnis schliessen. */
int closedir(DIR *dp)
{
  if (!dp) 
    return 0;

  _findclose(dp->handle);

  if (dp->dir)
    free(dp->dir);

  if (dp)
    free(dp);

  return 0;
}

/* End of os/win32/dirent.c */
