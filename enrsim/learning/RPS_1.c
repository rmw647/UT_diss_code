#include <stdio.h>

/*use to test FP algorithm with rock, papers, scissors game*/

#define MAXDSTRATS 3
#define MAXASTRATS 3
#define IT	1000

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

vlow = -10;
vup = 10;

int it_num;
	
/*randomly pick first attacker move*/ 
currentcol = 0;

/*populate U vector with payoffs*/
for (i=0;i<MAXDSTRATS;i++)
	U[i] = payoff[i][currentcol];
	
for (i=0;i<MAXDSTRATS;i++)
	printf("%.1f\n",U[i]);
	
/*update y strategy vector*/
y[currentcol] = y[currentcol]+1;

/*initialize V*/
for (j=0;j<MAXASTRATS;j++)
	V[j] = 0;
	

/****begin repeated fictitious play algorithm****/

for (it_num=1;it_num<=IT;it_num++)	{

	Umax = U[0];
	currentrow = 0;
	
	/*find biggest value in U and record row*/
	for (i=1;i<MAXDSTRATS;i++)
		if (U[i] > Umax)	{
			Umax = U[i];
			currentrow = i;
		}
				
	printf("Umax: %.1f\n",Umax);	
	printf("currentrow: %d\n",currentrow);
		
	/*assign vup to smaller of two values-- current vup or Umax/it_num*/
	if (Umax/it_num < vup)
		vup = Umax/it_num;
		
	printf("vup: %.1f\n",vup);
		
	/*update V vector*/
	for (j=0;j<MAXASTRATS;j++)
		V[j] = V[j] + payoff[currentrow][j];
		
	for (j=0;j<MAXASTRATS;j++)
		printf("V[%d]:%.1f\n",j,V[j]);

	/*update defender strategy history*/
	x[currentrow] = x[currentrow]+1;
	
	Vmin = V[0];
	currentcol = 0;
	
	/*find smallest value in V and record column*/
	for (j=1;j<MAXASTRATS;j++)
		if (V[j] < Vmin)	{
			Vmin = V[j];
			currentcol = j;
		}
			
	printf("Vmin: %.1f\n",Vmin);
	printf("currentcol: %d\n",currentcol);
	
	/*assign vlow to bigger of two values-- current vlow or Vmin/it_num*/
	if (Vmin/it_num > vlow)
		vlow = Vmin/it_num;

	printf("vlow:%.1f\n",vlow);
		
	/*update U*/
	for (i=0;i<MAXDSTRATS;i++)
		U[i] = U[i]+payoff[i][currentcol];
		
	for (i=0;i<MAXDSTRATS;i++)
		printf("U[%d]: %.1f\n",i,U[i]);
	
	/*update attacker strategy vector*/
	y[currentcol] = y[currentcol]+1;
	
	
	xplay[it_num-1] = currentrow;
	yplay[it_num-1] = currentcol;
	
}

printf("\n\n");
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
	printf("y[%d]=%.3f\n",j,(float)y[j]/(IT+1));
printf("\n");

for (j=0;j<MAXASTRATS;j++)
	printf("y[%d]=%d\n",j,y[j]);
printf("\n");




}