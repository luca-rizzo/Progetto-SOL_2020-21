#include<server.h>

static int BThenA(struct timespec a,struct timespec b);
static t_file* FIFO(t_file* file1, t_file* file2);
static int trovaVittima(t_file** fileDaEspellere,t_file* notToRemove);

//da chiamare con la lock sulla struttura dati e una writelock su notToRemove
int espelliFile(t_file* notToRemove, int myid){
    t_file* fileDaEspellere=NULL;
    trovaVittima(&fileDaEspellere,notToRemove);
    SYSCALLRETURN(startWrite(fileDaEspellere),-1);
    int size = fileDaEspellere -> dimByte;
    //fprintf(stderr, "Ho espulso il file: %s\n", fileDaEspellere->path);
    if(icl_hash_delete(file_storage->storage,(void*) fileDaEspellere->path,free,NULL)==-1){
        if(fileDaEspellere!=NULL)
            SYSCALLRETURN(doneWrite(fileDaEspellere),-1);
        return -1;
    }
    file_storage->dimBytes-=size;
    file_storage->numeroFile--;
    scriviSuFileLog(logStr,"Thread %d: ALGORITMO RIMPIAZZAMENTO. File %s è stato espulso per far spazio a file %s\n\n", myid, fileDaEspellere->path , notToRemove->path);
    liberaFile(fileDaEspellere);
    return 0;
}
//ritorna 0 in caso di successo; -1 in caso di fallimento
static int trovaVittima(t_file** fileDaEspellere, t_file* notToRemove){
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
            SYSCALLRETURN(startRead(file),-1);
            if(victim==NULL)
                victim=file;
            else{
                victim=FIFO(file,victim);
            }
            SYSCALLRETURN(doneRead(file),-1);
            curr=curr->next;
        }
    }
    (*fileDaEspellere)=victim;
    return 0;
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
