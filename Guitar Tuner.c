/*
 * Guitar_Tuner.c
 *
 * Created: 03/07/2012 20:23:54
 *  Author: Tom
 */ 

#include <avr\io.h>
#include <avr\interrupt.h>




#define F_CPU				8000000			// CPU clock frequency
#define PRESCALER			64					// CPU prescaler value



#define BASE_FREQUENCY		(F_CPU/PRESCALER)	// counter frequency	

#define TUNING_FORK_A		440.0				// "base" A
#define NOTE_DIFF			1.05946309436		// the 12'th root of 2


#define E_STRING	164.81						// top string (1)
#define A_STRING	220.00
#define D_STRING	293.66
#define G_STRING	391.99
#define B_STRING	493.88				
#define EH_STRING	659.26						// bottom string (6)


// The guitar note span
//  # # #  # #  # # #  # #
//EF G A BC D EF G A BC D E
//1    2    3    4 * 5    6
//

unsigned int Center_Count[] =
{
	BASE_FREQUENCY/EH_STRING,			// High E
	BASE_FREQUENCY/B_STRING,			// B
	BASE_FREQUENCY/G_STRING,			// G
	BASE_FREQUENCY/D_STRING,			// D
	BASE_FREQUENCY/A_STRING,			// A
	BASE_FREQUENCY/E_STRING,			// Low E
};

unsigned int Transition_Count[] =
{
	BASE_FREQUENCY/(B_STRING+(EH_STRING-B_STRING)/2),	// E to B
	BASE_FREQUENCY/(G_STRING+(B_STRING-G_STRING)/2),		// B to G
	BASE_FREQUENCY/(D_STRING+(G_STRING-D_STRING)/2),		// G to D
	BASE_FREQUENCY/(A_STRING+(D_STRING-A_STRING)/2),		// D to A
	BASE_FREQUENCY/(E_STRING+(A_STRING-E_STRING)/2),		// A to E
};




volatile unsigned char count_hi;	// overflow accumulator

SIGNAL(SIG_OVERFLOW0)	
{
	count_hi++;
						// increment overflow count
}


void Initialise (void)
{
	//PORTD is all output for debugging
	DDRD = 0xFF;
	//PORTB is all input
	DDRB = 0x00;
	//Prescaler of /64
	TCCR0B = TCCR0B | 0x03;
	//Enable overflow interrupts
	TIMSK0 = _BV(TOIE0);
	sei();
}

int SampleLoop(int count, int count_hi)
{
		unsigned int i;
			for (i=0;i<32;i++)
			{

			while ((PINB | 0xFE) == 0xFF)		// ignore hi->lo edge transitions
				if (count_hi > 60)			// skip if no edge is seen within
					break;					// a reasonable time

			while ((PINB | 0xFE) == 0xFE)	// wait for lo->hi edge
				if (count_hi > 60)			// skip if no edge is seen within
					break;					// a reasonable time

			count += (count_hi << 8) + TCNT0; // get counter value
			TCNT0 = 0;					// clear counter		 

			if (count_hi > 60)				// skip if counter has accumulated a
				break;						// too high value

			count_hi = 0;					// clear hi count
			}
			
			return count;
}

void DetectRange(int count, int count_hi)
{
	int i = 0;
	PORTD = 0x00;
		if (count_hi <= 80)					// if count is reasonable
		{

			count = count >> 5;				// average accumulated count by dividing with 32
	
			// now we have to find the correct string
						
			// go through transition frequencies
			
			for (i=0;i<sizeof(Transition_Count)/sizeof(Transition_Count[0]);i++)
			{
				if (count < Transition_Count[i])	// stop if lower than this transition count
					break;
			}				
			
			// i now holds the string index
			
			// check the count for a match, allowing
			// 1 extra count "hysteresis" to avoid too 
			// much LED flickering
			
			if (count-1 <= Center_Count[i])			// if count <= this string count
				PORTD = 0x80;						// light "Too High" LED
	
			if (count+1 >= Center_Count[i])			// if count >= this string count
				PORTD = 0x40;						// light "Too Low" LED
		}
}

int main(void) 
{
	unsigned int count;
	Initialise();
	while (1)								// loop forever
	{
		count = 0;							// clear sample count
		TCNT0 = 0;							// clear counter
		 
		count_hi = 0;						// clear hi count

		//Perform sampling
		count = SampleLoop(count, count_hi);
		//Detect if the sample is in range, and react appropriately
		DetectRange(count, count_hi);		
			
	}
}



 
