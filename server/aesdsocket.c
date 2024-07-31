#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define LOG_DEBG(format, ...) syslog(LOG_DEBUG,  format, __VA_ARGS__)
#define LOG_WARN(format, ...) syslog(LOG_WARNING,  format, __VA_ARGS__)
#define LOG_ERROR(...) syslog(LOG_ERR,  __VA_ARGS__)

#define SERVICE_PORT "9000"
#define BACKLOG 10   // how many pending connections queue will hold
#define TEST_FILE_PATH "/var/tmp/aesdsocketdata"
#define BUF_STEP_LEN  4096
bool caught_sigint = false;
bool caught_sigterm = false;

void *get_in_addr(struct sockaddr *sa)
{
    printf("\n[get_in_addr] sa_family:%d \n",sa->sa_family);
    if (sa->sa_family == AF_INET)
    {
        printf("[get_in_addr] port:%d addr:%d\n",((struct sockaddr_in *)sa)->sin_port, (uint32_t)((struct sockaddr_in *)sa)->sin_addr.s_addr);
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int store_socket_packet(char *pbuf, int len)
{
    int fd = open(TEST_FILE_PATH, O_RDWR| O_APPEND | O_CREAT , 0664);
    if (fd == -1) {
        perror("open file error");
        LOG_ERROR("open file %s error: %s",TEST_FILE_PATH,strerror(errno));
        return -1;
    }
    // do write op
    int write_sz = 0;
    int totallen = len;
    while (totallen != 0)
    {
        write_sz = write(fd, pbuf, totallen);
        if (write_sz == -1) {
            perror("write error");
            LOG_ERROR("write %s error: %s",TEST_FILE_PATH,strerror(errno));
            return -1;
        }
        totallen -= write_sz;
        pbuf += write_sz;
    }

    close(fd);
    return 0;
}
void send_packet(int new_fd, char *buf, int len)
{
    int send_sz = 0;
    int totallen = len;
    while (totallen != 0)
    {
        send_sz = send(new_fd, buf, totallen, MSG_TRUNC);
        totallen -= send_sz;
        buf += send_sz;
    }
}

void send_all_packet(int new_fd)
{
    FILE *file = fopen(TEST_FILE_PATH, "r");
    ssize_t read_len = 0;
    size_t buflen = 4096;
    char *buf;
    buf = (char *)malloc(buflen);
    while (!feof(file))
    {
        // read_len = getline(&buf, &buflen, file);
        printf("[0] buf:%p buflen:%ld",buf, buflen );
        fgets(buf, buflen, file);
        read_len = strlen(buf);
        printf("[1] buf:%p buflen:%ld read_len:%ld \n",buf, buflen, read_len);
        if (feof(file))
        {
            break;
        }
        send_packet(new_fd, buf, read_len);
    }
    free(buf);
    fclose(file);
}

void recv_packet(int new_fd)
{
    int recv_sz = 0;
    int buf_len = BUF_STEP_LEN;
    int pack_size = 0;
    char *buf = (char *)malloc(buf_len);
    printf("buf:%p\n", buf);
    char *pNewlineC = NULL;
    while (pNewlineC == NULL)
    {
        recv_sz = recv(new_fd, buf+pack_size, BUF_STEP_LEN, 0); // MSG_TRUNC
        if (recv_sz == -1) {
            perror("recv");
            exit(1);
        }
        pack_size += recv_sz;
        printf("pack_size:%d recv_sz:%d \n", pack_size, recv_sz);
        pNewlineC = strchr(buf, '\n');
        if(pNewlineC != NULL){
            /* new line character found, storing the packet and send back */
            uint32_t lenPacket = (uint32_t)(pNewlineC - buf +1);
            printf("lenPacket:%d \n", lenPacket);
            int ret = store_socket_packet(buf, lenPacket);
            free(buf);
            if (ret != 0)
                exit(1);
            send_all_packet(new_fd);
            break;
        }
        if ((buf_len-pack_size) <= 32 ) {
            buf_len += BUF_STEP_LEN;
            buf = realloc(buf, buf_len);
            printf("buf:%p \n", buf);
            //printf("[%s]\n", buf);
            if (buf == NULL){
                perror("realloc");
                printf("strerror:%s",strerror(errno));
                exit(1);
            }
        }
    }
}

int start_socket(bool deamon)
{
    char addrstr[INET6_ADDRSTRLEN];
    int status, sockfd;

    struct addrinfo hints = {.ai_flags = 0, .ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
    //hints.ai_flags = AI_PASSIVE;
    struct addrinfo *res_servinfo, *p;
    // char givenAddr[] = "127.0.0.1";

    if ((status = getaddrinfo(0, SERVICE_PORT, &hints, &res_servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = res_servinfo; p != NULL; p = p->ai_next)
    {
        inet_ntop(p->ai_addr->sa_family, get_in_addr(p->ai_addr), addrstr, sizeof(addrstr));

        printf("addrstr:%s\n",addrstr);

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        
        int optvalue = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(int)) == -1)
        {
            syslog( LOG_ERR, "setsockopt() error: %d (%s)\n",errno, strerror( errno ) );
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
        }

    freeaddrinfo(res_servinfo);
    if( deamon ){
        /*start deamon*/
        pid_t pid = fork();
        printf("start in deamon mode  pid:%d\n",pid);
        switch (pid){
            case 0:
                break;

            case -1:{
                perror("fork");
                exit(1);
            }
            break;

            default:{
                int status;
                printf("child process executing, wait \n");
                pid = wait(&status);
                printf("child process done executing, returned status:%d \n", status);
            }
                break;
        }
    } else{
        printf("start in normal mode  \n");
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    struct sockaddr_storage connected_addr;
    socklen_t con_addr_len = sizeof connected_addr;

    while (!caught_sigint && !caught_sigterm)
    {
        int new_fd = accept(sockfd, (struct sockaddr *)&connected_addr, &con_addr_len);
        if (new_fd == -1) {
            perror("accept");
            exit(1);
        }
        inet_ntop(connected_addr.ss_family, get_in_addr((struct sockaddr *)&connected_addr),addrstr, sizeof addrstr);
        printf("server: got connection from %s\n", addrstr);
        LOG_DEBG("Accepted connection from %s\n", addrstr);
        recv_packet(new_fd);
        printf("recv done \n");
        if (close( new_fd )) {
		    syslog( LOG_ERR, "close(new_fd) error: %d (%s)\n",errno, strerror( errno ) );
	    }
	    syslog( LOG_DEBUG, "Closed connection from %s", addrstr );
    }
    char *ptmp = malloc(1024);
    ptmp[0] = 'c';
    free(ptmp);
    if (caught_sigint)
    {
        syslog( LOG_DEBUG, "Caught SIGINT, exiting" );
    }
    if (caught_sigterm) {
	    syslog( LOG_DEBUG, "Caught SIGTERM, exiting" );
	}
    syslog( LOG_DEBUG, "Deleting file \"%s\"", TEST_FILE_PATH );
	remove( TEST_FILE_PATH );
    
    if (close(sockfd))
    {
        syslog( LOG_ERR, "close(fd) error: %d (%s)\n",
		    errno, strerror( errno ) );
    }
    return 0;
}
void sig_handler(int signum)
{
    int temp_errorno = errno;
    printf("[sig_handler] signum:%d \n", signum);
    // do sig handling
    switch (signum) {
	case SIGINT:
	    caught_sigint = true;
	    break;
	    
	case SIGTERM:
	    caught_sigterm = true;
	    break;
	    
	// case SIGPIPE:
	//     caught_sigpipe = true;
	    
    }

    errno = temp_errorno;

}

int main(int argc, char **argv)
{
    bool deamon = false;
    printf("argc:%d arg1:%s \n", argc, argv[1]);
    for (int i = 0; i < argc; i++){
        if(!strcmp(argv[i],"-d"))
            deamon = true;
    }
    int fd = open(TEST_FILE_PATH, O_RDWR | O_APPEND | O_CREAT | O_TRUNC, 0664);
    close(fd);

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    if(sigaction(SIGTERM, &sa, NULL) != 0){
        perror("sigaction register SIGTERM error");
    }
    if(sigaction(SIGINT, &sa, NULL) != 0){
        perror("sigaction register SIGINT error");
    }
    start_socket(deamon);
    return 0;
}

    // LOG_ERROR("[main] argc:%d \n", argc);
    // LOG_DEBG("argc:%d \n", argc);
    // char *tmpC = "asdfg\nzxcvbn\nqwertyuiop";
    // printf("tmpC:%s ", tmpC);
    // char *nC=strchr(tmpC, '\n');
    // printf("nC:%p ", nC);
