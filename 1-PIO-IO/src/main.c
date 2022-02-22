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
	  pio_set_output(LED_PIO, LED_PIO_IDX_MASK, 0, 0, 0);
	  
	  //Inicializa os leds da placa 
	  //
	  pio_set_output(OLED_PIO1, OLED_PIO1_IDX_MASK, 0, 0, 0);
	  pio_set_output(OLED_PIO2, OLED_PIO2_IDX_MASK, 0, 0, 0);
	  pio_set_output(OLED_PIO3, OLED_PIO3_IDX_MASK, 0, 0, 0);
	  
	  //Configura botões da placa OLED
	  //
	  pio_set_input(BUT_PIO1, BUT_PIO1_IDX_MASK, PIO_DEFAULT);
	  pio_pull_up(BUT_PIO1, BUT_PIO1_IDX_MASK, 1);
	  
	  pio_set_input(BUT_PIO2, BUT_PIO2_IDX_MASK, PIO_DEFAULT);
	  pio_pull_up(BUT_PIO2, BUT_PIO2_IDX_MASK, 1);
	  
	  pio_set_input(BUT_PIO3, BUT_PIO3_IDX_MASK, PIO_DEFAULT);
	  pio_pull_up(BUT_PIO3, BUT_PIO3_IDX_MASK, 1);
	  
	  // configura pino ligado ao botão como entrada com um pull-up.
	  pio_set_input(BUT_PIO, BUT_PIO_IDX_MASK, PIO_DEFAULT);
	  pio_pull_up(BUT_PIO, BUT_PIO_IDX_MASK, 1);
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
		if(pio_get(PIOD, PIO_INPUT, BUT_PIO1_IDX_MASK) != 1){
			int i;
			for(i=0; i<5; i++){
				pio_set(PIOA, OLED_PIO1_IDX_MASK);      // Coloca 1 no pino LED
				delay_ms(200);                        // Delay por software de 200 ms
				pio_clear(PIOA, OLED_PIO1_IDX_MASK);    // Coloca 0 no pino do LED
				delay_ms(200);                        // Delay por software de 200 ms
			}
		}
	  
	  
	  //OLED 2
		if(pio_get(PIOC, PIO_INPUT, BUT_PIO2_IDX_MASK) != 1){
			int i;
			for(i=0; i<5; i++){
				pio_set(PIOC, OLED_PIO2_IDX_MASK);      // Coloca 1 no pino LED
				delay_ms(200);                        // Delay por software de 200 ms
				pio_clear(PIOC, OLED_PIO2_IDX_MASK);    // Coloca 0 no pino do LED
				delay_ms(200);                        // Delay por software de 200 ms
			}
		}
		
		//OLED 3
		if(pio_get(PIOA, PIO_INPUT, BUT_PIO3_IDX_MASK) != 1){
			int i;
			for(i=0; i<5; i++){
				pio_set(PIOB, OLED_PIO3_IDX_MASK);      // Coloca 1 no pino LED
				delay_ms(200);                        // Delay por software de 200 ms
				pio_clear(PIOB, OLED_PIO3_IDX_MASK);    // Coloca 0 no pino do LED
				delay_ms(200);                        // Delay por software de 200 ms
			}
		}
	  
	  //LED
		if(pio_get(PIOA, PIO_INPUT, BUT_PIO_IDX_MASK) != 1){
			int i;
			for(i=0; i<5; i++){
				pio_set(PIOC, LED_PIO_IDX_MASK);      // Coloca 1 no pino LED
				delay_ms(200);                        // Delay por software de 200 ms
				pio_clear(PIOC, LED_PIO_IDX_MASK);    // Coloca 0 no pino do LED
				delay_ms(200);                        // Delay por software de 200 ms
			}
		}
  }
  return 0;
}
