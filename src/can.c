#include "can.h"
#include <globals.h>

// Aceitando todos os CAN
const struct can_filter my_filter = {
    .flags = 0U, .id = 0x000, .mask = CAN_STD_ID_MASK};
int filter_id;

int Init_CAN() {
  int ret;
  LOG_INF("Inicializando");

  ret = can_set_mode(can_dev, CAN_MODE_NORMAL);
  if (ret != 0) {
    printf("Error setting CAN mode [%d]", ret);
    return ret;
  }

  ret = can_set_bitrate(can_dev, 125000);
  if (ret != 0) {
    LOG_ERR("Failed to set timing (%d)", ret);
  }

  ret = can_start(can_dev);
  if (ret != 0) {
    printf("Error starting CAN controller [%d]", ret);
    return ret;
  }
  filter_id =
      can_add_rx_filter(can_dev, rx_callback_function, NULL, &my_filter);
  if (filter_id < 0) {
    LOG_ERR("Unable to add rx filter [%d]", filter_id);
  }

  return 0;
}