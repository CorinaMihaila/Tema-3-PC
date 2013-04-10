CC=g++
LIBSOCKET=-lnsl
CCFLAGS=-Wall -g
SRV=server
CLT=client
SRV_SRC=server.cpp server_util.cpp util.cpp
CLT_SRC=client.cpp client_util.cpp util.cpp

all: $(SRV) $(CLT)

$(SRV): $(SRV_SRC) 
	$(CC) -o $(SRV) $(CCFLAGS) $(LIBSOCKET) $(SRV_SRC)

$(CLT):	$(CLT_SRC)
	$(CC) -o $(CLT) $(CCFLAGS) $(LIBSOCKET) $(CLT_SRC)

clean:
	rm -f *.o *~
	rm -f $(SRV) $(CLT)

