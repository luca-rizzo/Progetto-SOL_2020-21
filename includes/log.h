#ifndef LOG_H
#define LOG_H
typedef struct{
	FILE* lfp;
	pthread_mutex_t mtx;
}t_log;
t_log* inizializzaFileLog(const char* dirToSave);
int scriviSuFileLog(t_log* log,char* buffer,...);
int distruggiStrutturaLog(t_log* logStr);
#endif
