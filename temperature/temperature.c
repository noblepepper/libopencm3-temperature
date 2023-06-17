/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Thomas Otto <tommi@viadmin.org>
 * Copyright (C) 2012 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2012 Ken Sarkies <ksarkies@internode.on.net>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>

#define LED_GREEN_PORT GPIOB
#define LED_GREEN_PIN GPIO3

#define USART_CONSOLE USART2  /* PA2/15 , af7 */

#include "util.c"
#include "setup.c"

static void setup(void)
{
	clock_setup();
	systick_setup();
	gpio_setup();
	usart_setup();
	adc_setup();
}


int main(void)
{
	int i;
	uint8_t channel_array[16];
	uint16_t reading = 0;
	uint32_t averages[9]= {0,0,0,0,0,0,0,0,0};
	uint16_t num_readings = 0;
	uint16_t cmd;
	int status = 0; /* 0 = waiting 1 = running */
	setup();
	gpio_clear(LED_GREEN_PORT, LED_GREEN_PIN);		/* LED off */
	delay_ms(1000);
	printf("\r\nHello from your stm nucleo l432kc\r\n");
	for ( i = 0; i <10; i++){
		delay_ms(100);
		gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
	}
	delay_ms(1000);
	/* Analog in on PA0-5 PA1-6 PA3-8 PA4-9 PA5-10 PA6-11 PA7-12 */
	/*              PB0-15 PB1-16 */
	while(1){
		cmd = get_cmd();
		if (cmd == 'G'){
			status = 1;
		}
		while (status == 1){
			if (num_readings < 65535){
				for (i = 5; i < 7; i++){
					channel_array[0] = i;
					adc_set_regular_sequence(ADC1, 1, channel_array);
					adc_start_conversion_regular(ADC1);
//					Wait for end of conversion.
					while (!(adc_eoc(ADC1)));
					reading = adc_read_regular(ADC1);
					averages[i - 5] += reading;
					gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
				}
				for (i = 8; i < 13; i++){
				channel_array[0] = i;
					adc_set_regular_sequence(ADC1, 1, channel_array);
					adc_start_conversion_regular(ADC1);
//					Wait for end of conversion.
					while (!(adc_eoc(ADC1)));
					reading = adc_read_regular(ADC1);
					averages[i - 6] += reading;
					gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN); 
				}
				for (i = 15; i < 17; i++){
					channel_array[0] = i;
					adc_set_regular_sequence(ADC1, 1, channel_array);
					adc_start_conversion_regular(ADC1);
//					Wait for end of conversion.
					while (!(adc_eoc(ADC1)));
					reading = adc_read_regular(ADC1);
					averages[i - 8] += reading;
					gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
				}
				num_readings++;
			}
			cmd = get_cmd();
			if ( cmd == 'S' ){
				status = 0;
				if (num_readings !=0)
				{	printf("numpoints %i ", num_readings);
					for ( i = 0; i < 9; i++)
						printf("t%i %li ", i+1, averages[i] / num_readings);
					printf("\r\n");
				}
				for ( i = 0; i < 9; i++)
					averages[i] = 0;
				num_readings = 0;
			}
		}
	}
	return 0;
}
