#include <server.h>

//*****VARIABILI GLOBALI*****//

config_server config; //contiene le configurazioni del nostro file storage server
t_file_storage* file_storage; //struttura File Server condivisa
clientConnessi* clientAttivi; //struttura dati che tiene traccia del numero di client connessi e gli effettivi client connessi
t_log* logStr; //struttura dati usata per gestire file log

//*****FUNZIONI PRIVATE DI IMPLEMENTAZIONE*****//
/*
    @funzione unlink_socket
    @descrizione esegue lo unlink del socket name dal socket file. Funzione installata con atexit
*/
static void unlink_socket();

/*
    @funzione inizializzaFileStorage
    @descrizione: inizializza il file storage settando le configurazioni del server e allocando la variabile globale file_storage in base alle configurazioni.
    @param argc è il numero di argomenti passati da linea di comando
    @param argv è il puntatore all'array degli argomenti passati da linea di comando
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int inizializzaFileStorage(int argc, char** argv);


/*
    @funzione inizializzaClientAttivi
    @descrizione: inizializza la variabile globale clientAttivi
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int inizializzaClientAttivi();

/*
    @funzione ottieniNumClientConnessi
    @descrizione permette di ottenere il numero di client attualmente connessi al server
    @return numero di client connessi in caso di successo, -1 in caso di fallimento
*/
static int ottieniNumClientConnessi();


/*
    @funzione updatemax
    @descrizione: permette di individuare il valore massimo tra gli fd settati in set a partire da un precedente valore massimo fdmax
    @param set è l'insieme di fd tra i quali voglio trovare quello di valore maggiore
    @param fdmax è il vecchio massimo fd
    @return il nuovo fd di valore massimo in caso di successo, -1 in caso di fallimento
*/
static int updatemax(fd_set set, int fdmax);


/*
    @funzione ottieniConfigServer
    @descrizione: esegue il parsing del file pathConfig per ottenere le configurazioni del server
    @param pathConfig è il path del file di configurazione dal quale prelevare i valori
    @return 0 in caso di successo nel parsing di TUTTE le opzioni di configurazione, -1 altrimenti
*/
static int ottieniConfigServer(char* pathConfig);


/*
    @funzione stampaInformazioni
    @descrizione: stampa delle informazioni riguardanti il file server storage
    In particolare stamperà: il numero massimo di file memorizzato nel server, la dimensione massima in Mbytes raggiunta dal server,
    numero di volte in cui è stato avviato l'algoritmo di rimpiazzamento e la lista dei file presenti nel file server storage
*/
static void stampaInformazioni();


/*
    @funzione aggiungiConnessione
    @descrizione: aggiunge una connessione (un nuovo fd) alla coda clientAttivi
    @param fd_client è l'fd del client che ha stabilito una connessione
    @return 0 in caso di successo, -1 in caso di fallimento setta errno
*/
static int aggiungiConnessione(int fd_client);


/*
    @funzione chiudiTutteConnessioni
    @descrizione: permette di chiudere tutte le connessioni attive con i client e di deallocare la struttura dati clientAttivi
    @return 0 in caso di successo, -1 in caso di fallimento setta errno
*/
static int chiudiTutteConnessioni();


/*
    @funzione sigHandler
    @descrizione funzione eseguita dal signal handler thread. Gestisce l'arrivo dei segnali SIGINT;SIGQUIT;SIGHUP e notifica il main thread
    della recezione di tali segnali
    @param arg è un puntatore a una struttura che contiene gli argomenti necessari per la gestione dei segnali
*/
static void *sigHandler(void *arg);

int main(int argc,char** argv){
    int r;    
    //***GESTIONE SEGNALI***//

    //blocco i segnali per evitare che vengano segnalati a thread diversi dal sighandler thread
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT); 
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    if(pthread_sigmask(SIG_BLOCK, &mask,NULL)==-1){
        perror("pthread_sigmask");
        exit(EXIT_FAILURE); //errore fatale
    }
    //ignoro SIGPIPE per evitare che una scrittura su un socket chiuso (lato client) termini il server
    struct sigaction s;
    memset(&s,0,sizeof(s));    
    s.sa_handler=SIG_IGN;
    if ((sigaction(SIGPIPE,&s,NULL)) == -1 ) {   
	    perror("sigaction");
	    exit(EXIT_FAILURE); //errore fatale
    }
    int pfd[2];//pipe usata per far comunicare thread dispatcher e signal handler thread
    if(pipe(pfd)==-1){ 
        perror("pipe");
        exit(EXIT_FAILURE); //errore fatale
    }
    //installo il signal handler thread
    pthread_t sighandler_thread;
    sigHandler_t handlerArgs = { &mask, pfd[1]};
    if (pthread_create(&sighandler_thread, NULL, sigHandler, &handlerArgs) != 0) {
		perror("Errore nella creazione del signal handler thread");
		exit(EXIT_FAILURE); //errore fatale
    }
    
    //***INIZIALIZZAZIONE STRUTTURE DATI***//

    //inizializzo file storage con valori di default o con valori passati tramite file .txt
    if(inizializzaFileStorage(argc,argv)==-1){
        perror("Errore nell'inizializzazione server");
        exit(EXIT_FAILURE); //errore fatale
    }
    //inizializzo clientAttivi
    if(inizializzaClientAttivi()==-1){
        perror("Errore nell'inizializzazione coda client attivi");
        exit(EXIT_FAILURE); //errore fatale
    }
    int readyDescr[2]; //pipe usata dai threads worker per notificare thread dispatcher su quale fd deve essere riascoltato 
    if(pipe(readyDescr)==-1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    //creo il pool di thread
    threadpool_t* threadpool=createThreadPool(config.numThreadsWorker, 3*config.numThreadsWorker);
    if (!threadpool){
        perror("Errore nell'inizializzare il threads pool"); //errore fatale
        exit(EXIT_FAILURE);
    }
    //creo struttura dati log
    logStr=inizializzaFileLog(LOGDIR);
    if(!logStr){
        perror("Errore nell'inizializzare file di log. Le operazioni non verranno registrate nel file di log");
    }

    //installo funzione di unlink all'uscita del processo
    if(atexit(unlink_socket)!=0){
        perror("atexit"); //proseguo lo stesso
    }

    int fd_skt,fd_client;
    int fd_num=0; //indice dell'fd di valore più alto da ascoltare
    struct sockaddr_un sa;
    (void)unlink(config.sockname);
    memset(&sa, '0', sizeof(sa));
    strncpy(sa.sun_path, config.sockname , UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if((fd_skt=socket(AF_UNIX, SOCK_STREAM,0))==-1){
        perror("socket");
        exit(EXIT_FAILURE);//errore fatale
    }
    if(bind(fd_skt,(struct sockaddr *) &sa, sizeof(sa))==-1){
        perror("bind");
        exit(EXIT_FAILURE);//errore fatale
    }
    if(listen(fd_skt,SOMAXCONN)==-1){
        perror("listen");
        exit(EXIT_FAILURE);//errore fatale
    }
    fd_set readset,set;
    //inizializzo il set di fd che la select deve "ascoltare"
    FD_ZERO(&set);
    FD_SET(fd_skt,&set);
    FD_SET(pfd[0],&set); //pipe per la comunicazione con signal handler thread
    FD_SET(readyDescr[0],&set); //pipe per la comunicazione con thread worker
    //setto fd_num
    fd_num = MAX(fd_skt,pfd[0]);
    fd_num = MAX(fd_num,readyDescr[0]);
    while(config.stato!=termina){ //fin quando non ricevo segnale SIGINT/SIGQUIT
        readset=set;
        struct timeval timeout={0, 20000}; // 200 milliseconds: usato per controllare se ci sono client ancora connessi dopo il segnale SIGHUP
        if((r=select(fd_num+1,&readset,NULL,NULL,&timeout))<0){ 
            perror("select");
            exit(EXIT_FAILURE);//errore fatale
        }
        if(r==0){
            //mi sono svegliato a causa del timer
            if(config.stato==rifiutaConnessioni){
                //ho già ricevuto il segnale SIGHUP
                int clientAttivi=ottieniNumClientConnessi();
                if(clientAttivi==-1){ 
                    perror("ottieniNumClientConnessi");
                    exit(EXIT_FAILURE);//errore fatale di protocollo interno
                }
                if(clientAttivi==0) //posso uscire dal loop perchè tutti i client hanno chiuso la connessione
                    break;
            }
            continue;
        }
        for(int fd=0;fd<=fd_num;fd++){
            if(FD_ISSET(fd,&readset)){
                if(fd==fd_skt){
                    //fd_skt è pronto ad accettare una nuova connessione
                    if((fd_client=accept(fd_skt,NULL,NULL))==-1){
                        perror("accept");
                        exit(EXIT_FAILURE);//errore fatale
                    }
                    if(aggiungiConnessione(fd_client)==-1){
                        fprintf(stderr,"Errore nell'aggiungere la connessione con client %d\n",fd_client);
                        perror("aggiungiConnessione");
                        continue;
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
                        exit(EXIT_FAILURE);//errore fatale
                    }
                    if(sig==SIGINT || sig==SIGQUIT){
                        //termina il prima possibile
                        config.stato=termina;
                    }
                    else{
                        config.stato=rifiutaConnessioni;
                        //non accettare nuove connessioni: chiudo fd_skt e non faccio più ascoltare fd_skt alla select
                        close(fd_skt);
                        FD_CLR(fd_skt,&set); 
                        fd_skt=-1;
                    }
                    close(pfd[0]); //chiudo il lato della pipe che ascoltava signal handler
                    FD_CLR(pfd[0],&set);
                    if (fd == fd_num) 
                        fd_num = updatemax(set, fd_num);
                    if(fd_num==-1){
                        fprintf(stderr, "Nessun fd da ascoltare: errore protocollo interno\n");
                        exit(EXIT_FAILURE);//errore fatale
                    }
                }
                else if(fd==readyDescr[0]){
                    //un worker mi sta avvisando che devo riascoltare un client
                    if(readn(fd,&fd_client,sizeof(int))==-1){
                        fprintf(stderr,"Errore nella lettura della pipe\n");
                        continue;
                    }
                    //ascolta nuovamente fd_client
                    FD_SET(fd_client,&set);
                    if(fd_client>fd_num)
                        fd_num=fd_client;
                }
                else{ 
                    //nuova richiesta da un client
                    threadW_args* args = malloc(sizeof(threadW_args));
                    if(!args){
                        perror("malloc"); 
                        exit(EXIT_FAILURE);//errore fatale
                    }
                    //setto gli argomenti da passare al worker
                    args->fd_daServire = fd;
                    args->pipe = readyDescr[1];
                    int r=addToThreadPool(threadpool,funcW,(void*)args);
                    // per adesso non riscoltare fd: sarà il worker a notificarti quando rifarlo
                    FD_CLR(fd,&set); 
                    if (fd == fd_num) 
                        fd_num = updatemax(set, fd_num);

                    if(fd_num==-1){
                        fprintf(stderr, "Nessun fd da ascoltare: errore protocollo interno\n");
                        exit(EXIT_FAILURE);//errore fatale
                    }  
                    if(r==0){
                        continue; //richiesta aggiunta correttemente
                    }
                    if(r<0){
                        fprintf(stderr, "Errore interno nell'aggiunta di un task\n");
                        free(args);
                    }
                    else{
                        fprintf(stderr, "Server troppo occupato\n");
                        free(args);
                    }
                }
            }
            if(config.stato==termina)
                break;
        }
    }
    //faccio il join del signal handler thread
    int err;
    if((err=(pthread_join(sighandler_thread,NULL)))!=0){ 
        errno=err;
        perror("pthread_join sighandler_thread");
    }
    scriviSuFileLog(logStr,"Thread dispatcher: notifico threads workers della terminazione\n\n");

    //distruggo il pool di threads
    if(destroyThreadPool(threadpool,1)==-1){ 
        perror("Errore nella cancellazione del threads pool");
        //non esco: cerco di deallocare il possibile
    }

    //stampo informazioni sull'attività del server
    stampaInformazioni();

    //chiudo tutte le connessioni attive e libero memoria allocata da clientAttivi
    if(chiudiTutteConnessioni()==-1){
        perror("Errore nella chiusura delle connessioni con i client");
    }

    //libero il file storage
    if(icl_hash_destroy(file_storage->storage,free,liberaFile)==-1){
        perror("Errore nella distruzione struttura hash file storage");
    }
    pthread_mutex_destroy(&(file_storage->mtx));
    free(file_storage);

    //libero logStr
    if(distruggiStrutturaLog(logStr)==-1){
        perror("Errore nella distruzione struttura dati log");
    }
    //chiudo pipe usata per far comunicare workers e thread dispatcher
    close(readyDescr[0]);
    close(readyDescr[1]);
    //se ho teminato con SIGINT o SIGQUIT chiudo il descrittore aperto sul listen socket
    if(fd_skt!=-1){
        close(fd_skt);
    }
    return 0;
}

static int inizializzaFileStorage(int argc, char** argv){
    if(argc<2 || ottieniConfigServer(argv[1])==-1){
        //VALORI DI DEFAULT
        fprintf(stdout,"Avvio il server con i seguenti valori di default:\n");
        config.maxNumeroFile=10;
        config.maxDimBytes=1048576;//10 MB==1048576
        config.numThreadsWorker=10;
        strncpy(config.sockname,SOCKNAME,UNIX_PATH_MAX-1);
    }
    else{
        fprintf(stdout, "Avvio server con le seguenti configurazioni:\n");
    }
    fprintf(stdout, "Massimo numero di file memorizzabili: %d\n",config.maxNumeroFile);
    fprintf(stdout, "Capacità massima server in bytes: %ld\n", config.maxDimBytes);
    fprintf(stdout, "Numero thread worker: %d\n", config.numThreadsWorker);
    fprintf(stdout, "Nome del socket file: %s\n\n", config.sockname);
    fflush(stdout);
    config.stato=attivo;
    file_storage=(t_file_storage*)malloc(sizeof(t_file_storage));
    if(!file_storage){
        return -1;
    }
    file_storage->numeroFile=0;
    file_storage->dimBytes=0;
    file_storage->maxFile=0;
    file_storage->maxDimBytes=0;
    file_storage->nReplacement=0;
    file_storage->storage = icl_hash_create((2*config.maxNumeroFile),NULL,NULL);
    if(!file_storage->storage){
        free(file_storage);
        return -1;
    }
    if(pthread_mutex_init(&(file_storage->mtx), NULL)!=0){
        free(file_storage);
        if(icl_hash_destroy(file_storage->storage,free,free)!=0){
            fprintf(stderr,"Errore nel liberare tabella hash\n");
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
        fprintf(stderr, "Errore distruzione coda file\n");
    }
    pthread_mutex_destroy(&(file->mtx));
    pthread_mutex_destroy(&(file->ordering));
    pthread_cond_destroy(&(file->cond_Go));
    free(file);
}

static int updatemax(fd_set set, int fdmax) {
    for(int i=(fdmax-1);i>=0;--i)
	    if (FD_ISSET(i, &set)) 
            return i;
    return -1;
}

static int ottieniConfigServer(char* pathConfig){
    if(pathConfig ==NULL)
        return -1;
    FILE* ifp;
    long n;
    char* token,*state;
    int n_workers=0,socket_name=0,n_max_file=0,max_storage=0; //variabili usate per verificare che tutte le configurazioni siano passate correttamente
    char buffer[250];
    //apro il file di configurazione
    if((ifp=fopen(pathConfig,"r"))==NULL){
        return -1;
    }
    //eseguo il parsing del file
    while(fgets(buffer,250,ifp)!=NULL){
        token=strtok_r(buffer,":", &state);
        if(token==NULL)
            return -1;
        if(strncmp(token,"n_max_file",10)==0){
            if(n_max_file==1){
                fprintf(stderr,"n_max_file va specificato una sola volta\n");
                return -1;
            }
            n_max_file=1;
            token=strtok_r(NULL,"\n\0", &state);
            if(token==NULL){
                return -1;
            }
            if(isNumber(token,&n)!=0){
                fprintf(stderr,"n_max_file deve essere seguito da un intero che specifica il numero massimo di file nel server\n");
                return -1;
            }
            config.maxNumeroFile=n;
        }
        else if(strncmp(token,"max_storage",11)==0){
            if(max_storage==1){
                fprintf(stderr,"max_storage va specificato una sola volta\n");
                return -1;
            }
            max_storage=1;
            token=strtok_r(NULL,"\n\0", &state);
            if(token==NULL){
                return -1;
            }
            if(isNumber(token,(long*) &(config.maxDimBytes))!=0){
                fprintf(stderr,"max_storage deve essere seguito da un intero che specifica la capacità massima del server\n");
                return -1;
            }
        }
        else if(strncmp(token,"n_workers",9)==0){
            if(n_workers==1){
                fprintf(stderr,"n_workers va specificato una sola volta\n");
                return -1;
            }
            n_workers=1;
            token=strtok_r(NULL,"\n\0", &state);
            if(token==NULL){
                return -1;
            }
            if(isNumber(token,&n)!=0){
                fprintf(stderr,"n_workers deve essere seguito da un intero che specifica il numero di workers del server\n");
                return -1;
            }
            config.numThreadsWorker=n;
        }
        else if(strncmp(token,"sockname",11)==0){
            if(socket_name==1){
                fprintf(stderr,"sockname va specificato una sola volta\n");
                return -1;
            }
            socket_name=1;
            token=strtok_r(NULL,"\n\0", &state);
            if(token==NULL){
                return -1;
            }
            while(*token==' '){ //salto tutti gli spazi vuoti
                token++;
            }
            if(*token=='\0' || *token =='\n'){
                fprintf(stderr,"sockname deve essere seguito da una stringa che indica path del socket\n");
                return -1;
            }
            strncpy(config.sockname,token,UNIX_PATH_MAX-1);
        }
    }
    if(!n_workers || !socket_name || !n_max_file || !max_storage){
        fprintf(stderr,"errore nel file di configurazione: mancano parametri\n");
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
    if(!clientAttivi){
        return -1;
    }
    if ((pthread_mutex_init(&(clientAttivi->mtx), NULL) != 0)){
        return -1;
    }
    clientAttivi->client=inizializzaCoda(NULL);
    if(!(clientAttivi->client)){
        pthread_mutex_destroy(&(clientAttivi->mtx));
        free(clientAttivi);
        return -1;
    }
    return 0;
}

static int aggiungiConnessione(int fd_client){
    LOCK_RETURN(&(clientAttivi->mtx),-1);
    if(isInCoda(clientAttivi->client,&fd_client)){
        //il client è già connesso
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
    //aggiungo fd associato al client alla coda clientAttivi
    if(aggiungiInCoda(clientAttivi->client,(void*) fd)==-1){
        UNLOCK_RETURN(&(clientAttivi->mtx),-1);
        errno=EPROTO;//errore di protocollo interno
        return -1;
    }
    UNLOCK_RETURN(&(clientAttivi->mtx),-1);
    return 0;
}

static int ottieniNumClientConnessi(){
    int r;
    LOCK_RETURN(&(clientAttivi->mtx),-1);
    r=clientAttivi->client->size;
    UNLOCK_RETURN(&(clientAttivi->mtx),-1);
    return r;
}

static void stampaInformazioni(){
    fprintf(stdout,"\nSTAMPA INFORMAZIONI\n");
    fprintf(stdout,"Numero massimo di file memorizzato nel file storage: %d\n", file_storage->maxFile);
    double sizeMbytes = file_storage->maxDimBytes;
    sizeMbytes/=1000000;
    fprintf(stdout,"Dimensione massima del file storage in Mbytes: %f\n", sizeMbytes);
    fprintf(stdout,"Numero di volte in cui è stato eseguito algoritmo di rimpiazzamento: %d\n", file_storage->nReplacement);
    fprintf(stdout,"Lista file presenti nel file storage al momento della chiusura:\n");
    fprintf(stdout,"%-130s\tDIMENSIONE IN BYTE\n","NOME FILE");
    //itero tutta la tabella hash per stampare informazioni sui file presenti
    icl_entry_t *bucket, *curr;
    t_file* file;
    int i;
    icl_hash_t *ht = file_storage->storage;
    for (i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            file=(t_file*) curr->data;
            //non ho bisogno di acquisire lock sul file perchè ormai vi è solo il main thread
            fprintf(stdout,"%-130s\t%ld\n",file->path,file->dimBytes);
            curr=curr->next;
        }
    }
}
static int chiudiTutteConnessioni(){
    nodo* conn;
    int fd_toClose;
    conn=prelevaDaCoda(clientAttivi->client); //non ho bisogno di acquisire la lock perchè ormai non c'è più concorrenza
    //ogni nodo ha come valore l'fd di un client attivo: devo chiudere tale connessione
    while(conn){
        fd_toClose=*((int*)conn->val);
        if(close(fd_toClose)==-1){
            return -1;
        }
        free(conn->val);
        free(conn);
        conn=prelevaDaCoda(clientAttivi->client);
    }
    //distruggo la coda contenuta in clientAttivi
    if(distruggiCoda(clientAttivi->client,free)==-1){
        errno=EPROTO;
        return -1;
    }
    pthread_mutex_destroy(&(clientAttivi->mtx));
    free(clientAttivi);
    return 0;
}
static void *sigHandler(void *arg) {
    //insieme di segnali da ascoltare
    sigset_t *set = ((sigHandler_t*)arg)->set;
    //pipe su cui scrivere per notificare thread dispatcher dell'arrivo di un segnale
    int fd_pipe   = ((sigHandler_t*)arg)->signal_pipe;
    for( ;; ) {
	    int sig;
	    int r = sigwait(set, &sig);
	    if (r != 0) {
	        errno = r;
	        perror("sigwait sigHandler");
	        return NULL;
	    }
	    switch(sig) {
	        case SIGINT:
	        case SIGQUIT:
                scriviSuFileLog(logStr, "Signal handler thread: ricevuto segnale %s, esco il prima possibile\n\n", (sig==SIGINT) ? "SIGINT":"SIGQUIT");
                //notifico il thread dispatcher della ricezione del segnale indicando quale segnale ho ricevuto
                if(writen(fd_pipe,&sig,sizeof(int))==-1){ 
                    perror("writen sigHandler");
                    return NULL;
                }
                //chiudo la pipe di comunicazione con thread dispatcher
	            close(fd_pipe);  
	            return NULL;
            case SIGHUP:
                scriviSuFileLog(logStr, "Signal handler thread: ricevuto segnale %s, esco non appena ho servito tutti i client\n\n", "SIGHUP");
                //notifico il thread dispatcher della ricezione del segnale indicando quale segnale ho ricevuto
                if(writen(fd_pipe,&sig,sizeof(int))==-1){ 
                    perror("writen sigHandler");
                    return NULL;
                }
                //chiudo la pipe di comunicazione con thread dispatcher
	            close(fd_pipe);  
	            return NULL;
	        default:  ; 
	    }
    }
    return NULL;	   
}
static void unlink_socket(){
    (void)unlink(config.sockname);
}