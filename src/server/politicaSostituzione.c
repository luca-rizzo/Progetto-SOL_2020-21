#include<server.h>

static int BThenA(struct timespec a,struct timespec b);
static t_file* FIFO(t_file* file1, t_file* file2);
static t_file* trovaVittima(t_file* notToRemove);

//da chiamare con la lock sulla struttura dati e una writelock su notToRemove

//rimuove il file dalla struttura dati e lo inserisce nella coda filesDaEspellere se filesDaEspellere!=NULL;
//se fileDaEspellere==NULL rimuove il file dalla struttura dati e lo libera
int espelliFile(t_file* notToRemove, int myid, t_coda* filesDaEspellere){
    t_file* fileDaEspellere=trovaVittima(notToRemove);
    if(fileDaEspellere==NULL){
        return -1;
    }
    SYSCALLRETURN(startWrite(fileDaEspellere),-1);
    //fprintf(stderr, "Ho espulso il file: %s\n", fileDaEspellere->path);
    if(icl_hash_delete(file_storage->storage,(void*) fileDaEspellere->path,free,NULL)==-1){
        SYSCALLRETURN(doneWrite(fileDaEspellere),-1);
        return -1;
    }
    file_storage->dimBytes-=fileDaEspellere -> dimByte;
    file_storage->numeroFile--;
    scriviSuFileLog(logStr,"Thread %d: ALGORITMO RIMPIAZZAMENTO. File %s è stato espulso per far spazio a file %s\n\n", myid, fileDaEspellere->path , notToRemove->path);
    if(filesDaEspellere==NULL){
        liberaFile(fileDaEspellere);
        return 0;
    }
    if(aggiungiInCoda(filesDaEspellere, fileDaEspellere)==-1){
        return -1;
    }
    return 0;
}
//ritorna un file vittima in caso di successo, NULL in caso di fallimento
static t_file* trovaVittima(t_file* notToRemove){
    icl_entry_t *bucket, *curr;
    t_file* file,*victim=NULL;
    int i;
    icl_hash_t *ht = file_storage->storage;
    for (i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            file=(t_file*) curr->data;
            if(file==notToRemove){
                curr=curr->next;
                continue;
            }
            SYSCALLRETURN(startRead(file),NULL);
            if(victim==NULL)
                victim=file;
            else{
                victim=FIFO(file,victim);
            }
            SYSCALLRETURN(doneRead(file),NULL);
            curr=curr->next;
        }
    }
    return victim;
}
static t_file* FIFO(t_file* file1, t_file* file2){
    if(BThenA(file1->tempoCreazione,file2->tempoCreazione)){
        return file2;
    }
    return file1;
}
//ritorna 1 se b è avvenuto prima di a; 0 altrimenti
static int BThenA(struct timespec a,struct timespec b) {
    if (a.tv_sec == b.tv_sec)
        return a.tv_nsec > b.tv_nsec;
    else
        return a.tv_sec > b.tv_sec;
}
