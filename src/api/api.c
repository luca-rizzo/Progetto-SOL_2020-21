#include<api.h>
#include<util.h>
int fd_skt=-1;
char* socketname=NULL;
int openConnection(const char* sockname, int msec, const struct timespec abstime){
    struct sockaddr_un sa;
    NULLSYSCALL(sockname,malloc(UNIX_PATH_MAX),"malloc");
    strncpy(socketname, sockname, UNIX_PATH_MAX);
    strncpy(sa.sun_path, socketname, UNIX_PATH_MAX);
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
    if(strncmp(sockname,socketname,UNIX_PATH_MAX)!=0){
        return -1;
    }
    int notused;
    NEGSYSCALL(notused, close(fd_skt), "close");
    free(socketname);
    socketname=NULL;
    fd_skt=-1;
    return 0;
}
int OpenFile(const char* pathname,int flags){
    if(pathname==NULL){
        errno =ENOENT;
        return -1;
    }
    if(fd_skt==-1){
        errno=ENOTCONN;
        return -1;
    }
    if(writen(fd,"op",2)==-1){//secondo il protocollo di comunicazione invio operazione da effettuare
        return -1; //errno settata da writen
    }
    int len=strlen(pathname);
    if(writen(fd,&len,sizeof(int))==-1){//secondo il protocollo di comunicazione invio lunghezza path, path del file da aprire e relativi flag
        return -1;
    }
    if(writen(fd,pathname,len)==-1){
        return -1;
    }
    if(writen(fd,&flags,sizeof(int))==-1){
        return -1;
    }
    int res;
    if(readn(fd,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione
        return -1;
    }
    if(r!=0){
        errno=r;
        return -1;
    } 
    //l'operazione è terminata correttamente -> il file è stato aperto
    return 0;
}
int readFile(const char* pathname, void** buf, size_t* size){
    if(pathname==NULL){
        errno =ENOENT;
        return -1;
    }
    if(fd_skt==-1){
        errno=ENOTCONN;
        return -1;
    }
    if(writen(fd,"rd",2)==-1){//secondo il protocollo di comunicazione invio operazione da effettuare
        return -1; //errno settata da writen
    }
    int len=strlen(pathname);
    if(writen(fd,&len,sizeof(int))==-1){//secondo il protocollo di comunicazione invio lunghezza path e path del file da leggere
        return -1;
    }
    if(writen(fd,pathname,len)==-1){
        return -1;
    }
    int res;
    if(readn(fd,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione
        return -1;
    }
    if(r!=0){
        errno=r;
        return -1;
    } 
    //l'operazione è terminata correttamente
    if(readn(fd,&len,sizeof(int))==-1){ //leggo la dimensione del file
        return -1;
    }
    char* buffer=malloc(sizeof(len)); //alloco un buffer di tale dimensione
    if(readn(fd,buffer,len)==-1){ //leggo contenuto file
        return -1;
    }
    *size=len; //ritorno byte letti
    (*buf)=buffer; //ritorno contenuto file
    return 0;
}
//permette di scrivere sul server un file solo se l'ultima operazione sul file è stata la sua creazione
int WriteFile(const char* pathname, const chear* dirname){
    if(pathname==NULL){
        errno =ENOENT;
        return -1;
    }
    if(fd_skt==-1){
        errno=ENOTCONN;
        return -1;
    }
    FILE* ifp;
    if((ifp=fopen(pathname),"rb")==-1){ //apro il file
        return -1; //setta errno
    }
    //scopro la dimensione del file
    if(fseek(ifp, 0, SEEK_END)!=0){
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    long size = ftell(ifp);
    if(size==-1){
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1
    }
    rewind(ifp);
    void* buffer= malloc(sizeof(size));
    if(fread(buffer,1,size,ifp)<size){
        free(buffer);
        errno = EIO;
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    //abbiamo letto il contenuto del file in buffer; adesso lo mandiamo al server
    if(writen(fd,"wr",2)==-1){//secondo il protocollo di comunicazione invio operazione da effettuare
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1; //errno settata da writen
    }
    //secondo il protocollo di comunicazione invio lunghezza path e path del file da scrivere
    int len=strlen(pathname);
    if(writen(fd,&len,sizeof(int))==-1){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    if(writen(fd,pathname,len)==-1){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    //secondo il protocollo di comunicazione invio dimensione file e contenuto
    if(writen(fd,&size,sizeof(int))==-1){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    if(writen(fd, buffer, size)==-1){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    int res;
    if(readn(fd,&res,sizeof(int))==-1){ //leggo il risultato dell'operazione
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        return -1;
    }
    if(r!=0){
        free(buffer);
        if(fclose(ifp)!=0){
            return -1;
        }
        errno=r;
        return -1;
    }
    free(buffer);
    if(fclose(ifp)!=0){
        return -1;
    }
    //operazione terminata correttamente
    return 0;
}
