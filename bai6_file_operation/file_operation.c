#include <Linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Ngo");
MODULE_DESCRIPTION("A simple Linux driver example");
MODULE_VERSION("1.0");

dev_t dev_num = 0; // Device number

static int __init file_operation_init(void) {
    pr_info("File operation module initialized.\n");

    return 0;
}

static void __exit file_operation_exit(void) {
    pr_info("File operation module exited successfully.\n");
}

module_init(file_operation_init);
module_exit(file_operation_exit);