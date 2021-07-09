#if !defined(CLIENT_H)
#define CLIENT_H

#include<util.h>
#include<api.h>
#include<conn.h>
#include<coda.h>

typedef enum{
    WriteFile,
    ReadFile,
    WhereToSave,
    WhereToBackup,
    ReadNFiles,
    DeleteFile
}tipoOperazione;

typedef struct{
    tipoOperazione op;
    char* path;
    int n;
    char* pathToSave;
}richiesta;

typedef struct{
    int stampaStOut; //opzione che mi dice se stampare sullo standard output le varie richieste
    long millisec; //opzione (settata con -t) che mi dice il tempo che deve intercorrere tra l'invio di due richieste 
    char* socket; //indirizzo socket a cui connettersi
}config_client;

int ottieniRichieste(config_client* config, t_coda* richieste, int argc, char** argv);
int validazioneRichieste(t_coda* richieste);
int eseguiRichieste(t_coda* richieste, config_client config);
void freeRichiesta(void* val);
#endif
