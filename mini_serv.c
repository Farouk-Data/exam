#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

const int max = 100000;
int sockfd, connfd, maxfd, id = 0;
int ids[max];
socklen_t len;
struct sockaddr_in servaddr, cli; 
fd_set mainSet, writeSet, readSet;
char *msgArr[max];
char buffer[FD_SETSIZE], temp[1000];

int extract_message(char **buf, char **msg);

void sendMsg(int fd, char *msg){
    for (int i = 0; i <= maxfd; i++)
        send(i, msg, strlen(msg), 0);
}

void createClient(int fd){
    if (fd > maxfd)
        maxfd = fd;
    FD_SET(fd, &mainSet);
    ids[fd] = id++;
    msgArr[fd] = NULL;
    sprintf(temp, "server: client %d just arrived\n", ids[fd]);
    sendMsg(fd, temp);
}

void deleteClient(int fd){
    FD_CLR(fd, &mainSet);
    close(fd);
    free(msgArr[fd]);
    sprintf(temp, "server: client %d just left\n", ids[fd]);
    sendMsg(fd, temp);
}

void broadcast(int fd){
    char *msg = 0;

    sprintf(temp, "client %d: ", ids[fd]);
    while (extract_message(&msgArr[fd], &msg)){
        sendMsg(fd, temp);
        sendMsg(fd, msg);
        free(msg);
    }
}

void error(){
    write(2, "Fatal error\n", strlen("Fatal error\n"));
    exit(1);
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				error();
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		error();
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}


int main(int ac, char **av) {

    if (ac != 2){
        write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
        exit(1);
    }

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) error();
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) error();
	if (listen(sockfd, 10) != 0) error();
	
    FD_ZERO(&mainSet);
    FD_SET(sockfd, &mainSet);

    maxfd = sockfd;
    while (1){
        writeSet = readSet = mainSet;
        int test = select(maxfd + 1, &readSet, &writeSet, 0, 0);
        if (test < 0) error();

        for (int fd = 0; fd <= maxfd; fd++){
            if (!FD_ISSET(fd, &readSet))
                continue;
            if (fd == sockfd){
                len = sizeof(cli);
                connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
                if (connfd < 0) error();
                createClient(connfd);
                break;
            }
            else{
                int rd = recv(fd, buffer, 1023, 0);
                if (rd <= 0){
                    deleteClient(fd);
                    break;
                }
                buffer[rd] = 0;
                msgArr[fd] = str_join(msgArr[fd], buffer);
                broadcast(fd);
            }
        }
    }
}