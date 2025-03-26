#include "gps.h"
#include "stdlib.h"
#include "zephyr/drivers/uart.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include <stdio.h>
#include <string.h>

LOG_MODULE_DECLARE();

struct s_gps gps;

static void get_location() {
  char *buf = malloc(gps.buff_len * sizeof(char));

  // Obter somente uma linha do buffer
  sscanf(gps.buffer, "%[^\n]s", buf);

  // Procuramos o início do comando GPGLL
  while (strncmp(buf, "$GPGLL", 6)) {
    // Se não tivermos encontrado o início, incrementamos o ponteiro
    buf = buf + 1;
    if (strlen(buf) < 6) {
      LOG_ERR("Não foi encontrado o comando $GPGLL");
      return;
    }
  }
  // Formato das mensagens do GPGLL:
  // $GPGLL,2249.36930,S,04703.87635,W,220458.00,A,A*62
  //  ID     Latitude     Longitude      UTC     Status
  //  Status: A=Valid Data
  //          V=Invalid Data
  char *lat_str = calloc(15, sizeof(char));
  char *long_str = calloc(15, sizeof(char));
  char status;
  int contador_de_virgulas = 0;

  for (int i = 0; i < strlen(buf); i++) {
    if (buf[i] == ',') {
      contador_de_virgulas++;
    }
    switch (contador_de_virgulas) {
    // Latitude
    case 1:
      // Esses comnandos são um pouco confusos, mas basicamente quando
      // encontramos a primeira vírgula, obtemos a string da latitude
      // e incrementamos o i para continuar a partir de quando a latitude
      // acaba
      sscanf(buf + i + 1, "%[^,]s", lat_str);
      i += strlen(lat_str);
      break;
    // Longitude
    case 3:
      // Mesma coisa da Latitude
      sscanf(buf + i + 1, "%[^,]s", long_str);
      i += strlen(long_str);
      break;
    case 6:
      status = buf[i];
      break;
    }
  }

  free(gps.latitude);
  gps.latitude = lat_str;

  free(gps.longitude);
  gps.longitude = long_str;

  gps.information_status = status;

  LOG_INF("Latitude: %s | Longitude: %s | Status: %c", lat_str, long_str,
          status);
}

static void send_uart_gps(unsigned char *buf, int len) {
  for (int i = 0; i < len; i++) {
    uart_poll_out(gps.dev, buf[i]);
  }
}

static void
disable_type(int mod) {     // a função está desse jeito pra não estourar a RAM,
                            // se tentar fazer tudo de uma vez vai dar ruim
  volatile uint8_t *dis;

  if (mod == 0) {
    volatile uint8_t dis_GPGLL[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x47, 0x4c, 0x4c, 0x2a, 0x32, 0x31, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x01, 0x00, 0xfb, 0x11, 0x03};
    dis = dis_GPGLL;
    LOG_INF("Desativando GPGLL");
  } if (mod == 1) {
    volatile uint8_t dis_GPDTM[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x44, 0x54, 0x4d, 0x2a, 0x33, 0x42, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x0a, 0x00, 0x04, 0x23, 0x03};
    dis = dis_GPDTM;
    LOG_INF("Desativando GPDTM");
  } else if (mod == 2) {
    volatile uint8_t dis_GPGBS[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x47, 0x42, 0x53, 0x2a, 0x33, 0x30, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x09, 0x00, 0x03, 0x21, 0x03};
    dis = dis_GPGBS;
    LOG_INF("Desativando GPGBS");
  } else if (mod == 3) {
    volatile uint8_t dis_GPGGA[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x47, 0x47, 0x41, 0x2a, 0x32, 0x37, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x00, 0x00, 0xfa, 0x0f, 0x03};
    dis = dis_GPGGA;
    LOG_INF("Desativando GPGGA");
  } else if (mod == 4) {
    volatile uint8_t dis_GPGRS[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x47, 0x52, 0x53, 0x2a, 0x32, 0x30, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x06, 0x00, 0x00, 0x1b, 0x03};
    dis = dis_GPGRS;
    LOG_INF("Desativando GPGRS");
  } else if (mod == 5) {
    volatile uint8_t dis_GPGSA[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x47, 0x53, 0x41, 0x2a, 0x33, 0x33, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x02, 0x00, 0xfc, 0x13, 0x03};
    dis = dis_GPGSA;
    LOG_INF("Desativando GPGSA");
  } else if (mod == 6) {
    volatile uint8_t dis_GPGST[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x47, 0x53, 0x54, 0x2a, 0x32, 0x36, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x07, 0x00, 0x01, 0x1d, 0x03};
    dis = dis_GPGST;
    LOG_INF("Desativando GPGST");
  } else if (mod == 7) {
    volatile uint8_t dis_GPGSV[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x47, 0x53, 0x56, 0x2a, 0x32, 0x34, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x03, 0x00, 0xfd, 0x15, 0x03};
    dis = dis_GPGSV;
    LOG_INF("Desativando GPGSV");
  } else if (mod == 8) {
    volatile uint8_t dis_GPRMC[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x52, 0x4d, 0x43, 0x2a, 0x33, 0x41, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x04, 0x00, 0xfe, 0x17, 0x03};
    dis = dis_GPRMC;
    LOG_INF("Desativando GPRMC");
  } else if (mod == 9) {
    volatile uint8_t dis_GPVTG[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x56, 0x54, 0x47, 0x2a, 0x32, 0x33, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x05, 0x00, 0xff, 0x19, 0x03};
    dis = dis_GPVTG;
    LOG_INF("Desativando GPVTG");
  } else if (mod == 10) {
    volatile uint8_t dis_GPZDA[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                    0x5a, 0x44, 0x41, 0x2a, 0x33, 0x39, 0x0d,
                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                    0xf0, 0x08, 0x00, 0x02, 0x1f, 0x03};
    dis = dis_GPZDA;
    LOG_INF("Desativando GPZDA");
  }

  send_uart_gps((unsigned char *)dis, 27);
}

static void
enable_type(enum TYPE tp) {
    // A função está desse jeito pra não estourar a RAM,
    // se tentar fazer tudo de uma vez vai dar ruim
    volatile uint8_t *enbl = NULL;

    switch (tp) {
        case GPGLL: {
            static volatile uint8_t enbl_GPGLL[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x47, 0x4c, 0x4c, 0x2a, 0x32, 0x31, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x01, 0x01, 0xfc, 0x12}; // Enable GPGLL
            enbl = enbl_GPGLL;
            LOG_INF("Ativando GPGLL");
            break;
        }
        case GPDTM: {
            static volatile uint8_t enbl_GPDTM[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x44, 0x54, 0x4d, 0x2a, 0x33, 0x42, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x0a, 0x01, 0x05, 0x24}; // Enable GPDTM
            enbl = enbl_GPDTM;
            LOG_INF("Ativando GPDTM");
            break;
        }
        case GPGBS: {
            static volatile uint8_t enbl_GPGBS[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x47, 0x42, 0x53, 0x2a, 0x33, 0x30, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x09, 0x01, 0x04, 0x22}; // Enable GPGBS
            enbl = enbl_GPGBS;
            LOG_INF("Ativando GPGBS");
            break;
        }
        case GPGGA: {
            static volatile uint8_t enbl_GPGGA[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x47, 0x47, 0x41, 0x2a, 0x32, 0x37, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x00, 0x01, 0xfb, 0x10}; // Enable GPGGA
            enbl = enbl_GPGGA;
            LOG_INF("Ativando GPGGA");
            break;
        }
        case GPGRS: {
            static volatile uint8_t enbl_GPGRS[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x47, 0x52, 0x53, 0x2a, 0x32, 0x30, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x06, 0x01, 0x01, 0x1c}; // Enable GPGRS
            enbl = enbl_GPGRS;
            LOG_INF("Ativando GPGRS");
            break;
        }
        case GPGSA: {
            static volatile uint8_t enbl_GPGSA[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x47, 0x53, 0x41, 0x2a, 0x33, 0x33, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x02, 0x01, 0xfd, 0x14}; // Enable GPGSA
            enbl = enbl_GPGSA;
            LOG_INF("Ativando GPGSA");
            break;
        }
        case GPGST: {
            static volatile uint8_t enbl_GPGST[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x47, 0x53, 0x54, 0x2a, 0x32, 0x36, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x07, 0x01, 0x02, 0x1e}; // Enable GPGST
            enbl = enbl_GPGST;
            LOG_INF("Ativando GPGST");
            break;
        }
        case GPGSV: {
            static volatile uint8_t enbl_GPGSV[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x47, 0x53, 0x56, 0x2a, 0x32, 0x34, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x03, 0x01, 0xfe, 0x16}; // Enable GPGSV
            enbl = enbl_GPGSV;
            LOG_INF("Ativando GPGSV");
            break;
        }
        case GPRMC: {
            static volatile uint8_t enbl_GPRMC[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x52, 0x4d, 0x43, 0x2a, 0x33, 0x41, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x04, 0x01, 0xff, 0x18}; // Enable GPRMC
            enbl = enbl_GPRMC;
            LOG_INF("Ativando GPRMC");
            break;
        }
        case GPVTG: {
            static volatile uint8_t enbl_GPVTG[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x56, 0x54, 0x47, 0x2a, 0x32, 0x33, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x05, 0x01, 0x00, 0x1a}; // Enable GPVTG
            enbl = enbl_GPVTG;
            LOG_INF("Ativando GPVTG");
            break;
        }
        case GPZDA: {
            static volatile uint8_t enbl_GPZDA[] = {0x24, 0x45, 0x49, 0x47, 0x50, 0x51, 0x2c,
                                                    0x5a, 0x44, 0x41, 0x2a, 0x33, 0x39, 0x0d,
                                                    0x0a, 0xb5, 0x62, 0x06, 0x01, 0x03, 0x00,
                                                    0xf0, 0x08, 0x01, 0x03, 0x20}; // Enable GPZDA
            enbl = enbl_GPZDA;
            LOG_INF("Ativando GPZDA");
            break;
        }
        
        default:
            LOG_ERR("Módulo inválido");
            break;
    }

    // Chame a função para enviar o comando de ativação aqui
    if (enbl != NULL) {
        send_uart_gps((unsigned char *)enbl, 27);
    }
}

static void gps_callback(const struct device *dev, struct uart_event *evt,
                         void *user_data) {

  switch (evt->type) {
  case UART_RX_RDY:
    gps.ready = true;
    uart_rx_disable(gps.dev); // Não precisamos receber novas informações até
                              // terminarmos de lidar com as que já temos
    break;
  }
}

int init_gps() {
  // A documentação do GPS pede que esperemos 300ms para inicializar,
  // mas vamos esperar 1seg pra garantir que ele estará inicializado
  k_msleep(1000);
  LOG_INF("Inicializando GPS teste");
  gps.dev = DEVICE_DT_GET(DT_ALIAS(gps));
  gps.buff_len = 60;
  gps.buffer = malloc(gps.buff_len * sizeof(char));
  gps.ready = false;
  // desabilitando todas as linhas recebidas do GPS
  for (int i = 0; i < 11; i++) {
    disable_type(i);
    k_msleep(100);
  }
  // habilitando a linha desejada do gps
  enable_type(GPGLL);

  int ret = uart_rx_enable(gps.dev, gps.buffer, gps.buff_len, 1000);
  if (ret) {
    LOG_ERR("ERRO UART RX Enable GPS: %d", ret);
    return ret;
  } else {
    LOG_INF("UART Callback definido");
  }

  ret = uart_callback_set(gps.dev, gps_callback, NULL);
  if (ret) {
    LOG_ERR("ERRO UART RX Callback GPS: %d", ret);
    return ret;
  }
}

// Essa função deve ser executada no while(true) da main
void gps_tick() {
  if (gps.ready) {
    get_location();
    gps.ready = false;
    memset(gps.buffer, 0, gps.buff_len);
    uart_rx_enable(gps.dev, gps.buffer, gps.buff_len, 1000);
  }
}
