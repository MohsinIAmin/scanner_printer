#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

int start_server(int server_port)
{
	int sockfd, newsockfd;
	pid_t pid;
	struct sockaddr_in serv_addr, clt_addr;
	socklen_t addrlen;
	int portno;

	portno = server_port;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "can't open socket\n");
		return -1;
	}

	printf("create socket...\n");
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		fprintf(stderr, "can't bind\n");
		return -1;
	}

	printf("binding socket to port %d...\n", portno);
	listen(sockfd, MAX_CLIENT);

	for (;;)
	{
		addrlen = sizeof(clt_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&clt_addr, &addrlen);
		if (newsockfd < 0)
		{
			fprintf(stderr, "server can't accept\n");
			return -1;
		}

		printf("\nNew incoming connection\n");
		pid = fork();
		if (pid < 0)
		{
			fprintf(stderr, "Error on fork\n");
			return -1;
		}
		if (pid == 0)
		{
			close(sockfd);
			process_request(newsockfd);
			for (;;)
				;
			exit(0);
		}
		else
		{
			close(newsockfd);
		}
	}
	close(sockfd);
	return 0;
}
