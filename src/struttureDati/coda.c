#include<util.h>
#include<coda.h>
//ritorna un puntatore ad una coda in caso di successo; NULL altrimenti
t_coda* inizializzaCoda(){
    t_coda* coda=(t_coda*)malloc(sizeof(t_coda));
    if(coda==NULL){
        perror("malloc");
        return NULL;
    }
    coda->head=NULL;
    coda->tail=NULL;
    return coda;
}
//ritorna 0 in caso di successo, -1 altrimenti
int aggiungiInCoda(t_coda* coda,nodo* p){
    if(coda==NULL){
        return -1;
    }
    if(coda->head == NULL){
        coda->head = p;
    }
    else{
        coda->tail->next= p;
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
int distruggiCoda(t_coda* coda){
    if(coda==NULL){
        return -1;
    }
    nodo* p;
    while(coda->head!=NULL){
        p=coda->head;
        coda->head=coda->head->next;
        free(p);
    }
    coda->tail=NULL;
    free(coda);
    return 0;
}
