#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFLEN 1024
/* Determina tipul comenzii introduse de la tastura. 
 * Fiecare comanda are o codificare in numar intreg. 
 * Codificare(in progress..): 
 *  1). listclients
 *  2). infoclient <client> */
int getCommandType(char* command, char** params);
