#include <stdio.h> // perror and fprintf
#include <stdlib.h> // exit and atoi
#include <fcntl.h> // open
#include <unistd.h> // close and read
#include <sys/ioctl.h> // ioctl

#include "message_slot.h"

#define BUF_SIZE 128   

int main(int argc, char *argv[])
{
    int fd;
    unsigned int channel_id;
    char buffer[BUF_SIZE];
    ssize_t bytes;

    // check the number of arguments
    if (argc != 3) {
        fprintf(stderr, "this program accepts only 2 arguments\n");
        exit(1);
    }

    // parse arguments
    channel_id = (unsigned int)atoi(argv[2]);

    // open
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open device file failed");
        exit(1);
    }

    // set channel
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0) {
        perror("ioctl MSG_SLOT_CHANNEL failed");
        close(fd);
        exit(1);
    }

    // read
  
    if ( bytes = read(fd, buffer, BUF_SIZE) < 0) {
        perror("read failed");
        close(fd);
        exit(1);
    }

    // close
    if (close(fd) < 0) {
        perror("close failed");
        exit(1);
    }

    // write message to stdout (only the message)
    if (write(STDOUT_FILENO, buffer, bytes) < 0) {
        perror("write failed");
        exit(1);
    }
    else

   return 0;
}
