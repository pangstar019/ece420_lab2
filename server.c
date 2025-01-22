#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include "common.h"
#include "timer.h"

char ** theArray;
pthread_mutex_t ** mutexes; // mutices?
int NUM_STR;
double* times;
int times_ind;
pthread_mutex_t* times_mutex;

void *ProcessRequest(void *args) {
    double start, end;
    GET_TIME(start)
    int clientFileDescriptor = *(int*)args;
    char* str = (char*) malloc(COM_BUFF_SIZE * sizeof(char));
    for (int i = 0; i < COM_BUFF_SIZE; i++) str[i] = '\0';
    ClientRequest* rqst = (ClientRequest*) malloc(sizeof(ClientRequest));
    read(clientFileDescriptor, str, COM_BUFF_SIZE);
    if (COM_IS_VERBOSE){
        printf("Server received the request: %s\n", str);
    }
    ParseMsg(str, rqst);
    if (rqst->is_read){
        pthread_mutex_lock(mutexes[rqst->pos]);
        getContent(str, rqst->pos, theArray);
        pthread_mutex_unlock(mutexes[rqst->pos]);
        write(clientFileDescriptor, str, COM_BUFF_SIZE);
    } else {
        pthread_mutex_lock(mutexes[rqst->pos]);
        setContent(rqst->msg, rqst->pos, theArray);
        getContent(str, rqst->pos, theArray);
        pthread_mutex_unlock(mutexes[rqst->pos]);
        // page 3 "the server will first update the string stored at the specified
        // array position with a new client-supplied string and then return the updated string
        // from the same array position back to the client"
        write(clientFileDescriptor, str, COM_BUFF_SIZE); 
    }
    GET_TIME(end);
    pthread_mutex_lock(times_mutex);
    times[times_ind] = end - start;
    times_ind++;
    pthread_mutex_unlock(times_mutex);
    free(args);
    free(str);
    free(rqst);
    close(clientFileDescriptor);
    return NULL;
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <Size of theArray_ on server> <server_ip> <port>\n", argv[0]);
        exit(0);
    }

    struct sockaddr_in sock_var;
    int serverFileDescriptor=socket(AF_INET,SOCK_STREAM,0);
    int clientFileDescriptor;
    int i;
    pthread_t t[COM_NUM_REQUEST];

    NUM_STR = strtol(argv[1], NULL, 10);

    // initialize theArray
    theArray = (char**) malloc(NUM_STR * sizeof(char*));
    for (i = 0; i < NUM_STR; i ++) {
        theArray[i] = (char*) malloc(COM_BUFF_SIZE * sizeof(char));
        sprintf(theArray[i], "String %d: the initial value", i);
    }

    // give each string in theArray a mutex
    mutexes = (pthread_mutex_t**) malloc(NUM_STR * sizeof(pthread_mutex_t*));
    for (i = 0; i < NUM_STR; i++) {
        mutexes[i] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(mutexes[i], NULL);
    }

    // initialize times array and mutex
    times = malloc(COM_NUM_REQUEST * sizeof(double));
    times_ind = 0;
    times_mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(times_mutex, NULL);

    sock_var.sin_addr.s_addr=inet_addr(argv[2]);
    sock_var.sin_port = atoi(argv[3]);
    sock_var.sin_family=AF_INET;
    if(bind(serverFileDescriptor,(struct sockaddr*)&sock_var,sizeof(sock_var))>=0) {
        printf("socket has been created\n");
        listen(serverFileDescriptor,2000); 
        while(1) {
            for(int i = 0; i < COM_NUM_REQUEST; i++) {
                clientFileDescriptor = accept(serverFileDescriptor,NULL,NULL);
                if (clientFileDescriptor < 0) {
                    printf("server acccept failed\n");
                    continue;
                }
                int * args = malloc(sizeof(int));
                *args = clientFileDescriptor;
                printf("Connected to client %d\n",clientFileDescriptor);
                pthread_create(&t[i],NULL,ProcessRequest,(void *)args);
            }
            saveTimes(times, COM_NUM_REQUEST);
            times_ind = 0;
            for (int i = 0; i < COM_NUM_REQUEST; i++) {
                pthread_join(t[i], NULL);
            }
        }
        close(serverFileDescriptor);
    }
    else {
        printf("socket creation failed\n");
    }
    for (i = 0; i < NUM_STR; i++) {
        free(theArray[i]);
    }
    free(theArray);
    for (i = 0; i < NUM_STR; i++) {
        pthread_mutex_destroy(mutexes[i]);
        free(mutexes[i]);
    }
    free(mutexes);
    free(times);
    pthread_mutex_destroy(times_mutex);
    free(times_mutex);
    
    return 0;
}