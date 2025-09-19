#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>
#include <zephyr/drivers/uart.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <ctype.h>


// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
 static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
 static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Red led thread initialization
#define STACKSIZE 500
#define PRIORITY 5

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define MAX_REPEAT_BUFFER 64


K_FIFO_DEFINE(dispatcher_fifo);

struct data_t {
	/*************************
	// Add fifo_reserved below
	*************************/
	void *fifo_reserved;
	char msg[20];
};

int init_uart(void) {
	// UART initialization
	if (!device_is_ready(uart_dev)) {
		return 1;
	} 
	return 0;
}


void red_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void blue_led_task(void *, void *, void*);
K_THREAD_DEFINE(blue_thread,STACKSIZE,blue_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
//void pause_led_task(void *, void *, void*);
//K_THREAD_DEFINE(pause_thread,STACKSIZE,pause_led_task,NULL,NULL,NULL,PRIORITY,0,0);

struct k_thread red_thread_data;
struct k_thread green_thread_data;
struct k_thread blue_thread_data;
struct k_thread dispatcher_thread_data;

K_SEM_DEFINE(red_sem, 0, 1);
K_SEM_DEFINE(green_sem, 0, 1);
K_SEM_DEFINE(blue_sem, 0, 1);

// Semaphore for release signal
K_SEM_DEFINE(release_sem, 0, 1);


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
	// Tekijät Taneli Rounioja ja Mikael Rounioja

	// Saatiin 2 pisteeseen asti (lisättiin toisto)
	init_led();
	
	int rebtn = init_button();
	if (rebtn < 0) {
		return 0;
	}

	

	int reu = init_uart();
	if (reu != 0) {
		printk("UART initialization failed!\n");
		return reu;
	}

	

	return 0;

	
}

/********************
 * UART task
 */
static void uart_task(void *unused1, void *unused2, void *unused3)
{
	// Received character from UART
	char rc=0;
	// Message from UART
	char uart_msg[20];
	memset(uart_msg,0,20);
	int uart_msg_cnt = 0;

	while (true) {
		// Ask UART if data available
		if (uart_poll_in(uart_dev,&rc) == 0) {
			// printk("Received: %c\n",rc);
			// If character is not newline, add to UART message buffer
			if (rc != '\r') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
				printk("UART msg: %s\n", uart_msg);
                
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}

				// Copy UART message to dispatcher data
				//strncpy(buf->msg, 20, uart_msg); // mitä ihmettä, miksi kaatuu!!
				snprintf(buf->msg, 20, "%s", uart_msg);

				// You need to:
				// Put dispatcher data to FIFO buffer
				k_fifo_put(&dispatcher_fifo, buf);
				printk("fifo data: %s\n",buf->msg);

				// Clear UART receive buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);

				// Clear UART message buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);
			}
		}
		k_msleep(10);
	}
	return 0;
}

/********************
 * Dispatcher task
 */
static void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {
		// Receive dispatcher data from uart_task fifo
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence,rec_item->msg,20);
		k_free(rec_item);
		char repeat_buffer[MAX_REPEAT_BUFFER];
   		int repeat_buffer_len = 0;

		int i = 0;

		printk("Dispatcher: %s\n", sequence);

		
		while (sequence[i] != '\0') {
        char ch = sequence[i];

        if (ch == 'R' || ch == 'G' || ch == 'Y') {
            // Signal the corresponding light
            switch (ch) {
                case 'R': k_sem_give(&red_sem); break;
                case 'G': k_sem_give(&green_sem); break;
                case 'Y': k_sem_give(&blue_sem); break;
            }

            // Save to repeat buffer
            if (repeat_buffer_len < MAX_REPEAT_BUFFER) {
                repeat_buffer[repeat_buffer_len++] = ch;
            }

            // Wait for light task to finish
            k_sem_take(&release_sem, K_FOREVER);
            i++;

        } else if (ch == 'T') {
            // Parse the number after T
            i++; // move past 'T'
            int repeat_count = 0;
            while (isdigit(sequence[i])) {
                repeat_count = repeat_count * 10 + (sequence[i] - '0');
                i++;
            }

            // Repeat the buffer n times
            for (int r = 0; r < repeat_count; r++) {
                for (int j = 0; j < repeat_buffer_len; j++) {
                    char rep_ch = repeat_buffer[j];
                    switch (rep_ch) {
                        case 'R': k_sem_give(&red_sem); break;
                        case 'G': k_sem_give(&green_sem); break;
                        case 'Y': k_sem_give(&blue_sem); break;
                    }
                    k_sem_take(&release_sem, K_FOREVER);
                }
            }

            
        } else {
            printk("Invalid character: %c\n", ch);
            i++;
        }
    }

    printk("Dispatcher done.\n");


        // You need to:
        // Parse color and time from the fifo data
        // Example
        //    char color = sequence[0];
        //    int time = atoi(sequence+2);
		//    printk("Data: %c %d\n", color, time);
        // Send the parsed color information to tasks using fifo
        // Use release signal to control sequence or k_yield
	}
}

K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);

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
		k_sem_take(&red_sem, K_FOREVER);

		//if (led_state == 0){
		// 1. set led on 
		gpio_pin_set_dt(&red,1);
		//printk("Red on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
		//printk("Red off\n");
		// 4. sleep for 2 seconds
		/*if (led_state == 0){
			led_state = 1;
		}*/
		
		k_msleep(10);
		k_sem_give(&release_sem);

	}
}

// Task to handle blue led
void blue_led_task(void *, void *, void*) {
	
	printk("green led thread started\n");
	while (true) {
			k_sem_take(&green_sem, K_FOREVER);
			//if (led_state == 1){
			// 1. set led on 
			gpio_pin_set_dt(&green,1);
			//printk("blue on\n");
			// 2. sleep for 2 seconds
			k_sleep(K_SECONDS(1));
			// 3. set led off
			gpio_pin_set_dt(&green,0);
			//printk("blue off\n");
			// 4. sleep for 2 seconds
			/*if (led_state == 1){
				led_state = 2;
			}*/
			
			
			
			k_msleep(10);
			k_sem_give(&release_sem);
	}
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
	
	printk("yellow led thread started\n");
	while (true) {
		k_sem_take(&blue_sem, K_FOREVER);
		//if (led_state == 2){
		// 1. set led on 
		gpio_pin_set_dt(&red,1);
		gpio_pin_set_dt(&green,1);
		//printk("yellow on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
	 	gpio_pin_set_dt(&green,0);
		//printk("yellow off\n");
		// 4. sleep for 2 seconds	
		/*if (led_state == 2){
			led_state = 0;
		}*/		
		
		
		k_msleep(10);
		k_sem_give(&release_sem);
	}
}
