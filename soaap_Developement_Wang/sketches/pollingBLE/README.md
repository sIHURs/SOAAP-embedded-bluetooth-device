# Hinweise zu den Sketches

## Allgemein
Diese Sketches sind zum Testen der Bibliotheken entwickelt worden. Dabei wurde versucht, die Testumgebung durch entsprechende Schalter zu kennzeichnen. Das Ziel ist, dass auch die spätere Zielversion in diesen Sketches enthalten ist und durch entsprechende Schalter extrahiert werden kann.

Folgende Compilerschalter sind zu setzen:
- Für den Master (SoaapBleMaster.ino): -DsmnDEFBYBUILD -DsmnNANOBLE33 -DsmnDEBUG
- Für die Slaves (SoaapBleSlave.ino): -DsmnDEFBYBUILD -DsmnNANOBLE33 -DSlaveACM1 -DsmnDEBUG

Diese Schalter werden in der Regel in der Projektumgebung gesetzt (bei Eclipse/Sloeber in Properties/Arduino/Compiler Options).

Mit -DSlaveACMx wird (zur Zeit) die Polling-Adresse x (1 bis 255) für den Slave angegeben. Es können also (zur Zeit) maximal 255 Slaves im Netzwerk betrieben werden.

## Vorwärtsreferenzen
Bei Eclipse/Sloeber werden die Vorwärtsreferenzen auf verzeigerte Funktionen (Aufrufe in der Zustandsmaschine) nach dem Reinigen des Projektes (Clean Project) automatisch angelegt (in der Datei sloeber.ino.cpp). Bei anderen IDEs muss dies in der Regel über eine extra Liste (z.B. in einer H-Datei) erledigt werden.
