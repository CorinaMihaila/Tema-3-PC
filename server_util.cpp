#include "server_util.h"

int getMessageType(char* message, char** restOfMessage){
	char messagecpy[BUFLEN];
	int type;
	strcpy(messagecpy, message);
	char* messageType = strtok(messagecpy, "\n");
	if (messageType != NULL)
		type = atoi(messageType); 
	*restOfMessage = strtok(NULL, "\n");
	
	return type;
}

int getNameAndPort(char* message, char** name, int* port){ 
	char* portstr;
	char messagecpy[BUFLEN]; 
	strcpy(messagecpy, message);
	strtok(messagecpy,"\n");
	*name = strtok(NULL,"\n");
	portstr = strtok(NULL, "\n");
	if (*name != NULL && portstr != NULL){
		*port = atoi(portstr);
		return 1; 
	}
	else 
		return -1;
} 

bool clientExists(string name, map<string, clientInfo>& clients){ 
	if (clients.find(name) != clients.end())
		return true;
	else 
		return false;
}
