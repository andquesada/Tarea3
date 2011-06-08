#include "nweb.h"

void print_usage()
{
   int i;

   (void) printf("hint: nweb Port-Number Top-Directory\n\n"
                 "\tnweb is a small and very safe mini web server\n"
                 "\tnweb only servers out file/web pages with extensions named below\n"
                 "\t and only from the named directory or its sub-directories.\n"
                 "\tThere is no fancy features = safe and secure.\n\n"
								 "\tUsage: nweb -p port_number -f /path/to/files -n number_of_threads\n\n"
                 "\tExample: nweb -n 3 -f /home/nwebdir/ -p 3671\n\n"
                 "\tOnly Supports:");

   for (i = 0; extensions[i].ext != 0; i++)
   {
      (void) printf(" %s", extensions[i].ext);
   }

   (void) printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
                 "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
                 "\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n"
                 );
}

void nweb_log(int type,
              char *s1,
              char *s2,
              int num)
{
   int fd;
   char log_buffer[BUFSIZE * 2];

   switch (type) {
      case ERROR:
         (void) sprintf(log_buffer,
                        "ERROR: %s:%s Errno=%d exiting.",
                        s1,
                        s2,
                        errno);
         break;

      case SORRY:
         (void) sprintf(log_buffer,
                        "<HTML><BODY><H1>nweb Web Server Sorry: %s %s</H1></BODY></HTML>\r\n",
                        s1,
                        s2);

         (void) write(num,
                      log_buffer,
                      strlen(log_buffer));

         (void) sprintf(log_buffer,
                        "SORRY: %s:%s",
                        s1,
                        s2);
         break;

      case LOG:
         (void) sprintf(log_buffer,
                        " INFO: %s:%s:%d",
                        s1,
                        s2,
                        num);
         break;
   }

   /* no checks here, nothing can be done a failure anyway */
   if ((fd = open("nweb.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0)
   {
      (void) write(fd,
                   log_buffer,
                   strlen(log_buffer));

      (void) write(fd,
                   "\n",
                   1);

      (void) close(fd);
   }
}

/* this is a child web server process, so we can exit on errors */
void *web(void *p)
{
   bool loop;
   par_web_t *ptr_web;

   int fd;
   int hit;
   int j, file_fd, buf_len, len;
   long i, ret;
   char * fstr;
   static char buffer[BUFSIZE + 1]; /* static so zero filled */

   loop     = true;
   ptr_web  = p;
	 fd				= 0;

   while (loop)
   {
		  //the first thing is try to close the current socket
		  close(fd);

      pthread_mutex_lock(ptr_web->mutex_fd);
      pthread_cond_wait(ptr_web->cond_wait,
                        ptr_web->mutex_fd);

      //the assigned socket handle is copied
      fd = *ptr_web->fd;

      ret = read(fd,
                 buffer,
                 BUFSIZE); /* read Web request in one go */

      if (ret == 0 || ret == -1)
      { /* read failure stop now */
         nweb_log(SORRY,
                  "failed to read browser request",
                  "",
                  fd);

         //put the thread to wait for signal again
         pthread_mutex_unlock(ptr_web->mutex_fd);
         continue;
      }

      if (ret > 0 && ret < BUFSIZE) /* return code is valid chars */
      {
         buffer[ret] = 0; /* terminate the buffer */
      }
      else
      {
         buffer[0] = 0;
      }

      for (i = 0; i < ret; i++) /* remove CF and LF characters */
      {
         if (buffer[i] == '\r' || buffer[i] == '\n')
         {
            buffer[i] = '*';
         }
      }

      nweb_log(LOG,
               "request",
               buffer,
               hit);

      if (strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4))
      {
         nweb_log(SORRY,
                  "Only simple GET operation supported",
                  buffer,
                  fd);

         //put the thread to wait for signal again
         pthread_mutex_unlock(ptr_web->mutex_fd);
         continue;
      }

      for (i = 4; i < BUFSIZE; i++) /* null terminate after the second space to ignore extra stuff */
      {
         if (buffer[i] == ' ')
         { /* string is "GET URL " +lots of other stuff */
            buffer[i] = 0;
            break;
         }
      }

      for (j = 0; j < i - 1; j++) /* check for illegal parent directory use .. */
      {
         if (buffer[j] == '.' && buffer[j + 1] == '.')
         {
            nweb_log(SORRY,
                     "Parent directory (..) path names not supported",
                     buffer,
                     fd);

            //put the thread to wait for signal again
            pthread_mutex_unlock(ptr_web->mutex_fd);
            continue;
         }
      }

      if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6)) /* convert no filename to index file */
      {
         (void) strcpy(buffer,
                       "GET /index.html");
      }

      /* work out the file type and check we support it */
      buf_len  = strlen(buffer);
      fstr     = (char *) 0;
      for (i = 0; extensions[i].ext != 0; i++)
      {
         len = strlen(extensions[i].ext);

         if (!strncmp(&buffer[buf_len - len], extensions[i].ext, len))
         {
            fstr = extensions[i].filetype;
            break;
         }
      }

      if (fstr == 0)
      {
         nweb_log(SORRY,
                  "file extension type not supported",
                  buffer,
                  fd);

         //put the thread to wait for signal again
         pthread_mutex_unlock(ptr_web->mutex_fd);
         continue;
      }

      if ((file_fd = open(&buffer[5], O_RDONLY)) == -1) /* open the file for reading */
      {
         nweb_log(SORRY,
                  "failed to open file",
                  &buffer[5],
                  fd);

         //put the thread to wait for signal again
         pthread_mutex_unlock(ptr_web->mutex_fd);
         continue;
      }

      nweb_log(LOG,
               "SEND",
               &buffer[5],
               hit);

      (void) sprintf(buffer,
                     "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n",
                     fstr);

      (void) write(fd,
                   buffer,
                   strlen(buffer));

      /* send file in 8KB block - last block may be smaller */
      while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
      {
         (void) write(fd, buffer, ret);
      }

#ifdef LINUX
      sleep(1); /* to allow socket to drain */
#endif			
   }
}

int main(int argc,
         char **argv)
{
   int c;

   int num_threads;
   pthread_t **threads;

   int i, port, listen_fd, socket_fd;
   size_t length;
   static struct sockaddr_in cli_addr; /* static = initialised to zeros */
   static struct sockaddr_in serv_addr; /* static = initialised to zeros */

   pthread_cond_t cond_wait;
   pthread_mutex_t mutex_param;
   int fd_param;
   par_web_t param;
   void * ptr_param;

   bool loop;

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //														PARAMETER VALIDATION
   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

   if (argc != MIN_AMOUNT_ARG || !strcmp(argv[1], "-?"))
   {
      print_usage();
      exit(0);
   }

   //if the specified option does not exist, that case is evaluated in the switch
   opterr = 0;
   while ((c = getopt(argc, argv, OPTIONS)) != -1)
   {
      switch (c) {
         case PORT_ARG:
            port = atoi(optarg);

            if (port < 0 || port > 60000)
            {
               nweb_log(ERROR,
                        "Invalid port number (try 1->60000)",
                        optarg,
                        0);
               exit(3);
            }

            break;

         case FOLDER_ARG:
            if (!strncmp(optarg, "/", 2)     || !strncmp(optarg, "/etc", 5) ||
                !strncmp(optarg, "/bin", 5)  || !strncmp(optarg, "/lib", 5) ||
                !strncmp(optarg, "/tmp", 5)  || !strncmp(optarg, "/usr", 5) ||
                !strncmp(optarg, "/dev", 5)  || !strncmp(optarg, "/sbin", 6))
            {
               (void) printf("ERROR: Bad top directory %s, see nweb -?\n",
                             optarg);
               exit(3);
            }
            else if (chdir(optarg) == -1)
            {
               (void) printf("ERROR: Can't Change to directory %s\n",
                             optarg);
               exit(4);
            }

            break;

         case NUM_THREADS_ARG:
            num_threads = atoi(optarg);

            if (num_threads == 0)
            {
               (void) printf("ERROR: Invalid number of threads.\n",
                             optarg);
            }

            break;

         case '?':
            print_usage();
            exit(0);

         default:
            print_usage();
            exit(3);
      }
   }

	 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 //											DEAMONIZATION OF THE SERVER
	 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*
      // Become deamon + unstopable and no zombies children (= no wait())
      if (fork() != 0)
         return 0; // parent returns OK to shell

      //(void) signal(SIGCLD, SIG_IGN); // ignore child death
      //(void) signal(SIGHUP, SIG_IGN); // ignore terminal hangups

      for (i = 0; i < 32; i++)
      {
         (void) close(i); // close open files
      }

      (void) setpgrp(); // break away from process group
    */

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //												WEB SERVER PORT BIND
   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

   nweb_log(LOG,
            "nweb starting",
            argv[1],
            getpid());

   /* setup the network socket */
   if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      nweb_log(ERROR,
               "system call",
               "socket",
               0);
      exit(3);
   }

   serv_addr.sin_family       = AF_INET;
   serv_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
   serv_addr.sin_port         = htons(port);

   if (bind(listen_fd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
   {
      nweb_log(ERROR,
               "system call",
               "bind",
               0);
      exit(3);
   }

   if (listen(listen_fd, 64) < 0)
   {
      nweb_log(ERROR,
               "system call",
               "listen",
               0);
      exit(3);
   }

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //                         THREADS INITIALIZATION
   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

   pthread_cond_init(&cond_wait,
                     NULL);

   pthread_mutex_init(&mutex_param,
                      NULL);

	 param.cond_wait	= &cond_wait;
   param.fd					= &fd_param;
   param.mutex_fd		= &mutex_param;
   ptr_param				= (void *) &param;

   threads = (pthread_t **) malloc(num_threads * sizeof (pthread_t *));
   for (i = 0; i < num_threads; i++)
   {
		  threads[i] = (pthread_t *) malloc(sizeof(pthread_t));

      pthread_create(threads[i],
                     NULL,
                     web,
                     ptr_param);
   }

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

   loop = true;
   while (loop)
   {
      length = sizeof (cli_addr);

      //accept incoming connection requests
      if ((socket_fd = accept(listen_fd, (struct sockaddr *) &cli_addr, &length)) < 0)
      {
         nweb_log(ERROR,
                  "system call",
                  "accept",
                  0);

         continue;
      }

      pthread_mutex_lock(&mutex_param);

      //copy the opened socket handle
      fd_param = socket_fd;

      //wake one thread up
      pthread_cond_signal(&cond_wait);

      pthread_mutex_unlock(&mutex_param);
   }

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //														RESOURCE FREE
   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

   pthread_cond_destroy(&cond_wait);
   pthread_mutex_destroy(&mutex_param);


	 for (i = 0; i < num_threads; i++)
   {

      free(threads[i]);
   }

   free(threads);

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}
