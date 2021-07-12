#include<client.h>
void eseguiRichieste(t_coda* richieste, config_client config){
    nodo* p;
    struct timespec time;
    //imposto il tempo che deve intercorrere tra l'invio di due richieste consecutive
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
                if(writeFile(r->path,r->pathToSave)==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Scrittura file \nFile di riferimento: %s\nEsito: Negativo\nMotivazione: ", r->path);
                        perror("API--writeFile");
                        fprintf(stderr,"\n");
                    }
                }
                else{
                    if(config.stampaStOut){
                        if(r->pathToSave!=NULL)
                            fprintf(stdout,"Tipo Operazione: Scrittura file \nFile di riferimento: %s\nEsito: Positivo\nGli eventuali file espulsi in seguito a capacity miss e inviati dal server vengono salvati nella directory: %s\n\n", r->path,r->pathToSave);
                        else
                            fprintf(stdout,"Tipo Operazione: Scrittura file \nFile di riferimento: %s\nEsito: Positivo\nGli eventuali file espulsi in seguito a capacity miss e inviati dal server verranno eliminati\n\n", r->path);
                    }
                }
                //devo comunque chiudere il file anche in caso di errore in seguito ad una writeFile
                if(closeFile(r->path)==-1){
                    if(config.stampaStOut){
                        perror("API--closeFile");
                        fprintf(stderr,"\n");
                    }
                    break;
                }
                break;
            case ReadFile:
                //apro il file
                if(openFile(r->path,-1)==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Negativo\nMotivazione: ", r->path);
                        perror("API--openFile");
                        fprintf(stderr,"\n");
                    }
                    break;
                }
                void* buffer;
                size_t size;
                if(readFile(r->path,&buffer,&size)==-1){
                    if(config.stampaStOut){
                        fprintf(stderr,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Negativo\nMotivazione: ", r->path);
                        perror("API--readFile");
                        fprintf(stderr,"\n");
                    }
                }
                else{
                    if(r->pathToSave==NULL){
                        //non devo salvare su disco i file letti
                        if(config.stampaStOut)
                            fprintf(stdout,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nDimensione: %ld\nEsito: Positivo\nFile non salvato su disco\n\n", r->path,size);
                    }
                    else{
                        //devo salvare su disco i file letti
                        if(scriviContenutoInDirectory(buffer,size,r->path,r->pathToSave)==-1){
                            if(config.stampaStOut){
                                fprintf(stdout,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nDimensione: %ld\nEsito: Positivo\nFile non salvato su disco\nMotivazione: ", r->path,size);
                                perror("scriviContenutoInDirectory");
                            }
                        }  
                        else{
                            if(config.stampaStOut)
                                fprintf(stdout,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nDimensione: %ld\nEsito: Positivo\nFile salvato su disco nella directory: %s\n\n", r->path, size, r->pathToSave);
                        }
                    }
                    //se il file non era vuoto
                    if(buffer!=NULL)
                        free(buffer);
                }
                //devo comunque chiudere il file anche in caso di errore nella lettura
                if(closeFile(r->path)==-1){
                    if(config.stampaStOut){
                        perror("API--closeFile");
                        fprintf(stderr,"\n");
                    }
                    break;
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
                            fprintf(stdout,"Tipo Operazione: Lettura N file \nEsito: Positivo\nFile letti: %d\nFile salvati nella directory:%s\n\n", nread,r->pathToSave);
                    }
                    else{
                        if(config.stampaStOut)
                            fprintf(stdout,"Tipo Operazione: Lettura N file \nEsito: Positivo\nFile letti: %d\nFile non salvati su disco\n", nread);
                    }
                }
                break;
            }
            default: //WhereToSave e //WhereToBackup non sono richieste da fare al server
                break;
        }
        //aspetto msec prima di fare una nuova richiesta
        nanosleep(&time,NULL);
        //libero la struttura richiesta
        freeRichiesta(p->val);
        //libero nodo
        free(p);
        //prelevo nuova richiesta
        p=prelevaDaCoda(richieste);
    }
    return;
}
