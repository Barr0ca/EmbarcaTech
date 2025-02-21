#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"

const uint LED = 12; //pino do LED
const uint16_t PERIODO = 2000; //periodo do PWM (valor maximo do contador)
const float DIVISOR_PWM = 16.0; //divisor fracional do clock para o PWM
const uint16_t PASSO_LED = 100; //passo de incremento/drecremento para o duty cycle do LED
uint16_t nivel_led = 100; //nivel inicial do PWM (duty cycle)

void setup_pwm()
{
    uint slice;
    gpio_set_function(LED, GPIO_FUNC_PWM); //configurando o pino do LED como saida PWM
    slice = pwm_gpio_to_slice_num(LED); //pega o slice associado ao pino do PWM
    pwm_set_clkdiv(slice, DIVISOR_PWM); //define o divisor de clock para o PWM
    pwm_set_wrap(slice, PERIODO); //configura o valor maximo do contador (periodo do PWM)
    pwm_set_gpio_level(LED, nivel_led); //nivel inicial do PWM para o pino do LED
    pwm_set_enabled(slice, true); //habilita o PWM no slice correspondente

}

int main()
{
    uint up_down = 1;
    stdio_init_all();
    setup_pwm();
    while (true)
    {   
        pwm_set_gpio_level(LED, nivel_led); //nivel atual do PWM
        sleep_ms(1000); //atraso de 1 segundo
        if (up_down)
        {
            nivel_led += PASSO_LED; //incrementa o nivel do LED
            if (nivel_led >= PERIODO)
                up_down = 0; //muda direcao apos atingir o nivel maximo
        }
        else
        {
            nivel_led -= PASSO_LED; //decrementa o nivel do LED
            if (nivel_led <= PASSO_LED)
                up_down = 1; //muda de direcao apos atingir o nivel minimo
        }
    }
}
