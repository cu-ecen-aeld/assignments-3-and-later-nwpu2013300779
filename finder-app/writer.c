#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define LOG_DEBG(format, ...) syslog(LOG_DEBUG,  format, __VA_ARGS__)
#define LOG_WARN(format, ...) syslog(LOG_WARNING,  format, __VA_ARGS__)
#define LOG_ERROR(...) syslog(LOG_ERR,  __VA_ARGS__)

int write_files(char *filepathname, char *writestr);

int write_files(char *filepathname, char * writestr)
{
    openlog("write_files", 0, LOG_USER);
    int fd = open(filepathname, O_RDWR | O_CREAT | O_TRUNC, 0664);
    if (fd == -1) {
        perror("open file error");
        LOG_ERROR("open file %s error: %s",filepathname,strerror(errno));
        return -1;
    }

    LOG_DEBG("Writing <%s> to <%s> \n", writestr, filepathname);

    size_t len = strlen(writestr);
    const char *buf = writestr;
    ssize_t ret;
    while (len != 0 && (ret = write(fd, buf, len)) != 0)
    {
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            perror ("write");
            //LOG_ERROR("Error writing string <%s> to <%s>: %s \n",writestr, filepathname,strerror(errno));
            break;
        }
        len -= ret;
        buf += ret;
    }
    return close(fd);
}

int main(int argc, char **argv)
{
    int i = 0;
    LOG_DEBG("argc:%d \n",argc);
    for (; i < argc; i++)
    {
        LOG_DEBG("arg%d:%s\n",i,argv[i]);
    }
    if (argc < 3){
        LOG_ERROR("Error no enough argument, argc:%d", argc);
        return 1;
    }
    if(strlen(argv[0])==0){
        LOG_ERROR("First argument is not a valid file name!");
        return -1;
    }
    return write_files(argv[1],argv[2]);
}

