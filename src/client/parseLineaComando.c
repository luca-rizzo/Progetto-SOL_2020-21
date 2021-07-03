#include<util.h>
#include<coda.h>
#include<client.h>
//ritorna -1 in caso di errore; 1 se ho digitato flag -h; 0 se lo scan delle richieste è andato a buon fine
int isDirectory(char* file);
void printfUsage();
int scanDirectory(char* directory, t_coda* richieste,int n);
//modifica un path locale in un path assoluto
//ritorna un puntatore a path assoluto in caso di successo; NULL altrimenti
char* ottieniPathAssoluto(char* pathRelativo){
    char* pathCorrente;
    if((pathCorrente=malloc(MAX_PATH_LENGTH))==NULL){
        perror("malloc");
        return NULL;
    }
    if(getcwd(pathCorrente,MAX_PATH_LENGTH)==NULL){
        perror("getcwd");
        return NULL;
    }
    int len=MAX_PATH_LENGTH-strlen(pathCorrente);
    if(len<2)
        return NULL; //ci entra nella nostra stringa?
    strncat(pathCorrente,"/", len);
    len=MAX_PATH_LENGTH-strlen(pathCorrente);
    if(len<strlen(pathRelativo)+1){//ci entra nella nostra stringa?
        return NULL;
    }
    strncat(pathCorrente,pathRelativo, len);
    return pathCorrente;
}
//parsa tutta la linea di comando per ottenere richietse del client;
//ritorna 0 in caso di successo, -1 altrimenti
int ottieniRichieste(config_client* config, t_coda* richieste, int argc, char** argv){
    char* tmpstr;
    char* token;
    int opt,prev_ind;
    int n;
    int flag_f=0,flag_p=0;
    nodo* p;
    if(argc<1){
        printf("Usage: ./client arg");
        return -1;
    }
    while(prev_ind = optind, (opt =  getopt(argc, argv, ":f:W:w:r:d:R:t:c:ph")) != EOF){
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
                NULLSYSCALL(config->socket,(void*)malloc(MAX_PATH_LENGTH),"malloc");
                strncpy(config->socket,optarg,MAX_PATH_LENGTH);
                flag_f=1;
                break;
            case 'W':
                tmpstr=NULL;
                token=strtok_r(optarg,",",&tmpstr);
                while(token!=NULL){ //aggiungi tante richieste di scrittura quanti sono gli argomenti passati a -W
                    NULLSYSCALL(p,malloc(sizeof(nodo)),"malloc");
                    NULLSYSCALL(p->val,(void*)malloc(sizeof(richiesta)),"malloc");
                    ((richiesta*)p->val)->op = WriteFile; 
                    ((richiesta*)p->val)->path=realpath(token,NULL);
                    if(((richiesta*)p->val)->path==NULL){
                        printf("Errore path nell'operazione -W");
                        return -1;
                    }
                    ((richiesta*)p->val)->n=0;
                    ((richiesta*)p->val)->pathToSave=NULL;
                    p->next=NULL;
                    aggiungiInCoda(richieste,p);
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
                    NULLSYSCALL(p,malloc(sizeof(nodo)),"malloc");
                    NULLSYSCALL(p->val,(void*)malloc(sizeof(richiesta)),"malloc");
                    int len = strlen(token)+1;
                    ((richiesta*)p->val)->path = malloc(len);
                    if(((richiesta*)p->val)->path==NULL){
                        perror("malloc");
                        return -1;
                    }
                    strncpy(((richiesta*)p->val)->path,token,len);
                    ((richiesta*)p->val)->op=ReadFile; 
                    ((richiesta*)p->val)->n=0;
                    ((richiesta*)p->val)->pathToSave=NULL;
                    p->next=NULL;
                    aggiungiInCoda(richieste,p);
                    token = strtok_r(NULL, ",", &tmpstr);
                }
                break;
            case 'R': //a R è stato passato un parametro n
                NULLSYSCALL(p,malloc(sizeof(nodo)),"malloc");
                NULLSYSCALL(p->val,(void*)malloc(sizeof(richiesta)),"malloc");
                ((richiesta*)p->val)->op=ReadNFiles; //operazione richiesta
                ((richiesta*)p->val)->path=NULL;
                if(isNumber(optarg,(long*)&(((richiesta*)p->val)->n))!=0){
                   printf("L'opzione -R va usata seguita da un intero che specifica quanti file leggere\n");
                   return -1;
                }
                ((richiesta*)p->val)->pathToSave=NULL;
                p->next=NULL;
                aggiungiInCoda(richieste,p);
                break;
            case 'd':
                NULLSYSCALL(p,malloc(sizeof(nodo)),"malloc");
                NULLSYSCALL(p->val,(void*)malloc(sizeof(richiesta)),"malloc");
                ((richiesta*)p->val)->op = WhereToSave; 
                ((richiesta*)p->val)->path = NULL;
                ((richiesta*)p->val)->n=0;
                ((richiesta*)p->val)->pathToSave = realpath(optarg,NULL);
                p->next=NULL;
                aggiungiInCoda(richieste,p); //aggiungo richiesta di salvare i file letti in una particolare directory
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
                    NULLSYSCALL(p,malloc(sizeof(nodo)),"malloc");
                    NULLSYSCALL(p->val,(void*)malloc(sizeof(richiesta)),"malloc");
                    ((richiesta*)p->val)->op=DeleteFile; 
                    int len = strlen(token)+1;
                    ((richiesta*)p->val)->path = malloc(len);
                    if(((richiesta*)p->val)->path==NULL){
                        perror("malloc");
                        return -1;
                    }
                    strncpy(((richiesta*)p->val)->path,token,len);
                    ((richiesta*)p->val)->n=0;
                    ((richiesta*)p->val)->pathToSave=NULL;
                    p->next=NULL;
                    aggiungiInCoda(richieste,p); //aggiungo richiesta di cancellazione file alla coda
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
            case '?': 
                printf("%c : Opzione non suportata\n", optopt);
                return -1;
            case ':' :
                if(optopt=='R'){
                    NULLSYSCALL(p,malloc(sizeof(nodo)),"malloc");
                    NULLSYSCALL(p->val,(void*)malloc(sizeof(richiesta)),"malloc");
                    ((richiesta*)p->val)->op=ReadNFiles; //operazione richiesta
                    ((richiesta*)p->val)->path=NULL;
                    ((richiesta*)p->val)->n=-1;
                    ((richiesta*)p->val)->pathToSave=NULL;
                    p->next=NULL;
                    aggiungiInCoda(richieste,p);
                    break;
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
        if(((richiesta*)tmp->val)->op==WhereToSave){ //la richiesta di prima deve essere stata una -r o una -R
            if(tmp->previous!=NULL && (((richiesta*)tmp->previous->val)->op==ReadFile || ((richiesta*)tmp->previous->val)->op==ReadNFiles)){
                nodo* tmp1=tmp->previous;
                do{// setto il parametro dove salvare il file letto in tutte le richieste di lettura date da una -r
                    NULLSYSCALL(((richiesta*)tmp1->val)->pathToSave,(void*)malloc(MAX_PATH_LENGTH),"malloc");
                    strncpy(((richiesta*)tmp1->val)->pathToSave, ((richiesta*)tmp->val)->pathToSave, MAX_PATH_LENGTH);
                    tmp1=tmp1->previous;
                }while(tmp1!=NULL && (((richiesta*)tmp1->val)->op==ReadFile || ((richiesta*)tmp1->val)->op==ReadNFiles));
                tmp->previous->next=tmp->next; //rimuovo il nodo -d perchè non è una richiesta da fare al server
                if(tmp->next!=NULL)
                    tmp->next->previous=tmp->previous;
                free(((richiesta*)tmp->val)->pathToSave);
                free(((richiesta*)tmp->val));
                free(tmp);
                break;
            }
            else{
                printf("L'opzione -d va usata subito dopo l'opzione -r o -R per specificare dove salvare i file richiesti\n");
                return -1;
            }
        }
        else{
            tmp=tmp->next;
        }
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
    char buf[MAX_PATH_LENGTH];
    nodo* p;
     if(getcwd(buf, MAX_PATH_LENGTH)==NULL){
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
                NULLSYSCALL(p,malloc(sizeof(nodo)),"malloc");
                NULLSYSCALL(p->val,(void*)malloc(sizeof(richiesta)),"malloc");
                ((richiesta*)p->val)->op = WriteFile; 
                ((richiesta*)p->val)->path=realpath(file->d_name,NULL);
                if(((richiesta*)p->val)->path==NULL){
                    printf("Errore path nell'operazione -W\n");
                    return -1;
                }
                ((richiesta*)p->val)->n=0;
                ((richiesta*)p->val)->pathToSave=NULL;
                p->next=NULL;
                aggiungiInCoda(richieste,p);
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
