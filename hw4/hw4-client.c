/* hw4client.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void receive_and_echo( int sock, char * buffer )
{
  int n = recv( sock, buffer, BUFFER_SIZE, 0 );

  if ( n < 0 )
  {
    perror( "recv() failed" );
  }
  else if ( n == 0 )
  {
    printf( "Server disconnected\n" );
    fflush( stdout );
  }
  else
  {
    buffer[n] = '\0';
    printf( "Received [%s]\n", buffer );
    fflush( stdout );
  }
}


void send_store( int sock, char * filename, int size, char * buffer )
{
  sprintf( buffer, "STORE %s %d\n", filename, size );
  printf("Sent [%s]", buffer);
  int fd = open( filename, O_RDONLY );

  if ( fd == -1 )
  {
    perror( "open() failed" );
    exit( EXIT_FAILURE );
  }

  int i = strlen( buffer );
  int k, n;
  int bytes = i + size;

  if ( bytes <= BUFFER_SIZE )
  {
    k = read( fd, buffer + i, size );

    if ( k == -1 )
    {
      perror( "read() failed" );
      exit( EXIT_FAILURE );
    }

    close( fd );

    n = send( sock, buffer, bytes, 0 );
    fflush( NULL );

    if ( n != bytes )
    {
      perror( "send() failed" );
      exit( EXIT_FAILURE );
    }
  }
  else
  {
    n = send( sock, buffer, i, 0 );
    fflush( NULL );

    if ( n != i )
    {
      perror( "send() failed" );
      exit( EXIT_FAILURE );
    }

    while ( size > 0 )
    {
      k = read( fd, buffer, ( size < BUFFER_SIZE ? size : BUFFER_SIZE ) );

      if ( k == -1 )
      {
        perror( "read() failed" );
        exit( EXIT_FAILURE );
      }

      size -= k;

      n = send( sock, buffer, k, 0 );
      fflush( NULL );

      if ( n != k )
      {
        perror( "send() failed" );
        exit( EXIT_FAILURE );
      }
    }

    close( fd );
  }
}


void send_read( int sock, char * filename, int offset, int length, char * buffer )
{
  sprintf( buffer, "READ %s %d %d\n", filename, offset, length );
  printf("Sent [%s]", buffer);
  
  int bytes = strlen( buffer );

  int n = send( sock, buffer, bytes, 0 );
  fflush( NULL );

  if ( n != bytes )
  {
    perror( "send() failed" );
    exit( EXIT_FAILURE );
  }
}


void send_list( int sock, char * buffer )
{
  sprintf( buffer, "LIST\n" );
  printf("Sent [%s]", buffer);
  
  int bytes = strlen( buffer );

  int n = send( sock, buffer, bytes, 0 );
  fflush( NULL );

  if ( n != bytes )
  {
    perror( "send() failed" );
    exit( EXIT_FAILURE );
  }
}


int main( int argc, char * argv[] )
{
  if ( argc != 4 )
  {
    fprintf( stderr, "ERROR: Invalid arguments\n" );
    fprintf( stderr, "USAGE: %s <server-hostname> <listener-port> <test-case>\n", argv[0] );
    return EXIT_FAILURE;
  }

  /* create TCP client socket (endpoint) */
  int sock = socket( PF_INET, SOCK_STREAM, 0 );

  if ( sock < 0 )
  {
    perror( "socket() failed" );
    exit( EXIT_FAILURE );
  }

  struct hostent * hp = gethostbyname( argv[1] );
  if ( hp == NULL )
  {
    perror( "gethostbyname() failed" );
    exit( EXIT_FAILURE );
  }

  struct sockaddr_in server;
  server.sin_family = PF_INET;
  memcpy( (void *)&server.sin_addr, (void *)hp->h_addr,
          hp->h_length );
  unsigned short port = atoi( argv[2] );
  server.sin_port = htons( port );

  printf( "Server address is %s\n", inet_ntoa( server.sin_addr ) );
  fflush( stdout );

  if ( connect( sock, (struct sockaddr *)&server,
                sizeof( server ) ) < 0 )
  {
    perror( "connect() failed" );
    exit( EXIT_FAILURE );
  }

  printf( "Connected to the server\n" );
  fflush( stdout );


  int testcase = atoi( argv[3] );

  char buffer[ BUFFER_SIZE ];

  if ( testcase == 1 )
  {
    send_store( sock, "mouse.txt", 917, buffer );
    receive_and_echo( sock, buffer );  /* "ACK\n" */
    send_read( sock, "xyz.jpg", 5555, 2000, buffer );
    receive_and_echo( sock, buffer );  /* "ERROR NO SUCH FILE\n" */
    send_list( sock, buffer );
    receive_and_echo( sock, buffer );  /* "1 mouse.txt\n" */
  }
  else if ( testcase == 2 )
  {
    send_store( sock, "mouse.txt", 917, buffer );
    receive_and_echo( sock, buffer );  /* "ERROR FILE EXISTS\n" */
    send_store( sock, "legend.txt", 70672, buffer );
    receive_and_echo( sock, buffer );  /* "ACK\n" */
    send_store( sock, "chicken.txt", 31, buffer );
    receive_and_echo( sock, buffer );  /* "ACK\n" */
    send_list( sock, buffer );
    receive_and_echo( sock, buffer );  /* "3 chicken.txt legend.txt mouse.txt\n" */
    send_read( sock, "chicken.txt", 4, 5, buffer );
    receive_and_echo( sock, buffer );  /* "ACK 5\n" */
    receive_and_echo( sock, buffer );  /* "quick" */
    send_read( sock, "legend.txt", 50092, 39, buffer );
    receive_and_echo( sock, buffer );  /* "ACK 39\n" */
    receive_and_echo( sock, buffer );  /* "broken rocks and trunks of fallen trees" */
  }
  else if ( testcase == 3 )
  {
    send_store( sock, "chicken.txt", 31, buffer );
    receive_and_echo( sock, buffer );  /* "ERROR FILE EXISTS\n" */
    send_store( sock, "sonny1978.jpg", 100774, buffer );
    receive_and_echo( sock, buffer );  /* "ACK\n" */
    send_list( sock, buffer );
    receive_and_echo( sock, buffer );  /* "4 chicken.txt legend.txt mouse.txt sonny1978.jpg\n" */
    send_read( sock, "sonny1978.jpg", 920, 11, buffer );
    receive_and_echo( sock, buffer );  /* "ACK 11\n" */
    receive_and_echo( sock, buffer );  /* "Cocoa Puffs" */
    send_read( sock, "sonny1978.jpg", 95898, 3, buffer );
    receive_and_echo( sock, buffer );  /* "ACK 3\n" */
    receive_and_echo( sock, buffer );  /* "Yum" */
  }
  else if ( testcase == 4 )
  {
    send_store( sock, "mouse.txt", 917, buffer );
    receive_and_echo( sock, buffer );  /* "ERROR FILE EXISTS\n" */
    send_store( sock, "chicken.txt", 31, buffer );
    receive_and_echo( sock, buffer );  /* "ERROR FILE EXISTS\n" */
    send_store( sock, "ospd.txt", 614670, buffer );
    receive_and_echo( sock, buffer );  /* "ACK\n" */
    send_read( sock, "ospd.txt", 104575, 26, buffer );
    receive_and_echo( sock, buffer );  /* "ACK 26\n" */
    receive_and_echo( sock, buffer );  /* "coco\ncocoa\ncocoanut\ncocoas" */
    send_list( sock, buffer );
    receive_and_echo( sock, buffer );  /* "5 chicken.txt legend.txt mouse.txt ospd.txt sonny1978.jpg\n" */
  }

  close( sock );

  return EXIT_SUCCESS;
}