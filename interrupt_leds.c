#include <stdio.h>
#include "pico/stdlib.h"

// Definição dos pinos
const uint pin_led_red = 13;
const uint pin_led_blue = 12;
const uint btn_A = 5;
const uint btn_B = 6;

void gpio_irq_handler(uint gpio, uint32_t events)
{
    bool estado_atual = gpio_get(pin_led_blue);
    gpio_put(pin_led_blue, !estado_atual);
}

int main()
{
    stdio_init_all();
    // Inicialização e configuração do LED
    gpio_init(pin_led_red);
    gpio_set_dir(pin_led_red, GPIO_OUT);
    gpio_init(pin_led_blue);
    gpio_set_dir(pin_led_blue, GPIO_OUT);

    // Inicialização e configuração dos botões
    gpio_init(btn_A);
    gpio_set_dir(btn_A, GPIO_IN);
    gpio_pull_up(btn_A);
    gpio_init(btn_B);
    gpio_set_dir(btn_B, GPIO_IN);
    gpio_pull_up(btn_B);

    gpio_set_irq_enabled_with_callback(btn_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(btn_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Loop principal, LED vermelho pisca constantemente
    while (true)
    {
        gpio_put(pin_led_red, false);
        sleep_ms(100);
        gpio_put(pin_led_red, true);
        sleep_ms(100);
    }
}
