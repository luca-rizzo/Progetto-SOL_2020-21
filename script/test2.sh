#!/bin/bash
CLIENT=./bin/client 
WFILE=./test/test2/fileDaScrivere
RDIR=./test/test2/fileLetti
SOCKPATH=./tmp/csSock

echo "Avvio 3 client, il primo invierà tanti file di piccole dimensioni, il secondo file di dimensioni maggiori, il terzo legge tutti i file dal server" 
echo "Client 1 salva file di piccole dimensioni" #tutti i file entreranno nel server--->nessuna rimozione
${CLIENT} -p -f ${SOCKPATH} -w ${WFILE}/filePiccoli -t 200 
echo "Client 2 salva file di grandi dimensioni" #i file si espelleranno a vicenda per essere salvati nel server; un file sarà troppo grande per essere memorizzato
${CLIENT} -p -f ${SOCKPATH} -w ${WFILE}/fileGrandi -t 200 
echo "Client 3 legge tutti i file del server"
${CLIENT} -p -f ${SOCKPATH} -R -d ${RDIR}/readNFiles -t 200 


