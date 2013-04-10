#include "util.h"

int getCommandType(char* command, char** params){ 
	char* mCommand = new char[strlen(command)+4];
	sprintf(mCommand," %s \n",command);
	//printf("mCommand%s|\n", mCommand);
	char* type = strtok(mCommand, " \n");
	*params = strtok(NULL, " \n");
	
	
	if (type == NULL) 
		type = command;
		
	if (strcmp(type,"listclients") == 0){ 
		return 1;
	}
	else if(strcmp(type,"infoclient") == 0){
		return 2;
	}
	else if(strcmp(type,"message") == 0){
		return 3;
	}
	else if(strcmp(type,"sharefile") == 0){
		return 4;
	}
	else if(strcmp(type,"unsharefile") == 0){ 
		return 5;
	}
	else if(strcmp(type,"getshare") == 0){ 
		return 6;
	}
	else if(strcmp(type,"getfile") == 0){ 
		return 7;
	}
	else if(strcmp(type,"quit") == 0){
		return 8;
	}
	
	
	delete[] mCommand;	
	return -1;
}

