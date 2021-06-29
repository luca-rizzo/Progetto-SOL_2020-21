#include<util.h>
#include<api.h>
#include<conn.h>
int fd_skt=-1;
char* socketname=NULL;
int openConnection(const char* sockname, int msec, const struct timespec abstime){
    struct sockaddr_un sa;
    int len=strlen(sockname);
    NULLSYSCALL(socketname, malloc(len+1),"malloc");
    strncpy(socketname, sockname, len+1);
    strncpy(sa.sun_path, socketname, len+1);
    sa.sun_family = AF_UNIX;
    NEGSYSCALL(fd_skt,socket(AF_UNIX, SOCK_STREAM,0),"socket");
    while(connect(fd_skt,(struct sockaddr *) &sa, sizeof(sa)) == -1){
        if(errno == ENOENT){
            printf("ciaoo\n");
            sleep(10);
            continue;
        }
        else{
            return -1;
        }
    }
    return 0;
}
int closeConnection(const char* sockname){
    int len=strlen(sockname);
    if(strncmp(sockname,socketname,len+1)!=0){
        errno=ENOTCONN;
        return -1;
    }
    int notused;
    NEGSYSCALL(notused, close(fd_skt), "close");
    free(socketname);
    socketname=NULL;
    fd_skt=-1;
    return 0;
}
int openFile(const char* pathname,int flags){
    if(pathname==NULL){
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=op; //open
    if(writen(fd_skt,&operazione,sizeof(t_op))==-1){//secondo il protocollo di comunicazione invio operazione da effettuare
        return -1; //errno settata da writen
    }
    int len=strlen(pathname)+1;
    if(writen(fd_skt,&len,sizeof(int))==-1){//secondo il protocollo di comunicazione invio lunghezza path, path del file da aprire e relativi flag
        return -1;
    }
    if(writen(fd_skt,(void*)pathname,len)==-1){
        return -1;
    }
    if(writen(fd_skt,&flags,sizeof(int))==-1){
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
    //l'operazione è terminata correttamente -> il file è stato aperto
    return 0;
}
int readFile(const char* pathname, void** buf, size_t* size){
    if(pathname==NULL){
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){
        errno=ENOTCONN;
        return -1;
    }
    t_op operazione=rd;
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
    //l'operazione è terminata correttamente
    size_t dim;
    if(readn(fd_skt,&dim,sizeof(size_t))==-1){ //leggo la dimensione del file
        return -1;
    }
    void* buffer=NULL;
    if(dim!=0){
        buffer=malloc(dim); //alloco un buffer di tale dimensione
        if(readn(fd_skt,buffer,dim)==-1){ //leggo contenuto file
            return -1;
        }
    }
    *size=dim; //ritorno byte letti
    (*buf)=buffer; //ritorno contenuto file
    return 0;
}
//permette di scrivere sul server un file solo se l'ultima operazione sul file è stata la sua creazione
int writeFile(const char* pathname, const char* dirname){
    if(pathname==NULL){
        errno =EINVAL;
        return -1;
    }
    if(fd_skt==-1){
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
    }
    //secondo il protocollo di comunicazione invio dimensione file e contenuto
    if(writen(fd_skt,&size,sizeof(size_t))==-1){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    if(writen(fd_skt, buffer, size)==-1){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    int res;
    if(readn(fd_skt,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione
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
    }
    free(buffer);
    if(fclose(ifp)!=0){
        return -1;
    }
    //operazione terminata correttamente
    return 0;
}
int removeFile(const char* pathname){
    if(pathname==NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_skt==-1){
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
