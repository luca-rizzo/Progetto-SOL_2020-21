#include<util.h>
#include<coda.h>
//se non mi passano una funzione di compare tra valori della coda userò questa
int compare (void* a, void* b){
    int x = *((int*)a);
    int y = *((int*)b);
    return x==y;
}
//ritorna un puntatore ad una coda in caso di successo; NULL altrimenti
t_coda* inizializzaCoda(int (*cmp) (void*,void*)){
    t_coda* coda=(t_coda*)malloc(sizeof(t_coda));
    if(coda==NULL){
        perror("malloc");
        return NULL;
    }
    coda->head=NULL;
    coda->tail=NULL;
    if(cmp!=NULL)
        coda->cmp=cmp;
    else
        coda->cmp=compare;
    return coda;
}
//ritorna 0 in caso di successo, -1 altrimenti
int aggiungiInCoda(t_coda* coda,void* val){
    if(coda==NULL){
        return -1;
    }
    nodo* p = malloc(sizeof(nodo));
    if(p==NULL){
        return -1;
    }
    p->val=val;
    p->next=NULL;
    if(coda->head == NULL){
        coda->head = p;
        coda->head->previous=NULL;
    }
    else{
        coda->tail->next= p;
        p->previous=coda->tail;
    }
    coda->tail=p;
    return 0;
}
//ritorna il nodo in testa ala coda in caso di successo; NULL se la coda è vuota
nodo* prelevaDaCoda(t_coda* coda){
    if(coda->head==NULL){
        return NULL;
    }
    nodo* p = coda->head;
    coda->head=coda->head->next;
    if(coda->head==NULL){
        coda->tail=NULL;
    }
    return p;
}
//ritorna 0 se distrugge correttamente la coda;-1 altrimenti
int distruggiCoda(t_coda* coda, void (*liberaValoreNodo)(void*)){
    if(coda==NULL){
        return -1;
    }
    nodo* p;
    while(coda->head!=NULL){
        p=coda->head;
        coda->head=coda->head->next;
        if(*liberaValoreNodo!=NULL)
            (*liberaValoreNodo)(p->val);
        free(p);
    }
    coda->tail=NULL;
    free(coda);
    return 0;
}
//ritorna 1 se esiste un nodo con valore=val; 0 altrimenti
int isInCoda(t_coda* coda, void* val){
    if(coda==NULL){
        return 0;
    }
    nodo* p=coda->head;
    while(p!=NULL){
        if(((*coda->cmp)(p->val, val))){
            return 1;
        }
        p=p->next;
    }
    return 0;
}
//ritorna 0 se il nodo con valore val viene rimosso; -1 altrimenti
int rimuoviDaCoda(t_coda* coda, void* val, void (*liberaValoreNodo)(void*)){
    if(coda==NULL){
        return -1;
    }
    nodo* p;
    if(coda->head==NULL){
        return -1;
    }
    if(((*coda->cmp)(coda->head->val, val))){
        p=coda->head;
        coda->head=coda->head->next;
        if(coda->head!=NULL)
            coda->head->previous=NULL;
        else
            coda->tail=NULL;
        if(*liberaValoreNodo!=NULL){
            (*liberaValoreNodo)(p->val);
        }
        free(p);
        return 0;
    }
    p=coda->head->next;
    while(p!=NULL){
        if(((*coda->cmp)(p->val, val))){
            break;
        }
        p=p->next;
    }
    if(p==NULL){
        return -1;
    }
    if(coda->tail==p){
        coda->tail=coda->tail->previous;
        coda->tail->next=NULL;
    }
    else{
        p->next->previous=p->previous;
        p->previous->next=p->next;
    }
    if(*liberaValoreNodo!=NULL){
        (*liberaValoreNodo)(p->val);
    }
    free(p);
    return 0;
}
