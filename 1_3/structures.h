

// structures du client

// structures vers le tracker 
typedef struct s_client
{
    short int length; // 6 pour IPV4, 16 pour IPV6
    short int port;
    char address_ip [16];
    time_t time;
    struct s_client * next;
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
    hash_c chunk_h;
} message_get;
// message REP_GET
typedef struct s_message_rep_get
{
    char type;
    short int length;
    hash_f file_h;
    hash_c chunk_h;
    char * chunk;
} message_rep_get;

// message REP_LIST
typedef struct s_message_rep_list
{
    char type;
    short int length;
    hash_f file_h;
    hash_c * chunk_list;
} message_rep_list;

typedef struct infos_s
{
    int sockfd_tracker;
    int sockfdtarget; // fd qui sert à faire les sockets
    int sockfdmy;
    struct sockaddr_in target;
    struct sockaddr_in tracker;
    struct sockaddr_in my_addr;
    socklen_t addrlen_s;
    socklen_t addrlen_c;
    char * addr_dest;
    char * addr_tracker;
    unsigned short int port_listen;
    unsigned short int port_tracker;
    unsigned short int port_dest;
    client_s *liste_clients;
    unsigned char * hash;
    unsigned char ** tab_chunks;
    int * tab_index_chunks;
    int nb_chunks;
    unsigned char * filename;
    unsigned char * dest_file;
} infos;
typedef struct s_tab_chunks
{
    short int nb_chunks;
    unsigned char ** tab_chunks;
    short int * tab_index_chunks;
    short int index;
} table_chunks;
