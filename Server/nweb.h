/* 
 * File:   nweb.h
 * Author: Andrés Quesada
 *
 * Created on June 5, 2011, 5:22 PM
 */

#ifndef NWEB_H
#define	NWEB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>

#define BUFSIZE         8096
#define ERROR           42
#define SORRY           43
#define LOG             44
#define OPTIONS         "p:f:n:h::"
#define PORT_ARG        'p'
#define FOLDER_ARG      'f'
#define NUM_THREADS_ARG 'n'

struct {
   char *ext;
   char *filetype;
} extensions [] = {
   {"gif", "image/gif"},
   {"jpg", "image/jpeg"},
   {"jpeg", "image/jpeg"},
   {"png", "image/png"},
   {"zip", "image/zip"},
   {"gz", "image/gz"},
   {"tar", "image/tar"},
   {"htm", "text/html"},
   {"html", "text/html"},
   {0, 0}
};

typedef struct par_web_s {
   pthread_cond_t *cond_wait;
   pthread_mutex_t *mutex_fd;
   int *fd;
} par_web_t;

void *web(void *params);
void nweb_log(int type, char *s1, char *s2, int num);
void print_usage();

#endif	/* NWEB_H */