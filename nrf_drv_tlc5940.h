#ifndef NRF_DRV_TLC5940_H
#define NRF_DRV_TLC5940_H

#include "nrf.h"

void nrf_drv_tlc5940_init(void);
void nrf_drv_tlc5940_set(uint8_t channel, uint16_t value);
void nrf_drv_tlc5940_clear(void);
uint32_t nrf_drv_tlc5940_update(void);

#endif //NRF_DRV_TLC5940_H
