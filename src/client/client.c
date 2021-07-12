#include<client.h>
//*****VARIABILI GLOBALI*****//
t_coda* richieste; //coda che mantiene tutte le richieste del client

//*****FUNZIONI PRIVATE DI IMPLEMENTAZIONE*****//
/*
    @funzione clean_up
    @descrizione: effettua la pulizia della coda delle richieste quando il programma termina. Viene installata con la funzione atexit 
*/
static void clean_up ();

int main(int argc,char** argv){
    config_client config;
    //valori di default configurazione client
    config.socket=NULL;
    config.millisec=0;
    config.stampaStOut=0;
    config.socket=SOCKNAME;
    //installo funzione di pulizia per gli elementi allocati sullo heap
    if(atexit(clean_up)!=0){
        perror("atexit"); //proseguo lo stesso
    }
    richieste=inizializzaCoda(NULL);
    if(richieste==NULL){
        fprintf(stderr, "Errore nell'inizializzazione della coda delle richieste");
        exit(EXIT_FAILURE);
    }
    //effettuo il parsing della linea di comando
    int r;
    if((r=ottieniRichieste(&config,richieste,argc,argv))==-1){
        fprintf(stderr,"Richieste non valide!\n");
        exit(EXIT_FAILURE);
    }
    if(r==1){ 
        //ho stampato helpUsage()-> esco
        exit(EXIT_SUCCESS);    
    }
    //controllo se la coda delle richieste soddisfa la specifica 
    if(validazioneRichieste(richieste)==-1){
        fprintf(stderr,"Richieste mal formulate!\n");
        exit(EXIT_FAILURE);
    }
    //APRI CONNESSIONE
    struct timespec t={10, 0}; //riprova a collegarsi al server dopo 10 secondi
    if(openConnection(config.socket,1000, t)==-1){
        perror("openConnection");
        exit(EXIT_FAILURE);
    }

    //ESEGUI RICHIESTE
    //in base alle richieste farò varie chiamate all'API implementata
    eseguiRichieste(richieste,config);   

    // //CHIUDI CONNESSIONE
    if(closeConnection(config.socket)==-1){
        perror("closeConnection");
        exit(EXIT_FAILURE);
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
