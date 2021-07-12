#ifndef CODA_H
#define CODA_H
/*
	Struct che rappresenta un generico nodo della coda.
*/
struct node{
    void* val; //puntatore al valore del nodo
    struct node* next; //puntatore al prossimo nodo
	struct node* previous; //puntatore al nodo precedente
};
typedef struct node nodo;
/*
	Struct che rappresenta la coda
*/
typedef struct{
    nodo* tail; //puntatore all'ultimo elemento della coda
    nodo* head; //puntatore alla testa della coda
    int size; //numero di elementi presenti nella coda
    int (*cmp) (void*,void*); //funzione di comparazione tra valori dei nodi
}t_coda;

//*****FUNZIONI*****//

/*
	@funzione inizializzaCoda
	@descrizione: crea una nuova coda
	@param cmp è il puntatore alla funzione che realizza la comparazione tra i valori dei nodi presenti nella coda
	@return un puntatore alla nuova coda in caso di successo, NULL altrimenti, setta errno
*/
t_coda* inizializzaCoda(int (*cmp) (void*,void*));


/*
	@funzione aggiungiInCoda
	@descrizione: aggiunge un nuovo nodo alla coda con valore val
	@param coda è la coda in cui voglio aggiungere il nodo
	@param val è il valore associato al nuovo nodo
	@return 0 in caso di successo, -1 altrimenti, setta errno
*/
int aggiungiInCoda(t_coda* coda, void* val);


/*
	@funzione prelevaDaCoda
	@descrizione: preleva un nodo dalla coda secondo la politica FIFO
	@param coda è la coda da cui voglio prelevare un nodo
	@return nodo prelevato in caso di successo, NULL se la coda è vuota, setta errno
*/
nodo* prelevaDaCoda(t_coda* coda);


/*
	@funzione distruggiCoda
	@descrizione: distrugge una coda deallocando tutta la memoria
	@param coda è la coda che voglio distruggere
	@param liberaValoreNodo è la funzione da utilizzare per liberare i valori dai nodi. Se liberaValoreNodo==NULL il valore del nodo non viene deallocato
	@return 0 in caso di successo, -1 altrimenti, setta errno
*/
int distruggiCoda(t_coda* coda, void (*liberaValoreNodo)(void*));


/*
	@funzione isInCoda
	@descrizione: permette di sapere se un dato valore è presente nella coda
	@param coda è la coda che voglio esaminare
	@param val è il valore che voglio cercare
	@return 1 se il valore è presente nella coda, 0 altrimenti
*/
int isInCoda(t_coda* coda, void* val);


/*
	@funzione rimuoviDaCoda
	@descrizione: permette di rimuovere e deallocare il nodo che contiene un dato valore passato come argomento. Se più nodi possono avere lo stesso valore allora verrà rimosso il nodo che è stato inserito prima (politica FIFO).
	@param coda è la coda dalla quale voglio eliminare il nodo con il rispettivo valore
	@param val è il valore che voglio cercare
	@param liberaValoreNodo è la funzione da utilizzare per liberare il valore del nodo rimosso. Se liberaValoreNodo==NULL il valore del nodo non viene deallocato
	@return 0 in caso di successo, -1 altrimenti
*/
int rimuoviDaCoda(t_coda* coda, void* val, void (*liberaValoreNodo)(void*));
#endif
