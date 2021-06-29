#include<util.h>
#include<threadW.h>
#include <icl_hash.h>
#include<server.h>
#include<coda.h>
//funzioni di scrittura e lettura di un file in muta esclusione
int startRead(t_file* file);
int doneRead(t_file* file);
int startWrite(t_file* file);
int doneWrite(t_file* file);

int OpenFile(int fd_client);
int ReadFile(int fd_client);
int WriteFile(int fd_client);
int RemoveFile(int fd_client);

void funcW(void* arg){
    threadW_args* args= (threadW_args*) arg;
    int fd_client = args->fd_daServire;
    int pipe = args->pipe;
    free(arg);
    //gestione richieste
    int nonAggiungereFd=0;
    t_op operazione;
    int nread = readn(fd_client, &operazione, sizeof(t_op));
    if(nread==-1){ 
        perror("readn");
        if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
            perror("writen");
        }
        return;
    } 
    if(nread==0){
        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd
        if(close(fd_client)==-1){
            perror("Close");
        }
        printf("Connessione terminata\n");
    }
    else{
        switch(operazione){
            case op:
                if(OpenFile(fd_client)==-1){
                    //perror("OpenFile");
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno=EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(close(fd_client)==-1){
                            perror("Close");
                            return;
                        }
                        printf("Connessione terminata\n");
                    }
                }
                else{
                    int r=0;
                    if(writen(fd_client,&r,sizeof(int))==-1){//invio operazione completata!
                        perror("writen");
                    }
                }
                break;
            case rd:
                if(ReadFile(fd_client)==-1){
                    //perror("ReadFile");
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno=EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(close(fd_client)==-1){
                            perror("Close");
                            return;
                        }
                        printf("Connessione terminata\n");
                    }
                }
                break;
            case wr:
                if(WriteFile(fd_client)==-1){
                    //perror("OpenFile");
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno=EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(close(fd_client)==-1){
                            perror("Close");
                            return;
                        }
                        printf("Connessione terminata\n");
                    }
                }
                else{
                    int r=0;
                    if(writen(fd_client,&r,sizeof(int))==-1){//invio operazione completata!
                        perror("writen");
                    }
                }
                break;
            case re:
                if(RemoveFile(fd_client)==-1){
                    //perror("OpenFile");
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno=EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(close(fd_client)==-1){
                            perror("Close");
                            return;
                        }
                        printf("Connessione terminata\n");
                    }
                }
                else{
                    int r=0;
                    if(writen(fd_client,&r,sizeof(int))==-1){//invio operazione completata!
                        perror("writen");
                    }
                }
                break;

            default:
                break;
        }
    }
    if(nonAggiungereFd!=1)
        if(writen(pipe, &fd_client,sizeof(int))==-1){
            perror("writen in pipe");//notifico il thread dispatcher che deve riascoltare l'fd_client
        } 
    return;
}
int startRead(t_file* file){
    LOCK_RETURN(&(file->ordering),-1);
    LOCK_RETURN(&(file->mtx),-1);
    while(file->scrittoriAttivi >0){
        WAIT_RETURN(&(file->cond_Go),&(file->mtx),-1);
    }
    file->lettoriAttivi++;
    UNLOCK_RETURN(&(file->ordering),-1);
    UNLOCK_RETURN(&(file->mtx),-1);
    return 0;
}
int doneRead(t_file* file){
    LOCK_RETURN(&(file->mtx),-1);
    file->lettoriAttivi--;
    if(file->lettoriAttivi==0){
        SIGNAL_RETURN(&(file->cond_Go),-1);
    }
    UNLOCK_RETURN(&(file->mtx),-1);
    return 0;
}
int startWrite(t_file* file){
    LOCK_RETURN(&(file->ordering),-1);
    LOCK_RETURN(&(file->mtx),-1);
    while(file->scrittoriAttivi>0 || file->lettoriAttivi>0){
        WAIT_RETURN(&(file->cond_Go),&(file->mtx),-1);
    }
    file->scrittoriAttivi++;
    UNLOCK_RETURN(&(file->ordering),-1);
    UNLOCK_RETURN(&(file->mtx),-1);
    return 0;
}
int doneWrite(t_file* file){
    LOCK_RETURN(&(file->mtx),-1);
    file->scrittoriAttivi--;
    SIGNAL_RETURN(&(file->cond_Go),-1);
    UNLOCK_RETURN(&(file->mtx),-1);
    return 0;
}
int OpenFile(int fd_client){
    //secondo il protocollo di comunicazione leggo lunghezza path, contenuto path e flag
    int len;  
    void* buffer;
    int flag;
    int nread;
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if((nread=readn(fd_client,buffer,len))==-1){
        free(buffer);
        return -1; //setta errno
    }
    if(nread==0){
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    if((nread=readn(fd_client, &flag, sizeof(int)))==-1){
        free(buffer);
        return -1; //setta errno
    }
    if(nread == 0){ 
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    LOCK_RETURN(&(file_storage->mtx),-1);
    //printf("%s\n",(char*) buffer);
    t_file* file = icl_hash_find(file_storage->storage, buffer);
    if(file!=NULL){
        if(flag==O_CREAT){
            free(buffer);
            errno=EEXIST;
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        else{
            if(startWrite(file)==-1){
                free(buffer);
                return -1;
            }
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            nodo* p = (nodo*) malloc(sizeof(nodo));
            p->val = (void*)(&fd_client);
            p->next = NULL;
            // if(isInCoda(file->fd, &fd_client)){//l'utente ha già aperto il file?
            //     free(p);
            //     errno=EIO;
            //     doneWrite(file);
            //     free(buffer);
            //     return -1;
            // }
            aggiungiInCoda(file->fd,p); //aggiungo fd alla lista di client che hanno aperto il file
            file->ultima_operazione=op;
            if(doneWrite(file)==-1){
                free(buffer);
                return -1;
            }
            free(buffer);
            return 0;
        }
    }
    else{
        if(flag!=O_CREAT){
            free(buffer);
            errno=ENOENT;
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        if(file_storage->numeroFile==config.maxNumeroFile){
            //politicaDiSostituzione
        }
        file_storage->numeroFile++;
        //creo un nuovo file
        t_file* file = malloc(sizeof(t_file));
        if(file==NULL){
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        file->contenuto=NULL;
        file->dimByte=0;
        file->scrittoriAttivi=0;
        file->lettoriAttivi=0;
        file->fd=inizializzaCoda(NULL);
        if(file->fd==NULL){
            free(buffer);
            free(file);
            errno=EIO;
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        nodo* p = (nodo*) malloc(sizeof(nodo));
        p->val = (void*)(&fd_client);
        p->next = NULL;
        aggiungiInCoda(file->fd,p); //aggiungo fd alla lista di client che hanno aperto il file
        file->ultima_operazione=cr;
        if ((pthread_mutex_init(&(file->mtx), NULL) != 0) || (pthread_mutex_init(&(file->ordering), NULL) != 0) || (pthread_cond_init(&(file->cond_Go), NULL) != 0))  {
	        errno=EIO;
            distruggiCoda(file->fd,NULL);
            free(buffer);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
	        return -1;
        }
        if(icl_hash_insert(file_storage->storage, buffer,(void*) file)==NULL){
	        errno=EIO;
            distruggiCoda(file->fd,NULL);
            pthread_mutex_destroy(&(file->mtx));
            pthread_mutex_destroy(&(file->ordering));
            pthread_cond_destroy(&(file->cond_Go));
            free(buffer);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
	        return -1;
        }
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return 0;
    }
    return 0;
}
int ReadFile(int fd_client){
    //secondo il protocollo di comunicazione leggo lunghezza path e contenuto path 
    int len;  
    void* buffer;
    int nread;
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if((nread=readn(fd_client,buffer,len))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    LOCK_RETURN(&(file_storage->mtx),-1);
    t_file* file = icl_hash_find(file_storage->storage, buffer);
    if(file==NULL){
        free(buffer);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        errno=ENOENT; //il file non esiste
        return -1;
    }
    free(buffer);
    SYSCALLRETURN(startRead(file),-1);
    UNLOCK_RETURN(&(file_storage->mtx),-1); //posso rilasciare mutex globale
    if(!isInCoda(file->fd,&fd_client)){//il client non ha aperto il file!
        errno=EACCES;
        doneRead(file);
        return -1;
    }
    int r=0;
    if(writen(fd_client,&r,sizeof(int))==-1){//invio operazione completata!
        perror("writen");
        doneRead(file);
        return -1;
    }
    size_t dim = file->dimByte;
    buffer = file->contenuto;
    if(writen(fd_client,&dim,sizeof(size_t))==-1){
        perror("writen");
        doneRead(file);
        return -1;
    }
    if(dim!=0){ // se il file non è vuoto invio contenuto
        if(writen(fd_client,buffer,dim)==-1){
            perror("writen");
            doneRead(file);
            return -1;
        }
    }
    SYSCALLRETURN(doneRead(file),-1);
    return 0;
}
int WriteFile(int fd_client){
    //secondo il protocollo di comunicazione leggo lunghezza path e contenuto path 
    int len;  
    void* buffer;
    int nread;
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if((nread=readn(fd_client,buffer,len))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    LOCK_RETURN(&(file_storage->mtx),-1);
    t_file* file = icl_hash_find(file_storage->storage, buffer);
    if(file==NULL){
        free(buffer);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        errno=ENOENT; //il file non esiste
        return -1;
    }
    free(buffer);
    SYSCALLRETURN(startWrite(file),-1);
    if(file->ultima_operazione!=cr){ //l'ultima operazione deve essere stata la creazione del file
        errno=EACCES;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    if(!isInCoda(file->fd,&fd_client)){ //devi avere aperto il file
        errno=EACCES;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //secondo il protocollo di comunicazione leggo dimensione file e contenuto
    size_t size;
    if((nread=readn(fd_client,&size,sizeof(size_t)))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        errno=EBADMSG;
        return -1;
    }
    if(size>config.maxDimbyte){ //il file è troppo grande
        errno=EFBIG;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    buffer=malloc(size); 
    if((nread=readn(fd_client,buffer,size))==-1){ //leggo contenuto
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1; //setta errno
    }
    if(nread==0){
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    if(file_storage->dimBytes+size>config.maxDimbyte){ //il file entra nel nostro storage o dobbiamo eliminare qualche file secondo la politica adottata?
        while(file_storage->dimBytes+size>config.maxDimbyte){
            //politica di rimpiazzamento
        }
    }
    //il file entra nel nostro file storage
    file_storage->dimBytes+=size;
    UNLOCK_RETURN(&(file_storage->mtx),-1);
    file->contenuto = malloc(size);
    memcpy(file->contenuto,buffer,size);
    file->ultima_operazione=wr;
    file->dimByte=size;
    free(buffer);
    SYSCALLRETURN(doneWrite(file),-1);
    return 0;
}

int RemoveFile(int fd_client){
    //secondo il protocollo di comunicazione leggo lunghezza path e contenuto path 
    int len;  
    void* buffer;
    int nread;
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if((nread=readn(fd_client,buffer,len))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    LOCK_RETURN(&(file_storage->mtx),-1);
    t_file* file = icl_hash_find(file_storage->storage, buffer);
    if(file==NULL){ 
        free(buffer);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        errno=ENOENT; //il file non esiste
        return -1;
    }
    free(buffer);
    SYSCALLRETURN(startWrite(file),-1);
    if(!isInCoda(file->fd,&fd_client)){ //devi avere aperto il file
        errno=EACCES;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    if(icl_hash_delete(file_storage->storage,(void*) buffer,free,liberaFile)==-1){
        errno=EIO;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    SYSCALLRETURN(doneWrite(file),-1);
    UNLOCK_RETURN(&(file_storage->mtx),-1);
    return 0;
}
