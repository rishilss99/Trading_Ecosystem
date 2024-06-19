#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

int main()
{
    struct iovec iov[2];
    const char *buf[] = {"Damn! First I scatter\n", "Then I gather\n"};
    auto fd = open("scatter_gather.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd == -1)
    {
        perror("open");
        return 1;
    }
    for (int i = 0; i < 2; i++)
    {
        iov[i].iov_base = (void*)buf[i];
        iov[i].iov_len = strlen(buf[i]) + 1;
    }
    auto nr = writev(fd, iov, 2);
    if (nr == -1)
    {
        perror("writev");
        return 1;
    }
    printf("%ld bytes written\n", nr);
    if (close(fd))
    {
        perror("close");
        return 1;
    }
    return 0;
}