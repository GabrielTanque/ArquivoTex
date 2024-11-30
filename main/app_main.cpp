/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_lvgl_port.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "i2c_oled.h"
#include "cbspI2C.h"
#include "cBMP280.h"
#include "cSMP3011.h"

static const char *TAG = "example";


/*
        HARDWARE DEFINITIONS
*/
#define I2C_BUS_PORT                  0
#define EXAMPLE_PIN_NUM_SDA           GPIO_NUM_5
#define EXAMPLE_PIN_NUM_SCL           GPIO_NUM_4
#define EXAMPLE_PIN_LED               GPIO_NUM_16 
#define BOTAO_PAGINAS                 GPIO_NUM_27             

/*
        Components
*/
cbspI2C I2CChannel1;
cBMP280 BMP280;
cSMP3011 SMP3011;


/*
        TASKS
*/
//Prototypes
void TaskBlink(void *parameter);
void TaskDisplay(void *parameter);
void TaskSensors(void *parameter);

//Handlers
TaskHandle_t taskBlinkHandle   = nullptr;
TaskHandle_t taskDisplayHandle = nullptr;
TaskHandle_t taskSensorsHandle = nullptr;


// Set de contador para mavegação das páginas

int contador_navegacao = 0;

//Main function
extern "C"
void app_main()
{

    //Setup pin for LED
    gpio_set_direction(EXAMPLE_PIN_LED, GPIO_MODE_OUTPUT);

    // Setup pins for buttons
    gpio_set_direction(BOTAO_PAGINAS, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOTAO_PAGINAS, GPIO_PULLUP_ONLY);

    //Setup I2C 0 for display
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = EXAMPLE_PIN_NUM_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags
        {
            .enable_internal_pullup = true,
        }
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    
    //Setup I2C 1 for sensors
    I2CChannel1.init(I2C_NUM_1, GPIO_NUM_33, GPIO_NUM_32);
    I2CChannel1.openAsMaster(100000);

    //Initialize sensors
    BMP280.init(I2CChannel1);
    SMP3011.init(I2CChannel1);

    //Initialize display
    i2c_oled_start(i2c_bus);
    
    //Create tasks
    xTaskCreate( TaskBlink  , "Blink"  , 1024,  nullptr,   tskIDLE_PRIORITY,  &taskBlinkHandle   );  
    xTaskCreate( TaskSensors, "Sensors", 2048,  nullptr,   tskIDLE_PRIORITY,  &taskSensorsHandle );    
    xTaskCreate( TaskDisplay, "Display", 4096,  nullptr,   tskIDLE_PRIORITY,  &taskDisplayHandle );
}

void TaskDisplay(void *parameter)
{
    // Inicialização da tela e criação da borda e linha
    lv_obj_t *scr = lv_disp_get_scr_act(nullptr);

    // Estilo da borda e criação do objeto de borda
    static lv_style_t style1;
    lv_style_init(&style1);
    lv_style_set_bg_color(&style1, lv_color_hex(0xFFFFFF));
    lv_style_set_border_color(&style1, lv_color_hex(0x000000));
    lv_style_set_border_width(&style1, 1);

    lv_obj_t *border = lv_obj_create(scr);
    lv_obj_set_size(border, 128, 64);
    lv_obj_add_style(border, &style1, 0);
    lv_obj_align(border, LV_ALIGN_CENTER, 0, 0);

    // Linha horizontal
    static lv_point_t line_points[] = {{0, 15}, {127, 15}};
    lv_obj_t *line = lv_line_create(scr);
    lv_line_set_points(line, line_points, 2);

    static lv_style_t line_style;
    lv_style_init(&line_style);
    lv_style_set_line_color(&line_style, lv_color_hex(0x000000));
    lv_style_set_line_width(&line_style, 1);
    lv_obj_add_style(line, &line_style, 0);
    lv_obj_align(line, LV_ALIGN_TOP_LEFT, 0, 0);

    // Estilo para o retângulo
    static lv_style_t retangulo;
    lv_style_init(&retangulo);
    lv_style_set_bg_color(&retangulo, lv_color_hex(0xFFFFFF));
    lv_style_set_border_color(&retangulo, lv_color_hex(0x000000));
    lv_style_set_border_width(&retangulo, 1);

    lv_obj_t *retangulo_bar = lv_obj_create(scr);
    lv_obj_set_size(retangulo_bar, 65, 18);
    lv_obj_add_style(retangulo_bar, &retangulo, 0);
    lv_obj_set_pos(retangulo_bar, 35, 20);

    lv_obj_t *retangulo_psi = lv_obj_create(scr);
    lv_obj_set_size(retangulo_psi, 55, 18);
    lv_obj_add_style(retangulo_psi, &retangulo, 0);
    lv_obj_set_pos(retangulo_psi, 45, 40);

    lv_obj_t *labelPressure = lv_label_create(scr);
    lv_label_set_long_mode(labelPressure, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(labelPressure, &lv_font_montserrat_14, 0);
    lv_label_set_text(labelPressure, "");
    lv_obj_set_pos(labelPressure, 34, 21);

    lv_obj_t *labelTemperature = lv_label_create(scr);
    lv_label_set_long_mode(labelTemperature, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(labelTemperature, &lv_font_montserrat_14, 0);
    lv_label_set_text(labelTemperature, "");
    lv_obj_set_pos(labelTemperature, 45, 41);

    // Criação do rótulo da linha 1
    static lv_obj_t *label_cabecalho = lv_label_create(scr);
    lv_obj_set_style_text_font(label_cabecalho, &lv_font_montserrat_14, 0);
    lv_label_set_text(label_cabecalho, "MEDICAO");
    lv_obj_align(label_cabecalho, LV_ALIGN_TOP_LEFT, 2, 1);

    // Criação do rótulo da linha 2
    lv_obj_t *label_pressao = lv_label_create(scr);
    lv_obj_set_style_text_font(label_pressao, &lv_font_montserrat_14, 0);
    lv_label_set_text(label_pressao, "");
    lv_obj_align(label_pressao, LV_ALIGN_TOP_LEFT, 2, 20);

    // Criação do rótulo da linha 3
    lv_obj_t *label_temp = lv_label_create(scr);
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_14, 0);
    lv_label_set_text(label_temp, "");
    lv_obj_align(label_temp, LV_ALIGN_TOP_LEFT, 2, 40);

    lv_obj_t* triangle = nullptr;
    lv_obj_t* linha = nullptr;
    lv_obj_t* ponto = nullptr;

    // Loop principal de exibição
    while (1)
    {
        // Atualiza a exibição com base no valor da pressão
        double pressure = SMP3011.getPressurePsi();
        double pressure_bar = SMP3011.getPressureBar();

        // Controle de navegação com debounce
        if (gpio_get_level(BOTAO_PAGINAS) == 0)
        {
            contador_navegacao++;
            while (gpio_get_level(BOTAO_PAGINAS) == 0)
            {
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }

        // Demosntra o valor do contador no terminal
        ESP_LOGI(TAG, "Contador de navegação: %d", contador_navegacao);

        // Função para zerar o contador
        if (contador_navegacao > 1)
        {
            contador_navegacao = 0;
        }

        // Se a pressão for menor que 28 e maior do que 1 ou maior que 34 PSI, desenha o triângulo e uma exclação em seu centro
        if (pressure >= 1 && pressure < 28 || pressure > 34)
        {
            if (triangle == nullptr) {
                lv_point_t triangle_points[] = {
                    {115, 20}, {125, 40},
                    {115, 20}, {105, 40},
                    {105, 40}, {125, 40}
                };
                lv_point_t linha_points[] = {
                    {115, 25}, {115, 35}
                };
                lv_point_t ponto_points[] = {
                    {115, 37}, {115, 38}
                };

                triangle = lv_line_create(scr);
                lv_line_set_points(triangle, triangle_points, 6);

                linha = lv_line_create(scr);
                lv_line_set_points(linha, linha_points, 2);

                ponto = lv_line_create(scr);
                lv_line_set_points(ponto, ponto_points, 2);

                static lv_style_t triangle_style;
                lv_style_init(&triangle_style);
                lv_style_set_line_color(&triangle_style, lv_color_hex(0x000000));
                lv_style_set_line_width(&triangle_style, 1);
                lv_obj_add_style(triangle, &triangle_style, 0);
                lv_obj_add_style(linha, &triangle_style, 0);
                lv_obj_add_style(ponto, &triangle_style, 0);
            }
        }
        else
        {
            // Remove o triângulo e seus elementos, se foram criados
            if (triangle != nullptr) {
                lv_obj_del(triangle);
                triangle = nullptr;
            }
            if (linha != nullptr) {
                lv_obj_del(linha);
                linha = nullptr;
            }
            if (ponto != nullptr) {
                lv_obj_del(ponto);
                ponto = nullptr;
            }
        }

        // Cálculo da média de pressão para exibição
        if (pressure >= 0 || pressure_bar >= 0)
        {   
            if (leitura > 0)
            {
                soma_sensores += pressure;
                leitura--;
            }
            else
            {
                media = soma_sensores / 10;
                exibir_media = true;
                soma_sensores = 0;
                leitura = 10;
            }
        }

        if (contador_navegacao == 0)
        {
            lv_label_set_text(label_pressao, "PSI");
            lv_label_set_text(label_temp, "TEMP");
            if (exibir_media == true)
            {
                lv_label_set_text_fmt(labelPressure, "%6.2f", media);
                vTaskDelay(5000 / portTICK_PERIOD_MS);
            }
            else
            {
                lv_label_set_text_fmt(labelPressure, "%6.2f", pressure);
                exibir_media = false;
            }
            lv_label_set_text_fmt(labelTemperature, "%6.2fC", SMP3011.getTemperature());
        }

        if (contador_navegacao == 1)
        {
            lv_label_set_text(label_pressao, "BAR");
            lv_label_set_text(label_temp, "TEMP");
            if (exibir_media == true)
            {
                lv_label_set_text_fmt(labelPressure, "%6.2f", media);
                vTaskDelay(5000 / portTICK_PERIOD_MS);
            }
            else
            {
                lv_label_set_text_fmt(labelPressure, "%6.2f", pressure_bar);
                exibir_media = false;
            }
            lv_label_set_text_fmt(labelTemperature, "%6.2fC", SMP3011.getTemperature());
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}




void TaskBlink(void *parameter)
{
    while(1)
    {
        gpio_set_level(EXAMPLE_PIN_LED, 1); 
        vTaskDelay(500/portTICK_PERIOD_MS);
        gpio_set_level(EXAMPLE_PIN_LED, 0); 
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}

void TaskSensors(void *parameter)
{
    while(1)
    {
        BMP280.poll();
        SMP3011.poll();
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

