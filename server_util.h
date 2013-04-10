#include <string.h>
#include <time.h>
#include <map>
#include <set>
#include "util.h"

using namespace std;

struct clientInfo {
	int port;
	int sockfd;
	time_t start_time;
	string ip_address;
	string client_name;
	//Map ce contine fisierele si dimensiunile acestora
	map<string, long int> shared_files;
};

/** Determina tipul mesajului care a venit de la client.
 * Codificare: 
 *	1). S-a trimis nume client adaugat in sistem.
 *	2). Se cere lista de clienti.  
 *  etc... */
int getMessageType(char* message, char** restOfMessage);

/** Salveza din parametrii unui mesaj care initializeaza conexiunea 
 * intre client si server, numele si portul primului. */ 
int getNameAndPort(char* restofMessage, char** name, int* port);

/** Determina daca exista un anumit utilizator in sistem sau nu.
 * Dupa caz returneaza true sau false. */ 
bool clientExists(string name, map<string, clientInfo>& clients);


