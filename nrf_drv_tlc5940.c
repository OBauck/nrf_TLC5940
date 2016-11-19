
#include <stdio.h>
#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"
#include "app_util_platform.h"
#include "nrf_drv_spi.h"

#define GSCLK_PIN	22
#define BLANK_PIN	23
#define SCLK_PIN	24
#define SIN_PIN		25
#define XLAT_PIN	26

#define SPI_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done = true;

#define NR_OF_CHANNELS	16
#define NR_OF_SPI_VALUES	24		//NR_OF_CHANNELS * 12bit/8bit

static uint16_t tlc5940_values[NR_OF_CHANNELS];
static uint8_t tlc5940_spi_values[NR_OF_SPI_VALUES];

void gsclk_init()
{
	nrf_gpio_cfg_output(GSCLK_PIN);
    nrf_gpio_pin_set(GSCLK_PIN);
    
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    
    NRF_GPIOTE->CONFIG[0] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
                            GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos |
                            GSCLK_PIN << GPIOTE_CONFIG_PSEL_Pos | 
                            GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos;
                            
    NRF_TIMER1->PRESCALER = 0;
    NRF_TIMER1->CC[0] = 1;
    NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
    
    NRF_PPI->CH[0].EEP = (uint32_t) &NRF_TIMER1->EVENTS_COMPARE[0];
    NRF_PPI->CH[0].TEP = (uint32_t) &NRF_GPIOTE->TASKS_OUT[0];
    
    NRF_PPI->CHENSET = PPI_CHENSET_CH0_Enabled << PPI_CHENSET_CH0_Pos;
	
	NRF_TIMER1->TASKS_START = 1;
}

void blank_init()
{
	nrf_gpio_cfg_output(BLANK_PIN);
	nrf_gpio_pin_clear(BLANK_PIN);	//enables LEDs
	
	NRF_GPIOTE->CONFIG[1] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
                            GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos |
                            BLANK_PIN << GPIOTE_CONFIG_PSEL_Pos | 
                            GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos;
                            
    NRF_TIMER2->PRESCALER = 0;
    NRF_TIMER2->CC[0] = 4095;
	NRF_TIMER2->CC[1] = 4096;
    NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos;
    
    NRF_PPI->CH[1].EEP = (uint32_t) &NRF_TIMER2->EVENTS_COMPARE[0];
    NRF_PPI->CH[1].TEP = (uint32_t) &NRF_GPIOTE->TASKS_OUT[1];
    
    NRF_PPI->CHENSET = PPI_CHENSET_CH1_Enabled << PPI_CHENSET_CH1_Pos;
	
	NRF_PPI->CH[2].EEP = (uint32_t) &NRF_TIMER2->EVENTS_COMPARE[1];
    NRF_PPI->CH[2].TEP = (uint32_t) &NRF_GPIOTE->TASKS_OUT[1];
    
    NRF_PPI->CHENSET = PPI_CHENSET_CH2_Enabled << PPI_CHENSET_CH2_Pos;
	
	NRF_TIMER2->TASKS_START = 1;
}

void spi_event_handler(nrf_drv_spi_evt_t const * p_event)
{
	spi_xfer_done = true;
	
	//toggle xlat to show the new values
	nrf_gpio_pin_set(XLAT_PIN);
	nrf_gpio_pin_clear(XLAT_PIN);
}

void spi_init()
{
	uint32_t err_code;
	
	nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
	
	spi_config.frequency = NRF_DRV_SPI_FREQ_8M;
    spi_config.mosi_pin = SIN_PIN;
    spi_config.sck_pin  = SCLK_PIN;
	
	err_code = nrf_drv_spi_init(&spi, &spi_config, spi_event_handler);
    APP_ERROR_CHECK(err_code);
}

void nrf_drv_tlc5940_init()
{
	nrf_gpio_cfg_output(XLAT_PIN);
	nrf_gpio_pin_clear(XLAT_PIN);
	
	gsclk_init();
	blank_init();
	spi_init();
	
	memset(tlc5940_values, 0, sizeof(tlc5940_values)/sizeof(tlc5940_values[0]));
}

void nrf_drv_tlc5940_set(uint8_t channel, uint16_t value)
{
	tlc5940_values[channel] = value;
}

void nrf_drv_tlc5940_clear()
{
	memset(tlc5940_values, 0, sizeof(tlc5940_values));
}

uint32_t nrf_drv_tlc5940_update()
{
	uint32_t err_code;
	
	if(spi_xfer_done == false)
	{
		return NRF_ERROR_BUSY;
	}
	
	//translate 12 bit values stored in tlc5940_values to 8 bit values in tlc5940_spi_values
	uint8_t counter = 0;
	uint8_t rest = 0;
	
	for(int i = 0; i < NR_OF_SPI_VALUES; i++)
	{
		rest = i % 3;
		
		if( rest == 0)
		{
			tlc5940_spi_values[i] = tlc5940_values[counter] & 0xff;
		}
		else if( rest == 1)
		{
			tlc5940_spi_values[i] = ((tlc5940_values[counter] >> 8) & 0x0f) + (tlc5940_values[counter+1] & 0x0f);
		}
		else
		{
			tlc5940_spi_values[i] = ((tlc5940_values[counter+1] >> 4) & 0xff);
			counter += 2;
		}
	}
	
	spi_xfer_done = false;
	err_code = nrf_drv_spi_transfer(&spi, tlc5940_spi_values, NR_OF_SPI_VALUES, NULL, 0);
	APP_ERROR_CHECK(err_code);
	
	return NRF_SUCCESS;
}
