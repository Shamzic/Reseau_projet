
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#include "structures.h"
#include "messages.h"

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
    return infos_com;
}






/*
void send_msg(infos infos_com,char * hash, char * action)
{
    char * message;
    // Analyse arguments
    // possible actions : GET, LIST, PUT, GET, KEEP_ALIVE, PRINT ? 
    if (strcmp(action,"get") == 0 )
    {
        printf("put\n");
        message = 
        
    }
    else if (strcmp(action,"list") == 0)
    {
        printf("list\n");
    }
    else if (strcmp(action,"put") == 0)
    {
        printf("put\n");
    }
    else if (strcmp(action,"keep_alive") == 0)
    {
        printf("keep_alive\n");
    }
    else
    {
        errno = EINVAL;
        close(infos_com.sockfdmy);
        close(infos_com.sockfdtarget);
        perror("Action ");
        exit(EXIT_FAILURE);
    }
}
*/

/*    //
    
    if(sendto(infos_com.sockfdtarget,infos_com.message, strlen(argv[3]),0,(struct sockaddr * ) &server,addrlen) == -1)
    {
        perror("sendto");
        close(sockfd);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

*/

// send the message
void send_packet (infos infos_com, unsigned char* message)
{
    if(sendto(infos_com.sockfdtarget, message, *(int*)(message+1), 0, NULL, 0) == -1)
    {
        perror("sendto");
        close(infos_com.sockfdtarget);
        close(infos_com.sockfdmy);
        exit(EXIT_FAILURE);
    }
}



unsigned char * send_msg_tracker(infos infos_com,unsigned char * hash, char * action)
{
    unsigned char * msg_send;
    // Analyse arguments
    // possible actions : PUT, GET, KEEP_ALIVE, PRINT ? 
    if (strcmp(action,"put") == 0 )
    {
        printf("put\n");
        msg_send = create_message_put(hash,6,infos_com.addr_dest,infos_com.port_dest);
    }
    else if (strcmp(action,"get") == 0)
    {
        printf("get\n");
        msg_send = create_message_get(hash, 6, infos_com.addr_dest, infos_com.port_dest); // create the message
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
        close(infos_com.sockfdtarget);
        perror("Action ");
        exit(EXIT_FAILURE);
    }
    send_packet(infos_com,msg_send); // send the packet to tracker
    return msg_send;
}

int test_response_tracker(infos infos_com , char * action, unsigned char * msg_send)
{
    struct sockaddr tracker;
    socklen_t addrlen;
    unsigned char msg_recv[1024];
    usleep(100); // sleep for x ms
    // wait for response
    if(recvfrom(infos_com.sockfdtarget, msg_recv, 1024, MSG_DONTWAIT,&tracker,&addrlen) == -1)
    {
        if ( errno == EAGAIN || errno == EWOULDBLOCK) // there were no packet
            return -1;
        perror("recv");
        close(infos_com.sockfdmy);
        close(infos_com.sockfdtarget);
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
    short int msg_send_length = buf_to_s_int(msg_send +1) +3; // length in message + 3 (type + length)
    
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

unsigned char * communicate_tracker(infos infos_com, unsigned char * hash, char * action)
{
    int msg_received = -1;
    unsigned char * msg_send;
    
    while (msg_received == -1)
    {
        // begin with comunicate with tracker
        printf("Send message to tracker\n");
        msg_send = send_msg_tracker(infos_com,hash,action);
        msg_received = test_response_tracker(infos_com, action, msg_send);
    }
    return msg_send;
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
    infos_com.addr_dest = argv[1];
    infos_com.port_dest = port_send;
    infos_com           = init_listen(port_listen, infos_com);
    infos_com           = init_connexion(infos_com.addr_dest, port_send, infos_com);

    // communicate with tracker
    message = communicate_tracker(infos_com,hash,action);
    
    // communicate with tracker or with clients
    
    // keep_alive (put et get fait dans communicate_tracker)
    if(strcmp(action,"keep_alive") == 0 ) 
    {
        message = communicate_tracker(infos_com,hash,action);
    }
    else // communication client-client : GET, REP_GET, LIST, REP_LISTE
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
    // communicate with client
    else if(strcmp(action,"get") == 0 || strcmp(action,"list") == 0 ) 
    {
        
        
    }
    
    // close the socket

    return 0;
}
