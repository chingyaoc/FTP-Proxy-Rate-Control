/**
 * Simple FTP Proxy with RATE CONTROL
 * Author: JamesChuang
 * Feb 2016
 * **/
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define MagicNumber 2
#define MAXSIZE 512
#define FTP_PORT 8740
#define FTP_PASV_CODE 227
#define FTP_ADDR "  .  .  .  "      // your server IP
#define max(X,Y) ((X) > (Y) ? (X) : (Y))

int proxy_IP[4];

int connect_FTP(int ser_port, int clifd);
int proxy_func(int ser_port, int clifd, int rate ,bool download);
int create_server(int port);
void rate_control(clock_t t, int rate, int size, double magic);

int main (int argc, char **argv) {
    int ctrlfd, connfd, port, rate = 0;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    if (argc < 3) {
        printf("[v] Usage: ./executableFile <ProxyIP> <ProxyPort> \n");
        return -1;
    }

    sscanf(argv[1], " %d.%d.%d.%d", &proxy_IP[0], &proxy_IP[1], &proxy_IP[2], &proxy_IP[3]);
    port = atoi(argv[2]);
    // James
    rate = atoi(argv[3]);

    ctrlfd = create_server(port);
    clilen = sizeof(struct sockaddr_in);
    for (;;) {
        connfd = accept(ctrlfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            printf("[x] Accept failed\n");
            return 0;
        }

        printf("[v] Client: %s:%d connect!\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
        if ((childpid = fork()) == 0) {
            close(ctrlfd);
            proxy_func(FTP_PORT, connfd, rate, false);
            printf("[v] Client: %s:%d terminated!\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
            exit(0);
        }

        close(connfd);
    }
    return 0;
}

int connect_FTP(int ser_port, int clifd) {
    int sockfd;
    char addr[] = FTP_ADDR;
    int byte_num;
    char buffer[MAXSIZE];
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[x] Create socket error");
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(ser_port);

    if (inet_pton(AF_INET, addr, &servaddr.sin_addr) <= 0) {
        printf("[v] Inet_pton error for %s", addr);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("[x] Connect error");
        return -1;
    }

    printf("[v] Connect to FTP server\n");
    if (ser_port == FTP_PORT) {
        if ((byte_num = read(sockfd, buffer, MAXSIZE)) <= 0) {
            printf("[x] Connection establish failed.\n");
        }

        if (write(clifd, buffer, byte_num) < 0) {
            printf("[x] Write to client failed.\n");
            return -1;
        }
    }

    return sockfd;
}

int proxy_func(int ser_port, int clifd, int rate, bool download) {
    char buffer[MAXSIZE];
    int serfd = -1, datafd = -1, connfd;
    int data_port;
    int byte_num;
    int status, pasv[7];
    int childpid;
    int size, old_size, new_size;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    // James
    clock_t t1=0, t2=0, t;     // uplaod start time
    unsigned long int buf_size;

    // select vars
    int maxfdp1;
    int i, nready = 0;
    fd_set rset, allset;

    buf_size = sizeof(buffer);
    printf("size of buffer = %lu\n" ,buf_size);
    printf("clock tick = 1/%d\n", CLOCKS_PER_SEC);

    // connect to FTP server
    if ((serfd = connect_FTP(ser_port, clifd)) < 0) {
        printf("[x] Connect to FTP server failed.\n");
        return -1;
    }

    datafd = serfd;

    // initialize select vars
    FD_ZERO(&allset);
    FD_SET(clifd, &allset);
    FD_SET(serfd, &allset);

    // selecting
    for (;;) {
        // reset select vars
        rset = allset;
        maxfdp1 = max(clifd, serfd) + 1;

        // select descriptor
        nready = select(maxfdp1 + 1, &rset, NULL, NULL, NULL);
        if (nready > 0) {
            // check FTP client socket fd
            if (FD_ISSET(clifd, &rset)) {
                memset(buffer,'\0', MAXSIZE);
                if ((byte_num = read(clifd, buffer, MAXSIZE)) <= 0) {
                    printf("[!] Client terminated the connection.\n");
                    break;
                }
                if(size==0){
                  size = strlen(buffer)*sizeof(char);
                  old_size = size;
                }
                else
                  old_size = size;
                size = strlen(buffer)*sizeof(char);
                t2 = t1;
                t1 = clock();
                printf("t1 = %ld, t2 = %ld", t1, t2);
                t = t1-t2;

                rate_control(t, rate, old_size, MagicNumber);

                if (write(serfd, buffer, byte_num) < 0) {
                    printf("[x] Write to server failed.\n");
                    break;
                }
            }
            // check FTP server socket fd
            if (FD_ISSET(serfd, &rset)) {
                memset(buffer, '\0', MAXSIZE);
                if ((byte_num = read(serfd, buffer, MAXSIZE)) <= 0) {
                    printf("[!] Server terminated the connection.\n");
                    break;
                }

                if(ser_port == FTP_PORT)
                  buffer[byte_num] = '\0';

                status = atoi(buffer);
                //printf("LINE 173 : buffer %s\n",buffer);
                if (status == FTP_PASV_CODE && ser_port == FTP_PORT) {

                    sscanf(buffer, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)",&pasv[0],&pasv[1],&pasv[2],&pasv[3],&pasv[4],&pasv[5],&pasv[6]);
                    memset(buffer, '\0', MAXSIZE);
                    sprintf(buffer, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n", status, proxy_IP[0], proxy_IP[1], proxy_IP[2], proxy_IP[3], pasv[5], pasv[6]);
                    // James
                    //printf("LINE 180 : buffer %s\n",buffer);
                    if ((childpid = fork()) == 0) {
                        data_port = pasv[5] * 256 + pasv[6];
                        datafd = create_server(data_port);
                        printf("[-] Waiting for data connection!\n");
                        clilen = sizeof(struct sockaddr_in);
                        connfd = accept(datafd, (struct sockaddr *)&cliaddr, &clilen);
                        if (connfd < 0) {
                            printf("[x] Accept failed\n");
                            return 0;
                        }

                        printf("[v] Data connection from: %s:%d connect.\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
                        proxy_func(data_port, connfd, rate, true);
                        printf("[!] End of data connection!\n");
                        exit(0);
                    }
                }

                // Download condition (download = true)
                if(download){
                  old_size = size;
                  size = strlen(buffer)*sizeof(char);
                  t2 = t1;
                  t1 = clock();
                  printf("t1 = %ld, t2 = %ld\n", t1, t2);
                  t = t1-t2;
                  rate_control(t, rate, old_size, MagicNumber);
                }

                if (write(clifd, buffer, byte_num) < 0) {
                    printf("[x] Write to client failed.\n");
                    break;
                }

            }
        } else {
            printf("[x] Select() returns -1. ERROR!\n");
            return -1;
        }
    }
    return 0;
}

int create_server(int port) {
    int listenfd;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&servaddr , sizeof(servaddr)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return -1;
    }

    listen(listenfd, 3);
    return listenfd;
}

void rate_control(clock_t t, int rate, int size, double magic) {
     long desire_time, t_delay;

     printf("buffer size = %d bytes\n",size);
     desire_time = magic*((long)(size*1000000.0)/(rate*1024.0));    // 10^(-6) sec
     printf("desire time = %ld\n", desire_time);
     printf("t = %ld usec\n",t);

     if(t < desire_time){   // you can change the number to control rate!
       t_delay = (int)(desire_time-t);
       printf("delay = %ld\n", t_delay);
       usleep(t_delay);
     }
}
