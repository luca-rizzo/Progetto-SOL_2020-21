#include<util.h>
#include<api.h>
#include<conn.h>
int fd_skt=-1;
const char* socketname=NULL;
static char* ottieniNomeDaPath(char* path);
static struct timespec difftimespec(struct timespec begin, struct timespec end);
static int BThenA(struct timespec a,struct timespec b);
//permette di aprire una connessione con il server tramite il socket file sockname
//ritorna 0 in caso di successo; -1 in caso di fallimento, setta errno
int openConnection(const char* sockname, int msec, const struct timespec abstime){
    if(fd_skt!=-1){ //sei già connesso
        errno=EISCONN;
        return -1;
    }
    if(sockname==NULL){ //parametri invalidi
        errno=EINVAL;
        return -1;
    }
    int res;
    struct sockaddr_un sa;
    memset(&sa, '0', sizeof(sa));
    struct timespec timeToRetry,tempoInizio,tempoCorrente,tempoPassato;
    timeToRetry.tv_sec = msec / 1000;
    timeToRetry.tv_nsec = (msec % 1000) * 1000000;
    if(strlen(sockname)+1 > UNIX_PATH_MAX){ //sockname deve essere lungo al massimo UNIX_PATH_MAX
        errno=EINVAL;
        return -1;
    }
    strncpy(sa.sun_path, sockname, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if((fd_skt=socket(AF_UNIX, SOCK_STREAM,0))==-1){
        return -1;
    }
    if(clock_gettime(CLOCK_MONOTONIC,&tempoInizio)==-1){//inizializzo tempo inizio
        fd_skt=-1;
        return -1;
    }
    if(clock_gettime(CLOCK_MONOTONIC,&tempoCorrente)==-1){//inizializzo tempo corrente
        fd_skt=-1;
        return -1;
    }
    tempoPassato=difftimespec(tempoInizio,tempoCorrente);
    while(BThenA(abstime,tempoPassato) && (res=connect(fd_skt,(struct sockaddr *) &sa, sizeof(sa))) == -1){
        fprintf(stdout,"Provo a ricollegarmi\n");
        if(nanosleep(&timeToRetry,NULL)!=0){
            fd_skt=-1;
            return -1;
        }
        if(clock_gettime(CLOCK_MONOTONIC,&tempoCorrente)==-1){//aggiorno tempo corrente
            fd_skt=-1;
            return -1;
        }
        tempoPassato=difftimespec(tempoInizio,tempoCorrente);
    }
    if(res==-1){ // sono uscito perchè è scaduto il timer
        fd_skt=-1;
        errno = ETIMEDOUT;
        return -1;
    }
    socketname=sockname;
    return 0;
}
//permette di chiudere la connessione associata al socket file sockname. 
//ritorna 0 in caso di successo, -1 in caso di fallimento, setta errno
int closeConnection(const char* sockname){
    int len=strlen(sockname);
    if(strncmp(sockname,socketname,len+1)!=0){
        errno=ENOTCONN;
        return -1;
    }
    socketname=NULL;
    if(close(fd_skt)==-1){
        return -1;
    }
    fd_skt=-1;
    return 0;
}
//permette di aprire il file pathname nel server; se flags=O_CREAT permette di creare il file (se questo non esiste)
//ritorna 0 in caso di successo, -1 in caso di fallimento, setta errno;
int openFile(const char* pathname,int flags){
    if(pathname==NULL){ //parametri non validi
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){ //non sei connesso al server
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=op; //open
    //secondo il protocollo di comunicazione invio operazione da effettuare
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path, path del file da aprire e relativi flag
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){
        return -1;
    }
    //invio path
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    }
    //invio flag
    if(writen(fd_skt,&flags,sizeof(int))==-1){
        return -1;
    }
    int res;
    //leggo il risultato dell'operazione
    if(readn(fd_skt,&res,sizeof(int))==-1){ 
        return -1;
    }
    if(res!=0){
        errno=res;
        return -1;
    } 
    //l'operazione è terminata correttamente -> il file è stato aperto
    return 0;
}
int readFile(const char* pathname, void** buf, size_t* size){
    if(pathname==NULL){ //parametri non validi
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){ //non sei connesso al server
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=rd;
    //secondo il protocollo di comunicazione invio operazione da effettuare
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path e path del file da leggere
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){
        return -1;
    }
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    }
    //leggo il risultato dell'operazione
    int res;
    if(readn(fd_skt,&res,sizeof(int))==-1){ 
        return -1;
    }
    if(res!=0){
        errno=res;
        return -1;
    } 
    //l'operazione è terminata correttamente
    size_t dim;
    int nread;
    //leggo la dimensione del file
    if((nread=readn(fd_skt,&dim,sizeof(size_t)))==-1){ 
        return -1;
    }
    if(nread==0){
        errno = EBADMSG;
        return -1;
    }
    void* buffer=NULL;
    if(dim!=0){
        buffer=malloc(dim); //alloco un buffer di tale dimensione
        if(readn(fd_skt,buffer,dim)==-1){ //leggo contenuto file
            return -1;
        }
        if(nread==0){
            free(buffer);
            errno = EBADMSG;
            return -1;
        }
    }
    *size=dim; //ritorno byte letti
    (*buf)=buffer; //ritorno contenuto file
    //l'operazione è terminata correttamente -> il file è stato letto
    return 0;
}
//permette di scrivere sul server un file solo se l'ultima operazione sul file è stata la sua creazione
int writeFile(const char* pathname, const char* dirname){
    if(pathname==NULL){ //parametri non validi
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){ //non sei connesso
        errno=ENOTCONN;
        return -1;
    }
    FILE* ifp;
    if((ifp=fopen(pathname,"rb"))==NULL){ //apro il file
        return -1; //setta errno
    }
    //scopro la dimensione del file
    if(fseek(ifp, 0, SEEK_END)!=0){
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    size_t size = ftell(ifp);
    if(size==-1){
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    rewind(ifp);
    void* buffer= malloc(size);
    if(buffer==NULL){
        
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    //leggo contenuto file in buffer
    if(fread(buffer,1,size,ifp)<size){
        free(buffer);
        errno = EIO;
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    //abbiamo letto il contenuto del file in buffer; adesso lo mandiamo al server
    t_op operazione=wr;
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){//secondo il protocollo di comunicazione invio operazione da effettuare
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path e path del file da scrivere
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    if(writen(fd_skt,(void*)pathname,len)==-1){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    } //posso inviare il file?
    int res;
    if(readn(fd_skt,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione fino a questo punto
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    if(res!=0){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        errno=res;
        return -1;
    } //posso inviare il file
    //secondo il protocollo di comunicazione invio dimensione file e contenuto
    if(writen(fd_skt,&size,sizeof(size_t))==-1){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    //se il file non è vuoto invio contenuto
    if(size>0){
        if(writen(fd_skt, buffer, size)==-1){
            free(buffer);
            if(fclose(ifp)!=0){
                return -1;
            }
        return -1;
        }
    }
    free(buffer);
    if(readn(fd_skt,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    if(res!=0){
        if(fclose(ifp)!=0){
            return -1;
        }
        errno=res;
        return -1;
    }
    if(fclose(ifp)!=0){
        return -1;
    }
    //l'operazione è terminata correttamente -> il file è stato scritto
    return 0;
}
int removeFile(const char* pathname){
    if(pathname==NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_skt==-1){ //non sei connesso
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=re;
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){//secondo il protocollo di comunicazione invio operazione da effettuare
        return -1; //errno settata da writen
    }
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){//secondo il protocollo di comunicazione invio lunghezza path e path del file da leggere
        return -1;
    }
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    }
    int res;
    if(readn(fd_skt,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione
        return -1;
    }
    if(res!=0){
        errno=res;
        return -1;
    } 
    //l'operazione è terminata correttamente -> il file è stato cancellato
    return 0;
}
int closeFile(const char* pathname){
    if(pathname==NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_skt==-1){//non sei connesso
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=cl;
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){//secondo il protocollo di comunicazione invio operazione da effettuare
        return -1; //errno settata da writen
    }
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){//secondo il protocollo di comunicazione invio lunghezza path e path del file da leggere
        return -1;
    }
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    }
    int res;
    if(readn(fd_skt,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione
        return -1;
    }
    if(res!=0){
        errno=res;
        return -1;
    } 
    //l'operazione è terminata correttamente -> il file è stato chiuso
    return 0;
}
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
    if(pathname==NULL){
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=ap;
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){//secondo il protocollo di comunicazione invio operazione da effettuare
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path e path del file da scrivere
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){
        return -1;
    }
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    } //posso inviare il contenuto da aggiungere al file?
    int res;
    if(readn(fd_skt,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione fino a questo punto
        return -1;
    }
    if(res!=0){
        errno=res;
        return -1;
    } //posso inviare buf

    //secondo il protocollo di comunicazione invio dimensione file e contenuto
    //invio dimensione
    if(writen(fd_skt,&size,sizeof(size_t))==-1){
        return -1;
    }
    //invio contenuto
    if(writen(fd_skt, buf, size)==-1){
        return -1;
    }
    //leggo il risultato dell'operazione
    if(readn(fd_skt,&res,sizeof(int))==-1){ 
        return -1;
    }
    if(res!=0){
        errno=res;
        return -1;
    }
    //operazione terminata correttamente-> contenuto appeso al file
    return 0;    
}
int readNFiles(int N, const char* dirname){
    if(fd_skt==-1){
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=rn;
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){//secondo il protocollo di comunicazione invio operazione da effettuare
        return -1; //errno settata da writen
    }
    if(writen(fd_skt, &N ,sizeof(int))==-1){//secondo il protocollo di comunicazione invio numero file da leggere
        return -1; //errno settata da writen
    }

    int res;
    if(readn(fd_skt,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione
        return -1;
    }
    if(res==-1){
        errno=res;
        return -1;
    } 
    //l'operazione è terminata correttamente: il server ha letto un numero di file pari a res
    
    int len;  
    void* path;
    int nread;

    for(int i=0;i<res;i++){
        if((nread=readn(fd_skt,&len,sizeof(int)))==-1){ //secondo il protocollo di comunicazione leggo lunghezza path e path
            return -1; //setta errno
        }
        if(nread==0){
            errno=EBADMSG;
            return -1;
        }
        path=malloc(len);
        if(path==NULL){
            return -1;
        }
        if((nread=readn(fd_skt,path,len))==-1){
            free(path);
            return -1; //setta errno
        }
        if(nread==0){
            free(path);
            errno=EBADMSG;
            return -1;
        }
        //secondo il protocollo di comunicazione leggo dimensione file e contenuto
        size_t dim;
        if(readn(fd_skt,&dim,sizeof(size_t))==-1){ //leggo la dimensione del file
            return -1;
        }
        void* buffer=NULL;
        if(dim!=0){
            buffer=malloc(dim); //alloco un buffer di tale dimensione
            if(buffer==NULL){
                free(path);
                return -1;
            }
            if(readn(fd_skt,buffer,dim)==-1){ //leggo contenuto file
                free(path);
                return -1;
            }
        }
        if(dirname==NULL){ //non devo salvare i file letti
            free(path);
            if(buffer!=NULL)
                free(buffer);
            continue;
        }
        if(scriviContenutoInDirectory(buffer,dim,path,(char*) dirname)==-1){
            fprintf(stderr,"Errore nella scrittura su disco del file %s\n", (char*) path);
            perror("scriviContenutoInDirectory");
        }
        free(path);
        if(buffer!=NULL)
            free(buffer);
    }
    return res;
}
//scrive il contenuto di buffer nella directory dirToSave in un file il cui nome è ottenuto da pathFile
//ritorna 0 in caso di successo; -1 in caso di fallimento
int scriviContenutoInDirectory(void* buffer, size_t size, char* pathFile, char* dirToSave){
    FILE* ifp;
    char* nomeFile=ottieniNomeDaPath(pathFile);
    int dimNewPath=strlen(nomeFile)+strlen(dirToSave)+1;
    char* newPath=malloc(dimNewPath);
    if(newPath==NULL){
        return -1;
    }
    snprintf(newPath, dimNewPath, "%s%s", dirToSave, nomeFile);
    if((ifp=fopen(newPath,"wb"))==NULL){
        free(newPath);
        return -1;
    }
    if(buffer!=NULL){
        if(fwrite(buffer,1,size,ifp)<size){
            free(newPath);
            if(fclose(ifp)!=0){
                return -1;
            }
            return -1;
        }
    }
    free(newPath);
    if(fclose(ifp)!=0){
        return -1;
    }
    return 0;
}
static char* ottieniNomeDaPath(char* path){
    if(path == NULL) {
        return NULL;
    }
    char* name;
    if( (name = strrchr(path, '/')) == NULL )
        name = path;
    return name;
}
static struct timespec difftimespec(struct timespec begin, struct timespec end){
    struct timespec timepass;
    timepass.tv_sec=end.tv_sec - begin.tv_sec;
    timepass.tv_nsec=end.tv_nsec - begin.tv_nsec;
    return timepass;
}
//ritorna 1 se b è avvenuto prima di a
static int BThenA(struct timespec a,struct timespec b) {
    if (a.tv_sec == b.tv_sec)
        return a.tv_nsec > b.tv_nsec;
    else
        return a.tv_sec > b.tv_sec;
}
