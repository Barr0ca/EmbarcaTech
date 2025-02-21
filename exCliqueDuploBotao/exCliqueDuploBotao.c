#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

const uint PINO_LED = 13;
const uint PINO_BOTAO = 5;

//variaveis globais volateis para sincronixar interrupcoes 
volatile int contagem_click_botao = 0; //contador de clicks no botao
volatile bool led_estatus = false; //guarda estado atual do LED
volatile bool led_ativo = false; //indica se o LED esta ativo

//funcao callback desliga o LED apos 4 segundos
//temporizado chama essa funcao apos o termino do alarme
int64_t turn_off_callback(alarm_id_t id, void *user_data)
{
    gpio_put(PINO_LED, false); //desliga LED 
    led_ativo = false; //atualiza o estado do LED
    printf("LED esta desligado\n");
    return 0; //retorna 0 para nao repetir o alarme
}

//funcao de callback do temporizador que repete a cada 100ms
//funcao verifica o estado do botao e gerencia o estado do LED
bool repeating_timer_callback(struct repeating_timer *t)
{
    static absolute_time_t ultimo_click = 0; //armazena o tempo do ultimo click feito no botao
    static bool ultimo_estado_botao = false; //armazena o estado anterior do botao (pressionado ou nao)

    //verifica o estado atual do botao (pressionado = LOW, liberado = HIGH)
    bool botao_pressionado = !gpio_get(PINO_BOTAO); //botao pressionado gera nivel logico baixo (0)

    //verifica se o botao foi pressionado e realiza o debounce
    if (botao_pressionado && !ultimo_estado_botao && 
        absolute_time_diff_us(ultimo_click, get_absolute_time()) > 200000) { //verifica se 200ms se passaram (debounce)
        ultimo_click = get_absolute_time(); //atualiza o tempo do ultimo click
        ultimo_estado_botao = true; //atualiza o estado do botao como pressionado
        contagem_click_botao++; //incrementa um click

        //verifica se o botao foi pressionado 2 vezes
        if (contagem_click_botao == 2) {
            gpio_put(PINO_LED, true); //liga o LED
            led_ativo = true; //ativa o LED
            printf("LED ligado\n");

            //inicia um alarme para desligar o LED em 4 segundos (4000ms)
            add_alarm_in_ms(4000, turn_off_callback, NULL, false);
            contagem_click_botao == 0; //reseta o contador de clicks do botao
        }
    } else if (!botao_pressionado){
        ultimo_estado_botao = false; //atualiza o estado do botao para liberado quando nao ha clicks nele
    }

    return true; //retorna true para continuar o temporizador de repeticao
}

int main()
{
    stdio_init_all();

    // Configura o pino do LED como saída.
    gpio_init(PINO_LED);
    gpio_set_dir(PINO_LED, GPIO_OUT);
    gpio_put(PINO_LED, 0); // Garante que o LED comece apagado.

    // Configura o pino do botão como entrada com resistor de pull-up interno.
    gpio_init(PINO_BOTAO);
    gpio_set_dir(PINO_BOTAO, GPIO_IN);
    gpio_pull_up(PINO_BOTAO); // Habilita o resistor pull-up interno para evitar leituras incorretas.

    //configura o temporizador repetitivo para verificar o estado do botao a cada 100ms
    struct repeating_timer timer;
    add_repeating_timer_ms(100, repeating_timer_callback, NULL, &timer);

    //loop vazio, pois controle do LED e feito pelo temporizador
    while (true)
    {
        tight_loop_contents(); //otimiza o loop vazio. evita consumo excessivo de CPU
    }
    
}
