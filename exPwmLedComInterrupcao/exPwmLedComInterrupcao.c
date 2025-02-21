#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"

const uint LED = 12;                        //pino do LED
const uint16_t PERIODO = 2000;              //periodo do PWM (valor maximo do contador)
const float DIVISOR_PWM = 16.0;             //divisor fracional do clock para o PWM
const uint16_t PASSO_LED = 100;             //passo de incremento/drecremento para o duty cycle do LED
const uint32_t PWM_ATUALIZA_NIVEL = 40000;  //contagem para suavizar transicao do LED
uint16_t nivel_led = 100;                   //nivel inicial do PWM (duty cycle)

void pwm_irq_handler()
{
    static uint alto_baixo = 1;                  //verifica se o led esta em nivel baixo ou alto
    static uint32_t contador = 0;                //contador para verificar a frequencia do duty cycle
    uint32_t slice =  pwm_get_irq_status_mask(); //status da interrupcao do PWM
    pwm_clear_irq(slice);                        //limpa interrupcoes no slice correspondente
    if (contador++ < PWM_ATUALIZA_NIVEL) return; //verifica se o contador atingiu o nivel de atualizacao 
    contador = 0;                                //reseta o contador para a proxima verificacao
    if (alto_baixo)
    {
        nivel_led += PASSO_LED; //incrementa o nivel do LED
        if (nivel_led >= PERIODO)
            alto_baixo = 0; //muda direcao apos atingir o nivel maximo
    }
    else
    {
        nivel_led -= PASSO_LED; //decrementa o nivel do LED
        if (nivel_led <= PASSO_LED)
            alto_baixo = 1; //muda de direcao apos atingir o nivel minimo
    }
    pwm_set_gpio_level(LED, nivel_led); //nivel atual do PWM
}

void setup_pwm()
{
    uint slice;

    gpio_set_function(LED, GPIO_FUNC_PWM);  //configurando o pino do LED como saida PWM
    slice = pwm_gpio_to_slice_num(LED);     //pega o slice associado ao pino do PWM
    pwm_set_clkdiv(slice, DIVISOR_PWM);     //define o divisor de clock para o PWM
    pwm_set_wrap(slice, PERIODO);           //configura o valor maximo do contador (periodo do PWM)
    pwm_set_gpio_level(LED, nivel_led);     //nivel inicial do PWM para o pino do LED
    pwm_set_enabled(slice, true);           //habilita o PWM no slice correspondente

    //configuracao da interrupcao do PWM
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_irq_handler);  //define o handler da interrupcao
    pwm_clear_irq(slice);                                      //limpa interrupcoes pendentes
    pwm_set_irq_enabled(slice, true);                          //habilita interrupcoes para o slice do PWM
    irq_set_enabled(PWM_IRQ_WRAP, true);                       //habilita interrupcoes globais para o PWM
}

int main()
{
    stdio_init_all();
    setup_pwm();

    while (true)
    {   
        //livre
    }
}
