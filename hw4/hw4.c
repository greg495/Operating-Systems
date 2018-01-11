// Greg McCutcheon

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include <sys/select.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>

#include <signal.h>
#include <ctype.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct //Struct for passing arguments to thread function
{
  int sock;
} client_t;

typedef struct
{
  char name[20]; //Filename
  int size; //Size of file
} file_type;

const char dir_name[6] = "files"; //The directory where it will store the files
file_type files[20]; //Stores the names of the files that were added
int num_files = 0; //Keeps track of number of files
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void * client_thread(void * arg) //Function called by pthread_create each time a new client comes
{
  client_t* client = (client_t*) arg;
  int newsock = client->sock;
  int n = 0, k = 0;
  char* full_command; //String for passing buffer to string function
  unsigned short tid = (unsigned short) pthread_self();
  
  do //Continues recieving messages until client terminates
  {
    /* can also use read() and write()..... */
    char buffer[ BUFFER_SIZE ];
    fflush(NULL);
    n = recv( newsock, buffer, BUFFER_SIZE, 0 );
    fflush(NULL);
    if ( n < 0 ){
      printf( "recv() failed\n" );
      fflush(NULL);
    }
    else      /***Reading the command happens here***/
    {
      buffer[n] = '\0';  /* assuming text.... */
      
      full_command = (char*) calloc(n+1, sizeof(char));
      memcpy(full_command, buffer, n+1); //Converts char array to char * for using string operations
      
      char* filename = (char*) calloc(20, sizeof(char));
      int size = 0, offset = 0;
      char* temp_str = (char*) calloc(50, sizeof(char));
      
      char * pch;
      pch = strchr(buffer,'\n');
      int loc = 0;
      while (pch!=NULL)
      {
          loc = pch-buffer+1;
          break;
      }
      
      //split the array beforehand
      char command[loc];
      int lpe = 0;
      for(lpe = 0; lpe < loc - 1;lpe++){
      	command[lpe] = buffer[lpe];
      }

      command[loc] = '\0';

      k = sscanf(full_command, "%s %s", command, temp_str); //Gets the type of command: "STORE", "LIST", "READ", or garbage
      
      if (strcmp(command, "STORE") == 0){
        temp_str = (char*) calloc(70, sizeof(char));
        //Format is: STORE <filename> <bytes>\n<file-contents>
        k = sscanf(full_command, "STORE %s %d\n", filename, &size); //Ignore the command type, since we already have that. We'll do the file contents below.
        if (k != 2){
          printf("%s %d\n", filename, size);
          fflush(NULL);
          temp_str = "ERROR STORE HAS THESE ARGUMENTS\nSTORE <filename> <bytes>\n<file-contents>\n";
          k = send( newsock, temp_str, strlen(temp_str), 0 );
          printf("[child %u] Sent ARGUMENT ERROR\n", tid);
          fflush( NULL );
          if ( k != strlen(temp_str) ){
            perror( "send() failed" );
          }
          continue;
        }
        printf( "[child %u] Received STORE %s %d\n", tid, filename, size ); //Newline added automatically
        fflush( stdout );
        
        for (k = 0; k < num_files; k++){
          if (strcmp(files[k].name, filename) == 0){
            offset = 1; //Marks that the file already exists
          }
        }
        if (offset == 1){
          temp_str = "ERROR FILE EXISTS\n";
          n = send( newsock, temp_str, strlen(temp_str), 0 );
          printf("[child %u] Sent %s", tid, temp_str);
          fflush( NULL );
          if ( n != strlen(temp_str) ){
            perror( "send() failed" );
          }
          continue;
        }
        char* file_t = filename + strlen(filename) - 4;
        if ((size < 0) || ( (strcmp(file_t, ".txt") != 0) && (strcmp(file_t, ".jpg") != 0) )){
          //If the size or file type is invalid
          temp_str = "ERROR INVALID REQUEST\n";
          n = send( newsock, temp_str, strlen(temp_str), 0 );
          printf("[child %u] Sent %s\n", tid, temp_str);
          fflush( NULL );
          if ( n != strlen(temp_str) ){
            perror( "send() failed" );
          }
          continue;
        }

        strcpy(files[num_files].name, filename);
        files[num_files].size = size;
        num_files++;
        
        //After newline, receive the next packet
        sprintf(temp_str, "%s/%s", dir_name, filename); //Allows storing the file in the new directory
        
        pthread_mutex_lock( &mutex );
        FILE* file = fopen(filename, "wb");
        if (file == NULL){
          fprintf(stderr, "fopen(\"%s\") failed, %s\n", temp_str, strerror(errno));
          exit(EXIT_FAILURE);
        }
        else{
          fflush(NULL);
        }
        fwrite(buffer + loc, sizeof(char), n - loc, file); //Write the first packet to the file
        //offset is now the number of bytes that have been read
        offset = n - loc+1;
          
        while (offset < size) //If the file is larger than the given buffer, break into multiple packets
        {
          fflush(NULL);
          n = recv(newsock, buffer, ( size-offset < BUFFER_SIZE ? size-offset : BUFFER_SIZE ), 0 );
          fflush(NULL);
          if ( n < 0 ){
            perror( "recv() failed" );
          }
          fwrite(buffer, sizeof(char), n, file);
          offset += n;
        }
        
        fclose(file);
        pthread_mutex_unlock( &mutex );
        
        
        printf("[child %u] Stored file \"%s\" (%d bytes)\n", tid, filename, size);
        fflush( stdout );
        
        /* send ack message back to the client */
        n = send( newsock, "ACK\n", 4, 0 );
        fflush( NULL );
        if ( n != 4 ){
          perror( "send() failed" );
          fflush(NULL);
        }
        else{
          printf("[child %u] Sent ACK\n", tid);
          fflush(NULL);
        }
      }
      else if (strcmp(command, "LIST") == 0){
        temp_str = (char*) calloc(50, sizeof(char));
        printf("[child %u] Received LIST\n", tid);
        fflush( stdout );
        if (k > 1){
          temp_str = "ERROR LIST TAKES NO ARGUMENTS\n";
          n = send( newsock, temp_str, strlen(temp_str), 0 );
          printf("[child %u] Sent ARGUMENT ERROR\n", tid);
          fflush( NULL );
          if ( n != strlen(temp_str) ){
            perror( "send() failed" );
          }
          continue;
        }
        pthread_mutex_lock( &mutex );
        
        //Print the files in alpahbetical order:
        //Have to concatenate and print the largest first, because Linux is little endian
        int m = 0; //Counter for number of sorted files
        char files_left[num_files][50]; //Stores the names of the unsorted files
        int largest; //Index of the running largest in files_left
        char empty[6] =  "zzzzz"; //What qualifies as empty
        for(n = 0; n < num_files; n++){ //Initializes it to all the files
          strcpy(files_left[n], files[n].name);
        }
        
        char* temp_str2 = (char*) calloc(50, sizeof(char));
        for (m = 0; m < num_files; m++){
          largest = 0;
          for(n = 0; n < num_files; n++){
            if ((strcmp(files_left[n], empty) != 0) && (strcmp(files_left[largest], files_left[n]) > 0)){ 
              //If this is not empty and is smaller than the running largest, it is the new largest
              largest = n;
            }
          }
          sprintf(temp_str2, " %s", files_left[largest]);
          strcat(temp_str, temp_str2); //strcat apparently appends to the front...most annoying
          strcpy(files_left[largest], empty);
        }
        
        pthread_mutex_unlock( &mutex );
        
        sprintf(temp_str2, "%d%s\n", n, temp_str);
        n = send( newsock, temp_str2, strlen(temp_str2), 0 );
        if ( n < 0 ){
          perror( "send() failed" );
        }  
        
        printf("[child %u] Sent %s", tid, temp_str2);
        fflush(NULL);
      }
      else if (strcmp(command, "READ") == 0){
        char* contents = (char*) calloc(BUFFER_SIZE, sizeof(char));
        temp_str = (char*) calloc(100, sizeof(char));
        //Format is: READ <filename> <byte-offset> <length>\n
        n = sscanf(full_command, "READ %s %d %d\n", filename, &offset, &size);
        if (n != 3){ //Error check
          temp_str = "ERROR READ HAS THESE ARGUMENTS\nREAD <filename> <byte-offset> <length>\n";
          n = send( newsock, temp_str, strlen(temp_str), 0 );
          printf("[child %u] Sent ARGUMENT ERROR\n", tid);
          fflush( NULL );
          if ( n != strlen(temp_str) ){
            perror( "send() failed" );
          }
          continue;
        }
        printf("[child %u] Received READ %s %d %d\n", tid, filename, offset, size);
        fflush( stdout );
        printf("[child %u] Sent ACK %d\n", tid, size);
        fflush(NULL);
        
        char* file_t = filename + strlen(filename) - 4;
        if ((size < 0) || ( (strcmp(file_t, ".txt") != 0) && (strcmp(file_t, ".jpg") != 0) )){
          //If the size or file type is invalid
          temp_str = "ERROR INVALID REQUEST\n";
          n = send( newsock, temp_str, strlen(temp_str), 0 );
          printf("[child %u] Sent %s\n", tid, temp_str);
          fflush( NULL );
          if ( n != strlen(temp_str) ){
            perror( "send() failed" );
          }
          continue;
        }
        
        for (n = 0; n < num_files; n++){ //Search for the file name
          if (strcmp (files[n].name, filename) == 0){
            break;
          }
        }
        
        if (strcmp(files[n].name, filename) == 0){ //If the search was successful
          FILE* file = fopen(filename, "rb");
          if (file == NULL){
            fprintf(stderr, "fopen(\"%s\") failed, %s\n", temp_str, strerror(errno));
            exit(EXIT_FAILURE);
          }
          if (offset + size > files[n].size){
            temp_str = "ERROR INVALID BYTE RANGE\n";
            n = send( newsock, temp_str, strlen(temp_str), 0 );
            printf("[child %u] Sent ERROR INVALID BYTE RANGE\n", tid);
            fflush( NULL );
            if ( n != strlen(temp_str) ){
              perror( "send() failed" );
            }
            continue;
          }
          
          pthread_mutex_lock( &mutex );
          sprintf(buffer, "ACK %d\n", size);
          
          //Send the data back, assuming it's less than the buffer size:
          fseek(file, offset, SEEK_SET); //Sets file pointer to correct place
          fgets(contents, size, file);
          n = strlen(buffer);
          strcpy(buffer+n, contents);
          n = send( newsock, buffer, n+size+1, 0 );
          fflush(NULL);
            if ( n < 0 ){
              perror( "send() failed" );
          }
            
          fflush(NULL);
          if ( n < 0 ){
              perror( "send() failed" );
              fflush(NULL);
          }
          
          pthread_mutex_unlock( &mutex );
        
          fclose(file);
          printf("[child %u] Sent %d bytes of \"%s\" from offset %d\n", tid, size, filename, offset);
          fflush(NULL);
        }
        else{
          n = send( newsock, "ERROR NO SUCH FILE\n", 19, 0 );
          if ( n != 19 ){
            perror( "send() failed" );
          }
          printf("[child %u] Sent ERROR NO SUCH FILE\n", tid);
          fflush( NULL );
        }
      }
      else if (strcmp(command, "\0") != 0){ //Invalid Command, ignoring blank ones
        sprintf(temp_str, "ERROR INVALID COMMAND \"%s\"--Try STORE, LIST, or READ\n", command);
        n = send( newsock, temp_str, strlen(temp_str), 0 );
        fflush(NULL);
        printf("[child %u] %s\n", tid, temp_str);
        fflush( NULL );
        if ( n != strlen(temp_str) ){
          printf( "send() failed" );
          fflush(NULL);
        }
        
      }
    }
    fflush(NULL);
  }
  while ( n > 0 );
  
  printf( "[child %u] Client disconnected\n", tid );
  fflush( NULL );
  close( newsock );
  return NULL;
}


int main(int argc, char * argv[])
{
  if (argc != 2){
    perror("Incorrect arguments. Also enter the port number.");
    exit(EXIT_FAILURE);
  }
  
  int rc = mkdir(dir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); //Create directory for the files, if it does not already exist

  DIR* FD;
  struct dirent* in_file;
        
	if (NULL == (FD = opendir (dir_name))) 
  {
    perror( "opendir() failed" );
    exit(EXIT_FAILURE);
  }
  chdir(dir_name);
        
  //Search through files from directory
  while ((in_file = readdir(FD))){
    if (!strcmp (in_file->d_name, "."))
      continue;
    if (!strcmp (in_file->d_name, ".."))    
      continue;
    strcpy(files[num_files].name, in_file->d_name);
    num_files++;
  }
  
  unsigned short port_number = atoi(argv[1]);
  int sd = socket( PF_INET, SOCK_STREAM, 0 );
  if ( sd < 0 )
  {
    perror( "socket() failed" );
    exit( EXIT_FAILURE );
  }

  
  printf("Started server; listening on port: %u\n", port_number);
  fflush( stdout );
  
  /* socket structures */
  struct sockaddr_in server;

  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  
  server.sin_port = htons( port_number );
  int len = sizeof( server );

  if ( bind( sd, (struct sockaddr *)&server, len ) < 0 )
  {
    perror( "bind() failed" );
    exit( EXIT_FAILURE );
  }

  listen( sd, 5 );   /* 5 is the max number of waiting clients */

  struct sockaddr_in client;
  int fromlen = sizeof( client );

  pthread_t tid;
  client_t* new_client = (client_t*)malloc(sizeof(client_t));
    
  while(1) //Continues accepting new clients until program is terminated
  {
    new_client->sock = accept( sd, (struct sockaddr *)&client, (socklen_t*)&fromlen );
    printf("Received incoming connection from: %s\n", inet_ntoa( client.sin_addr ));
    fflush( stdout );
    rc = pthread_create(&tid, NULL, client_thread, (void *)new_client);
  
    if ( rc != 0 )
  	{
  	  fprintf( stderr, "pthread_create() failed (%d): %s\n", rc, strerror( rc ) );
  	  return EXIT_FAILURE;
  	}
  }  	
  close(sd);
  return EXIT_SUCCESS;
}
