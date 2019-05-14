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
/* File src/mh.c (maintained by: ???)                                   */
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

/*----------------------------------------------------------------------*/

MHTAB l2heard, l3heard;

static BOOLEAN  load_table(MHTAB *);
static void     save_table(char *, MHTAB *);
static void     ccp_mh(MHTAB *);
#ifdef MH_LISTE
static char    *lese_rx_bytes(char *rx_bytes,int rxbytes);
static char    *lese_tx_bytes(char *tx_bytes,int txbytes);
static void     mh_delete(MHTAB *mh, char *bufcal, int option);
#endif
/* MHEARD-Liste in den Speicher laden
 */
static BOOLEAN
load_table(MHTAB *mh)
{
  FILE *fp;
  MHEARD *mhp;
  char file[128];               /* Puffer fuer Filenamen                */
  char *ptr;
  WORD  cnt;

  strcpy(file, confpath);
  strcat(file, mh->name);
  if ((fp = xfopen(file, "rt")) != NULL)
  {
    while (fgets(file, 120, fp) != NULL)
    {
      if (*file == ':')
      {
        sscanf(file + 1, "%ld", (long *) &mh->mhstart);
        continue;
      }
      mhp = (MHEARD *)allocb(ALLOC_MHEARD);
      ptr = file;
      cnt = strlen(ptr) - 1;                           /* \n uebersehen */
      mhp->heard = nxtlong(&cnt, &ptr);
      mhp->rx_bytes = nxtlong(&cnt, &ptr);
      mhp->tx_bytes = nxtlong(&cnt, &ptr);
      mhp->port = nxtnum(&cnt, &ptr);
      mhp->damawarn = nxtnum(&cnt, &ptr);
      mhp->rx_rej = nxtlong(&cnt, &ptr);
      mhp->tx_rej = nxtlong(&cnt, &ptr);
#ifdef EAX25
      /* Standardmaessig kein EAX.25 */
      mhp->eax_link = FALSE;
#endif
#ifdef MH_LISTE
      mhp->flag = nxtnum(&cnt, &ptr);
#endif
      if (getcal(&cnt, &ptr, TRUE, mhp->id) == YES)
      {
        mh->act++;
        relink((LEHEAD *)mhp, (LEHEAD *)(mh->heardl.tail));
        continue;
      }
      else
        dealoc((MBHEAD *)mhp);
    }
    fclose(fp);
    return(TRUE);
  }
  return(FALSE);
}

/*----------------------------------------------------------------------*/

/*
 *  MHEARD-Liste abspeichern
 */

static void
save_table(char *path, MHTAB *mh)
{
  FILE *fp;
  MHEARD huge *mhp;
  char file[128];               /* Puffer fuer Filenamen                */
  char call[12];

  strcpy(file, path);
  strcat(file, mh->name);
  if ((fp = xfopen(file, "wt")) != NULL)
  {
    for (mhp  = (MHEARD *)mh->heardl.head;
         mhp != ((MHEARD huge *)&(mh->heardl));
         mhp  = mhp->next)
    {
      call2str(call, mhp->id);
      fprintf(fp, "%lu %lu %lu %u %u %lu %lu"
#ifdef MH_LISTE
                  " %u"
#endif
                  " %s\n"
                ,(long)mhp->heard, mhp->rx_bytes, mhp->tx_bytes
                ,mhp->port, mhp->damawarn, mhp->rx_rej, mhp->tx_rej
#ifdef MH_LISTE
                ,mhp->flag
#endif
                ,call);
    }

    fprintf(fp, ":%ld\n", (long) mh->mhstart);
    fclose(fp);
  }
}

/*----------------------------------------------------------------------*/
void save_mh(void)
{
#ifdef ST
  char *mcp;
#endif

  save_table(confpath, &l2heard);
#ifdef ST
  if ((mcp = getenv("MHTOPPATH")) != NULL)
    save_table(mcp, &l2heard);
#endif

  save_table(confpath, &l3heard);
}

/*----------------------------------------------------------------------*/
MHEARD *mh_lookup(MHTAB *mh, char *id)
{
  MHEARD *mhp;
  LHEAD  *heardl = &mh->heardl;

  for (mhp  = (MHEARD *)heardl->tail;
       mhp != (MHEARD *)heardl;
       mhp  = mhp->prev)
    if (cmpid(id, mhp->id))
      return(mhp);
  return(NULL);
}

/*----------------------------------------------------------------------*/
/* Call auf dem richtigen Port suchen */
#ifdef MH_LISTE
MHEARD *mh_lookup_port(MHTAB *mh, char *id, UWORD port, int txrx)
#else
MHEARD *mh_lookup_port(MHTAB *mh, char *id, UWORD port)
#endif
{
  MHEARD *mhp;
  LHEAD  *heardl = &mh->heardl;

  for (mhp  = (MHEARD *)heardl->tail;
       mhp != (MHEARD *)heardl;
       mhp  = mhp->prev)
#ifdef MH_LISTE
  {
    /* TX-FRAME */
    if (txrx == TRUE)
    {
#endif
      if (cmpid(id, mhp->id) && port == mhp->port)
        return(mhp);
#ifdef MH_LISTE
    }
    /* RX-FRAME */
    else
      {
        /* Rufzeichen gefunden? */
        if (cmpid(id, mhp->id))
        {
          /* auf dem gleichen Port? */
          if (port == mhp->port)
            /* Rufzeichen und Port */
            /* stimmen ueber ein.  */
            return(mhp);
          else
            /* Markiere Rufzeichen,    */
            /* keine Anzeige unter MH. */
            mhp->flag = FALSE;
        }
      }
  }
 #endif
  return(NULL);
}

/*----------------------------------------------------------------------*/
void mh_update(MHTAB *mh, MHEARD *mhp, char *id, UWORD port)
{
  mhp->heard = sys_time;
  cpyid(mhp->id, id);
  mhp->via[0] = NUL;
  mhp->port = port;
#ifdef MH_LISTE
  mhp->flag = TRUE;
#endif
  ulink((LEHEAD *)mhp);
  relink((LEHEAD *)mhp, (LEHEAD *)(mh->heardl.tail));
}

/*----------------------------------------------------------------------*/
void mh_clear(MHEARD *mhp)
{
  mhp->tx_bytes =
  mhp->rx_bytes =
  mhp->tx_rej =
  mhp->rx_rej = 0L;
  mhp->damawarn = 0;
#ifdef MHEAX_LINKFIX
  mhp->eax_link = FALSE;
#endif /* MHEAX_LINKFIX */
}

/*----------------------------------------------------------------------*/
MHEARD *mh_add(MHTAB *mh)
{
  MHEARD *mhp;

  if (mh->max == 0) return(NULL);
  while (mh->act >= mh->max) {
    dealoc((MBHEAD *)ulink((LEHEAD *)mh->heardl.head));
    mh->act--;
  }
  relink((LEHEAD *)(mhp = ((MHEARD *)allocb(ALLOC_MHEARD))),
         (LEHEAD *)(mh->heardl.head));
  mh->act++;
  mh_clear(mhp);
  return(mhp);
}

static void
ccp_mh(MHTAB *mh)
{
#ifndef MH_LISTE
  MHEARD        *prevmhp;
#endif
  MHEARD        *mhp;
  char           call[L2IDLEN];
  char          *cpoisa = NUL;
  WORD           ccntsa = FALSE;
  WORD           num = 10;
  WORD           i;
  WORD           n;
  WORD           what = 0;
  char           mask[MAXMASK];
  WORD           comp(const void *, const void *);
  char           tmp1[10];
  char           tmp2[10];
  char           buffer[100], *bp;
  FILE           *fp;
  char           *tmpfile;
  struct         tm *ts;
  BOOLEAN        show_counter = FALSE;
#ifdef MH_LISTE
  char           _rx_bytes[7];
  char           _tx_bytes[7];
#define          _TNN     0
#define          _FLEXNET 1
  WORD           show_mode;
  ULONG          d, h, m, s; /* Tag, Stunde, Minute, Sekunde */
  WORD           j = 0;      /* Zaehler fuer Flexnet         */
  WORD           uport = FALSE;      /* einzelne Portausgabe         */
#endif
  /* parameter auswerten */

#ifdef MH_LISTE
  if (MHdefault)          /* Welche MH-Liste ist zur auswahl ? */
    show_mode = _FLEXNET; /* ist das etwa FLEXNET ?            */
  else                    /* oder doch                         */
    show_mode = _TNN;     /* unser altes TNN (HI)              */
#endif

  /* Parameter auswerten */
  if (clicnt)
  {
    cpoisa = clipoi;
    ccntsa = clicnt;
    do
    {
      if (*cpoisa == '+')
      {
        show_counter = TRUE;
        *cpoisa = 32;
      }
      cpoisa++;
    } while (--ccntsa > 0);

    if (issyso() == TRUE && *clipoi == '=')
    {
      clipoi++;
      clicnt--;
      num = nxtnum(&clicnt, &clipoi);
      num = min(5000, num);
      num = max(0, num);
      mh->max = num;
      while (mh->act > mh->max)
      {
        dealoc((MBHEAD *)ulink((LEHEAD *)mh->heardl.head));
        mh->act--;
      }

      /* Meldung */
      memset(buffer, 0 , sizeof(buffer));
      sprintf(buffer, "Capacity set to %u entries.\r", mh->max);
      putmsg(buffer);
      return;
    }

    if (issyso() == TRUE && *clipoi == '-')
#ifdef MH_LISTE
      mh_delete(mh,clipoi,0);
#else
    {
      clipoi++;
      if (--clicnt == 0)
      {
        for (mhp  = (MHEARD *)mh->heardl.tail;
             mhp != (MHEARD *)&(mh->heardl);
             mhp  = prevmhp)
        {
          prevmhp  = mhp->prev;

          if (mhp->tx_bytes + mhp->rx_bytes == 0)
          {
            dealoc((MBHEAD *)ulink((LEHEAD *)mhp));
            mh->act--;
          }
          else
            mh_clear(mhp);
        }
        mh->mhstart = sys_time;
      }
      else
      {
        if (getcal(&clicnt, &clipoi, FALSE, call) != YES)
        {
          putmsg("Invalid Call");
          return;
        }
        for (mhp  = (MHEARD *)mh->heardl.tail;
             mhp != (MHEARD *)&(mh->heardl);
             mhp  = prevmhp)
        {
          prevmhp  = mhp->prev;

          if (!cmpcal(call,mhp->id))
            continue;
          dealoc((MBHEAD *)ulink((LEHEAD *)mhp));
          mh->act--;
        }
      }
    }
#endif
    else
      {
        cpoisa = clipoi;
        ccntsa = clicnt;

        if ((num = nxtnum(&clicnt, &clipoi)) == 0)
        {
          clipoi = cpoisa;
          clicnt = ccntsa;
          num = 10;
        }
#ifdef MH_LISTE
        /* Pruefen ob im Buffer eine Zahl ist. Wenn */
        /* nicht, kann das nur eine Rufzeichen sein.*/
        if ((isalpha(*cpoisa) == FALSE) && !(*cpoisa == '*'))
        {
          /* Liegt die Portangabe zwischen 0..15?       */
          /* Wenn ja, markieren wir den Port, ansonsten */
          /* wird die Anzahl der Rufzeichen ausgegeben. */
          if (atoi(cpoisa) < 16)
          {
            /* Portangabe holen. */
            uport = atoi(cpoisa);
            /* Pruefen ob der Port eingeschaltet ist. */
            if (portenabled(uport))
            {
              num = MHlen;
              /* Markiere Portangabe. */
              what = 3;
            }
            else
              {
#ifdef SPEECH
                putmsg(speech_message(194));
#else
                putmsg("Port is disabled!\r");
#endif
                return;
              }
          }
      }
      else
#endif
        if ((mhprm(clipoi, clicnt, mask) == TRUE)  && !(*cpoisa == '*'))
        {
          what = 2;
          num = mh->max;
        }
        else
          if ((getcal(&clicnt, &clipoi, FALSE, call) == YES) && !(*cpoisa == '*'))
          {
            what = 1;
            num = mh->max;
          }
    }
  }
  else
#ifdef MH_LISTE
    num = MHlen;
#else
    num = 10;
#endif
#ifdef MH_LISTE
  if (ccntsa)
  {
    if (*cpoisa == '*')
      num = mh->max;
  }
#endif

  if ((tmpfile = tempnam(textpath, "mh")) == NULL)
    return;

  if ((fp = xfopen(tmpfile, "wt")) == NULL)
  {
    putmsg("Sri, can't open tempfile...\r");
    free(tmpfile);
    return;
  }

  call2str(tmp1, myid);
  for (i = 0; alias[i] != ' ' && i < L2CALEN; ++i)
    fprintf(fp, "%c", alias[i]);

  fprintf(fp, ":%s> MHEARD (%d/%d)\n", tmp1, mh->act, mh->max);

#ifdef MH_LISTE
  /* Pruefen ob TNN-Liste eingeschaltet ist.         */
  /* Wenn ja, Senden wir paar extra Infos fuer       */
  /* den User, ansonsten kann nur die Flexnet-Liste  */
  /* eingeschaltet sein.                             */
  if (show_mode == _TNN)
  {
#ifdef SPEECH
    fprintf(fp, speech_message(281));
#else
    fprintf(fp, "Date     Time  Port");
#endif

    if (!show_counter)
#ifdef SPEECH
      fprintf(fp,speech_message(282));
#else
      fprintf(fp,"                Rx         Tx    Call\n");
#endif
    else
#ifdef SPEECH
      fprintf(fp,speech_message(283));
#else
      fprintf(fp,"       RX         TX    Call       RX-Rej  TX-Rej    DAMA\n");
#endif
  }
#endif

  /* kommando ausfuehren */
  for (n = 0, mhp  = (MHEARD *)mh->heardl.tail;
              mhp != (MHEARD *)&(mh->heardl) && n < num;
              mhp  = mhp->prev, n++)
  {
    if (mhp->heard == 0L)
      continue;

    switch (what)
    {
      case 0:  break;

      case 1:  if (! cmpcal(call,mhp->id))
                 continue;
               break;

      case 2:  if (! c6mtch(mhp->id, mask))
                 continue;
               break;
#ifdef MH_LISTE
      case 3:  if ( mhp->port != uport )
               {
                 n--;
                 continue;
               }
               break;
#endif

      default: continue;
    }

#ifdef MH_LISTE
    if (what == 0 && mhp->flag == FALSE)
    {
      n--;
      continue;
    }
#endif

    call2str(tmp2, mhp->id);
    ts = localtime(&mhp->heard);

#ifdef MH_LISTE
    if (show_mode == _TNN)
    {
#endif
      bp = buffer;
      bp += sprintf(bp, "%02d.%02d.%02d %02d:%02d ", ts->tm_mday,
                                                     ts->tm_mon+1,
                                                     ts->tm_year%100,
                                                     ts->tm_hour,
                                                     ts->tm_min);

      if (mh == &l2heard)
      {
        if (show_counter)
        {
#ifdef MH_LISTE
          bp += sprintf(bp, "P%2u [%10s,%10s] %-9s %6lur %6lut %6ud"
                          ,mhp->port
                          ,lese_rx_bytes(_rx_bytes,mhp->rx_bytes)
                          ,lese_tx_bytes(_tx_bytes,mhp->tx_bytes)
                          ,tmp2
                          ,mhp->rx_rej
                          ,mhp->tx_rej
                          ,mhp->damawarn);
#else
          bp += sprintf(bp, "P%2u [%10lu,%10lu] %-9s %6lur %6lut %6ud"
                          ,mhp->port
                          ,mhp->rx_bytes
                          ,mhp->tx_bytes
                          ,tmp2
                          ,mhp->rx_rej
                          ,mhp->tx_rej
                          ,mhp->damawarn);
#endif
        }
        else
          {
#ifdef MH_LISTE
            bp += sprintf(bp, "(%s)%.*s [%10s,%10s] %s"
                            ,portpar[mhp->port].name
                            ,10 - (WORD)strlen(portpar[mhp->port].name)
                            ,"          "
                            ,lese_rx_bytes(_rx_bytes,mhp->rx_bytes)
                            ,lese_tx_bytes(_tx_bytes,mhp->tx_bytes)
                            ,tmp2);
#else
            bp += sprintf(bp, "(%s)%.*s [%10lu,%10lu] %s"
                            ,portpar[mhp->port].name
                            ,10 - (WORD)strlen(portpar[mhp->port].name)
                            ,"          "
                            ,mhp->rx_bytes
                            ,mhp->tx_bytes
                            ,tmp2);
#endif
#ifdef EAX25
            /* EAX.25-Verbindungen markieren */
            if (mhp->eax_link == TRUE)
              bp += sprintf(bp, "%.*s (EAX.25)"
                              ,9 - (WORD)strlen(tmp2)
                              ,"         ");
#endif
          }
      }
      else
        if (mh == &l3heard)
#ifdef MH_LISTE
          bp += sprintf(bp, "(%s)%.*s [%10s,%10s] %s"
                          ,portpar[mhp->port].name
                          ,10 - (WORD)strlen(portpar[mhp->port].name)
                          ,"          "
                          ,lese_rx_bytes(_rx_bytes,mhp->rx_bytes)
                          ,lese_tx_bytes(_tx_bytes,mhp->tx_bytes)
                          ,tmp2);
#else
          bp += sprintf(bp, "P%2u [%10lu,%10lu] %-9s"
                          ,mhp->port
                          ,mhp->rx_bytes
                          ,mhp->tx_bytes
                          ,tmp2);
#endif
        fprintf(fp, "%s\n", buffer);
    }
#ifdef MH_LISTE
    if (show_mode == _FLEXNET)
    {
      if (j == 3)
      {
        fprintf(fp, "\n");
        j = 0;
      }

      j ++;
      d = (sys_time-mhp->heard);
      s = d % 60L; d /= 60L;
      m = d % 60L; d /= 60L;
      h = d % 24L; d /= 24L;

      if (d > 0)
        fprintf(fp, " %2ldd,%2ldh ", d, h);    /* dd,hh */
      else
        if (h > 0)
          fprintf(fp, " %2ldh,%2ldm ", h, m);  /* hh,mm */
        else
          if (m > 0)
            fprintf(fp, " %2ldm,%2lds ", m, s); /* mm,ss */
          else
            fprintf(fp, "     %2lds ", s);      /*    ss */

      if (mh == &l2heard)
        fprintf(fp, "P%-2u %-9s"
                  ,mhp->port
                  ,tmp2);

      if (mh == &l3heard)
        fprintf(fp, "P%-2u %-9s"
                  ,mhp->port
                  ,tmp2);
    }
  }
#endif

  fclose(fp);               /* Temporaeres File schliessen */
  userpo->fname = tmpfile;
  ccpread(tmpfile);
}

/**************************************************************************/
/*  ccpmh()   Erweiterter MHEARD-Befehl mit Wildcards wie bei NODES       */
/*            Beispiel: "MHEARD D?5*"              04.06.91  DG5OJ/DB2OS  */
/*            Special Counter Erweiterung, DB7KG                          */
/*------------------------------------------------------------------------*/
void ccpl2mh(void)
{
  ccp_mh(&l2heard);
}

void ccpl3mh(void)
{
  ccp_mh(&l3heard);
}

/*........................................................................*/
/*  mhprm()   Parameter des erweiterten MHEARD-Befehls auswerten,         */
/*            testet ob 'p' ein Wort mit Wildcards enthaelt und kopiert   */
/*            dieses nach 'mp'.  Case-Conversion:  db*E wird zu DB*E      */
/*            Parameter 'n' muss > 0 sein!          04.06.91 DB5OJ/DB2OS  */
/*........................................................................*/
BOOLEAN mhprm(char *p, WORD n, char *mp)
{
  WORD   i, c, ret;

  ret = FALSE;

  if (skipsp(&n, &p))
  {
    for (i = 0; i < (MAXMASK - 1); )
    {
      if (!n || ((c = *p++) == ' '))
        break;

      n--;

      if ((c == MATCHMANY) || (c == MATCHONE))
        ret = TRUE;

      mp[i++] = isascii(c) ? toupper(c) : MATCHONE;
    }
    mp[i] = MATCHEND;
  }
  return(ret);
}

/*----------------------------------------------------------------------*/
void init_mh(void)
{
  l2heard.mhstart =
  l3heard.mhstart = sys_time;
  l2heard.act =
  l3heard.act = 0;
  l2heard.max =
  l3heard.max = 10;
  l2heard.name = "MHEARD.TAB";
  l3heard.name = "L3HEARD.TAB";
  inithd(&l2heard.heardl);
  inithd(&l3heard.heardl);
  load_table(&l2heard);
  load_table(&l3heard);
}

/*----------------------------------------------------------------------*/
void exit_mh(void)
{
  save_mh();
}

#ifdef MH_LISTE
static char *lese_rx_bytes(char *rx_bytes,int rxbytes)
{
  if(rxbytes >= 999999)
  {
    sprintf(rx_bytes,"%iMB",rxbytes/(1024*1024));
    return(rx_bytes);
  }
  else
    if(rxbytes >= 9999)
    {
      sprintf(rx_bytes,"%iKB",rxbytes/1024);
      return(rx_bytes);
    }
    else
      sprintf(rx_bytes,"%iB ",rxbytes);
      return(rx_bytes);
}

static char *lese_tx_bytes(char *tx_bytes,int txbytes)
{
  if(txbytes >= 999999)
  {
    sprintf(tx_bytes,"%iMB",txbytes/(1024*1024));
    return(tx_bytes);
  }
  else
    if(txbytes >= 9999)
    {
      sprintf(tx_bytes,"%iKB",txbytes/1024);
      return(tx_bytes);
    }
    else
      sprintf(tx_bytes,"%iB ",txbytes);
      return(tx_bytes);
}

static void mh_delete(MHTAB *mh, char *bufcal, int option)
{
  MHEARD        *prevmhp;
  MHEARD        *mhp;
  WORD           laenge = strlen(bufcal);
  char           call[10];

  bufcal++;
  if (--laenge == 0)
  {
    for (mhp  = (MHEARD *)mh->heardl.tail;
         mhp != (MHEARD *)&(mh->heardl);
         mhp  = prevmhp)
    {
      prevmhp  = mhp->prev;

      if (mhp->tx_bytes + mhp->rx_bytes == 0)
      {
        dealoc((MBHEAD *)ulink((LEHEAD *)mhp));
        mh->act--;
      }
      else
        mh_clear(mhp);
    }

    mh->mhstart = sys_time;
  }
  else
    {
      if (getcal(&laenge, &bufcal, FALSE, call) != YES)
      {
#ifdef SPEECH
        putmsg(speech_message(196));
#else
        putmsg("Invalid Call\n");
#endif
        return;
      }

      for (mhp  = (MHEARD *)mh->heardl.tail;
           mhp != (MHEARD *)&(mh->heardl);
           mhp  = prevmhp)
      {
        prevmhp  = mhp->prev;

        if (!cmpcal(call,mhp->id))
          continue;

        dealoc((MBHEAD *)ulink((LEHEAD *)mhp));
        mh->act--;
      }
    }
}
#endif /* MH_LISTE */

/* End of src/mh.c */
