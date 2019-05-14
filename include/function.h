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
/* File include/function.h (maintained by: ???)                         */
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

#define VMSG if(bVerbose==TRUE)printf

/*-------------------------------------------- Funktionen in src/main.c */

void    memerr(void);
void    quit_program(int);

#ifdef ST          /*------------------------------- Fuer ST ---------- */
void    stime(time_t *);
BOOLEAN enable_port(UWORD);
BOOLEAN disable_port(UWORD);
void    tzset(void);
char   *tempnam(const char *, const char *);
#endif

#ifdef MC68302     /*------------------------------- Fuer TNC3 ---------*/
void    stime(time_t *);
void    getftime(char *, struct ftime *);
char   *tempnam(const char *, const char *);
void    mem_init(void);
#endif

/*------------------------------------------------ allgemeine Low-Level */

void    DIinc(void);
void    decEI(void);

BOOLEAN init_hardware(int argc, char *argv[]);
void    exit_hardware(void);
void    reboot_system(void);

void    init_console(void);
void    exit_console(void);

BOOLEAN ishget(void);
BOOLEAN ishput(void);
char    hgetc(void);
void    toggle_rts(void);

int     xchdir(const char *);
int     xaccess(const char *, int);
int     xfindfirst(const char *, struct ffblk *, int);
int     xfindnext(struct ffblk *);
FILE   *xfopen(const char *, const char *);
int     xremove(const char *);
int     xrename(const char *, const char *);
char   *normfname(char *);

void    shellsrv(void);
BOOLEAN l7tosh(MBHEAD *);
BOOLEAN tnnshell(char *);
void    tnnexec(char *);

unsigned long coreleft(void);
void          update_timer(void);

#ifdef __GO32__
int access(const char *, int);
#endif
/*------------------------------------------ Funktionen in src/buffer.c */

void    inithd(LHEAD *);
void    init_buffers(void);
LEHEAD *ulink(LEHEAD *);
LEHEAD *relink(LEHEAD *, LEHEAD *);
#ifdef BUFFER_DEBUG
LEHEAD *allocb(int);
#else
LEHEAD *allocb(void);
#endif
void    dealoc(MBHEAD *);
void    dealmb(MBHEAD *);
void    dealml(LEHEAD *);
void    rwndmb(MBHEAD *);
void    putchr(char, MBHEAD *);
#ifndef __WIN32__
char    getchr(MBHEAD *);
#else
unsigned
char    getchr(MBHEAD *);
#endif /* WIN32 */
UWORD   get16(MBHEAD *);
void    put16(UWORD, MBHEAD *);
ULONG   get32(MBHEAD *);
void    put32(ULONG, MBHEAD *);
BOOLEAN splcpy(WORD, MBHEAD *, MBHEAD *);

/*-------------------------------------------------- Funktionen in L1.C */

void    l1rxtx(void);
void    l1init(void);
void    l1exit(void);
void    l1timr(UWORD);
void    kicktx(int);
void    l1ctl(int, int);
WORD    iscd(int);
int     l1attach(int, char *);
void    l1detach(int);
void    l1hwstr(int, MBHEAD *);
void    l1hwcfg(int, MBHEAD *);
void    l1enum(MBHEAD *);
void    l1stat(const char *, MBHEAD *);
void    l1sclr(const char *);

/*---------------------------------------- Funktionen in src/callstr.c */

BOOLEAN         cmpcal(const char *, const char *);
BOOLEAN         cmpid(const char *, const char *);
BOOLEAN         cmpidl(const char *, const char *);
void            cpyid(char *, const char *);
void            cpyidl(char *, const char *);
void            addid(char *, const char *);
const char     *ndigipt(const char *);
#ifdef MH_LISTE
const char     *ydigipt(const char *viap);
#endif
void            putvia(const char *, MBHEAD *);
void            putfid(const char *, MBHEAD *);
void            call2str(char *, char *);
void            callss2str(char *, char *);
void            str2call(char *, char *);
BOOLEAN         c6mtch(const char *, const char *);
const char     *dheardcall(const char *);

/*------------------------------------------ Funktionen in src/l2dama.c */

void    iniDAMA(void);
void    timDAMA(UWORD);
void    clrDAMA(void);
void    incDAMA(void);
void    clearDT(UWORD);
void    polDAMA(void);
void    getMCs(void);
BOOLEAN multiconn(int, char *);

/*------------------------------------------ Funktionen in src/l2misc.c */

void    l2(void);
void    l2init(void);
void    autopar(int);
void    autopers(int);
BOOLEAN busy(int);
void    l2profiler(void);
BOOLEAN istome(const char *);
int     istomev(void);
void    newlnk(void);
void    dsclnk(void);
void    inilnk(void);
void    clrlnk(void);
void    reslnk(void);
void    acklnk(LNKBLK *);
void    rejlnk(LNKBLK *);
LNKBLK *getlnk(UBYTE, char *, char *, char *);
void    change_maxframe(LNKBLK *, int);

/*-------------------------------------------- Funktionen in src/l2rx.c */

void    l2rx(void);
BOOLEAN takfhd(MBHEAD *);
void    l2tolx(WORD);
void    i2tolx(BOOLEAN);
void    i2xclr(void);
void    gateway_ui(char *, char *, char *, MBHEAD *);

/*------------------------------------------ Funktionen in src/l2stma.c */

void    l2stma(STENTRY[]);
void    l2newstate(WORD);

/*----------------------------------------- Funktionen in src/l2timer.c */

void    l2timr(void);
void    t2rrr(void);
void    t2rnrr(void);
void    t2rejr(void);
void    setT1(void);
void    clrT1(void);
void    setT2(UBYTE);
void    clrT2(void);
void    setT3(void);
void    clrT3(void);
void    setRTT(void);
void    clrRTT(void);
void    chknoa(void);
void    setiSRTT(void);

/*-------------------------------------------- Funktionen in src/l2tx.c */

void    l2tx(void);
void    clrstfl(void);
void    sdfrmr(char);
void    xnull(void);
void    xrrc(void);
void    xrrr(void);
void    xrnrc(void);
void    xrnrr(void);
void    xrejr(void);
void    xdm(void);
void    xua(void);
void    xsabm(void);
void    xdisc(void);
void    xfrmr(void);
void    stxfad(void);
void    sendS(UBYTE);
void    stxcfr(void);
UBYTE   setNR(UBYTE);
void    sdl2fr(MBHEAD *, BOOLEAN);
MBHEAD  *makfhd(int);
BOOLEAN itolnk(int, BOOLEAN, MBHEAD *);
char    outsdI(void);
char    itxwnd(void);
void    sdoi(void);
void    sdi(int, int);
void    sdui(const char *, const char *, const char *, char, MBHEAD *);

BOOLEAN getfid(char *, MBHEAD *);
/* TEST DG9OBU */
BOOLEAN getfidc(char *, MBHEAD *);

/* ----------------------------------------- Funktionen in src/l3misc.c */

void    l3init(void);
void    i3tolnk(UBYTE, LNKBLK *, MBHEAD *);
void    l3rx(void);
void    l3tx(void);
void    l3rest(void);
void    brosrv(void);
BOOLEAN l2tol3(WORD);
BOOLEAN tol3sw(MBHEAD *);
void    request_nrr(char *id, UID uid);
BOOLEAN islinkport(int);
BOOLEAN register_network(int,int);
void    unregister_network(void);
/* TEST DG9OBU (gilt fuer alle noch folgenden Funktionen aus l3misc.c) */
TRILLIAN islocal(const char *);
#ifdef PROXYFUNC
BOOLEAN isproxy(const char *);
#endif
void    getSSIDrange(int *, int *);
int     maxSSID(void);
BOOLEAN SSIDinrange(int);

/* ------------------------------------------ Funktionen in src/l3nbr.c */

#ifndef LINKSMOD_MSG
void    unregister_neigb(const char *, const char *, int);
#else
BOOLEAN unregister_neigb(const char *, const char *, int);
#endif /* LINKSMOD_MSG */
PEER   *register_neigb(const char *, const char *, const char *, int, int
#ifdef LINKSMODINFO
                      ,const char *
#endif /* LINKSMODINFO */
#ifdef AUTOROUTING
                      , UWORD
#endif /* AUTOROUTING */
                      );

/* ------------------------------------------ Funktionen in src/l3rtt.c */

void    l3rtt_service(void);
BOOLEAN match(MBHEAD *, const char *);

/* ------------------------------------------ Funktionen in src/l3tab.c */

INDEX    find_node_ssid_range(const char *);
INDEX    find_node_this_ssid(const char *);
#define  find_best_qual(index, pp, opt) do_find_best_qual(index, NULL, pp, opt)
#define  find_best_notvia(index, notthis, pp, opt) do_find_best_qual(index, notthis, pp, opt)
unsigned do_find_best_qual(INDEX, PEER *, PEER **, int);
unsigned getquality(unsigned, PEER *);
int      l3_find_route(char *, DEST *);
#define NODE_UNKNOWN   1
#define NODE_DOWN      2
#define NODE_AVAILABLE 0
int      find_alias(char *);
BOOLEAN  iscall(const char *, NODE **, PEER **, int);
void     unregister_all_peers(void);

/* ------------------------------------------- Funktionen in src/l3vc.c */

void     flex_route_query(char *);

/*---------------------------------------- Funktionen in src/l7showl3.c */

void ccpnod(void);
void ccpdest(void);
void nrr2usr(NRRLIST *, char);
void ccprou(void);

/*---------------------------------------------- Funktionen in src/l4.c */

void    initl4(void);
void    l4tx(void);
void    l4rx(NODE *, NODE *, MBHEAD *);
BOOLEAN l4istome(char *, char *);
void    l4rest(void);
void    trasrv(void);
void    newcir(void);
void    discir(void);
BOOLEAN itocir(BOOLEAN, MBHEAD *);
MBHEAD  *gennhd(void);
void    l3tol4(NODE *);
#ifdef CONL3LOCAL
void    ackcir(CIRBLK *cp);
void    bsycir(void);
#endif
#ifdef L4KILL
UWORD   KillL4(char *, char *);
#endif /* L4KILL */

/*------------------------------------------ Funktionen in src/l7time.c */
void    timsrv(void);
void    beacsv(void);

/*---------------------------------------------- Funktionen in src/l7.c */

void    initl7(void);
void    l2tol7(WORD, void *, WORD);
void    disusr(UID);
BOOLEAN fmlink(BOOLEAN, MBHEAD *);
void    seteom(MBHEAD *);
BOOLEAN send_msg(BOOLEAN, MBHEAD *);
BOOLEAN new_user(BOOLEAN, UID);
BOOLEAN itousr(UID, UID, BOOLEAN, MBHEAD *);
UID     g_uid(void *, UBYTE);
UBYTE   g_utyp(UID);
void   *g_ulink(UID);
void    resptc(UID);
void    clrptc(UID);

/*------------------------------------------- Funktionen in src/l7ccp.c */

void    l7rx(void);
void    l7tx(void);
void    ccpbea(void);
void    ccpread(char *);
void    ccpedi(void);
void    ccpload(void);
void    ccpsys(void);
void    ccpuse(void);
void    viaput(LINKTYP, UID, MBHEAD *);
void    ccptim(void);
void    ccpclr(void);
void    ccpsta(void);
void    ccptst(void);
void    ccpver(void);
void    ccplnk(void);
void    ccpquit(void);
void    ccpecho(void);
void    ccptalk(void);
void    ccpmail(void);
void    ccpdxc(void);
#ifdef PADDLE
void    ccppaddle(void);
#endif

/*------------------------------------------ Funktionen in src/l7cmds.c */

#ifdef ALIASCMD
void    ccpalias(void);
void    clean_aliaslist(void);
#endif
void    ccpstart(void);
void    ccppar(void);
void    ccpres(void);
void    ccpprompt(void);
#if defined(MC68302) || defined(__WIN32__)
void    ccpdelete(void);
void    ccpcopy(void);
void    ccpdir(void);
#endif /* WIN32 */
#ifndef MC68302
void    ccpshell(void);
#endif
void    ccpport(void);
void    ccphelp(void);
void    ccpsusp(void);
void    ccpreadb(void);
void    ccpesc(void);
void    ccpkill(void);
void    ccpsave(void);
void    ccptrace(void);
void    ccprun(void);
void    ccpdcd(void);
#ifdef BUFFER_DEBUG
void    ccpbuf(void);
#endif

/*------------------------------------------ Funktionen in src/l7conn.c */

BOOLEAN isheard(char *, DEST *);
void    ccpcon(char *);
void    ccpcq(void);
void    gateway(void);
BOOLEAN conn_ok(const char *);

/*------------------------------------------ Funktionen in src/l7host.c */

void    init_host(void);
void    exit_host(void);
void    hostsv(void);
BOOLEAN runbatch(char *);
void    hostim(void);
BOOLEAN hstreq(void);
void    hstout(void);
BOOLEAN itohst(BOOLEAN, MBHEAD *);
void    blieco(int);
void    bliloe(void);
void    bputbs(void);
void    hstcmd(MBHEAD *);
void    hstcon(char);
void    hstdis(void);
void    hstsen(BOOLEAN);
void    hputid(char *);
void    hputc(char);
void    hputcc(char);
void    hputby(UBYTE);
void    hputbt(time_t *);
void    hprintf(const char *, ...);
void    hputs(const char *);
void    hputmb(MBHEAD *);
void    xprintf(const char *, ...);

/*--------------------------------------------------------CVS_CVSD.C*/

void    send_proto    __ARGS((const char *modul, const char *format __DOTS));
void    send_proto_to __ARGS((int channel,char *modul, const char *format __DOTS));

char    *personalmanager __ARGS((WORD, CONNECTION *, char *));

/*--------------------------------------------------------CVS_SERV.C*/

void    convers_init __ARGS((void));
void    conversd __ARGS((void));
BOOLEAN convers_input __ARGS((MBHEAD *));
void    convers_output __ARGS((void));
void    ccpcvs __ARGS((void));

/*---------------------------------------------------------- CVS_CMDS.C */

void    bye_command2 __ARGS((CONNECTION *cp, char *reason));

/*---------------------------------------- Funktionen in src/l7hstcmd.c */

void    Bcmd(void);             /* Anzahl der Runden / Sekunde          */
void    Ccmd(void);             /* An Console connecten                 */
void    Dcmd(void);             /* Von Console disconnecten             */
void    Ecmd(void);             /* Ausgabe auf File umleiten            */
void    Gcmd(void);             /* Hostmode-Poll                        */
void    Icmd(void);             /* Rufzeichen, Alias, Login-Passwort    */
void    Jcmd(void);             /* JHOST ein/aus                        */
void    Kcmd(void);             /* Datum + Uhrzeit anzeigen             */
void    Lcmd(void);             /* Status der Hostkanaele abfragen      */
void    Mcmd(void);             /* Monitor einstellen                   */
void    Qcmd(void);             /* Programm verlassen: QUIT             */
void    Rcmd(void);             /* Token-Recoveries anzeigen j/n        */
void    Scmd(void);             /* Hostkanal waehlen                    */
void    Tcmd(void);             /* Token-Ring-Baudrate einstellen       */
void    Vcmd(void);             /* Version abfragen                     */
void    Ycmd(void);             /* Connect zum Host erlaubt j/n         */
void    extcmd(void);           /* erweiterte Befehle                   */
void    hmputr(unsigned);
void    rspini(unsigned);
void    rspsuc(void);
void    rsperr(int);

/*------------------------------------------ Funktionen in src/l7moni.c */

void    monitor(MBHEAD *);
void    moncmd(MBHEAD *, MONBUF *, char *, WORD);

/*----------------------------------------- Funktionen in src/l7utils.c */

void     invcal(void);
void     msgfrm(LINKTYP, UID, char *);
void     puttfu(const char *);
void     putmsg(const char *);
MBHEAD  *putals(const char *);
MBHEAD  *getmbp(void);
void     putuse(LINKTYP, UID, MBHEAD *);
void     putcvsu(USRBLK *, MBHEAD *);
void     putdil(const char *, MBHEAD *);
void     putid(const char *,MBHEAD *);
void     putalt(char *,MBHEAD *);
void     putcal(char *,MBHEAD *);
void     puttim(time_t *, MBHEAD *);
void     putnum(int,MBHEAD *);
void     putlong(ULONG, BOOLEAN, MBHEAD *);
void     putstr(const char *, MBHEAD *);
void     putprintf(MBHEAD *, const char *, ...);
void     putspa(WORD, MBHEAD *);
TRILLIAN getdig(WORD *, char **, BOOLEAN, char *);
TRILLIAN getcal(WORD *, char **, BOOLEAN ,char *);
BOOLEAN  getport(WORD *, char **, WORD *);
TRILLIAN getide(WORD *, char **, char *);
UWORD    nxtnum(WORD *, char **);
LONG     nxtlong(WORD *, char **);
TRILLIAN fvalca(char *);
TRILLIAN valcal(char *);
BOOLEAN  ismemr(void);
char    *calofs(LINKTYP, UID);
BOOLEAN  getlin(MBHEAD *);
BOOLEAN  issyso(void);
BOOLEAN  skipsp(WORD *, char **);
WORD     getparam(WORD *, char **, WORD, WORD, WORD);
BOOLEAN  nextspace(WORD *, char **);
void     prompt(MBHEAD *);
void     prompt2str(MBHEAD *, char *);
void     putide(char *, MBHEAD *);
void     putkbytes(ULONG, MBHEAD *);
void     invmsg(void);
void     dump_beacn(MBHEAD *);
void     dump_convc(MBHEAD *);
void     dump_links(MBHEAD *);
void     dump_parms(MBHEAD *);
void     dump_ports(MBHEAD *);
void     dump_suspd(MBHEAD *);
#ifdef IPROUTE
void     dump_ipr(MBHEAD *);
#endif
#ifdef KERNELIF
void     dump_kernel(MBHEAD *);
#endif
#ifdef AX25IP
void     dump_ax25ip(MBHEAD *);
#endif
void     dump_divrs(MBHEAD *);
void     save_parms(void);
void     notify(int, const char * __DOTS);
BOOLEAN  is_down_suspended(char *, int);
BOOLEAN  is_suspended(USRBLK *);
void     init_crctab(void);
BOOLEAN  read_txt(void);
BOOLEAN  do_file(char *);
BOOLEAN  extern_command(void);
TRILLIAN intern_command(COMAND *);
void     inv_cmd(void);
void     program_load(MBHEAD *);
void     start_autobin(void);
void     get_password(void);
void     out_ctext(char*, MBHEAD*);
int      userport(USRBLK *);
void     send_ctext(void);
void     send_async_response(USRBLK *, const char *, const char *);
void     ccp_par(const char *, PARAM *, int);
void     ccp_call(char *);
int      ptc_p_max(void *, UBYTE);
BOOLEAN  is_iplink(void *, UBYTE);
void     load_text(void);

/*------------------------------------------------ Funktionen in FILE.C */

BOOLEAN save_configuration(void);
BOOLEAN save_stat(void);
BOOLEAN load_configuration(void);
BOOLEAN load_stat(void);
void    addslash(char *);
#ifdef PC
BOOLEAN good_file_name (const char *);
#endif

/*------------------------------------------- Funktionen in src/graph.c */

#ifdef GRAPH
void    ccpgraph(void);
void    graphclear(void);
void    graph_timer(void);
BOOLEAN save_graph(void);
BOOLEAN load_graph(void);
#endif
#ifdef PORTGRAPH
void     graph_actual_trxpeak(TPORTGRAPHELEMENT *, BOOLEAN);
#endif

/*-------------------------------------------------- Funktionen in MH.C */

void    init_mh(void);
void    exit_mh(void);
void    save_mh(void);
void    ccpl2mh(void);
void    ccpl3mh(void);
BOOLEAN mhprm(char *, WORD, char *);

MHEARD *mh_lookup(MHTAB *, char *);
#ifdef MH_LISTE
MHEARD *mh_lookup_port(MHTAB *, char *, UWORD, int txrx);
#else
MHEARD *mh_lookup_port(MHTAB *, char *, UWORD);
#endif
void mh_update(MHTAB *, MHEARD *, char *, UWORD);
void mh_clear(MHEARD *);
MHEARD *mh_add(MHTAB *);

/*----------------------------------------------- Funktionen in 16550.C */

void clear_rs232(int);
void close_rs232(int);
void rts_off(int);
void rts_on(int);
int setbaud (int, int);
void setstopbits(int, int);
int rs232_in(int);
void rs232_out(int, int);
int rs232_read(int, UBYTE *, int);
void rs232_write(int, UBYTE *, UBYTE *);
int rs232_in_status(int);
int rs232_out_status(int);
void init_rs232(void);
#if defined (__DOS16__) || defined (__GO32__)
void read_envcom(char *, int *, int *, int *);
int  open_rs232(int, int, int, int, int);
#endif
#ifdef ST
void read_envdev(char *, char *,char *);
int  open_rs232(char *);
#endif
#ifdef MC68302
int  open_rs232(int);
#endif

/*----------------------------------------------- Funktionen in TIMER.C */

void    init_timer(void);
void    exit_timer(void);

/*--------------------------------------------- Funktionen von MEM386.C */

#if defined (__DOS16__)
void    done_umb(void);
void    alloc_umb(void);
#endif

/*------------------------------------------ Funktionen in src/pacsat.c */

void    ccpbox(void);
void    ccppacsat(void);
LONG    getdiskfree (char *);

/*------------------------------------------ Funktionen src/pacserv.c */

void filesystem_init(void);
void new_file(char *);
void pacsat_init(void);
void pacsrv(void);
void l7_to_pacsat(void);

/*------------------------------------------ Funktionen in linux/kernelif.c */
#ifdef KERNELIF
void    ccpkif(void);
#endif

void route_age(void);


#ifdef AXIPR_HTML
void SetHTML(int, char *, PEER *, BOOLEAN);
#endif /* AXIPR_HTML */

#ifdef HOSTMYCALL
void ccpmyhost(void);                            /* Consolen-Mycall setzen. */
#endif

#ifdef SYSOPPASSWD
void ccppasswd(void);                            /* Sysop-Passwort aendern. */
#endif /* SYSOPPASSWD */

#ifdef BEACON_STATUS
char *convers_user(char *cuser);
void bake(void);
#endif

#ifdef IPOLL_FRAME
void    sdipoll(void);
#endif /* IPOLL_FRAME */

#ifdef LINKSMOD_LOCALMOD
BOOLEAN CheckLocalLink(const char *); /* Pruefe auf LOCAL-Links "L-""L#"*/
#endif /* LINKSMOD_LOCALMOD */

#ifdef  CONNECTMOD_SET_NODE
void    SetMyNode(char *);                   /* Einstiegsknoten setzen. */
#endif /* CONNECTMOD_SET_NODE */

#ifdef PORT_SUSPEND
BOOLEAN is_port_suspended(USRBLK *u_block);
#endif

#ifdef CONNECTTIME             /* Connect-Zeit in Flexnet-Stil ausgeben */
char *ConnectTime(unsigned long);
#endif /* CONNECTTIME */

/* End of include/function.h */
