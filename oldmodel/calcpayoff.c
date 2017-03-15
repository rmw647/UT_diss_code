#include <stdio.h>
#include <math.h>

#define PB 0.01
#define MAXDAYS 30

static double mass = 1000;
static double mu = 75;
static 	double sigma = 2000;

double main(int dstrat, int astrat)
{	
	double dailydet[30],cumdet;					/* matrices that hold daily and scenario det probs*/
	int i,j;									/*counters*/
	int freq, a;
	
	if (dstrat%7 == 1)
			freq = 1;
		else if (dstrat%7 == 2)
			freq = 2;
		else if(dstrat%7 == 3)
			freq = 3;
		else if(dstrat%7 == 4)
			freq = 4;
		else if(dstrat%7 == 5)
			freq = 5;
		else if(dstrat%7 == 6)
			freq = 6;
		else
			freq = 7;
		
		if (dstrat <= 7)
			a = 19;
		else if(dstrat <= 14)
			a = 6;
		else if(dstrat <= 21)
			a = 1;
		else
			a = 0;
	
	for (i=0; i<MAXDAYS; ++i)						/*loops over 30 days in scenario (max) */
		dailydet[i] = 0;
			
	dailydet[0] = 0.5*(1+erf(((mass/astrat)-mu)/sqrt(sigma))); /*calculate Pdi*/
	
	int index = freq;
	while (index < astrat){	
		dailydet[index] = 1-((1+a*(1-dailydet[index-freq]))/(a+1));		/*HRA calculation*/
		index = index + freq;
	}
	for (i=0;i< astrat;++i)								/*combine daily prob with Pb*/
		dailydet[i] = 1 - (1-PB) * (1-dailydet[i]);
		
	cumdet = 0;
	
	for (i=0;i<=29;++i)								/*calculate cum det prob for strat pair*/
		cumdet = 1 - (1-dailydet[i])*(1-cumdet);
		
	printf("%0.4f\n",cumdet);
			
	return cumdet;
}
