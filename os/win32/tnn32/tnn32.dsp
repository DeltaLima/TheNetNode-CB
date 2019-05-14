# Microsoft Developer Studio Project File - Name="tnn32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=tnn32 - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "tnn32.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "tnn32.mak" CFG="tnn32 - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "tnn32 - Win32 Release" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE "tnn32 - Win32 Debug" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tnn32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "tnn32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "tnn32 - Win32 Release"
# Name "tnn32 - Win32 Debug"
# Begin Group "include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\include\all.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\allmodif.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\conversd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cvs_cmds.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\function.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\global.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\host.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\icmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\ip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\ipv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l1attach.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l1httpd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l1ipconv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l1irc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l1tcpip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l1telnet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l2s.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l3global.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l3local.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l3sock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l3tcp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l3thenet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l4.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\l7.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\ostcpip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\profil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\profiler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\speech.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\stat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\system.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\tnn.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\typedef.h
# End Source File
# End Group
# Begin Group "os"

# PROP Default_Filter ""
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Group "sys"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sys\dirent32.c
# End Source File
# Begin Source File

SOURCE=..\sys\dirent32.h
# End Source File
# Begin Source File

SOURCE=..\sys\fnmatch.c
# End Source File
# Begin Source File

SOURCE=..\sys\fnmatch.h
# End Source File
# Begin Source File

SOURCE=..\sys\strings.c
# End Source File
# Begin Source File

SOURCE=..\sys\strings.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\6pack.c
# End Source File
# Begin Source File

SOURCE=..\6pack.h
# End Source File
# Begin Source File

SOURCE=..\ax25ip.c
# End Source File
# Begin Source File

SOURCE=..\ax25ip.h
# End Source File
# Begin Source File

SOURCE=..\init.c
# End Source File
# Begin Source File

SOURCE=..\l1attach.c
# End Source File
# Begin Source File

SOURCE=..\l1win32.c
# End Source File
# Begin Source File

SOURCE=..\osipconv.c
# End Source File
# Begin Source File

SOURCE=..\ostcpip.c
# End Source File
# Begin Source File

SOURCE=..\win32.c
# End Source File
# Begin Source File

SOURCE=..\win32.h
# End Source File
# Begin Source File

SOURCE=..\winclude.h
# End Source File
# End Group
# End Group
# Begin Group "src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\buffer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\callstr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\cvs_cmds.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\cvs_cvrt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\cvs_cvsd.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\cvs_serv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\file.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\global.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\graph.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l1httpd.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l1ipconv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l1irc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l1tcpip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l1telnet.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l2dama.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l2misc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l2rx.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l2stma.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l2timer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l2tx.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3inp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3ip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3misc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3nbr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3netrom.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3rtt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3sock.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3tab.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3tcp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3thenet.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3var.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l3vc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l4.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7ccp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7cmds.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7conn.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7host.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7hstcmd.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7ip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7moni.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7showl3.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7time.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\l7utils.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\main.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mh.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pacsat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pacserv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\profil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\profiler.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\speech.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\version.c
# End Source File
# End Group
# Begin Group "Ressourcen"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\tnn32.rc
# End Source File
# End Group
# End Target
# End Project
