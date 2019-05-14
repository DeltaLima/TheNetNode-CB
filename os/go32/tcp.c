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
/* File os/go32/tcp.c (maintained by: DAA531)                           */
/*                                                                      */
/* This file is part of "TheNetNode" - Software Package                 */
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
#ifdef TCP_STACK

#if defined(__GO32__)
#define KI_BUFFERSIZE      8192         /* bei DPMI kann es mehr sein   */
#else
#define KI_BUFFERSIZE      700          /* Groesse eines Buffers        */
#endif


/* Kisstype setzen. */
void ReadEnvcomTCP(char *what, int *Interface)
{
  *Interface = 0;

  if (getenv(what) != NULL)
    sscanf(getenv(what), "%u", Interface);

  switch (*Interface)
  {
    case  KISS_TELNET:
      *Interface = KISS_TELNET;
     break;

    case  KISS_HTTPD:
      *Interface = KISS_HTTPD;
     break;

    case  KISS_IPCONV:
      *Interface = KISS_IPCONV;
     break;

    /* Ungueltiges Interface. */
    default :
      *Interface = EOF;
     break;
  }

  if (*Interface == 0)
    *Interface = EOF;
}

/************************************************************************/
/* Level1 Init                                                          */
/************************************************************************/
static BOOLEAN InitL1TCP(void)
{
  T_INTERFACE *tpoi;
  int          tcp;
  char         str[20];
  int          Interface = 0;
  int          numactive = 0;

  /* Interface initialisieren. */
  InitIFC();

  /* Alle Interface durchgehen. */
  for (tcp = 0; tcp < MAXINTERFACE; tcp++)
  {
    /* Zeiger auf das aktuelle Interface. */
    tpoi = &ifp[tcp];
    /* String setzen. */
    sprintf(str, "TCPIP%u", tcp + 1);
    /* Interface einlesen. */
    ReadEnvcomTCP(str, &Interface);

    /* Nur wenn Interface gefunden, */
    if (Interface != -1)
    {
      /* bzw. Interface nicht belegt. */
      if (tpoi->actively == FALSE)
      {
        /* Interface, L2Port setzen. */
        printf("--- TCPIP Initialisieren KissType %d\n", Interface);
        tpoi->actively = FALSE;
        tpoi->l2port   = EOF;

        /* Um ein erhoehen. */
        numactive++;
      } /* Interface ist schon belegt. */
    } /* Ungueltiges Interface. */
  } /* for */

  return(numactive);
}

/************************************************************************/
/* Level1 Exit                                                          */
/************************************************************************/
static void ExitL1TCP(void)
{
  T_INTERFACE *tpoi;
  register int i;

  for (i = 0; i < MAXINTERFACE; i++)
  {
    tpoi = &ifp[i];

    L1ExitTCP(tpoi->Interface);
  }
}

/* DCD-Status liefern. */
static WORD DcdL1TCP(PORTINFO *port)
{
  /* Sender ist immer frei. */
  return(FALSE);
}

static void CtlL1TCP(int req, int port)
{
  switch (req)
  {
    case L1CCMD :
    case L1CRES :
      break;
  }

  default_l1ctl(req, port); /* Flags loeschen */
};


static int IstomeL1TCP(int major, char *devname)
{
  char   name[20 + 1];

  strncpy(name, devname, 20); /* Minor bestimmen und abschneiden */
  name[20] = 0;

#ifdef L1TELNET
  if (strnicmp(name, "TELNET", strlen(name))==0)
    return(TEL_ID);
#endif /* L1TELNET */

#ifdef L1HTTPD
  if (strnicmp(name, "HTTPD", strlen(name))==0)
    return(HTP_ID);
#endif /* L1HTTPD */

#ifdef L1IPCONV
  if (strnicmp(name, "IPCONV", strlen(name))==0)
    return(CVS_ID);
#endif /* L1IPCONV */

#ifdef L1IRC
  if (strnicmp(name, "IRC", strlen(name))==0)
    return(IRC_ID);
#endif /* L1IPCONV */

  return(NO_MINOR);
}

static int AttachL1TCP(int port, int minor, BOOLEAN check_only)
{
  /* Zeiger auf das aktuelle Interface. */
  T_INTERFACE *tpoi = &ifp[minor];

  if (tpoi->actively == FALSE)
  {
    if (tpoi->l2port == EOF)
    {
      if (!check_only)
      {
        if (!L1InitTCP(tpoi->Interface, port, tpoi->tcpport))
          return(FALSE);

        portpar[port].minor = minor;
      }

      return(1);
    }

    if (tpoi->l2port == port)
      return(1);
  }

  return(0); /* versuchte Doppeleintragung */
}

static int DetachL1TCP(int port)
{
  /* Zeiger auf das aktuelle Interface. */
  T_INTERFACE *tpoi = &ifp[portpar[port].minor];

  /* Port deaktivieren. */
  tpoi->l2port   = EOF;
  /* Schliesse Interface. */
  L1ExitTCP(tpoi->Interface);
  return(TRUE);
}

static void InfoL1TCP(int what, int port, MBHEAD *mbp)
{
   /* Zeiger auf das aktuelle Interface. */
  T_INTERFACE *tpoi = &ifp[portpar[port].minor];

  switch (what)
  {
    case HW_INF_IDENT :
    case HW_INF_INFO  :
      putprintf(mbp, "%s %u", tpoi->name, Htons(tpoi->tcpport));
      break;

    case HW_INF_STAT  :
    case HW_INF_CLEAR :
      break;
      /* durchfallen */

    default:
      default_l1info(what, port, mbp);
  }
}

/* Pruefe auf TCP-Port. */
int CheckPortTCP(int port)
{
  /* Inetrface ermitteln. */
  T_INTERFACE *tpoi = &ifp[portpar[port].minor];

  /* Nur wenn aktiv. */
  if (tpoi->actively == TRUE)
  {
    switch (tpoi->Interface)
    {
#ifdef L1TELNET
      case KISS_TELNET :
        if (tpoi->l2port == port)
       return(TRUE);
#endif /* L1TELNET */

#ifdef L1HTTPD
      case KISS_HTTPD :
        if (tpoi->l2port == port)
       return(TRUE);
#endif /* L1HTTPD */

#ifdef L1IPCONV
      case KISS_IPCONV :
        if (tpoi->l2port == port)
       return(TRUE);
#endif /* L1IPCONV */

#ifdef L1IRC
      case KISS_IRC :
        if (tpoi->l2port == port)
       return(TRUE);
#endif /* L1IRC */

      /* Ungueltiges Interface. */
      default :
       return(FALSE);
    } /* switch. */
  }
  else
    return(FALSE);
}


static int RegisterTCP(void)
{
  MAJOR *m;

  if (InitL1TCP()) {
    m = register_major();
    m->name    = "TCPIP";
    m->istome  = IstomeL1TCP;
    m->exit    = ExitL1TCP;
    m->ctl     = CtlL1TCP;
    m->dcd     = DcdL1TCP;
    m->attach  = AttachL1TCP;
    m->detach  = DetachL1TCP;
    m->info    = InfoL1TCP;
    return(num_major);
  }
  return(0);
}

#endif /* TCP_STACK */
/*----------------------------------------------------------------------*/

/* End of os/go32/tcp.c */
