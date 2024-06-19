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
    char foo[23], bar[15];
    auto fd = open("scatter_gather.txt", O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    iov[0].iov_base = foo;
    iov[0].iov_len = sizeof(foo);
    iov[1].iov_base = bar;
    iov[1].iov_len = sizeof(bar);

    auto nr = readv(fd, iov, 2);
    if (nr == -1)
    {
        perror("readv");
        return 1;
    }
    
    for(int i = 0; i<2; i++)
    {
        printf("%s",(char*)iov[i].iov_base);
    }

    if (close(fd))
    {
        perror("close");
        return 1;
    }
    return 0;
}