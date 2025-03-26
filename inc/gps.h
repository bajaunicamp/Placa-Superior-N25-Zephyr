#ifndef GPS_H
#define GPS_H

#include <zephyr/logging/log.h>

enum TYPE {
    GPDTM,
    GPGBS,
    GPGGA,
    GPGLL,
    GPGRS,
    GPGSA,
    GPGST,
    GPGSV,
    GPRMC,
    GPVTG,
    GPZDA,
};

/**
 * @struct s_gps 
 * @brief Representa o GPS
 */
extern struct s_gps {
  /** Referência ao device tree que representa */
  const struct device *dev;
  /** Buffer onde serão registradas as mensagens do callback */
  char *buffer;
  /** Número de caracteres do buffer */
  int buff_len;
  /** Booleano que registra se o GPS está pronto para receber um novo dado */
  bool ready;

  char *latitude;          // Latitude não traduzida, leia a documentação
  char *longitude;         // Longitude não traduzida, leia a documentação
  char information_status; // A: status OK
                           // V: status NOT ok

} gps;

/** 
 * @brief lê uma linha do buffer do GPS
 */
static void get_location();


/**
 * @brief 
 * 
 * @param buf ponteiro para a mensagem que será enviada
 * @param len tamanho da mensagem que será enviada
 */
static void send_uart_gps(unsigned char* buf, int len);

/**
 * @brief desabilita todas as linhas do GPS, uma de cada vez
 * 
 * @param mod representa qual linha será desativada
 */
static void disable_type(int mod);

/**
 * @brief habilita a linha de um determinado tipo
 * 
 * @param tp tipo que será habilitado
 */
static void enable_type(enum TYPE tp);

/**
 * @brief Inicializa o GPS
 * 
 * @return int que representa algum erro no callback
 */
int init_gps();

/**
 * @brief executa o get_location, dessa forma o get_location não
 * segura o restante do código
 */
void gps_tick();

#endif // GPS_H
