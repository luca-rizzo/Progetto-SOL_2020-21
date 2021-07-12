#include<util.h>
#include<coda.h>


/*
    @funzione compare
    @descrizione funzione di comparazione fra interi. Viene usata come generica funzione di compare se non ne viene passata una
    @param a è il puntatore al primo intero
    @param b è il puntatore al secondo intero
    @return 1 se il valore puntato da a è uguale al valore puntato da b, 0 altrimenti
*/
static int compare (void* a, void* b){
    int x = *((int*)a);
    int y = *((int*)b);
    return x==y;
}

//*****FUNZIONI DI IMPLEMENTAZIONE INTERFACCIA*****//

t_coda* inizializzaCoda(int (*cmp) (void*,void*)){
    t_coda* coda=(t_coda*)malloc(sizeof(t_coda));
    if(!coda){
        errno = EINVAL;
        return NULL;
    }
    coda->head=NULL;
    coda->tail=NULL;
    coda->size=0;
    //se non passo alcuna funzione di compare userò la funzione generica di sopra
    coda->cmp = cmp ? cmp : compare;
    return coda;
}

int aggiungiInCoda(t_coda* coda,void* val){
    if(!coda){
        errno = EINVAL;
        return -1;
    }
    //alloco nuovo nodo
    nodo* p = malloc(sizeof(nodo));
    if(!p){
        return -1;
    }
    //il valore del nodo sarà quello passato come argomento
    p->val=val;
    p->next=NULL;
    if(!(coda->head)){
        //coda vuota
        coda->head = p;
        coda->head->previous=NULL;
    }
    else{
        coda->tail->next= p;
        p->previous=coda->tail;
    }
    coda->tail=p;
    coda->size++;
    return 0;
}

nodo* prelevaDaCoda(t_coda* coda){
    if(!coda){
        errno = EINVAL;
        return NULL;
    }
    if(!(coda->head)){
        //coda vuota
        return NULL;
    }
    nodo* p = coda->head;
    coda->head=coda->head->next;
    if(!(coda->head)){
        //coda vuota
        coda->tail=NULL;
    }
    coda->size--;
    return p;
}

int distruggiCoda(t_coda* coda, void (*liberaValoreNodo)(void*)){
    if(!coda){
        errno = EINVAL;
        return -1;
    }
    nodo* p;
    while(coda->head){
        p=coda->head;
        coda->head=coda->head->next;
        //se ho passato una funzione per liberare il valore, userò tale funzione per deallocare il valore
        if(*liberaValoreNodo!=NULL)
            (*liberaValoreNodo)(p->val);
        free(p);
    }
    coda->tail=NULL;
    free(coda);
    return 0;
}

int isInCoda(t_coda* coda, void* val){
    if(!coda){
        return 0;
    }
    nodo* p=coda->head;
    while(p){
        if(((*coda->cmp)(p->val, val))){
            return 1;
        }
        p=p->next;
    }
    return 0;
}

int rimuoviDaCoda(t_coda* coda, void* val, void (*liberaValoreNodo)(void*)){
    if(!coda){
        errno = EINVAL;
        return -1;
    }
    nodo* p;
    if(!(coda->head)){
        return -1;
    }
    if(((*coda->cmp)(coda->head->val, val))){
    //il valore da rimuovere è la testa della coda
        p=coda->head;
        coda->head=coda->head->next;
        if(coda->head!=NULL)
            coda->head->previous=NULL;
        else{
            //la coda è vuota
            coda->tail=NULL;
        }
        if(*liberaValoreNodo!=NULL){
            (*liberaValoreNodo)(p->val);
        }
        free(p);
        coda->size--;
        return 0;
    }
    //vado alla ricerca del valore
    p=coda->head->next;
    while(p){
        if(((*coda->cmp)(p->val, val))){
            break;
        }
        p=p->next;
    }
    if(p==NULL){
        return -1;
    }
    if(coda->tail==p){
        //il valore da rimuovere è l'ultimo elemento della coda
        coda->tail=coda->tail->previous;
        coda->tail->next=NULL;
    }
    else{
        p->next->previous=p->previous;
        p->previous->next=p->next;
    }
    //se ho passato una funzione per liberare il valore, userò tale funzione per deallocare il valore
    if(*liberaValoreNodo!=NULL){
        (*liberaValoreNodo)(p->val);
    }
    //libero nodo
    free(p);
    coda->size--;
    return 0;
}