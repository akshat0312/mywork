#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>  // For copy_to_user
#include <linux/kthread.h>  // For kernel threads

#define I2C_BUS_AVAILABLE   (1)
#define SLAVE_DEVICE_NAME   ("BH1750")
#define BH1750_SLAVE_ADDR   (0x23) // Default I2C address

static struct i2c_adapter *bh1750_i2c_adapter = NULL;
static struct i2c_client *bh1750_i2c_client = NULL;

// Function prototypes
static int bh1750_probe(struct i2c_client *client);
static void bh1750_remove(struct i2c_client *client);
static int bh1750_read_light_intensity(void);
static int bh1750_read_thread(void *data);

// Global variable to store calculated light intensity
static u16 light_intensity;

// Kernel thread for continuous light intensity readings
static struct task_struct *bh1750_thread;

static int bh1750_read_light_intensity(void) {
    unsigned char data[2];
    int ret;

    // Read data from the sensor (example: continuous measurement mode)
    ret = i2c_master_recv(bh1750_i2c_client, data, sizeof(data));
    if (ret < 0) {
        pr_err("BH1750: I2C read failed (%d)\n", ret);
        return ret;
    }

    // Combine bytes and calculate light intensity based on BH1750 datasheet
    light_intensity = (data[0] << 8) | data[1];

    // Print light intensity to kernel log
    pr_info("BH1750: Light intensity: %u\n", light_intensity);

    return 0;
}

static int bh1750_read_thread(void *data) {
    while (!kthread_should_stop()) {
        bh1750_read_light_intensity();
        msleep(5000); // Sleep for 5 seconds
    }
    return 0;
}

static const struct i2c_device_id bh1750[] = {
    { SLAVE_DEVICE_NAME, 0 },
    { }
};


MODULE_DEVICE_TABLE(i2c,bh1750);


struct of_device_id bh1750_ids[]={
   { .compatible = "rohm,bh1750" },
    { }
};

MODULE_DEVICE_TABLE(of, bh1750_ids);

static struct i2c_driver bh1750_driver = {
    .driver = {
        .name = SLAVE_DEVICE_NAME,
        .owner = THIS_MODULE,
        .of_match_table = bh1750_ids,
        
    },
    .probe = bh1750_probe,
    .remove = bh1750_remove,
    .id_table = bh1750,
    
};

static int bh1750_probe(struct i2c_client *client) {
    int ret;


    bh1750_i2c_client = client;

    // Create kernel thread for continuous light intensity readings
    bh1750_thread = kthread_run(bh1750_read_thread, NULL, "bh1750_read_thread");
    if (IS_ERR(bh1750_thread)) {
        pr_err("Failed to create kernel thread\n");
        return PTR_ERR(bh1750_thread);
    }

    pr_info("BH1750 probe successful\n");
    return 0;
}

static void bh1750_remove(struct i2c_client *client) {
    // Stop and clean up kernel thread
    if (bh1750_thread) {
        kthread_stop(bh1750_thread);
    }
    
}

static struct i2c_board_info bh1750_i2c_board_info = {
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, BH1750_SLAVE_ADDR)
};

static int __init bh1750_init_module(void) {
    int ret;

    // Get I2C adapter
    bh1750_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
    if (!bh1750_i2c_adapter) {
        pr_err("BH1750: Could not get I2C adapter\n");
        return -ENODEV;
    }

    // Create I2C client
    bh1750_i2c_client = i2c_new_client_device(bh1750_i2c_adapter, &bh1750_i2c_board_info);
    if (!bh1750_i2c_client) {
        pr_err("BH1750: Could not create I2C client\n");
        i2c_put_adapter(bh1750_i2c_adapter);
        return -ENODEV;
    }

    // Register I2C driver
    ret = i2c_add_driver(&bh1750_driver);
    if (ret < 0) {
        pr_err("BH1750: Could not register I2C driver\n");
        i2c_unregister_device(bh1750_i2c_client);
        i2c_put_adapter(bh1750_i2c_adapter);
        return ret;
    }

    return 0;
}

static void __exit bh1750_exit_module(void) {
    // Unregister I2C driver
    i2c_del_driver(&bh1750_driver);

    // Remove I2C client
    i2c_unregister_device(bh1750_i2c_client);

    // Release I2C adapter
    i2c_put_adapter(bh1750_i2c_adapter);
}

module_init(bh1750_init_module);
module_exit(bh1750_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("BH1750 Light Intensity Sensor Driver");
MODULE_VERSION("1.0");

