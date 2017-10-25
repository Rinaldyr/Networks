#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include <memory.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <math.h>
#include "service_client_socket.h"

/* why can I not use const size_t here? */
#define buffer_size 1024

char * 
fileToStr(char *file) {
  FILE *fp = fopen(file, "r");
  if (!fp) {
    return NULL;
  }
  char *buffer; 
  long lSize;
  fseek(fp, 0L, SEEK_END);
  lSize = ftell(fp);
  rewind(fp);
  
  buffer = calloc(1, lSize+1);
  if (!buffer) {
    fclose(fp);
    fputs("memory alloc fails", stderr);
    exit(1);
  }
  if (1 != fread(buffer, lSize, 1, fp)) {
    fclose(fp);
    free(buffer);
    fputs("entire read fails", stderr);
    exit(1);
  }  
  buffer[lSize] = '\0';
  fclose(fp);

  return buffer;
}

char * route(char * page) {
  if (*page == '\0') page = "index.html";
  
  char * webpage = fileToStr(page);
  char * website = malloc(buffer_size);
  if (!webpage) {
    webpage = fileToStr("404.html");
  }
  char header[] = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: ";
  size_t weblen = strlen(webpage);
  sprintf(website, "%s%u\r\n\r\n%s", header, weblen, webpage);
  free(webpage);
  
  return website;
}

int
service_client_socket (const int s, const char *const tag) {
  char buffer[buffer_size];
  bzero(buffer, buffer_size);
  size_t bytes;

  printf ("new connection from %s\n", tag);

  /* repeatedly read a buffer load of bytes, leaving room for the
     terminating NUL we want to add to make using printf() possible */
  
  while (1) {
    if ((bytes = read (s, buffer, buffer_size - 1)) < 0) {
      fprintf(stderr, "Failed to read\n");
      return 1;
    }
    /* this code is not quite complete: a write can in this context be
       partial and return 0<x<bytes.  realistically you don't need to
       deal with this case unless you are writing multiple megabytes */
    printf("Line70 BUFFER: %s", buffer);
    if (buffer[i])
    char page[buffer_size];
    bzero(page, buffer_size);
    int at, i;
    for (at = 0; at < buffer_size && buffer[at] != ' '; at++);
    for (i = 0, at += 2; at < buffer_size && buffer[at] != ' '; page[i++] = buffer[at++]); 
    page[i] = '\0';
    
    char * requestedPage = route(page);
    if (!requestedPage) {
      perror("Page doesn't exist");
      return -1;
    }
    write (s, requestedPage, strlen(requestedPage));
      
    /* NUL-terminal the string */
    buffer[bytes] = '\0';
    /* special case for tidy printing: if the last two characters are
       \r\n or the last character is \n, zap them so that the newline
       following the quotes is the only one. */
    if (bytes >= 1 && buffer[bytes - 1] == '\n') {
      if (bytes >= 2 && buffer[bytes - 2] == '\r') {
	      strcpy (buffer + bytes - 2, "..");
      } else {
	      strcpy (buffer + bytes - 1, ".");
      }
    }
    
#if (__SIZE_WIDTH__ == 64 || __SIZEOF_POINTER__ == 8)
    printf ("echoed %ld bytes back to %s, \"%s\"\n", bytes, tag, buffer);
#else
    printf ("echoed %d bytes back to %s, \"%s\"\n", bytes, tag, buffer);
#endif
  }
  /* bytes == 0: orderly close; bytes < 0: something went wrong */
  if (bytes != 0) {
    perror ("read");
    return -1;
  }
  
  printf ("connection from %s closed\n", tag);
  close (s);
  return 0;
}

