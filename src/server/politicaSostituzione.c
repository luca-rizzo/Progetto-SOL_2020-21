#include<server.h>
//*****FUNZIONI DI IMPLEMENTAZIONE PRIVATE*****//

/*
    @funzione FIFO
    @descrizione: funzione di confronto fra due file presenti nel file storage server. Ritorna il file "più vecchio" tra i due passati come argomento.
    @param file1 è il primo file da confrontare
    @param file2 è il secondo file da confrontare
    @return il file presente da più tempo nel file storage server in caso di successo, NULL in caso di fallimento
*/
static t_file* FIFO(t_file* file1, t_file* file2);

/*
    @funzione trovaVittima
    @descrizione: funzione che permette di inviduare una vittima tra quelle presenti. La vittima individuata non deve essere notToRemove
    @param notToRemove è il puntatore al file che non vogliamo sia individuato come vittima
    @return una vittima in caso di successo, NULL in caso di fallimento
*/
static t_file* trovaVittima(t_file* notToRemove);

int espelliFile(t_file* notToRemove, int myid, t_coda* filesDaEspellere){
    //individuo la vittima
    t_file* fileDaEspellere=trovaVittima(notToRemove);
    if(!fileDaEspellere){
        return -1;
    }
    SYSCALLRETURN(startWrite(fileDaEspellere),-1);
    //rimuovo la vittima dalla mia tabella hash: non passerò alcuna funzione di free del valore perchè potrei voler inserire il file rimosso nella
    //coda filesDaEspellere
    if(icl_hash_delete(file_storage->storage,(void*) fileDaEspellere->path,free,NULL)==-1){
        SYSCALLRETURN(doneWrite(fileDaEspellere),-1);
        return -1;
    }
    //modifico dimensione e numero file
    file_storage->dimBytes-=fileDaEspellere -> dimBytes;
    file_storage->numeroFile--;
    //scrivo sul file di log l'operazione
    scriviSuFileLog(logStr,"Thread %d: ALGORITMO RIMPIAZZAMENTO. File %s è stato espulso per far spazio a file %s\n\n", myid, fileDaEspellere->path , notToRemove->path);
    if(!filesDaEspellere){
        //posso deallocare il file
        liberaFile(fileDaEspellere);
        return 0;
    }
    if((fileDaEspellere->ultima_operazione_mod)==wr || (fileDaEspellere->ultima_operazione_mod)==ap){ //il file deve essere stato aperto e modificato
        //lo aggiungo alla coda filesDaEspellere
        if(aggiungiInCoda(filesDaEspellere, fileDaEspellere)==-1){
            return -1;
        }
    }
    else{
        //se il file non era stato modificato posso deallocare il file
        liberaFile(fileDaEspellere);
    }
    return 0;
}

static t_file* trovaVittima(t_file* notToRemove){
    //ho già acquisito la lock sulla struttura dati globale
    icl_entry_t *bucket, *curr;
    t_file* file,*victim=NULL;
    int i;
    icl_hash_t *ht = file_storage->storage;
    //itero tutti gli elementi della tabella hash
    for (i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            file=(t_file*) curr->data;
            if(file==notToRemove){
                //si tratta del file che non voglio rimuovere: passa al prossimo file
                curr=curr->next;
                continue;
            }
            SYSCALLRETURN(startRead(file),NULL);
            if(!victim)
                victim=file;
            else{
                victim=FIFO(file,victim);
            }
            SYSCALLRETURN(doneRead(file),NULL);
            curr=curr->next;
        }
    }
    //ritorno la vittima individuata, NULL se non ho individuato alcuna vittima
    return victim;
}
static t_file* FIFO(t_file* file1, t_file* file2){
    if(!file1 || !file2)
        return NULL;
    if(BThenA(file1->tempoCreazione,file2->tempoCreazione)){
        return file2;
    }
    return file1;
}
