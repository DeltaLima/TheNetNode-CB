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
/* File src/cvs_serv.c (maintained by: DL1XAO)                          */
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

/*
 * This is Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@hpfcmdd.fc.hp.com>
 *
 * Modifications by Fred Baumgarten <dc6iq@insl1.etec.uni-karlsruhe.de>
 * $Revision: 3.12 $$Date: 1996/03/03 10:09:47 $
 *
 * Modifications by Oliver Kern <daa531@gmx.de>
 * $Revision: 3.13 $$Date: 17.08.2004 22:22 $
 */
/* modified for use with TheNetNode by DL1XAO                       */
/* Hierdrin befinden sich die mit TNN kommunizierenden Routinen     */

#include "tnn.h"

#ifdef PPCONVERS

#include "conversd.h"

char cnvinbuf[2048];
WORD s_CVOUT = FALSE;


/*---------------------------------------------------------------------------*/

CONNECTION *alloc_connection(USRBLK *up)
{
  CONNECTION *cp;

  cp = (CONNECTION *) calloc(1, sizeof(CONNECTION));
  if (cp != NULLCONNECTION) {
    cp->time = currtime;
    cp->atime = currtime;
    cp->width = 80;
    cp->up = up;
#ifdef L1IRC
    cp->IrcMode = FALSE;
#endif /* L1IRC */
    cp->next = connections;
    connections = cp;
  }
  return(cp);
}

/*---------------------------------------------------------------------------*/

void free_connection(CONNECTION *cp)
{
  USRBLK   *save_userpo;
  MBHEAD   *mbp;
  PERMLINK *p;

  p = permlink_of(cp);
  if (p)
    p->connection = NULLCONNECTION;

  if (cp->up != NULL) {
    save_userpo = userpo;
    userpo = cp->up;
    if (userpo->convflag != 0) {               /* kein recon in ccp */
      disusr(userpo->uid);
      userpo->convers = NULLCONNECTION;
      /* kilusr(); das macht l2tol7 */
    } else {
      mbp = getmbp();
      putstr(timestamp, mbp);
#ifdef SPEECH
      putstr(speech_message(1), mbp);
#else
      putstr("Convers session terminated.\r", mbp);
      putstr("Enter command. Type HELP for help.\r", mbp);
#endif
      userpo->convers = NULLCONNECTION;
      prompt(mbp);
      seteom(mbp);
    }
    userpo = save_userpo;
  }

  if (cp->mbout)
    dealmb(cp->mbout);
  if (cp->pers)
    free(cp->pers);
  if (cp->away)
    free(cp->away);
  if (cp->notify)
    free(cp->notify);
  if (cp->filter)
    free(cp->filter);
  free(cp);
}

/*---------------------------------------------------------------------------*/

BOOLEAN invite_ccp(char *toname, char *message)
{
  char   tmp[8];
  USRBLK *save_userpo;
  MBHEAD *mbp;

  save_userpo = userpo;
  for (userpo  = (USRBLK *) usccpl.head;
       userpo != (USRBLK *) &usccpl;
       userpo  = (USRBLK *) userpo->unext)
  {
    if (userpo->convers == NULLCONNECTION && userpo->status == US_CCP) {
      callss2str(tmp, calofs(UPLINK, userpo->uid));
      strlwr(tmp);

      if (!Strcmp(tmp, toname)) {
        mbp = getmbp();
        putstr(message, mbp);
        seteom(mbp);
        userpo = save_userpo;
        return(TRUE);
      }
    }
  }
  userpo = save_userpo;
  return(FALSE);
}

/*---------------------------------------------------------------------------*/

BOOLEAN connect_cvshost(PERMLINK *p)
{
  CONNECTION *cp;
  USRBLK     *save_userpo;
  CIRBLK     *save_cirpoi;
  NODE       *node;
  PTCENT     *ptcp;
  char        convcall[L2IDLEN];

  if ((cp = alloc_connection(NULL)) != NULL) {
    save_userpo = userpo;
    save_cirpoi = cirpoi;
    userpo = (USRBLK *) allocb(ALLOC_USRBLK1);
    userpo->uid      = 0;
    userpo->status   = US_CHST;
    userpo->sysflg   = 0;
    userpo->errcnt   = 0;
    userpo->mbhd     = NULL;
    userpo->talkcall[0]= '\0';
    userpo->monitor  = NULL;
    userpo->auditlevel = 0;
    userpo->fp       = NULL;
    userpo->fname    = NULL;
    userpo->convers  = cp;
    userpo->convflag = 2;         /* kam von uns, kein recon in ccp */
#ifdef PACSAT
    userpo->pacsat   = NULL;
#endif

    send_proto("serv", "Try Hostconnect to %s (%p)", p->cname, cp);

    strcpy(cp->name, p->cname);
    cp->type      = CT_UNKNOWN;
    cp->up        = userpo;
    p->connection = cp;

#ifdef OS_IPLINK
    if (p->TcpLink)
    {
      if (IPConvConnect(p->call, myid, TRUE))
      {
         dealoc((MBHEAD *)userpo);                   /* das war alles nix      */
         connections = cp->next;                     /* tun wir so, als haetten*/
         free(cp);                                   /* wir nichts bemerkt     */
         p->connection = NULLCONNECTION;
         return(FALSE);
      }

      relink((LEHEAD *)userpo,(LEHEAD *) usccpl.tail);
      userpo->uid = g_uid(tcppoi, TCP_USER);

      cpyid(tcppoi->Downcall, p->call);
      memcpy(tcppoi->Upcall, myid, L2CALEN);
      tcppoi->Upcall[L2IDLEN-1] = (convid<<1)|0x60;

      userpo->status = US_CCP;
      clrptc(userpo->uid);
      ptcp = ptctab + userpo->uid;
      ptcp->ublk = userpo;
      ptcp->state = UPLINK;        /* Convers ist eigentlich Downlink  */
      userpo = save_userpo;
      cirpoi = save_cirpoi;
      return(TRUE);
    }
#endif /* OS_IPLINK */

    if (p->port == 255) {
      if (iscall(p->call,&node,NULL,DG)) {
        for (cirpoi = cirtab; cirpoi < &cirtab[NUMCIR]; ++cirpoi)
          if (cirpoi->state == 0)         /* Level 3 Kontrollblock = leer */
            break;                        /* freien Block gefunden..      */

        if (cirpoi != &cirtab[NUMCIR]) {         /* neues Ciruit erzeugen */
          relink((LEHEAD *)userpo,(LEHEAD *) usccpl.tail);
          userpo->uid = g_uid(cirpoi, L4_USER);
          cpyid(cirpoi->downca, node->id);
          memcpy(cirpoi->upcall, myid, L2CALEN);
          cirpoi->upcall[L2IDLEN-1] = (convid<<1)|0x60;
          cpyid(cirpoi->upnod, myid);                   /* Uplink hier    */
          *cirpoi->upnodv = '\0';                       /* kein Digi      */
          cpyid(cirpoi->l3node, node->id);
          newcir();                             /* L3-Verbindung aufbauen */
          clrptc(userpo->uid);
          ptcp = ptctab + userpo->uid;
          ptcp->ublk = userpo;
          ptcp->state = UPLINK;        /* Convers ist eigentlich Downlink  */
          userpo = save_userpo;
          cirpoi = save_cirpoi;
          return(TRUE);
        }
      }
      send_proto("serv", "...no circuit/not a node");
    }
    else {
      if (portenabled(p->port)) {
        memcpy(convcall, myid, L2CALEN);
        convcall[L2IDLEN-1] = (convid<<1)|0x60;
#ifdef OS_IPLINK
        if (IPConvConnect(p->call, myid, TRUE) == FALSE)
        {
          relink((LEHEAD *)userpo,(LEHEAD *) usccpl.tail);
          userpo->uid = g_uid(tcppoi, TCP_USER);

          cpyid(tcppoi->Downcall, p->call);
          memcpy(tcppoi->Upcall, myid, L2CALEN);
          tcppoi->Upcall[L2IDLEN-1] = (convid<<1)|0x60;

          userpo->status = US_CCP;
          clrptc(userpo->uid);
          ptcp = ptctab + userpo->uid;
          ptcp->ublk = userpo;
          ptcp->state = UPLINK;        /* Convers ist eigentlich Downlink  */
          userpo = save_userpo;
          cirpoi = save_cirpoi;
          return(TRUE);
        }
#endif /* OS_IPLINK */

        if ((lnkpoi = getlnk((UBYTE)p->port, convcall, p->call, p->via)) != NULL) {
          relink((LEHEAD *)userpo,(LEHEAD *) usccpl.tail);
          userpo->uid = g_uid(lnkpoi, L2_USER);
          newlnk();                             /* L2-Verbindung aufbauen! */
          clrptc(userpo->uid);
          ptcp = ptctab + userpo->uid;
          ptcp->ublk = userpo;
          ptcp->state = UPLINK;        /* Convers ist eigentlich Downlink  */
          userpo = save_userpo;
          cirpoi = save_cirpoi;
          return(TRUE);
         } else
          send_proto("serv", "...no free link");
      } else
        send_proto("serv", "...port off");
    }
    dealoc((MBHEAD *)userpo);                   /* das war alles nix      */
    connections = cp->next;                     /* tun wir so, als haetten*/
    free(cp);                                   /* wir nichts bemerkt     */
    p->connection = NULLCONNECTION;
    userpo = save_userpo;
    cirpoi = save_cirpoi;
  }
  return(FALSE);
}

/*---------------------------------------------------------------------------*/

void putcvsstr(CONNECTION *cp, char *str)
{
  if (cp->mbout == NULL) {
    cp->mbout = (MBHEAD *) allocb(ALLOC_MBHEAD);
    cp->mbout->l2link = g_ulink(cp->up->uid);
    cp->mbout->type   = g_utyp(cp->up->uid);
  }
  putstr(str, cp->mbout);
  s_CVOUT = TRUE;
}

static UWORD cvs2tnn(CONNECTION *cp)
{
 UWORD ret = 0;

  if (cp->mbout != NULL) {
    ret = cp->mbout->mbpc - cp->mbout->mbgc;
    seteom(cp->mbout);
    cp->mbout = NULL;
  }
  return(ret);
}

LONG queuelength(CONNECTION *cp)
{
  return((cp->mbout != NULL) ? (cp->mbout->mbpc - cp->mbout->mbgc) : 0L);
}

/*---------------------------------------------------------------------------*/

/*****************************************************************************/
/* Initialisieren der conversd-Umgebung                                      */
/*---------------------------------------------------------------------------*/
void convers_init(void)
{
  char tmp[82];
  int  pl;
  char *cp;

  time(&boottime);
  currtime = boottime;
  sprintf(myrev, "PP-%4.4st", strchr(REV, ':')+2);

  callss2str(tmp, myid); /* SSID ja egal ... */
  strlwr(tmp);
  myhostname = strdup(tmp);

  sprintf(tmp, "%sconvers.prs", textpath);
  personalmanager(INIT, NULL, tmp);

#ifdef USERPROFIL
  InitProfilTAB();
#endif /* USERPROFIL */

  for (pl = 0; pl < MAXCVSHOST; pl++)
     permarray[pl] = NULLPERMLINK;

  cvs_pc = 1;             /* Debug-Kanal 32767 per Default eingeschaltet!! */

  if ((cp = getenv("CONVERSD")) != NULL) {
    while (*cp && *(cp+1) == ':') {
      switch (*cp) {
        case 'P': cvs_pc = atoi(cp+2);
                  break;
        /*case 'x': cvs_xx = atoi(cp+2);*/
      }
      while (*cp && *cp != ' ')
        cp++;
      while (*cp == ' ')
        cp++;
    }
  }
}

/*****************************************************************************/
/* Freigeben der geschlossenen Verbindungen und Aufbau der Hostverbindungen  */
/*---------------------------------------------------------------------------*/
void conversd()
{
  time(&currtime);
  free_closed_connections();
  connect_permlinks();
  convers_output();
}

/************************************************************************/
/* Convers-Daten-Uebergabe                                              */
/*----------------------------------------------------------------------*/
BOOLEAN convers_input(MBHEAD *mhdp)
{
  char c;
  char *ibuf;
  WORD icnt;

  if (mhdp->mbgc >= mhdp->mbpc)
      return(TRUE);

  do {
#ifndef L1IRC
    if ((c = getchr(mhdp)) == '!')       /* process !tnn commands */
      return(FALSE);
#else
    if ((c = getchr(mhdp)) == '!')       /* process !tnn commands */
    {
      if (userpo->IrcMode == FALSE)
        return(FALSE);
    }
#endif /* L1IRC */

    if (c == CR || c == LF)
      continue;

    ibuf = cnvinbuf;
    *ibuf++ = c;
    icnt = 1;
    while (   mhdp->mbgc < mhdp->mbpc
           && (c = getchr(mhdp)) != CR
           && c != LF
#ifdef __WIN32__
           && icnt < (short)sizeof(cnvinbuf) - 5)
#else
           && icnt < sizeof(cnvinbuf) - 5)
#endif /* WIN32 */
    {
      if (c == BS) {
        if (icnt > 0) {
          --ibuf;
          --icnt;
        }
      } else if (c != ESC) {                    /* ESC ausfiltern       */
        *ibuf++ = c;
        ++icnt;
      }
    }
    *ibuf = '\0';
    userpo->convers->received += icnt;
    process_input(userpo->convers);
  }
  while (mhdp->mbgc < mhdp->mbpc && getlin(mhdp));

  return(TRUE);
}
/************************************************************************/
/* Convers-Daten-Ausgabe                                                */
/*----------------------------------------------------------------------*/
void convers_output()
{
  CONNECTION *cp;

  if (!s_CVOUT)
      return;

  s_CVOUT = FALSE;

  for (cp = connections; cp; cp = cp->next)
    if (cp->mbout != NULL && cp->up != NULL)
      cp->xmitted += cvs2tnn(cp);
}

/**************************************************************************/
/* Starten von convers oder Abfragen Userliste, sowie Editieren der Hosts */
/*------------------------------------------------------------------------*/
void ccpcvs()
{
  char        name[8];
  char        buffer[256];
  WORD        i;
  LONG        channel = 0L;
  CONNECTION  *cp;
  PERMLINK    *p;
  MBHEAD      *mbp;
#ifdef CONVERS_LINKS
  char call[10];
#endif

  if (!ismemr())      /* kein Speicher mehr */
      return;

  if (userpo->convers != NULLCONNECTION) {    /* !conv abfangen     */
#ifdef SPEECH
    appenddirect(userpo->convers, speech_message(2));
#else
    appenddirect(userpo->convers, "*** You are already in convers-mode.\r");
#endif
    return;
  }

  if ((cp = alloc_connection(userpo)) == NULLCONNECTION) {
    puttfu("Convers");

    return;
  }

  skipsp(&clicnt, &clipoi);  /* Abfangen der Nichtteilnehmerbefehle */
  *buffer = '\0';
  if (clicnt) {
    switch (*clipoi) {
      case 'o':                        /* Online */
      case 'O':
      case 'c':                        /* CStat  */
      case 'C': *buffer = '/';
                *(clipoi+clicnt) = '\0';
                strcpy(buffer+1, (char *) clipoi);
                break;
#ifdef CONVERS_HOSTNAME
      case 'm':
      case 'M':
      case 'h':
      case 'H': *buffer = '/';
                *(clipoi+clicnt) = '\0';
                strcpy(buffer+1, (char *) clipoi);
                break;
#endif
      default:  channel = nxtlong(&clicnt, &clipoi);
    }
  }

#ifdef L1IRC
  cp->IrcMode = userpo->IrcMode;
#endif /* L1IRC */
  userpo->convers = cp;
  callss2str(name, usrcal);
  strlwr(name);
#ifdef CONVERS_LINKS
  strcpy(call,name);
  strupr(call);
#endif
  cp->type = CT_UNKNOWN;
  cp->operator = userpo->sysflg ? 2 : 0; /* Sysop-Status durchreichen */

#ifdef CONV_CHECK_USER
  {
    CONNECTION *pP;

    if ((pP = CheckUserCVS(name)) != NULL)    /* Pruefung auf Doppel-Login. */
    {                                            /* Kein Login moeglich. */
#ifdef L1IRC
      if (cp->IrcMode == FALSE)
#endif /* L1IRC */
      {
        mbp = getmbp();
#ifdef SPEECH
        putprintf(mbp, speech_message(114)
                     , pP->name
                     , pP->host);
#else
        putprintf(mbp, "none login possible!\n"
                       "call %s is logged in at the host %s!\n"
                     , pP->name
                     , pP->host);
#endif /* SPEECH */
        prompt(mbp);
        seteom(mbp);
        return;
      }
    }
  }
#endif /* CONV_CHECK_USER */

  strcpy(cp->name, name);
  strcpy(cp->host, myhostname);

#ifdef CONVNICK
  memset(cp->OldNickname, 0, sizeof(cp->OldNickname));
#endif /* CONVNICK */

  if (*buffer) {
    putcvsstr(cp, "\r");
    strcpy(cnvinbuf, buffer);
  }
  else {
    for (i = 0; i < MAXCVSHOST; i++) {     /* besucht uns ein Host? */
      p = permarray[i];
#ifdef CONVERS_LINKS
      if (p == NUL)
          break;
      if ((!strcmp(p->cname, name)) || (cmpcal(p->call, strupr(call))))
#else
      if (p && !strcmp(p->cname, name))
#endif
          {
          if (p->locked) {                 /* Wird sofort abgewehrt */
            userpo->convers = NULLCONNECTION;
            connections = cp->next;
            free(cp);
            mbp = getmbp();
#ifdef SPEECH
            putstr(speech_message(5), mbp);
#else
            putstr("Locked!\r", mbp);
#endif
            prompt(mbp);
            seteom(mbp);
            return;
          }
          if (p->connection)        /* die fliegen hier gleich raus */
            bye_command2(p->connection, "link reorg");
          p = update_permlinks(name, cp, 0);
          if (p) {
            p->waittime = 60;       /* 60s Zeit zur /..host Eingabe */
            if (p->tries > 3)
              p->waittime = 120;
            p->retrytime = currtime + p->waittime;
          }
          userpo->convflag = 1; /* kam von aussen, kein recon in ccp */
          return;
      }
    }
#ifndef L1IRC
    sprintf(cnvinbuf, "/NAME %s %ld\r", name, channel);
#else
    if (cp->IrcMode == TRUE)
      sprintf(cnvinbuf, "USER %s\r", name);
    else
      sprintf(cnvinbuf, "/NAME %s %ld\r", name, channel);
#endif /* L1IRC */
  }

  process_input(cp);

  if (*buffer) {           /* Kommandoausgaben ausgeben cp abmelden */
    cvs2tnn(cp);
    userpo->convers = NULLCONNECTION;
    connections = cp->next;
    free(cp);
    mbp = getmbp();
    putchr(CR, mbp);
    prompt(mbp);
    seteom(mbp);
  }
}

/*------------------------------------------------------------------------*/

#else
#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn -par
#endif

BOOLEAN convers_input(MBHEAD *mhdp)
{
  if (mhdp->mbgc >= mhdp->mbpc)
    return(TRUE);

  return(FALSE);
}

void convers_init(void)
{
}

void conversd()
{
}

void convers_output()
{
}

void ccpcvs()
{
  inv_cmd();
}
#if !defined(__WIN32__) && !defined(__LINUX__)
#pragma warn .par
#endif
#endif
/* End of $RCSfile$ */
