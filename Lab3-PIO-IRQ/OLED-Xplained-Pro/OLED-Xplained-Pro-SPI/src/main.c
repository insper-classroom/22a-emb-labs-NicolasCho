#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/***********************************************************************/
//LED
#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      8
#define LED_IDX_MASK (1 << LED_IDX)
// Botão - SW0
#define BUT_PIO      PIOA
#define BUT_PIO_ID   ID_PIOA
#define BUT_IDX  11
#define BUT_IDX_MASK (1 << BUT_IDX)
//BOTAO 1 - FREQUENCIA
#define BUT1_PIO    PIOD
#define BUT1_PIO_ID  ID_PIOD
#define BUT1_IDX   28
#define BUT1_IDX_MASK (1u << BUT1_IDX)
//BOTAO 2 - PARA O PISCA
#define BUT2_PIO    PIOC
#define BUT2_PIO_ID  ID_PIOC
#define BUT2_IDX   31
#define BUT2_IDX_MASK (1u << BUT2_IDX)
//BOTAO 3 - DIMINUI FREQ (APERTO CURTO)
#define BUT3_PIO    PIOA
#define BUT3_PIO_ID  ID_PIOA
#define BUT3_IDX   19
#define BUT3_IDX_MASK (1u << BUT3_IDX)

/**************************************************************************/
//FLAGS
volatile char stop_flag;
volatile char but_flag, but1_flag_down, but1_flag_up, but2_flag_stop, but3_flag;

/**************************************************************************/
//DECLARATION
void io_init(void);
void pisca_led(int n, int t);
void progress_bar(int progress);

/***************************************************************************/
//CALLBACKS
void but_callback(void)
{
	but_flag = 1;
}

void but1_callback(void)
{
	if(pio_get(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK)){
		but1_flag_down = 0;
		but1_flag_up = 1;
	}else{
		but1_flag_down = 1;
	}
}

void but2_callback(void){
	stop_flag = 1;
}

void but3_callback(void){
	but3_flag = 1;
}

/***************************************************************************/
// pisca led N vez no periodo T
void pisca_led(int n, int t){
	gfx_mono_draw_rect(5, 20, 30, 5, 1);
	for (int i=0;i<n;i++){
		pio_clear(LED_PIO, LED_IDX_MASK);
		delay_ms(t);
		pio_set(LED_PIO, LED_IDX_MASK);
		delay_ms(t);	
		if(stop_flag){
			i = n-1;
		}
		progress_bar(i);
	}
	gfx_mono_draw_filled_rect(5, 20, 30, 5, 0);
	stop_flag = 0;
}

void show_oled(int delay){
	double freq = 1/(delay*2*0.001);
	char str[128]; 
	sprintf(str, "%.1f", freq); 
	gfx_mono_draw_string("freq:", 5,8, &sysfont);
	gfx_mono_draw_string(str, 60, 8, &sysfont);
	gfx_mono_draw_string("Hz", 90, 8, &sysfont);
}

int aumenta_diminui(int cont, int delay){
	if(cont > 30000000){
		delay += 100;
	}else{
		if(delay > 100){
			delay -= 100;
		}
	}
	return delay;
}

void progress_bar(int progress){
	gfx_mono_draw_filled_rect(5+progress, 20, 1, 5, 1);
	if(stop_flag){
		gfx_mono_draw_filled_rect(5, 20, 30, 5, 1);
	}
}

// Inicializa botao do kit com interrupcao
void io_init(void)
{
	// Configura led
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);

	// Inicializa clock do periférico PIO responsavel pelos botões
	pmc_enable_periph_clk(BUT_PIO_ID);
	pmc_enable_periph_clk(BUT1_PIO_ID);
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pmc_enable_periph_clk(BUT3_PIO_ID);

	// Configura PIO para lidar com o pino dos botões como entrada
	// com pull-up
	pio_configure(BUT_PIO, PIO_INPUT, BUT_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO, BUT_IDX_MASK, 60);
	
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT1_PIO, BUT1_IDX_MASK, 60);
	
	pio_configure(BUT2_PIO, PIO_INPUT, BUT2_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT2_PIO, BUT2_IDX_MASK, 60);
	
	pio_configure(BUT3_PIO, PIO_INPUT, BUT3_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT3_PIO, BUT3_IDX_MASK, 60);
	
	// Configura interrupção no pino referente aos botões e associa
	// função de callback caso uma interrupção for gerada
	// a função de callback é a: but_callback()
	pio_handler_set(BUT_PIO,
					BUT_PIO_ID,
					BUT_IDX_MASK,
					PIO_IT_RISE_EDGE,
					but_callback);
	
	pio_handler_set(BUT1_PIO,
					BUT1_PIO_ID,
					BUT1_IDX_MASK,
					PIO_IT_EDGE,
					but1_callback);
					
	pio_handler_set(BUT2_PIO,
					BUT2_PIO_ID,
					BUT2_IDX_MASK,
					PIO_IT_FALL_EDGE,
					but2_callback);	

	pio_handler_set(BUT3_PIO,
					BUT3_PIO_ID,
					BUT3_IDX_MASK,
					PIO_IT_FALL_EDGE,
					but3_callback);

	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(BUT_PIO, BUT_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO);
	
	pio_enable_interrupt(BUT1_PIO, BUT1_IDX_MASK);
	pio_get_interrupt_status(BUT1_PIO);
	
	pio_enable_interrupt(BUT2_PIO, BUT2_IDX_MASK);
	pio_get_interrupt_status(BUT2_PIO);
	
	pio_enable_interrupt(BUT3_PIO, BUT3_IDX_MASK);
	pio_get_interrupt_status(BUT3_PIO);
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 4); // Prioridade 4
	
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4); // Prioridade 4
	
	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_SetPriority(BUT2_PIO_ID, 3); // Prioridade 3
	
	NVIC_EnableIRQ(BUT3_PIO_ID);
	NVIC_SetPriority(BUT3_PIO_ID, 4); // Prioridade 3
}



int main (void)
{
	board_init();
	sysclk_init();
	delay_init();
	io_init();

  // Init OLED
	gfx_mono_ssd1306_init();
  
	
  // FREQUENCIA DE PISCA
	int delay = 200;
	int cont = 0;
	
	show_oled(delay);
  /* Insert application code here, after the board has been initialized. */
	while(1) {
		
		//PISCA O LED
		if(but_flag){
			pisca_led(30, delay);
			but_flag = 0;
		}
		
		//AUMENTA OU DIMINUI FREQUENCIA
		while(but1_flag_down){
			cont++;	
		}
		if(but1_flag_up){
			delay = aumenta_diminui(cont, delay);
			show_oled(delay);
			cont = 0;
			but1_flag_up = 0;
		}
		
		if(but_flag){
			pisca_led(30, delay);
			but_flag = 0;
		}
		
		//ABAIXA A FLAG DE STOP
		if(stop_flag){
			stop_flag = 0;
		}
		
		//DIMINUI FREQ
		if(but3_flag){
			delay += 100;
			show_oled(delay);
			but3_flag = 0;
		}
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);

	}
}
