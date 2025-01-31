#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "interrupt_leds.pio.h"

// Definição dos pinos
const uint pin_led_red = 13;
const uint pin_led_blue = 12;
const uint btn_A = 5;
const uint btn_B = 6;

#define NUM_PIXELS 25
#define OUT_PIN 7

double numeros[2][5][5] = {
    {// Número 1
     {0.0, 0.3, 0.3, 0.3, 0.0},
     {0.0, 0.3, 0.0, 0.3, 0.0},
     {0.0, 0.3, 0.3, 0.3, 0.0},
     {0.0, 0.3, 0.0, 0.3, 0.0},
     {0.0, 0.3, 0.3, 0.3, 0.0}},
    {// Número 2
     {1.0, 0.0, 0.0, 0.0, 1.0},
     {0.0, 1.0, 0.0, 1.0, 0.0},
     {0.0, 0.0, 1.0, 0.0, 0.0},
     {0.0, 1.0, 0.0, 1.0, 0.0},
     {1.0, 0.0, 0.0, 0.0, 1.0}}};

uint32_t matrix_rgb(double r, double g, double b)
{
    return ((uint8_t)(g * 255) << 24) | ((uint8_t)(r * 255) << 16) | ((uint8_t)(b * 255) << 8);
}

void acender_todos_leds(PIO pio, uint sm, double r, double g, double b)
{

    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            uint32_t cor = matrix_rgb(numeros[0][i][j], g, b);
            pio_sm_put_blocking(pio, sm, cor);
        }
    }
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    bool estado_atual = gpio_get(pin_led_blue);
    gpio_put(pin_led_blue, !estado_atual);
}

int main()
{
    PIO pio = pio0;
    set_sys_clock_khz(128000, false);
    stdio_init_all();

    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

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
        acender_todos_leds(pio, sm, 0, 0, 0);
        sleep_ms(1000);
        gpio_put(pin_led_red, false);
        sleep_ms(100);
        gpio_put(pin_led_red, true);
        sleep_ms(100);
    }
}
