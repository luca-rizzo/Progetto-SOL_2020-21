#include<util.h>
#include<api.h>
#include<conn.h>

//*****VARIABILI GLOBALI*****//

//fd del socket file associato alla connessione AF_UNIX con il server
int fd_skt=-1;

//nome del socket file al quale sono connesso
const char* socketname=NULL;

//*****FUNZIONI PRIVATE DI IMPLEMENTAZIONE*****//

/*
    @funzione ottieniDirDaPath
    @descrizione: permette di ottenere il path della directory in cui è contenuto il file con pathname path
    @param path è il path del file di cui voglio sapere path directory
    @return una stringa allocata sullo heap contente il path della directory in caso di successo, NULL altrimenti, setta errno
*/
static char* ottieniDirDaPath(char* path);

/*
    @funzione leggiNFileinDir
    @descrizione: permette di leggere n file (inviati dal server) tramite fd_skt e salvarli nella directory dirname
    @param n è il numero di file da leggere
    @param dirname è la directory in cui salvare i file letti
    @param fd_skt è l'fd del socket da cui leggere i file
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno

*/
static int leggiNFileinDir(int n, const char* dirname, int fd_skt);

/*
    source: https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
    @funzione mkdir_r
    @descrizione: permette di creare ricorsivamente le directory secondo quanto indicato da path
    @param path indica quali directory creare ricorsivamente
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
static int mkdir_p(const char *path);

/*
    @funzione: difftimespec
    @descrizione: permette di effettuare la differenza tra due tipi di dato struct timespec
    @param begin indica il tempo iniziale
    @param end indica tempo finale
    @return una struct timespec che contiene la differenza tra le due strutture dato timespec
*/
static struct timespec difftimespec(struct timespec begin, struct timespec end);

//*****FUNZIONI DI IMPLEMENTAZIONE API*****//

int openConnection(const char* sockname, int msec, const struct timespec abstime){
    if(sockname==NULL || msec<0){ 
        //argomenti invalidi
        errno=EINVAL;
        return -1;
    }
    int len=strlen(sockname);
    if(socketname!=NULL){
        if(strncmp(sockname,socketname,len)==0){ 
            //sei già connesso al server
            errno=EISCONN;
            return -1;
        }
    }
    int res;
    struct timespec timeToRetry,tempoInizio,tempoCorrente,tempoPassato;
    struct sockaddr_un sa;
    //imposto il tempo che deve intercorrere tra due richieste di connessione
    timeToRetry.tv_sec = msec / 1000;
    timeToRetry.tv_nsec = (msec % 1000) * 1000000;
    //inizializzo sa
    memset(&sa, '0', sizeof(sa));
    if(strlen(sockname)+1 > UNIX_PATH_MAX){ 
        //sockname deve essere lungo al massimo UNIX_PATH_MAX
        errno=EINVAL;
        return -1;
    }
    strncpy(sa.sun_path, sockname, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if((fd_skt=socket(AF_UNIX, SOCK_STREAM,0))==-1){
        return -1;
    }
    //inizializzo tempo inizio
    if(clock_gettime(CLOCK_MONOTONIC,&tempoInizio)==-1){
        fd_skt=-1;
        return -1;
    }
    //inizializzo tempo corrente
    if(clock_gettime(CLOCK_MONOTONIC,&tempoCorrente)==-1){
        fd_skt=-1;
        return -1;
    }
    //inizializzo tempo passato
    tempoPassato=difftimespec(tempoInizio,tempoCorrente);
    while(BThenA(abstime,tempoPassato) && (res=connect(fd_skt,(struct sockaddr *) &sa, sizeof(sa))) == -1){
        fprintf(stdout,"Provo a ricollegarmi\n");
        //aspetto msec prima di tentare la riconnessione
        if(nanosleep(&timeToRetry,NULL)!=0){
            fd_skt=-1;
            return -1;
        }
        //aggiorno tempo corrente
        if(clock_gettime(CLOCK_MONOTONIC,&tempoCorrente)==-1){
            fd_skt=-1;
            return -1;
        }
        //aggiorno tempo passato
        tempoPassato=difftimespec(tempoInizio,tempoCorrente);
    }
    if(res==-1){ 
        // sono uscito perchè è scaduto il timer
        fd_skt=-1;
        errno = ETIMEDOUT;
        return -1;
    }
    socketname=sockname;
    return 0;
}

int closeConnection(const char* sockname){
    if(sockname==NULL){
        //argomenti invalidi
        return -1;
    }
    int len=strlen(sockname);
    if(strncmp(sockname,socketname,len+1)!=0){
        //non sei connesso al server
        errno=ENOTCONN;
        return -1;
    }
    //chiudo la connessione con il server resettando variabili globali
    socketname=NULL;
    if(close(fd_skt)==-1){
        return -1;
    }
    fd_skt=-1;
    return 0;
}

int openFile(const char* pathname,int flags){
    if(pathname==NULL){ 
        //argomenti non validi
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){ 
        //non sei connesso al server
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=op; //open
    //secondo il protocollo di comunicazione invio operazione da effettuare
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path, path del file da aprire e relativi flag

    //invio lunghezza path
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
    int res, nread;
    //leggo il risultato dell'operazione
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    if(res!=0){
        //l'operazione è fallita -> setto errno col valore ritornato dal server e ritorno -1;
        errno=res;
        return -1;
    } 
    //l'operazione è terminata correttamente -> il file è stato aperto/creato
    return 0;
}
int readFile(const char* pathname, void** buf, size_t* size){
    if(pathname==NULL){ 
        //argomenti non validi
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){ 
        //non sei connesso al server
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=rd;
    //secondo il protocollo di comunicazione invio operazione da effettuare
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path e path del file da leggere

    //invio lunghezza path
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){
        return -1;
    }
    //invio contenuto path
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    }
    //leggo il risultato dell'operazione
    int res, nread;
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    if(res!=0){ 
        //l'operazione è fallita -> setto errno col valore ritornato dal server e ritorno -1;
        errno=res;
        return -1;
    } 
    //l'operazione è terminata correttamente
    size_t dim;
    //leggo la dimensione del file
    if((nread=readn(fd_skt,&dim,sizeof(size_t)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno = EBADMSG;
        return -1;
    }
    void* buffer=NULL;
    if(dim!=0){
        //alloco un buffer di tale dimensione
        buffer=malloc(dim); 
        //leggo contenuto file
        if(readn(fd_skt,buffer,dim)==-1){ 
            return -1;
        }
        if(nread==0){
            //errore nel protocollo di comunicazione
            free(buffer);
            errno = EBADMSG;
            return -1;
        }
    }
    //ritorno bytes letti
    *size=dim; 
    //ritorno contenuto file
    (*buf)=buffer; 
    //l'operazione è terminata correttamente -> il file è stato letto
    return 0;
}

int writeFile(const char* pathname, const char* dirname){
    if(pathname==NULL){ 
        //argomenti non validi
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){ 
        //non sei connesso
        errno=ENOTCONN;
        return -1;
    }
    FILE* ifp;
    //apro il file presente nel mio file system
    if((ifp=fopen(pathname,"rb"))==NULL){ 
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
    //alloco un buffer di tale dimensione
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
    //chiudo il file
    if(fclose(ifp)!=0){ 
        return -1;
    }
    //abbiamo letto il contenuto del file in buffer; adesso lo mandiamo al server

    //secondo il protocollo di comunicazione invio operazione da effettuare
    t_op operazione=wr;
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){
        free(buffer);
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path e path del file da scrivere

    //invio lunghezza path
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){
        free(buffer);
        return -1;
    }
    //invio path
    if(writen(fd_skt,(void*)pathname,len)==-1){
        free(buffer);
        return -1;
    } 
    //posso inviare il file?

    int res, nread;
    //leggo risultato operazione fino a questo punto
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        free(buffer);
        return -1;
    }
    if(nread==0){
        free(buffer);
        errno=EBADMSG;
        //errore nel protocollo di comunicazione
        return -1;
    }
    if(res!=0){
        free(buffer);
        errno=res;
        //l'operazione è fallita -> setto errno col valore ritornato dal server e ritorno -1;
        return -1;
    } 
    
    //posso inviare il file

    //secondo il protocollo di comunicazione invio dimensione file e contenuto
    if(writen(fd_skt,&size,sizeof(size_t))==-1){
        free(buffer);
        return -1;
    }
    //se il file non è vuoto invio contenuto
    if(size>0){
        if(writen(fd_skt, buffer, size)==-1){
            free(buffer);
            return -1;
        }
    }
    free(buffer);
    //leggo risultato operazione fino a questo punto
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    if(res!=0){
        errno=res;
        //l'operazione è fallita -> setto errno col valore ritornato dal server e ritorno -1;
        return -1;
    }
    //operazione completata correttamente->contenuto scritto sul file

    //leggo numero file espulsi
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    //leggo file espulsi e li salvo in dirname
    if(leggiNFileinDir(res,dirname,fd_skt)==-1){
        return -1;
    }
    return 0;
}
int removeFile(const char* pathname){
    if(pathname==NULL){
        //argomenti non validi
        errno = EINVAL;
        return -1;
    }
    if(fd_skt==-1){ 
        //non sei connesso
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=re;
    //secondo il protocollo di comunicazione invio operazione da effettuare
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path e path del file da leggere
    //invio lunghezza path
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){
        return -1;
    }
    //invio contenuto path
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    }

    //leggo risultato operazione
    int res, nread;
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    if(res!=0){
        //l'operazione è fallita -> setto errno col valore ritornato dal server e ritorno -1;
        errno=res;
        return -1;
    } 
    //l'operazione è terminata correttamente -> il file è stato cancellato
    return 0;
}
int closeFile(const char* pathname){
    if(pathname==NULL){
        //argomenti non validi
        errno = EINVAL;
        return -1;
    }
    if(fd_skt==-1){
        //non sei connesso
        errno=ENOTCONN;
        return -1;
    }
    //secondo il protocollo di comunicazione invio operazione da effettuare
    t_op operazione=cl;
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path e path del file da leggere
    //invio lunghezza path
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){
        return -1;
    }
    //invio contenuto path
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    }
    //leggo risultato operazione
    int res, nread;
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    if(res!=0){
        //l'operazione è fallita -> setto errno col valore ritornato dal server e ritorno -1;
        errno=res;
        return -1;
    } 
    //l'operazione è terminata correttamente -> il file è stato chiuso
    return 0;
}
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
    if(pathname==NULL){
        //argomenti invalidi
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){
        //non sei connesso
        errno=ENOTCONN;
        return -1;
    }
    //secondo il protocollo di comunicazione invio operazione da effettuare
    t_op operazione=ap;
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path e path del file da scrivere

    //invio lunghezza path
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){
        return -1;
    }
    //invio contenuto path
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    } 

    //posso inviare il contenuto da aggiungere al file? leggo risultato operazione fino a questo punto
    int res, nread;
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    if(res!=0){
        //l'operazione è fallita -> setto errno col valore ritornato dal server e ritorno -1;
        errno=res;
        return -1;
    } 
    //posso inviare buf

    //secondo il protocollo di comunicazione invio dimensione buf e buf

    //invio dimensione
    if(writen(fd_skt,&size,sizeof(size_t))==-1){
        return -1;
    }
    //invio contenuto
    if(writen(fd_skt, buf, size)==-1){
        return -1;
    }
    //leggo il risultato dell'operazione
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    if(res!=0){
        //l'operazione è fallita -> setto errno col valore ritornato dal server e ritorno -1;
        errno=res;
        return -1;
    }
    //operazione terminata correttamente-> contenuto appeso al file

    //leggo numero file espulsi
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    //leggo file espulsi e li salvo in dirname
    if(leggiNFileinDir(res,dirname,fd_skt)==-1){
        return -1;
    }
    return 0;    
}
int readNFiles(int N, const char* dirname){
    if(fd_skt==-1){
        //non sei connesso al server
        errno=ENOTCONN;
        return -1;
    }
    //secondo il protocollo di comunicazione invio operazione da effettuare
    t_op operazione=rn;
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio numero file da leggere
    if(writen(fd_skt, &N ,sizeof(int))==-1){
        return -1; //errno settata da writen
    }

    //leggo risultato operazione fino a questo punto
    int res, nread;
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    if(res!=0){
        //l'operazione è fallita -> setto errno col valore ritornato dal server e ritorno -1;
        errno=res;
        return -1;
    }
    //leggo numero file letti
    if((nread=readn(fd_skt,&res,sizeof(int)))==-1){ 
        return -1;
    }
    if(nread==0){
        //errore nel protocollo di comunicazione
        errno=EBADMSG;
        return -1;
    }
    //l'operazione è terminata correttamente: il server ha letto un numero di file pari a res e li sta inviando
    if(leggiNFileinDir(res, dirname,fd_skt)==-1){
        return -1;
    }
    return res;
}

static int leggiNFileinDir(int n, const char* dirname, int fd_skt){
    int len;  
    void* path;
    int nread;
    int notAllFile=0;
    for(int i=0;i<n;i++){
        //secondo il protocollo di comunicazione leggo lunghezza path e path

        //secondo il protocollo di comunicazione leggo lunghezza path
        if((nread=readn(fd_skt,&len,sizeof(int)))==-1){ 
            return -1; //setta errno
        }
        if(nread==0){
            //errore protocollo di comunicazione
            errno=EBADMSG;
            return -1;
        }
        path=malloc(len);
        if(path==NULL){
            return -1;
        }
        //secondo il protocollo di comunicazione leggo contenuto path
        if((nread=readn(fd_skt,path,len))==-1){
            free(path);
            return -1; //setta errno
        }
        if(nread==0){
            //errore protocollo di comunicazione
            free(path);
            errno=EBADMSG;
            return -1;
        }
        //secondo il protocollo di comunicazione leggo dimensione file e contenuto
        
        //leggo la dimensione del file
        size_t dim;
        if((nread=readn(fd_skt,&dim,sizeof(size_t)))==-1){ 
            return -1;
        }
        if(nread==0){
            free(path);
            errno=EBADMSG;
            return -1;
        }
        void* buffer=NULL;
        if(dim!=0){
            //alloco un buffer di tale dimensione
            buffer=malloc(dim); 
            if(buffer==NULL){
                free(path);
                return -1;
            }
            //leggo contenuto file
            if((nread=readn(fd_skt,buffer,dim))==-1){ 
                free(buffer);
                free(path);
                return -1;
            }
            if(nread==0){
                //errore protocollo di comunicazione
                free(buffer);
                free(path);
                errno=EBADMSG;
                return -1;
            }
        }
        if(dirname==NULL){ 
            //non devo salvare i file letti
            free(path);
            if(buffer!=NULL)
                free(buffer);
            continue;
        }
        //salvo il file che ho letto
        if(scriviContenutoInDirectory(buffer,dim,path,(char*) dirname)==-1){
            fprintf(stderr,"Errore nella scrittura su disco del file %s\n", (char*) path);
            perror("scriviContenutoInDirectory");
            notAllFile=1;
            //continuo comunque a salvare i file che riesco a salvare
        }
        free(path);
        if(buffer!=NULL)
            free(buffer);
    }
    if(notAllFile==1){
        errno=EIO;
        return -1;
    }
    return 0;
}

int scriviContenutoInDirectory(void* buffer, size_t size, char* pathFile, char* dirToSave){
    FILE* ifp;
    char* dirToCreate = ottieniDirDaPath(pathFile);
    if(dirToCreate!=NULL){ 
        //devo creare delle directory in dirToSave
        char* fullPath = malloc(PATH_MAX);
        if(fullPath==NULL){
            return -1;
        }
        snprintf(fullPath,PATH_MAX,"%s/%s",dirToSave,dirToCreate);
        //creo ricorsivamente le directory in cui salvare il file
        if(mkdir_p(fullPath)==-1){ 
            return -1;
        }
        free(fullPath);
        free(dirToCreate);
    }
    //creo il nuovo path del file concantenando dirToSave con il path che il file aveva nel server
    int dimNewPath=strlen(pathFile)+strlen(dirToSave)+1;
    char* newPath=malloc(dimNewPath);
    if(newPath==NULL){
        return -1;
    }
    snprintf(newPath, dimNewPath, "%s%s", dirToSave, pathFile);
    //creo il file nel mio file system locale
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

static char* ottieniDirDaPath(char* path){
    if(path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    //faccio una copia del path completo
    int len = strlen(path)+1;
    char* dup = malloc(len);
    if(dup==NULL)
        return NULL;
    strncpy(dup,path,len);
    char* ptr = strrchr(dup, '/');
    if (ptr==NULL) { 
        //se non ho / il file si trova nella directory corrente
        return dup;
    }
    else{
        *ptr='\0'; //elimino il nome del file inserendo un carattere di terminazione stringa
    }
    return dup;
}

static struct timespec difftimespec(struct timespec begin, struct timespec end){
    struct timespec timepass;
    timepass.tv_sec=end.tv_sec - begin.tv_sec;
    timepass.tv_nsec=end.tv_nsec - begin.tv_nsec;
    return timepass;
}

static int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p; 

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1; 
    }   
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return -1; 
            }

            *p = '/';
        }
    }   

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
            return -1; 
    }   
    return 0;
}