/*reprocessing simulation and FP*/
/*for new payoff, set PAYOFF equal to 1; for old, set PAYOFF = 0*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

/**** successfully generates cumulative DPs ****/

/*structure "safeguard" holds tunable parameters for each safeguards that the 
defender selects; structure "sginfo" holds parameters inherent to that SG that cannot be
altered by the defender*/

#define DUALCS		9		/*permutations of dual c/s*/
#define DIV			5		/*permutations of DIV*/
#define SMMS		3		/*permutations of SMMS*/
#define RNDA		5		/*permutations of reprocessing nda*/
#define RDA			3		/*permutations of reprocessing da*/

#define CHOPSF		28		/*permutations of chopped spent fuel diversion*/
#define TRUSOL		28		/*permutations of TRU solution diversion*/

#define BUDGET		1500		/*defender budget*/
#define EMPTY		-1		/*use to initialize payoff matrix-- -1 means DP not yet calculated*/
#define OVERBUDGET	-2		/*use in payoff matrix when defender can't afford row*/	
#define DEPMULT		3		/*multiply by for larger team*/
#define	FAPMULT		1.1		/*multiply by for lower fap*/
#define FAPMULTCEMO	2		/*multiply by for lower fap for cemo*/
#define RINSPCOST	5		/*inspection cost- per day*/
#define RANALYSISCOST 2	/*per DA sample or batch of seals analysis cost*/
#define	RASSESSCOST	0.6		/*assessment time- per day cost*/
#define RADDINSPCOST	1		/*additional insp activities cost 2 s$/day*/
#define LRFDCOST	9		/*3DLRFD cost-- do not include in "fixed costs" in info array
							bc inspector does not have to buy it*/

#define MAXDSTRATS	1263
#define MAXASTRATS	54
#define	NUMSG		5		/*number of safeguards*/
#define NUMAO		2		/*number of attacker options*/
#define EXTRADAYS	30		/*number of days past end of diversion simulation runs*/

#define VIDEOLOGDP	0.43	/*initial DP for videolog*/
#define VISINSPDP	0.02	/*DP for small anomalies using visual inspection*/
#define VISINSPDPNC	0.29	/*DP for non-critical anomaly using visual inspection*/
#define VISINSPDPC	0.60	/*DP for critical anomaly using visual inspection*/

#define PAYOFF	1		/*1 - new payoff, 0- old payoff*/
#define IT		1000	/*number of iterations for FP algorithm*/
#define VGAP	0.001		/*convergence criterion*/
#define ALPHA		0.0		/*exponent in denominator of payoff calculation--
							payoff = DP/(FOM*mass)^alpha*/
#define SIMYEAR	360	/*360 days in a simulation year*/

char fileName[] = "results_r_1500_0.txt";


float numcyl[2] = {13,3};		/*number of cylinders in feed and product storage, resp*/
float nomcylmass[2] = {27360,5180};	/*feed and product nominal cylinder weights (kg)*/
float cylUF6mass[2] = {12500,2270};	/*full feed and product UF6 mass*/
float erfinv[3] = {1.1631,1.6450,2.1851};	/*inverf(1-2*FAP) for FAP = 0.05, 0.01, 0.001*/
float FOM[7] = {0.033,0.1,0.991,1.69,2.15,0.50,2};		/*FOM for NU,4.5%,19.7%,50%,90%,
														spent fuel, TRU*/

/*nominal process characteristics*/
float nommassFE = 800;		/*nominal material mass in front-end accountancy tank in kg*/
float nommassBE = 8;		/*nominal TRU mass in product tank in kg*/
float TRUfrac = 0.01;				/*fraction of TRU in SF*/
float nomvolBE = 31;		/*nominal volume in TRU product tank in L*/
float nomdenBE = 0.300;		/*nominal density of TRU in product tank in kg/L*/
float nomvolFE = 2880;		/*nominal volume in FE accountability tank in L*/
float nomdenFE = 0.300;		/*nominal density of SF in FE accountability tank in kg/L*/

/*OTHER DP'S*/
float intelDP[NUMAO] = {0,0};	/*intelligence DP-- applied daily*/

/*DRD CONSTANTS*/
float neutroneff = 0.01;	/*single He-3 neutron detector efficiency*/
float nomSFR = 1.4e5;		/*nominal spon. fiss. emission rate for SF-- n/s-kgSF*/
int drdcounttime = 60;		/*count time- 60 secs*/
float neutronerror = 0.10;	/*10% uncertainty assumed for neutron detectors*/
int numDetectors = 4;		/*number of detectors in DRD network*/

/*DIV CONSTANTS*/
float lowDIV = 0.29;		/*minor indicators*/
float highDIV = 0.60;		/*major indicators*/

/*NDA CONSTANTS*/
float nomprod = 88;			/*nominal CR (cps) for nda on product cylinder (includes bg)*/
float ndaerror = 0.04;		/*standard error for nda on product cylinder*/
float ndacounttime = 300;		/*assume a count time of 300 seconds for nda*/

/*SMMS CONSTANTS*/
float smmserror = 0.003;			/*uncertainty for smms used to calculate stddev*/

/*DA CONSTANTS*/
float DADP = 1;					/*if sample is taken from batch that has been tampered with,
								DP = 1*/

/*budget values*/
float inspcostslope = 0.1724;	/*slope of linear function that determines per insp cost*/
float inspcostyint = 4.8276;		/*y-intercept of linear function that determines per insp cost*/
float addinspmult = 0.2;		/*additional inspections cost 20% of base insp cost*/

clock_t start, end;
double cpu_time_used;


/*general structure that stores parameters associated with each safeguard*/
struct safeguards {
	char name[11];	/*name of SG*/
	int active;		/*did the defender select?*/
	int fap;		/*false alarm probability*/
	float number;	/*for reprocessing - area; 0 - front end, 1- back end*/
	int tcount;		/*for reprocessing - 3DLRFD; 0 - no equip, 1 - equip*/
	int freq;		/*scheduled/random inspection frequency*/
	int dep;		/*scheduled/random inspection dependency*/
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
	float xp;			/*product enrichment*/
	float xf;			/*feed enrichment*/
};

/*general structure that stores defender strategies*/
struct dstrategy	{
	int uniqueID;					/*uniquely identifies strategy*/
	struct safeguards SG[NUMSG];	/*array of SG structures that hold parameter 
									specifications for each SG*/
} ;

/*general structure that stores attacker strategies*/
/*stores only one active attacker option*/
struct astrategy	{
	int uniqueID;
	struct aoptions AO;
};

/*general structure of properties of SGs that cannot be altered by defender*/
struct sginfo	{
	char name[11];	/*name of SG*/
	float cost;		/*annual fixed cost for SG, or per seal cost*/
	int cont;		/*0 if SG is discrete; 1 if cont*/
	int insptype;	/*0 if conducted at normal inspections; 1 for special insp.*/
	int analysis;	/*number of days required for post-mortem analysis*/
};

/*general structure of properties of aoptions that cannot be altered by attacker*/
struct aoptionsinfo	{
	char name[11];	/*name of attacker options*/
	int cont;		/*is it continuous? 0 if discrete, 1 if continuous*/
};

/*effective def. strategies-- holds 1 if xstrat is effective against ystrat; 0 otherwise*/
/*right now EVERYTHING is "effective" against udfeed until random inspections are in*/
int effstrats[NUMSG][NUMAO] = { {1,1},{1,1},{1,1},{0,1},{1,0} };

/*allowable values for defender parameters; game picks from these values*/
int rfap[2] = {1,2};				/*reprocessing FAP; 1 = 0.05, 2 = 0.01*/
									/*except for NDA, where 1=0.01, 2=0.001*/	
int rfreq[2] = {1,7};			/*reprocessing inspections frequency options*/
int dep[2] = {1,19};			/*dependency options*/
int facarea[2] = {0,1};			/*facility area; 0 - front-end, 1- back-end*/
int equip[2] = {0,1};			/*3DLRFD purchased?; 0 - no, 1- yes*/
	
/*allowable values for attacker parameters; game picks from these values*/
int dur[3] = {7,30,360};				/*attack duration*/
int rattackfreq[3] = {1,2,7};				/*frequency of attack*/
float SFdeltam[3] = {0.8,8,80};			/*kg of SF material (cladding not included)*/
float TRUdeltam[3] = {0.008,0.08,0.8};			/*kg TRU in solution*/

/***COSTS NOT CURRENT!!!!***/
/* specification of properties of SGs*/
struct sginfo dualcsi = {"dualcs",130.09,0,0,EMPTY};	
struct sginfo divi = {"div",0.18,0,0,EMPTY};
struct sginfo smmsi = {"smms",55,1,EMPTY,EMPTY};
struct sginfo rndai = {"rnda",38.25,0,0,EMPTY};
struct sginfo rdai = {"rda",765,0,0,EMPTY};


/*specification of properties of attacker options*/
struct aoptionsinfo chopsfi = {"chopsf",0};		/*discrete*/
struct aoptionsinfo trusoli = {"trusol",1};			/*continuous*/

/*function called by fp to calc payoff-- arguments are defender and attacker strategies--
calculates DP and weights  by mass and FOM*/
float calcPayoff(struct dstrategy, struct astrategy);	/*function prototype*/

/*function takes single defender strategy and returns integer cost value*/
float calcCost(struct dstrategy dstrat,struct astrategy astrat);

/*functions that generate an array of defender and attacker strategies, respectively*/
void genDStrats(struct dstrategy []);			/*function prototype*/
void genAStrats(struct astrategy []);			/*function prototype*/

/*function that prints defender and attacker strategies*/
void printDStrat(FILE *,struct dstrategy dstrat);		/*function prototype*/
void printAStrat(FILE *,struct astrategy astrat);		/*function prototype*/

/*functions that calculate product and feed mass flows at enrichment facility*/
float calcMassFlow(float xf,float xp, float xw, float fraccasc);
float calcFeedMass(float xf,float xp, float xw, float fraccasc);

/*calculate factorial of a number-- used for hypergeometric distribution in DA sampling*/
double factorial(int);

/*function performs "choose"-- were choose(a,b) = a!/(b!*(b-a)!)*/
double choose(int, int);

/*variables used for the FP algorithm*/
/*allocate memory here so that they aren't on "stack"*/

int x[MAXDSTRATS];	/*hold defender strategy history*/
int y[MAXASTRATS];	/*holds attacker strategy history*/
float U[MAXDSTRATS];	/*vector that holds columns*/
float V[MAXASTRATS];	/*vector that holds rows*/
float payoff[MAXDSTRATS][MAXASTRATS];	/*payoff matrix- populated by simulation calls*/
/*int xplay[IT];
int yplay[IT];*/

struct dstrategy defstrats[MAXDSTRATS];
struct astrategy attstrats[MAXASTRATS];



/****************** MAIN *********************/

int main(void)	{

int i,j;				/*random indices*/
int calcPayoffcalls = 0;	/*track how many times code calls calcDP*/

genDStrats(defstrats);		/*pass pointer to genDStrats to manipulate defstrats*/

printf("defender strategies generated\n");

/*generate attacker strategy array*/

genAStrats(attstrats);
printf("attacker strategies generated\n");

printf("so far, so good!\n\n");

/*int binsize = 500;
int numbins = 8700/binsize;		/*number of histogram bins*/

/*float plotdata[numbins];		/*cumulative distribution fcn*/
/*float pdfdata[numbins];		/*probability distribution fcn data*/
/*int attackerstrat = 2;

/*int cheapeststrat=1;
int mostexpensivestrat=0;

for (i=2;i<MAXDSTRATS;i++)
	if (calcCost(defstrats[i],attstrats[attackerstrat])<calcCost(defstrats[cheapeststrat],
		attstrats[attackerstrat]))
		cheapeststrat = i;

for (i=0;i<MAXDSTRATS;i++)
	if (calcCost(defstrats[i],attstrats[attackerstrat])>calcCost(defstrats[mostexpensivestrat],
		attstrats[attackerstrat]))
		mostexpensivestrat = i;
		
printf("cheapest strategy:%d, cost:%.2f\n",cheapeststrat,calcCost(defstrats[cheapeststrat],
											attstrats[attackerstrat]));
printf("most expensive strategy:%d, cost:%.2f\n",mostexpensivestrat,
									calcCost(defstrats[mostexpensivestrat],attstrats[attackerstrat]));*/
	
/*probability density function-- go to 86 bc most expensive strat is 8576 s$*/

/*for (i=0;i<numbins;i++)
	plotdata[i] = 0;
	
for (i=0;i<MAXDSTRATS;i++)
	for (j=1;j<numbins+1;j++)
		if (calcCost(defstrats[i],attstrats[2])<j*binsize)	{
			plotdata[j-1]++;
			printf("bin %d done\n",j);
			break;
		}
					

/*****WRITE BUDGET DATA TO FILE*****/
 
/* FILE *fp;  
  /* open the file */
/*  fp = fopen("budgetpdf_rep.txt", "w");
  if (fp == NULL) {
	 printf("I couldn't open results.dat for writing.\n");
  }

  /* write to the file */
/*  for (j=0; j<numbins; j++)
	 fprintf(fp, "%d, %.2f\n", (j+1)*binsize, plotdata[j]);

  /* close the file */
/*fclose(fp);

/********** FP ALGORITHM ***********/

/* variables used for FP */
int it_num;			/*counts iterations*/
float v_gap = 1;	/*criterion used to establish convergence and end run*/
int currentrow;		/*holds current row number*/
int currentcol;		/*holds current column number*/
float vup = 10;			/*holds current upper bound*/
float vlow = -10;			/*holds current lower bound*/
float Vmin;			/*holds smallest value in V*/
float Umax;			/*holds largest value in U*/

start = clock();		/*used to calculate time for fp algorithm to run*/

/*initialize payoff matrix to zeros*/
for (i=0;i<MAXDSTRATS;i++)
	for (j=0;j<MAXASTRATS;j++)
		payoff[i][j] = EMPTY; 

for (i=0;i<MAXDSTRATS;i++)
	x[i] = 0;
	
for (j=0;j<MAXASTRATS;j++)
	y[j] = 0;
	
/*randomly pick first attacker move*/ 
currentcol = 2;

/*populate column of payoff matrix for currentcol*/
for (i=0;i<MAXDSTRATS;i++)	{
	payoff[i][currentcol] = calcPayoff(defstrats[i],attstrats[currentcol]);
	calcPayoffcalls++;
}

/*copy column of payoff into U vector*/
for (i=0;i<MAXDSTRATS;i++)	
	U[i] = payoff[i][currentcol];
	
/*for (i=0;i<MAXDSTRATS;i++)
	printf("%.1f\n",U[i]); */
	
/*update y strategy vector*/
y[currentcol] = y[currentcol]+1;

/*initialize V*/
for (j=0;j<MAXASTRATS;j++)
	V[j] = 0;
	
/****begin repeated fictitious play algorithm****/
it_num = 1;

while (v_gap > VGAP)	{
/*for (it_num=1;it_num<=IT;it_num++)	{ */

	Umax = 0;
	for (i=0;i<MAXDSTRATS;i++)
		/*if U[i] is bigger than the current Umax*/
		if (U[i] > Umax)	{
			/*and the defender can afford that row*/
			if (calcCost(defstrats[i],attstrats[currentcol]) <= BUDGET)	{
				Umax = U[i];
				currentrow = i;
			}
			else
				for (j=0;j<MAXASTRATS;j++)
					payoff[i][j] = OVERBUDGET;
		}	
				
/*	printf("Umax: %.1f\n",Umax);	
	printf("currentrow: %d\n",currentrow); */
		
	/*assign vup to smaller of two values-- current vup or Umax/it_num*/
	if (Umax/it_num < vup)
		vup = Umax/it_num;
		
/*printf("Umax: %.1f, itnum: %d\n",Umax, it_num);	
printf("vup: %.1f\n",vup); */
	
	/*populate row of payoff matrix for currentrow*/
	for (j=0;j<MAXASTRATS;j++)	
		if (payoff[currentrow][j] == EMPTY)	{
			payoff[currentrow][j] = calcPayoff(defstrats[currentrow],attstrats[j]);
			calcPayoffcalls++;
		}
		
	/*update row into V vector*/
	for (j=0;j<MAXASTRATS;j++)
		V[j] = V[j] + payoff[currentrow][j];
		
/*	for (j=0;j<MAXASTRATS;j++)
		printf("V[%d]:%.1f\n",j,V[j]); */

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
			
/*	printf("Vmin: %.1f\n",Vmin);
	printf("currentcol: %d\n",currentcol); */
	
	/*assign vlow to bigger of two values-- current vlow or Vmin/it_num*/
	if (Vmin/it_num > vlow)
		vlow = Vmin/it_num;

/*	printf("Vmin: %.1f, itnum: %d\n",Vmin, it_num);
	printf("vlow:%.1f\n",vlow); */
		
	/*populate column of payoff matrix for currentcol*/
	for (i=0;i<MAXDSTRATS;i++)	
		if (payoff[i][currentcol] == EMPTY)	{
			payoff[i][currentcol] = calcPayoff(defstrats[i],attstrats[currentcol]);
			calcPayoffcalls++;
		}	
	
	/*copy into U vector*/
	for (i=0;i<MAXDSTRATS;i++)
		U[i] = U[i]+payoff[i][currentcol];
		
/*	for (i=0;i<MAXDSTRATS;i++)
		printf("U[%d]: %.1f\n",i,U[i]); */
	
	/*update attacker strategy vector*/
	y[currentcol] = y[currentcol]+1;	
	
/*	xplay[it_num-1] = currentrow;
	yplay[it_num-1] = currentcol;*/
	
	it_num++;
	v_gap = (vup-vlow)/vup;
	if (it_num%10000 == 0)	{
		printf("iteration %d complete\n",it_num);
		/*printf("vlow: %.3f\n",vlow);
		printf("vup: %.3f\n",vup);
		printf("vgap: %.3f\n",v_gap);*/
	}
	
}

end = clock();

cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC; 

/*printf("xplays:\t\t\typlays:\n");
for (i=0;i<IT;i++)
	printf("%d\t\t\t%d\n",xplay[i],yplay[i]);
printf("\n"); 

/*print results*/


FILE *fp;
   
  /* open the file */
  fp = fopen(fileName, "w");
  if (fp == NULL) {
	 printf("I couldn't open results.dat for writing.\n");
	 /*exit(0);*/
  }

  /* write to the file */
fprintf(fp,"budget:%d\n",BUDGET);
fprintf(fp,"alpha:%.2f\n",ALPHA);
fprintf(fp,"vlow:%.6f\n",vlow);
fprintf(fp,"vup:%.6f\n",vup);
fprintf(fp,"vgap: %.3f\n",v_gap);
fprintf(fp,"iterations:%d\n",it_num);

for (i=0;i<MAXDSTRATS;i++)
	if ((float)x[i]/it_num > 5/it_num)
		fprintf(fp,"x[%d]=%.3f\n",i,(float)x[i]/(it_num));
fprintf(fp,"\n");

for (i=0;i<MAXDSTRATS;i++)
	if ((float)x[i]/it_num > 5/it_num)
		printDStrat(fp,defstrats[i]);
fprintf(fp,"\n");

for (j=0;j<MAXASTRATS;j++)
	if ((float)y[j]/it_num > 5/it_num)
		fprintf(fp,"y[%d]=%.3f\n",j,(float)y[j]/(it_num+1));
fprintf(fp,"\n");

for (j=0;j<MAXASTRATS;j++)
	if ((float)y[j]/it_num > 5/it_num)
		printAStrat(fp,attstrats[j]);
fprintf(fp,"\n");

fprintf(fp,"time:%.4f\n",cpu_time_used); 

fprintf(fp,"calcPayoff calls: %d\n\n",calcPayoffcalls);

  /* close the file */
  fclose(fp);

return 0;
}

void genDStrats(struct dstrategy defstrats[])	{

/***** generate all permutations for available SGs *****/

int count = 0;			/*counts in strategy array*/
unsigned int i,j,k;				/*random indices*/

/*array to hold all dual C/S permutations*/
/*dualcs includes videolog and directional radiation detectors*/
struct safeguards dualcs[DUALCS];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<DUALCS;i++)	{
	dualcs[i].number = EMPTY;
	dualcs[i].tcount = EMPTY;
}

/*no dualcs option*/
strcpy(dualcs[0].name,"dualcs");
dualcs[0].active = 0;
dualcs[0].freq = 0;
dualcs[0].dep = 0;
dualcs[0].fap = 0;


int index = 1;			/*counts in dualcs array*/

/*rest of dualcs options*/
for (j=0;j<sizeof(rfreq)/sizeof(int);j++)
	for (k=0;k<sizeof(dep)/sizeof(int);k++)
		for (i=0;i<sizeof(rfap)/sizeof(int);i++)	{
			strcpy(dualcs[index].name,"dualcs");
			dualcs[index].active = 1;
			dualcs[index].freq = rfreq[j];
			dualcs[index].dep = dep[k];
			dualcs[index].fap = rfap[i];
			index++;
		}
	
/*array to hold all div permutations*/
/*no frequency is assigned to DIV bc it is ASSUMED TO EQUAL THE DUALCS FREQUENCY*/
struct safeguards div[DIV];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<DIV;i++)	{
	div[i].freq = EMPTY;
	div[i].fap = EMPTY;
	div[i].dep = EMPTY;
}

/*no div option*/
strcpy(div[0].name,"div");
div[0].active = 0;	
div[0].number = 0;
div[0].tcount = 0;

index = 1;			/*counts in div array*/

/*rest of div options*/
for (j=0;j<sizeof(facarea)/sizeof(int);j++)
	for (k=0;k<sizeof(equip)/sizeof(int);k++)	{
		strcpy(div[index].name,"div");
		div[index].active = 1;	
		div[index].number = facarea[j];
		div[index].tcount = equip[k];
		index++;
	}
	
/*array to hold all SMMS permutations*/
struct safeguards smms[SMMS];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<SMMS;i++)	{
	smms[i].dep = EMPTY;
	smms[i].number = EMPTY;
	smms[i].tcount = EMPTY;
}

/*no smms option*/
strcpy(smms[0].name,"smms");
smms[0].active = 0;	
smms[0].freq = 0;
smms[0].fap = 0;


index = 1;			/*counts in smms array*/

/*rest of smms options*/
for (j=0;j<sizeof(rfap)/sizeof(int);j++) {
	strcpy(smms[index].name,"smms");
	smms[index].active = 1;	
	smms[index].freq = 1;
	smms[index].fap = rfap[j];
	index++;
	}
	
/*array to hold all nda permutations*/
struct safeguards rnda[RNDA];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<RNDA;i++)	{
	rnda[i].number = EMPTY;
	rnda[i].tcount = EMPTY;
	rnda[i].dep = EMPTY;
}

/*no nda option*/
strcpy(rnda[0].name,"rnda");
rnda[0].active = 0;
rnda[0].fap = 0;
rnda[0].freq = 0;

index = 1;			/*counts in inventory array*/

/*rest of nda options*/
for (j=0;j<sizeof(rfap)/sizeof(int);j++)
	for (k=0;k<sizeof(rfreq)/sizeof(int);k++)	{
		strcpy(rnda[index].name,"rnda");
		rnda[index].active = 1;
		rnda[index].fap = rfap[j];
		rnda[index].freq = rfreq[k];
		index++;
	}
	
/*array to hold all da permutations*/
struct safeguards rda[RDA];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<RDA;i++)	{
	rda[i].fap = EMPTY;
	rda[i].number = EMPTY;
	rda[i].tcount = EMPTY;
	rda[i].dep = EMPTY;
}

/*no da option*/
strcpy(rda[0].name,"rda");
rda[0].active = 0;
rda[0].freq = 0;

index = 1;			/*counts in inventory array*/

/*rest of da options*/

for (k=0;k<sizeof(rfreq)/sizeof(int);k++)	{
	strcpy(rda[index].name,"rda");
	rda[index].active = 1;
	rda[index].freq = rfreq[k];
	index++;
}


/***** generate all defender strategies *****/

int d1,d2,d3,d4,d5;
int skip = 0;
/*generate all strategy combinations and store in defstrats*/
for (d1=0;d1<DUALCS;d1++)
	for (d2=0;d2<DIV;d2++)	
		for (d3=0;d3<SMMS;d3++)
			for (d4=0;d4<RNDA;d4++)
				for (d5=0;d5<RDA;d5++)	{
					(defstrats[count]).uniqueID = count;
					(defstrats[count]).SG[0] = dualcs[d1];
					(defstrats[count]).SG[1] = div[d2];
					(defstrats[count]).SG[2] = smms[d3];
					(defstrats[count]).SG[3] = rnda[d4];
					(defstrats[count]).SG[4] = rda[d5];
					/*if dual C/S is inactive and div or nda or da is active, skip strat*/
					/*if freq(nda or da) > freq(C/S), skip strat*/
					if((defstrats[count].SG[0].active == 0 && 
						defstrats[count].SG[1].active == 1) |
						(defstrats[count].SG[0].active == 0 && 
						defstrats[count].SG[3].active == 1) |
						(defstrats[count].SG[0].active == 0 && 
						defstrats[count].SG[4].active == 1) |
						(defstrats[count].SG[3].freq < defstrats[count].SG[0].freq 
						&& defstrats[count].SG[3].active == 1)|
						(defstrats[count].SG[4].freq < defstrats[count].SG[0].freq
						&& defstrats[count].SG[4].active == 1))
						skip++;
					else
						count++;
	}

}

void genAStrats(struct astrategy attstrats[])	{

/***** generate all permutations for available AOs *****/

struct astrategy *pattstrats;		/*a pointer to a structure*/
pattstrats = &attstrats[0];			/*point the pointer at attstrats[0]*/

int count = 0;			/*counts in strategy array*/
int index = 1;			/*counts in inventory array*/
unsigned int i,j,k;				/*random indices*/

/*array to hold all chopped spent fuel diversion permutations*/
struct aoptions chopsf[CHOPSF];

for (i=0;i<CHOPSF;i++)	{
	chopsf[i].nitems = EMPTY;
	chopsf[i].area = EMPTY;
	chopsf[i].xf = EMPTY;
	chopsf[i].xp = EMPTY;
}
	
/*no chopped SF option*/
strcpy(chopsf[0].name,"chopsf");
chopsf[0].active = 0;
chopsf[0].tend = 0;
chopsf[0].freq = 0;
chopsf[0].deltam = 0;


index = 1;  /*index through chopped SF array*/
/*rest of chopped SF options*/
for (i=0;i<sizeof(dur)/sizeof(int);i++)
	for (j=0;j<sizeof(rattackfreq)/sizeof(int);j++)
		for (k=0;k<sizeof(SFdeltam)/sizeof(float);k++)	{
			strcpy(chopsf[index].name,"chopsf");
			chopsf[index].active = 1;
			chopsf[index].tend = dur[i];
			chopsf[index].freq = rattackfreq[j];
			chopsf[index].deltam = SFdeltam[k];
			index++;
	}
	
/*array to hold all TRUsol permutations*/
struct aoptions trusol[TRUSOL];

for(i=0;i<TRUSOL;i++)	{
	trusol[i].nitems = EMPTY;
	trusol[i].area = EMPTY;
	trusol[i].xf = EMPTY;
	trusol[i].xp = EMPTY;
}

/*no trusol option*/
strcpy(trusol[0].name,"trusol");
trusol[0].active = 0;
trusol[0].tend = 0;
trusol[0].freq = 0;
trusol[0].deltam = 0;

index = 1;

/*rest of TRU sol options*/
for (i=0;i<sizeof(dur)/sizeof(int);i++)
	for (j=0;j<sizeof(rattackfreq)/sizeof(int);j++)
		for (k=0;k<sizeof(TRUdeltam)/sizeof(float);k++)	{
			strcpy(trusol[index].name,"trusol");
			trusol[index].active = 1;
			trusol[index].tend = dur[i];
			trusol[index].freq = rattackfreq[j];
			trusol[index].deltam = TRUdeltam[k];
			index++;
	}
				

count = 0;		/*index through attstrats*/

/*generate all strategy combinations for each AO individually and store in attstrats*/
/*unlike for defender strategies, only one AO is active at a time*/

/*if statement ensures that duration is longer than frequency, ie cannot have strategy
where attack occurs every 30 days but duration is 7 days*/

for (i=1;i<CHOPSF;i++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = chopsf[i];
	if (attstrats[count].AO.tend >= attstrats[count].AO.freq)
		count++;
	}

for (j=1;j<TRUSOL;j++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = trusol[j];
	if (attstrats[count].AO.tend >= attstrats[count].AO.freq)
		count++;
	}

/*print all strategy combinations to test*/
/*for (i=0;i<MAXASTRATS;i++)	
	printf("strategy:%d AO:%s dur:%d freq:%d deltam:%.0f \n\n",
			attstrats[i].uniqueID,attstrats[i].AO.name,attstrats[i].AO.tend,
			attstrats[i].AO.freq,attstrats[i].AO.deltam); */
}


float calcPayoff(struct dstrategy dstrat, struct astrategy astrat)	{

/*dstrat is a single defender strategy passed as an element of the defstrats array*/
/*astrat is a single attacker strategy passed as an element of the attstrats array*/

int t,actastrat;				/*t indexes time; j indexes att options; active attacker strat*/
int tend;						/*end day of diversion*/
int m;							/*schedule array dimensions-- detection days*/
int k;							/*indexes safeguards*/
float DP;

DP = 0;
tend = 0;

/*array of "sginfo" structures that holds information about properties inherent to SGs
order of sginfo elements MUST match order of safeguards elements in dstrat*/

struct sginfo dstratinfo[NUMSG] = {dualcsi,divi,smmsi,rndai,rdai};
	
/***initialize schedule arrays-- run EXTRADAYS days past end of diversion***/

/*compute m, the total number of simulation days*/
if (astrat.AO.tend == EMPTY)		/*if its a discrete attacker strategy*/
	m = EXTRADAYS;
else		{				/*continuous attacker strategy*/
	tend = astrat.AO.tend;
	m = tend+EXTRADAYS;	
}

/*printf("number of sim days = %d\n",m);*/

/*create schedule arrays-- defender array is kxm with separate row for each SG*/
int aschedule[m], dschedule[NUMSG][m];
	
/***populate schedule arrays-- 0 for days with no activity, 1 for days when an activity
	occurs***/
	
	/*initialize attacker array with 0's*/
	for(t=0;t<m;t++)
		aschedule[t] = 0;
		
	if (astrat.AO.tend == EMPTY)			/*if attacker strategy is disrete*/
		aschedule[0]=1;					/*attacker activity on first day, nothing on rest of days*/ 
	else								/*if it's continuous*/
		for(t=0;t<tend;t=t+astrat.AO.freq)	/*activity of first day and repeating every f days (f=freq) */
			aschedule[t] = 1;
			
	/*for (t=0;t<m;t++)
		printf("%d\n",aschedule[t]);
	printf("\n");*/
		
						
/*initialize defender array with 0's*/
	for(k=0;k<NUMSG;k++)
		for(t=0;t<m;t++)
			dschedule[k][t] = 0;
			
/*schedule all SGs-- schedule in two groups: continuous, scheduled inspections,
SGs that require post-mortem analysis*/
/*1 represents day on which safeguard is active, not when detection occurs (in case
of post-mortem analysis)*/			
	
for (k=0;k<NUMSG;k++)	{
	if (dstrat.SG[k].active == 1)	{	
		if (dstratinfo[k].cont == 1)	{/*if it's a continuous defender strategy*/
			for (t=0;t<m;t++)	{
				dschedule[k][t] = 1;	/*schedule it everyday*/
				/*printf("%d ",dschedule[k][t]);
				printf("\n");*/
			}	
		}		
		else if (strcmp(dstrat.SG[k].name,"div")!=0) 	{
			for (t=dstrat.SG[k].freq-1;t<m;t=t+dstrat.SG[k].freq)	{
				dschedule[k][t] = 1;		/*it's active on insp days*/
				/*printf("%d ",dschedule[k][t]);
				printf("\n");*/
			}
		}
		else if (strcmp(dstrat.SG[k].name,"div")==0)	{
			for (t=dstrat.SG[0].freq-1;t<m;t=t+dstrat.SG[0].freq)	{
				dschedule[k][t] = 1;		/*it's active on insp days*/
				/*printf("%d ",dschedule[k][t]);
				printf("\n");*/
			}
		}
	}
}	
/*loop through time and safeguards to populate dailyDP array
		time loop	{
			safeguard loop	{
				if k is active
					if k is effective against attacker strategy
						calculate DP							*/
						
float dailyDP[NUMSG][m];		/*holds DP for each day for each safeguards over entire period*/
float cumdailyDP[m];			/*holds cum. DP across all SGs each day over entire period*/
float cumDP = 0;					/*cumDP across all SGs over entire simulation period*/
float vlDP = 0;					/*holds DP from videolog*/
float perdetectorDP;			/*holds DP for each radiation detector*/
float drdDP = 0;				/*holds dir. radiation det. DP*/
float massTRU[m];					/*holds mass of TRU obtained each day*/
float massU[m];						/*holds mass of U obtained each day*/
float mattype = 0;					/*FOM to describe material attractiveness*/
int attackday = 0;					/*if an attack happens that day, attackday is set to t--
									use this to track when last attack happened*/
int attackssinceinsp = 0;			/*count number of attacks that have occurred since last
									inspection*/
									
float cumdrdDP = 0;					/*use for drd validation*/

/*use to handle HRA for per instance DP--- here use the frequency of inspection bc
that must be the most frequent safeguard, so array will definitely have enough space*/
int arraydim = 0;		/*dimensions of DPlog array*/
if (dstrat.SG[0].active == 1)
	arraydim = ceil(m/dstrat.SG[0].freq)+1;	

float DPlog[arraydim];
				
/*use to integer set here to refer to position in FOM array-- ex: NU is FOM[0]*/
enum MAT	{
		NU,
		LEU,
		LEU197,
		HEU50,
		HEU90,
		SF,
		TRU
};
	

for (t=0;t<m;t++)					/*initialize to O's*/
	massTRU[t] = 0;

for (k=0;k<NUMSG;k++)
	for(t=0;t<m;t++)
		dailyDP[k][t] = 0;			/*initialize dailyDP to 0*/
		
for(t=0;t<m;t++)
	cumdailyDP[t] = 0;

int inspindex[NUMSG];				/*track how many inspections have occurred*/
for (k=0;k<NUMSG;k++)
	inspindex[k] = 0;
	
float invDP[m];					/*holds inventory DP-- used for HRA*/
for (t=0;t<m;t++)
    invDP[t] = 0;
		
/*** find active attacker strategy and assign number ***/
/** this is only used for "effstrats"*/

if (strcmp(astrat.AO.name,"chopsf") == 0)
	actastrat = 0;
else if (strcmp(astrat.AO.name,"trusol") == 0)
	actastrat = 1;
	
/*this loop calculates material quantity and quality*/
for (t=0;t<m;t++)	{
	if (aschedule[t] == 1)	{
		if (strcmp(astrat.AO.name,"chopsf") == 0)	{	
			/*mass stolen in each attack is SF-- need to calc mass TRU (kg)*/
			massTRU[t] = astrat.AO.deltam * TRUfrac;
			mattype = FOM[SF];
		}
		if (strcmp(astrat.AO.name,"trusol") == 0) {	
			/*mass stolen in each attack is deltam (kg)-- material is TRU*/
			massTRU[t] = astrat.AO.deltam;
			mattype = FOM[TRU];
		}
	}
}
	
/*printf("%d",actastrat);*/	

/*this loop calculates DP*/
for (t=0;t<m;t++)	{			/*index through days*/
	if (aschedule[t] == 1)	{	/*if at attack happened today, set attackday equal to t*/
		attackday = t;
		attackssinceinsp++;
	}
	for (k=0;k<NUMSG;k++)	{	/*index through safeguards*/
		if (dschedule[k][t] == 1)	{			/*if safeguard is active that day*/
			/*printf("\n%d active on day %d\n",k,t+1); */
				if (effstrats[k][actastrat] == 1){	/*if safeguards is effective against att. strat*/
					/*diversion of chopped spent fuel pieces*/
					if (strcmp(astrat.AO.name,"chopsf") == 0)	{
						if (strcmp(dstrat.SG[k].name,"dualcs")==0)	{		
						/***first calculate videolog DP and store in vlDP***/
							
							DPlog[0] = VIDEOLOGDP;
							
							/*make sure that if not attack's have occurred since last
							inspection, "inspindex" doesn't increment bc inspection
							does not affect HRA calculation*/
							if (attackssinceinsp == 0)
								vlDP = 0;
							else	{
					
							/*calculate DP-- need to use previous per instance DP, which 
							is stored in DPlog. Find cumulative sum of per instance DPs 
							for all instances that have occured since last inpsection*/
							
							/*if it's the first inspection*/
							if (inspindex[k] == 0)		{
								vlDP = 1-pow((1-DPlog[0]),attackssinceinsp);
								inspindex[k]++;
							}
							/*if it's not the first inspection*/
							else	{
								DPlog[inspindex[k]] = 1-((1+dstrat.SG[k].dep*
									(1-DPlog[inspindex[k]-1]))/(dstrat.SG[k].dep+1)); 
								vlDP = 1-pow((1-DPlog[inspindex[k]]),attackssinceinsp);
								inspindex[k]++;
							}
							}
							
							/*printf("vlDP:%.4f\n",vlDP);*/
							/*** then calculate directional radiation detectors***/
							
							/*first calculate per attack event DP*/
							
							float divSFR = 0;		/*spon. fiss. rate after missing mass*/
							float signal = 0;		/*signal in one detector*/
							float nominal = 0;		/*nominal count in one detector*/
							float threshold = 0;	/*alarm trigger threshold*/
							float stdsig = 0;		/*std dev signal*/
							float stdnom = 0;		/*std dev nominal*/
								
							divSFR = (nommassFE- astrat.AO.deltam)*nomSFR;
							signal = neutroneff * drdcounttime * divSFR;
							
							/*counts in detector is spent fuel count rate * mass spent fuel
							* count time * detector efficiency*/
							nominal = nomSFR * nommassFE * drdcounttime * neutroneff;
							
							stdsig = neutronerror * signal;
							stdnom = neutronerror * nominal;
							
							threshold = nominal - sqrt(2)*stdnom*erfinv[dstrat.SG[k].fap-1];
							perdetectorDP = 1-0.5*(1-erf((threshold-signal)/(sqrt(2)*stdsig)));
							
							/*DP for network of detectors per attack*/
							drdDP= 1-pow((1-perdetectorDP),numDetectors);
							
							/*DP per inspections (multiple attacks might occur between
							inspections and inspector would review records for all at inspector)*/
							drdDP = 1-pow((1-drdDP),attackssinceinsp);
							cumdrdDP = 1-(1-cumdrdDP)*(1-drdDP);
							
							/*printf("drdDP:%.4f\n",drdDP);*/
							
							/*make dailyDP equal inspDP*/
							dailyDP[k][t] = 1-(1-vlDP)*(1-drdDP);
							attackssinceinsp = 0;
						}
						if (strcmp(dstrat.SG[k].name,"div")==0)	{
							/*if the defender is performing DIV on front end*/
							if (dstrat.SG[k].number == facarea[0])	{
								/*only DP if attack is still ongoing*/
								if (t < tend)	{
									/*if no 3DLRFD*/
									if (dstrat.SG[k].tcount == equip[0])
										dailyDP[k][t] = lowDIV;
									/*if 3DLRFD, decrease NDP by 50%*/
									else
										dailyDP[k][t] = 1-0.5*(1-lowDIV);
								}
							}
						}
						if (strcmp(dstrat.SG[k].name,"smms")==0)	{
							float FEDP = 0;			/*DP from FE accountability tank*/
							float BEDP = 0;			/*DP from BE accountability tank*/
							/*det. opp. in FE accountabilit tank if an attack occurred 
							yesterday*/
							/*use t>=1 clause so don't try to access negative array values*/
							if ((aschedule[t-1] == 1) && (t>=1))	{
								float signal = 0;			/*signal volume*/
								float nominal = nomdenFE;	/*nominal density*/
								float threshold = 0;		/*alarm trigger threshold*/
								float stdsig = 0;			/*std dev signal*/
								float stdnom = 0;			/*std dev nominal*/
						
								/*signal density equals (V*d-deltam)/V*/
								signal = (nomvolFE*nominal-astrat.AO.deltam)/nomvolFE;
								/*printf("nominal: %.4f\n",nominal);
								printf("signal: %.4f\n",signal);*/
					
								/*calculate standard deviations*/
								stdsig = smmserror * signal;
								stdnom = smmserror * nominal;
					
								threshold = nominal - sqrt(2)*stdnom*erfinv[dstrat.SG[k].fap-1];
								/*DP from FE on day t*/
								FEDP = 1-0.5*(1-erf((threshold-signal)/(sqrt(2)*stdsig)));
								/*printf("frontend DP on day %d: %.4f\n",t,FEDP);*/
							}
							/*det. opp. in TRU prod tank if an attack occurred 5 days ago*/
							if ((aschedule[t-5] == 1) && (t>=5))	{
								float signal = 0;			/*signal volume*/
								float nominal = nomdenBE;	/*nominal density*/
								float threshold = 0;		/*alarm trigger threshold*/
								float stdsig = 0;			/*std dev signal*/
								float stdnom = 0;			/*std dev nominal*/
						
								/*signal density equals (V*d-deltam)/V*/
								/*deltam*TRUfrac bc deltam is mass SF, and need mass TRU*/
								signal = (nomvolBE*nominal-astrat.AO.deltam*TRUfrac)/nomvolBE;
					
								/*calculate standard deviations*/
								stdsig = smmserror * signal;
								stdnom = smmserror * nominal;
					
								threshold = nominal - sqrt(2)*stdnom*erfinv[dstrat.SG[k].fap-1];
								/*DP from backend on day t*/
								BEDP = 1-0.5*(1-erf((threshold-signal)/(sqrt(2)*stdsig)));
								/*printf("backend DP on day %d: %.4f\n",t,BEDP);*/
							}
							dailyDP[k][t] = 1-(1-FEDP)*(1-BEDP);
							/*printf("daily DP on day %d: %.4f\n",t+1,dailyDP[k][t]);*/
						}
						if (strcmp(dstrat.SG[k].name,"rda")==0)	{
							/*if an attack occurred yesterday*/
							if (aschedule[t-1] == 1)	{
								/*find next insp (SG0)*/
								int nextinsp = t+1;
								while (dschedule[0][nextinsp] == 0)
									nextinsp++;
								dailyDP[k][nextinsp] = DADP;
							}
						}
	
					}
					/*diversion of TRU solution from product tank*/
					if (strcmp(astrat.AO.name,"trusol") == 0) {	
						if (strcmp(dstrat.SG[k].name,"dualcs")==0)	{		
					
							/*** calculate directional radiation detectors***/
							
							/*first calculate per attack event DP*/
							
							float divSFR = 0;		/*spon. fiss. rate after missing mass*/
							float signal = 0;		/*signal in one detector*/
							float nominal = 0;		/*nominal count in one detector*/
							float threshold = 0;	/*alarm trigger threshold*/
							float stdsig = 0;		/*std dev signal*/
							float stdnom = 0;		/*std dev nominal*/
								
							/*divide by TRUfrac bc actual mass being stolen "deltam" is
							mass of TRU, but need equivalent mass of SF for calculations*/
							divSFR = (nommassBE/TRUfrac- astrat.AO.deltam/TRUfrac)*nomSFR;
							signal = neutroneff * drdcounttime * divSFR;
							
							/*counts in detector is spent fuel count rate * mass spent fuel
							* count time * detector efficiency*/
							nominal = nomSFR * nommassFE * drdcounttime * neutroneff;
							
							stdsig = neutronerror * signal;
							stdnom = neutronerror * nominal;
							
							threshold = nominal - sqrt(2)*stdnom*erfinv[dstrat.SG[k].fap-1];
							perdetectorDP = 1-0.5*(1-erf((threshold-signal)/(sqrt(2)*stdsig)));
							
							/*DP for network of detectors per attack*/
							drdDP= 1-pow((1-perdetectorDP),numDetectors);
							
							/*DP per inspections (multiple attacks might occur between
							inspections and inspector would review records for all at inspector)*/
							drdDP = 1-pow((1-drdDP),attackssinceinsp);
							cumdrdDP = 1-(1-cumdrdDP)*(1-drdDP);
							
							/*printf("drdDP:%.4f\n",drdDP);*/
							
							/*make dailyDP equal inspDP*/
							dailyDP[k][t] = drdDP;
							attackssinceinsp = 0;
						}
						if (strcmp(dstrat.SG[k].name,"div")==0)	{
							/*if the defender is performing DIV on back end*/
							if (dstrat.SG[k].number == facarea[1])	{
								/*only DP if attack is still ongoing*/
								if (t < tend)	{
									/*if no 3DLRFD*/
									if (dstrat.SG[k].tcount == equip[0])
										dailyDP[k][t] = lowDIV;
									/*if 3DLRFD, decrease NDP by 50%*/
									else
										dailyDP[k][t] = 1-0.5*(1-lowDIV);
								}
							}
						}
						if (strcmp(dstrat.SG[k].name,"smms")==0)	{
							/*det. opp. if an attack occurred today*/
							if (aschedule[t] == 1)	{
								float signal = 0;			/*signal volume*/
								float nominal = nomvolBE;	/*nominal volume*/
								float threshold = 0;		/*alarm trigger threshold*/
								float stdsig = 0;			/*std dev signal*/
								float stdnom = 0;			/*std dev nominal*/
						
								/*signal density equals (V*d-deltam)/d*/
								signal = (nominal*nomdenBE-astrat.AO.deltam)/nomdenBE;
					
								/*calculate standard deviations*/
								stdsig = smmserror * signal;
								stdnom = smmserror * nominal;
					
								threshold = nominal - sqrt(2)*stdnom*erfinv[dstrat.SG[k].fap-1];
								
								dailyDP[k][t] = 1-0.5*(1-erf((threshold-signal)/(sqrt(2)*stdsig)));
							
							}
						}
				
					}	
			}
		}
	}

}
		
/*calculate cumulative DP across all SGs on each day*/

/*include this here to isolate DP from certain SGs that always 
occur with inspection*/

/*printf("cumDP for DRD: %.4f\n",cumdrdDP);*/

/*float cumSGDP[NUMSG];
for (k=0;k<NUMSG;k++)
	cumSGDP[k] = 0;
for (k=0;k<NUMSG;k++){
	for (t=0;t<m;t++)	
		cumSGDP[k] = 1-(1-cumSGDP[k])*(1-dailyDP[k][t]);
	printf("cumDP for safeguard %d: %.4f\n",k,cumSGDP[k]);
}*/

/*initialize array to 0's*/
for (t=0;t<m;t++)
	cumdailyDP[t] = 0;
/*populate with daily cumulative DP*/
for (t=0;t<m;t++)	{
	for (k=0;k<NUMSG;k++)
		cumdailyDP[t] = 1-(1-cumdailyDP[t])*(1-dailyDP[k][t]);
}

for (t=0;t<m;t++)
	cumdailyDP[t] = 1-(1-cumdailyDP[t])*(1-intelDP[actastrat]);

/*initialize value to 0*/
cumDP = 0;

/*calculate cumulative DP over entire simulation period-- using cumdailyDP*/
for (t=0;t<m;t++)
	cumDP = 1-(1-cumDP)*(1-cumdailyDP[t]);
	
/*calculate total TRU obtained*/
float totalTRU = 0;
for (t=0;t<m;t++)
	totalTRU = massTRU[t]+totalTRU;

	
/*printf("\ntotalTRU:%.5f\n",totalTRU);
printf("cumDP: %.12f\n",cumDP);
printf("mattype: %.3f\n",mattype);	*/
float epsilon = 0.001;				/*in payoff-- 1-epsilon*DP in denominator-- epsilon
										prevents error when DP = 1*/
float payoff = 0;

if (PAYOFF == 1)
	payoff = cumDP/((1+epsilon-cumDP)*pow((mattype * totalTRU),ALPHA));
else
	payoff = cumDP/pow((mattype * totalTRU),ALPHA);

return payoff;


}

/*function calculates and returns the cost of a safeguarding strategy*/

float calcCost(struct dstrategy dstrat,struct astrategy astrat)	{

/*use to retrieve analysis info*/
struct sginfo dstratinfo[NUMSG] = {dualcsi,divi,smmsi,rndai,rdai};

float totalcost = 0;	/*total cost of strategy*/
float dualcs=0;
float div=0;
float smms=0;
float rnda=0;
float rda=0;
float inspcost = 0;		/*per inspection cost for dualc/s-- all other activities are 
						20% of this cost per inspection*/


/*number of inspections for strategy*/
int numinsp = 0;

/*cost for dual c/s*/
if (dstrat.SG[0].active == 1)	{
	/*number of inspections required for entire year of strategy*/
	numinsp = floor((SIMYEAR)/dstrat.SG[0].freq);
	/*printf("numinp: %d\n",numinsp);*/
	
	/*time costs*/
	/*calc per inspection cost*/
	inspcost = inspcostslope * dstrat.SG[0].freq + inspcostyint;
	dualcs = inspcost*numinsp;
	/*printf("time cost: %2f\n",dualcs);*/

	if (dstrat.SG[0].dep == dep[1])	{
		dualcs = dualcs * DEPMULT;
	}
		
	if (dstrat.SG[0].fap == rfap[0])
		dualcs = dualcs * FAPMULT;

	/*add in equipment costs*/
	dualcs = dualcs + dstratinfo[0].cost;
	/*printf("dualcs: %.2f\n",dualcs);*/
}
/*cost for div*/
if (dstrat.SG[1].active == 1)	{
	
	/*number of inspections required for entire year of strategy-- here dstrat.SG[0].freq
	is used bc a seperate frequency IS NOT specified for DIV; it occurs as frequently as 
	dualcs*/
	
	numinsp = floor((SIMYEAR)/dstrat.SG[0].freq);
	
	/*time costs-- additional inspection cost, multiply by addinspmult*/	
	div = addinspmult * inspcost * numinsp;
		
	/*add in cost of 3DLRFD if purchased*/
	if (dstrat.SG[1].tcount == equip[1])
		div = div + LRFDCOST;
		
	/*fixed costs-- maintenance*/ 

	div = div + dstratinfo[1].cost;
				
}
/*smms cost*/
if (dstrat.SG[2].active == 1)	{
	
	/*number of inspections required for entire year of strategy*/
	numinsp = SIMYEAR;
	
	/*time costs-- assessment cost*/
	
	smms = RASSESSCOST * numinsp;
	
	if (dstrat.SG[2].fap == rfap[0])
		smms = smms * FAPMULT;
		
	/*fixed cost*/
	smms = smms + dstratinfo[2].cost;
	
}
/*reprocessing da cost*/
if (dstrat.SG[4].active == 1)	{
	
	/*number of inspections required for entire year of strategy--
	sample results analyzed at inspection (dualcs freq)*/
	
	numinsp = floor((SIMYEAR)/dstrat.SG[4].freq);
	
	/*time cost-- additional inspection activity*/

	rda = inspcost* addinspmult * numinsp;
		
	/*analysis cost-- $2/batch; here numinsp used as proxy for number of batches
	number of batches equals number of samples*/
	rda = rda + RANALYSISCOST * numinsp;
				
	/*fixed costs*/
	rda = rda + rdai.cost;
	
}


totalcost = dualcs + div + smms + rda;

return totalcost;

}
/*function prints defender strategy*/
void printDStrat(FILE *fp,struct dstrategy dstrat)	{

/*scan through every SG-- if it's active, print define parameters*/
int i;

fprintf(fp,"strategy %d:\n",dstrat.uniqueID);
for (i=0;i<NUMSG;i++)
	if (dstrat.SG[i].active == 1)	{
		fprintf(fp,"SG%d: %s\n",i,dstrat.SG[i].name);
		if (dstrat.SG[i].fap != EMPTY)	{
			if (dstrat.SG[i].fap == rfap[0])
				fprintf(fp,"\tFAP: 0.05\n");
			else
				fprintf(fp,"\tFAP: 0.01\n");
		}
		if (dstrat.SG[i].number != EMPTY)	{
			if (strcmp(dstrat.SG[i].name,"div") == 0)	
				if (dstrat.SG[i].number == facarea[0])
					fprintf(fp,"\tarea: front-end\n");
				else
					fprintf(fp,"\tarea: back-end\n");
		}
		if (dstrat.SG[i].tcount != EMPTY)
			if (dstrat.SG[i].tcount == equip[0])
				fprintf(fp,"\t3DLRFD purchased: NO\n");
			else
				fprintf(fp,"\t3DLRFD purchased: YES\n");
		if (dstrat.SG[i].freq != EMPTY)
			fprintf(fp,"\tfreq: %d 1/days\n",dstrat.SG[i].freq);
		if (dstrat.SG[i].dep != EMPTY)	{
			if (dstrat.SG[i].dep == dep[0])
				fprintf(fp,"\tsize of inspection team: small\n");
			else
				fprintf(fp,"\tsize of inspection team: large\n");
		}
	}

}	

void printAStrat(FILE *fp,struct astrategy astrat)	{	

/*print parameters for active AO*/

fprintf(fp,"strategy %d:\n",astrat.uniqueID);
	fprintf(fp,"AO: %s\n",astrat.AO.name);
	if (astrat.AO.tend != EMPTY)
		fprintf(fp,"\tduration: %d days\n",astrat.AO.tend);
	if (astrat.AO.freq != EMPTY)
		fprintf(fp,"\tfreq: %d 1/days\n",astrat.AO.freq);
	if (astrat.AO.deltam != EMPTY)	{
		if (strcmp(astrat.AO.name,"chopsf") == 0)
			fprintf(fp,"\tSF mass taken: %.3f kg\n",astrat.AO.deltam);
		else
			fprintf(fp,"\tTRU mass taken: %.3f kg\n",astrat.AO.deltam);
	}
}


