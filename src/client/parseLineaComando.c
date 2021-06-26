#include<util.h>
#include<coda.h>
#include<client.h>
int ottieniRichieste(t_coda* richieste, int argc, char** argv){
    char* tmpstr;
    char* token;
    long millisec;
    int opt;
    if(argc<1){
        printf("Usage: ./client arg");
        return -1;
    }
    while ((opt = getopt(argc,argv, ":hf:W:r:R:d:t:c:p")) != -1) {
        switch(opt) {
            case 'h':
                //printfUsage();
                break;
            case 'f':
            case 'W':
            case 'r':
                token=strtok_r(optarg,",",&tmpstr);
                while(token!=NULL){ //aggiungi tante richieste di lettura quanti sono gli argomenti passati a -r
                    nodo* p = malloc(sizeof(nodo));
                    p->val=(void*)malloc(sizeof(richiesta));
                    ((richiesta*)p->val)->op=ReadFile;
                    strncpy(((richiesta*)p->val)->path,token,strlen(token));
                    p->next=NULL;
                    aggiungiInCoda(richieste,p);
                    token = strtok_r(NULL, " ", &tmpstr);
                }
                break;
            case 'R':
                break;
            case 'd':
                break;
            case 't':
                
                if(isNumber(optarg,&millisec)!=0){
                    printf("L'opzione -t va usata seguita da un intero che specifica dopo quanti millisecondi inviare richiesta!\n");
                }
                break;
            case 'c':
            case 'p':
                config.stampaStOut=1;
                break;
            case '?': {
                printf("%c : Invalid option character\n", optopt);
            } break;
            case ':' :
                printf("Option %c requires parameter\n", optopt);
                break;
        }
    }
    return 0;
}
