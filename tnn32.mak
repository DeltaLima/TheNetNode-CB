# Makefile - tnn32
#
# Die .dsw Datei von VC wurde mit dsw2mak aus dem MinGW Paket uebersetzt
# und etwas angepasst. Folgende Configurationen koennen verwendet werden:
#  - Linux mit MinGW runtime
#  - Cygwin mit MinGW runtime
#  - MinGW mit MSYS
#
#
# Zu tun:
# - w32api header in beliebiges Verzeichnis kopieren
# - und patchen z.B.:
#
#      windef.h:     * folgende Zeilen auskommentieren:
#                        typedef unsigned char BYTE;
#                        typedef unsigned short WORD;
#
#      winnt.h:     * folgende Zeilen auskommentieren:
#                        typedef BYTE BOOLEAN,*PBOOLEAN;
#
#      ws2tcpip.h:  * folgende Zeilen auskommentieren:
#                        typedef int socklen_t;
#
# - dazu kommen noch diverse Abhaengigkeiten, aber da muss man
#   vorerst improvisieren
#
# - bei variable SYSHDR den Pfad zu den gepatchten w32api headern
#
#
#
# EXPERIMENTELL: Die w32api Header muessen nicht mehr gepatcht
# werden, wenn vor dem Compilieren folgendes ausgeführt wird:
# 
#   make -f tnn32.mak hardpatch
#   
# Da die Quelldateien hier komplett gepatcht werden, werden zur
# Sicherheit alle svn Verzeichnisse geloescht.
# 
# Wegen unterschiedlicher Syntax des sed Befehls, kann es bei
# einigen Systemen zu Fehlermeldungen kommen.
#
ifdef SYSHDR
CFLAGS=-isystem $(SYSHDR)
RCFLAGS=--include-dir $(SYSHDR)
endif
#
#
#
# Alles compilieren (mit geaenderten gcc namen):
#   make -f tnn32.mak CC=mingw32-gcc CXX=mingw32-g++ RC="windres -O COFF"
#
# Nur TNN32.EXE compilieren (mit geaendertem gcc prefix):
#   make -f tnn32.mak tnn PREFIX="i386-mingw32-"
#
# Pfad zu w32api headern angeben:
#   make -f tnn32.mak SYSHDR=/path/to/headers
#
# Fuer Debugging compilieren:
#   make -f tnn32.mak DEBUGGING=on
#
#
#
# ACHTUNG: evtl. CC, CXX und RC anpassen
#

#
# Hier ggf. noch die Namen zu den Executables korrigieren
#

ifeq ($(PREFIX),)
ifeq ($(CC),cc)
ifeq ($(CXX),g++)
ifeq ($(RC),)

ifeq (CYGWIN,$(findstring CYGWIN,$(shell uname -s)))
CC=gcc -mno-cygwin
CXX=g++ -mno-cygwin
RC=windres -O COFF
endif

ifeq (Linux,$(findstring Linux,$(shell uname -s)))
PREFIX=i586-mingw32msvc-
endif

ifeq (MINGW32,$(findstring MINGW32,$(shell uname -s)))
CC=mingw32-gcc
CXX=mingw32-g++
RC=windres -O COFF
endif

ifeq (,$(shell uname -s))
CC=mingw32-gcc
CXX=mingw32-g++
RC=windres -O COFF
endif

endif
endif
endif
endif


ifeq ($(CC),cc)
CC=$(PREFIX)gcc
endif

ifeq ($(CXX),g++)
CXX=$(PREFIX)g++
endif

ifeq ($(RC),)
RC=$(PREFIX)windres -O COFF
endif

# Linken mit C++
CXXFLAGS=$(CFLAGS)
LD=$(CXX) $(CXXFLAGS)
LDFLAGS=
LDFLAGS+=-Wl,--subsystem,console


# Include Dateien
CFLAGS+=-Iinclude -Ios/linux -Ios/win32 -Isrc
# vom DSW -> MAKEFILE Uebersetzer
CFLAGS+=-fexceptions -W
#wozu? CFLAGS+=-DWIN32 -D_CONSOLE -D_MBCS 
# "patches" (sonst vanessa - schlechte definition in der linux.h)
CFLAGS+=-Ui386
# debugging ein/aus
ifneq ($(DEBUGGING),on)
CFLAGS+=-O0
#wozu? CFLAGS+=-DNDEBUG
else
CFLAGS+=-O0 -g
#wozu? CFLAGS+=-D_DEBUG
endif

# Libraries
LIBS+=-lws2_32 -lwsock32
LIBS+=-lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32
LIBS+=-lshell32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32

BIN=os/win32/tnn32
TARGET=$(BIN)/tnn32.exe


.PHONY: all contrib tnn clean cleantnn cleancontrib
all: tnn contrib
clean: cleantnn cleancontrib
tnn: $(TARGET)
contrib: $(BIN)/help.exe $(BIN)/msg.exe $(BIN)/msy.exe $(BIN)/pfhadd.exe $(BIN)/top.exe


%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	
%.res: %.rc
	$(RC) $(CPPFLAGS) $(RCFLAGS) --include-dir os/win32/tnn32 -o $@ -i $<
	
RESSOURCENDATEIEN= \
	os/win32/tnn32/icon1.ico \
	os/win32/tnn32/tnn32.rc

WIN32= \
	os/win32/sys/dirent32.c \
	os/win32/sys/dirent32.h \
	os/win32/sys/fnmatch.c \
	os/win32/sys/fnmatch.h \
	os/win32/sys/strings.c \
	os/win32/sys/strings.h \
	os/win32/6pack.c \
	os/win32/6pack.h \
	os/win32/ax25ip.c \
	os/win32/ax25ip.h \
	os/win32/init.c \
	os/win32/l1attach.c \
	os/win32/l1win32.c \
	os/win32/ostcpip.c \
	os/win32/osipconv.c \
	os/win32/win32.c \
	os/win32/win32.h \
	os/linux/linclude.h

INCLUDE= \
	include/all.h \
	include/allmodif.h \
	include/conversd.h \
	include/cvs_cmds.h \
	include/function.h \
	include/global.h \
	include/host.h \
	include/icmp.h \
	include/ip.h \
	include/ipv.h \
	include/l1attach.h \
	include/l2.h \
	include/l2s.h \
	include/l3global.h \
	include/l3local.h \
	include/l3thenet.h \
	include/l4.h \
	include/l7.h \
	include/profiler.h \
	include/stat.h \
	include/system.h \
	include/tnn.h \
	include/typedef.h

SRC= \
	src/buffer.c \
	src/callstr.c \
	src/cvs_cmds.c \
	src/cvs_cvrt.c \
	src/cvs_cvsd.c \
	src/cvs_serv.c \
	src/file.c \
	src/global.c \
	src/graph.c \
	src/l1tcpip.c \
	src/l1httpd.c \
	src/l1ipconv.c \
	src/l1telnet.c \
	src/l2dama.c \
	src/l2misc.c \
	src/l2rx.c \
	src/l2stma.c \
	src/l2timer.c \
	src/l2tx.c \
	src/l3inp.c \
	src/l3ip.c \
	src/l3misc.c \
	src/l3nbr.c \
	src/l3netrom.c \
	src/l3thenet.c \
	src/l3rtt.c \
	src/l3tab.c \
	src/l3var.c \
	src/l3vc.c \
	src/l3sock.c \
	src/l3tcp.c \
	src/l4.c \
	src/l7.c \
	src/l7ccp.c \
	src/l7cmds.c \
	src/l7conn.c \
	src/l7host.c \
	src/l7hstcmd.c \
	src/l7ip.c \
	src/l7moni.c \
	src/l7showl3.c \
	src/l7time.c \
	src/l7utils.c \
	src/main.c \
	src/mh.c \
	src/pacsat.c \
	src/pacserv.c \
	src/profil.c \
	src/profiler.c \
	src/speech.c \
	src/version.c

SRCS=$(RESSOURCENDATEIEN) $(OS) $(LINUX) $(WIN32) $(INCLUDE) $(SRC)

OBJS=$(patsubst %.rc,%.res,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(SRCS))))

HELPSRCS= \
	contrib/onlhelp/oh.c

MSGSRCS= \
	os/win32/sys/dirent32.c \
	os/win32/sys/dirent32.h \
	os/win32/sys/fnmatch.c \
	os/win32/sys/fnmatch.h \
	contrib/msgmsy/msg_lin.c

MSYSRCS= \
	os/win32/sys/dirent32.c \
	os/win32/sys/dirent32.h \
	os/win32/sys/fnmatch.c \
	os/win32/sys/fnmatch.h \
	contrib/msgmsy/msy_lin.c

PACSRCS= \
	contrib/pfhadd/pfhadd.c

TOPSRCS= \
	contrib/top/top_gnu.c

HELPOBJS=$(patsubst %.rc,%.res,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(HELPSRCS)))))))
MSGOBJS=$(patsubst %.rc,%.res,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(MSGSRCS)))))))
MSYOBJS=$(patsubst %.rc,%.res,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(MSYSRCS)))))))
PACOBJS=$(patsubst %.rc,%.res,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(PACSRCS)))))))
TOPOBJS=$(patsubst %.rc,%.res,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(TOPSRCS)))))))


$(BIN):
ifneq ($(wildcard $(BIN)), $(BIN))
		echo Creating directory $(BIN)
		- mkdir $(BIN)
endif


$(TARGET): $(OBJS) $(BIN)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(BIN)/help.exe: $(HELPOBJS) $(BIN)
	$(LD) $(LDFLAGS) -o $@ $(HELPOBJS) $(LIBS)
	
$(BIN)/msg.exe: $(MSGOBJS) $(BIN)
	$(LD) $(LDFLAGS) -o $@ $(MSGOBJS) $(LIBS)
	
$(BIN)/msy.exe: $(MSYOBJS) $(BIN)
	$(LD) $(LDFLAGS) -o $@ $(MSYOBJS) $(LIBS)

$(BIN)/pfhadd.exe: $(PACOBJS) $(BIN)
	$(LD) $(LDFLAGS) -o $@ $(PACOBJS) $(LIBS)

$(BIN)/top.exe: $(TOPOBJS) $(BIN)
	$(LD) $(LDFLAGS) -o $@ $(TOPOBJS) $(LIBS)


cleantnn:
	-rm -f $(OBJS) $(TARGET)

cleancontrib:
	-rm -f $(HELPOBJS) $(BIN)/help.exe
	-rm -f $(MSGOBJS) $(BIN)/msg.exe
	-rm -f $(MSYOBJS) $(BIN)/msy.exe
	-rm -f $(PACOBJS) $(BIN)/pfhadd.exe
	-rm -f $(TOPOBJS) $(BIN)/top.exe

hardpatch:
	-echo "!! WARNING !! EXPERIMENTAL !! DO NOT USE THIS OPTION !! (too late now)"
	-find . \( -name '.svn' \) -exec rm -Rf {} \;
	-find . \( -name '*.c' -o -name '*.h' \) -exec sed -ri 's/(^|[() *,;:])BOOLEAN([() *,;:]|$$)/\1BOOLEANN\2/g' {} \;
	-find . \( -name '*.c' -o -name '*.h' \) -exec sed -ri 's/(^|[() *,;:])BYTE([() *,;:]|$$)/\1BYTEE\2/g' {} \;
	-find . \( -name '*.c' -o -name '*.h' \) -exec sed -ri 's/(^|[() *,;:])WORD([() *,;:]|$$)/\1WORDD\2/g' {} \;
	-find . \( -name '*.c' -o -name '*.h' \) -exec sed -ri 's/(^|[() *,;:])WORD([() *,;:]|$$)/\1WORDD\2/g' {} \;

