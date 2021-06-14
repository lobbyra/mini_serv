#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
				return (-1);
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

char *ft_strjoin(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void    ft_putstr(char *str, int fd) { write(fd, str, strlen(str)); };

int g_cli_fd[4096];
char *g_cli_buf[4096];
int g_id = 1;

void    ft_send(char *msg, int fd) {
    int size;
    int bytes_send;
    int msg_len = strlen(msg);

    for (int i = 0; i < 4096; i++) {
        if (g_cli_fd[i] == 0 || i == fd)
            continue;
        size = 0;
        bytes_send = 0;
        while ((bytes_send = send(i, msg + size, msg_len - size, 0)) > 0) {
            size += bytes_send;
        }
    }
    return ;
}

void    new_client(int sockfd) {
    int cli_fd = accept(sockfd, NULL, NULL);
    char msg[50];

    if (cli_fd < 1)
        return ;
    bzero(msg, 50);
    sprintf(msg, "server: client %d just arrived\n", g_id - 1);
    ft_send(msg, cli_fd);
    g_cli_fd[cli_fd] = g_id++;
    return ;
}

void    new_message(fd_set *r_set) {
    int bytes_read;
    char *msg;
    char to_send[5000];
    char buffer[101000];

    for (int i = 0; i < 4096; i++) {
        if (g_cli_fd[i] == 0 || !FD_ISSET(i, r_set))
            continue;
        bytes_read = recv(i, buffer, 101000, 0);
        buffer[bytes_read] = '\0';
        if (bytes_read == 0) {       // Client left
            to_send[sprintf(to_send, "server: client %d just left\n", g_cli_fd[i] - 1)] = '\0';
            g_cli_fd[i] = 0;
            ft_send(to_send, i);
            if (g_cli_buf[i]) {
                free(g_cli_buf[i]);
                g_cli_buf[i] = NULL;
            }
            continue;
        } else if (bytes_read > 0) { // New message !
            g_cli_buf[i] = ft_strjoin(g_cli_buf[i], buffer);
            while (extract_message(&(g_cli_buf[i]), &msg) == 1) {
                to_send[sprintf(to_send, "client %d: %s", g_cli_fd[i] - 1, msg)] = '\0';
                ft_send(to_send, i);
                free(msg);
            }
            continue;
        }
    }
}

void    mini_serv(int sockfd) {
    int i;
    int i_save;
    fd_set r_set;

    bzero(&g_cli_fd, 4096);
    bzero(&g_cli_buf, 4096);
    while (1) {
        i = 0;
        i_save = sockfd;
        FD_ZERO(&r_set);
        FD_SET(sockfd, &r_set);
        for (; i < 4096; i++) {
            if (g_cli_fd[i] == 0)
                continue;
            FD_SET(i, &r_set);
            i_save = i;
        }
        select(i_save + 1, &r_set, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &r_set))
            new_client(sockfd);
        new_message(&r_set);
    }
}

int         main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;

    if (argc != 2) {
        ft_putstr("Wrong number of arguments\n", 2);
        exit(1);
    }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        ft_putstr("Fatal error\n", 2);
        exit(1);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        ft_putstr("Fatal error\n", 2);
        exit(1);
    }
    if (listen(sockfd, 10) == -1) {
        ft_putstr("Fatal error\n", 2);
        exit(1);
    }
    mini_serv(sockfd);
    return (0);
}
