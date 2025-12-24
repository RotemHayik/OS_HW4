#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

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
        fprintf(stderr,"not enough arguments\n");
        exit(1);
    }

    / parse arguments
    channel_id = (unsigned int)atoi(argv[2]);
    censor_mode = (unsigned int)atoi(argv[3]);

    if (censor_mode != 0 && censor_mode != 1) {
        fprintf(stderr, "Invalid censor mode\n");
        exit(1);
    }

    // we dont want to count the null terminator
    len = strlen(argv[4]); 

    // open
    fd = open(argv[1], O_WRONLY);
    if (fd < 0) {
        perror("open failed");
        exit(1);
    }

    // set censor
    if (ioctl(fd, MSG_SLOT_SET_CENSOR, censor_mode) < 0) {
        perror("ioctl MSG_SLOT_SET_CENSOR failed");
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
    bytes = write(fd, argv[4], len);
    if (bytes < 0) {
        perror("write failed");
        close(fd);
        exit(1);
    }

    if ((size_t)bytes != len) {
        fprintf(stderr, "only partial write\n");
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
