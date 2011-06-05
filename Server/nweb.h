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

#define BUFSIZE 8096
#define ERROR 42
#define SORRY 43
#define LOG   44

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

#endif	/* NWEB_H */

