#if !defined(CLIENT_H)
#define CLIENT_H

#include<util.h>
#include<api.h>
#include<conn.h>
#include<coda.h>
//*****TIPI DEFINITI*****//

/*
	Una struct che rappresenta una possibile operazione richiesta da un client
*/
typedef enum{
    WriteFile, //operazione richiesta tramite argomenti -w e -W da linea di comando
    ReadFile, //operazione richiesta tramite argomento -r da linea di comando
    WhereToSave, //operazione richiesta tramite argomento -d da linea di comando
    WhereToBackup, //operazione richiesta tramite argomento -D da linea di comando
    ReadNFiles, //operazione richiesta tramite argomento -r da linea di comando
    DeleteFile //operazione richiesta tramite argomento -c da linea di comando
}tipoOperazione;

/*
	Una struct che rappresenta una particolare richiesta da parte di un client
*/
typedef struct{
    tipoOperazione op; //tipo di operazione richiesta
    char* path; //path del file che riguarda l'operazione
    int n;	//indica il numero di file da leggere per opzione -R
    char* pathToSave; //indica dove salvare un file letto o dove eseguire il backup dei file espulsi dal server in seguito a capacity miss
}richiesta;

/*
	Una struct che rappresenta le configurazioni del client
*/
typedef struct{
    int stampaStOut; //opzione (settata con -p) che mi dice se stampare sullo standard output le varie richieste
    long millisec; //opzione (settata con -t) che mi dice il tempo che deve intercorrere tra l'invio di due richieste consecutive al server
    char* socket; //nome socket a cui connettersi (settato con -f)
}config_client;

//*****FUNZIONI*****//

/*
	@funzione ottieniRichieste
	@descrizione: effettua il parsing della linea di comando generando una coda di richieste e settando le configurazioni del client
	@param config è il puntatore alle configurazioni del client che vengono modificate tramite argomenti passati da liena di comando
	@param richieste è la coda in cui vengono inserite le varie richieste fatte dal client tramite linea di comando
	@param argc numero di argomenti passati da linea di comando
	@param argv puntatore all'array che contiene argomenti passati da linea di comando
	@return 0 in caso di successo, -1 in caso di fallimento
*/
int ottieniRichieste(config_client* config, t_coda* richieste, int argc, char** argv);


/*
	@funzione validazioneRichieste
	@descrizione: controlla la validità delle richieste effettuate da linea di comando. In particolare controlla che il parametro -d è sempre preceduto da almeno una 
	richiesta di lettura e il parametro -D è sempre preceduto da almeno una richiesta di scrittura
	@param richieste è la coda contenente tutte le richieste 
	@return 0 in caso di successo, -1 in caso di fallimento
*/
int validazioneRichieste(t_coda* richieste);


/*
	@funzione eseguiRichieste
	@descrizione: data una coda di richieste, effettua una serie di chiamate all'API implementata. Se la variabile config.stdOut è settata a 1,
	stampa, per ogni operazione, l'esito e varie informazioni riguardanti tale operazione
	@param richieste è la coda contenente tutte le richieste 
	@param config è la struttura che contiene le configurazioni del client
*/
void eseguiRichieste(t_coda* richieste, config_client config);


/*
	@funzione freeRichiesta
	@descrizione: libera la memoria allocata per contenere una richiesta da parte del client
*/
void freeRichiesta(void* val);
#endif
