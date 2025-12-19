#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE   

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>      
#include <linux/uaccess.h>  

MODULE_LICENSE("GPL");
//---------------------------------------------------------------

// structs


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

struct slot slot_array[256]; // array of slots

//---------------------------------------------------------------



// helper functions

struct slot* create_slot(int minor)
{
    
    struct slot* new_slot = kmalloc(sizeof(struct slot), GFP_KERNEL);
    if (new_slot != NULL) {
        new_slot->minor = minor;
    }
    return new_slot;
}
//---------------------------------------------------------------

// device functions

// inode and file are created and initialized by the kernel
static int device_open(struct inode *inode, struct file *file)
{
    // get the opened file's minor number
    int minor = iminor(inode);
    // find or create the slot for this minor number
    struct slot *s = create_slot(minor);
    file->private_data = NULL; // initialize private data
    return 0;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    printk(KERN_INFO "IOCTL command received: %u\n", cmd);
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