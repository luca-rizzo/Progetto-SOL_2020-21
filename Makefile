# compilatore da usare
CC			=  gcc
# aggiungo alcuni flags di compilazione
CFLAGS	    += -std=c99 -Wall -g
# directory librerie
LIB_DIR 	= ./mylibs
# directory includes
INCL_DIR	= ./includes
#flag inclusione 
INCLUDES	= -I ${INCL_DIR}
#flag di ottimizzazione
OPTFLAGS	= -O3 -DNDEBUG 
LIBS        = -pthread
#per linkare librerie dinamiche
DYNLINK		= -Wl,-rpath=${LIB_DIR} -L ${LIB_DIR}

#OBJECTS
CLIENT_OBJS = objs/client/client.o objs/client/parseLineaComando.o objs/client/eseguiRichieste.o 
SERVER_OBJS = objs/server/server.o objs/server/threadpool.o objs/server/threadW.o objs/server/politicaSostituzione.o
LIB_API_OBJS = objs/api/api.o;
LIB_STRUTTURE_DATI_OBJS = objs/struttureDati/coda.o objs/struttureDati/icl_hash.o
#BIN
CLIENT_BIN = bin/client
SERVER_BIN = bin/server

TARGETS		= ${CLIENT_BIN} \
			  ${SERVER_BIN} \
			  



.PHONY: all cleanall cleanTest1 cleanTest2 test1 test2
.SUFFIXES: .c .h

objs/client/%.o: src/client/%.c ${INCL_DIR}/client.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<  $(LIBS)

objs/server/%.o: src/server/%.c ${INCL_DIR}/server.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<  $(LIBS)

all		: $(TARGETS)

${CLIENT_BIN}: $(CLIENT_OBJS) ${LIB_DIR}/libapi.so ${LIB_DIR}/libstruttureDati.so
	$(CC) $(CCFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $(CLIENT_OBJS) $(LIBS) $(DYNLINK) -lapi -lstruttureDati

${SERVER_BIN}: $(SERVER_OBJS) ${LIB_DIR}/libstruttureDati.so
	$(CC) $(CCFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $(SERVER_OBJS) $(LIBS) $(DYNLINK) -lstruttureDati

#libreria api
${LIB_DIR}/libapi.so: ${LIB_API_OBJS} 
	$(CC) -shared -o $@ $^

objs/api/api.o: src/api/api.c ${INCL_DIR}/api.h ${INCL_DIR}/conn.h ${INCL_DIR}/util.h
	$(CC) $(CCFLAGS) $(INCLUDES) $< -c -fPIC -o $@
	
#libreria struttura dati
${LIB_DIR}/libstruttureDati.so: ${LIB_STRUTTURE_DATI_OBJS}
	$(CC) -shared -o $@ $^

objs/struttureDati/coda.o: src/struttureDati/coda.c includes/coda.h includes/util.h
	$(CC) $(CCFLAGS) $(INCLUDES) $< -c -fPIC -o $@

objs/struttureDati/icl_hash.o: src/struttureDati/icl_hash.c includes/icl_hash.h 
	$(CC) $(CCFLAGS) $(INCLUDES) $< -c -fPIC -o $@

#test

test1: 	cleanTest1
		./script/test1.sh

test2:	cleanTest1
		./script/test2.sh

cleanall: cleanTest1 cleanTest2
		rm -f -r objs/*/*.o
		rm -f -r mylibs/*
		rm -f -r tmp/csSock.csSock
		rm -f -r bin/*

cleanTest1: 
		rm -f -r test/test1/fileLetti/*/*

cleanTest2: 
		rm -f -r test/test2/fileLetti/*/*
