#include "tnn.h"

#ifdef L1HTTPD

/* Interfacezeiger auf HTTPD. */
static T_INTERFACE *ifhtt = &ifp[HTP_ID];
/* Zeiger auf das Aktuelle Interface. */
static T_INTERFACE *ifpp;

enum { H_GET, H_HEAD, H_POST } method;
enum { NONE, BIN, TXT, GIF, JPG, WAV, MP3} h_mimetype;

static char  h_inbuf[400];
static char  h_cookie_login[30];
static char  h_cookie_pw[16+1];
static long  h_contentlength;
static char *h_content;
static int   h_status;
static char  h_file[80];
static char  h_uri[200];
static char  h_login[30];
static char  h_pw[17];
static int   h_nocookie;
static char  h_header[HEADLEN];
static char  h_userpass[50];
static char  h_referer[100];
static char  h_host[100];
static int   h_version;


/* Buffer besorgen und Socket eintragen. */
MBHEAD *SetBufSock(void)
{
  MBHEAD *tbp = NULL;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(SetBufSock)";
#endif /* DEBUG_MODUS */

  /* kein Speicher. */
  if (nmbfre < 300)
  {
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(SetBufSock):%s\nSpeicher (%d) ist voll!\n"
                  , tcppoi->ip
                  , nmbfre);
    /* Kein Buffer zur Verfuegung. */
    tcppoi->disflg |= 0x80;               /* Segment auf Disconnect setzen. */
    return(NULL);
  }

  /* Buffer besorgen. */
  if ((tbp = (MBHEAD *) allocb(ALLOC_L1HTTPD_RX)) == NULL)
  {
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL1(TRUE, "(SetBufSock):%s\nSpeicher (%d) ist voll!\n"
                  , tcppoi->ip
                  , nmbfre);
    /* Kein Buffer zur Verfuegung. */
    tcppoi->disflg |= 0x80;               /* Segment auf Disconnect setzen. */
    return(NULL);
  }

  return(tbp);
}

/* HTTPD-Server Einstellung aendern/setzen. */
void ccphttpd(void)
{
  MBHEAD *mbp;
  char    ch;
  int     tmp_fd         = EOF;
  int     newloglevel    = 0;
  int     new_tcp_port   = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(ccphttpd)";
#endif /* DEBUG_MODUS */

  /* Zeiger auf das aktuelle Interface? */
  if (ifhtt == NULL)
    /* Dann Zeiger auf das HTTPD-Interface. */
    ifpp = &ifp[HTP_ID];
  else
    ifpp = ifhtt;

  /* Sysop will aendern?  */
  if (issyso() && skipsp(&clicnt, &clipoi))
  {
    clicnt--;
    ch = toupper(*clipoi++);
    switch (ch)
    {
    /* Sysop will httpd-Port aendern */
      case 'P':
        if (!skipsp(&clicnt, &clipoi))
        {
          putmsg("Invalid Parameter\r");
          return;
        }

        /* Neuen TCP-Port einlesen. */
        new_tcp_port = nxtlong(&clicnt, &clipoi);

        if (  (new_tcp_port < 1)
            ||(new_tcp_port > 65535))
        {
#ifdef SPEECH
          putmsg(speech_message(328));
#else
          putmsg("HTTPD-Port not valid, not changed !!!\r");
#endif
          return;
        }

        /* Wenn NEUER TCP-Port und ALTER TCP-Port */
        /* GLEICH sind, brauchen wir nix aendern. */
        if (ifpp->tcpport == Htons((unsigned short)new_tcp_port))
        {
#ifdef SPEECH
          putmsg(speech_message(330));
#else
          putmsg("TCP-Port successfully changed\r");
#endif
          return;
        }

        /* Ist HTTPD-Server aktiv ? */
        if (ifpp->actively == FALSE)
        {
#ifdef SPEECH
          putmsg(speech_message(327));
#else
          putmsg("TCP-Port haven not switched on!\r");
#endif
          return;
        }

        if ((tmp_fd = SetupTCP(ifpp->name, (unsigned short)new_tcp_port)) != EOF)
        {
#ifdef OS_STACK
          int tmp_fd_OS;

          if ((tmp_fd_OS = SetupOS(ifpp, (unsigned short)new_tcp_port)) != EOF)
          {
            /* alten Socket schliessen */
            close(ifpp->OsSock);
            /* Neuen Socket merken */
            ifpp->OsSock = tmp_fd_OS;
          }
          else
            {
              Close(tmp_fd);
              putmsg("ERROR: Changing UDP-Port failed, Port not changed !!!\r");
              return;
            }
#endif /* OS_STACK */

          Close(ifpp->ISock);                   /* Alten Socket schliessen. */
          ifpp->ISock = tmp_fd ;                     /* Neuen Socket merken */
        }
        else
          {              /* Neuen TCP-Port liess sich nicht Initialisieren. */
            putmsg("ERROR: Changing UDP-Port failed, Port not changed !!!\r");
            return;
          }


        /* TCP-Port OK, dann markieren wir neuen TCP-Port. */
        ifpp->tcpport = Htons((unsigned short)new_tcp_port);

#ifdef SPEECH
        putmsg(speech_message(330));
#else
        putmsg("TCP-Port successfully changed\r");
#endif
        return;


      /* Sysop will LOGLevel aendern */
      case 'L':
        if (!skipsp(&clicnt, &clipoi))
        {
          /* Keine Parameter angegeben, */
          /* status Loglevel anzeigen.  */
          mbp = putals("HTTPD-Server:\r");
#ifdef SPEECH
          putprintf(mbp, speech_message(299),ifpp->log);
#else
          putprintf(mbp, "My LogLevel: %d\r",ifpp->log);
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }

        /* Markiere neuen Loglevel */
        newloglevel = nxtnum(&clicnt, &clipoi);

        if (  (newloglevel < 0)
            ||(newloglevel > 3))
        {
          mbp = putals("HTTPD-Server:\r");
#ifdef SPEECH
          putprintf(mbp, speech_message(325));
#else
          putprintf(mbp, "Fehler: Loglevel werte von 0 bis 3!\r");
#endif
          prompt(mbp);
          seteom(mbp);
          return;
        }


        /* Neuen Loglevel Sysop zeigen */
        ifpp->log = newloglevel;
        mbp = putals("HTTPD-Server:\r");
#ifdef SPEECH
        putprintf(mbp, speech_message(299), ifpp->log);
#else
        putprintf(mbp, "My Loglevel: %d\r", ifpp->log);
#endif
        prompt(mbp);
        seteom(mbp);
        return;

       break;


      default:
        putmsg("Invalid Paramter\r");
        return;
    }
  }

  mbp = putals("HTTPD-Server:\r");
#ifdef SPEECH
  putprintf(mbp, speech_message(299), ifpp->log);
  putprintf(mbp, speech_message(326), Ntohs(ifpp->tcpport));
#else
  putprintf(mbp, "My Loglevel: %u\r", ifpp->log);
  putprintf(mbp, "My TCP-Port: %u\r", Ntohs(ifpp->tcpport));
#endif
  prompt(mbp);
  seteom(mbp);
}

/* Schreibe Konverierung in Buffer. */
static void putf(MBHEAD *tbp, char *format, ...)
{
  va_list argpoint;
  char    cbuf[260];
  char    *s=cbuf;
  int     i = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(putf)";
#endif /* DEBUG_MODUS */

  va_start(argpoint,format);
  vsprintf(cbuf,format,argpoint);
  va_end(argpoint);

  while(*s && ++i != HEADLEN)
    putv(tbp, *(s++));
}

/* Konvertierung IBM/HTML. */
void putv( MBHEAD *tbp, int c)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1http(putv)";
#endif /* DEBUG_MODUS */

  if(tcppoi && tcppoi->http == TCP_CMD)
  {
    switch(c)
    { /* Conversion IBM -> HTML */
      case '>':
        putf(tbp, "&gt;");
      return;

      case '<':
        putf(tbp, "&lt;");
      return;

      case 0x84:
        putf(tbp, "&auml;");
      return;

      case 0x94:
        putf(tbp, "&ouml;");
      return;

      case 0x81:
        putf(tbp, "&uuml;");
      return;

      case 0x8e:
        putf(tbp, "&Auml;");
      return;

      case 0x99:
        putf(tbp, "&Ouml;");
      return;

      case 0x9a:
        putf(tbp, "&Uuml;");
      return;

       case 0xe1:
         putf(tbp, "&szlig;");
       return;
    }
  }

  if(c == '\n')
    putchr(CR, tbp);

  putchr((char)c, tbp);
}

static void base64bin(char *in, char *out, int maxlen)
{
  int  i,a,end  = FALSE;
  long outword  = FALSE;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(base64bin)";
#endif /* DEBUG_MODUS */

  while (in[0] && end==0 && maxlen > 3)
  {
    for (i = 0; i < 4; i++)
    {
      if (in[i] == 0)
        end = 1;

      outword <<= 6;
      a = in[i];

      if (isalpha(a) && isupper(a))
        a -= ('A');
      else
        if (isalpha(a) && islower(a))
          a-= ('a' - 26);
        else
          if (isdigit(a))
            a += 4;
          else
            if (a == '+')
              a = 62;
            else
              if (a == '/')
                a = 63;
              else
                a = 0;

      outword |= a;
    }

    out[0] = (outword >> 16) & 255;
    out[1] = (outword >>  8) & 255;
    out[2] = (outword)       & 255;
    in += 4;
    out += 3;
    maxlen -= 3;
  }
  out[0] = 0;
}

static void get_authorization(char *buf ,BOOLEAN cookie, MBHEAD *tbp)
{
  char *search;
  char *basic;
  char  loginpw[50];
  char *locpw;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(get_authorization)";
#endif /* DEBUG_MODUS */

  if(cookie)
    search="TheNetNode";
  else
    search="Basic";

  basic = strstr(buf, search);

  if(basic)
  {
    basic += (strlen(search) + 1);
    if(strlen(basic) > 50)
      basic[50] = 0;

    strncpy(h_userpass, basic, 50);
    base64bin(basic, loginpw, 49);
    locpw = strchr(loginpw, ':');

    if(locpw)
    {
      locpw[16 + 1] = 0;
      if(cookie)
        strncpy(h_cookie_pw, locpw + 1, 17);
      else
        strncpy(h_pw, locpw + 1, 17);

      locpw[0] = ' ';
    }

    locpw = strchr(loginpw, ' ');
    if(locpw)
      locpw[0] = 0;

    loginpw[16] = 0;
    if(cookie)
      strncpy(h_cookie_login, loginpw, 30);
    else
      {
        strncpy(h_login, loginpw, 30);
        putprintf(tbp, "%s", h_login);
      }
  }
}

static int blkill(char *bf)
{
 int i = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(blkill)";
#endif /* DEBUG_MODUS */

 while (bf[i] == 13 || bf[i] == ' ' || bf[i] == ',')
   i++;

 return i;
}

static char *blankweg(char *s)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1http(blankweg)";
#endif /* DEBUG_MODUS */

  if (s == NULL)
    return NULL;

  s = strchr(s,' ');
  if (s)
  {
    s[0] = 0;
    s += 1;
    s += blkill(s);
    return s;
  }

  return s;
}

static char *stristr(char *s1, char *s2)
{
  int i;
  int l = strlen(s2);
#ifdef DEBUG_MODUS
  lastfunc = "l1http(stristr)";
#endif /* DEBUG_MODUS */

  while (s1[0])
  {
    i = 0;
    while ((((s1[i] ^ s2[i]) &0x5f) == 0) && s2[i])
      i++;

    if(i == l)
      return s1;

    s1++;
  }

  return NULL;
}

static char *ht_time(time_t t)
{
  static char zeitdatum[40];
#ifdef DEBUG_MODUS
  lastfunc = "l1http(ht_time)";
#endif /* DEBUG_MODUS */

  strftime(zeitdatum,sizeof(zeitdatum), "%a, %d-%b-%Y %H:%M:%S GMT",localtime(&t));
  return(zeitdatum);
}

static long filesize(char *name)
{
  struct stat st;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(filesize)";
#endif /* DEBUG_MODUS */

  if (stat(name, &st) == 0)
    return st.st_size;
  else
    return 0L;
}

static time_t filetime(char *name)
{
  struct stat st;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(filetime)";
#endif /* DEBUG_MODUS */

  if (stat(name, &st) == 0)
    return st.st_mtime;
  else
    return 0L;
}

static void form_referer(void)
{
  char tmp[80];
  char *hostpos=stristr(h_referer,h_host);
#ifdef DEBUG_MODUS
  lastfunc = "l1http(form_referer)";
#endif /* DEBUG_MODUS */

  if(h_referer[0] && h_host[0] && hostpos)
  {
    if ((strlen(hostpos+strlen(h_host))>79))
#ifdef SPEECH
      printf(speech_message(339));
#else
      printf("form_referer str too long");
#endif
    strncpy(tmp, hostpos+strlen(h_host), 80);
    strncpy(h_referer, tmp, 80);
  }
  if(h_referer[0]==0)
    strcpy(h_referer, "/");
}

static void get_field(char *buf,char *dest,int maxlen)
{
  char *dop=strchr(buf,':');
#ifdef DEBUG_MODUS
  lastfunc = "l1http(get_field)";
#endif /* DEBUG_MODUS */

  if(dop)
  {
    dop++;

    while(dop[0] == ' ')
      dop++;

    if(strlen(dop)>=(unsigned)maxlen)
      dop[maxlen-1] = FALSE;

    strncpy(dest,dop, 100);
  }
  else
    dest[0] = FALSE;
}

static void get_contentlength(char *buf)
{
  char *dop=strchr(buf,':');
#ifdef DEBUG_MODUS
  lastfunc = "l1http(get_contentlength)";
#endif /* DEBUG_MODUS */

  if(dop)
    h_contentlength=atol(dop+1);
}

static void get_postarea(char *tag,char *result,int max,int wrap)
{
  int i;
  char *firstresult;
  char *found;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(get_postarea)";
#endif /* DEBUG_MODUS */

  result[0] = FALSE;
  firstresult=result;

  if(h_content==NULL)
      return;

  found=stristr(h_content,tag);
  if(found)
  {
    found+=strlen(tag);
    while(found[0] && found[0]!='&' && max)
    {
      if(found[0]=='+')
        result[0]=' ';
      else if(found[0]=='%')
      { char hex[3];
        unsigned val;
        hex[0]=found[1];
        hex[1]=found[2];
        hex[2]=0;
        sscanf(hex,"%2X",&val);
        found+=2;
        if(val==13)
        {
        found++;
        continue;
        }
        result[0]=val;
      }
      else
        result[0]=found[0];
      result++;
      found++;
      max--;
      result[0]=0;
    }
  }
  /* do word wrapping since the browser seems not to be able to do it */
  if(wrap)
  {
    result=firstresult;
    while(result[0])
    {
    if(result[0]=='\n')
      {
      result++;
      continue;
      }
      for(i=0;i<wrap && result[i] && result[i]!='\n';i++);
      if(i==wrap)
      {
      while(i && result[i]!=' ')
          i--;
        if(i==0)
          i=wrap;
        else
          result[i]='\n';
      }
      result+=i;
    }
  }
  result=firstresult;
  /* convert ANSI-Umlaut to IBM */
  for(;result[0];result++)
  {
  switch(result[0])
    {
    case 0xe4: result[0]=(char)0x84;
               break; /*ae*/
    case 0xf6: result[0]=(char)0x94;
               break; /*oe*/
    case 0xfc: result[0]=(char)0x81;
               break; /*ue*/
    case 0xc4: result[0]=(char)0x8e;
               break; /*Ae*/
    case 0xd6: result[0]=(char)0x99;
               break; /*Oe*/
    case 0xdc: result[0]=(char)0x9a;
               break; /*Ue*/
    case 0xdf: result[0]=(char)0xe1;
               break; /*ss*/
    }
  }
}

/* Status, Login-daten einlesen. */
static void read_data(MBHEAD *data, char *buffer)
{
  char  c;
  int   i = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(read_data)";
#endif /* DEBUG_MODUS */

  while (data->mbpc != data->mbgc && i != 300)
  {
   /* Hole Zeichen aus den Buffer. */
   c = getchr(data);
   /* Zeichen ein CR-Return? */
   if (c == CR)
    /* Eingelesene Zeichen zur weiteren Auswertung. */
    break;

   buffer[i++] = c;
  }
  /* Nullzeichen setzen. */
  buffer[i++] = '\0';

}

/* Status, Login auswerten. */
static void GetRequest(MBHEAD *data)
{
  MBHEAD *rxcall;
  char    locinbuf[301];
  char   *locmethod;
  char   *locuri;
  char   *locprotocol;
  char   *slash;
  char   *ext;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(GetRequest)";
#endif /* DEBUG_MODUS */

  /* Buffer besorgen. */
  if ((rxcall = SetBufSock()) == NULL)
    /* Buffer voll. */
    return;

  /* Status, Login-Daten zurueckspulen. */
  rwndmb(data);
  /* Status, Login-Daten einlesen. */
  read_data(data, locinbuf);

  /* ggf. Logbuch fuehren. */
  ifpp = SetInterface(tcppoi->Interface);
  T_LOGL3(TRUE, "(GetRequest):%s\nInput: %s"
                , tcppoi->ip
                , locinbuf);

  strncpy(h_inbuf, locinbuf, 400);
  locmethod = locinbuf;
  locuri = blankweg(locmethod);
  locprotocol = blankweg(locuri);
  blankweg(locprotocol);

  h_contentlength=0L;
  h_content=NULL;
  h_status=200;
  h_version=9;             /* default if no protocol is given */
  h_file[0]=0;
  h_login[0]=0;
  h_pw[0]=0;
  h_cookie_login[0]=0;
  h_cookie_pw[0]=0;
  h_nocookie=0;
  h_referer[0]=0;  /*deti 16-jul-97*/
  h_mimetype=NONE; /*deti 15-jul-97*/

  if(locprotocol)
  {
    slash = strchr(locprotocol, '/');
    if (slash && isdigit(slash[1]))
      h_version = (slash[1]-'0') * 10;

    if (slash && isdigit(slash[3])) /* DH3MB: Added "slash &&"*/
      h_version += (slash[3] - '0');
  }

  h_uri[0] = 0;
  if (locuri)
    strncpy(h_uri, locuri, 200);

  if (h_uri[0] != '/')
    h_status = 400;

  if (stricmp(locmethod, "GET") == 0)
    method = H_GET;
  else
    if (stricmp(locmethod, "HEAD") == 0)
      method = H_HEAD;
  else
    if (stricmp(locmethod, "POST") == 0)
      method = H_POST;
  else
    h_status = 501;

  if ((ext = strchr(h_uri,'.')) == 0)
  {
    if(h_file[strlen(h_file)-1]=='/')
      h_file[strlen(h_file)-1]=0;
  }

  if (ext != NULL)
  {
    if (  (strstr(h_uri, "htm")
        ||(strstr(h_uri, "php"))) == 0)
    {
      if (strncmp(locuri, "/./", 3) == FALSE)
        h_status = 401;

      strcat(h_file, locuri);

      if(h_file[strlen(h_file)-1] == '/')
        h_file[strlen(h_file)-1] = FALSE;
    }
  }

  /* Abmelden */
  if(stricmp(h_uri, "/logout") == FALSE)
  {
    /* Neuen Status setzen (Logout). */
    h_status   = 901;
    /* Name loeschen. */
    h_login[0] = 0;

    /* Buffer fuer Rufzeichen gefuellt? */
    if (rxcall != NULL)
      /* Buffer freigeben. */
      dealmb(rxcall);

    return;
  }

  if(locprotocol && stristr(locprotocol, "HTTP/") == locprotocol)
  {
    do
    {
      /* Status, Login-daten einlesen. */
      read_data(data, locinbuf);

      /* ggf. Logbuch fuehren. */
      ifpp = SetInterface(tcppoi->Interface);
      T_LOGL3(FALSE, "%s",locinbuf);

      if(locinbuf[0])
      {
        if(stristr(locinbuf, "Authorization") == locinbuf)
          get_authorization(locinbuf, FALSE, rxcall);

        if(stristr(locinbuf, "Cookie") == locinbuf)
          get_authorization(locinbuf, TRUE, rxcall);

        if(stristr(locinbuf, "Referer") == locinbuf)
          get_field(locinbuf, h_referer, sizeof(h_referer));

        if(stristr(locinbuf, "Host") == locinbuf)
          get_field(locinbuf, h_host, sizeof(h_host));

        if(stristr(locinbuf, "Content-length") == locinbuf)
          get_contentlength(locinbuf);

      }
    }

    while(locinbuf[0]);

    if(h_contentlength)
    {
      long i;

      if (h_contentlength > 10000000L)
        printf("httpd::start_http content too long");

      h_content = (char *)malloc(h_contentlength * 2);

      for (i = 0; i < h_contentlength; i++)
        h_content[i] = ReadSockTCP();

      h_content[i] = 0;
    }
  }

  if(h_status != 200)
    h_uri[0] = FALSE;

  locuri = strchr(h_uri, '?');
  if (locuri)
  {
    if(h_content == NULL)
    {
      locuri[0] = FALSE;
      h_content = locuri + 1;
    }
  }
  form_referer();

  if (ext != NULL)
  {
    if (strncasecmp(ext, ".htm", 4))
    {
      h_status = 200;

      if (rxcall != NULL)
        dealmb(rxcall);

      return;
    }
  }

  /* Anmelden */
  if(stricmp(h_uri, "/login") == FALSE)
  {
    /* noch keine Anmeldung. */
    if (h_login[0] == FALSE)
    {
      /* Neuen Status setzen. */
      h_status = 401;

      /* Buffer fuer Rufzeichen gefuellt? */
      if (rxcall != NULL)
        /* Buffer entsorgen. */
        dealmb(rxcall);

      return;
    }
  }

  if (h_login[0] != FALSE)
  {
    /* User/Link anmelden. */
    if (LoginTCP(rxcall))
    {
      /* Buffer fuer Rufzeichen gefuellt? */
      if (rxcall->owner != ALLOC_NO_OWNER)
        /* Buffer entsorgen. */
        dealmb(rxcall);

      /* Login war erfolglos, status setzen. */
      h_status = 401;
    }
  }

  /* Will User einen Befehl ausfuehren? */
  if (stricmp(h_uri, "/cmd") == 0)
  {
    if (h_login[0] == FALSE)
    {
      /* Buffer fuer Rufzeichen gefuellt? */
      if (rxcall->owner != ALLOC_NO_OWNER)
        /* Buffer entsorgen. */
        dealmb(rxcall);

      /* Neuen Status setzen. */
      h_status = 401;
    }
  }

  h_nocookie = 1;

  /* Buffer fuer Rufzeichen gefuellt? */
  if (rxcall->owner != ALLOC_NO_OWNER)
    /* Buffer entsorgen. */
    dealmb(rxcall);
}

/* aktuelle Zeit setzen. */
static time_t ad_time(void)
{
  time_t newvalue;
  return(newvalue=time(NULL));
}

/* Aktuellen Header in Buffer schreiben. */
static void PutHeader(MBHEAD *tbp, char *head, BOOLEAN frame)
{
  char *s = h_header;
  WORD  j = 0, i = strlen(s);
  char  call[10];
#ifdef DEBUG_MODUS
  lastfunc = "l1http(PutHeader)";
#endif /* DEBUG_MODUS */

  while (j != i && j != HEADLEN)
    /* Zeichen in Buffer schreiben. */
    putchr(s[j++], tbp);

  /* Zusaetzlichen HTML-Code? */
  if (!frame)
    /* Nein, dann abbruch. */
    return;

  call2str(call, myid);
  putprintf(tbp, "<html><head><title>%s - %s</title>\n",call,head);
  putprintf(tbp, "<link rel=stylesheet type=text/css href=\"/tnn.css\"></head>\n");
  putprintf(tbp, "<body>\n");
}

/* HTML-Header erstellen. */
static int PrepareHeader(MBHEAD *tbp)
{
  struct
  {
    int status;
    char *phrase;
  } st_tab[] = {
               { 200,"OK" },
               { 201,"Created" },
               { 202,"Accepted" },
               { 204,"No Content" },
               { 301,"Moved Permanently" },
               { 302,"Moved Temporarily" },
               { 304,"Not Modified" },
               { 400,"Bad Request" },
               { 401,"Unauthorized" },
               { 403,"Forbidden" },
               { 404,"Not Found" },
               { 500,"Internal Server Errror" },
               { 501,"Not Implemented" },
               { 502,"Bad Gateway" },
               { 503,"Service Unavailable" },
               { 901,"Logout" },
               { 0,"Unknown" }
  };

  int   st;
  char *mimestr;
  char *h=h_header;
  char  call[10];
#ifdef DEBUG_MODUS
  lastfunc = "l1http(PrepareHeader)";
#endif /* DEBUG_MODUS */

  /* Konvertiere MYCALL. */
  call2str(call, myid);

  for(st = 0; st_tab[st].status; st++)
  {
    if(st_tab[st].status == h_status)
      break;
  }

  if (h_status == 901)
    h_status = 401;

  if(h_version > 9)
  {
    h += sprintf(h,"HTTP/1.0 %d %s\n",h_status,st_tab[st].phrase);
    h += sprintf(h,"Date: %s\n",ht_time(ad_time()));
    h += sprintf(h,"Server: TheNetNode V1.79\n");

    /* Login war erfolglos. */
    if (h_status == 401)
    {
      h += sprintf(h,"WWW-Authenticate: Basic realm=\"%s\"\n",call);
      h += sprintf(h,"Set-Cookie: %s=; path=/; expires=%s\n",call,ht_time(0));
    }
    else
      if(strcmp(h_uri, "/") == 0 && h_status == 200 && h_pw[0] && h_nocookie == 0)
        h += sprintf(h,"Set-Cookie: %s=%s; path=/; expires=%s\n",
                     call,h_userpass,ht_time(ad_time()+3600*24*30));

    /* Kein Mime-Type gesetzt. */
    if(h_mimetype == NONE)
    {
      if(stristr(h_uri, ".jpg"))
        h_mimetype = JPG;

      if(stristr(h_uri, ".gif"))
        h_mimetype = GIF;

      if(stristr(h_uri, ".wav"))
        h_mimetype = WAV;

      if(stristr(h_uri, ".mp3"))
        h_mimetype = MP3;

      if(stristr(h_uri, ".tar"))
        h_mimetype = BIN;

      if(stristr(h_uri, ".zip"))
        h_mimetype = BIN;
    }

    /* Content-type stzen. */
    switch(h_mimetype)
    {
      case JPG: mimestr = MIME_JPG; break;
      case GIF: mimestr = MIME_GIF; break;
      case BIN: mimestr = MIME_BIN; break;
      case WAV: mimestr = MIME_WAV; break;
      case MP3: mimestr = MIME_MP3; break;
      default: mimestr = MIME_TXT;
    }
    h += sprintf(h,"Content-type: %s\n",mimestr);

    if(h_file[0] && !access(h_file, 0))
    {
      h += sprintf(h,"Content-length: %ld\n",filesize(h_file));
      h += sprintf(h,"Last-modified: %s\n",ht_time(filetime(h_file)));
    }
    else
      {
        char cmd[30];
        int expsec=180;

        get_postarea("cmd=",cmd,29,0);

        if(cmd[0] && cmd[0] != 'd' && cmd[0] != 'c' && cmd[0] != 'h')
          expsec = (-10000);

        if(stristr(h_uri, "/send") == h_uri)
          expsec=8000; /*OE3DZW was 7200 */

        h += sprintf(h,"Expires: %s\n",ht_time(ad_time()+expsec));
    }
    h += sprintf(h, "\n");
  }

  /* Abmelden /logout */
  if(stricmp(h_uri, "/logout") == FALSE)
  {
    h += sprintf(h,"<html><head><title>Logout</title>\n");
    h += sprintf(h,"<link rel=stylesheet type=text/css href=\"/tnn.css\"></head>\n");
    h += sprintf(h,"<body><h1>%s</h1>\n",st_tab[st].phrase);
    h += sprintf(h,"<pre><span style=\"color:black\">");

    h += sprintf(h,"Sie haben sich erfolgreich abgemeldet.\n");
    h += sprintf(h,"Rejected request: <tt>'%s'</tt></span>\n",h_inbuf);
    h += sprintf(h,"</pre><hr><span style=\"color:black; font-size:12\">"
                   "<ADDRESS>HTTPD-Server TheNetNode V1.79 - Server at ´%s Port %d </ADDRESS></span>", call, Htons(ifp[HTP_ID].tcpport));
    h += sprintf(h,"</body></html>\n");

    /* Aktuellen Header in Buffer schreiben. */
    PutHeader(tbp, st_tab[st].phrase,0);
    return(FALSE);
  }

  if(h_status != 200)
  {
    h += sprintf(h,"<html><head><title>Unauthorized</title>\n");
    h += sprintf(h,"<link rel=stylesheet type=text/css href=\"/tnn.css\"></head>\n");
    h += sprintf(h,"<body><h1>%s (%d)</h1>\n",st_tab[st].phrase,h_status);
    h += sprintf(h,"<pre><span style=\"color:black\">"
                   "Benutzername ist ungueltig !!!\n"
                   "Benutzen Sie ein Rufzeichen aus DAA000 bis DZZ999 oder als GAST-Nutzer GAST.\n"
                   "\n");
    h += sprintf(h,"An error ocurred while processing your query.\n");
    h += sprintf(h,"Rejected request: <tt>'%s'</tt></span>\n",h_inbuf);
    h += sprintf(h,"</pre><hr><span style=\"color:black; font-size:12\">"
                   "<ADDRESS>HTTPD-Server TheNetNode V1.79 - Server at ´%s Port %d </ADDRESS></span>", call, Htons(ifp[HTP_ID].tcpport));
    h += sprintf(h,"</body></html>\n");

    /* Aktuellen Header in Buffer schreiben. */
    PutHeader(tbp, st_tab[st].phrase,0);
    return(FALSE);
  }

  if(method == H_GET || method == H_POST)
    return(TRUE);

  /* Aktuellen Header in Buffer schreiben. */
  PutHeader(tbp, "",1);
  return(FALSE);
}

/* Befehl weiterleiten. */
static void PutComand(void)
{
  MBHEAD *tbp;
  int     i = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(PutComand)";
#endif /* DEBUG_MODUS */

  /* Steht nix in der Befehlszeile, */
  if (!tcppoi->cmdlen)
    /* brechen wir hier ab,.        */
    return;

  /* zum Schluss das Return setzen. */
  tcppoi->cmd[tcppoi->cmdlen++] = CR;

  /* Buffer besorgen. */
  tbp = (MBHEAD *) allocb(ALLOC_MBHEAD);

  /* Befehlszeile in Buffer schreiben. */
  while (tcppoi->cmdlen-- && i != RXLEN)
   /* Zeichen in Buffer schreiben. */
   putchr(tcppoi->cmd[i++], tbp);

  /* Befehl weiterleiten. */
  RelinkTCP(tbp);
}

/* Path setzen/offnen. */
BOOLEAN SetFileOpen(char *urifile, MBHEAD *tbp)
{
  char  cfgfile[255];
#ifdef DEBUG_MODUS
  lastfunc = "l1http(SetFileOpen)";
#endif /* DEBUG_MODUS */

  /* Path setzen. */
  strcpy(cfgfile, textpath);
  strcat(cfgfile, "http");
  strcat(cfgfile, urifile);

  /* File-Pointer belegt? */
  if (tcppoi->fp == NULL)
  {
    /* Datei oeffnen. */
    if ((tcppoi->fp = fopen(cfgfile, "rb")) == NULL)
    {
      ifpp = SetInterface(tcppoi->Interface);
      T_LOGL1(TRUE, "(SetFileOpen):%s\nDatei (%s) kann nicht geoeffnet werden!\n"
                  , tcppoi->ip
                  , cfgfile);

      /* Datei laest sich nicht offnen, abbruch. */
      return(TRUE);
    }

    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL2(TRUE, "(SetFileOpen):%s\nDatei (%s) zum lesen geoeffnet.\n"
                  , tcppoi->ip
                  , cfgfile);
  }

  PutHeader(tbp, "TheNetNode V1.79", FALSE);
  return(FALSE);
}

/* HTML-Datei konnte nicht geoeffnet */
/* werden und nun wird gemeckert!    */
static void PutHtmlError(MBHEAD *tbp, char *url)
{
  char call[10];
#ifdef DEBUG_MODUS
  lastfunc = "l1http(PutHtmlError)";
#endif /* DEBUG_MODUS */

  /* Markiere, html/text. */
  h_mimetype = TXT;
  /* Neuen HTML-Haeder erzeugen. */
  PrepareHeader(tbp);

  /* Path setzen/offnen. */
  if (SetFileOpen("/unknown.html", tbp) == TRUE)
  {
    /* Aktuellen Header einlesen/schreiben. */
    PutHeader(tbp, "TheNetNode V1.79", FALSE);

    call2str(call, myid);
    /* Fehlermeldung zusammenbasteln. */
    putprintf(tbp, "<html><head><title>404 Nicht gefunden</title></head>\n");
    putprintf(tbp, "<body><center><span style=\"color:red\">\nDie Seite wurde nicht gefunden!</span>\n");
    putprintf(tbp, "<br>(Angegebene URL: %s)</center><br><br>",url);
    putprintf(tbp, "<HR>\n");
    putprintf(tbp, "<ADDRESS>HTTPD-Server TheNetNode V1.79 - Server at %s Port %d </ADDRESS>", call, Htons(ifp[HTP_ID].tcpport));
    putprintf(tbp, "</body>\n</html>\n");
  }
  else
    /* Externe Datei laden. */
    tcppoi->status = TCP_URI;
}

/* Pruefe Index-Verzeichnis. */
DIR *CheckPath(MBHEAD *tbp, char *Path)
{
  DIR    *dp;
  char    TmpPath1[256];
#ifdef DEBUG_MODUS
  lastfunc = "l1http(CheckPath)";
#endif /* DEBUG_MODUS */

  if (strncmp(Path, "//", 2) == FALSE)
    return(NULL);

  Path += 1;

  strcpy(TmpPath1, htmlpath);
  strcat(TmpPath1, Path);

  /* Verzeichnis oeffnen. */
  dp = opendir(TmpPath1);

  /* Fehler, Verzeichnis letzt sich nicht oeffnen. */
  if (dp == NULL)
    /* Abbrechen. */
    return(NULL);

  return(dp);
}

/* Index-Verzeichnis setzen. */
void SetIndexPath(char *Path, char *IndexPath)
{
  char Buffer[256];
  char *BPath = Buffer;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(SetIndexPath)";
#endif /* DEBUG_MODUS */

  strcpy(BPath, Path);

  BPath += 1;
  addslash(BPath);

  sprintf(IndexPath, "%s", BPath);
}

/* Parent-Verzeichnis setzen. */
void SetParentPath(char *Path, char *ParentPath)
{
  char Buffer[256];
  char TmpPath2[256];
  char *BPath = Buffer;
  char *loc;
  int   pos;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(SetParentPath)";
#endif /* DEBUG_MODUS */

  SetIndexPath(Path, BPath);

  strcpy(TmpPath2, htmlpath);
  strcat(TmpPath2, BPath);

  /* Letztes zeichen (Back/Slash) loeschen. */
  TmpPath2[(strlen(TmpPath2) -1)] = 0;

  /* Suche das letzte Back/Slash. */
  if ((loc = strrchr(TmpPath2, FILE_SEP)) != NULL)
  {
    /* Position setzen. */
    pos = (int)(loc - TmpPath2);
    /* aktuelles Verzeichnis setzen. */
    TmpPath2[pos] = 0;
    /* Back/Slash neu setzen. */
    addslash(TmpPath2);
  }

  /* Vergleiche Haupt "HTTPD" Verzeichnis mit aktuellen Verzeichnis. */
  if (strcasecmp(htmlpath, TmpPath2))
  {
    /* komplettes Verzeichnis setzen. */
    loc  = TmpPath2;
    /* zum aktuellen Verzeichnis vorruecken. */
    loc += (strlen(htmlpath));
    /* aktuelles Verzeichnis setzen. */
    sprintf(ParentPath, "/%s", loc);
    return;
  }

  /* Local Hostname setzen. */
  sprintf(ParentPath,"%c", FILE_SEP);
}

/* Dateien / Verzeichnisse listen. */
void DirList(MBHEAD *tbp, DIR *dir, char *Path)
{
  struct dirent *dirp;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(DirList)";
#endif /* DEBUG_MODUS */

  /* Dateien sowie Verzeichnisse auflisten. */
  while ((dirp = readdir(dir)) != NULL)
  {
    char        Size[256];
    char        FileName[256];
    long int    fSize = 0;
    struct stat FSize;

    /* nicht anzeigen. */
    if (dirp->d_name[0] == '.')
      continue;

    /* Kompletten Dateiname zusammenfassen. */
    sprintf(FileName, "%s%s%s", htmlpath, Path, dirp->d_name);

    /* Stat-Werte (Zeit, Datum, Dateigroesse) ermitteln. */
    if (!stat(FileName, &FSize))
      /* Bytes uebertragen. */
      fSize = FSize.st_size;

    /* Wert umrechnen in MB. */
    if(fSize >= 999999)
      sprintf(Size,"%8ldMB",fSize/(1024*1024));
    else
    {
      /* Wert umrechnen in KB. */
      if(fSize >= 9999)
        sprintf(Size,"%8ldKB",fSize/1024);
       else
        /* Wert umrechnen in Bytes. */
        sprintf(Size,"%8ldB ",fSize);
    }

    putprintf(tbp, " <tr>\n  <td><li><A HREF=""/%s%s""><span style=\"font-size:12\">%s</span></A></li><td align=\"right\"><span style=\"color:black; font-size:12\">%10s</span></td>\n </tr>\n",Path ,dirp->d_name, dirp->d_name, (fSize ? Size : ""));
  }
}

/* Httpd-Service. */
void TcpipHttpd(MBHEAD *data)
{
  MBHEAD  *tbp = NULL;
  char     cmd[200];
  char    *head;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(TcpipHttpd)";
#endif /* DEBUG_MODUS */

  /* Buffer besorgen. */
  if ((tbp = SetBufSock()) == NULL)
    /* Kein Speicher.*/
    return;

  /* Status, Login auswerten. */
  GetRequest(data);

  MhUpdateTCP(data, TRUE);                      /* MH-Liste updaten */

  /* HTML-Header erstellen. */
  if(PrepareHeader(tbp))
  {
    /* Binaertransfer? */
    if(h_file[0])
    {
      /* Path setzen/offnen. */
      if (SetFileOpen(h_file, tbp) == TRUE)
        /* Externe HTML-Datei kann  nicht geoffnet werden. */
        PutHtmlError(tbp, h_file);
      else
        {
          /* Externe Datei laden. */
          tcppoi->status = TCP_URI;
          /* Binaertransfer. */
          tcppoi->http = TCP_BIN;
        }
    }
    /* HTML-Code. */
    else
    {
      get_postarea("cmd=", cmd, 59, 0);

      /* Befehl? */
      if(cmd[0])
        /* Markiere Befehl. */
        head = cmd;
      /* Kein Befehl. */
      else
        /* String-Header setzen. */
        head = "TheNetNode V1.79";

      /* Index laden? */
      if( (strcmp(h_uri, "/") == 0)
        ||(strcmp(h_uri, "/login") == 0))
      {
        /* Path setzen/offnen. */
        if (SetFileOpen(("/index.html"), tbp) == TRUE)
          /* Externe HTML-Datei kann  nicht geoffnet werden. */
          PutHtmlError(tbp, "http/index.html");
        else
          /* Externe Datei laden. */
          tcppoi->status = TCP_URI;
      }
      /* evl. Kommando ausfuehren. */
      else
        /* Will User einen Befehl ausfuehren? */
        if (stricmp(h_uri, "/cmd") == 0)
        {
          /* Befehl fuer spaeter sichern. */
          strncpy(tcppoi->cmd, head, RXLEN);
          /* Laenge setzen. */
          tcppoi->cmdlen = strlen(tcppoi->cmd);

          /* Path setzen/offnen. */
          if (SetFileOpen(("/cmd.html"), tbp) == TRUE)
            /* Externe HTML-Datei kann  nicht geoffnet werden. */
            PutHtmlError(tbp, "/cmd.html");
          else
            {
              /* Externe Datei laden. */
              tcppoi->status = TCP_URI;
              /* Befehl ausfuehren. */
              tcppoi->http   = TCP_CMD;
            }
        }
        /* evl. Verzeichnis/Datei listen. */
        else
          {
          DIR           *Tree;
          char           call[10];
          char           Buffer1[256];
          char           Buffer2[256];
          char          *IndexPath = Buffer1;
          char          *ParentPath =Buffer2;

          /* Pruefe Index-Verzeichnis. */
          if ((Tree = CheckPath(tbp, h_uri)) != NULL)
          {
            /* Index-Verzeichnis setzen. */
            SetIndexPath(h_uri, IndexPath);

            /* Markiere, html/text. */
            h_mimetype = TXT;
            /* Neuen HTML-Haeder erzeugen. */
            PrepareHeader(tbp);

            /* Aktuellen Header einlesen/schreiben. */
            PutHeader(tbp, "TheNetNode V1.79", FALSE);

            /* HTML Elemente setzen. */
            putprintf(tbp, "<html>\n <head>\n  <title>Index of %s</title>\n  </head>\n <body>\n", IndexPath);
            putprintf(tbp, "<H1>Index of %s</H1>\n", IndexPath);
            putprintf(tbp, "<pre>\n<table border=\"0\">\n <colgroup>\n  <col width=\"200\">\n  <col width=\"100\">\n </colgroup>\n");
            putprintf(tbp, " <tr>\n  <td>Name</td>\n  <td align=\"right\">Size</td>\n </tr>\n");

            /* Parent-Verzeichnis setzen. */
            SetParentPath(h_uri, ParentPath);

            putprintf(tbp, " <tr>\n <ul><td>");
            putprintf(tbp, "<li><a href=\"%s\">Parent Directory</a></li></td>\n  <td>&#160;</td>\n </tr>\n", ParentPath);

            /* Verzeichnisse/Dateien Listen. */
            DirList(tbp, Tree, IndexPath);

            call2str(call, myid);
            putprintf(tbp, "</tr>\n</table>\n</ul><hr>\n");
            putprintf(tbp, "<ADDRESS>HTTPD-Server TheNetNode V1.79 - Server at %s Port %d </ADDRESS>", call, Htons(ifp[HTP_ID].tcpport));

            /* Buffer entsorgen. */
            closedir(Tree);
          }
          /* Auch kein Befehl. */
          /* Dann kann es nur eine externe HTML-Datei sein. */
          else
            /* Path setzen/offnen. */
            if (SetFileOpen(h_uri, tbp) == TRUE)
              /* Externe HTML-Datei kann  nicht geoffnet werden. */
              PutHtmlError(tbp, h_uri);
            else
              /* Externe Datei laden. */
              tcppoi->status = TCP_URI;
      }
    }
  }
  /* Ist der Buffer gefuellt und gibt es eine anfrage. */
  if ((tbp->mbpc) && (h_uri[0]))
  {
    /* Buffer zurueck spulen. */
    rwndmb(tbp);
    /* Buffer umhaengen in die Sendeliste. */
    relink((LEHEAD *) tbp, (LEHEAD *)tcppoi->outbuf.tail);
    /* Markiere, Frame in Sendeliste. */
    ++tcppoi->outlin;
  }
  /* Es gibt keine Anfrage, damit auch kein Buffer. */
  else
    {
      /* Buffer entsorgen. */
      dealmb(tbp);
      tcppoi->disflg |= 0x80;
    }
}

/* Aktuelle Zeichen auswerten. */
TRILLIAN GetContensHTP(char contens)
{
  int   i = 0;
  char s[RXLEN];
#ifdef DEBUG_MODUS
  lastfunc = "l1http(GetContensHTP)";
#endif /* DEBUG_MODUS */

  /* Alles auf 0 setzen. */
  tcppoi->cmdlen = 0;
  tcppoi->rxc = 0;

  /* Es gibt Zeichen? */
  if (!tcppoi->RecvLen)
    /* Nein, brechen wir ab. */
    return(ERRORS);

  /* Alle Zeichen vom Socket holen. */
  while (tcppoi->rxc < tcppoi->RecvLen)
  {
    /* Aktuelle Zeichen. */
    s[i] = tcppoi->rxbuf[tcppoi->rxc++];
    /* Zeichen ein LF-Return. */
    if (s[i] == LF)
      /* zum naechsten Zeichen. */
      continue;

    /* Zeichen ein Return? */
    if (s[i] == CR)
    {
      /* Return setzen. */
      tcppoi->cmd[tcppoi->cmdlen++] = CR;
      /* zum naechsten Zeichen. */
      continue;
    }

    /* Aktuelles Zeichen setzen. */
    tcppoi->cmd[tcppoi->cmdlen++] = s[i++];
  }

  /* Nullzeichen setzen. */
  tcppoi->cmd[tcppoi->cmdlen] = '\0';
  /* Frame ist komplett. */
  return(YES);
}

/* String in Buffer schreiben. */
static void PutWcard(MBHEAD *tbp, char *buf)
{
  WORD  i = strlen(buf);
  int   j = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(PutWcard)";
#endif /* DEBUG_MODUS */

  while (j != i && j != 256)
    /* Zeichen in Buffer schreiben. */
    putchr(buf[j++], tbp);
}

/* Suche nach Platzhalter. Findet man einen  */
/* senden wir den jeweiligen String          */
static void search_wildcard(MBHEAD *tbp, char buffer)
{
  char  strbuf[256 + 1];
  char  call[10];
  static char wildcard = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(search_wildcard)";
#endif /* DEBUG_MODUS */

  /* Ist Zeichen ein Platzhalter? */
  if ((wildcard == '~') || (buffer == '~'))
  {
    /* Aktuelles Zeichen sichern. */
    wildcard = buffer;

    switch (buffer)
    {
      /* Autorstring */
      case 'a':
        strncpy(strbuf, author, 256);
        /* String in Buffer schreiebn.*/
        PutWcard(tbp, strbuf);
      return;

      /* Befehl ausfuehren. */
      case 'b':
        /* Befehl weiterleiten. */
        PutComand();
        /* Conversion IBM -> HTML einschalten. */
        tcppoi->http  = TCP_CMD;
        /* Markiere, Befehl ausfuehren. */
        tcppoi->status = TCP_CMD;
      return;

      /* IP-Adresse. */
      case 'i':
        strncpy(strbuf, tcppoi->ip, 256);
        /* String in Buffer schreiben. */
        PutWcard(tbp, strbuf);
      return;

      /* Loginstring. */
      case 'l':
        strncpy(strbuf, loginstr, 256);
        /* String in Buffer schreiben. */
        PutWcard(tbp, strbuf);
      return;

      /* Mycall. */
      case 'm':
        call2str(call,myid);
        strncpy(strbuf, call, 256);
        /* String in Buffer schreiben. */
        PutWcard(tbp, strbuf);
      return;

      /* Login-Rufzeichen. */
      case 'r':
        /*Login als GAST */
        if (tcppoi->Upcall[0] == FALSE)
        {
          strncpy(strbuf, "GUEST", 5);
          strbuf[5] = 0;
          /* String in Buffer schreiben. */
          PutWcard(tbp, strbuf);
          return;
        }

        /* Login als USER */
        call2str(call, tcppoi->Upcall);
        strncpy(strbuf, call, 256);
        /* String in Buffer schreiben. */
        PutWcard(tbp, strbuf);
      return;

      /* Dateiname sichern. */
      case 'u':
        /* index.html? */
        if(strcmp(h_uri, "/") == 0)
          strncpy(strbuf, "/http/index.html", 256);
        /* Keine index.html. */
        else
          strncpy(strbuf, h_uri, 256);

        /* String in Buffer schreiben. */
        PutWcard(tbp, strbuf);
      return;

      /* TCP-Port. */
      case 'p':
        sprintf(strbuf,"%d", Htons(ifp[HTP_ID].tcpport));
        PutWcard(tbp, strbuf);
      return;

      /* Platzhalter. */
      case '~':
        /* Bei Binaertransfer keine  */
        /* Beachtung auf Platzhalter */
        if (tcppoi->http != TCP_BIN)
          /* Binaertransfer, abbruch. */
          return;
    }
  }
  /* Zeichen in Buffer schreiben. */
  putchr((wildcard = (char)buffer), tbp);
}

/* Externe HTML-Datei laden. */
BOOLEAN load_uri(void)
{
  SENDTX      *STx = NULL;
  register int i = 0;
  WORD         c;
  BOOLEAN      end = TRUE;
  /* Kein Frame in der Warteschlange. */
  int mem = (nmbfre_max / 2);

#ifdef DEBUG_MODUS
  lastfunc = "l1http(load_uri)";
#endif /* DEBUG_MODUS */

  /* Frame in der Warteschlange. */
  if (tcppoi->outlin)
  {
    /* TX-Segment besorgen. */
    if ((STx = (SENDTX *)allocb(ALLOC_L1HTTPD_TX)) == NULL)
    {
      /* Seqment auf Disc setzen. */
      tcppoi->disflg |= 0x80;
      /* Kein Speicher frei. */
      return(FALSE);
    }

    /* Hole Frame aus der Warteschlange. */
    STx->Data = (MBHEAD *)ulink((LEHEAD *)tcppoi->outbuf.head);
    /* Entferne Markierung aus der Warteschlange. */
    --tcppoi->outlin;

    /* Socket setzen. */
    STx->Sock      = tcppoi->sock;
    /* Interface setzen, */
    STx->Interface = tcppoi->Interface;
    /* Stack-Mode setzen.   */
    STx->Mode      = tcppoi->mode;

    /* Buffer zurueckspulen.*/
    rwndmb(STx->Data);
    /* Buffer in die Sendeliste haengen. */
    relink((LEHEAD *) STx, (LEHEAD *)rxflTX.tail);
    /* Frame senden. */
    return(TRUE);
  }

  /* Pruefen, ob genug Speicher vorhanden ist. */
  if (  (nmbfre < 300)
      ||(mem < (nmbfre_max - nmbfre)))
  {
    /* ggf. Logbuch fuehren. */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL2(TRUE, "(load_url):%s\nSpeicher (%d) ist voll!\n"
                , tcppoi->ip
                , nmbfre);
    /* Segment auf Disc setzen. */
    tcppoi->disflg |= 0x80;
    /* Kein Speicher frei. */
    return(FALSE);
  }

/* TX-Segment besorgen. */
  if ((STx = (SENDTX *)allocb(ALLOC_L1HTTPD_TX)) == NULL)
  {
    /* ggf. Logbuch fuehren. */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL2(TRUE, "(load_url):%s\nSpeicher (%d) ist voll!\n"
                , tcppoi->ip
                , nmbfre);
    /* Segment auf Disc setzen. */
    tcppoi->disflg |= 0x80;
    /* Kein Segment frei, abbruch. */
    return(FALSE);
  }

  /* Buffer besorgen.*/
  if ((STx->Data = SetBuffer()) == NULL)
  {
    /* TX-Segment entsorgen.*/
    dealoc((MBHEAD *)ulink((LEHEAD *)STx));

    /* ggf. Logbuch fuehren. */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL2(TRUE, "(load_url):%s\nSpeicher (%d) ist voll!\n"
                , tcppoi->ip
                , nmbfre);

    /* Seqment auf Disc setzen. */
    tcppoi->disflg |= 0x80;
    /* Buffer ist voll, abbruch. */
    return(FALSE);
  }

  /* Socket setzen. */
  STx->Sock      = tcppoi->sock;
  /* Interface setzen. */
  STx->Interface = tcppoi->Interface;
  /* Stack-Mode setzen.   */
  STx->Mode      = tcppoi->mode;

  /* Fehler beim Offnen der externen  HTML-Datei. */
  if (tcppoi->fp == NULL)
  {
    /* Buffer entsorgen. */
    dealmb(STx->Data);
    /* Seqment auf Disc setzen. */
    tcppoi->disflg |= 0x80;
    /* ggf. Logbuch fuehren. */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL2(TRUE, "(load_url):%s\nDatei ist nicht geoeffnet!\n"
                , tcppoi->ip);
    return(FALSE);
  }

  /* max. 1024 Zeichen einlesen/schreiben. */
  for (; STx->Data->mbpc < 1024; ++i)
  {
    /* Es wird ein Befehl ausfuehren. */
    if (tcppoi->status == TCP_CMD)
      /* Ja, dann spaeter den restlichen html-code laden. */
      break;


    /* aktuelle Zeichen holen. */
    if ((c = fgetc(tcppoi->fp)) == EOF)
    {
      /* Dateiende erreicht. */
      end = FALSE;
      /* Schleife abbrechen. */
      break;
    }

    /* Bei Binaertranfer keine durchsuchung von Wildcards. */
    if (tcppoi->http == TCP_BIN)
      /* Schreibe Zeichen in Buffer. */
      putchr((char)(c), STx->Data);
    /* Text-Transfer. */
    else
     /* Nach Wildcard suchen, ggf auswerten */
     /* und in Buffer schreiben.            */
     search_wildcard(STx->Data, (char)(c));
  }

  /* Dateiende erreicht. */
  if (end == FALSE)
  {
    /* Externe HTML-Datei schliessen. */
    fclose(tcppoi->fp);

    /* ggf. Logbuch fuehren. */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL2(TRUE, "(load_url):%s\nDatei wird geschlossen.\n"
                , tcppoi->ip);

    /* File-Pointer zuruecksetzen. */
    tcppoi->fp = NULL;
    /* Markiere, Download abgeschlossen. */
    tcppoi->status = TCP_NULL;
    /* Seqment auf Disc setzen. */
    tcppoi->disflg |= 0x80;
  }

  /* Buffer zurueckspulen.*/
  rwndmb(STx->Data);
  /* Buffer in die Sendeliste haengen. */
  relink((LEHEAD *) STx, (LEHEAD *)rxflTX.tail);
  return(TRUE);
}

void PutHtmlEnd(void)
{
  SENDTX *STx;
#ifdef DEBUG_MODUS
  lastfunc = "l1http(PutHtmlEnd)";
#endif /* DEBUG_MODUS */

  if ((STx = (SENDTX *)allocb(ALLOC_L1HTTPD_TX)) == NULL)/* TX-Segment besorgen. */
  {
    /* ggf. Logbuch fuehren. */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL2(TRUE, "(PutHtmlEnd):%s\nSpeicher (%d) ist voll!\n"
                  , tcppoi->ip
                  , nmbfre);
    tcppoi->disflg |= 0x80;
    return;                                    /* Kein Segment frei, abbruch. */
  }

  if ((STx->Data = SetBuffer()) == NULL)             /* Buffer besorgen.      */
  {
    dealoc((MBHEAD *)ulink((LEHEAD *)STx));           /* TX-Segment entsorgen.*/

    /* ggf. Logbuch fuehren. */
    ifpp = SetInterface(tcppoi->Interface);
    T_LOGL2(TRUE, "(PutHtmlEnd):%s\nSpeicher (%d) ist voll!\n"
                , tcppoi->ip
                , nmbfre);
    tcppoi->disflg |= 0x80;
    return;                                      /* Buffer ist voll, abbruch. */
  }

  STx->Sock      = tcppoi->sock;
  STx->Interface = tcppoi->Interface;
  STx->Mode      = tcppoi->mode;                      /* Stack-Mode setzen.   */

  putprintf(STx->Data, "\n</pre></body></html>");

  rwndmb(STx->Data);                                  /* Buffer zurueckspulen.*/
  /* Buffer in die Sendeliste haengen. */
  relink((LEHEAD *) STx, (LEHEAD *)rxflTX.tail);
}
#endif
/* End of os/win32/httpd.c */
