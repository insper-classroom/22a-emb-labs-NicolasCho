/**
 * 5 semestre - Eng. da Computação - Insper
 * Rafael Corsi - rafael.corsi@insper.edu.br
 *
 * Projeto 0 para a placa SAME70-XPLD
 *
 * Objetivo :
 *  - Introduzir ASF e HAL
 *  - Configuracao de clock
 *  - Configuracao pino In/Out
 *
 * Material :
 *  - Kit: ATMEL SAME70-XPLD - ARM CORTEX M7
 */

/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include "asf.h"

/************************************************************************/
/* defines                                                              */
/************************************************************************/

#define LED_PIO     PIOC
#define LED_PIO_ID    ID_PIOC   
#define LED_PIO_IDX    8   
#define LED_PIO_IDX_MASK   (1 << LED_PIO_IDX)  

// Configuracoes do botao
#define BUT_PIO    PIOA
#define BUT_PIO_ID  ID_PIOA
#define BUT_PIO_IDX   11
#define BUT_PIO_IDX_MASK (1u << BUT_PIO_IDX)

// Placa OLED
#define OLED_PIO1     PIOA
#define OLED_PIO1_ID    ID_PIOA
#define OLED_PIO1_IDX    0
#define OLED_PIO1_IDX_MASK   (1 << OLED_PIO1_IDX)

#define OLED_PIO2     PIOC
#define OLED_PIO2_ID    ID_PIOC
#define OLED_PIO2_IDX    30
#define OLED_PIO2_IDX_MASK   (1 << OLED_PIO2_IDX)

#define OLED_PIO3     PIOB
#define OLED_PIO3_ID    ID_PIOB
#define OLED_PIO3_IDX    2
#define OLED_PIO3_IDX_MASK   (1 << OLED_PIO3_IDX)
//
#define BUT_PIO1    PIOD
#define BUT_PIO1_ID  ID_PIOD
#define BUT_PIO1_IDX   28
#define BUT_PIO1_IDX_MASK (1u << BUT_PIO1_IDX)

#define BUT_PIO2    PIOC
#define BUT_PIO2_ID  ID_PIOC
#define BUT_PIO2_IDX   31
#define BUT_PIO2_IDX_MASK (1u << BUT_PIO2_IDX)

#define BUT_PIO3    PIOA
#define BUT_PIO3_ID  ID_PIOA
#define BUT_PIO3_IDX   19
#define BUT_PIO3_IDX_MASK (1u << BUT_PIO3_IDX)

//LAB2
/*  Default pin configuration (no attribute). */
#define _PIO_DEFAULT             (0u << 0)
/*  The internal pin pull-up is active. */
#define _PIO_PULLUP              (1u << 0)
/*  The internal glitch filter is active. */
#define _PIO_DEGLITCH            (1u << 1)
/*  The internal debouncing filter is active. */
#define _PIO_DEBOUNCE            (1u << 3)


/************************************************************************/
/* constants                                                            */
/************************************************************************/

/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

void init(void);



/************************************************************************/
/* interrupcoes                                                         */
/************************************************************************/

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/
void _pio_set(Pio *p_pio, const uint32_t ul_mask)
{
	p_pio->PIO_SODR = ul_mask; 
}

void _pio_clear(Pio *p_pio, const uint32_t ul_mask)
{
	p_pio->PIO_CODR = ul_mask;
}

void _pio_pull_up(Pio *p_pio, const uint32_t ul_mask,
		const uint32_t ul_pull_up_enable){
	if(ul_pull_up_enable){
		p_pio->PIO_PUER = ul_mask;
	}else{
		p_pio->PIO_PUDR = ul_mask;
	}
}

void _pio_set_input(Pio *p_pio, const uint32_t ul_mask,
		const uint32_t ul_attribute)
{
	_pio_pull_up(p_pio, ul_mask, PIO_PULLUP & ul_attribute);
	if(ul_attribute & (_PIO_DEGLITCH | _PIO_DEBOUNCE)){
		p_pio->PIO_IFER = ul_mask;
	}else{
		p_pio->PIO_IFDR = ul_mask;
	}
	
}

void _pio_set_output(Pio *p_pio, const uint32_t ul_mask,
		const uint32_t ul_default_level,
		const uint32_t ul_multidrive_enable,
		const uint32_t ul_pull_up_enable)
{
	p_pio->PIO_PER = ul_mask;  //PIO como controlador
	p_pio->PIO_OER = ul_mask;  //Pino como saida
	//Saida inicial do pino
	if(ul_default_level){
		_pio_set(p_pio, ul_mask);
	}else{
		_pio_clear(p_pio, ul_mask);
	}
	
	//Ativar ou nao multidrive
	if(ul_multidrive_enable){
		p_pio->PIO_MDER = ul_mask;
	}else{
		p_pio->PIO_MDDR = ul_mask;
	}
	
	//Ativar ou nao pull-up
	_pio_pull_up(p_pio, ul_mask, ul_pull_up_enable);
}

uint32_t _pio_get(Pio *p_pio, const pio_type_t ul_type,
        const uint32_t ul_mask)
{
	uint32_t io_mask; //Mascara do valor a ser lido
	
	//Verifica se esta lendo entrada ou saida
	if(ul_type == PIO_INPUT){
		io_mask = p_pio->PIO_PDSR;
	}else{
		io_mask = p_pio->PIO_ODSR;
	}
	
	//Verifica se o pino passado como argumento esta em alto
	if((io_mask & ul_mask) == 0){
		return 0;
	}else{
		return 1;
	}
}

_delay_ms(int delay){
	for(int i = 0; i<300000*delay; i++){
		asm("NOP");
	}
}

// Função de inicialização do uC
void init(void)
{
	  // Initialize the board clock
	  sysclk_init();

	  // Desativa WatchDog Timer
	  WDT->WDT_MR = WDT_MR_WDDIS;
	  
	  // Ativa o PIO na qual o LED foi conectado
	  // para que possamos controlar o LED.
	  pmc_enable_periph_clk(LED_PIO_ID);
	  
	  // Inicializa PIO do botao
	  pmc_enable_periph_clk(BUT_PIO_ID);
	  
	  // Ativa os PIO's dos leds da placa de OLED
	  //
	  pmc_enable_periph_clk(OLED_PIO1_ID);
	  pmc_enable_periph_clk(OLED_PIO2_ID);
	  pmc_enable_periph_clk(OLED_PIO3_ID);
	  
	  // Inicializa PIO dos botões da OLED
	  //
	  pmc_enable_periph_clk(BUT_PIO1_ID);
	  pmc_enable_periph_clk(BUT_PIO2_ID);
	  pmc_enable_periph_clk(BUT_PIO3_ID);
	  
	  
	  //Inicializa PC8 como saída
	  _pio_set_output(LED_PIO, LED_PIO_IDX_MASK, 0, 0, 0);
	  
	  //Inicializa os leds da placa 
	  //
	  _pio_set_output(OLED_PIO1, OLED_PIO1_IDX_MASK, 0, 0, 0);
	  _pio_set_output(OLED_PIO2, OLED_PIO2_IDX_MASK, 0, 0, 0);
	  _pio_set_output(OLED_PIO3, OLED_PIO3_IDX_MASK, 0, 0, 0);
	  
	  //Configura botões da placa OLED
	  //
	  _pio_set_input(BUT_PIO1, BUT_PIO1_IDX_MASK, PIO_DEFAULT);
	  _pio_pull_up(BUT_PIO1, BUT_PIO1_IDX_MASK, 1);
	  
	  _pio_set_input(BUT_PIO2, BUT_PIO2_IDX_MASK, PIO_DEFAULT);
	  _pio_pull_up(BUT_PIO2, BUT_PIO2_IDX_MASK, 1);
	  
	  _pio_set_input(BUT_PIO3, BUT_PIO3_IDX_MASK, PIO_DEFAULT);
	  _pio_pull_up(BUT_PIO3, BUT_PIO3_IDX_MASK, 1);
	  
	  // configura pino ligado ao botão como entrada com um pull-up.
	  /* 
	  pio_set_input(BUT_PIO, BUT_PIO_IDX_MASK, PIO_DEFAULT);
	  _pio_pull_up(BUT_PIO, BUT_PIO_IDX_MASK, 1);
	  */
	  _pio_set_input(BUT_PIO, BUT_PIO_IDX_MASK, _PIO_PULLUP | _PIO_DEBOUNCE);

}

/************************************************************************/
/* Main                                                                 */
/************************************************************************/

// Funcao principal chamada na inicalizacao do uC.
int main(void)
{
  init();

  // super loop
  // aplicacoes embarcadas não devem sair do while(1).
  while (1)
  {		
	  //OLED 1
		if(_pio_get(PIOD, PIO_INPUT, BUT_PIO1_IDX_MASK) != 1){
			int i;
			for(i=0; i<5; i++){
				_pio_set(PIOA, OLED_PIO1_IDX_MASK);      // Coloca 1 no pino LED
				_delay_ms(200);                        // Delay por software de 200 ms
				_pio_clear(PIOA, OLED_PIO1_IDX_MASK);    // Coloca 0 no pino do LED
				_delay_ms(200);                        // Delay por software de 200 ms
			}
		}
	  
	  
	  //OLED 2
		if(_pio_get(PIOC, PIO_INPUT, BUT_PIO2_IDX_MASK) != 1){
			int i;
			for(i=0; i<5; i++){
				_pio_set(PIOC, OLED_PIO2_IDX_MASK);      // Coloca 1 no pino LED
				_delay_ms(200);                        // Delay por software de 200 ms
				_pio_clear(PIOC, OLED_PIO2_IDX_MASK);    // Coloca 0 no pino do LED
				_delay_ms(200);                        // Delay por software de 200 ms
			}
		}
		
		//OLED 3
		if(_pio_get(PIOA, PIO_INPUT, BUT_PIO3_IDX_MASK) != 1){
			int i;
			for(i=0; i<5; i++){
				_pio_set(PIOB, OLED_PIO3_IDX_MASK);      // Coloca 1 no pino LED
				_delay_ms(200);                        // Delay por software de 200 ms
				_pio_clear(PIOB, OLED_PIO3_IDX_MASK);    // Coloca 0 no pino do LED
				_delay_ms(200);                        // Delay por software de 200 ms
			}
		}
	  
	  //LED
		if(_pio_get(PIOA, PIO_INPUT, BUT_PIO_IDX_MASK) != 1){
			int i;
			for(i=0; i<5; i++){
				_pio_set(PIOC, LED_PIO_IDX_MASK);      // Coloca 1 no pino LED
				_delay_ms(200);                        // Delay por software de 200 ms
				_pio_clear(PIOC, LED_PIO_IDX_MASK);    // Coloca 0 no pino do LED
				_delay_ms(200);                        // Delay por software de 200 ms
			}
		}
  }
  return 0;
}
