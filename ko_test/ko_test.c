#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

//#define DEVICE_CREAT
#define USE_LEDS_C
#define SIZE 10
static int int_param;
static char *string_param;
static int array_param[SIZE], num;
module_param(int_param, int, 0644);
module_param(string_param, charp, 0644);
module_param_array(array_param, int, &num, 0644);

static char kk_test_state1;
static struct device *kk_test_dev;

struct device_node *kk_test_node;
static unsigned int kk_test_en;
static unsigned int kk_test_en_mode;
static unsigned int kk_test_en_if_config = 1;
static unsigned int kk_test_el;
static unsigned int kk_test_el_mode;
static unsigned int kk_test_el_if_config = 1;

struct pinctrl *kk_test_pin;
struct pinctrl_state *pinctrl_kk_test_def;
struct pinctrl_state *pinctrl_kk_test_def_low;
struct pinctrl_state *pinctrl_kk_test_def_high;
struct pinctrl_state *pinctrl_kk_test_set;
struct pinctrl_state *pinctrl_kk_test_set_low;
struct pinctrl_state *pinctrl_kk_test_set_high;

struct input_dev *kk_test_input_dev;
static int kk_test_hall_flag = 0;
unsigned int kk_test_hall_irq = 0;
static DECLARE_WAIT_QUEUE_HEAD(kk_test_hall_waiter);
static struct task_struct *kk_test_hall_thread = NULL;

static int kk_test_probe(struct platform_device *pdev);
static int kk_test_remove(struct platform_device *pdev);

#ifdef USE_LEDS_C
extern int backlight_is_off(void);
#endif

static const struct of_device_id kk_test_of_match[] = {
	      {.compatible = "kk,kk_test"},
        {},
};
static struct platform_driver kk_test_drv = {
        .probe = kk_test_probe,
        .remove = kk_test_remove,
//#ifndef USE_EARLY_SUSPEND
//        .suspend = kpd_pdrv_suspend,
//        .resume = kpd_pdrv_resume,
//#endif
        .driver = {
                   .name = "kk_test",
                   .owner = THIS_MODULE,
                   .of_match_table = kk_test_of_match,
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
				//res = snprintf(buf, PAGE_SIZE, "the result is %d\n", kk_test_state1);
				//int_param = kk_test_state1;
				pinctrl_select_state(kk_test_pin,pinctrl_kk_test_def_high);
				pinctrl_select_state(kk_test_pin,pinctrl_kk_test_set_low);
		}
		else if (kk_test_state1 == 2)
		{
				pinctrl_select_state(kk_test_pin,pinctrl_kk_test_def_low);
				pinctrl_select_state(kk_test_pin,pinctrl_kk_test_set_high);
		}else{ 
				pinctrl_select_state(kk_test_pin,pinctrl_kk_test_def_low);
				pinctrl_select_state(kk_test_pin,pinctrl_kk_test_set_low);
				//res = snprintf(buf, PAGE_SIZE, "enter error!!!!!%d\n", kk_test_state1);
		}	
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

static int kk_test_get_gpio_info(struct platform_device *pdev)
{
		int ret;
		
		kk_test_node = of_find_compatible_node(NULL, NULL, "kk,kk_test");		
		if (kk_test_node == NULL) {
				printk("KK_TEST - get kk_test node failed\n");
		} else {
				if (of_property_read_u32_index(kk_test_node, "kk_test_en", 0, &kk_test_en)) {
						kk_test_en_if_config = 0;
						printk("get dtsi kk_test_en fail\n");
				}
				if (of_property_read_u32_index(kk_test_node, "kk_test_en", 1, &kk_test_en_mode))
						printk("get dtsi kk_test_en_mode fail\n");

				if (of_property_read_u32_index(kk_test_node, "kk_test_el", 0, &kk_test_el)) {
						kk_test_el_if_config = 0;
						printk("get dtsi kk_test_el fail\n");
				}
				if (of_property_read_u32_index(kk_test_node, "kk_test_el", 1, &kk_test_el_mode))
						printk("get dtsi kk_test_el_mode fail\n");
				
				printk("====kk_test===check en is %d ,en mode is %d,el pin is %d,el mode is %d== \n",kk_test_en,kk_test_en_mode,kk_test_el,kk_test_el_mode);												
		}		
						
		kk_test_pin = devm_pinctrl_get(&pdev->dev);
		if(IS_ERR(kk_test_pin)){
				ret = PTR_ERR(kk_test_pin);
				printk("can't find kk_test pinctrl!\n");
		}
		
		pinctrl_kk_test_def = pinctrl_lookup_state(kk_test_pin, "kk_test_gpio_default");
		if (IS_ERR(pinctrl_kk_test_def)) {
				ret = PTR_ERR(pinctrl_kk_test_def);
				printk("Cannot find pinctrl_kk_test_def pinctrl!\n");
		}		
		
		pinctrl_kk_test_def_low = pinctrl_lookup_state(kk_test_pin, "kk_test_gpio_default_low");
		if (IS_ERR(pinctrl_kk_test_def_low)) {
				ret = PTR_ERR(pinctrl_kk_test_def_low);
				printk("Cannot find pinctrl_kk_test_def_low pinctrl!\n");
		}
		
		pinctrl_kk_test_def_high = pinctrl_lookup_state(kk_test_pin, "kk_test_gpio_default_high");
		if (IS_ERR(pinctrl_kk_test_def_high)) {
				ret = PTR_ERR(pinctrl_kk_test_def_high);
				printk("Cannot find pinctrl_kk_test_def_high pinctrl!\n");
		}

		pinctrl_kk_test_set = pinctrl_lookup_state(kk_test_pin, "kk_test_gpio_set");
		if (IS_ERR(pinctrl_kk_test_set)) {
				ret = PTR_ERR(pinctrl_kk_test_set);
				printk("Cannot find pinctrl_kk_test_set pinctrl!\n");
		}	
		
		pinctrl_kk_test_set_low = pinctrl_lookup_state(kk_test_pin, "kk_test_gpio_set_low");
		if (IS_ERR(pinctrl_kk_test_set_low)) {
				ret = PTR_ERR(pinctrl_kk_test_set_low);
				printk("Cannot find pinctrl_kk_test_set_low pinctrl!\n");
		}
		
		pinctrl_kk_test_set_high = pinctrl_lookup_state(kk_test_pin, "kk_test_gpio_set_high");
		if (IS_ERR(pinctrl_kk_test_set_high)) {
				ret = PTR_ERR(pinctrl_kk_test_set_high);
				printk("Cannot find pinctrl_kk_test_set_high pinctrl!\n");
		}		
	
		return 0;
}

static irqreturn_t kk_test_hall_eint_handler(unsigned irq, struct irq_desc *desc)
{
    disable_irq_nosync(kk_test_hall_irq);
    kk_test_hall_flag = 1;
    wake_up_interruptible(&kk_test_hall_waiter);

    return IRQ_HANDLED;
}

// FILE 属于c库，作用于用户空间,一般java和cpp可以使用这种方式，kernel使用filp_open的方式
#ifndef USE_LEDS_C
static int kk_test_bl_status;
static const char *bl_path = "/sys/devices/platform/leds-mt65xx/leds/lcd-backlight/brightness";
static int kk_test_check_bl_status(const char *filename)
{
	struct file *bl = NULL;
	int len;
	char buf[64];
	
	bl = filp_open(filename,O_RDONLY,0);
	if (IS_ERR(bl)) {
			printk("===kk_test open %s failed!!!\n",filename/*,strerror(errno)*/);
			return -1;
	} else {
	//	ret = bl->f_op->llseek(bl, 0, SEEK_SET);
		len = bl->f_op->read(bl,buf,1,bl->f_op);
		if (len < 0){
			printk("===kk_test read %s buf failed!!\n",filename);
			return -1;
		}
		printk("===kk_test check the buf[0] is %d ==len is %d =\n",buf[0],len);
		if ( buf[0] == '0')
			kk_test_bl_status = 0;
		else if ( buf[0] > '0')
			kk_test_bl_status = 1;

		filp_close(bl,NULL);
	}

	return kk_test_bl_status;	
}
#endif
static int kk_test_flag = 0;
static void kk_test_hall_handler(void)
{
		int ret;
		int bl_status;
		printk("===kk_test===%s ==\n",__func__);
	
		ret = gpio_get_value(kk_test_en);
#ifdef USE_LEDS_C
		bl_status = backlight_is_off();
#else		
		bl_status = kk_test_check_bl_status(bl_path);
#endif
		printk ("====kk_test===check io is %d =%d===\n",ret,bl_status);
				
		if ( bl_status == 1 ){
			if (ret == 1){
					//printk("===konka===1===\n");
 					input_report_key(kk_test_input_dev, KEY_POWER, 1);
    			input_sync(kk_test_input_dev);       
    			input_report_key(kk_test_input_dev, KEY_POWER, 0);
    			input_sync(kk_test_input_dev);
    			kk_test_flag = 1;     
  		} else if (ret == 0){
  				printk("===konka===1===\n");
  		}
  	}else if (bl_status == 0){
  		if ( ret == 1)
  				printk("===konka===2===\n");
  		else if (ret == 0){
  	 			input_report_key(kk_test_input_dev, KEY_POWER, 1);
    			input_sync(kk_test_input_dev);       
    			input_report_key(kk_test_input_dev, KEY_POWER, 0);
    			input_sync(kk_test_input_dev);
    			kk_test_flag = 0; 
  		}
  	}
}

static int kk_test_hall_event_handler(void *unused)
{
	struct sched_param param = {.sched_priority = 4};
    sched_setscheduler(current, SCHED_RR, &param);

    do
    {
        enable_irq(kk_test_hall_irq); 
        set_current_state(TASK_INTERRUPTIBLE); 
        wait_event_interruptible(kk_test_hall_waiter, kk_test_hall_flag != 0);
        kk_test_hall_flag = 0;
        set_current_state(TASK_RUNNING);

        kk_test_hall_handler();
    }while(!kthread_should_stop());

    return 0;
}

static void kk_test_init_irq(void)
{   
		int ret=0;
		u32 ints[2] = { 0, 0 };
		struct device_node *node = NULL;
		
		printk("===kk_test==%s==%d==\n",__func__,kk_test_en);

	  kk_test_hall_thread = kthread_run(kk_test_hall_event_handler, 0, "kk_test_hall");
    if (IS_ERR(kk_test_hall_thread))
    { 
        printk("%s() creat thread fail!!\n",__func__);
    }		
    	
		    /*gpio*/
		ret = gpio_request(kk_test_en, "gpio_hall_eint");
		if (ret < 0)
				printk("mt_hall_init gpio request error!\n");
		ret = gpio_direction_input(kk_test_en);
		if (ret < 0)
				printk("mt_hall_init gpio direction error!\n");
		ret = gpio_to_irq(kk_test_en);
		if (ret < 0)
				printk("mt_hall_init gpio to irq error!\n");

#if 1				
		node = of_find_matching_node(node, kk_test_of_match);
		if (node) {
				of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
				gpio_set_debounce(ints[0], 0);

				kk_test_hall_irq = irq_of_parse_and_map(node, 0);
				ret =request_irq(kk_test_hall_irq, (irq_handler_t)kk_test_hall_eint_handler, IRQF_TRIGGER_MASK,"kk_test-eint", NULL);
				if (ret > 0) {
						ret = -1;
						printk("kk_test_hall request_irq IRQ LINE NOT AVAILABLE!.\n");
				}
		} else {
				printk("kk_test_hall request_irq can not find touch eint device node!.\n");
				ret = -1;
		}
		printk("[%s]irq:%d, debounce:%d-%d:\n", __func__, kk_test_hall_irq, ints[0], ints[1]);
#endif	
}


static int kk_test_probe(struct platform_device *pdev)
{
	
		int i,err = 0;
		
		printk("\n\n\n\n====welcome to kk world!!!\n\n\n\n");
		
		kk_test_get_gpio_info(pdev);	
		kk_test_input_dev = input_allocate_device();  //  分配输入设备结构体
		//input_allocate_device()函数在内存中为输入设备结构体分配一个空间,并对其主要的成员进行了初始化.  
    if (!kk_test_input_dev)
                return -ENOMEM;		
		kk_test_input_dev->name = "kk_test";
    kk_test_input_dev->id.bustype = BUS_HOST;
    kk_test_input_dev->id.vendor = 0x0001;
    kk_test_input_dev->id.product = 0x0001;
    kk_test_input_dev->id.version = 0x0001;
        
    __set_bit(EV_KEY, kk_test_input_dev->evbit); //注册输入时间类型为按键类型，不然无法识别报值
		__set_bit(KEY_POWER, kk_test_input_dev->keybit);  //注册电power按键	        
		err = input_register_device(kk_test_input_dev); //注册一个输入设备
    if (err){
        	printk("%s regist input device failed!!!\n",__func__);
          input_free_device(kk_test_input_dev);
          return err;
      }	
		kk_test_init_irq();
		
#ifdef DEVICE_CREAT
		err = device_create_file(&pdev->dev, &dev_attr_kk_test_state);		//dev_attr driver_attr_kk_test_state
		if (err < 0 )
				printk("Failed to create device file(%s)!\n", dev_attr_kk_test_state.attr.name);
#else 
		err = kk_test_create_attr(&kk_test_drv.driver);//err = driver_create_file(&kk_test.driver, &driver_attr_kk_test_state);
		if (err){
				printk("Failed to create device file(%s)!\n", driver_attr_kk_test_state.attr.name);
				kk_test_delete_attr(&kk_test_drv.driver);
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
	
  ret = platform_driver_register(&kk_test_drv);
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
