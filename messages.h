#ifndef MESSAGES_H
#define MESSAGES_H

unsigned char *  create_message_get_peer(unsigned char * hash_file,unsigned char * hash_chunk, short int index);

unsigned char * create_message_rep_get(unsigned char * hash_file,unsigned char * hash_chunk,char * chunk,short int index, short int max_index,short int nb_cars_lus);

unsigned char* create_message_list(unsigned char * hash_file);

unsigned char * create_message_put(unsigned char * hash_file,char IP_TYPE, char * address,unsigned short int port);

unsigned char * create_message_get(unsigned char * hash_file,char IP_TYPE, char * address,unsigned short int port);

unsigned char * create_message_keep_alive(unsigned char * hash_file);

void send_packet (unsigned char* message,int sockfd,struct sockaddr_in addr);

unsigned char * send_msg_tracker(infos infos_com,unsigned char * hash, char * action);

int u_strncmp(unsigned char * string1,unsigned char * string2, int n);

int test_response_tracker(infos infos_com , char * action, unsigned char * message);

unsigned char * communicate_tracker(infos infos_com, unsigned char * hash, char * action);

int u_strncmp(unsigned char * string1,unsigned char * string2, int n);

int u_strlen ( unsigned char * string);

int buf_to_int ( unsigned char * buf);

short int buf_to_s_int ( unsigned char * buf);

unsigned char * us_int_to_buf(unsigned char * buf,unsigned short int i,int begin);

unsigned char * s_int_to_buf(unsigned char * buf,short int i,int begin);

unsigned char * int_to_buf(unsigned char * buf,int i,int begin);

int test_rep( char * action, unsigned char * msg_send, unsigned char * msg_rcv);

void print_hash(unsigned char hash[]);

unsigned char * hash_to_char( char* hash);

#endif
