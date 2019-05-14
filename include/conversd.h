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
/* File include/conversd.h (maintained by: DL1XAO)                      */
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

/*--------------------------------------------------------CVS_CVSD.C*/

#define GET_NXTLC 0
#define GET_ALL   1
#define GET_NXTCS 2

void appendformline __ARGS((CONNECTION *cp, char *prefix, char *text));
void appenddirect __ARGS((CONNECTION *cp, const char *string));
void appendstring __ARGS((CONNECTION *cp, const char *string));
void appendc __ARGS((CONNECTION *cp, const WORD n, const WORD ast));
void appendprompt __ARGS((CONNECTION *cp, const WORD ast));
void destroy_channel __ARGS((WORD number));
PERMLINK *permlink_of __ARGS((CONNECTION *cp));
void free_closed_connections __ARGS((void));
char *getarg __ARGS((char *line, WORD all));
char *ts __ARGS((time_t gmt));
void ts2 __ARGS((void));
char *ts3 __ARGS((time_t seconds, char *buffer));
char *ts4 __ARGS((time_t seconds));
void clear_locks __ARGS((void));
WORD count_user __ARGS((WORD channel));
#ifndef CONVNICK
void send_awaymsg __ARGS((char *name, char *host, time_t time, char *text));
#else
void send_awaymsg __ARGS((char *name, char *nick, char *host, time_t time, char *text));
#endif /* CONVNICK */
#ifndef L1IRC
void send_mode __ARGS((CHANNEL *chan));
void send_opermsg __ARGS((char *toname, char *hostname, char *fromname, WORD channel));
#else
void send_mode __ARGS((CHANNEL *chan, CONNECTION *, WORD oldflags));
void send_opermsg __ARGS((char *, char *, char *, WORD, int));
#endif /* L1IRC */
#ifndef CONVNICK
void send_persmsg __ARGS((char *name, char *host, WORD channel, char *text, time_t time));
void send_topic __ARGS((char *name, char *host, time_t time, WORD channel, char *text));
void send_user_change_msg __ARGS((char *name, char *host, WORD oldchannel, WORD newchannel, char *pers, time_t time));
#else
void send_persmsg __ARGS((char *name, char *nick, char *host, WORD channel, char *text, time_t time));
void send_topic __ARGS((char *name, char *nick, char *host, time_t time, WORD channel, char *text));
void send_uaddmsg __ARGS((CONNECTION *name));
void makeName __ARGS((CONNECTION*, char*));
void send_user_change_msg __ARGS((char *name, char *nick, char *host, WORD oldchannel, WORD newchannel, char *pers, time_t time));
#endif /* CONVNICK */
void send_msg_to_user __ARGS((char *fromname, char *toname, char *text));
void send_msg_to_channel __ARGS((char *fromname, WORD channel, char *text));
void send_invite_msg __ARGS((char *fromname, char *toname, WORD channel));
void update_destinations __ARGS((PERMLINK *p, char *name, time_t rtt, char *rev));
PERMLINK *update_permlinks __ARGS((char *name, CONNECTION *cp, WORD isperm));
void connect_permlinks __ARGS((void));
char *getflags __ARGS((WORD flag));
void Strcpy __ARGS((char *dst, char *src));
WORD Strcmp __ARGS((char *a, char *b));
void setstring __ARGS((char **adr, char *str, WORD max));

#ifdef CONV_CHECK_USER
CONNECTION *CheckUserCVS(char *);
#endif /* CONV_CHECK_USER */

/*--------------------------------------------------------CVS_SERV.C*/

CONNECTION *alloc_connection __ARGS((USRBLK *));
void free_connection __ARGS((CONNECTION *cp));
BOOLEAN invite_ccp __ARGS((char *, char *));
BOOLEAN connect_cvshost __ARGS((PERMLINK *));
void putcvsstr __ARGS((CONNECTION *, char *));
LONG queuelength __ARGS((CONNECTION *));

/*--------------------------------------------------------CVS_CMDS.C*/

void process_input __ARGS((CONNECTION *cp));
void bye_command __ARGS((CONNECTION *cp));

/*--------------------------------------------------------CVS_CVRT.C*/

WORD get_charset_by_name __ARGS((char *buf));
char *get_charset_by_ind __ARGS((WORD ind));
char *list_charsets __ARGS((void));
char *convertin __ARGS((WORD in, char *string));
char *convertout __ARGS((WORD out, char *string));

/* End of include/conversd.h */
