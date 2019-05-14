Installationshinweise zu TNN 1.79 :
===================================

Diese Anleitung richtet sich in erster Linie an Linux-User, fuer DOS kann
ein geringfuegig anderes Vorgehen notwendig werden. Als Basisverzeichnis
dient hier in der Regel "c:\tnn".

Leider ist noch keine endgueltige Dokumentation verfuegbar, vieles ist aber
im Vergleich zur Version 1.78 gleich geblieben, Unterschiede und neue Features,
bzw. Kommandos, koennen im history-Verzeichnis nachgelesen werden, ansonsten
stellt die alte Dokumentation der Version 1.78 einen guten Leitfaden dar.

Unter GCC 3.x meldet der Compiler beim Uebersetzen des Convers einige Warnings,
diese koennen ignoriert werden.


Neuinstallation :
-----------------

1.) Archiv auspacken (z.B. in /usr/local/src/tnn) und in das Verzeichnis wechseln

Optional: Im Verzeichnis "include" die Datei "all.h" ansehen und ggf. Optionen
          aktivieren oder unerwuenschte Optionen deaktivieren (auskommentieren).

2.) "make baseinstall" ausfuehren

TNN kompiliert nun und wird anschliessend in das Verzeichnis "/usr/local/tnn"
installiert. Je nach Betriebssystem werden Beispieldateien fuer die
Konfiguration hinzugefuegt. Ein normales "make" erzeugt lediglich die
ausfuehrbaren Dateien von TNN und der extrenen Programme, es findet KEINE
Installation statt !

3.) "cd /usr/local/tnn"
3.) Die Konfigurationsdateien tnn.ini, tnn179.{tnb,pas} und ax25ip.cfg
    ansehen und ggf. anpassen (bei DOS nicht alle benoetigt)
4.) Dateizugriffsrechte ueberpruefen. TNN sollte root gehoeren, das Verzeichnis
    in dem sich TNN befindet, sollte auch NUR fuer root zugaenglich sein !
    Laeuft TNN nicht als root, so koennen einige Linuxfeatures nicht benutzt
    werden !!!
5.) TNN starten ("./tnn")
6.) Am Passwortprompt mit dem Konsolenpasswort aus der Datei tnn.pas einloggen
    (Standard: "Geheim")
7.) "<ESC> c <Enter>" verbindet mit der ersten Konsole

Jetzt ggf. noch im laufenden Betrieb Einstellungen vornehmen (IP, Baken,
Links, Convers etc.).

Optional:

A.) Mit dem Kommando "sp" an der TNN-Konsole die aktuelle Konfiguration
    speichern wenn alle Einstellungen gemacht sind
B.) Die neue Datei "parms.tnb" sichten, ist alles korrekt, so kann diese in
    "tnn179.tnb" umbenannt werden. Achtung: "parms.tnb" selber wird beim
    Systemstart NICHT ausgewertet !!!

In der Datei "startup.log" befinden sich die Reaktionen auf die Verarbeitung
der einzelnen Befehle der tnn179.tnb. Hier kann im Fehlerfall ueberprueft
werden, welche Befehle nicht akzeptiert wurden.


Update von TNN Version 1.78/1.79pre? :
--------------------------------------

1.) Archiv auspacken

Optional: Optionen in "all.h" ueberpruefen und ggf. anpassen

2.) "make" oder "make install" ausfuehren. Bei letzterem wird die Verzeichnis-
    struktur mit angelegt und NUR das "tnn"-Executeable kopiert.
3.) "tnn"-Executable ins TNN-Verzeichnis kopieren
4.) Optional: externe Programm in die entsprechenden Verzeichnisse kopieren

Alte Konfigurationsdateien (tnn178.tnb etc.) werden automatisch geupdated.
Auf Schreibberechtigungen des Users achten, der TNN dann startet, kann TNN die
neuen Dateien nicht erzeugen, so wird mit STANDARDEINSTELLUNGEN (!) gestartet.
Zur Sicherheit koennen die alten Dateien auch vor dem ersten Start von Hand
in die neuen Namen kopiert werden (tnn178.tnb -> tnn179.tnb etc.). Als weitere
Alternative bietet sich das "upd"-Utility an, welches nach dem Uebersetzen
ebenfalls im "bin"-Verzeichnis zu finden ist.


Update einer TNN-Version 1.77 oder aelter :
-------------------------------------------

Hier ist leider Handarbeit angesagt. Es sollte das Neuinstallations-Verfahren
durchgefuehrt werden, danach von Hand die Einstellungen (vor allem die PAR-
Sektion) uebertragen und die Bedeutung der Parameter pruefen. Alternativ kann
auch versucht werden, die alten .tnb-Dateien in die neue, laufende TNN mit dem
"run"-Kommando einzulesen ("run tnn177.tnb"), und anschliessend mittels "sp"
eine parms.tnb-Datei zur weiteren Anpassung erzeugt werden.


Noch Probleme und Fragen ? Hier kann geholfen werden :
------------------------------------------------------

* Im DL-Convers auf Kanal 170.
* Auf www.nordlink.org im "Forum", Rubrik "TNN".
