
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
/*
 *  CREATE MESSAGES CLIENT SIDE 
*/


// message GET between 2 clients
unsigned char *  create_message_get_peer(unsigned char * hash_file,unsigned char * hash_chunk, short int index)
{
    unsigned char *buffer=malloc(75);
    short int length;
    // create the message
    buffer[0] = 100 ; // set type
    length    = 3 + 32 + 3 + 32 + 2;
    buffer    = s_int_to_buf(buffer,length, 1);
    // hash file
    buffer[3] = 50;
    buffer    = s_int_to_buf(buffer,32,4);
    memcpy(buffer+6,hash_file,32);
    // hash chunk
    buffer[38] = 51;
    buffer = s_int_to_buf(buffer,32,39);
    memcpy(buffer+41,hash_chunk,32);
    buffer      = s_int_to_buf(buffer,index,73);
    return buffer;
}



// message REP_GET
// chunk doit finir par un \0
unsigned char * create_message_rep_get(unsigned char * hash_file,unsigned char * hash_chunk,char * chunk,short int index, short int max_index,short int nb_cars_lus)
{
    unsigned char *buffer=malloc( 3+3+32+3+32+2+1+2+2+2+nb_cars_lus ); // 82 + nb_cars_lus
    short int length;
    // create the message
    buffer[0] = 101 ; // set type
    length    = 3+32+3+32+2+1+2+2+2+nb_cars_lus;
    buffer    = s_int_to_buf(buffer,length, 1);
    // hash file
    buffer[3] = 50;
    buffer    = s_int_to_buf(buffer,32,4);
    memcpy(buffer+6,hash_file,32);
    // hash chunk
    buffer[38] = 51;
    buffer     = s_int_to_buf(buffer,32,39);
    memcpy(buffer+41,hash_chunk,32);
    buffer = s_int_to_buf(buffer, index,73);
    // chunk
    buffer[75]  = 60;
    buffer      = s_int_to_buf(buffer,nb_cars_lus + 2 + 2,76);
    buffer      = s_int_to_buf(buffer,index,78);
    buffer      = s_int_to_buf(buffer,max_index,80);
    memcpy(buffer+82,chunk,nb_cars_lus);
    return buffer;
}

// message LIST
unsigned char * create_message_list(unsigned char * hash_file)
{
    unsigned char *buffer=malloc(38); // 
    short int length;
    // create the message
    buffer[0] = 102 ; // set type
    length    = 35; // type + length + hash
    buffer    = s_int_to_buf(buffer,length, 1);
    // hash file
    buffer[3] = 50;
    buffer    = s_int_to_buf(buffer,32,4);
    memcpy(buffer+6,hash_file,32);
    printf("res %d\n",strncmp( (char*) hash_file,(char*)buffer+6,32));
    return buffer;
}

// message REP_LIST
unsigned char * create_message_rep_list(unsigned char * hash_file);


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
    buffer=malloc(43 + length_address);
    buffer[0] = 110 ; // set type
    length    = 43 + length_address - 3;
    buffer    = s_int_to_buf(buffer,length, 1);
    
    // hash file
    buffer[3] = 50;
    buffer    = s_int_to_buf(buffer,32,4);
    memcpy(buffer+6,hash_file,32);
    
    // client
    buffer[38] = 55;
    buffer     = s_int_to_buf(buffer,length_address + 2, 39);
    buffer     = us_int_to_buf(buffer,port, 41);
    memcpy(buffer+43,address,length_address);
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






// Fonctions used to create messages


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


void print_hash(unsigned char hash[])
{
   int idx;
   for (idx=0; idx < 32; idx++)
      printf("%02x",hash[idx]);
}

// transforme un hash sur 64 bits en un hash sur 32 bits
unsigned char * hash_to_char( char* hash)
{
    int i;
    unsigned char * new = malloc(32);
    char a,b;
    for(i = 0 ; i < 32 ; i++)
    {
        if(hash[i*2] <= '9')
            a = hash[i*2] - '0';
        else
            a = hash[i*2] - 'a' + 10;
        if(hash[i*2+1] <= '9')
            b = hash[i*2+1] - '0';
        else
            b = hash[i*2+1] - 'a' + 10;
        new[i] = a * 16 + b;
    }
    return new;
}


