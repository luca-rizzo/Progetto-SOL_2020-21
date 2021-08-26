# Progetto-SOL_2020-21
Progetto SOL anno 2020-2021.

Lo scopo del progetto è la realizzazione di un file server storage per la memorizzazione dei file in memoria
principale.
Lo sviluppo è stato guidato dalla realizzazione di 3 componenti interconnesse:
 Server: si occupa della memorizzazione dei file in memoria centrale e della gestione delle connessioni
e delle richieste da parte dei client.
 API: libreria di interazione con il file server. Si occupa di gestire tutto il protocollo di comunicazione
con il server. La chiamata di una funzione di tale libreria genererà una serie di interazioni, secondo
specifici protocolli, con il server, al fine di soddisfare la specifica della suddetta funzione.
 Client: si occupa di parsare la linea di comando e generare così una lista di richieste che saranno
tradotte in una serie di chiamate alle funzioni dell’API implementata.

Tutte le informazioni relative al progetto si trovano nel file Relazione.pdf
