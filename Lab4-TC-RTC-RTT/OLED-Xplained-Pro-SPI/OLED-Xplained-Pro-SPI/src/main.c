#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

//LED1
#define OLED_PIO1     PIOA
#define OLED_PIO1_ID    ID_PIOA
#define OLED_PIO1_IDX    0
#define OLED_PIO1_IDX_MASK   (1 << OLED_PIO1_IDX)
//LED2
#define OLED_PIO2     PIOC
#define OLED_PIO2_ID    ID_PIOC
#define OLED_PIO2_IDX    30
#define OLED_PIO2_IDX_MASK   (1 << OLED_PIO2_IDX)
//LED3
#define OLED_PIO3     PIOB
#define OLED_PIO3_ID    ID_PIOB
#define OLED_PIO3_IDX    2
#define OLED_PIO3_IDX_MASK   (1 << OLED_PIO3_IDX)
//BUT1
#define BUT_PIO1    PIOD
#define BUT_PIO1_ID  ID_PIOD
#define BUT_PIO1_IDX   28
#define BUT_PIO1_IDX_MASK (1u << BUT_PIO1_IDX)

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

volatile char flag_but1, flag_rtc_alarm;
/************************************************************/

void inits(int estado);
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
void pin_toggle(Pio *pio, uint32_t mask);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);
void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);


/************************************************************/
//FUNCOES
void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask)){
		pio_clear(pio, mask);
	}else{
	pio_set(pio,mask);
	}
}

void pisca_led (int n, int t) {
	for (int i=0;i<n;i++){
		pio_clear(OLED_PIO3, OLED_PIO3_IDX_MASK);
		delay_ms(t);
		pio_set(OLED_PIO3, OLED_PIO3_IDX_MASK);
		delay_ms(t);
	}
}
/***********************/
//HANDLERS e CALLBACKS
void TC1_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 1);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(OLED_PIO1, OLED_PIO1_IDX_MASK);  
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	pin_toggle(OLED_PIO2, OLED_PIO2_IDX_MASK);    // BLINK Led
}

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/*
	// second tick 
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o código para irq de segundo vem aqui
	}
	*/
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o código para irq de alame vem aqui
		flag_rtc_alarm = 1;
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

void but1_callback(){
	flag_but1 = 1;
}

/*****************************/
//INITS
void inits(int estado) {
	pmc_enable_periph_clk(OLED_PIO1_ID);
	pio_set_output(OLED_PIO1, OLED_PIO1_IDX_MASK, estado, 0, 0);
	
	pmc_enable_periph_clk(OLED_PIO2_ID);
	pio_set_output(OLED_PIO2, OLED_PIO2_IDX_MASK, estado, 0, 0);
	
	pmc_enable_periph_clk(OLED_PIO3_ID);
	pio_set_output(OLED_PIO3, OLED_PIO3_IDX_MASK, estado, 0, 0);
	
	//Botao
	pmc_enable_periph_clk(BUT_PIO1_ID);

	pio_configure(BUT_PIO1, PIO_INPUT, BUT_PIO1_IDX_MASK, PIO_PULLUP|PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO1, BUT_PIO1_IDX_MASK, 60);

	pio_handler_set(BUT_PIO1,
					BUT_PIO1_ID,
					BUT_PIO1_IDX_MASK,
					PIO_IT_FALL_EDGE,
					but1_callback);	
					
	pio_enable_interrupt(BUT_PIO1, BUT_PIO1_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO1);

	NVIC_EnableIRQ(BUT_PIO1_ID);
	NVIC_SetPriority(BUT_PIO1_ID, 4); // Prioridade 4
	
};

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
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
/****************************************************************/

int main (void)
{
	board_init();
	sysclk_init();
	inits(1);
	TC_init(TC0, ID_TC1, 1, 2);
	tc_start(TC0, 1);
	RTT_init(0.25, 0, RTT_MR_RTTINCIEN); 
	
	calendar rtc_initial = {2018, 3, 19, 12, 15, 45 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN);
	
	delay_init();

  // Init OLED
	gfx_mono_ssd1306_init();
  
  
	gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
   gfx_mono_draw_string("mundo", 50,16, &sysfont);
  
  

  /* Insert application code here, after the board has been initialized. */
	while(1) {
		if(flag_but1){
			/* Leitura do valor atual do RTC */
			uint32_t current_hour, current_min, current_sec;
			uint32_t current_year, current_month, current_day, current_week;
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
			    
			/* configura alarme do RTC para daqui 20 segundos */
			rtc_set_date_alarm(RTC, 1, current_month, 1, current_day);
			rtc_set_time_alarm(RTC, 1, current_hour, 1, current_min, 1, current_sec + 20);
			
			flag_but1 = 0;
		}
		
		if(flag_rtc_alarm){
			pisca_led(1, 300);
			flag_rtc_alarm = 0;
		}
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);			
	}
}
