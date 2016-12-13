
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "structures.h"
#include "messages.h"

#define EXPIRATION 15







infos init_all(infos infos_com)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    memset (&infos_com.my_addr, 0, sizeof (struct sockaddr_in));
    memset (&infos_com.tracker, 0, sizeof (struct sockaddr_in)); // remplit les bytes sur lesquels pointe &server avec des 0
    
    
    // init_listen  //
    
    
    if((infos_com.sockfdmy = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socked");
        exit(EXIT_FAILURE);
    }

    // init local addr structure and other params
    infos_com.my_addr.sin_family      = AF_INET;
    infos_com.my_addr.sin_port        = infos_com.port_listen;
    infos_com.my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    
    // bind addr structure with socket
    if(bind(infos_com.sockfdmy, (struct sockaddr *) &infos_com.my_addr, addrlen) == -1)
    {
      perror("bind");
      close(infos_com.sockfdmy);
      exit(EXIT_FAILURE);
    }
    printf("listening on %d\n", infos_com.port_listen);
    
    
    // init connection to tracker //
    
    if((infos_com.sockfd_tracker = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // init remote addr structure and other params
    infos_com.tracker.sin_family = AF_INET;
    infos_com.tracker.sin_port   = htons(infos_com.port_tracker);
    if(inet_pton(AF_INET,infos_com.addr_tracker,&infos_com.tracker.sin_addr) != 1)
    {
        perror("inet_pton");
        close(infos_com.sockfd_tracker);
        exit(EXIT_FAILURE);
    }
    
    infos_com.target.sin_family = AF_INET;
    infos_com.target.sin_port   = htons(0);
    infos_com.target.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(infos_com.sockfd_tracker, (struct sockaddr *) &infos_com.target, addrlen) == -1)
    {
      perror("bind sockfd_tracker");
      close(infos_com.sockfd_tracker);
      exit(EXIT_FAILURE);
    }
    
    printf("connect to tracker on port %d\n", infos_com.port_tracker);
    
    return infos_com;
}



unsigned char * communicate_tracker(infos infos_com, unsigned char * hash, char * action)
{
    unsigned char * msg_received=NULL;
    unsigned char * msg_send;
    
    while (msg_received == NULL)
    {
        // begin with comunicate with tracker
        printf("Send message to tracker\n");
        msg_send = send_msg_tracker(infos_com,hash,action);
        printf("\n");
        printf("hash dans le message : \n");
        print_hash(msg_send+6);
        printf("\n");
        usleep(100); // sleep for x ms
        msg_received = (unsigned char*)test_response_tracker(infos_com, action, msg_send);
        
        if(strcmp(action,"gtt")==0)
            action[1]='e';
        printf("ATTENTE 10 SECONDES \n");
        sleep(2);
    }
    printf("%s %s from %s port %d\n",action,hash,infos_com.addr_tracker,infos_com.port_tracker);
    free(msg_send);
    return msg_received;
}




unsigned char * send_msg_tracker(infos infos_com,unsigned char * hash, char * action)
{
    unsigned char * msg_send;
    char loopbackaddr[4];
    loopbackaddr[0] = 127; loopbackaddr[1] = 0; loopbackaddr[2] = 0; loopbackaddr[3] = 1;
    // Analyse arguments
    // possible actions : PUT, GET, KEEP_ALIVE, PRINT ? 
   // printf("%s %s to %s port %d\n", action, hash,infos_com.addr_tracker,infos_com.port_tracker);
    printf("\n");
    print_hash(hash);
    printf("\n");
    
    if (strcmp(action,"put") == 0 )
    {
        printf("put\n");
        //msg_send = create_message_put(hash,4,loopbackaddr,infos_com.port_tracker);
        msg_send = create_message_put(hash,4,loopbackaddr,infos_com.port_listen);
    }
    else if (strcmp(action,"get") == 0)
    {
        printf("get\n");
        msg_send = create_message_get(hash,4, infos_com.addr_tracker, infos_com.port_tracker); // create the message
    }
    else if (strcmp(action,"keep_alive") == 0)
    {
        printf("keep_alive\n");
        msg_send = create_message_keep_alive(hash);
    }
    else
    {
        errno = EINVAL;
        close(infos_com.sockfdmy);
        close(infos_com.sockfd_tracker);
        perror("Action :-( ");
        exit(EXIT_FAILURE);
    }
    //printf("send to %d on addr %s with sockfd : %d\n", htons(infos_com.tracker.sin_port),infos_com.addr_tracker,infos_com.sockfd_tracker);
    send_packet(msg_send, infos_com.sockfd_tracker, infos_com.tracker); // send the packet to tracker
    return msg_send;
}


// send the message
void send_packet (unsigned char* message,int sockfd,struct sockaddr_in addr)
{
    if(sendto(sockfd, message, *(short int*)(message+1)+3, 0,(struct sockaddr*) &addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Message envoyé\n");
    sleep(2);
}


 char* test_response_tracker(infos infos_com , char * action, unsigned char * msg_send)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    unsigned char* msg_recv = malloc(1024);
    
    // wait for response
    if(recvfrom(infos_com.sockfd_tracker, msg_recv, 1024, MSG_DONTWAIT,(struct sockaddr *)&infos_com.tracker,&addrlen) == -1)
    {
        if ( errno == EAGAIN || errno == EWOULDBLOCK) // there were no packet
        {
            printf("Aucune réponse reçue\n");
            return NULL;
        }
        perror("recv");
        close(infos_com.sockfdmy);
        close(infos_com.sockfd_tracker);
        exit(EXIT_FAILURE);
    }
    printf("\t\t\tputain %d\n",msg_recv[43]);
            printf("\n hash recu : ");
        print_hash(msg_recv+6);
        printf("\n");

    /* if(strcmp(action,"get") == 0)
         action[1] = 't';*/
    /*
    printf("\n\nune réponse à été reçue\n\n");
    printf("\n");
    print_hash(msg_recv + 3 + 3);
    printf("\n");
    */
    // test of message received
    if (test_rep(action,msg_send,msg_recv) == 0) 
        return ( char*)msg_recv;
    else
    {
        free(msg_recv);
        return NULL;
    }
}

// test if right message was received
int test_rep( char * action, unsigned char * msg_send, unsigned char * msg_recv)
{
    short int msg_send_length = buf_to_s_int(msg_send +1) + 3; // length in message + 3 (type + length)
    int tmp;
    // communication with clients
    if(strcmp(action,"gtt") == 0) // message get client
    {
        if (msg_recv[0] != 101) // test if good type of message
            return -1;
        msg_recv[0] = 100; // change type to compare with msg_send
        msg_recv    = s_int_to_buf( msg_recv, buf_to_s_int(msg_send + 1), 1);
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
        return 0;
    }
    else if(strcmp(action,"list") == 0) // message list client
    {
        if (msg_recv[0] != 103) // test if good type of message
            return -1;
        msg_recv[0] = 102; // change type to compare with msg_send
        msg_recv    = s_int_to_buf(msg_recv, buf_to_s_int(msg_send + 1), 1); 
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
        return 0;
    }
    
    // communication with tracker
    else if(strcmp(action,"put") == 0) // message put tracker
    {
        if (msg_recv[0] != 111) // test if good type of message
            return -1;
        msg_recv[0] = 110; // change type to compare with msg_send
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
        return 0;
    }
    else if(strcmp(action,"get") == 0) // message get tracker
    {
        if (msg_recv[0] != 113) // test if good type of message
            return -1;
        
        msg_recv[0] = 112; // change type to compare with msg_send
        tmp = buf_to_s_int(msg_recv + 1);
        msg_recv    = s_int_to_buf(msg_recv, buf_to_s_int(msg_send + 1), 1);
        if(u_strncmp(msg_send,msg_recv,3+3+32) != 0) // strings different
            return -1;
        msg_recv    = s_int_to_buf(msg_recv, tmp, 1);
        printf("Message conforme\n");
        return 0;
    }
    if(strcmp(action,"keep_alive") == 0) // message keep_alive tracker
    {
        if (msg_recv[0] != 115) // test if good type of message
            return -1;
        msg_recv[0] = 114; // change type to compare with msg_send
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
        return 0;
    }
    
    return -1;
}











// analyze of message ACKput
infos analyze_ack_get(unsigned char*message, infos infos_com)
{
    client_s *liste_clients = NULL;
    client_s *new_client;
    int length = buf_to_s_int(message +1) ;
    int index = 3 + 3 + 32;
    int i;
    printf("length %d\n",length);
    while (index < length)
    {
        printf(" le type %d index %d length %d\n",message[index],index,length);
        new_client = malloc(sizeof(client_s));
        index ++; // jump type
        new_client->length = buf_to_s_int(message+index) ; // type
        index +=2;
        new_client->port = buf_to_s_int(message +index);
        index +=2;
        for(i = 0; i < new_client->length -2 ;i++)
            new_client->address_ip[i] = message[index+i];
        index+=new_client->length -2;
        new_client->next = liste_clients;
        liste_clients = new_client;
  //      printf("IP : %d.%d.%d.%d port %d\n",new_client->address_ip[0],new_client->address_ip[1],new_client->address_ip[2],new_client->address_ip[3], new_client->port);
    }
    infos_com.liste_clients = liste_clients;
    return infos_com;
}

// analyze messages from tracker
infos analyze_messages_tracker(unsigned char *message,infos infos_com)
{
    client_s *liste,*tmp;
    if(message[0] == 110) // message ACKPUT
        ; // nothing to do ( test was made in test_rep)
    else if(message[0] == 115)
        ; // nothing to do ( test was made in test_rep)
    else if(message[0] == 112) // message ACKGET
    {
        infos_com = analyze_ack_get(message,infos_com);
        // print every client
        liste = infos_com.liste_clients;
        while(liste != NULL)
        {
            printf("IP : %d.%d.%d.%d port %d\n",liste->address_ip[0],liste->address_ip[1],liste->address_ip[2],liste->address_ip[3], liste->port);
            tmp = liste;
            liste=liste->next;
            free(tmp);
        }
        printf("j'ai fini \n");
    }
    else
    {
        printf("wrong message");
        exit(EXIT_FAILURE);
    }
    
    
    return infos_com;
}

void *send_keep_alive( void * info)
{
    infos* infos_com = (infos *) info;
    char* msg_received = NULL;
    unsigned char* msg_send;
    
    while(1)
    {
        sleep(EXPIRATION);
        msg_received=NULL;
        // send keep alive to tracker
        while(msg_received == NULL)
        {
            printf("Send keep_alive to tracker\n");
            msg_send = send_msg_tracker(*infos_com,infos_com->hash,"keep_alive");
            usleep(100);
            msg_received = test_response_tracker(*infos_com, "keep_alive", msg_send);
        }
    }
    
}


int main(int argc, char **argv)
{
    infos infos_com;
    short int port_send,port_listen;
    char * action;
    unsigned char * hash;
    unsigned char * message;
    pthread_t thread_keep_alive;
    
    // check the number of args on command line
    if(argc != 6)
    {
      printf("USAGE: %s @server port_send port_listen action hash \n", argv[0]);
      exit(-1);
    }
    
    
    // set parameters
    port_send           = atoi(argv[2]);
    port_listen         = atoi(argv[3]);
    action              = argv[4];
    hash                = (unsigned char *)argv[5];
    infos_com.addr_tracker = argv[1];
    infos_com.port_tracker = port_send;
    infos_com.port_listen = port_listen;
    infos_com.hash = hash;
    infos_com           = init_all(infos_com);
    
    // communicate with tracker
    message = communicate_tracker(infos_com,hash,action);
    printf("\t COMMUNICATE WITH TRACKER END\n");
    
    // if put then make thread which send keep_alive to tracker
    if(strcmp(action,"put") == 0 )
    {
        if(pthread_create(&thread_keep_alive, NULL, send_keep_alive,&infos_com) == -1)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    if(strcmp(action,"get")==0)
    {
        analyze_messages_tracker(message,infos_com);
    }
    
    
    
    // END
    if(strcmp(action,"put")==0)
    {
        if(pthread_cancel(thread_keep_alive) == -1)
        {
            perror("pthread_cancel");
            exit(EXIT_FAILURE);
        }
        
        if(pthread_join(thread_keep_alive,NULL) != 0)
        {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }
    free(message);
    return 0;
}
