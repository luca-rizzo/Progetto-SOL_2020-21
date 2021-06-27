#include<util.h>
#include<coda.h>
#include<client.h>
//ritorna -1 in caso di errore; 1 se ho digitato flag -h; 0 se lo scan delle richieste è andato a buon fine
int isDirectory(char* file);
void printfUsage();
int scanDirectory(char* directory, t_coda* richieste,int n);

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
int ottieniRichieste(config_client* config, t_coda* richieste, int argc, char** argv){
    char* tmpstr;
    char* token;
    int opt;
    int n;
    nodo* p;
    if(argc<1){
        printf("Usage: ./client arg");
        return -1;
    }
    while ((opt = getopt(argc,argv, ":hf:W:w:r:R:d:t:c:p")) != -1) {
        switch(opt) {
            case 'h':
                printfUsage();
                return 1;
                break;
            case 'f':
                NULLSYSCALL(config->socket,(void*)malloc(MAX_PATH_LENGTH),"malloc");
                strncpy(config->socket,optarg,MAX_PATH_LENGTH);
                break;
            case 'W':
                tmpstr=NULL;
                token=strtok_r(optarg,",",&tmpstr);
                while(token!=NULL){ //aggiungi tante richieste di scrittura quanti sono gli argomenti passati a -W
                    NULLSYSCALL(p,malloc(sizeof(nodo)),"malloc");
                    NULLSYSCALL(p->val,(void*)malloc(sizeof(richiesta)),"malloc");
                    ((richiesta*)p->val)->op = WriteFile; 
                    ((richiesta*)p->val)->path=ottieniPathAssoluto(token);
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
                    ((richiesta*)p->val)->op=ReadFile; 
                    NULLSYSCALL(((richiesta*)p->val)->path,(void*)malloc(MAX_PATH_LENGTH),"malloc");
                    strncpy(((richiesta*)p->val)->path,token,MAX_PATH_LENGTH);
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
                ((richiesta*)p->val)->op=ReadNFile; //lo leggo
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
                NULLSYSCALL(((richiesta*)p->val)->pathToSave,(void*)malloc(MAX_PATH_LENGTH),"malloc");
                strncpy(((richiesta*)p->val)->pathToSave,optarg,MAX_PATH_LENGTH);
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
                    NULLSYSCALL(((richiesta*)p->val)->path,(void*)malloc(MAX_PATH_LENGTH),"malloc");
                    strncpy(((richiesta*)p->val)->path,token,MAX_PATH_LENGTH);
                    ((richiesta*)p->val)->n=0;
                    ((richiesta*)p->val)->pathToSave=NULL;
                    p->next=NULL;
                    aggiungiInCoda(richieste,p); //aggiungo richiesta di cancellazione file alla coda
                    token = strtok_r(NULL, ",", &tmpstr);
                }
                break;
            case 'p':
                config->stampaStOut=1;
                break;
            case '?': 
                printf("%c : Opzione non suportata\n", optopt);
                return -1;
            
            case ':' :
                if(optopt=='R'){ // R non ha parametro -> devo leggere tutti i file del server
                    NULLSYSCALL(p,malloc(sizeof(nodo)),"malloc");
                    NULLSYSCALL(p->val,(void*)malloc(sizeof(richiesta)),"malloc");
                    ((richiesta*)p->val)->op=ReadNFile; //lo leggo
                    ((richiesta*)p->val)->path=NULL;
                    ((richiesta*)p->val)->n=-1;
                    ((richiesta*)p->val)->pathToSave=NULL;
                    p->next=NULL;
                    aggiungiInCoda(richieste,p);
                }
                else{
                    printf("Opzione %c richiede parametri\n", optopt);
                    return -1;
                }
                break;
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
            if(tmp->previous!=NULL && (((richiesta*)tmp->previous->val)->op==ReadFile || ((richiesta*)tmp->previous->val)->op==ReadNFile)){
                nodo* tmp1=tmp->previous;
                do{// setto il parametro dove salvare il file letto in tutte le richieste di lettura date da una -r
                    NULLSYSCALL(((richiesta*)tmp1->val)->pathToSave,(void*)malloc(MAX_PATH_LENGTH),"malloc");
                    strncpy(((richiesta*)tmp1->val)->pathToSave, ((richiesta*)tmp->val)->pathToSave, MAX_PATH_LENGTH);
                    tmp1=tmp1->previous;
                }while(tmp1!=NULL && (((richiesta*)tmp1->val)->op==ReadFile || ((richiesta*)tmp1->val)->op==ReadNFile));
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
                ((richiesta*)p->val)->path=ottieniPathAssoluto(file->d_name);
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
