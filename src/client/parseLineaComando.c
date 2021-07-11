#include<client.h>
//*****FUNZIONI PRIVATE DI IMPLEMENTAZIONE*****//

/*
    @funzione isDirectory
    @descrizione dato un pathname che identifica un file, mi dice se quel file è una directory
    @param file è il pathname del file di cui chiediamo informazioni
    @return 1 se il file è un directory, 0 se il file non è una directory, -1 in caso di errore
*/
static int isDirectory(char* file);


/*
    @funzione printUsage
    @descrizione stampa la lista di tutte le opzioni accettate dal client
*/
static void printfUsage();

/*
    @funzione scanDirectory
    @descrizione: visita ricorsivamente una directory e le sue subdirectory e inserisce al più n richieste di scrittura alla coda 
    @param directory è il pathname della directory da visitare ricorsivamente 
    @param richieste è la coda in cui inserire le varie richieste di scrittura
    @param n è il numero di richieste di scrittura che voglio inserire alla coda. Se n=0 non c'è un limite al numero di richieste di scrittura
    @return 0 in caso di successo, -1 altrimenti
*/
static int scanDirectory(char* directory, t_coda* richieste,int n);


/*
    @funzione myrealpath
    @descrizione: converte un path relativo (a partire dalla WD) in assoluto anche se il file non esiste. Non funziona per path relativi che cominciano con ../
    @param path è il path relativo del file
    @return "ipotetico" path assoluto in caso di successo, NULL in caso di fallimento, setta errno
*/
static char* myrealpath(char* path);

//*****FUNZIONI DI IMPLEMENTAZIONE INTERFACCIA*****//
int ottieniRichieste(config_client* config, t_coda* richieste, int argc, char** argv){
    //variabili "di lavoro"
    char* tmpstr;
    char* token;
    int n;
    richiesta* req;
    //flag che mi notificano se ho già passato argomento -f e -p
    int flag_f=0,flag_p=0;
    if(argc<2){
        printfUsage();
        return 1;
    }
    //variabili usate per implementare una versione "migliorata" di getopt
    int opt,prev_ind;
    while(prev_ind = optind, (opt =  getopt(argc, argv, ":D:f:W:w:r:d:R:t:c:ph")) != EOF){
        //controllo se per tutti gli option character che richiedono un argomento tale argomento è presente.
        //se non è presente opt= ':'. Nell'implementazione standard di getopt tale funzionalità è fornita solo per l'ultimo option character
        if ( optind == prev_ind + 2 && *optarg == '-' ) { 
            //il comando richiedeva un parametro perchè optarg!=NULL ma io non ho passato alcun parametro
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
                    fprintf(stderr, "Il flag -f può essere usato una sola volta\n");
                    return -1;
                }
                NULLSYSCALL(config->socket,(void*)malloc(PATH_MAX),"malloc");
                strncpy(config->socket,optarg,PATH_MAX);
                flag_f=1;
                break;
            case 'W':
                tmpstr=NULL;
                token=strtok_r(optarg,",",&tmpstr);
                while(token!=NULL){ 
                    //aggiungi tante richieste di scrittura quanti sono gli argomenti passati a -W
                    NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                    req->op = WriteFile; 
                    if((req->path=realpath(token,NULL))==NULL){
                        free(req);
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
                //controllo se l'argomento è una directory
                int r=isDirectory(token);
                if(r!=1){
                    fprintf(stderr,"L'argomento di -w DEVE essere una directory\n");
                    return -1;
                }
                char *token1 = strtok_r(NULL,"\n",&tmpstr);
                if(token1!=NULL){
                    //abbiamo passato anche il valore n
                    if(isNumber(token1,(long*)&n)!=0){
                        fprintf(stderr,"L'opzione -w deve essere seguita da i seguenti parametri: dirname,[n]\n");
                        return -1;
                    }
                    if(n==0)
                        n=INT_MAX;
                }
                else
                    n=INT_MAX;
                if(scanDirectory(optarg, richieste,n)==-1){
                    fprintf(stderr,"Errore nello scan della directory\n");
                    return -1;
                }
                break;
            case 'r':
                tmpstr=NULL;
                token=strtok_r(optarg,",",&tmpstr);
                while(token!=NULL){ 
                    //aggiungi tante richieste di lettura quanti sono gli argomenti passati a -r
                    NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                    req->op = ReadFile; 
                    if((req->path=myrealpath(token))==NULL){
                        free(req);
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
            case 'R': 
                //a -R è stato passato un argomento n
                NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                req->op = ReadNFiles; 
                req->pathToSave=NULL;
                req->path=NULL;
                if(isNumber(optarg,(long*)&(req->n))!=0){
                    free(req);
                    fprintf(stderr,"L'opzione -R va usata seguita da un intero che specifica quanti file leggere\n");
                    return -1;
                }
                if(aggiungiInCoda(richieste,(void*) req)==-1){
                    fprintf(stderr,"Errore aggiunta richiesta di lettura N file \n");
                }
                break;
            case 'd':
                //indico dove salvare file letti per le precedenti richieste di lettura
                NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                req->op = WhereToSave; 
                if((req->pathToSave=realpath(optarg,NULL))==NULL){
                    perror("Errore nel convertire path locale in globale opzione -d");
                    free(req);
                    return -1;
                }
                req->n=0;
                req->path=NULL;
                if(aggiungiInCoda(richieste,(void*) req)==-1){
                    fprintf(stderr,"Errore aggiunta richiesta di salvataggio file letti in directory %s \n",req->pathToSave);
                }
                break;
            case 't':
                if(isNumber(optarg,&(config->millisec))!=0 && config->millisec<0){
                    fprintf(stderr, "L'opzione -t va usata seguita da un intero positivo che specifica dopo quanti millisecondi inviare richiesta!\n");
                    return -1;
                }
                break;
            case 'c':
                tmpstr=NULL;
                token=strtok_r(optarg,",",&tmpstr);
                while(token!=NULL){
                    //aggiungi tante richieste di rimozione file quanti sono gli argomenti passati a -c
                    NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                    req->op = DeleteFile; 
                    if((req->path=myrealpath(token))==NULL){
                        perror("Errore nel convertire path locale in globale opzione -c");
                        free(req);
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
                //indico dove salvare file espulsi in seguito a capacity miss per le precedenti richieste di scrittura
                NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                req->op = WhereToBackup; 
                if((req->pathToSave=realpath(optarg,NULL))==NULL){
                    perror("Errore nel convertire path locale in globale opzione -D");
                    free(req);
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
                    //a -R è non è stato passato un argomento
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

int validazioneRichieste(t_coda* richieste){
    nodo* tmp=richieste->head;
    while(tmp!=NULL){
        richiesta* req = (richiesta*) tmp->val;
        if(req->op==WhereToSave){
            //la richiesta di prima deve essere stata una -r o una -R
            if(tmp->previous!=NULL && (((richiesta*)(tmp->previous->val))->op==ReadFile || ((richiesta*)(tmp->previous->val))->op==ReadNFiles)){
                nodo* tmp1=tmp->previous;
                do{
                    //setto il parametro dove salvare il file letto in tutte le richieste di lettura date da una -r o -R immediatamente precedenti a opzione -d
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
            //la richiesta di prima deve essere stata una -w o -W
            if(tmp->previous!=NULL && ((richiesta*)(tmp->previous->val))->op==WriteFile){
                nodo* tmp1=tmp->previous;
                do{
                    //setto il parametro dove salvare il file espulso in tutte le richieste di scrittura date da -w o -W immediatamente precedenti a opzione -D
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
    fprintf(stdout, "Uso: client opzioni...\n");
    fprintf(stdout, "Programma usato per inviare richieste di creazione/apertura/scrittura/lettura/rimozione/chiusura file al file storage server\n");
    fprintf(stdout, "Opzioni disponibili:\n\n");
    fprintf(stdout, "-h: stampa la lista di tutte le opzioni accettate dal client e termina immediatamente\n\n");
    fprintf(stdout, "-f <sockname>: specifica il nome del file socket a cui connettersi. Questa opzione va settata una sola volta\n\n");
    fprintf(stdout, "-t <time>: specifica il tempo in millisecondi che intercorre tra l'invio di due richieste consecutive al server. Se non specificata si suppome -t 0\n\n");
    fprintf(stdout, "-p: abilita le stampe sullo standard output per ogni operazione. Questa opzione va settata una sola volta\n\n");
    fprintf(stdout, "-w <dirname,[n]>: effettua al più n richieste di scrittura al server di file contenuti nella directory locale dirname.\n");
    fprintf(stdout, "Se n non è specificato non c'è un limite al numero di richieste d scrittura di file: verranno inviate tante richieste quanti sono i file presenti in dirname e nelle sue subdirectory\n\n");
    fprintf(stdout, "-W <file1,[,file]>: effettua una richiesta di scrittura per ogni file passato come argomento. I path dei file devono essere separati da ','\n\n");
    fprintf(stdout, "-D <dirname>: specifica in quale directory locale salvare i file che vengono espulsi dal server in seguito a capacity miss per servire le scritture richieste tramite opzione -w e -W.\n");
    fprintf(stdout, "Va usata congiuntamente alle opzioni -w e -W e si riferisce a tutte le opzioni di scrittura che la precedono immediatamente.\n");
    fprintf(stdout, "Se tale opzione non viene specificata, in seguito ad una richiesta di scrittura i file espulsi verranno cancellati e non salvati su disco\n\n");
    fprintf(stdout, "-r <file1,[,file]>: effettua una richiesta di lettura per ogni file passato come argomento. I path dei file devono essere separati da ','\n\n");
    fprintf(stdout, "-R <[n]>: effettua una richiesta di lettura di n file casuali salvati nel file server. Se n=0 o n non è specificato allora vengono letti tutti i file presenti nel file server storage\n\n");
    fprintf(stdout, "-d <dirname>: specifica in quale directory locale salvare i file letti dal server con l'opzione -r o -R.\n");
    fprintf(stdout, "Va usata congiuntamente alle opzioni -r e -R e si riferisce a tutte le opzioni di lettura che la precedono immediatamente.\n");
    fprintf(stdout, "Se tale opzione non viene specificata, in seguito ad una richiesta di lettura i file letti verranno cancellati e non salvati su disco\n\n");
    fprintf(stdout, "-c <file1,[,file]>: effettua una richiesta di rimozione per ogni file passato come argomento. I path dei file devono essere separati da ','\n\n");
}

static int isDirectory(char* file){
	struct stat info;
	if(stat(file,&info)==-1){
		perror("stat");
        return -1;
	}
    //controllo se il file è una directory
	if(S_ISDIR(info.st_mode)){
		return 1;
	}
	return 0;
}

static int scanDirectory(char* directory, t_coda* richieste, int n){
    static int i=0;
    char buf[PATH_MAX];
    richiesta* req;
    //ottengo path WD
    if(getcwd(buf, PATH_MAX)==NULL){
        perror("getcwd");
        return -1;
    }
    //cambio WD del processo
    if(chdir(directory)==-1){
        perror("chdir");
        return -1;
    }
    DIR* d;
    struct dirent* file;
    //apro directory 
    if((d=opendir("."))==NULL){
        perror("opendir");
        return -1;
     }
     while((errno=0,file=readdir(d))!=NULL){
        //esamino tutti i file contenuti nella directory
        if(strcmp(file->d_name,".")!=0 && strcmp(file->d_name,"..")!=0 && isDirectory(file->d_name)){  
            //Hai trovato una directory (che non sia . o ..)? Entraci
            if(scanDirectory(file->d_name,richieste,n)==-1){
                return -1;
            }
        }
        else if(strcmp(file->d_name,".")!=0 && strcmp(file->d_name,"..")!=0){
            //non è una directory: aggiungi richiesta di scrittura
            if(i<n){
                NULLSYSCALL(req,(richiesta*)malloc(sizeof(richiesta)),"malloc");
                req->op = WriteFile; 
                if((req->path=realpath(file->d_name,NULL))==NULL){
                    perror("Errore nel convertire path locale in globale");
                    free(req);
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
                //hai raggiunto il numero massimo di richieste di scrittura
                break;
        }
     }
    if(errno!=0){
        perror("readdir");
        return -1;
    }
    //mi risposto nella WD originale
    if(chdir(buf)==-1){
        perror("chdir");
        return -1;
    }
    //chiudo la directory
    if(closedir(d)==-1){
        perror("error closing");
        return -1;
    }
    return 0;
}

static char* myrealpath(char* path){
    if(path==NULL){
        errno = EINVAL;
        return NULL;
    }
    if(*path=='/'){ 
        //è già un path assoluto
        char* absPath=malloc(PATH_MAX);
        if(absPath==NULL){
            return NULL;
        }
        strncpy(absPath,path,PATH_MAX);
        return absPath;
    }
    if(strlen(path)>2 && strncmp(path,"./",2)==0){ 
        //path del tipo ./file1.txt --->salto il ./
        path=path+2;
    }
    char* workDir;
    if((workDir=malloc(PATH_MAX))==NULL){
        return NULL;
    }
    //ottengo path working directory
    if(getcwd(workDir,PATH_MAX)==NULL){
        return NULL;
    }
    int lenPath= strlen(workDir)+strlen(path)+2; 
    if(lenPath>PATH_MAX){
        //path assoluto troppo lungo
        free(workDir);
        return NULL;
    }
    char* absPath=malloc(PATH_MAX);
    if(absPath==NULL){
        free(workDir);
        return NULL;
    }
    //ottengo path assoluto concatenando path working directory con path relativo passato come argomento
    snprintf(absPath, PATH_MAX, "%s/%s", workDir, path);
    free(workDir);
    return absPath;
}