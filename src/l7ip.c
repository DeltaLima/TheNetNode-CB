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
/* File src/l7ip.c (maintained by: DG1KWA)                              */
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

#ifdef IPROUTE

/* fuer "IPA" zur Meldung von Aenderungen an die INP-Nachbarn */
extern void send_inp_nodebeacon(PEER *);

/************************************************************************
 * Function      : ccpipr - The IProute command
 *
 * Inputs        : clipoi/clicnt
 *
 * Outputs       : Routing Tabelle updaten bzw fuer den User anzeigen
 *
 * Operation    : Verwaltung der Routing-Tabellen
 *----------------------------------------------------------------------*/
void ccpipr(void)
{
    ipaddr    host    = 0L;
    ipaddr    gateway = 0L;
    UWORD     metric;
    WORD      port;
    int       bits    = 32;
    MBHEAD   *mbp;
    IP_ROUTE *ipr;
    unsigned  i       = 1;
    int       flags   = 0;

    mbp = getmbp();

    /* nachschaun, ob Parameter angegeben wurden */
    if (clicnt > 0)
    {
        /* eine gueltige IP-Addresse muss der erste Parameter sein,
         * danach folgt die Anzahl der Host-bits
         */
        if (get_ip_addr(&host, &clicnt, &clipoi))
        {
            i = 0;
            if (skipsp(&clicnt, &clipoi))
            {
                if (*clipoi == '/')
                {
                    clipoi++;
                    clicnt--;
                    if ((bits = nxtnum(&clicnt, &clipoi)) > 32)
                      bits = 32;
                }

                /* the sysop only commands are '+' and '-'
                 */
                if (issyso())
                {
                  skipsp(&clicnt, &clipoi);
                  switch (*clipoi)
                  {
                    case '-':
                      rt_drop(host, bits, FALSE);
                      break;

                    case '+':
                      clipoi++;
                      clicnt--;
                      if (skipsp(&clicnt, &clipoi))
                      {
                        if (clicnt >= 2 && clipoi[1] == ' ')
                          if (toupper(*clipoi) == 'D') {
                            clipoi += 2;
                            clicnt -= 2;
                            flags |= RTDYNAMIC;
                          }
                        if (getport(&clicnt, &clipoi, &port) == FALSE)
                        {
#ifdef KERNELIF
                          if (toupper(*clipoi) == 'K')
                          {
                            nextspace(&clicnt, &clipoi);
                            port = KERNEL_PORT;
                          }
                          else
                          {
                            nextspace(&clicnt, &clipoi);
                            port = NETROM_PORT;
                          }
#else
                          nextspace(&clicnt, &clipoi);
                          port = NETROM_PORT;
#endif
                        }
                        get_ip_addr(&gateway, &clicnt, &clipoi);
                        metric = nxtnum(&clicnt, &clipoi);
                        host >>= (32 - bits);
                        host <<= (32 - bits);
                        rt_add(host, bits, gateway, port, metric, 0, flags, FALSE);
                      }
                  }
                }
            }
        }
    } else
    if (issyso())
      putstr("IPROUTE DestIP/HostBts [+/-] [Port/NETROM] GateIP Metric\r", mbp);

    host &= ~0L << (32 - bits);
#ifdef SPEECH
    putstr(speech_message(264), mbp);
#else
    putstr("IP-Routes of ", mbp);
#endif
    putalt(alias, mbp);
    putid(myid, mbp);
#ifdef SPEECH
    putstr(speech_message(265), mbp);
#else
    putstr("\rDestination------Len--Flags-Interface--Gateway----------Metric----\r", mbp);
#endif

    while (mbp->mbpc < 17) putchr(' ', mbp);

    for (ipr =  (IP_ROUTE *)IP_Routes.head;
         ipr != (IP_ROUTE *)&IP_Routes;
         ipr =  (IP_ROUTE *)ipr->nextip)
          if ((host == ipr->dest &&
               bits == ipr->bits) || i) {
            showroute(ipr, mbp);
          }
    prompt(mbp);
    seteom(mbp);
}

/************************************************************************
 * Function      : Einen IPRoute-Eintrag anzeigen
 *
 * Inputs        : Zeiger auf den Eintrag
 *
 * Outputs       : nix
 *
 * Operation     :
 *----------------------------------------------------------------------*/
void showroute(IP_ROUTE *rp, MBHEAD *bufpoi)
{
    bufpoi->l4time = bufpoi->mbpc;           /* Position merken  */

    show_ip_addr(rp->dest, bufpoi);          /* Adresse anzeigen */
    putspa(17, bufpoi);
    putnum(rp->bits, bufpoi);                /* Bitmaske zeigen  */
    putspa(22, bufpoi);
#ifdef __WIN32__
    putchr((char)(rp->flags & RTDYNAMIC ? 'D' : ' '), bufpoi);
#else
    putchr(rp->flags & RTDYNAMIC ? 'D' : ' ', bufpoi);
#endif /* >WIN32 */
    putspa(28, bufpoi);
#ifndef KERNELIF
    putstr(rp->port == NETROM_PORT ?
           "NET/ROM" :
           portpar[rp->port].name, bufpoi);
#else
    switch(rp->port)
    {
        case NETROM_PORT: putstr("NET/ROM", bufpoi);
                          break;
        case KERNEL_PORT: putstr("KERNEL", bufpoi);
                          break;
        default         : putstr(portpar[rp->port].name, bufpoi);
    }
#endif
    putspa(39, bufpoi);
    if (rp->gateway != 0)
        show_ip_addr(rp->gateway, bufpoi);   /* Gateway          */
    putspa(58, bufpoi);
    if (rp->metric != 0)                     /* if metric set,   */
        putnum(rp->metric, bufpoi);          /* show metric      */
    putchr('\r', bufpoi);
    return;
}

/************************************************************************
 * Function     : eine IP-Addresse anzeigen
 *
 * Inputs       : Die IP-Addresse und ein IP-Buffer
 *
 * Outputs      : nix
 *
 * Operation    :
 *----------------------------------------------------------------------*/
void show_ip_addr(ipaddr address, MBHEAD *bufpoi)
{
#ifdef __WIN32__
  ULONG adr[4];
#else
  UWORD adr[4];
#endif /* WIN32 */
  int   i;

  for (i = 0; i < 4; i++) adr[i] = ((address >> 8 * i) & 0xff);

  putprintf(bufpoi, "%d.%d.%d.%d", adr[3], adr[2], adr[1], adr[0]);
}

/* **********************************************************************
 * Function      : Eine IP-Addresse aus der Kommandozeile lesen
 *
 * Inputs        : clipoi/clicnt
 *
 * Outputs       : FALSE bei Fehler, TRUE wenn ok
 *
 * Operation     :
 * ---------------------------------------------------------------------*/
BOOLEAN get_ip_addr(ipaddr *target, WORD *count, char **ptr)
{
  UWORD num;
  ULONG adr;
  int   i;

  for (adr = 0, i = 0; i < 4; i++) {
    if ((num = nxtnum(count, ptr)) > 255)
      return(FALSE);
    adr = (adr << 8) | num;
    if (i < 3) {
      if ((*count < 1) || (**ptr != '.'))
        return(FALSE);
      else
      {
        (*ptr)++;
        (*count)--;
      }
     }
  }
  *target = adr;
  return(TRUE);
}

/************************************************************************
 * Function        : ARP - Der ARP-Befehl
 *
 * Inputs          : clicnt, clipoi & arp table
 *
 * Outputs         : Updates table.
 *
 * Operation       : Eintrag Hinzufuegen (Sysop) oder Tabelle anzeigen
 * ---------------------------------------------------------------------*/
void ccparp(void)
{
    ipaddr    host;
    WORD      hwport = 0, dgmode = 0;
    unsigned  i;
    MBHEAD   *mbp;
    ARP_TAB  *arp;
    char      call[L2IDLEN];
    char      digi[65];
/* wieso 65??? */
    unsigned  publish;
    unsigned  ttl;

    mbp = getmbp();

    digi[0] = 0;
    publish = FALSE;
    host    = 0;
    ttl     = 0;

   if (issyso())
   {
     if (skipsp(&clicnt, &clipoi))
     {
#ifndef IPROUTEMOD
       if (!get_ip_addr(&host, &clicnt, &clipoi))
         host = 0L;
#else
       if (get_ip_addr(&host, &clicnt, &clipoi) == FALSE)
       {
#ifdef SPEECH
         putmsg(speech_message(267));
#else
         putmsg("\rERROR : Invalid IP-Adresse !\r");
#endif /* SPEECH */
         return;
       }
       else
         {
           /* Wir puerfen den Host ob dieser Gueltig ist.*/
           if (host == FALSE)
           {
             /* Host ist 0, keine Gueltig IP-Adresse. */
#ifdef SPEECH
             putmsg(speech_message(267));
#else
             putmsg("\rERROR : Invalid IP-Adresse !\r");
#endif /* SPEECH */
             return;
           }
         }
#endif /* IPROUTEMOD */

      skipsp(&clicnt, &clipoi);

      switch (*clipoi)
      {
        case '-':
          clipoi++;
          clicnt--;

          if (!host) break;

          if (skipsp(&clicnt, &clipoi))
            if (getport(&clicnt, &clipoi, &hwport) == FALSE)
              hwport = NETROM_PORT;

          arp_drop(host, hwport, FALSE);
          break;

        case '+':
          clipoi++;
          clicnt--;

          if (skipsp(&clicnt, &clipoi)) {
            if (clipoi[1] == ' ') {
              publish = toupper(*clipoi++) == 'P';
              clicnt--;
              nextspace(&clicnt, &clipoi);
            }
          }

          if (skipsp(&clicnt, &clipoi)) {
            if (getport(&clicnt, &clipoi, &hwport) == FALSE) {
              hwport = NETROM_PORT;
              nextspace(&clicnt, &clipoi);
            } else
              if (skipsp(&clicnt, &clipoi)) {
                if (clicnt > 2 && clipoi[2] == ' ') {
                  if (toupper(*clipoi) == 'D') dgmode = 1;
                  else if (toupper(*clipoi) == 'V') dgmode = 2;
                  nextspace(&clicnt, &clipoi);
                 }
              }
          }

         if (getcal(&clicnt, &clipoi, TRUE, call) == YES) {
            getdig(&clicnt, &clipoi, TRUE, digi);
            digi[14] = 0; /* auf 2 Hops begrenzen */
            arp_add(host, hwport, call, digi, dgmode, ttl, publish, FALSE);
         }
         else /* Fehler */
#ifdef SPEECH
           putstr(speech_message(258),mbp);
#else
           putstr("\rERROR : Port or Call invalid !\r",mbp);
#endif
      }

    } else
      if (issyso())
        putstr("ARP DestIP + [Publ.] PORT [DG/VC] CALL [DIGI1[DIGI2]]\r"
               "ARP DestIP - PORT\r", mbp);
   }
    /* ARP-Tabelle ausgeben */
#ifdef SPEECH
    putstr(speech_message(259), mbp);
#else
    putstr("ARP-Table of ", mbp);
#endif
    putalt(alias, mbp);
    putid(myid, mbp);
#ifdef SPEECH
    putstr(speech_message(260), mbp);
#else
    putstr("\rDestination      P Interface  Callsign  Digi                    Mode Timer\r", mbp);
#endif
    i = (host == 0L);
    for (arp =  (ARP_TAB *)Arp_tab.head;
         arp != (ARP_TAB *)&Arp_tab;
         arp =  (ARP_TAB *)arp->nextar)
        if ((i != 0) ||
            (host == arp->dest))
            showarp(arp, mbp);
    prompt(mbp);
    seteom(mbp);
}

/************************************************************************
 * Function      : showarp()
 *
 * Inputs        : Zeiger auf einen arp-Eintrag und einen Buffer
 *
 * Outputs       : Text...
 *
 * Operation     :
 *----------------------------------------------------------------------*/
void showarp(ARP_TAB *arp, MBHEAD *bufpoi)
{
    const char *dgmode_tab[] = {"", "DG", "VC"};
    bufpoi->l4time = bufpoi->mbpc;
    show_ip_addr(arp->dest, bufpoi);
    putspa(17, bufpoi);
    putstr((arp->publish_flag ? "P " : "  "), bufpoi);
    putstr((arp->port == NETROM_PORT ?
            "NET/ROM" : portpar[arp->port].name) , bufpoi);
    putspa(30, bufpoi);
    putid(arp->callsign, bufpoi);
    putspa(39, bufpoi);
    putdil(arp->digi, bufpoi);
    putspa(64, bufpoi);
    putstr(dgmode_tab[(int)arp->dgmode], bufpoi);
    if (arp->timer != 0)
    {
        putspa(70, bufpoi);
        putnum(arp->timer, bufpoi);
    }
    putchr('\r', bufpoi);
}

/************************************************************************/
/*                                                                      */
/* Function : Die IP-Adresse und Bits fuer Subnetz-Maske des Knotens    */
/*            anzeigen bzw. aendern                                     */
/*                                                                      */
/* Inputs   : clipoi/clicnt                                             */
/*                                                                      */
/* Outputs  : nix, my_ip_address und my_ip_bits wird geaendert bzw.     */
/*            angezeigt                                                 */
/*                                                                      */
/* Operation: nur wenn Sysop Aenderung erlaubt                          */
/*                                                                      */
/************************************************************************/
void
ccpipa(void)
{
  MBHEAD *bufpoi;
  PEER   *pp;

  int i;
  int iCalls = 0;
  int max_peers = netp->max_peers;

  BOOLEAN bChanges = FALSE;

  ipaddr  t_ip_addr = 0L;               /* temporaere IP                */
  int     t_ip_bits = 32;               /* temporaere Subnetzbits       */

  if (issyso())         /* nur als Sysop duerfen wir aendern            */
  {
    if (get_ip_addr(&t_ip_addr, &clicnt, &clipoi) == TRUE)  /* IP lesen */
    {
/* Unmoegliche IP-Adressen abfangen (nur niederwertigstes Byte)         */
/* eigentlich muesste man die anderen auch noch pruefen...              */
      if (((t_ip_addr & 0xFF) == 0L) || ((t_ip_addr & 0xFF) == 0xFF))
      {
#ifdef SPEECH
        putmsg(speech_message(266));
#else
        putmsg("Invalid IP address!\r");
#endif
        return;
      }

      if (skipsp(&clicnt, &clipoi))    /* sind noch weitere Zeichen da? */
      {
        if (*clipoi++ == '/')          /* Subnetz-Trenner vorhanden?    */
        {
          clicnt--;
/* Subnetz-Bits lesen und Idiotencheck machen                           */
          t_ip_bits = nxtnum(&clicnt, &clipoi);
          if (t_ip_bits > 32 || t_ip_bits < 1)
            t_ip_bits = 32;
        }
      }
      my_ip_addr = t_ip_addr;
      if (t_ip_addr == 0L)
        my_ip_bits = 0;
      else
        my_ip_bits = t_ip_bits;         /* Subnetz-Bits uebernehmen     */

      bChanges = TRUE;  /* es gibt Aenderungen */
    }
  }

  bufpoi = getmbp();
#ifdef SPEECH
  putstr(speech_message(261), bufpoi);
#else
  putstr("My IP address: ", bufpoi);
#endif
  show_ip_addr(my_ip_addr, bufpoi);

  if ((issyso()) && (bChanges == TRUE))
  {
    /* INP-Nachbarn die Aenderung mitteilen */
    for (i = 0, pp = netp->peertab; i < max_peers; ++i, ++pp)
    {
      if (!pp->used)
        continue;
      if (pp->typ != INP)
        continue;

      if (iCalls == 0)
        putprintf(bufpoi, "Notifying INP-neighbours about changes. (");

      if (++iCalls > 1)
        putprintf(bufpoi, " ");

      putid(pp->l2link->call, bufpoi);  /* Node Call ausgeben */

      send_inp_nodebeacon(pp);
    }

    if (iCalls != 0)
      putprintf(bufpoi, ")\r");
  }

  putprintf(bufpoi, "/%d\r", my_ip_bits);
  prompt(bufpoi);
  seteom(bufpoi);
}

void ccp_ip_help(ipaddr *ip_addr, const char *name)
{
    MBHEAD *bufpoi;

    if (issyso())
        get_ip_addr(ip_addr, &clicnt, &clipoi);
    bufpoi = putals(name);
#ifdef SPEECH
    putstr(speech_message(262), bufpoi);
#else
    putstr(" IP address : ", bufpoi);
#endif
    show_ip_addr(*ip_addr, bufpoi);
    putchr('\r', bufpoi);
    prompt(bufpoi);
    seteom(bufpoi);
}

/************************************************************************
 * Function      : Die Knoten Broadcast-Addresse aendern
 *
 * Inputs        : clipoi/clicnt
 *
 * Outputs       : bcast_ip_address wird geaendert und angezeigt
 *
 * Operation     : nur wenn Sysop Aenderung erlaubt
 * ---------------------------------------------------------------------*/
/* erstmal gesperrt - weil man's laut doku sowieso nicht verwenden darf */
#if 0
void ccpipb()
{
    ccp_ip_help(&bcast_ip_addr, "Broadcast");
}
#endif

/*partyp iptab[] = {
    &Ip_mib[0].value.integer,    0,    MAXPORTMASK,
    &Ip_mib[1].value.integer,    0,    1,
    &Ip_mib[2].value.integer,    2,    MAXTTL,
    &Ip_mib[3].value.integer,    0,    0,
    &Ip_mib[4].value.integer,    0,    0,
    &Ip_mib[5].value.integer,    0,    0,
    &Ip_mib[6].value.integer,    0,    0,
    &Ip_mib[7].value.integer,    0,    0,
    &Ip_mib[8].value.integer,    0,    0,
    &Ip_mib[9].value.integer,    0,    0,
    &Ip_mib[10].value.integer,    0,    0,
    &Ip_mib[11].value.integer,    0,    0,
    &Ip_mib[12].value.integer,    0,    0,
    &Ip_mib[13].value.integer,    1,    65535,
    &Ip_mib[14].value.integer,    0,    0,
    &Ip_mib[15].value.integer,    0,    0,
    &Ip_mib[16].value.integer,    0,    0,
    &Ip_mib[17].value.integer,    0,    0,
    &Ip_mib[18].value.integer,    0,    0,
    &Ip_mib[19].value.integer,    0,    0
};

partyp arptab[] =
{
    &ARPrunning,    0,    1,
    &ARPtimer,    15,    24*60
};
*/

/* ***********************************************************************
 * as per iptab, this is the ICMP MIB. It is not currently used
 */
#ifdef ICMPSTATS

partyp icmptab[] = {
    &Icmp_mib[1].value.integer,    0,    0,
    &Icmp_mib[2].value.integer,    0,    0,
    &Icmp_mib[3].value.integer,    0,    0,
    &Icmp_mib[4].value.integer,    0,    0,
    &Icmp_mib[5].value.integer,    0,    0,
    &Icmp_mib[6].value.integer,    0,    0,
    &Icmp_mib[7].value.integer,    0,    0,
    &Icmp_mib[8].value.integer,    0,    0,
    &Icmp_mib[9].value.integer,    0,    0,
    &Icmp_mib[10].value.integer,    0,    0,
    &Icmp_mib[11].value.integer,    0,    0,
    &Icmp_mib[12].value.integer,    0,    0,
    &Icmp_mib[13].value.integer,    0,    0,
    &Icmp_mib[14].value.integer,    0,    0,
    &Icmp_mib[15].value.integer,    0,    0,
    &Icmp_mib[16].value.integer,    0,    0,
    &Icmp_mib[17].value.integer,    0,    0,
    &Icmp_mib[18].value.integer,    0,    0,
    &Icmp_mib[19].value.integer,    0,    0,
    &Icmp_mib[20].value.integer,    0,    0,
    &Icmp_mib[21].value.integer,    0,    0,
    &Icmp_mib[22].value.integer,    0,    0,
    &Icmp_mib[23].value.integer,    0,    0,
    &Icmp_mib[24].value.integer,    0,    0,
    &Icmp_mib[25].value.integer,    0,    0,
    &Icmp_mib[26].value.integer,    0,    0
};

/* ***************************************************************************
 * Function  : ccpics() - the icmp stats parameters command
 *
 * Inputs    : none - reads ICMP MIB passes it on to ccp_par()
 *
 * Outputs   : as per ccp_par
 *
 * Operation : updates and / or displays ICMP MIB data
 * -------------------------------------------------------------------------*/
void ccpics(void)
{
    ccp_par(icmptab, (sizeof(icmptab) / sizeof(struct param)));
}

#endif

/************************************************************************
 * Function  : Einen Ping absenden
 *
 * Inputs    : clipoi/clicnt
 *
 * Outputs   : nix, bis auf ein Ping-Frame
 *
 * Operation : Die Antwort erfolgt dann durch das Echo-Frame
 * ---------------------------------------------------------------------*/
void ccpping(void)
{
    ipaddr ip_addr;
    MBHEAD *mbp;

    mbp = putals("");
    if (get_ip_addr(&ip_addr, &clicnt, &clipoi))
    {
      if (pingem(ip_addr, 0, 0, 0, NULL)) {
        putstr("ping ", mbp);
        show_ip_addr(ip_addr, mbp);
        putstr(" ...\r", mbp);
      } else {
#ifdef SPEECH
        putstr(speech_message(263), mbp);
#else
        putstr("No route to ", mbp);
#endif
        show_ip_addr(ip_addr, mbp);
        putstr("\r", mbp);
      }
    } else putstr("Syntax: PING <ipaddr>\r", mbp);
    prompt(mbp);
    seteom(mbp);
}

/************************************************************************
 * Function       : rt_add
 *
 * Inputs         : Host-Addressen
 *                  Hostbits in der Addresse
 *                  Gateway-Addresse ( 0 wenn kein Gateway )
 *                  Port-Nummer ( NETROM_PORT oder L2-Port )
 *                  Metric
 *                  Time to Live fuer diesen Eintrag
 *                  Flag fuer private Routen (kein Publish)
 *
 * Returns        : TRUE / FALSE ob die Tabelle gaendert wurde
 *
 * Operation      : Eintrag hinzufuegen oder aendern
 *----------------------------------------------------------------------*/
BOOLEAN rt_add(ipaddr target, unsigned bits, ipaddr gateway, int port,
unsigned metric, unsigned ttl, int flags, BOOLEAN automatic)
{
    IP_ROUTE *iprp;
    IP_ROUTE *iprp2;
    ipaddr temp;
    BOOLEAN new_entry = FALSE;

    /* Tabelle durchsuchen, wenn ein Eintrag existiert dann diesen updaten, */
    /* sonst neuen Eintrag erzeugen                                         */
    if (!route_find(&iprp, &temp, target, bits))
    {
        iprp2 = (IP_ROUTE *)allocb(ALLOC_IP_ROUTE);
        relink((LEHEAD *)iprp2, (LEHEAD *)iprp->previp);
        iprp = iprp2;
        new_entry = TRUE;
    }
    iprp2          = iprp;
    iprp2->dest    = target;
    iprp2->gateway = gateway;
    iprp2->bits    = bits;
    iprp2->port    = port;
    iprp2->metric  = metric;
    iprp2->timer   = ttl;
    iprp2->flags   = flags;

    /* neuer Eintrag, nur dann dieses Flag anfassen */
    if (new_entry == TRUE)
      iprp2->automatic_flag = automatic;

    return(TRUE);
}

/************************************************************************
 * Function   : route_find() - Sucht nach einer Route fuer eine IP-Addresse
 *
 * Inputs     : Zeiger auf den IP-Route-Buffer, Zeiger auf das Ergebnis
 *              Zeiger auf die gewuenschte Ziel-Addresse und die Hostbits
 *
 * Returns    : BOOLEAN, die maskierte Addresse, und der Eintrag,
 *              falls gefunden
 *
 * Operation  : Sucht nach einem exakt passenden Eintrag nach Hostbits
 *              und Addresse
 *----------------------------------------------------------------------*/
BOOLEAN route_find(IP_ROUTE **iprptr, ipaddr *temp, ipaddr target,
unsigned bits)
{
    IP_ROUTE *iprp;

    (*temp) = target;

    if (bits > 32)
        bits = 32;

    (*temp) &= (~0L) << (32 - bits);

    for (iprp =  (IP_ROUTE *)IP_Routes.head;
         iprp != (IP_ROUTE *)&IP_Routes && bits <= iprp->bits;
         iprp =  (IP_ROUTE *)iprp->nextip)
        if (iprp->dest == (*temp) && iprp->bits == bits)
        {
            *iprptr = iprp;
            return(TRUE);
        }

    *iprptr = iprp;
    return(FALSE);
}

/************************************************************************
 * Function     : Einen Eintrag aus der Route-Tabelle loeschen
 *
 * Inputs       : Die IP-Addresse und die Bitmaske
 *
 * Returns      : Ergebnis BOOLEAN, veraendert die Routing-Tabelle
 *
 * Operation    : Eintrag suchen und loeschen
 * ---------------------------------------------------------------------*/
BOOLEAN rt_drop(ipaddr target, unsigned bits, BOOLEAN automatic)
{
    IP_ROUTE *iprp;
    ipaddr temp;

    if (route_find(&iprp, &temp, target, bits))
    {
        /* nicht automatisch gemachte Eintrage nicht anfassen wenn */
        /* wir von einer Automatik aufgerufen worden sind          */
        if ((automatic == TRUE) && (iprp->automatic_flag != TRUE))
            return(FALSE);

        dealoc((MBHEAD *) ulink((LEHEAD *) iprp));
        return(TRUE);
    }
    return(FALSE);
}

/************************************************************************
 * Function     : Einen Eintrag in der ARP-Tabelle machen
 *
 * Inputs       : Die IP-Addresse, der Hardware-Port, die Hardware-
 *                Addresse (Rufzeichen), Datagram-Flag, Time_to_Live
 *                und Publish-Flag
 *                WICHTIG: digi darf nicht zu lang sein, sonst krachts!
 *                (Dies erledigt die aufrufende Routine!)
 *
 * Returns      : Ergebnis BOOLEAN, aber in Wirklichkeit immer TRUE
 *
 * Operation    : Den passenden Eintrag finden und aendern, sonst einen
 *                neuen anlegen.
 *                WICHTIG - Die Tabelle ist in absteigender Reihenfolge
 *                der Hostbits gespeichert.
 *----------------------------------------------------------------------*/
BOOLEAN arp_add(ipaddr target, WORD port, char *callsign, const char *digi,
unsigned dgmode, unsigned ttl, BOOLEAN publish, BOOLEAN automatic)
{
    ARP_TAB *arprp;
    ARP_TAB *arprp2;
    BOOLEAN new_entry = FALSE;

    /* Tabelle durchsuchen, wenn ein Eintrag existiert dann diesen updaten, */
    /* sonst neuen Eintrag erzeugen                                         */
    if (!find_arp(&arprp, target, port))
    {
        arprp = (ARP_TAB *)allocb(ALLOC_ARP_TAB);
        relink((LEHEAD *)arprp, (LEHEAD *)Arp_tab.tail);
        new_entry = TRUE;
    }
    arprp2 = arprp;
    arprp2->dest = target;
#ifdef __WIN32__
    arprp2->port = (unsigned char)port;
#else
    arprp2->port = port;
#endif /* WIN32 */
    arprp2->hwtype = port == NETROM_PORT ? ARP_NETROM : ARP_AX25;
    arprp2->timer = ttl;
    arprp2->publish_flag = publish;
    arprp2->dgmode = dgmode;

    /* neuer Eintrag, nur dann dieses Flag anfassen */
    if (new_entry == TRUE)
      arprp2->automatic_flag = automatic;

    cpyid(arprp2->callsign, callsign);
    cpyidl(arprp2->digi, digi);
    return(TRUE);
}

#ifdef NOTUSE
/****************************************************************************
 * Function     : Ist eine IP-Addresse als ARP bekannt?
 *
 * Inputs       : die IP-Addresse
 *
 * Returns      : Ergebnis BOOLEAN
 *-------------------------------------------------------------------------*/
static BOOLEAN is_ipaddr_inuse(ipaddr target)
{
    ARP_TAB *arprp;

    for (arprp =  (ARP_TAB *)Arp_tab.head;
         arprp != (ARP_TAB *)&Arp_tab;
         arprp =  (ARP_TAB *)arprp->nextar)
        if (arprp->dest == target)
          return(TRUE);
    return(FALSE);
}

/****************************************************************************
 * Function     : Ist auf einem Port ein User als ARP bekannt?
 *
 * Inputs       : die AX25-Addresse, Port
 *
 * Returns      : Ergebnis BOOLEAN
 *-------------------------------------------------------------------------*/
static ipaddr is_hwaddr_inuse(char *hwaddr, WORD hwport)
{
    ARP_TAB *arprp;

    for (arprp =  (ARP_TAB *)Arp_tab.head;
         arprp != (ARP_TAB *)&Arp_tab;
         arprp =  (ARP_TAB *)arprp->nextar)
        if (cmpid(arprp->callsign, hwaddr) && arprp->port == hwport)
          return(arprp->dest);
    return(0L);
}
#endif
/****************************************************************************
 * Function     : Eine IP-Addresse aufloesen
 *
 * Inputs       : Zeiger auf das Ergebnis, die IP-Addresse
 *
 * Returns      : Ergebnis BOOLEAN, arpptr wird gesetzt
 *
 * Operation    : Die ARP-Tabelle nach einem passenden Eintrag absuchen
 *-------------------------------------------------------------------------*/
BOOLEAN find_arp(ARP_TAB **arpptr, ipaddr target, WORD hwport)
{
    ARP_TAB *arprp;

    for (arprp =  (ARP_TAB *)Arp_tab.head;
         arprp != (ARP_TAB *)&Arp_tab;
         arprp =  (ARP_TAB *)arprp->nextar)
        if ((arprp->dest == target) &&
            (hwport == arprp->port))
        {
            *arpptr = arprp;
            return(TRUE);
        }
    return(FALSE);
}

/************************************************************************
 * Function  : Einen ARP-Eintrag loeschen
 *
 * Inputs    : Die IP-Nummer und die Hostbits
 *
 * Returns   : Ergebnis BOOLEAN, ARP-Tabelle wird geandert
 *
 * Operation : Den passenden Eintrag suchen und ab in den Muell
 *----------------------------------------------------------------------*/
BOOLEAN arp_drop(ipaddr target, WORD hwport, BOOLEAN automatic)
{
    ARP_TAB *arprp;

    if (find_arp(&arprp, target, hwport))
    {
        /* nicht automatisch gemachte Eintrage nicht anfassen wenn */
        /* wir von einer Automatik aufgerufen worden sind          */
        if ((automatic == TRUE) && (arprp->automatic_flag != TRUE))
            return(FALSE);

        dealoc((MBHEAD *)ulink((LEHEAD *)arprp));
        return(TRUE);
    }
    return(FALSE);
}

/*- End of IP router switch commands ---------------------------------------*/

/*- Start of IP router timer service routine--------------------------------*/

void arpsrv(void)
{
     ARP_TAB *arprp;
     ARP_TAB *arp;

    if (--ARPcounter == 0)
    {
        ARPcounter = 60;
        for (arprp =  (ARP_TAB *)Arp_tab.head;
             arprp != (ARP_TAB *)&Arp_tab; )
        {
            if (IPpar[arprp->port].ipMode & ARP_OK) {
              arp = arprp;
              arprp = (ARP_TAB *)arprp->nextar;
              /* ARP-Eintrag entfernen */
              if (arp->timer != 0 && --arp->timer == 0)
              {
                /* eventuell zusaetzlich angelegte Route entfernen */
                rt_drop(arp->dest, 32, TRUE);

                arp_drop(arp->dest, arp->port, FALSE);
              }
            }
        }
    }
}

/*- End of IP router timer service routine------------------------------*/

#endif

/* End of src/l7ip.c */
