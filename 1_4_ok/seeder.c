
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


#include "structures.h"
#include "messages.h"
#include "sha256.h"

#define EXPIRATION 5
#define chunk_size 1000000 
#define FRAGMENT_TAILLE 1000 // data = 0 à 1000

int sendvar;
// init listen port
infos init_listen(infos infos_com)
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
    infos_com.my_addr.sin_port        = htons(infos_com.port_listen);
    infos_com.my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    
    // bind addr structure with socket
    if(bind(infos_com.sockfdmy, (struct sockaddr *) &infos_com.my_addr, addrlen) == -1)
    {
      perror("bind");
      close(infos_com.sockfdmy);
      exit(EXIT_FAILURE);
    }
    printf("listening on %d\n", infos_com.port_listen);
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
    printf("Message envoyé\n");
}







infos hashage(infos infos_com)
{
    int fd;
    unsigned char *hash;
    unsigned char ** tab_chunks;
    SHA256_CTX ctx;
    int size_file;
    unsigned char* buffer;
    int pos=0,chunk_number = 0;
    infos_com.hash = malloc(32);
    
    if((fd=open((char*)infos_com.filename,O_RDONLY)) < -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    

    // fait une empreinte du fichier total -> il faut le lire entièrement
    
    // débute par rechercher la taille du fichier avec lseek;
    if((size_file = lseek(fd,0,SEEK_END)) == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    
    // retourne au début
    if( lseek(fd,0,SEEK_SET) == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    
    buffer = malloc(size_file);
    if(read(fd,buffer,size_file) != size_file)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    sha256_init(&ctx);
    sha256_update(&ctx,buffer,size_file);
    sha256_final(&ctx,infos_com.hash);
    
    
    if(size_file % chunk_size != 0)
    {
        tab_chunks = malloc( sizeof( char *) *(size_file / chunk_size +1));
        infos_com.tab_index_chunks = malloc(sizeof(int *) *(size_file / chunk_size +1));
    }
    else
    {
        tab_chunks = malloc( sizeof( char *) *(size_file / chunk_size));
        infos_com.tab_index_chunks = malloc(sizeof(int *) *(size_file / chunk_size ));
    }
    // passe tout le buffer tous les chunk_size octets et fait un hash
    while(pos != size_file)
    {
        sha256_init(&ctx);
        hash = malloc( 32);
        if(size_file - pos > chunk_size)
        {
            sha256_update(&ctx,buffer+pos,chunk_size);
            sha256_final(&ctx,hash);
            tab_chunks[chunk_number] = hash;
            if(chunk_size % FRAGMENT_TAILLE != 0)
                infos_com.tab_index_chunks[chunk_number] = chunk_size /FRAGMENT_TAILLE+1;
            else
                infos_com.tab_index_chunks[chunk_number] = chunk_size /FRAGMENT_TAILLE;
            chunk_number ++;
            pos += chunk_size;
        }
        else
        {
            sha256_update(&ctx,buffer+pos,size_file - pos);
            sha256_final(&ctx,hash);
            tab_chunks[chunk_number] = hash;
            if((size_file - pos) % FRAGMENT_TAILLE != 0)
                infos_com.tab_index_chunks[chunk_number] = (size_file - pos) /FRAGMENT_TAILLE+1;
            else
                infos_com.tab_index_chunks[chunk_number] = (size_file - pos) /FRAGMENT_TAILLE;
            chunk_number ++;
            pos += size_file - pos;
        }
    }
    
    free(buffer); 
    infos_com.tab_chunks = tab_chunks;
    infos_com.nb_chunks = chunk_number;
    if(close(fd)==-1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    return infos_com;
}

unsigned char * rep_list(unsigned char * msg,infos infos_com)
{
    unsigned int length_hash = (unsigned int)buf_to_s_int(msg + 4);
    //unsigned char * hash = malloc(length_hash);
    unsigned int length_msg;
    int i,pos=0;
    
    /*
    if((hash=(unsigned char*)strncpy((char*)hash,(char*)msg + 6, length_hash)) == NULL)
    {
        perror("strncpy");
        exit(EXIT_FAILURE);
    }
    */
    // fait le message de réponse
    length_msg = 1 + 2 + 1 + 2 + 32 + infos_com.nb_chunks*(5 + 32);
    
    
    unsigned char * message = malloc(length_msg); 
    message[0] =  103;
    message = us_int_to_buf(message, length_msg - 3, 1);
    
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
        us_int_to_buf(message,infos_com.tab_index_chunks[i],pos);
        pos +=2;
    }
    return message;
}

int pos_hash (  char * hash,  char ** tab_chunks, int nb_chunks)
{
    int i;
    for(i=0;i< nb_chunks ; i++)
    {
        if(strncmp(hash,tab_chunks[i],32) == 0)
            return i;
    }
    return -1;
}
unsigned char * rep_get(unsigned char * msg,unsigned char * hash,infos infos_com,short int fragment_index)
{
    int pos = 1 + 2 + 1 + 2 + 32 + 1 ;
    // récupère d'abord les informations 
    char * chunk=malloc(chunk_size);
    int fd;
    int max_index;
    unsigned char * message;
    short int nb_cars_lus;
    short int chunk_index;
    
    int length_hash_chunk = buf_to_s_int(msg + pos) ; // enlève 2 octets (taille de l'index)
    
    pos += 2;
    unsigned char * hash_chunk = malloc(length_hash_chunk);
    if(strncpy((char*)hash_chunk,(char*)msg+pos,length_hash_chunk) ==NULL) // sort le hash du chunk
    {
        perror("strncpy");
        exit(EXIT_FAILURE);
    }
    pos +=length_hash_chunk;
    //chunk_index = buf_to_s_int(msg + pos);
    chunk_index = pos_hash((char*)hash_chunk,(char**) infos_com.tab_chunks,infos_com.nb_chunks);
    if(chunk_index == -1)
    {
        printf("Mauvais chunk :");
        print_hash(hash_chunk);
        printf("\n");
        return NULL;
    }
    
    
    if((fd= open((char*)infos_com.filename,O_RDONLY)) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    if( lseek(fd,chunk_index * chunk_size + fragment_index * FRAGMENT_TAILLE,SEEK_SET) == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    printf("pour le chunk %d et index %d se place à %d\n",chunk_index,fragment_index,chunk_index * chunk_size + fragment_index * FRAGMENT_TAILLE);
    //sleep(1);
    if((nb_cars_lus=read(fd,chunk,FRAGMENT_TAILLE)) == -1)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    max_index = infos_com.tab_index_chunks[chunk_index];
    
    if(nb_cars_lus < 1000)
        printf("nb_cars_lus %d\n",nb_cars_lus);
    printf("lindex du fragment est %d\n",fragment_index);
    message =create_message_rep_get( hash, hash_chunk, chunk, chunk_index,fragment_index, max_index,nb_cars_lus);
    free(chunk);
    free(hash_chunk);
    if(close(fd)==-1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    return message;
}

int traitement_message(infos infos_com, unsigned char * msg)
{
    unsigned char * response;
    unsigned short int length_hash =(unsigned short int) buf_to_s_int(msg + 4);
    unsigned char * hash = malloc(length_hash);
    short int asked_chunk;
    int i;
    
    if(strncpy((char*)hash,(char*)msg + 6, length_hash) == NULL)
    {
        perror("strncpy");
        exit(EXIT_FAILURE);
    }
    
    
    // regarde si le hash correspond bien au hash du fichier partagé
    if(strncmp((char*)hash,(char*) infos_com.hash,length_hash) != 0)
    {
        printf("fichier inconnu demandé\n");
        free(hash);
        return -1;
    }
    
    // fait les messages
    if (msg[0] == 102)  // message LIST
    {
        response = rep_list(msg,infos_com);
        send_packet ( response, infos_com.sockfdtarget,infos_com.target);
        free(response);
        free(hash);
        printf("msg type 103 envoyé \n");
        return 1;
    }
    else if (msg[0] == 100)
    {// envoie tous les fragments
        asked_chunk = buf_to_s_int(msg + 3 + 3 + 32 + 3 + 32);
        for (i = 0 ; i < infos_com.tab_index_chunks[asked_chunk] ; i++)
        {
            response = rep_get( msg,hash,infos_com,i);
            send_packet ( response, infos_com.sockfdtarget,infos_com.target);
            free(response);
            usleep(1500);
            sendvar++;
        }
        free(hash);
        return 1;
    }
    else
    {
        free(hash);
        return -1;
    }
    free(hash);
    return -1;
}



// argv[1] : port, argv[2] : file
int main(int argc, char **argv)
{
    sendvar =0;
    // check the number of args on command line
    if(argc != 3)
    {
      printf("USAGE: %s port file\n", argv[0]);
      exit(-1);
    }
    infos infos_com;
    infos_com.port_listen = atoi(argv[1]);
    unsigned char * file_name = (unsigned char*)argv[2];
    unsigned char buf[1082]; // 3 +3+32+ 3+32+2+ 3+4+data(0 à 1000)
    socklen_t addrlen = sizeof(struct sockaddr);
    int message;
    int i;
    infos_com.filename = file_name;
    infos_com = init_listen(infos_com);
    infos_com = hashage(infos_com);
    printf("listening on %d, file %s, hash ",infos_com.port_listen, file_name);
    print_hash(infos_com.hash);
    printf("\n");
    
    printf("chunks [  ");
    for(i=0; i < infos_com.nb_chunks ; i++)
    {
        print_hash(infos_com.tab_chunks[i]);
        printf("  ");
    }
    printf("]\n");
    
    printf("il y a %d chunks \n",infos_com.nb_chunks);
    
    // wait for connexion
    while(1) // communication client-client : GET, REP_GET, LIST, REP_LISTE
    {
        // wait for messages
        if(recvfrom(infos_com.sockfdmy, buf, 1082, 0,(struct sockaddr*)&infos_com.target,&addrlen) == -1)
        {
            perror("recv");
            close(infos_com.sockfdmy);
            exit(EXIT_FAILURE);
        }
        printf("received message \n");
        // établit la connexion avec la source
        if((infos_com.sockfdtarget = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            perror("socket");
            exit(EXIT_FAILURE);
        }
        
        // traite le message
        message = traitement_message(infos_com,buf);
        if(message ==1)
            printf("Message envoyé\n");
        else
            printf("Message invalide \n");
        if(close(infos_com.sockfdtarget) ==-1)
        {
            perror("close");
            exit(EXIT_FAILURE);
        }
        printf(" seend vaut %d\n",sendvar);
    }
    if(close(infos_com.sockfdmy) == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    return 0;
}
