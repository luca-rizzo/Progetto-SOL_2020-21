#include<server.h>


//*****FUNZIONI DI IMPLEMENTAZIONE PRIVATE*****//

/*
    @funzione OpenFile
    @descrizione: gestisce una richiesta di opertura/creazione file da parte di un client
    @param fd_client è l'fd associato al client da servire
    @param myid è l'id del thread che serve la richiesta
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int OpenFile(int fd_client, int myid);

/*
    @funzione ReadFile
    @descrizione: gestisce una richiesta di lettura file da parte di un client
    @param fd_client è l'fd associato al client da servire
    @param myid è l'id del thread che serve la richiesta
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int ReadFile(int fd_client, int myid);

/*
    @funzione WriteFile
    @descrizione: gestisce una richiesta di scrittura file da parte di un client
    @param fd_client è l'fd associato al client da servire
    @param myid è l'id del thread che serve la richiesta
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int WriteFile(int fd_client, int myid);

/*
    @funzione OpenFile
    @descrizione: gestisce una richiesta di rimozione file da parte di un client
    @param fd_client è l'fd associato al client da servire
    @param myid è l'id del thread che serve la richiesta
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int RemoveFile(int fd_client, int myid);

/*
    @funzione CloseFile
    @descrizione: gestisce una richiesta di chiusura file da parte di un client
    @param fd_client è l'fd associato al client da servire
    @param myid è l'id del thread che serve la richiesta
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int CloseFile(int fd_client, int myid);

/*
    @funzione AppendToFile
    @descrizione: gestisce una richiesta di append contenuto in un file da parte di un client
    @param fd_client è l'fd associato al client da servire
    @param myid è l'id del thread che serve la richiesta
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int AppendToFile(int fd_client, int myid);

/*
    @funzione CloseConnection
    @descrizione: chiude la connessione con il client. Rimuove l'fd del client da clientAttivi e chiude tutti i file aperti da tale client (operazione fondamentale
    perchè lo stesso fd potrà essere associato alla connessione con un altro client successivamente e se non chiudessimo tutti i file aperti questo porterebbe a comportamenti anomali)
    @param fd_client è l'fd associato al client a cui chiudiamo la connessione
    @param myid è l'id del thread che serve la richiesta
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int CloseConnection(int fd_client, int myid);

/*
    @funzione ReadNFiles
    @descrizione: gestisce una richiesta di lettura n file qualsiasi da parte di un client
    @param fd_client è l'fd associato al client da servire
    @param myid è l'id del thread che serve la richiesta
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int ReadNFiles(int fd_client, int myid);

/*
    @funzione inviaEspulsiAlClient
    @descrizione: invia i file contenuti nella coda filesDaEspellere al client associato a fd_client secondo il protocollo di comunicazione. In caso di successo dealloca tutti i file inviati
    e la coda filesDaEspellere
    @param fd_client è l'fd associato al client da servire
    @param myid è l'id del thread che serve la richiesta
    @param filesDaEspellere è la coda contenente tutti i file espulsi da inviare al client
    @return 0 in caso di successo, -1 in caso di fallimento
*/
static int inviaEspulsiAlClient(int fd_client,int myid, t_coda* filesDaEspellere);

/*
    @funzione inviaLettiAlClient
    @descrizione: invia i file contenuti nella coda inviaLettiAlClient al client associato a fd_client secondo il protocollo di comunicazione. In caso di successo dealloca la coda inviaLettiAlClient
    @param fd_client è l'fd associato al client da servire
    @param myid è l'id del thread che serve la richiesta
    @param inviaLettiAlClient è la coda contenente tutti i file letti da inviare al client
    @return 0 in caso di successo, -1 in caso di fallimento
*/
static int inviaLettiAlClient(int fd_client,int myid, t_coda* filesLetti);

/*
    @funzione unlockAllfile
    @descrizione: rilascia la lock in lettura su tutti i file su cui avevo acquisito precedentemente la lock
    @param files è la lista di file su cui devo rilasciare lock
    @return 0 in caso di successo, -1 in caso di fallimento
*/
static int unlockAllfile(t_coda* files);

void funcW(void* arg){
    threadW_args* args= (threadW_args*) arg;
    //prelevo argomenti
    int fd_client = args->fd_daServire; //fd client da servire
    int pipe = args->pipe; //pipe su cui scrivere per avvertire il thread dispatcher che deve riascoltare un client
    int myid = args->idWorker; //id del worker thread
    free(arg);

    //gestione richieste
    int nonAggiungereFd=0; //mi notifica se devo segnalare il thread dispatcher di riascoltare fd_client 
    t_op operazione;
    
    //leggo il tipo di operazione secondo il protocollo
    int nread = readn(fd_client, &operazione, sizeof(t_op));
    if(nread==-1){ 
        perror("readn");
        if(writen(fd_client,&errno,sizeof(int))==-1){
            //invio errore
            perror("writen");
        }
        return;
    } 
    if(nread==0){ 
        //il client ha chiuso la connessione
        nonAggiungereFd=1; // non devo segnalare il thread dispatcher di riascoltare fd_client
        //chiudo la connessione internamente
        if(CloseConnection(fd_client, myid)==-1){
            perror("CloseConnection");
        }
    }
    else{
        switch(operazione){
            case op:
                if(OpenFile(fd_client, myid)==-1){
                    //invio errore
                    if(writen(fd_client,&errno,sizeof(int))==-1){
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il thread dispatcher di riascoltare fd_client: errore fatale nel protocollo di comunicazione
                        //chiudo la connessione internamente
                        if(CloseConnection(fd_client, myid)==-1){ 
                            perror("CloseConnection");
                        }
                    }
                }
                else{
                    int r=0;
                    //invio operazione completata!
                    if(writen(fd_client,&r,sizeof(int))==-1){
                        perror("writen");
                    }
                }
                break;
            case rd:
                if(ReadFile(fd_client, myid)==-1){
                    //invio errore
                    if(writen(fd_client,&errno,sizeof(int))==-1){
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il thread dispatcher di riascoltare fd_client: errore fatale nel protocollo di comunicazione
                        //chiudo la connessione internamente
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                } 
                //operazione completata inviata nella ReadFile
                break;
            case wr:
                if(WriteFile(fd_client, myid)==-1){
                    //invio errore
                    if(writen(fd_client,&errno,sizeof(int))==-1){
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il thread dispatcher di riascoltare fd_client: errore fatale nel protocollo di comunicazione
                        //chiudo la connessione internamente
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }
                //operazione completata inviata nella WriteFile
                break;
            case ap:
                if(AppendToFile(fd_client, myid)==-1){
                    //invio errore
                    if(writen(fd_client,&errno,sizeof(int))==-1){
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il thread dispatcher di riascoltare fd_client: errore fatale nel protocollo di comunicazione
                        //chiudo la connessione internamente
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }
                //operazione completata inviata nella AppendToFile
                break;
            case re:
                if(RemoveFile(fd_client, myid)==-1){
                    //invio errore
                    if(writen(fd_client,&errno,sizeof(int))==-1){
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il thread dispatcher di riascoltare fd_client: errore fatale nel protocollo di comunicazione
                        //chiudo la connessione internamente
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }
                else{
                    int r=0;
                    //invio operazione completata!
                    if(writen(fd_client,&r,sizeof(int))==-1){
                        perror("writen");
                    }
                }
                break;
            case cl:
                if(CloseFile(fd_client, myid)==-1){
                    //invio errore
                    if(writen(fd_client,&errno,sizeof(int))==-1){
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il thread dispatcher di riascoltare fd_client: errore fatale nel protocollo di comunicazione
                        //chiudo la connessione internamente
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }
                else{
                    int r=0;
                    //invio operazione completata!
                    if(writen(fd_client,&r,sizeof(int))==-1){
                        perror("writen");
                    }
                }
                break;
            case rn:
                if(ReadNFiles(fd_client, myid)==-1){
                    //invio errore
                    if(writen(fd_client,&errno,sizeof(int))==-1){
                        perror("writen");
                    }
                    if(errno==EBADMSG){
                        nonAggiungereFd=1; // non devo segnalare il thread dispatcher di riascoltare fd_client: errore fatale nel protocollo di comunicazione
                        //chiudo la connessione internamente
                        if(CloseConnection(fd_client, myid)==-1){
                            perror("CloseConnection");
                        }
                    }
                }
                //operazione completata inviata nella readNFiles
                break;
            default: 
                //operazione sconosciuta: errore fatale nel protocollo di comunicazione
                errno=EBADMSG;
                //invio errore
                if(writen(fd_client,&errno,sizeof(int))==-1){
                    perror("writen");
                }
                nonAggiungereFd=1; // non devo segnalare il thread dispatcher di riascoltare fd_client: errore fatale nel protocollo di comunicazione
                if(CloseConnection(fd_client, myid)==-1){
                    perror("CloseConnection");
                }
                break;
        }
    }
    if(!nonAggiungereFd){
        //notifico il thread dispatcher che deve riascoltare l'fd_client
        if(writen(pipe, &fd_client,sizeof(int))==-1){
            perror("writen in pipe");
        } 
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
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if(!buffer){
        return -1;
    }
    //leggo contenuto path
    if((nread=readn(fd_client,buffer,len))==-1){
        perror("readn");
        free(buffer);
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    //leggo flag
    if((nread=readn(fd_client, &flag, sizeof(int)))==-1){
        perror("readn");
        free(buffer);
        return -1; //setta errno
    }
    if(nread == 0){ 
        //errore nel protocollo di comunicazione
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    LOCK_RETURN(&(file_storage->mtx),-1);
    t_file* file = icl_hash_find(file_storage->storage, buffer);
    if(file!=NULL){ 
        //il file esiste
        if(flag==O_CREAT){ 
            //non puoi creare un file se già esiste
            free(buffer);
            errno=EEXIST;
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        else{
            //il client vuole aprire il file
            SYSCALLRETURN(startWrite(file),-1);
            UNLOCK_RETURN(&(file_storage->mtx),-1); //operazione non-distruttiva/non-creativa: posso rilasciare mutex globale
            int* val = malloc(sizeof(int));
            if(!val){
                doneWrite(file);
                free(buffer);
                return -1;
            }
            *val= fd_client;
            //l'utente ha già aperto il file?
            if(isInCoda(file->fd, &fd_client)){
                //non puoi aprire un file due volte
                errno=EACCES;
                doneWrite(file);
                free(buffer);
                return -1;
            }
            //aggiungo fd alla lista di client che hanno aperto il file
            if(aggiungiInCoda(file->fd,(void*)val)==-1){
                perror("Errore protocollo interno--aggiungiInCoda");
                errno=EPROTO;
                doneWrite(file);
                free(buffer);
                return -1;
            } 
            SYSCALLRETURN(doneWrite(file),-1);
            //scrivo operazione sul file di log
            scriviSuFileLog(logStr,"Thread %d: apertura del file %s su richiesta del client %d.\n\n",myid,buffer,fd_client);
            free(buffer);
            return 0;
        }
    }
    else{
        //il file non esiste
        if(flag!=O_CREAT){ 
            //il file non esiste
            free(buffer);
            errno=ENOENT;
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }

        //creo un nuovo file
        t_file* file = malloc(sizeof(t_file));
        if(!file){
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
        if (!(file->path)) {
            free(buffer);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        //copio il path nel file
        strncpy(file->path,buffer,len);
        file->contenuto=NULL;
        file->dimBytes=0;
        file->scrittoriAttivi=0;
        file->lettoriAttivi=0;
        file->fd=inizializzaCoda(NULL);
        if(!(file->fd)){
            free(buffer);
            free(file->path);
            free(file);
            perror("Errore protocollo interno--inizializzaCoda");
            errno=EPROTO;
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1;
        }
        int* val = malloc(sizeof(int));
        if(!val){
            free(buffer);
            free(file->path);
            distruggiCoda(file->fd,free);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
	        return -1;
        }
        //aggiungo fd alla lista di client che hanno aperto il file
        *val= fd_client;
        if(aggiungiInCoda(file->fd,(void*)val)==-1){
            perror("Errore protocollo interno--aggiungiInCoda");
            errno=EPROTO;
            free(buffer);
            free(file->path);
            distruggiCoda(file->fd,free);
            free(file);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
	        return -1;
        } 
        //modifico ultima operazione su file
        file->ultima_operazione_mod=cr;
        if ((pthread_mutex_init(&(file->mtx), NULL) != 0) || (pthread_mutex_init(&(file->ordering), NULL) != 0) || (pthread_cond_init(&(file->cond_Go), NULL) != 0))  {
            perror("Errore protocollo interno--pthread_mutex_init");
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
            //espelli un file
            if(espelliFile(file,myid,NULL)==-1){
                fprintf(stderr, "Errore protocollo interno--espelliFile\n");//espelliFile non setta erro
                errno=EPROTO;
                free(buffer);
                UNLOCK_RETURN(&(file_storage->mtx),-1);
                return -1;
            }
            //incremento numero replecement
            file_storage->nReplacement+=1;
        }
        if(icl_hash_insert(file_storage->storage, buffer,(void*) file)==NULL){
            perror("Errore protocollo interno--icl_hash_insert");
	        errno=EPROTO;
            liberaFile(file);
            free(buffer);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
	        return -1;
        }
        file_storage->numeroFile++;
        if(file_storage->numeroFile>file_storage->maxFile)
            file_storage->maxFile=file_storage->numeroFile;
        UNLOCK_RETURN(&(file_storage->mtx),-1); //operazione creativa: rilascio la lock globale alla fine
        //registro operazione sul file di log
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
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    //leggo contenuto path
    buffer=malloc(len);
    if(!buffer){
        return -1;
    }
    if((nread=readn(fd_client,buffer,len))==-1){
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    LOCK_RETURN(&(file_storage->mtx),-1);
    t_file* file = icl_hash_find(file_storage->storage, buffer);
    if(file==NULL){
        free(buffer);
        errno=ENOENT; //il file non esiste
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    free(buffer);

    //acquisisco prima mutex sul file!
    SYSCALLRETURN(startRead(file),-1); 
    //operazione non-distruttiva/non-creativa: posso rilasciare mutex globale
    UNLOCK_RETURN(&(file_storage->mtx),-1); 

    if(!isInCoda(file->fd,&fd_client)){ 
        //il client non ha aperto il file: non hai il diritto di accesso al file!
        errno=EACCES;
        doneRead(file);
        return -1;
    }
    //invio operazione completata: il client può leggere dimensione file e contenuto!
    int r=0;
    if(writen(fd_client,&r,sizeof(int))==-1){
        perror("writen");
        doneRead(file);//rilascio lock in caso di errore
        return -1;
    }
    //invio dimensione file e contenuto
    size_t dim = file->dimBytes;
    buffer = file->contenuto;
    //invio dimensione file
    if(writen(fd_client,&dim,sizeof(size_t))==-1){
        perror("writen");
        errno=EBADMSG;
        doneRead(file);
        return -1;
    }
    if(dim!=0){ 
        // se il file non è vuoto invio contenuto
        if(writen(fd_client,buffer,dim)==-1){
            perror("writen");
            errno=EBADMSG;
            doneRead(file);
            return -1;
        }
    }
    SYSCALLRETURN(doneRead(file),-1);
    scriviSuFileLog(logStr,"Thread %d: lettura del file %s su richiesta del client %d. %d bytes sono stati inviati al client.\n\n", myid, file->path, fd_client, file->dimBytes);
    return 0;
}
static int WriteFile(int fd_client, int myid){
    //secondo il protocollo di comunicazione leggo lunghezza path e contenuto path 
    int len;  
    void* buffer;
    int nread;
    //leggo lunghezza path
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if(!buffer){
        return -1;
    }
    //leggo contenuto path
    if((nread=readn(fd_client,buffer,len))==-1){
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
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
    if(file->ultima_operazione_mod!=cr){ 
        //l'ultima operazione di modifica deve essere stata la creazione del file
        errno=EACCES;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //l'utente ha aperto il file?
    if(!isInCoda(file->fd,&fd_client)){ 
        //devi avere aperto il file
        errno=EACCES;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //comunico al client che può inviare il file
    int r=0; 
    if(writen(fd_client,&r,sizeof(int))==-1){
        perror("writen");
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //secondo il protocollo di comunicazione leggo dimensione file e contenuto
    size_t size;
    if((nread=readn(fd_client,&size,sizeof(size_t)))==-1){
        perror("readn");
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    buffer=malloc(size); 
    if(!buffer){
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //leggo contenuto se file non è vuoto
    if(size>0){
        if((nread=readn(fd_client,buffer,size))==-1){ 
            perror("readn");
            free(buffer);
            SYSCALLRETURN(doneWrite(file),-1);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            return -1; //setta errno
        }
        if(nread==0){
            errno=EBADMSG;
            SYSCALLRETURN(doneWrite(file),-1);
            UNLOCK_RETURN(&(file_storage->mtx),-1);
            free(buffer);
            return -1;
        }
    }
    if(size>config.maxDimBytes){ 
        //il file è troppo grande
        errno=EFBIG;
        free(buffer);
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //il file potrebbe entrare nel file storage
    t_coda* filesDaEspellere=inizializzaCoda(NULL);
    if(!filesDaEspellere){
        fprintf(stderr,"Errore inizializzazione coda di file da espellere: i file espulsi non verranno inviati al client");
    }
    //il file entra nel nostro storage o dobbiamo eliminare qualche file secondo la politica adottata?
    while(file_storage->dimBytes+size>config.maxDimBytes){
        if(espelliFile(file,myid,filesDaEspellere)==-1){
            fprintf(stderr, "Errore protocollo interno--espelliFile\n");//espelliFile non setta erro
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
    UNLOCK_RETURN(&(file_storage->mtx),-1); //non si tratta di un operazione distruttiva: posso rilasciare lock globale
    file->contenuto = malloc(size);
    if(!(file->contenuto)){
        free(buffer);
        SYSCALLRETURN(doneWrite(file),-1);
        return -1;
    }
    memcpy(file->contenuto,buffer,size);
    file->ultima_operazione_mod=wr;
    file->dimBytes=size;
    free(buffer);
    SYSCALLRETURN(doneWrite(file),-1);
    scriviSuFileLog(logStr,"Thread %d: scrittura del file %s su richiesta del client %d. %d bytes sono stati scritti sul file.\n\n",myid,file->path,fd_client,size);
    //comunico al client che fino a questo momento l'operazione è andata a buon fine
    int t=0;
    if(writen(fd_client,&t,sizeof(int))==-1){
        perror("writen");
        distruggiCoda(filesDaEspellere,liberaFile);
        return -1;
    }
    //invio file espulsi al client
    if(inviaEspulsiAlClient(fd_client,myid,filesDaEspellere)==-1){
        distruggiCoda(filesDaEspellere,liberaFile);
        errno=EBADMSG;
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
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if(!buffer){
        return -1;
    }
    //leggo contenuto path
    if((nread=readn(fd_client,buffer,len))==-1){
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
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
        errno=EACCES;
        free(buffer);
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //rimuovo il file dalla tabella hash e dealloco la memoria del file
    int size = file->dimBytes;
    if(icl_hash_delete(file_storage->storage,(void*) buffer,free,liberaFile)==-1){
        free(buffer);
        perror("Errore protocollo interno--icl_hash_delete");
        errno=EPROTO;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    file_storage->dimBytes-=size;
    file_storage->numeroFile--;
    UNLOCK_RETURN(&(file_storage->mtx),-1); //operazione distruttiva: rilascio la lock globale alla fine
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
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if(!buffer){
        return -1;
    }
    if((nread=readn(fd_client,buffer,len))==-1){
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
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
    //rimuovo fd_client dalla lista interna al file che contiene tutti i fd associati ai client che hanno aperto il file
    if(rimuoviDaCoda(file->fd,&fd_client,free)==-1){
        free(buffer);
        perror("Errore protocollo interno--rimuoviDaCoda");
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
    //leggo lunghezza path
    if((nread=readn(fd_client,&len,sizeof(int)))==-1){
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    buffer=malloc(len);
    if(!buffer){
        return -1;
    }
    //leggo contenuto buffer
    if((nread=readn(fd_client,buffer,len))==-1){
        perror("readn");
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        free(buffer);
        errno=EBADMSG;
        return -1;
    }
    LOCK_RETURN(&(file_storage->mtx),-1);
    t_file* file = icl_hash_find(file_storage->storage, buffer);
    if(file==NULL){
        errno=ENOENT; //il file non esiste
        free(buffer);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    free(buffer);
    SYSCALLRETURN(startWrite(file),-1);
    if(!isInCoda(file->fd,&fd_client)){ 
        //devi avere aperto il file
        errno=EACCES;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    int r=0; 
    if(writen(fd_client,&r,sizeof(int))==-1){
        perror("writen");
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //secondo il protocollo di comunicazione leggo dimensione in bytes e contenuto

    //leggo dimensione in bytes
    size_t size;
    if((nread=readn(fd_client,&size,sizeof(size_t)))==-1){
        perror("readn");
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1; //readn setta errno
    }

    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    buffer=malloc(size); 
    if(!buffer){
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //leggo contenuto da appendere al file
    if((nread=readn(fd_client,buffer,size))==-1){ 
        perror("readn");
        free(buffer);
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1; //setta errno
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        free(buffer);
        return -1;
    }
    if(file->dimBytes + size > config.maxDimBytes){ 
        //il file con i byte da appendere è troppo grande!
        errno=EFBIG;
        free(buffer);
        SYSCALLRETURN(doneWrite(file),-1);
        UNLOCK_RETURN(&(file_storage->mtx),-1);
        return -1;
    }
    //il file potrebbe entrare nel fileStorage
    t_coda* filesDaEspellere=inizializzaCoda(NULL);
    if(filesDaEspellere==NULL){
        fprintf(stderr,"Errore inizializzazione coda di file da espellere: i file espulsi non verranno inviati al client\n");
    }
    //dobbiamo eliminare qualche file per fare posto al nuovo contenuto?
    while(file_storage->dimBytes + size >config.maxDimBytes){ 
        if(espelliFile(file,myid,filesDaEspellere)==-1){
            fprintf(stderr, "Errore protocollo interno--espelliFile\n"); //espelliFile non setta erro
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
    UNLOCK_RETURN(&(file_storage->mtx),-1); //posso rilasciare mtx globale: non si tratta di un'operazione distruttiva
    int newDim;
    if(file->dimBytes==0){
        //il file era vuoto
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
        //il file era già stato scritto
        newDim=file->dimBytes+size;
        if((file->contenuto=realloc(file->contenuto, newDim))==NULL){
            free(buffer);
            SYSCALLRETURN(doneWrite(file),-1);
            return -1;
        }
        memcpy(((char *)(file->contenuto) + file->dimBytes),buffer,size);
    }
    file->dimBytes=newDim;
    file->ultima_operazione_mod=ap;
    free(buffer);
    SYSCALLRETURN(doneWrite(file),-1);
    scriviSuFileLog(logStr,"Thread %d: scrittura in append sul file %s su richiesta del client %d. %d bytes sono stati appesi al file.\n\n",myid,file->path,fd_client,size);
    int t=0;
    //comunico al client che l'oprazione è andata a buon fine
    if(writen(fd_client,&t,sizeof(int))==-1){
        perror("writen");
        distruggiCoda(filesDaEspellere,liberaFile);
        return -1;
    }
    //invio file espulsi al client
    if(inviaEspulsiAlClient(fd_client,myid,filesDaEspellere)==-1){
        distruggiCoda(filesDaEspellere,liberaFile);
        errno=EBADMSG;
        return -1;
    }
    return 0;
}

static int CloseConnection(int fd_client, int myid){ 
    LOCK_RETURN(&(file_storage->mtx),-1);
    icl_entry_t *bucket, *curr;
    t_file* file;
    int i;
    icl_hash_t *ht = file_storage->storage;
    //controllo per ogni file se fd_client aveva aperto tale file: se sì lo chiudo
    for (i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            file=(t_file*) curr->data;
            SYSCALLRETURN(startWrite(file),-1);
            if(isInCoda(file->fd,&fd_client)){ 
                //fd aveva aperto il file
                if(rimuoviDaCoda(file->fd,&fd_client,free)==-1){
                    perror("Errore protocollo interno--rimuoviDaCoda");
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
    //nonostante non si tratti di un'operazione distruttiva/creativa devo comunque mantenere la lock globale perchè itero su tutti i file
    UNLOCK_RETURN(&(file_storage->mtx),-1); 
    //modifico la coda di client connessi
    LOCK_RETURN(&(clientAttivi->mtx),-1);
    if(rimuoviDaCoda(clientAttivi->client,&fd_client,free)==-1){
        UNLOCK_RETURN(&(clientAttivi->mtx),-1);
        perror("Errore protocollo interno--rimuoviDaCoda");
        errno=EPROTO; //errore di protocollo interno
        return -1;
    }
    UNLOCK_RETURN(&(clientAttivi->mtx),-1);
    if(close(fd_client)==-1){
        perror("Close");
    }
    scriviSuFileLog(logStr,"Thread %d: chiusura connessione con client %d\n\n",myid,fd_client);
    return 0;
}
static int ReadNFiles(int fd_client, int myid){
    int n;
    int nread;
    int numFileDaInviare;
    //secondo il protocolo di comunicazione leggo il numero di file da leggere
    if((nread=readn(fd_client,&n,sizeof(int)))==-1){ 
        perror("readn");
        errno=EBADMSG;
        return -1;
    }
    LOCK_RETURN(&(file_storage->mtx),-1);
    //stabilisco il numero di file da inviare
    if(n<=0){
        numFileDaInviare = file_storage->numeroFile;
    }
    else{
        numFileDaInviare= MIN(file_storage->numeroFile,n);
    }
    t_coda* filesDaInviare=inizializzaCoda(NULL);
    if(!filesDaInviare){
        perror("Errore protocollo interno--inizializzaCoda");
        errno=EPROTO;
        UNLOCK_RETURN(&(file_storage->mtx),-1);
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
            if(aggiungiInCoda(filesDaInviare,(void*)file)==-1){ //continuo ad andare avanti
                curr=curr->next;
                continue;
            }
            if(filesDaInviare->size==numFileDaInviare)
                break;
            curr=curr->next;
        }
        if(filesDaInviare->size==numFileDaInviare)
            break;
    }
    UNLOCK_RETURN(&(file_storage->mtx),-1);//posso rilasciare lock globale poichè non si tratta di un'operazione distruttiva e 
    //ho già lockato i file in lettura
    
    //comunico al client che fino a questo momento l'operazione è andata a buon fine
    int t=0;
    if(writen(fd_client,&t,sizeof(int))==-1){
        perror("writen");
        distruggiCoda(filesDaInviare,NULL);
        return -1;
    }
    //invio i file letti al client
    if(inviaLettiAlClient(fd_client,myid,filesDaInviare)==-1){
        unlockAllfile(filesDaInviare); //rilascio la lock su tutti i file lockati in lettura
        distruggiCoda(filesDaInviare,NULL); //distruggo la coda ma non i file
        errno=EBADMSG;
        return -1;
    }
    return 0;   
}


static int inviaEspulsiAlClient(int fd_client,int myid, t_coda* filesDaEspellere){
    //devo inviare i file espulsi al client
    if(filesDaEspellere==NULL){
        return -1;
    }
    //comunico al client il numero di file espulsi 
    if(writen(fd_client,&(filesDaEspellere->size),sizeof(int))==-1){
        perror("writen");
        return -1;
    }
    nodo* p=prelevaDaCoda(filesDaEspellere);
    while(p!=NULL){
        t_file* fileToSend=(t_file*) p->val;
        //secondo il protocollo di comunicazione invio lunghezza path e path del file espulso

        //invio lunghezza path
        int len=strlen(fileToSend->path)+1;
        if(writen(fd_client, &len,sizeof(int))==-1){
            perror("writen");
            return -1;
        }
        //invio contenuto path
        if(writen(fd_client,(void*)fileToSend->path,len)==-1){
            perror("writen");
            return -1;
        }
        //secondo il protocollo di comunicazione invio dimensione file e contenuto file espulso

        //invio dimensione file
        if(writen(fd_client, &(fileToSend->dimBytes),sizeof(size_t))==-1){
            perror("writen");
            return -1;
        }
        if(fileToSend->dimBytes>0){
            //se il file non è vuoto invio contenuto
            if(writen(fd_client, fileToSend->contenuto, fileToSend->dimBytes)==-1){
                perror("writen");
                return -1;
            }
        }
        //scrivo sul file di log l'operazione
        scriviSuFileLog(logStr,"Thread %d: invio file espulso %s al client %d\n\n",myid,fileToSend->path,fd_client);
        //posso liberare il file poichè è stato espulso
        liberaFile(fileToSend);
        free(p);
        p=prelevaDaCoda(filesDaEspellere);
    }
    //distruggo la coda
    distruggiCoda(filesDaEspellere,liberaFile);
    return 0;
}


static int inviaLettiAlClient(int fd_client,int myid, t_coda* filesLetti){
    if(filesLetti==NULL){
        return -1;
    }
    //comunico al client il numero di file letti con successo
    if(writen(fd_client,&(filesLetti->size),sizeof(int))==-1){
        perror("writen");
        return -1;
    }
    int sizetot=0;
    int numfile=filesLetti->size;
    nodo* p=prelevaDaCoda(filesLetti);
    while(p!=NULL){
        t_file* file=(t_file*) p->val;
        //secondo il protocollo di comunicazione invio lunghezza path e path del file da scrivere

        //invio lunghezza path
        int len=strlen(file->path)+1;
        if(writen(fd_client, &len,sizeof(int))==-1){
            perror("writen");
            SYSCALLRETURN(doneRead(file),-1);
            return -1;
        }
        //invio contenuto path
        if(writen(fd_client,(void*)file->path,len)==-1){
            perror("writen");
            SYSCALLRETURN(doneRead(file),-1);
            return -1;
        }
        //secondo il protocollo di comunicazione invio dimensione file e contenuto file da scrivere

        //invio dimensione file
        if(writen(fd_client, &(file->dimBytes),sizeof(size_t))==-1){
            perror("writen");
            SYSCALLRETURN(doneRead(file),-1);
            return -1;
        }
        if(file->dimBytes>0){
            //se il file non è vuoto invio contenuto
            if(writen(fd_client, file->contenuto, file->dimBytes)==-1){
                perror("writen");
                SYSCALLRETURN(doneRead(file),-1);
                return -1;
            }
        }
        sizetot+=file->dimBytes;
        free(p); //non devo liberare il file ma solo il nodo!
        p=prelevaDaCoda(filesLetti);
        SYSCALLRETURN(doneRead(file),-1);
    }
    //distruggo la coda
    distruggiCoda(filesLetti,NULL);
    //registro operazione sul file di log
    scriviSuFileLog(logStr,"Thread %d: lettura %d file su richiesta del client %d. %d byte sono stati inviati al client.\n\n",myid,numfile,fd_client,sizetot);
    return 0;
}
static int unlockAllfile(t_coda* files){
    nodo* p=prelevaDaCoda(files);
    while(p!=NULL){
        t_file* file = (t_file*) p->val;
        SYSCALLRETURN(doneRead(file),-1);
        p=prelevaDaCoda(files);
    }
    return 0;
}