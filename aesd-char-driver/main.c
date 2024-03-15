/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h> /* kmalloc() */
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Madeleine Monfort");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device; //allocated in init function

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    //could check for errors
    //if(!inode || !filp)?
    //no hardware to cause errors
    
    //use container_of to find the aesd_dev from the inode's cdev field
    struct aesd_dev* dev;
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    
    //set the file pointer to the aesd_dev struct found from the inode
    filp->private_data = dev;
    
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    //shouldn't need to do anything since I didn't malloc in open
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    
    if(count == 0) goto end;
    
    //get the circular buffer (get device struct)
    struct aesd_dev* dev = filp->private_data;
    struct aesd_circular_buffer* cbuf = dev->cbuf;
    
    //get entry at fpos
    size_t entry_pos = 0;
    struct aesd_buffer_entry* entry = aesd_circular_buffer_find_entry_offset_for_fpos(cbuf, *f_pos, &entry_pos);
    //do error checking
    if(!entry) {
        retval = 0; //end of file reached
        goto end;
    }
    PDEBUG("read: epos=%ld, size=%ld", entry_pos, entry->size);
    //do size math
    size_t new_size = entry->size - entry_pos; 
    if(count < new_size)
        new_size = count;
    
    //copy data to user
    char* tmp_ptr = entry->buffptr;
    tmp_ptr = tmp_ptr + entry_pos;
    if(copy_to_user(buf, tmp_ptr, new_size)) {
        retval = -EFAULT;
        goto end;
    }
    
    //update fpos
    *f_pos += new_size;
    
    //update retval
    retval = new_size;
    
end:
    PDEBUG("read: fpos=%lld, retval=%ld", *f_pos, retval);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    
    //get the circular buffer (get device struct)
    struct aesd_dev* dev = filp->private_data;
    struct aesd_circular_buffer* cbuf = dev->cbuf;
    
    //create an entry
    struct aesd_buffer_entry entry;
    
    //fill the entry
    entry.size = count;
    char* command_buf = kmalloc(count, GFP_KERNEL); //need to set up the buffer for the command
    if(!command_buf) {
        retval = -ENOMEM;
        goto end;
    }
    
    if(copy_from_user(command_buf, buf, count)) { //!= 0
        retval = -EFAULT;
        goto fail_fill;
    }
    entry.buffptr = command_buf;
    retval = count;
    
    //handle overwriting freeing
    if(cbuf->full) {
        uint8_t index = cbuf->in_offs;
        struct aesd_buffer_entry* e_overwrite = &(cbuf->entry[index]);
        if(e_overwrite) {
            kfree(e_overwrite->buffptr);
        }
    }
    
    //perform a write operation on cbuf
    aesd_circular_buffer_add_entry(cbuf, &entry);
    
    goto end;
    

fail_fill:
    kfree(command_buf);
end:
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

     //init circular buffer dynamically
     aesd_device.cbuf = kmalloc(sizeof(struct aesd_circular_buffer), GFP_KERNEL);
       //check failure
     aesd_circular_buffer_init(aesd_device.cbuf);
     
     //init the entry dynamically
     aesd_device.current_command = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
       //check failure
     
     //init the mutex dynamically
     mutex_init(&aesd_device.lock_cbuf);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    //free the circular buffer
    uint8_t index = 0;
    struct aesd_buffer_entry *entry;
    AESD_CIRCULAR_BUFFER_FOREACH(entry,aesd_device.cbuf,index) {
        //free each entry and it's pointers
        kfree(entry->buffptr);
    }
    kfree(aesd_device.cbuf);
    
    //free the entry
    kfree(aesd_device.current_command);
    
    //release the mutex?

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
