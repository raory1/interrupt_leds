#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "interrupt_leds.pio.h"

// Definição dos pinos
const uint PIN_LED_RED = 13;
const uint PIN_LED_BLUE = 12;
const uint BTN_A = 5;
const uint BTN_B = 6;

#define NUM_PIXELS 25
#define OUT_PIN 7

volatile bool red_led_state = false;
volatile int current_pattern = 0;
volatile absolute_time_t last_press_time = 0;

PIO pio;
uint sm;

double numeros[11][5][5] = {
    {
        // Apagar
        {0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0},
    },
    {
        // Número 0
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 1
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.1, 0.0, 0.0},
        {0.0, 0.0, 0.1, 0.0, 0.0},
        {0.0, 0.1, 0.1, 0.0, 0.0},
        {0.0, 0.0, 0.1, 0.0, 0.0},
    },
    {
        // Número 2
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 3
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 4
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
    },
    {
        // Número 5
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 6
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 7
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 8
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 9
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
};

uint32_t matrix_rgb(double r, double g, double b)
{
    return ((uint8_t)(g * 255) << 24) | ((uint8_t)(r * 255) << 16) | ((uint8_t)(b * 255) << 8);
}

void acender_todos_leds(PIO pio, uint sm, int current_pattern)
{
    for (int j = 0; j < 5; j++)
    {
        for (int k = 0; k < 5; k++)
        {
            uint32_t cor = matrix_rgb(numeros[current_pattern][j][k], 0, 0);
            pio_sm_put_blocking(pio, sm, cor);
        }
    }
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    bool btn_last_state = false;

    uint32_t current_time = to_us_since_boot(get_absolute_time());
    bool btn_pressed = !gpio_get(gpio);

    if (btn_pressed && !btn_last_state &&
        (absolute_time_diff_us(last_press_time, get_absolute_time()) > 200000))
    {
        last_press_time = get_absolute_time();
        btn_last_state = true;
        if (gpio == BTN_A && current_pattern > 0)
        {
            printf("Botão A pressionado\n");
            current_pattern--;
            acender_todos_leds(pio, sm, current_pattern);
        }

        else if (gpio == BTN_B && current_pattern < 10)
        {
            printf("Botão B pressionado\n");
            current_pattern++;
            acender_todos_leds(pio, sm, current_pattern);
        }
    }

    else if (!btn_pressed)
    {
        printf("Botão liberado");
        printf("\n");
        btn_last_state = false;
    }
}

bool blink_led(struct repeating_timer *t)
{
    red_led_state = !red_led_state;
    gpio_put(PIN_LED_RED, red_led_state);
    return true;
}

int main()
{
    pio = pio0;
    set_sys_clock_khz(128000, false);
    stdio_init_all();

    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    // Inicialização e configuração do LED
    gpio_init(PIN_LED_RED);
    gpio_set_dir(PIN_LED_RED, GPIO_OUT);
    gpio_init(PIN_LED_BLUE);
    gpio_set_dir(PIN_LED_BLUE, GPIO_OUT);

    // Inicialização e configuração dos botões
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    acender_todos_leds(pio, sm, current_pattern);

    struct repeating_timer timer;
    add_repeating_timer_ms(-200, blink_led, NULL, &timer);
    //  Loop principal, LED vermelho pisca constantemente

    while (true)
    {
        tight_loop_contents();
    }
}