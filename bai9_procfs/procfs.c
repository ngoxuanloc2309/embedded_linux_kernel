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

#define WRITE_VALUE     _IOW('a', 1, int32_t)
#define READ_VALUE      _IOR('b', 2, int32_t)
char *kernel_buffer = NULL;
char *proc_buffer = NULL;

dev_t dev_num=0;
static struct class *logan_class;
static struct device *logan_device;
static struct cdev device_cdev;
static size_t data_len = 0;
static size_t proc_data_len = 0;
int32_t value=0;
int32_t value_1=23;

//procfs
static struct proc_dir_entry *parent;

static int logan_open(struct inode *inode, struct file *file);
static int logan_release(struct inode *inode, struct file *file);
static ssize_t logan_write(struct file *file, const char __user *buff, size_t len, loff_t *offset);
static ssize_t logan_read(struct file *file, char __user *buff, size_t len, loff_t *offset);
static long logan_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


static int open_procfs(struct inode *inode, struct file *file);
static int release_procfs(struct inode *inode, struct file *file);
static ssize_t write_procfs(struct file *file, const char __user *buffer, size_t length, loff_t *offset);
static ssize_t read_procfs(struct file *file, char __user *buff, size_t len, loff_t *off);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = logan_open,
    .write = logan_write,
    .read = logan_read,
    .release = logan_release,
    .unlocked_ioctl = logan_ioctl
};

static struct proc_ops proc_fops = {
    .proc_open = open_procfs,
    .proc_release = release_procfs,
    .proc_write = write_procfs,
    .proc_read = read_procfs
};

static int open_procfs(struct inode *inode, struct file *file){
     pr_info("Hi, your procfs is opened ^_^! \n");
    
    // Chỉ allocate memory nếu chưa có
    if(!proc_buffer){
        proc_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
        if(!proc_buffer){
            pr_err("Failed to allocate memory\n");
            return -ENOMEM;
        }
        // Khởi tạo buffer với null terminator
        memset(proc_buffer, 0, BUFFER_SIZE);
        pr_info("Allocate the memory success!");
    }
    return 0;
}

static int release_procfs(struct inode *inode, struct file *file){
    pr_info("Bye, see you again v_v !!! \n");
    return 0;
}

static ssize_t write_procfs(struct file *file, const char __user *buffer, size_t length, loff_t *offset) {
    pr_info("ProcFS: Write function called with length %zu\n", length);
    
    if (!proc_buffer) {
        pr_err("ProcFS: Buffer not allocated\n");
        return -ENOMEM;
    }
    
    // Giới hạn độ dài để tránh buffer overflow
    if (length >= BUFFER_SIZE) {
        length = BUFFER_SIZE - 1;
    }
    
    // Copy data từ user space sang kernel space
    if (copy_from_user(proc_buffer, buffer, length) != 0) {
        pr_err("ProcFS: Failed to copy data from user\n");
        return -EFAULT;
    }
    
    proc_buffer[length] = '\0'; // Null terminate
    proc_data_len = length;
    
    pr_info("ProcFS: Data written (%zu bytes): %s\n", length, proc_buffer);
    return length;
}

static ssize_t read_procfs(struct file *file, char __user *buff, size_t len, loff_t *off) {
    pr_info("ProcFS: Read function called, offset: %lld, len: %zu\n", *off, len);
    
    if (!proc_buffer) {
        pr_err("ProcFS: Buffer not allocated\n");
        return -ENOMEM;
    }
    
    // Nếu offset >= data length, nghĩa là đã đọc hết, return 0 (EOF)
    if (*off >= proc_data_len) {
        pr_info("ProcFS: End of file reached\n");
        return 0;
    }
    
    // Tính số bytes cần đọc
    size_t bytes_to_read = proc_data_len - *off;
    if (len < bytes_to_read) {
        bytes_to_read = len;
    }
    
    // Copy data từ kernel space sang user space
    if (copy_to_user(buff, proc_buffer + *off, bytes_to_read) != 0) {
        pr_err("ProcFS: Failed to copy data to user\n");
        return -EFAULT;
    }
    
    *off += bytes_to_read;
    pr_info("ProcFS: Data read (%zu bytes)\n", bytes_to_read);
    return bytes_to_read;
}

static int logan_open(struct inode *inode, struct file *file){
    pr_info("Hi, your driver is opened ^_^! \n");
    
    // Chỉ allocate memory nếu chưa có
    if(!kernel_buffer){
        kernel_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
        if(!kernel_buffer){
            pr_err("Failed to allocate memory\n");
            return -ENOMEM;
        }
        // Khởi tạo buffer với null terminator
        memset(kernel_buffer, 0, BUFFER_SIZE);
        pr_info("Allocate the memory success!");
    }
    return 0;
}

static int logan_release(struct inode *inode, struct file *file){
    pr_info("Bye, see you again v_v !!! \n");
    return 0;
}

// Mục đích của hàm write này để lấy dữ liệu từ user_space để ghi vào kernel_space (echo)
static ssize_t logan_write(struct file *file, const char __user *buff, size_t len, loff_t *offset){
    if(!kernel_buffer){
        pr_err("Kernel buffer not allocated\n");
        return -ENOMEM;
    }
    
    if(len >= BUFFER_SIZE)  // >= để còn chỗ cho '\0'
        len = BUFFER_SIZE - 1;

    if(copy_from_user(kernel_buffer, buff, len) != 0){
        pr_err("FAILED TO WRITE DATA \r\n");
        return -EFAULT;
    }

    kernel_buffer[len] = '\0';  // Kết thúc chuỗi
    data_len = len;
    
    pr_info("DATA WRITE (%zu bytes): %s\n", len, kernel_buffer);
    return len;
}

static ssize_t logan_read(struct file *file, char __user *buff, size_t len, loff_t *offset){
    if(!kernel_buffer){
        pr_err("Kernel buffer not allocated\n");
        return -ENOMEM;
    }
    
    // Nếu đã đọc hết dữ liệu, trả về 0 (EOF)
    if (*offset >= data_len)
        return 0;

    // Tính toán số bytes cần đọc
    size_t bytes_to_read = data_len - *offset;
    if(len < bytes_to_read)
        bytes_to_read = len;

    // Copy dữ liệu từ kernel space sang user space
    if(copy_to_user(buff, kernel_buffer + *offset, bytes_to_read) != 0){
        pr_err("FAILED TO READ DATA \r\n");
        return -EFAULT;
    }

    *offset += bytes_to_read;
    pr_info("DATA READ (%zu bytes): %.*s\n", bytes_to_read, (int)bytes_to_read, kernel_buffer + (*offset - bytes_to_read));
    return bytes_to_read;
}

static long logan_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    switch(cmd){
        case WRITE_VALUE:
            if(copy_from_user(&value, (int32_t*)arg, sizeof(value))){
                pr_err("Failed to write data! \n");
                return -EFAULT;
            }
            pr_info("DATA_WRITTEN: %d\n", value);
            break;
        case READ_VALUE:
            if(copy_to_user((int32_t*)arg, &value_1, sizeof(value_1))){
                pr_err("FAILED TO READ \n");
                return -EFAULT;
            }
            pr_info("DATA_READ: %d\n", value_1);
            break;
        default:
            pr_err("Invalid ioctl command\n");
            return -EINVAL;
    }
    return 0;
}

int ret, a;

static int __init device_driver_init(void){
    pr_info("Hi, Let's start to my driver. \n");
    
    // Tạo device với major và minor 
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if(ret < 0){
        pr_err("Failed to allocate character device region\n");
        return ret;
    }
    pr_info("Device allocated with major_number: %d and minor_number: %d .\r\n", MAJOR(dev_num), MINOR(dev_num));

    // Tạo character device
    cdev_init(&device_cdev, &fops);
    a = cdev_add(&device_cdev, dev_num, 1);

    if(a < 0){
        pr_err("Fail to add device character v_v! \n");
        unregister_chrdev_region(dev_num, 1);
        return a;
    }else{
        pr_info("Load device character successfull ^_^! \n");
    }

    // Tạo class và device 
    logan_class = class_create(THIS_MODULE, "Logan_Class");
    if(IS_ERR(logan_class)){
        pr_err("Failed to create your class! \n");
        cdev_del(&device_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(logan_class);
    }else{
        pr_info("Your class is created successfull. \n");
    }
    
    // Tạo device
    logan_device = device_create(logan_class, NULL, dev_num, NULL, "Logan_Device");
    if(IS_ERR(logan_device)){
        pr_err("Failed to create your device! \n");
        class_destroy(logan_class);
        cdev_del(&device_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(logan_device);
    }else{
        pr_info("Your device is created successfull. \n");
    }

    //procfs

    parent = proc_mkdir("logan_folder_proc", NULL);
    if(parent == NULL){
        pr_info("Error create logan_folder_proc!\n");
        class_destroy(logan_class);
    }

    // Tao file entry

    proc_create("logan_proc", 0666, parent, &proc_fops);

    return 0;
}

static void __exit device_driver_exit(void){
    pr_info("======================EXIT=======================\n");
    
    // Giải phóng memory nếu có
    if(kernel_buffer){
        kfree(kernel_buffer);
        kernel_buffer = NULL;
    }
    remove_proc_entry("logan_proc", parent);
    proc_remove(parent);
    device_destroy(logan_class, dev_num);
    class_destroy(logan_class);
    cdev_del(&device_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("GOOD BYE! \r\n");
}

module_init(device_driver_init);
module_exit(device_driver_exit);