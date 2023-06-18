
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>

#define LED_GREEN_PORT GPIOB
#define LED_GREEN_PIN GPIO3

#define USART_CONSOLE USART2  /* PA2/15 , af7 */

void systick_setup(void);

int _write(int file, char *ptr, int len);


static void clock_setup(void)
{
	/* FIXME - this should eventually become a clock struct helper setup */
	rcc_osc_on(RCC_HSI16);
	
	flash_prefetch_enable();
	flash_set_ws(4);
	flash_dcache_enable();
	flash_icache_enable();
	/* 16MHz / 4 = > 4 * 40 = 160MHz VCO => 80MHz main pll  */
	rcc_set_main_pll(RCC_PLLCFGR_PLLSRC_HSI16, 4, 40,
			0, 0, RCC_PLLCFGR_PLLR_DIV2);
	rcc_osc_on(RCC_PLL);
	/* either rcc_wait_for_osc_ready() or do other things */

	/* Enable clocks for the ports we need */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOE);

	/* Enable clocks for peripherals we need */
	rcc_periph_clock_enable(RCC_USART2);
        rcc_periph_clock_enable(RCC_TIM7);
	rcc_periph_clock_enable(RCC_SYSCFG);
	rcc_periph_clock_enable(RCC_ADC1);
	rcc_periph_clock_enable(RCC_DMA1);


	rcc_set_sysclk_source(RCC_CFGR_SW_PLL); /* careful with the param here! */
	rcc_wait_for_sysclk_status(RCC_PLL);
	/* FIXME - eventually handled internally */
	rcc_ahb_frequency = 80e6;
	rcc_apb1_frequency = 80e6;
	rcc_apb2_frequency = 80e6;
}

static void usart_setup(void)
{
	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2|GPIO15);

	/* Setup USART2 TX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2);
	
	/* Setup GPIO pins for USART2 receive. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO15);
	gpio_set_af(GPIOA, GPIO_AF3, GPIO15);
	
	usart_set_baudrate(USART_CONSOLE, 115200);
	usart_set_databits(USART_CONSOLE, 8);
	usart_set_stopbits(USART_CONSOLE, USART_STOPBITS_1);
	usart_set_mode(USART_CONSOLE, USART_MODE_TX_RX);
	usart_set_parity(USART_CONSOLE, USART_PARITY_NONE);
	usart_set_flow_control(USART_CONSOLE, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART_CONSOLE);
}

static void gpio_setup(void)
{
	/* Green Led */
	/* Set GPIO3 (in GPIO port B) to 'output push-pull'. */
	gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
			LED_GREEN_PIN);
	/* Analog in on PA0-5 PA1-6 PA3-8 PA4-9 PA5-10 PA6-11 PA7-12 */
	/*              PB0-15 PB1-16 */
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, 
			GPIO0|GPIO1|GPIO3|GPIO4|GPIO5|GPIO6|GPIO7);
	gpio_mode_setup(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, 
			GPIO0|GPIO1);
}

static void adc_setup(void)
{
	int i;
	rcc_set_peripheral_clk_sel(ADC1, 3);
	rcc_periph_clock_enable(RCC_ADC1);
	rcc_periph_reset_release(RST_ADC1);
	adc_disable_deeppwd(ADC1);
	adc_enable_regulator(ADC1);
	/* Wait for ADC starting up. */
	delay_us(25);
	adc_power_off(ADC1);
	adc_calibrate(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_247DOT5CYC);
	//set oversample ratio
	adc_set_ovrsmp_ratio(ADC1, ADC_CFGR2_OVSR_256x);
	//set oversample shift
	adc_set_ovrsmp_shift(ADC1, 4 );
	adc_enable_reg_ovrsmp(ADC1);
	adc_power_on(ADC1);
}

void systick_setup(void)
{
	systick_set_frequency(1000000, 80000000);
	systick_interrupt_enable();
	systick_counter_enable();
}
