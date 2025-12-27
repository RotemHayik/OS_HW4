#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE   

#define MAX_SLOTS 256
#define MAX_MESSAGE_LENGTH 128

#include <linux/kernel.h>
#include <linux/module.h>  
#include <linux/fs.h>      
#include <linux/uaccess.h>  
#include <linux/slab.h> 
#include <linux/string.h> 


#include "message_slot.h"

// announces to the kernel that this modul maches the GPL license
MODULE_LICENSE("GPL");

//---------------------------------------------------------------

// structs

// a specifiec object for each fd
struct slot_file{
    struct slot *slot; // a pointer to the slot in the global array
    unsigned int channel_id;
    int censor; // default mode is 0 = uncensored
};

struct channel {
    unsigned int id;
    char message[MAX_MESSAGE_LENGTH];
    size_t len;
    struct channel *next;
};

struct slot {
    unsigned int minor;
    struct channel *channels;
};

//---------------------------------------------------------------

//  variables

static struct slot* slots_arr[MAX_SLOTS]; // initialized array of pointers to slots describing all possible minors

//---------------------------------------------------------------


// helper functions

// find slot by minor number, create if not exists
struct slot* find_or_create_slot(int minor)
{
    struct slot* slot_ptr;

    if (minor < 0 || minor >= MAX_SLOTS)
        return NULL;

    if (slots_arr[minor] != NULL) {
        slot_ptr = slots_arr[minor];
    } else {
        slot_ptr = kmalloc(sizeof(struct slot), GFP_KERNEL);
        if (!slot_ptr)
            return NULL;

        slot_ptr->minor = minor;
        slot_ptr->channels = NULL;
        slots_arr[minor] = slot_ptr;
    }

    return slot_ptr;
}

//---------------------------------------------------------------

// device functions

// inode and file are created and initialized by the kernel
static int device_open(struct inode *inode, struct file *file)
{
    // get the opened file's minor number
    int minor = iminor(inode);
    // find or create the slot for this minor number
    struct slot *s = find_or_create_slot(minor);
    struct slot_file *fd_data;
    if (!s) {
        return -ENOMEM; // memory allocation failed
    }
    fd_data = kmalloc(sizeof(*fd_data), GFP_KERNEL);
    if (!fd_data)
        return -ENOMEM;

    fd_data->slot = s;
    fd_data->channel_id = 0;   // no channel selected yet
    fd_data->censor = 0;          // censor off by default
    file->private_data = fd_data;

    return SUCCESS;
}

static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param)
{
    struct slot_file *fd_data = (struct slot_file *)file->private_data;

    if (!fd_data)
        return -EINVAL; // Invalid argument

    switch (ioctl_command_id) {

    case MSG_SLOT_CHANNEL:
        if (ioctl_param == 0)
            return -EINVAL; // Invalid argument
        fd_data->channel_id = (int)ioctl_param;
        break;

    case MSG_SLOT_SET_CEN:
        if (ioctl_param != 0 && ioctl_param != 1)
            return -EINVAL; // Invalid argument
        // if arg is diff than 0, put 1. else 0.
        fd_data->censor = (unsigned int)ioctl_param;
        break;

    // there are only two cmds
    default:
        return -EINVAL;
    }

    return SUCCESS;
}


static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    struct slot_file *fd_data = (struct slot_file *)file->private_data;
    struct slot *slot;
    struct channel *curr;

    if(!fd_data)
        return -EINVAL;

    if(fd_data->channel_id == 0)
        return -EINVAL; 

    slot = fd_data->slot;

    if (!slot)
        return -EINVAL;

    curr = slot->channels; // head of the channel list

    // search for the channel
      while (curr != NULL && curr->id != fd_data->channel_id) {
        curr = curr->next;
    }

    // if the channel does not exist or has no message
    if (curr == NULL || curr->len <= 0)
        return -EWOULDBLOCK;

    // buffer too small
    if (length < curr->len)
        return -ENOSPC;

    // copy message to user
    if (copy_to_user(buffer, curr->message, curr->len) != 0)
        return -EINVAL;

    // return number of bytes read
    return curr->len;
}



static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    struct slot_file *fd_data = (struct slot_file *)file->private_data;
    struct slot *slot;
    struct channel *curr;
    char tmp[MAX_MESSAGE_LENGTH];
    size_t i;

    if (!fd_data)
        return -EINVAL;

    if (fd_data->channel_id == 0)
        return -EINVAL;

    if (length < 1 || length > MAX_MESSAGE_LENGTH)
        return -EMSGSIZE;

    // copy message to tmp buffer
    if (copy_from_user(tmp, buffer, length) != 0)
        return -EINVAL;

    // apply censor logic to the tmp buffer
    if (fd_data->censor == 1) {
        for (i = 0; i < length; i++) {
            if (i % 4 == 3)
                tmp[i] = '#';
        }
    }

    slot = fd_data->slot;
    if (!slot)
        return -EINVAL;

    // find channel 
    curr = slot->channels;
    while (curr != NULL && curr->id != fd_data->channel_id)
        curr = curr->next;

    // allocate channel if not found
    if (curr == NULL) {
        curr = kmalloc(sizeof(*curr), GFP_KERNEL);
        if (!curr)
            return -ENOMEM;

        curr->next = slot->channels;
        slot->channels = curr;
    }

    // store message in channel
    memcpy(curr->message, tmp, length);
    curr->len = length;
    curr->id = fd_data->channel_id;

    return length;
}

static int device_release(struct inode *inode, struct file *file)
{
    struct slot_file *fd_data = (struct slot_file *)file->private_data;
    if (fd_data != NULL) {
        kfree(fd_data->private_data);
    }
    fd_data->private_data = NULL;
    return SUCCESS;
}

//---------------------------------------------------------------

// creating the object Fops of file operations struct
// assigning the functions to the struct variables
struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .read           = device_read,
  .write        = device_write,
  .release        = device_release,
};

//---------------------------------------------------------------

// init function
static int __init message_slot_init(void)
{
    int rc = -1;

    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);
    if (rc < 0) {
        printk(KERN_ALERT "%s registration failed for %d\n",
               DEVICE_RANGE_NAME, MAJOR_NUM);
        return rc;
    }

    return 0;
}


// exit function
static void __exit message_slot_exit(void)
{
    int i;
    struct slot *slot_in_arr;
    struct channel *curr, *next;

    for (i = 0; i < MAX_SLOTS; i++) {
        slot_in_arr = slots_arr[i];

        if (!slot_in_arr)
            continue;

        // free all channels in this slot 
        curr = slot_in_arr->channels;
        while (curr) {
            next = curr->next;
            kfree(curr);
            curr = next;
        }

        // free the slot itself 
        kfree(slot_in_arr);
        slots_arr[i] = NULL; // change pointer to NULL
    }
    // Unregister the device, should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);

}

//---------------------------------------------------------------


// Registration Macros

module_init(message_slot_init);
module_exit(message_slot_exit);