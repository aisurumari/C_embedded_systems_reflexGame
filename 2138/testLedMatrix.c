/******************************************************************************
 *
 * Autorzy:
 *    Piotr Kiełek (Lider)
 *    Szymon Ruciński
 *    Maria Wichrowska
 * 
 *****************************************************************************/



#include "../pre_emptive_os/api/osapi.h"
#include "../pre_emptive_os/api/general.h"
#include <printf_P.h>
#include <lpc2xxx.h>
#include <consol.h>
#include <config.h>

/******************************************************************************
 * Defines and typedefs
 *****************************************************************************/
#define CRYSTAL_FREQUENCY FOSC
#define PLL_FACTOR        PLL_MUL
#define VPBDIV_FACTOR     PBSD

#define  SPI_CS   0x00008000  //<= new board, old board = 0x00800000


tU8 pattern[8] = {0,0,0,0,0,0,0,0};
tU8 left[8] = {0x08,0x1c,0x2a,0x49,0x08,0x08,0x08,0x08};
tU8 right[8] = {0x08,0x08,0x08,0x08,0x49,0x2a,0x1c,0x08};
tU8 up[8] = {0x08,0x04,0x02,0xff,0x02,0x04,0x08,0x00};
tU8 down[8] = {0x10,0x20,0x40,0xff,0x40,0x20,0x10,0x00};
tU8 circle[8] = {0x18,0x24,0x42,0x81,0x81,0x42,0x24,0x18};
tU8 win[8]= {0x10,0x26,0x46,0x40,0x40,0x46,0x26,0x10};
void ledMatrix(void);	   // Funkcja wywolywana przez przerwania IRQ
extern void zeroTimer();   // Funkcja zerujaca ponizsza zmienna	
extern unsigned int Timer; // Czas liczony w milisekundach
extern volatile int rgbColor; //blue=0,green=1,red=2
volatile unsigned int arrowNumber=10; // Numer obecnej strzalki, od 0 do 9
extern void lcdShowResult();
extern void lcdShowNewGame();
extern void lcdShowBadResult();
extern void lcdShowLose();
extern void lcdShowWin();

/*****************************************************************************
 *
 * Opis:
 *		Funkcja inicjująca Timer1 oraz przerwanie IRQ dla tego timer'a

 ****************************************************************************/
static void
startTimer1(tU16 delayInMs)
{
  //initialize VIC for Timer1 interrupts
  VICIntSelect &= ~0x20;           //Timer1 interrupt is assigned to IRQ (not FIQ)
  VICVectAddr5  = (tU32)ledMatrix; //register ISR address
  VICVectCntl5  = 0x25;            //enable vector interrupt for timer1
  VICIntEnable  = 0x20;            //enable timer1 interrupt
  
  //initialize and start Timer #0
  T1TCR = 0x00000002;                           //disable and reset Timer1
  T1PC  = 0x00000000;                           //no prescale of clock
  T1MR0 = delayInMs *                           //calculate no of timer ticks
         ((CRYSTAL_FREQUENCY * PLL_FACTOR) / (1000 * VPBDIV_FACTOR));
  T1IR  = 0x000000ff;                           //reset all flags before enable IRQs
  T1MCR = 0x00000003;                           //reset counter and generate IRQ on MR0 match
  T1TCR = 0x00000001;                           //start Timer1
}

/*****************************************************************************
 *
 * Opis:
 *		Funkcja pokazujaca strzalke na 8x8 LEDMatrix
 *		Strzalki numerowane sa zgodnie z numerowaniem
 *		IOPIN'ow na LPC2148v3, z dodaniem 17 do numeru strzalki:
 * 		0 - Up, 1- Right, 2- Left, 3 - Down
 *
 ****************************************************************************/
void
displayArrow(int arrow)
{
	if(arrow==0) {
		memcpy(&pattern,&up,sizeof(pattern));
	} else if(arrow==1) {
		memcpy(&pattern,&right,sizeof(pattern));
	} else if(arrow==2) {
		memcpy(&pattern,&left,sizeof(pattern));
	} else {
		memcpy(&pattern,&down,sizeof(pattern));
	}
}

/*****************************************************************************
 *
 * Opis:
 *    Funkcja w ktorej rozgrywa sie wlasciwa rozgrywka gry Refleks
 *
 ****************************************************************************/
void
testLedMatrix(void)
{
  PINSEL0 |= 0x00001500 ;  //Initiering av SPI
  SPI_SPCCR = 0x08;    
  SPI_SPCR  = 0x60;
  IODIR0 |= SPI_CS;
  
  startTimer1(1);

  volatile unsigned int result;
  for(;;)
  {
		int circleFound = 0;
		memcpy(&pattern,&circle,sizeof(pattern));
		lcdShowNewGame();
		rgbColor=0;
		while(!circleFound) { // poczatek gry, oczekiwanie na rozpoczecie
			if ((IOPIN0&(1<<14))==0) {
				circleFound=1;
			}
	    }
		volatile int arrow=0;
		volatile int lastArrow=4;
		volatile int lives=3;
		int foundBad=0;
		int found;
		result=0;
		for(arrowNumber=0;arrowNumber<10;arrowNumber++) {
			found=0;
			while((!foundBad)&&(arrow==lastArrow)){
				arrow=T1TC%4; // wylosowanie nowej strzalki
			}
			if(!foundBad) {
				zeroTimer();
			}
			foundBad=0;
			lastArrow=arrow;
			displayArrow(arrow);
			while(!found) {
				if((IOPIN0&(1<<(17+arrow)))==0) {
					// zostala wcisnieta poprawna strzalka
					found=1;
				}  
				if (((IOPIN0&(1<<(17+(arrow+1)%4)))==0)||((IOPIN0&(1<<(17+(arrow+2)%4)))==0)||((IOPIN0&(1<<(17+(arrow+3)%4)))==0)) {
					// zostala wcisnieta zla strzalka
					if(lives) {  
						--lives;
					}
					foundBad=1;
					found=1;
					--arrowNumber;
				}
			}
			if(!foundBad) {
				// wyswietlenie poprawnego wyniku i zapalenie zielonej diody
				result+=Timer;
				rgbColor=1;
				lcdShowResult(arrowNumber, arrow, Timer, lives);
			}
			else if(lives) {
				// wyswietlenie bledu i zapalenie czerwonej diody
				rgbColor=2;
				lcdShowBadResult(lives);
			}
			else {
				// wyswietlenie przegrania gry i zapalenie czerwonej diody
				rgbColor=2;
				lcdShowLose(arrowNumber+1);
				arrowNumber=11;
				osSleep(500);
			}
		}
		if(lives) {
			// wyswietlenie konca gry
			osSleep(100);
			memcpy(&pattern,&win,sizeof(pattern));
			lcdShowWin(result);
			osSleep(500);
		}
	}
}
