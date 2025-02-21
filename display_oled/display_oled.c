#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12

#define BTN_A_PIN 5

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

int A_state = 0;    //Botao A está pressionado?

void SinalAberto(){
    gpio_put(LED_R_PIN, 0);
    gpio_put(LED_G_PIN, 1);
    gpio_put(LED_B_PIN, 0);

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

restart:

    char *aberto[] = {
        "SINAL ABERTO",
        "ATRAVESSAR COM",
        "CUIDADO"};

    int y = 0;
    for (uint i = 0; i < count_of(aberto); i++)
    {
        ssd1306_draw_string(ssd, 5, y, aberto[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
}

void SinalAtencao(){
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 1);
    gpio_put(LED_B_PIN, 0);

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

restart:

    char *atencao[] = {
        "SINAL DE",
        "ATENCAO",
        "PREPARE-SE"};

    int y = 0;
    for (uint i = 0; i < count_of(atencao); i++)
    {
        ssd1306_draw_string(ssd, 5, y, atencao[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
}

void SinalFechado(){
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 0);
    gpio_put(LED_B_PIN, 0);

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

restart:

    char *fechado[] = {
        "SINAL FECHADO",
        "AGUARDE"};

    int y = 0;
    for (uint i = 0; i < count_of(fechado); i++)
    {
        ssd1306_draw_string(ssd, 5, y, fechado[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
}

int WaitWithRead(int timeMS){
    for(int i = 0; i < timeMS; i = i+100){
        A_state = !gpio_get(BTN_A_PIN);
        if(A_state == 1){
            return 1;
        }
        sleep_ms(100);
    }
    return 0;
}

int main(){
    stdio_init_all();   // Inicializa os tipos stdio padrão presentes ligados ao binário
    
    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // INICIANDO LEDS
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    // INICIANDO BOTÄO
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    while(true){
        SinalFechado();
        A_state = WaitWithRead(8000);//espera com leitura do botäo
        //sleep_ms(8000);

        if(A_state){
            //ALGUEM APERTOU O BOTAO 
            //SINAL AMARELO PARA OS CARROS POR 5s
            SinalAtencao();
            sleep_ms(5000);

            //SINAL VERDE PARA OS PEDESTRES POR 10s
            SinalAberto();
            sleep_ms(10000);

            //SINAL DE ATENÇÃO PARA SINALIZAR O FIM DO SINAL 

        }else{                          //NINGUEM APERTOU O BOTAO - CONTINUA NO SEMAFORO NORMAL
                                      
            SinalAtencao();
            sleep_ms(2000);

            //SINAL VERDE PARA OS PEDESTRES POR 8s
            SinalAberto();
            sleep_ms(8000);
        }      
    }
    return 0;
}