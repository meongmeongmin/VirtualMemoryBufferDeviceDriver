#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(void)
{
    const char *path = "/dev/vmemdev";  // 가상 디바이스 위치
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        perror("open"); 
        return 1;
    }

    const char *msg = "hello, vmemdev!\n";
    ssize_t n = write(fd, msg, strlen(msg));
    if (n < 0) 
    {
        perror("write");
        return 1;
    }
    
    printf("write %zd bytes\n", n);

    if (lseek(fd, 0, SEEK_SET) < 0) 
    { 
        perror("lseek");
        return 1;
    }

    char buf[1024] = {0};
    n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0)
    {
        perror("read");
        return 1;
    }
    
    buf[n] = '\0';
    printf("read %zd bytes: %s", n, buf);

    close(fd);
    return 0;
}