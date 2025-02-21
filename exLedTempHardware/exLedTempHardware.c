#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

const uint LED_PIN = 12; //pino do led

//funcao de interrupcao do temporizador
bool repeating_timer_callback(struct repeating_timer *t)
{
    //altera o estado do led
    static bool led_on = false;
    led_on = !led_on;
    gpio_put(LED_PIN, led_on);

    //mensagem opcional (depuracao)
    printf("LED %s\n", led_on ? "ligado" : "desligado");
    return true; //retorna true para continuar repetindo a interrupcao
}

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    struct repeating_timer timer; //configurando um temporizador de repeticao

    //configurando o temporizador para chamar a funcao a cada 1 segundo 
    add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &timer);
    while (true)
    {
        //loop principal livre para outras tarefas
    }
}
