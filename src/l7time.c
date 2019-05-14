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
/* File src/l7time.c (maintained by: ???)                               */
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

#ifdef BEACON_STATUS
static void noden(MBHEAD *mbp);
static void statusqual(MBHEAD *mbp, ULONG qual);
#endif

/************************************************************************/
/*----                                                              ----*/
/*----------------------------------------------------------------------*/
void timsrv(void)
{
  static LONG    Time10 = 0L;
#ifdef BEACON_STATUS
  static LONG    Time500 = 0L;
#endif
  static WORD    s_time = 0;
  static WORD    sec_cnt = 0;
  UWORD          zeit;
  PTCENT        *ptcp;
  PORTSTAT      *statp;
  int            i;
  CQBUF         *cqp;
  UID            uid, p_uid;
#ifdef CONNECTTIME
  PEER          *pp;
  int            max_peers = netp->max_peers;
#endif /* CONNECTTIME */

 /************************************************************************/
/*                                                                      */
/* Die Verbindungen bei Antwort auf CQ-Ruf herstellen.                  */
/* In der Liste cq_statl sind die herzustellenden Verbindungen festge-  */
/* legt. In jedem Buffer dieser Liste steht die UID des CQ-Rufers in    */
/* uid sowie die UID dessen, der antwortet in p_uid.                    */
/*                                                                      */
/************************************************************************/

  while ((LHEAD *)cq_statl.head != &cq_statl)
   {
    cqp = (CQBUF *)ulink((LEHEAD *)cq_statl.head);
    uid = cqp->uid;
    p_uid = cqp->p_uid;
    l2tol7(L7MCONNT, g_ulink(uid), g_utyp(uid));
    l2tol7(L7MCONNT, g_ulink(p_uid), g_utyp(p_uid));
    (ptctab + p_uid)->state = UPLINK;
    (ptctab + uid)->state = DOWNLINK;
    dealoc((MBHEAD *)cqp);
   }

   zeit = (UWORD) (tic10-lastic);

  if (zeit != 0)
  {
     lastic = tic10;
#ifndef MC68K
     l1timr(zeit);
#endif

     hostsv();

#ifdef __LINUX__
     if ((tic10 % 10) == 0)                        /* Alle 100ms arbeiten */
        shellsrv();
#endif

     if ((UWORD)(tic1s += zeit) >= 100)           /* Alle Sekunde arbeiten */
     {
        tic1s -= 100;
        time(&sys_time);
        sys_localtime = localtime(&sys_time);

        for (i = 0, ptcp = ptctab; i < NUMPAT; i++, ptcp++)
         if (ptcp->state)
             ptcp->contime++;

#ifdef CONNECTTIME
                                                 /* Alle Segmente durchgehen. */
        for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
        {
          if (!pp->used)                               /* Unbenutzer Eintrag, */
            continue;                               /* zum naechsten Segment. */

          if (pp->nbrl2l == NULL)                        /* Link ist deaktiv. */
          {
            pp->contime = 0;                    /* Connectzeit zuruecksetzen. */
            continue;
          }

          if (pp->nbrl2l->state < L2SIXFER)/* Link ist aktiv,aber im Auf/Abbau*/
          {
            pp->contime = 0;                    /* Connectzeit zuruecksetzen. */
            continue;
          }

          if (pp->nbrl2l->state >= L2SIXFER)               /* Link ist aktiv. */
          {
            if (pp->quality)                  /* Nur wenn gueltige Qualitaet, */
              pp->contime++;                 /* connectzeit um eins erhoehen. */
            else
              pp->contime = 0;                  /* Connectzeit zuruecksetzen. */
          }
        }
#endif /* CONNECTTIME */

         if (++sec_cnt % 10 == 0)          /* alle 10 Sekunden             */
         {
#ifdef __LINUX__
             calculate_load ();
#endif
             for (i = 0, statp = portstat; i < L2PNUM; i++, statp++) {

              if (statp->rx_baud)
                statp->rx_baud = ((statp->rx_baud+1)*7L-1+
                                  (statp->rx_bytes-statp->last_rx)/10L ) / 8L;
              else
                statp->rx_baud = (statp->rx_bytes-statp->last_rx)/10L / 4L;

              if (statp->tx_baud)
                statp->tx_baud = ((statp->tx_baud+1)*7L-1+
                                  (statp->tx_bytes-statp->last_tx)/10L ) / 8L;
              else
                statp->tx_baud = (statp->tx_bytes-statp->last_tx)/10L / 4L;

              statp->last_rx = statp->rx_bytes;
              statp->last_tx = statp->tx_bytes;
            }

            if (thbps)                      /* Durchsatz/Baudrate berechnen */
              thbps = ((thbps+1)*7L-1+throughput/10L)/8L;
            else
              thbps = throughput/10L / 4L;
            if (thbps > thbps_max)          /* Maximalwert merken           */
              thbps_max = thbps;
            throughput = 0L;                /* Durschsatzzaehler leeren     */

            for (i = 0, ptcp = ptctab; i < NUMPAT; i++, ptcp++)
              if (ptcp->state) {
                if (ptcp->rxbps)            /* Durchsatz/Baudrate berechnen */
                  ptcp->rxbps = ((ptcp->rxbps+1)*7L-1+
                          (ptcp->inforx-ptcp->lastrx)/10L ) / 8L;
                else
                  ptcp->rxbps = (ptcp->inforx-ptcp->lastrx)/10L / 4L;

                if (ptcp->txbps)
                  ptcp->txbps = ((ptcp->txbps+1)*7L-1+
                          (ptcp->infotx-ptcp->lasttx)/10L ) / 8L;
                else
                  ptcp->txbps = (ptcp->infotx-ptcp->lasttx)/10L / 4L;

                ptcp->lastrx = ptcp->inforx;
                ptcp->lasttx = ptcp->infotx;
              }
          }

          if (sec_cnt == 60)        /* Alle Minute                  */
          {
             sec_cnt = 0;

             rounds_pro_sec = (rounds_count / 60L);
             if (rounds_max_sec && rounds_min_sec) {
               rounds_max_sec = rounds_pro_sec > rounds_max_sec
                              ? rounds_pro_sec : rounds_max_sec;
               rounds_min_sec = rounds_pro_sec > rounds_min_sec
                              ? rounds_min_sec : rounds_pro_sec;
             } else
               rounds_min_sec = rounds_max_sec = rounds_pro_sec;
             rounds_count = 0;
#ifdef GRAPH
             /* Eine Minute warten nach Programmstart*/
            if (graph.enabled == FALSE)
              graph.enabled = TRUE;
#endif
#ifdef MC68K
             l1timr(zeit);
#endif
             beacsv();                           /* Baken bearbeiten      */
#ifdef  __GO32__
             toggle_lpt();
#endif
          }
#ifdef GRAPH
          graph_timer();
#endif
          if (Time10 <= 0L)                      /* 10 min abgelaufen     */
          {
            Time10 = 600L;                       /* neu setzen            */
                                                 /* 600 Sekunden = 10 min */
            if (save_timer != 0)
            {
              if (s_time <= 0)
              {
                save_stat();                        /* Einstellungen sichern*/
#ifdef GRAPH
                save_graph();
#endif
                if (!startup_running)
                  save_mh();
                personalmanager(SAVE, NULL, NULL);  /* Convers pers. Daten  */
                s_time = save_timer & 0x7FFF;
              }
              s_time--;
            }
#ifdef MC68302
            compact();                      /* RAM aufraeumen */
#endif
          }
          Time10--;                         /* 10 min Zaehler decrementieren*/
#ifdef BEACON_STATUS
          /* Ist statusbake abgelaufen */
          if (Time500 <= 0L)
          {
            /* Timer statusbake neusetzen. */
            Time500 = statustim;
            /* Bake aussenden. */
            bake();
          }
          Time500--;
#endif

          hostim();

          chknoa();
          l3rtt_service();
          brosrv();
          trasrv();
          conversd();
#ifdef IPROUTE
          arpsrv();
#endif
#ifndef __LINUX__
          shellsrv();
#endif
          /* Timeout fuer gelernte AX25IP-Routen durchfuehren */
#if defined(__LINUX__) || defined(__WIN32__)
          route_age();
#endif
#ifdef L1TCPIP
          /* TCPIP Timer, */
          /* Noactivity-Timer fuer alle TCPIP Connect   */
          /* reduzieren und ggf. Link disconnecten.     */
          TimerTCP();
#endif /* L1TCPIP */
       }
  }
} /* timsrv() */


/*------------------------------------------------------------------------*/
/*  beacsv()       Baken-Server zur Identifikation des Knotens und        */
/*                 per BEACON-Befehl zuschaltbarer Telemetrie.            */
/*------------------------------------------------------------------------*/
void beacsv(void)
{
  MBHEAD *mbp;
  BEACON *beapoi;
  time_t  upt;
  LONG    upd, uph;
  WORD    i, k;
  ULONG   summe;
  struct tm *p;

  p = localtime(&sys_time);

  for (i = 0, beapoi = beacon; i < L2PNUM; ++beapoi, ++i)
  {
    beapoi->beatim++;
    if (nmbfre > 300 && beapoi->interval != 0)
    {
      if (beapoi->beatim >= beapoi->interval)
      {
        beapoi->beatim = 0;
#ifndef BAKEFIX
        (mbp = (MBHEAD *) allocb(ALLOC_MBHEAD))->l2fflg = L2CPID;
        putstr(signon, mbp);
        if (alias[0] != ' ')
          putalt(alias,mbp);
        putcal(myid, mbp);
        putstr(")\r", mbp);
        if (beapoi->text[0] != '\0')
        {
          putstr(beapoi->text, mbp);
          putchr('\r', mbp);
        }
        rwndmb(mbp);
#ifdef __WIN32__
        sdui(beapoi->beadil, beapoi->beades, myid, (char)i, mbp);
#else
        sdui(beapoi->beadil, beapoi->beades, myid, i, mbp);
#endif /* WIN32 */
        dealmb(mbp);
#else
        if (beapoi->text[0] != '\0')/* Bake nur senden wenn Text gesetzt ist. */
        {                                                 /* Buffer besorgen. */
          (mbp = (MBHEAD *) allocb(ALLOC_MBHEAD))->l2fflg = L2CPID;
          putstr(beapoi->text, mbp);
          putchr('\r', mbp);
          rwndmb(mbp);
          sdui(beapoi->beadil, beapoi->beades, myid, (char)i, mbp);
          dealmb(mbp);
        }
#endif /* BAKEFIX */
#ifdef BEACON_STATUS
        if (beapoi->telemetrie == 2)
#else
        if (beapoi->telemetrie)
#endif
        {
          (mbp = (MBHEAD *) allocb(ALLOC_MBHEAD))->l2fflg = L2CPID;
          upt = sys_time - start_time;          /* Uptime in seconds     */
          /* Summe */
          summe = 0L;                           /* Gesamtdurchsatz       */
          for (k = 0; k < L2PNUM; k++)          /* Alle Ports des Knotens*/
            if (portenabled(k))
              summe += (portstat[k].rx_bytes+portstat[k].tx_bytes);
          if (beapoi->telemetrie == 3) {
            putprintf(mbp,"%02d.%02d.%02d %02d:%02d:%02d %7lu %8lu %4u %5lu",
                          p->tm_mday, p->tm_mon+1, p->tm_year % 100,
                          p->tm_hour, p->tm_min, p->tm_sec,
                          upt,
                          (ULONG) coreleft(),
                          nmbfre,
                          rounds_pro_sec);
            putprintf(mbp, " %3d %3d %10lu %7lu\r",
                          nmblks,                 /* Number of L2-Links    */
                          nmbcir,                 /* Numer of aktive Cir   */
                          summe,
                          thbps*8L);
#ifdef BEACON_STATUS
                  }
                  if (beapoi->telemetrie == 1)
                  {
#else
          } else {
#endif
            putprintf(mbp,"%02d%02d%02d %02d%02d%02d",
                          p->tm_year % 100, p->tm_mon+1, p->tm_mday,
                          p->tm_hour, p->tm_min, p->tm_sec);
            upd = upt/SECONDS_PER_DAY;            /* Uptime days           */
            upt %= SECONDS_PER_DAY;
            uph = upt/SECONDS_PER_HOUR;           /* Uptime hours          */
            upt %= SECONDS_PER_HOUR;
            upt /= SECONDS_PER_MIN;               /* Uptime minutes        */
            putprintf(mbp," Up=%3ld%02ld%02lu Mem=%6lu Buf=%4d Rps=%5lu",
                          upd, uph, upt,          /* Uptime                */
                          (ULONG) coreleft(),     /* Free Mem              */
                          nmbfre,                 /* Free Buffer           */
                          rounds_pro_sec);        /* Runden                */
            putprintf(mbp, "\rLnk=%3d Cir=%3d Sum=%10lu Thr=%6lu\r",
                           nmblks,                /* Number of L2-Links    */
                           nmbcir,                /* Numer of aktive Cir   */
                           summe,
                           thbps*8L);
          }
          rwndmb(mbp);
#ifdef __WIN32__
          sdui(beapoi->beadil, "STAT  \140", myid, (char)i, mbp);
#else
          sdui(beapoi->beadil, "STAT  \140", myid, i, mbp);
#endif /* WIN32 */
          dealmb(mbp);
        }
      }
    }
  }
}

#ifdef BEACON_STATUS
static void statusqual(MBHEAD *mbp, ULONG qual)
{
   if (qual)
           putprintf(mbp,":%-4ld ", qual/10);
   else
           putstr(":--   ",mbp);
}

static void noden(MBHEAD *mbp)
{
  PEER   *pp;
  int    i;
  int    max_peers = netp->max_peers;
  int    len,width = 0;

  for (i = 0, pp = netp->peertab; i < max_peers; i++, pp++)
  {
    if (pp->used)
    {
      mbp->l4time = mbp->mbpc;

      if (pp->typ >= LOCAL)
        continue;

      putide(pp->l2link->call,mbp);
      statusqual(mbp,pp->quality);

      len = (mbp->mbpc - mbp->l4time); /* Laenge eines Eintrages          */
      width += len;
      if (width + len < 79)
      {                                /* einer passt noch in die Zeile   */
        len = (79 % len) / (79 / len);
        while (len--)
          putchr(' ', mbp);
      }
      else
        {
          putchr('\r', mbp);
          width = 0;
        }
    }
  }
}


void bake(void)
{
   MBHEAD *mbp;
   MBHEAD *tmpmbp;
   BEACON *beapoi;
   BOOLEAN frame1 = TRUE;
   char    cuser[4];
   int     i;
   int     anz_frag,info,pacl = 0;

   for (i = 0, beapoi = beacon; i < L2PNUM; ++beapoi, ++i)
   {
     if (beapoi->telemetrie == 3)
     {
       char          runtime[8 + 1];
#ifndef CONNECTTIME_
       unsigned long zeit,
                     sec,
                     min,
                     std;

       zeit = sys_time - start_time;                 /* Uptime in seconds     */
       sec = zeit % 60L; zeit /= 60L;
       min = zeit % 60L; zeit /= 60L;
       std = zeit % 24L; zeit /= 24L;

       if (zeit > 0)
         sprintf(runtime, "%2lud,%2luh",zeit, std);         /* Tage, Stunden. */
       else
         if (std > 0)
           sprintf(runtime, "%2luh,%2lum",std, min);     /* Stunden, Minuten. */
         else
           if (min > 0)
             sprintf(runtime, "%2lum,%2lus", min, sec); /* Minuten, Sekunden. */
           else
             {
               if (sec > 0)
                 sprintf(runtime, "    %2lus", sec);  /* Ausgabe in Sekunden. */
               else
                 sprintf(runtime, "       ");
             }
#else
       sprintf(runtime, "%s", ConnectTime(tic10 / 100));
#endif /* CONNECTTIME */

       (mbp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2fflg = L2CPID;
       convers_user(cuser);

       putprintf(mbp, "Links: %2d, Convers: %2s, Dest/Nodes: %3d, Buffers: %5lu, Runtime: %7s\r"
                      , nmblks
                      , cuser
                      , netp->num_nodes
                      , nmbfre
                      , runtime);

       noden(mbp);
       rwndmb(mbp);

       info = ((mbp->mbpc) - (mbp->mbgc));
       if (info < 257)
       {
#ifdef __WIN32__
         sdui("", "STATUS\140", myid, (char)i, mbp);
#else
         sdui("", "STATUS\140", myid, i, mbp);
#endif /* WIN32 */
         dealmb(mbp);
       }
       else
         {
           anz_frag = info / 217;
           if (anz_frag > 127)
           {
             dealmb(mbp);
             return;
           }
           do
            {
              (tmpmbp = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2fflg = L2CPID;
              if (frame1 == TRUE)
              {
                pacl = 229;
                frame1 = FALSE;
              }
              else
                pacl = 234;

              while (tmpmbp->mbpc < pacl)
              {
                if (!(mbp->mbpc - mbp->mbgc))
                  break;

                putchr(getchr(mbp), tmpmbp);
              }

              rwndmb(tmpmbp);
#ifdef __WIN32__
              sdui("", "STATUS\140", myid, (char)i, tmpmbp);
#else
              sdui("", "STATUS\140", myid, i, tmpmbp);
#endif /* WIN32 */
              dealmb(tmpmbp);
         }

         while (anz_frag-- > 0);
           dealmb(mbp);
       }
     }
   }
}
#endif
/* End of src/l7time.c */
