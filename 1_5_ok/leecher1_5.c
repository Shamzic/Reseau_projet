
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>


#include "structures.h"
#include "messages.h"
#include "sha256.h"

#define EXPIRATION 5
#define chunk_size 1000000
#define FRAGMENT_TAILLE 1000
int var;
// init listen port
// my_addr : listen messages from seeders
// seeder[i].addr_struct : communication with seeder i
// send_addr : addr for send fragments to other leechers
infos init_connexion_seeders(infos infos_com)
{
    int i;
    //socklen_t addrlen = sizeof(struct sockaddr_in);
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if((infos_com.sockfdseeder = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // init local addr structure and other params
    for(i=0;i<infos_com.nb_seeders;i++)
    {
        memset (&infos_com.liste_seeder[i].addr_struct, 0, sizeof (struct sockaddr_in));
        infos_com.liste_seeder[i].addr_struct.sin_family      = AF_INET;
        infos_com.liste_seeder[i].addr_struct.sin_port            = htons(infos_com.liste_seeder[i].port);
        infos_com.liste_seeder[i].addr_struct.sin_addr.s_addr = htonl(INADDR_ANY);
        infos_com.liste_seeder[i].asked_chunk = 0;
    }
    
    infos_com.my_addr.sin_family      = AF_INET;
    infos_com.my_addr.sin_port        = htons(0);
    infos_com.my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // bind addr structure with socket
    if(bind(infos_com.sockfdseeder, (struct sockaddr *) &infos_com.my_addr, addrlen) == -1)
    {
      perror("bind sockfdtarget");
      close(infos_com.sockfdseeder);
      exit(EXIT_FAILURE);
    }
    
    // init listen port for leechers which ask segments
    infos_com.send_addr.sin_family    = AF_INET;
    infos_com.send_addr.sin_port      = htons(infos_com.port_listen);
    infos_com.send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if((infos_com.sockfdsend = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if(bind(infos_com.sockfdsend, (struct sockaddr *) &infos_com.send_addr, addrlen) == -1)
    {
        perror("bind");
        close(infos_com.sockfdsend);
        exit(EXIT_FAILURE);
    }
    
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
//    printf("Message envoyé\n");
}




unsigned char * rep_list(unsigned char * msg,infos infos_com)
{
    unsigned int length_hash = (unsigned int)buf_to_s_int(msg + 4);
    unsigned char * hash = malloc(length_hash);
    unsigned int length_msg;
    int i,pos=0;
    if((hash=(unsigned char*)strncpy((char*)hash,(char*)msg + 6, length_hash)) == NULL)
    {
        perror("strncpy");
        exit(EXIT_FAILURE);
    }
    // fait le message de réponse
    length_msg = 1 + 2 + 1 + 2 + 32 + infos_com.nb_chunks*(5 + 32);
    unsigned char * message = malloc(length_msg); 
    message[0] =  103;
    message = us_int_to_buf(message, length_msg-3, 1);
    
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




// test if right message was received
int test_rep( char * action, unsigned char * msg_send, unsigned char * msg_recv)
{
    int test = 0;
    short int msg_send_length = buf_to_s_int(msg_send +1) + 3; // length in message + 3 (type + length)
    short int tmp_length;
    // communication with clients
    if(strcmp(action,"get") == 0) // message get client
    {
        if (msg_recv[0] != 101) // test if good type of message
            return -1;
        msg_recv[0] = 100; // change type to compare with msg_send
        tmp_length = buf_to_s_int(msg_recv +1);
        msg_recv    = s_int_to_buf( msg_recv, buf_to_s_int(msg_send + 1), 1);
        if((test=u_strncmp(msg_send,msg_recv,msg_send_length)) != 0) // strings different
        {
            //printf("j'attends\n");
            return -1;
        }
        msg_recv    = s_int_to_buf(msg_recv, tmp_length, 1);
        return 0;
    }
    else if(strcmp(action,"list") == 0) // message list client
    {
        if (msg_recv[0] != 103) // test if good type of message
            return -1;
        msg_recv[0] = 102; // change type to compare with msg_send
        tmp_length = buf_to_s_int(msg_recv +1);
        msg_recv    = s_int_to_buf(msg_recv, buf_to_s_int(msg_send + 1), 1);
        if(u_strncmp(msg_send,msg_recv,msg_send_length) != 0) // strings different
            return -1;
        msg_recv    = s_int_to_buf(msg_recv, tmp_length, 1);
        return 0;
    }
    
    return -1;
}


unsigned char * seek_response(unsigned char * message, int sockfd,struct sockaddr_in addr,char * action)
{
    unsigned char *msg_received = malloc(1082);
    socklen_t addrlen = sizeof(struct sockaddr_in);
    
    if(recvfrom(sockfd, msg_received, 1082, MSG_DONTWAIT,(struct sockaddr *)&addr,&addrlen) == -1)
    {
        if ( errno == EAGAIN || errno == EWOULDBLOCK) // there were no packet
        {
   //         printf("Aucune réponse reçue\n");
            free(msg_received);
            return NULL;
        }
        perror("recv");
        exit(EXIT_FAILURE);
    }
    var ++;
    
    // test if type of response is good
    if(test_rep(action,message,msg_received)==-1) // wrong hash
    {
  //      printf("Wrong msg \n");
        free(msg_received);
        return NULL;
    }
    else
        return msg_received;
}


table_chunks save_chunks(unsigned char * message)
{
    table_chunks table;
    int i;
    int pos                  = 3 + 3 + 32; // saute type + length du message + le hash
    short int length_message =  buf_to_s_int(message + 1);
    table.nb_chunks          = (length_message - 32 + 3)/(1+2+2 +32); // 1 + 2 + 2 + 32 -> taille de hash_chunk
    table.index              = 0;
    table.tab_chunks         = malloc( table.nb_chunks * sizeof(char*));
    table.tab_index_chunks         = malloc( table.nb_chunks * sizeof(int));
    // remplit la table
    for(i=0;i< table.nb_chunks; i++)
    {
        pos += 3;  // saute type + length
        table.tab_chunks[i] = malloc(32);
        if(strncpy((char*)table.tab_chunks[i], (char *)message + pos,32) == NULL)
        {
            perror("strncpy");
            exit(EXIT_FAILURE);
        }
        pos += 32;
        table.tab_index_chunks[i] = buf_to_s_int(message + pos);
        printf("jintialise %d qui vaut %hd\n",i,(int)table.tab_index_chunks[i]);
        pos += 2;
    }
   //sleep(20);
    return table;
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


int write_chunk(unsigned char * response,int fd)
{
    short int size_fragment = buf_to_s_int( response + 3 + 3 + 32 + 3 + 32 +2 + 1)- 4;
    short int index_chunk = buf_to_s_int( response + 3 + 3 + 32 + 3 + 32 );
    short int index_fragment = buf_to_s_int( response + 3 + 3 + 32 + 3 + 32 +2 + 1 +2);
    
    // se place
    if(lseek(fd, index_chunk * chunk_size + index_fragment * FRAGMENT_TAILLE,SEEK_SET) == -1)
    {
        perror("lseek write");
        exit(EXIT_FAILURE);
    }
  //  printf("write se place %d  index chunk %d index fra %d\n",index_chunk * chunk_size + index_fragment * FRAGMENT_TAILLE,index_chunk,index_fragment);
    // write chunk into file
    if(write(fd, response + 3 + 3 + 32 + 3 + 32 + 2 + 1 + 6, size_fragment) == -1)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
    return 0;
}

seeder* save_seeder ( int argc,char ** argv)
{
    int nb_seeder = (argc -4) / 2;
    seeder * liste_seeder = malloc(sizeof(seeder) * nb_seeder);
    int i;
    
    for(i=0 ; i< nb_seeder ; i++)
    {
        liste_seeder[i].addr = argv[ i*2 + 1 ];
        liste_seeder[i].port = atoi(argv[ i*2 + 2 ]);
    }
    return liste_seeder;
}

// test if every chunk was received
int file_received ( have_chunk *table, int nb_chunks)
{
    int i;
    for(i = 0;i< nb_chunks; i ++)
    {
        if(table[i].have == -1 || table[i].have == 0) // chunk pas encore cherché ou chunk en cours de recherche
            return -1;
    }
    return 0;
}

// attribut à un seeder un fichier à rechercher
// Si le fichier est en train d'être cherché ( have_fragments = 0 ) ne fait rien. 
// Si plus aucun chunk non trouvé qui n'est pas en train d'être cherché ne fais rien non plus
seeder attr_index_search(seeder s, infos infos_com, table_chunks chunk_tab)
{
    int i;
    //printf("putain cherche %d %d\n",s.asked_chunk);
  //  printf("putain\n");
    for(i=0;i<chunk_tab.nb_chunks;i++) // passe chaque chunk
    {
      //  printf("demande\n");
       // printf("lindex est %d\n",chunk_tab.tab_index_chunks[i]);
        if(infos_com.have_table[i].have == -1) // possède le chunk;
        {
            s.asked_chunk = i;
            infos_com.have_table[i].have = 0;
            return s;
        }
    //    printf("je vais attendre 100 s\n");
  //      sleep(100);
    }
    return s;
}


unsigned char * rep_get(unsigned char * msg,unsigned char * hash,infos infos_com)
{
    int pos = 1 + 2 + 1 + 2 + 32 + 1 ;
    // récupère d'abord les informations 
    short int index;
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
    index = buf_to_s_int(msg + pos);
    chunk_index = pos_hash((char*)hash_chunk,(char**) infos_com.tab_chunks,infos_com.nb_chunks);
    if(chunk_index == -1)
    {
        printf("Mauvais chunk :");
        print_hash(hash_chunk);
        printf("\n");
        return NULL;
    }
    
    //printf("try to open file %s\n",infos_com.dest_file);
    if((fd= open((char*)infos_com.dest_file,O_RDONLY)) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    //printf("va a fd %d\n",fd);
    if( lseek(fd,chunk_index * chunk_size + index * FRAGMENT_TAILLE,SEEK_SET) == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    printf("pour le chunk %d et index %d se place à %d\n",chunk_index,index,chunk_index * chunk_size + index * FRAGMENT_TAILLE);
    //sleep(1);
    if((nb_cars_lus=read(fd,chunk,FRAGMENT_TAILLE)) == -1)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    max_index = infos_com.tab_index_chunks[chunk_index];
    printf("le max index %d\n",max_index);
   // sleep(1);
    message =create_message_rep_get( hash, hash_chunk, chunk, index, max_index,nb_cars_lus);
    free(chunk);
    free(hash_chunk);
    if(close(fd)==-1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    return message;
}


unsigned char * traitement_message(infos infos_com, unsigned char * msg)
{
    unsigned char * response;
    unsigned short int length_hash =(unsigned short int) buf_to_s_int(msg + 4);
    unsigned char * hash = malloc(length_hash);
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
        return NULL;
    }
    // fait les messages
    if (msg[0] == 102)  // message LIST
    {
        response = rep_list(msg,infos_com);
    }
    else if (msg[0] == 100)
    {
        response = rep_get( msg,hash,infos_com);
    }
    else
    {
        free(hash);
        return NULL;
    }
    free(hash);
    return response;
}

have_chunk  set_fragments_not_received ( have_chunk table,int  max_index)
{
    int i;
    for(i=0;i<max_index; i++)
        table.have_fragments[i] = -1;
    return table;
}

int chunk_complet(have_chunk table, int max_index)
{
    int i;
    for(i=0;i <max_index ;i++)
    {
        if(table.have_fragments[i] == -1)
            return -1;
    }
    return 1;
}

void * send_chunks (void * info)
{
    unsigned char buf[1082];
    socklen_t addrlen = sizeof(struct sockaddr_in);
    infos * infos_com = (infos*)info;
    unsigned char * message;
    short int asked_chunk;
    unsigned char * hash_chunk = malloc(32);
    // wait for connexion
    while(1) // communication client-client : GET, REP_GET, LIST, REP_LISTE
    {
        // wait for messages
        if(recvfrom(infos_com->sockfdsend, buf, 1082, 0,(struct sockaddr*)&infos_com->asker,&addrlen) == -1)
        {
            perror("recv");
            close(infos_com->sockfdmy);
            exit(EXIT_FAILURE);
        }
        printf("received message in send_chunks\n");
        // traite le message
        // test if chunk valide 
        if(strncpy((char*)hash_chunk,(char*)buf+41,32) ==NULL) // sort le hash du chunk
        {
            perror("strncpy");
            exit(EXIT_FAILURE);
        }
        
        asked_chunk = pos_hash((char*)hash_chunk,(char**) infos_com->tab_chunks,infos_com->nb_chunks);
        if(buf[0]== 100 &&(asked_chunk > infos_com->nb_chunks || infos_com->have_table[asked_chunk].have != 1))
        {
            printf("CHUNK non possédé \n");
            continue;
        }
        printf("CHUNK possédé\n");
        message = traitement_message(*infos_com,buf);
        if(message !=NULL)
        {
            send_packet ( message, infos_com->sockfdsend,infos_com->asker);
            printf("Message envoyé from send_chunks\n");
        }
        else
            printf("Message invalide \n");
        
        free(message);
    }
    free(hash_chunk);
}


int main(int argc, char **argv)
{
    var=0;
    // check the number of args on command line
    if( argc % 2 != 0 || argc < 6)
    {
      printf("USAGE: %s address port [ address port] listen port hash_file dest_file\n", argv[0]);
      exit(-1);
    }
    struct pollfd fdpol;
    unsigned char * message;
    unsigned char * response = NULL;
    int fd,i,j;
    table_chunks table ;
    have_chunk * have_table;
    infos infos_com;
    seeder s ;
    unsigned char ** tab_request;
    int * first_message ;
    int max_index,pos;
    int empty, received_fragment,received_chunk;
    pthread_t send_chunk_thread;
    fdpol.events = POLLIN;
    
    int n = 1000000000;
    if(setsockopt(infos_com.sockfdseeder, SOL_SOCKET, SO_RCVBUF,&n,sizeof(n)) == -1)
        perror("setsockopt");
    infos_com.hash      =hash_to_char( argv[argc - 2]);
    infos_com.dest_file = (unsigned char *)argv[argc - 1];
    infos_com.port_listen = atoi(argv[argc - 3]);
    // enregistre les seeder
    infos_com.liste_seeder = save_seeder(argc,argv);
    infos_com.nb_seeders    = (argc-4)/2;
    tab_request = malloc(sizeof(char *)*infos_com.nb_seeders);
    
    // first send message LIST to client
    infos_com           = init_connexion_seeders(infos_com);
    message             = create_message_list(infos_com.hash);
    fdpol.fd = infos_com.sockfdseeder;
    
    first_message = malloc(infos_com.nb_seeders *sizeof(int));
    
    while(response == NULL)
    {
        send_packet(message,infos_com.sockfdseeder,infos_com.liste_seeder[0].addr_struct);
        //printf("je suis ici\n");
        switch(poll(&fdpol, 1 , 1000))
        {
            case -1:
                perror("poll");
                exit(EXIT_FAILURE);
                break;
            case 0: // Timeout
                printf("timeout\n");
        send_packet(message,infos_com.sockfdseeder,infos_com.liste_seeder[0].addr_struct);
                break;
            default:
            {
                response = seek_response(message,infos_com.sockfdseeder, infos_com.my_addr,"list");
                break;
            }
        }
    }
      
    
    // analyze function
    table = save_chunks(response);
    infos_com.tab_chunks = table.tab_chunks;
    infos_com.nb_chunks = table.nb_chunks;
    // initialise tous les have à -1
    have_table = malloc(sizeof( have_chunk) * table.nb_chunks);
    printf("size have table %d\n",(int) sizeof( have_chunk) * table.nb_chunks);
    for(i=0 ; i < table.nb_chunks ; i++)
    {
        have_table[i].have = -1;
        printf(" index de chunks %d\n",table.tab_index_chunks[i]);
    }
    infos_com.have_table = have_table;
    infos_com.tab_index_chunks = table.tab_index_chunks;
//    // intialise tab_index_chunks
//    infos_com.tab_index_chunks = malloc(sizeof(int *)*infos_com.nb_chunks);
    
    
    free(response);
    free(message);
    // create file
    if( (fd = open(argv[4],O_RDWR|O_CREAT,0666)) == -1 )
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    infos_com.fd = fd;
    
    if(pthread_create(&send_chunk_thread, NULL, send_chunks,&infos_com) == -1)
    {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    
    while(file_received(have_table,table.nb_chunks) == -1) // tant que fichier n'a pas été reçu
    {
        // send get messages
        printf("il y a %d seeder\n",infos_com.nb_seeders);
        for(i=0; i < infos_com.nb_seeders ; i ++) // cherche un fragment à demander pour chaque packet à envoyer
        {
            printf("je suis i %d\n",i);
            infos_com.liste_seeder[i] = attr_index_search(infos_com.liste_seeder[i],infos_com,table);
            printf("jai demandé le chunk %d\n",infos_com.liste_seeder[i].asked_chunk);
            s = infos_com.liste_seeder[i];
            tab_request[i] = create_message_get_peer( infos_com.hash, table.tab_chunks[s.asked_chunk],s.asked_chunk);
            send_packet(tab_request[i],infos_com.sockfdseeder, s.addr_struct);
            
        //    if (var > 0)
          //      printf("first msg %d avec i %d\n",first_message[i],i);
            first_message[i] = -1 ; //initialise first_message à -1
        }
        
        max_index = infos_com.nb_seeders; // init max_index au nombre de seeders (par la suite le corrige lorsqu'il reçoit les premiers messages)
        for(i=0; i < max_index ; i++)
        {
            ioctl(infos_com.sockfdseeder,FIONREAD,&empty);
            if(empty != 0)
            {
                for(j=0;j<infos_com.nb_seeders;j++)
                {
                    response = seek_response(tab_request[j],infos_com.sockfdseeder,infos_com.liste_seeder[j].addr_struct,"get");
                    if(response != NULL)
                        break;
                }
                if(response == NULL)
                    continue;
                // regarde si c'est le 1 er message reçu du chunk demandé 
                pos = buf_to_s_int( response + 3 + 3 + 32 + 3 + 32 );
                for(j=0;j<infos_com.nb_seeders; j++)
                {
                    //printf("il y a %d seeder \n",infos_com.nb_seeders);
                    if(infos_com.liste_seeder[j].asked_chunk == pos)
                    {
                        if(first_message[j] == -1) // first message which was received
                        {
                            printf("firts message %d du fragment %d j vaut %d\n",first_message[j],pos,j);
                            //if(pos == 1)
                           //     sleep(19);
                            first_message[j] =buf_to_s_int(response + 3+3+32+3+32+2+3+2);
                            max_index +=  buf_to_s_int(response + 3+3+32+3+32+2+3+2) -1; // enlève  pour le message déjà eu
                            infos_com.have_table[pos].have_fragments = malloc(first_message[j]);
                            received_fragment = buf_to_s_int(response + 3+3+32+3+32+2+3);
                            infos_com.have_table[pos].have_fragments[received_fragment] =1;
                            break;
                        }
                    }
                }
                
                if(write_chunk(response,fd) == -1)
                    printf("Mauvais chunk\n");
                else // good write -> set tab_have[i].fragments_have[fragment_received] to 1
                {
                    received_fragment = buf_to_s_int(response + 3+3+32+3+32+2+3);
                    received_chunk = buf_to_s_int( response + 3 + 3 + 32 + 3 + 32 );
                    infos_com.have_table[received_chunk].have_fragments[received_fragment] = 1;// marque l'index comme reçu
          //          printf("MESSAGE REÇU \n");
                }
                free(response);
                response = NULL;
            }
            else
            {
                switch(poll(&fdpol,1,100))
                {
                    case -1:
                        perror("poll");
                        exit(EXIT_FAILURE);
                        break;
                    case 0: // Timeout
                        printf("Timeout au message %d\n",i);
                        break;
                    case 1:
                    {
                        for(j=0;j<infos_com.nb_seeders;j++)
                        {
                            response = seek_response(tab_request[j],infos_com.sockfdseeder,infos_com.liste_seeder[j].addr_struct,"get");
                            if(response != NULL)
                                break;
                        }
                        if(response == NULL)
                            continue;
                        // regarde si c'est le 1 er message reçu du chunk demandé 
                        pos = buf_to_s_int( response + 3 + 3 + 32 + 3 + 32 );
                        for(j=0;j<infos_com.nb_seeders; j++)
                        {
                            if(infos_com.liste_seeder[j].asked_chunk == pos)
                            {
                                if(first_message[j] == -1) // first message which was received
                                {
                            printf("firteee message %d du chunk %d\n",first_message[j],pos);
                                   first_message[j] =buf_to_s_int(response + 3+3+32+3+32+2+3+2);
                                    max_index +=  buf_to_s_int(response + 3+3+32+3+32+2+3+2) -1; // enlève  pour le message déjà eu
                                    printf("fait un malloc de %d\n",first_message[j]);
                                    infos_com.have_table[pos].have_fragments = malloc(first_message[j]);
                                    received_fragment = buf_to_s_int(response + 3+3+32+3+32+2+3);
                                    
                          //  if(pos == 1)
                            //    sleep(19);
                                    infos_com.have_table[pos].have_fragments[received_fragment] =1;
                                    break;
                                }
                            }
                        }
                        
                        if(write_chunk(response,fd) == -1)
                            printf("Mauvais chunk\n");
                        else // good write -> set tab_have[i].fragments_have[fragment_received] to 1
                        {
                            received_fragment = buf_to_s_int(response + 3+3+32+3+32+2+3);
                            received_chunk = buf_to_s_int( response + 3 + 3 + 32 + 3 + 32 );
                            infos_com.have_table[received_chunk].have_fragments[received_fragment] = 1;// marque l'index comme reçu
            //                printf("MESSAGE REÇU \n");
                        }
                        free(response);
                        response = NULL;
                    }
                }
            }
        }
        for(i=0;i<infos_com.nb_seeders;i++)
        {
            free(tab_request[i]);
            if(chunk_complet(infos_com.have_table[infos_com.liste_seeder[i].asked_chunk],first_message[i]) == 1)
            {
                infos_com.have_table[infos_com.liste_seeder[i].asked_chunk].have = 1;
                free(infos_com.have_table[infos_com.liste_seeder[i].asked_chunk].have_fragments);
                printf("chunk %d is received \n",infos_com.liste_seeder[i].asked_chunk);
                printf("j ai reçu variables %d\n",var);
            }
            else
            {
                printf("chunk not received\n");
    //               infos_com.have_table[infos_com.liste_seeder[i].asked_chunk].asked_chunk = -1;
                infos_com.have_table[infos_com.liste_seeder[i].asked_chunk] = set_fragments_not_received(infos_com.have_table[infos_com.liste_seeder[i].asked_chunk],first_message[i]);
            }
            first_message[i] = -1;
        }
        
    }
    //printf("ask for chunk %d\n",table.index);
    //printf("end ask\n");
    
    
    if(pthread_join(send_chunk_thread,NULL) != 0)
    {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }
    if(close(fd) == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    for(i=0;i<table.nb_chunks;i++)
        free(have_table[i].have_fragments);
    for(i=0;i<table.nb_chunks;i++)
        free(table.tab_chunks[i]);
    free(table.tab_chunks);
    free(table.tab_index_chunks);
    free(infos_com.hash);
    close(infos_com.sockfdseeder);
    free(have_table);
    free(infos_com.liste_seeder);
    free(tab_request);
    free(first_message);
    printf("la fin =)\n");
    return 0;
}
