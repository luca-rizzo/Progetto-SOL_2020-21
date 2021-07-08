# compilatore da usare
CC			=  gcc
# aggiungo alcuni flags di compilazione
CFLAGS	    += -std=c99 -Wall -g -D_DEFAULT_SOURCE
# directory librerie
LIB_DIR 	= ./mylibs
# directory includes
INCL_DIR	= ./includes
#flag inclusione 
INCLUDES	= -I ${INCL_DIR}

LIBS        = -pthread
#per linkare librerie dinamiche
DYNLINK		= -Wl,-rpath=${LIB_DIR} -L ${LIB_DIR}

#OBJECTS DEPENDENCIES
CLIENT_OBJS = objs/client/client.o objs/client/parseLineaComando.o objs/client/eseguiRichieste.o 

SERVER_OBJS = objs/server/server.o objs/server/threadpool.o objs/server/threadW.o objs/server/politicaSostituzione.o objs/server/log.o
LIB_API_OBJS = objs/api/api.o;
LIB_STRUTTURE_DATI_OBJS = objs/struttureDati/coda.o objs/struttureDati/icl_hash.o
#BIN
CLIENT_BIN = bin/client
SERVER_BIN = bin/server

TARGETS		= $(CLIENT_BIN) \
			  $(SERVER_BIN) \



.PHONY: all cleanall cleanTest1 cleanTest2 test1 test2 test3
.SUFFIXES: .c .h

#file oggetto client
objs/client/%.o: src/client/%.c $(INCL_DIR)/client.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<  $(LIBS)

#file oggetto server
objs/server/%.o: src/server/%.c $(INCL_DIR)/server.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<  $(LIBS)

all		: $(TARGETS)

#eseguibile client
$(CLIENT_BIN): $(CLIENT_OBJS) $(LIB_DIR)/libapi.so $(LIB_DIR)/libstruttureDati.so
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $(CLIENT_OBJS) $(LIBS) $(DYNLINK) -lapi -lstruttureDati

#eseguibile server
${SERVER_BIN}: $(SERVER_OBJS) $(LIB_DIR)/libstruttureDati.so
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $(SERVER_OBJS) $(LIBS) $(DYNLINK) -lstruttureDati

#libreria api
$(LIB_DIR)/libapi.so: $(LIB_API_OBJS)
	$(CC) -shared -o $@ $^

objs/api/api.o: src/api/api.c $(INCL_DIR)/api.h $(INCL_DIR)/conn.h $(INCL_DIR)/util.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -fPIC -o $@
	
#libreria struttura dati
$(LIB_DIR)/libstruttureDati.so: $(LIB_STRUTTURE_DATI_OBJS)
	$(CC) -shared -o $@ $^

objs/struttureDati/coda.o: src/struttureDati/coda.c includes/coda.h includes/util.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -fPIC -o $@

objs/struttureDati/icl_hash.o: src/struttureDati/icl_hash.c includes/icl_hash.h 
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -fPIC -o $@

#test

test1: 	cleanTest1
		valgrind --leak-check=full $(SERVER_BIN) test/test1/config.txt &
#aspetto si avvii server
		@sleep 2 
		./script/test1.sh
		@killall -s 1 -w memcheck-amd64-
#aspetto che valgrind stampi risultati
		@sleep 1
		@printf "\ntest1 terminato\n"

test2: 	cleanTest2
		$(SERVER_BIN) test/test2/config.txt &
#aspetto si avvii server
		@sleep 1 
		./script/test2.sh
		@killall -s 1 -w $(SERVER_BIN)
		@printf "\ntest2 terminato\n"

#cleanall 

cleanall: cleanTest1 cleanTest2
		rm -f -r objs/*/*.o
		rm -f -r mylibs/*
		rm -f -r tmp/csSock.csSock
		rm -f -r bin/*

cleanTest1: 
		rm -f -r test/test1/fileLetti/*/*

cleanTest2: 
		rm -f -r test/test2/fileLetti/*/*
		rm -f -r test/test2/backupFile



