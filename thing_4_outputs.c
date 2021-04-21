/*
 * thing_4_outputs.c
 *
 *  Created on: Apr 20, 2021
 *      Author: Krzysztof Zurek
 *		e-mail: krzzurek@gmail.com
 */
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "simple_web_thing_server.h"
#include "thing_4_outputs.h"

//button GPIO
#define GPIO_OUTPUT_1	    (CONFIG_OUTPUT_1_GPIO)
#define GPIO_OUTPUT_2	    (CONFIG_OUTPUT_2_GPIO)
#define GPIO_OUTPUT_3	    (CONFIG_OUTPUT_3_GPIO)
#define GPIO_OUTPUT_4	    (CONFIG_OUTPUT_4_GPIO)
#define GPIO_OUTPUT_MASK	(1ULL << GPIO_OUTPUT_1)|(1ULL << GPIO_OUTPUT_2)|\
							(1ULL << GPIO_OUTPUT_3)|(1ULL << GPIO_OUTPUT_4)

#define CNT_MAX_OUT 1000

//xSemaphoreHandle DRAM_ATTR output_sem;
xSemaphoreHandle DRAM_ATTR thing_mux;
static int32_t DRAM_ATTR output_counter = 0;

thing_t *iot_4_outputs = NULL;
property_t *prop_output_1, *prop_output_2, *prop_output_3, *prop_output_4;
property_t *prop_output_counter, *prop_out_on_off;
at_type_t iot_4_outputs_type, output_1_prop_type, output_3_prop_type;
at_type_t output_4_prop_type;
at_type_t output_counter_prop_type, on_off_prop_type;
static bool out_1, out_2, out_3, out_4;
static bool on_off_state = false;


/* *****************************************************************
 *
 * turn the device ON or OFF
 *
 * *****************************************************************/
int16_t set_output(char *name, char *new_value_str){
	int8_t output_nr = -1;
	bool new_value = false;
	bool prev_on_state;
	int16_t result = 0;
	
	output_counter++;
	if (output_counter > CNT_MAX_OUT){
		output_counter = 0;
	}
	
	if (strcmp(name, prop_output_1 -> id) == 0){
		output_nr = 1;
	}
	else if (strcmp(name, prop_output_2 -> id) == 0){
		output_nr = 2;
	}
	else if (strcmp(name, prop_output_3 -> id) == 0){
		output_nr = 3;
	}
	else if (strcmp(name, prop_output_4 -> id) == 0){
		output_nr = 4;
	}
	else{
		return -1;
	}
	
	if (strcmp(new_value_str, "true") == 0){
		new_value = true;
	}
	
	if (output_nr >= 0){
		switch(output_nr){
			case 1:
				if (new_value == true){
					gpio_set_level(GPIO_OUTPUT_1, 1);
					if (out_1 == false){
						out_1 = true;
						result = 1;
					}
				}
				else{
					gpio_set_level(GPIO_OUTPUT_1, 0);
					if (out_1 == true){
						out_1 = false;
						result = 1;
					}
				}
				break;
			case 2:
				if (new_value == true){
					gpio_set_level(GPIO_OUTPUT_2, 1);
					if (out_2 == false){
						out_2 = true;
						result = 1;
					}
				}
				else{
					gpio_set_level(GPIO_OUTPUT_2, 0);
					if (out_2 == true){
						out_2 = false;
						result = 1;
					}
				}
				break;
			case 3:
				if (new_value == true){
					gpio_set_level(GPIO_OUTPUT_3, 1);
					if (out_3 == false){
						out_3 = true;
						result = 1;
					}
				}
				else{
					gpio_set_level(GPIO_OUTPUT_3, 0);
					if (out_3 == true){
						out_3 = false;
						result = 1;
					}
				}
				break;
			case 4:
				if (new_value == true){
					gpio_set_level(GPIO_OUTPUT_4, 1);
					if (out_4 == false){
						out_4 = true;
						result = 1;
					}
				}
				else{
					gpio_set_level(GPIO_OUTPUT_4, 0);
					if (out_4 == true){
						out_4 = false;
						result = 1;
					}
				}
				break;
		}
	}
	
	prev_on_state = on_off_state;
	if ((out_1 == true)||(out_2 == true)||(out_3 == true)||(out_4 == true)){
		on_off_state = true;
	}
	else{
		on_off_state = false;
	}
	
	if (prev_on_state != on_off_state){
		inform_all_subscribers_prop(prop_out_on_off);
	}
	if (output_counter%10 == 0){
		inform_all_subscribers_prop(prop_output_counter);
	}

	return result;
}


/*************************************************************
 *
 * main button function
 *
 * ************************************************************/
void outputs_fun(void *pvParameter){
	uint32_t cnt = 0;

	printf("Outputs task is ready\n");

	for(;;){
		vTaskDelay(30000 / portTICK_PERIOD_MS);
		cnt++;
		//printf("Output cnt: %i\n", cnt);
	}
}


/*******************************************************************
 *
 * initialize button's GPIO
 *
 * ******************************************************************/
void init_outputs_io(void){
	gpio_config_t io_conf;

	//interrupt on both edges
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//bit mask of the pins
	io_conf.pin_bit_mask = GPIO_OUTPUT_MASK;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//enable pull-up mode
	io_conf.pull_up_en = 0;
	io_conf.pull_down_en = 0;
	gpio_config(&io_conf);
	gpio_set_level(GPIO_OUTPUT_1, 0);
	gpio_set_level(GPIO_OUTPUT_2, 0);
	gpio_set_level(GPIO_OUTPUT_3, 0);
	gpio_set_level(GPIO_OUTPUT_4, 0);
}


/*****************************************************************
 *
 * Initialize button thing and all it's properties and event
 *
 * ****************************************************************/
thing_t *init_4_outputs(void){
	out_1 = false;
	out_2 = false;
	out_3 = false;
	out_4 = false;

	//vSemaphoreCreateBinary(output_sem);
	//xSemaphoreTake(output_sem, 0);
	init_outputs_io();
	
	thing_mux = xSemaphoreCreateMutex();
	//create button thing
	iot_4_outputs = thing_init();	
	iot_4_outputs -> id = "4-outputs";
	iot_4_outputs -> at_context = things_context;
	iot_4_outputs -> model_len = 2000;
	//set @type
	iot_4_outputs_type.at_type = "MultiLevelSwitch";
	iot_4_outputs_type.next = NULL;
	set_thing_type(iot_4_outputs, &iot_4_outputs_type);
	iot_4_outputs -> description = "4 outputs";
	
	//create output-1 property
	prop_output_1 = property_init(NULL, NULL);
	prop_output_1 -> id = "out-1";
	prop_output_1 -> description = "output-1 value";
	output_1_prop_type.at_type = "BooleanProperty";
	output_1_prop_type.next = NULL;
	prop_output_1 -> at_type = &output_1_prop_type;
	prop_output_1 -> type = VAL_BOOLEAN;
	prop_output_1 -> value = &out_1;
	prop_output_1 -> title = "Output-1";
	prop_output_1 -> read_only = false;
	prop_output_1 -> set = set_output;
	prop_output_1 -> mux = thing_mux;
	add_property(iot_4_outputs, prop_output_1); //add property to thing
	
	//create output-2 property
	prop_output_2 = property_init(NULL, NULL);
	prop_output_2 -> id = "out-2";
	prop_output_2 -> description = "output-2 value";
	prop_output_2 -> at_type = &output_1_prop_type;
	prop_output_2 -> type = VAL_BOOLEAN;
	prop_output_2 -> value = &out_2;
	prop_output_2 -> title = "Output-2";
	prop_output_2 -> read_only = false;
	prop_output_2 -> set = set_output;
	prop_output_2 -> mux = thing_mux;
	add_property(iot_4_outputs, prop_output_2); //add property to thing
	
	//create output-3 property
	prop_output_3 = property_init(NULL, NULL);
	prop_output_3 -> id = "out-3";
	prop_output_3 -> description = "output-3 value";
	prop_output_3 -> at_type = &output_1_prop_type;
	prop_output_3 -> type = VAL_BOOLEAN;
	prop_output_3 -> value = &out_3;
	prop_output_3 -> title = "Output-3";
	prop_output_3 -> read_only = false;
	prop_output_3 -> set = set_output;
	prop_output_3 -> mux = thing_mux;
	add_property(iot_4_outputs, prop_output_3); //add property to thing
	
	//create output-4 property
	prop_output_4 = property_init(NULL, NULL);
	prop_output_4 -> id = "out-4";
	prop_output_4 -> description = "output-4 value";
	prop_output_4 -> at_type = &output_1_prop_type;
	prop_output_4 -> type = VAL_BOOLEAN;
	prop_output_4 -> value = &out_4;
	prop_output_4 -> title = "Output-4";
	prop_output_4 -> read_only = false;
	prop_output_4 -> set = set_output;
	prop_output_4 -> mux = thing_mux;
	add_property(iot_4_outputs, prop_output_4); //add property to thing
	
	//create pushed property
	prop_out_on_off = property_init(NULL, NULL);
	prop_out_on_off -> id = "on-off";
	prop_out_on_off -> description = "outputs value";
	on_off_prop_type.at_type = "OnOffProperty";
	on_off_prop_type.next = NULL;
	prop_out_on_off -> at_type = &on_off_prop_type;
	prop_out_on_off -> type = VAL_BOOLEAN;
	prop_out_on_off -> value = &on_off_state;
	prop_out_on_off -> title = "ON-OFF";
	prop_out_on_off -> read_only = true;
	prop_out_on_off -> set = NULL;
	prop_out_on_off -> mux = thing_mux;
	on_off_state = false;
	add_property(iot_4_outputs, prop_out_on_off); //add property to thing
	
	//create on-off counter property
	prop_output_counter = property_init(NULL, NULL);
	prop_output_counter -> id = "counter";
	prop_output_counter -> description = "output counter";
	output_counter_prop_type.at_type = "LevelProperty";
	output_counter_prop_type.next = NULL;
	prop_output_counter -> at_type = &output_counter_prop_type;
	prop_output_counter -> type = VAL_INTEGER;
	prop_output_counter -> value = &output_counter;
	prop_output_counter -> max_value.int_val = CNT_MAX_OUT;
	prop_output_counter -> min_value.int_val = 0;
	prop_output_counter -> title = "Counter";
	prop_output_counter -> read_only = true;
	prop_output_counter -> set = NULL;
	prop_output_counter -> mux = thing_mux;
	add_property(iot_4_outputs, prop_output_counter); //add property to thing
	
	//if (output_sem != NULL){
		xTaskCreate(&outputs_fun, "outputs_task",
					configMINIMAL_STACK_SIZE * 4, NULL, 0, NULL);
	//}

	return iot_4_outputs;
}
