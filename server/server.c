// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define PORT 8080
#define DEBUG 0
#define BUF_SIZE 4096

#include "functions.h"
int main(){
	int server_sock=socket(AF_INET,SOCK_STREAM,0);
	if(server_sock<0) {
		perror("socket");
		exit(1);
	}

	struct sockaddr_in addr={
		.sin_family=AF_INET,
		.sin_addr.s_addr=INADDR_ANY,
		.sin_port=htons(PORT)
	};
	int opt=1;
	setsockopt(server_sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	if(bind(server_sock,(struct sockaddr*)&addr,sizeof(addr))<0){
		perror("bind");
		exit(1);
	}
	
	listen(server_sock,5);
	printf("Server listening on http://localhost:%d\n",PORT);

	while(1){
		int client_sock=accept(server_sock,NULL,NULL);
		if(client_sock >=0){
			serve_client(client_sock);
		}
	}
	close(server_sock);
	return 0;
}









