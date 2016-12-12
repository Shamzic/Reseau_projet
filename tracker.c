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


message_t reception_msg_put_get(unsigned char * buf,char IP_TYPE)
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
  //message.client.type = buf[38];             // Type du client TOUJOURS 55 donc osef
  message.client.length = buf_to_s_int(buf+39); // Taille du client
  message.client.port = buf_to_s_int(buf+41);                  // Port du client 
  memcpy(buf+43,message.client.address_ip,length_address);  // Addresse du client
  return message;
}

int HashDansListe(char* hash, stock_list *sc,int taille) // Cherche hash dans tab
{                                                    // de [Hash+clients]            
    int i,bool=1;                                    // return 0 = true
    for (i =0; i<taille; i++)                        // return 1 =false
    {
        if(strncmp(sc[i].hash,hash,strlen(hash)))
                bool=0;
    }
    return bool; 
}

int positionHash(char* hash, stock_list *sc,int taille) // Cherche la position du
{                                                   // hash dans le tableau 
    int i=0;                                        // de [hash + client]
    while((i<taille) && (!strncmp(sc[i].hash,hash,strlen(hash))))
    {
        i++;
    }
    return i;
}

char traite_msg(message_t mess,stock_list *sc,taille_l t) // traite TOUS les msgs
{                                                     // recu par le trackers        
    if( mess.type == 110 )
    {
        printf("Cherche la position dans le tab de hash ");
        print_hash((unsigned char*)mess.hash.hash);
        printf("\n");
        if( HashDansListe(mess.hash.hash,sc,t.taille_actuelle) == 1) 
        {                                        // Hash pas dans tab => nouveau hash
                                                 // ajouté en fin de tableau + client
            printf("Le hash n'est pas encore dans le tableau des hash : %s\n",mess.hash.hash);
            if(t.taille_actuelle == t.taille_max)
            {
                t.taille_max=t.taille_max+t.taille_max;
                sc=realloc(sc,sizeof(stock_list)*t.taille_max);
            }
            sc[t.taille_actuelle-1].hash = malloc(32);
            strncpy(sc[t.taille_actuelle-1].hash,mess.hash.hash,strlen(mess.hash.hash));
            printf("Ajout du hash dans la liste chainée !\n");
            sc[t.taille_actuelle-1].liste_clients=NULL;
            
            sc[t.taille_actuelle-1].liste_clients=ajouterEnTete(sc[t.taille_actuelle-1].liste_clients,mess.client.length,mess.client.port,mess.client.address_ip);
            printf("Affichage de la liste des clients du hash\n");
            afficherListe (sc[t.taille_actuelle-1].liste_clients);
            t.taille_actuelle++;
        }
        else if(HashDansListe(mess.hash.hash,sc,t.taille_actuelle)==0) 
        // hash dans tab => ajout à la suite
        {
            int pos = positionHash(mess.hash.hash,sc,t.taille_actuelle);
            printf("Hash %s déjà dans dans le tableau des hash à la position %d\n",mess.hash.hash,pos);
            sc[pos].liste_clients=ajouterEnFin(sc[pos].liste_clients,
            mess.client.length,mess.client.port,mess.client.address_ip);
        }
        return 110;
    }
    
    else if( mess.type == 112 )
    {
        if(HashDansListe(mess.hash.hash,sc,t.taille_actuelle)==0) 
    	{   
    	    printf("hash trouvé dans la liste !\n");
       		return 112;
    	}
    	else
    	    printf("hash PAS trouvé dans la liste :(\n");
    }
    return 0;
}

void ack_put(int sockfd,struct sockaddr_in dest,  char * buf,socklen_t addrlen)
{
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

// char * retour = malloc(100*sizeof(client_s));
int creer_char_liste_client(char* retour,char * hash, stock_list *stlist,char IP_TYPE,taille_l t)
{
    int pos = positionHash(hash,stlist,t.taille_actuelle); 
    // A priori on met max 100 clients          
    int taille_clients = 0;
    client_s* temp = stlist[pos].liste_clients;
    
    // Affichage de la liste des clients pour vérifier :
    printf("Liste du client\n");
    afficherListe(temp);
    
    int i=0;
    short int length;
    if(IP_TYPE == 4)
        length = 4;
    else
        length = 16;
    while ( temp != NULL );
    {
        retour[i]=55;
        i=i+1;
        retour[i]=2+length; // longueur du client
        i=i+2;
        retour[i]=temp->port; // port du client
        i=i+2;
        memcpy(retour+i,temp->address_ip,length);
        taille_clients = taille_clients + length + 3;
        temp= temp->next;
    }
    return taille_clients;
}
//ack_get(sockfd,client,buf,addrlen,stlist,t,(char)4);
void ack_get(int sockfd,struct sockaddr_in dest,  char * buf,socklen_t addrlen,stock_list *stlist,message_t mess,taille_l t,char IP_TYPE)
{

    char * retour = malloc(36+100*sizeof(client_s));
    retour[0]=113;
    // PREMIERE ETAPE copier la partie hash dans le retour
    memcpy(retour,buf+3,35);
    // DEUXIEME ETAPE concatener la liste des clients dans le retour
    char * retour2 = malloc(100*sizeof(client_s));
    int taille_clients = creer_char_liste_client(retour,mess.hash.hash,stlist,IP_TYPE,t);
    strncat(retour,retour2,taille_clients);         
    printf("Avant l'envoi du paquet..\n");
    // TROISIEME ETAPE Envoie le ACK au client qui l'a demandé
    if(sendto(sockfd,retour,strlen(retour),0,(struct sockaddr *)&dest,addrlen) == -1)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
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
    // my_addr.sin_addr.s_addr = htonl(atoi(argv[1]));
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
    stlist[0].hash = malloc(32);
    //strncpy(stlist[0].hash,"00000000000000000000000000000000",32);
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

            // print the received ip
        printf("New message from %s : \n",ip);
        mess = reception_msg_put_get((unsigned char*)buf,(char)4);
	    printf("valeur du type de message reçu : %d\n",mess.type);
	    char retour = traite_msg(mess,stlist,t);
        
       printf("addresse client : %s\n", inet_ntoa(client.sin_addr));

        if(retour == 110)
        {
            ack_put(sockfd,client,buf,addrlen);
        }
        if (retour == 112)
        {
            printf("On a reçu un message get\n");
            ack_get(sockfd,client,buf,addrlen,stlist,mess,t,(char)4);
        }
        sleep(1);
        // => socket , sockaddr de destination , port , 
        // , address ip destionation , taille addrlen , message à envoyer
    }
    // close the socket
    close(sockfd);
    return 0;
}



