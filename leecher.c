
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "structures.h"
#include "messages.h"
#include "sha256.h"

#define EXPIRATION 5
#define chunk_size 10
#define FRAGMENT_TAILLE 2



// init listen port
infos init_connexion_target(infos infos_com)
{
    //socklen_t addrlen = sizeof(struct sockaddr_in);
    memset (&infos_com.target, 0, sizeof (struct sockaddr_in));
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if((infos_com.sockfdtarget = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socked");
        exit(EXIT_FAILURE);
    }

    // init local addr structure and other params
    infos_com.target.sin_family      = AF_INET;
    infos_com.target.sin_port        = infos_com.port_dest;
    infos_com.target.sin_addr.s_addr = htonl(INADDR_ANY);

    
    infos_com.my_addr.sin_family      = AF_INET;
    infos_com.my_addr.sin_port        = htons(0);
    infos_com.my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // bind addr structure with socket
    if(bind(infos_com.sockfdtarget, (struct sockaddr *) &infos_com.my_addr, addrlen) == -1)
    {
      perror("bind sockfdtarget");
      close(infos_com.sockfdtarget);
      exit(EXIT_FAILURE);
    }
    
    printf("listening on %d\n", infos_com.port_dest);
    return infos_com;
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
    usleep(1000);
    printf("Message envoyé\n");
}


/*
infos listen_target(short int port_listen, infos infos_com,struct sockaddr s_addr)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);    
    
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
  

*/


unsigned char * rep_list(unsigned char * msg,infos infos_com)
{
    unsigned int length_hash = (unsigned int)buf_to_s_int(msg + 4);
    unsigned char * hash = malloc(length_hash);
    unsigned int length_msg;
    int i,pos=0;
    if((hash=(unsigned char*)strncpy((char*)hash,(char*)msg + 6, length_hash)) == NULL)
    {
        perror("strncpy");
        exit(EXIT_FAILURE);
    }
    // fait le message de réponse
    length_msg = 1 + 2 + 1 + 2 + 32 + infos_com.nb_chunks*(5 + 32);
    unsigned char * message = malloc(length_msg); 
    message[0] =  103;
    message = us_int_to_buf(message, length_msg, 1);
    
    // entre le hash
    message[3] = 50;
    message = us_int_to_buf(message,32,4);
    if(strncpy((char*)message + 6, (char*)msg + 6, length_hash) == NULL)
    {
        perror("strncpy");
        exit(EXIT_FAILURE);
    }
    
    pos += 3 + 3 + 32;
    // met les chunks dans le message
    for(i=0 ; i < infos_com.nb_chunks; i++)
    {
        message[pos++] = 51;
        us_int_to_buf(message,32 + 2,pos);
        pos +=2;
        if(strncpy((char*)message + pos,(char*)infos_com.tab_chunks[i],32) == NULL)
        {
            perror("strncpy");
            exit(EXIT_FAILURE);
        }
        pos += 32;
        us_int_to_buf(message,i,pos);
        pos +=2;
    }
    return message;
}




// test if right message was received
int test_rep( char * action, unsigned char * msg_send, unsigned char * msg_recv)
{
    int test = 0;
    short int msg_send_length = buf_to_s_int(msg_send +1) + 3; // length in message + 3 (type + length)
    short int tmp_length;
    // communication with clients
    if(strcmp(action,"get") == 0) // message get client
    {
        if (msg_recv[0] != 101) // test if good type of message
            return -1;
        msg_recv[0] = 100; // change type to compare with msg_send
        tmp_length = buf_to_s_int(msg_recv +1);
        msg_recv    = s_int_to_buf( msg_recv, buf_to_s_int(msg_send + 1), 1);
        if((test=u_strncmp(msg_send,msg_recv,msg_send_length)) != 0) // strings different
        {
            printf("test %d\n",test);
            return -1;
        }
        msg_recv    = s_int_to_buf(msg_recv, tmp_length, 1);
        return 0;
    }
    else if(strcmp(action,"list") == 0) // message list client
    {
        if (msg_recv[0] != 103) // test if good type of message
            return -1;
        msg_recv[0] = 102; // change type to compare with msg_send
        tmp_length = buf_to_s_int(msg_recv +1);
        msg_recv    = s_int_to_buf(msg_recv, buf_to_s_int(msg_send + 1), 1);
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
        msg_recv    = s_int_to_buf(msg_recv, tmp_length, 1);
        return 0;
    }
    
    return -1;
}


unsigned char * seek_response(unsigned char * message, int sockfd,struct sockaddr_in addr,char * action)
{
    unsigned char *msg_received = malloc(1024);
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if(recvfrom(sockfd, msg_received, 1024, MSG_DONTWAIT,(struct sockaddr *)&addr,&addrlen) == -1)
    {
        if ( errno == EAGAIN || errno == EWOULDBLOCK) // there were no packet
        {
            printf("Aucune réponse reçue\n");
            free(msg_received);
            return NULL;
        }
        perror("recv");
        exit(EXIT_FAILURE);
    }
    
    // test if type of response is good
    if(test_rep(action,message,msg_received)==-1) // wrong hash
    {
        printf("Wrong msg %d\n",msg_received[0]);
        free(msg_received);
        return NULL;
    }
    else
        return msg_received;
}


table_chunks save_chunks(unsigned char * message)
{
    table_chunks table;
    int i;
    int pos                  = 3 + 3 + 32; // saute type + length du message + le hash
    short int length_message =  buf_to_s_int(message + 1);
    table.nb_chunks          = (length_message - 32 + 3)/(1+2+2 +32); // 1 + 2 + 2 + 32 -> taille de hash_chunk
    table.index              = 0;
    table.tab_chunks         = malloc( table.nb_chunks * sizeof(char*));
    table.tab_index_chunks         = malloc( table.nb_chunks * sizeof(int));
    // remplit la table
    for(i=0;i< table.nb_chunks; i++)
    {
        pos += 3;  // saute type + length
        table.tab_chunks[i] = malloc(32);
        if(strncpy((char*)table.tab_chunks[i], (char *)message + pos,32) == NULL)
        {
            perror("strncpy");
            exit(EXIT_FAILURE);
        }
        pos += 32;
        table.tab_index_chunks[i] = buf_to_s_int(message + pos);
        pos += 2;
    }
    return table;
}


int write_chunk(unsigned char * response,int fd,short int fragment)
{
    short int size_fragment = buf_to_s_int( response + 3 + 3 + 32 + 3 + 32 +2 + 1)- 4;
    short int index = buf_to_s_int( response + 3 + 3 + 32 + 3 + 32 +2 + 1 +2);
    
    // check if good fragment
    if( fragment != index)
        return -1;
    
    // write chunk into file
    
    if(write(fd, response + 3 + 3 + 32 + 3 + 32 + 2 + 1 + 6, size_fragment) == -1)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
    return 0;
}

// argv[1] : port, argv[2] : file
int main(int argc, char **argv)
{
    // check the number of args on command line
    if(argc != 5)
    {
      printf("USAGE: %s address port hash_file dest_file\n", argv[0]);
      exit(-1);
    }
    unsigned char * message;
    unsigned char * response = NULL;
    unsigned char * request;
    int fd,i;
    table_chunks table ;
    infos infos_com;
    short int index;
    infos_com.addr_dest = argv[1];
    infos_com.port_dest = htons(atoi(argv[2]));
    infos_com.hash      =hash_to_char( argv[3]);
    infos_com.dest_file = (unsigned char *)argv[4];
    
    
    // first send message LIST to client
    infos_com           = init_connexion_target(infos_com);
    message             = create_message_list(infos_com.hash);

    while(response == NULL) // until right message is received
    {
        send_packet(message,infos_com.sockfdtarget,infos_com.target);
        usleep(20); // wait for response
        response = seek_response(message,infos_com.sockfdtarget, infos_com.my_addr,"list");
    }
    // analyze function
    table = save_chunks(response);
    free(response);
    free(message);
    // create file
    if( (fd = open(argv[4],O_RDWR|O_CREAT,0666)) == -1 )
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    // ask chunks
    while(table.index < table.nb_chunks)
    {
        printf("ask for chunk %d\n",table.index);
        index = 0;
        while (index != table.tab_index_chunks[table.index])
        {
            printf("ask for fragment %d \n",index);
            response = NULL;
            // make message get
            request = create_message_get_peer(infos_com.hash,table.tab_chunks[table.index],index );
            while(response == NULL)
            {
                send_packet(request, infos_com.sockfdtarget,infos_com.target);
                usleep(20);
                response = seek_response(request,infos_com.sockfdtarget,infos_com.target,"get");
            }
            
            if(write_chunk(response,fd,index) == -1)
            {
                printf("Mauvais chunk\n");
                free(response);
                response = NULL;
            }
            
            free(request);
            printf("fragment %d reçu\n",index);
            if(response != NULL)
                index ++;
            free(response);
            
        }
        table.index ++;
        printf("end ask\n");
    }
    
    if(close(fd) == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    for(i=0;i<table.nb_chunks;i++)
        free(table.tab_chunks[i]);
    free(table.tab_chunks);
    free(table.tab_index_chunks);
    free(infos_com.hash);
    close(infos_com.sockfdtarget);
    printf("la fin =)\n");
    return 0;
}
