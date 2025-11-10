#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define COMMAND_OK	0
#define TIME_LEN_ERROR -1
#define TIME_ARRAY_ERROR -2
#define TIME_VALUE_ERROR -3
#define TIME_ZERO_ERROR -4

int main(void)
{
	// UART initialization
	if (!device_is_ready(uart_dev)) {
		printk("UART initialization failed!\r\n");
		return 0;
	} 

	// Wait for everything to initialize and threads to start
	k_msleep(100);
	// Sanity check
	//printk("Started serial led example\n");

	/************************************************
	 TEKIJÄT TANELI ROUNIOJA JA MIKAEL ROUNIOJA
         TEHTIIN 2 PISTEESEEN ASTI (LISÄTTIIN LISÄTESTEJÄ)
	 ***********************************************/
	char rc=0;
	char uart_msg[20];
	int uart_msg_cnt=0;
	memset(uart_msg,0,20);
			
	while (true) {
		// Ask UART if data available
		while (uart_poll_in(uart_dev,&rc) == 0) {
			// Add characters into buffer until X is received 
			if (rc != 'X') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			} else {
				// Send response to UART
				// X has been removed, so we need to add it to the end
				// printk("-1X"); example return error

				// Example check with time_parse
				
                                int timer_delay = time_parse(uart_msg);
                                printk("%dX",timer_delay);
				//printk("%dX",timer_delay);
				//printk("-1X");
				//uart_poll_out(uart_dev, '-');  // Send '-'
				//uart_poll_out(uart_dev, '1');  // Send '1'
				//uart_poll_out(uart_dev, 'X');  // Send 'X'
				// Clear UART message buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);			
			}
		}
		k_msleep(10);
	}
	return 0;
}

int time_parse(char *time) {

    if (time == NULL || strlen(time) != 6) {
        return TIME_LEN_ERROR; // Return length error
    }
	if (strcmp(time, "000000") == 0) {
    return TIME_ZERO_ERROR;
}

    // Initialize values for hours, minutes, and seconds
    int values[3] = {0}; // [0] = hours, [1] = minutes, [2] = seconds
    int seconds = 0;

    
    values[2] = atoi(time + 4); // seconds 
    time[4] = 0; 
    values[1] = atoi(time + 2); // minutes 
    time[2] = 0; 
    values[0] = atoi(time); // hours

    
    if (values[0] < 0 || values[0] > 23) {
        return TIME_VALUE_ERROR; // Invalid hour
    }
    if (values[1] < 0 || values[1] > 59) {
        return TIME_VALUE_ERROR; // Invalid minute
    }
    if (values[2] < 0 || values[2] > 59) {
        return TIME_VALUE_ERROR; // Invalid second
    }

    
    seconds = (values[0] * 3600) + (values[1] * 60) + values[2];

    return seconds;
}
/*
int time_parse(char *str) {
	// testi: palauttaa virhekoodin -1
	return -1;
}*/