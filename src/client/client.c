#include<client.h>

t_coda* richieste; //coda che mantiene tutte le richieste del client

static void stampaRichieste(t_coda* richieste);
static void clean_up ();

int main(int argc,char** argv){
    config_client config;
    //valori di default configurazione client
    config.socket=NULL;
    config.millisec=0;
    config.stampaStOut=0;
    config.socket=SOCKNAME;
    if(atexit(clean_up)!=0){
        perror("atexit"); //proseguo lo stesso
    }
    richieste=inizializzaCoda(NULL);
    int r;
    if((r=ottieniRichieste(&config,richieste,argc,argv))==-1){
        fprintf(stderr,"Richieste non valide!\n");
        return -1;
    }
    if(r==1){ //ho stampato helpUsage()-> esco
        return 0;
    }
    
    if(validazioneRichieste(richieste)==-1){
        fprintf(stderr,"Richieste mal formulate!\n");
        return -1;
    }
    //APRI CONNESSIONE
    struct timespec t={10, 0}; //riprova a collegarsi al server dopo 10 secondi
    if(openConnection(config.socket,1000, t)==-1){
        perror("openConnection");
        return -1;
    }
    //ESEGUI RICHIESTE
    eseguiRichieste(richieste,config);   //in base alle richieste fa varie chiamate all'API implementata
    //stampaRichieste(richieste);
    // //CHIUDI CONNESSIONE
    if(closeConnection(config.socket)==-1){
        perror("closeConnection");
        return -1;
    }
    return 0;
}
static void clean_up (){
    if(richieste == NULL)
        return;
    if(distruggiCoda(richieste,freeRichiesta)!=0){
        fprintf(stderr,"Errore nella liberazione coda di richieste!\n");
    }
}
    
void stampaRichieste(t_coda* richieste){
    nodo* p=prelevaDaCoda(richieste);
    while(p!=NULL){
        printf("%d, %s, %s\n", ((richiesta*)p->val)->op, ((richiesta*)p->val)->path, ((richiesta*)p->val)->pathToSave);
        p=prelevaDaCoda(richieste);
    }
}
void freeRichiesta(void* val){
    richiesta* r= (richiesta*)val;
    if(r==NULL){
        return;
    }
    if(r->path!=NULL){
        free(r->path);
    }
    if(r->pathToSave!=NULL){
        free(r->pathToSave);
    }
    free(r);
}
