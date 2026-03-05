#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <inttypes.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include <hd44780.h>
#include <string.h>

#define LOOP_DELAY_MS           10      // Loop sampling time (ms)
#define DEBOUNCE_TIME           40      // Debounce time (ms)
#define NROWS                   4       // Number of keypad rows
#define NCOLS                   4       // Number of keypad columns

#define ACTIVE                  0       // Keypad active state (0 = low, 1 = high)

#define NOPRESS                 '\0'    // NOPRESS character



#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_N          LEDC_CHANNEL_0
#define LEDC_CHANNEL_S          LEDC_CHANNEL_2
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT
#define LEDC_MOTOR_1            (9)
#define LEDC_MOTOR_2            (10)        
#define LEDC_FREQUENCY          (50)
#define GREEN_LED               GPIO_NUM_11
#define RED_LED                 GPIO_NUM_12
#define BUZZER_GPIO             GPIO_NUM_5

int potentiometers[] =    {GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_21};

int row_pins[] = {GPIO_NUM_3, GPIO_NUM_8, GPIO_NUM_18, GPIO_NUM_17};     // Pin numbers for rows
int col_pins[] = {GPIO_NUM_16, GPIO_NUM_15, GPIO_NUM_7, GPIO_NUM_6};   // Pin numbers for columns

char keypad_array[NROWS][NCOLS] = {   // Keypad layout
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

hd44780_t lcd = {
    .write_cb = NULL,
    .font = HD44780_FONT_5X8,
    .lines = 2,
    .pins = {
        .rs = GPIO_NUM_38,
        .e  = GPIO_NUM_37,
        .d4 = GPIO_NUM_36,
        .d5 = GPIO_NUM_35,
        .d6 = GPIO_NUM_48,
        .d7 = GPIO_NUM_47,
        .bl = HD44780_NOT_USED
    }
};



static void ledc_init(void);
void lcd_test(void *pvParameters);
char scan_keypad(void);

typedef enum{
    OPEN,
    CLOSED
} State_t;
State_t openState = CLOSED;
void declare_items(void);


void app_main(void)
{
    ESP_ERROR_CHECK(hd44780_init(&lcd));
    declare_items();
    ledc_init();
    int timer = 0;
    char passcode[7] = "3C3218";
    char code[7] = "";
    char prevKey = NOPRESS;
    int attemptsFailed = 0;
    char code_str[16];
    hd44780_gotoxy(&lcd, 0, 0);
    snprintf(code_str,  17, "Passcode:");
    hd44780_puts(&lcd, code_str);
    while(1){
        switch(openState) {
            case CLOSED:
                gpio_set_level(RED_LED, 1);
                gpio_set_level(GREEN_LED, 0);
                if(timer > 0){
                    hd44780_clear(&lcd);
                    hd44780_gotoxy(&lcd, 0, 0);
                    char time[15];
                    //strcat(time, timer);
                    snprintf(time,  22, "Time left: %d", timer);
                    hd44780_puts(&lcd, time);
                    timer--;
                    vTaskDelay(1000/ portTICK_PERIOD_MS);
                    if(timer == 0){
                        hd44780_clear(&lcd);
                        hd44780_gotoxy(&lcd, 0, 0);
                        snprintf(code_str,  17, "Passcode: %s", code);
                        hd44780_puts(&lcd, code_str);
                    }
                    gpio_set_level(BUZZER_GPIO, 0);
                    if(timer != 0){
                        hd44780_clear(&lcd);
                    }
                }
                if(timer == 0){
                    char currentKey = scan_keypad();
                    if(currentKey != NOPRESS && currentKey != prevKey && currentKey != '#'){
                        int len = strlen(code);
                        if(len < 6){
                            code[len] = currentKey;
                            code[len + 1] = '\0';
                        }
                        if(currentKey == '*'){
                            code[0] = '\0';
                        }
                        //snprintf(code, 7, "%s %c", code, currentKey);
                        prevKey = currentKey;

                        hd44780_clear(&lcd);
                        hd44780_gotoxy(&lcd, 0, 0);
                        snprintf(code_str,  17, "Passcode: %s", code);
                        hd44780_puts(&lcd, code_str);
                        printf("%s \n", code_str);
                    }
                    else if(currentKey == NOPRESS){
                        prevKey = NOPRESS;
                    }
                    else if(currentKey == '#'){
                        bool codeequal=true;
                        if(strlen(code) == strlen(passcode)){
                            for(int i = 0; i < strlen(code); i++){
                                if(code[i] != passcode[i]){
                                    codeequal = false;
                                }
                            }
                        }
                        else{
                            codeequal = false;
                        }
                        if(codeequal){
                            openState = OPEN;
                            printf("the safe is open \n");
                        }
                        else{
                            attemptsFailed++;
                            printf("incorrect code \n");
                            if(attemptsFailed < 3){
                                timer = 1;
                                gpio_set_level(BUZZER_GPIO, 1);
                            }
                            else{
                                timer = 60 * (attemptsFailed-2);
                                gpio_set_level(BUZZER_GPIO, 1);
                            }
                        }
                        //code[0] = '\0';
                    }
                    vTaskDelay(25/ portTICK_PERIOD_MS);
                }
                break;
            case OPEN:
                gpio_set_level(RED_LED, 0);
                gpio_set_level(GREEN_LED, 1);
                vTaskDelay(25/ portTICK_PERIOD_MS);
                break;
        }
    }
}

void open_door(void){}

void declare_items()
{
    for (int i = 0; i < sizeof(row_pins)/sizeof(row_pins[0]); i++){
        gpio_reset_pin(row_pins[i]);
        gpio_set_direction(row_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(row_pins[i], 1);
    }
    for (int i = 0; i < sizeof(col_pins)/sizeof(col_pins[0]); i++){
        gpio_reset_pin(col_pins[i]);
        gpio_set_direction(col_pins[i], GPIO_MODE_INPUT);
        gpio_pullup_en(col_pins[i]);
    }
    gpio_reset_pin(GREEN_LED);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);

    gpio_reset_pin(RED_LED);
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);

    gpio_reset_pin(BUZZER_GPIO);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
}
static void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 50 Hz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));


    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_N,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_MOTOR_2,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    ledc_channel_config_t ledc_channel_2 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_N,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_MOTOR_2,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_2);
}


char scan_keypad()
{
    for(int i = 0; i < sizeof(row_pins)/sizeof(row_pins[0]); i++){
        gpio_set_level(row_pins[i], 0);
        for(int k = 0; k < sizeof(col_pins)/sizeof(col_pins[0]); k++){
            if(!gpio_get_level(col_pins[k])){
                gpio_set_level(row_pins[i], 1);
                //printf("%c \n", keypad_array[i][k]);
                return keypad_array[i][k];
            }
        }
        gpio_set_level(row_pins[i], 1);
    }
    
    return '\0';
}