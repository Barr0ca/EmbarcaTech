#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

// Definição das credenciais da rede Wi-Fi e chave da API do ThingSpeak
#define WIFI_SSID "..." 
#define WIFI_PASS "..." 
#define CHANNEL_API_KEY "6ZIOUMDAMKMBM0HH"

// Definição dos pinos I2C para o display OLED
#define I2C_PORT i2c1
const uint PINO_SCL = 14;
const uint PINO_SDA = 15;
ssd1306_t disp;

// Definição dos pinos dos LEDs indicadores
const uint LED_R_PIN = 13;
const uint LED_G_PIN = 11;

// Variáveis de monitoramento do sistema
static uint qtdLitrosAgua = 100; 
static uint simulaChuva = 0;
static uint umidadeSolo = 3;

// Definição do pino e frequência do buzzer
const uint BUZZER_PIN = 21;
const uint BUZZER_FREQUENCY = 100;
static char thingspeak_response[512] = "Esperando resposta o servidor...";

// Callback para receber resposta do ThingSpeak
static err_t receive_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
  if (!p) {
    tcp_close(tpcb);
    return ERR_OK;
  }

  snprintf(thingspeak_response, sizeof(thingspeak_response), "Resposta do ThingSpeak: %.*s", p->len, (char *)p->payload);

  pbuf_free(p);
  tcp_close(tpcb);
  return ERR_OK;
}

// Função para enviar os dados do sistema ao ThingSpeak
static void enviar_para_thingspeak() {
  char post_data[256];
  snprintf(post_data, sizeof(post_data),
    "api_key=%s&field1=%d&field2=%d&field3=%d",
    CHANNEL_API_KEY, qtdLitrosAgua, simulaChuva, umidadeSolo);

  sleep_ms(700);
  struct tcp_pcb *pcb = tcp_new();
  if (!pcb) {
    printf("Erro ao criar PCB TCP\n");
    return;
  }

  ip_addr_t ip;
  IP4_ADDR(&ip, 184, 106, 153, 149);

  err_t err = tcp_connect(pcb, &ip, 80, NULL);
  if (err != ERR_OK) {
    printf("Erro ao conectar ao ThingSpeak\n");
    tcp_close(pcb);
    return;
  }

  char request[512];
  snprintf(request, sizeof(request),
    "POST /update.json HTTP/1.1\r\n"
    "Host: api.thingspeak.com\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: %d\r\n\r\n"
    "%s",
    (int)strlen(post_data), post_data);

  err_t send_err = tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
  if (send_err != ERR_OK) {
    printf("Erro ao enviar os dados para o ThingSpeak\n");
    tcp_close(pcb);
    return;
  } else{
    printf("Dados enviados para ThingSpeak\n");        
  }

  tcp_output(pcb);

  tcp_recv(pcb, receive_callback);
}

// Inicializa o buzzer com PWM
void pwm_init_buzzer(uint pin) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(pin);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
  pwm_init(slice_num, &config, true);
  pwm_set_gpio_level(pin, 0);
}

// Aciona o buzzer para emitir um som de alerta
void beep(uint pin, uint duration_ms) {
  uint slice_num = pwm_gpio_to_slice_num(pin);
  pwm_set_gpio_level(pin, 2048);
  sleep_ms(duration_ms);
  pwm_set_gpio_level(pin, 0);
  sleep_ms(100);
}

// Alerta visual e sonoro quando o nível de água está baixo
void alerta_nivel_baixo() {
  static absolute_time_t next_blink_time = 0;
  static bool led_blink = false;
  pwm_init_buzzer(BUZZER_PIN);
  beep(BUZZER_PIN, 500);
  if (time_reached(next_blink_time)){
    led_blink = !led_blink;
    gpio_put(LED_R_PIN, led_blink);
    gpio_put(LED_G_PIN, false);
    next_blink_time = make_timeout_time_ms(1000);
  }
}

// Função para exibir texto no display OLED
void print_texto(char *msg, uint pos_x, uint pos_y, uint scale){
  ssd1306_draw_string(&disp, pos_x, pos_y, scale, msg);
  ssd1306_show(&disp);                                 
}

int main() {
  stdio_init_all();

  // Inicialização dos LEDs
  gpio_init(LED_R_PIN);
  gpio_init(LED_G_PIN);
  gpio_set_dir(LED_R_PIN, GPIO_OUT);
  gpio_set_dir(LED_G_PIN, GPIO_OUT);

  // Inicialização do barramento I2C
  i2c_init(I2C_PORT, 400*1000);
  gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
  gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
  gpio_pull_up(PINO_SCL);
  gpio_pull_up(PINO_SDA);

  // Configuração do display OLED
  disp.external_vcc=false;
  ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);
  char *text = "";

  uint32_t interval = 15000; // Intervalo de tempo para o temporizador (15000 ms)
  absolute_time_t next_wake_time =  make_timeout_time_ms(interval);
  bool led_r_on = false;
  bool led_g_on = false;

  static uint32_t last_request_time = 0;
  
  // Inicialização do Wi-Fi
  if (cyw43_arch_init()) {
    printf("Erro ao inicializar o Wi-Fi\n");
    return 1;
  }

  cyw43_arch_enable_sta_mode();
  printf("Conectando ao Wi-Fi...\n");

  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
    printf("Falha ao conectar ao Wi-Fi\n");
    return 1;
  } else {
    printf("Conectado ao Wi-Fi!\n"); 
  }

  ssd1306_clear(&disp);   
  print_texto(text="INICIANDO EM", 6, 2, 1);
  print_texto(text="15 SEGUNDOS", 6, 14, 1);

  while (true) {    
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (qtdLitrosAgua < 25) {
      ssd1306_clear(&disp);
      print_texto(text="ALERTA", 6, 2, 2);
      print_texto(text="Nivel de agua baixo", 6, 20, 1);
      alerta_nivel_baixo();
      continue;
    }
    if (time_reached(next_wake_time)) {
      next_wake_time = make_timeout_time_ms(interval);
      int leitura_simulada = rand() % 4097; // Gera um leitura aleatoria simulando um ADC 0-4096
      simulaChuva = rand() % 101;

      printf("Leitura Simulada do Sensor: %d\nQuantidade de agua: %d\nProbabilidade de Chuva: %d\n", leitura_simulada, qtdLitrosAgua, simulaChuva);
      char buffer[32];

      if ((leitura_simulada <= 1500) && (simulaChuva <= 70)) {
        if (qtdLitrosAgua >= 25) {
          qtdLitrosAgua -= 25;
          umidadeSolo = 1;

          ssd1306_clear(&disp);
          print_texto(text="Umidade: Ruim", 6, 2, 1);
          sprintf(buffer, "Prob. de chuva: %d%%", simulaChuva);
          print_texto(buffer, 6, 14, 1);
          sprintf(buffer, "Qtd. Agua: %dL", qtdLitrosAgua);
          print_texto(buffer, 6, 26, 1);
          print_texto(text="Irrigando o solo", 6, 38, 1);

          led_r_on = true;
          led_g_on = false;
        }
      } else if ((leitura_simulada <= 3500) && (simulaChuva <= 50)) {
        if (qtdLitrosAgua >= 25) {
          qtdLitrosAgua -= 25;
          umidadeSolo = 2;

          ssd1306_clear(&disp);
          print_texto(text="Umidade: Media", 6, 2, 1);
          sprintf(buffer, "Prob. de chuva: %d%%", simulaChuva);
          print_texto(buffer, 6, 14, 1);
          sprintf(buffer, "Qtd. Agua: %dL", qtdLitrosAgua);
          print_texto(buffer, 6, 26, 1);
          print_texto(text="Irrigando o solo", 6, 38, 1);

          led_r_on = true;
          led_g_on = true;
        }
      } else if (simulaChuva >= 80) {
        ssd1306_clear(&disp);
        sprintf(buffer, "Prob. de chuva: %d%%", simulaChuva);
        print_texto(buffer, 6, 14, 1);
        sprintf(buffer, "Qtd. Agua: %dL", qtdLitrosAgua);
        print_texto(buffer, 6, 26, 1);
        print_texto(text="Provavel chuva: sem irrigar", 6, 38, 1);

        led_r_on = false;
        led_g_on = true;
      } else {
        umidadeSolo = 3;

        ssd1306_clear(&disp);
        print_texto(text="Umidade: Bom", 6, 2, 1);
        sprintf(buffer, "Prob. de chuva: %d%%", simulaChuva);
        print_texto(buffer, 6, 14, 1);
        sprintf(buffer, "Qtd. Agua: %dL", qtdLitrosAgua);
        print_texto(buffer, 6, 26, 1);
        print_texto(text="Solo esta irrigado", 6, 38, 1);

        led_r_on = false;
        led_g_on = true;
      }
      gpio_put(LED_R_PIN, led_r_on);
      gpio_put(LED_G_PIN, led_g_on);
    }
    if (current_time - last_request_time >= 15000) {
      enviar_para_thingspeak();
      last_request_time = current_time;
    }
  }
  cyw43_arch_deinit();
  return 0;
}
