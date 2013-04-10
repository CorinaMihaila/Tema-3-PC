#include "util.h"
#include "client_util.h"

#define MAX_CONN 5
using namespace std;

void error(string msg)
{
    perror(msg.c_str());
    exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, sockfd_client, sockfd_send, n, fdmax;
	struct sockaddr_in serv_addr, client_addr, send_addr;
	char buffer[BUFLEN];
	char *name, *file, *address; 
	int port;
	long int fsize; 
	// Un map ce retine mesajele pana cand vin informatii 
	// de la server. Cheia reprezinta clientul despre care
	// se doreste informatia, iar valoarea mesajul propriu-zis 
	// in cazul message sau numele fisierului in cazul getfile.
	map<string, queue<string> > messages;
	// Un map ce retine fisierele in curs de transfer - fisiere
	// ce trebuie luate de la alti clienti. Cheia reprezinta 
	// fd-ul clientului de la care se asteapta informatia. Un client
	// se poate conecta de mai multe ori la alt client pentru ca de
	// fiecare data se va creea un alt fd (unic). 
	map<int, transfer> filesToGet;
	// Un map ce retine fisierele in curs de transfer - fisiere 
	// ce trebuie trimise catre alti clienti. Cheia reprezinta fd-ul 
	// clientului catre care se trimite informatia.
	map<int, transfer> filesToSend;
	fd_set read_fds, temp_fds;
	FD_ZERO(&read_fds);

	if (argc < 5) {
       fprintf(stderr,"Utilizare: %s nume_client port_ascultare adresa_server port_server\n", argv[0]);
       exit(0);
    }

	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

	/* Parametri conexiunii client-server */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[4]));
    inet_aton(argv[3], &serv_addr.sin_addr);


    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
	
	/* Parametrii conexiunii pe care asculta clientul */
	sockfd_client = socket(AF_INET, SOCK_STREAM, 0);
	
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(atoi(argv[2]));
	client_addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd_client, (sockaddr*)&client_addr, sizeof(client_addr)) < 0)
		error("ERROR on binding.");
		
	listen(sockfd_client, MAX_CONN);
	
	/* Trimitem informatia cu numele si portul clientului.*/
	sprintf(buffer,"1\n%s\n%i\n", argv[1], atoi(argv[2]));
	//printf("buffer%s", buffer);
	n = send(sockfd, buffer, strlen(buffer)+1, 0);
	if (n < 0)
		error("ERROR writing to socket");

    FD_SET(sockfd, &read_fds);
    FD_SET(sockfd_client, &read_fds);
    FD_SET(0, &read_fds);
    FD_SET(0, &temp_fds);
    fdmax = sockfd_client;
	
	struct timeval t, *tv;
	t.tv_sec = 0;
	t.tv_usec = 1;
	
    while(1){
		
		temp_fds = read_fds;
		// Daca nu se asteapta fisier nu mai impunem 
		// nici un timeout pentru stdin. Altfel impunem
		// timeout pentru a nu intrerupe transferul. 
		if (filesToSend.size() == 0)
			tv = NULL;
		else 
			tv = &t;
			
    	if (select(fdmax+1, &temp_fds, NULL, NULL, tv) == -1)
    		error("Error in select");
    		
		if (FD_ISSET(0, &temp_fds)) {
			//Citesc de la tastatura
			memset(buffer, 0 , BUFLEN);
			fflush(stdin);
			fgets(buffer, BUFLEN-1, stdin);
			//scanf("%s", buffer);
			//printf("bambam%s", buffer);
			
			if (strcmp(buffer, "\n") != 0){
				char* params = NULL;
				switch (getCommandType(buffer, &params)) { 
					case 1: // Comanda listclients
					case 2:{// Comanda infoclient
						n = send(sockfd,buffer,strlen(buffer)+1, 0);
						if (n < 0)
    	 					error("ERROR writing to socket");
						break;
					} 
					case 3: {
						n = getMessageContent(buffer, messages, false);
						if (n < 0)
							printf(">>Comanda e de forma message <client> <corp_mesaj>.\n");
						else {
							n = send(sockfd, buffer, strlen(buffer)+1, 0);
							if (n < 0)
								error("ERROR writing to socket");
						}
						break;
					}
					case 4: {
						// Comanda sharefile
						int fdl;
						if (params == NULL) {
							printf(">>Comanda are nevoie de numele fisierului!\n");
							break;
						}
						fdl = open(params, O_RDONLY);
						if (fdl < 0)
							printf(">>Fisierul nu exista!\n");
						else {
							struct stat fileStats;
							n = stat(params, &fileStats);
							if (n < 0)
								error("ERROR file stat");
							char size[100];
							sprintf(size, "%ld", fileStats.st_size);
							strcat(buffer, " ");
							strcat(buffer, size);
							n = send(sockfd, buffer, strlen(buffer)+1, 0);
							if (n < 0)
    	 						error("ERROR writing to socket");
    	 					else 
								printf(">>Fisierul a fost adaugat la share\n");
						}
						close(fdl);
						break;
					}
					case 5:
						// Comanda unsharefile
						if (params == NULL){ 
							printf(">>Comanda are nevoie de numele fisierului!\n");
							break;
						}
						send(sockfd, buffer, strlen(buffer)+1, 0);
						break;
					case 6: {
						// Comanda getshare
						n = send(sockfd, buffer, strlen(buffer)+1, 0);
						if (n < 0)
							error("ERROR writing to socket");
						break;
					}
					case 7: {
						// Comanda getfile
						n = getMessageContent(buffer, messages, true);
						if (n < 0) 
							printf(">>Comanda e de forma getfile <client> <nume_fisier>.\n");
						else { 
							n = send(sockfd, buffer, strlen(buffer)+1, 0);
							if (n < 0)
								error("ERROR writing to socket");
						}
						break;
					}
					case 8: {
						n = send(sockfd,buffer, strlen(buffer)+1, 0);
						if (n < 0)
							error("ERROR writing to socket");
						else 
							close(sockfd); 
						printf(">>Clientul a fost inchis.\n");
						return 0;
						break;
					}
					default: { 
						printf(">>Invalid command!\n");
						break;
					}
				}
			// printf("params:%s|\n",params);
			// Trimit mesaj la server
			}
			
		}

		else if (FD_ISSET(sockfd, &temp_fds)){
			
			n = recv(sockfd, buffer, BUFLEN, 0);
			if (n <= 0){ 
				//conexiunea s-a inchis 
				if (n == 0){
					printf(">>Serverul a fost inchis.\n");
					break;
				}
				else
					printf("ERROR in recv");
			}
			
			if (n > 0) {
				int type = messageInfo(buffer, &name, &file, &fsize, &address, &port);
				if (type == 1){
				
					// Inseamna ca primim informatii despre un client catre 
					// care dorim sa trimitem un mesaj. Creeam un socket temporar
					// prin care trimitem un mesaj iar apoi il inchidem.
					char destbuffer[BUFLEN];
					sockfd_send = socket(AF_INET, SOCK_STREAM, 0);
    				if (sockfd_send < 0)
        				error("ERROR opening socket");
					
					send_addr.sin_family = AF_INET;
   	 				send_addr.sin_port = htons(port);
    				inet_aton(address, &send_addr.sin_addr);
    				
    				if (connect(sockfd_send,(struct sockaddr*) &send_addr,sizeof(send_addr)) < 0)
        				error("ERROR connecting");
        			else {
        			
		    			if (messages.find(string(name)) != messages.end()){
		    				sprintf(destbuffer,"message %s %s\n", argv[1], 
		    						messages[string(name)].front().c_str());
		    				n = send(sockfd_send, destbuffer, strlen(destbuffer)+1, 0);
		    				if (n < 0)
		    					error("ERROR on send");
		    				messages[string(name)].pop();
		    			}
		    			else
		    				error("ERROR in message");
        				close(sockfd_send);
        			} 
				}
				else if (type == 2){ 
					// Inseamna ca primim informatii despre un client de la care 
					// dorim sa luam un fisier. Vom crea socketul necesar 
					// transferului, vom trimite o cerere clientului de la care
					// dorim sa luam date si vom salva in map, fd-ul folosit pentru transfer.
					
					/*printf("RECIEVED: %s\n", buffer); */
					printf("name: %s, file %s, files %ld, address %s, port %i\n", 
							name, file, fsize, address, port);
					string fileout = string(file);
					char destbuffer[BUFLEN];
					sockfd_send = socket(AF_INET, SOCK_STREAM, 0);
    				if (sockfd_send < 0)
        				error("ERROR opening socket");
					
					send_addr.sin_family = AF_INET;
   	 				send_addr.sin_port = htons(port);
    				inet_aton(address, &send_addr.sin_addr);
    				
    				if (connect(sockfd_send,(struct sockaddr*) &send_addr,sizeof(send_addr)) < 0)
        				error("ERROR connecting");
        			else {
        				      			
		    			if (messages.find(string(name)) != messages.end()){
		    				string username = string(name);
		    				sprintf(destbuffer,"getfile %s\n", messages[username].front().c_str());
		    				n = send(sockfd_send, destbuffer, strlen(destbuffer)+1, 0);
		    				if (n < 0)
		    					error("ERROR on send");
		    				
		    				//Creeam fisierul in care vom scrie informatia
		    				char newname[BUFLEN]; 
		    				string completeName;
		    				sprintf(newname,"%s_primit", messages[username].front().c_str());
		    				filenameSuffix(string(newname), completeName);
		    				int filed = open(completeName.c_str(), O_CREAT, 0777);
		    				
		    				if (filed < 0) 
		    					error("ERROR on file creation");
		    				
		    				close(filed);  
		    				filed = open(completeName.c_str(), O_WRONLY);
		    				if (filed < 0)
		    					error("ERROR on file open");
		    					
		    				messages[username].pop(); 
		    				
		    				size_t suffix =  username.rfind("_file");
		    				username.replace(suffix, 5, "");
		    					
		    				filesToGet.insert(pair<int, transfer>(sockfd_send, transfer()));
		    				
		    				filesToGet[sockfd_send].fileout = fileout; // Nume initial
							filesToGet[sockfd_send].filein = completeName; // Nume salvat
        					filesToGet[sockfd_send].size = fsize; // Dimensiunea
							filesToGet[sockfd_send].processed = 0; // Cat a fost transferat
							filesToGet[sockfd_send].username = username; 
							filesToGet[sockfd_send].filed = filed;
		    				printf("username %s\n", filesToGet[sockfd_send].username.c_str());
		    				// Fisierul se va inchide cand se termina transferul
		    				
		    				FD_SET(sockfd_send, &read_fds);
		    				if (sockfd_send > fdmax)
								fdmax = sockfd_send; 
								
		    			}
		    			else
		    				error("ERROR in message");
        			}
        			// Socketul nu se inchide deocamdata, pentru ca asteptam si 
        			// continutul fisierului pe acelasi socket. O data ce se primeste tot fisierul 
        			// se va inchide si socketul.
					
				}
				else if (strcmp(buffer, "reject") == 0) {
					printf(">>Clientul NU a fost acceptat de serverul central.\n");
					return -1;
				}
				else if (strcmp(buffer, "accept") == 0) {
					printf(">>Clientul a fost acceptat de serverul central.\n");
				}
				else 
					printf(">>%s\n", buffer);
			}
			
		}
		
		else if (FD_ISSET(sockfd_client, &temp_fds)){ 
			int tempsock;
			
			unsigned int clilen = sizeof(client_addr);
			tempsock = accept(sockfd_client, (struct sockaddr*)&client_addr, &clilen);
			if (tempsock < 0)
				error("ERROR in accept");
			memset(buffer, 0, BUFLEN);
			n = recv(tempsock, buffer, BUFLEN, 0);
			if (n < 0)
				error("ERROR on receive");
			char* info;
			info = strtok(buffer," ");
			if (strcmp(info, "message") == 0){
				close(tempsock);
				// Afisam mesajul
				printf("%s\n", strtok(NULL, "\n"));
			}
			else if(strcmp(info, "getfile") == 0){
				// Nu mai inchidem socketul
				string filename(strtok(NULL, "\n"));
				
				int filed = open(filename.c_str(),O_RDONLY); 
				if (filed < 0) { 
					// Verificam daca mai exista fisierul
					char destbuffer[BUFLEN];
					strcpy(destbuffer,"Fisierul nu mai exista pe masina clientului.");
					n = send(tempsock, destbuffer, strlen(destbuffer)+1, 0);
					if (n < 0) 
						error("ERROR on send");
					// Cum nu mai avem ce transfera inchidem socketul
					close(tempsock);
				}
				else {
					struct stat fileStats;
					n = stat(filename.c_str(), &fileStats);
					if (n < 0)
						error("ERROR file stat");
					filesToSend.insert(pair<int, transfer>(tempsock, transfer()));
					filesToSend[tempsock].filein = filename;
					filesToSend[tempsock].size = fileStats.st_size;
					filesToSend[tempsock].processed = 0;
					filesToSend[tempsock].filed = filed;
				}
			}
		}
		
		else {
				//Se primeste in buffer continutul unui fisier
				for(int i = 1; i <= fdmax; i++) {
					if (FD_ISSET(i, &temp_fds)) { 
						if (filesToGet.find(i) != filesToGet.end()){ 
							// Daca exista o intrare in map asociata fisierului
							// care vine atunci scrie in fisier si actualizam 
							// statisticile
							int chars = recv(i, buffer, BUFLEN, 0);
							if (chars < 0)
								error("ERROR in recv");
							filesToGet[i].processed += chars;
							write(filesToGet[i].filed, buffer, chars);
							//int proc = (float)filesToGet[i].processed/(float)filesToGet[i].size * 100;
						
							if (filesToGet[i].size <= filesToGet[i].processed || 
								chars == 0) { 
								printf(">>Fisierul %s a fost transferat cu succes!\n",
										filesToGet[i].fileout.c_str());
								printf(">>Fisierul a fost salvat ca %s.\n", 
										filesToGet[i].filein.c_str());
								close(filesToGet[i].filed);
								close(i);
								FD_CLR(i, &read_fds);
							} /*else 
								printf("Fisierul %s se transfera... (%i%%)\n", 
										filesToGet[i].filein.c_str(), proc); */
						}
					}
				}
		}
			
			
		// La sfarsit dupa ce am citit unul din socketi, abia atunci 
		// trimitem BUFLEN octeti din fisierele pe care trebuie sa le trimitem
		map<int, transfer>::iterator itm = filesToSend.begin();
		while (itm != filesToSend.end()){
			int tempsock = itm->first; 
			int filed = itm->second.filed;
			memset(buffer, 0, BUFLEN);
			int chars = read(filed, buffer, BUFLEN);
			if (n < 0) 
				error("ERROR on file read"); 
			itm->second.processed += n;
			n = send(tempsock, buffer, chars, 0);
			//printf(">>S-a trimis un pachet.\n");
			if (n < 0) 
				error("ERROR on send");
			if (itm->second.processed == itm->second.size || chars == 0) {
				//S-a trimis tot fisierul.
				close(filed);
				close(tempsock);
				filesToSend.erase(itm++);
			}
			else 
				++itm;
		}
    }
    
    close(sockfd);
    return 0;
}

