#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/kdev_t.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Ngo");
MODULE_DESCRIPTION("A simple character device driver example");
MODULE_VERSION("0.1");

#define DEVICE_NAME "logan_device"
#define DEVICE_CLASS "logan_class"
#define BUFFER_SIZE 1024
char *kernel_buffer = NULL;

dev_t dev_num=0;
static struct class *logan_class;
static struct device *logan_device;
static struct cdev device_cdev;

static int logan_open(struct inode *inode, struct file *file);
static int logan_release(struct inode *inode, struct file *file);
static ssize_t logan_write(struct file *file, const char __user *buff, size_t len, loff_t *offset);
static ssize_t logan_read(struct file *file, char __user *buff, size_t len, loff_t *offset);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = logan_open,
    .write = logan_write,
    .read = logan_read,
    .release = logan_release
};

static int logan_open(struct inode *inode, struct file *file){
    pr_info("Hi, your driver is opened ^_^! \n");
    kernel_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if(!kernel_buffer){
        pr_err("Failed to allocate memory\n");
        return -ENOMEM; // Ma loi chuan khi thieu bo nho ^^
    }
    else{
        pr_info("Allocate the memory success!");
    }
    return 0;
}

static int logan_release(struct inode *inode, struct file *file){
    if(kernel_buffer){
        kfree(kernel_buffer);
        kernel_buffer=NULL;
    }
    pr_info("Bye, see you again v_v !!! \n");
    return 0;
}

//Muc dich cua ham write nay de lay du lieu tu user_space de ghi vao kernel_space (echo)

static ssize_t logan_write(struct file *file, const char __user *buff, size_t len, loff_t *offset){
    if(len > BUFFER_SIZE)
        len = BUFFER_SIZE;
    if(copy_from_user(kernel_buffer, buff, len) != 0){ //dich den, nguon, kich thuoc
        pr_err("FAILED TO WRITE DATA \r\n");
        return -EFAULT; // Loi
    }
    pr_info("DATA WRITE: %s\n", kernel_buffer);
    return len;
}

static ssize_t logan_read(struct file *file, char __user *buff, size_t len, loff_t *offset){
    if(*offset >= BUFFER_SIZE){
        return 0;
    }
    if(len > BUFFER_SIZE - *offset){
        len = (BUFFER_SIZE - *offset);
    }

    if (copy_to_user(buff, kernel_buffer + *offset, len) != 0) {
        pr_err("FAILED TO READ DATA \r\n");
        return -EFAULT;
    }

    *offset += len;
    pr_info("DATA READ (%zu bytes): %.*s\n", len, (int)len, kernel_buffer);
    return len;
}

int ret, a;

static int __init device_driver_init(void){
    pr_info("Hi, Let's start to my driver. \n");
    // Tao device voi major va minor 
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    // Check
    if(ret<0){
        pr_err("Failed to allocate character device region\n");
        return ret;
    }
    // Neu tao duoc:
    pr_info("Device allocated with major_number: %d and minor_number: %d .\r\n", MAJOR(dev_num), MINOR(dev_num));

    // Tao character device
    cdev_init(&device_cdev, &fops);
    // Add
    a = cdev_add(&device_cdev, dev_num, 1);

    if(a<0){
        pr_err("Fail to add device character v_v! \n");
        cdev_del(&device_cdev);
        unregister_chrdev_region(dev_num, 1); // If adding the character device fails, release the device number
        return a;
    }else{
        pr_info("Load device character successfull ^_^! \n");
    }

    // Tao class va device 
    logan_class = class_create(THIS_MODULE, "Logan_Class");
    // Check
    if(IS_ERR(logan_class)){
        pr_err("Failed to create your class! \n");
        // Khong tao duoc thi phai xoa dev_num
        cdev_del(&device_cdev);
        unregister_chrdev_region(dev_num, 1); // If adding the character device fails, release the device number
    }else{
        pr_info("Your class is created successfull. \n");
    }
    // Tao device
    logan_device = device_create(logan_class, NULL, dev_num, NULL, "Logan_Device");
    //Check
    if(IS_ERR(logan_device)){
        pr_err("Failed to create your device! \n");
        cdev_del(&device_cdev);
        class_destroy(logan_class); // Khi khong tao duoc device, can giai phong class
        return PTR_ERR(logan_device);
    }else{
        pr_info("Your device is created successfull. \n");
    }

    return 0;
}

static void __exit device_driver_exit(void){
    pr_info("======================EXIT=======================\n");
    device_destroy(logan_class, dev_num);
    class_destroy(logan_class);
    cdev_del(&device_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("GOOD BYE! \r\n");
}

module_init(device_driver_init);
module_exit(device_driver_exit);