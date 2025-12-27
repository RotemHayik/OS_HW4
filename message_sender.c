#include <stdio.h> // perror and fprintf
#include <stdlib.h> // exit and atoi
#include <string.h> // strlen
#include <fcntl.h> // open
#include <unistd.h> // close and write
#include <sys/ioctl.h> // ioctl
#include "message_slot.h"

int main(int argc, char *argv[])
{
    int fd;
    unsigned int channel_id;
    unsigned int censor_mode;
    ssize_t bytes;
    size_t len;

    // check the number of arguments
    if (argc != 5) {
        fprintf(stderr,"this program accepts only 4 arguments\n");
        exit(1);
    }

    // parse arguments
    channel_id = (unsigned int)atoi(argv[2]);
    censor_mode = (unsigned int)atoi(argv[3]);


    // we dont want to count the null terminator
    len = strlen(argv[4]); 

    // open
    fd = open(argv[1], O_WRONLY);
    if (fd < 0) {
        perror("open device file failed");
        exit(1);
    }

    // set censor
    if (ioctl(fd, MSG_SLOT_SET_CEN, censor_mode) < 0) {
        perror("ioctl MSG_SLOT_SET_CEN failed");
        close(fd);
        exit(1);
    }

    // set channel
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0) {
        perror("ioctl MSG_SLOT_CHANNEL failed");
        close(fd);
        exit(1);
    }

    // write
    
    if (write(fd, argv[4], len) < 0) {
        perror("write failed");
        close(fd);
        exit(1);
    }

    // close 
    if (close(fd) < 0) {
        perror("close failed");
        exit(1);
    }

    return 0;
}
