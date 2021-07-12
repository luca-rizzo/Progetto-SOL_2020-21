#ifndef LOG_H
#define LOG_H
//*****TIPI DEFINITI*****//

/*
	Struct che permette di gestire la creazione e la gestione del file di log
*/
typedef struct{
	FILE* lfp; //puntatore ritornato dalla open()
	pthread_mutex_t mtx; //mtx per la scrittura in mutua esclusione sul file di log
}t_log;

//*****FUNZIONI*****//

/*
	@funzione inizializzaFileLog
	@descrizione: permette di creare una struttura per la gestione del file di log
	@param dirToSave è la directory in cui creare il file di log log.txt
	@return puntatore alla struttura dati inizializzata in caso di successo, NULL in caso di fallimento, setta errno
*/
t_log* inizializzaFileLog(const char* dirToSave);

/*
	@funzione scriviSulFileLog
	@descrizione: permette di scrivere sul file di log una stringa con un numero variabile di argomenti
	@param log è la struttura che gestisce il file di log
	@param buffer è la stringa da scrivere sul file di log
	@parma ... argomenti associati a buffer da scrivere nel file di log
	@return 0 in caso di successo, -1 in caso di fallimento
*/
int scriviSuFileLog(t_log* log,char* buffer,...);

/*
	@funzione distruggiStrutturaLog
	@descrizione: permette di deallocare la memoria relativa alla struttura logStr. Viene stampato anche un messaggio di "fine log" nel file di log e chiuso 
	il file di log associato
	@param logStr è la struttura da deallocare
	@return 0 in caso di successo, -1 in caso di fallimento
*/
int distruggiStrutturaLog(t_log* logStr);
#endif
