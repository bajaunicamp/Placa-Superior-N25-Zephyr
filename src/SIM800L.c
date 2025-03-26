#include "SIM800L.h"
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE();

struct s_server server; //UART3

static void server_callback(const struct device *dev, struct uart_event *evt, void *user_data);

void uart_poll_out_str(const struct device *dev, unsigned char *out_str, unsigned int len){
    for(int i = 0; i < len; i++){
        uart_poll_out(dev, out_str[i]);
    }
}

int init_server(unsigned int port, enum OPERADORA op){
    // Inicializando a comunicação UART
    server.dev = DEVICE_DT_GET(DT_ALIAS(sim800l));
    server.buffer_size = 50;
    server.buffer = calloc(50, sizeof(char));
    memset(server.buffer, '.', 50);
    server.connected = false;

    {
    LOG_INF("setando callback do sim800L");
    int ret = uart_callback_set(server.dev, server_callback, NULL);
    if(ret)
    {
        LOG_ERR("erro ao setar callback");
        printk("%d", ret);
        return ret;
    } 
    LOG_INF("Habilitando rx sim800L");
    uart_rx_enable(server.dev, server.buffer, server.buffer_size, 1000);
    }

    char temp[100];

    // TODO: substituir o delay por uma verificação de conexão
    LOG_INF("[SIM800L]: teste AT");
    sprintf(temp, "AT\r\n");
    uart_poll_out_str(server.dev, temp, strlen(temp));

    k_msleep(10000);

    // Select TCIP Application Mode
    LOG_INF("Select TCIP Application Mode");
    sprintf(temp, "AT+CIPMODE=0\r\n");
    uart_poll_out_str(server.dev, temp, strlen(temp));
    k_msleep(1000);

    // Startup single IP connection
    LOG_INF("Startup single IP connection");
	sprintf(temp, "AT+CIPMUX=0\r\n");
    uart_poll_out_str(server.dev, temp, strlen(temp));
    k_msleep(1000);

	// Attach from GPRS Service
    LOG_INF("Attach from GPRS Service");
	sprintf(temp, "AT+CGATT=1\r\n");
    uart_poll_out_str(server.dev, temp, strlen(temp));
    k_msleep(1000);

	// Start task and set APN
    LOG_INF("Start task and set APN");
    switch(op){
        case VIVO:
            sprintf(temp, "AT+CSTT=\"zap.vivo.com.br\",\"vivo\",\"vivo\"\r\n");
            break;
        case TIM:
            sprintf(temp, "AT+CSTT=\"timbrasil.br\",\"tim\",\"tim\"\r\n");
            break;
        case CLARO:
            sprintf(temp, "AT+CSTT=\"java.claro.com.br\",\"Claro\",\"Claro\"\r\n");
            break;
        case OUTRO:
            sprintf(temp, "AT+CSTT=\"[arg1]\",\"[arg2]\",\"[arg3]\"\r\n");
            break;
    }
    uart_poll_out_str(server.dev, temp, strlen(temp));
    k_msleep(3000);

	// Bring up wireless connection with GPRS  or CSD
    LOG_INF("Bring up wireless connection with GPRS  or CSD");
	sprintf(temp, "AT+CIICR\r\n");
    uart_poll_out_str(server.dev, temp, strlen(temp));
    k_msleep(3000);

	// Get Local IP address
    LOG_INF("Get Local IP address");
	sprintf(temp, "AT+CIFSR\r\n");
    uart_poll_out_str(server.dev, temp, strlen(temp));
    k_msleep(1000);

	// Start up TCP connection
    LOG_INF("Start up TCP connection");
	sprintf(temp, "AT+CIPSTART=\"TCP\",\"0.tcp.sa.ngrok.io\",\"%05d\"\r\n", port);
    uart_poll_out_str(server.dev, temp, strlen(temp));
    server.connected = true;
    k_msleep(5000);


	// Enviar mensagem pro servidor que é o baja que tá conectando
    LOG_INF("Enviar mensagem pro servidor que é o baja que tá conectando");
    send_server("[CONNECTION] Baja");

    return 0;
}

void send_server(char *message){
    char temp[100];

    sprintf(temp, "AT+CIPSEND=%d\r\n", strlen(message));
    uart_poll_out_str(server.dev, temp, strlen(temp));
    k_msleep(1000);

    sprintf(temp, "%s\r\n", message);
    uart_poll_out_str(server.dev, temp, strlen(temp));
    k_msleep(1000);
}

void server_callback(const struct device *dev, struct uart_event *evt, void *user_data){
    switch(evt->type){
        case UART_RX_RDY:
            char msg[100];
            //sscanf(evt->data.rx.buf, " %[^\r]", msg);
            printk("Mensagem recebida: [%.50s]\n", evt->data.rx.buf);
            uart_rx_disable(dev);
            break;

        case UART_RX_DISABLED:
            //printk("disabled");
            memset(server.buffer, '.', 50);
            uart_rx_enable(server.dev, server.buffer, server.buffer_size, 1000);
            break;
    }
}



