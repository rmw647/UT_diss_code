#include <stdio.h>

/*use to test FP algorithm with rock, papers, scissors game*/

#define MAXDSTRATS 3
#define MAXASTRATS 3
#define IT	100000

int main(void)	{

float payoff[3][3] = { {0.5,1,0},{1,0.5,0},{1,-1,0.5} };

int i,j;

float U[MAXDSTRATS];
float V[MAXASTRATS];

int x[MAXDSTRATS];
int y[MAXASTRATS];
int xplay[IT];
int yplay[IT];

for (i=0;i<MAXDSTRATS;i++)
	x[i] = 0;
	
for (j=0;j<MAXASTRATS;j++)
	y[j] = 0;

int currentrow, currentcol;

float Vmin,Umax;
float vlow,vup;

int it_num;

for (i=0;i<MAXDSTRATS;i++)
	U[i] = 0;
	
for (j=0;j<MAXASTRATS;j++)
	V[j] = 0;
 
currentrow = 0;

/*update defender history*/
x[currentrow] = x[currentrow]+1;
	
/*copy row into V vector*/
for (j=0;j<MAXASTRATS;j++)
	V[j] = payoff[currentrow][j];
	
/*for (j=0;j<MAXASTRATS;j++)
	printf("V[%d]:%.2f\n",j,V[j]); */
	
vlow = 0;	/*vlow holds lower bound*/
Vmin = 100;	/*initialize Vmin to arbitrarily high number*/

/*assign vlow to smallest value in row*/
for (j=0;j<MAXASTRATS;j++)
	if (V[j] < Vmin)	{
		Vmin = V[j];
		currentcol = j;
	}
	
vlow = Vmin;

/*printf("vlow:%.2f\n",vlow);
printf("Vmin:%.2f\n",Vmin);
printf("col:%d\n",currentcol); 
*/

/*initialize vup-- upper bound and 'currentrow'*/

/*update attacker history*/
y[currentcol] = y[currentcol]+1;

/*copy column into U vector*/
for (i=0;i<MAXDSTRATS;i++)
	U[i] = payoff[i][currentcol];	

/*for (i=0;i<200;i=i+2)
	printf("U[%d]:%.2f  cost:%d\n",i,U[i],calcCost(defstrats[i])); */
	
vup = 0;	/*vup holds upper bound*/
Umax = 0;	/*initialize Umax to arbitrarily low number*/

/*make Umax biggest number in columns*/

for (i=0;i<MAXDSTRATS;i++)
	/*if U[i] is bigger than the current Umax*/
	if (U[i] > Umax)	{
			Umax = U[i];
			currentrow = i;
	}

vup = Umax;
		
/*printf("vup:%.2f\n",vup);
printf("row:%d\n",currentrow); 
printf("payoff for x[%d],y[%d]: %.3f\n",currentrow, currentcol,calcDP(defstrats[currentrow],
		attstrats[currentcol])); */
		
xplay[0] = currentrow;
yplay[0] = currentcol;
	

/****begin repeated fictitious play algorithm****/

for (it_num=2;it_num<=IT;it_num++)	{

/*	printf("currentrow at start of it %d: %d\n",it_num,currentrow); */

	/*update defender strategy history*/
	x[currentrow] = x[currentrow]+1;
	
	/*update V vector*/
	for (j=0;j<MAXASTRATS;j++)
		V[j] = V[j] + payoff[currentrow][j];
	
/*	for (j=0;j<MAXASTRATS;j++)
		printf("V[%d]:%.2f\n",j,V[j]);  */
	
	Vmin = 100;
	
	/*attacker looks at defender payoffs in vector V and selects column that minimizes payoff
	-- find payoff and record column*/
	for (j=0;j<MAXASTRATS;j++)
		if (V[j] < Vmin)	{
			Vmin = V[j];
			currentcol = j;
		}
	
/*	printf("Vmin:%.2f col:%d\n",Vmin,currentcol); */
	
	/*establish new lower bound-- pick bigger of two numbers*/
	if ((Vmin/it_num) > vlow)
		vlow = (Vmin/it_num);
		
/*	printf("Vmin/it:%.2f\n",Vmin/it_num); 
	printf("vlow:%.2f\n",vlow);  */
	
			
/*	for (i=0;i<MAXDSTRATS;i++)
		printf("payoff[%d][%d]:%.2f\n",i,currentcol,payoff[i][currentcol]); */
			
	/*update attacker strategy*/
	y[currentcol] = y[currentcol]+1;
	
	/*update attacker payoff vector*/
	for (i=0;i<MAXDSTRATS;i++)
		U[i] = U[i] + payoff[i][currentcol];
		
/*	for (i=0;i<MAXDSTRATS;i++)
		printf("U[%d]:%.2f\n",i,U[i]); */
		
	Umax = 0;
	
	/*defender looks at attacker payoff vector U and pick the row that maximizes her payoff
	-- find payoff and record row*/
	for (i=0;i<MAXDSTRATS;i++)
		if (U[i] > Umax)	{
				Umax = U[i];
				currentrow = i;
			}
	
		
/*	printf("Umax:%.2f row:%d\n",Umax,currentrow);  */

		
	
	/*vup is assigned to smaller of values: Umax/it_num or current vup value*/
	/*upper bound on value moves down*/
	if (Umax/it_num < vup)		
		vup = Umax/it_num;
	
/*	printf("vup:%.2f\n",vup); 
	printf("Umax/it:%.2f\n",Umax/it_num); 	
	
	printf("iteration %d complete\n",it_num);   */
	
	xplay[it_num-1] = currentrow;
	yplay[it_num-1] = currentcol;
	
}

/*print results*/
printf("vlow:%.3f\n",vlow);
printf("vup:%.3f\n",vup);

for (i=0;i<MAXDSTRATS;i++)
	printf("x[%d]=%.3f\n",i,(float)x[i]/(IT));
printf("\n");

for (i=0;i<MAXDSTRATS;i++)
	printf("x[%d]=%d\n",i,x[i]);
printf("\n");


for (j=0;j<MAXASTRATS;j++)
	printf("y[%d]=%.3f\n",j,(float)y[j]/(IT));
printf("\n");

for (j=0;j<MAXASTRATS;j++)
	printf("y[%d]=%d\n",j,y[j]);
printf("\n");




}