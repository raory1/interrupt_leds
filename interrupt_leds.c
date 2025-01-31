#include <stdio.h>
#include "pico/stdlib.h"

// Definição dos pinos
const uint pin_led_red = 13;

int main()
{
    stdio_init_all();
    // Inicialização e configuração do LED
    gpio_init(pin_led_red);
    gpio_set_dir(pin_led_red, GPIO_OUT);
 
    // Loop principal, LED vermelho pisca constantemente
    while (true){
        gpio_put(pin_led_red, false);
        sleep_ms(100);
        gpio_put(pin_led_red, true);
        sleep_ms(100);
    }
}
