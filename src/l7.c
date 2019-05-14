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
/* File src/l7.c (maintained by: ???)                                   */
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

static BOOLEAN itoccp(MBHEAD *);
static void kill_partner(UID);
static void clrccp(void);

/************************************************************************/
/*----                                                              ----*/
/*----------------------------------------------------------------------*/
void initl7(void)
{
  UID     uid;

  time(&sys_time);                      /* Systemzeit lesen             */
  sys_localtime = localtime(&sys_time);

  init_crctab();                        /* CRC-Tabelle aufbauen         */
  inithd(&usccpl);                      /* CCP-User-Liste (L7)          */
  inithd(&userhd);                      /* USER-Liste                   */
  inithd(&cq_user);                     /* CQ-Rufer                     */
  inithd(&cq_statl);                    /* Connect-Meldungen bei CQ     */

  for (uid = 0; uid < NUMPAT; uid++)
    clrptc(uid);

  convers_init();                       /* CONVERS                      */

  tic1s = 0;
}

/************************************************************************/
/*                                                                      */
/* Informationstransfer vom L7.                                         */
/*                                                                      */
/* Frames fuer den Level 2 werden mit der gewuenschten PID gesendet.    */
/* (mbp->l2fflg)                                                        */
/*                                                                      */
/*----------------------------------------------------------------------*/
BOOLEAN
itousr(UID to_uid, UID fm_uid, BOOLEAN conflg, MBHEAD *mbp)
{
  BOOLEAN ok = FALSE;
  int     bytes = mbp->mbpc - mbp->mbgc;

  if (g_utyp(fm_uid) != L4_USER)
    mbp->morflg = 0;
  mbp->l2link = g_ulink(to_uid);
  switch (g_utyp(to_uid))               /* je nach Typ                  */
  {
    case L4_USER:
      ok = itocir(conflg, mbp);
      break;
    case L2_USER:
      ok = itolnk(mbp->l2fflg, conflg, mbp);
      break;
    case HOST_USER:
      ok = itohst(conflg, mbp);
      break;
#ifdef L1TCPIP
    case TCP_USER:
      ok = itoTCP(conflg, mbp);
      break;
#endif /* L1TCPIP */
 }
  if (ok)
  {
    ptctab[to_uid].infotx += bytes;
    if (fm_uid != NO_UID)
      ptctab[fm_uid].inforx += bytes;
  }
  return(ok);
}

/************************************************************************/
/*                                                                      */
/* IP-Frames herrausfiltern, nur PID F0 (L2CPID) durchlassen.           */
/*                                                                      */
/************************************************************************/
static BOOLEAN itoccp(MBHEAD *mbp) {
  int     uid;
  PTCENT *ptcp;

  mbp->type = uid = g_uid(mbp->l2link, (unsigned char)mbp->type);
  ptcp = ptctab + uid;
  ptcp->inforx += (mbp->mbpc-mbp->mbgc);   /* reine Info ohne Header  */

  switch (mbp->l2fflg) {
#ifdef IPROUTE
    case L2CIP       :
      relink(ulink((LEHEAD *)mbp), (LEHEAD *)iprxfl.tail);
      break;

    case L2CARP      :
      relink(ulink((LEHEAD *)mbp), (LEHEAD *)arprxfl.tail);
      break;
#endif

    case L2CPID      :
      relink(ulink((LEHEAD *)mbp), (LEHEAD *)userhd.tail);
      break;

    default          :
      dealmb((MBHEAD *)ulink((LEHEAD *)mbp));
      break;
  }

  return(TRUE);
}

/************************************************************************/
/*                                                                      */
/* Informationstransfer in den L7.                                      */
/*                                                                      */
/* Wenn der Link einen Partner hat, die Daten direkt transferieren,     */
/* ansonsten die Daten in den CCP.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
BOOLEAN fmlink(BOOLEAN conflg, MBHEAD *mbp)
{
  PTCENT *ptcp;
  UID     p_uid;
  UID     uid;

  uid  = g_uid(mbp->l2link, (unsigned char)mbp->type);
  ptcp = ptctab + uid;

  p_uid = ptcp->p_uid;                     /* gibts einen Partner-Link? */
  if (ptcp->local == PTC_LOCAL) {          /* Locals senden nie in CCP  */
    if (p_uid != NO_UID)                   /* gibts einen Partner-Link? */
      return(itousr(p_uid, uid, conflg, mbp));
    /* Wenn der Link noch keinen Partner hat, haben wir l7tx() noch     */
    /* nicht erreicht, um den Connect einzuleiten. Wir lehnen die Info  */
    /* aus dem L2 solange ab, bis der Weiterconnect steht.              */
    return(FALSE);                         /* Info abgelehnt            */
  } else {
    if (ptcp->ublk)                        /* noch im CCP?              */
      return(itoccp(mbp));                 /* Info in den CCP           */
    if (p_uid != NO_UID)                   /* gibts einen Partner-Link? */
      return(itousr(p_uid, uid, conflg, mbp));
    return(itoccp(mbp));                   /* den Rest auch in den CCP  */
  }
}

/************************************************************************/
/*                                                                      */
/*----------------------------------------------------------------------*/
static void kill_partner(UID this_uid) {
  UID     uid;
  PTCENT *ptcp;

  for (uid = 0, ptcp = ptctab; uid < NUMPAT; uid++, ptcp++)
    if (ptcp->p_uid == this_uid) {
      disusr(uid);
      ptcp->p_uid = NO_UID;
    }
}

/************************************************************************/
/*                                                                      */
/* Statusmeldungen an L7. Es wird auf Connect, Disconnect, Busy,        */
/* Failure reagiert.                                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/
void
l2tol7(WORD opcod, void *userlink, WORD typ)
{
  UID      uid = g_uid(userlink, (unsigned char)typ);
  UID      p_uid;
  PTCENT  *ptcp;
  PTCENT  *p_ptcp;
  MBHEAD  *mbp;
  LNKBLK  *oldlp = lnkpoi;
  CIRBLK  *oldcp = cirpoi;
  HOSTUS  *oldhp = hstusr;
  USRBLK  *oldup = userpo;
#ifdef L1TCPIP
  TCPIP  *oldtc = tcppoi;
#endif /* L1TCPIP */

/* Wenn das Sysop-Port Flag gesetzt ist, duerfen wir keinen via-Connect */
/* zulassen, weil eine Password-Pruefung nicht moeglich ist.            */

  if (typ == L2_USER && opcod == L2MCONNT)
  {
    if (lnkpoi->state == L2SHTH)
      if (portpar[lnkpoi->liport].l2mode & MODE_s)
      {
        rejlnk(lnkpoi);
        return;
      }
  }

  ptcp   = ptctab + uid;                /* Zeiger auf den Patchcord     */
  p_uid  = ptcp->p_uid;                 /* Partner-UID, wenn vorhanden  */
  switch (opcod)                        /* je nach Opcode               */
  {
    case L2MCONNT:
    case L7MCONNT:
   if (p_uid != NO_UID)          /* hat der Link einen Partner?      */
      {
        p_ptcp = ptctab + p_uid;    /* Zeiger auf Partner-Patchcord     */
        userpo = p_ptcp->ublk;      /* Userblock des Partners im CCP    */
        if (userpo)
        {
          if (p_ptcp->local == PTC_NORMAL)
            msgfrm(opcod == L2MCONNT
                   ? DOWNLINK
                   : CQ_LINK, uid, conmsg);
#ifdef CONL3LOCAL
          if (g_utyp(p_uid) == L4_USER) /* Partner kommt via L4?        */
            ackcir(g_ulink(p_uid));     /* ja? mit CACK bestaetigen     */
#endif
          if (g_utyp(p_uid) == L2_USER) /* Partner kommt via L2?        */
            acklnk(g_ulink(p_uid));     /* falls HTH, bestaetigen       */
          clrccp();                     /* User aus dem CCP werfen      */
          ptcp->state = DOWNLINK;       /* ist ein Downlink             */
          p_ptcp->ublk = NULL;
        }
        else
        {
          kill_partner(uid);          /* eventuelle Partner loeschen    */
          disusr(uid);
          lnkpoi = oldlp;
          cirpoi = oldcp;
          hstusr = oldhp;
          userpo = oldup;
#ifdef L1TCPIP
          tcppoi = oldtc;
#endif /* L1TCPIP */
          return;
        }
      }
      else
      {
        kill_partner(uid);            /* eventuelle Partner loeschen    */
        if ((userpo = ptcp->ublk) != NULL)   /* Connect vom CCP         */
        {
          if (userpo->status == US_CHST)
          {
            userpo->status = US_CCP;
            send_proto("L7", "Entering convers");
            mbp = getmbp();
            putstr("convers\r", mbp); /* Partner in den Convers         */
            seteom(mbp);
            mbp = getmbp();
            putprintf(mbp, "/\377\200HOST %s %s %s\r", myhostname, myrev, myfeatures);
            seteom(mbp);
          }
                                    /* war nur ein Linkreset            */
        }
        else
        {                           /* neuer User                       */
          if (!new_user(FALSE, uid))         /* User abgelehnt?         */
          {
            disusr(uid);
            lnkpoi = oldlp;
            cirpoi = oldcp;
            hstusr = oldhp;
            userpo = oldup;
#ifdef L1TCPIP
            tcppoi = oldtc;
#endif /* L1TCPIP */
            return;
          }
          ptcp->state = UPLINK;             /* ein User ist Uplink      */
          ptcp->ublk = userpo;              /* Partnerblock eintragen   */
#ifdef USER_PASSWORD
          if (typ != L2_USER)
            userpo->pwdtyp = PW_USER;
          else
            if (!(portpar[lnkpoi->liport].l2mode & MODE_s))
              userpo->pwdtyp = PW_USER;
#endif
#ifdef USERPROFIL
          if (SearchPasswdProfil())
#endif /* USERPROFIL */
            send_ctext();                     /* CTEXT senden             */
        }
      }
      break;
    case L2MDISCF :
    case L2MBUSYF :
    case L2MFAILW :
      if (p_uid != NO_UID)          /* User hatte einen Partner         */
      {
        p_ptcp = ptctab + p_uid;
        if ((userpo = p_ptcp->ublk) == NULL)   /* Partner nicht im CCP? */
        {
          if (p_ptcp->recflg)                  /* moechte aber zurueck  */
          {
            if (new_user(TRUE, p_uid))         /* dann erneut in CCP    */
            {
              msgfrm(DOWNLINK, NO_UID, recmsg);/* Reconnected to ...    */
              p_ptcp->ublk = userpo;
            }
            else
              disusr(p_uid);
          }
          else
            disusr(p_uid);          /* sonst Partner abwerfen           */
        }
        else
        {                           /* Partner ist noch im CCP          */
          if (p_ptcp->local == PTC_LOCAL)   /* Local?                   */
          {
            if (g_utyp(p_uid) == L2_USER && opcod == L2MBUSYF)
              rejlnk(g_ulink(p_uid));       /* Busy durchreichen        */
            disusr(p_uid);
          }
          else
          {
            if (opcod != L2MDISCF)
              msgfrm(DOWNLINK, uid, (opcod == L2MBUSYF) ? dmmsg : failmsg);
            p_ptcp->local = PTC_NORMAL;
            userpo->status = US_CCP;
          }
        }
        p_ptcp->p_uid = NO_UID;
      }
      if ((userpo = ptcp->ublk) != NULL)
        clrccp();
      clrptc(uid);
      kill_partner(uid);            /* eventuelle Partner loeschen      */
  }
  lnkpoi = oldlp;
  cirpoi = oldcp;
  hstusr = oldhp;
  userpo = oldup;
#ifdef L1TCPIP
  tcppoi = oldtc;
#endif /* L1TCPIP */
}

/************************************************************************/
/*                                                                      */
/* Usereintrag aus der CCP-Userliste loeschen.                          */
/* Vorher die von diesem User verwendeten Resourcen freigeben.          */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void
clrccp(void)
{
  MBHEAD *mbp;
  char    file[128];
  CQBUF  *cqp;

  if (userpo->sysflg > 1)
    save_stat();

  if (userpo->monitor)
  {
    moncmd(NULL, userpo->monitor, "N", 1);      /* Monitor abschalten   */
    dealoc((MBHEAD *)userpo->monitor);
    userpo->monitor = NULL;
  }

  for (cqp = (CQBUF *)cq_user.head;             /* CQ-Liste durchsuchen */
       cqp != (CQBUF *)&cq_user;
       cqp = cqp->next)
  {
    if (userpo->uid == cqp->uid)                /* Eintrag gefunden     */
      dealoc((MBHEAD *)ulink((LEHEAD *)cqp));   /* CQ-Eintrag loeschen  */
    break;
  }

  if (   userpo->status == US_WBIN              /* Upload beenden       */
      || userpo->status == US_RBIN
      || userpo->status == US_RTXT
     )
  {
    fclose(loadfp);
#ifndef MC68302
    xremove(loadtmp);
#else
    xremove(loadname);
#endif
    loadname[0] = loadtmp[0] = NUL;
  }

  if (userpo->fp != NULL)                       /* Read beenden         */
  {
    fclose(userpo->fp);
    userpo->fp = NULL;
    if (userpo->fname != NULL)
    {
      xremove(userpo->fname);
      free(userpo->fname);
      userpo->fname = NULL;
    }
  }

  if (userpo->convers != NULLCONNECTION)           /* Convers beenden */
  {
    userpo->convers->up = NULL;
    send_proto("L7", "link failure with %s", userpo->convers->name);
    if (userpo->convers->type == CT_HOST)
    {
#ifdef SPEECH
    sprintf(file, speech_message(139), myhostname, userpo->convers->name);
#else
    sprintf(file, "%s<>%s broken", myhostname, userpo->convers->name);
#endif
      bye_command2(userpo->convers, file);
    }
    else
      bye_command2(userpo->convers, "link failure");
  }
  userpo->convers = NULLCONNECTION;
  userpo->convflag = 0;

  userpo->status = US_NULL;

  if ((mbp = userpo->mbhd) != NULL)
     dealmb(mbp);

#ifdef PACSAT
  if (userpo->pacsat != NULL)
  {
    if (userpo->pacsat->tempfp != NULL)
    {
      fclose(userpo->pacsat->tempfp);
      xremove(userpo->pacsat->tempfile);
      free(userpo->pacsat->tempfile);
    }
    dealoc((MBHEAD *)userpo->pacsat);
    userpo->pacsat = NULL;
  }
#endif

  dealoc((MBHEAD *)ulink((LEHEAD *)userpo));
  userpo = NULL;
}

/************************************************************************/
/*----                                                              ----*/
/*----------------------------------------------------------------------*/
void disusr(UID uid)
{
  LNKBLK *cblk2;
  CIRBLK *cblk4;
  HOSTUS *cblk0;
#ifdef L1TCPIP
  TCPIP  *cblk8;
#endif /* L1TCPIP */

  switch (g_utyp(uid)) {
    case L4_USER:
             cblk4 = cirpoi;
             cirpoi = (CIRBLK *) g_ulink(uid);
             discir();
             cirpoi = cblk4;
             break;
    case L2_USER:
             cblk2 = lnkpoi;
             lnkpoi = (LNKBLK *) g_ulink(uid);
             dsclnk();
             lnkpoi = cblk2;
             break;
    case HOST_USER:
             cblk0 = hstusr;
             hstusr = (HOSTUS *) g_ulink(uid);
             hstout();
             hstusr = cblk0;
             break;
#ifdef L1TCPIP
     case TCP_USER:
             cblk8 = tcppoi;
             tcppoi = (TCPIP *) g_ulink(uid);
             SetDiscTCP();
             tcppoi = cblk8;
             break;
#endif /* L1TCPIP */
  }
}

/************************************************************************/
/*----                                                              ----*/
/* Neuen Message-Buffer besorgen und Inhalte umkopieren. Dabei je       */
/* nach Paclen-Parameter entsprechend viele Packete der vorgegebenen    */
/* Laenge erzeugen und mit send_msg() an L2, L4 oder Host weitergeben.  */
/*----------------------------------------------------------------------*/
void seteom(MBHEAD *mbp1)
{
  MBHEAD *mbp2;
#ifdef PORT_MANUELL
  LNKBLK *link;
  UWORD   PacLen;
  UID     uid;
#endif /* PORT_MANUELL */

  rwndmb(mbp1);

  if (mbp1->mbpc == mbp1->mbgc) {               /* Info im Frame?       */
    dealmb(mbp1);                               /* Nein, dann weg damit */
    return;
  }

  mbp2 = (MBHEAD *) allocb(ALLOC_MBHEAD);
  mbp2->l2link = mbp1->l2link;
  mbp2->type = mbp1->type;

#ifdef PORT_MANUELL
  uid = g_uid(mbp1->l2link, (unsigned char)mbp1->type);

  switch (g_utyp(uid))
  {
    case L2_USER:
      link = g_ulink(uid);
      PacLen = portpar[link->liport].paclen;
    break;

#ifdef L1TCPIP
    case TCP_USER:
      link = g_ulink(uid);
      PacLen = TXLEN;
    break;
#endif /* L1TCPIP */

    default:
      PacLen = 236;
    break;
  }
#endif /* PORT_MANUELL */
  while (mbp1->mbpc > mbp1->mbgc) {
    putchr(getchr(mbp1), mbp2);

#ifdef PORT_MANUELL
        if (mbp2->mbpc == PacLen) {
#else
        if (mbp2->mbpc == paclen) {
#endif /* PORT_MANUELL */
      send_msg(TRUE, mbp2);
      mbp2 = (MBHEAD *) allocb(ALLOC_MBHEAD);
      mbp2->l2link = mbp1->l2link;
      mbp2->type = mbp1->type;
    }
  }

  if (mbp2->mbpc > mbp2->mbgc)
    send_msg(TRUE, mbp2);
  else
    dealmb(mbp2);
  dealmb(mbp1);
}

/*------------------------------------------------------------------------*/

BOOLEAN send_msg(BOOLEAN conflg, MBHEAD *mbp)
{
  static LHEAD mbhd;

  rwndmb(mbp);
  inithd(&mbhd);
  relink((LEHEAD *)mbp, (LEHEAD *)mbhd.tail);
  mbp->l2fflg = L2CPID;

  return(itousr(g_uid(mbp->l2link, (unsigned char)mbp->type), NO_UID, conflg, mbp));
}

/************************************************************************/
/*--- neuen User in Liste aufnehmen                                  ---*/
/*                                                                      */
/* Parameter im Aufruf:                                                 */
/*      check = TRUE: nicht aufnehmen, wenn Nachbar oder L2-Link        */
/*      uid:          User-ID des Users                                 */
/*                                                                      */
/* Rueckgabe: FALSE, wenn User abgelehnt.                               */
/*----------------------------------------------------------------------*/
BOOLEAN new_user(BOOLEAN check, UID uid)
{
  PERMLINK *p;
  WORD     i;
  char     call[L2IDLEN];
  MBHEAD  *mbp;
  FILE    *fp;
  char     name[MAXPATH];

  userpo = NULL;
  if (check) {
    cpyid(call,calofs(UPLINK, uid));
    for (i = 0; i < MAXCVSHOST; i++) {
          p = permarray[i];
          if (p && cmpid(call,p->call))
          return(FALSE);
    }
  }

/*--- fuer alle uebrigen User:                                          */
  userpo = (USRBLK *) allocb(ALLOC_USRBLK2);    /* Kontrollblock holen  */
  userpo->mbhd = NULL;                  /* keine Info wartend           */
#ifdef PACSAT
  userpo->pacsat = NULL;                /* User noch nicht in der Box   */
#endif
  userpo->monitor = NULL;               /* User will keinen Monitor     */
  userpo->auditlevel = 0;
  userpo->sysflg = 0;                   /* erstmal nicht Sysop          */
#ifdef USER_PASSWORD
  userpo->pwdtyp = PW_NOPW;             /* noch kein Password           */
#endif
  if (g_utyp(uid) == HOST_USER)
   {
    if (   !ishmod                           /* Terminalmodus -> Sysop  */
        || (HOSTUS *)g_ulink(uid) == hstubl) /* Hostkanal 0 auch (TNBs) */
      userpo->sysflg = 1;
   }
  userpo->uid = uid;
  userpo->errcnt = 0;

  userpo->status = US_CCP;              /* am CCP, Befehl kommt         */
  userpo->convers = NULLCONNECTION;
  userpo->convflag = 0;                 /* noch nicht in CONVERS        */
  userpo->talkcall[0] = '\0';
  userpo->fp = NULL;
  userpo->fname = NULL;                 /* kein Dateiname gespeichert   */
#ifdef MAKRO_FILE
  userpo->read_ok = FALSE;
#endif
#ifdef L1IRC
  userpo->IrcMode = FALSE;
#endif /* L1IRC */

#ifdef PORT_SUSPEND
  if (check == FALSE)
  {
  if (port_suspend_enabled(rxfprt)) {           /* Wurde Flag l gesetzt ?(Port gesperrt)   */
          if (is_port_suspended(userpo)) {          /* Ja, dann checken wir ob dieser L2       */
       strcpy(name, textpath);                  /* auch ein eingetragener LINK ist!!!      */
       strcat(name, "LOCK.TXT");                /* Hat der Sysop eine LOCK.TXT erstelt.    */
      mbp = getmbp();
          if ((fp = xfopen(name, "rt")) != NULL) {  /* Wenn ja, oeffnen wir diese und geben den*/
      while ((i = getc(fp)) != EOF)             /* Inhalt dem L2. Anschliessend disconnect. */
          putchr((BYTE)(i == '\n' ? '\r' : i), mbp);

      fclose(fp);
      putchr('\r', mbp);
    } else
    putstr("INTERLINK.\r", mbp);                 /* Bevor der L2 Disconnectet wird, bekommt */
    seteom(mbp);                                 /* er noch eine Meldung, WARUM.            */
    dealoc((MBHEAD *)userpo);
    return(FALSE);                               /* ablehnen                                */
        }
  }
 }
#endif
  if (is_suspended(userpo)) {
    strcpy(name, textpath);
    strcat(name, "SUSPEND.TXT");
    mbp = getmbp();
    if ((fp = xfopen(name, "rt")) != NULL) {
      while ((i = getc(fp)) != EOF)
        putchr((char)(i == '\n' ? '\r' : i), mbp);

      fclose(fp);
      putchr('\r', mbp);
    } else
#ifdef SPEECH
      putstr(speech_message(140), mbp);
#else
      putstr("You are suspended.\r", mbp);
#endif
    seteom(mbp);
    dealoc((MBHEAD *)userpo);
    return(FALSE);                      /* ablehnen                     */
  }
                                        /* neuen User in Kette haengen  */
  relink((LEHEAD *)userpo, (LEHEAD *) usccpl.tail);

  return(TRUE);
}

/************************************************************************\
*                                                                        *
* "get user id"                                                          *
*                                                                        *
* Die User-ID-Nummer fuer einen Link-Pointer und einen Link-Typ (Level 2,*
* Level 4, Host) feststellen. Die UID setzt sich wie folgt zusammen:     *
*                                                                        *
* 0               .. LINKNMBR-1                 Level 2 Links            *
* LINKNMBR        .. LINKNMBR+NUMCIR-1          Level 4 Circuits         *
* LINKNMBR+NUMCIR .. LINKNMBR+NUMCIR+MAXHST-1   Host User                *
*                                                                        *
\************************************************************************/
UID g_uid(void *userlink, UBYTE typ) {

        switch (typ) {
    case L2_USER:                   /* Level 2 User                     */
      return((UWORD)(((LNKBLK *)userlink) - lnktbl));
    case L4_USER:                   /* Level 4 User                     */
      return(((UWORD)(((CIRBLK *)userlink) - cirtab)) + LINKNMBR);
#ifndef L1TCPIP
    case HOST_USER:                 /* Host User                        */
      return(((UWORD)(((HOSTUS *)userlink) - hstubl)) + LINKNMBR + NUMCIR);
#else
    case HOST_USER:                 /* Host User                        */
      return(((UWORD)(((HOSTUS *)userlink) - hstubl)) + LINKNMBR + NUMCIR + MAXTCPIP);
    case TCP_USER:                   /* TCPIP User/Link                  */
      return(((UWORD)(((TCPIP *)userlink) - tcptbl)) + LINKNMBR + NUMCIR);
#endif /* L1TCPIP */
  }
  return(NO_UID);
}

/************************************************************************\
*                                                                        *
* "get user typ"                                                         *
*                                                                        *
* Den User-Typ eines User anhand seiner UID feststellen.                 *
*                                                                        *
* 0               .. LINKNMBR-1                 Level 2 Links            *
* LINKNMBR        .. LINKNMBR+NUMCIR-1          Level 4 Circuits         *
* LINKNMBR+NUMCIR .. LINKNMBR+NUMCIR+MAXHST-1   Host User                *
*                                                                        *
\************************************************************************/
UBYTE g_utyp(UID uid) {
#ifndef L1TCPIP
  if (uid >= LINKNMBR+NUMCIR)
    return(HOST_USER);
#else
  if (uid >= LINKNMBR+NUMCIR+MAXTCPIP)
    return(HOST_USER);

  if (uid >= LINKNMBR+NUMCIR)
    return(TCP_USER);
#endif /* L1TCPIP */
  if (uid >= LINKNMBR)
    return(L4_USER);
  return(L2_USER);
}

/************************************************************************\
*                                                                        *
* "get user link"                                                        *
*                                                                        *
* Den User-Link eines User anhand seiner UID feststellen.                *
*                                                                        *
* 0               .. LINKNMBR-1                 Level 2 Links            *
* LINKNMBR        .. LINKNMBR+NUMCIR-1          Level 4 Circuits         *
* LINKNMBR+NUMCIR .. LINKNMBR+NUMCIR+MAXHST-1   Host User                *
*                                                                        *
\************************************************************************/
void *g_ulink(UID uid) {
  switch (g_utyp(uid)) {
    case L2_USER : return(lnktbl + uid);
    case L4_USER : return(cirtab + uid - LINKNMBR);
#ifndef L1TCPIP
  }
  return(hstubl + uid - (LINKNMBR + NUMCIR));
#else
    case TCP_USER : return(tcptbl + uid - (LINKNMBR + NUMCIR));
  }
  return(hstubl + uid - (LINKNMBR + NUMCIR + MAXTCPIP));
#endif /* L1TCPIP */
}

/************************************************************************\
*                                                                        *
* "reset patchcord entry"                                                *
*                                                                        *
\************************************************************************/
void resptc(UID uid) {
  PTCENT *ptcp = ptctab + uid;

  ptcp->inforx  = ptcp->lastrx =
  ptcp->infotx  = ptcp->lasttx =
  ptcp->rxbps   = ptcp->txbps  = 0L;
  ptcp->contime = 1L;               /* Ueberlauf verhindern             */
}

/************************************************************************\
*                                                                        *
* "clear patchcord entry"                                                *
*                                                                        *
\************************************************************************/
void clrptc(UID uid) {
  PTCENT *ptcp = ptctab+uid;

  resptc(uid);
  ptcp->p_uid   = NO_UID;
  ptcp->ublk    = NULL;
  ptcp->state   = EMPTY;
  ptcp->local   = PTC_NORMAL;       /* 18.10.97 (1) hier zuruecksetzen  */
  ptcp->recflg  = FALSE;
}

/* End of src/l7.c */
