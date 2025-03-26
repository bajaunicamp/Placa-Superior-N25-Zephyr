#include "zephyr/device.h"
#include "zephyr/sys/printk.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "can.h"
#include "SIM800L.h"
#include "gps.h"

LOG_MODULE_REGISTER();

// esse código controla apenas o SIM800L e o GPS

int port;
enum OPERADORA op;

int main(){
  LOG_INF("Inicializando placa superior...");
  
  if(Init_CAN()) LOG_ERR("Não foi possível inicializar o CAN");

  if(init_gps()) LOG_ERR("Não foi possível inicializar o GPS");

  if(init_server(port, op)) LOG_ERR("Não foi possível se conectar ao servidor");

  

}
