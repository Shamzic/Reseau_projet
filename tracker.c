#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "structures.h"
#include "messages.h"

/* réceptionne l'arrivée d'un message d'un client dans une structure message_t */
/* IP_TYPE = 4 si IPv4 et 6 pour IPv6 */

typedef struct s_stock_list
{
    char * hash;
    client_s* liste_clients;
} *stock_list;


client_s* ajouterEnTete (client_s* liste_clients, short int length,short int port,char *address_ip)
{
  client_s* new_client = malloc(sizeof(client_s));
  new_client->length=length;
  new_client->port=port;
  strcpy (new_client->address_ip,address_ip); // tableau statique , no need init
  new_client->next = liste_clients;
  return new_client;
}


client_s* ajouterEnFin (client_s* liste_clients,short int length,short int port,char *address_ip)
{
  client_s* new_client = malloc(sizeof(client_s));
  new_client->length=length;
  new_client->port=port;
  strcpy (new_client->address_ip,address_ip); // tableau statique , no need init
  new_client->next = NULL;
  
  if (liste_clients == NULL)
    {
      return new_client;
    }
  else
    {
      client_s* temp = liste_clients;
      while (temp->next != NULL)
	{
	  temp = temp->next;
	}
      temp->next = new_client;
      return liste_clients;
    }
}


void afficherListe (client_s* liste_clients)
{
  client_s* tmp = liste_clients;
  int i=1;
  while (tmp != NULL)
    {
      printf ("Client %d :\n",i);
      printf("\t-Length : %hi\n",tmp->length);
      printf("\t-Port : %hi\n",tmp->port);
      printf("\t-Adresse : %s\n\n",tmp->address_ip);
      tmp = tmp->next;
      i++;
    }
}


client_s* effacerListe (client_s* liste_clients)
{
  if (liste_clients == NULL)
    {
      return NULL;
    }
  else
    {
      client_s* tmp;
      tmp = liste_clients->next;
      free (liste_clients);
      return effacerListe (tmp);
    }
}


client_s* supprimerElementEnTete (client_s* liste_clients)
{
  if (liste_clients != NULL)
    {
      client_s* aRenvoyer = liste_clients->next;
      free (liste_clients);
      return aRenvoyer;
    }
  else
    {
      return NULL;
    }
}


message_t reception_msg_put(unsigned char * buf,char IP_TYPE)
{
  message_t message;
  short int length_address;
  if (IP_TYPE == 4)
        length_address = 4;
    else
        length_address = 16;
        
  message.type = buf[0];                       // Type du message total
  message.length= buf_to_s_int(buf+1);         // Longueur du message total
  message.hash.type = buf[3];                  // Type du msg_hash
  message.hash.length = buf_to_s_int(buf+4);   // Taille du msg_hash
  memcpy(buf+6,message.hash.hash,32);          // Hash
  //message.client.type = buf[38];               // Type du client TOUJOURS 55
  message.client.length = buf_to_s_int(buf+39); // Taille du client
  message.client.port = buf_to_s_int(buf+41);                  // Port du client 
  memcpy(buf+43,message.client.address_ip,length_address);  // Addresse du client
  return message;
}
/*
void init_stocklist(stock_list *stocklist)
{
  stocklist=malloc(1*sizeof(stock_list));
  stocklist->liste_clients=NULL;
}*/


int HashDansListe(char* hash, stock_list *sc,int taille) // Cherche hash dans tab
{                                                    // de [Hash+clients]            
    int i,bool=1;                                    // return 0 = true
    for (i =0; i<taille; i++)                        // return 1 =false
    {
        if(sc[i]->hash == hash)
                bool=0;
    }
    return bool; 
}

int positionHash(char* hash, stock_list *sc,int taille) // Cherche la position du
{                                                   // hash dans le tableau 
    int i=0;                                        // de [hash + client]
    while((i<taille) && (sc[i]->hash!=hash))
    {
        i++;
    }
    return i;
}

void traite_msg(message_t mess,stock_list *sc,int taille) // traite TOUS les msgs
{                                                     // recu par le trackers
    if( mess.type == 110 )
    {
        if( HashDansListe(mess.hash.hash,sc,taille) == 1) 
        {                                        // Hash pas dans tab => nouveau hash
                                                 // ajouté en fin de tableau + client
            taille++;
            sc=realloc(sc,sizeof(stock_list)*taille);
            sc[taille-1]->hash = malloc(sizeof(mess.hash.hash));
            strcpy(sc[taille-1]->hash,mess.hash.hash);
            sc[taille-1]->liste_clients=NULL;
            sc[taille-1]->liste_clients=ajouterEnTete(sc[taille-1]->liste_clients,mess.client.length,mess.client.port,mess.client.address_ip);
        }
        else if(HashDansListe(mess.hash.hash,sc,taille)==0) 
        // hash dans tab => ajout à la suite
        {
            int pos = positionHash(mess.hash.hash,sc,taille);
            sc[pos]->liste_clients=ajouterEnFin(sc[pos]->liste_clients,
            mess.client.length,mess.client.port,mess.client.address_ip);
        }
    }
}

int main(int argc, char **argv)
{
    int sockfd;
    socklen_t addrlen;
    char buf[1024];

    struct sockaddr_in my_addr;
    memset (&my_addr, 0, sizeof (struct sockaddr_in));
    struct sockaddr_in client;
    memset (&client, 0, sizeof (struct sockaddr_in));

    // check the number of args on command line
    if(argc != 3)
    {
      printf("USAGE: %s source_address port_num\n", argv[0]);
      exit(-1);
    }

    // socket factory
    if((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("socket");
	      exit(EXIT_FAILURE);
    }

    // init local addr structure and other params
    my_addr.sin_family      = AF_INET;
    my_addr.sin_port        = htons(atoi(argv[2]));
    if(inet_pton(AF_INET,argv[1],&my_addr.sin_addr.s_addr) != 1)
    {
        perror("inet_pton\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    addrlen  = sizeof(struct sockaddr_in);
    memset(buf,'\0',1024);

    // bind addr structure with socket
    if(bind(sockfd, (struct sockaddr *) &my_addr, addrlen) == -1)
    {
      perror("bind");
      close(sockfd);
      exit(EXIT_FAILURE);
    }

    // set the socket in passive mode (only used for accept())
    // and set the list size for pending connection
    if(listen(sockfd, 5) == -1)
    {
        perror("listen");
	close(sockfd);
	exit(EXIT_FAILURE);
    }

    printf("Waiting for incomming connection\n");
    fd_set messockets; // ensemble de sockets à tout moment
    FD_ZERO(&messockets);
    FD_SET(sockfd,&messockets);
	// Il faut à tout moment la valeur max des sockets d'un ensemble
    int max = sockfd;
    int i,sockfdnew, rcv;
    
    /************* déclaration structures + initialisation stock_list **************/
    message_t mess;
    stock_list stlist=malloc(sizeof(stock_list));
    int taille_liste = 1;
    stlist->hash= NULL;
    stlist->liste_clients=NULL;
    
    while(1)
    {
		fd_set readfds = messockets; // celui qu'on utilise pour select (pck select modifie)
		if(select(max + 1,&readfds,NULL,NULL,NULL) == -1)
		{
			perror("select");
			exit(1);
		}
		//printf("après select\n");
		for(i=0;i<max + 1; i++)
		{
			if(FD_ISSET(i,&readfds))
			{
				if(i==sockfd) // socket passive qui attend les co
				{// accept
					if((sockfdnew = accept(sockfd,(struct sockaddr *) &client, &addrlen ) ) == -1)
					{
						perror("accept");
						close(sockfd);
						exit(EXIT_FAILURE);
					}
					FD_SET(sockfdnew,&messockets);
					if(sockfdnew > max)
						max = sockfdnew;
				}
				else
				{// recv
					//printf("avant\n");
					rcv = recv(i, buf, 1023, 0);
					//printf("après\n");
					if(rcv == -1)
					{
					  perror("recv");
					  close(sockfd);
					  exit(EXIT_FAILURE);
					}
					else if (rcv == 0) // fermer la connexion
					{
						close(i);
						FD_CLR(i,&messockets);
						printf("close connexion to %d \n" , i);
					}
					else
					{
						// print the received char
						printf("%s\n",buf);			
						mess = reception_msg_put
						((unsigned char*)buf,(char)4);
					        printf("valeur du type : %c",mess.type);
					        traite_msg(mess,&stlist,taille_liste);
						
						
						
						
									
					}
				}
			}
		}
	}
    
    
    
    // fermeture des sockets
    close(sockfd);
    //close(sockfd2);

    return 0;
}
