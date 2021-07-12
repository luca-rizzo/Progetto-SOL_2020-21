#ifndef API_H
#define API_H

/* 
    @funzione openConnection
    @descrizione: permette di aprire una connessione AF_UNIX al socket file sockname
    @param sockname è il nome del socket al quale connettersi
    @param msec è il tempo in millisecondi che intercorre tra due richieste consecutive di un client al server
    @param abstime è il tempo assoluto che indica per quanto tempo ritentare la connessione con il server
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
    @errori possibili:  errno = EINVAL se sockname==NULL o msec<0;
                        errno = EINVAL se sockname è più lungo di UNIX_PATH_MAX;
                        errno = EISCONN se sei già connesso al socket file sockname;
                        errno = ETIMEDOUT se è scaduto il tempo assoluto abstime e il servernon ha ancora accettato la connessione
 */
int openConnection(const char* sockname, int msec, const struct timespec abstime);


/*
    @funzione closeConnection
    @descrizione: permette di chiudere la connessione AF_UNIX associata al socket file socketname
    @param sockname è il nome del socket associato alla connessione
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
    @errori possibili:  errno = EINVAL se sockname==NULL
                        errno = ENOTCONN se non sono collegato al file sockname;
*/              
int closeConnection(const char* sockname);


/*
    @funzione openFile
    @descrizione: permette di aprire un file presente nel file storage server o di crearne uno (se non presente) tramite il flag O_CREAT
    @param pathname è il path assoluto del file da aprire/creare
    @param flags se è settato a O_CREAT permette di creare un file
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
    @errori possibili:  errno = EINVAL se pathname==NULL
                        errno = ENOTCONN se non sono connesso al server
                        errno = EBADMSG in caso di errori di comunicazione con il server
                        errno = EPROTO in caso di errori di protocollo interni al server
                        errno = EEXIST se provo a creare un file già esistente
                        errno = EACCESS se provo ad aprire un file che ho già aperto
                        errno = ENOENT se provo ad aprire un file che non esiste
*/
int openFile(const char* pathname, int flags);


/*
    @funzione readFile
    @descrizione: permette di leggere un file dal file storage server
    @param pathname è il path assoluto del file da leggere
    @param buf è il buffer da cui leggere il contenuto del file
    @param size è la dimensione del file in byte
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
    @errori possibili:  errno = EINVAL se pathname==NULL
                        errno = ENOTCONN se non sono connesso al server
                        errno = EBADMSG in caso di errori di comunicazione con il server
                        errno = EPROTO in caso di errori di protocollo interni al server
                        errno = ENOENT se il file pathname non esiste nel server
                        errno = EACCESS se non ho aperto il file
*/
int readFile(const char* pathname, void** buf, size_t* size);


/*
    @funzione readNFiles
    @descrizione: permette di leggere N file qualsiasi al server. 
    @param N è il numero di file che richiedo di leggere dal file storage server. Se N<=0 richiederò la lettura di tutti i file del file storage.
    @param dirname è la directory (lato client) dove salvare i file inviati dal server. Se dirname == NULL, i file letti vengono distrutti
    @return numero di file letti in caso di successo, -1 in caso di fallimento, setta errno
    @errori possibili:  errno = ENOTCONN se non sono connesso al server
                        errno = EBADMSG in caso di errori di comunicazione con il server
                        errno = EPROTO in caso di errori di protocollo interni al server
                        errno = EIO in caso di NON salvataggio di tutti i file inviati dal server
*/
int readNFiles(int N, const char* dirname);


/*
    @funzione writeFile
    @descrizione: permette di scrivere un file che è appena stato creato tramite il flag O_CREAT. Se la scrittura del file provoca l'espulsione
    di file nel file server storage, questi verrano inviati dal server e verranno salvati nella directory dirname
    @param pathname è il nome del file che deve essere scritto
    @param dirname è la directory dove salvare i file espulsi dal server. Se dirname==NULL i file vengono distrutti
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
    @errori possibili:  errno = EINVAL se pathname==NULL
                        errno = ENOTCONN se non sono connesso al server
                        errno = EBADMSG in caso di errori di comunicazione con il server
                        errno = EPROTO in caso di errori di protocollo interni al server
                        errno = ENOENT se il file pathname non esiste nel server
                        errno = EACCESS se provo a scrivere un file la cui ultima operazione di modifica non è stata la creazione
                        errno = EACCESS se non ho aperto il file
                        errno = EFBIG se il file inviato non entra nel file storage server
                        errno = EIO se il file pathname non esiste nel file system del client
                        errno = EIO in caso di NON salvataggio di tutti i file espulsi inviati dal server
*/
int writeFile(const char* pathname, const char* dirname);

/*
    @funzione writeFile
    @descrizione: permette di appendere ad un file il cotenuto di buf. Se la scrittura del file provoca l'espulsione
    di file nel file server storage, questi verrano inviati dal server e verranno salvati nella directory dirname
    @param pathname è il nome del file su cui devo appendere buf
    @param buf è il buffer che contiene ciò che devo appendere al file
    @param size è la dimensione di buf
    @param dirname è la directory dove salvare i file espulsi dal server. Se dirname==NULL i file vengono distrutti
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
    @errori possibili:  errno = EINVAL se pathname==NULL
                        errno = ENOTCONN se non sono connesso al server
                        errno = EBADMSG in caso di errori di comunicazione con il server
                        errno = EPROTO in caso di errori di protocollo interni al server
                        errno = ENOENT se il file pathname non esiste nel server
                        errno = EACCESS se non ho aperto il file
                        errno = EFBIG se il file presente nel file storage server con i byte aggiunti da buf non entra nel file storage server
                        errno = EIO in caso di NON salvataggio di tutti i file espulsi inviati dal server
*/
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);


/*
    @funzione closeFile
    @descrizione: permette di chiudere un file che ho precedentemente aperto nel file storage server
    @param pathname è il nome del file che voglio chiudere
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
    @errori possibili:  errno = EINVAL se pathname==NULL
                        errno = ENOTCONN se non sono connesso al server
                        errno = EBADMSG in caso di errori di comunicazione con il server
                        errno = EPROTO in caso di errori di protocollo interni al server
                        errno = ENOENT se il file pathname non esiste nel server
                        errno = EACCESS se non ho aperto il file   

    
*/
int closeFile(const char* pathname);


/*
    @funzione removeFile
    @descrizione: permette di rimuovere un file che ho precedentemente aperto nel file storage server
    @param pathname è il nome del file che voglio rimuovere
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
    @errori possibili:  errno = EINVAL se pathname==NULL
                        errno = ENOTCONN se non sono connesso al server
                        errno = EBADMSG in caso di errori di comunicazione con il server
                        errno = EPROTO in caso di errori di protocollo interni al server
                        errno = ENOENT se il file pathname non esiste nel server
                        errno = EACCESS se non ho aperto il file   
*/
int removeFile(const char* pathname);

/*
    @funzione scriviContenutoInDirectory
    @descrizione: permette di scrivere il contenuto di buffer nel file pathFile nella directory dirToSave. pathFile viene interpretato come un vero e proprio path: verrrano
    create delle subdirectory nella cartella dirToSave per mantenere il path che il file aveva nel file storage server
    @param buffer è il buffer che contiene il contenuto del file da salvare
    @param size è la dimensione di buffer
    @param pathFile è il pathname del file da scrivere
    @param dirToSave è la directory in cui salvare il file
    @return 0 in caso di successo, -1 in caso di fallimento, setta errno
*/
int scriviContenutoInDirectory(void* buffer, size_t size, char* pathFile, char* dirToSave);
#endif
