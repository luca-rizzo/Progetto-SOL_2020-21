#ifndef SERVER_H_
#define SERVER_H_

#include <conn.h>
#include <icl_hash.h>
#include <coda.h>
#include <util.h>
#include <threadpool.h>
#include <log.h>
/*
	Cartella salvataggio file di log
*/
#define LOGDIR "log/"

//*****TIPI DEFINITI*****//


/*
    Enumerazione per indicare stati possibili del server
*/
typedef enum{
	attivo, //server attivo con tutte le funzionalità disponibili
	rifiutaConnessioni, //in seguito ad un segnale di SIGHUP il server passa in uno stato in cui non accetta nuove connessioni
	termina //in seguito ad un segnale SIGINT/SIGQUIT il server termina il prima possibile
}server_mode;

/*
    Struct di configurazione del server 
*/
typedef struct{
    int maxNumeroFile; //numero massimo di file memorizzabile nel file storage server
    size_t maxDimBytes; //capienza massima in byte del file storage server
    int numThreadsWorker; //numero di threads worker
    char sockname[UNIX_PATH_MAX]; //nome del socket AF_UNIX al quale i client si connetteranno
    server_mode stato; //specifica lo stato del sistema
}config_server;

/*
    Struct che mantiene una coda di tutti i client connessi e una mutex per l'accesso mutuamente esclusivo
*/
typedef struct{
	t_coda* client; //coda che contiene tutti i client connessi
	pthread_mutex_t mtx; //mutex per l'accesso mutuamente esclusivo
}clientConnessi;

/*
    Struct che implementa un file nel nostro file storage server
*/
typedef struct{
    size_t dimBytes; //dimensione del file in bytes
    int scrittoriAttivi; //numero di scrittori attivi
    int lettoriAttivi; //numero di lettori attivi
    char* path; //path identificativo del file
    void* contenuto; //contenuto del file
    t_coda* fd; //insieme di fd associati a client che hanno aperto il file
    struct timespec tempoCreazione; //tempo della creazione del file
    t_op ultima_operazione_mod; //ultima operazione di modifica del conteuto del file (può essere una cr,wr,ap)

    pthread_mutex_t mtx; //mutex per l'accesso mutuamente esclusivo
    pthread_mutex_t ordering; //mutex per regolare l'accesso
    pthread_cond_t cond_Go; //variabile di condizione usata per la sospensione sia dei lettori che degli scrittori
}t_file;

/*
    Struct che implementa il file storage server
*/
typedef struct{
    icl_hash_t* storage; //tabella hash che mantiene tutti i riferimenti "path assoluto-file associato"
    int numeroFile; //numero di file presenti in un dato momento nel file storage server
    pthread_mutex_t mtx; //mutex per l'accesso in mutua esclusione
    size_t dimBytes; //dimensione in bytes in un dato momento nel file storage server
    int maxFile; //numero di file massimo memorizzato nel server
    size_t maxDimBytes; //dimensione massima in bytes raggiunta dal file storage server
    int nReplacement; //numero di volte in cui è stato avviato l'algoritmo di rimpiazzamento per individuare una vittima
}t_file_storage;

/*
	Struct che implementa il tipo di dati da passare al sigHandler
*/
typedef struct { 
    sigset_t     *set;           // set dei segnali da gestire (mascherati)
    int           signal_pipe;   // descrittore di scrittura di una pipe senza nome
} sigHandler_t;


/*
	Struct che implementa il tipo di dati da passare al generico worker thread
*/
typedef struct { 
	int idWorker; //id del worker
    int fd_daServire;//fd da servire
    int pipe;//pipe dove eventualmente scrivere quando ho servito la richiesta
} threadW_args;


//*****VARIABILI GLOBALI*****//


extern config_server config; //contiene le configurazioni del nostro file storage server
extern t_file_storage* file_storage; //struttura File Server condivisa
extern clientConnessi* clientAttivi; //struttura dati che tiene traccia del numero di client connessi e gli effettivi client connessi
extern t_log* logStr; //struttura dati usata per gestire file log

//*****FUNZIONI*****//


/*
    @funzione liberaFile
    @descrizione: permette di deallocare una struttura dati t_file
    @param val è ilk puntatore al file da deallocare
*/
void liberaFile (void* val);

/*
    @funzione espelliFile
    @descrizione: permette di individuare e di espellere il file "più vecchio" del file storage server(politica FIFO). 
    Funzione chiamata in seguito ad un capacity-miss dovuto ad operazioni di scrittura o quando ho raggiunto il numero massimo di file memorizzabili nel file storage server
    Da chiamare con la lock sulla struttura dati e una writelock su notToRemove
    @param notToRemove è il file che non voglio venga individuato come vittima e rimosso. Nelle scritture coincide con il file che voglio scrivere o a cui voglio appendere contenuto
    @param myid è l'id del thread che effettua la rimozione
    @param filesDaEspellere è la coda in cui inserire il file che ho rimosso dal mio file storage server se questo era stato modificato. Se filesDaEspellere==NULL il file rimosso verrà deallocato
    @return 0 in caso di successo, -1 in caso di fallimento
*/
int espelliFile(t_file* notToRemove, int myid, t_coda* filesDaEspellere);

/*
    @funzione startRead
    @descrizione: permette di acquisire mutua esclusione 'in lettura' ad un file
    @param file è il file su cui voglio acquisire muta esclusione
    @return 0 in caso di successo, -1 in caso di fallimento
*/


/*
	@funzione funcW
	@descrizione: funzione eseguita dal generico Worker del pool di thread
	@param arg è un puntatore agli argomenti che verranno usati dal worker thread per gestire una richiesta
*/
void funcW(void *arg);


int startRead(t_file* file);

/*
    @funzione doneRead
    @descrizione: permette di rilasciare mutua esclusione 'in lettura' ad un file
    @param file è il file su cui voglio rilasciare mutua esclusione
    @return 0 in caso di successo, -1 in caso di fallimento
*/
int doneRead(t_file* file);

/*
    @funzione startWrite
    @descrizione: permette di acquisire mutua esclusione 'in scrittura' ad un file
    @param file è il file su cui voglio acquisire mutua esclusione
    @return 0 in caso di successo, -1 in caso di fallimento
*/
int startWrite(t_file* file);

/*
    @funzione doneWrite
    @descrizione: permette di rilasciare mutua esclusione 'in scrittura' ad un file
    @param file è il file su cui voglio rilasciare mutua esclusione
    @return 0 in caso di successo, -1 in caso di fallimento
*/
int doneWrite(t_file* file);

#endif
