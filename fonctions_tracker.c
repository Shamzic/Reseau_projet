
char traite_msg ; // traite_msg renvoie le type du message reçu
    

if (traite_msg == 110)
    ack_put()
else if (traite_msg == 112)
    ack_get()
else if (traite_msg == 114)
    ack_get()    
    
    
void ack_put(int sockfd,struct sockaddr_in dest, char * buf,socklen_t addrlen)
{
    buf[0]=111;
    if(sendto(sockfd,buf,strlen(buf),0,(struct sockaddr *)&dest,addrlen) == -1)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
} 
    
    
ack_get(int sockfd,struct sockaddr_in dest, char * buf,socklen_t addrlen,stock_list *sc)
{
    buf[0]=113;
    
    if(sendto(sockfd,buf,strlen(buf),0,(struct sockaddr *)&dest,addrlen) == -1)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

ack_keep_alive(int sockfd,struct sockaddr_in dest, char * buf,socklen_t addrlen)
{
    buf[0]=115;
    if(sendto(sockfd,buf,strlen(buf),0,(struct sockaddr *)&dest,addrlen) == -1)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}
