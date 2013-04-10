#include "client_util.h"
#include <sstream>

using namespace std;

/* Determina daca mesajul primit de la server contine informatii 
 * care sunt necesare pentru a trimite un mesaj text catre alt
 * client. De asemenea poate contine informatii pentru un mesaj
 * de tip getfile. */
int messageInfo(char* message, char** name, char** file, 
				long int* filesize, char** address, int* port) { 
	char messagecpy[BUFLEN];
	char *portstr, *sizestr; 
	strcpy(messagecpy, message);
	char* type = strtok(messagecpy,"\n");
	if (type != NULL && strcmp(type, "message") == 0){
		*name = strtok(NULL, "\n");
		*address = strtok(NULL, "\n");
		portstr = strtok(NULL, "\n");
		if (!(*name) || !(*address) || !(portstr))
			return -1; 
		*port = atoi(portstr);
		return 1;
	}
	else if (type != NULL && strcmp(type, "getfile") == 0){
		*name = strtok(NULL, "\n");
		*file = strtok(NULL, "\n");
		sizestr = strtok(NULL, "\n");
		*address = strtok(NULL, "\n");
		portstr = strtok(NULL, "\n");
		if (!(*name) || !(*address) || !(portstr) ||!(*file))
			return -1;
		sscanf(sizestr,"%ld", filesize);
		*port = atoi(portstr);
		return 2;
	}
	else 
		return -1;
}

/* Primeste ca parametru un char* de forma message <nume_client>
 * <corp mesaj> si adauga in mapul dat ca parametru prin referinta 
 * o pereche ce contine drept cheie numele clientului si un mesaj 
 * in lista de mesaje a clientului respectiv */
int getMessageContent(char* buffer, map<string, queue<string> >& pendingMessages, bool file){
	char buffercpy[BUFLEN];
	strcpy(buffercpy, buffer);
	strtok(buffercpy, " "); // scapam de message
	char *name_client = strtok(NULL, " ");
	if (!name_client) 
		return -1; // Lipseste nume client din mesaj
	char message[BUFLEN];
	memset(message, 0, BUFLEN);
	char *messagep = strtok(NULL, "\n");
	if (!messagep)
		return -2; // Lipseste corpul mesajului
	strcat(message, messagep);
	char name[BUFLEN];
	strcpy(name, name_client);
	if (file) 
		strcat(name, "_file");
		
	pendingMessages.insert(pair<string, queue<string> >(string(name), 
						   queue<string>()));
	pendingMessages[string(name)].push(string(message));
	
	return 1;
}

/* Decide daca numele atribuit fisierului mai are nevoie de un sufix 
 * in plus sau nu astfel incat sa nu suprascrie un fisier deja existent. */
void filenameSuffix(string name, string& finalname){ 
	int fd; 
	string tempname;
	int count = 1;
	fd = open(name.c_str(), O_RDONLY);
	if (fd < 0) { 
		finalname = name;
		return;
	}	
	while (1) {
		stringstream ss;
		ss << count;
		tempname = name;
		tempname.append(ss.str());
		fd = open(tempname.c_str(), O_RDONLY);
		if (fd < 0){ 
			finalname = tempname;
			break;
		}
		ss.flush(); 
		count++;
	}
}
