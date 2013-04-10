#include <stdio.h>
#include <string.h>
#include <map>
#include <list>
#include <sys/time.h>
#include <queue>
#include "util.h"

using namespace std;

/* O folosim pentru fisiere trimise/primite */
struct transfer {
	string fileout; // numele fisierului de pe alta masina clienta
	string filein; // numele local al clientului 
	string username; // numele utilizatorului implicat in transfer
	int filed; // fd folosit pentru a inchide fisierul la sfarsit
	long int processed; // nr de pachete primite
	long int size; // nr de pachete totale
};

/* Determina daca mesajul primit de la server contine informatii 
 * care sunt necesare pentru a trimite un mesaj text catre alt
 * client. De asemenea poate contine informatii pentru un mesaj
 * de tip getfile. */
int messageInfo(char* message, char** name, char** file, long int* filesize,
				char** address, int* port);

/* Primeste ca parametru un char* de forma message <nume_client>
 * <corp mesaj> si adauga in mapul dat ca parametru prin referinta 
 * o pereche ce contine drept cheie numele clientului si un mesaj 
 * in lista de mesaje a clientului respectiv */
int getMessageContent(char* buffer, map<string, queue<string> >& pendingMessages, bool file);

/* Decide daca numele atribuit fisierului mai are nevoie de un sufix 
 * in plus sau nu astfel incat sa nu suprascrie un fisier deja existent. */
void filenameSuffix(string name, string& finalname);
