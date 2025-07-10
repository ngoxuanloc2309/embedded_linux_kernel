// #include <linux/module.h>
// #include <linux/kernel.h>
// #include <linux/fs.h>
// #include <linux/init.h>
// #include <linux/kdev_t.h>

// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("Logan Ngo");
// MODULE_DESCRIPTION("A simple Linux driver example");
// MODULE_VERSION("1.0");

// dev_t dev_num= 0; // Device number

// static int __init device_file_init(void) {
//     pr_info("The beginning after the end. \n");

//     if(alloc_chrdev_region(&dev_num, 0, 1, "logan_device") < 0) {
//         pr_err("Failed to allocate device number.\n");
//         return -1;
//     }else {
//         pr_info("Device number allocated: Major: %d\n, Minor: %d\n", MAJOR(dev_num), MINOR(dev_num));
//         pr_info("Device instance created successfully.\n");
//     }

//     return 0;
// }

// static void __exit device_file_exit(void) {
//     unregister_chrdev_region(dev_num, 1);
//     pr_info("Device file module exited successfully.\n");
// }

// module_init(device_file_init);
// module_exit(device_file_exit);

// // After that, in the terminal, you can use the following commands to load and unload the module:
// // The first step: enter "make" to compile the module.
// // 1. Load the module: `sudo insmod device_file.ko`
// // 2. "dmesg | tail" to view the kernel log and see the output from the module.
// // 3. Manually device file: sudo mknod /dev/logan_device c <major_number> 0
// // 4. Unload the module: `sudo rmmod device_file`
// // 5. "dmesg | tail" to view the kernel log and see the output from the module.
// // 6. Remove the device file: `sudo rm /dev/logan_device`
// // 7. remove the module: `sudo rmmod device_file`
// // 8. Thanks for reading the code ^^!

// Section2: Create a device file with Automatically

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Ngo");
MODULE_DESCRIPTION("A simple Linux driver example");
MODULE_VERSION("1.0");

dev_t dev_num= 0; // Device number
int ret;

static struct class *logan_class;

static struct device *logan_device;

static int __init device_file_init(void) {
    
    pr_info("The beginning after the end. \n");
    ret = alloc_chrdev_region(&dev_num, 0, 1, "logan_device");
    if(ret < 0){
        pr_err("Fail to allocate device number. \n");
        return ret;
    }else
    pr_info("Device number allocated: Major: %d\n, Minor: %d\n", MAJOR(dev_num), MINOR(dev_num));
    logan_class = class_create(THIS_MODULE, "logan_class");
    if(IS_ERR(logan_class)){
        pr_err("Failed to create class. \n");
        unregister_chrdev_region(dev_num,1); // Khi class bị lỗi, ta cần giải phóng vùng nhớ đã cấp phát cho device number (hẹ hẹ)
        return PTR_ERR(logan_class);
    }
    logan_device = device_create(logan_class, NULL, dev_num, NULL, "logan_device");
    if(IS_ERR(logan_device)){
        pr_err("Failed to create device. \n");
        class_destroy(logan_class);  // Có tạo thì phải có phá, khi tạo thất bại có nghĩa là class đ cần thiết nữa nên t phải phá nó
        return PTR_ERR(logan_device);     
    }
    pr_info("Device instance created successfully.\n");
    return 0;
}

static void __exit device_file_exit(void) {
    // Phải theo thứ tự như dưới: 
    device_destroy(logan_class, dev_num);
    class_destroy(logan_class);
    unregister_chrdev_region(dev_num, 1);
    pr_info("Device file module exited successfully.\n");
}

module_init(device_file_init);
module_exit(device_file_exit);