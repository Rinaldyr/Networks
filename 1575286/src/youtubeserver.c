#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_len = sizeof (struct sockaddr_in);
    int fd_server, fd_client;
    char buf[2048], *webpage = NULL, website[2048];
    int on = 1;
    
    FILE *fp = fopen("index.html", "r");
    if (fp) {
        if (fseek(fp, 0L, SEEK_END) == 0) {
            long bufsize = ftell(fp);
            if (bufsize == -1) { 
                perror("Error\n"); 
            }
            webpage = malloc(sizeof(char) * (bufsize + 1));
            if (fseek(fp, 0L, SEEK_SET) != 0) { 
                perror("Error fseek\n"); 
            }
            size_t newLen = fread(webpage, sizeof(char), bufsize, fp);
            if ( ferror(fp) != 0 ) {
                fputs("error reading file", stderr);
            } else {
                webpage[newLen++] = '\0';
            }
        }
        fclose(fp);
    } 
    printf("%s\n", webpage);
    website = webpage;
    fd_server = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_server < 0) {
        perror("socket");
        exit(1);
    }
    
    setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    
    if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(fd_server);
        exit(1);
    }
    
    if (listen(fd_server, 10) == -1) {
        perror("listen");
        close(fd_server);
        exit(1);
    }
    
    while(1) {
        fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);
        if (fd_client == -1) {
            perror("Connection failed...\n");
            continue;
        }
        printf("Got client connection...\n");
        
        if (!fork()) {
            close(fd_server);
            memset(buf, '\0', 2048);
            read(fd_client, buf, 2047);
            printf("%s\n", buf);
            write(fd_client, website, sizeof(website) -1);
            close(fd_client);
            printf("closing...\n");
            fclose(fp);
            exit(0);
        }
        close(fd_client);
    }
    free(webpage);
    return 0;
}
