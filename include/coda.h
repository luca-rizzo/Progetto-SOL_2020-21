#ifndef CODA_H
#define CODA_H
struct node{
    void* val;
    struct node* next;
	struct node* previous;
};
typedef struct node nodo;

typedef struct{
    nodo* tail;
    nodo* head;
    int (*cmp) (void*,void*);
}t_coda;

t_coda* inizializzaCoda(int (*cmp) (void*,void*));
int aggiungiInCoda(t_coda* coda, nodo* p);
nodo* prelevaDaCoda(t_coda* coda);
int distruggiCoda(t_coda* coda, void (*liberaValoreNodo)(void*));
int isInCoda(t_coda* coda, void* val);
int rimuoviDaCoda(t_coda* coda, void* val, void (*liberaValoreNodo)(void*));
#endif
