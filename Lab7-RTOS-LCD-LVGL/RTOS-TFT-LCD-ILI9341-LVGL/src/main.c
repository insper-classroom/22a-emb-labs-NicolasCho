/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"

//Fontes
LV_FONT_DECLARE(dseg70);
LV_FONT_DECLARE(dseg40);
LV_FONT_DECLARE(dseg20);

/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/

#define LV_HOR_RES_MAX          (320)
#define LV_VER_RES_MAX          (240)

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv;          /*A variable to hold the drivers. Must be static or global.*/
static lv_indev_drv_t indev_drv;

//global
static lv_obj_t * labelBtn1;
static lv_obj_t * labelMenu;
static lv_obj_t * labelClock;
static lv_obj_t * labelUp;
static lv_obj_t * labelDown;
static lv_obj_t * labelFloor;
static lv_obj_t * labelFloorDigit;
static lv_obj_t * labelSetValue;
static lv_obj_t * labelClockValue;

volatile uint32_t current_hour, current_min, current_sec;
volatile char clock_setter=0;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

#define TASK_RTC_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_RTC_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}

/********************************************************/
/* RTC                                                  */
/********************************************************/
/** Semaforo a ser usado pela task led */
SemaphoreHandle_t xSemaphoreRTC;


typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	// second tick 
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o código para irq de segundo vem aqui
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreRTC, &xHigherPriorityTaskWoken);
	}

	
	/*
	/* Time or date alarm 
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o código para irq de alame vem aqui
		flag_rtc_alarm = 1;
	}
	*/
	
	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/

static void event_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		lv_obj_clean(lv_scr_act());
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void menu_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void clock_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
		clock_setter = !clock_setter;
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void up_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *c;
    int temp;
    if(code == LV_EVENT_CLICKED) {
		if(clock_setter){
			current_min++;
			if(current_min == 60){
				current_hour++;
				current_min = 0;
				
				if(current_hour == 24){
					current_hour = 0;
				}
			}
			rtc_set_time(RTC, current_hour, current_min, current_sec);
		}else{
			c = lv_label_get_text(labelSetValue);
			temp = atoi(c);
			lv_label_set_text_fmt(labelSetValue, "%02d", temp + 1);
		}
		
	    
    }
}

static void down_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *c;
    int temp;
    if(code == LV_EVENT_CLICKED) {
		if(clock_setter){
			current_min--;
			
			if(current_min < 0){
				current_min = 59;
				current_hour--;
				
				if(current_hour < 0){
					current_hour = 23;
				}
			}
		rtc_set_time(RTC, current_hour, current_min, current_sec);
		
		}else{
			c = lv_label_get_text(labelSetValue);
			temp = atoi(c);
			lv_label_set_text_fmt(labelSetValue, "%02d", temp - 1);
		}
    }
}

void lv_ex_btn_1(void) {
	lv_obj_t * label;

	lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);

	label = lv_label_create(btn1);
	lv_label_set_text(label, "Corsi");
	lv_obj_center(label);

	lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);

	label = lv_label_create(btn2);
	lv_label_set_text(label, "Toggle");
	lv_obj_center(label);
}

void lv_termostato() {
	//define estilo
	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_bg_color(&style, lv_color_black());
	lv_style_set_border_color(&style, lv_color_black());
	lv_style_set_border_width(&style, 0);
	
	//POWER BTN
	lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_BOTTOM_LEFT, 10, -30);
	
	lv_obj_add_style(btn1, &style, 0); //aplica estilo 

	labelBtn1 = lv_label_create(btn1);
	lv_label_set_text(labelBtn1, "[  " LV_SYMBOL_POWER);
	lv_obj_center(labelBtn1);
	
	//M BUTTON
	lv_obj_t * menu_btn = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(menu_btn, menu_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(menu_btn, btn1,  LV_ALIGN_OUT_RIGHT_MID, 0, -10);
	
	lv_obj_add_style(menu_btn, &style, 0); //aplica estilo

	labelMenu = lv_label_create(menu_btn);
	lv_label_set_text(labelMenu, "| M |");
	lv_obj_center(labelMenu);
	
	//CLOCK BUTTON
	lv_obj_t * clock_btn = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(clock_btn, clock_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(clock_btn, menu_btn,  LV_ALIGN_OUT_RIGHT_MID, -3, -10);
	
	lv_obj_add_style(clock_btn, &style, 0); //aplica estilo

	labelClock = lv_label_create(clock_btn);
	lv_label_set_text(labelClock, LV_SYMBOL_SETTINGS "  ]");
	lv_obj_center(labelClock);
	
	//UP BUTTON
	lv_obj_t * up_btn = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(up_btn, up_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(up_btn, clock_btn,  LV_ALIGN_OUT_RIGHT_MID, 10, -9);
	
	lv_obj_add_style(up_btn, &style, 0); //aplica estilo

	labelUp = lv_label_create(up_btn);
	lv_label_set_text(labelUp,"[ "LV_SYMBOL_UP);
	lv_obj_center(labelUp);
	
	//DOWN BUTTON
	lv_obj_t * down_btn = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(down_btn, down_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(down_btn, up_btn,  LV_ALIGN_OUT_RIGHT_MID, 0, -10);
	
	lv_obj_add_style(down_btn, &style, 0); //aplica estilo

	labelDown = lv_label_create(down_btn);
	lv_label_set_text(labelDown,"| "LV_SYMBOL_DOWN " ] ");
	lv_obj_center(labelDown);
	
	//TEMPERATURA
	labelFloor = lv_label_create(lv_scr_act());
	lv_obj_align(labelFloor, LV_ALIGN_LEFT_MID, 35 , -45);
	lv_obj_set_style_text_font(labelFloor, &dseg70, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelFloor, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelFloor, "%02d", 23);
	
	//TEMPERATURA DIGITO
	labelFloorDigit = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelFloorDigit, labelFloor,  LV_ALIGN_OUT_RIGHT_MID, 5, 9);
	lv_obj_set_style_text_font(labelFloorDigit, &dseg40, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelFloorDigit, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelFloorDigit, ".%d", 3);
	
	//TEMPERATURA REFERENCIA (SET VALUE)
	labelSetValue = lv_label_create(lv_scr_act());
	lv_obj_align(labelSetValue, LV_ALIGN_RIGHT_MID, 0 , -60);
	lv_obj_set_style_text_font(labelSetValue, &dseg40, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelSetValue, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelSetValue, "%02d", 22);
	
	//RELOGIO HORAS (VALOR)  
	labelClockValue = lv_label_create(lv_scr_act());
	lv_obj_align(labelClockValue, LV_ALIGN_TOP_RIGHT, 0 , 0);
	lv_obj_set_style_text_font(labelClockValue, &dseg20, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelClockValue, lv_color_white(), LV_STATE_DEFAULT);
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_lcd(void *pvParameters) {
	int px, py;
	
	lv_termostato();

	for (;;)  {
		lv_tick_inc(50);
		lv_task_handler();
		//lv_termostato();
		vTaskDelay(50);
	}
}

static void task_rtc(void *pvParameters) {
	calendar rtc_initial = {2022, 5, 12, 12, 0, 0 ,0};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_SECEN);	
	/*
	uint32_t current_year, current_month, current_day, current_week;
	rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
	*/
		
	for (;;) {
		rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
		if (xSemaphoreTake(xSemaphoreRTC, 10)) {
			lv_label_set_text_fmt(labelClockValue, "%02d:%02d", current_hour, current_min);
		}
		
  }
}

/************************************************************************/
/* configs                                                              */
/************************************************************************/

static void configure_lcd(void) {
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
	
	ili9341_init();
	ili9341_backlight_on();
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT,
	};

	/* Configure console UART. */
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
	ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p,  (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
	
	/* IMPORTANT!!!
	* Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
	int px, py, pressed;
	
	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED; 
	
	data->point.x = px;
	data->point.y = py;
}

void configure_lvgl(void) {
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

	lv_disp_t * disp;
	disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
	
	/* Init input on LVGL */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_input_read;
	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
	/* board and sys init */
	board_init();
	sysclk_init();
	configure_console();

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();

	 /* Attempt to create a semaphore. */
	 
	 xSemaphoreRTC = xSemaphoreCreateBinary();
	 if (xSemaphoreRTC == NULL)
		printf("falha em criar o semaforo \n");

	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}

	/* Create task to control RTC */
	if (xTaskCreate(task_rtc, "RTC", TASK_RTC_STACK_SIZE, NULL, TASK_RTC_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create rtc task\r\n");
	}
		
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){ }
}
