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
  message.client.type = buf[38];               // Type du client
  message.client.length = buf_to_s_int(buf+39); // Taille du client
  message.client.port = buf_to_s_int(buf+41);                  // Port du client 
  memcpy(buf+43,message.client.adresse_ip,length_address);  // Addresse du client
  return message;
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
    message_t mess;
    
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
						mess = reception_msg_put((unsigned char*)buf,(char)4);
						printf("valeur du type : %c",mess.type);
									
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
