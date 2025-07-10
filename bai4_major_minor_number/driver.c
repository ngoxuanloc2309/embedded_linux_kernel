/*
    Cap phat tinh (static allocation) cho mot driver
*/

// #include <linux/kernel.h>
// #include <linux/module.h>
// #include <linux/init.h>
// #include <linux/fs.h>
// #include <linux/kdev_t.h>

// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("Logan Ngo");
// MODULE_DESCRIPTION("A simple Linux driver example");
// MODULE_VERSION("1.0");

// #define MAJOR_NUM 240
// #define MINOR_NUM 0
// static dev_t dev_num;
// int ret;

// dev_num = MKDEV(MAJOR_NUM, MINOR_NUM);

// static int __init drive_init(void){
//     ret =  register_chrdev_region(dev_num, 1, "logans_driver");
//     if(ret<0){
//         pr_info("Failed to register character device region.\n");
//         return ret;
//     }
//     if(ret==0){
//         pr_info("Character device region registered successfully.\n");
//         pr_info("Major number: %d, Minor number: %d\n", MAJOR(dev_num), MINOR(dev_num));
//     } 
// }

// static void __exit drive_exit(void){
//     unregister_chrdev_region(dev_num, 1);
//     pr_info("Character device region unregistered successfully.\n");
//     printk(KERN_INFO "Driver exited successfully.\n");
//     //pr_info("Driver exited successfully.\n");
// }

// module_init(drive_init);
// module_exit(drive_exit);

/*
    cap phat dong (dynamic allocation) cho mot driver
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Ngo");
MODULE_DESCRIPTION("A simple Linux driver example");
MODULE_VERSION("1.0");

dev_t dev = 0;
int ret;


static int __init drive_init(void){
    pr_info("Logan Ngo - Driver Example\n");
    ret = alloc_chrdev_region(&dev, 0, 1, "logan_handsome_driver");
    if(ret < 0){
        pr_info("Failed to allocate character device region.\n");
        return ret;
    }else{
        pr_info("Character device region allocated successfully.\n");
        pr_info("Major number: %d, Minor number: %d\n", MAJOR(dev), MINOR(dev));
    }
    return 0;
}

static void __exit drive_exit(void){
    unregister_chrdev_region(dev, 1);
    pr_info("Character device region unregistered successfully.\n");
    printk(KERN_INFO "Driver exited successfully.\n");
    //pr_info("Driver exited successfully.\n");
}

module_init(drive_init);
module_exit(drive_exit);