#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/***********************************************************************/
//ECHO
#define ECHO_PIO      PIOA
#define ECHO_PIO_ID   ID_PIOA
#define ECHO_PIO_IDX      6
#define ECHO_PIO_IDX_MASK (1 << ECHO_PIO_IDX)

#define TRIGGER_PIO      PIOD
#define TRIGGER_PIO_ID   ID_PIOD
#define TRIGGER_PIO_IDX      30
#define TRIGGER_PIO_IDX_MASK (1 << TRIGGER_PIO_IDX)

// Botão 
#define BUT_PIO      PIOD
#define BUT_PIO_ID   ID_PIOD
#define BUT_PIO_IDX  28
#define BUT_PIO_IDX_MASK (1 << BUT_PIO_IDX)

/**************************************************************************/
//FLAGS
volatile char but_flag, echo_flag, display_flag, timeout_flag;
volatile double freq = 1/(0.000058*2);
volatile double curr_time = 0;

volatile double history[4] = {0, 0, 0, 0};

/**************************************************************************/
//prot

void io_init(void);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);

/***************************************************************************/
//Functions

void add_measurement(double arr[4], double new_value){
	arr[3] = arr[2];
	arr[2] = arr[1];
	arr[1] = arr[0];
	arr[0] = new_value;
}

void show_oled(double distance){
	char str[128];
	sprintf(str, "%.1f", distance);
	gfx_mono_draw_string("      ", 5,8, &sysfont);
	gfx_mono_draw_string(str, 5, 8, &sysfont);
}

void show_error(){
	gfx_mono_generic_draw_filled_rect(60, 3, 60, 6, 0);
	gfx_mono_generic_draw_filled_rect(60, 10, 60, 6, 0);
	gfx_mono_generic_draw_filled_rect(60, 17, 60, 6, 0);
	gfx_mono_generic_draw_filled_rect(60, 24, 60, 6, 0);

	gfx_mono_draw_string("       ", 5,8, &sysfont);
	gfx_mono_draw_string("Erro", 5,8, &sysfont);
}

void plot(double arr[4]){
	gfx_mono_generic_draw_filled_rect(60, 3, 60, 6, 0);
	gfx_mono_generic_draw_filled_rect(60, 10, 60, 6, 0);
	gfx_mono_generic_draw_filled_rect(60, 17, 60, 6, 0);
	gfx_mono_generic_draw_filled_rect(60, 24, 60, 6, 0);
	int y = 3;
	for(int i=0; i<4; i++){
		int n_pixels = arr[i]*60/400;
		if(n_pixels == 0){
			gfx_mono_generic_draw_filled_rect(60, y, 1, 6, 1);
		}else{
			gfx_mono_generic_draw_filled_rect(60, y, n_pixels, 6, 1);
		}
		y += 7;
	}
}


/***************************************************************************/
//CALLBACKS
void but_callback(void){
	but_flag = 1;
}

void echo_callback(void){
	if(!echo_flag){
		tc_stop(TC0, 0);
		RTT_init(freq, 0, 0);
		echo_flag = 1;
	}else{
		curr_time = rtt_read_timer_value(RTT);
		echo_flag = 0;
		display_flag = 1;
	}
	
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);
	
	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {

	}

}

void TC0_Handler(void){
	volatile uint32_t status = tc_get_status(TC0, 0);
	
	timeout_flag = 1;
}
/***************************************************************************/
//INICIALIZACOES

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

	
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);

}

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

void io_init(void)
{
	// Configura Trigger
	pmc_enable_periph_clk(TRIGGER_PIO_ID);
	pio_configure(TRIGGER_PIO, PIO_OUTPUT_0,TRIGGER_PIO_IDX_MASK, PIO_DEFAULT);

	// Configura echo
	pmc_enable_periph_clk(ECHO_PIO_ID);
	pio_set_input(ECHO_PIO, ECHO_PIO_IDX_MASK, PIO_DEFAULT);
	
	// Configura botao
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
					PIO_IT_FALL_EDGE,
					but_callback);

	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(BUT_PIO, BUT_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO);
	
	pio_enable_interrupt(ECHO_PIO, ECHO_PIO_IDX_MASK);
	pio_get_interrupt_status(ECHO_PIO);
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 2); // Prioridade 4
	
	NVIC_EnableIRQ(ECHO_PIO_ID);
	NVIC_SetPriority(ECHO_PIO_ID, 1); // Prioridade 4
}



int main (void)
{
	board_init();
	sysclk_init();
	delay_init();
	io_init();
	
  // Init OLED
	gfx_mono_ssd1306_init();
  
  /* Insert application code here, after the board has been initialized. */
	while(1) {
		if(but_flag){
			pio_set(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK);
			delay_us(10);
			pio_clear(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK);
			
			//
			TC_init(TC0, ID_TC0, 0, 2);
			tc_start(TC0, 0);
			but_flag = 0;
		}
		
		if(timeout_flag){
			show_error();
			tc_stop(TC0, 0);
			timeout_flag = 0;
		}
		
		if(display_flag){
			double rt = curr_time/freq;      //tempo real
			double dist = (340*rt/2.0)*100;  //em cm
			if(dist > 400){
				show_error();
			}else{
				add_measurement(history, dist);
				show_oled(dist);
				plot(history);
			}
			display_flag = 0;
		}
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);

	}
}
