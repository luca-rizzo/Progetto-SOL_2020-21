#include<client.h>

int isDirectory(char* file);
void printfUsage();
int scanDirectory(char* directory, t_coda* richieste,int n);
char* myrealpath(char* path);

//parsa tutta la linea di comando per ottenere richieste del client;
//ritorna -1 in caso di errore; 1 se ho digitato flag -h; 0 se lo scan delle richieste è andato a buon fine
int ottieniRichieste(config_client* config, t_coda* richieste, int argc, char** argv){
    char* tmpstr;
    char* token;
    int opt,prev_ind;
    int n;
    int flag_f=0,flag_p=0;
    richiesta* req;
    if(argc<1){
        printf("Usage: ./client arg");
        return -1;
    }
    while(prev_ind = optind, (opt =  getopt(argc, argv, ":D:f:W:w:r:d:R:t:c:ph")) != EOF){
        if ( optind == prev_ind + 2 && *optarg == '-' ) { //il comando richiedeva un parametro perchè optarg!=NULL
            optopt=opt;
            opt = ':';
            -- optind;
        }
        switch(opt) {
            case 'h':
                printfUsage();
                return 1;
                break;
            case 'f':
                if(flag_f) {
                    printf("Il flag -f può essere usato una sola volta\n");
                    return -1;
                }
                NULLSYSCALL(config->socket,(void*)malloc(PATH_MAX),"malloc");
                strncpy(config->socket,optarg,PATH_MAX);
                flag_f=1;
                break;
            case 'W':
                tmpstr=NULL;
                token=strtok_r(optarg,",",&tmpstr);
                while(token!=NULL){ //aggiungi tante richieste di scrittura quanti sono gli argomenti passati a -W
                    NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                    req->op = WriteFile; 
                    if((req->path=realpath(token,NULL))==NULL){
                        perror("Errore nel convertire path locale in globale opzione -W");
                        return -1;
                    }
                    req->n=0;
                    req->pathToSave=NULL;
                    if(aggiungiInCoda(richieste,(void*) req)==-1){
                        fprintf(stderr,"Errore aggiunta richiesta di scrittura file %s\n",req->path);
                    }
                    token = strtok_r(NULL, ",", &tmpstr);
                }
                break;
            case 'w':
                tmpstr=NULL;
                token=strtok_r(optarg,",",&tmpstr);
                int r=isDirectory(token);
                if(r==0){
                    printf("L'argomento di -w DEVE essere una directory\n");
                    return -1;
                }
                if(r==-1){
                    printf("Errore nello scan della directory\n");
                    return -1;
                }
                char *token1 = strtok_r(NULL,"\n",&tmpstr);
                if(token1!=NULL){
                    if(isNumber(token1,(long*)&n)!=0){
                        printf("L'opzione -w deve essere seguita da i seguenti parametri: dirname,[n]\n");
                        return -1;
                    }
                }
                else
                    n=INT_MAX;
                if(scanDirectory(optarg, richieste,n)==-1){
                    printf("Errore nello scan della directory\n");
                    return -1;
                }
                break;
            case 'r':
                tmpstr=NULL;
                token=strtok_r(optarg,",",&tmpstr);
                while(token!=NULL){ //aggiungi tante richieste di lettura quanti sono gli argomenti passati a -r
                    NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                    req->op = ReadFile; 
                    if((req->path=myrealpath(token))==NULL){
                        perror("Errore nel convertire path locale in globale opzione -r");
                        return -1;
                    }
                    req->n=0;
                    req->pathToSave=NULL;
                    if(aggiungiInCoda(richieste,(void*) req)==-1){
                        fprintf(stderr,"Errore aggiunta richiesta di lettura file %s\n",req->path);
                    }
                    token = strtok_r(NULL, ",", &tmpstr);
                }
                break;
            case 'R': //a R è stato passato un parametro n
                NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                req->op = ReadNFiles; 
                req->pathToSave=NULL;
                req->path=NULL;
                if(isNumber(optarg,(long*)&(req->n))!=0){
                   printf("L'opzione -R va usata seguita da un intero che specifica quanti file leggere\n");
                   return -1;
                }
                if(aggiungiInCoda(richieste,(void*) req)==-1){
                    fprintf(stderr,"Errore aggiunta richiesta di lettura N file \n");
                }
                break;
            case 'd':
                NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                req->op = WhereToSave; 
                if((req->pathToSave=realpath(optarg,NULL))==NULL){
                    perror("Errore nel convertire path locale in globale opzione -d");
                    return -1;
                }
                req->n=0;
                req->path=NULL;
                if(aggiungiInCoda(richieste,(void*) req)==-1){
                    fprintf(stderr,"Errore aggiunta richiesta di salvataggio file letti in directory %s \n",req->pathToSave);
                }
                break;
            case 't':
                if(isNumber(optarg,&(config->millisec))!=0){
                    printf("L'opzione -t va usata seguita da un intero che specifica dopo quanti millisecondi inviare richiesta!\n");
                }
                break;
            case 'c':
                tmpstr=NULL;
                token=strtok_r(optarg,",",&tmpstr);
                while(token!=NULL){
                    NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                    req->op = DeleteFile; 
                    if((req->path=myrealpath(token))==NULL){
                        perror("Errore nel convertire path locale in globale opzione -c");
                        return -1;
                    }
                    req->n=0;
                    req->pathToSave=NULL;
                    if(aggiungiInCoda(richieste,(void*) req)==-1){
                        fprintf(stderr,"Errore aggiunta richiesta di rimozione file %s\n",req->path);
                    }
                    token = strtok_r(NULL, ",", &tmpstr);
                }
                break;
            case 'p':
                if(flag_p){
                    printf("Il flag -p può essere usato una sola volta\n");
                    return -1;
                }
                config->stampaStOut=1;
                flag_p=1;
                break;
            case 'D':
                NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                req->op = WhereToBackup; 
                if((req->pathToSave=realpath(optarg,NULL))==NULL){
                    perror("Errore nel convertire path locale in globale opzione -D");
                    return -1;
                }
                req->n=0;
                req->path=NULL;
                if(aggiungiInCoda(richieste,(void*) req)==-1){
                    fprintf(stderr,"Errore aggiunta richiesta di backup file espulsi nella directory %s\n",req->pathToSave);
                }
                break;
            case '?': 
                printf("%c : Opzione non suportata\n", optopt);
                return -1;
            case ':' :
                if(optopt=='R'){
                    NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                    req->op = ReadNFiles; 
                    req->pathToSave=NULL;
                    req->path=NULL;
                    req->n=-1;
                    if(aggiungiInCoda(richieste,(void*) req)==-1){
                        fprintf(stderr,"Errore aggiunta richiesta di lettura N file \n");
                    }
                }
                else{
                    printf("Opzione %c richiede parametri\n", optopt);
                    return -1;
                }
        }
    }
    return 0;
}
//controlla la validità delle richieste fatte;
//ritorna 0 in caso di richieste ben formulate, -1 altrimenti
int validazioneRichieste(t_coda* richieste){
    nodo* tmp=richieste->head;
    while(tmp!=NULL){
        richiesta* req = (richiesta*) tmp->val;
        if(req->op==WhereToSave){
            //la richiesta di prima deve essere stata una -r o una -R
            if(tmp->previous!=NULL && (((richiesta*)(tmp->previous->val))->op==ReadFile || ((richiesta*)(tmp->previous->val))->op==ReadNFiles)){
                nodo* tmp1=tmp->previous;
                do{// setto il parametro dove salvare il file letto in tutte le richieste di lettura date da una -r o -R precedenti a opzione -d
                    NULLSYSCALL(((richiesta*)tmp1->val)->pathToSave,(void*)malloc(PATH_MAX),"malloc");
                    strncpy(((richiesta*)tmp1->val)->pathToSave, ((richiesta*)tmp->val)->pathToSave, PATH_MAX);
                    tmp1=tmp1->previous;
                }while(tmp1!=NULL && (((richiesta*)tmp1->val)->op==ReadFile || ((richiesta*)tmp1->val)->op==ReadNFiles));
                tmp=tmp->next;
            }
            else{
                printf("L'opzione -d va usata subito dopo l'opzione -r o -R per specificare dove salvare i file richiesti\n");
                return -1;
            }
        }
        else if(req->op==WhereToBackup){
            if(tmp->previous!=NULL && ((richiesta*)(tmp->previous->val))->op==WriteFile){
                nodo* tmp1=tmp->previous;
                do{// setto il parametro dove salvare il file espulso in tutte le richieste di scrittura date da -w o -W precedenti a opzione -D
                    NULLSYSCALL(((richiesta*)tmp1->val)->pathToSave,(void*)malloc(PATH_MAX),"malloc");
                    strncpy(((richiesta*)tmp1->val)->pathToSave, ((richiesta*)tmp->val)->pathToSave, PATH_MAX);
                    tmp1=tmp1->previous;
                }while(tmp1!=NULL && (((richiesta*)tmp1->val)->op==WriteFile));
                tmp=tmp->next;
            }
            else{
                printf("L'opzione -D va usata subito dopo l'opzione -w o -W per specificare dove salvare i file rimossi in seguito a capacity miss\n");
                return -1;
            }
        }
        else    
            tmp=tmp->next;
    }
    return 0;
}
void printfUsage(){
    printf("Uso\n");
}
//ritorna 1 se file è il path di una directory; -1 in caso di errore; 0 altrimenti
int isDirectory(char* file){
	struct stat info;
	if(stat(file,&info)==-1){
		perror("stat");
        return -1;
	}
	if(S_ISDIR(info.st_mode)){
		return 1;
	}
	return 0;
}
//visita ricorsivamente a directory e aggiunge n richieste di scrittura alla coda;
//ritorna 0 in caso di successo; -1 altrimenti
int scanDirectory(char* directory, t_coda* richieste, int n){
    static int i=0;
    char buf[PATH_MAX];
    richiesta* req;
     if(getcwd(buf, PATH_MAX)==NULL){
        perror("getcwd");
        return -1;
    }
     if(chdir(directory)==-1){
        perror("chdir");
        return -1;
    }
    DIR* d;
    struct dirent* file;
    if((d=opendir("."))==NULL){
        perror("opendir");
        return -1;
     }
     while((errno=0,file=readdir(d))!=NULL){
        if(strcmp(file->d_name,".")!=0 && strcmp(file->d_name,"..")!=0 && isDirectory(file->d_name)){  //trovi una directory?Entraci
            if(scanDirectory(file->d_name,richieste,n)==-1){
                return -1;
            }
        }
        else if(strcmp(file->d_name,".")!=0 && strcmp(file->d_name,"..")!=0){
            if(i<n){
                NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                req->op = WriteFile; 
                if((req->path=realpath(file->d_name,NULL))==NULL){
                    perror("Errore nel convertire path locale in globale");
                    return -1;
                }
                req->n=0;
                req->pathToSave=NULL;
                if(aggiungiInCoda(richieste,(void*) req)==-1){
                    fprintf(stderr,"Errore aggiunta richiesta di scrittura file %s\n",req->path);
                }
                i++;
            }
            else
                break;
        }
     }
    if(errno!=0){
        perror("readdir");
        return -1;
    }
    if(chdir(buf)==-1){
        perror("chdir");
        return -1;
    }
    if(closedir(d)==-1){
        perror("error closing");
        return -1;
    }
    return 0;
}
//converte un path relativo(a partire dalla WD) in assoluto anche se il file non esiste
//non funziona per path relativi che cominciano con ../
//ritorna "ipotetico" path assoluto in caso di successo, -1 in caso di fallimento
char* myrealpath(char* path){
    if(path==NULL){
        return NULL;
    }
    if(*path=='/'){ //è già un path assoluto
        char* absPath=malloc(PATH_MAX);
        if(absPath==NULL){
            return NULL;
        }
        strncpy(absPath,path,PATH_MAX);
        return absPath;
    }
    if(strlen(path)>2 && strncmp(path,"./",2)==0){ //path del tipo ./file1.txt --->salto il ./
        path=path+2;
    }
    char* workDir;
    if((workDir=malloc(PATH_MAX))==NULL){
        perror("malloc");
        return NULL;
    }
    //ottengo path working directory
    if(getcwd(workDir,PATH_MAX)==NULL){
        perror("getcwd");
        return NULL;
    }
    int lenPath= strlen(workDir)+strlen(path)+2; 
    if(lenPath>PATH_MAX){//path assoluto troppo lungo
        free(workDir);
        return NULL;
    }
    char* absPath=malloc(PATH_MAX);
    if(absPath==NULL){
        free(workDir);
        return NULL;
    }
    snprintf(absPath, PATH_MAX, "%s/%s", workDir, path);
    return absPath;
}