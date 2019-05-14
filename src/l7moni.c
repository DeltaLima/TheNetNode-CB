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
/* File src/l7moni.c (maintained by: ???)                               */
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

UWORD   tracnt = 0;                 /* Anzahl der laufenden TRACEes     */
#ifdef BUFFER_DEBUG
MONBUF  consmon = {NULL, NULL, ALLOC_MONBUF, 0, 255, 0, ""};
#else
MONBUF  consmon = {0, 255, 0, ""};  /* default: Console Monitor aus     */
#endif
extern MBHEAD    *hstmbp;

static void     putmb(MBHEAD *, MBHEAD *);
static void     frimon(MBHEAD *, MBHEAD *);
static BOOLEAN  owndigipt(const char *);
static void     statsv(int, int, BOOLEAN, int);
static void     dump_nodes(MBHEAD *, MBHEAD *);
static void     dump_inp_nodes(MBHEAD *, MBHEAD *);
static void     dump_netrom(MBHEAD *, MBHEAD *);
static void     dump_nrr(MBHEAD *, MBHEAD *);
#ifdef IPROUTE
static void     dump_arp(MBHEAD *, MBHEAD *);
static void     dump_ip(MBHEAD *, MBHEAD *);
static void     tcp_DumpHeader(MBHEAD *, MBHEAD *, IP *);
static void     dump_udp(MBHEAD *, MBHEAD *, IP *);
static void     dump_icmp(MBHEAD *, MBHEAD *, IP *);
#endif
static void     dump_frag(MBHEAD *, MBHEAD *);
static void     frhmon(MBHEAD *, MBHEAD *);
static void     nethmon(MBHEAD *, MBHEAD *);
static BOOLEAN  ismonf(MBHEAD *, MONBUF *);
static BOOLEAN invial(char *, char *);


static void putmb(MBHEAD *mbp, MBHEAD *mbp2)
{
  UBYTE c;

  rwndmb(mbp2);
  while (mbp2->mbgc < mbp2->mbpc) {
    c = getchr(mbp2);
    if (c != TAB && c != LF) {
      if (c != CR && c < ' ') {
        putchr('^', mbp);
        c += '@';
      }
      putchr(c, mbp);
    }
  }
}

/* Info-Daten umkopieren */
static void frimon(MBHEAD *mbp, MBHEAD *fbp)
{
  BOOLEAN cr = (fbp->mbgc < fbp->mbpc);
  UBYTE   ch;

  while (fbp->mbgc < fbp->mbpc) {   /* verfuegbare Info holen           */
    ch = getchr(fbp);               /* ein Byte holen                   */
    if (cr) {                       /* wenn ein CR aussteht             */
      if (ch == CR) cr = FALSE;     /* es ist schon gekommen            */
    } else {                        /* kein CR ausstehen                */
      if (ch != CR && ch != LF)     /* CR,LF sind ok, sonst CR          */
        cr = TRUE;                  /* anfordern                        */
    }
    if (ch != LF)                   /* Line-Feed filtern                */
    putchr(ch, mbp);                /* Info umkopieren                  */
  }
  if (cr) putchr(CR, mbp);          /* neue Zeile am Schluss            */
}

/*----------------------------------------------------------------------*/
/* Frames fuer Monitor bearbeiten                                       */
/*----------------------------------------------------------------------*/
void monitor(MBHEAD *fbp)
{
  MBHEAD    *l2mbp;
  MBHEAD    *netmbp;
  MBHEAD    *imbp;
  MBHEAD    *tmpmbp;
  char huge *mbbp;
  UWORD      mbgc;
  char       inflen[128];

  mbbp = fbp->mbbp;
  mbgc = fbp->mbgc;

  /* Bei I- und UI-Frames ist fbp->mbgc um eins zu klein, wegen PID     */
#ifndef MHRXTXBYTESFIX
  statsv(fbp->mbpc,
#else
  statsv(fbp->mbpc - fbp->mbgc,
#endif /* MHRXTXBYTESFIX */
         (fbp->mbgc < fbp->mbpc) ? fbp->mbgc + 1 : fbp->mbgc,
         fbp->tx,
         fbp->repeated);

  if (consmon.Mpar || tracnt)           /* nur wenn Monitor oder Trace  */
   {
    l2mbp = (MBHEAD *)allocb(ALLOC_MBHEAD);  /* Buffer fuer L2-Header   */
    netmbp = (MBHEAD *)allocb(ALLOC_MBHEAD); /* Buffer fuer L3/4-Header */
    imbp = (MBHEAD *)allocb(ALLOC_MBHEAD);   /* und einen fuer die Info */
    frhmon(l2mbp, fbp);                 /* L2-Header auswerten          */
    netmbp->type = l2mbp->type;         /* PID kopieren                 */
    nethmon(netmbp, fbp);               /* L3/4-Header auch             */
    if (!(rxfctl & L2CNOIM))
     {
      if (fbp->tx && fbp->repeated > 0)
        sprintf(inflen, "[Info %u Bytes repeated: %u]",
                fbp->mbpc - fbp->mbgc,
                fbp->repeated);
      else
        sprintf(inflen, "[Info %u Bytes]",
                fbp->mbpc - fbp->mbgc);
     }
    else
     {
      inflen[0] = NUL;
     }
    frimon(imbp, fbp);                  /* Infofeld auswerten           */
    if (tracnt)                         /* min. ein User will lauschen  */
     {
      for (userpo =  (USRBLK *) usccpl.head;
           userpo != (USRBLK *) &usccpl;
           userpo =  (USRBLK *) userpo->unext)
       {
#ifdef USER_MONITOR
        if (userpo->status == US_TRAC && userpo->monitor)
#else
        if (userpo->status == US_CCP && userpo->monitor)
#endif /* USER_MONITOR */
         {
          if (ismonf(fbp, userpo->monitor))
           {
            tmpmbp = getmbp();          /* ein Buffer fuer den User     */
            putmb(tmpmbp, l2mbp);       /* L2-Header ausgeben           */
            if (userpo->monitor->Mpar & MONT)
             {
              putstr(" - ", tmpmbp);
              puttim(&sys_time, tmpmbp);
              putlong(tic10, FALSE, tmpmbp);
             }
            putmb(tmpmbp, netmbp);             /* L3/4_Header hinterher */
            if (   (userpo->monitor->Mpar & MONL)
                && !(rxfctl & L2CNOIM))
              putprintf(tmpmbp, "%s\r", inflen);/* Infolaenge anzeigen  */
            if (userpo->monitor->Mpar & MONF)  /* wenn gewuenscht, auch */
              putmb(tmpmbp, imbp);             /* die Infos ausgeben    */

/* Damit der User nicht mehr Info bekommt, als er abnimmt, wird die     */
/* Ausgabe verworfen, wenn sein Link abgefuellt ist.                    */

            if (!send_msg(FALSE,tmpmbp))
              dealmb(tmpmbp);
          }
        }
      }
    }
    if (   monlin < 100                 /* falls nix abgeholt wird      */
        && ismonf(fbp, &consmon))       /* Ausgabe gewuenscht           */
     {
      if (consmon.Mpar & MONT)
       {
        putstr(" - ", l2mbp);
        puttim(&sys_time, l2mbp);
       }
      l2mbp->type = HMRMONH;
      if (!ishmod)
       {
        putmb(netmbp, l2mbp);
        if (   (consmon.Mpar & MONL)
            && !(rxfctl & L2CNOIM))
            putprintf(l2mbp, "\r%s", inflen);
       }
      dealmb(netmbp);
      rwndmb(l2mbp);
      relink((LEHEAD *)l2mbp, (LEHEAD *)smonfl.tail);
      monlin++;
      if ((imbp->mbpc != 0) && (consmon.Mpar & MONF))
       {
        l2mbp->type = HMRMONIH;
        if (ishmod)             /* im Hostmode direkt das Infofeld      */
         {                      /* ausgeben, nicht die ^A gewandelte    */
          dealmb(imbp);
          imbp = (MBHEAD *)allocb(ALLOC_MBHEAD);
          fbp->mbbp = mbbp;
          fbp->mbgc = mbgc;
          getchr(fbp);              /* ueberlesen                   */
          while ((fbp->mbgc < fbp->mbpc) && (imbp->mbpc < 256))
            putchr(getchr(fbp), imbp);
         }
        rwndmb(imbp);                           /* zurueckspulen        */
        imbp->type = HMRMONI;
        relink((LEHEAD *)imbp, (LEHEAD *)smonfl.tail);
        monlin++;
       }
      else
        dealmb(imbp);   /* Info-Buffer freigeben                        */
     }
    else                /* Host ohne Monitor                            */
     {
      dealmb(l2mbp);    /* noch belegte Buffer freigeben                */
      dealmb(netmbp);
      dealmb(imbp);
     }
    fbp->mbbp = mbbp;
    fbp->mbgc = mbgc;
  }
}

static BOOLEAN
owndigipt(const char *digis)
{
  if (*digis)
  { /* via eigenes Call, zaehlt nicht als via in der Statistik */
    if (istome(digis) && !*(digis + L2IDLEN))
      return (TRUE);
  }
  return (FALSE);
}

/*----------------------------------------------------------------------*/
/* Statistik Arbeiten                                                   */
/*----------------------------------------------------------------------*/
static void
statsv(int count, int over_count, BOOLEAN tx, int repeated)
{
  WORD          i;                      /* zum Zaehlen diverser Sachen  */
  STAT         *statp;                  /* Zeiger auf Statistiktabelle  */
  char         *viap;
  const char   *p;
  char         *callp;
  MHEARD       *mhp;
  PORTSTAT     *pstatp = portstat + (int)rxfprt;
  WORD          trx = tx ? 1 : 0;
#ifdef MH_LISTE
  char          via[8 * L2IDLEN + 1];
  char          SaveHeader[L2AFLEN + 1];
#endif

  throughput += count;                  /* fuer den Gesammtdurchsatz    */

/*------------------------------------- Portstatistikliste updaten -----*/
/* gesendete und empfangene Bytes zaehlen                               */
  if (!tx)
  {
    pstatp->rx_bytes += (ULONG) count;
    pstatp->rx_overhead += (ULONG) over_count;
  }
  else
  {
    pstatp->tx_bytes += (ULONG) count;
    pstatp->tx_overhead += (ULONG) over_count;
  }
/*------------------------------------------- Portgraph updaten --------*/
#ifdef PORTGRAPH
  if (!(rxfctl & L2CNOIM))
  {
    graph_actual_trxpeak(&graph.info[(int)rxfprt], tx);
  }
  else
    if (!(rxfctl & L2CNOSM))
    {
      if ((rxfctl & ~L2CPF) == L2CREJ)
        graph_actual_trxpeak(&graph.reject[(int)rxfprt], tx);
    }
    else
    {
      switch (rxfctl & ~L2CPF)
      {
        case L2CSABM:
#ifdef EAX25
        case L2CSABME:
#endif
          graph_actual_trxpeak(&graph.sabm[(int)rxfprt], tx);
          break;
        case L2CDISC:
          graph_actual_trxpeak(&graph.disc[(int)rxfprt], tx);
          break;
        case L2CDM:
          graph_actual_trxpeak(&graph.dm[(int)rxfprt], tx);
          break;
        case L2CFRMR:
          graph_actual_trxpeak(&graph.frmr[(int)rxfprt], tx);
          break;
      } /* switch */
    } /* else */
#endif

/************************************************************************/
/*  MH-Liste bearbeiten                                                 */
/************************************************************************/
#ifdef MH_LISTE
    /* Original Haeder sichern. */
    strncpy(SaveHeader, rxfhdr, 2 * L2IDLEN + L2VLEN + 1);
    /* Hole das aktuelle via-call. */
    strncpy(via, dheardcall(rxfhdr), 8 * L2IDLEN);

    /* Unser Eigenes Knotencall oder call was zuletzt gedigipeated hat. */
    if (via[0] != FALSE)
    {
      /* Eigenes Knotencall. */
      if (cmpid(via, myid))
      {
        /* Zum naechsten Rufzeichen. */
        if (via[L2IDLEN + 1] != FALSE)
          /* Das ist unser Rufzeichen. */
          cpyid(rxfhdr, via + L2IDLEN);
        /* Keine Digipeaterkette. */
        else
          /* 2. Rufzeichen im rxfhdr. */
          cpyid(rxfhdr + L2IDLEN, rxfhdr);
      }
      /* Nicht unser Knotencall. */
      else
        {
          /* TX-Frame. */
          if (tx)
            /* Das ist unser Rufzeichen. */
            cpyid(rxfhdr, via);
          /* RX-Frame */
          else
              /* Keine Digipeaterkette. */
              if (via[L2IDLEN + 1] == FALSE)
                /* Das ist unser Rufzeichen. */
                cpyid(rxfhdr + L2IDLEN, via);
              /* Digipeaterkette. */
              else
                /* 2. Rufzeichen im rxfhdr. */
                cpyid(rxfhdr + L2IDLEN, via);
        }
    }
#endif

  if (updmheard((int)rxfprt))
  {
    if (!tx)   /* RX-Frame */
    {
#ifdef MH_LISTE
#ifdef __WIN32__
      if ((mhp = mh_lookup_port(&l2heard, rxfhdr + L2IDLEN, (unsigned short)rxfprt,tx)) == NULL)
#else
      if ((mhp = mh_lookup_port(&l2heard, rxfhdr + L2IDLEN, rxfprt,tx)) == NULL)
#endif /* WIN32 */
#else /* MH_LISTE */
#ifdef __WIN32__
      if ((mhp = mh_lookup_port(&l2heard, rxfhdr + L2IDLEN, (unsigned short)rxfprt)) == NULL)
#else
      if ((mhp = mh_lookup_port(&l2heard, rxfhdr + L2IDLEN, rxfprt)) == NULL)
#endif /* WIN32 */
#endif /* MH_LISTE */
        mhp = mh_add(&l2heard);
      if (mhp)
      {
#ifdef __WIN32__
        mh_update(&l2heard, mhp, rxfhdr + L2IDLEN, (unsigned short)rxfprt);
#else
        mh_update(&l2heard, mhp, rxfhdr + L2IDLEN, rxfprt);
#endif /* WIN32 */
#ifdef EAX25
        switch (rxfctl)
        {
          case L2CSABM : mhp->eax_link = FALSE; break;
          case L2CSABME: mhp->eax_link = TRUE;  break;
#ifdef MHEAX_LINKFIX
          case L2CUA   : mhp->eax_link = FALSE; break;
#endif /* MHEAX_LINKFIX */

          default: break;
        }
#endif
        mhp->rx_bytes += count;
        if ((rxfctl & 0xf) == L2CREJ)
          mhp->rx_rej++;
      }
    }
    else
    {
#ifdef MH_LISTE
#ifdef __WIN32__
      if ((mhp = mh_lookup_port(&l2heard, rxfhdr, (unsigned short)rxfprt,tx)) != NULL)
#else
      if ((mhp = mh_lookup_port(&l2heard, rxfhdr, rxfprt,tx)) != NULL)
#endif /* WIN32 */
#else /* MH_LISTE */
#ifdef __WIN32__
      if ((mhp = mh_lookup_port(&l2heard, rxfhdr, (unsigned short)rxfprt)) != NULL)
#else
      if ((mhp = mh_lookup_port(&l2heard, rxfhdr, rxfprt)) != NULL)
#endif /* WIN32 */
#endif /* MH_LISTE */
      {
        mhp->tx_bytes += count;
        if ((rxfctl & 0xf) == L2CREJ)
          mhp->tx_rej++;
      }
    }
  }

#ifdef MH_LISTE
  strncpy(rxfhdr, SaveHeader, 2 * L2IDLEN + L2VLEN);
#endif /* MHLISTDIRECTLY */


  /*------------------------------------------ LinkStatistik updaten -----*/

  for (statp = mh, i = 0; i < MAXSTAT; statp++, i++)
  {
    if (!(*statp->call))
      continue;

/************************************************************************/
/* Die rxfhdr Liste ist in Ziel/Quell/via Rufzeichen aufgeteilt. Bei    */
/* einem TX-Frame (tx == 1) ist somit das Zielid (rxfhdr) interessant   */
/* und bei einem RX-Frame (tx == 0) ist das Quellid (rxfhdr+L2IDLEN)    */
/* von Interesse.                                                       */
/************************************************************************/

    if (!tx)
      callp = rxfhdr + L2IDLEN;               /* Quellcall bei Rx-Frame */
    else
      callp = rxfhdr;                         /* Zielcall bei Tx-Frame  */

/************************************************************************/
/* Bei einem Linkstateintrag ohne viacall-Angabe darf trotzdem das      */
/* eigene Call im Viafeld stehen.                                       */
/************************************************************************/

    if (statp->viacall[0] == NUL)               /* ohne via Angabe      */
    {
      if (   cmpid(callp, statp->call)          /* Stimmt Rufzeichen    */
          && (   *(rxfhdr + L2ILEN) == NUL      /* und kein Viafeld     */
              || owndigipt(rxfhdr + L2ILEN)))   /* oder eigenes Call    */
        break;
    }
    else                                        /* mit via Angabe       */
    {
      if (   cmpid(statp->call, callp)          /* Stimmt Rufzeichen    */
          || cmpid(statp->call, anycall))
      {
        if (!tx)
        {

/************************************************************************/
/* Viacall-Liste nach entsprechenden via-Rufzeichen durchsuchen.        */
/* Korrekter waere es, das letzte "digipeated" Rufzeichen zu suchen und */
/* zu vergleichen!                                                      */
/************************************************************************/

          if (*(viap = rxfhdr + L2ILEN) != NUL) /* Zeiger auf Via-Liste */
            if (invial(viap, statp->viacall))   /* Wenn Call in Liste   */
              if (*(viap + L2IDLEN - 1) & L2CH) /* und ge-digipeatet    */
                break;
        }
        else            /* "first not digipeated" Rufzeichen ermitteln  */
        {
          if (*(p = ndigipt(rxfhdr + L2ILEN)) != NUL)
            if (cmpid(statp->viacall, p))
              break;
        }
      }
    }
  } /* for */

/************************************************************************/
/* ACHTUNG !!! Bei einer doppelten Eintragung des Via-Rufzeichens und   */
/* der Verwendung des "*" Symbols ist darauf zu achten, dass das        */
/* normale Rufzeichen vor der Eintragung mit dem "*" Symbol steht,      */
/* ansonsten wird fuer die normale Eintragung keine Zaehlung            */
/* vorgenommen.                                                         */
/************************************************************************/

  if (i < MAXSTAT)
  {
    if (statp->hfirst == 0L)            /* zum ersten Mal gehoert       */
      statp->hfirst = sys_time;

    if (!tx)
      statp->hlast = sys_time;                  /* last heard updaten   */

    statp->Bytetotal[trx] += (ULONG) count;
    statp->Byteheader[trx] += (ULONG) over_count;

    if (!(rxfctl & L2CNOIM))
    {
      statp->Ino[trx]++;
/* wiederholt gesendetes I-Frame ?                                      */
      if (tx && repeated && count > over_count)
        statp->txByterepeated += (ULONG)(count - over_count);
    }
    else
      if (!(rxfctl & L2CNOSM))
      {
        switch ((rxfctl >> 2) & 0x3)
        {
          case 0  :   statp->RRno[trx]++;
                      break;
          case 1  :   statp->RNRno[trx]++;
                      break;
          case 2  :   statp->REJno[trx]++;
                      break;
        }
      }
      else
      {
        switch ((rxfctl >> 2) & 0x3b)
        {
          case 0  :   statp->UIno[trx]++;
                      break;
          case 3  :   statp->DMno[trx]++;
                      break;
          case 11 :   statp->SABMno[trx]++;
                      break;
          case 16 :   statp->DISCno[trx]++;
                      break;
          case 24 :   statp->UAno[trx]++;
                      break;
          case 33 :   statp->FRMRno[trx]++;
                      break;
        } /* switch */
      } /* else */
  }
}

static void dump_nodes(MBHEAD *mbp, MBHEAD *fbp)
{
  char id[L2IDLEN];
  int  i;

  putstr("\rNODES-Broadcast:", mbp);
  while (fbp->mbpc - fbp->mbgc >= 21) {
    if (getfid(id,fbp) == TRUE) {
        putchr('\r', mbp);
        for (i = 0; i < L2CALEN; ++i)
          putchr(getchr(fbp), mbp);
        putchr(':', mbp);
        putid(id, mbp);
        if (getfid(id,fbp) == TRUE) {
          putstr(" v ", mbp);
          putid(id, mbp);
          putchr(' ', mbp);
          putnum(getchr(fbp), mbp);
          continue;
        }
    }
    return;
  }
}

static void
dump_inp_nodes(MBHEAD *mbp, MBHEAD *fbp)
{
  char  desnod[L2IDLEN];
  char  beaide[L2IDLEN], *bp;
  int   qual;
  int   ttl;
  int   tag;
  int   len;
  int   ch, i;
  UBYTE ipa[5];

  getchr(fbp);                                  /* Kennung uebergehen   */
  putstr("\rRouting Information:", mbp);
  while (fbp->mbpc - fbp->mbgc > 10)
  {
    if (!getfid(desnod, fbp))
      break;
    putchr('\r', mbp);
    putid(desnod, mbp);
    ttl = getchr(fbp);
    qual = get16(fbp);
    putprintf(mbp, " %u / %u", qual, ttl);
    while (fbp->mbgc < fbp->mbpc)       /* 1 RIP Eintrag lesen          */
    {
      if ((len = getchr(fbp)) > 0)      /* Eintragsende? (EOP Byte)     */
        len--;                          /* Laengenbyte selbst abziehen  */

      if (fbp->mbpc - fbp->mbgc < len)  /* noch genug vorhanden?        */
      {
#ifdef SPEECH
        putprintf(mbp, speech_message(268), len, (int) (fbp->mbpc - fbp->mbgc));
#else
        putprintf(mbp, "\rINP: frame error, len = %u, left = %u\r",
                       len, (int) (fbp->mbpc - fbp->mbgc));
#endif
        break;
      }
      if (len-- < 1)                    /* TAG abziehen                 */
        break;

      switch (tag = getchr(fbp))
      {
        case INP_ALIAS:
          if (len > L2CALEN)
          {
#ifdef SPEECH
            putstr(speech_message(271), mbp);
#else
            putstr(" (INVALID ALIAS!)\r", mbp);
#endif
            return;
          }
          for (i = 0, bp = beaide; i < len; i++)
          {
            ch = getchr(fbp);
            if ((ch < ' ') || (ch > 127))
              ch = '?';
            *bp++ = ch;
          }
          for (; i < L2CALEN; i++)
            *bp++ = ' ';
          putprintf(mbp, " (ALIAS=%6.6s)", beaide);
          break;
        case INP_IPA:
          if (len != 5)
          {
#ifdef SPEECH
            putstr(speech_message(272), mbp);
#else
            putstr(" (INVALID IP ADDRESS!)\r", mbp);
#endif
            return;
          }
          for (i = 0; i < 5; i++)               /* IP-Nr + Subnet lesen */
            ipa[i] = (UBYTE)getchr(fbp);
          putprintf(mbp, " (IP = %u.%u.%u.%u/%u)",
                                 ipa[0], ipa[1], ipa[2], ipa[3], ipa[4]);
          break;
        default :
          putprintf(mbp, " (LEN=%u TAG=%u", len, tag);
          while (len-- > 0)
            putprintf(mbp, " %02X", getchr(fbp));
          putchr(')', mbp);
          break;
      }
    }
  }
  putchr('\r', mbp);
}

const char *
l4opctab = "ESCAPE"
           "CONREQ"
           "CONACK"
           "DISREQ"
           "DISACK"
           "INFTRA"
           "INFACK"
#ifdef NEW_L4
           "PIDCHG";
#else
           "STATRA";
#endif

static void dump_netrom(MBHEAD *mbp, MBHEAD *fbp)
{
  char id[L2IDLEN];
  UBYTE l4opco = 0;
  UBYTE l4idx[4];
  WORD i;

  if (rxfctl != L2CUI) {          /* I-Frames                       */
    if (*fbp->mbbp == 0xFF)
      dump_inp_nodes(mbp, fbp);
    else
    if (getfid(id,fbp) == TRUE) { /* L3-Teil beginnt mit Call       */
      putstr("\r(L3 ", mbp);      /* L3-Teil in neue Zeile          */
      putid(id, mbp);             /* L3-Absendercall                */
      if (getfid(id,fbp) == TRUE) { /* L3-Empfaenger vorhanden?     */
        putchr('>', mbp);
        putid(id, mbp);              /* L3-Empfaenger anzeigen      */
        if (fbp->mbgc < fbp->mbpc) { /* L3-Lifetime vorhanden?      */
          putstr(" TTL=", mbp);
          putnum(getchr(fbp), mbp);  /* dann anzeigen               */
        }
        if (fbp->mbpc - fbp->mbgc >= 5) { /* gueltiger L4-Teil?     */
          putstr(")\r(L4 ", mbp);         /* L4-Teil in neue Zeile  */
          for (i = 0; i < 4; ++i)         /* 4 L4-Header Bytes      */
            l4idx[i] = getchr(fbp);
          l4opco = getchr(fbp);
          if (l4opco & 0x18)
            putnum(l4opco, mbp);
          else
            putprintf(mbp, "%6.6s", l4opctab + (l4opco & L4OPMASK) * 6);
          if (l4opco & L4CMORE)
            putstr("+MORE", mbp);
          if (l4opco & L4CNAK)
            putstr("+NAK", mbp);
          if (l4opco & L4CCHOKE)
            putstr("+CHOKE", mbp);
          for (i = 0; i < 4; ++i) {       /* Opcodes anzeigen       */
            putchr(' ', mbp);
            putnum(l4idx[i], mbp);
          }
          if (l4opco == 5 &&
              cmpid(id, "L3RTT \140") &&
              match(fbp, "BROAD")         /* BROAD Kennung ?        */
              ) {
            putchr(')', mbp);
            dump_nodes(mbp, fbp);         /* Nodes zeigen           */
            return;
          }
          if (l4opco == 1 || l4opco == 2) /* ConnReq o. ConnAck     */
            if (fbp->mbgc < fbp->mbpc) {  /* es sollte noch mehr    */
              putchr(' ', mbp);           /* kommen                 */
              putnum(getchr(fbp), mbp);   /* L4 Window Size         */
              if (l4opco == 1)            /* Connect Request        */
                if (getfid(id,fbp) == TRUE) {
                  putchr(' ', mbp);
                  putid(id, mbp);         /* User Call              */
                  if (getfid(id,fbp) == TRUE)
                  {
                    putchr(' ', mbp);
                    putid(id, mbp);               /* Absenderknoten */
                    if (fbp->mbpc - fbp->mbgc >= 7) /* neue Soft-   */
                    {                               /* ware?        */
                      if (getfid(id,fbp) == TRUE)   /* Uplinkknoten */
                      {
                        putstr(" v ",mbp);   /*wie normale Digiliste*/
                        putid(id, mbp);       /* aber max. 9 Calls  */
                        while (*fbp->mbbp != NUL)  /* Ende erreicht?*/
                        {
                          if (fbp->mbpc - fbp->mbgc < 7) break;
                          if (!getfid(id,fbp)) break;
                          putchr(' ', mbp);
                          putid(id, mbp);
                        }
                        if (fbp->mbgc < fbp->mbpc) getchr(fbp);
                        /* Nullterminierung Digiliste abholen   */
                      }
                    }
                  }
                }
            }
        }
      }
      putchr(')', mbp);          /* Ende L3/4-Teil                */
      if (l4opco == L3TCPUDP) {
        if (l4idx[0] == 0 && l4idx[1] == 1) {
          dump_nrr(mbp, fbp);
        }
#ifdef IPROUTE
        else
          dump_ip(mbp, fbp);
#endif
      }
    }
  }
  else                                    /* L3-UI Frame          */
  if (fbp->mbpc - fbp->mbgc >= 7)
  {
    putprintf(mbp, "\r%02X ", getchr(fbp));
    for (i = 0; i < L2CALEN; ++i)
      putchr(getchr(fbp), mbp);
    dump_nodes(mbp, fbp);           /* Node-Informationen zeigen  */
  }
}

/* ---------------------------------------- */
/* ---- Netrom-Record-Route ausgeben ------ */
/* ---------------------------------------- */
static void dump_nrr(MBHEAD *mbp, MBHEAD *fbp)
{
  char id[L2IDLEN];
  int  lt;

  putstr(" NET/ROM Route Record\r", mbp);
  while (fbp->mbpc - fbp->mbgc >= L2IDLEN+1) { /* Eintrag da?       */
    if (getfid(id, fbp) == TRUE) {
      lt = getchr(fbp);
      if (lt & ECHO_FLAG) putchr('*', mbp);
      putid(id, mbp);
      putprintf(mbp, "(%u) ", lt & LT_MASK);
    }
  }

  while (fbp->mbgc < fbp->mbpc) getchr(fbp); /* kein Schrott auf den Schirm */
}

#ifdef IPROUTE
/* ------------------------------------ */
/* Adress-Resolution-Protokoll ausgeben */
/* ------------------------------------ */
static void dump_arp(MBHEAD *mbp, MBHEAD *fbp)
{
  ARP arp;

  arp.hardware = get16(fbp);
  arp.protocol = get16(fbp);
  arp.hwalen = getchr(fbp);
  arp.pralen = getchr(fbp);
  arp.opcode = get16(fbp);
  getfid((char *)arp.shwaddr, fbp);
  arp.sprotaddr = get32(fbp);
  getfid((char *)arp.thwaddr, fbp);
  arp.tprotaddr = get32(fbp);

  putstr("\r(", mbp);
  switch (arp.opcode) {
    case ARP_REQUEST    : putstr("ARP-REQ ", mbp); break;
    case ARP_REPLY      : putstr("ARP-REPLY ", mbp); break;
    case REVARP_REQUEST : putstr("REVARP-REQ ", mbp); break;
    case REVARP_REPLY   : putstr("REVRARP-REPLY ", mbp); break;
    default             : putprintf(mbp, "ARP%04X ", arp.opcode); break;
  }
  switch (arp.hardware) {
    case ARP_NETROM : putstr("NET/ROM ", mbp); break;
    case ARP_AX25   : putstr("AX25 ", mbp); break;
    default         : putprintf(mbp, "hardware=%04X ", arp.hardware);
  }
  switch (arp.protocol) {
    case L2CIP      : putstr("IP-Proto", mbp); break;
    default         : putprintf(mbp, "Proto=%04X", arp.protocol); break;
  }
  if (arp.hwalen != 7 || arp.pralen != 4)
    putprintf(mbp, " hwalen=%u pralen=%u", arp.hwalen, arp.pralen);
  putstr(")\rSource=", mbp);
  putid((char *)arp.shwaddr, mbp);
  putchr(':', mbp);
  show_ip_addr(arp.sprotaddr, mbp);
  putstr(" Dest=", mbp);
  if (arp.opcode == ARP_REQUEST)
    putstr("unknown", mbp);
  else
    putid((char *)arp.thwaddr, mbp);
  putchr(':', mbp);
  if (arp.opcode == REVARP_REQUEST)
    putstr("unknown", mbp);
  else
    show_ip_addr(arp.tprotaddr, mbp);
}

/* -------------------------------------------------------- */
/* ------------ IP-Header ausgeben ------------------------ */
/* -------------------------------------------------------- */
static void dump_ip(MBHEAD *mbp, MBHEAD *fbp)
{
  IP ip;
  int ip_len;

  ip_len = getchr(fbp);         /* Laenge und Version lesen          */
  ip.ihl = ip_len & 0x0f;
  ip.version = (ip_len >> 4) & 0x0f;    /* Versionsnummer            */
  ip.tos      = getchr(fbp);            /* Type off Service          */
  ip.length   = get16(fbp);             /* Laenge Header und Data    */
  ip.id       = get16(fbp);             /* Fragment ID               */
  ip.offset   = get16(fbp);             /* Fragnent Offset           */
  ip.flags.mf = (ip.offset & 0x2000) ? 1 : 0; /* more follow         */
  ip.flags.df = (ip.offset & 0x4000) ? 1 : 0; /* dont fragment       */
  ip.offset   = (ip.offset & 0x1fff);         /* << 3 Fragment Offset*/
  ip.ttl      = getchr(fbp);                  /* Time-to-Live        */
  ip.protocol = getchr(fbp); /* Kennzahl des aufsetzenden Protokolls */
  ip.checksum = get16(fbp);
  ip.source = get32(fbp);
  ip.dest = get32(fbp);

  putprintf(mbp, "\r(IPV%u ", ip.version);
  if (ip.version == 4) {
    putstr("fm ", mbp);
    show_ip_addr(ip.source, mbp);
    putstr(" to ", mbp);
    show_ip_addr(ip.dest, mbp);
    putprintf(mbp, " IHL=%d TOS:%x Length=%d ID=%d \rFlags:MF=%x DF=%x FraOff=%u TTL=%u ",
              ip.ihl, ip.tos, ip.length, ip.id ,ip.flags.mf , ip.flags.df ,ip.offset ,ip.ttl);
    switch (ip.protocol) {
      case ICMP_PTCL : putstr("ICMP)\r", mbp);
                       if (ip.offset == 0)
                         dump_icmp(mbp, fbp, &ip);
                       break;
      case TCP_PTCL  : putstr("TCP)\r",  mbp);
                       if (ip.offset == 0) /* das Folgefragment hat keinen TCP-Header */
                         tcp_DumpHeader(mbp, fbp, &ip);
                       break;
      case UDP_PTCL  : putstr("UDP)\r",  mbp);
                       if (ip.offset == 0)
                         dump_udp(mbp, fbp, &ip);
                       break;
      default        : putprintf(mbp, "Proto=%02X)", ip.protocol);
    }
  } else
    putstr("unknown)", mbp);
}

/* ------------------------------------------------------------------ */
/* ----- Dump tcp protocol header of a packet ----------------------- */
/* ------------------------------------------------------------------ */
static void tcp_DumpHeader(MBHEAD *mbp, MBHEAD *fbp, IP *in_header)
{
   static const char *flags[] = { "FIN", "SYN", "RST", "PUSH", "ACK", "URG" };
   int    len,f,temp_flag;
   TCP    tcp;

    tcp.srcPort        = get16(fbp); /* Port-Nummer der Quelle */
    tcp.dstPort        = get16(fbp); /* Port-Nummer des Ziels  */
    tcp.seqnum         = get32(fbp); /* Sequenznummer des 1. Datenbytes in diesem Segment */
    tcp.acknum         = get32(fbp); /* Bestaetigungsnummer    */
    temp_flag          = get16(fbp);
    tcp.data_offset    = (temp_flag >> 12) & 0x0f; /* Datenworte im header      */
    tcp.res            = (temp_flag >>  6) & 0x3f; /* nicht benutzt -> 0        */
    tcp.flags          = temp_flag & 0x3f; /* URG,ACK,PSH,RST,SYN & FIN         */
    tcp.window         = get16(fbp);    /* Groesse des Puffers fuer diese Verb. */
    tcp.checksum       = get16(fbp);    /* CRC                                  */
    tcp.urgentPointer  = get16(fbp);

    len = (in_header->length) - ((tcp.data_offset * 4) + (in_header->ihl * 4));

    putprintf(mbp, "TCP Packet: SrcP:%d DstP:%d SeqN=%lx AckN=%lx Wind=%d\r",
              tcp.srcPort,
              tcp.dstPort,
              tcp.seqnum,
              tcp.acknum,
              tcp.window);

    putprintf(mbp, "DataLen=%d DataOffset=%d, CRC=%x Urgent=%d\r",
             len,
             tcp.data_offset,
             tcp.checksum,
             tcp.urgentPointer);

    /* output flags */
    f = (tcp.flags);
    for (len = 0; len < 6; len++)
        if (f & (1 << len))
           putprintf(mbp,"Flag: %s\r", flags[len]);
}

/* DUMP UDP */
static void dump_udp(MBHEAD *mbp, MBHEAD *fbp, IP *in_header)
{

  UDP udp;

   udp.srcPort  = get16(fbp);
   udp.dstPort  = get16(fbp);
   udp.length   = get16(fbp);
   udp.checksum = get16(fbp);

   putprintf(mbp, "UDP Packet: SrcP:%d DstP:%d Len=%d CRC=%x\r",
              udp.srcPort,
              udp.dstPort,
              udp.length,
              udp.checksum);

}

/* DUMP ICMP */
static void dump_icmp(MBHEAD *mbp, MBHEAD *fbp, IP *in_header)
{

  ICMP icmp;

   icmp.type     = getchr(fbp);
   icmp.code     = getchr(fbp);
   icmp.checksum = get16(fbp);

   putprintf(mbp, "ICMP Packet: Type:%d Code:%d CRC=%x\r",
              icmp.type,
              icmp.code,
              icmp.checksum);
}
#endif

/*----------------------------------------------------------------------*/
/* Fragmentierte AX25 Frames bearbeiten                                 */
/*----------------------------------------------------------------------*/
static void
dump_frag(MBHEAD *mbp, MBHEAD *fbp)
{
  BYTE anz_frames;
  BYTE pid;

  anz_frames = getchr(fbp); /* erstes Byte lesen */

  if (anz_frames & 0x80) /* erstes Frame ? */
  {
    pid = getchr(fbp);
#ifdef SPEECH
    putprintf(mbp,speech_message(269), (anz_frames & 0x7F), (pid & 0xFF));
#else
    putprintf(mbp,"\r[AX25 Fragment; %u Frame(s) to follow - "
                  "original PID %X]\r", (anz_frames & 0x7F), (pid & 0xFF));
#endif
#ifdef IPROUTE
    /* Je nach PID noch eine extra Ausgabe */
    switch (pid & 0xFF)
    {
      case L2CIP:
        dump_ip(mbp, fbp);
        break;

      case L2CNETROM:
        dump_netrom (mbp, fbp);
        break;
    }
#endif
  }
  else /* Folgeframe */
#ifdef SPEECH
    putprintf(mbp,speech_message(270),(anz_frames & 0x7F));
#else
    putprintf(mbp,"\r[AX25 Fragment; %u Frame(s) to follow]\r",
              (anz_frames & 0x7F));
#endif
}

/*----------------------------------------------------------------------*/
/* Frame Header im Monitor zeigen                                       */
/*----------------------------------------------------------------------*/
static void
frhmon(MBHEAD *mbp, MBHEAD *fbp)
{
#if MAX_TRACE_LEVEL > 0
  UBYTE uFRMR_Bytes[4];
#endif

#ifdef NOPORTINMON
  putstr("fm ", mbp);
#else
  putprintf(mbp,"%c%d: fm ", fbp->tx ? 'T' : 'R', fbp->l2port);
#endif

  putid(rxfhdr + L2IDLEN, mbp);     /* Absender-Rufzeichen              */
  putstr(" to ", mbp);
  putid(rxfhdr, mbp);               /* Zielrufzeichen                   */
  putdil(rxfhdr + L2ILEN, mbp);
  putchr(' ', mbp);
  if (!(rxfctl & L2CNOIM))
    putchr('I', mbp);
  else
    if (!(rxfctl & L2CNOSM))
      switch (rxfctl & 0x0c)
      {
        case L2CRR & 0x0c:
          putstr("RR", mbp);
          break;
        case L2CRNR & 0x0c:
          putstr("RNR", mbp);
          break;
        case L2CREJ & 0x0c:
          putstr("REJ", mbp);
          break;
        default:
          putprintf(mbp, "?%02XH", (char)(rxfctl | rxfPF));
          break;
      }
    else
      /* U-Frame */
      switch (rxfctl & 0xFF)
      {
        case L2CUI:
          putstr("UI", mbp);
          break;
        case L2CDM:
          putstr("DM", mbp);
          break;
        case L2CSABM:
          putstr("SABM", mbp);
          break;
#ifdef EAX25
        case L2CSABME:
          putstr("SABME", mbp);
          break;
#endif
        case L2CDISC:
          putstr("DISC", mbp);
          break;
        case L2CUA:
          putstr("UA", mbp);
          break;
        case L2CFRMR:
          putstr("FRMR", mbp);

#if MAX_TRACE_LEVEL > 0
          switch (fbp->mbpc - fbp->mbgc)
          {
            /* AX.25 hat drei FRMR-Bytes */
            case 3:
            /* EAX.25 hat fuenf FRMR-Bytes */
            case 5:
              if ((fbp->mbpc - fbp->mbgc) == 3)
              { /* AX.25 */
                uFRMR_Bytes[0] = getchr(fbp);
                uFRMR_Bytes[1] = getchr(fbp);

                notify(1, "FRMR: Ctl: 0x%0.2X, N(r): %u, N(s): %u, C/R: %u",
                       uFRMR_Bytes[0],
                       (uFRMR_Bytes[1] >> 5 & 0x07),
                       (uFRMR_Bytes[1] >> 4 & 0x01),
                       (uFRMR_Bytes[1] & 0x01));
              }
              else
              { /* EAX.25 */
                uFRMR_Bytes[0] = getchr(fbp);
                uFRMR_Bytes[1] = getchr(fbp);
                uFRMR_Bytes[2] = getchr(fbp);
                uFRMR_Bytes[3] = getchr(fbp);

                notify(1, "FRMR: Ctl1: 0x%0.2X, Ctl2: 0x%0.2X, N(s): %u, N(r): %u, C/R: %u",
                       uFRMR_Bytes[0],
                       uFRMR_Bytes[1],
                       (uFRMR_Bytes[2] >> 1 & 0x7F),
                       (uFRMR_Bytes[3] >> 1 & 0x7F),
                       (uFRMR_Bytes[3] & 0x01));
              }

              /* AX.25 und EAX.25 */
              /* ZYXW-Byte */
              uFRMR_Bytes[0] = getchr(fbp);
              /* W */
              if (uFRMR_Bytes[0] & 0x01)
                notify(1, "Invalid control field");
              /* X */
              if (uFRMR_Bytes[0] & 0x02)
                notify(1, "Illegal I-field");
              /* Y */
              if (uFRMR_Bytes[0] & 0x04)
                notify(1, "I-field too long");
              /* Z */
              if (uFRMR_Bytes[0] & 0x08)
                notify(1, "Invalid sequence number");

              break;

            /* keine spezielle Auswertung / unbekannte Anzahl FRMR-Bytes */
            default:
              while (fbp->mbgc < fbp->mbpc)
                putprintf(mbp, "%02X", getchr(fbp));
          }
#else
            while (fbp->mbgc < fbp->mbpc)
              putprintf(mbp, "%02X", getchr(fbp));
#endif
          break;

        default:
          putprintf(mbp, "?%02XH", (char)(rxfctl | rxfPF));
          break;
      }

  if ((rxfctl & 0x3) != 3) /* keine U-Frames, nur I- und S-Frames */
  {
#ifdef EAX25
    if (rxfEAX)  /* EAX.25-Frames */
    {
      /* S-Frames */
      putprintf(mbp, "%02X", (rxfctlE >> 1) & 0x7F);   /* N(r) */

      if ((rxfctl & 0x01) == 0) /* zusaetzliches fuer I-Frames */
        putprintf(mbp, "%02X", (rxfctl >> 1) & 0x7F);  /* N(s) */
    }
    else
    {
#endif
      /* normale AX.25-Frames */
      putnum((rxfctl >> 5) & 0x7, mbp);

      if (!(rxfctl & L2CNOIM))
        putnum((rxfctl >> 1) & 0x7, mbp);
#ifdef EAX25
    }
#endif
  }

  /* Pollflag und Command/Response */
  if (rxfPF != 0)
#ifdef __WIN32__
    putchr((char)(rxfCR != 0 ? '+' : '-'), mbp);
#else
    putchr(rxfCR != 0 ? '+' : '-', mbp);
#endif /* WIN32 */
  else
#ifdef __WIN32__
    putchr((char)(rxfCR != 0 ? '^' : 'v'), mbp);
#else
    putchr(rxfCR != 0 ? '^' : 'v', mbp);
#endif /* WIN32 */
  if (!(rxfctl & L2CNOIM) || rxfctl == L2CUI)
    putprintf(mbp, " pid %02X",
                     mbp->type = (fbp->mbgc < fbp->mbpc) ? getchr(fbp) : 0);
  /* DAMA-Frame */
  if (rxfDA)
    putstr(" [DAMA]", mbp);

#ifdef EAX25
  /* EAX.25-Frame */
  if (rxfEAX)
    putstr(" [EAX]", mbp);
#endif

#ifdef MAXFRAMEDEBUG
  if (fbp->tx == 1 && !(rxfctl & L2CNOIM))
    putprintf(mbp, " f=%04x, lmf=%u, pmf=%u, t=%u",
              fbp->lnkflag, fbp->lmf, fbp->pmf, fbp->tosend);
#endif
}

/*----------------------------------------------------------------------*/
/* Network Header im Monitor zeigen                                     */
/*----------------------------------------------------------------------*/
static void
nethmon(MBHEAD *netmbp, MBHEAD *fbp)
{
  if (!(rxfctl & L2CNOIM) || rxfctl == L2CUI)
   {
    switch (netmbp->type)                       /* PID                  */
     {
      case L2CTEXNET:
      case L2CNETROM: dump_netrom(netmbp, fbp);
                      break;
      case L2CFRAG  : dump_frag(netmbp, fbp);
                      break;
#ifdef IPROUTE
      case L2CARP   : dump_arp(netmbp, fbp);
                      break;
      case L2CIP    : putstr(" TCP/IP", netmbp);
                      dump_ip(netmbp, fbp);
                      break;
#endif
     }
   }
    putchr('\r', netmbp);
}

/*----------------------------------------------------------------------*/
/* is monitor frame?                                                    */
/*----------------------------------------------------------------------*/
static BOOLEAN ismonf(MBHEAD *fbp, MONBUF *m)
{
  if ((m->Mpar & MONC) != FALSE) {
    if ((fbp->l2port == m->Mport) || (m->Mport > L2PNUM)) {
      if (   ((!(rxfctl & L2CNOIM)) && (((m->Mpar & MONI) != 0)))
          || (((rxfctl & 3) == 1) && ((m->Mpar & MONS) != 0))
          || (   ((rxfctl & 3) == 3)
              && (rxfctl != L2CUI)
              && ((m->Mpar & MONS) != 0))
          || ((rxfctl == L2CUI) && ((m->Mpar & MONU) != 0)))
      {
        if (m->mftsel != 0) {
          if (   invial(m->mftidl,rxfhdr + L2IDLEN) == TRUE
              || invial(m->mftidl,rxfhdr) == TRUE)
          {
            if (m->mftsel == 2)
              return(FALSE);
          }
          else
            if (m->mftsel == 1)
              return(FALSE);
        }
        return(TRUE);
      }
    }
  }
  return(FALSE);
}

/*----------------------------------------------------------------------*/
/* in via list?                                                         */
/*----------------------------------------------------------------------*/
static BOOLEAN invial(char *vial, char *id)
{
  while (*vial != NUL)
    if (cmpid(vial,id) == TRUE)
      return(TRUE);
    else
      vial += L2IDLEN;
  return(FALSE);
}

void moncmd(MBHEAD  *mbp, MONBUF *m, char *blipoi, WORD blicnt)
{
  WORD     arg;
  BOOLEAN  host = (m == &consmon);
  char    *c;
  char     str[128];
  char     call[15];
  char    *s;

  if (blicnt == 0)
  {
    if (host)
      rspini(HMRSMSG);
    else
      putstr(", Monitor=", mbp);

    if (m->Mpar == 0)
      strcpy(str, "N");
    else {
      s = str;
      if ((m->Mpar & MONI) || (m->Mpar & MONU))
       {
        if (m->Mpar & MONF)
          *s++ = 'F';
        if (m->Mpar & MONL)
          *s++ = 'L';
       }
      if (m->Mpar & MONU)
        *s++ = 'U';
      if (m->Mpar & MONS)
        *s++ = 'S';
      if (m->Mpar & MONT)
        *s++ = 'T';
      if (m->Mpar & MONI)
        *s++ = 'I';
      if (m->Mpar & MONC)
        *s++ = 'C';
      if ((m->Mpar & MONI) || (m->Mpar & MONU))
        if (!(m->Mpar & MONF))
          *s++ = 'H';

      if (m->Mport < L2PNUM)               /* Welcher Port selektiert? */
        s += sprintf(s, " Port%d", m->Mport);

      *s = NUL;

      if (m->mftsel) {
        if (m->mftsel == 1)
          strcat(s, " +");
        if (m->mftsel == 2)
          strcat(s, " -");
        if (*(c = m->mftidl )) {
          while (*c) {
           strcat(s, " ");
           call2str(call, c);
           strcat(s, call);
           c += L2IDLEN;
          }
        }
      }
    }

    if (host) {
      putstr(str, hstmbp);
    } else {
      putstr(str, mbp);
      putstr("\r", mbp);
    }
  } else {
    m->Mport = 255;
    m->Mpar = 0;
    m->mftsel = 0;
    arg = 0;
    while (blicnt)
    {
      skipsp(&blicnt, &blipoi);
      if (toupper(*blipoi) == 'N')
      {
        blicnt--;
        blipoi++;
        m->Mport = 255;                        /* Alle Ports freigeben */
        if (skipsp(&blicnt,&blipoi) == FALSE)
          arg = 0;
        else
        {
          if (*blipoi == '+' || *blipoi == '-')
            arg = 0;
          else
          {
            if (host) {
              rsperr(HMEIPA);
              return;
            }
            blicnt = 1;
            arg = m->Mpar;
          }
        }
      }
      else
      {
        switch (toupper(*blipoi))
        {
           case 'M': if (host)          /* An der Konsole ist M falsch  */
                      {
                       rsperr(HMEIPA);
                       return;
                      }
                     break;             /* Bei TRACE ist M erlaubt      */
           case 'T': arg |= MONT;
                     break;
           case 'L': arg |= MONL;
                     break;
           case 'A': arg |= (MONI | MONU | MONS | MONC | MONF | MONT | MONL);
                     break;
           case 'U': arg |= MONU;
                     break;
           case 'S': arg |= MONS;
                     break;
           case 'I': arg |= MONI;
                     break;
           case 'C': arg |= MONC;
                     break;
           case 'F': arg |= MONF;    /* Full-Monitor: Infos anzeigen */
                     break;
           case 'H': arg &= ~MONF;   /* Header-Monitor: nur Header   */
                     break;
           case '+': --blicnt;
                     ++blipoi;
                     if (getdig(&blicnt, &blipoi, FALSE, m->mftidl) != YES) {
                       if (host) {
                         rsperr(HMEIPA);
                         return;
                       }
                     } else {
                       if (m->mftidl[0] == NUL)
                         m->mftsel = 0;
                       else
                         m->mftsel = 1;
                     }
                     blicnt = 1;
                     break;
           case '-': --blicnt;
                     ++blipoi;
                     if (getdig(&blicnt, &blipoi, FALSE, m->mftidl) != YES) {
                       if (host) {
                         rsperr(HMEIPA);
                         return;
                       }
                     } else {
                       if (m->mftidl[0] == NUL)
                         m->mftsel = 0;
                       else
                         m->mftsel = 2;
                     }
                     blicnt = 1;
                     break;
           default:  if (isdigit(*blipoi))
                     {
                       m->Mport = nxtnum(&blicnt, &blipoi);
                       while (isdigit(*blipoi)) {
                         blicnt--;
                         blipoi++;
                       }
                       continue;
                     }
                     else
                     {
                       if (host) {
                         rsperr(HMEIPA);
                         return;
                       }
                       blicnt = 1;
                       arg = 0;
                     }
        }
        blicnt--;
        blipoi++;
      }
    }
    if (   (arg & MONL)               /* Laenge nur sinnvoll wenn I-    */
        && !(arg & (MONI | MONU)))    /* oder UI-Frames ausgewaehlt     */
      arg &= ~MONL;
    if (!host) {                      /* nicht fuer die Console...      */
      if (m->Mpar && !arg) tracnt--;  /* der Monitor wurde abgeschaltet */
      if (!m->Mpar && arg) tracnt++;  /* ... oder angeschaltet          */
      if (arg)
        arg |= MONC;    /* Parameter C nur von Bedeutung an der Konsole */
    } else
      rspsuc();
#ifdef __WIN32__
    m->Mpar = (unsigned char)arg;
#else
    m->Mpar = arg;
#endif /* WIN32 */
  }
}

/* End of src/l7moni.c */
