/**
 * @file client-tcp.c
 * @author Julien Montavont
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * Simple program that creates an IPv4 TCP socket and tries to connect
 * to a remote host before sending a string to this host. The string,
 * IPv4 addr and port number of the remote host are passed as command
 * line parameters as follow:
 * ./pg_name IPv4_addr port_number string
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>


// structures du client

// structures vers le tracker 
typedef struct s_client
{
    char type;
    short int length; // 6 pour IPV4, 18 pour IPV6
    short int port;
    char adresse_ip [16];
} client_s;


typedef struct s_hash_f
{
    char type;
    short int length; 
    char hash[32];
} hash_f;

typedef struct s_message_t
{
    char type; // type du message
    short int length; // taille du message en octets
    hash_f  hash;
    client_s client;
} message_t;

// structures pour message venant du tracker
typedef struct s_liste_clients
{
    char type;
    short int length;
    client_s* liste;    
} liste_c;


// structures supplémentaires pour communication entre pairs 
typedef struct s_hash_c
{
    char type;
    short int length;
    char hash[32];
    short int index;
} hash_c;

// message GET
typedef struct s_message_get
{
    char type;
    short int length;
    hash_f file_h;
    hash_c chunk_h;
} message_get;
// message REP_GET
typedef struct s_message_rep_get
{
    char type;
    short int length;
    hash_f file_h;
    hash_c chunk_h;
    char * chunk;
} message_rep_get;

// message REP_LIST
typedef struct s_message_rep_list
{
    char type;
    short int length;
    hash_f file_h;
    hash_c * chunk_list;
} message_rep_list;

typedef struct infos_s
{
    int sockfdtarget; // fd qui sert à faire les sockets
    int sockfdmy;
    struct sockaddr_in target;
    struct sockaddr_in my_addr;
    socklen_t addrlen_s;
    socklen_t addrlen_c;
    char * addr_dest;
    unsigned short int port_dest;
} infos;

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



// function which put i (int) into buf begin on begin
unsigned char * int_to_buf(unsigned char * buf,int i,int begin)
{
	buf[begin]=i & 0xFF;
	buf[begin+1]=(i>>8) & 0xFF;
	buf[begin+2]=(i>>16)& 0xFF;
	buf[begin+3]=(i>>24) & 0xFF;
	return buf;	
}


unsigned char * s_int_to_buf(unsigned char * buf,short int i,int begin)
{
	buf[begin]=i & 0xFF;
	buf[begin+1]=(i>>8) & 0xFF;
	return buf;	
}
unsigned char * us_int_to_buf(unsigned char * buf,unsigned short int i,int begin)
{
	buf[begin]=i & 0xFF;
	buf[begin+1]=(i>>8) & 0xFF;
	return buf;	
}

short int buf_to_s_int ( unsigned char * buf)
{
    return *(short int*) buf;
}
int buf_to_int ( unsigned char * buf)
{
    return *(int*) buf;
}

// strlen for unsigned string
int u_strlen ( unsigned char * string)
{
    int length = 0;
    while(string[length] != '\0')
        length ++ ;
    return length;
}


// length hash fichier : 32
// length hash chunk : 32
// length client : 1+2+2+4 = 9 or 1+2+2+16 = 21


// message GET between 2 clients
unsigned char *  create_message_get_peer(unsigned char * hash_file,unsigned char * hash_chunk)
{
    unsigned char *buffer=malloc(74); // ou 74 ? 
    short int length;
    // create the message
    buffer[0] =100 ; // set type
    length    = 32 + 32;
    buffer    = s_int_to_buf(buffer,length, 1);
    // hash file
    buffer[3] = 50;
    buffer    = s_int_to_buf(buffer,32,4);
    memcpy(buffer+6,hash_file,32);
    // hash chunk
    buffer[38] = 51;
    buffer = s_int_to_buf(buffer,32,39);
    memcpy(buffer+41,hash_chunk,32);
    return buffer;
}



// message REP_GET
// chunk doit finir par un \0
unsigned char * create_message_rep_get(unsigned char * hash_file,unsigned char * hash_chunk,char * chunk,short int index, short int max_index)
{
    short int length_chunk = u_strlen(hash_chunk) ;
    unsigned char *buffer=malloc(74+length_chunk+ 7); // ou 74 ? 
    short int length;
    // create the message
    buffer[0] = 101 ; // set type
    length    = 32 + 32;
    buffer    = s_int_to_buf(buffer,length, 1);
    // hash file
    buffer[3] = 50;
    buffer    = s_int_to_buf(buffer,32,4);
    memcpy(buffer+6,hash_file,32);
    // hash chunk
    buffer[38] = 51;
    buffer     = s_int_to_buf(buffer,32,39);
    memcpy(buffer+41,hash_chunk,32);
    // chunk
    buffer[74]  =60;
    buffer      = s_int_to_buf(buffer,length,75);
    buffer      = s_int_to_buf(buffer,index,77);
    buffer      = s_int_to_buf(buffer,max_index,79);
    memcpy(buffer+81,chunk,length);
    return buffer;
}

// message LIST
unsigned char * create_message_list(unsigned char * hash_file)
{
    unsigned char *buffer=malloc(38); // ou 74 ? 
    short int length;
    // create the message
    buffer[0] = 102 ; // set type
    length    = 32;
    buffer    = s_int_to_buf(buffer,length, 1);
    // hash file
    buffer[3] = 50;
    buffer    = s_int_to_buf(buffer,32,4);
    memcpy(buffer+6,hash_file,32);
    return buffer;
}

// message REP_LIST
unsigned char * create_message_list(unsigned char * hash_file);


// message PUT
unsigned char * create_message_put(unsigned char * hash_file,char IP_TYPE, char * address,unsigned short int port)
{
    short int length_address;
    unsigned char *buffer; // ou 74 ? 
    short int length;
    
    if (IP_TYPE == 4)
        length_address = 4;
    else
        length_address = 16;
    // create the message
    buffer=malloc(38);
    buffer[0] = 102 ; // set type
    length    = 32 + length_address + 5;
    buffer    = s_int_to_buf(buffer,length, 1);
    
    // hash file
    buffer[3] = 50;
    buffer    = s_int_to_buf(buffer,32,4);
    memcpy(buffer+6,hash_file,32);
    
    // client
    buffer[39] = 55;
    buffer     = s_int_to_buf(buffer,length_address + 2, 40);
    buffer     = us_int_to_buf(buffer,port, 42);
    memcpy(buffer+44,address,length_address);
    return buffer;
}

// message GET
unsigned char * create_message_get(unsigned char * hash_file,char IP_TYPE, char * address,unsigned short int port)
{
    unsigned char * buffer = create_message_put( hash_file, IP_TYPE, address, port);
    buffer[0] = 112;
    return buffer;
}

// message KEEP_ALIVE
unsigned char * create_message_keep_alive(unsigned char * hash_file)
{
    unsigned char *buffer = create_message_list( hash_file);
    buffer[0] = 114;
    return buffer;
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
    unsigned char * message;
    // Analyse arguments
    // possible actions : PUT, GET, KEEP_ALIVE, PRINT ? 
    if (strcmp(action,"put") == 0 )
    {
        printf("put\n");
        message = create_message_put(hash,6,infos_com.addr_dest,infos_com.port_dest);
    }
    else if (strcmp(action,"get") == 0)
    {
        printf("get\n");
        message = create_message_get(hash, 6, infos_com.addr_dest, infos_com.port_dest); // create the message
    }
    else if (strcmp(action,"keep_alive") == 0)
    {
        printf("keep_alive\n");
        message = create_message_keep_alive(hash);
    }
    else
    {
        errno = EINVAL;
        close(infos_com.sockfdmy);
        close(infos_com.sockfdtarget);
        perror("Action ");
        exit(EXIT_FAILURE);
    }
    send_packet(infos_com,message); // send the packet to tracker
    return message;
}
int u_strncmp(unsigned char * string1,unsigned char * string2, int n)
{
    int i;
    for(i=0;i<n;i++)
    {
        if(string1[i]!=string2[i])
            return i;
    }
    return 0;
}
int test_response_tracker(infos infos_com , char * action, unsigned char * message)
{
    struct sockaddr tracker;
    socklen_t addrlen;
    unsigned char buf[1024];
    usleep(100); // sleep for x ms
    // wait for response
    if(recvfrom(infos_com.sockfdtarget, buf, 1024, MSG_DONTWAIT,&tracker,&addrlen) == -1)
    {
        if ( errno == EAGAIN || errno == EWOULDBLOCK) // there were no packet
            return -1;
        perror("recv");
        close(infos_com.sockfdmy);
        close(infos_com.sockfdtarget);
        exit(EXIT_FAILURE);
    }
    
    // test of message received
    
    if (strcmp(action,"put") == 0 )
    {
        if ( buf[0] != 110)
            return -1;
        buf[0]=110;
        if(u_strncmp(buf,message,addrlen)!=0) // strings !=
            return -1;
    }
    else if (strcmp(action,"get") == 0)
    {
        printf("faut le faire\n");
    }
    else if (strcmp(action,"keep_alive") == 0)
    {
        if ( buf[0] != 114)
            return -1;
        buf[0]=115;
        if(u_strncmp(buf,message,addrlen)!=0) // strings !=
            return -1;
    }
    else
        return -1;
    return 0;
}
 
unsigned char * communicate_tracker(infos infos_com, unsigned char * hash, char * action)
{
    int msg_received = -1;
    unsigned char * message;
    
    while (msg_received == -1)
    {
        // begin with comunicate with tracker
        printf("Send message to tracker\n");
        message = send_msg_tracker(infos_com,hash,action);
        msg_received = test_response_tracker(infos_com, action, message);
    }
    return message;
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
    
    // communicate with tracker
    if(strcmp(action,"put") == 0 || strcmp(action,"get") == 0 || strcmp(action,"keep_alive") == 0 ) 
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
