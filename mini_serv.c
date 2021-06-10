#include <libc.h>
#include <sys/socket.h>

void    ft_putstr(char *str, int fd) { write(fd, str, strlen(str)); }

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

int    g_id = 1;         // id of clients
char   *g_cli_buf[4096]; // index accord to the client fd contain buff addr
int  g_cli_fd[4096];    // index is fd and int == id. If no connect ==0, >1 else

void    ft_send(char *msg) {
    int i = 0;
    int size;
    int bytes_send;
    int msg_len = strlen(msg);

    for (;i < 4096; i++) {
        if (g_cli_fd[i] == 0)
            continue ;
        size = 0;
        bytes_send = 0;
        while ((bytes_send = send(i, msg + size, msg_len - size, 0)) > 0) {
            size += bytes_send;
            if (size == bytes_send)
                break ;
        }
    }
}

int     new_client(int sock_srv) {
    struct sockaddr_in  cliaddr;
    socklen_t                 addr_len = sizeof(cliaddr);
    int                 cli_fd;
    char                msg[50];
    fd_set              r_set;
    struct timeval      timeout = {0, 100};

    FD_ZERO(&r_set);
    FD_SET(sock_srv, &r_set);
    select(sock_srv + 1, &r_set, NULL, NULL, &timeout);
    if (!FD_ISSET(sock_srv, &r_set))
        return (0);
    bzero(msg, 50);
    cli_fd = accept(sock_srv, (struct sockaddr*)&cliaddr, &addr_len);
    sprintf(msg, "server: client %d just arrived\n", g_id - 1);
    ft_send(msg);
    g_cli_fd[cli_fd] = g_id++;
    return (1);
    return (0);
}

void    new_message(void) {
    int     i = 0;
    int     i_save;
    int     status;
    int     save_id;
    char    buffer[101000];
    char    *msg = NULL;
    char    to_send[5000];
    struct timeval      timeout = {0, 400};
    fd_set  r_set;

    FD_ZERO(&r_set);
    for (; i < 4096; ++i) { // Create the set
        if (g_cli_fd[i] == 0)
            continue;
        FD_SET(i, &r_set);
        i_save = i;
    }
    select(i_save + 1, &r_set, NULL, NULL, &timeout);
    i = 0;
    for (; i < 4096; ++i) { // read clients
        if (g_cli_fd[i] == 0 || !FD_ISSET(i, &r_set))
            continue;
        bzero(to_send, 5000);
        bzero(buffer, 101000);
        status = recv(i, buffer, 101000, 0);
        if (status == -1) {
            close(i);
            g_cli_fd[i] = 0;
            continue;
        } else if (status == 0) {
            sprintf(to_send, "server: client %d just left\n", g_cli_fd[i] - 1);
            g_cli_fd[i] = 0;
            ft_send(to_send);
            if (g_cli_buf[i]) {
                free(g_cli_buf[i]);
                g_cli_buf[i] = NULL;
            }
            continue;
        } else {
            g_cli_buf[i] = ft_strjoin(g_cli_buf[i], buffer);
            save_id = g_cli_fd[i];
            g_cli_fd[i] = 0;
            while (extract_message(&g_cli_buf[i], &msg) == 1) {
                sprintf(to_send, "client %d: %s", save_id - 1, msg);
                ft_send(to_send);
                free(msg);
            }
            g_cli_fd[i] = save_id;
            continue;
        }
    }
}

int     mini_serv(int sock_srv) {
    while (1) {
        if (new_client(sock_srv) == 0) 
            new_message();
    }
    return (0);
}

int         main(int argc, char **argv) {
    int sock_srv;
    struct sockaddr_in servaddr;

    if (argc != 2) {
        ft_putstr("Wrong number of arguments\n", 2);
        exit(1);
    }
    bzero(g_cli_buf, 4096);
    bzero(g_cli_fd, 4096);
    if ((sock_srv = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        ft_putstr("Fatal error\n", 2);
        exit(1);
    }
    bzero(&servaddr, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(sock_srv, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        ft_putstr("Fatal error\n", 2);
        exit(1);
    }
    if (listen(sock_srv, 1000) == -1) {
        ft_putstr("Fatal error\n", 2);
        exit(1);
    }
    return (mini_serv(sock_srv));
}
