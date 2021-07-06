#!/bin/bash
CLIENT=./bin/client 
WFILE=./test/test1/fileDaScrivere
RDIR=./test/test1/fileLetti
SOCKPATH=./tmp/csSock

echo "Avvio 4 client, ciascuno dei quali testa alcune funzionalità"
echo "Client 1 testa -w"
${CLIENT}  -f ${SOCKPATH} -w ${WFILE}/programmiEsercitazioni -t 200 -p
echo "Client 2 testa -W"
${CLIENT}  -f ${SOCKPATH} -W ${WFILE}/mare.jpg,${WFILE}/prato.jpg -t 200 -p
echo "Client 3 testa -r e -c"
${CLIENT}  -f ${SOCKPATH} -r ${WFILE}/mare.jpg,${WFILE}/prato.jpg -d ${RDIR}/readFile -t 200 -c ${WFILE}/mare.jpg,${WFILE}/prato.jpg -p
echo "Client 4 testa -R"
${CLIENT}  -f ${SOCKPATH} -R -d ${RDIR}/readNFiles -t 200 -p
