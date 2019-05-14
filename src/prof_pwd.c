
TRILLIAN
GetPasswd(WORD *laenge, char **inbuf, int Len, char *outbuf)
{
  char  Buffer[80 + 1];
  char *bufpoi;
  char  zeichen;
  WORD  i;
  char *p = *inbuf;
  WORD  n = *laenge;

  skipsp(&n, &p);

  bufpoi = Buffer;
  i = 0;
  while (n > 0)
  {
    if (((zeichen = (char)(*p)) == ' ' ))
      break;

    if (i++ == Len - 1)
      return(ERRORS);

    *bufpoi++ = zeichen;
    ++p;
    --n;
  }

  if (i == 0)
    return(NO);

  Buffer[i] = 0;

  memset(outbuf, 0, sizeof(outbuf));

  strncpy(outbuf, Buffer, NAMESIZE);
  *laenge = n;
  *inbuf  = p;

  return(YES);
}


/* Eine SIMPLE Kodierung eines Strings.      */
/* Koennte man spaeter noch weiter ausbauen! */
void PasswdKodierung(const char *in, char *out)
{
  char passwd[80 + 1] = DEFAULT_PASS;
  int  i,
       a = 0;
  int  j = 0;

  /* String leer? */
  if (in[0] == FALSE)
  {
    out[0] = 0;
    return;
  }

  j = strlen(in);

  if (  (j < 5)
      ||(j > 81))
  {
    out[0] = 0;
    return;
  }

  /* Frischer Buffer. */
  memset(out, 0, sizeof(out));

  for (i = 0; i < j; i++)
  {
    a = passwd[i] + in[i];
    out[i] = a;
  }

  /* Und zum Schluss das 0 Zeichen. */
  out[i] = 0;
  return;
}

/* Eine SIMPLE Dekodierung eines Strings.    */
/* Koennte man spaeter noch weiter ausbauen! */
void PasswdDekodierung(const char *in, char *out)
{
  char passwd[80 + 1] = DEFAULT_PASS;
  int  i,
       a = 0;
  int  j;

  /* String leer? */
  if (in[0] == FALSE)
  {
    out[0] = 0;
    return;
  }

  j = strlen(in);

  if (  (j < 5)
      ||(j > 81))
  {
    out[0] = 0;
    return;
  }

  /* Frischer Buffer. */
  memset(out, 0, sizeof(out));

  for (i = 0; i < j; i++)
  {
    a = in[i] - passwd[i];
    out[i] = a;
  }

  /* Und zum Schluss das 0 Zeichen. */
  out[i] = 0;
  return;
}

/* End of src/cvs_passwd.c */
