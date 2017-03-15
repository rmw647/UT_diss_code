#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

/*mini model used to demonstrate proof-of-concept*/
/*2-3 SGs and 2-3 attacker options with DPs calculated and FP coupled*/

/**** successfully generates cumulative DPs ****/

/*structure "safeguard" holds tunable parameters for each safeguards that the 
defender selects; structure "sginfo" holds parameters inherent to that SG that cannot be
altered by the defender*/

#define BUDGET		7		/*defender budget*/
#define EMPTY		-1		/*use to initialize payoff matrix-- -1 means DP not yet calculated*/
#define OVERBUDGET	-2		/*use in payoff matrix when defender can't afford row*/	
#define BASECOST	1		/*base cost of inspection*/
#define BASEFREQ	7		/*frequency of $1 SG option*/
#define BASEDEP		1		/*dependency of $1 SG option*/
#define MAXDSTRATS	25
#define MAXASTRATS	36
#define	NUMSG		2		/*number of safeguards*/
#define NUMAO		2		/*number of attacker options*/
#define EXTRADAYS	30		/*number of days past end of diversion simulation runs*/
#define	NUMCASC		60		/*number of cascades at facility*/

#define IT			10000		/*number of iterations for FP algorithm*/

int depmult = 3;	/*used to calculate cost-- lower dependency costs 3x more*/

clock_t start, end;
double cpu_time_used;


/*general structure that stores parameters associated with each safeguard*/
struct safeguards {
	char name[11];	/*name of SG*/
	int active;		/*did the defender select?*/
	int freq;	/*scheduled inspection frequency*/
	int dep;	/*scheduled inspection dependency*/
} ;

/*general structure that stores parameters associate with each attacker options*/
struct aoptions	{
	char name[11];		/*name of attacker option*/
	int active;			/*did the attacker select it*/
	int tend;			/*duration of attack period*/
	int freq;			/*frequency of attack*/
	float nitems;		/*# cylinders (as1/2),# cascades tapped or repiped (as3/4)*/
	int area;			/*area under attack*/
	float deltam;		/*grams/cascade/instance (as3)*/
};

/*general structure that stores defender strategies*/
struct dstrategy	{
	int uniqueID;
	struct safeguards SG[NUMSG];
};

/*general structure that stores attacker strategies*/
struct astrategy	{
	int uniqueID;
	struct aoptions AO;
};

/*general structure of properties of SGs that cannot be altered by defender*/
struct sginfo	{
	char name[11];	/*name of SG*/
	int cost;		/*SG cost*/
	int cont;		/*0 if SG is discrete; 1 if cont-- defender and attacker*/
	int insptype;	/*0 if conducted at scheduled inspections; 1 for random insp.*/
};

/*general structure of properties of aoptions that cannot be altered by attacker*/
struct aoptionsinfo	{
	char name[11];	/*name of attacker options*/
	int cont;		/*is it continuous? 0 if discrete, 1 if continuous*/
};

/*effective def. strategies-- holds 1 if xstrat is effective against ystrat; 0 otherwise*/
int effstrats[NUMSG][NUMAO] = { {1,0},{1,1} };

/*allowable values for defender parameters; game picks from these values*/
int freq[2] = {1,7};		/*frequency options*/
int dep[2] = {1,19};		/*dependency options*/

/*allowable values for attacker parameters; game picks from these values*/
int dur[2] = {7,30};					/*attack duration*/
int attackfreq[2] = {1,7};				/*frequency of attack*/
float ncyl[2] = {1,2};					/*number of cylinders*/
int area[2] = {0,1};					/*0-feed,1-product*/
float deltam[2] = {10,100};				/*grams/item/instance (as3)*/

/* specification of properties of SGs*/
struct sginfo inventoryi = {"inventory",BASECOST,0,0};
struct sginfo videologi = {"videolog",5,0,0};

/*specification of properties of attacker options*/
struct aoptionsinfo cylthefti = {"cyltheft",0};
struct aoptionsinfo matcyli = {"matcyl",1};

/*function called by fp to calc DP-- arguments are defender and attacker strategies 
and frequency and dependency for scheduled and random inspections*/
float calcDP(struct dstrategy, struct astrategy);	


/****************** MAIN *********************/

int main(void)	{


start = clock();

int count = 0;			/*counts in strategy array*/
int index = 1;			/*counts in inventory array*/
int i,j,k;				/*random indices*/

/* variables used for FP */
int it_num;			/*counts iterations*/
int currentrow;		/*holds current row number*/
int currentcol;		/*holds current column number*/
float vup;			/*holds current upper bound*/
float vlow;			/*holds current lower bound*/
float Vmin;			/*holds smallest value in V*/
float Umax;			/*holds largest value in U*/
int x[MAXDSTRATS];	/*hold defender strategy history*/
int y[MAXASTRATS];	/*holds attacker strategy history*/
float U[MAXDSTRATS];	/*holds columns*/
float V[MAXASTRATS];	/*holds rows*/
float payoff[MAXDSTRATS][MAXASTRATS];	/*payoff matrix- populated by simulation calls*/

/*initialize defender and attacker strategy vectors to zeros*/
for (i=0;i<MAXDSTRATS;i++)
	x[i] = 0;
	
for (j=0;j<MAXASTRATS;j++)
	y[i]=0;

/*initialize payoff matrix to zeros*/
for (i=0;i<MAXDSTRATS;i++)
	for (j=0;j<MAXASTRATS;j++)
		payoff[i][j] = EMPTY;

/***** generate all permutations for available SGs *****/

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

/***** generate all defender strategies *****/

/*array to hold all strategy combinations*/
struct dstrategy defstrats[MAXDSTRATS];

/*generate all strategy combinations and store in defstrats*/
for (i=0;i<=4;i++)
	for (j=0;j<=4;j++)	{
		defstrats[count].uniqueID = count;
		defstrats[count].SG[0] = inventory[i];
		defstrats[count].SG[1] = videolog[j];
		count++;
	}

/*print all strategy combinations to test*/
for (i=0;i<MAXDSTRATS;i++)	
	printf("strategy:%d  SG1:%s active:%d freq:%d dep:%d	SG2:%s active:%d freq:%d dep:%d\n",
			defstrats[i].uniqueID,defstrats[i].SG[0].name,defstrats[i].SG[0].active,defstrats[i].SG[0].freq,
			defstrats[i].SG[0].dep,defstrats[i].SG[1].name,defstrats[i].SG[1].active,defstrats[i].SG[1].freq,
			defstrats[i].SG[1].dep);

printf("\n\n");
/***** generate all permutations for available AOs *****/

/*array to hold all cyltheft permutations*/
struct aoptions cyltheft[5];

/*no cyltheft options*/
strcpy(cyltheft[0].name,"cyltheft");
cyltheft[0].active = 0;
cyltheft[0].tend = -1;
cyltheft[0].freq = -1;
cyltheft[0].nitems = 0;
cyltheft[0].area = 0;
cyltheft[0].deltam = -1;

index = 1;  /*index through cyltheft array*/
/*rest of cyltheft options*/
for (j=0;j<=1;j++)
	for (k=0;k<=1;k++)	{
		strcpy(cyltheft[index].name,"cyltheft");
		cyltheft[index].active = 1;
		cyltheft[index].tend = -1;
		cyltheft[index].freq = -1;
		cyltheft[index].nitems = ncyl[j];
		cyltheft[index].area = area[k];
		cyltheft[index].deltam = -1;
		index++;
	}
	
/*array to hold all matcyl permutations*/
struct aoptions matcyl[33];

/*no matcyl option*/
strcpy(matcyl[0].name,"matcyl");
matcyl[0].active = 0;
matcyl[0].tend = 0;
matcyl[0].freq = 0;
matcyl[0].nitems = 0;
matcyl[0].area = 0;
matcyl[0].deltam = 0;

index = 1;
int m,n;	/*extra indices*/

/*rest of cyltheft options*/
for (i=0;i<=1;i++)
	for (j=0;j<=1;j++)
		for (k=0;k<=1;k++)
			for (m=0;m<=1;m++)
				for (n=0;n<=1;n++)	{
					strcpy(matcyl[index].name,"matcyl");
					matcyl[index].active = 1;
					matcyl[index].tend = dur[i];
					matcyl[index].freq = attackfreq[j];
					matcyl[index].nitems = ncyl[k];
					matcyl[index].area = area[m];
					matcyl[index].deltam = deltam[n];
					index++;
				}
/*array to hold all strategy combinations*/
struct astrategy attstrats[MAXASTRATS];

count = 0;		/*index through attstrats*/

/*generate all strategy combinations with cyltheft and store in attstrats*/
/*indexing starts at 1 to skip "no cyltheft" options bc these are strategies where
cyltheft is active; use matcyl[0] because matcyl is inactive with cyltheft is active*/

for (i=1;i<=4;i++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = cyltheft[i];
	count++;
	}

for (j=1;j<=32;j++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = matcyl[j];
	count++;
	}

/*print all strategy combinations to test*/
for (i=0;i<MAXASTRATS;i++)	
	printf("strategy:%d AO:%s dur:%d freq:%d ncyl:%.0f area:%d deltam:%.0f\n\n",
			attstrats[i].uniqueID,attstrats[i].AO.name,attstrats[i].AO.tend,
			attstrats[i].AO.freq,attstrats[i].AO.nitems,attstrats[i].AO.area,attstrats[i].AO.deltam);

printf("so far, so good!\n\n");

/********** FP ALGORITHM ***********/

for (i=0;i<m;i++)
	U[i] = 0;

int temp = 0;
int cost;


/*find affordable first row to for defender to play first--
start at 1 because 0 is the zero action option*/
/*if strategy is over budget, set values in payoff array equal to 2*/
for (i=1;i<MAXDSTRATS;i++)	{
	cost = calcCost(defstrats[i]);
	if (cost <= BUDGET)	{
		currentrow = i;
		break;
	}
	else 
		for (j=0;j<MAXASTRATS;j++)
			payoff[i][j] = OVERBUDGET;
} 

printf("cost:%d\n",cost);
printf("currentrow:%d\n",currentrow);

/*call calcDP to calculate payoffs for row and store in payoff matrix*/
for (j=0;j<MAXASTRATS;j++)
	payoff[currentrow][j] = calcDP(defstrats[currentrow],attstrats[j]);
	
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
printf("col:%d\n",currentcol); */

/*initialize vup-- upper bound and 'currentrow'*/

/*call calcDP to calculate payoffs for column and store in payoff matrix*/
for (i=0;i<MAXDSTRATS;i++)
	payoff[i][currentcol] = calcDP(defstrats[i],attstrats[currentcol]);

/*update attacker history*/
y[currentcol] = y[currentcol]+1;

/*copy column into U vector*/
for (i=0;i<MAXDSTRATS;i++)
	U[i] = payoff[i][currentcol];	

/*for (i=0;i<MAXDSTRATS;i++)
	printf("U[%d]:%.2f\n",i,U[i]); */

vup = 0;	/*vup holds upper bound*/
Umax = 0;	/*initialize Umax to arbitrarily low number*/

/*make Umax biggest number in columns*/

for (i=0;i<MAXDSTRATS;i++)
	/*if U[i] is bigger than the current Umax*/
	if (U[i] > Umax)	{
		/*and the defender can afford that row*/
		if (calcCost(defstrats[i]) <= BUDGET)	{
			Umax = U[i];
			currentrow = i;
		}
		else
			for (j=0;j<MAXASTRATS;j++)
				payoff[i][j] = OVERBUDGET;
	}	

vup = Umax;
		
/*printf("vup:%.2f\n",vup);
printf("row:%d\n",currentrow); 	*/
	

/****begin repeated fictitious play algorithm****/

for (it_num=2;it_num<=IT;it_num++)	{

	/*printf("currentrow at start of it %d: %d\n",it_num,currentrow); */
	
	/*call calcDP to fill in row ONLY IF it hasn't already been filled in*/
	for (j=0;j<MAXASTRATS;j++)
		if (payoff[currentrow][j] == EMPTY)
			payoff[currentrow][j] = calcDP(defstrats[currentrow],attstrats[j]);

	/*update defender strategy history*/
	x[currentrow] = x[currentrow]+1;
	
	/*update V vector*/
	for (j=0;j<MAXASTRATS;j++)
		V[j] = V[j] + payoff[currentrow][j];
	
/*	for (j=0;j<MAXASTRATS;j++)
		printf("V[%d]:%.2f\n",j,V[j]); */
	
	Vmin = 100;
	
	/*attacker looks at defender payoffs in vector V and selects column that minimizes payoff
	-- find payoff and record column*/
	for (j=0;j<MAXASTRATS;j++)
		if (V[j] < Vmin)	{
			Vmin = V[j];
			currentcol = j;
		}
	
/*	printf("Vmin:%.2f col:%d\n",Vmin,currentcol); */
	
	/*establish new lower bound-- pick bigger of two numbes*/
	if ((Vmin/it_num) > vlow)
		vlow = (Vmin/it_num);
		
/*	printf("vlow:%.2f\n",vlow); 
	printf("Vmin/it:%.2f\n",Vmin/it_num); */
	
	for (i=0;i<MAXDSTRATS;i++)
		if (payoff[i][currentcol] == -1)
			payoff[i][currentcol] = calcDP(defstrats[i],attstrats[currentcol]);
			
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
	
	printf("iteration %d complete\n",it_num);  */
	
}

end = clock();

cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

/*print results*/
printf("vlow:%.3f\n",vlow);
printf("vup:%.3f\n",vup);
for (i=0;i<MAXDSTRATS;i++)
	printf("x[%d]=%d ",i,x[i]);
printf("\n");
for (j=0;j<MAXASTRATS;j++)
	printf("y[%d]=%d ",j,y[j]);
printf("\n");
printf("\n");

for (i=0;i<MAXDSTRATS;i++)
	printf("x[%d]=%.2f\n",i,(float)x[i]/(IT));
printf("\n");

for (j=0;j<MAXASTRATS;j++)
	printf("y[%d]=%.2f\n",j,(float)y[j]/IT);
printf("\n");

printf("time:%.4f\n",cpu_time_used);

/*calculate and print entire payoff matrix, if desired*/

/*for (i=0;i<MAXDSTRATS;i++)
	for (j=0;j<MAXASTRATS;j++)
		payoff[i][j] = calcDP(defstrats[i],attstrats[j]);

for (i=0;i<MAXDSTRATS;i++)	{
	for (j=0;j<MAXASTRATS;j++)
		printf("%.3f\t",payoff[i][j]);
	printf("\n");
}
*/


}

float calcDP(struct dstrategy dstrat, struct astrategy astrat)	{

/*dstrat is a single defender strategy passed as an element of the defstrats array*/
/*astrat is a single attacker strategy passed as an element of the attstrats array*/

int t,j,actastrat;				/*t indexes time; j indexes att options; active attacker strat*/
int tend;						/*end day of diversion*/
int m;							/*schedule array dimensions-- detection days*/
int k;							/*indexes safeguards*/

/*array of "sginfo" structures that holds information about properties inherent to SGs
order of sginfo elements MUST match order of safeguards elements in dstrat*/

struct sginfo dstratinfo[NUMSG] = {inventoryi,videologi};
	
/***initialize schedule arrays-- run EXTRADAYS days past end of diversion***/

/*compute m, the total number of simulation days*/
if (astrat.AO.tend == -1)		/*if its a discrete attacker strategy*/
	m = EXTRADAYS;
else		{				/*continuous attacker strategy*/
	tend = astrat.AO.tend;
	m = tend+EXTRADAYS;	
}

/*create schedule arrays-- defender array is kxm with separate row for each SG*/
int aschedule[m], dschedule[NUMSG][m];
	
/***populate schedule arrays-- 0 for days with no activity, 1 for days when an activity
	occurs***/
	
	/*initialize attacker array with 0's*/
	for(t=0;t<m;t++)
		aschedule[t] = 0;
		
	if (astrat.AO.tend == -1)			/*if attacker strategy is disrete*/
		aschedule[0]=1;					/*attacker activity on first day, nothing on rest of days*/ 
	else								/*if it's continuous*/
		for(t=0;t<tend;t=t+astrat.AO.freq)	/*activity of first day and repeating every f days (f=freq) */
			aschedule[t] = 1;
			
/*** print to check ***/
/*
	printf("attacker schedule:\n");
	printf("%-12s","day");
	for (t=0;t<m;t++)	
		printf("%-3d",t+1);
	printf("\n");
	printf("%-12s","activity");
	for (t=0;t<m;t++)
		printf("%-3d",aschedule[t]);
	printf("\n");  
						*/
/*initialize defender array with 0's*/
	for(k=0;k<NUMSG;k++)
		for(t=0;t<m;t++)
			dschedule[k][t] = 0;
			
/*schedule all SGs-- schedule in three groups: continuous, scheduled inspections,
SGs that require post-mortem analysis*/
/*1 represents day on which DETECTION OPPORTUNITY OCCURS*/			
	
for (k=0;k<NUMSG;k++)
	if (dstrat.SG[k].active == 1)	{	
		if (dstratinfo[k].cont == 1)	/*if it's a continuous defender strategy*/
			for (t=0;t<m;t++)
				dschedule[k][t] = 1;	/*schedule it everyday*/
		/*else if it's a discrete defender strategy and occurs during reg. insp.*/
		else				
			for (t=dstrat.SG[k].freq-1;t<m;t=t+dstrat.SG[k].freq)
				dschedule[k][t] = 1;		/*it's active on insp days*/
			
}

/************CHECK************/
/*	printf("defender schedule:\n");
	printf("%-12s","day");
	for (t=0;t<m;t++)	
		printf("%-3d",t+1);
	printf("\n");
	if (dstrat.SG[0].active == 1)	{
		printf("%-12s",dstrat.SG[0].name);
		for (t=0;t<m;t++)
			printf("%-3d",dschedule[0][t]);
			printf("\n");
	}
	if (dstrat.SG[1].active == 1)	{
		printf("%-12s",dstrat.SG[1].name);
		for (t=0;t<m;t++)
			printf("%-3d",dschedule[1][t]);
			printf("\n");
	}
	
	printf("\n");	*/
	
/*loop through time and safeguards to populate dailyDP array
		time loop	{
			safeguard loop	{
				if k is active
					if k is effective against attacker strategy
						calculate DP							*/
						
float dailyDP[NUMSG][m];		/*holds DP for each day for each safeguards over entire period*/
float cumdailyDP[m];			/*holds cum. DP across all SGs each day over entire period*/
float cumDP;					/*cumDP across all SGs over entire simulation period*/
float DPlog[m];		/*use to handle HRA for per instance DP*/

for (k=0;k<NUMSG;k++)
	for(t=0;t<m;t++)
		dailyDP[k][t] = 0;

int inspindex[NUMSG];				/*track how many inspections have occurred*/
for (k=0;k<NUMSG;k++)
	inspindex[k] = 0;
		
/*** TEMPORARY-- FIX ***/

if (strcmp(astrat.AO.name,"cyltheft") == 0)
	actastrat = 0;
else
	actastrat = 1;
/*	
printf("%d",actastrat);	*/

for (t=0;t<m;t++)	{			/*index through days*/
	for (k=0;k<NUMSG;k++)	{	/*index through safeguards*/
		if (dschedule[k][t] == 1)	{			/*if safeguard is active that day*/
		/*	printf("\n%d active on day %d\n",k,t+1); */
				if (effstrats[k][actastrat] == 1){	/*if safeguards is effective against att. strat*/
					/*cylinder theft*/
					if (strcmp(astrat.AO.name,"cyltheft") == 0)	{	
					/*	printf("cylinder theft\n");	*/
						if (strcmp(dstrat.SG[k].name,"inventory")==0)	{				/*inventory*/
						/*	printf("%s\n",dstrat.SG[k].name); */
							if (inspindex[k] == 0)	{
							/*	printf("first inspection\n"); */
								dailyDP[k][t] = 1-exp(-0.65*(dstrat.SG[k].dep+1)*astrat.AO.nitems);
								inspindex[k]++;	
							/*	printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]); */
							}
							else	{
								dailyDP[k][t] = 1-((1+dstrat.SG[k].dep*(1-dailyDP[k][t-dstrat.SG[k].freq]))
								/(dstrat.SG[k].dep+1));
						/*		printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]); */
							}
						}
						if (strcmp(dstrat.SG[k].name,"videolog")==0)	{
						/*	printf("%s\n",dstrat.SG[k].name); */
							while (inspindex[k] == 0)	{
								dailyDP[k][t] = 0.43;
								inspindex[k]++;
							/*	printf("insp. index: %d\n",inspindex[k]); */
							}
							inspindex[k]++;
						/*	printf("insp. index: %d\n",inspindex[k]);
							printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]); */
						}
					
					}
					/*some material from cylinder*/
					if (strcmp(astrat.AO.name,"matcyl") == 0) {	
						if (strcmp(dstrat.SG[k].name,"videolog")==0)	{

							DPlog[0] = 0.43;
					/*		printf("%s\n",dstrat.SG[k].name); */
							int i ;		/*theft events between inspections*/
							int ntevents = 0;
							/*calculate diversion events between inspections*/
					/*		printf("t:%d,last insp.:%d\n",t,t-dstrat.SG[k].freq+1); */
							for (i=t-dstrat.SG[k].freq+1;i<=t;i++)
								if (aschedule[i] == 1)	
									ntevents++;
					/*		printf("number of diversion events: %d\n",ntevents);   */
							/*calculate DP-- need to use previous per instance DP, which is stored in
							DPlog. Find cumulative sum of per instance DPs for all instances
							that have occured since last inpsection*/
							if (inspindex[k] == 0)		{
								dailyDP[k][t] = 1-pow((1-DPlog[0]),ntevents);
					/*			printf("Per instance DP on insp. %d: %.2f\n",inspindex[k]+1	,DPlog[inspindex[k]]); */
								inspindex[k]++;
							}
							else	{
								DPlog[inspindex[k]] = 1-((1+dstrat.SG[k].dep*
									(1-DPlog[inspindex[k]-1]))/(dstrat.SG[k].dep+1)); 
					/*			printf("Per instance DP on insp. %d: %.2f\n",inspindex[k]+1	,DPlog[inspindex[k]]); */
								dailyDP[k][t] = 1-pow((1-DPlog[inspindex[k]]),ntevents);
								inspindex[k]++;
							}
					/*	printf("DP: %.2f\n",dailyDP[k][t]); */
							
						}
					}	
				}
			}
		}
	}

/****** CHECK ******/
/*
for (t=0;t<m;t++)
	printf("DP on day %d: %.2f\n",t,dailyDP[0][t]);	
	
for (t=0;t<m;t++)
	printf("DP on day %d: %.2f\n",t,dailyDP[1][t]);	*/


/*calculate cumulative DP across all SGs on each day*/

/*initialize array to 0's*/
for (t=0;t<m;t++)
	cumdailyDP[t] = 0;
/*populate with daily cumulative DP*/
for (t=0;t<m;t++)	{
	for (k=0;k<NUMSG;k++)
		cumdailyDP[t] = 1-(1-cumdailyDP[t])*(1-dailyDP[k][t]);
/*	printf("daily cumulative DP: %.2f\n",cumdailyDP[t]); */
}

/*calculate cumulative DP over entire simulation period*/

/*initialize value to 0*/
cumDP = 0;
/*calculate cumulative DP*/
for (t=0;t<m;t++)
	cumDP = 1-(1-cumDP)*(1-cumdailyDP[t]);
	
return cumDP;

}	

/*function calculates and returns the cost of a safeguarding strategy*/

int calcCost(struct dstrategy dstrat)	{

int i,k;		/*indexes- k is for SGs*/

int cost;		/*cost of SG*/
int totalcost;	/*total cost of strategy*/

struct sginfo dstratinfo[NUMSG] = {inventoryi,videologi};	/*use to retrieve cost info*/

totalcost = 0;

for (k=0;k<NUMSG;k++)	{
	/*first determine inspection component of cost*/
	/*if insp occurs more frequently than base, multiple cost by increased frequency*/
	if (dstrat.SG[k].active == 1) {
		cost = BASECOST;
		if (dstrat.SG[k].freq != BASEFREQ)
			cost = (BASEFREQ/dstrat.SG[k].freq)*cost;
		if (dstrat.SG[k].dep != BASEDEP)
			cost = depmult * cost;
		/*then determine other costs associated with SG-- use cost listed in SGinfo*/
		if (dstratinfo[k].cost != BASECOST)
			cost = cost + dstratinfo[k].cost; 	
	}
	else
		cost = 0;
	
	/*printf("\n");
	printf("cost of %s: %d\n",dstrat.SG[k].name,cost); */
	
	totalcost = totalcost + cost;
	
}

	return totalcost;

}
	
	