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
/* File src/l3ip.c (maintained by: DG1KWA)                              */
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
static void     send_ip_mb(MBHEAD *, unsigned, ipaddr, unsigned);

char            QST[] = {'Q', 'S', 'T', ' ', ' ', ' ', '\140'};

ipaddr          my_ip_addr = 0x00000000L;    /* default: eigene IP      */
ipaddr          bcast_ip_addr = 0x00000000L; /* default: meine BCAST-IP */
int             my_ip_bits = 0;              /* default: Subnet-Bits    */

#ifdef IPROUTEMOD
                /* alle nicht definierten IP-Adressen */
                /* an die my_default_ip_addr schicken */
ipaddr          my_default_ip_addr = 0x00000000L;
#endif /* IPROUTEMOD */

/* Globale Variablen */

LHEAD           IP_Routes;      /* Tabelle mit den eingetragenen Routen */
LHEAD           Arp_tab;        /* Tabelle mit den ARP-Requests         */
LHEAD           iprxfl;         /* Empfangene IP-Frames                 */
LHEAD           arprxfl;        /* Empfangene ARP-Frames                */

ARP             arp;            /* ARP Header                           */

MIB_ENTRY Ip_mib[NUMIPMIB + 1] =
{
  {"",                  {0}},
  {"ipForwarding",      {1}},
  {"ipDefaultTTL",      {32}},
  {"ipInReceives",      {0}},
  {"ipInHdrErrors",     {0}},
  {"ipInAddrErrors",    {0}},
  {"ipForwDatagrams",   {0}},
  {"ipInUnknownProtos", {0}},
  {"ipInDiscards",      {0}},
  {"ipInDelivers",      {0}},
  {"ipOutRequests",     {0}},
  {"ipOutDiscards",     {0}},
  {"ipOutNoRoutes",     {0}},
  {"ipReasmTimeout",    {TLB}},
  {"ipReasmReqds",      {0}},
  {"ipReasmOKs",        {0}},
  {"ipReasmFails",      {0}},
  {"ipFragOKs",         {0}},
  {"ipFragFails",       {0}},
  {"ipFragCreates",     {0}}
};

WORD            ipnat = 600;

IPPORTPAR       IPpar[L2PNUM + 1];      /* fuer jeden L2-Port + NETROM  */

UWORD           ARPcounter;             /* ARP-Timing                   */
UWORD           ARPtimer;

ULONG           ip_cksum;

/************************************************************************
 * Function : send_ip_mb
 *
 * Inputs   : mhbp - Ein Messagebuffer
 *            port - Der L2-Port zum Senden (oder NETROM_PORT)
 *            gateway - Zieladdresse
 *            tos
 *
 * Returns  : Fehler-Code
 *
 * Operation: Ein Frame an ein Gateways senden
 *
 *----------------------------------------------------------------------*/
static void
send_ip_mb(MBHEAD *mhbp, unsigned port, ipaddr gateway, unsigned tos)
{
#ifdef KERNELIF
  if (port == KERNEL_PORT)
  {
    ifip_frame_to_kernel(mhbp);
    return;
  }
#endif
  if (port == NETROM_PORT)
    nr_iface(mhbp, gateway);
  else
    l2_iface(mhbp, port, gateway, tos);
}

/************************************************************************
 * Function : ipinit
 *
 * Inputs   : none
 *
 * Returns  : none
 *
 * Operation: Initialisation des IP-Gateways beim Softwarestart
 *
 *----------------------------------------------------------------------*/
void
ipinit(void)                    /* Router initialisation                */
{
  register int    i;

  inithd(&iprxfl);
  inithd(&arprxfl);

  inithd(&IP_Routes);
  inithd(&Arp_tab);

#ifdef TCP_STACK
  /* TCP-Stack Initialisieren. */
  StackInitTCP();
#endif /* TCP_STACK */

  /* NET/ROM MUSS immer freigeschaltet bleiben ! */
  for (i = 0; i <= L2PNUM; i++)
  {
    IPpar[i].ipMode = ARP_OK + IP_FORWARDING;
  }

  ARPcounter = ARPtimer = 60;
}

/************************************************************************
 * Function : IP service
 *
 * Inputs   : None
 *
 * Returns  : None
 *
 * Operation: Alle empfangenen IP-Frames aus der Empfangsliste holen und
 *            verarbeiten, eventuell ARP ausloesen
 *----------------------------------------------------------------------*/
void
ipserv(void)
{
  register MBHEAD *mbhd;

/* die Empfangsliste durchgehen und jedes Frame durch den Router        */
/* schicken                                                             */
  while ((mbhd = (MBHEAD *)iprxfl.head) != (MBHEAD *)&iprxfl)
  {
    ulink((LEHEAD *)mbhd);
    ip_route(mbhd);             /* Frames an den ip_router */
    dealmb(mbhd);
  }

  /* jetzt verarbeiten wir die ARP-Frames */
  while ((mbhd = (MBHEAD *)arprxfl.head) != (MBHEAD *)&arprxfl)
  {
    ulink((LEHEAD *)mbhd);
    arp_service(mbhd);          /* Frames an den arp_service */
    dealmb(mbhd);
  }
}

/************************************************************************
 * Function : Ein Frame for a single frame. Called by ip server
 *
 * Inputs   : Zeiger auf das Frame, das geroutet werden soll
 *
 * Returns  : None
 *
 * Operation: Leitet das Frame weiter oder generiert ein ICMP-Frame, das
 *            Frame wird segmentiert, wenn der Sendelayer das erlaubt hat
 * ---------------------------------------------------------------------*/
void
ip_route(MBHEAD *mbhd)          /* Internet server                      */
{
  register unsigned int i;      /* genereller Zaehler                   */
  register MBHEAD *tbp;         /* Buffer fuer Ausgabe (z.B. ICMP)      */
  char huge      *rxnxt;        /* die Position in mbhd merken          */
  static IP       ip;           /* Structure zum decodieren des         */
                                /* IP-Header                            */
  int             strict;       /* Flag (opt-Feld) strict-routing       */
  ipaddr          gateway;      /* Addresse des gefundenen Gateways     */
  IP_ROUTE       *rp;           /* Pointer um die Route zu finden       */
  unsigned        length;       /* Anzahl der Datenbytes im Frame       */
  unsigned        offset;       /* Offset fuer Fragmentation            */
  BOOLEAN         rxbroadcast;  /* ist es ein Broadcast-Frame?          */
  BOOLEAN         mf_flag;      /* More Flag bei Fragmentation          */
  unsigned char  *opt,
                 *ptr;          /* Pointers bei den option-Flags        */
  unsigned char   opt_len;      /* Length bei option-Flags              */
  unsigned        ip_len;       /* Ip-Header Laenge                     */
  unsigned        mtu;          /* mtu fuer das Interface bei tx        */

#ifdef KERNELIF
  /* die eingeschleusten Frames vom Kernel vorbeimogeln */
  if (mbhd->l2port != KERNEL_PORT)
  {
#endif
    if (   (mbhd->l2fflg != L2CIP)
        || (!IPpar[mbhd->l2port].ipMode & IP_FORWARDING))
      return;
#ifdef KERNELIF
  }
#endif
  rxnxt = mbhd->mbbp;                   /* Anfang des Headers merken    */
  ip_len = getchr(mbhd); /* Laenge und Version lesen     */
  ip.version = (ip_len >> 4) & 0x0f;    /* Versionsnummer               */
  ip_len = (ip_len & 0x0f) << 2;        /* und Laenge in Bytes          */
  mbhd->mbbp = rxnxt;                   /* zurueck an den Anfang        */
  if (   (mbhd->mbpc - (--mbhd->mbgc)) < IPLEN
      || (ip_len < IPLEN)
      || (ip.version != IPVERSION)
#ifdef __WIN32__
      || cksum(NULLHEADER, mbhd, (unsigned short)ip_len) != 0)
#else
      || cksum(NULLHEADER, mbhd, ip_len) != 0)
#endif /* WIN32 */
  {
    ipInHdrErrors++;
    return;
  }

  /* jetzt wird der IP-Header ausgewertet */
  getchr(mbhd);                                 /* skip length/version  */
  ip.tos = getchr(mbhd);
  ip.length = get16(mbhd);
  ip.id = get16(mbhd);
  ip.offset = get16(mbhd);
  ip.flags.mf = (ip.offset & 0x2000) ? 1 : 0;
  ip.flags.df = (ip.offset & 0x4000) ? 1 : 0;
  ip.offset = (ip.offset & 0x1fff) << 3;
  ip.ttl = getchr(mbhd);
  ip.protocol = getchr(mbhd);
  ip.checksum = get16(mbhd);
  ip.source = get32(mbhd);
  ip.dest = get32(mbhd);

  /* ist das Ziel eine Broadcast-Addresse ? */
  rxbroadcast = is_broadcast_address(ip.dest);

  /* Optionsfeld auswerten */
  if ((ip.optlen = ip_len - IPLEN) != 0)
    for (i = 0; i < ip.optlen; i++)
      ip.options[i] = getchr(mbhd);
  length = ip.length - ip_len;

  /* wenn das Frame direkt an uns gerichtet ist und wir keinen Speicher */
  /* mehr haben, muessen wir bescheid sagen                             */
  if (!rxbroadcast && nmbfre < 256)
    icmp_output(&ip, mbhd, ICMP_QUENCH, 0, NULLICMP);

  /* Optionsfelder auswerten */
  strict = 0;
  for (opt = ip.options; opt < &ip.options[ip.optlen]; opt += opt_len)
  {
    opt_len = opt[1];
    if ((opt[0] & OPT_NUMBER) == IP_EOL)
      break;

    switch (opt[0] & OPT_NUMBER)
    {
      case IP_NOOP:
        opt_len = 1;
        break;

      case IP_SSROUTE:
        strict = 1;

      case IP_LSROUTE:
        if (!is_my_ip_addr(ip.dest))
          break;
        if (opt[2] >= opt_len)
          break;
        ptr = opt + opt[2] - 1;

        memcpy(&ip.dest, ptr, 4);
        memcpy(ptr, &my_ip_addr, 4);

        opt[2] += 4;
        break;
      case IP_RROUTE:
        if (opt[2] >= opt_len)
        {
          if (!rxbroadcast)
          {
            union icmp_args icmp_args;

            icmp_args.pointer = IPLEN + opt - ip.options;
            icmp_output(&ip, mbhd, ICMP_PARAM_PROB, 0, &icmp_args);
          }
          return;
        }
        ptr = opt + opt[2] - 1;
        memcpy(ptr, &my_ip_addr, 4);
        opt += 4;
        break;
    }
  }

#ifdef TCP_STACK
  /* Protokoll TCP, */
  if (  (ip.protocol == TCP_PTCL)
      /* und an mich. */
      &&(is_my_ip_addr(ip.dest)))
  {
    /* An TCP-Stack weiterleiten. */
    TCPIPProcess(&ip, mbhd);
    return;
  }
#endif /* TCP_STACK */

  if ((is_my_ip_addr(ip.dest)) || rxbroadcast)
  {
    if (   !rxbroadcast && !ip.flags.mf && ip.offset == 0
        && ip.protocol == ICMP_PTCL)
    {
      icmp_input(&ip, mbhd);
      ipInReceives++;
      return;
    }
    else
    {
      if (!rxbroadcast)
        icmp_output(&ip, mbhd, ICMP_DEST_UNREACH,
                    ICMP_PROT_UNREACH, NULLICMP);
      ipInUnknownProtos++;
      return;
    }
  }

  ipForwDatagrams++;

  /* Wenn die Lifetime des Frames abgelaufen ist, Frame vernichten und  */
  /* Sender benachrichtigen                                             */
  if (--ip.ttl == 0)
  {
    icmp_output(&ip, mbhd, ICMP_TIME_EXCEED, 0, NULLICMP);
    ipInHdrErrors++;
    return;
  }

  /* Einen passenden Eintrag in der routing-Tabelle finden, wenn nichts */
  /* passt, einen NetSearch durchfuehren (zZ noch nicht implementiert), */
  /* wenn wir dort auch nichts finden, * wird das Frame vernichtet.     */
#ifndef IPROUTEMOD
  if ((rp = rt_find(ip.dest)) == NULLROUTE)
  {
    icmp_output(&ip, mbhd, ICMP_DEST_UNREACH,
                ICMP_HOST_UNREACH, NULLICMP);
    ipOutNoRoutes++;
    return;
  }
#else /* IPROUTEMOD */
  if ((rp = rt_find(ip.dest)) == NULLROUTE)
  {                                       /* Auf default-route pruefen. */
    if ((rp = rt_find(my_default_ip_addr)) == NULLROUTE)
    {
      icmp_output(&ip, mbhd, ICMP_DEST_UNREACH,
                  ICMP_HOST_UNREACH, NULLICMP);
      ipOutNoRoutes++;
      return;
    }
  }
#endif /* IPROUTEMOD */

  /* Den naechsten IP-Router berechnen, an den wir das Frame schicken   */
  /* (z.B. das Gateway). Wenn der Eintrag kein Gateway aufweist, dann   */
  /* die Zieladdresse nehmen. Wenn das Gateway nicht das Ziel ist, aber */
  /* strictes Routen gefordert wurde, denn Fehler melden !              */
  gateway = rp->gateway == 0L ? ip.dest : rp->gateway;
  if (strict && gateway != ip.dest)
  {
    icmp_output(&ip, mbhd, ICMP_DEST_UNREACH,
                ICMP_ROUTE_FAIL, NULLICMP);
    ipOutNoRoutes++;
    return;
  }

  /* Maximale Blockgroese lesen, auf Segmentation pruefen Bei NET/ROM   */
  /* wird immer eine MTU von 236 angenommen, da NET/ROM selber einen    */
  /* Segmenter enthaelt, wird dieser notfalls zuschlagen.               */
#ifndef KERNELIF
  mtu = (rp->port == NETROM_PORT) ? 236 : portpar[rp->port].mtu;
#else
  switch(rp->port)
  {
    case NETROM_PORT: mtu = 236;
                      break;
    /* Fuer den Kernel eine Extrawurst */
    case KERNEL_PORT: mtu = 1500;
                      break;

    default: mtu = portpar[rp->port].mtu;
  }
#endif

  /* Frame ist klein genug, dass es ohne Fragmentierung geroutet werden kann */
  /* Alle Frames vom Kernel sollten hier lang gehen */
  if (ip.length <= mtu)
  {
    if ((tbp = htonip(&ip, mbhd, 0)) == NULL)
      return;

    send_ip_mb(tbp, rp->port, gateway, ip.tos);
    return;
  }

  /* Das Frame ist zu gross, duerfen wir es zerlegen ? */
  if (ip.flags.df && rp->port == NETROM)  /* nein, Fehler melden */
  {
    union icmp_args icmp_args;

    icmp_args.mtu = mtu;
    icmp_output(&ip, mbhd, ICMP_DEST_UNREACH,
                ICMP_FRAG_NEEDED, &icmp_args);
    ipFragFails++;
    return;
  }

  /* Das Frame bei NETROM aufspalten */
  if (rp->port == NETROM)
  {
    offset = ip.offset;
    mf_flag = ip.flags.mf;
    while (length != 0)
    {
      unsigned        fragsize;

      ip.offset = offset;
      if (length + ip_len <= mtu)
      {
        fragsize = length;
        ip.flags.mf = mf_flag;
      }
      else
      {
        fragsize = (mtu - ip_len) & 0xfff8;
        ip.flags.mf = 1;
      }
      ip.length = fragsize + ip_len;
      if ((tbp = htonip(&ip, NULLBUF, 0)) == NULL)
        return;

      for (i = fragsize; i != 0; i--)
        putchr(getchr(mbhd), tbp);
      send_ip_mb(tbp, rp->port, gateway, ip.tos);
      ipFragCreates++;
      offset += fragsize;
      length -= fragsize;
    }
  }

/* Frame ist zu gross, aber wir nehmen eine AX25- Fragmentierung vor    */
  else
  {
    if ((tbp = htonip(&ip, mbhd, 0)) == NULL)
      return;

    send_ip_mb(tbp, rp->port, gateway, ip.tos);
  }
  ipFragOKs++;
  return;
}

/************************************************************************
 * Function : In der Routing-Tabelle den Weg zu einem Ziel suchen
 *
 * Inputs   : IP addresse
 *
 * Returns  : Entweder zeiger auf den Tabelleneintrag oder NULL
 *
 * Operation: Erst schauen ob der Cache-Eintrag passt, ansonsten wird die
 *            Tabelle in absteigender Reihenfolge der Addressbits, wenn
 *            einer passt, gehts los
 *----------------------------------------------------------------------*/
IP_ROUTE *
rt_find(register ipaddr target)
{
  register IP_ROUTE *iprp;
  ipaddr             temp = target;
  register unsigned  bits = 32;

  /* Routentabelle durchgehen, passenden Eintrag suchen                */
  for (iprp  = (IP_ROUTE *)IP_Routes.head;
       iprp != (IP_ROUTE *)&IP_Routes;
       iprp  = (IP_ROUTE *)iprp->nextip)
  {
    if (bits > iprp->bits)
      temp &= ~0L << (32 - (bits = iprp->bits));

    /* Route gefunden */
    if (iprp->dest == temp)
      return (iprp);
  }

  return (NULLROUTE);
}

/************************************************************************
 * Function : den IP-Header aufbauen und in ein Buffer speichern
 *
 * Inputs   : Zeiger auf den Header, den Buffer, Daten & Ckecksum Flag
 *
 * Returns  : Zeiger auf den neuen Buffer im Netzwerk-format
 *
 * Operation: einen neuen Buffer erstellen, den Header abspeichern,
 *            und wenn data nicht NULL ist, die Daten reinspeichern
 *----------------------------------------------------------------------*/
MBHEAD *
htonip(register IP * iphdr, MBHEAD *data, BOOLEAN cflag)
{
  unsigned        hdr_len;
  register unsigned i;
  register MBHEAD *bufpoi;
  unsigned        fl_offs;
  unsigned        checksum;
  char huge      *ptr,
                  huge * cksum_ptr;

  hdr_len = IPLEN + iphdr->optlen;
  if ((bufpoi = ((MBHEAD *)allocb(ALLOC_MBHEAD))) == NULL)
    return(NULL);

#ifdef __WIN32__
  putchr((char)((IPVERSION << 4) | (hdr_len >> 2)), bufpoi);
#else
  putchr((IPVERSION << 4) | (hdr_len >> 2), bufpoi);
#endif /* WIN32 */
  putchr(iphdr->tos, bufpoi);
#ifdef __WIN32__
  put16((unsigned short)iphdr->length, bufpoi);
  put16((unsigned short)iphdr->id, bufpoi);
#else
  put16(iphdr->length, bufpoi);
  put16(iphdr->id, bufpoi);
#endif /* WIN32 */
  fl_offs = iphdr->offset >> 3;
  if (iphdr->flags.df)
    fl_offs |= 0x4000;
  if (iphdr->flags.mf)
    fl_offs |= 0x2000;
#ifdef __WIN32__
  put16((unsigned short)fl_offs, bufpoi);
#else
  put16(fl_offs, bufpoi);
#endif /* WIN32 */
  putchr(iphdr->ttl, bufpoi);
  putchr(iphdr->protocol, bufpoi);
  cksum_ptr = bufpoi->mbbp;
  put16(0, bufpoi);
  put32(iphdr->source, bufpoi);
  put32(iphdr->dest, bufpoi);
  for (i = 0; i < iphdr->optlen; i++)
    putchr(iphdr->options[i], bufpoi);
  ptr = bufpoi->mbbp;
  i = bufpoi->mbpc;
  rwndmb(bufpoi);
#ifdef __WIN32__
  checksum = cflag ? iphdr->checksum : cksum(NULLHEADER, bufpoi, (unsigned short)hdr_len);
#else
  checksum = cflag ? iphdr->checksum : cksum(NULLHEADER, bufpoi, hdr_len);
#endif /* WIN32 */
  bufpoi->mbbp = cksum_ptr;
  bufpoi->mbpc = 10;
#ifdef __WIN32__
  put16((unsigned short)checksum, bufpoi);
#else
  put16(checksum, bufpoi);
#endif /* WIN32 */
  bufpoi->mbbp = ptr;
  bufpoi->mbpc = i;
  if (data != NULL)
  {
    i = data->mbpc - data->mbgc;
    while (i--)
      putchr(getchr(data), bufpoi);
  }
  return (bufpoi);
}

/* ***********************************************************************
 * Function : Ein IP datagramm senden
 *            Modelled after the example interface on p 32 of RFC 791
 *
 * Inputs   :
 *
 * Returns  :
 *
 * Operation: Netzwerk-Header erzeugen, Daten anfuegen und so tun, als ob
 *            wir das Frame empfangen haetten
 * ---------------------------------------------------------------------*/
int
ip_send(ipaddr          source,   /* source address                     */
        ipaddr          dest,     /* Destination address                */
        unsigned        protocol, /* Protocol                           */
        unsigned        tos,      /* Type of service                    */
        unsigned        ttl,      /* Time-to-live                       */
        MBHEAD         *bp,       /* Data portion of datagram           */
        unsigned short  length,   /* Optional length of data portion    */
        unsigned short  id,       /* Optional identification            */
        unsigned        df)       /* Don't-fragment flag                */
{
  register MBHEAD       *tbp;
  IP                     ip;            /* Pointer to IP header         */
  register IP           *ipptr = &ip;
  static unsigned short  id_cntr;       /* Datagram serial number       */

  ipOutRequests++;

  if (length == 0 && bp != NULL)
    length = bp->mbpc - bp->mbgc;

  /* Fill in IP header */
  ipptr->tos = tos;
  ipptr->length = IPLEN + length;
  ipptr->id = (id == 0) ? id_cntr++ : id;
  ipptr->offset = 0;
  ipptr->flags.mf = 0;
  ipptr->flags.df = df;
  ipptr->ttl = (ttl == 0) ? ipDefaultTTL : ttl;
  ipptr->protocol = protocol;
  ipptr->source = source;
  ipptr->dest = dest;
  ipptr->optlen = 0;
  if ((tbp = htonip(&ip, bp, 0)) == NULL)
  {
    dealmb(bp);
    return(1);
  }
  dealmb(bp);
  tbp->l2fflg = L2CIP;
  rwndmb(tbp);
  tbp->l2port = NETROM_PORT;    /* der Port ist immer eingeschaltet */
  relink((LEHEAD *)tbp, (LEHEAD *)iprxfl.tail);
  return (0);
}

/************************************************************************
 * Function : Ip Router Interface zu NET/ROM
 *
 * Inputs   : Frame zum senden, die Gateway-Addresse, tos-flag
 *
 * Returns  : nix
 *
 * Operation: Das Frame wird nach der arp-Tabelle addressiert und in die
 *            L3-Sendeliste gehaengt
 *----------------------------------------------------------------------*/
void
nr_iface(MBHEAD *mhbp, ipaddr gateway)
{
  register unsigned i;
  register MBHEAD *mb;
  register ARP_TAB *arp;
  NODE           *np;

  /* den passenden ARP-Eintrag suchen und pruefen, ob der Node bekannt  */
  /* und erreichbar ist                                                 */
  if (   (arp = res_arp(gateway, ARP_NETROM)) != NULLARP
      && (iscall(arp->callsign, &np, NULL, DG)))
  {
    l4pidx = l4pcid = NR_PROTO_IP;      /* NETROM IP Family             */
    l4ahd2 = l4ahd3 = 0;                /* unbenutzt ...                */
    l4aopc = NR4_OP_PID;                /* NET/ROM Opcode fuer IP       */
    mb = gennhd();                      /* L3/L4 Header erzeugen        */
    rwndmb(mhbp);                       /* Zurueckspulen zum senden     */
    i = mhbp->mbpc - mhbp->mbgc;        /* Daten kopieren               */
    while (i--)
      putchr(getchr(mhbp), mb);
    mb->l2link = (LNKBLK *)np;          /* Zielnode setzen              */
    mb->l4type = L4TNORMAL;             /* Prioritaet setzen            */
    relink((LEHEAD *)mb, (LEHEAD *)l3txl.tail); /* nur umhaengen        */
  }
  dealmb(mhbp);
}

/************************************************************************
 * Function : AX.25 Handler fuer den ip router
 *
 * Inputs   : wie oben fuer NET/ROM
 *
 * Returns  : nothing
 *
 * Operation: Feststellen, ob dg oder vc, dann Frame senden
 *----------------------------------------------------------------------*/
void
l2_iface(MBHEAD *mhbp, unsigned port, ipaddr gateway, unsigned tos)
{
  register ARP_TAB *arp;
  PTCENT           *ptcp;
#ifdef EAX25
  MHEARD           *mhp = NULL;
#endif


  /* IP-Pid setzen */
  rwndmb(mhbp);
  mhbp->l2fflg = L2CIP;

  /* find the arp entry. Die if none ! */
  if ((arp = res_arp(gateway, ARP_AX25)) == NULLARP)
  {
    arp_request(gateway, ARP_AX25, port);
    dealmb(mhbp);
  }

  /* Now check to see if we are going to DG or VC it */
  else
    if (   (arp->dgmode & 1)
        || (   (arp->dgmode == 0)
            && (   tos & DELAY
                || (   !(tos & RELIABILITY)
                 /* && (ipPar[port].ipMode & DG_OK) */
       ))))
  {
#ifdef __WIN32__
    sdui(arp->digi, arp->callsign, myid, (char)port, mhbp);
#else
    sdui(arp->digi, arp->callsign, myid, port, mhbp);
#endif /* WIN32 */
    dealmb(mhbp);
  }
  else
    /* Virtual Circuit */
  {
#ifdef __WIN32__
    lnkpoi = getlnk((unsigned char)port, myid, arp->callsign, arp->digi);
#else
    lnkpoi = getlnk(port, myid, arp->callsign, arp->digi);
#endif /* WIN32 */
    if (lnkpoi)
    {
      if (lnkpoi->state == L2SDSCED)
      {
        ptcp = ptctab + g_uid(lnkpoi, L2_USER);
        ptcp->state = D_IPLINK;
        ptcp->ublk = NULL;
        ptcp->p_uid = NO_UID;
#ifdef EAX25
        /* TEST DG9OBU */
        /* Call in L2-MHeard suchen */
        if ((mhp = mh_lookup(&l2heard, arp->callsign)) != NULL)
        {
          if (   (mhp->eax_link == TRUE) /* mit EAX.25 gehoert */
              && (portpar[mhp->port].eax_behaviour >= 1) /* und auf Port erlaubt */
             )
          {
            lnkpoi->bitmask = 0x7F;
#ifdef __WIN32__
            lnkpoi->maxframe = (unsigned char)portpar[lnkpoi->liport].maxframe_eax;
#else
            lnkpoi->maxframe = portpar[lnkpoi->liport].maxframe_eax;
#endif /* WIN32 */
          }
        }
#endif

        newlnk();               /* eventuell erst aufbauen          */
      }
      rwndmb(mhbp);
      i3tolnk(mhbp->l2fflg, lnkpoi, mhbp);  /* -> ab in den Link    */
    }
    else
      dealmb(mhbp);
  }
}


/************************************************************************
 * Function : Reverse ARP fuer eine Addresse/Hardwaretyp durchfuehren
 *
 * Inputs   : Pointer to internet address and a hardware ( arp ) type
 *
 * Returns  : Pointer to entry in arp table or a null pointer
 *
 * Operation: Die Tabelle linear durchsuchen ob ein Eintrag passt
 *----------------------------------------------------------------------*/
ARP_TAB *
res_arp(register ipaddr target, register unsigned hwtype)
{
  register ARP_TAB *arprp;

  for (arprp = (ARP_TAB *) Arp_tab.head;
       arprp != (ARP_TAB *) & Arp_tab;
       arprp = (ARP_TAB *) arprp->nextar)
#ifdef __WIN32__
    if (   (char)hwtype == arprp->hwtype
#else
    if (   hwtype == arprp->hwtype
#endif /* WIN32 */
        && arprp->dest == target)
      return (arprp);
  return (NULLARP);
}

/************************************************************************
 * Function : Eine ICMP-Antwort an den Absender des Frames zurueck
 *            schicken, der Aufrufer gibt das Frame frei!
 *
 * Inputs   :
 *
 * Returns  : nix
 *
 * Operation: Did you ever hear the one about the vampire rabbit ?
 *            Note lots of standard icmp bits are commented out.
 *----------------------------------------------------------------------*/
void
icmp_output(IP              *ip,        /* Header of offending datagram */
            register MBHEAD *data,      /* Data portion of datagram     */
            unsigned         type,      /* Codes to send                */
            unsigned         code,
            union icmp_args *args)
{
  MBHEAD                  *bp,
                          *bp2;
  ICMP                     icmp;        /* ICMP protocol header         */
  unsigned short           length;      /* Total length of reply        */
  register unsigned short  i;
  char huge               *rxnxt;

  if ((unsigned char)ip->protocol == ICMP_PTCL)
  {
    /* Den Header durchsuchen, ob es sicher ist, eine ICMP Nachricht    */
    /* zurueckzuschicken                                                */
    rxnxt = data->mbbp;
    i = getchr(data) & 0xff;
    data->mbbp = rxnxt;
    data->mbgc--;
    switch (i)
    {
      case ICMP_ECHO_REPLY:
      case ICMP_ECHO:
      case ICMP_TIMESTAMP:
      case ICMP_TIME_REPLY:
      case ICMP_INFO_RQST:
      case ICMP_INFO_REPLY:
        break;                /* These are all safe */
      default:
        /* Auf eine Fehlermeldung keine Fehlermeldung senden (gibt      */
        /* sonst einen netten Ping-Pong)                                */
        return;
    }
  }

  /* Berechnen, wieviel wir vom Frame zurueckschicken wollen, das sind  */
  /* maximal der fremde Header und 8 Bytes                              */
  i = data->mbpc - data->mbgc;
  if (i > 8)
    i = 8;
  length = i + ICMPLEN + IPLEN + ip->optlen;

  /* Den Header neu aufbauen */
  if ((bp = htonip(ip, NULLBUF, 1)) == NULL)
    return;

  /* Die Antwort-Daten hineinkopieren */
  while (i--)
    putchr(getchr(data), bp);
  icmp.type = type;
  icmp.code = code;
  icmp.args.unused = 0;

  switch (type)
  {
/*  case ICMP_ECHO:
      icmpOutEchos++;
      break;

    case ICMP_ECHO_REPLY:
      icmpOutEchoReps++;
      break;

    case ICMP_INFO_RQST:
      break;

    case ICMP_INFO_REPLY:
      break;

    case ICMP_TIMESTAMP:
      icmpOutTimestamps++;
      break;

    case ICMP_ADDR_MASK:
      icmpOutAddrMasks++;
      break;

    case ICMP_ADDR_MASK_REPLY:
      icmpOutAddrMaskReps++;
      break;

    case ICMP_TIME_EXCEED:
      icmpOutTimeExcds++;
      break;

    case ICMP_QUENCH:
      icmpOutSrcQuenchs++;
      break;
*/
    case ICMP_PARAM_PROB:
/*    icmpOutParmProbs++;
*/    icmp.args.pointer = args->pointer;
      break;

    case ICMP_REDIRECT:
/*    icmpOutRedirects++;
*/    icmp.args.address = args->address;
      break;

    case ICMP_TIME_REPLY:
/*    icmpOutTimestampReps++;
*/    icmp.args.echo.id = args->echo.id;
      icmp.args.echo.seq = args->echo.seq;
      break;

    case ICMP_DEST_UNREACH:
      if (icmp.code == ICMP_FRAG_NEEDED)
        icmp.args.mtu = args->mtu;
/*    icmpOutDestUnreachs++;
*/    break;
  }

  /* Jetzt generieren wir das eigentliche ICMP-Frame */
  if ((bp2 = htonicmp(&icmp, bp)) == NULL)
  {
    dealmb(bp);
    return;
  }
  dealmb(bp);

  ip_send(my_ip_addr, ip->source, ICMP_PTCL, ip->tos,
          0, bp2, length, 0, 0);
}

/************************************************************************
 * Function : Einen ICMP Echo Request erzeugen und absenden
 *
 * Inputs   : Zieladdresse, Sequenznummer und id, ein optionales
 *            Laengenfeld (maximal 4 Bytes) zur freien Verwendung
 *
 * Returns  : int - TRUE wenn das Frame erzeugt wurde
 *
 * Operation: Ein Frame anlegen und an die Zieladdresse schicken
 *----------------------------------------------------------------------*/
BOOLEAN
pingem(ipaddr target, int seq, int id, int len, char *opt)
{
  MBHEAD         *bp,
                 *data;
  ICMP            icmp;         /* ICMP protocol header                 */

  /* Pruefen, ob Ziel irgendwie erreichbar ist */
  if (rt_find(target) == NULLROUTE)
    return (FALSE);

  if ((data = ((MBHEAD *)allocb(ALLOC_MBHEAD))) == NULL)
    return(FALSE);

  put32(tic10, data);           /* aktuelle Uhrzeit hineinschreiben     */
  put32((ULONG)userpo, data);   /* und den User                         */
  while (len--)
    putchr(*opt++, data);       /* Optionsfeld schreiben                */

  /* icmpOutEchos++; icmpOutMsgs++; */

  icmp.type = ICMP_ECHO;
  icmp.code = 0;
  icmp.args.echo.seq = seq;
  icmp.args.echo.id = id;

  if ((bp = (htonicmp(&icmp, data))) == NULL)
  {
    dealmb(data);
    return(FALSE);
  }

  dealmb(data);
  if (ip_send(my_ip_addr, target, ICMP_PTCL, LOW_DELAY, 0, bp, 0, 0, 0) == TRUE)
    return(FALSE);

  return (TRUE);
}

/************************************************************************
 * Function : Einen ICMP-Header erzeugen, Daten hineinkopieren
 *
 * Inputs   : Zeiger auf den ICMP-Header und Daten
 *
 * Returns  : Zeiger auf das fertige Frame
 *
 * Operation: Well, there was this traveller in Transylvania,
 *----------------------------------------------------------------------*/
MBHEAD *
htonicmp(register ICMP * icmp, MBHEAD *data)
{
  register MBHEAD *bp;
  unsigned short  checksum;
  register unsigned putcnt;
  /* char huge *nxtchr; */
  char huge      *cksum_ptr;

  if ((bp = ((MBHEAD *)allocb(ALLOC_MBHEAD))) == NULL)
    return(NULL);

  putchr(icmp->type, bp);
  putchr(icmp->code, bp);
  cksum_ptr = bp->mbbp;
  put16(0, bp);                 /* Clear checksum */

  switch (icmp->type)
  {
    case ICMP_DEST_UNREACH:
      put16(0, bp);
      if (icmp->code == ICMP_FRAG_NEEDED)  /* Deering/Mogul max MTU
                                              indication */
#ifdef __WIN32__
        put16((unsigned short)icmp->args.mtu, bp);
#else
        put16(icmp->args.mtu, bp);
#endif /* WIN32 */
      else
        put16(0, bp);
      break;

    case ICMP_PARAM_PROB:
      putchr(icmp->args.pointer, bp);
      putchr(0, bp);
      put16(0, bp);
      break;

    case ICMP_REDIRECT:
      put32(icmp->args.address, bp);
      break;

    case ICMP_ECHO:
    case ICMP_ECHO_REPLY:
    case ICMP_TIMESTAMP:
    case ICMP_TIME_REPLY:
    case ICMP_INFO_RQST:
    case ICMP_INFO_REPLY:
#ifdef __WIN32__
      put16((unsigned short)icmp->args.echo.id, bp);
      put16((unsigned short)icmp->args.echo.seq, bp);
#else
      put16(icmp->args.echo.id, bp);
      put16(icmp->args.echo.seq, bp);
#endif /* WIN32 */
      break;

    default:
      put16(0, bp);
      put16(0, bp);
      break;
  }
  rwndmb(data);
  putcnt = data->mbpc;
  while (putcnt--)
    putchr(getchr(data), bp);

  /* Compute checksum, and stash result */
  putcnt = bp->mbpc;
  rwndmb(bp);
  checksum = cksum(NULLHEADER, bp, bp->mbpc);
  bp->mbbp = cksum_ptr;
  bp->mbpc = 2;
  put16(checksum, bp);
  bp->mbpc = putcnt;
  rwndmb(bp);
  return (bp);
}

/* ***********************************************************************
 * Function : Behandlung von eingehenden ICMP-Frames
 *
 * Inputs   : Dekodierter IP-Header und Framedaten
 *
 * Returns  : nix
 *
 * Operation: Nur eine sehr einfache ICMP-Behandlung durchfuehren, um
 *            PING-Anfragen zu beantworten.
 * ---------------------------------------------------------------------*/
void
icmp_input(register IP * ip, register MBHEAD *bp)
{
  MBHEAD         *tbp;
  register MBHEAD *bp2;
  ICMP            icmp;
  unsigned        length,
                  i;
  ULONG           ping_tic;
  USRBLK         *ping_user;
  char            buf[50];
  UWORD           adr[4];

  length = ip->length - IPLEN - ip->optlen;
#ifdef __WIN32__
  if (cksum(NULLHEADER, bp, (unsigned short)length) != 0)
#else
  if (cksum(NULLHEADER, bp, length) != 0)
#endif /* WIN32 */
    return;
  if (bp->mbpc - bp->mbgc < 8)
    return;
  icmp.type = getchr(bp);
  icmp.code = getchr(bp);

  /* fremde Pings beantworten */
  if (icmp.type == ICMP_ECHO)
  {
    get16(bp);
    icmp.args.echo.id = get16(bp);
    icmp.args.echo.seq = get16(bp);
    icmp.type = ICMP_ECHO_REPLY;
    bp2 = (MBHEAD *)allocb(ALLOC_MBHEAD);
    if (length > 8)
    {
      i = length - 8;
      while (i--)
        putchr(getchr(bp), bp2);
    }
    if ((tbp = htonicmp(&icmp, bp2)) == NULL)
    {
      dealmb(bp2);
      return;
    }
    dealmb(bp2);
    ip_send(ip->dest, ip->source, ICMP_PTCL,
#ifdef __WIN32__
            ip->tos, 0, tbp, (unsigned char)length, 0, 0);
#else
            ip->tos, 0, tbp, length, 0, 0);
#endif /* WIN32 */
    return;
  }

  /* eigene Pings dem User ausgeben */
  if (icmp.type == ICMP_ECHO_REPLY)
  {
    get16(bp);
    get16(bp);                  /* id */
    get16(bp);                  /* seq */
    ping_tic = get32(bp);
    ping_user = (void *)get32(bp);

    ping_tic = (tic10 - ping_tic) * 10;
    if (ping_tic <= 10)
      ping_tic = 10;

    for (i = 0; i < 4; i++)
#ifdef __WIN32__
      adr[i] = (unsigned short)(ip->source >> 8 * i) & 0xff;
#else
      adr[i] = (ip->source >> 8 * i) & 0xff;
#endif /* WIN32 */
    sprintf(buf, "%d.%d.%d.%d rtt = %lums, ttl = %u",
            adr[3], adr[2], adr[1], adr[0], ping_tic, ip->ttl);
#ifdef SEND_ASYNC_RESFIX
    /* Sicher ist sicher. */
    buf[256] = 0;
#endif /* SEND_ASYNC_RESFIX */
    send_async_response(ping_user, "Ping response from ", buf);
  }
}

/************************************************************************
 * Function : Ein "end-around-carry adjustment" durchfuehren
 *
 * Inputs   : aktuelle Checksumme
 *
 * Returns  : korrigierte Pruefsumme
 *
 * Operation:
 *
 *----------------------------------------------------------------------*/
unsigned short
eac(long sum)
{
  register unsigned short csum;

#ifdef __WIN32__
  sum = csum = (sum >> 16) + (sum & 0xffff);
#else
  while ((csum = sum >> 16) != 0)
    sum = csum + (sum & 0xffffL);
#endif /* WIN32 */
  return ((unsigned short)(sum & 0xffffL));  /* nur 16 bits */
}

/* ***********************************************************************
 * Function : Checksum an mhtyp, with optional pseudo-header
 *
 * Inputs   : pseudo header, buffer to checksum and byte count
 *
 * Returns  : unsigned integer checksum with eac corrected
 *
 * Operation: add up all bytes in a long, in perverse arpa way then eac
 * ---------------------------------------------------------------------*/
unsigned short
cksum(register PSEUDO_HEADER * ph, register MBHEAD *m,
      register unsigned short len)
{
  long            sum;
  unsigned        rxcnt;
  char huge      *rxchr;

  sum = 0L;

  /* Sum pseudo-header, if present */
  if (ph != NULLHEADER)
  {
#ifndef L1TCPIP
    sum = ((unsigned *)(&ph->source))[0];
    sum += ((unsigned *)(&ph->source))[1];
    sum += ((unsigned *)(&ph->dest))[0];
    sum += ((unsigned *)(&ph->dest))[1];
    sum += ((unsigned)ph->protocol & 0xff);
    sum += ((unsigned)ph->length);
#else
    sum +=  (ph->source  & 0xffffUL);
    sum += ((ph->source >> 16) & 0xffffUL);
    sum +=  (ph->dest & 0xffffUL);
    sum += ((ph->dest >> 16) & 0xffffUL);
    sum +=  (ph->protocol & 0xff);
    sum +=  (ph->length);
#endif /* L1TCPIP */
  }

  rxcnt = m->mbgc;
  rxchr = m->mbbp;
  while (len > 0)
  {
    if (len > 1)
    {
      sum += (unsigned)get16(m);
      len -= 2;
    }
    else
    {
#ifndef L1TCPIP
      sum += ((unsigned)getchr(m) & 0xFF00);
#else
      sum += ((getchr(m) << 8) & 0xFF00);
#endif /* L1TCPIP */
      len--;
    }
  }
  m->mbgc = rxcnt;
  m->mbbp = rxchr;
  return ((unsigned short)(~eac(sum) & 0xffff));
}

/* **********************************************************************
 * Function : ARP service zur Beantwortung von ARP-Requests und
 *            die Auswertung der Antworten
 *
 * Inputs   : Zeiger auf das ARP-Frame
 *
 * Returns  : nix
 *
 * Outputs  : UI Frame mit dem Ergebnis der Bearbeitung
 *
 * Operation: Wenn nach uns gefragt wurde, oder nach einem Eintrag, der
 *            als published gekennzeichnet ist, dann beantworten
 * ---------------------------------------------------------------------*/
void
arp_service(MBHEAD *mhbp)
{
  unsigned char   arp_not_revarp;
  /* char *ptr; */
  register MBHEAD *mbhd = mhbp;
  register ARP_TAB *ap;
  register unsigned i;
  ARP_TAB        *atp;
  IP_ROUTE       *ipr;

  /* ARP auf dem Port erlaubt ? */
  if (!(IPpar[mbhd->l2port].ipMode & ARP_OK))
  {
    return;
  }

  /* Hardware korrekt ? */
  if ((arp.hardware = get16(mbhd)) != ARP_AX25)
  {
/*        arp_stat.badtype++; */
    return;
  }

  /* Protokoll ok ? */
  if ((arp.protocol = get16(mbhd)) != L2CIP)
  {
/*        arp_stat.badtype++; */
    return;
  }

  arp.hwalen = getchr(mbhd);
  arp.pralen = getchr(mbhd);

  /* Laenge(n) ok ? */
  if (arp.hwalen > MAXHWALEN || arp.pralen != sizeof(ipaddr))
  {
/*        arp_stat.badlen++; */
    return;
  }

  /* Opcode */
  arp.opcode = get16(mbhd);

  /* Quell- und Ziel-Adressen (jeweils Call und IP-Adress) */
  getfid((char *)arp.shwaddr, mbhd);
  arp.sprotaddr = get32(mbhd);
  getfid((char *)arp.thwaddr, mbhd);
  arp.tprotaddr = get32(mbhd);

  /* Frame an "QST ? */
  if (cmpid((char *)arp.shwaddr, QST))
  {
/*        arp_stat.badaddr++; */
    return;
  }

  ap = res_arp(arp.sprotaddr, arp.hardware);

  /* IP-Adresse anhand des ARP-Requests lernen */
  if (   (((i = is_my_ip_addr(arp.tprotaddr)) != 0) && ap == NULLARP)
      || (ap != NULLARP && ap->timer != 0))
  {
    /* ARP-Eintrag machen (mit Timeout) */
    arp_add(arp.sprotaddr, mbhd->l2port, (char *)arp.shwaddr, "", 2, ARPtimer, 1, FALSE);

    /* Falls der Weg zum neu eingetragenen Host nicht oder von einer          */
    /* Route erfasst wird, die die Frames auf einem anderen Port als dem      */
    /* Port auf dem das ARP empfangen wurde aussenden wuerde, dann eine       */
    /* explizite /32 Route zu diesem Host auf dem ARP-Empfangsport eintragen. */
    ipr = rt_find((ipaddr)arp.sprotaddr);

    if (   (ipr == NULLROUTE)
        || (   (ipr != NULLROUTE)
            && (ipr->port != mbhd->l2port)
           )
       )
      rt_add(arp.sprotaddr, 32, 0L, mbhd->l2port, 0, 0, 0, TRUE);
  }

  if (   arp.opcode == REVARP_REQUEST
      || (   arp.opcode == ARP_REQUEST
          && (   (i /* = is_my_ip_addr(&arp.tprotaddr) */ )
              || (   (ap = res_arp(arp.tprotaddr, arp.hardware)) != NULLARP
                  && ap->publish_flag))))
  {
    arp_not_revarp = (arp.opcode == ARP_REQUEST);
    if (!arp_not_revarp)
      for (atp = (ARP_TAB *) Arp_tab.head;
           atp != (ARP_TAB *) & Arp_tab;
           atp = (ARP_TAB *) atp->nextar)
        if ((i = cmpid(atp->callsign, (char *)arp.thwaddr)) != 0)
          break;
    if (arp_not_revarp || (i && ap->publish_flag))
    {
      if (arp_not_revarp)
      {
        cpyid((char *)arp.thwaddr, (char *)arp.shwaddr);
        /* if (arp.hardware == ARP_AX25) */
        arp.thwaddr[arp.hwalen - 1] |= 1;
      }

      cpyid((char *)arp.shwaddr, i ? myid : ap->callsign);
      arp.tprotaddr = arp_not_revarp ? arp.sprotaddr : ap->dest;
      arp.sprotaddr = i ? my_ip_addr : ap->dest;
      arp.opcode = arp_not_revarp ? ARP_REPLY : REVARP_REPLY;
      arp_send(mhbp->l2port, (char *)arp.thwaddr);
/*            Arp_stat.inreq++;    */
    }
  }
}

/************************************************************************
 * Function : Einen ARP request/response Frame senden ( L2 AX25 )
 *
 * Inputs   : Internes ARP-Frame und Port
 *
 * Returns  : nix
 *
 * Outputs  : UI Frame mit ARP request/response
 *
 *----------------------------------------------------------------------*/
void
arp_send(unsigned port, char *hwaddr)
{
  register MBHEAD *mbhd;

  mbhd = (MBHEAD *)allocb(ALLOC_MBHEAD);
#ifdef __WIN32__
  put16((unsigned short)arp.hardware, mbhd);
  put16((unsigned short)arp.protocol, mbhd);
#else
  put16(arp.hardware, mbhd);
  put16(arp.protocol, mbhd);
#endif /* WIN32 */
  putchr(arp.hwalen, mbhd);
  putchr(arp.pralen, mbhd);
#ifdef __WIN32__
  put16((unsigned short)arp.opcode, mbhd);
#else
  put16(arp.opcode, mbhd);
#endif /* WIN32 */
  putfid((char *)arp.shwaddr, mbhd);
  put32(arp.sprotaddr, mbhd);
  putfid((char *)arp.thwaddr, mbhd);
  put32(arp.tprotaddr, mbhd);
  mbhd->l2fflg = L2CARP;
  rwndmb(mbhd);
#ifdef __WIN32__
  sdui("", hwaddr, myid, (char)port, mbhd);
#else
  sdui("", hwaddr, myid, port, mbhd);
#endif /* WIN32 */
  dealmb(mbhd);
}

/************************************************************************
 * Function : Ein ARP request Frame senden (L2 AX25 UI)
 *
 * Inputs   : Internes ARP-Frame und der port
 *
 * Returns  : nix
 *
 * Outputs  : UI Frame mit ARP request senden
 *
 *----------------------------------------------------------------------*/
void
arp_request(ipaddr gateway, unsigned hwtype, unsigned port)
{
  register unsigned i;

  arp.hardware = hwtype;
  arp.protocol = L2CIP;
  arp.hwalen = L2IDLEN;
  arp.pralen = sizeof(ipaddr);
  arp.opcode = ARP_REQUEST;
  cpyid((char *)arp.shwaddr, myid);
  arp.sprotaddr = my_ip_addr;
  arp.tprotaddr = gateway;
  for (i = 0; i < L2IDLEN; i++)
    arp.thwaddr[i] = 0;
  arp_send(port, QST);
}

/*----------------------------------------------------------------------*/
/* Neue Status-Meldung verarbeiten                                      */
/* 0=nicht verwendet, 1=connectet, 2=disconnectet, 3=busy, 4=Failure    */
/*----------------------------------------------------------------------*/
BOOLEAN
l2toip(WORD status)
{
  ARP_TAB        *ap;
  PTCENT         *ptcp;
  const char     *viap;

  viap = ndigipt(lnkpoi->viaidl);

  if (!cmpid(lnkpoi->srcid, myid))
    return (FALSE);
  for (ap = (ARP_TAB *) Arp_tab.head;
       ap != (ARP_TAB *) & Arp_tab;
       ap = (ARP_TAB *) ap->nextar)
    if (   (ap->port == lnkpoi->liport)
        && cmpidl(ap->digi, viap)
        && cmpid(ap->callsign, lnkpoi->dstid))
    {
      ptcp = ptctab + g_uid(lnkpoi, L2_USER);
      switch (status)
      {
        case L2MCONNT:
        case L2MLRESF:
        case L2MLREST:
          if (ptcp->state != D_IPLINK)
            ptcp->state = U_IPLINK;
          ptcp->ublk = NULL;
          ptcp->p_uid = NO_UID;
          break;

        case L2MDISCF:
        case L2MBUSYF:
        case L2MFAILW:
          clrptc(g_uid(lnkpoi, L2_USER));
      }
      return (TRUE);            /* Meldung verarbeitet */
    }
  return (FALSE);
}
#endif

/* End of src/l3ip.c */
