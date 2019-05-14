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
/* File include/ip.h (maintained by: DG1KWA)                            */
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

/* IP protocol field values */
#define    ICMP_PTCL    1    /* Internet Control Message Protocol */
#define    TCP_PTCL    6    /* Transmission Control Protocol */
#define    UDP_PTCL    17    /* User Datagram Protocol */
#define    RSPF_PTCL    73    /* Radio Shortest Path First Protocol */

#define    MAXTTL        255    /* Maximum possible IP time-to-live value */

/* DoD-style precedences */
#define    ROUTINE        0x00
#define    PRIORITY    0x20
#define    IMMEDIATE    0x40
#define    FLASH        0x60
#define    FLASH_OVER    0x80
#define    CRITIC        0xa0
#define    INET_CTL    0xc0
#define    NET_CTL        0xe0

/* Amateur-style precedences */
#define    AM_ROUTINE    0x00
#define    AM_WELFARE    0x20
#define    AM_PRIORITY    0x40
#define    AM_EMERGENCY    0x60

/* Class-of-service bits */
#define    LOW_DELAY    0x10
#define    THROUGHPUT    0x08
#define    RELIABILITY    0x04

/* IP TOS fields */
#define    PREC(x)        ((x)>>5 & 7)
#define    DELAY        0x10
#define    THRUPUT        0x8
#define    RELIABLITY    0x4

/* structure for an ip address (long) */
typedef unsigned long ipaddr;

/* Format of a MIB entry for statistics gathering */
typedef struct mib_entry {
     const char *name;
    struct {
        unsigned int integer;
    } value;
} MIB_ENTRY;

#define    TLB    30            /* Default reassembly timeout, sec      */
#define    IPVERSION    4       /* IP-Version 4.0                       */
#define    IP_MAXOPT    40      /* Largest option field, bytes          */

/* SNMP MIB variables, used for statistics and control. See RFC 1066 */
#define    ipForwarding        Ip_mib[1].value.integer
#define    ipDefaultTTL        Ip_mib[2].value.integer
#define    ipInReceives        Ip_mib[3].value.integer
#define    ipInHdrErrors        Ip_mib[4].value.integer
#define    ipInAddrErrors        Ip_mib[5].value.integer
#define    ipForwDatagrams        Ip_mib[6].value.integer
#define    ipInUnknownProtos    Ip_mib[7].value.integer
#define    ipInDiscards        Ip_mib[8].value.integer
#define    ipInDelivers        Ip_mib[9].value.integer
#define    ipOutRequests        Ip_mib[10].value.integer
#define    ipOutDiscards        Ip_mib[11].value.integer
#define    ipOutNoRoutes        Ip_mib[12].value.integer
#define    ipReasmTimeout        Ip_mib[13].value.integer
#define    ipReasmReqds        Ip_mib[14].value.integer
#define    ipReasmOKs            Ip_mib[15].value.integer
#define    ipReasmFails        Ip_mib[16].value.integer
#define    ipFragOKs            Ip_mib[17].value.integer
#define    ipFragFails            Ip_mib[18].value.integer
#define    ipFragCreates        Ip_mib[19].value.integer

#define    NUMIPMIB    19

/* IP header, INTERNAL representation */
typedef struct ip_struct {
    unsigned char version;    /* IP version number */
    unsigned char ihl;        /* Internet Header Length         */
    unsigned char tos;        /* Type of service */
    unsigned length;        /* Total length */
    unsigned id;            /* Identification */
    unsigned offset;        /* Fragment offset in bytes */
    struct {
        unsigned char df;    /* Don't fragment flag */
        unsigned char mf;    /* More Fragments flag */
    } flags;

    unsigned char ttl;        /* Time to live */

    unsigned char protocol;    /* Protocol */
    unsigned checksum;        /* Header checksum */
    ipaddr source;            /* Source address */
    ipaddr dest;            /* Destination address */
    unsigned char options[IP_MAXOPT];/* Options field */
    unsigned optlen;        /* Length of options field, bytes */
} IP;

#define TCP_MAXOPT 4           /* max Anzahl als Option-Bytes    */
typedef unsigned long SEQ;
typedef unsigned long ACK;
/* TCP header */
typedef struct tcp_struct {
   unsigned srcPort;                   /* Source-Port                    */
   unsigned dstPort;                   /* Destination-POrt               */
   SEQ seqnum;                         /* Sequence Number                */
   ACK acknum;                         /* Acknowledgement Number         */
   unsigned char data_offset;          /* Data Offset 4 Bits             */
   unsigned char res;                  /* reserviert                     */
   unsigned flags;                     /* Flags                          */
   unsigned window;                    /* Window                         */
   unsigned checksum;                  /* Checksum                       */
   unsigned urgentPointer;             /* Urgent Pointer                 */
   unsigned char options[TCP_MAXOPT];  /* Options and Padding            */
} TCP;

/* UDP header */
typedef struct udp_struct {
   unsigned srcPort;
   unsigned dstPort;
   unsigned length;
   unsigned checksum;
} UDP;

#define    NULLIP    (IP *)NULL
#define    IPLEN    20    /* Length of standard IP header */

/* Fields in option type byte */
#define    OPT_COPIED    0x80    /* Copied-on-fragmentation flag */
#define    OPT_CLASS    0x60    /* Option class */
#define    OPT_NUMBER    0x1f    /* Option number */

/* IP option numbers */
#define    IP_EOL         0    /* End of options list */
#define    IP_NOOP         1    /* No Operation */
#define    IP_SECURITY     2    /* Security parameters */
#define    IP_LSROUTE     3    /* Loose Source Routing */
#define    IP_TIMESTAMP 4    /* Internet Timestamp */
#define    IP_RROUTE     7    /* Record Route */
#define    IP_STREAMID     8    /* Stream ID */
#define    IP_SSROUTE     9    /* Strict Source Routing */

/* Timestamp option flags */
#define    TS_ONLY        0    /* Time stamps only */
#define    TS_ADDRESS    1    /* Addresses + Time stamps */
#define    TS_PRESPEC    3    /* Prespecified addresses only */

/* structure for routing tables */

typedef struct iproute
{
    struct iproute *nextip;
    struct iproute *previp;
#ifdef BUFFER_DEBUG
    UBYTE         owner;      /* Muss an 9. Bytestelle stehen */
#endif
    ipaddr        dest;
    ipaddr        gateway;
    unsigned      metric;
    unsigned      timer;
    unsigned char bits;
#ifdef __WIN32__
    unsigned char Interface;
#else
    unsigned char interface;
#endif
    unsigned char flags;
#define    RTDYNAMIC 0x01   /* dynamic ip-address */
    unsigned char spare_byte;
    UBYTE         port;
    BOOLEAN       automatic_flag;
} IP_ROUTE;

#define    NULLROUTE    (IP_ROUTE *)NULL

/* Cache for the last-used routing entry, speeds up the common case where
 * we handle a burst of packets to the same destination
 */
/*typedef struct rt_cache {
    ipaddr target;
    IP_ROUTE *route;
} RT_CACHE;
*/
typedef struct arp_tab
{
    struct arp_tab *nextar;
    struct arp_tab *prevar;
#ifdef BUFFER_DEBUG
    UBYTE         owner;      /* Muss an 9. Bytestelle stehen */
#endif
    ipaddr    dest;
    unsigned  timer;
    char      dgmode;
    UBYTE     hwtype;
    char      publish_flag;
    char      state;
    char      callsign[7];
    char      digi[15];
    WORD      port;
    BOOLEAN   automatic_flag;
} ARP_TAB;

#define NULLARP (ARP_TAB *)NULL

#define ARP_NETROM 0
#define ARP_AX25 3

#define MAXHWALEN 10
#define IPTYPE 42

#define ARP_REQUEST    1
#define ARP_REPLY      2
#define REVARP_REQUEST 3
#define REVARP_REPLY   4

typedef struct arp
{
    unsigned hardware;
    unsigned protocol;
    unsigned char hwalen;
    unsigned char pralen;
    unsigned opcode;
    unsigned char shwaddr[MAXHWALEN];
    ipaddr sprotaddr;
    unsigned char thwaddr[MAXHWALEN];
    ipaddr tprotaddr;
} ARP;

#define NULLBUF ( void * )NULL

/* Pseudo-header for TCP and UDP checksumming */
typedef struct pseudo_header {
    ipaddr source;        /* IP source */
    ipaddr dest;        /* IP destination */
    unsigned char protocol;        /* Protocol */
    unsigned length;        /* Data field length */
} PSEUDO_HEADER;
#define    NULLHEADER    (struct pseudo_header *)NULL

#ifdef IPROUTE
/* src/l7ip.c */
void ccpipr(void);
void showroute(IP_ROUTE *,MBHEAD *);
void show_ip_addr(ipaddr,MBHEAD *);
BOOLEAN get_ip_addr(ipaddr *,WORD *,char **);
void ccparp(void);
void showarp(ARP_TAB *,MBHEAD *);
void ccpipa(void);
void ccpipb(void);
void ccpips(void);
BOOLEAN rt_add(ipaddr,unsigned int ,ipaddr,int ,unsigned int ,unsigned int ,int, BOOLEAN);
BOOLEAN route_find(IP_ROUTE **,ipaddr *,ipaddr,unsigned int );
BOOLEAN rt_drop(ipaddr,unsigned int, BOOLEAN);
BOOLEAN arp_add(ipaddr, WORD, char *, const char *, unsigned int,
                unsigned int, BOOLEAN, BOOLEAN);
BOOLEAN find_arp(ARP_TAB * *,ipaddr, WORD );
BOOLEAN arp_drop(ipaddr, WORD, BOOLEAN);
void arpsrv(void);
void ccpping(void);
BOOLEAN l2toip(WORD);

/* src/iproute.c */
void ipinit(void );
void ipserv(void );
void ip_route(MBHEAD * );
void arp_service(MBHEAD *);
IP_ROUTE *rt_find(ipaddr);
void nr_iface(MBHEAD *,ipaddr );
void l2_iface(MBHEAD *,unsigned int ,ipaddr,unsigned int );
ARP_TAB *res_arp(ipaddr, unsigned int );
MBHEAD *htonip(IP *,MBHEAD *, BOOLEAN );
int ip_send(ipaddr,ipaddr,unsigned ,unsigned ,unsigned ,MBHEAD *,unsigned short ,unsigned short ,unsigned );
unsigned short eac(long );
unsigned short cksum(PSEUDO_HEADER *,MBHEAD *,unsigned short );
void arp_request(ipaddr,unsigned,unsigned);
void arp_send( unsigned, char *);
void ccp_ip_help(ipaddr *, const char *);
BOOLEAN pingem(ipaddr target,int seq,int id, int len, char *opt);

#define NR4_OP_PID      0
#define NR_PROTO_IP     0x0c

typedef struct ipportpar {
    WORD    ipMode;        /* Mode-Flags fuer diesen Port */
#define ARP_OK 0x0001      /* ARP erlaubt */
#define IP_FORWARDING 0x0002 /* das Weiterleiten von IP-Frames */
    WORD    mtu;           /* Maximale Blockgroesse */
} IPPORTPAR;

extern LHEAD arprxfl;          /* Empfangene ARP-Frames */

extern IPPORTPAR IPpar[];      /* fuer jeden L2-Port + NETROM */
#define NETROM_PORT L2PNUM

#ifdef KERNELIF
#define KERNEL_PORT L2PNUM + 1
#endif

#define is_my_ip_addr( address ) ( address == my_ip_addr )
#define is_broadcast_address( address ) ( bcast_ip_addr != 0 && bcast_ip_addr == address )

#else
#define l2toip(x) FALSE

#endif /* IPROUTE */
/* End of $RCSfile$ */
