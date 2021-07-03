#ifndef SERVER_H_
#define SERVER_H_
#include <conn.h>
#include <icl_hash.h>
#include <coda.h>

typedef enum{
	attivo,
	rifiutaConnessioni,
	termina
}server_mode;

typedef struct{
    int maxNumeroFile;
    long maxDimbyte;
    int numThreadsWorker;
    char sockname[UNIX_PATH_MAX];
    server_mode stato;
}config_server;

typedef struct{
	int c;
	pthread_mutex_t mtx;
}numClientConnessi;

typedef struct{
    size_t dimByte;
    int scrittoriAttivi;
    int lettoriAttivi;
    char* path;
    void* contenuto;
    t_coda* fd;//insieme di fd che hanno aperto il file
    struct timespec tempoCreazione;
    t_op ultima_operazione;
    pthread_mutex_t mtx;
    pthread_mutex_t ordering;
    pthread_cond_t cond_Go;
}t_file;

typedef struct{
    icl_hash_t* storage;
    int numeroFile;
    pthread_mutex_t mtx;
    size_t dimBytes;
}t_file_storage;

typedef struct { //tipo dati da passare al sigHandler
    sigset_t     *set;           /// set dei segnali da gestire (mascherati)
    int           signal_pipe;   /// descrittore di scrittura di una pipe senza nome
} sigHandler_t;

typedef struct { //tipo dati da passare al threadWorker
    int fd_daServire;//fd da servire
    int pipe;//pipe dove scrivere quando ho servito la richiesta
} threadW_args;
extern config_server config;
extern t_file_storage* file_storage;
extern numClientConnessi* numClient;

void liberaFile (void* val);
int modificaNumClientConnessi(int incr);
int espelliFile();

int startRead(t_file* file);
int doneRead(t_file* file);
int startWrite(t_file* file);
int doneWrite(t_file* file);
#endif