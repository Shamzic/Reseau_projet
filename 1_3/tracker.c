#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "structures.h"
#include "messages.h"

#define EXPIRATION 40
/* réceptionne l'arrivée d'un message d'un client dans une structure message_t */
/* IP_TYPE = 4 si IPv4 et 6 pour IPv6 */

typedef struct s_stock_list
{
    //char * hash;
    char hash[32];
    client_s* liste_clients;
    int taille;
}stock_list;

typedef struct taille_liste
{
    int taille_actuelle;
    int taille_max;
    int nb_hash;
}taille_l;

client_s* ajouterEnTete (client_s* liste_clients, short int length,short int port,char *address_ip)
{
  client_s* new_client = malloc(sizeof(client_s));
  new_client->length=length;
  new_client->port=port;
  memcpy (new_client->address_ip,address_ip,4); // tableau statique , no need to init
  new_client->next = liste_clients;
  new_client->time = time(NULL);
  return new_client;
}


client_s* ajouterEnFin (client_s* liste_clients,short int length,short int port,char *address_ip)
{
  client_s* new_client = malloc(sizeof(client_s));
  new_client->length=length;
  new_client->port=port;
  memcpy (new_client->address_ip,address_ip,4); // tableau statique
  new_client->next = NULL;
  new_client->time = time(NULL);
  
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
      printf("\t-Adresse : ");
      printf("%d.",tmp->address_ip[0]);
      printf("%d.",tmp->address_ip[1]);
      printf("%d.",tmp->address_ip[2]);
      printf("%d\n",tmp->address_ip[3]);
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

  //  printf("\nle vrai hash : ");
   // print_hash(buf + 6);
   // printf("\n");
  message.type = buf[0];                       // Type du message total
  message.length= buf_to_s_int(buf+1);         // Longueur du message total
  message.hash.type = buf[3];                  // Type du msg_hash
  message.hash.length = buf_to_s_int(buf+4);   // Taille du msg_hash
  memcpy(message.hash.hash,buf+6,32);          // Hash
 // printf("\n le hash :");
 // print_hash((unsigned char*)message.hash.hash);
 // printf("\n");
  //message.client.type = buf[38];             // Type du client TOUJOURS 55 donc osef
  message.client.length = buf_to_s_int(buf+39); // Taille du client
  message.client.port = buf_to_s_int(buf+41);                  // Port du client 
  memcpy(message.client.address_ip,buf+43,length_address);  // Addresse du client
  return message;
}

int HashDansListe(char* hash, stock_list *sc,int taille) // Cherche hash dans tab
{                                                    // de [Hash+clients]            
    int i,bool=1;                                    // return 0 = true
    printf(" \t\t\t la taille %d\n",taille);
    for (i =0; i<taille; i++)                        // return 1 =false
    {   
        
        if(memcmp(sc[i].hash,hash,strlen(hash))==0)
        {
            printf("trouvé !\n");   
            return bool=0;
        }
    }
    return bool; 
}

int positionHash(char* hash, stock_list *sc,int taille) // Cherche la position du
{                                                   // hash dans le tableau 
    int i=0;                                        // de [hash + client]
    printf("Recherche de la position : Taille du tableau %d\n",taille);
    while((i<taille) && (memcmp(sc[i].hash,hash,strlen(hash)))!=0)
    {
        i++;
    }
    return i;
}

char traite_msg(message_t mess,stock_list *sc,taille_l* t) // traite TOUS les msgs
{                                                     // recu par le trackers        
    if( mess.type == 110 )
    {
        printf("Cherche la position dans le tab de hash ");
        print_hash((unsigned char*)mess.hash.hash);
        printf("\n");
        if( HashDansListe(mess.hash.hash,sc,t->taille_actuelle) == 1) 
        {                                        // Hash pas dans tab => nouveau hash
                                                 // ajouté en fin de tableau + client
            printf("Le hash n'est pas encore dans le tableau des hash : %s\n",mess.hash.hash);
            if(t->taille_actuelle == t->taille_max)
            {
                t->taille_max=t->taille_max+t->taille_max;
                sc=realloc(sc,sizeof(stock_list)*t->taille_max);
            }
            //sc[t.taille_actuelle-1].hash = malloc(32);
            
            printf("Ajout du hash");
            print_hash((unsigned char*)mess.hash.hash);
            printf(" dans la liste chainée à la position %d\n",t->nb_hash);
            strncpy(sc[t->nb_hash].hash,mess.hash.hash,32);
            
            printf("Ajout d'un client en tête de liste en même pos\n");
            sc[t->nb_hash].liste_clients=NULL;
            sc[t->nb_hash].liste_clients=ajouterEnTete(sc[t->nb_hash].liste_clients,mess.client.length,mess.client.port,mess.client.address_ip);
            printf("Affichage de la liste des clients du hash\n");
            afficherListe (sc[t->nb_hash].liste_clients);
            t->taille_actuelle++; 
            t->nb_hash++;
            printf("Taille du tableau des hash après ajout du client : %d\n"
            ,t->taille_actuelle);
            //init à 0 le prochain hash
            strncpy(sc[t->nb_hash].hash,"00000000000000000000000000000000",32);
        }
        else if(HashDansListe(mess.hash.hash,sc,t->taille_actuelle)==0) 
        // hash dans tab => ajout à la suite
        {
            int pos = positionHash(mess.hash.hash,sc,t->nb_hash);
            printf("Hash %s déjà dans dans le tableau des hash à la position %d\n",mess.hash.hash,pos);
            sc[pos].liste_clients=ajouterEnTete(sc[pos].liste_clients,
            mess.client.length,mess.client.port,mess.client.address_ip);
            printf("Affichage de la liste des clients du hash\n");
            afficherListe (sc[pos].liste_clients);
        }
        return 110;
    }
    
    else if( mess.type == 112 )
    {
        if(HashDansListe(mess.hash.hash,sc,t->taille_actuelle)==0) 
    	{   
    	    printf("hash trouvé dans la liste !\n");
       		return 112;
    	}
    	else
    	    printf("hash PAS trouvé dans la liste :(\n");
    }
    else if(mess.type == 114 ) // keep alive
    {
        // met le fichier à jour 
    }
    return 0;
}

void ack_put(int sockfd,struct sockaddr_in dest,  char * buf,socklen_t addrlen)
{
    // modifie le type du buf à renvoyer :
    buf[0]=111;
    // Envoie le ACK
    if(sendto(sockfd,buf,buf_to_s_int((unsigned char *)(buf + 1)) + 3,0,(struct sockaddr *)&dest,addrlen) == -1)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

// char * retour = malloc(100*sizeof(client_s));
int creer_char_liste_client(unsigned char * retour,char * hash, stock_list *stlist,char IP_TYPE,taille_l* t)
{
    int pos = positionHash(hash,stlist,t->taille_actuelle); 
    printf("Hash à get à la position %d du tableau de HASH\n",pos);
    // A priori on met max 100 clients          
    int taille_clients = 0;
    client_s* temp = stlist[pos].liste_clients;
    if ( temp == NULL )
        printf("TEMP VAUT NULL \n");
    // Affichage de la liste des clients pour vérifier :
    printf("Liste du client\n");
    //afficherListe(temp);
    
    int i=0;
    short int length;
    if(IP_TYPE == 4)
        length = 4;
    else
        length = 16;
        printf("test\n");
    while ( temp != NULL )
    {
        retour[i]=55;
        i=i+1;
        retour[i]=2+length; // longueur du client, 4+2 si ipv4 ou 16+2 si ipv6
        i=i+2;
        retour[i]=temp->port; // port du client
        i=i+2;
        memcpy(retour+i,temp->address_ip,length);
        i = i+ 3 + length;
        taille_clients = taille_clients + length + 3;
        printf("copie d'un client sur %d octets\n",taille_clients);
        temp= temp->next;
    }
    printf("liste copiée\n");
    return taille_clients;
}
//ack_get(sockfd,client,buf,addrlen,stlist,t,(char)4);
void ack_get(int sockfd,struct sockaddr_in dest,  char * buf,socklen_t addrlen,stock_list *stlist)
{
    printf("------- MGS GET ------------\n");
    int pos=0;
    client_s * clients = stlist->liste_clients;
    int taille = 0;
    while (clients != NULL)
    {
        taille += 1+2+clients->length;
        clients = clients->next;
    }
    printf("putain de taille %d\n",taille);
    clients = stlist->liste_clients;
    unsigned char * retour = malloc(38+taille);
    memcpy(retour+3,buf+3,35);
    retour[0]=113;
    printf("juste avant l'envoie %d\n",retour[0]);
    
    pos += 3+3+32 ;
    // copie la liste des clients
    while ( clients != NULL)
    {
        if(clients->time +60 > time(NULL))
        {
            printf("pos est à %d\n",pos);
            retour[pos] = 55;
            printf("pos est à %d\n",pos);
            pos++;
            retour = s_int_to_buf((unsigned char*)retour, clients->length,pos);
            pos +=2;
            printf("pos est à %d\n",pos);
            retour = s_int_to_buf((unsigned char*)retour,clients->port,pos);
            pos +=2;
            printf("IP : %d.%d.%d.%d port %d\n",clients->address_ip[0],clients->address_ip[1],clients->address_ip[2],clients->address_ip[3], clients->port);
            memcpy(retour + pos , clients->address_ip,clients->length-2);
            printf("pos est à %d\n",pos);
            pos += clients->length -2;
            clients = clients->next;
            printf("pos est à %d\n",pos);
        }
        else
            clients = clients->next;
    }
    
    printf(" la taille est %d\n", taille + 38);
    retour = s_int_to_buf(retour,taille + 38,1);                    // copie la taille du msg_get dans
    
                                                     // la 2e case sur 2 octets
    //printf("Avant l'envoi du paquet..\n");
    
    // Envoie le ACK au client qui l'a demandé
    printf("juste avant l'envoie %d\n",retour[0]);
    if(sendto(sockfd,retour,taille + 38 ,0,(struct sockaddr *)&dest,addrlen) == -1)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    free(retour);
   // printf("hash fichier envoyé : ");
   // print_hash(retour);
   // printf("\nack_get envoyé !\n");
//    free(retour);
}

//thread de nettoyage
void *controle_tab_alive( void * stlist)
{
    int i;
    client_s* tmp,*next;
    stock_list* list = (stock_list*)stlist;
    while(1)
    {
        sleep(EXPIRATION);
        // nettoie la table
        for (i=0;i<list->taille;i++)
        {
            tmp = list->liste_clients;
            while(tmp !=NULL && tmp->time + 60 < time(NULL))
            {
                next = tmp->next;
                free(tmp);
                tmp = next;
            }
            while(tmp->next != NULL)
            {
                if(tmp->next->time + 60 < time(NULL))
                {
                    next = tmp->next->next;
                    free(tmp->next);
                    tmp->next= next;
                }
            
            }
        }
    }
}

int main(int argc, char **argv)
{
    int sockfd;
    int i;
    char buf[1024];
    char ip[20];
    socklen_t addrlen;
    int port;
    struct sockaddr_in my_addr;
    struct sockaddr_in client;
    pthread_t thread_keep_alive;

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
    char * hash;
    char retour;
    t.taille_actuelle = 1;
    t.taille_max=10;
    client_s *clients;
    t.nb_hash=0;
    stock_list* stlist=malloc(sizeof(stock_list)*t.taille_max);
    stock_list * tmplist;
    stlist[0].liste_clients=NULL;
    //stlist[0].hash = malloc(32);
    strncpy(stlist[0].hash,"00000000000000000000000000000000",32);
    
    if(pthread_create(&thread_keep_alive, NULL, controle_tab_alive,&stlist) == -1)
    {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    
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
        printf("\tMessage reçu\n");
        
        //const char *inet_ntop(int af, const void *src,char *dst, socklen_t size);
        if(inet_ntop(AF_INET,&client.sin_addr.s_addr,ip,20) == NULL)
        {
            perror("inet_ntop");
	        close(sockfd);
	        exit(EXIT_FAILURE);
        }

        // print the received ip
        printf("New message from %s : \n",ip);
        
        if(buf[0] == 110 || buf[0] == 112)
        {
            mess = reception_msg_put_get((unsigned char*)buf,(char)4);
            retour = traite_msg(mess,stlist,&t);
            stlist->taille = t.nb_hash;
        }
        else if(buf[0] == 114) // keep_alive
        {
            // met juste le champ du client à jour
            hash = malloc(32);
            memcpy(hash,buf + 6,32);
            port = client.sin_port; // port du client
            
            tmplist = stlist;
            clients =NULL;
            for (i=0;i<t.nb_hash;i++)
            {
                if(strncpy(hash,tmplist[i].hash,32) == 0)
                {
                    clients = tmplist->liste_clients;
                    break;
                }
            }
            if (clients == NULL)
                continue;
            // Recherche le client dans la liste de client
            while(clients->port != port)
                clients = clients->next;
            // maintenant met le client à jour
            clients->time = time(NULL);
            buf[0] = 115;
            printf("send message to \n");
            if(sendto(sockfd,buf,buf_to_s_int((unsigned char *)(buf + 1)) + 3,0,(struct sockaddr *)&client,addrlen) == -1)
            {
                perror("sendto");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }
        printf("addresse client : %s\n", inet_ntoa(client.sin_addr));

        if(retour == 110)
        {
            ack_put(sockfd,client,buf,addrlen);
        }
        if (retour == 112)
        {
            ack_get(sockfd,client,buf,addrlen,stlist);
        }
        
        sleep(1);
        // => socket , sockaddr de destination , port , 
        // , address ip destionation , taille addrlen , message à envoyer
    }
    // close the socket
    close(sockfd);
    
    if(pthread_join(thread_keep_alive,NULL) != 0)
    {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }
    return 0;
}



