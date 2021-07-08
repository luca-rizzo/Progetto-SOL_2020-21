#include<client.h>
int eseguiRichieste(t_coda* richieste, config_client config){
    int err;
    void* buffer;
    size_t size;
    nodo* p;
    struct timespec time;
    time.tv_sec = config.millisec / 1000;
    time.tv_nsec = (config.millisec % 1000) * 1000000;
    p=prelevaDaCoda(richieste);
    while(p!=NULL){
        richiesta* r=(richiesta*)p->val;
        switch(r->op){
            case WriteFile:
                if(openFile(r->path,O_CREAT)==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Scrittura file \nFile di riferimento: %s\nEsito: Negativo\nMotivazione: ", r->path);
                        perror("API--openFile");
                        fprintf(stderr,"\n");
                    }
                    break;
                }
                if((err=writeFile(r->path,r->pathToSave))==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Scrittura file \nFile di riferimento: %s\nEsito: Negativo\nMotivazione: ", r->path);
                        perror("API--writeFile");
                        fprintf(stderr,"\n");
                    }
                }//devo comunque chiudere il file
                if(closeFile(r->path)==-1){
                    if(config.stampaStOut){
                        perror("API--closeFile");
                        fprintf(stderr,"\n");
                    }
                    break;
                }
                if(err!=-1 && config.stampaStOut)
                    fprintf(stdout,"Tipo Operazione: Scrittura file \nFile di riferimento: %s\nEsito: Positivo\n\n", r->path);
                break;
            case ReadFile:
                if(openFile(r->path,-1)==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Negativo\nMotivazione: ", r->path);
                        perror("API--openFile");
                        fprintf(stderr,"\n");
                    }
                    break;
                }
                if((err=readFile(r->path,&buffer,&size))==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Negativo\nMotivazione: ", r->path);
                        perror("API--readFile");
                        fprintf(stderr,"\n");
                    }
                }//devo comunque chiudere il file
                if(closeFile(r->path)==-1){
                    if(config.stampaStOut){
                        perror("API--closeFile");
                        fprintf(stderr,"\n");
                    }
                    break;
                }
                if(err!=-1){
                    if(r->pathToSave==NULL){
                        if(config.stampaStOut)
                            fprintf(stdout,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Positivo\nFile non salvato su disco\n\n", r->path);
                    }
                    else{
                        if(scriviContenutoInDirectory(buffer,size,r->path,r->pathToSave)==-1){
                            fprintf(stderr,"Errore nella scrittura su disco del file %s\n", (char*) r->path);
                            perror("scriviContenutoInDirectory");
                            if(config.stampaStOut)
                                fprintf(stdout,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Positivo\nFile non salvato su disco\n\n", r->path);
                        }  
                        else{
                            if(config.stampaStOut)
                            fprintf(stdout,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Positivo\nFile salvati su disco nella directory: %s\n\n", r->path, r->pathToSave);
                        }
                    }
                    if(buffer!=NULL)
                        free(buffer);
                }
                break;
            case DeleteFile:
                if(openFile(r->path,-1)==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Rimozione file \nFile di riferimento: %s\nEsito: Negativo\nMotivazione: ", r->path);
                        perror("API--openFile");
                        fprintf(stderr,"\n");
                    }
                    break;
                }
                if(removeFile(r->path)==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Rimozione file \nFile di riferimento: %s\nEsito: Negativo\nMotivazione: ", r->path);
                        perror("API--removeFile");
                        fprintf(stderr,"\n");
                    }
                    //devo comunque chiudere il file perchè non lo ho eliminato correttamente
                    if(closeFile(r->path)==-1){
                        if(config.stampaStOut){
                            perror("API--closeFile");
                            fprintf(stderr,"\n");
                        }
                    }
                    break;
                }
                if(config.stampaStOut)
                    fprintf(stdout,"Tipo Operazione: Rimozione file \nFile di riferimento: %s\nEsito: Positivo\n\n", r->path);
                break;
            case ReadNFiles:{
                int nread;
                if((nread=readNFiles(r->n,r->pathToSave))==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Lettura N file \nEsito: Negativo\nMotivazione: ");
                        perror("API--readNFiles");
                        fprintf(stderr,"\n");
                    }
                }
                else{
                    if((r->pathToSave)!=NULL){
                        if(config.stampaStOut)
                            fprintf(stdout,"Tipo Operazione: Lettura N file \nEsito: Positivo\nFile letti: %d\nSalvati nella directory:%s\n\n", nread,r->pathToSave);
                    }
                    else{
                        if(config.stampaStOut)
                            fprintf(stdout,"Tipo Operazione: Lettura N file \nEsito: Positivo\nFile letti: %d\nFile non salvati su disco\n", nread);
                    }
                }
                break;
            }
            default:
                break;
        }
        nanosleep(&time,NULL);
        freeRichiesta(p->val);
        free(p);
        p=prelevaDaCoda(richieste);
    }
    return 0;
}
