#include <math.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "inc/ssd1306.h"

// Pinos e módulos controlador i2c selecionado
#define I2C_PORT i2c1
#define PINO_SCL 14
#define PINO_SDA 15

// Configuração do pino do buzzer
#define BUZZER_PIN 21

// Flag global para abortar o modo buzzer
volatile bool buzzer_abort = false;

// Notas musicais para a música tema de Star Wars
const uint star_wars_notes[] = {
    330, 330, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 523, 494, 440, 392, 330,
    659, 784, 659, 523, 494, 440, 392, 330,
    659, 659, 330, 784, 880, 698, 784, 659,
    523, 494, 440, 392, 659, 784, 659, 523,
    494, 440, 392, 330, 659, 523, 659, 262,
    330, 294, 247, 262, 220, 262, 330, 262,
    330, 294, 247, 262, 330, 392, 523, 440,
    349, 330, 659, 784, 659, 523, 494, 440,
    392, 659, 784, 659, 523, 494, 440, 392
};

// Duração das notas em milissegundos
const uint note_duration[] = {
    500, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 650, 500, 150, 300, 500, 350,
    150, 300, 500, 150, 300, 500, 350, 150,
    300, 650, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 350, 150, 300, 500, 500,
    350, 150, 300, 500, 500, 350, 150, 300,
};


// Botão do Joystick
const int SW = 22;  

// Variável para armazenar a posição do seletor do display
uint pos_y = 12;

ssd1306_t disp;

const uint LED = 12;            // Pino do LED RGB (Azul)
const uint16_t PERIODO = 2000;  // Periodo do PWM (valor maximo do contador)
const float DIVISOR_PWM = 16.0; // Divisor fracional do clock para o PWM
const uint16_t PASSO_LED = 100; // Passo de incremento/drecremento para o duty cycle do LED
uint16_t nivel_led = 100;       // Nivel inicial do PWM (duty cycle)

const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X do joystick
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y do joystick

const int LED_G = 13;                    // Pino para controle do LED verde via PWM
const int LED_R = 11;                    // Pino para controle do LED vermelho via PWM
const float DIVIDER_PWM = 16.0;          // Divisor fracional do clock para o PWM
const uint16_t PERIOD = 4096;            // Período do PWM (valor máximo do contador)
uint16_t led_g_level, led_r_level = 100; // Inicialização dos níveis de PWM para os LEDs
uint slice_led_g, slice_led_r;           // Variáveis para armazenar os slices de PWM correspondentes aos LEDs

// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); // 50% de duty cycle

    uint elapsed_time = 0;
    while (elapsed_time < duration_ms) {
        if (gpio_get(SW) == 0) { // Se botão for pressionado, interrompe
            buzzer_abort = true;
            break;
        }
        sleep_ms(10); // Pequeno atraso para evitar sobrecarga do loop
        elapsed_time += 10;
    }

    pwm_set_gpio_level(pin, 0); // Desliga o som após a duração
    sleep_ms(50);               // Pausa entre notas
}

// Função principal para tocar a música
void play_star_wars(uint pin) {
    buzzer_abort = false;
    for (int i = 0; i < sizeof(star_wars_notes) / sizeof(star_wars_notes[0]); i++) {
        if (gpio_get(SW) == 0) {
            buzzer_abort = true;
            break;
        }
        if (star_wars_notes[i] == 0) {
            for (int note = 0; note < note_duration[i]; note += 10) {
                if (gpio_get(SW) == 0) {
                    buzzer_abort = true;
                    break;
                }
                sleep_ms(50);
            }
        } else {
            play_tone(pin, star_wars_notes[i], note_duration[i]);
        }
        if (buzzer_abort) {
            break;
        }
    }
    pwm_set_gpio_level(pin, 0); // Garante que o buzzer seja desligado ao sair
}

void setup_pwm()
{
    uint slice;
    gpio_set_function(LED, GPIO_FUNC_PWM);  // Configurando o pino do LED como saida PWM
    slice = pwm_gpio_to_slice_num(LED);     // Pega o slice associado ao pino do PWM
    pwm_set_clkdiv(slice, DIVISOR_PWM);     // Define o divisor de clock para o PWM
    pwm_set_wrap(slice, PERIODO);           // Configura o valor maximo do contador (periodo do PWM)
    pwm_set_gpio_level(LED, nivel_led);     // Nivel inicial do PWM para o pino do LED
    pwm_set_enabled(slice, true);           // Habilita o PWM no slice correspondente
}

// AS FUNÇÕES SÃO BASICAMENTE IGUAIS, SENDO UMA PARA UM LED E OUTRA PARA DOIS LEDS

// Função para configurar o PWM de um LED (genérica para verde e vermelho)
void setup_pwm_led(uint led, uint *slice, uint16_t level)
{
  gpio_set_function(led, GPIO_FUNC_PWM); // Configura o pino do LED como saída PWM
  *slice = pwm_gpio_to_slice_num(led);   // Obtém o slice do PWM associado ao pino do LED
  pwm_set_clkdiv(*slice, DIVIDER_PWM);   // Define o divisor de clock do PWM
  pwm_set_wrap(*slice, PERIOD);          // Configura o valor máximo do contador (período do PWM)
  pwm_set_gpio_level(led, level);        // Define o nível inicial do PWM para o LED
  pwm_set_enabled(*slice, true);         // Habilita o PWM no slice correspondente ao LED
}

// Função de configuração geral
void setup()
{
  setup_pwm_led(LED_G, &slice_led_g, led_g_level); // Configura o PWM para o LED azul
  setup_pwm_led(LED_R, &slice_led_r, led_r_level); // Configura o PWM para o LED vermelho
}

// Função para ler os valores dos eixos do joystick (X e Y)
void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value)
{
  // Leitura do valor do eixo X do joystick
  adc_select_input(ADC_CHANNEL_0); // Seleciona o canal ADC para o eixo X
  sleep_us(2);                     // Pequeno delay para estabilidade
  *vrx_value = adc_read();         // Lê o valor do eixo X (0-4095)

  // Leitura do valor do eixo Y do joystick
  adc_select_input(ADC_CHANNEL_1); // Seleciona o canal ADC para o eixo Y
  sleep_us(2);                     // Pequeno delay para estabilidade
  *vry_value = adc_read();         // Lê o valor do eixo Y (0-4095)
}

// Função para inicialização de todos os recursos do sistema
void inicializa(){
    stdio_init_all();

    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);

    i2c_init(I2C_PORT, 400*1000);// I2C Inicialização. Usando 400Khz.
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SCL);
    gpio_pull_up(PINO_SDA);

    disp.external_vcc=false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);

    // Botão do Joystick
    gpio_init(SW);             // Inicializa o pino do botão
    gpio_set_dir(SW, GPIO_IN); // Configura o pino do botão como entrada
    gpio_pull_up(SW); 

    pwm_init_buzzer(BUZZER_PIN);
}

// Função escrita no display.
void print_texto(char *msg, uint pos_x, uint pos_y, uint scale){
    ssd1306_draw_string(&disp, pos_x, pos_y, scale, msg);// Desenha texto
    ssd1306_show(&disp);                                 // Apresenta no Oled
}

// O desenho do retangulo fará o papel de seletor
void print_retangulo(int x1, int y1, int x2, int y2){
    ssd1306_draw_empty_square(&disp,x1,y1,x2,y2);
    ssd1306_show(&disp);
}

// Função para o modo LED controlado pelo joystick
void menu_led_joystick(void) {
    ssd1306_clear(&disp);
    print_texto("LED JOYSTICK ATIVO", 6, 18, 1.5);
    print_texto("PARA SAIR CLIQUE", 6, 32, 1.5);
    print_texto("NO JOYSTICK", 6, 44, 1.5);

    uint16_t vrx_value, vry_value;
    setup(); // Configura os PWM dos LEDs

    while (true) {
        joystick_read_axis(&vrx_value, &vry_value);
        pwm_set_gpio_level(LED_G, vrx_value);
        pwm_set_gpio_level(LED_R, vry_value);
        sleep_ms(100);  // Pequeno delay para estabilidade

        // Se o botão do joystick for pressionado, sai deste modo
        if (gpio_get(SW) == 0) {
            // Aguarda a liberação para evitar debounce (opcional)
            while (gpio_get(SW) == 0) {
                sleep_ms(50);
            }
            // Desliga os LEDs antes de sair
            pwm_set_gpio_level(LED_G, 0);
            pwm_set_gpio_level(LED_R, 0);
            break;
        }
    }
    // Mensagem auxilixar após sair do laço
    ssd1306_clear(&disp);
    print_texto("MOVA O JOYSTICK", 6, 32, 1.5);
}

// Função para o modo Buzzer
void menu_buzzer(void) {
    ssd1306_clear(&disp);
    print_texto("BUZZER PWM ATIVO", 6, 18, 1.5);
    print_texto("PARA SAIR CLIQUE", 6, 32, 1.5);
    print_texto("NO JOYSTICK", 6, 44, 1.5);

    while (true) {
        sleep_ms(100);
        play_star_wars(BUZZER_PIN);
        if (buzzer_abort) {
            pwm_set_gpio_level(BUZZER_PIN, 0);
            break;
        }
        sleep_ms(100);
    }
    ssd1306_clear(&disp);
    print_texto("MOVA O JOYSTICK", 6, 32, 1.5);
}

// Função para o modo LED PWM
void menu_led_pwm(void) {
    ssd1306_clear(&disp);
    print_texto("LED PWM ATIVO", 6, 5, 1.5);
    print_texto("PARA SAIR CLIQUE ", 6, 20, 1.5);
    print_texto("(OU SEGURE)", 6, 35, 1.5);
    print_texto("NO JOYSTICK", 6, 50, 1.5);

    uint up_down = 1;
    setup_pwm(); // Configura o PWM do LED
    while (true)
    {   
        pwm_set_gpio_level(LED, nivel_led); // Nivel atual do PWM
        sleep_ms(1000);                     // Atraso de 1 segundo

        if (up_down) 
        {
            nivel_led += PASSO_LED; // Incrementa o nivel do LED
            if (gpio_get(SW) == 0) {
                pwm_set_gpio_level(LED, 0);
                sleep_ms(50);
                break;
            } else { 
                if (nivel_led >= PERIODO)
                    up_down = 0; // Muda direcao apos atingir o nivel maximo
            }
        } else {
            nivel_led -= PASSO_LED; // Decrementa o nivel do LED
            if (gpio_get(SW) == 0) {
                pwm_set_gpio_level(LED, 0);
                sleep_ms(50);
                break;
            } else {
                if (nivel_led <= PASSO_LED)
                    up_down = 1; // Muda de direcao apos atingir o nivel minimo
            }
        }
    }
    ssd1306_clear(&disp);
    print_texto("MOVA O JOYSTICK", 6, 32, 1.5);
}

int main()
{
    inicializa();
    char *text = "";    // Texto do menu
    uint countdown = 0; // Verificar seleções para baixo do joystick
    uint countup = 2;   // Verificar seleções para cima do joystick  
    uint adc_y_raw_old = 0; 
   
    while(true){
        adc_select_input(0);
        uint adc_y_raw = adc_read();
        const uint bar_width = 40;
        const uint adc_max = (1 << 12) - 1;
        uint bar_y_pos = adc_y_raw * bar_width / adc_max + 1; // Posição do joystick

        // Só atualiza se houver mudança significativa no joystick
        if (fabs((int)bar_y_pos - (int)adc_y_raw_old) > 1) { 

            // Limpa e redesenha o menu
            ssd1306_clear(&disp);
            print_texto(text="Menu", 52, 2, 1);
            print_retangulo(2, pos_y, 120, 18);
            print_texto(text="Joystick LED", 6, 18, 1.5);
            print_texto(text="Tocar Buzzer", 6, 30, 1.5);
            print_texto(text="LED PWM", 6, 42, 1.5);

            // Verifica movimento do joystick
            if (bar_y_pos < 20 && countdown < 2) {
                pos_y += 12;
                countdown += 1;
                countup -= 1;
            } else if (bar_y_pos > 20 && countup < 2) {
                pos_y -= 12;
                countup += 1;
                countdown -= 1;
            }

            adc_y_raw_old = bar_y_pos;
        }
        sleep_ms(250);

        // Verifica se botão foi pressionado. Se sim, entra no switch case 
        // para verificar posição do seletor e chama acionamento dos leds.
        if (gpio_get(SW) == 0) {
            switch (pos_y){
                case 12:
                    menu_led_joystick();
                    break;
                case 24:
                    menu_buzzer();
                    break;
                case 36:
                    menu_led_pwm();
                    break;
                default:
                    break;
            }
            sleep_ms(200);
       }
    }
    return 0;
}
