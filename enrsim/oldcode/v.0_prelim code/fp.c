#include <stdio.h>
#include <math.h>
#include <time.h>

#define IT 10000		/*number of iterations*/

clock_t start, end;
double cpu_time_used;

int main(void)	{
	
start = clock();

int m = 4;
int n = 4;

/*example payoff matrix*/
int payoff[4][4] = { {1,2,3,4},{4,2,2,1},{0,1,0,1},{6,2,1,4} };
	
/*strategy vectors*/
int x[4];
int y[4];

int i,j;
int it_num;	/*use to loop through iterations*/
int vlow;	/*holds current lower bound*/
int vup;	/*holds current upper bound*/
int Umax;	/*holds max element of U*/
int Vmin;	/*holds min element of V*/
int currentcol;	/*holds current column number*/
int currentrow;	/*holds current row number*/

vlow = 100;

/*initialize vectors to zeros*/
for (i=0;i<m;i++)
	x[i] = 0;
	
for (i=0;i<n;i++)
	y[i]=0;

/*holds columns*/
int U[4];

/*holds rows*/
int V[4];


for (i=0;i<m;i++)
	U[i] = 0;

int temp = 0;

/*initialize vup-- upper bound-- to minmax value and record column in 'currentcol'*/
for (j=0;j<n;j++)	{
	for (i=0;i<m;i++)	
		if (payoff[i][j] > temp)	{
			temp = payoff[i][j];
		}
	V[j] = temp;
	temp = 0;	
}	

/*for (j=0;j<n;j++)
	printf("V[%d]:%d\n",j,V[j]); */

/*vup holds upper bound*/
vup = V[0];	

for (j=1;j<n;j++)
	if (V[j] < vup)	{
		vup = V[j];
		currentcol = j;
	}
		
/*printf("vup:%d\n",vup);
printf("col:%d\n",currentcol); */

/*initialize vlow-- lower bound-- to maxmin value and record row in 'currentrow'*/

/*initialize temp to some arbitrarily high number*/
temp = 100;
currentrow = 1;

for (i=0;i<m;i++)	{
	for (j=0;j<n;j++)	
		if (payoff[i][j] < temp)	{
			temp = payoff[i][j];
		}
	U[i] = temp;
	temp = 100;	
}	

/*for (i=0;i<m;i++)
	printf("U[%d]:%d\n",i,U[i]); */

/*vlow holds lower bound*/
vlow = 0;	

for (i=0;i<m;i++)
	if (U[i] > vlow)	{
		vlow = U[i];
		currentrow = i;
	}
		
/*printf("vlow:%d\n",vlow);
printf("row:%d\n",currentrow); */
		
/*copy all values from the minmax column into the vector that holds columns
copies the column the attacker plays into the attackers payoff vector*/

for (i=0;i<m;i++)
	U[i] = payoff[i][currentcol];
	
/*for (i=0;i<m;i++)
	printf("U[%d]:%d\n",i,U[i]);	*/
	
/*update attacker's strategy history*/
y[currentcol] = y[currentcol] + 1;

/*clear out V vector-- was being used as temporary storage place*/
for (j=0;j<n;j++)
	V[j] = 0;

/****begin repeated fictitious play algorithm****/

for (it_num=1;it_num<IT;it_num++)	{
	Umax = 0;	/*initialize Umax to arbitrarily low number*/
	Vmin = 100;	/*initialize Vmin to arbitrarily high number*/
	
	/*defender looks at attacker payoff vector U and pick the  row that maximizes her payoff
	-- find payoff and record row*/
	for (i=0;i<m;i++)
		if (U[i] > Umax)	{
			Umax = U[i];
			currentrow = i;
		}
		
/*	printf("Umax:%d row:%d\n",Umax,currentrow); */
	
	/*vup is assigned to smaller of values: Umax/it_num or current vup value*/
	/*upper bound on value moves down*/
	if (vup > Umax/it_num)		
		vup = Umax/it_num;
	
/*	printf("vup:%d\n",vup); */
		
	/*update V vector*/
	for (j=0;j<n;j++)
		V[j] = V[j] + payoff[currentrow][j];
	
/*	for (j=0;j<n;j++)
		printf("V[%d]:%d\n",j,V[j]); */
		
	/*update defender strategy history*/
	x[currentrow] = x[currentrow]+1;
	
	/*attacker looks at defender payoffs in vector V and selects column that minimizes payoff
	-- find payoff and record column*/
	for (j=0;j<n;j++)
		if (V[j] < Vmin)	{
			Vmin = V[j];
			currentcol = j;
		}
	
/*	printf("Vmin:%d col:%d\n",Vmin,currentcol); */
	
	/*establish new lower bound*/
	if (vlow < Vmin/it_num)
		vlow = Vmin/it_num;
		
/*	printf("vlow:%d\n",vlow); */
	
	/*update attacker payoff vector*/
	for (i=0;i<m;i++)
		U[i] = U[i] + payoff[i][currentcol];
		
/*	for (i=0;i<m;i++)
		printf("U[%d]:%d\n",i,U[i]); */
	
	/*update attacker strategy*/
	y[currentcol] = y[currentcol]+1;
	
/*	printf("iteration %d complete\n",it_num); */
	
}

end = clock();

cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

/*print results*/
printf("vlow:%d\n",vlow);
printf("vup:%d\n",vup);
for (i=0;i<m;i++)
	printf("x[%d]=%d ",i,x[i]);
printf("\n");
for (j=0;j<m;j++)
	printf("y[%d]=%d ",j,y[j]);
printf("\n");

for (i=0;i<m;i++)
	printf("x[%d]=%.2f ",i,(float)x[i]/(IT-1));
printf("\n");

for (j=0;j<n;j++)
	printf("y[%d]=%.2f ",j,(float)y[j]/IT);
printf("\n");

printf("time:%.4f\n",cpu_time_used);


}