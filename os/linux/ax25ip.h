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
/* File os/linux/ax25ip.h (maintained by: DG9OBU)                       */
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

/* Defines */
#define NO_MODE  0
#define IP_MODE  1
#define UDP_MODE 2
#define IPPROTO_AX25 93
#define DEFAULT_UDP_PORT 10093
#define CONFIG_FILE "ax25ip.cfg"
#define NOT_LAST(p)     (((*(p+6))&0x01)==0)
#define NOTREPEATED(p)  (((*(p+6))&0x80)==0)
#define NO_DIGIS(f)     (((*(f+13))&0x01)!=0)
#define MAX_FRAME  2048
#define TABLE_SIZE  128
#define LOGL1 if(loglevel>0)(void)write_log
#define LOGL2 if(loglevel>1)(void)write_log
#define LOGL3 if(loglevel>2)(void)write_log
#define LOGL4 if(loglevel>3)(void)write_log

#define HNLEN     64

#define OP_NONE    0
#define OP_ADD     1
#define OP_DEL     2
#define OP_MYUDP   3
#define OP_LOG     4
#define OP_TIMEOUT 5
#define OP_HNUPD   6

#define BUFLEN    32

static unsigned int uDynTimeout = 3600; /* eine Stunde */

/* Variablen */
static LHEAD  *l2flp;
static DEVICE *l1pp;
static int     ax25ip_port;
static BOOLEAN ax25ip_active = FALSE;
static BOOLEAN sockopt_keepalive = FALSE;
static BOOLEAN sockopt_throughput = FALSE;
static WORD    mode;
/* Nameservice-Updateintervall (in Zehntelsekunden !) */
static UWORD   uNamesUpdate = 18000;
static struct  sockaddr_in udpbind;
static struct  sockaddr_in ipbind;
static struct  sockaddr_in to;
static struct  sockaddr_in from;
#ifndef ATTACH
unsigned short my_udp = 0;
#endif /* ATTACH */
int            route_tbl_top = 0;
int            loglevel = 0;

int            fd_udp = -1;             /* Filedescriptor fuer UDP */
int            fd_ip = -1;              /* Filedescriptor fuer IP  */

/* TEST DG9OBU */
static BOOLEAN new_parser = FALSE;      /* Standardmaessig alte Syntax */

struct route_table_entry {
   unsigned char callsign[L2IDLEN];     /* the callsign and ssid */
   unsigned long ip_addr;               /* the IP address */
   unsigned short udp_port;             /* the port number if udp */
   unsigned int timeout;                /* Timeout der Route */
   unsigned char hostname[HNLEN];
#ifdef AXIPR_UDP
   unsigned short org_udp_port;         /* Original UDP-Port. */
#endif
#ifdef AXIPR_HTML
   unsigned char  timeset[17 + 1];      /* Login-Zeit/Date. */
   unsigned short protokoll;            /* Protokoll (INP,Flexnet..) */
   unsigned short online;               /* Route online/offline. */
#endif
};

struct route_table_entry route_tbl[TABLE_SIZE];
struct route_table_entry default_route;

/* Funktionsprototypen */
int                setup_ip(void);
int                setup_udp(unsigned short);
void               show_rt_entry(struct route_table_entry, MBHEAD*);
void               ccpaxipr(void);
void               ax25ip_send(void);
void               ax25ip_recv(void);
void               ax25ip_check_up(void);
void               ax25ip_check_down(void);
#ifndef AXIPR_UDP
static void        route_add(unsigned char*, struct hostent*, unsigned char*, int, int, int);
#else
static BOOLEAN     route_add(unsigned char*, struct hostent*, unsigned char*, int, int, int
                            , char *
#ifdef AXIPR_HTML
                            , char *, UWORD
#endif /* AXIPR_HTML */
                            );
#endif /* AXIPR_UDP */

BOOLEAN            route_del(unsigned char*);
unsigned int       count_routes(BOOLEAN);
void               route_init(void);
void               route_age(void);
void               route_update(struct route_table_entry*);
BOOLEAN            route_canlearn(MBHEAD*);
void               add_crc(unsigned char*, int);
int                addrmatch(unsigned char*, unsigned char*);
void               add_ax25crc(unsigned char*, int);
UWORD              compute_crc(unsigned char*, int);
BOOLEAN            ok_crc(unsigned char*, int);
int                config_read(void);
int                parse_line(char*);
int                a_to_call(char*, unsigned char*);
char              *call_to_a(unsigned char*);

struct route_table_entry *call_to_ip(unsigned char*);

unsigned char     *next_addr(unsigned char*);
void               write_log(const char*, ...);

#ifdef AXIPR_UDP
void           route_analyse(struct in_addr, unsigned char *, int);
void           route_check(register int);
struct route_table_entry *search_route(unsigned char *);
void           UpdateRoute(unsigned char *, int, int, char *
#ifdef AXIPR_HTML
                           , char *, UWORD
#endif /* AXIPR_HTML */
                           );
#endif /* AXIPR_UDP */

#ifdef AXIPR_HTML
#define P_OFFLINE  0
#define P_USER     1
#define P_THENET   2
#define P_INP      3
#define P_FLEXNET  4

#define CSS_RFILE  "rstat.css"
#define HTML_RFILE "rstat.html"
#define HTML_INV   60

static unsigned int HtmlStat = FALSE; /* HtmlStatistik ausgeshaltet. */

void h_timer(void);
void w_statistik(void);
char *set_time(void);
void set_status(register int, int, PEER *);
#endif

/* End */
