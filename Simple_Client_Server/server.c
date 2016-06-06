#include <stdio.h>
#include <string.h>//strlen
#include <sys/socket.h>
#include <arpa/inet.h>//inet_addr
#include <unistd.h>//write
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>


int main(int argc , char *argv[]) {
	int socket_desc , client_sock , c;
	struct sockaddr_in server , client;
	char client_message[2000];
	time_t ticks;
	int on = 1;

	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("Could not create socket");
	}
	//puts("Socket created");


	if ((setsockopt(socket_desc,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on))) < 0) {
		perror("setsockopt failed");
		exit(1);
	}

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);

	//Bind
	if (bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	//puts("bind done");
	//Listen
	listen(socket_desc , 3);
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if(client_sock<0)
	{
		perror("accept failed");
		return 1;
	}
	puts("Connection accepted");

	printf("IP address is: %s\n", inet_ntoa(client.sin_addr));
	printf("port is: %d\n", (int)ntohs(client.sin_port));

	//Get data from client by James
	char buf[512];
	int nbytes;
	/*
	if((nbytes = read(client_sock, buf, sizeof(buf))) < 0){
		perror("read");
		exit(1);
	}
	printf("server print : %s\n",buf);
	
	//Write a response by James
	if((nbytes = write(client_sock, buf, sizeof(buf))) < 0){
                 perror("write");
                 exit(1);
        }
	*/
	read(client_sock, buf, sizeof(buf));
	while(strcmp("quit",buf) != 0){
		printf("server print : %s\n",buf);
		write(client_sock, buf, sizeof(buf));
		read(client_sock, buf, sizeof(buf));
	}

	//Receive a message from client
	ticks=time(NULL);
	snprintf(client_message,sizeof(client_message),"%.24s\r\n",ctime(&ticks));
	write(client_sock ,client_message,strlen(client_message));

	close(client_sock);
	close(socket_desc);
	return 0;
}
