#include "util.h"
#include "server_util.h"

#define MAX_CLIENTS	5

using namespace std;

void error(string msg){
    perror(msg.c_str());
    exit(1);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	unsigned int clilen;
	char buffer[BUFLEN], *content = NULL;
	struct sockaddr_in serv_addr, cli_addr;
	int n, i;

	fd_set read_fds; //multimea de citire folosita in select()
	fd_set tmp_fds;  //multime folosita temporar
	int fdmax;		 //valoare maxima file descriptor din multimea read_fds
	
	/* Folosim un map pentru a retine informatiile despre client. 
	 * Cheile unice sunt file descriptori. Desi sistemul ne asigura 
	 * unicitatea acestora se foloseste map pentru a facilita acesarea
	 * informatiilor */ 
	map<int, clientInfo> fdclients;
	/* Tot un map cu informatii despre clienti. De aceasta data cheile 
	 * sunt numele clientilor. Valorile sunt cele din fdclients. Acestea 
	 * sunt pointeri catre cele din fdclients */
	map<string,clientInfo*> clients;
	
	clientInfo info;
	
	if (argc < 2) {
		fprintf(stderr,"Usage : %s port\n", argv[0]);
		exit(1);
	}

	//golim multimea de descriptori de citire (read_fds) si multimea tmp_fds
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
    	error("ERROR opening socket");

	portno = atoi(argv[1]);

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	// foloseste adresa IP a masinii
	serv_addr.sin_addr.s_addr = INADDR_ANY;	
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
		error("ERROR on binding");

	listen(sockfd, MAX_CLIENTS);

	// adaugam noul file descriptor (socketul pe care se 
	// asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	//adaugam 0 la file descritors
	FD_SET(0, &read_fds);
	fdmax = sockfd;

	// main loop
	while (1) {
		tmp_fds = read_fds;
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			error("ERROR in select");

		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				if (i == sockfd) {
					// a venit ceva pe socketul de ascultare = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					}
					else {
						//adaug noul socket intors de accept() la multimea 
						//descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax)
							fdmax = newsockfd;
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n",
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
					
					info.ip_address = inet_ntoa(cli_addr.sin_addr);
					info.port = ntohs(cli_addr.sin_port);
					info.start_time = time(NULL);
					info.sockfd = newsockfd;
					
					fdclients.insert(pair<int, clientInfo>(newsockfd, info));
				}

				else if (i == 0) {
					// Se citeste ceva de la tastatura
					memset(buffer, 0 , BUFLEN);
					fflush(stdin);
					fgets(buffer, BUFLEN-1, stdin);
					
					// Tratarea comenzilor
					if (strcmp(buffer, "status\n") == 0){
						// Afisarea informatiilor despre toti clientii
						map<int, clientInfo>::iterator itm;
						map<string, long int>::iterator its;
						for (itm = fdclients.begin(); itm != fdclients.end(); itm++){
							clientInfo value = itm->second;
							printf("nume: %-15s | adresa ip: %s | port: %-5i | fisiere share:\n",
									value.client_name.c_str(), value.ip_address.c_str(), 
									value.port);
							if (!value.shared_files.size()) 
								printf("(none)\n");
							its = value.shared_files.begin();
							for ( ; its != value.shared_files.end(); its++){
								printf("*%s %ld\n", its->first.c_str(), its->second);
							}
						}
					}
					else if (strcmp(buffer, "quit\n") == 0){
						// Serverul se inchide
						for(int j = 0; j <= fdmax; j++) {
							FD_CLR(j, &read_fds);
							close(j);
						}
						close(sockfd); 
						printf("Serverul a fost inchis.\n");
						return 0;
					}
					else { 
						printf("Comanda invalida!\n");
					}
				}
				
				// am primit date pe unul din socketii cu care vorbesc cu clientii
				//actiunea serverului: recv()
				else {
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("Clientul cu socketfd %d a inchis conexiunea.\n", i);
							clients.erase(fdclients[i].client_name);
							fdclients.erase(i);
							
						} else {
							error("ERROR in recv");
						}
						close(i);
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care
					}

					else { //recv intoarce >0
						printf ("Am primit de la clientul de pe socketul %d un mesaj.\n", i);
						switch (getMessageType(buffer, &content)) {
							case 1: {
								int portn;
								char* name;
								if (getNameAndPort(buffer, &name, &portn)){
									// Mesajul de tip 1 contine numele unui client recent adaugat
									string namestr(name);
									if (clients.find(name) == clients.end() && 
										fdclients.find(i) != fdclients.end()){ 
										//Inseamna ca numele clientului nu exista
										fdclients[i].client_name =  namestr;
										fdclients[i].port = portn;
										clients[name] = &fdclients[i];
										strcpy(buffer, "accept");
										send(i, buffer, strlen(buffer)+1, 0);	
									}
									else {
										fdclients.erase(i);
										strcpy(buffer, "reject");
										send(i, buffer, strlen(buffer)+1, 0);
									}
								}
								else 
									printf("Mesajul nu a putut fi prelucrat!\n");
								break;
							}
							default: {  
								// In mod default se primeste o comanda de la client
								char *params = NULL;
								getCommandType(buffer, &params);
								
								switch (getCommandType(buffer, &params)){ 
									case 1: {
										// Se cere lista tuturor clientilor conectati la server
										char destbuffer[BUFLEN] = "";
										map<int, clientInfo>::iterator it;
										strcpy(destbuffer, "Lista clienti: \n");
										unsigned int k = 0;
										for(it = fdclients.begin(); it != fdclients.end(); it++, k++){
											// Vom trimite in acelasi buffer numele a mai multor 
											// clienti. Nu stim sigur ca incap toti clienti intr-un
											// singur buffer.
											if (strlen(destbuffer) + (*it).second.client_name.size()+2 >= BUFLEN){
												send(i, destbuffer, strlen(destbuffer)+1, 0);
												memset(destbuffer, 0, BUFLEN);
											}
							 				//printf("*%s\n", (*it).second.client_name.c_str());
											strcat(destbuffer, it->second.client_name.c_str());
											if (k != fdclients.size()-1)
												strcat(destbuffer, "\n");
											
										}
										if (strlen(destbuffer) > 0)
											send(i, destbuffer, strlen(destbuffer)+1, 0);
										break;
									}
									
									case 2: { 
										// Se cere informatie despre un anumit client din sistem
										char destbuffer[BUFLEN];
										if (params != NULL) {  
											string name(params);
											if (clients.find(name) != clients.end()) {
												
												clientInfo* info = clients[name]; 
												sprintf(destbuffer,"nume: %s, port: %i, timp scurs de la conectare: %lds",
														info->client_name.c_str(), info->port, 
														(time(NULL) - info->start_time));
										
											}
											else  
												sprintf(destbuffer,"Clientul nu exista!");
											send(i, destbuffer, strlen(destbuffer)+1, 0);
										}		
										break;					
									}
									
									case 3: { 
										// Un client doreste sa trimita un mesaj catre alt
										// client. Ii vom trimite informatiile inapoi informatii.
										char destbuffer[BUFLEN];
										if (params != NULL){
											string name(params);
											if (clients.find(name) != clients.end()){ 
												clientInfo* info = clients[name];
												sprintf(destbuffer,"message\n%s\n%s\n%i\n", name.c_str(),
														info->ip_address.c_str(), info->port);
											}
											else 
												sprintf(destbuffer,"Clientul nu exista!");
											send(i, destbuffer, strlen(destbuffer)+1, 0);
										}
										break;
									}
									
									case 4: { 
										// Se adauga un anumit fisier la share
										if (params != NULL) { 
											string file_name(params);
											long int size; 
											sscanf(buffer, "%*s %*s %ld", &size);
											clientInfo& info = fdclients[i];
											// Faptul ca aveam stocate intr-un set 
											// numele fisierelor aflate la share ne asigura 
											// ca nu exista 2 fisiere cu acelasi nume
											info.shared_files.insert(pair<string, long int>(file_name, size));
										}
										
										break;
									}
									
									case 5: { 
										// Se scoate un fisier de la share
										char destbuffer[BUFLEN];
										if (params != NULL) { 
											string file_name(params);
											map<string, long int>& shared_files = fdclients[i].shared_files;
											
											if (shared_files.find(file_name) == shared_files.end()) { 
												sprintf(destbuffer, "Fisierul %s nu este la share!",
														file_name.c_str());
											}
											else { 
												shared_files.erase(params);
												sprintf(destbuffer, "Fisierul %s a fost scos de la share.",
														file_name.c_str());
											}
											
											send(i, destbuffer, strlen(destbuffer)+1, 0);
											
										}
									}
									
									case 6: { 
										// Se trimite shareul unui client
										char destbuffer[BUFLEN];
										if (params != NULL){
											string client(params);
											if (clients.find(client) == clients.end()){
												sprintf(destbuffer, "Clientul nu exista!");
												send (i, destbuffer, strlen(destbuffer)+1, 0);
											}
											else {
												sprintf(destbuffer, "Share '%s':\n", params);
												map<string, long int>& shared_files = clients[client]->shared_files;
												if (!shared_files.size()) { 
													sprintf(destbuffer, "Clientul '%s' nu are nimic la share.", params);
													send(i, destbuffer, strlen(destbuffer)+1, 0);
												}
												unsigned int k = 0;
												map<string, long int>::iterator it;
												
												for(it = shared_files.begin(); it != shared_files.end(); it++, k++){
													if (strlen(destbuffer) + it->first.size()+2 >= BUFLEN){
														send(i, destbuffer, strlen(destbuffer)+1, 0);
														memset(destbuffer, 0, BUFLEN);
													}
													strcat(destbuffer,it->first.c_str()); 
													if (k != shared_files.size() - 1){ 
														strcat(destbuffer,"\n");
													}
												}
												
												if (strlen(destbuffer) > 0){
													send(i, destbuffer, strlen(destbuffer)+1, 0);
												}
											}
										}
										break;
									}
									case 7: { 
										// Comanda getfile. Ii vom trimite clientului care
										// vrea sa ia date de la un alt client informatiile
										// despre acesta. 
										char destbuffer[BUFLEN];
										if (params != NULL){
											string name(params);
											char filename[BUFLEN];
											sscanf(buffer, "%*s %*s %s", filename);
											// Verificam daca exista clientul cautat
											if (clients.find(name) != clients.end()){ 
												clientInfo* info = clients[name];
												// Verificam daca fisierul este la share
												if (info->shared_files.find(string(filename)) != 
													info->shared_files.end()) { 
														
													sprintf(destbuffer,"getfile\n%s_file\n%s\n%ld\n%s\n%i\n", 
															name.c_str(), filename, info->shared_files[string(filename)], 
															info->ip_address.c_str(), info->port);
												}
												else 
													//Altfel vom trimite mesaje de eroare
													sprintf(destbuffer,"Clientul '%s' nu are fisierul '%s' la share!",
															params, filename); 
											}
											else 
												sprintf(destbuffer,"Clientul nu exista!");
											send(i, destbuffer, strlen(destbuffer)+1, 0);
										}
										break;
									}
									case 8: { 
										printf("Clientul cu socketfd %d a inchis conexiunea.\n", i);
										clients.erase(fdclients[i].client_name);
										fdclients.erase(i);
										FD_CLR(i, &read_fds);
										close(i);
										break;
									}
									default: {
										printf("Mesajul nu a putut fi prelucrat!\n");
										break;
									}
					
								}
								break;
							}
						}
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;

}

