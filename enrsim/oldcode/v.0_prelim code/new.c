#include <stdio.h>
#include <math.h>
#include <string.h>

/*use to test new way to generate and store SGs/strategies*/

/*general structure that stores parameters associated with each safeguard*/
struct safeguards {
	char name[11];	/*name of SG*/
	int active;		/*did the defender select?*/
	int freq;	/*scheduled inspection frequency*/
	int dep;	/*scheduled inspection dependency*/
} ;

/*general structure that stores strategies*/
struct dstrategy	{
	int uniqueID;
	struct safeguards SG1;
	struct safeguards SG2;
};

int main(void)	{

int freq[2] = {1,7};		/*frequency options*/
int dep[2] = {1,19};		/*dependency options*/
int count = 0;			/*counts in strategy array*/
int index = 1;			/*counts in inventory array*/
int i,j,k;
	
/*array to hold all inventory permutations*/
struct safeguards inventory[5];

/*no inventory option*/
strcpy(inventory[0].name,"inventory");
inventory[0].active = 0;
inventory[0].freq = 0;
inventory[0].dep = 0;

/*rest of inventory options*/
for (j=0;j<=1;j++)
	for (k=0;k<=1;k++)	{
		strcpy(inventory[index].name,"inventory");
		inventory[index].active = 1;
		inventory[index].freq = freq[j];
		inventory[index].dep = dep[k];
		index++;
	}
			
/*array to hold all videolog permutations*/
struct safeguards videolog[5];

/*no videolog option*/
strcpy(videolog[0].name,"videolog");
videolog[0].active = 0;
videolog[0].freq = 0;
videolog[0].dep = 0;

index = 1;

/*rest of videolog options*/
for (j=0;j<=1;j++)
	for (k=0;k<=1;k++)	{
		strcpy(videolog[index].name,"videolog");
		videolog[index].active = 1;
		videolog[index].freq = freq[j];
		videolog[index].dep = dep[k];
		index++;
	}
									
/*array to hold all strategy combinations*/
struct dstrategy defstrats[25];

/*generate all strategy combinations and store in defstrats*/
for (i=0;i<=4;i++)
	for (j=0;j<=4;j++)	{
		defstrats[count].uniqueID = count;
		defstrats[count].SG1 = inventory[i];
		defstrats[count].SG2 = videolog[j];
		count++;
	}
	
/*print all strategy combinations*/
for (i=0;i<25;i++)	
	printf("strategy:%d  SG1:%s active:%d freq:%d dep:%d	SG2:%s active:%d freq:%d dep:%d\n",
			defstrats[i].uniqueID,defstrats[i].SG1.name,defstrats[i].SG1.active,defstrats[i].SG1.freq,
			defstrats[i].SG1.dep,defstrats[i].SG2.name,defstrats[i].SG2.active,defstrats[i].SG2.freq,
			defstrats[i].SG2.dep);
}
