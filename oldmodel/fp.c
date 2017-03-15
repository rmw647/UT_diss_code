#include <stdio.h>
#include <math.h>

#define BUDGET 60
#define PB 0.01
#define MAXDAYS 30

static double mass = 1000;
static double mu = 75;
static 	double sigma = 2000;
int affstrat[28];					/*holds 1 for strategies under budget; 0 for other strategies*/
double A[28][30];
int count;							/*number of def. strategies within budget*/


void checkBudget(int);
double calcpayoff(int, int);
void print2file(int,int,double[][MAXDAYS]);
void play(int, int, double[][MAXDAYS]);


main(){
	int i,j,m;
	
	for (i=1;i<=28;i++){
		for (j=1;j<=MAXDAYS;j++)
			A[i-1][j-1] = calcpayoff(i,j);
	}
	print2file(28,MAXDAYS,A);
	checkBudget(BUDGET);
	
	double strat[count][MAXDAYS];			/*holds payoffs for affordable defender strategies*/
	
	for (i = 0;i<count; i++)
		for (j=0;j<MAXDAYS;j++)
			strat[i][j] = 0;


	m = 0;						/*indexes through rows in new array*/
	for (i=0;i<28;i++)	{		/*checks affstrat to see if def. strat is under budget*/
		if (affstrat[i] == 1) {
			for (j=0; j<MAXDAYS; j++) {   
				strat[m][j] = A[i][j];
			}
			m=m+1;
		} 	
	}	
	
	play(count,MAXDAYS,strat);
	
	print2file(count,MAXDAYS,strat);	
				
	return 0;
}
		
/*checks budget and modifies affstrat to have a 1 for affordable defender strats and 0 otherwise*/	
void checkBudget(int defbudget){
	int cost[28] = {240, 120, 80, 60, 48, 40, 34, 120, 60, 40, 30, 24, 20, 17,
					  60, 30, 20, 15, 12, 10, 9, 30, 15, 10, 8, 6, 5, 4};
	int i;
	
	for (i = 0; i< 28; i++){
/*	(defbudget >= cost[i]) ?  (affstrat[i] = 1): (affstrat[i] = 0);	*/
		if (defbudget >= cost[i]) {
			affstrat[i] = 1;
			count ++;
		}
		else
			affstrat[i] = 0;
	}

}

/*takes defender and attacker strategy as input and outputs payoff for the pair */
double calcpayoff(int dstrat, int astrat){
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
			
	return cumdet;
}

void print2file(int row,int col,double matrix[][MAXDAYS]){	
	
	FILE *file;
	int i,j;
	/*prints output to file*/
	file = fopen("file.txt","a+"); /* apend file (add text to a file or create a file if it does not exist.*/ 

	fprintf(file,"\n");
	
	for(i=0;i<row;++i){
		for(j=0;j<col;++j)			/*inputs value*/
			fprintf(file,"%0.4f ",matrix[i][j]);
		fprintf(file,"\n");
	}
	
	fclose(file); /*done!*/ 
}

void play(int row, int col,double matrix[][MAXDAYS]){
/*lines 144-171 initialize strategy matrices; while loop iteratively swaps strategy choices to 
converge on v */
	
	int x[row], y[col];
	int it = 0;
	int i, j, currentrow, currentcol;
	double vup, vlow, v_gap;
	
	for (i=0; i<row; i++)
		x[i] = 0;
		
	for (j = 0; j<col; j++)
		y[j] = 0;
		
	currentrow = 0;			/*instead of randomly picking first defender strategy, pick most expensive */
	currentcol = 0;
	x[currentrow] = x[currentrow] + 1;	/*update x array to record defender history */
	
	vlow = matrix[currentrow][currentcol];			/*initialize vlow*/

	for (j=1;j<col;++j)	{	/*scan through selected row and find lowest value = vlow; remember position in array */
		if ((matrix[currentrow][j]) < vlow){
			vlow = matrix[currentrow][j];
			currentcol = j;
		}
	}
	
	y[currentcol] = y[currentcol] + 1;	/*update y array to record attacker history*/
	
	v_gap = 100;			/*initialize v_gap to arbitrary high number */
	vup = 10;				/*initialize v_gap to arbitrary high number */
	
	while (v_gap>0.0001) {
		double U[col], V[row]; 	/* U holds the expected payoffs for the attacker. U[j] corresponds
								to the payoff for playing the jth column against the defender's
								mixed strategy, x */
		int sumy, sumx;
		double Vmax, Umin;

			
		sumy = 0;
		for (j=0;j<col;j++)
			sumy = y[j] + sumy;
			
		printf("sumy is: %d\n",sumy);
	
			
		for (i=0; i<row; i++)
			V[i] = 0;
		
		for (j = 0; j<col; j++)
			U[j] = 0;
	
		/* defender looking at attacker; defender observes y, attacker strat history; then calculates
		expected payoff for each row, based on attacker mixed strategy */
		
		for (i=0;i<row;i++)
			for (j=0;j<col;j++)
				V[i] = (y[j]/sumy) * matrix[i][j] + V[i];	
		
				
		for(j=0;j<col;j++)
			printf("%d ",y[j]);
		
		printf("\n");
		
/*		for (i=0;i<row;i++)
			printf("%0.4f ",V[i]);	*/
			
		printf("\n");
		
		Vmax = V[0];
		for (i=1;i<row;++i)	{	/*scan through V array and find max value; record row that has max payoff value */
			if (V[i] > Vmax ){
				Vmax = V[i];
				currentrow = i;
			}
		}
		(Vmax < vup) ? (vup = Vmax): (vup = vup);
		x[currentrow] = x[currentrow] + 1;
		
		for(i=0;i<row;i++)
			printf("%d ",x[i]);
			
		printf("\n");
			
		sumx = 0;
		for (i=0;i<row;i++)		/*sumx tracks total # of def. strats that have been played*/
			sumx = x[i] + sumx;	
			
		printf("sumx is: %d\n",sumx);
	
		for (j=0;j<col;j++)
			for (i=0;i<row;i++)
				U[j] = (x[i]/sumx) * matrix[i][j] + U[j];	
				
		Umin = U[0];
		for (j=0;j<col;j++) {
			if (U[j] < Umin) {
				Umin = U[j];
				currentcol = j;
			}
		}
		(Umin > vlow) ? (vlow = Umin) : (vlow = vlow);
		
		y[currentcol] = y[currentcol] + 1;
		
		v_gap = vup - vlow;
		it = it + 1;
				
/*		for (j=0;j<col;j++)
			printf("%0.4f ",U[j]);	*/
			
		printf("\n");
		
		printf("%0.4f  %d\n",vup, currentrow);		
		printf("%0.4f  %d\n",vlow, currentcol);	
		printf("v_gap: %f, iterations: %d\n",v_gap,it);
		printf("vup: %f, vlow: %f\n",vup, vlow);
		
	
	}	
	
}
	
	
		
	
	
	
	
	
	