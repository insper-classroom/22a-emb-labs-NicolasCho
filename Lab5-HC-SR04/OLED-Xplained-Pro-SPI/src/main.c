#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/***********************************************************************/
//ECHO
#define ECHO_PIO      PIOD
#define ECHO_PIO_ID   ID_PIOD
#define ECHO_PIO_IDX      30
#define ECHO_PIO_IDX_MASK (1 << ECHO_PIO_IDX)

#define TRIGGER_PIO      PIOA
#define TRIGGER_PIO_ID   ID_PIOA
#define TRIGGER_PIO_IDX      6
#define TRIGGER_PIO_IDX_MASK (1 << TRIGGER_PIO_IDX)

// Botão - SW0
#define BUT_PIO      PIOA
#define BUT_PIO_ID   ID_PIOA
#define BUT_PIO_IDX  11
#define BUT_PIO_IDX_MASK (1 << BUT_PIO_IDX)

/**************************************************************************/
//FLAGS
volatile char but_flag, echo_up, echo_down;

/**************************************************************************/
//DECLARATION
void io_init(void);

void show_oled(double distance){
	char str[128];
	sprintf(str, "%.1f", distance);
	gfx_mono_draw_string(str, 60, 8, &sysfont);
}

/***************************************************************************/
//CALLBACKS
void but_callback(void){
	but_flag = 1;
}

void echo_callback(void){
	if(pio_get(ECHO_PIO, PIO_INPUT, ECHO_PIO_IDX_MASK)){
		//Subida
		echo_up = 1;
	}else{
		echo_down = 1;
	}
	
}
/***************************************************************************/
//INICIALIZACOES

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	/*
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}
	*/

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/*
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	*/	
}

void io_init(void)
{
	// Configura Trigger
	pmc_enable_periph_clk(TRIGGER_PIO_ID);
	pio_configure(TRIGGER_PIO, PIO_OUTPUT_0, TRIGGER_PIO_IDX_MASK, PIO_DEFAULT);

	// Configura echo
	pmc_enable_periph_clk(ECHO_PIO_ID);
	pio_configure(ECHO_PIO, PIO_INPUT, ECHO_PIO_IDX_MASK, PIO_DEBOUNCE);
	pio_set_debounce_filter(ECHO_PIO, ECHO_PIO_IDX_MASK, 60);
	
	// Configura botao SW
	pmc_enable_periph_clk(BUT_PIO_ID);
	pio_configure(BUT_PIO, PIO_INPUT, BUT_PIO_IDX_MASK, PIO_PULLUP|PIO_DEBOUNCE);
	pio_set_debounce_filter(ECHO_PIO, BUT_PIO_IDX_MASK, 60);
	
	
	// Configura interrupção no pino referente aos botões e associa
	// função de callback caso uma interrupção for gerada
	// a função de callback é a: but_callback()
	pio_handler_set(ECHO_PIO,
					ECHO_PIO_ID,
					ECHO_PIO_IDX_MASK,
					PIO_IT_EDGE,
					echo_callback);
	
	pio_handler_set(BUT_PIO,
					BUT_PIO_ID,
					BUT_PIO_IDX_MASK,
					PIO_IT_EDGE,
					but_callback);

	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(BUT_PIO, BUT_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO);
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 4); // Prioridade 4
}



int main (void)
{
	board_init();
	sysclk_init();
	delay_init();
	io_init();
	
	int vel = 340;
	double dist;
	
  // Init OLED
	gfx_mono_ssd1306_init();
  
  /* Insert application code here, after the board has been initialized. */
	while(1) {
		if(but_flag){
			pio_set(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK);
			delay_us(10);
			pio_clear(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK);
			but_flag = 0;
		}
		
		if(echo_up){
			RTT_init(1, 0, 0);
			echo_up = 0;
		}
		
		if (echo_down){
			uint32_t current_time = rtt_read_timer_value(RTT);
			dist = vel * current_time;
			show_oled(dist);
			echo_down = 0;
		}
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);

	}
}
