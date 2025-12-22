#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE   

#define MAX_SLOTS 256
#define MAX_MESSAGE_LENGTH 128

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>      
#include <linux/uaccess.h>  

#include "message_slot.h"

// announces to the kernel that this modul maches the GPL license
MODULE_LICENSE("GPL");

//---------------------------------------------------------------

// structs

// a specifiec object for each fd
struct slot_file{
    struct slot *slot; // a pointer to the slot in the global array
    int channel_id;
    int censor; // default mode is 0 = uncensored
};

struct channel {
    int id;
    char message[MAX_MESSAGE_LENGTH];
    size_t len;
    struct channel *next;
};

struct slot {
    int minor;
    struct channel *channels;
};

//---------------------------------------------------------------

// global variables

static struct slot* slots_arr[MAX_SLOTS]; // initialized array of pointers to slots

//---------------------------------------------------------------

// declerations
struct slot* find_or_create_slot(int minor);
static int device_open(struct inode *inode, struct file *file);
static long device_ioctl(struct file *file, int cmd, unsigned int arg);
static ssize_t device_write(struct file *file, char __user *buffer, size_t length, loff_t *offset);
static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset);
static int __init message_slot_init(void);
static void __exit message_slot_exit(void);

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
    if (!s) {
        return -ENOMEM; // memory allocation failed
    }
    struct slot_file *fd_data = kmalloc(sizeof(*fd_data), GFP_KERNEL);
    if (!fd_data)
        return -ENOMEM;

    fd_data->slot = s;
    fd_data->channel_id = 0;   // no channel selected yet
    fd_data->censor = 0;          // censor off by default

    file->private_data = fd_data;
    return SUCCESS;
}

static long device_ioctl(struct file *file, int cmd, unsigned int arg)
{
    struct slot_file *fd_data = file->private_data;

    if (!fd_data)
        return -EINVAL; // Invalid argument

    switch (cmd) {

    case MSG_SLOT_CHANNEL:
        if (arg == 0)
            return -EINVAL; // Invalid argument
        fd_data->channel_id = (int)arg;
        break;

    case MSG_SLOT_SET_CENSOR:
        // if arg is diff than 0, put 1. else 0.
        fd_data->censor = (arg != 0);
        break;

    // there are only two cmds
    default:
        return -EINVAL;
    }

    return SUCCESS;
}


static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    struct slot_file *fd_data = file->private_data;
    struct slot *slot;
    struct channel *curr;

    if(!fd_data)
        return -EINVAL;
    if(fd_data->channel_id == 0)
        return -EINVAL; 

    slot = fd_data->slot;
    curr = slot->channels; // head of the channel list

    // search for the channel
      while (curr && curr->id != fd_data->channel_id) {
        curr = curr->next;
    }

    // if the channel does not exist (there cant be a channel without a message)
    if (!curr)
        return -EWOULDBLOCK;

    // buffer too small
    if (length < curr->len)
        return -ENOSPC;

    // copy message to user
    if (copy_to_user(buffer, curr->message, curr->len))
        return -EFAULT;

    // return number of bytes read
    return curr->len;
}


static ssize_t device_write(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    struct slot_file *fd_data = file->private_data;
    struct slot *slot;
    struct channel *curr;
    int is_new = 0;

    if (!fd_data)
        return -EINVAL;

    if (fd_data->channel_id == 0)
        return -EINVAL;

    if (length == 0 || length > MAX_MESSAGE_LENGTH)
        return -EMSGSIZE;

    slot = fd_data->slot;

    // find channel 
    curr = slot->channels;
    while (curr && curr->id != fd_data->channel_id)
        curr = curr->next;

    // if not found allocate, but don't link yet 
    if (!curr) {
        curr = kmalloc(sizeof(*curr), GFP_KERNEL);
        if (!curr)
            return -ENOMEM;
        is_new = 1;
    }

    // copy message 
    if (copy_from_user(curr->message, buffer, length)) {
        if (is_new)
            kfree(curr);
        return -EFAULT;
    }

    // censor logic 
    if (fd_data->censor) {
        size_t i;
        for (i = 0; i < length; i++) {
            if (i % 4 == 3)
                curr->message[i] = '#';
        }
    }

    curr->len = length;
    curr->id = fd_data->channel_id;

    // link only after success
    if (is_new) {
        curr->next = slot->channels;
        slot->channels = curr;
    }

    return length;
}


//---------------------------------------------------------------

// creating the object Fops of file operations struct
// assigning the functions to the struct variables
static struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .read           = device_read,
  .write        = device_write,
};

//---------------------------------------------------------------

// init function
static int __init message_slot_init(void)
{
    int rc = -1;

    // if someone will reach to the MAJOR_NUM, the calls will be transfered to the FOPS functions
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 ) {
    printk( KERN_ALERT "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
     return rc;
     }
    // memory allocation

    // TODO: CHECK OTHER FAILURES
    
    // allocation failure handling
    if (0) {
        printk(KERN_ERR "Memory allocation failed\n");
        return -ENOMEM;
    }
    // success
    return 0;
}

// exit function
static void __exit message_slot_exit(void)
{
    printk(KERN_INFO "Message Slot Module Unloaded\n");
}

//---------------------------------------------------------------


// Registration Macros

module_init(message_slot_init);
module_exit(message_slot_exit);