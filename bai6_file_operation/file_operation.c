#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Ngo");
MODULE_DESCRIPTION("A simple Linux driver example");
MODULE_VERSION("1.0");


//Cach de viet character driver tu A-Z (How to write a character driver step by step)

/*
STEP 1: Include necessary headers and define module information.
STEP 2: Declare local variables, such as the device number.
STEP 3: Write struct file_operations structures.
STEP 4: Define the functions process the file operations.
STEP 5: In init function, subcribe the device
STEP 6: Create a device file (if you use udev)
STEP 7: In exit function, clean up resources and unregister the device.
*/

//STEP2:

dev_t dev_num = 0; // Device number
static struct class *logan_class; // Lớp thiết bị (dùng cho udev tạo file /dev) (class device)
static struct device *logan_device; // Thiết bị (device instance) (dùng cho udev tạo file /dev)
static struct cdev logan_cdev; // Character device structure



//STEP3:

static int logan_open(struct inode *inode, struct file *file); // Day la ham mo file nen can truyen vao inode va file
// inode: dai dien cho file thiet bi, vi du nhu /dev/logan_device hoac ghi nho nhanh la dai dien cho file thiet bi
// file: dai dien cho file mo, cung cap thong tin ve file thiet bi dang duoc mo

static int logan_release(struct inode *inode, struct file *file); // Day la ham dong file nen can truyen vao inode va file

static ssize_t logan_read(struct file *file, char __user *buf, size_t len, loff_t *offset); // Ham doc du lieu tu file
// file: dai dien cho file thiet bi dang duoc mo
// buf: con tro den bo dem nguoi dung, noi luu tru du lieu doc tu file
// len: kich thuoc bo dem nguoi dung, so byte toi da co the doc
//offset: con tro den vi tri hien tai trong file, cho phep doc du lieu tu mot vi tri xac dinh
static ssize_t logan_write(struct file *file, const char __user *buf, size_t len, loff_t *offset); // Ham ghi du lieu vao file


static struct file_operations fops ={
    .owner = THIS_MODULE,
    .open = logan_open,
    .read = logan_read,
    .release = logan_release,
    .write = logan_write
};

//STEP4:

static int logan_open(struct inode *inode, struct file *file) {
    pr_info("Device file opened successfully.\n");
    return 0; // Tra ve 0 neu mo thanh cong
}

static int logan_release(struct inode *inode, struct file *file) {
    pr_info("Device file closed successfully.\n");
    return 0; // Tra ve 0 neu dong thanh cong
}

static ssize_t logan_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    pr_info("Reading from device file.\n");
    // Here you would typically read data from the device and copy it to the user buffer
    // For simplicity, we return 0 bytes read
    return 0; // Tra ve so byte da doc
}

static ssize_t logan_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    pr_info("Writing .... to device file.\n");
    return len;
}



static int __init file_operation_init(void) {

    //STEP5:
    pr_info("File operation module initialized.\n");
    int ret;
    ret = alloc_chrdev_region(&dev_num, 0, 1, "logan_device");
    if(ret<0){
        pr_err("Failed to allocate device number.\n");
        return ret;
    }
    pr_info("Device number allocated: Major: %d, Minor: %d\n", MAJOR(dev_num), MINOR(dev_num));

    cdev_init(&logan_cdev, &fops); // Initialize the character device structure with file operations
    int a = cdev_add(&logan_cdev, dev_num, 1); // Add the character device to the system
    if(a < 0) {
        pr_err("Failed to add character device.\n");
        cdev_del(&logan_cdev); // If adding the character device fails, delete the cdev structure
        unregister_chrdev_region(dev_num, 1); // If adding the character device fails, release the device number
        return a;
    } else {
        pr_info("Character device added successfully.\n");
    }
    logan_class = class_create(THIS_MODULE, "logan_class");
    if (IS_ERR(logan_class)) {
        pr_err("FAILED to create class.\n");
        cdev_del(&logan_cdev);
        unregister_chrdev_region(dev_num, 1); // Khi khong tao duoc class, can giai phong device number
    }else{
        pr_info("Class created successfully.\n");
    }
    logan_device = device_create(logan_class, NULL, dev_num, NULL, "logan_device");
    if(IS_ERR(logan_device)) {
        pr_err("Failed to create device.\n");
        class_destroy(logan_class); // Khi khong tao duoc device, can giai phong class
        return PTR_ERR(logan_device);
    } else {
        pr_info("Device instance created successfully.\n");
    }

    return 0;
}

static void __exit file_operation_exit(void) {
    device_destroy(logan_class, dev_num); // Destroy the device instance
    class_destroy(logan_class); // Destroy the class
    cdev_del(&logan_cdev); // Delete the character device structure
    unregister_chrdev_region(dev_num, 1); // Unregister the device number    
    pr_info("File operation module exited successfully.\n");
}

module_init(file_operation_init);
module_exit(file_operation_exit);