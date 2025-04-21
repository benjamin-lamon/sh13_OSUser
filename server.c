#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <arpa/inet.h>

#include <pthread.h>

//funny color stuff
#define ANSI_RESET_ALL          "\x1b[0m"

#define ANSI_COLOR_BLACK        "\x1b[30m"
#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_YELLOW       "\x1b[33m"
#define ANSI_COLOR_BLUE         "\x1b[34m"
#define ANSI_COLOR_MAGENTA      "\x1b[35m"
#define ANSI_COLOR_CYAN         "\x1b[36m"
#define ANSI_COLOR_WHITE        "\x1b[37m"

#define ANSI_BACKGROUND_BLACK   "\x1b[40m"
#define ANSI_BACKGROUND_RED     "\x1b[41m"
#define ANSI_BACKGROUND_GREEN   "\x1b[42m"
#define ANSI_BACKGROUND_YELLOW  "\x1b[43m"
#define ANSI_BACKGROUND_BLUE    "\x1b[44m"
#define ANSI_BACKGROUND_MAGENTA "\x1b[45m"
#define ANSI_BACKGROUND_CYAN    "\x1b[46m"
#define ANSI_BACKGROUND_WHITE   "\x1b[47m"

#define ANSI_STYLE_BOLD         "\x1b[1m"
#define ANSI_STYLE_ITALIC       "\x1b[3m"
#define ANSI_STYLE_UNDERLINE    "\x1b[4m"


struct _client
{
        char ipAddress[40];
        int port;
        char name[40];
		int elimine; 	// Utilisé quand un joueur fait une mauvaise accusation;
						// Lorsque le joueur fait une mauvaise accusation, on lève le flag.
						// Alors, son tour sera toujours passé.
						// 0 = pas éliminé, 1 = éliminé. C'est un booléen en somme.
} tcpClients[4];
int nbClients;
int fsmServer;
int deck[13]={0,1,2,3,4,5,6,7,8,9,10,11,12};
int tableCartes[4][8];
int elimCount = 0;
char *nomcartes[]=
{"Sebastian Moran", "Irene Adler", "inspector Lestrade",
  "inspector Gregson", "inspector Baynes", "inspector Bradstreet",
  "inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
  "Mrs. Hudson", "Mary Morstan", "James Moriarty"};
int joueurCourant;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void melangerDeck()
{
        int i;
        int index1,index2,tmp;

        for (i=0;i<1000;i++)
        {
                index1=rand()%13;
                index2=rand()%13;

                tmp=deck[index1];
                deck[index1]=deck[index2];
                deck[index2]=tmp;
        }
}

void createTable()
{
	// Le joueur 0 possede les cartes d'indice 0,1,2
	// Le joueur 1 possede les cartes d'indice 3,4,5 
	// Le joueur 2 possede les cartes d'indice 6,7,8 
	// Le joueur 3 possede les cartes d'indice 9,10,11 
	// Le coupable est la carte d'indice 12
	int i,j,c;

	for (i=0;i<4;i++)
		for (j=0;j<8;j++)
			tableCartes[i][j]=0;

	for (i=0;i<4;i++)
	{
		for (j=0;j<3;j++)
		{
			c=deck[i*3+j];
			switch (c)
			{
				case 0: // Sebastian Moran
					tableCartes[i][7]++;
					tableCartes[i][2]++;
					break;
				case 1: // Irene Adler
					tableCartes[i][7]++;
					tableCartes[i][1]++;
					tableCartes[i][5]++;
					break;
				case 2: // Inspector Lestrade
					tableCartes[i][3]++;
					tableCartes[i][6]++;
					tableCartes[i][4]++;
					break;
				case 3: // Inspector Gregson 
					tableCartes[i][3]++;
					tableCartes[i][2]++;
					tableCartes[i][4]++;
					break;
				case 4: // Inspector Baynes 
					tableCartes[i][3]++;
					tableCartes[i][1]++;
					break;
				case 5: // Inspector Bradstreet 
					tableCartes[i][3]++;
					tableCartes[i][2]++;
					break;
				case 6: // Inspector Hopkins 
					tableCartes[i][3]++;
					tableCartes[i][0]++;
					tableCartes[i][6]++;
					break;
				case 7: // Sherlock Holmes 
					tableCartes[i][0]++;
					tableCartes[i][1]++;
					tableCartes[i][2]++;
					break;
				case 8: // John Watson 
					tableCartes[i][0]++;
					tableCartes[i][6]++;
					tableCartes[i][2]++;
					break;
				case 9: // Mycroft Holmes 
					tableCartes[i][0]++;
					tableCartes[i][1]++;
					tableCartes[i][4]++;
					break;
				case 10: // Mrs. Hudson 
					tableCartes[i][0]++;
					tableCartes[i][5]++;
					break;
				case 11: // Mary Morstan 
					tableCartes[i][4]++;
					tableCartes[i][5]++;
					break;
				case 12: // James Moriarty 
					tableCartes[i][7]++;
					tableCartes[i][1]++;
					break;
			}
		}
	}
} 

void printDeck()
{
        int i,j;

        for (i=0;i<13;i++)
                printf("%d %s\n",deck[i],nomcartes[deck[i]]);

	for (i=0;i<4;i++)
	{
		for (j=0;j<8;j++)
			printf("%2.2d ",tableCartes[i][j]);
		puts("");
	}
}

void printClients()
{
        int i;

        for (i=0;i<nbClients;i++)
                printf("%d: %s %5.5d %s\n",i,tcpClients[i].ipAddress,
                        tcpClients[i].port,
                        tcpClients[i].name);
}

int findClientByName(char *name)
{
        int i;

        for (i=0;i<nbClients;i++)
                if (strcmp(tcpClients[i].name,name)==0)
                        return i;
        return -1;
}

void sendMessageToClient(char *clientip,int clientport,char *mess)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server = gethostbyname(clientip);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(clientport);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        {
                printf("ERROR connecting\n");
                exit(1);
        }

        sprintf(buffer,"%s\n",mess);
        n = write(sockfd,buffer,strlen(buffer));

    close(sockfd);
}

void broadcastMessage(char *mess)
{
        int i;

        for (i=0;i<nbClients;i++)
                sendMessageToClient(tcpClients[i].ipAddress,
                        tcpClients[i].port,
                        mess);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	int i;

        char com;
        char clientIpAddress[256], clientName[256];
        int clientPort;
        int id;
        char reply[256];

		//à gérer avec des threads pour que tout le monde ait accès à la mémoire
		//un thread par client
		//à chaque fois qu'un client renvoie une info, on check si la "commande" est correcte
		//puis on broadcast quelque chose à tout le monde
		//ensuite, chaque client répond
		//pas besoin de mutex ? on peut en mettre un en place par précaution mais pas vraiment d'intérêt ici...


		// Tout compte fait, quelle est l'utilité d'un thread ici????
		// Les clients sont "ajoutés" un par un dans tcpClients[] lorsqu'on est déjà dans la boucle while...
		// Donc on ouvre une instance du serveur par partie.

	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr,"ERROR opening socket\n");
		exit(1);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on binding");
		exit(1);
	}
              

	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	printDeck();
	melangerDeck();
	createTable();
	printDeck();
	joueurCourant=0;

	for (i=0;i<4;i++){
		strcpy(tcpClients[i].ipAddress,"localhost");
		tcpClients[i].port=-1;
		strcpy(tcpClients[i].name,"-");
		tcpClients[i].elimine = 0; // À la base, personne n'est éliminé, ça arrive suelement lors d'un maauvais guilt.
	}

     while (1){    
     	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			error("ERROR on accept");
		}
          	

     	bzero(buffer,256);
     	n = read(newsockfd,buffer,255);
     	if (n < 0) {
			error("ERROR reading from socket");
		}

        printf("Received packet from %s:%d\nData: [%s]\n\n",
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buffer);

        if (fsmServer==0){
        	switch (buffer[0]){
                	case 'C':
                        	sscanf(buffer,"%c %s %d %s", &com, clientIpAddress, &clientPort, clientName);
                        	printf("COM=%c ipAddress=%s port=%d name=%s\n",com, clientIpAddress, clientPort, clientName);

                        	// fsmServer==0 alors j'attends les connexions de tous les joueurs
                                strcpy(tcpClients[nbClients].ipAddress,clientIpAddress);
                                tcpClients[nbClients].port=clientPort;
                                strcpy(tcpClients[nbClients].name,clientName);
                                nbClients++;

                                printClients();

				// rechercher l'id du joueur qui vient de se connecter

                                id=findClientByName(clientName);
                                printf("id=%d\n",id);

				// lui envoyer un message personnel pour lui communiquer son id

                                sprintf(reply,"I %d",id);
                                sendMessageToClient(tcpClients[id].ipAddress, tcpClients[id].port,reply);
								bzero(reply,256);

				// Envoyer un message broadcast pour communiquer a tout le monde la liste des joueurs actuellement
				// connectes

                                sprintf(reply,"L %s %s %s %s", tcpClients[0].name, tcpClients[1].name, tcpClients[2].name, tcpClients[3].name);
								broadcastMessage(reply);
								bzero(reply,256);

				// Si le nombre de joueurs atteint 4, alors on peut lancer le jeu

                if (nbClients==4){

				//Probablement à faire avec des threads ? sinon je ne vois pas l'utilité du module...

					// On envoie ses cartes au joueur 0, ainsi que la ligne qui lui correspond dans tableCartes
					// RAJOUTER DU CODE ICI
					//cartes 0,1,2
					// Mettre les cartes dans le 'buffer' reply :
					// D si j'en crois ce qui est dans sh13.c
					sprintf(reply,"D %d %d %d", deck[0], deck[1], deck[2]);
					// envoyer reply au joueur 0
					sendMessageToClient(tcpClients[0].ipAddress, tcpClients[0].port, reply);

					//ATTENTION
					//Il faut peut-être burn le contenu de reply avant de le réutiliser
					bzero(reply,256);

					// Est-ce qu'on lui envoie aussi ce qui lui correspond dans tableCartes ?? I guess??
					for (int j = 0; j < 8; j++){
						//format : joueur (=ligne), colonne, valeur
						sprintf(reply,"V 0 %d %d", j, tableCartes[0][j]);
						sendMessageToClient(tcpClients[0].ipAddress, tcpClients[0].port, reply);
						//burn en question
						bzero(reply,256);
					}

					// On envoie ses cartes au joueur 1, ainsi que la ligne qui lui correspond dans tableCartes
					// RAJOUTER DU CODE ICI
					//idem avec les cartes 3,4,5
					sprintf(reply,"D %d %d %d", deck[3], deck[4], deck[5]);
					sendMessageToClient(tcpClients[1].ipAddress, tcpClients[1].port, reply);
					//idem pour les valeurs de tableCartes
					for (int j = 0; j < 8; j++){
						sprintf(reply,"V 1 %d %d", j, tableCartes[1][j]);
						sendMessageToClient(tcpClients[1].ipAddress, tcpClients[1].port, reply);
						bzero(reply,256);
					}


					// On envoie ses cartes au joueur 2, ainsi que la ligne qui lui correspond dans tableCartes
					// RAJOUTER DU CODE ICI
					//u get the idea
					sprintf(reply,"D %d %d %d", deck[6], deck[7], deck[8]);
					sendMessageToClient(tcpClients[2].ipAddress, tcpClients[2].port, reply);
					for (int j = 0; j < 8; j++){
						sprintf(reply,"V 2 %d %d", j, tableCartes[2][j]);
						sendMessageToClient(tcpClients[2].ipAddress, tcpClients[2].port, reply);
						bzero(reply,256);
					}


					// On envoie ses cartes au joueur 3, ainsi que la ligne qui lui correspond dans tableCartes
					// RAJOUTER DU CODE ICI
					sprintf(reply,"D %d %d %d", deck[9], deck[10], deck[11]);
					sendMessageToClient(tcpClients[3].ipAddress, tcpClients[3].port, reply);
					for (int j = 0; j < 8; j++){
						sprintf(reply,"V 3 %d %d", j, tableCartes[3][j]);
						sendMessageToClient(tcpClients[3].ipAddress, tcpClients[3].port, reply);
						bzero(reply,256);
					}
					


					// On envoie enfin un message a tout le monde pour definir qui est le joueur courant=0
					// RAJOUTER DU CODE ICI
					sprintf(reply,"M %d", joueurCourant);
					broadcastMessage(reply);
					fsmServer=1;
					bzero(reply,256);

					// À ce stade, le coupable est la carte deck[12].
				}

			break;
			}
		}
		else if (fsmServer==1){

			if (tcpClients[joueurCourant].elimine == 1){
				printf("Joueur courant: %d [éliminé]\n", joueurCourant);
				joueurCourant++;
				joueurCourant %= 4;
				printf("Joueur courant: %d\n", joueurCourant);
			}
			else{
				printf("Joueur courant: %d\n", joueurCourant);
			}

			switch (buffer[0]){
				case 'G':
					// RAJOUTER DU CODE ICI
					// truc en rapport avec tcpClients[i].elimine
					// à faire seulement si le guess est erroné
					int joueur, guess;
					sscanf(buffer,"G %d %d", &joueur, &guess);
					if (joueur == joueurCourant){
						printf("Accusation: %d %d\n", joueur, guess);
					if (guess == deck[12]){
							//win
							sprintf(reply,"Le joueur %d a trouvé le coupable !\nFin de la partie.", joueur);
							broadcastMessage(reply);
							close(newsockfd);
							close(sockfd);
							fsmServer=0;
							exit(0);
							break;
						}
						else{
							//eliminate
							tcpClients[joueur].elimine = 1;
							sprintf(reply,"Joueur %d éliminé!", joueur);
							broadcastMessage(reply);
							elimCount++;
							//if n-1 people guessd wrong,
							if (elimCount == 3){
								//figure out who wins
								int winner = -1;
								for (int i = 0; i < 4; i++){
									if (tcpClients[i].elimine == 0){
										winner = i;
										break;
									}
								}
								sprintf(reply,"3 joueurs éliminés. Le joueur gagnant est %d !",winner);
								broadcastMessage(reply);
								close(newsockfd);
								close(sockfd);
								fsmServer=0;
								exit(0);
							}
							else{
								joueurCourant++;
								joueurCourant %= 4;
							}
						}
					}
					break;
				case 'O':
					// RAJOUTER DU CODE ICI
					// Enquête 2 (symbole)
					int sender, symbol;
					sscanf(buffer,"O %d %d", &sender, &symbol);
					if (sender == joueurCourant){
						printf("Enquête 2 demandée par %d sur le symbole %d\n", sender, symbol);
						//Envoie la valeur de tableCartes[sender][symbol]
						for (int i = 0; i < 4; i++){
							if (tableCartes[i][symbol] > 0){
								sprintf(reply,"V %d %d 9", i, symbol); 	// le max de symbole étant 5, 9 nous indique
																		// que le joueur possède le symbole, pas combien...
																		// = à préciser + 9 est arbitraire tant qu'on est >5
								broadcastMessage(reply);
								bzero(reply,256);
							}
							else{
								sprintf(reply,"V %d %d 0", i, symbol);
								sendMessageToClient(tcpClients[sender].ipAddress, tcpClients[sender].port, reply);	
								broadcastMessage(reply);
								bzero(reply,256);
							}
						}
						//sprintf(reply,"V %d %d %d", sender, symbol, tableCartes[sender][symbol]);
						//sendMessageToClient(tcpClients[sender].ipAddress, tcpClients[sender].port, reply);	
						//broadcastMessage(reply);
						bzero(reply,256);
						joueurCourant++;
						joueurCourant %= 4;
					}
					
					break;
				case 'S':
					// RAJOUTER DU CODE ICI
					// Enquête 1 (joueur et symbole)
					int idSender, idJoueur, idObjet;
					sscanf(buffer,"S %d %d %d", &idSender, &idJoueur, &idObjet);
					if (idSender == joueurCourant){
						printf("Enquête 1 demandée par %d sur %d avec l'objet %d\n", idSender, idJoueur, idObjet);
						//Envoie la valeur de tableCartes[idJoueur][idObjet]
						sprintf(reply,"V %d %d %d", idJoueur, idObjet, tableCartes[idJoueur][idObjet]);
						//sendMessageToClient(tcpClients[idSender].ipAddress, tcpClients[idSender].port, reply);	
						broadcastMessage(reply);
						bzero(reply,256);
						joueurCourant++;
						joueurCourant %= 4;
					}

					break;
				
				default:
					break;
				}
		}
		close(newsockfd);
	}
    close(sockfd);
    return 0; 
}
