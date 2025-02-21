#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

int main()
{
    stdio_init_all();

    const uint LED_PIN = 12;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    uint32_t interval=1000; //intervalo de tempo para o temporizador (1000 ms)

    //calcula o proximo tempo absoluto que a acao vai ocorrer
    //absolute_time_t() retorna o tempo absoluto do sistema
    //delayed_by_us() calcula o tempo futuro adicionando um tempo em microssegundos ao tempo atual
    absolute_time_t next_wake_time =  delayed_by_us(get_absolute_time(), interval * 1000);

    bool led_on = false; //define o led como false (desligado)

    while (true)
    {
        //verifica se o tempo atual atingiu ou utrapassou o tempo definido em next_wake_time
        //se sim a funcao time_reached() retorna True
        if (time_reached(next_wake_time)) 
        {
            led_on = !led_on; //altera o estado do led entre ligado e desligado e o armazena

            gpio_put(LED_PIN, led_on); //define o estado do LED com base em led_on

            //atualiza next_wake_time adicionando um intervalo de 1 segundo ao tempo atual
            //define o tempo de espera para a proxima mensagem. 
            next_wake_time = delayed_by_us(next_wake_time, interval * 1000);
        }

        //Faz o programa “dormir” por 1 milissegundo. Essa pausa é utilizada 
        //para reduzir o uso da CPU, evitando que o microcontrolador
        //execute o loop incessantemente sem pausa. Isso ajuda a economizar
        //energia e a reduzir o desgaste da CPU.
        sleep_ms(1);
    }

    return 0;
}
