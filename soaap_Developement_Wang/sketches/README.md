# Robert Patzke:
Die von mir eingefügten Sketches sind in der Regel über Schalter (#define) organisiert. Diese werden in zwei Stufen definiert:
1. In der Datei `environment.h` sind Eigenschaften verschiedener Arduino-Boards und Prozessoren berücksichtigt. Das war erforderlich wegen des direkten Zugriffs auf die Prozessoren, der von der Arduino-Umgebung nicht ohne weiteres unterstützt wird.
2. Die o.g. Datei wird ebenfalls gesteuert (damit man nicht verschiedene Dateien für die unterschiedlichen Projekte braucht). Ich mache das durch projektspezifische Festlegungen in der IDE. Zum Beispiel bei Eclipse/Sloeber in den Projekteigenschaften (Properties) unter Arduino -> Compile Options -> Append to C and C++ : `-DsmnDEFBYBUILD -DsmnNANOBLE33 -DsmnDEBUG`

