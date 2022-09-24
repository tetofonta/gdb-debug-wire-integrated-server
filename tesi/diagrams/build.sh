#!/bin/bash

if [ ! -f plantuml.jar ]; then
    wget https://github.com/plantuml/plantuml/releases/download/v1.2022.7/plantuml-pdf-1.2022.7.jar -O plantuml.jar
fi

for file in *.puml; do
    java -jar plantuml.jar -tpdf ./$file -o ../images/
done