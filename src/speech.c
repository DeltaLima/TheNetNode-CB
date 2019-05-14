#include "tnn.h"

#ifdef SPEECH
#include "speech.h"

struct speech_msg
{
  char speech[20];
  char num1[255];
  char mode_num[8];
  char default_num[8];
  char default_mode_num[9];
  unsigned      mode;
  unsigned      default_mode;
};

struct speech_msg speech_tbl[MAXZEILEN+1];

static int sprachen  = EOF;
static int meldungen = TRUE;

char speech[15]          = "Englisch";
char speechpath[MAXPATH] = SPEECHPATH; /* Pfad fuer Sprachen */

static unsigned set_substitute_symbols(const char *speechbuf, char *mode);


/* Default Sprache laden. */
static void speech_default(void)
{
  struct
  {
    int nummer;
    int  mode;
    char mode_num[9];
    char meldung[255];
  }
  speech_tab[] = {

    { 0,  0x0000000,"00000000","Convers session terminated.\rEnter command. Type HELP for help.\r" },
    { 1,  0x0000000,"00000000","*** You are already in convers-mode.\r" },
    { 2,  0x0000000,"00000000","The Convers-Hostname is to long (max.9 indications)!\r" },
    { 3,  0x0050000,"00010000","Convers-Hostname successful changed in %s.\r" },
    { 4,  0x0000000,"00000000","Locked!\r" },
    { 5,  0x0050000,"00040000","%s%s@%s has gone away:\r    %s\r" },
    { 6,  0x0050000,"00030000","%s%s@%s is back again.\r" },
    { 7,  0x0050020,"00020010","%s%s made you a channel operator for channel %d\r" },
    { 8,  0x0050020,"00030010","%s%s@%s is now a channel operator for channel %d\r" },
    { 9,  0x0000020,"00000010","channel %d" },
    { 10, 0x0050000,"00050000","%s%s@%s on %s set personal text:\r    %s\r" },
    { 11, 0x0050000,"00040000","%s%s@%s on %s removed personal text.\r" },
    { 12, 0x0050000,"00050000","%s%s@%s on %s set channel topic:\r               %s\r" },
    { 13, 0x0050000,"00040000","%s%s@%s on %s removed channel topic.\r" },
    { 14, 0x0000000,"00000000","secret channel" },
    { 15, 0x0000000,"00000000","this invisible channel" },
    { 16, 0x0050000,"00060000","%s%s:%s@%s on %s set personal text:\r    %s\r" },
    { 17, 0x0050000,"00050000","%s%s:%s@%s left %s.\r" },
    { 18, 0x0050000,"00040000","%s%s@%s left %s.\r" },
    { 19, 0x0050000,"00050000","%s%s@%s left %s (%s).\r" },
    { 20, 0x0050000,"00050000","%s%s:%s@%s joined %s\r" },
    { 21, 0x0050000,"00040000","%s%s@%s joined %s\r" },
    { 22, 0x0050000,"00030000","%s%s is away: %s" },
    { 23, 0x0050000,"00020000","%sYour messages are ignored by %s" },
    { 24, 0x0050020,"00030010","\r\007\007%sMessage from %s...\rPlease join %s channel %d.\r\007\007\r" },
    { 25, 0x0050000,"00040000","%s%s Invitation sent to %s @ %s." },
    { 26, 0x0050000,"00020000","%sUser %s is already on this channel." },
    { 27, 0x0000000,"00000000","*** Unknown command '/" },
    { 28, 0x0000000,"00000000","'. Type /HELP for Help.\r" },
    { 29, 0x0000000,"00000000","*** You are away, aren't you ? :-)\r" },
    { 30, 0x0000000,"00000000","*** This is a moderated channel. Only channel operators may write.\r" },
    { 31 ,0x0000000,"00000000","*** Queried user left channel.\r" },
    { 32, 0x0050000,"00010000","%sYou are marked as being away.\r" },
    { 33, 0x0050000,"00010000","%sYou are no longer marked as being away.\r" },
    { 34, 0x0050000,"00010000","%sActually you were marked as being here :-)\r" },
    { 35, 0x0050000,"00010000","*** Beep mode %sabled\r" },
    { 36, 0x0050020,"00010020","%sYou are talking to channel %d. There are %d users.\r" },
    { 37, 0x0050000,"00020000","*** current Topic by: %s (%s):\r               " },
    { 38, 0x0000000,"00000000","*** Also attached:" },
    { 39, 0x0000020,"00000010","channel %d (alone)" },
    { 40, 0x0000020,"00000020","channel %d (%d users)" },
    { 41, 0x0050020,"00010010","%sChannel number must be in the range 0..%d.\r" },
    { 42, 0x0050020,"00010010","%sChannel %d is already default.\r" },
    { 43, 0x0050020,"00010010","%sYou need an invitation to join the privat channel %d.\r" },
    { 44, 0x0050000,"00030000","%s%s@%s try to join your privat channel." },
    { 45, 0x0050020,"00010010","%scannot join channel %d, no more space.\r" },
    { 46, 0x0050020,"00010010","%sYou are now talking to channel %d" },
    { 47, 0x0000000,"00000000","You're alone.\r" },
    { 48, 0x0000020,"00000010","There are %d users.\r" },
    { 49, 0x0050000,"00020000","*** Charset in/out is %s/%s.\r" },
    { 50, 0x0050000,"00020000","Unknown charset: '%s'. You may use one of them:\r%s***\r" },
    { 51, 0x0050000,"00020000","*** Charset in/out set to %s/%s.\r" },
    { 52, 0x0000000,"00000000","Help on this command not yet implemented ...\rWrite it - or try another one :-)\r" },
    { 53, 0x0000000,"00000000","No such command...\r" },
    { 54, 0x0050000,"00010000","*** no route to %s\r" },
    { 55, 0x0050020,"00010010","%sDefault channel is now %d.\r" },
    { 56, 0x0050020,"00010010","%sLeft channel %d.\r" },
    { 57 ,0x0050020,"00010010","%sYou were not on channel %d.\r" },
    { 58, 0x0050000,"00010000","*** Reguest sent to %s.\r" },
    { 59, 0x0000000,"00000000","You must be an operator to set a new links\r" },
    { 60, 0x0000000,"00000000","Link table full !\r" },
    { 61, 0x0000000,"00000000","Argument error !\r" },
    { 62, 0x0050020,"00010010","%sYou have not joined channel %d.\r" },
    { 63, 0x0050000,"00020000","%sNo such user: %s.\r" },
    { 64 ,0x0050000,"00030000","%s%s is away: %s\r" },
    { 65, 0x0000000,"00000000","*** non existing channel !\r" },
    { 66, 0x0000000,"00000000","*** no modes on channel 0 !\r" },
    { 67, 0x0000000,"00000000","*** You are not on operator !\r" },
    { 68, 0x0050000,"00020000","%s @ %s PingPong-Release %5.5s (TNN) - Type /HELP for help.\r" },
    { 69, 0x0050020,"00010010","%sYou need an invitation to join channel %d.\r" },
    { 70, 0x0050000,"00030000","%s%s@%s try to join your privat channel." },
    { 71, 0x0000020,"00000010","*** You created a new channel %d.\r" },
    { 72, 0x0050000,"00010000","*** Personal text and date set.\rHello, %s\r" },
    { 73, 0x0000000,"00000000","*** Please set your personal text. ( /H PERS )" },
    { 74, 0x0050000,"00010000","%sYou are notified if one of the following users sign on/off:\r" },
    { 75, 0x0050000,"00010000","%sYou filter the messages of the following users:\r" },
    { 76, 0x0050000,"00010000","*** %s is online.\r" },
    { 77, 0x0000000,"00000000","and data saved.\r" },
    { 78, 0x0000000,"00000000","*** No personal text save.\r" },
    { 79, 0x0000000,"00000000","deleted.\r" },
    { 80, 0x0000000,"00000000","and data set.\r" },
    { 81, 0x0050000,"00010000","*** Prompting mode %sabled\r" },
    { 82, 0x0000000,"00000000","en" },
    { 83, 0x0000000,"00000000","dis" },
    { 84, 0x0000000,"00000000","You must be an operator to restart!\r" },
    { 85 ,0x0050000,"00010000","Link to %s delocked.\r" },
    { 86, 0x0050000,"00010000","%sNo such user: %.20s.\r" },
    { 87, 0x0050000,"00020000","%sStarting privat conversation with %s.\r" },
    { 88 ,0x0050000,"00020000","%sEnding privat conversation with %s.\r" },
    { 89, 0x0050020,"00030010","%sChannel topic on channel %d removed from %s (%s).\r" },
    { 90, 0x0050020,"00030010","%sChannel topic set on channel %d from %s (%s).\r" },
    { 91, 0x0050020,"00030010","%sCurrent channel topic on channel %d from %s (%s) is\r               " },
    { 92, 0x0050020,"00010010","%sNo current channel topic on channel %d.\r" },
    { 93, 0x0050020,"00010010","%sChannel channel %d non existent.\r" },
    { 94, 0x0050000,"00030000","*** %s@%s is up for %s\r" },
    { 95, 0x0050000,"00010000","*** Verbose mode %sabled\r" },
    { 96 ,0x0000000,"00000000","  This conversd implementation was originally written by Dieter Deyke\r  <deyke@mdddhd.fc.hp.com>. It was modified and maintained up to version\r  3.11 by Fred Baumgarten, dc6iq." },
    { 97, 0x0000000,"00000000"," This implementation is partly rewritten,\r  enhanced and maintained by Odo Roscher <dl1xao@db0hhb.#hh.deu.eu>\r  for TheNetNode and Xnet.\r" },
    { 98, 0x0000000,"00000000"," Idle Personal\r" },
    { 99 ,0x0000000,"00000000","Login State\r" },
    { 100,0x0000000,"00000000","Login Personal\r" },
    { 101,0x0000000,"00000000","(here)" },
    { 102,0x0000000,"00000000","\r        Away: " },
    { 103,0x0000000,"00000000"," (since " },
    { 104,0x0000000,"00000000"," (AWAY)" },
    { 105,0x0000000,"00000000","\r        Last Activity: " },
    { 106,0x0000020,"00000010","*** Current screen width is %d\r" },
    { 107,0x0000000,"00000000","*** Range 32 to 255\r" },
    { 108,0x0000020,"00000010","*** Screen width set to %d\r" },
    { 109,0x0050000,"00010000","*** Urgent message from operator (%s):\r    " },
    { 110,0x0050000,"00010000","*** Links at %s" },
    { 111,0x0000000,"00000000","free" },
    { 112,0x0050000,"00010000","*** Nickname set to: %s.\r" },
    { 113,0x0050000,"00020000","none login possible!\ncall signal %s is logged in at the host %s!\n" },
    { 114,0x0000000,"00000000","*** your password was deleted!\r" },
    { 115,0x0000000,"00000000","*** no password set!\r" },
    { 116,0x0050000,"00010000","*** reads. your password:\r%s\r" },
    { 117,0x0000000,"00000000","*** it is not defined a password!\r" },
    { 118,0x0000000,"00000000","*** password was set!\r" },
    { 119,0x0000000,"00000000","*** password must contain at least 5 and/or maximally 80 indications!\r" },
    { 120,0x0000000,"00000000","*** is missing. to password!\r" },
    { 121,0x0000000,"00000000","*** Syntax: pass <shows |again|deletes> [new password]\r" },
    { 122,0x0000000,"00000000","*** password is correct!\r" },
    { 123,0x0000000,"00000000","*** password is wrong!\r" },
    { 124,0x0050000,"00030000","*** current Topic by: %s (%s):\r               %s\r" },
    { 125,0x0050020,"00010010","\r*** %d user on channel%s ***\r" },
    { 126,0x7000000,"01000000","  Average: %lu" },
    { 127,0x0000000,"00000000","\r%7s00  02  04  06  08  10  12  14  16  18  20  22\r%9s01  03  05  07  09  11  13  15  17  19  21  23 Hour\r\r" },
    { 128,0x0000000,"00000000","\r%7s0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1\r%7s0 6 2 8 0 6 2 8 0 6 2 8 0 6 2 8 0 6 2 8 0 6 2 8 0 6 2 8 h\r%7sMonday  Tuesday Wednes. Thursd. Friday  Saturd. Sunday\r\r" },
    { 129,0x0000000,"00000000"," Elap. time\r%6s-3600 -3240 -2880 -2520 -2160 -1800 -1440 -1080 -720  -360  0 Seconds\r\r" },
    { 130,0x0600000,"00100000","> DISCONNECT: Too many non-DAMA Polls (%u) !!\r" },
    { 131,0x0600000,"00200000","WARNING: non-DAMA Poll #%u, Disconnect after %u !!\r" },
    { 132,0x0000000,"00000000","Graph cleared!\r" },
    { 133,0x0000000,"00000000","SYSTEMGRAPH:\r" },
    { 134,0x0000000,"00000000","(G)raph (H)our (B)aud\r        (D)ay  (C)ircuits\r        (W)eek (F)ree buffers\r               (L)2-Links\r               (N)odes\r               (R)ounds\r               (*) All\r" },
    { 135,0x0000000,"00000000","PORTGRAPH:\r(G)raph <PortNr> (H)our (I)nfo frames\r                 (D)ay  (R)eject frames\r                 (W)eek (F)rmr frames\r                        (S)abm frames\r                        dis(C) frames\r" },
    { 136,0x0000000,"00000000","                        d(M) frames\r                        (*) All\r" },
    { 137,0x0000000,"00000000","\r*** from DAMA-Master " },
    { 138,0x0050000,"00020000","%s<>%s broken" },
    { 139,0x0000000,"00000000","You are suspended.\r" },
    { 140,0x0000000,"00000000","\r- Aborted -\r\r" },
    { 141,0x0000000,"00000000","Sri, no text available!\r" },
    { 142,0x0000000,"00000000","CLI failed!\r" },
    { 143,0x0000000,"00000000","Waiting for AUTOBIN-Transfer...\r" },
    { 144,0x0000000,"00000000","\rL2 - User:\rPo SrcCall   DstCall   LS  Rx Tx Tr SRTT    RxB     TxB    Baud   ConTime Pr Da\r" },
    { 145,0x0000000,"00000000","\rL4 - User:\rCall      Node       S  Rx  Tx Tr Win SRTT     RxB     TxB    Baud   ConTime\r" },
    { 146,0x0000000,"00000000","free" },
    { 147,0x0000000,"00000000","free" },
    { 148,0x0000000,"00000000","\rHost-User:\rCH Call      F     NT    RX    TX    ST     RxB     TxB    Baud   ConTime\r-------------------------------------------------------------------------\r" },
    { 149,0x0000000,"00000000","free" },
    { 150,0x0000000,"00000000","\r System Statistics: " },
    { 151,0x0000000,"00000000","\r           Startup: " },
    { 152,0x0000000,"00000000","\r\r Port-Statistics:\r\r              Links      RxB      TxB   RxBaud   TxBaud  RxOver TxOver\r" },
    { 153,0x0000000,"00000000","\rTotal = " },
    { 154,0x0000000,"00000000"," Bytes\r" },
    { 155,0x0000000,"00000000", "\r Error-Statistics:\r\r                RxID  RxLen  RxCtl Resets\r" },
    { 156,0x0000000,"00000000","\rLink-Statistics:\r" },
    { 157,0x0000000,"00000000","\rLink to " },
    { 158,0x0000000,"00000000"," via " },
    { 159,0x0000000,"00000000","\rFrames:      I         UI         RR       REJ       RNR  SABM/UA  DISC/DM FRMR\r" },
    { 160,0x0000000,"00000000","Bytes:   Total       Info     Header  Overhead      %I     %RR    %REJ    %RNR\r" },
    { 161,0x0000000,"00000000","  TQual:" },
    { 162,0x0000000,"00000000","\rIP-Gateway-Statistics:\r\r" },
    { 163,0x0000000,"00000000","free" },
    { 164,0x0000000,"00000000","free" },
    { 165,0x0000000,"00000000","Warning - Port not active.\r" },
    { 166,0x0000000,"00000000","Invalid callsign.\r" },
    { 167,0x0000000,"00000000","Invalid ident.\r" },
    { 168,0x0000000,"00000000","free" },
    { 169,0x0000000,"00000000","free" },
    { 170,0x0000000,"00000000","Type-Port--Alias:Call------Route--------------Infotext------------\r" },
    { 171,0x0000000,"00000000","No such User!\r" },
    { 172,0x0000000,"00000000","Das Sysop Passwort wurde geaendert!\r" },
    { 173,0x0000000,"00000000","Fehler: Das Passwort muss 80 Zeichen enthalten!\r" },
    { 174,0x0050000,"00010000","editing>%s\rEnter text. End with '.' in a new line.\r" },
    { 175,0x0000000,"00000000","free" },
    { 176,0x0000000,"00000000","\r                       (min)    (now)    (max)\r        Rounds/sec: %8lu %8lu %8lu\r" },
    { 177,0x0000000,"00000000","      Free Buffers: %8u %8u %8u\rOverall Throughput:%18lu %8lu Baud\r   Active L2-Links:%18u %8u\r   Active Circuits:%18u %8u\r      Active Nodes:%18u %8u\r" },
    { 178,0x0000000,"00000000","\r    Active Telnets:%18u %8u\r" },
    { 179,0x7000000,"01000000","\r      Buffer usage: %lu%%" },
    { 180,0x7000000,"01000000","\r      Network Heap: %lu Bytes" },
    { 181,0x7000000,"01000000","\r          CPU load: %lu%%" },
    { 182,0x0000000,"00000000","TX:      Once:%11lu  Repeated:%10lu  IQual:" },
    { 183,0x0050020,"00010030","     Copyright by NORD><LINK, free for non-commercial usage.\r         See www.nordlink.org for further information.\r  This version compiled for %d Ports, %d L2-Links and %d Circuits.\r       %s\r" },
    { 184,0x0050000,"00010000","You are now talking with %s. Leave this mode with /q\r" },
    { 185,0x0050000,"00020000","Msg from %s (use TALK to reply): %s\r" },
    { 186,0x0050000,"00010000","Consolen MyCall:%s\r" },
    { 187,0x0000000,"00000000","Invalid filename!\r" },
    { 188,0x0000000,"00000000","EDIT/LOAD in use by other Sysop\r" },
    { 189,0x0000000,"00000000","Open error!\r" },
    { 190,0x0000000,"00000000","File error!\r" },
    { 191,0x0000000,"00000000","Statistic-table cleared!\r" },
    { 192,0x0000000,"00000000","\rInvalid Port\r" },
    { 193,0x0000000,"00000000","\rPort is disabled\r" },
    { 194,0x0000000,"00000000","No Text\r" },
    { 195,0x0000000,"00000000","Invalid Call!\r" },
    { 196,0x0000000,"00000000","Msg sent\r" },
    { 197,0x0000000,"00000000","No User\r" },
    { 198,0x0000000,"00000000","No Arguments\r" },
    { 199,0x0000000,"00000000","No mailbox!\r" },
    { 200,0x0000000,"00000000","No DX-cluster!\r" },
    { 201,0x0600000,"00100000","Alias too long, max. %u characters.\r" },
    { 202,0x0600000,"00100000","Command too long, max. %u characters.\r" },
    { 203,0x0050000,"00010000","can't create %s !\r" },
    { 204,0x0050000,"00010000","can't open %s !\r" },
    { 205,0x7000000,"01000000","%lu Bytes free\r" },
    { 206,0x0050000,"00010000","\r*** Msg from Sysop: %s ***\r" },
    { 207,0x0000000,"00000000","%3u link(s) disconnected.\r" },
    { 208,0x0050000,"00010000","%s.TNB saved...\r" },
    { 209,0x0600000,"00100000","Fehler: Tracen auf Port %u ist nicht erlaubt!!!\r" },
    { 210,0x0600000,"00100000","Fehler: Port %u ist nicht aktiv!!!\r" },
    { 211,0x0000020,"00000010","Warning: max. trace level of this TNN version is %d\r" },
    { 212,0x7000000,"03000000","\rAllBuffer : %lu (%lu) a %lu Bytes\r" },
    { 213,0x7000020,"01000010","Wrong Owner: Buffer:%lu Wert:%d\r" },
    { 214,0x7000000,"02000000","FreeBuffer: %lu (%lu)\r" },
    { 215,0x7000000,"01000000","Errors : %lu\r" },
    { 216,0x0000000,"00000000","Currently defined aliasses:\r" },
    { 217,0x0000000,"00000000","No aliasses defined\r" },
    { 218,0x0000000,"00000000","Alias stored.\r" },
    { 219,0x0000000,"00000000","Can't store, no memory.\r" },
    { 220,0x0000000,"00000000","Alias deleted.\r" },
    { 221,0x0000000,"00000000","No such alias defined, can't delete.\r" },
    { 222,0x0000000,"00000000"," not deleted!\r" },
    { 223,0x0000000,"00000000"," deleted.\r" },
    { 224,0x0000000,"00000000"," is restricted " },
    { 225,0x0000000,"00000000","to Access denied\r" },
    { 226,0x0000000,"00000000","to level-2 access\r" },
    { 227,0x0000000,"00000000","to max " },
    { 228,0x0000000,"00000000"," simultanous connections\r" },
    { 229,0x0000000,"00000000","from using Port " },
    { 230,0x0000000,"00000000","File has no length!\r" },
    { 231,0x0000000,"00000000","File not found!\r" },
    { 232,0x0000000,"00000000","No L2-link found.\r" },
    { 233,0x0000000,"00000000","Fehler: Keine Portangabe oder ungueltiger Port! (Beispiel: MO S 1).\r        Weitere Hilfe mit HELP MO .\r" },
    { 234,0x0000000,"00000000","Fehler: Sie haben keine optionen angegeben! (Beispiel:MO S 1).\r        Weitere Hilfe mit HELP MO .\r" },
    { 235,0x0000000,"00000000","free" },
    { 236,0x0000000,"00000000","Invalid program!\r" },
    { 237,0x0000000,"00000000","Invalid Hostcommand\r" },
    { 238,0x0050000,"00020000","%s\rNode / User unknown! Please specify port, if %s is a User:" },
    { 239,0x0000000,"00000000"," (Telnetport is suspended!)" },
    { 240,0x0000000,"00000000"," (Httpdport is suspended!)" },
    { 241,0x0050000,"00010000","Welcome to %s" },
    { 242,0x0000000,"00000000","* CHANNEL NOT CONNECTED *\007\n" },
    { 243,0x0000000,"00000000","CONNECTED to " },
    { 244,0x0000000,"00000000","DISCONNECTED fm " },
    { 245,0x0000000,"00000000","CONNECT REQUEST fm " },
    { 246,0x0000000,"00000000","INVALID COMMAND\r" },
    { 247,0x0050020,"00010010","Laenge = %d; Call = %s\n" },
    { 248,0x0600000,"00100000","\r      receive %u" },
    { 249,0x0600000,"00100000","   send %u   unacked 0" },
    { 250,0x0600000,"00100000","INVALID VALUE: %u" },
    { 251,0x0000000,"00000000","not available" },
    { 252,0x0000000,"00000000","CHANNEL NOT CONNECTED" },
    { 253,0x0000000,"00000000","[FRAME EMPTY?!?]" },
    { 254,0x0000000,"00000000","[FRAME TOO LONG]" },
    { 255,0x0000000,"00000000","INVALID COMMAND: " },
    { 256,0x0000000,"00000000","INVALID EXTENDED COMMAND: " },
    { 257,0x0000000,"00000000","\rERROR : Port or Call invalid !\r" },
    { 258,0x0000000,"00000000","ARP-Table of " },
    { 259,0x0000000,"00000000","\rDestination      P Interface  Callsign  Digi                    Mode Timer\r" },
    { 260,0x0000000,"00000000","My IP address: " },
    { 261,0x0000000,"00000000"," IP address : " },
    { 262,0x0000000,"00000000","No route to " },
    { 263,0x0000000,"00000000","IP-Routes of " },
    { 264,0x0000000,"00000000","\rDestination------Len--Flags-Interface--Gateway----------Metric----\r" },
    { 265,0x0000000,"00000000","Invalid IP address!\r" },
    { 266,0x0000000,"00000000","\rERROR : Invalid IP-Adresse !\r" },
    { 267,0x0600000,"00200000","\rINP: frame error, len = %u, left = %u\r" },
    { 268,0x0600000,"00100000","\r[AX25 Fragment; %u Frame(s) to follow - original PID %X]\r" },
    { 269,0x0600000,"00100000","\r[AX25 Fragment; %u Frame(s) to follow]\r" },
    { 270,0x0000000,"00000000"," (INVALID ALIAS!)\r" },
    { 271,0x0000000,"00000000"," (INVALID IP ADDRESS!)\r" },
    { 272,0x0050000,"00010000","\rAll Nodes over %s to be routing to show:\r" },
    { 273,0x0000000,"00000000","free" },
    { 274,0x0000000,"00000000","free" },
    { 275,0x0000000,"00000000","Call does not stand in the links List!\r" },
    { 276,0x0000000,"00000000","Invalid command. Try HELP.\r" },
    { 277,0x0000000,"00000000","Commandline too long.." },
    { 278,0x0000000,"00000000"," table full\r" },
    { 279,0x0000000,"00000000","Programm wird beendet!\n" },
    { 280,0x0000000,"00000000","Date     Time  Port" },
    { 281,0x0000000,"00000000","                Rx         Tx    Call\n" },
    { 282,0x0000000,"00000000","       RX         TX    Call       RX-Rej  TX-Rej    DAMA\n" },
    { 283,0x0000000,"00000000","Sri, can't open tempfile...\r" },
    { 284,0x0000000,"00000000","%24.24s %6.6s: rejected (mailbox)\n" },
    { 285,0x7000001,"01000002","%lX-%lX (%lu Messages)\r" },
    { 286,0x0650000,"00110000","BROADCAST enabled on port %u (%s).\r" },
    { 287,0x0000000,"00000000","Server call: " },
    { 288,0x0000000,"00000000","\rMessage pool: " },
    { 289,0x0000000,"00000000","Empty\r" },
    { 290,0x0000000,"00000000","PACSAT BOX-Call is now " },
    { 291,0x0000000,"00000000","Filesystem reloaded\r" },
    { 292,0x0000000,"00000000","CRC Error\n" },
    { 293,0x0050000,"00010000","language %s cannot be loaded .\r" },
    { 294,0x0050000,"00010000","language is adjusted to %s .\r" },
    { 295,0x0050020,"00020010","error in language file %s line %d !!!\n %s\n" },
    { 296,0x0000000,"00000000","the following languages found:\r" },
    { 297,0x0000000,"00000000","it no language files found.\r" },
    { 298,0x0000020,"00000010","My LogLevel is %d\r" },
    { 299,0x0000000,"00000000","errors: Log level worth from 0 to 4!\r" },
    { 300,0x0000020,"00000020","(%d/%d)\rAlias-:Call------IP--------------Mode-Port--PMode---Time----RX-----TX---Online-\r" },
    { 301,0x0600000,"00100000","I am listening on UDP-port %u\r" },
    { 302,0x0050000,"00010000","AX25IP : cannot create ip raw-socket: %s\n" },
    { 303,0x0000000,"00000000","AX25IP : cannot set non-blocking I/O on ip raw-socket\n" },
    { 304,0x0050000,"00010000","AX25IP : cannot set SO_KEEPALIVE for ip : %s\n" },
    { 305,0x0050000,"00010000","AX25IP : cannot set IPTOS_THROUGHPUT for ip : %s\n" },
    { 306,0x0000020,"00000010","Fehler: startWinsock, fehler code: %d\n" },
    { 307,0x0050000,"00010000","AX25IP : cannot create socket: %s\n" },
    { 308,0x0000000,"00000000","AX25IP : cannot set non-blocking I/O on udp socket\n" },
    { 309,0x0050000,"00010000","AX25IP : cannot set SO_KEEPALIVE for udp : %s\n" },
    { 310,0x0050000,"00010000","AX25IP : cannot set IPTOS_THROUGHPUT for udp : %s\n" },
    { 311,0x0050000,"00010000","AX25IP : cannot bind udp socket: %s\n" },
    { 312,0x0000000,"00000000","Routing table is full; entry ignored.\n" },
    { 313,0x0000000,"00000000","Invalid IP address\r" },
    { 314,0x0000000,"00000000","Invalid Mode\r" },
    { 315,0x0000000,"00000000","Invalid Parameter\r" },
    { 316,0x0000000,"00000000","AX25IP-Port nicht eingeschaltet!!!\n" },
    { 317,0x0000000,"00000000","UDP-Port not valid, not changed !!!\r" },
    { 318,0x0000000,"00000000","ERROR: Changing UDP-Port failed, Port not changed !!!\n" },
    { 319,0x0000000,"00000000","UDP-Port successfully changed\r" },
    { 320,0x0050020,"00010020","errors in language file %s.txt, \n maximum line length of %d exceeded (last line %d)!!!\n" },
    { 321,0x0050000,"00010000","invalid language (%s)!!!\r " },
    { 322,0x0050020,"00010020","errors in language file %s.txt, \n maximum line length of %d fell below (last line %d)!!!\n" },
    { 323,0x0050020,"00010010","error in language file %s line %d is too long !!!\n" },
    { 324,0x0000000,"00000000","errors: Log level worth from 0 to 3!\r" },
    { 325,0x0600000,"00100000","My TCP-Port: %u\r" },
    { 326,0x0000000,"00000000","TCP-Port haven not switched on!\r" },
    { 327,0x0000000,"00000000","TCP-Port not valid, not changed !!!\r" },
    { 328,0x0000000,"00000000","ERROR: Changing TCP-Port failed, Port not changed !!!\r" },
    { 329,0x0000000,"00000000","TCP-Port successfully changed\r" },
    { 330,0x0050000,"00010000","cannot create socket for IPX : %s\n" },
    { 331,0x0050000,"00010000","cannot bind IPX address: %s\n" },
    { 332,0x0000000,"00000000","Error: can't set non-blocking I/O on IPX socket" },
    { 333,0x0000000,"00000000","Error: Winsock cannot be initialized!\n" },
    { 334,0x0050000,"00010000","Error %s: Socket cannot be constructed!\r" },
    { 335,0x0050000,"00010000","Error %s: NonBlocking Io cannot be initialized!\n" },
    { 336,0x0050000,"00010000","Error %s: Bind cannot be initialized!\n" },
    { 337,0x0050000,"00010000","Error %s: listen cannot be initialized!\n" },
    { 338,0x0000000,"00000000","form_referer str too long" },
    { 339,0x0000000,"00000000","A route to this callsign is already set up, delete it first!!!\n" },
    { 340,0x0000000,"00000000","you are already announced as Sysop!\r" },
    { 341,0x0050000,"00010000","Convers runs since : %s\r" },
    { 342,0x0000000,"00000000","invalid quality!\r" },
    { 343,0x0050000,"00010000","*** Hostname is %s.\r" },
    { 344,0x0000000,"00000000","\rCan't send, Port is disabled" },
    { 345,0x0600000,"00100000","Timeout for dynamically learned routes is %u seconds\r" },
    { 346,0x0000020,"00000010","My HTML-Statistic is %d.\r" },
    { 347,0x0000000,"00000000","Error: HTML-Statistic outof range (0 - 1) !\r\r" },
    { 348,0x0000000,"00000000","Error: timeout out of range (0 - 86400) !\r" },
    { 349,0x0000000,"00000000"," (Port is suspended!)\r" },
    { 350,0x0000020,"00000010","\r *** altogether %d user on all channels. ***\r" },
  };

  int i;

  /* Da im Text-Editor Zeile 1 nicht 0 ist       */
  /* sondern 1, muessen wir auch bei 1 anfangen. */
  /* Ist zur besseren Uebersicht.                */
  meldungen = 1;

  for (i = 0; i < MAXZEILEN; i++)
  {
    strcpy(speech_tbl[meldungen].num1,speech_tab[i].meldung);
    speech_tbl[meldungen].default_mode     = speech_tab[i].mode;
    strcpy(speech_tbl[meldungen].default_mode_num , speech_tab[i].mode_num);
    meldungen++;
  }
}


static char *speech_check(char *quell, char *ziel)
{
  WORD laenge = strlen(quell);
  int  i;

  for (i = 0; i < sprachen; i++)
      {
      if (strncmp(quell,speech_tbl[i].speech,laenge) == FALSE)
      {
      sprintf(ziel,"%s",speech_tbl[i].speech);
      return(ziel);
     }
   }
  return(NULL);
}

void ccpspeech(void)
{
  MBHEAD *mbp;
  int     i;

  if (issyso() && skipsp(&clicnt, &clipoi))
  {
    clicnt--;
    strlwr(clipoi);
    if (speech_check(clipoi,speech) == NULL)
        {
        mbp = putals("Sprachauswahl:\r");
        putprintf(mbp,"Ungueltige Sprache (%s)!!!\r",clipoi);
        prompt(mbp);
        seteom(mbp);
        return;
       }
    else
        if (speech_load(speech))
            {
            mbp = putals("Sprachauswahl:\r");
            putprintf(mbp,"Sprache ist auf %s eingestellt.\r",speech);
            prompt(mbp);
            seteom(mbp);
            return;
            }
        else
            {
            speech_default();
            mbp = putals("Sprachauswahl:\r");
            speech_check(clipoi,clipoi);
            putprintf(mbp,"Sprache %s kann nicht geladen werden.\r",clipoi);
            prompt(mbp);
            seteom(mbp);
            return;
        }
    }
  mbp = putals("Sprachauswahl:\r");
  speech_init();
  if (sprachen > 0 && *clipoi == NUL)
    putstr("Folgende Sprachen wurden gefunden:\r",mbp);
  else
    {
      if (sprachen == 0)
        putstr("Es wurden keine Sprachdateien gefunden\r",mbp);
    }

    if (*clipoi == NUL)
        {
        for (i = 0; i < sprachen; i++)
            {
             putprintf(mbp,"%s\r",speech_tbl[i].speech);
            }
        }

  putprintf(mbp,"Sprache ist auf %s eingestellt.\r",speech);
  prompt(mbp);
  seteom(mbp);
}
void dump_speech(MBHEAD *mbp)
{
  putstr(";\r; Sprache\r;\r", mbp);
  putprintf(mbp,"SPEECH %s\r;\r",speech);
}

static char *lese_meldung(char *buffer,FILE *fp)
{
  char buf[320];
  int  i = 1;
  int  j = 0;

  strcpy(buf,buffer);

  if(buffer[0]=='"')
      while(buf)
          {
           while( (buf[i] == 0x1b || buf[i] >= 0x20) &&
                (buf[i]!='"' || (i && buf[i-1]=='\\')) &&
                 buf[i]!='\n' && i < 255)
                 {
                 if(buf[i]=='\\')
               { i++;

           switch(buf[i])
               {
               case 'n':  buffer[j]='\n'; break;
               case 'r':  buffer[j]='\r'; break;
               case '"':  buffer[j]='"' ; break;
               case 'a':  buffer[j]='\a'; break;
               case '\\': buffer[j]='\\'; break;
               }
              }
           else
           buffer[j] = buf[i];
          j++;
          i++;
         }
      buffer[j] = 0;
      return(buffer);
     }
return(FALSE);
}

int speech_load(char *bufspeech)
{
  FILE *fp;
  char  speechcfg[255];
  char buffer[255];
  char mode_num[8] = "0000000";

  strcpy(speechcfg,speechpath);
  strcat(speechcfg,bufspeech);
  strcat(speechcfg,".txt");
  meldungen = 1;
  if ((fp = xfopen(speechcfg,"rt")) == NULL)
      return(TRUE);

  {
      while(!feof(fp))
          {
          if (meldungen > MAXZEILEN)
              {
              printf("Fehler in Sprachdatei %s, maximale Zeilenlaenge von %d ueberschritten !!!\r",speech,MAXZEILEN);
              fclose(fp);
              speech_default();
              return(FALSE);
             }

          fgets(buffer,255,fp);
          lese_meldung(buffer,fp);
          if (strlen(buffer) >= 254)
          {
            printf("Fehler in Sprachdatei %s Zeile %d ist zu lang !!!\r",speech,meldungen);
            fclose(fp);
            return(FALSE);
                  }
          strcpy(speech_tbl[meldungen].num1, buffer);
          strcpy(mode_num,"00000000");
          speech_tbl[meldungen].mode = set_substitute_symbols(speech_tbl[meldungen].num1,mode_num);
          strcpy(speech_tbl[meldungen].mode_num        ,mode_num);

          if ((strncmp(speech_tbl[meldungen].default_mode_num,speech_tbl[meldungen].mode_num,7) == FALSE)
                  && (speech_tbl[meldungen].mode == speech_tbl[meldungen].default_mode))
                  {
                  meldungen++;
                  }
              else
                  {
                  printf("Fehler in Sprachdatei %s Zeile %d !!!",speech,meldungen);
                  fclose(fp);
                  return(FALSE);
                  }
          }
      fclose(fp);
      }
      if (meldungen-1 != MAXZEILEN)
          {
          printf("Fehler in Sprachdatei %s.txt, maximale Zeilenlaenge von %d unterschritten (letzte Melung %d)!!!\n",speech,MAXZEILEN,meldungen);
          return(FALSE);
          }
  return(TRUE);
}

void speech_init(void)
{
  struct ffblk dir;
  char speechname[255];
  long erg,hfile;
  char path[255];
  char *loc;
  int i;

  if (sprachen == EOF)
      speech_default();

  sprachen = FALSE;
  strcpy(path,speechpath);
  addslash(path);
  strcat(path,"*.txt");
  sprachen = 0;

  hfile = xfindfirst(path,&dir,0);
  erg = (hfile == -1);
  while (erg == 0)
  {
    strcpy(speechname,dir.ff_name);
    loc = strstr(speechname,".");
    i = loc-speechname;
    speechname[i] = '\0';
    strcpy(speech_tbl[sprachen].speech,speechname);
    sprachen++;
    erg = xfindnext(&dir);
  }
}

char *speech_message(int msgnum)
{
  if (*speech_tbl[msgnum].num1 == NUL)
  printf("Fehler in Sprachdatei %s Zeile %d !!!",speech,msgnum);

return speech_tbl[msgnum].num1;
}

static unsigned set_substitute_symbols(const char *speechbuf, char *mode)
{
  unsigned char     a[11]  = "1234567890";
  unsigned num    = 0x0;
  int      num_lX = 0;
  int      num_d  = 0;
  int      num_ld = 0;
  int      num_f  = 0;
  unsigned num_s  = 0;
  int      num_u  = 0;
  int      num_lu = 0;

  while(*speechbuf)
  {
  if (strncmp(speechbuf,"%",1) == 0)
  {
    speechbuf++;
     switch (*speechbuf)
     {
         case 'd':
             num |= speech_mode_d;
             mode[speech_mode_d_num] = a[num_d];
             num_d++;
             break;

         case 'l':
             {
             speechbuf++;
             if (strncmp(speechbuf,"d",1) == FALSE)
                 {
                 num |= speech_mode_ld;
                 mode[speech_mode_ld_num] = a[num_ld];
                 num_ld++;

                     break;
                 }
             if (strncmp(speechbuf,"u",1) == FALSE)
                 {
                     num |= speech_mode_lu;
                     mode[speech_mode_lu_num] = a[num_lu];
                     num_lu++;
                     break;
                 }
             if (strncmp(speechbuf,"X",1) == FALSE)
                 {
                     num |= speech_mode_lX;
                     mode[speech_mode_lX_num] = a[num_lX];
                     num_lX++;
                     break;
                 }
             }

         case 'f':
             num |= speech_mode_f;
             mode[speech_mode_f_num] = a[num_f];
             num_f++;
             break;

         case 's':
             num |= speech_mode_s;
             mode[speech_mode_s_num] = a[num_s];
             num_s++;
             break;

         case 'u':
             num |= speech_mode_u;
             mode[speech_mode_u_num] = a[num_u];
             num_u++;
             break;
         }
      }
  speechbuf++;
 }

return(num);
}

#endif
