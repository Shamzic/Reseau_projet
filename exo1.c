#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> 
#include <sys/types.h> 

#define BUFFER_SIZE 10

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

int main (int argc, char * argv[])
{
  if (argc != 2) 
  {
        fprintf(stderr,"usage: %s nomfichier",argv[0]);
        exit(1);
  }
  
  unsigned char hash[32];
  int idx;
  SHA256_CTX ctx;
  
  printf("fichier haché : %s\n",argv[1]);
  int file=0;
  if((file=open(argv[1],O_RDONLY)) < -1)
    msg_error("open\n");
  int taille;
  char *buffer;
  buffer = (char*)malloc(BUFFER_SIZE);
  int i=1;
  
  while(taille = read(file,buffer,BUFFER_SIZE) > 0) // parcourt le fichier 10 octets par 10
  {
      printf("chunk %d : %s\n",i,buffer); 
      i++;     
  } 
  free(buffer);
  if(close(file) == -1)
    msg_error("close");
  return 0;
}
