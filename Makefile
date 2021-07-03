CC		=  gcc
AR              =  ar
CFLAGS	        += -std=c99 -Wall -g
ARFLAGS         =  rvs
INCLUDES	= -I ./includes
LDFLAGS 	= -L.
OPTFLAGS	= -O3 -DNDEBUG 
LIBS            = -pthread


TARGETS		= bin/client \
			  bin/server \
			  



.PHONY: all clean cleanall
.SUFFIXES: .c .h

bin/%: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

objs/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<  $(LIBS)

all		: $(TARGETS)

bin/client: objs/client/client.o objs/struttureDati/coda.o objs/client/parseLineaComando.o objs/client/eseguiRichieste.o objs/api/api.o
	$(CC) $(CCFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

bin/server: objs/server/server.o objs/struttureDati/icl_hash.o objs/server/threadpool.o objs/server/threadW.o objs/struttureDati/coda.o objs/server/politicaSostituzione.o
	$(CC) $(CCFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean		: 
	rm -f $(TARGETS)

cleanall	: clean
	rm -f objs/*.o *~ *.a
