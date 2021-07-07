#include <server.h>

//Struttura File Server condivisa
t_file_storage* file_storage;
//configurazione server
config_server config;
//struttura dati che tiene traccia del numero di client connessi e gli effettivi client connessi
clientConnessi* clientAttivi;
//struttura dati usata per gestire file log
t_log* logStr;

static int inizializzaFileStorage(int argc, char** argv);
static int inizializzaClientAttivi();
static int ottieniNumClientConnessi();
static int updatemax(fd_set set, int fdmax);
static int ottieniConfigServer(char* pathConfig);
static void stampaInformazioni();
static int aggiungiConnessione(int fd_client);
static int chiudiTutteConnessioni();

static void *sigHandler(void *arg) {
    sigset_t *set = ((sigHandler_t*)arg)->set;
    int fd_pipe   = ((sigHandler_t*)arg)->signal_pipe;
    for( ;; ) {
	    int sig;
	    int r = sigwait(set, &sig);
	    if (r != 0) {
	        errno = r;
	        perror("FATAL ERROR 'sigwait'");
	        return NULL;
	    }
	    switch(sig) {
	        case SIGINT:
	        case SIGQUIT:
	            //fprintf(stdout,"Ricevuto segnale %s, esco il prima possibile\n", (sig==SIGINT) ? "SIGINT":"SIGQUIT");
                scriviSuFileLog(logStr, "Signal handler thread: ricevuto segnale %s, esco il prima possibile\n\n", (sig==SIGINT) ? "SIGINT":"SIGQUIT");
                if(writen(fd_pipe,&sig,sizeof(int))==-1){ //notifico il listener thread della ricezione del segnale indicando quale segnale ho ricevuto
                    perror("writen sigHandler");
                    return NULL;
                }
	            close(fd_pipe);  
	            return NULL;
            case SIGHUP:
                //fprintf(stdout,"Ricevuto segnale %s, esco non appena ho servito tutti i client\n", "SIGHUP");
                scriviSuFileLog(logStr, "Signal handler thread: ricevuto segnale %s, esco non appena ho servito tutti i client\n\n", "SIGHUP");
                if(writen(fd_pipe,&sig,sizeof(int))==-1){ //notifico il listener thread della ricezione del segnale indicando quale segnale ho ricevuto
                    perror("writen sigHandler");
                    return NULL;
                }
	            close(fd_pipe);  
	            return NULL;
	        default:  ; 
	    }
    }
    return NULL;	   
}

int main(int argc,char** argv){
    int r;    
    //blocco i segnali per evitare che vengano segnalati a thread diversi dal sighandler thread
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT); 
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    if(pthread_sigmask(SIG_BLOCK, &mask,NULL)==-1){
        perror("pthread_sigmask");
        return -1;
    }
    //ignoro SIGPIPE per evitare che una scrittura su un socket chiuso (lato client) termini il server
    struct sigaction s;
    memset(&s,0,sizeof(s));    
    s.sa_handler=SIG_IGN;
    if ((sigaction(SIGPIPE,&s,NULL) ) == -1 ) {   
	    perror("sigaction");
	    return -1;
    }
    int pfd[2];//pipe usata per far comunicare thread dispatcher e signal handler thread
    if(pipe(pfd)==-1){ 
        perror("pipe");
        return -1;
    }
    //installo il signal handler thread
    pthread_t sighandler_thread;
    sigHandler_t handlerArgs = { &mask, pfd[1]};
    if (pthread_create(&sighandler_thread, NULL, sigHandler, &handlerArgs) != 0) {
		fprintf(stderr, "Errore nella creazione del signal handler thread\n");
		return -1;
    }
    
    //inizializzo file storage con valori di default o con valori passati tramite file .txt
    if(inizializzaFileStorage(argc,argv)==-1){
        fprintf(stderr,"Errore nell'inizializzazione server\n");
        return -1; //errore fatale
    }
    int readyDescr[2]; //pipe usata dai threads worker per notificare quale fd deve essere riascoltato dal listen socket
    if(pipe(readyDescr)==-1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if(inizializzaClientAttivi()==-1){
        fprintf(stderr,"Errore nell'inizializzazione server\n");
        return -1; //errore fatale
    }
    //creo il pool di thread
    threadpool_t* threadpool=createThreadPool(config.numThreadsWorker, 3*config.numThreadsWorker);
    if (threadpool==NULL){
        perror("malloc");
        printf("Errore nell'inizializzare il threads pool\n"); //errore fatale
        return -1;
    }
    //creo struttura dati log
    logStr=inizializzaFileLog(LOGDIR);
    if(logStr==NULL){
        fprintf(stderr,"Errore nell'inizializzare file di log\n");
    }
    int fd_skt,fd_client;
    int fd_num=0;
    struct sockaddr_un sa;
    (void)unlink(config.sockname);
    strncpy(sa.sun_path, config.sockname , UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if((fd_skt=socket(AF_UNIX, SOCK_STREAM,0))==-1){
        perror("socket");
        return -1;//errore fatale
    }
    if(bind(fd_skt,(struct sockaddr *) &sa, sizeof(sa))==-1){
        perror("bind");
        return -1;//errore fatale
    }
    if(listen(fd_skt,SOMAXCONN)==-1){
        perror("listen");
        return -1;//errore fatale
    }
    fd_set readset,set;
    FD_ZERO(&set);
    FD_SET(fd_skt,&set);
    FD_SET(pfd[0],&set);
    FD_SET(readyDescr[0],&set);
    //setto fd_num
    fd_num = MAX(fd_skt,pfd[0]);
    fd_num = MAX(fd_num,readyDescr[0]);
    while(config.stato!=termina){
        readset=set;
        struct timeval timeout={0, 20000}; // 200 milliseconds: usato per controllare se ci sono client ancora connessi dopo il segnale SIGHUP
        if((r=select(fd_num+1,&readset,NULL,NULL,&timeout))<0){ 
            perror("select");
            return -1; //errore fatale
        }
        if(r==0){
            if(config.stato==rifiutaConnessioni){
                int clientAttivi=ottieniNumClientConnessi();
                if(clientAttivi==-1){ 
                    perror("lock");
                    return -1; //errore fatale
                }
                if(clientAttivi==0)
                    break;
            }
            continue;
        }
        for(int fd=0;fd<=fd_num;fd++){
            if(FD_ISSET(fd,&readset)){
                if(fd==fd_skt){
                    if((fd_client=accept(fd_skt,NULL,NULL))==-1){
                        perror("accept");
                        return -1;
                    }
                    fprintf(stdout,"Connessione accettata\n");
                    if(aggiungiConnessione(fd_client)==-1){
                        fprintf(stderr,"Errore nell'aggiungere la connessione con client %d\n",fd_client);
                        perror("aggiungiConnessione");
                        return -1; //errore fatale
                    }
                    scriviSuFileLog(logStr,"Thread dispatcher: apertura connessione con client %d\n\n",fd_client);
                    FD_SET(fd_client,&set);
                    if(fd_client>fd_num)
                        fd_num=fd_client;
                }
                else if(fd==pfd[0]){
                    int sig;
                    if(readn(fd,&sig,sizeof(int))==-1){
                        perror("readn");
                        return -1; //setta errno
                    }
                    if(sig==SIGINT || sig==SIGQUIT)
                        config.stato=termina;
                    else{
                        config.stato=rifiutaConnessioni;
                        close(fd_skt);
                        FD_CLR(fd_skt,&set); // non accettare nuove connessioni
                        fd_skt=-1;
                    }
                    close(pfd[0]); //chiudo il lato della pipe che ascoltava signal handler
                    FD_CLR(pfd[0],&set);
                    if (fd == fd_num) 
                        fd_num = updatemax(set, fd_num);
                }
                else if(fd==readyDescr[0]){ //un worker mi sta avvisando che devo riascoltare un client
                    if(readn(fd,&fd_client,sizeof(int))==-1){
                        printf("Errore nella lettura della pipe\n");
                        continue;
                    }
                    FD_SET(fd_client,&set);
                    if(fd_client>fd_num)
                        fd_num=fd_client;
                }
                else{ // nuova richiesta da un client
                    threadW_args* args = malloc(sizeof(threadW_args));
                    if(args == NULL){
                        perror("Malloc"); //errore fatale
                        return -1;
                    }
                    args->fd_daServire = fd;
                    args->pipe = readyDescr[1];
                    int r=addToThreadPool(threadpool,funcW,(void*)args);
                    FD_CLR(fd,&set); // per adesso non riscoltare fd: sarà il worker a notificarti quando farlo
                    if (fd == fd_num) 
                        fd_num = updatemax(set, fd_num);
                    if(r==0){
                        continue; //richiesta aggiunta correttemente
                    }
                    if(r<0){
                        printf("Errore interno nell'aggiunta di un task\n");
                        free(args);
                    }
                    else{
                        printf("Server troppo occupato\n");
                        free(args);
                    }  
                }
            }
            if(config.stato==termina)
                break;
        }
    }
    int err;
    if((err=(pthread_join(sighandler_thread,NULL)))!=0){ //faccio il join del signal handler thread
        errno=err;
        perror("pthread_join");
        return -1; //errore fatale
    }
    printf("Notifico threads workers che ho finito\n");
    if(destroyThreadPool(threadpool,1)==-1){ //distruggo il pool di threads
        fprintf(stderr,"Errore nella cancellazione del threads pool\n");
    }
    stampaInformazioni();
    //chiudo tutte le connessioni attive e libero memoria allocata da clientAttivi
    if(chiudiTutteConnessioni()==-1){
        fprintf(stderr,"Errore nella chiusura delle connessioni con i client\n");
        perror("chiudiTutteConnessioni");
        return -1;
    }
    //libero il file storage
    if(icl_hash_destroy(file_storage->storage,free,liberaFile)==-1){
        fprintf(stderr,"Errore nella distruzione struttura hash file storage\n");
    }
    pthread_mutex_destroy(&(file_storage->mtx));
    free(file_storage);
    //libero logStr
    if(distruggiStrutturaLog(logStr)==-1){
        fprintf(stderr,"Errore nella distruzione struttura dati log\n");
    }
    (void)unlink(config.sockname);
    //chiudo pipe usata per far comunicare workers e thread dispatcher
    close(readyDescr[0]);
    close(readyDescr[1]);
    //
    if(fd_skt!=-1){
        close(fd_skt);
    }
    unlink(SOCKNAME);
    return 0;
}

static int inizializzaFileStorage(int argc, char** argv){
    if(argc<2 || ottieniConfigServer(argv[1])==-1){
        //VALORI DI DEFAULT
        fprintf(stdout,"Avvio il server con valori di default\n");
        config.maxNumeroFile=10;
        config.maxDimbyte=1048576;//10 MB==1048576
        config.numThreadsWorker=10;
        strncpy(config.sockname,SOCKNAME,UNIX_PATH_MAX-1);
    }
    printf("%d %d %ld %s\n",config.maxNumeroFile,config.numThreadsWorker,config.maxDimbyte,config.sockname);
    config.stato=attivo;
    file_storage=(t_file_storage*)malloc(sizeof(t_file_storage));
    if(file_storage==NULL){
        perror("malloc");
        return -1;
    }
    file_storage->numeroFile=0;
    file_storage->dimBytes=0;
    file_storage->maxFile=0;
    file_storage->maxDimBytes=0;
    file_storage->nReplacement=0;
    file_storage->storage = icl_hash_create((2*config.maxNumeroFile),NULL,NULL);
    if(file_storage->storage==NULL){
        free(file_storage);
        perror("malloc");
        return -1;
    }
    if(pthread_mutex_init(&(file_storage->mtx), NULL)!=0){
        free(file_storage);
        if(icl_hash_destroy(file_storage->storage,free,free)!=0){
            printf("Errore nel liberare la coda\n");
        }
        return -1;
    }
    return 0;
}
void liberaFile (void* val){
    t_file* file = (t_file*) val;
    if(file->contenuto!=NULL)
        free(file->contenuto);
    if(file->path!=NULL){
        free(file->path);
    }
    if(distruggiCoda(file->fd,free)==-1){
        printf("Errore distruzione coda\n");
    }
    pthread_mutex_destroy(&(file->mtx));
    pthread_mutex_destroy(&(file->ordering));
    pthread_cond_destroy(&(file->cond_Go));
    free(file);
}


// ritorno l'indice massimo tra i descrittori attivi
static int updatemax(fd_set set, int fdmax) {
    for(int i=(fdmax-1);i>=0;--i)
	    if (FD_ISSET(i, &set)) 
            return i;
    return -1;
}


//ritorna 0 se ho ottenuto le richieste tutte correttamente; -1 altrimenti
static int ottieniConfigServer(char* pathConfig){
    if(pathConfig ==NULL)
        return -1;
    FILE* ifp;
    long n;
    char* token,*state;
    int n_workers=0,socket_name=0,n_max_file=0,max_storage=0; //variabili usate per verificare che tutte le configurazioni siano passate correttamente
    char buffer[108];
    if((ifp=fopen(pathConfig,"r"))==NULL){
        return -1;
    }
    while(fgets(buffer,108,ifp)!=NULL){
        token=strtok_r(buffer,":", &state);
        if(token==NULL)
            return -1;
        if(strncmp(token,"n_max_file",10)==0){
            if(n_max_file==1){
                fprintf(stdout,"n_max_file va specificato una sola volta\n");
                return -1;
            }
            n_max_file=1;
            token=strtok_r(NULL,"\n\0", &state);
            if(token==NULL){
                return -1;
            }
            if(isNumber(token,&n)!=0){
                fprintf(stdout,"n_max_file deve essere seguito da un intero che specifica il numero massimo di file nel server\n");
                return -1;
            }
            config.maxNumeroFile=n;
        }
        else if(strncmp(token,"max_storage",11)==0){
            if(max_storage==1){
                fprintf(stdout,"max_storage va specificato una sola volta\n");
                return -1;
            }
            max_storage=1;
            token=strtok_r(NULL,"\n\0", &state);
            if(token==NULL){
                return -1;
            }
            if(isNumber(token,&(config.maxDimbyte))!=0){
                fprintf(stdout,"max_storage deve essere seguito da un intero che specifica la capacità massima del server\n");
                return -1;
            }
        }
        else if(strncmp(token,"n_workers",9)==0){
            if(n_workers==1){
                fprintf(stdout,"n_workers va specificato una sola volta\n");
                return -1;
            }
            n_workers=1;
            token=strtok_r(NULL,"\n\0", &state);
            if(token==NULL){
                return -1;
            }
            if(isNumber(token,&n)!=0){
                fprintf(stdout,"n_workers deve essere seguito da un intero che specifica il numero di workers del server\n");
                return -1;
            }
            config.numThreadsWorker=n;
        }
        else if(strncmp(token,"sockname",11)==0){
            if(socket_name==1){
                fprintf(stdout,"sockname va specificato una sola volta\n");
                return -1;
            }
            socket_name=1;
            token=strtok_r(NULL,"\n\0", &state);
            if(token==NULL){
                return -1;
            }
            while(*token==' '){
                token++;
            }
            if(*token=='\0'){
                fprintf(stdout,"sockname deve essere seguito da una stringa che indica path del socket\n");
                return -1;
            }
            strncpy(config.sockname,token,UNIX_PATH_MAX-1);
        }
    }
    if(!n_workers || !socket_name || !n_max_file || !max_storage){
        fprintf(stdout,"errore nel file di configurazione: mancano parametri\n");
        return -1;
    }
    if(fclose(ifp)!=0){
        perror("fclose");
        return -1;
    }
    return 0;
}

static int inizializzaClientAttivi(){
    clientAttivi= malloc(sizeof(clientConnessi));
    if(clientAttivi==NULL){
        printf("Errore fatale");
        return -1;
    }
    clientAttivi->nclient=0;
    if ((pthread_mutex_init(&(clientAttivi->mtx), NULL) != 0)){
        perror("pthread_mutex_init");
        return -1;
    }
    clientAttivi->client=inizializzaCoda(NULL);
    if(clientAttivi->client==NULL){
        pthread_mutex_destroy(&(clientAttivi->mtx));
        free(clientAttivi);
        return -1;
    }
    return 0;
}

//aggiunge fd all'insieme dei client connessi
//ritorna 0 in caso di successo, -1 altrimenti
static int aggiungiConnessione(int fd_client){
    LOCK_RETURN(&(clientAttivi->mtx),-1);
    if(isInCoda(clientAttivi->client,&fd_client)){
        UNLOCK_RETURN(&(clientAttivi->mtx),-1);
        errno=EISCONN;
        return -1;
    }
    int* fd=malloc(sizeof(int));
    if(fd==NULL){
        UNLOCK_RETURN(&(clientAttivi->mtx),-1);
        return -1;
    }
    *fd=fd_client;
    if(aggiungiInCoda(clientAttivi->client,(void*) fd)==-1){
        UNLOCK_RETURN(&(clientAttivi->mtx),-1);
        errno=EPROTO;//errore di protocollo interno
        return -1;
    }
    clientAttivi->nclient++;
    UNLOCK_RETURN(&(clientAttivi->mtx),-1);
    return 0;
}

static int ottieniNumClientConnessi(){
    int r;
    LOCK_RETURN(&(clientAttivi->mtx),-1);
    r=clientAttivi->nclient;
    UNLOCK_RETURN(&(clientAttivi->mtx),-1);
    return r;
}

static void stampaInformazioni(){
    fprintf(stdout,"\nSTAMPA INFORMAZIONI\n");
    fprintf(stdout,"Numero massimo di file memorizzato nel file storage: %d\n", file_storage->maxFile);
    fprintf(stdout,"Dimensione massima del file storage: %ld\n", file_storage->maxDimBytes);
    fprintf(stdout,"Numero di volte in cui è stato eseguito algoritmo di rimpiazzamento: %d\n", file_storage->nReplacement);
    fprintf(stdout,"Lista file presenti nel file storage al momento della chiusura:\n");
    fprintf(stdout,"%-130s\tDIMENSIONE IN BYTE\n","NOME FILE");
    icl_entry_t *bucket, *curr;
    t_file* file;
    int i;
    icl_hash_t *ht = file_storage->storage;
    for (i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            file=(t_file*) curr->data;
            //non ho bisogno di acquisire lock sul file perchè ormai vi è solo il main thread
            fprintf(stdout,"%-130s\t%ld\n",file->path,file->dimByte);
            curr=curr->next;
        }
    }
}
static int chiudiTutteConnessioni(){
    nodo* conn;
    int fd_toClose;
    conn=prelevaDaCoda(clientAttivi->client); //non ho bisogno di acquisire la lock perchè ormai non c'è più concorrenza
    while(conn!=NULL){
        fd_toClose=*((int*)conn->val);
        if(close(fd_toClose)==-1){
            return -1;
        }
        free(conn->val);
        free(conn);
        conn=prelevaDaCoda(clientAttivi->client);
    }
    if(distruggiCoda(clientAttivi->client,free)==-1){
        errno=EPROTO;
        return -1;
    }
    pthread_mutex_destroy(&(clientAttivi->mtx));
    free(clientAttivi);
    return 0;
}
