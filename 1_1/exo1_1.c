/*  
-----------------------------
Test du fichier : 
 >gcc exo1.c sha256.c -o main -Wall -Werror -Wextra
 >./main toto
-----------------------------
*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <memory.h>
#include <string.h>
#include "sha256.h"

#define chunk_size 1000000


extern int errno;

void msg_error(const char * msg) {
    perror(msg);
    exit(1);
}

void print_hash(unsigned char hash[])
{
   int idx;
   for (idx=0; idx < 32; idx++)
      printf("%02x",hash[idx]);
   printf("\n");
}

char *suppr_retour_chariot(char * s)
{
  if(s[strlen(s)]=='\n')
    s[strlen(s)]='\0';
  return s;
}


// problème au dernier chunk avec les primitives syst : dernier chunk mal haché

int main (int argc, char * argv[])
{
    int fd;
    unsigned char hash[32];
    SHA256_CTX ctx;
    int size_file;
    unsigned char* buffer;
    int pos=0,chunk_number = 0;

    if (argc != 2) 
    {
        fprintf(stderr,"usage: %s nomfichier\n",argv[0]);
        exit(1);
    }
    if((fd=open(argv[1],O_RDONLY)) < -1)
        msg_error("open\n");

    // fait une empreinte du fichier total -> il faut le lire entièrement
    
    // débute par rechercher la taille du fichier avec lseek;
    if((size_file = lseek(fd,0,SEEK_END)) == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    size_file --;
    printf("file offset %d\n",size_file);
    
    // retourne au début
    if( lseek(fd,0,SEEK_SET) == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    
    buffer = malloc(size_file+1);
    if(read(fd,buffer,size_file) != size_file)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    buffer[size_file] = 0;
    
    printf("FILE HASH : ");
    sha256_init(&ctx);
    sha256_update(&ctx,buffer,size_file);
    sha256_final(&ctx,hash);
    print_hash(hash);

    // passe tout le buffer tous les x octets et fait un hash
    while(pos != size_file)
    {
        sha256_init(&ctx);
        if(size_file - pos > chunk_size)
        {
            sha256_update(&ctx,buffer+pos,chunk_size);
            sha256_final(&ctx,hash);
            printf("CHUNK %d : ",chunk_number);
            print_hash(hash);
            chunk_number ++;
            pos += chunk_size;
        }
        else
        {
            sha256_update(&ctx,buffer+pos,size_file - pos);
            sha256_final(&ctx,hash);
            printf("CHUNK %d : ",chunk_number);
            print_hash(hash);
            chunk_number ++;
            pos += size_file - pos;
        }
    }
    
    free(buffer); 
    if(close(fd) == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    return 0;
}
