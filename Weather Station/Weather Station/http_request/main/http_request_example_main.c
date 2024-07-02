/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include <math.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "169.233.231.69"
#define WEB_PORT "1234"
#define WEB_PATH "/weather"

#include "driver/i2c.h"

#define SDA_PIN GPIO_NUM_10
#define SCL_PIN GPIO_NUM_8
#define SHTC3_I2C_ADDRESS 0x70

void initialize_i2c() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

void write_command(uint8_t command) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (SHTC3_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
 //   i2c_master_write_byte(cmd_handle, command, true);
    i2c_master_write_byte(cmd_handle, 0x78, true);
    i2c_master_write_byte(cmd_handle, 0x66, true);

    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);

    vTaskDelay(pdMS_TO_TICKS(20));
}
void read_sensor_data(uint8_t command, uint8_t* data) {
    // Write command to initiate measurement
    write_command(command);

    // Read 6 bytes of data
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (SHTC3_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    for (int i = 0; i < 5; i++) {
        i2c_master_read_byte(cmd_handle, &data[i], I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd_handle, &data[5], I2C_MASTER_NACK);
    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
}


static const char *TAG = "example";

static const char *REQUEST = "POST " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 1\r\n"
    "\r\n"
    "a";


static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];
    char send_buf[500];
    while(1) {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

       
       
	uint8_t data[6];

        // Read temperature data
        read_sensor_data(0x66, data);
        float temperature_raw = (data[0] << 8) | data[1];
        float temperature = 175.0 * (temperature_raw / 65535.0) - 45.0;

        // Read humidity data
        read_sensor_data(0x5E, data);
        float humidity_raw = (data[3] << 8) | data[4];
        float humidity = 100.0 * (humidity_raw / 65535.0);

        // Round the values to the nearest whole number
        temperature = round(temperature);
        humidity = round(humidity);
	char postData[200] = "temperature=%.0f&humidity=%.0f"; 
	snprintf(postData, sizeof(postData), "temperature=%.0f&humidity=%.0f", temperature, humidity);
	snprintf(send_buf, sizeof(send_buf), "POST " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: %d\r\n"
    "\r\n"
    "%s",strlen(postData), postData); 




	if (write(s, send_buf, strlen(send_buf)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

void app_main(void)
{
 initialize_i2c();

 	ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
