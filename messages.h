#ifndef MESSAGES_H
#define MESSAGES_H

unsigned char* create_message_get_peer(unsigned char * hash_file,unsigned char * hash_chunk);

unsigned char* create_message_rep_get(unsigned char * hash_file,unsigned char * hash_chunk,char * chunk,short int index, short int max_index);

unsigned char* create_message_list(unsigned char * hash_file);

unsigned char * create_message_put(unsigned char * hash_file,char IP_TYPE, char * address,unsigned short int port);

unsigned char * create_message_get(unsigned char * hash_file,char IP_TYPE, char * address,unsigned short int port);

unsigned char * create_message_keep_alive(unsigned char * hash_file);

void send_packet (infos infos_com, unsigned char* message);

unsigned char * send_msg_tracker(infos infos_com,unsigned char * hash, char * action);

int u_strncmp(unsigned char * string1,unsigned char * string2, int n);

int test_response_tracker(infos infos_com , char * action, unsigned char * message);

unsigned char * communicate_tracker(infos infos_com, unsigned char * hash, char * action);


#endif
