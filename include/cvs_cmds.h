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
/* File include/cvs_cmds.h (maintained by: DL1XAO)                      */
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

/*
 * This is Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@hpfcmdd.fc.hp.com>
 *
 * Modifications by Fred Baumgarten <dc6iq@insl1.etec.uni-karlsruhe.de>
 * $Revision: 3.12 $$Date: 1996/03/03 10:09:47 $
 *
 * Modifications by Oliver Kern <daa531@gmx.de>
 * $Revision: 3.13 $$Date: 17.08.2004 22:22 $
 */

/* zusammengefuehrtes USER.H und HOST.H, damit alle Kommandos samt
   Kommandotabelle mit statischen Funktionen auskommen                    */

#include "tnn.h"
#ifdef PPCONVERS
#include "conversd.h"
#define  ISO    0

extern char cnvinbuf[];

static CHANNEL *ins_channel __ARGS((WORD chan));
static BOOLEAN has_ChOp __ARGS((WORD chan));
static void disp_links __ARGS((CONNECTION *cp, char *user));
static void ed_list __ARGS((CONNECTION *cp, WORD which));
static WORD is_looped __ARGS((PERMLINK *l, char *host));
#ifdef CONVERS_NO_NAME_OK
#else
static BOOLEAN name_ok __ARGS((char *call));
#endif
static BOOLEAN host_ok __ARGS((char *call));

static void all_command __ARGS((CONNECTION *cp));
static void away_command __ARGS((CONNECTION *cp));
static void beep_command __ARGS((CONNECTION *cp));
static void channel_command __ARGS((CONNECTION *cp));
static void charset_command __ARGS((CONNECTION *cp));
static void cstat_command __ARGS((CONNECTION *cp));
static void filter_command __ARGS((CONNECTION *cp));
#ifndef L1IRC
static void help_command __ARGS((CONNECTION *cp));
#endif /* L1IRC */
#ifdef CONVERS_HOSTNAME
static void hostname_command __ARGS((CONNECTION *cp));
#endif
static void hosts_command __ARGS((CONNECTION *cp));
static void invite_command __ARGS((CONNECTION *cp));
static void imsg_command __ARGS((CONNECTION *cp));
#ifndef L1IRC
static void leave_command __ARGS((CONNECTION *cp));
static void links_command __ARGS((CONNECTION *cp));
#endif /* L1IRC */
static void list_command __ARGS((CONNECTION *cp));
#ifndef L1IRC
static void msg_command __ARGS((CONNECTION *cp));
#endif /* L1IRC */
static void me_command __ARGS((CONNECTION *cp));
static void mode_command __ARGS((CONNECTION *cp));
static void name_command __ARGS((CONNECTION *cp));
#ifdef CONVNICK
static void nickname_command __ARGS((CONNECTION *cp));
static void nonickname_command __ARGS((CONNECTION *cp));
#endif
static void notify_command __ARGS((CONNECTION *cp));
static void personal_command __ARGS((CONNECTION *cp));
static void prompt_command __ARGS((CONNECTION *cp));
static void query_command __ARGS((CONNECTION *cp));
static void restart_command __ARGS((CONNECTION *cp));
#ifdef CONVERS_SYSINFO
static void sysinfo_command __ARGS((CONNECTION *cp));
#endif
static void topic_command __ARGS((CONNECTION *cp));
static void uptime_command __ARGS((CONNECTION *cp));
static void verbose_command __ARGS((CONNECTION *cp));
static void version_command __ARGS((CONNECTION *cp));
static void width_command __ARGS((CONNECTION *cp));
static void who_command __ARGS((CONNECTION *cp));
static void wall_command __ARGS((CONNECTION *cp));
#ifdef CVS_ZAPPING
static void zap_command __ARGS((CONNECTION *cp));
#endif

static void h_away_command __ARGS((CONNECTION *cp));
static void h_cmsg_command __ARGS((CONNECTION *cp));
static void h_dest_command __ARGS((CONNECTION *cp));
static void h_host_command __ARGS((CONNECTION *cp));
static void h_invi_command __ARGS((CONNECTION *cp));
static void h_link_command __ARGS((CONNECTION *cp));
static void h_oper_command __ARGS((CONNECTION *cp));
static void h_ping_command __ARGS((CONNECTION *cp));
static void h_pong_command __ARGS((CONNECTION *cp));
static void h_rout_command __ARGS((CONNECTION *cp));
static void h_topi_command __ARGS((CONNECTION *cp));
#ifdef CONVNICK
static void h_uadd_command __ARGS((CONNECTION *cp));
#endif
static void h_udat_command __ARGS((CONNECTION *cp));
static void h_unknown_command __ARGS((CONNECTION *cp));
static void h_umsg_command __ARGS((CONNECTION *cp));
static void h_user_command __ARGS((CONNECTION *cp));

typedef struct cmdtable {
  char *name;
  void (*fnc)(CONNECTION *);
  char *help;
  WORD states;
} CMDTABLE;

static CMDTABLE cmdtable[] = {

  {"?",            help_command,       "HELP",   CM_USER},
  {"away",         away_command,       "AWAY",   CM_USER},
  {"action",       me_command,         "ME",     CM_USER},
  {"all",          all_command,        "ALL",    CM_USER},
  {"beep",         beep_command,       "BEEP",   CM_USER},
  {"bell",         beep_command,       "BEEP",   CM_USER},
  {"bye",          bye_command,        "QUIT",   CM_USER},
  {"channel",      channel_command,    "JOIN",   CM_USER},
  {"charset",      charset_command,    "CHAR",   CM_USER},
  {"cstat",        cstat_command,      NULL,     CM_UNKNOWN},
  {"destinations", hosts_command,      "DEST",   CM_USER},
  {"exit",         bye_command,        "QUIT",   CM_USER},
  {"exclude",      imsg_command,       "EXCL",   CM_USER},
  {"filter",       filter_command,     "FILT",   CM_USER},
  {"help",         help_command,       "HELP",   CM_USER},
#ifdef CONVERS_HOSTNAME
  {"hostname",     hostname_command,   "HOST",   CM_UNKNOWN},
#endif
  {"hosts",        hosts_command,      "DEST",   CM_USER},
  {"invite",       invite_command,     "INVI",   CM_USER},
  {"imsg",         imsg_command,       "EXCL",   CM_USER},
  {"iwrite",       imsg_command,       "EXCL",   CM_USER},
  {"join",         channel_command,    "JOIN",   CM_USER},
  {"links",        links_command,      "LINK",   CM_USER},
  {"leave",        leave_command,      "LEAV",   CM_USER},
  {"list",         list_command,       "LIST",   CM_USER},
  {"msg",          msg_command,        "MSG",    CM_USER},
  {"me",           me_command,         "ME",     CM_USER},
  {"mode",         mode_command,       "MODE",   CM_USER},
  {"name",         name_command,       NULL,     CM_UNKNOWN},
#ifdef CONVNICK
  {"nickname",     nickname_command,   "NICK",   CM_USER},
  {"nonickname",   nonickname_command, "NONICK", CM_USER},
#endif
  {"notify",       notify_command,     "NOTI",   CM_USER},
  {"note",         personal_command,   "PERS",   CM_USER},
  {"online",       who_command,        NULL,     CM_UNKNOWN},
  {"personal",     personal_command,   "PERS",   CM_USER},
  {"prompt",       prompt_command,     "PROM",   CM_USER},
  {"quit",         bye_command,        "QUIT",   CM_USER},
  {"query",        query_command,      "QUER",   CM_USER},
  {"restart",      restart_command,    NULL,     CM_USER},
  {"send",         msg_command,        "MSG",    CM_USER},
#ifdef CONVERS_SYSINFO
  {"sysinfo",      sysinfo_command,    "SYSI",   CM_USER},
#endif
  {"topic",        topic_command,      "TOPI",   CM_USER},
  {"users",        who_command,        "WHO",    CM_USER},
  {"uptime",       uptime_command,     "UPTI",   CM_USER},
  {"verbose",      verbose_command,    "VERB",   CM_USER},
  {"version",      version_command,    "VERS",   CM_USER},
  {"who",          who_command,        "WHO",    CM_USER},
  {"width",        width_command,      "WIDT",   CM_USER},
  {"wall",         wall_command,       NULL,     CM_USER},
  {"write",        msg_command,        "MSG",    CM_USER},
#ifdef CVS_ZAPPING
  {"zap",          zap_command,        NULL,     CM_USER},
#endif

  {"\377\200away", h_away_command,     NULL,     CM_HOST},
  {"\377\200cmsg", h_cmsg_command,     NULL,     CM_HOST},
  {"\377\200dest", h_dest_command,     NULL,     CM_HOST},
  {"\377\200host", h_host_command,     NULL,     CM_UNKNOWN},
  {"\377\200invi", h_invi_command,     NULL,     CM_HOST},
  {"\377\200link", h_link_command,     NULL,     CM_HOST},
  {"\377\200mode", mode_command,       NULL,     CM_HOST},
  {"\377\200oper", h_oper_command,     NULL,     CM_HOST},
  {"\377\200ping", h_ping_command,     NULL,     CM_HOST},
  {"\377\200pong", h_pong_command,     NULL,     CM_HOST},
  {"\377\200rout", h_rout_command,     NULL,     CM_HOST},
  {"\377\200topi", h_topi_command,     NULL,     CM_HOST},
#ifdef CONVNICK
  {"\377\200uadd", h_uadd_command,     NULL,     CM_HOST},
#endif
  {"\377\200udat", h_udat_command,     NULL,     CM_HOST},
  {"\377\200umsg", h_umsg_command,     NULL,     CM_HOST},
  {"\377\200user", h_user_command,     NULL,     CM_HOST},

  {NULL,           0,                  NULL,     0}
};

#endif
/* End of $RCSfile$ */
