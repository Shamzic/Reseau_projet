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
}stock_list;

typedef struct taille_liste
{
    int taille_actuelle;
    int taille_max;
}taille_l;

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
        if(sc[i].hash == hash)
                bool=0;
    }
    return bool; 
}

int positionHash(char* hash, stock_list *sc,int taille) // Cherche la position du
{                                                   // hash dans le tableau 
    int i=0;                                        // de [hash + client]
    while((i<taille) && (sc[i].hash!=hash))
    {
        i++;
    }
    return i;
}

void traite_msg(message_t mess,stock_list *sc,taille_l t) // traite TOUS les msgs
{                                                     // recu par le trackers
    if( mess.type == 110 )
    {
        if( HashDansListe(mess.hash.hash,sc,t.taille_actuelle) == 1) 
        {                                        // Hash pas dans tab => nouveau hash
                                                 // ajouté en fin de tableau + client
            t.taille_actuelle++;
            if(t.taille_actuelle == t.taille_max)
            {
                t.taille_max=t.taille_max+t.taille_max;
                sc=realloc(sc,sizeof(stock_list)*t.taille_max);
            }
            sc[t.taille_actuelle-1].hash = malloc(sizeof(mess.hash.hash));
            strcpy(sc[t.taille_actuelle-1].hash,mess.hash.hash);
            sc[t.taille_actuelle-1].liste_clients=NULL;
            sc[t.taille_actuelle-1].liste_clients=ajouterEnTete(sc[t.taille_actuelle-1].liste_clients,mess.client.length,mess.client.port,mess.client.address_ip);
        }
        else if(HashDansListe(mess.hash.hash,sc,t.taille_actuelle)==0) 
        // hash dans tab => ajout à la suite
        {
            int pos = positionHash(mess.hash.hash,sc,t.taille_actuelle);
            sc[pos].liste_clients=ajouterEnFin(sc[pos].liste_clients,
            mess.client.length,mess.client.port,mess.client.address_ip);
        }
    }
}

void ack_put(message_t mess,stock_list *sc,int taille)
{
;
}

int main(int argc, char **argv)
{
    int sockfd;
    char buf[1024];
    char ip[20];
    socklen_t addrlen;

    struct sockaddr_in my_addr;
    struct sockaddr_in client;

    // check the number of args on command line
    if(argc != 3)
    {
        printf("Usage: %s ip_address local_port\n", argv[0]);
	exit(-1);
    }

    // socket factory
    //int socket(int domain, int type, int protocol);
    if((sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == -1)
    {
        perror("socket");
	exit(EXIT_FAILURE);
    }

    // init local addr structure and other params
    my_addr.sin_family      = AF_INET;
    my_addr.sin_port        = htons(atoi(argv[2]));
 //   my_addr.sin_addr.s_addr = htonl(atoi(argv[1]));
    addrlen                 = sizeof(struct sockaddr_in);
    
    // get addr from command line and convert it
    if(inet_pton(AF_INET,argv[1],&my_addr.sin_addr) != 1)
    {
        perror("inet_pton");
	close(sockfd);
	exit(EXIT_FAILURE);
    }
    
	//int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
    // bind addr structure with socket
    if(bind(sockfd,(struct sockaddr *)&my_addr,addrlen) == -1)
    {
      perror("blind");
      close(sockfd);
      exit(EXIT_FAILURE);
    }
    
     // déclaration structures + initialisation stock_list 
    message_t mess;
    taille_l t;
    t.taille_actuelle = 1;
    t.taille_max=10;
    stock_list* stlist=malloc(sizeof(stock_list)*t.taille_max);
    stlist[0].liste_clients=NULL;

    while(1)
    {
        /*ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
               struct sockaddr *src_addr, socklen_t *addrlen);*/
        // reception de la chaine de caracteres
    
        memset(buf,'\0',1024);
        memset(ip,'\0',20);
        printf("je vais attendre\n");
        if(recvfrom(sockfd,buf,1024,0,(struct sockaddr *)&client,&addrlen) == -1)
        {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("jai eu\n");
            //const char *inet_ntop(int af, const void *src,char *dst, socklen_t size);
        if(inet_ntop(AF_INET,&client.sin_addr.s_addr,ip,20) == NULL)
        {
            perror("inet_ntop");
	    close(sockfd);
	    exit(EXIT_FAILURE);
        }

            // print the received char
        printf("New message from %s : \n%s\n\n",ip,buf);
        mess = reception_msg_put((unsigned char*)buf,(char)4);
	printf("valeur du type : %c",mess.type);
	traite_msg(mess,stlist,t);
	
	
	{
	    int sockfd;
            socklen_t addrlen;
            struct sockaddr_in dest;
            //  printf("USAGE: %s @dest port_num string\n", argv[0]);
            if((sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == -1)
            {
                perror("socket");
	        exit(EXIT_FAILURE);
            }

            // init remote addr structure and other params
            dest.sin_family = AF_INET;
            dest.sin_port   = htons(atoi(argv[2]));
            addrlen         = sizeof(struct sockaddr_in);
            
           if(inet_pton(AF_INET,mess.client.address_ip,&dest.sin_addr.s_addr) != 1)
           {
               perror("inet_pton");
	       close(sockfd);
	       exit(EXIT_FAILURE);
           }
           // modifie le type du buf à renvoyer :
           buf[0]=111;
           
           // Envoie le ACK 
           if(sendto(sockfd,buf,strlen(buf),0,(struct sockaddr *)&dest,addrlen) == -1)
           {
               perror("sendto");
	       close(sockfd);
	       exit(EXIT_FAILURE);
           }
        }
    }
    // close the socket
    close(sockfd);
    return 0;
}



