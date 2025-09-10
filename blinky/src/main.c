#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

 static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
 static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Red led thread initialization
#define STACKSIZE 500
#define PRIORITY 5
void red_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void blue_led_task(void *, void *, void*);
K_THREAD_DEFINE(blue_thread,STACKSIZE,blue_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
//void pause_led_task(void *, void *, void*);
//K_THREAD_DEFINE(pause_thread,STACKSIZE,pause_led_task,NULL,NULL,NULL,PRIORITY,0,0);


// Configure buttons
#define BUTTON_0 DT_ALIAS(sw0)
// #define BUTTON_1 DT_ALIAS(sw1)
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static struct gpio_callback button_0_data;

int led_state = 0;
int old_state = 0;
// Button interrupt handler
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button pressed\n");
	

	if (led_state == 4){
		led_state = old_state;
	}
	else{
		old_state = led_state;
		led_state = 4;

	}
}


// Main program
int main(void)
{
	init_led();
	
	int rebtn = init_button();
	if (rebtn < 0) {
		return 0;
	}

	return 0;
}

int init_button() {

	int rebtn;
	if (!gpio_is_ready_dt(&button_0)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}

	rebtn = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
	if (rebtn != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	rebtn = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
	if (rebtn != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
	gpio_add_callback(button_0.port, &button_0_data);
	printk("Set up button 0 ok\n");
	
	return 0;
}

// Initialize leds
int  init_led() {

	// Led pin initialization
	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	// set led off
	gpio_pin_set_dt(&red,0);

	int reb = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
	if (reb < 0) {
		printk("Error: Led configure failed\n");		
		return reb;
	}
	// set led off
	gpio_pin_set_dt(&blue,0);

	int reg = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (reg < 0) {
		printk("Error: Led configure failed\n");		
		return reg;
	}
	// set led off
	gpio_pin_set_dt(&green,0);

	printk("Led initialized ok\n");
	
	return 0;
}

// Task to handle red led
void red_led_task(void *, void *, void*) {
	
	printk("Red led thread started\n");
	while (true) {
		if (led_state == 0){
		// 1. set led on 
		gpio_pin_set_dt(&red,1);
		printk("Red on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
		printk("Red off\n");
		// 4. sleep for 2 seconds
		if (led_state == 0){
			led_state = 1;
		}
		
		}
		
		k_msleep(10);
	}
}

// Task to handle blue led
void blue_led_task(void *, void *, void*) {
	
	printk("blue led thread started\n");
	while (true) {
			if (led_state == 1){
			// 1. set led on 
			gpio_pin_set_dt(&blue,1);
			printk("blue on\n");
			// 2. sleep for 2 seconds
			k_sleep(K_SECONDS(1));
			// 3. set led off
			gpio_pin_set_dt(&blue,0);
			printk("blue off\n");
			// 4. sleep for 2 seconds
			if (led_state == 1){
				led_state = 2;
			}
			
			}
			k_msleep(10);
	}
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
	
	printk("yellow led thread started\n");
	while (true) {
		if (led_state == 2){
		// 1. set led on 
		gpio_pin_set_dt(&red,1);
		gpio_pin_set_dt(&green,1);
		printk("yellow on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
	 	gpio_pin_set_dt(&green,0);
		printk("yellow off\n");
		// 4. sleep for 2 seconds	
		if (led_state == 2){
			led_state = 0;
		}		
		
		}
		k_msleep(10);
		
	}
}