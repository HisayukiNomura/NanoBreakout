#include "lcd/lcd.h"
#include "gd32vf103.h"
#include "fatfs/tf_card.h"
#include "systick.h"
#include <stdio.h>
#include "game.h"

void printf_debug_init()
{
    // GPIOのAが、USART0なのでGPIOAにクロックを供給する
    rcu_periph_clock_enable(RCU_GPIOA);
    // USART0にクロックを供給
    rcu_periph_clock_enable(RCU_USART0);

    // TX、RXがGPIOAの9と10に出ているので、それぞれを初期化する。
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9); // USART0 TX
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10); // USART0 RX

    // TXの設定を行う。デバッグでは、ターミナルへの出力だけを行うので、最低限の設定でよい。
    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200U);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);

    //USARTを有効にする
    usart_enable(USART0);
}


int _put_char(int ch)
{
    usart_data_transmit(USART0, (uint8_t) ch );
    while ( usart_flag_get(USART0, USART_FLAG_TBE)== RESET){
    }
    return ch;
}



int main( void ) 
{

    printf_debug_init();
    led_init();
    Game(TRUE);
    
	while(1){
	}
};	