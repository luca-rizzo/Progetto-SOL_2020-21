#include<util.h>
#include<log.h>
//*****FUNZIONI PRIVATE*****//
/*
    @funzione now
    @descrizione: fornisce una versione "leggibile" del tempo corrente
    @param g conterra il giorno corrente
    @param m conterra il mese corrente
    @param a conterra l'anno corrente
    @param h conterra l'ora corrente
    @param min conterra i minuti correnti
    @param s conterra i secondi correnti
*/
static void now (int *g, int *m, int *a, int *h, int *min, int *s);

//*****FUNZIONI DI IMPLEMENTAZIONE INTERFACCIA*****//
t_log* inizializzaFileLog(const char* dirToSave){
    t_log* log=malloc(sizeof(t_log));
    if(!log)
        return NULL;
    //ricostruisco path file di log
    int dimPath=strlen(dirToSave)+strlen("log.txt")+2;
    char* path=malloc(dimPath);
    if(!path){
        return NULL;
    }
    snprintf(path, dimPath, "%s/%s", dirToSave, "log.txt");
    //apro il file di log in scrittura
    if((log->lfp=fopen(path,"w"))==NULL){
        free(log);
        free(path);
        return NULL;
    }
    free(path);
    if(pthread_mutex_init(&(log->mtx), NULL)!=0){
        free(log);
        free(path);
        fclose(log->lfp);
        return NULL;
    }
    fprintf(log->lfp,"//////FILE DI LOG///////\n\nRegistro tutte le operazioni completate correttamente\n\n");
    fflush(log->lfp);
    //ritorno la struttura allocata
    return log;
}
int scriviSuFileLog(t_log* log,char* buffer,...){
    if(!log){
        return -1;
    }
    //ottengo tempo corrente
    int secondi,minuti,ore,giorno,mese,anno;
    now(&giorno,&mese,&anno,&ore,&minuti,&secondi);
    //stampo sul file di log tempo corrente
    fprintf(log->lfp,"%02d/%02d/%4d  %02d:%02d:%02d\n", giorno, mese, anno, ore, minuti, secondi);
    LOCK_RETURN(&(log->mtx),-1);
    va_list argp;
    va_start(argp, buffer);
    //stampo argomenti sul file di log
    vfprintf(log->lfp,buffer,argp);
    fflush(log->lfp);
    va_end(argp);
    UNLOCK_RETURN(&(log->mtx),-1);
    return 0;
}

int distruggiStrutturaLog(t_log* logStr){
    if(!logStr)
        return -1;
    fprintf(logStr->lfp,"FINE FILE DI LOG\n\n");
    pthread_mutex_destroy(&(logStr->mtx));
    if(fclose(logStr->lfp)!=0){
        return -1;
    }
    free(logStr);
    return 0;
}

static void now (int *g, int *m, int *a, int *h, int *min, int *s) {   // Determinazione della data e dell’orario correnti   
    time_t data;   
    struct tm * leggibile = NULL; 
    time (&data);   
    leggibile = localtime (&data);   // Riempimento delle variabili   
    // Giorno del mese   
    *g = leggibile->tm_mday;   
    // Mese   
    *m = leggibile->tm_mon +1;   
    // Anno   
    *a = leggibile->tm_year+1900;
    // Ora   
    *h = leggibile->tm_hour;   
    // Minuti   
    *min = leggibile->tm_min;   
    // Secondi   
    *s = leggibile->tm_sec;      
    return; 
}
