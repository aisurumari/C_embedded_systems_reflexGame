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
//0 = blue, 1 = green, 2 = red
volatile int rgbColor=0;

/*****************************************************************************
 *
 * Opis:
 *    Funkcja inicjalizująca PWM
 *
 ****************************************************************************/
void
initializeRGB(void)
{
    PINSEL0 &= 0xfff03fff;  //Enable PWM2 on P0.7, PWM4 on P0.8, and PWM6 on P0.9
    PINSEL0 |= 0x000a8000;  //Enable PWM2 on P0.7, PWM4 on P0.8, and PWM6 on P0.9

  //PULSE WIDTH MODULATION INIT*********************************************
  PWM_PR  = 0x00;    // Prescale Register
  PWM_MCR = 0x02;    // Match Control Register
  PWM_MR0 = 0x1000;    // TOTAL PERIODTID   T
  PWM_MR2 = 0x0000;    // H�G SIGNAL        t
  PWM_MR4 = 0x0000;    // H�G SIGNAL        t
  PWM_MR6 = 0x0000;    // H�G SIGNAL        t
  PWM_LER = 0x55;    // Latch Enable Register
  PWM_PCR = 0x5400;  // Prescale Counter Register PWMENA2, PWMENA4, PWMENA6
  PWM_TCR = 0x09;    // Counter Enable och PWM Enable
  //************************************************************************ 

}

/*****************************************************************************
 *
 * Opis:
 * Funkcja zapalająca diodę czerwoną i wygaszająca poprzednią diodę
 *
 ****************************************************************************/
void
redRGB(int prev_color) {
	int i;
	for(i=0x0000;i<=0x0300;i+=0x0005) {
		if(prev_color==1) {
			PWM_MR6=0x0300-i;
			PWM_LER=0x40;
			osSleep(1);
		} else if (prev_color==0) {
			PWM_MR4=0x0300-i;
			PWM_LER=0x10;
			osSleep(1);
		}
		if(i>0x0080) {
			i+=0x0005;
		}
	}
	for(i=0;i<=0x0300;i+=0x0005) {
		PWM_MR2=i;
		PWM_LER=0x04;
		if(i>0x0080) {
			i+=0x0005;
		}
		osSleep(1);
	}
}

/*****************************************************************************
 *
 * Opis:
 * Funkcja zapalająca diodę niebieską i wygaszająca poprzednią diodę
 *
 ****************************************************************************/
void
blueRGB(int prev_color) {
	int i;
	for(i=0x0000;i<=0x0300;i+=0x0005) {
		if(prev_color==1) {
			PWM_MR6=0x0300-i;
			PWM_LER=0x40;
			osSleep(1);
		} else if (prev_color==2) {
			PWM_MR2=0x0300-i;
			PWM_LER=0x04;
			osSleep(1);
		}
		if(i>0x0080) {
			i+=0x0005;
		}
	}
	for(i=0;i<=0x0300;i+=0x0005) {
		PWM_MR4=i;
		PWM_LER=0x10;
		if(i>0x0080) {
			i+=0x0005;
		}
		osSleep(1);
	}
}

/*****************************************************************************
 *
 * Opis:
 * Funkcja zapalająca diodę zieloną i wygaszająca poprzednią diodę
 *
 ****************************************************************************/
void
greenRGB(int prev_color) {
	int i;
	for(i=0x0000;i<=0x0300;i+=0x0005) {
		if(prev_color==0) {
			PWM_MR4=0x0300-i;
			PWM_LER=0x10;
			osSleep(1);
		} else if (prev_color==2) {
			PWM_MR2=0x0300-i;
			PWM_LER=0x04;
			osSleep(1);
		}
		if(i>0x0080) {
			i+=0x0005;
		}
	}
	for(i=0;i<=0x0300;i+=0x0005) {
		PWM_MR6=i;
		PWM_LER=0x40;
		if(i>0x0080) {
			i+=0x0005;
		}
		osSleep(1);
	}
}

/*****************************************************************************
 *
 * Opis:
 * Funkcja zabezpieczająca, zgasza wszystkie trzy kolory diody, wywoływana tylko
 * w przypadku nałożenia się zmian koloru w czasie
 * 
 ****************************************************************************/

void
blackoutRGB() {
	PWM_MR4=0x0000;
	PWM_LER=0x10;
	PWM_MR2=0x0000;
	PWM_LER=0x04;
	PWM_MR6=0x0000;
	PWM_LER=0x40;
}

/*****************************************************************************
 *
 * Opis:
 *    Funkcja uruchamiająca proces obsługujący diodę RGB za pomocą PWM
 * 	  Zmiana koloru jest inicjowana poprzez zmianę wartości zmiennej rgbColor
 * 	  0 - niebieski, 1 - zielony, 2- czerwony
 *
 ****************************************************************************/
void
testRGB(){
	initializeRGB();
	blueRGB(0);
	volatile int prevColor=0;
	for(;;) {
		if(rgbColor!=prevColor) {
			int tempColor=rgbColor;
			if(prevColor==4) {
				blackoutRGB();
			}
			if(rgbColor==0) {
				blueRGB(prevColor);
			}
			else if(rgbColor==1) {
				greenRGB(prevColor);
			}
			else if(rgbColor==2) {
				redRGB(prevColor);
			}
			if(tempColor==rgbColor) {
				prevColor=rgbColor;
			} else {
				prevColor=4;
			}
		}
	}
}
