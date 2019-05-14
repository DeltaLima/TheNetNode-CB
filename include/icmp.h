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
/* File include/icmp.h (maintained by: DG1KWA)                          */
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

/* SNMP MIB variables, used for statistics and control. See RFC 1066 */
#define icmpInMsgs            Icmp_mib[1].value.integer
#define icmpInErrors        Icmp_mib[2].value.integer
#define icmpInDestUnreachs    Icmp_mib[3].value.integer
#define icmpInTimeExcds        Icmp_mib[4].value.integer
#define icmpInParmProbs        Icmp_mib[5].value.integer
#define icmpInSrcQuenchs    Icmp_mib[6].value.integer
#define icmpInRedirects        Icmp_mib[7].value.integer
#define icmpInEchos            Icmp_mib[8].value.integer
#define icmpInEchoReps        Icmp_mib[9].value.integer
#define icmpInTimestamps    Icmp_mib[10].value.integer
#define icmpInTimestampReps    Icmp_mib[11].value.integer
#define icmpInAddrMasks        Icmp_mib[12].value.integer
#define icmpInAddrMaskReps    Icmp_mib[13].value.integer
#define icmpOutMsgs            Icmp_mib[14].value.integer
#define icmpOutErrors        Icmp_mib[15].value.integer
#define icmpOutDestUnreachs    Icmp_mib[16].value.integer
#define icmpOutTimeExcds    Icmp_mib[17].value.integer
#define icmpOutParmProbs    Icmp_mib[18].value.integer
#define icmpOutSrcQuenchs    Icmp_mib[19].value.integer
#define icmpOutRedirects    Icmp_mib[20].value.integer
#define icmpOutEchos        Icmp_mib[21].value.integer
#define icmpOutEchoReps        Icmp_mib[22].value.integer
#define icmpOutTimestamps    Icmp_mib[23].value.integer
#define icmpOutTimestampReps Icmp_mib[24].value.integer
#define icmpOutAddrMasks    Icmp_mib[25].value.integer
#define icmpOutAddrMaskReps    Icmp_mib[26].value.integer
#define    NUMICMPMIB    26

/* Internet Control Message Protocol */

/* Message types */
#define    ICMP_ECHO_REPLY        0    /* Echo Reply */
#define    ICMP_DEST_UNREACH    3    /* Destination Unreachable */
#define    ICMP_QUENCH            4    /* Source Quench */
#define    ICMP_REDIRECT        5    /* Redirect */
#define    ICMP_ECHO            8    /* Echo Request */
#define    ICMP_TIME_EXCEED    11    /* Time-to-live Exceeded */
#define    ICMP_PARAM_PROB        12    /* Parameter Problem */
#define    ICMP_TIMESTAMP        13    /* Timestamp */
#define    ICMP_TIME_REPLY        14    /* Timestamp Reply */
#define    ICMP_INFO_RQST        15    /* Information Request */
#define    ICMP_INFO_REPLY        16    /* Information Reply */
#define    ICMP_ADDR_MASK        17    /* Address mask request */
#define    ICMP_ADDR_MASK_REPLY 18    /* Address mask reply */
#define    ICMP_TYPES            19

/* Internal format of an ICMP header */
typedef struct icmp_ {
    char type;
    char code;
     union icmp_args {
        int mtu;
        int unused;
        unsigned char pointer;
        ipaddr address;
        struct {
            int id;
            int seq;
        } echo;
    } args;
    unsigned checksum;
} ICMP ;

#define    ICMPLEN        8    /* Length of ICMP header on the net */
#define    NULLICMP    (union icmp_args *)NULL

/* Destination Unreachable codes */
#define    ICMP_NET_UNREACH    0    /* Net unreachable */
#define    ICMP_HOST_UNREACH    1    /* Host unreachable */
#define    ICMP_PROT_UNREACH    2    /* Protocol unreachable */
#define    ICMP_PORT_UNREACH    3    /* Port unreachable */
#define    ICMP_FRAG_NEEDED    4    /* Fragmentation needed and DF set */
#define    ICMP_ROUTE_FAIL        5    /* Source route failed */

#define    NUNREACH    6

/* Time Exceeded codes */
#define    ICMP_TTL_EXCEED        0    /* Time-to-live exceeded */
#define    ICMP_FRAG_EXCEED    1    /* Fragment reassembly time exceeded */

#define    NEXCEED        2

/* Redirect message codes */
#define    ICMP_REDR_NET    0    /* Redirect for the network */
#define    ICMP_REDR_HOST    1    /* Redirect for the host */
#define    ICMP_REDR_TOS    2    /* Redirect for Type of Service, or-ed with prev */

#define    NREDIRECT    3

void icmp_output(IP *,MBHEAD *,unsigned ,unsigned ,union icmp_args *);
MBHEAD *htonicmp(ICMP *,MBHEAD *);
void icmp_input(IP *,MBHEAD *);

/* End of $RCSfile$ */
