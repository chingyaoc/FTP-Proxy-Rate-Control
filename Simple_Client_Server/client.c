#include <stdio.h>
#include <string.h>//strlen
#include <sys/socket.h>
#include <arpa/inet.h>//inet_addr
#include <unistd.h>//write

int main (int argc , char **argv) {
	int sock;
	struct sockaddr_in server;
	char server_reply[3000];

	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock==-1) {
		printf("Could not create socket");
	}

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(8888);

	//Connect to remote server
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("connect failed. Error");
		return 1;
	}

	puts("Connected");
	
	//Write data to server by James
	char buf[512];
	//scanf("%s",buf);	//read from commend line
	int nbytes;
	/*
	if((nbytes = write(sock, buf, sizeof(buf))) < 0) {
		perror("write");
		exit(1); 
	}
	//read response
	if((nbytes = read(sock, buf, sizeof(buf))) < 0){
                 perror("read");
                 exit(1);
         }
         printf("client print : %s\n",buf);
	*/
	scanf("%s",buf);
	while(strcmp("quit",buf) != 0){
		write(sock, buf, sizeof(buf));
		read(sock, buf, sizeof(buf));
		printf("client print : %s\n",buf);
		scanf("%s",buf);
	}

	//Receive a reply from the server
	if (recv(sock, server_reply ,2000 ,0)<0) {
		puts("recv failed");
	}

	server_reply[26] = '\0';
	printf("Server datetimes: %s", server_reply);
	close(sock);
	return 0;
}
