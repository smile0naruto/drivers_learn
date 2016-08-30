#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

//#define DEVICE_CREAT

#define SIZE 10
static int int_param;
static char *string_param;
static int array_param[SIZE], num;
module_param(int_param, int, 0644);
module_param(string_param, charp, 0644);
module_param_array(array_param, int, &num, 0644);

static char kk_test_state1;
static struct device *kk_test_dev;

static int kk_test_probe(struct platform_device *pdev);
static int kk_test_remove(struct platform_device *pdev);

static const struct of_device_id kpd_of_match[] = {
	      {.compatible = "kk,kk_test"},
        {},
};
static struct platform_driver kk_test = {
        .probe = kk_test_probe,
        .remove = kk_test_remove,
//#ifndef USE_EARLY_SUSPEND
//        .suspend = kpd_pdrv_suspend,
//        .resume = kpd_pdrv_resume,
//#endif
        .driver = {
                   .name = "kk_test",
                   .owner = THIS_MODULE,
                   .of_match_table = kpd_of_match,
                   },
};

#ifdef DEVICE_CREAT
static ssize_t kk_test_show_state(struct device *dev, struct device_attribute *attr, char *buf)
#else
static ssize_t kk_test_show_state(struct device_driver *driver, char *buf)
#endif
{
	ssize_t res;

	res = snprintf(buf, PAGE_SIZE, "%d\n", kk_test_state1);
	return res;
}
#ifdef DEVICE_CREAT
static ssize_t kk_test_store_state(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
#else
static ssize_t kk_test_store_state(struct device_driver *driver, const char *buf,size_t count)
#endif
{
		int ret;
		int res;
		ret = sscanf(buf, "%d", &kk_test_state1);
		if (ret != 1) {
				printk("kpd call state: Invalid values\n");
				return -EINVAL;
		}
		if (kk_test_state1 == 1){
				res = snprintf(buf, PAGE_SIZE, "the result is %d\n", kk_test_state1);
				int_param = kk_test_state1;
		}
		else 
				res = snprintf(buf, PAGE_SIZE, "enter error!!!!!%d\n", kk_test_state1);
			
			return kk_test_state1;
				
}
#ifdef DEVICE_CREAT
static DEVICE_ATTR(kk_test_state, 0644, kk_test_show_state, kk_test_store_state);
#else
static DRIVER_ATTR(kk_test_state, 0644, kk_test_show_state, kk_test_store_state); 
#endif

#ifndef DEVICE_CREAT
static struct driver_attribute *kk_test_attr_list[] = {
	&driver_attr_kk_test_state,        
};

static int kk_test_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(kk_test_attr_list) / sizeof(kk_test_attr_list[0]));

	if (driver == NULL)
			printk("there is not real driver,just sys attr???!!!\n\n");//		return -EINVAL;

	printk("%s,num is %d \n\n",__func__,num);
	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, kk_test_attr_list[idx]);
					printk("111driver_create_file (%s) = %d\n", kk_test_attr_list[idx]->attr.name, err);
		if (err) {
			printk("222driver_create_file (%s) = %d\n", kk_test_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
static int kk_test_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(kk_test_attr_list) / sizeof(kk_test_attr_list[0]));

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, kk_test_attr_list[idx]);

	return err;
}
#endif

static int kk_test_probe(struct platform_device *pdev)
{
	
		int i,err = 0;
		printk("\n\n\n\n====welcome to kk world!!!\n\n\n\n");
#ifdef DEVICE_CREAT
		err = device_create_file(&pdev->dev, &dev_attr_kk_test_state);		//dev_attr driver_attr_kk_test_state
		if (err < 0 )
				printk("Failed to create device file(%s)!\n", dev_attr_kk_test_state.attr.name);
#else 
		err = kk_test_create_attr(&kk_test.driver);//err = driver_create_file(&kk_test.driver, &driver_attr_kk_test_state);
		if (err){
				printk("Failed to create device file(%s)!\n", driver_attr_kk_test_state.attr.name);
				kk_test_delete_attr(&kk_test.driver);
				return err;
			}
#endif
		return 0;
	
}

static int kk_test_remove(struct platform_device *pdev)
{
		return 0;
}


static int kk_test_init(void)
{
	int ret;
	printk("====enter ko world==\n");
	
  ret = platform_driver_register(&kk_test);
  if (ret) {
  		printk("register driver failed (%d)\n", ret);
      return ret;
  }
	
	return 0;
}

static void  kk_test_exit(void)
{
	printk("=====goodbye ====\n");
}


module_init(kk_test_init);
module_exit(kk_test_exit);

MODULE_AUTHOR("kk");
MODULE_DESCRIPTION("kk@konka.com");
MODULE_LICENSE("GPL");
