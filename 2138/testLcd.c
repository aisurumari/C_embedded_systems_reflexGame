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

#define LCD_DATA      0x00ff0000  //P1.16-P1.23
#define LCD_E         0x02000000  //P1.25
#define LCD_RW        0x00400000  //P0.22
#define LCD_RS        0x01000000  //P1.24
#define LCD_BACKLIGHT 0x40000000  //P0.30


/*****************************************************************************
 *
 * Opis:
 *    Funkcja inicjująca wyświetlacz LCD
 *
 ****************************************************************************/
static void
initLCD(void)
{
	IODIR1 |= (LCD_DATA | LCD_E | LCD_RS);
	IOCLR1  = (LCD_DATA | LCD_E | LCD_RS);

	IODIR0 |= LCD_RW;
	IOCLR0  = LCD_RW;
	
  	IODIR0 |= LCD_BACKLIGHT;
  	IOCLR0  = LCD_BACKLIGHT;
}

/*****************************************************************************
 *
 * Opis:
 *    Funkcja opóźniająca działanie procesora o 37us
 *
 ****************************************************************************/
static void
delay37us(void)
{
	volatile tU32 i;
	for(i=0; i<6*2500; i++) {
		asm volatile (" nop");
	}
}

/*****************************************************************************
 *
 * Opis:
 *    Funkcja wywołująca funkcje wyświetlacza LCD
 * 	  dla reg=1 jest to funkcja zapisująca napisy na wyświetlacz
 * 	  dla reg=0 są to poszczególne funkcje wyświetlacza opisane w dokumentacji
 *
 ****************************************************************************/
static void
writeLCD(tU8 reg, tU8 data)
{
	volatile tU8 i;

	if (reg == 0) {
	  	IOCLR1 = LCD_RS;
	} else {
		IOSET1 = LCD_RS;
	}

	  
  	IOCLR0 = LCD_RW;
	IOCLR1 = LCD_DATA;
	IOSET1 = ((tU32)data << 16) & LCD_DATA;
	IOSET1 = LCD_E;
	for(i=0; i<16; i++) {
    	asm volatile (" nop");
	}
	IOCLR1 = LCD_E;
	for(i=0; i<16; i++) {
		asm volatile (" nop");
	} 
}

/*****************************************************************************
 *
 * Opis:
 *    	Funkcja zapalająca podświetlenie wyświetlacza LCD
 *
 ****************************************************************************/
static void
lcdBacklight(tU8 onOff)
{
	if (onOff == TRUE) {
		IOSET0 = LCD_BACKLIGHT;
	} else {
		IOCLR0 = LCD_BACKLIGHT;
	}
}

/******************************************************************************
 *
 * Opis:
 * 		Funkcja wyświetlająca czas liczony w mikrosekundach na ekranie, w formacie
 * 		NN.NNNs
 * 		Funkcja zwraca długość napisu
 * 
 *****************************************************************************/
static int
lcdDisplayTime(unsigned int time) {
	char wynik[6];
	int i=0;
	while((i<6)&&(time!=0)) {
		wynik[i]=(char)(time%10)+'0';
		i++;
		time/=10;
	}
	int j;
	for(j=i-1;j>=0;j--) {
		if(i<4&&j==i-1) {
			writeLCD(1,'0');
			delay37us();
			writeLCD(1,'.');
			delay37us();
		}
		writeLCD(1,wynik[j]);
		delay37us();
		if(j==3) {
			writeLCD(1,'.');
			delay37us();
		}
		if(j==0) {
			writeLCD(1,'s');
			delay37us();
		}
	}
	int return0=i+1;
	if(i<4) {
		return0++;
	}
	return return0;
}

/******************************************************************************
 *
 * Opis:
 * 		Funkcja wyswietlajaca pierwszy wiersz dla wyniku pojedynczego poprawnego
 * 		wcisniecia
 * 		W wierszu wyswietlane sa czas wykorzystany na wcisniecie strzalki
 * 		oraz pozostale zycia
 * 
 *****************************************************************************/

static void
lcdDisplayFirstRow(unsigned int Timer,int lives)
{
	int i;
	for(i=lcdDisplayTime(Timer)+1;i<9;i++) {
		writeLCD(1,' ');
		delay37us();
	}
	char lives_text[]="Zycia:";
	for(i=0;i<6;i++) {
		writeLCD(1,lives_text[i]);
		delay37us();
	}
	writeLCD(1,(char)lives+'0');
	delay37us();
}

/******************************************************************************
 *
 * Opis:
 * 		Funkcja wyswietlajaca drugi wiersz dla wyniku pojedynczego poprawnego
 * 		wcisniecia
 * 		W wierszu wyswietlane sa: nazwa strzalki, i numer (0-9) w cyklu gry
 * 
 *****************************************************************************/
static void
lcdDisplaySecondRow(int arrowNumber, int arrow)
{
	//move cursor to second row
    writeLCD(0, 0x80 | 0x40);
    delay37us();
    char arrow_title[10];
    if(arrow==0) {
    	strcpy(arrow_title,"Gorna, Nr:");
    } else if(arrow==1) {
    	strcpy(arrow_title,"Prawa, Nr:");
    } else if(arrow==2) {
    	strcpy(arrow_title,"Lewa , Nr:");
    } else {
    	strcpy(arrow_title,"Dolna, Nr:");
    }
    int i;
    for(i=0;i<10;i++) {
    	writeLCD(1,arrow_title[i]);
    	delay37us();
    }
    writeLCD(1,((char)arrowNumber+'0'));
    delay37us();
    for(i=0;i<5;i++) {
    	writeLCD(1, ' ');
    	delay37us();
    }
}

/******************************************************************************
 *
 * Opis:
 *		Funkcja wyswietlajaca tytul gry oraz instrukcje do rozpoczecia gry
 * 
 *****************************************************************************/
static void
lcdDisplayNewGame()
{
	char title[]="Gra Refleks B06 ";
	int i=0;
	for(i=0;i<16;i++) {
		writeLCD(1,title[i]);
		delay37us();
	}
	//move cursor to second row
    writeLCD(0, 0x80 | 0x40);
    delay37us();
	char start_title[]="P0.14 - Start :)";
	for(i=0;i<16;i++) {
		writeLCD(1,start_title[i]);
		delay37us();
	}
}

/******************************************************************************
 *
 * Opis:
 * 		Funkcja wyswietlajaca napis informujacy o bledzie uzytkownika w grze
 * 		Wyswietlana jest takze ilosc pozostalych zyc
 * 
 *****************************************************************************/
static void
lcdDisplayBad(int lives)
{
	int i;
	char lives_text[]="Blad!    Zycia:";
	for(i=0;i<15;i++) {
		writeLCD(1,lives_text[i]);
		delay37us();
	}
	writeLCD(1,(char)lives+'0');
	delay37us();
    writeLCD(0, 0x80 | 0x40);
    delay37us();
	char start_title[]="Utrata zycia!   ";
	for(i=0;i<16;i++) {
		writeLCD(1,start_title[i]);
		delay37us();
	}
}

/******************************************************************************
 *
 * Opis:
 * 		Funkcja wyswietlajaca napis po przegraniu gry, informuje on takze
 * 		uzytkownika o numerze strzalki na ktorej przegral
 * 
 *****************************************************************************/
static void
lcdDisplayLoseResult(int arrowNumber)
{
	int i;
	char end_text[]="   Koniec Gry!  ";
	for(i=0;i<16;i++) {
		writeLCD(1,end_text[i]);
		delay37us();
	}
    writeLCD(0, 0x80 | 0x40);
    delay37us();
	char arrow_end[]="Na strzalce nr:";
	for(i=0;i<15;i++) {
		writeLCD(1,arrow_end[i]);
		delay37us();
	}
	writeLCD(1,(char)arrowNumber+'0');
	delay37us();
}

/******************************************************************************
 *
 * Opis:
 * 		Funkcja wyswietlajaca laczny czas gry po jej zakonczeniu
 * 
 *****************************************************************************/
static void
lcdDisplayWinResult(unsigned int result) {
	int i;
	char end_text[]="  Twoj czas to: ";
	for(i=0;i<16;i++) {
		writeLCD(1,end_text[i]);
		delay37us();
	}
	writeLCD(0, 0x80 | 0x40);
    delay37us();
    char congrats[]="Brawo!";
    for(i=lcdDisplayTime(result)+1;i<16;i++) {
    	if(i<10) {
    		writeLCD(1,' ');
    		delay37us();
    	}
    	else {
    		writeLCD(1,congrats[i-10]);
    		delay37us();
    	}
    }

}

/*****************************************************************************
 *
 * Opis:
 *    Funkcja startujaca LCD, uzywajaca funkcji wyswietlacza LCD
 *
 ****************************************************************************/
static void
lcdStartFunction()
{
	initLCD();
	lcdBacklight(FALSE);
	osSleep(10);

	//display clear
	writeLCD(0, 0x01);
	osSleep(10); //actually only 1.52 mS needed
 	lcdBacklight(TRUE);
    osSleep(10);

    //function set
    writeLCD(0, 0x30);
    osSleep(1);
    writeLCD(0, 0x30);
    delay37us();
    writeLCD(0, 0x30);
    delay37us();

    //function set
    writeLCD(0, 0x38);
    delay37us();

    //display off
    writeLCD(0, 0x08);
    delay37us();

    //display clear
    writeLCD(0, 0x01);
    osSleep(10); 
      
    //display control set
    writeLCD(0, 0x06);
    osSleep(10);

    //display control set
    writeLCD(0, 0x0c);
    delay37us();

    //cursor home
    writeLCD(0, 0x02);
    osSleep(1);
}

void
lcdShowResult(int arrowNumber, int arrow, unsigned int Timer, int lives)
{
    lcdStartFunction();
    lcdDisplayFirstRow(Timer, lives);
    lcdDisplaySecondRow(arrowNumber,arrow);
}

void
lcdShowNewGame()
{
	lcdStartFunction();
	lcdDisplayNewGame();
}

void
lcdShowBadResult(int lives) {
	lcdStartFunction();
	lcdDisplayBad(lives);
}
void
lcdShowLose(int arrowNumber) {
	lcdStartFunction();
	lcdDisplayLoseResult(arrowNumber);
}

void
lcdShowWin(unsigned int result) {
	lcdStartFunction();
	lcdDisplayWinResult(result);
}

