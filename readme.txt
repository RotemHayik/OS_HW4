
what to run in terminal:

sudo insmod message_slot.ko

// creates a leave in the hirarchy tree of the file system
// the leave "slot0" points to an inode that contains the metadata
// notifies the kernel that its not a regular file, its a device
// c - tells the kernel its a device
// the major and minor tells the kernel to which driver to turn

sudo mknod /dev/slot0 c 235 0

