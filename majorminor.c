#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>

dev_t dev= MKDEV(233,0);

/*
** Module Init function
*/
static int __init hello_world_init(void)
{
    register_chrdev_region(dev, 1, "akshat dev");
    printk(KERN_INFO "Major =%d Minor = %d \n",MAJOR(dev),MINOR(dev));
    printk(KERN_INFO "Kernel Module Inserted Successfully...\n");
    return 0;
}

/*
** Module Exit function
*/
static void __exit hello_world_exit(void)
{
        unregister_chrdev_region(dev,1);
        printk(KERN_INFO "Kernel Module Removed Successfully...\n");
}

module_init(hello_world_init);
module_exit(hello_world_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com>");
MODULE_DESCRIPTION("A simple hello world driver");
MODULE_VERSION("2:1.0");

