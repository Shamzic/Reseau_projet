
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
unsigned char *  create_message_get_peer(unsigned char * hash_file,unsigned char * hash_chunk)
{
    unsigned char *buffer=malloc(74); // ou 74 ? 
    short int length;
    // create the message
    buffer[0] = 100 ; // set type
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
    buffer[0] = 100 ; // set type
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






