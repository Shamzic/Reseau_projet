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
    hash_c chunck_h;
} message_get;
// message REP_GET
typedef struct s_message_rep_get
{
    char type;
    short int length;
    hash_f file_h;
    hash_c chunck_h;
    char * chunck;
} message_rep_get;

// message REP_LIST
typedef struct s_message_rep_list
{
    char type;
    short int length;
    hash_f file_h;
    hash_c * chunck_list;
} message_rep_list;

typedef struct infos_s
{
    int sockfdtarget; // fd qui sert à faire les sockets
    int sockfdmy;
    struct sockaddr_in target;
    struct sockaddr_in my_addr;
    socklen_t addrlen_s;
    socklen_t addrlen_c;
    void * message; // contain the message
} infos;

// open listen port on port port_listen
infos init_listen(short int port_listen, infos infos_com)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);    
    memset (&infos_com.my_addr, 0, sizeof (struct sockaddr_in));
    
    if((infos_com.sockfdmy = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("socked");
        exit(EXIT_FAILURE);
    }

    // init local addr structure and other params
    infos_com.my_addr.sin_family      = AF_INET;
    infos_com.my_addr.sin_port        = port_listen;
    infos_com.my_addr.sin_addr.s_addr = INADDR_ANY;
    
    
    // bind addr structure with socket
    if(bind(infos_com.sockfdmy, (struct sockaddr *) &infos_com.my_addr, addrlen) == -1)
    {
      perror("bind");
      close(infos_com.sockfdmy);
      exit(EXIT_FAILURE);
    }

    // set the socket in passive mode (only used for accept())
    // and set the list size for pending connection
    if(listen(infos_com.sockfdmy, 5) == -1)
    {
        perror("listen");
        close(infos_com.sockfdmy);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for incomming connection\n");

    return infos_com;
}
    
// open connexion to address addr_connect port port_send
infos init_connexion(char * addr_connect, short int port_send,infos infos_com)
{    
    socklen_t addrlen;
    memset (&infos_com.target, 0, sizeof (struct sockaddr_in)); // remplit les bytes sur lesquels pointe &server avec des 0
    
    
    if((infos_com.sockfdtarget = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // init remote addr structure and other params
    infos_com.target.sin_family = AF_INET;
    infos_com.target.sin_port   = port_send;
    addrlen                     = sizeof(struct sockaddr_in);

    // get addr from command line and convert it
    if(inet_pton(AF_INET,addr_connect,&infos_com.target.sin_addr) != 1)
    {
        perror("inet_pton");
        close(infos_com.sockfdtarget);
        exit(EXIT_FAILURE);
    }

    printf("Trying to connect to the remote host\n");
    if(connect(infos_com.sockfdtarget,(struct sockaddr*)&infos_com.target,addrlen) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connection OK\n");
    return infos_com;
}


int main(int argc, char **argv)
{
    infos infos_com;
    short int port_send,port_listen;
    char * addr;
    
    // check the number of args on command line
    if(argc != 6)
    {
      printf("USAGE: %s @server port_send port_listen action hash \n", argv[0]);
      exit(-1);
    }
    
    // Analyse arguments
    // possible actions : GET, LIST, PUT, GET, KEEP_ALIVE, PRINT ? 
    if (strcmp(argv[4],"get") == 0)
    {
        printf("put\n");
    }
    else if (strcmp(argv[4],"list") == 0)
    {
        printf("salut\n");
    }
    else if (strcmp(argv[4],"put") == 0)
    {
        printf("salut\n");
    }
    else if (strcmp(argv[4],"keep_alive") == 0)
    {
        printf("salut\n");
    }
    else
    {
        errno = EINVAL;
        perror("Action ");
        exit(EXIT_FAILURE);
    }
    
    
    
    // set parameters
    port_send   = atoi(argv[2]);
    port_listen = atoi(argv[3]);
    addr        = argv[1];
    
    infos_com   = init_listen(port_listen, infos_com);
    infos_com   = init_connexion(addr, port_send, infos_com);
    
    
    
    // close the socket

    return 0;
}
