#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

int main()
{
     char * buffer = malloc(20);
     char * string = "Salut";
     char * string2 = "je suis";
     strcpy(buffer,string);
     strcpy(buffer+3,string2);
    printf("%s\n",buffer);
    return 0;
}
