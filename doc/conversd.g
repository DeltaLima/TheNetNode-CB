Commands may be abbreviated. Commands are:
/Away [text]            Mark user as being away
/ALl text               Send text to all users
/Beep                   Toggle Beep-Mode
/Channel n              Switch to channel n
/CHARset [in [out]]     Change terminal emulation (default ansi)
/Destinations           List reachable ping-pong hosts
/Filter [calls]         Set calls you want to filter
/Help [command]         Print help information
/Invite user            Invite user to join your channel
/IMsg user text         Send Text to all on channel except user
/Links [args]           List or setup links
/LISt                   List all channels and topics
/LEave [channel]        Leaves specified or default channel
/Msg user|#channel text Send message to a user or joined channel
/ME text                Write action to channel
/MOde [channel] options Set channel options
/NIck nickname          Set your (nick)name
/NONick                 Delete your (nick)name
/NOTify [calls]         Send notice if one of calls signs on
/Personal [text]        Set personal description
/PRompt abcd            Prompts a=query b=std c=ctrl-g d=ctrl-h
/Quit                   Terminate the convers session
/QUEry [user]           Start/End a private conversation
/Topic [#chan] [text]   Set channel topic. Text=@ removes topic
/UPtime                 How long is this conversd up ?
/Verbose                Toggles verbose Flag
/VERSion                Show version information
/Who [N|*|A|L|U|@]      List users and their channel numbers
/WIdth [value]          Set/show terminal width
@@ALL
If you are in /query mode, a text written with /all at the front will
be displayed as a normal convers text written to all users on this channel.
@@AWAY
/away sets an away-text for the other users. called with no additional
arguments, you are marked as being back again.
@@BEEP
(/beep /bell)
The beep command toggles a warning bell (^G) being sent before every message
coming from another user. This command really is a subset of the /prompt
command, see there.
@@CHAR
With this command, you can tell the convers deamon which type of umlauts you
prefer to use. The Syntax is /char [intype [outtype]]. For example, if you
work on an Atari st you could say: "/char atari atari". If you use a pc and
like to write your own umlauts in TeX style, "/char tex pc" may work. Use
"/char ?" to see a list. Play a bit around with this feature !
Special Thanks to Thommy, <dl9sau@zedat. fu-berlin.de> (Internet mail)
<dl9sau@db0sao.ampr.org> (AmPR-Net mail) who wrote this nice feature.
Suggestions to this feature should be redirected to him :-)
Your char-setting will be stored when using "/pers", see there.
Please notice; use the correct charset for your program, not for your type
of computer.
@@DEST
(/destinations /hosts)
All Pingpong hosts in the network being connected to each other are listed.
The numbers shown in the list are response times in seconds. Use "/d #" to
see another form of this list.
@@EXCL
(/exclude /imsg /iwrite)
The exclude command is the opposite of the /msg command. You can send
messages to all users on the channel except the one given as the first
parameter. Internally these messages are sent as private messages to the
users, so links are flooded a bit more :-)
@@FAID
Welcome to the convers, select the right conversion for umlauts (/char),
set your personal description (/pers)
and change (/join) to another channel of your interest, if you want.
Whith /who you get an overview of the channels, more help with /help.
Have fun.
@@FILT
If you don't want to read texts from certain users, you can set a list
of calls. Messages from this calls are discarded for you. The syntax is
similar to "/notify". E.g. "/filter + dc1ik - db4ut" adds the user dc1ik
to the filter and removes db4ut from the list.
@@HELP
(/? /help)
The help command may be invoked with an additional parameter. I believe you
hacked in "help help" to see this, right ? :-). better try out help options
that are followed by an additional command.
@@INVI
An invitation tho the named user is sent. The invite is passed through all
convers nodes in the net. If the user is on another channel, he will be able
to join your private channel. If he is in the commandinterpreter on a node,
he will receive the invitation message. In the latter case he will be unable
to join a private channel directly.
@@JOIN
(/channel /join)
Join another channel. Unlike other other conversd implementations Ping-Pong-
conversd supports multiple channel joins the same time. Thus the old channel
is not left. You may do this by invoking "/leave".
@@LEAV
Invoking this command you will leave the actual channel or the specified
channel. If the given channel is your last existence in the convers channel,
you'll quit the whole program.
@@LINK
The current link state is displayed. In the standard version, host name,
link state, link round trip times, neighbour revision codes and state time
followed by time of next try and count of connect tries (on disconnected or
connecting links) are shown, connected lines print out their queue length
and total transmit statistics.
If you are sysop, you may also set up or remove links at runtime.
Connection-info will be shown in brackets additionally.
Syntax: /l [@ host]|[[+]|[-] host [port [via]]]
With "/l @host" you are able to get info from host (not all versions support
this function).
@@LIST
All channels, their topics (if set), flags (if some) and users are displayed.
@@ME
If you want to display an action you take, you might use this command. E.g.
entering the line "/me yawns" all connected users in this channel will see
a message like: "*** dc6iq yawns"
@@MODE
The mode command is one of the most complicated ones. It is invoked with
/mode <channel> <+|-><t|i|s|m|p|l|o<name>>.
  Flags may be a set of the following:
    t - channel topic may be set by channel operators only
    i - channel is invisible for users not on it
    s - channel is secrect, only its number not displayed
    m - channel is moderated, only channel-operators may talk
    p - channel is private, invitation needed to join here
    l - channel is local, no text forwarding to other nodes (NYI)
    o<name> - make <name> a channel operator

The plus indicates an option to be set, a dash resets the option.
A combination of + and - is allowed so the following command will work:
"/mode 69 -s+todc6iq". Channel 69 is no longer secret, but topics may be
set only by channel operators. In addition the user dc6iq becomes a channel
operator.
Without parameters, the current settings will be listed.
On channel 0 it is impossible to set modes!
@@MSG
(/msg /send /write)
Send a text to one special user or a joined channel. If the message should
be redirected to a channel, The command must be invoked in this syntax:
"/msg #<channel> <text>". The adressee (if user) can indicate the private
character of the message due to additional asterisks in his received lines.
a mesage written by dc6iq to dc1ik looks like this: "/m dc1ik This is a Test"
will be received by dc1ik like this: "<*dc6iq*>: This is a test".
@@NICK
You can set your nickname to be displayed in front of your callsign. The
nickname is forwarded to other convers hosts. Use "/nick @" or "/nonick"
to delete your nickname.
@@NONICK
If a nickname is set, it is deleted. (same as "/nick @")
@@NOTI
You are notified if one person out of the notification list appears on any
channel in this convers. E.g. "/notify + dc1ik" adds the user dc1ik to the
notification list, "/notify - db4ut" removes him from the list. The list is
initially treated as if a plus "+"-sign was given, multiple lists may be
specified. The following command would be legal and would result in adding
dc1ik, db4ut and dg3kcr to the list, while removing dc6iq and dh2paf:
"/notify dh2paf + dc1ik db4ut - dc6iq dh2paf +dg3kcr"
Removing non-members to the list is ignored.
@@PERS
(/note /personal)
A brief description can be set up, accessible to all users via the "/who"-
command. A typical Message looks like this: "/pers Fred, Buechig, JN49fb".
The description can be deleted with "/pers @".
The digi stores up to 118 chars of this description and set it automatically
when you enter convers (the settings of "/char" and "/width" will be stored
and set on login, too). Storing is also possible with simply "/pers".
@@PROM
The prompt command takes up to four arguments in one concatenated string.
Thus "/prompt abcd" will set up "a" for the query prompt, "b" for the
standard prompt. "d" is the character to remove the prompt, and "c" is
displayed whenever a text is sent to you. Normally you make "c" a control-G
(beep char) and "d" a backspace or delete character (control-h or 0x7f).
@@QUER
The username given as argument will be the only recipient for all further
texts. They are send as private messages (just like /m). With no optional
argument, the further texts go into the whole channel again.
@@QUIT
(/bye /exit /quit)
Passing this command to conversd, you will retire from the famous Ping-Pong
conversation. I hope you enjoyed this stuff :-)
@@TOPI
A brief description of the channel topic can be set. Other users can look
it up with the "/who quick" or "/list" commands. If the channel number is
omitted, the topic will be set to the current channel, else to the desired
one. If no #channel is given, the topic will appear on the current channel,
if specified, the topic on that channel will be set (only if youre logged
on to that channel). To clear a topic, just try to set a topic like "@" :-)
An empty topic command diplays the current's channel topic.
@@UPTI
The uptime command tells us how long this conversd is up and running.
@@VERB
The verbose flag is toggled. You will be flooded with information if you
enable this feature.
@@VERS
Print out version information to this software.
@@WHO
(/users /who)
This command shows the users and has multiple options:
 n [channel]  tabular form            (limitable to one channel)
 a [channel]  away-text listing       (limitable to one channel)
 l [channel]  long listing            (limitable to one channel)
 u users      detailed infos to specified users 
 * [channel]  listing of idle-times   (own or specified channel)
 @ host       to specified host limited tabular form
without option you get a brief listing ("/list").
@@WIDT
Tell conversd about your screen width. Then formatted messages will use the
entire screen. It is set to 80 by default.
Your width-setting will be stored when using "/pers", see there.
@@----
rev3.12c/20001003