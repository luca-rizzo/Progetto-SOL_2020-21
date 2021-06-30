#include<util.h>
#include <coda.h>
#include <icl_hash.h>
#include <server.h>
#include <threadpool.h>
#include<threadW.h>

#define UNIX_PATH_MAX 108
//Struttura File Server condivisa
t_file_storage* file_storage;
config_server config;
numClientConnessi* numClient;

int inizializzaFileStorage(char* pathConfig);
void liberaFile (void* val);

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
	            printf("ricevuto segnale %s, esco il prima possibile\n", (sig==SIGINT) ? "SIGINT":"SIGQUIT");
                if(writen(fd_pipe,&sig,sizeof(int))==-1){ //notifico il listener thread della ricezione del segnale indicando quale segnale ho ricevuto
                    perror("writen");
                    return NULL;
                }
	            close(fd_pipe);  
	            return NULL;
            case SIGHUP:
                printf("ricevuto segnale %s, esco non appena ho servito tutti i client\n", "SIGHUP");
                if(writen(fd_pipe,&sig,sizeof(int))==-1){ //notifico il listener thread della ricezione del segnale indicando quale segnale ho ricevuto
                    perror("writen");
                    return NULL;
                }
	            close(fd_pipe);  
                config.stato=rifiutaConnessioni;
	            return NULL;
	        default:  ; 
	    }
    }
    return NULL;	   
}

int main(int argc,char** argv){
    int r;
    int pfd[2];
    pthread_t sighandler_thread;
    if(pipe(pfd)==-1){
        perror("pipe");
        return -1;
    }
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
    //ignoro SIGPIPE per evitare che una scrittura su un socket chiuso (dall'altra parte) termini il server
    struct sigaction s;
    memset(&s,0,sizeof(s));    
    s.sa_handler=SIG_IGN;
    if ((sigaction(SIGPIPE,&s,NULL) ) == -1 ) {   
	    perror("sigaction");
	    return -1;
    } 
    //installo il signal handler thread
    sigHandler_t handlerArgs = { &mask, pfd[1]};
    if (pthread_create(&sighandler_thread, NULL, sigHandler, &handlerArgs) != 0) {
		fprintf(stderr, "errore nella creazione del signal handler thread\n");
		return -1;
    }
    
    if(inizializzaFileStorage(argv[1])==-1){
        return -1; //errore fatale
    }
    int readyDescr[2]; //pipe usata dai threads worker per notificare quale fd deve essere riascoltato
    if(pipe(readyDescr)==-1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    threadpool_t* threadpool=createThreadPool(config.numThreadsWorker, 3*config.numThreadsWorker);
    if (threadpool==NULL){
        perror("malloc");
        printf("Errore nell'inizializzare il threads pool\n"); //errore fatale
        return -1;
    }
    //inizializzo variabile numClient
    numClient= malloc(sizeof(numClientConnessi));
    if(numClient==NULL){
        printf("Errore fatale");
        return -1;
    }
    numClient->c=0;
    if ((pthread_mutex_init(&(numClient->mtx), NULL) != 0)){
        perror("pthread_mutex_init");
        return -1;
    }

    long fd_skt,fd_client;
    int fd_num=0;
    struct sockaddr_un sa;
    unlink(SOCKNAME);
    strncpy(sa.sun_path, config.sockname , UNIX_PATH_MAX-1);
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
    if(fd_skt>pfd[0]){
        fd_num=fd_skt;
    }
    else{
        fd_num=pfd[0];
    }
    if(readyDescr[0]>fd_num){
        fd_num= readyDescr[0];
    }
    while(config.stato!=termina){
        readset=set;
        struct timeval timeout={0, 10000}; // 100 milliseconds
        if((r=select(fd_num+1,&readset,NULL,NULL,&timeout))<0){ 
            perror("select");
            return -1; //errore fatale
        }
        if(r==0){
            if(config.stato==rifiutaConnessioni){
                int clientAttivi;
                if((clientAttivi=modificaNumClientConnessi(0))==-1){
                    perror("lock");
                    return -1;
                }
                if(clientAttivi==0)
                    break;
            }
            continue;
        }
        for(long fd=0;fd<=fd_num;fd++){
            if(FD_ISSET(fd,&readset)){
                if(fd==fd_skt){
                    if((fd_client=accept(fd_skt,NULL,NULL))==-1){
                        perror("accept");
                        return -1;
                    }
                    printf("Connessione accettata\n");
                    if(modificaNumClientConnessi(1)==-1){
                        perror("modificaNumClientConnessi");
                        return -1; //errore fatale
                    }
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
                        FD_CLR(fd_skt,&set); // non accettare nuove connessioni
                    }
                    close(pfd[0]);
                    FD_CLR(pfd[0],&set);
                    break;
                }
                else if(fd==readyDescr[0]){
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
                    FD_CLR(fd,&set);
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
        }
    }
    printf("Notifico threads che ho finito\n");
    if(destroyThreadPool(threadpool,0)==-1){
        printf("Errore nella cancellazione del threads pool\n");
    }
    int err;
    if((err=(pthread_join(sighandler_thread,NULL)))!=0){
        errno=err;
        perror("pthread_join");
        return -1; //errore fatale
    }
    icl_hash_destroy(file_storage->storage,free,liberaFile);
    //distruggo numClient
    pthread_mutex_destroy(&(numClient->mtx));
    free(numClient);
    free(file_storage);
    (void)unlink(config.sockname);
    close(readyDescr[0]);
    close(readyDescr[1]);
    close(fd_skt);
    unlink(SOCKNAME);
    return 0;
}

int inizializzaFileStorage(char* pathConfig){
    //VALORI DI DEFAULT
    config.maxNumeroFile=50;
    config.maxDimbyte=1048576;//10 MB
    config.numThreadsWorker=10;
    config.sockname=SOCKNAME;
    config.stato=attivo;
    file_storage=(t_file_storage*)malloc(sizeof(t_file_storage));
    if(file_storage==NULL){
        perror("malloc");
        return -1;
    }
    file_storage->numeroFile=0;
    file_storage->dimBytes=0;
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
    if(distruggiCoda(file->fd,NULL)==-1){
        printf("Errore distruzione coda\n");
    }
    pthread_mutex_destroy(&(file->mtx));
    pthread_mutex_destroy(&(file->ordering));
    pthread_cond_destroy(&(file->cond_Go));
    free(file);
}
int modificaNumClientConnessi(int incr){
    int r;
    LOCK_RETURN(&(numClient->mtx),-1);
    r=(numClient->c+=incr);
    UNLOCK_RETURN(&(numClient->mtx),-1);
    return r;
}
