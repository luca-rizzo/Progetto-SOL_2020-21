#include<util.h>
#include<coda.h>
#include<client.h>
#include<api.h>
int eseguiRichieste(t_coda* richieste, config_client config){
    void* buffer;
    size_t size;
    nodo* p;
    p=prelevaDaCoda(richieste);
    while(p!=NULL){
        richiesta* r=(richiesta*)p->val;
        switch(r->op){
            case WriteFile:
                if(openFile(r->path,O_CREAT)==-1){
                    perror("openFile");
                    IFSTOUT(fprintf(stdout,"Tipo Operazione: Scrittura file \nFile di riferimento: %s\nEsito: Negativo\n\n\n", r->path))
                    break;
                }
                if(writeFile(r->path,NULL)==-1){
                    perror("writeFile");
                    IFSTOUT(fprintf(stdout,"Tipo Operazione: Scrittura file \nFile di riferimento: %s\nEsito: Negativo\n\n\n", r->path))
                }
                if(closeFile(r->path)==-1){
                    perror("closeFile");
                    break;
                }
                IFSTOUT(fprintf(stdout,"Tipo Operazione: Scrittura file \nFile di riferimento: %s\nEsito: Positivo\n\n\n", r->path))
                break;
            case ReadFile:
                if(openFile(r->path,-1)==-1){
                    perror("openFile");
                    IFSTOUT(printf("Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Negativo\n\n\n", r->path))
                    break;
                }
                if(readFile(r->path,&buffer,&size)==-1){
                    perror("readFile");
                    IFSTOUT(fprintf(stdout,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Negativo\n\n\n", r->path))
                }
                if(closeFile(r->path)==-1){
                    perror("CloseFile");
                    break;
                }
                IFSTOUT(fprintf(stdout,"Tipo Operazione: Lettura file \nFile di riferimento: %s\nEsito: Positivo\nBytes letti: %ld\n Contenuto:\n%s\n\n", r->path,size,(char*)buffer))
                break;
            case DeleteFile:
                if(openFile(r->path,-1)==-1){
                    perror("openFile");
                    IFSTOUT(fprintf(stdout,"Tipo Operazione: Rimozione file \nFile di riferimento: %s\nEsito: Negativo\n\n\n", r->path))
                    break;
                }
                if(removeFile(r->path)==-1){
                    perror("writeFile");
                    IFSTOUT(fprintf(stdout,"Tipo Operazione: Rimozione file \nFile di riferimento: %s\nEsito: Negativo\n\n\n", r->path))
                }
                IFSTOUT(fprintf(stdout,"Tipo Operazione: Rimozione file \nFile di riferimento: %s\nEsito: Positivo\n\n\n", r->path))
                break;
            case ReadNFile: 
                break;
            default:
                break;
        }
        sleep(config.millisec);
        p=prelevaDaCoda(richieste);
    }
    return 0;
}