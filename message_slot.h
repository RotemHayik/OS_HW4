#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

// both kernel and user space need to agree on the major number
#define MAJOR_NUM 235
// logical name of the driver
#define DEVICE_RANGE_NAME "message_slot"

/* ioctl MACROS to define 2 commands */
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
#define MSG_SLOT_SET_1CEN _IOW(MAJOR_NUM, 1, unsigned int)

#define SUCCESS 0

#endif
