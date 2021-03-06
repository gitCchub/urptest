#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define SIZE     1024
#define IPADDRESS   "127.0.0.1"
#define PORT   8787
#define FDSIZE        1024
#define EPOLLEVENT 20

static void handle_connection(int cli_sock);
static void
handle_event(int epollfd, struct epoll_event *events, int num, int cli_sock, char *buf);
static void readata(int epollfd, int fd, int cli_sock, char *buf);
static void writedata(int epollfd, int fd, int cli_sock, char *buf);
static void add_event(int epollfd, int fd, int state);
static void delete_event(int epollfd, int fd, int state);
static void modify_event(int epollfd, int fd, int state);

int main(){
    int cli_sock, connect_fd;
    if((cli_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
          printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
          return 1;
    };
    struct sockaddr_in  seraddr;
    bzero(&seraddr, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, IPADDRESS, &seraddr.sin_addr) <= 0){
        printf("inet_pton error for %s\n", IPADDRESS);
        return 1;
    };
    if ((connect_fd = connect(cli_sock, (struct sockaddr*)&seraddr, sizeof(seraddr))) == -1){
        printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
        return 2;
    }
    handle_connection(cli_sock);
    printf("connect: %d\n", connect_fd);
    printf("sock: %d", cli_sock);
    close(cli_sock);
    printf("connect: %d\n", connect_fd);
    printf("sock: %d\n", cli_sock);
    return 0;
}

static void handle_connection(int cli_sock){
    int epollfd;
    struct epoll_event events[EPOLLEVENT];
    char buf[SIZE];
    int ret;
    epollfd = epoll_create(FDSIZE);
    add_event(epollfd, STDIN_FILENO, EPOLLIN);
    for (;;){
        ret = epoll_wait(epollfd, events, EPOLLEVENT, -1);
        handle_event(epollfd, events, ret, cli_sock, buf);
    }
    close(epollfd);
}

static void
handle_event(int epollfd, struct epoll_event *events, int num, int cli_sock, char *buf){
    int fd;
    int i;
    for (i = 0; i < num; i++){
        fd = events[i].data.fd;
        if (events[i].events & EPOLLIN){
            readata(epollfd, fd, cli_sock, buf);
        }
        else if (events[i].events & EPOLLOUT){
            writedata(epollfd, fd, cli_sock, buf);
        }
    }
}

 static void readata(int epollfd, int fd, int cli_sock, char *buf){
     ssize_t read_data = read(fd, buf, SIZE);
     if (read_data == -1){
         perror("read error");
         close(fd);
     }
     else if (read_data == 0){
         fprintf(stderr, "server close.\n");
         close(fd);
     }else{
         if (fd == STDIN_FILENO)
             add_event(epollfd, cli_sock, EPOLLOUT);
         else{
             delete_event(epollfd, cli_sock, EPOLLIN);
             add_event(epollfd, STDOUT_FILENO, EPOLLOUT);
         }
     }
 }

static void writedata(int epollfd, int fd, int cli_sock, char *buf){
    ssize_t write_data = write(fd, buf, strlen(buf));
    if (write_data == -1){
        perror("write error");
        close(fd);
    }else{
        if (fd == STDOUT_FILENO)
            delete_event(epollfd, fd, EPOLLOUT);
        else
            modify_event(epollfd, fd, EPOLLIN);
    }
    memset(buf, 0, SIZE);
    buf[SIZE] = '\0';
 }

static void add_event(int epollfd, int fd, int state){
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

static void delete_event(int epollfd, int fd, int state){
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

static void modify_event(int epollfd, int fd, int state){
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}
