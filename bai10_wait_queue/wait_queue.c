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
#include <linux/wait.h>
#include <linux/kthread.h>

DECLARE_WAIT_QUEUE_HEAD(logan_queue);

int flag=0; // condition
int count_condition=0;

static struct task_struct *logan_wait_thread;

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
static size_t data_len = 0;

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

static int logan_thread_func(void *unused){
    while(!kthread_should_stop()){

        pr_info("Thread: Waiting for event!\n");

        wait_event_interruptible(logan_queue, flag!=0);

        if(flag==2){
            pr_info("Thread: Exit signal received!\n");
            break;
        }
        
        if(flag==1){
            // Add code

            pr_info("Thread: Event from Logan_READ -- COUNT: %d\r\n", ++count_condition);
            flag = 0; //reset condition
        }

    }
    pr_info("Thread: Exitting...\n");
    return 0;
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
    size_t bytes_to_read;
    if(!kernel_buffer){
        pr_err("Kernel buffer not allocated\n");
        return -ENOMEM;
    }
    
    // Nếu đã đọc hết dữ liệu, trả về 0 (EOF)
    if (*offset >= data_len)
        return 0;

    // Tính toán số bytes cần đọc
    bytes_to_read = data_len - *offset;
    if(len < bytes_to_read)
        bytes_to_read = len;

    // Copy dữ liệu từ kernel space sang user space
    if(copy_to_user(buff, kernel_buffer + *offset, bytes_to_read) != 0){
        pr_err("FAILED TO READ DATA \r\n");
        return -EFAULT;
    }

    *offset += bytes_to_read;
    pr_info("DATA READ (%zu bytes): %.*s\n", bytes_to_read, (int)bytes_to_read, kernel_buffer + (*offset - bytes_to_read));
    pr_info("Wake up thread!\n");
    flag = 1;
    wake_up_interruptible(&logan_queue);

    return bytes_to_read;
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
    }

    // Create and start kernel thread
    logan_wait_thread = kthread_create(logan_thread_func, NULL, "logan_wait_thread");
    if(IS_ERR(logan_wait_thread)){
        pr_err('FAILED TO CREATE KERNEL THREAD\n');
        device_destroy(logan_class, dev_num);
        class_destroy(logan_class);
        cdev_del(&device_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(logan_wait_thread);
    }
    
    pr_info("Your device driver is created successfull\r\n");

    return 0;
}

static void __exit device_driver_exit(void){
    pr_info("======================EXIT=======================\n");

    if(logan_wait_thread){
        flag = 2;
        wake_up_interruptible(&logan_queue); // Wake up thread to exit
        kthread_stop(logan_wait_thread);
        pr_info("Kernel thread stopped!\n");
    }
    
    // Giải phóng memory nếu có
    if(kernel_buffer){
        kfree(kernel_buffer);
        kernel_buffer = NULL;
    }
    
    device_destroy(logan_class, dev_num);
    class_destroy(logan_class);
    cdev_del(&device_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("GOOD BYE! \r\n");
}

module_init(device_driver_init);
module_exit(device_driver_exit);