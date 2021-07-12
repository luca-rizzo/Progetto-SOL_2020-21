#if !defined(_UTIL_H)
#define _UTIL_H

#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <wait.h>
#include <signal.h>
#include <time.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)>(b))?(b):(a))

#define LOCK_RETURN(l, r)  if (pthread_mutex_lock(l)!=0)        {	\
    fprintf(stderr, "ERRORE FATALE lock\n");				\
    return r;								\
  }   

#define UNLOCK_RETURN(l,r)    if (pthread_mutex_unlock(l)!=0)      {	\
    fprintf(stderr, "ERRORE FATALE unlock\n");				\
    return r;								\
  }
  
#define WAIT_RETURN(c,l,r)    if (pthread_cond_wait(c,l)!=0)       {	    \
    fprintf(stderr, "ERRORE FATALE wait\n");			    \
    return r;			    \
  }
  
#define SIGNAL_RETURN(c,r)    if (pthread_cond_signal(c)!=0)       {		\
    fprintf(stderr, "ERRORE FATALE signal\n");				\
    return r;					\
  }
#define BCAST_RETURN(c,r)     if (pthread_cond_broadcast(c)!=0)    {		\
    fprintf(stderr, "ERRORE FATALE broadcast\n");			\
    return r;					\
 }

#define NEGSYSCALL(r,c,e) if((r=c)==-1) { perror(e);return -1; }
#define NULLSYSCALL(r,c,e) if((r=c)==NULL) { perror(e);return -1; }
#define SYSCALLRETURN(r,t) if(r==-1) { return t; }            
    
/*
    @funzione BThenA
    @descrizione: confronta due struct timespec
    @param a è il primo parametro da confrontare
    @param b è il secondo parametro da confrontare
    @return 1 se b è avvenuto PRIMA di a, 0 altrimenti
*/
static inline int BThenA(struct timespec a,struct timespec b){
	if (a.tv_sec == b.tv_sec)
        return a.tv_nsec > b.tv_nsec;
    else
        return a.tv_sec > b.tv_sec;
}

/*
	@funzione isNumber
	@descrizione: converte la stringa puntata da s in un long che viene salvato in n
	@param s è la stringa che voglio convertire
	@param n è il puntatore al long che conterra il valore convertito
	@return 0 in caso di successo, 2 in caso di overflow, 1 se la stringa non contiene un numero
*/

static inline int isNumber(const char* s, long* n) {
  if (s==NULL) return 1;
  if (strlen(s)==0) return 1;
  char* e = NULL;
  errno=0;
  long val = strtol(s, &e, 10);
  if (errno == ERANGE) return 2;    // overflow/underflow
  if (e != NULL && *e == (char)0) {
    *n = val;
    return 0;   // successo 
  }
  return 1;   // non e' un numero
}
#endif /* _UTIL_H */
