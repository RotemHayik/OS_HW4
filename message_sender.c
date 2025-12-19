
#include "chardev.h"    

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main()
{

    int fd = open("/dev/slot0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device file");
        return 1;
    }
    ioctl(fd, MSG_SLOT_CHANNEL, 1);
    write(fd, "Hello, World!", 13);
    

}

