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
/* File src/pacsat.c (maintained by: ???)                               */
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

#include "tnn.h"

#ifdef PACSAT
/* Funktionsprototypen                                                  */
static void     open_tempfile(void);
static void     send_user(const char *);
static void     store_line(void);
static void     pacsat_send(void);
static void     stop_send(void);
static void     pacsat_exit(void);
static void     store(void);
static void     pacsat_command_switch(void);

#define pacuser userpo->pacsat
#define pacver  "Version 1.15"

#if !defined(__LINUX__) && !defined(__WIN32__)
/************************************************************************/
/*                                                                      */
/* Platz auf dem angegebenen Laufwerk ermitteln                         */
/*                                                                      */
/************************************************************************/
LONG getdiskfree(char *drive)
{
  struct dfree free;

  getdfree(toupper(*drive) - 'A' + 1, &free);
  if (free.df_sclus == (unsigned)-1)
    return(0L);

  return((LONG) free.df_avail
    * (LONG) free.df_bsec
    * (LONG) free.df_sclus);
}
#endif

/************************************************************************/
/*                                                                      */
/* Temporaerdatei fuer den aktuellen User oeffnen                       */
/*                                                                      */
/************************************************************************/
static void open_tempfile(void)
{
  pacuser->tempfp = NULL;

  /* einen Temporaerdateinamen im TEXTPATH erstellen */
  pacuser->tempfile = tempnam(textpath, "ps");
  if (pacuser->tempfile == NULL) return;

  if (getdiskfree(textpath) > 2000000L)
  /* Datei oeffnen */
      pacuser->tempfp = xfopen(pacuser->tempfile, "wb");
  /* Versagt das oeffnen ist tempfp = NULL, das wird bei den */
  /* Schreib-Routinen abgefangen. */
}

/************************************************************************/
/*                                                                      */
/* eine Zeile an den Pacsat-User schicken                               */
/*                                                                      */
/************************************************************************/
static void send_user(const char *s)
{
  MBHEAD *mbp;

  mbp = getmbp();
  putstr(s, mbp);
  seteom(mbp);
}

/************************************************************************/
/*                                                                      */
/* Die empfangene Zeile in die Temporaerdatei speichern                 */
/*                                                                      */
/************************************************************************/
static void store_line(void)
{
  if (pacuser->tempfp)
  {
      fputs(clilin, pacuser->tempfp);
      fputc('\r', pacuser->tempfp);
      fputc('\12', pacuser->tempfp);
  }
}

/************************************************************************/
/*                                                                      */
/* eine Box moechte eine Nachricht speichern.                           */
/*                                                                      */
/************************************************************************/
static void pacsat_send(void)
{
  open_tempfile();                 /* in eine Temporaer-Datei speichern */
  if (pacuser->tempfp != NULL)     /* wenn Datei geoeffnet werden konnte*/
  {
      send_user("OK\r");           /* Bestaetigung fuer den User/die Box*/
      store_line();                /* Zeile speichern (S DB7KG @ ...)   */
  }
  else
      send_user("Disk full\r");    /* Abbrechen, kein File              */
}

/************************************************************************/
/*                                                                      */
/* Filespeicherung beenden, File in das Filesystem uebertragen          */
/*                                                                      */
/************************************************************************/
static void stop_send(void)
{
  if (pacuser->tempfp)
  {
      fclose(pacuser->tempfp);        /* Temporaerdatei schliessen      */
      pacuser->tempfp = NULL;
      new_file(pacuser->tempfile);
      xremove(pacuser->tempfile);     /* Temporaerdatei loeschen        */
      free(pacuser->tempfile);
      pacuser->tempfile = NULL;
  }
  send_user(">\r");                   /* S&F Prompt                     */
}

/************************************************************************/
/*                                                                      */
/* Box moechte PACSAT verlassen                                         */
/*                                                                      */
/************************************************************************/
static void pacsat_exit(void)
{
  MBHEAD *mbp;

  dealoc((MBHEAD *) pacuser);
  pacuser = NULL;
  mbp = putals("Reconnected to ");
  putid(myid, mbp);
  putchr('\r', mbp);
  prompt(mbp);
  seteom(mbp);
}

/************************************************************************/
/*                                                                      */
/* Routine zur Nachrichtenspeicherung                                   */
/*                                                                      */
/************************************************************************/
static void store(void)
{
  store_line();                               /* Zeile speichern        */
  if (strchr(clilin, 0x1A)) stop_send();      /* ein CTRL-Z gefunden ?  */
}

#if 0
/************************************************************************/
/*                                                                      */
/* Anzeigen/einlesen des PACSAT-BOX Rufzeichens                         */
/*                                                                      */
/************************************************************************/
void boxcall(void)
{
  char call[L2IDLEN];
  MBHEAD *mbp;

  clipoi++;
  clicnt--;

  if (clicnt > 2 && getcal(&clicnt, &clipoi, FALSE, call) == TRUE)
    cpyid(pacsatid, call);

  mbp = getmbp();
#ifdef SPEECH
  putstr(speech_message(291), mbp);
#else
  putstr("PACSAT BOX-Call is now ", mbp);
#endif
  putid(pacsatid, mbp);
  putstr("\r", mbp);
  seteom(mbp);
}
#endif

/************************************************************************/
/*                                                                      */
/* Analyse eines eingegeben Befehls                                     */
/*                                                                      */
/************************************************************************/
static void pacsat_command_switch(void)
{
  FILE *fp;
  static char file[MAXPATH], s[20] ,*cp;
  time_t t;

  /* muss er Passwort senden und hat nicht ? */
  if (*clipoi == '[')
  {
      if (pacuser->check_pwd)
      {
          cp = strchr((char *)clipoi, ']') + 2;

          callss2str(s, calofs(UPLINK, userpo->uid));
          sprintf(file, "%s%s.PWD", pacsatpath, s);
          if ((fp = xfopen(file, "rt")) != NULL)
          {
             pacuser->row++;
             while (pacuser->row--) if (!fgets(file, MAXPATH-1, fp)) break;
             fclose(fp);
             file[pacuser->col+4] = NUL;
             pacuser->check_pwd = strcmp(file+pacuser->col, cp);
          }
      }
      if (!pacuser->check_pwd) send_user(">\r");
      return;
  }

  if (pacuser->check_pwd && (*clipoi != '['))
  {
    if (syspro_flag == TRUE)
    {
      strcpy(file, confpath);
      strcat(file, "SYSOP.PRO");
      if ((fp = xfopen(file,"at")) != NULL)
      {
#ifdef SPEECH
        fprintf(fp, speech_message(285), ctime(&t), calofs(UPLINK, userpo->uid));
#else
        fprintf(fp, "%24.24s %6.6s: rejected (mailbox)\n", ctime(&t), calofs(UPLINK, userpo->uid));
#endif
        fclose(fp);
      }
    }
    pacsat_exit(); /* und das wars ... */
    return;
  }

  switch (toupper(*clipoi))              /* sonst Befehl auswerten      */
  {
    case 'S' : pacsat_send();
               break;
    case 'F' : pacsat_exit();              /* F> beendet                */
               break;
  }
}

/************************************************************************/
/*                                                                      */
/* Dataswitch L7 (CCP) -> PACSAT                                        */
/*                                                                      */
/************************************************************************/
void l7_to_pacsat(void)
{
  *clipoi = NUL;
  clipoi = clilin;
  if (pacuser->tempfp != NULL)
    store();                              /* bei Datenspeicherung       */
  else
    pacsat_command_switch();
}

/************************************************************************/
/*                                                                      */
/* In die PACSAT S&F Box einloggen                                      */
/*                                                                      */
/************************************************************************/
void ccpbox(void)
{
  MBHEAD *mbp;
  char    pwdfile[MAXPATH], s[20];
  struct  tm *tim;
  WORD    mon;

  if (cmpid(pacsatid, nullid))
  {
    invmsg();
    return;
  }
  pacuser = (PACSATBLK *)allocb(ALLOC_PACSATBLK);
  pacuser->tempfp = NULL;
  pacuser->tempfile = NULL;
  /* wenn ein Passwort-File vorhanden ist, dann muss der User sich
     auch privilegieren */
  pacuser->login = sys_time;
  callss2str(s, calofs(UPLINK, userpo->uid));
  sprintf(pwdfile, "%s%s.PWD", pacsatpath, s);
  pacuser->check_pwd = !xaccess(pwdfile, 0);

  send_user("Internal link setup ...\r");
  mbp = putals(conmsg);
  putid(pacsatid, mbp);
  putstr("\r", mbp);
  seteom(mbp);

  /* Begruessung oder S&F-SID senden */
  strcpy(pwdfile, "[THEBOX-1.8-$]");
  if (pacuser->check_pwd)
  {
    tim = localtime(&pacuser->login);
    mon = tim->tm_mon + 1;
    sprintf(s," %02d%02d%02d%02d%02d",tim->tm_year,mon,tim->tm_mday,
                                      tim->tm_hour,tim->tm_min);
    strcat(pwdfile, s);
    pacuser->row = (tim->tm_min + tim->tm_year) % 60;
    pacuser->col = tim->tm_hour;
  }
  strcat(pwdfile, "\r");
  send_user(pwdfile);
  send_user(">\r");

  return;
}

PARAM pacsatpartab[] = {                     /* Parameter Tabelle       */
  {&pacsat_timer,0,10000, "Timer"},
  {&pacsat_frames,0,20,   "Frames"},
  {&pacsat_free,0,10000,  "Diskfree"}
};

/************************************************************************/
/*                                                                      */
/* Pacsat-Server konfigurieren (nur durch Sysop)                        */
/*                                                                      */
/************************************************************************/
void ccppacsat(void)
{
  MBHEAD  *mbp = NULL;
  int      ch;
  WORD     port;

  if (issyso() && skipsp(&clicnt, &clipoi)) {
    ch = *clipoi;
    nextspace(&clicnt, &clipoi);
    skipsp(&clicnt, &clipoi);
    switch (toupper(ch)) {
      case 'R' : filesystem_init(); /* Reload */
#ifdef SPEECH
                 putmsg(speech_message(292));
#else
                 putmsg("Filesystem reloaded\r");
#endif
                 return;
      case 'C' : ccp_call(pacsatid);
                 if (!pacsatid[0])
                   cpyid(pacsatid, nullid);
                 return;
      case 'P' : ccp_par("BROADCAST-Parms:\r", pacsatpartab, sizeof(pacsatpartab)/sizeof(PARAM));
                 return;
      case '-' :
      case '+' : if (getport(&clicnt, &clipoi, &port))
                   pacsat_enabled[port] = (ch == '+');
    }
  }

  mbp = getmbp();
#ifdef SPEECH
  putstr(speech_message(288), mbp);
#else
  putstr("Server call: ", mbp);
#endif
  putid(pacsatid, mbp);
#ifdef SPEECH
  putstr(speech_message(289), mbp);
#else
  putstr("\rMessage pool: ", mbp);
#endif
  if (last_fid < first_fid)
#ifdef SPEECH
    putstr(speech_message(290), mbp);
#else
    putstr("Empty\r", mbp);
#endif
  else
#ifdef SPEECH
    putprintf(mbp,speech_message(286), first_fid, last_fid, last_fid - first_fid + 1);
#else
    putprintf(mbp,"%lX-%lX (%lu Messages)\r", first_fid, last_fid, last_fid - first_fid + 1);
#endif

  for (port = 0; port < L2PNUM; port++)
    if (pacsat_enabled[port])
#ifdef SPEECH
      putprintf(mbp, speech_message(287), port, portpar[port].name);
#else
      putprintf(mbp, "BROADCAST enabled on port %u (%s).\r", port, portpar[port].name);
#endif

  prompt(mbp);
  seteom(mbp);
}

#endif /* PACSAT */
/* End of src/pacsat.c */
