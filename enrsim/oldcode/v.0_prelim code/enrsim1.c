#include <stdio.h>
#include <math.h>

float calcDP(float *dstrat[9], float *astrat[6]);	/*function called by fp to calc DP--
arguments are two arrays of pointers to (array of) floats*/

int main(void)	{

/*define defender and attacker strategies to be passed to function--
this will usually not be explicitly done; the game will call the calcDP function and pass
strategy information as an array of pointers as an argument to the call*/

/*define defender strategies*/
/* x1- physical inventory; x2- mass balance ver.; x3- passive seals; 4- NDA; x5- DA; x6- video logged;
x7- video transm.; x8- active seals; x9- CEMo*/

	float *xstrat[9];			/*declare as array of pointers so cols can vary */
	float x1[3] = {1,0,6};   	/*active,disrete,a = 6*/
	float x2[3] = {1,0,0.01};	/*active,discrete,FAP = 0.01*/
	float x3[3] = {1,0,1};		/*active,discrete,fraction tagged = 1*/
	float x4[3] = {0,0};		/*inactive, discrete*/
	float x5[3] = {0,0};		/*inactive,discrete*/
	float x6[3] = {1,0,6};		/*active, discrete,a = 6*/
	float x7[3] = {0,1};		/*inactive, continuous*/
	float x8[3] = {0,1};		/*inactiv, continuous*/
	float x9[4] = {0,1,0.01,300};	/*inactive,continuous,FAP = 0.01, count time = 300 s*/
	xstrat[0] = &x1[0];
	xstrat[1] = &x2[0];
	xstrat[2] = &x3[0];
	xstrat[3] = &x4[0];
	xstrat[4] = &x5[0];
	xstrat[5] = &x6[0];
	xstrat[6] = &x7[0];
	xstrat[7] = &x8[0];
	xstrat[8] = &x9[0];
	
/* define attacker strategy */
/* y1- cylinder from storage; y2- some material from cylinder; y3- some material from cascade;
y4- cascade repiping; y5- cascade recycle; y6- undeclared feed */

	float *ystrat[6];
	float y1[7] = {1,0,16,16,1,1,1};	/*active, discrete (1-cont),start day 16,end day 16, freq 1,1 cylinder, 
	from prod storage (0-feed) */
	float y2[1] = {0};
	float y3[1] = {0};
	float y4[1] = {0};
	float y5[1] = {0};
	float y6[1] = {0};
	ystrat[0] = &y1[0];
	ystrat[1] = &y2[0];
	ystrat[2] = &y3[0];
	ystrat[3] = &y4[0];
	ystrat[4] = &y5[0];
	ystrat[5] = &y6[0];
	
		

	printf("so far, so good!\n");
}

float calcDP(float *dstrat[9], float *astrat[6]){

	/*effective defender strategies-- holds 1 if xstrat is effective against ystrat and 0 otherwise*/
	int effstrats[9][6] = { {1,0,0,0,0,0},{0,1,0,1,1,0},{0,1,1,1,1,0},{0,1,0,1,1,0},{0,1,0,1,1,0},
						  {1,1,0,0,0,0},{1,1,0,0,0,0},{0,1,1,1,0,0},{0,0,0,1,1,0} };
	
	int t,i,actastrat;				/*t indexes time; i indexes in loops; active attacker strat*/
	int tstart, tend;				/*beginning and end day of diversion*/
	int m;							/*schedule array dimensions-- detection days*/
	int k;							/*indexes safeguards*/
	int inspday = 15;				/*regular inspection day-- 15th of each month*/
	
	/*set actastrat equal to the number of the active attacker strategy*/
	/*defender and attacker strategies are referenced BY THEIR INDICES (strategy number -1)*/
	
	for (i=0;i<5;i++)
		if (astrat[i][0] == 1)
			actastrat = i;
	
	/*initialize schedule arrays-- run 30 days past end of diversion*/	
	tstart = astrat[actastrat][2];
	tend = astrat[actastrat][3];	
	
	if (astrat[actastrat][1] == 0)		/*if its a discrete attacker strategy*/
		m = 30+tstart;
	else								/*continuous attacker strategy*/
		m = tend+30;

	int aschedule[m], dschedule[9][m];

	/*populate schedule arrays*/
	
	for(t=0;t<m;t++)
		aschedule[t] = 0;
		
	if (astrat[actastrat][1] == 0)	/*if attacker strategy is disrete*/
		aschedule[tstart-1]=1;		/*attacker activity on first day, nothing on rest of days*/ 
	else
		for(t=tstart-1;t<tend;t=t+astrat[actastrat][4])	/*activity of first day and repeating every f days (f=freq) */
			aschedule[t] = 1;
	
	for(k=0;k<9;k++)
		for(t=0;t<m;t++)
			dschedule[k][t] = 0;	
		
	for(k=0;k<6;k++)	{		/*safeguards that occur during regular inspections-- every 30 days*/
		if(dstrat[k][0] == 1)	{
			dschedule[k][inspday-1]=1;
			for(t=inspday-1;t<m;t=t+30)
				dschedule[k][t] = 1;
		}	
	}		
	for(k=6;k<9;k++)
		if(dstrat[k][0] == 1)
			for (t=0;t<m;t++)
				dschedule[k][t] = 1;
	
	for(k=0;k<9;k++){		
		printf("%d: ",k);
		for (t=0;t<m;t++)
			printf("%d  ",dschedule[k][t]);
		printf("\n");
	}	
	

}


