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
/* linked list function */

client_s* ajouterEnTete (client_s* liste_clients, short int length,short int port,char *address_ip)
{
  client_s* new_client = malloc(sizeof(client_s));
  new_client->length=length;
  new_client->port=port;
  strcpy (new_client->address_ip,address_ip); // tableau statique , no need init
  new_client->next = liste_clients;
  return new_client;
}

client_s* ajouterEnFin (client_s* liste_clients,short int length,short int port,char *address_ip)
{
  client_s* new_client = malloc(sizeof(client_s));
  new_client->length=length;
  new_client->port=port;
  strcpy (new_client->address_ip,address_ip); // tableau statique , no need init
  new_client->next = NULL;
  
  if (liste_clients == NULL)
    {
      return new_client;
    }
  else
    {
      client_s* temp = liste_clients;
      while (temp->next != NULL)
	{
	  temp = temp->next;
	}
      temp->next = new_client;
      return liste_clients;
    }
}


void afficherListe (client_s* liste_clients)
{
  client_s* tmp = liste_clients;
  int i=1;
  while (tmp != NULL)
    {
      printf ("Client %d :\n",i);
      printf("\t-Length : %hi\n",tmp->length);
      printf("\t-Port : %hi\n",tmp->port);
      printf("\t-Adresse : %s\n\n",tmp->address_ip);
      tmp = tmp->next;
      i++;
    }
}


client_s* effacerListe (client_s* liste_clients)
{
  if (liste_clients == NULL)
    {
      return NULL;
    }
  else
    {
      client_s* tmp;
      tmp = liste_clients->next;
      free (liste_clients);
      return effacerListe (tmp);
    }
}


client_s* supprimerElementEnTete (client_s* liste_clients)
{
  if (liste_clients != NULL)
    {
      client_s* aRenvoyer = liste_clients->next;
      free (liste_clients);
      return aRenvoyer;
    }
  else
    {
      return NULL;
    }
}
