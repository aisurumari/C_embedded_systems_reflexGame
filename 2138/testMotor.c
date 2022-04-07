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
extern unsigned int arrowNumber;
/*****************************************************************************
 *
 * Opis:
 *	Funkcja uruchamiająca proces obsługujący silnik krokowy. Prędkość i kierunek
 *	obrotu silnika zależne są od strzałki, na której znajduje się gracz
 * 	Prędkość = 18stopni*(arrowNumber+1) na sekundę (regulowane funkcją osSleep)
 * 
 ****************************************************************************/
void
testMotor(void)
{
  static tU32 stepmotorSteps[4] = {0x00201000, 0x00200000, 0x00000000, 0x00001000};  //P0.21 and P0.12 are used to contol the stepper motor
  static tU8 stepmotorIndex = 0;
  
  IODIR0 |= 0x00201000;
  IOCLR0  = stepmotorSteps[0];

  for(;;)
  {  
	if(arrowNumber!=10) {
		if (arrowNumber%2==0) {
			//update to new step (forward)
			stepmotorIndex = (stepmotorIndex + 1) & 0x03;

			//output new step
			IOCLR0 = stepmotorSteps[0];
			IOSET0 = stepmotorSteps[stepmotorIndex];
		} else {
			//update to new step (backwards)
			if (stepmotorIndex == 0) {
				stepmotorIndex = 3;
			} else {
			  stepmotorIndex--;
			}
			//output new step
			IOCLR0 = stepmotorSteps[0];
			IOSET0 = stepmotorSteps[stepmotorIndex];
		}
		osSleep(80-8*arrowNumber);
	}
  }

}

