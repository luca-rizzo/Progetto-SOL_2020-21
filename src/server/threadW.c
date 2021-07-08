#include<server.h>

static int OpenFile(int fd_client, int myid);
static int ReadFile(int fd_client, int myid);
static int WriteFile(int fd_client, int myid);
static int RemoveFile(int fd_client, int myid);
static int CloseFile(int fd_client, int myid);
static int AppendToFile(int fd_client, int myid);
static int CloseConnection(int fd_client, int myid);
static int ReadNFile(int fd_client, int myid);
static int inviaEspulsiAlClient(int fd_client,int myid, t_coda* filesDaEspellere);

void funcW(void* arg){
    threadW_args* args= (threadW_args*) arg;
    int fd_client = args->fd_daServire;
    int pipe = args->pipe;
    int myid = args->idWorker;
    free(arg);
    //gestione richieste
    int nonAggiungereFd=0;
    t_op operazione;
    //leggo il tipo di operazione secondo il protocollo
    int nread = readn(fd_client, &operazione, sizeof(t_op));
    if(nread==-1){ 
        perror("readn");
        if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
            perror("writen");
        }
        return;
    } 
    if(nread==0){ //il client ha chiuso la connessione
        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd
        if(CloseConnection(fd_client, myid)==-1){
            perror("CloseConnection");
        }
    }
    else{
        switch(operazione){
            case op:
                if(OpenFile(fd_client, myid)==-1){
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(CloseConnection(fd_client, myid)==-1){ 
                            perror("CloseConnection");
                        }
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
                if(ReadFile(fd_client, myid)==-1){
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                } //operazione completata inviata nella readFile
                break;
            case wr:
                if(WriteFile(fd_client, myid)==-1){
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }
                break;
            case ap:
                if(AppendToFile(fd_client, myid)==-1){
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }
                break;
            case re:
                if(RemoveFile(fd_client, myid)==-1){
                    
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }
                else{
                    int r=0;
                    if(writen(fd_client,&r,sizeof(int))==-1){//invio operazione completata!
                        perror("writen");
                    }
                }
                break;
            case cl:
                if(CloseFile(fd_client, myid)==-1){
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }
                else{
                    int r=0;
                    if(writen(fd_client,&r,sizeof(int))==-1){//invio operazione completata!
                        perror("writen");
                    }
                }
                break;
            case rn:
                if(ReadNFile(fd_client, myid)==-1){
                    if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }//operazione completata inviata nella readNFile
                break;
            default:
                errno=EBADMSG;
                if(writen(fd_client,&errno,sizeof(int))==-1){//invio errore
                    perror("writen");
                }
                nonAggiungereFd=1; // non devo segnalare il server che deve riascoltare fd: errore nel protocollo di comunicazione
                if(CloseConnection(fd_client, myid)==-1){
                    perror("CloseConnection");
                }
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
static int OpenFile(int fd_client, int myid){
    //secondo il protocollo di comunicazione leggo lunghezza path, contenuto path e flag
    int len;  
    void* buffer;
    int flag;
    int nread;
    //leggo lunghezza path
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    //leggo contenuto path
    if(buffer==NULL){
        return -1;
    }
    if((nread=readn(fd_client,buffer,len))==-1){
        free(buffer);
        return -1; //setta errno
    }
    if(nread==0){
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    //leggo flag
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

    t_file* file = icl_hash_find(file_storage->storage, buffer);
    if(file!=NULL){ //il file esiste

        if(flag==O_CREAT){ //non puoi creare un file se già esiste
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
            int* val = malloc(sizeof(int));
            if(val==NULL){
                doneWrite(file);
                free(buffer);
                return -1;
            }
            *val= fd_client;
            if(isInCoda(file->fd, &fd_client)){//l'utente ha già aperto il file?
                errno=EIO;
                doneWrite(file);
                free(buffer);
                return -1;
            }
            if(aggiungiInCoda(file->fd,(void*)val)==-1){//aggiungo fd alla lista di client che hanno aperto il file
                errno=EPROTO;
                doneWrite(file);
                free(buffer);
                return -1;
            } 
            
            if(doneWrite(file)==-1){
                free(buffer);
                return -1;
            }
            scriviSuFileLog(logStr,"Thread %d: apertura del file %s su richiesta del client %d.\n\n",myid,buffer,fd_client);
            free(buffer);
            return 0;
        }
    }
    else{
        if(flag!=O_CREAT){ //il file non esiste
            free(buffer);
            errno=ENOENT;
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        //creo un nuovo file
        t_file* file = malloc(sizeof(t_file));
        if(file==NULL){
            free(buffer);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        //setto il tempo di creazione per la politica FIFO
        if(clock_gettime(CLOCK_MONOTONIC,&(file->tempoCreazione))==-1){
            free(buffer);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        file->path = malloc(len);
        if (file->path==NULL) {
            free(buffer);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        strncpy(file->path,buffer,len);
        file->contenuto=NULL;
        file->dimByte=0;
        file->scrittoriAttivi=0;
        file->lettoriAttivi=0;
        file->fd=inizializzaCoda(NULL);
        if(file->fd==NULL){
            free(buffer);
            free(file->path);
            free(file);
            errno=EPROTO;
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        int* val = malloc(sizeof(int));
        if(val==NULL){
            free(buffer);
            free(file->path);
            distruggiCoda(file->fd,free);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
	        return -1;
        }
        *val= fd_client;
        if(aggiungiInCoda(file->fd,(void*)val)==-1){//aggiungo fd alla lista di client che hanno aperto il file
            errno=EPROTO;
            free(buffer);
            free(file->path);
            distruggiCoda(file->fd,free);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
	        return -1;
        } 
        file->ultima_operazione_mod=cr;
        if ((pthread_mutex_init(&(file->mtx), NULL) != 0) || (pthread_mutex_init(&(file->ordering), NULL) != 0) || (pthread_cond_init(&(file->cond_Go), NULL) != 0))  {
            errno=EPROTO;
            free(buffer);
            free(file->path);
            distruggiCoda(file->fd,free);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
	        return -1;
        }
        //ho raggiunto il numero massimo di file memorizzabili?
        if(file_storage->numeroFile==config.maxNumeroFile){ 
            if(espelliFile(file,myid,NULL)==-1){//se si espelli un file
                errno=EPROTO;
                free(buffer);
                UNLOCK_RETURN(&(file_storage->mtx),-1);
                return -1;
            }
            file_storage->nReplacement+=1;
        }
        if(icl_hash_insert(file_storage->storage, buffer,(void*) file)==NULL){
	        errno=EPROTO;
            liberaFile(file);
            free(buffer);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
	        return -1;
        }
        file_storage->numeroFile++;
        if(file_storage->numeroFile>file_storage->maxFile)
            file_storage->maxFile=file_storage->numeroFile;
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        scriviSuFileLog(logStr,"Thread %d: creazione del file %s su richiesta del client %d.\n\n",myid,buffer,fd_client);
        return 0;
    }
}
static int ReadFile(int fd_client, int myid){
    //secondo il protocollo di comunicazione leggo lunghezza path e contenuto path 
    int len;  
    void* buffer;
    int nread;
    //leggo lunghezza path
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        errno=EBADMSG;
        return -1;
    }
    //leggo contenuto path
    buffer=malloc(len);
    if(buffer==NULL){
        return -1;
    }
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
    if(!isInCoda(file->fd,&fd_client)){ //il client non ha aperto il file: non hai il diritto di accesso al file!
        errno=EACCES;
        doneRead(file);
        return -1;
    }
    int r=0;
    if(writen(fd_client,&r,sizeof(int))==-1){//invio operazione completata: il client può leggere dimensione file e contenuto!
        perror("writen");
        doneRead(file);
        return -1;
    }
    //invio dimensione file e contenuto
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
    scriviSuFileLog(logStr,"Thread %d: lettura del file %s su richiesta del client %d. %d bytes sono stati inviati al client.\n\n", myid, file->path, fd_client, file->dimByte);
    return 0;
}
static int WriteFile(int fd_client, int myid){
    //secondo il protocollo di comunicazione leggo lunghezza path e contenuto path 
    int len;  
    void* buffer;
    int nread;
    //leggo lunghezza path
    
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if(buffer==NULL){
        return -1;
    }
    //leggo contenuto path
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
    if(file->ultima_operazione_mod!=cr){ //l'ultima operazione deve essere stata la creazione del file
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
    int r=0; 
    
    if(writen(fd_client,&r,sizeof(int))==-1){//comunico al client che può inviare il file
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //secondo il protocollo di comunicazione leggo dimensione file e contenuto
    size_t size;
    if((nread=readn(fd_client,&size,sizeof(size_t)))==-1){
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1; //setta errno
    }
    if(nread==0){
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(size); 
    if(buffer==NULL){
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //leggo contenuto se file non è vuoto
    if(size>0){
        if((nread=readn(fd_client,buffer,size))==-1){ 
            free(buffer);
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
    }

    if(size>config.maxDimbyte){ //il file è troppo grande
        errno=EFBIG;
        free(buffer);
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        fprintf(stderr,"%ld\n",size);
        return -1;
    }
    //il file potrebbe entrare nel file storage
    t_coda* filesDaEspellere=inizializzaCoda(NULL);
    if(filesDaEspellere==NULL){
        fprintf(stderr,"Errore inizializzazione coda di file da espellere: i file espulsi non verranno inviati al client");
    }
    while(file_storage->dimBytes+size>config.maxDimbyte){//il file entra nel nostro storage o dobbiamo eliminare qualche file secondo la politica adottata?
        if(espelliFile(file,myid,filesDaEspellere)==-1){
            errno=EPROTO;
            free(buffer);
            SYSCALLRETURN(doneWrite(file),-1);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        file_storage->nReplacement+=1;
    }
    //il file entra nel nostro file storage
    file_storage->dimBytes+=size;
    if(file_storage->dimBytes>file_storage->maxDimBytes)
        file_storage->maxDimBytes=file_storage->dimBytes;
    UNLOCK_RETURN(&(file_storage->mtx),-1);
    file->contenuto = malloc(size);
    if(file->contenuto==NULL){
        free(buffer);
        SYSCALLRETURN(doneWrite(file),-1);
        return -1;
    }
    memcpy(file->contenuto,buffer,size);
    file->ultima_operazione_mod=wr;
    file->dimByte=size;
    free(buffer);
    SYSCALLRETURN(doneWrite(file),-1);
    scriviSuFileLog(logStr,"Thread %d: scrittura del file %s su richiesta del client %d. %d bytes sono stati scritti sul file.\n\n",myid,file->path,fd_client,size);
    //comunico al client che fino a questo momento l'operazione è andata a buon fine
    int t=0;
    if(writen(fd_client,&t,sizeof(int))==-1){
        distruggiCoda(filesDaEspellere,liberaFile);
        return -1;
    }
    //invio file espulsi al client
    if(inviaEspulsiAlClient(fd_client,myid,filesDaEspellere)==-1){
        return -1;
    }
    return 0;
}

static int RemoveFile(int fd_client, int myid){
    //secondo il protocollo di comunicazione leggo lunghezza path e contenuto path 
    int len;  
    void* buffer;
    int nread;
    //leggo lunghezza path
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        return -1; //setta errno
    }
    if(nread==0){
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if(buffer==NULL){
        return -1;
    }
    //leggo contenuto path
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
    SYSCALLRETURN(startWrite(file),-1);
    if(!isInCoda(file->fd,&fd_client)){ //devi avere aperto il file
        free(buffer);
        errno=EACCES;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    int size = file->dimByte;
    if(icl_hash_delete(file_storage->storage,(void*) buffer,free,liberaFile)==-1){
        free(buffer);
        errno=EPROTO;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    file_storage->dimBytes-=size;
    file_storage->numeroFile--;
    UNLOCK_RETURN(&(file_storage->mtx),-1); //operazione distruttiva: rilascio la lock alla fine
    scriviSuFileLog(logStr,"Thread %d: rimozione file %s su richiesta del client %d. %d bytes sono stati rimossi dal file server.\n\n",myid,buffer,fd_client,size);
    free(buffer);
    return 0;
}
//la cancellazione funziona perchè io rilascio la lock globale solo quando accedo al file (prendo la lock): in tal modo è impossibile
//che qualcuno cancelli il file mentre sono in attesa di accedere (lettura o scrittura) alla lock perchè se io attendo accesso ad un file,
//mantengo la lock globale

static int CloseFile(int fd_client, int myid){ //chiudere un file==rimuovere l'fd del client dalla lista degli fd del file

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
    if(buffer==NULL){
        return -1;
    }
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
    SYSCALLRETURN(startWrite(file),-1);
    UNLOCK_RETURN(&(file_storage->mtx),-1); //posso rilasciare lock globale: non si tratta di un'operazione distruttiva
    if(!isInCoda(file->fd,&fd_client)){ //devi avere aperto il file
        free(buffer);
        errno=EACCES;
        SYSCALLRETURN(doneWrite(file),-1);
        return -1;
    }
    if(rimuoviDaCoda(file->fd,&fd_client,free)==-1){
        free(buffer);
        errno=EPROTO; //errore di protocollo interno
        SYSCALLRETURN(doneWrite(file),-1);
        return -1;
    }
    free(buffer);
    SYSCALLRETURN(doneWrite(file),-1);
    scriviSuFileLog(logStr,"Thread %d: chiusura file %s su richiesta del client %d.\n\n",myid,file->path,fd_client);
    return 0;
}
static int AppendToFile(int fd_client, int myid){
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
    if(buffer==NULL){
        return -1;
    }
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
    int r=0; 
    if(writen(fd_client,&r,sizeof(int))==-1){//comunico al client che può inviare il contenuto da aggiungere al file
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //secondo il protocollo di comunicazione leggo dimensione in byte e contenuto
    size_t size;
    if((nread=readn(fd_client,&size,sizeof(size_t)))==-1){
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1; //setta errno
    }
    if(nread==0){
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(size); 
    if(buffer==NULL){
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //leggo contenuto da appendere al file
    if((nread=readn(fd_client,buffer,size))==-1){ 
        free(buffer);
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
    if(file->dimByte + size > config.maxDimbyte){ //il file con i byte da appendere è troppo grande!
        errno=EFBIG;
        free(buffer);
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //il file potrebbe entrare nel fileStorage
    t_coda* filesDaEspellere=inizializzaCoda(NULL);
    if(filesDaEspellere==NULL){
        fprintf(stderr,"Errore inizializzazione coda di file da espellere: i file espulsi non verranno inviati al client");
    }
    while(file_storage->dimBytes + size >config.maxDimbyte){ //dobbiamo eliminare qualche file?
        if(espelliFile(file,myid,filesDaEspellere)==-1){
            errno=EPROTO;
            free(buffer);
            SYSCALLRETURN(doneWrite(file),-1);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        file_storage->nReplacement+=1;
    }
    //il contenuto può essere appeso al file
    file_storage->dimBytes+=size;
    if(file_storage->dimBytes>file_storage->maxDimBytes)
        file_storage->maxDimBytes=file_storage->dimBytes;
    UNLOCK_RETURN(&(file_storage->mtx),-1);
    int newDim;
    if(file->dimByte==0){
        newDim=size;
        file->contenuto = malloc(size);
        if(file->contenuto==NULL){
            free(buffer);
            SYSCALLRETURN(doneWrite(file),-1);
            return -1;
        }
        memcpy(file->contenuto,buffer,size);
    }
    else{
        newDim=file->dimByte+size;
        if((file->contenuto=realloc(file->contenuto, newDim))==NULL){
            free(buffer);
            SYSCALLRETURN(doneWrite(file),-1);
            return -1;
        }
        memcpy(((char *)(file->contenuto) + file->dimByte),buffer,size);
    }
    file->dimByte=newDim;
    file->ultima_operazione_mod=ap;
    free(buffer);
    SYSCALLRETURN(doneWrite(file),-1);
    scriviSuFileLog(logStr,"Thread %d: scrittura in append sul file %s su richiesta del client %d. %d bytes sono stati appesi al file.\n\n",myid,file->path,fd_client,size);
    int t=0;
    //comunico al client che l'oprazione è andata a buon fine
    if(writen(fd_client,&t,sizeof(int))==-1){
        distruggiCoda(filesDaEspellere,liberaFile);
        return -1;
    }
    //invio file espulsi al client
    if(inviaEspulsiAlClient(fd_client,myid,filesDaEspellere)==-1){
        return -1;
    }
    return 0;
}
//chiude la connessione con il client: rimuove fd_client da lista client connessi e fd_client dalla lista di ogni file
//ritorna 0 in caso di successo,-1 in caso di fallimento; setta errno
static int CloseConnection(int fd_client, int myid){ 
    LOCK_RETURN(&(file_storage->mtx),-1);
    icl_entry_t *bucket, *curr;
    t_file* file;
    int i;
    icl_hash_t *ht = file_storage->storage;
    for (i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            file=(t_file*) curr->data;
            SYSCALLRETURN(startWrite(file),-1);
            if(isInCoda(file->fd,&fd_client)){ //se hai aperto il file
                if(rimuoviDaCoda(file->fd,&fd_client,free)==-1){
                    errno=EPROTO; //errore di protocollo interno
                    SYSCALLRETURN(doneWrite(file),-1);
                    UNLOCK_RETURN(&(file_storage->mtx),-1);
                    return -1;
                }
            }
            SYSCALLRETURN(doneWrite(file),-1);
            curr=curr->next;
        }
    }
    UNLOCK_RETURN(&(file_storage->mtx),-1);
    LOCK_RETURN(&(clientAttivi->mtx),-1);
    clientAttivi->nclient--;
    if(rimuoviDaCoda(clientAttivi->client,&fd_client,free)==-1){
        UNLOCK_RETURN(&(clientAttivi->mtx),-1);
        errno=EPROTO; //errore di protocollo interno
        return -1;
    }
    UNLOCK_RETURN(&(clientAttivi->mtx),-1);
    if(close(fd_client)==-1){
        perror("Close");
    }
    scriviSuFileLog(logStr,"Thread %d: chiusura connessione con client %d\n\n",myid,fd_client);
    //printf("Connessione terminata\n");
    return 0;
}
static int ReadNFile(int fd_client, int myid){
    int n;
    int nread;
    int numFileDaInviare;
    if((nread=readn(fd_client,&n,sizeof(int)))==-1){ //secondo il protocolo di comunicazione leggo il numero di file da leggere
        errno=EBADMSG;
        return -1;
    }
    LOCK_RETURN(&(file_storage->mtx),-1);
    if(n<0){
        numFileDaInviare = file_storage->numeroFile;
    }
    else{
        numFileDaInviare= MIN(file_storage->numeroFile,n);
    }
    int c=0;
    t_coda* fileDaInviare=inizializzaCoda(NULL);
    if(fileDaInviare==NULL){
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        errno=EPROTO;
        return -1;
    }
    icl_entry_t *bucket, *curr;
    t_file* file;
    icl_hash_t *ht = file_storage->storage;
    for (int i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            file=(t_file*) curr->data;
            SYSCALLRETURN(startRead(file),-1); //blocco il file in lettura: evito che qualcuno possa cancellarlo o scriverlo
            if(aggiungiInCoda(fileDaInviare,(void*)file)==-1){ //continuo ad andare avanti
                curr=curr->next;
                continue;
            }
            c++;
            if(c==numFileDaInviare)
                break;
            curr=curr->next;
        }
        if(c==numFileDaInviare)
            break;
    }
    UNLOCK_RETURN(&(file_storage->mtx),-1);//posso rilasciare lock globale
    
    //comunico al client che fino a questo momento l'operazione è andata a buon fine
    int t=0;
    if(writen(fd_client,&t,sizeof(int))==-1){
        distruggiCoda(fileDaInviare,NULL);
        return -1;
    }
    //comunico al client il numero di file letti con successo
    if(writen(fd_client,&c,sizeof(int))==-1){
        distruggiCoda(fileDaInviare,NULL);
        return -1;
    }
    int sizetot=0;
    nodo* p=prelevaDaCoda(fileDaInviare);
    while(p!=NULL){
        t_file* file=(t_file*) p->val;
        //secondo il protocollo di comunicazione invio lunghezza path e path del file da scrivere
        int len=strlen(file->path)+1;
        if(writen(fd_client, &len,sizeof(int))==-1){
            SYSCALLRETURN(doneRead(file),-1);
            distruggiCoda(fileDaInviare,NULL);
            return -1;
        }
        //invio contenuto path
        if(writen(fd_client,(void*)file->path,len)==-1){
            SYSCALLRETURN(doneRead(file),-1);
            distruggiCoda(fileDaInviare,NULL);
            return -1;
        }
        //secondo il protocollo di comunicazione invio dimensione file e contenuto file da scrivere
        if(writen(fd_client, &(file->dimByte),sizeof(size_t))==-1){
            SYSCALLRETURN(doneRead(file),-1);
            distruggiCoda(fileDaInviare,NULL);
            return -1;
        }
        if(file->dimByte>0){//se il file non è vuoto invio contenuto
            if(writen(fd_client, file->contenuto, file->dimByte)==-1){
                SYSCALLRETURN(doneRead(file),-1);
                distruggiCoda(fileDaInviare,NULL);
                return -1;
            }
        }
        sizetot+=file->dimByte;
        free(p); //non devo liberare il file ma solo il nodo!
        p=prelevaDaCoda(fileDaInviare);
        SYSCALLRETURN(doneRead(file),-1);
    }
    distruggiCoda(fileDaInviare,NULL);
    scriviSuFileLog(logStr,"Thread %d: lettura %d file su richiesta del client %d. %d byte sono stati inviati al client.\n\n",myid,c,fd_client,sizetot);
    return 0;
}
static int inviaEspulsiAlClient(int fd_client,int myid, t_coda* filesDaEspellere){
    //devo inviare i file espulsi al client
    if(filesDaEspellere!=NULL){
    //comunico al client il numero di file espulsi 
        if(writen(fd_client,&(filesDaEspellere->size),sizeof(int))==-1){
            distruggiCoda(filesDaEspellere,liberaFile);
            return -1;
        }
        nodo* p=prelevaDaCoda(filesDaEspellere);
        while(p!=NULL){
            t_file* fileToSend=(t_file*) p->val;
            //secondo il protocollo di comunicazione invio lunghezza path e path del file espulso
            int len=strlen(fileToSend->path)+1;
            if(writen(fd_client, &len,sizeof(int))==-1){
                distruggiCoda(filesDaEspellere,liberaFile);
                return -1;
            }
            //invio contenuto path
            if(writen(fd_client,(void*)fileToSend->path,len)==-1){
                distruggiCoda(filesDaEspellere,liberaFile);
                return -1;
            }
            //secondo il protocollo di comunicazione invio dimensione file e contenuto file espulso
            if(writen(fd_client, &(fileToSend->dimByte),sizeof(size_t))==-1){
                distruggiCoda(filesDaEspellere,liberaFile);
                return -1;
            }
            if(fileToSend->dimByte>0){//se il file non è vuoto invio contenuto
                if(writen(fd_client, fileToSend->contenuto, fileToSend->dimByte)==-1){
                    distruggiCoda(filesDaEspellere,liberaFile);
                    return -1;
                }
            }
            scriviSuFileLog(logStr,"Thread %d: invio file espulso %s al client %d\n\n",myid,fileToSend->path,fd_client);
            liberaFile(fileToSend);
            free(p);
            p=prelevaDaCoda(filesDaEspellere);
        }
        distruggiCoda(filesDaEspellere,liberaFile);
    }
    return 0;
}