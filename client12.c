
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

#define EXPIRATION 60







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
    infos_com.tracker.sin_port   = infos_com.port_tracker;

    // get addr from command line and convert it
    if(inet_pton(AF_INET,infos_com.addr_tracker,&infos_com.tracker.sin_addr) != 1)
    {
        perror("inet_pton");
        close(infos_com.sockfd_tracker);
        exit(EXIT_FAILURE);
    }
    // init connexion
    
    if((infos_com.sockfd_tracker = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socked");
        exit(EXIT_FAILURE);
    }
    return infos_com;
}



unsigned char * communicate_tracker(infos infos_com, unsigned char * hash, char * action)
{
    int msg_received = -1;
    unsigned char * msg_send;
    
    while (msg_received == -1)
    {
        // begin with comunicate with tracker
        printf("Send message to tracker\n");
        msg_send = send_msg_tracker(infos_com,hash,action);
        usleep(100); // sleep for x ms
        msg_received = test_response_tracker(infos_com, action, msg_send);
    }
    if(strcmp(action,"gtt")==0)
        action[1]='e';
    printf("%s %s from %s port %d\n",action,hash,infos_com.addr_tracker,infos_com.port_tracker);
    return msg_send;
}




unsigned char * send_msg_tracker(infos infos_com,unsigned char * hash, char * action)
{
    unsigned char * msg_send;
    // Analyse arguments
    // possible actions : PUT, GET, KEEP_ALIVE, PRINT ? 
    printf("%s %s to %s port %d\n", action, hash,infos_com.addr_tracker,infos_com.port_tracker);
    if (strcmp(action,"put") == 0 )
    {
        printf("put\n");
        msg_send = create_message_put(hash,6,infos_com.addr_tracker,infos_com.port_tracker);
    }
    else if (strcmp(action,"get") == 0)
    {
        printf("get\n");
        msg_send = create_message_get(hash, 6, infos_com.addr_tracker, infos_com.port_tracker); // create the message
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
        perror("Action ");
        exit(EXIT_FAILURE);
    }
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
}


int test_response_tracker(infos infos_com , char * action, unsigned char * msg_send)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    unsigned char msg_recv[1024];
    
    // wait for response
    if(recvfrom(infos_com.sockfd_tracker, msg_recv, 1024, MSG_DONTWAIT,(struct sockaddr *)&infos_com.tracker,&addrlen) == -1)
    {
        if ( errno == EAGAIN || errno == EWOULDBLOCK) // there were no packet
        {
            printf("Aucune réponse reçue\n");
            return -1;
        }
        perror("recv");
        close(infos_com.sockfdmy);
        close(infos_com.sockfd_tracker);
        exit(EXIT_FAILURE);
    }
    if(strcmp(action,"get") == 0)
        action[1] = 't';
    
    // test of message received
    return test_rep( action, msg_send, msg_recv);
}

// test if right message was received
int test_rep( char * action, unsigned char * msg_send, unsigned char * msg_recv)
{
    short int msg_send_length = buf_to_s_int(msg_send +1) + 3; // length in message + 3 (type + length)
    
    // communication with clients
    if(strcmp(action,"gtt") == 0) // message get tracker
    {
        if (msg_recv[0] != 101) // test if good type of message
            return -1;
        msg_recv[0] = 100; // change type to compare with msg_send
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
    }
    else if(strcmp(action,"list") == 0) // message get tracker
    {
        if (msg_recv[0] != 103) // test if good type of message
            return -1;
        msg_recv[0] = 102; // change type to compare with msg_send
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
    }
    
    // communication with tracker
    else if(strcmp(action,"put") == 0) // message get tracker
    {
        if (msg_recv[0] != 111) // test if good type of message
            return -1;
        msg_recv[0] = 110; // change type to compare with msg_send
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
    }
    else if(strcmp(action,"get") == 0) // message get tracker
    {
        if (msg_recv[0] != 113) // test if good type of message
            return -1;
        msg_recv[0] = 112; // change type to compare with msg_send
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
    }
    if(strcmp(action,"keep_alive") == 0) // message get tracker
    {
        if (msg_recv[0] != 115) // test if good type of message
            return -1;
        msg_recv[0] = 114; // change type to compare with msg_send
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
    }
    
    return 0;
}





/*

// open listen port on port port_listen
infos init_listen(short int port_listen, infos infos_com)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);    
    memset (&infos_com.my_addr, 0, sizeof (struct sockaddr_in));
    
    if((infos_com.sockfdmy = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socked");
        exit(EXIT_FAILURE);
    }

    // init local addr structure and other params
    infos_com.my_addr.sin_family      = AF_INET;
    infos_com.my_addr.sin_port        = port_listen;
    infos_com.my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    
    // bind addr structure with socket
    if(bind(infos_com.sockfdmy, (struct sockaddr *) &infos_com.my_addr, addrlen) == -1)
    {
      perror("bind");
      close(infos_com.sockfdmy);
      exit(EXIT_FAILURE);
    }
    printf("listening on %d\n", port_listen);
    return infos_com;
}
    
// open connexion to address addr_connect port port_send
infos init_connexion(char * addr_connect, short int port_send,infos infos_com)
{    
    memset (&infos_com.target, 0, sizeof (struct sockaddr_in)); // remplit les bytes sur lesquels pointe &server avec des 0
    
    
    if((infos_com.sockfdtarget = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // init remote addr structure and other params
    infos_com.target.sin_family = AF_INET;
    infos_com.target.sin_port   = port_send;

    // get addr from command line and convert it
    if(inet_pton(AF_INET,addr_connect,&infos_com.target.sin_addr) != 1)
    {
        perror("inet_pton");
        close(infos_com.sockfdtarget);
        exit(EXIT_FAILURE);
    }
    // init connexion
    
    if((sockfd_tracker = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socked");
        exit(EXIT_FAILURE);
    }
    infos_com.sockfdtracker = sockfdtracker;
    
    
    return infos_com;
}

*/













// analyze of message ACKput
infos analyze_ack_get(unsigned char*message, infos infos_com)
{
    client_s *liste_clients = NULL;
    client_s *new_client;
    int length = strlen( (char*) message);
    int index = 3;
    int i;
    while (index < length)
    {
        new_client = malloc(sizeof(client_s));
        index ++; // jump type
        new_client->length = buf_to_s_int(message+index) ; // type
        index +=2;
        new_client->port = buf_to_s_int(message +index);
        index +=2;
        for(i = 0; i < new_client->length ;i++)
            new_client->address_ip[i] = message[index+i];
        i+=new_client->length;
        new_client->next = liste_clients;
        liste_clients = new_client;
    }
    infos_com.liste_clients = liste_clients;
    return infos_com;
}

// analyze messages from tracker
infos analyze_messages_tracker(unsigned char *message,infos infos_com)
{
    client_s *liste;
    if(message[0] == 111) // message ACKPUT
        ; // nothing to do ( test was made in test_rep)
    else if(message[0] == 115)
        ; // nothing to do ( test was made in test_rep)
    else if(message[0] == 113) // message ACKGET
    {
        infos_com = analyze_ack_get(message,infos_com);
        // print every client
        liste = infos_com.liste_clients;
        while(liste != NULL)
        {
            printf("IP : %s port %d",liste->address_ip, liste->port);
        }
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
    int msg_received = -1;
    unsigned char* msg_send;
    
    while(1)
    {
        sleep(EXPIRATION);
        // send keep alive to tracker
        while(msg_received == -1)
        {
            printf("Send keep_alive to tracker\n");
            msg_send = send_msg_tracker(*infos_com,infos_com->hash,"keep_alive");
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
    unsigned char buf[1024];
    struct sockaddr enter_co;
    socklen_t addrlen;
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
    
    // if put then make thread which send keep_alive to tracker
    if(strcmp(action,"put") == 0 )
    {
        if(pthread_create(&thread_keep_alive, NULL, send_keep_alive,&infos_com) == -1)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    
    
    
    while(1) // communication client-client : GET, REP_GET, LIST, REP_LISTE
    {
        // wait for messages
        if(recvfrom(infos_com.sockfdmy, buf, 1024, 0,&enter_co,&addrlen) == -1)
        {
            perror("recv");
            close(infos_com.sockfdmy);
            close(infos_com.sockfdtarget);
            exit(EXIT_FAILURE);
        }
    }
    printf("%s\n",message);
    
    
    // END
    
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

    return 0;
}
