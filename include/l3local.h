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
/* File include/l3local.h (maintained by: ???)                          */
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

#define L3_RTT_TIME 180        /* Messintervall in Sekunden-Schritten   */
#define LEARN (59999U)         /* Lernqualitaet                         */
#define HORIZONT (60000U)      /* alles darueber ist ausgefallen        */
#define ROUTE_TIMEOUT 21       /* 21 Minuten                            */

#define MESSTIME 180
#define DEFAULT_LT  10         /* fuer alle unbekannten Ziele           */

#ifdef FLEXHOST                /* Hosting auch fuer FlexNet-Fernziele?  */
#define HOST_MASK (VC+VC_FAR+DG)
#else
#define HOST_MASK (VC+DG)
#endif

#define FLEX_MASK VC|VC_FAR

/* Layer 3 Broadcast Typs ----------------------------------------------*/

#define ALL 1                  /* alle Nodes broadcasten                */
#define CHANGES 2              /* nur Aenderungen broadcasten           */


#define LEARNQUAL       2               /* Qualitaet eines Fastlearns   */

#define DONT_CHANGE_QUAL 65535U     /* Qualitaet nicht aendern       */
#define DONT_CHANGE_SSID (-1)       /* SSID nicht aendern            */
#define DONT_CHANGE_ALIAS (nulide)  /* Alias nicht aendern           */
#define DONT_CHANGE_LT   (-1)       /* LT nicht aendern              */

/* End of include/l3.h */

/* Definitionen, die nur im L3 verwendet werden                         */

#define RTT_MIN 1               /* Grenzwerte fuer SRTT 1ms..<60s       */
#define RTT_MAX 59999L

#define RTT_ALPHA1      7
#define RTT_ALPHA2      8
#define RTT_BETA        3

#define IGNORE_RTT  0           /* Messung ignorieren                   */

#define  cpyals(x,y) memmove(x,y,6)
#define  cmpals(x,y) (!strnicmp(x,y,6))

extern UWORD flexmode;
extern UWORD max_lt;
extern char  orgnod[];
extern char  desnod[];
extern char  time_to_live;
extern int   max_nodes;
extern int   max_peers;
extern int   num_nodes_max;

/* ------------------------------------------ Funktionen in src/l3inp.c */

void     propagate_node_update(INDEX);
BOOLEAN  rx_inp_broadcast(PEER *, MBHEAD *);
void     add_inp_info(MBHEAD **, NODE *, PEER *, unsigned, unsigned, int);
void     send_inp_nodebeacon(PEER *);

/* ----------------------------------------- Funktionen in src/l3misc.c */

BOOLEAN  register_network(int, int);
void     unregister_network(void);
BOOLEAN  ge6chr(char *, MBHEAD *);
void     pu6chr(char *, MBHEAD *);
void     cpyidl2(char *, const char *);
void     nrr_rx(MBHEAD *, PEER *);
void     send_nrr_frame(NRRLIST *);

/* ------------------------------------------ Funktionen in src/l3nbr.c */

void     discnbp(PEER *);
BOOLEAN  connbr(PEER *);
void     disnbr(PEER *);
PEER    *getnei(const char *, const char *, int);
void     toneig(PEER *, MBHEAD *);
void     newnbr(PEER *);
PEER    *ispeer(void);
void     rxneig(PEER *, MBHEAD *);

/* --------------------------------------- Funktionen in src/l3netrom.c */

void     netrom_rx(PEER *);
void     rx_broadcast(PEER *, MBHEAD *);
void     sdl3ui(PEER *, MBHEAD *);
void     brosnd(MBHEAD **, PEER *);
void     inform_peer(PEER *, int);

/* ------------------------------------------ Funktionen in src/l3rtt.c */

void     send_l3srtt_frame(PEER *);
void     rtt_metric(PEER *, long);
void     rx_l3rtt_frame(PEER *, MBHEAD *, char huge *, WORD);

/* ------------------------------------------ Funktionen in src/l3tab.c */

PEER    *register_peer(void);
void     unregister_peer(PEER *);
int      add_node(const char *);
void     del_node(int);
void     drop_unreachable_nodes(void);
int      add_route(PEER *, const char *, unsigned);
void     update_route(PEER *, INDEX, unsigned);
void     update_lt(PEER *, INDEX, int);
BOOLEAN  update_alias(INDEX, const char *);
BOOLEAN  update_ssid(int, int);
void     update_peer_quality(PEER *, unsigned long, unsigned long);
void     reset_peer(PEER *);
void     connect_peer(PEER *);
void     disconnect_peer(PEER *);
void     set_peer_typ(PEER *, int);
BOOLEAN  check_destot(INDEX);
void     check_all_destot(void);
void     destot(int);
void     update_primary_peer(char *);

/* ------------------------------------------- Funktionen in src/l3vc.c */

void     local_flex_srv(void);
void     local_status(PEER *, WORD);
void     flex_status(PEER *, WORD);
void     flex_rx(PEER *, MBHEAD *);
void     flexnet_rx(PEER *);

#ifdef MC68302             /* Haelt den L3 zusehr auf */
# undef MAX_TRACE_LEVEL
# define MAX_TRACE_LEVEL 0
#endif

/* End of include/l3local.h */
