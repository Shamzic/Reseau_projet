#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
 
extern int errno;
 
void handle_error(const char * msg) {
    perror(msg);
    exit(1);
}
 
void * thread_fct(void * arg) {
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s,SIGINT);
    if ((errno=pthread_sigmask(SIG_BLOCK,&s,NULL)) < 0) {
        handle_error("sigmask");
    }
    if ((errno=pthread_barrier_wait(arg)) > 0 ) {
        handle_error("barrier wait");
    }
    pause();
     
    return NULL;
}
 
void sighandler(int sig) {
    static int i = 1;
    if (sig == SIGINT) {
         
    } else if (sig == SIGUSR1) {
        printf("exit thread %d\n",i);
        i++;
        pthread_exit(NULL);
    }
}
 
int main(int argc, char ** argv) {
    struct sigaction s;
    int n, i;
    pthread_t * pid;
    pthread_barrier_t barriere;
     
    if (argc !=2) {
        fprintf(stderr,"usage: %s n\n",argv[0]);
        exit(1);
    }
    n= atoi(argv[1]);
    assert(n>0);
    pid = malloc(n*sizeof(pthread_t));
     
    s.sa_handler = sighandler;
    s.sa_flags = 0;
    sigemptyset(&s.sa_mask);
    sigaddset(&s.sa_mask,SIGUSR1);
     
    if (sigaction(SIGINT,&s,NULL) == -1) {
        handle_error("sigaction");
    }
    if (sigaction(SIGUSR1,&s,NULL) == -1) { 
        handle_error("sigaction");
    }
    if (pthread_barrier_init(&barriere,NULL,n+1) > 0) {
        handle_error("init barriere");
    }
     
    for (i=0; i<n; i++) {
        if ((errno=pthread_create(&pid[i],NULL, thread_fct, &barriere)) >0) {
            handle_error("thread creation");
        } 
    }
    if ((errno=pthread_barrier_wait(&barriere))>0) {
        handle_error("barrier wait");
    }
    if ((errno=pthread_barrier_destroy(&barriere)) >0 ) {
        handle_error("barrier destroy");
    }
     
    i=0;
    while (i<n) {
        pause();
        pthread_kill(pid[i],SIGUSR1);
        if ((errno=pthread_join(pid[i],NULL)) >0) {
            handle_error("pthread join");
        } 
        i++;
    }
     
    free(pid);
    return 0;
}
