/*  
-----------------------------
Test du fichier : 
 >gcc exo1.c sha256.c -o main
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

char *suppr_retour_chariot(char * s)
{
  if(s[strlen(s)]=='\n')
    s[strlen(s)]='\0';
  return s;
}


// problème au dernier chunk avec les primitives syst : dernier chunk mal haché

int main (int argc, char * argv[])
{
  if (argc != 2) 
  {
        fprintf(stderr,"usage: %s nomfichier\n",argv[0]);
        exit(1);
  }
 
  unsigned char hash[32];
  //int idx;
  SHA256_CTX ctx;
  
  printf("fichier haché : %s\n",argv[1]);
  int file=0;
  if((file=open(argv[1],O_RDONLY)) < -1)
    msg_error("open\n");
  int taille;
  unsigned char buffer[BUFFER_SIZE];
  //buffer = (char*)malloc(BUFFER_SIZE);
  int i=1;
  
  while((taille = read(file,buffer,BUFFER_SIZE)) > 0) // parcourt le fichier 10 octets par 10 pour l'instant
  {
      //suppr_retour_chariot(buffer);
      printf("taille %d ",taille);
      buffer[taille]='\0';
      printf("chunk %d : %s\n",i,buffer);       
      sha256_init(&ctx);
      sha256_update(&ctx,buffer,taille);
      sha256_final(&ctx,hash);
      print_hash(hash);
      i++;     
  } 
  //free(buffer); 
  if(close(file) == -1)
    msg_error("close");
    
//---------------------------------------------------------
// version avec fonction bibliothèques fgets fopen fclose
// --------------------------------------------------------
   /*
  unsigned char hash2[32];
  SHA256_CTX ctx2;
    
    
  FILE* fichier = NULL;
  fichier = fopen(argv[1], "r+");
  char buff[BUFFER_SIZE];
  if (fichier != NULL)
  {// On peut lire et écrire dans le fichier
  int j=1;
    while(fgets(buff,BUFFER_SIZE+1,fichier)!=NULL)
    {
      //if(buff[strlen(buff)]='\n')
      //{  
        printf("caractètre de fin de chaine de buff :%c.\n",buff[strlen(buff)]);
        //buff[strlen(buff)]='\0';
      //}
      sha256_init(&ctx2);
      sha256_update(&ctx2,buff,BUFFER_SIZE);
      sha256_final(&ctx2,hash2);
      printf("chunk %d : %s.\n",j,buff); 
      print_hash(hash2);
      j++;
    }
  
    fclose(fichier);
  }
  else
  {
// On affiche un message d'erreur si on veut
    printf("Impossible d'ouvrir le fichier test.txt");
  }*/
  return 0;
}
