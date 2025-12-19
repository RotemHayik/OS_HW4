#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE   

#define MAX_SLOTS 256

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>      
#include <linux/uaccess.h>  

MODULE_LICENSE("GPL");
//---------------------------------------------------------------

// structs

// there is a need to distinguish between different fds
struct my_file{
    struct slot *slot;
    int channel_id;
};

struct channel {
    int id;
    char message[128];
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
    struct my_file * ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;

    ctx->slot = s;
    ctx->channel_id = 0;   // no channel selected yet

    file->private_data = ctx;
    return 0;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    // connect fd to channel
    ((struct my_file*)file->private_data)->channel_id = (int)arg; // store channel id in private data
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    printk(KERN_INFO "Read %zu bytes from device\n", length);
    return length;
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    printk(KERN_INFO "Wrote %zu bytes to device\n", length);
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