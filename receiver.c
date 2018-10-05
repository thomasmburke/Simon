//receiver:
asm(" .length 10000");
asm(" .width 132");
#include "msp430g2553.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>


//********SIMON NOTES**********
#define G5 (638)
#define D5 (851)
#define C5 (956)
#define A4 (1136)
//*****************************

//*******SIMON NOTE LENGTH******
#define NOTE_LENGTH 40
//*****************************

//**********GPIO P1**************
#define MODE1 BIT0
#define RXPORT BIT1
#define TXPORT BIT2
#define SPEAKER BIT3
#define MODE2 BIT4
#define MODE3 BIT5
#define BUTTON1 BIT6
#define BUTTON2 BIT7
//*********************************

//*********GPIO P2************
#define RED BIT0
#define YELLOW BIT1
#define GREEN BIT2
#define BLUE BIT3
#define COLOR_MASK 0xF
#define DIO0 BIT4
#define DIO1 BIT5
//********************************

#define red 0x000E
#define yellow 0x000D
#define green 0x000B
#define blue 0x0007


#define R 0
#define Y 1
#define G 2
#define B 3



//************BAUDRATE******************
#define SMCLKRATE 1000000
#define BAUDRATE 9600
#define BRDIV16 ((16 * SMCLKRATE)/BAUDRATE)
#define BRDIV (BRDIV16/16)
#define BRMOD ((BRDIV16 - (16*BRDIV)+1)/16)
#define BRDIVHI (BRDIV/256)
#define BRDIVLO (BRDIV - BRDIVHI*256)
//**************************************

//************GLOBAL VARIABLES**************
unsigned int note;
volatile int note_counter = 0;
unsigned int note_length_counter=0;
volatile unsigned int simon_pattern[40];
volatile unsigned short int new_char;
volatile unsigned short int array_max = 0;
volatile unsigned short int previous_button_state = 0x000F;
volatile unsigned short int current_button_state = 0x000F;
volatile char state='A';
volatile unsigned short int pattern_guesses=0;
volatile unsigned short int button_pressed=0;
volatile unsigned short int receive;
volatile unsigned char last_button;
volatile unsigned char last_button2;
volatile unsigned char did_i_lose = 0;
volatile unsigned char mp = 0;
volatile unsigned char send;
int delay = 20;
int i=0;
//volatile unsigned char last_button; //used for polling WDT button press
//******************************************
void init_P1gpio(void);
void init_P2gpio(void);
void init_UART(void);
void init_timer(void);

int main(void)
{
   WDTCTL = (WDTPW + WDTTMSEL + WDTCNTCL + 0 + 1);
   IE1 |= WDTIE; //use WDT to poll for analysis
   DCOCTL = 0;
   BCSCTL1 = CALBC1_1MHZ;          // Set DCO and BCS at 1MHz
   DCOCTL = CALDCO_1MHZ;

   P1SEL |= RXPORT + TXPORT ;            // P1.1 = RXPORT, P1.2=TXPORT
   P1SEL2 |= RXPORT + TXPORT ;           // P1.1 = RXPORT, P1.2=TXPORT

   init_P1gpio();
   init_P2gpio();
   init_UART();
   init_timer();

   srand(time(NULL)); //initial random number seed

   for ( i=0; i<40; i++)
   {
	   simon_pattern[i]= (rand()%4);
   }

   __bis_SR_register(LPM0_bits + GIE);
}

void init_UART()
{
   UCA0CTL1 |= UCSSEL_2;           // SMCLK as the source
   UCA0BR1 = BRDIVHI;			   // set baud parameters, prescaler hi
   UCA0BR0 = BRDIVLO;              // prescaler lo
   UCA0MCTL = 2 * BRMOD;           //modulation
   UCA0CTL1 &= ~UCSWRST;           // **Initialize USCI state machine**
   IFG2 &= ~UCA0RXIFG;
   UC0IE |= UCA0RXIE;   // Enable USCI_A0 transmit and receive interrupts
   UC0IE &= ~UCA0TXIE;
}

void init_timer()
{
	TA0CTL |= TACLR;//reset clock
	TA0CTL= TASSEL_2+ID_0+MC_1;//clock source=SMCLK
	//clock divider=8
	//UP mode
	//timer A interrupt on
	CCTL0 = CCIE;
}

void init_P1gpio()
{
  //**********Button1 setup***********************
    P1DIR &= ~BUTTON1;                     // clear interrupt flag
    P1OUT |= BUTTON1;                      // pullup
    P1REN |= BUTTON1;                      // enable register
    //P1IE |= BUTTON1;						//enable interrupt
   // P1IES |= BUTTON1;						//hi/lo edge
   // P1IFG &= ~BUTTON1; 						//CLEAR INTERUPT FLAG
    //***********************************************

    //******BUTTON2 SETUP************************
    P1DIR &= ~BUTTON2;                     // clear interrupt flag
    P1OUT |= BUTTON2;                      // pullup
    P1REN |= BUTTON2;                      // enable register
   // P1IE |= BUTTON2;						//enable interrupt
   // P1IES |= BUTTON1;						//hi/Lo edge
   // P1IFG &= ~BUTTON2; 						//CLEAR INTERUPT FLAG
    //*******************************************

    //*********MODE SETUPS*********************
    //MODE1:
    P1DIR |= MODE1;//SET AS OUTPUT
    P1OUT &=~ MODE1;//TURN OFF
    //MODE2:
    P1DIR |= MODE2;//SET AS OUTPUT
    P1OUT &=~ MODE2;//TURN OFF
    //MODE3:
    P1DIR |= MODE3;//SET AS OUTPUT
    P1OUT &=~ MODE3;//TURN OFF
    //********************************************

    //*************SPEAKER*********************
	P1DIR |= SPEAKER;// set as output
	P1OUT &= ~SPEAKER; //low
	//*****************************************
}

void init_P2gpio()
{
	 //************COLOR BUTTON SETUP**************
	//RED:
	P2DIR &= ~RED;                     //
	P2OUT |= RED;                      // pullup
	P2REN |= RED;                      // enable register

	//YELLOW:
	P2DIR &= ~YELLOW;                     //
	P2OUT |= YELLOW;                      // pullup
	P2REN |= YELLOW;                      // enable register

	//GREEN:
	P2DIR &= ~GREEN;                     //
	P2OUT |= GREEN;                      // pullup
	P2REN |= GREEN;                      // enable register

	//BLUE:
	P2DIR &= ~BLUE;                     //
	P2OUT |= BLUE;                      // pullup
	P2REN |= BLUE;                      // enable register

	//DIO
	//P2DIR &= ~DIO0;
	//P2OUT &= ~DIO0;
	//P2REN |= DIO0;
	//P2DIR |= DIO1;
	//P2OUT &= ~DIO1;

	//********************************************
}


inline int note_synthesizer(volatile unsigned char new_char)
{
	int note;
	if(new_char==0)
	{
		note=G5;
		return note;
	}
	else if(new_char==1)
	{
		note=D5;
		return note;
	}
	else if(new_char==2)
	{
		note=C5;
		return note;
	}
	else
	{
		note=A4;
		return note;
	}
}

inline void pattern_playback()   // state B
{
	if(note_counter != 0 && delay < 10)
	{
		CCR0 = Y; //inaudible :)
	}
	--delay;
	if(note_counter < array_max && delay == 0)
	{
		delay = 100;
		//turn off all leds*************************
		P2DIR |= COLOR_MASK;
		P2OUT |= COLOR_MASK; // Turn off LED's
		P2REN &= ~COLOR_MASK;

		CCR0=note_synthesizer(simon_pattern[note_counter]);//play the not note_counter indexes
		if(simon_pattern[note_counter] == R)
		{
			P2OUT &= ~RED; // Turn on red led
		}
		else if (simon_pattern[note_counter] == Y)
		{
			P2OUT &= ~YELLOW; //turn on yellow led
		}
		else if (simon_pattern[note_counter] == G)
		{
			P2OUT &= ~GREEN; //turn on green led
		}
		else if (simon_pattern[note_counter] == B)
		{
			P2OUT &= ~BLUE; //turn on blue led
		}
		note_counter++; //move to the next note in the pattern
	}
	else if (note_counter == array_max && delay == 0)
	{
		//CCTL0 &= ~CCIE;//stop playing if pattern is finished
		//turn off all leds************************
		P2DIR &= ~COLOR_MASK;
		P2OUT &= ~COLOR_MASK;
		P2REN &= ~COLOR_MASK;
		note_counter=0;
		pattern_guesses = 0;
		delay = 100;
		state='C';
	}
}

inline void button_compare(){
	switch (current_button_state) {
		case red:
			if (simon_pattern[pattern_guesses] != R){
				pattern_guesses = 0;
				state = 'X';
			} else {
				CCR0=note_synthesizer(R);
				pattern_guesses++;
			}
			break;

		case yellow:
			if (simon_pattern[pattern_guesses] != Y){
				pattern_guesses = 0;
				state = 'X';
			} else {
				CCR0=note_synthesizer(Y);
				pattern_guesses++;
			}
			break;
		case green:
			if (simon_pattern[pattern_guesses] != G){
				pattern_guesses = 0;
				state = 'X';
			} else {
				CCR0=note_synthesizer(G);
				pattern_guesses++;
			}
			break;
		case blue:
			if (simon_pattern[pattern_guesses] != B){
				pattern_guesses = 0;
				state = 'X';
			} else {
				CCR0=note_synthesizer(B);
				pattern_guesses++;
			}
			break;
		default:
			CCR0=0;
			break;
	}
}

interrupt void WDT_interval_handler()
{
	unsigned char p;
	p = (P1IN & BUTTON1);
	unsigned char d;
	d = (P1IN & BUTTON2);
	switch(state)
	{
	case 'A'://add random value to array
		P1OUT &= ~(MODE1 + MODE2 + MODE3);
		CCR0 = 0;
		new_char=simon_pattern[array_max];
		note_counter = 0;
		pattern_guesses = 0;

		if (last_button && (p == 0)) //handle the button press
		{
			P1OUT ^= MODE1; //toggle the green LED at every start of the game
			state='P';
		}
		last_button = p;


		if (last_button2 && (d == 0)) //handle the button press
		{
			P1OUT ^= MODE2; //toggle the green LED at every start of the game
			P2OUT |= DIO1;
			state='D';
		}
		last_button2 = d;
		//if ((P2IN & DIO0 != 0) && (P2OUT & DIO1) !=0) state = 'D'; //multiplayer
		break;
	case 'P':
		array_max++;
		state='B';
		break;
	case 'B'://playback pattern
		pattern_playback();
		break;
	case 'C'://poll for button presses
		P1OUT |= MODE3;
		CCR0 = 0;
		current_button_state = P2IN & 0x000F;

		if((current_button_state != previous_button_state) && (pattern_guesses <= array_max))
		{
			button_compare();
		}

		else if (pattern_guesses == array_max)
		{
			P1OUT &= ~MODE3;
			pattern_guesses=0;
			if (!mp) state='P';
			else state = 'Q';
		}
		previous_button_state = current_button_state;
		break;
	case 'D':
		array_max++;
		mp = 1;
		state = 'M';
		break;
	case 'M':
		//we receive first
		if (mp == 1 && last_button2 && (d == 0))
		{
				state = 'A'; mp = 0;
		}
		last_button2 = d;
		break;
	case 'Q':
		current_button_state = P2IN & 0x000F;
		if(current_button_state != previous_button_state && previous_button_state == 0x000F)  // if a negedge trigger from no buttons pressed
		{
			switch (current_button_state)
			{
				case red:
					UC0IE &= ~UCA0TXIE;
					send = R;
					UC0IE |= UCA0TXIE;
					state = 'D';
					break;
				case yellow:
					UC0IE &= ~UCA0TXIE;
					send = Y;
					UC0IE |= UCA0TXIE;
					state = 'D';
					break;
				case green:
					UC0IE &= ~UCA0TXIE;
					send = G;
					UC0IE |= UCA0TXIE;
					state = 'D';
					break;
				case blue:
					UC0IE &= ~UCA0TXIE;
					send = B;
					UC0IE |= UCA0TXIE;
					state = 'D';
					break;
				default:
					break;
			}
		}
		previous_button_state = current_button_state;
		break;
	case 'X'://end game state
		if (receive != 'L' && mp == 1)
		{
			UC0IE &= ~UCA0TXIE;
			send = 'L';
			UC0IE |= UCA0TXIE;
		}
		CCR0 = 400;
		array_max = 0;
		mp = 0;
		P1OUT &= ~(MODE1 + MODE2 + MODE3);
		state = 'Y';
		break;
	case 'Y':
		if (++CCR0 == 500) state = 'Z';
		if (last_button && (p == 0)) //handle the button press
		{
			P1OUT ^= MODE1; //toggle the green LED at every start of the game
			state='A';
		}
		last_button = p;
		break;
	case 'Z':
		if (--CCR0 == 400) state = 'Y';
		if (last_button && (p == 0)) //handle the button press
		{
			P1OUT ^= MODE1; //toggle the green LED at every start of the game
			state='A';
		}
		last_button = p;
		break;
	}
}
ISR_VECTOR(WDT_interval_handler, ".int10")

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void) //Transmitter Interrupt
{
	UCA0TXBUF = send;
	UC0IE &= ~UCA0TXIE;
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
	receive=UCA0RXBUF;
	IFG2 &= ~UCA0RXIFG;
	if (receive >= 0 && receive <= 3)
	{
		simon_pattern[array_max - 1] = receive;
		state = 'B';
	}
	if (receive == 'L' && mp == 1)
		state = 'X';
}

// Timer A0 interrupt service routine
void interrupt Timer_A()
{
	P1OUT ^= SPEAKER;  // Toggle P1.0
}
ISR_VECTOR(Timer_A,".int09") // declare interrupt vector