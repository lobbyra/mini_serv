#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

int extract_message(char **buf, char **msg)
{
    char    *newbuf;
    int    i;

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
    char    *newbuf;
    int        len;

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

void ft_putstr(char *str, int fd) { write(fd, str, strlen(str)); }

int g_cli_fd[4096];
int g_id = 1;
char *g_cli_buf[4096];

void    ft_send(char *msg, int fd_excl) {
    int size;
    int bytes_read;
    int msg_len = strlen(msg);

    for (int i = 0; i < 4096; i++) {
        if (g_cli_fd[i] == 0 || fd_excl == i)
            continue;
        size = 0;
        bytes_read = 0;
        while ((bytes_read += send(i, msg + size, msg_len - size, MSG_DONTWAIT)) > 0) {
            size += bytes_read;
            if (size == msg_len)
                break;
        }
    }
}

void    new_client(int sock_srv) {
    int cli_fd;
    fd_set r_set;
    char msg[50];
    struct timeval timeout = {0, 400};

    FD_ZERO(&r_set);
    FD_SET(sock_srv, &r_set);
    select(sock_srv + 1, &r_set, NULL, NULL, &timeout);
    if (!FD_ISSET(sock_srv, &r_set))
        return ;
    cli_fd = accept(sock_srv, NULL, NULL);
    fcntl(cli_fd, F_SETFL, O_NONBLOCK);
    msg[sprintf(msg, "server: client %d just arrived\n", g_id - 1)] = '\0';
    ft_send(msg, cli_fd);
    g_cli_fd[cli_fd] = g_id++;
    return ;
}

void    new_message(void) {
    char buffer[8000];
    int i = 0;
    int i_save = 0;
    int status = 0;
    char to_send[5000];
    char *msg = NULL;
    struct timeval timeout = {0, 400};
    fd_set r_set;

    FD_ZERO(&r_set);
    for (; i < 4096; i++) {
        if (g_cli_fd[i] == 0)
            continue;
        FD_SET(i, &r_set);
        i_save = i;
    }
    i = 0;
    fflush(stdout);
    select(i_save + 1, &r_set, NULL, NULL, &timeout);
    for (; i < 4096; i++) {
        if (g_cli_fd[i] == 0 || !FD_ISSET(i, &r_set))
            continue;
        bzero(buffer, 8000);
        status = recv(i, buffer, 8000, 0);
        if (status == 0) {
            close(i);
            to_send[sprintf(to_send, "server: client %d just left\n", g_cli_fd[i] - 1)] = '\0';
            g_cli_fd[i] = 0;
            ft_send(to_send, i);
            if (g_cli_buf[i]) {
                free(g_cli_buf[i]);
                g_cli_buf[i] = NULL;
            }
            continue;
        } else if (status > 0) {
            g_cli_buf[i] = ft_strjoin(g_cli_buf[i], buffer);
            while ((extract_message(&g_cli_buf[i], &msg)) == 1) {
                bzero(to_send, 5000);
                sprintf(to_send, "client %d: %s", g_cli_fd[i] - 1, msg);
                ft_send(to_send, i);
                free(msg);
            }
        }
    }
}

int mini_serv(int sock_srv) {
    while (1) {
        new_client(sock_srv);
        new_message();
    }
    return (0);
}

int main(int argc, char **argv) {
    int sock_srv;
    struct sockaddr_in servaddr;

    if (argc != 2) {
        ft_putstr("Wrong number of arguments\n", 2);
        exit(1);
    }
    sock_srv = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_srv == -1) {
        ft_putstr("Fatal error\n", 2);
        exit(1);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(sock_srv, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        ft_putstr("Fatal error\n", 2);
        close(sock_srv);
        exit(1);
    }
    if (listen(sock_srv, 10) == -1) {
        ft_putstr("Fatal error\n", 2);
        close(sock_srv);
        exit(1);
    }
    return (mini_serv(sock_srv));
}
