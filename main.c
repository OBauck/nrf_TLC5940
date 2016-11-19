/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_error.h"
#include "nrf_drv_tlc5940.h"


int main(void)
{

	nrf_drv_tlc5940_init();

    while (1)
    {
		int direction = 1;
        for (int channel = 0; channel < 16; channel += direction)
		{
			nrf_drv_tlc5940_clear();
			
			if (channel == 0) 
			{
				direction = 1;
			} 
			else 
			{
				nrf_drv_tlc5940_set(channel - 1, 1000);
			}
			
			nrf_drv_tlc5940_set(channel, 4095);
			
			if (channel != 15) 
			{
				nrf_drv_tlc5940_set(channel + 1, 1000);
			} 
			else 
			{
				direction = -1;
			}
			
			nrf_drv_tlc5940_update();
			nrf_delay_ms(50);
		}

    }
}
