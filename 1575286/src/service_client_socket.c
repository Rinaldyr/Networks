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
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include "service_client_socket.h"

#define OK 0
#define ERROR 1
#define buffer_size 1024

// page names as variables
char page_index[] = "index.html";
char page_first[] = "firstpage.html";
char page_error[] = "404.html";

// response codes
char response_code_200[] = "HTTP/1.1 200 OK";
char response_code_206[] = "HTTP/1.1 206 Partial Content";
char response_code_404[] = "HTTP/1.1 404 Not Found";
char response_code_416[] = "HTTP/1.1 416 Range Not Satisfiable";

// contents
char content_type[] = "Content-Type: text/html; charset=UTF-8";
char content_length[] = "Content-Length: ";
char content_range[] = "Content-Range: bytes ";

// error switches
int error_404 = 0; // 1 if there is a Not Found error
int error_416 = 0; // 1 if there is a Range Not Satisfiable error

// functions
char *fileToStr(char *file);                                   // returns the string of a file
char *route(char *page);                                       // returns a response from the server
char *grabRange(char *page, int start, int end);               // returns a ranged response from the server
char *make_header(int code,                                    // returns a header based on its code
                  int c_length,                                // content length
                  int r_start, int r_end, int p_length);       // used for range requests. start, end and length of page
int validRange(int length, int start, int end);                // checks whether a given range is valid
int service_client_socket(const int s, const char *const tag); // services the client with its request

char *fileToStr(char *file) {
    FILE *fp = fopen(file, "r");
    if (!fp) {
        return NULL;
    }
    char *buffer;
    long lSize;
    fseek(fp, 0L, SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    buffer = calloc(1, lSize + 1);
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

char *make_header(int code, int c_length, int r_start, int r_end, int p_length) {
    char *header = malloc(buffer_size);
    switch (code) {
        case 200: {
            sprintf(header, "%s\r\n%s\r\n%s%d",
                    response_code_200,
                    content_type,
                    content_length, c_length);
            break;
        }
        case 206: {
            sprintf(header, "%s\r\n%s\r\n%s%d-%d/%d\r\n%s%d",
                    response_code_206,
                    content_type,
                    content_range, r_start, r_end, p_length,
                    content_length, c_length);
            break;
        }
        case 404: {
            sprintf(header, "%s\r\n%s\r\n%s%d",
                    response_code_404,
                    content_type,
                    content_length, c_length);
            break;
        }
        case 416: {
            sprintf(header, "%s\r\n%s\r\n%s*/%d\r\n%s%d",
                    response_code_416,
                    content_type,
                    content_range, p_length,
                    content_length, 0);
            break;
        }
        default: {
            printf("what how?");
            break;
        }
    }
    printf("\n\nheader: %s", header);
    return header;
}

char *route(char *page) {
    if (*page == '\0') page = page_index;
    char *data = malloc(2 * buffer_size);

    char *webpage = fileToStr(page);
    if (!webpage || error_404 == 1) {
        error_404 = 1;
        webpage = fileToStr(page_error);
    }
    size_t weblen = strlen(webpage);

    int response_code = (error_404 == 1 ? 404 : 200);
    char *header = make_header(response_code, weblen, 0, 0, 0);
    sprintf(data, "%s\r\n\r\n%s", header, webpage);
    free(webpage);
    free(header);
    return data;
}

int validRange(int length, int start, int end) {
    return (start >= 0
            && start < length
            && end >= 0
            && end < length
            && start <= end ? 1 : 0);
}

char *grabRange(char *page, int start, int end) {
    if (*page == '\0') page = page_index;

    char *data = malloc(2 * buffer_size);
    char *partial = malloc(buffer_size);

    char *webpage = fileToStr(page);
    if (!webpage) {
        error_404 = 1;
        perror("Page not found!\n");
        webpage = fileToStr(page_error);
    } else {
        int at, i;
        if (validRange(strlen(webpage), start, end) != 1) {
            error_416 = 1;
            perror("Range is bad!\n");
            webpage = fileToStr(page_error);
        } else {
            for (i = 0, at = start; at < end; partial[i++] = webpage[at++]);
            partial[i] = '\0';
        }
    }
    size_t weblen = strlen(partial);

    int response_code;
    if (error_404 == 1) response_code = 404;
    else if (error_416 == 1) response_code = 416;
    else response_code = 206;
    char *header = make_header(response_code, weblen, start, end, strlen(webpage));
    sprintf(data, "%s\r\n\r\n%s", header, partial);

    free(webpage);
    free(header);
    free(partial);
    return data;
}

int service_client_socket(const int s, const char *const tag) {
    char buffer[buffer_size];
    bzero(buffer, buffer_size);
    size_t bytes;

    printf("new connection from %s\n", tag);

    while ((bytes = read(s, buffer, buffer_size - 1)) > 0) {

        char *requestedPage;
        char page[buffer_size];
        bzero(page, buffer_size);

        int at, i;
        for (at = 0; at < buffer_size && buffer[at] != ' '; at++);
        for (i = 0, at += 2; at < buffer_size && buffer[at] != ' '; page[i++] = buffer[at++]);
        page[i] = '\0';

        if (buffer[at] != ' ') {
            error_404 = 1;
            bzero(page, buffer_size);
            strcpy(page, page_error);
            requestedPage = route(page);
        } else {
            char *checkRange = strstr(buffer, "Range: bytes=");
            if (checkRange != NULL) {
                int position = checkRange - buffer + 13;

                char start_str[12];
                bzero(start_str, 12);
                for (i = 0, at = position; at < buffer_size && isdigit(buffer[at]); start_str[i++] = buffer[at++]);

                char end_str[12];
                bzero(end_str, 12);
                for (i = 0, at += 1; at < buffer_size && isdigit(buffer[at]); end_str[i++] = buffer[at++]);
                requestedPage = grabRange(page, atoi(start_str), atoi(end_str));
            } else {
                requestedPage = route(page);
            }
        }

        write(s, requestedPage, strlen(requestedPage));
        free(requestedPage);
        buffer[bytes] = '\0';

        if (bytes >= 1 && buffer[bytes - 1] == '\n') {
            if (bytes >= 2 && buffer[bytes - 2] == '\r') {
                strcpy(buffer + bytes - 2, "..");
            } else {
                strcpy(buffer + bytes - 1, ".");
            }
        }

#if (__SIZE_WIDTH__ == 64 || __SIZEOF_POINTER__ == 8)
        printf ("echoed %ld bytes back to %s, \"%s\"\n", bytes, tag, buffer);
#else
        printf("echoed %d bytes back to %s, \"%s\"\n", bytes, tag, buffer);
#endif
        error_404 = 0;
        error_416 = 0;
    }

    if (bytes != 0) {
        perror("read");
        return -1;
    }

    printf("connection from %s closed\n", tag);
    close(s);
    return 0;
}

