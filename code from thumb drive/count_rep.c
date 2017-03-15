/*has strategy generation information in main function*/
/*prints out number of defender and attacker strategies*/
/*can also be made to print out strategies that are skipped, if desired*/
/*analogous to "count.c" for enrichment*/
/*also tests print functions*/

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
#define RDA			3		/*permutations of reprocessing da*/

#define CHOPSF		28		/*permutations of chopped spent fuel diversion*/
#define TRUSOL		28		/*permutations of TRU solution diversion*/

#define BUDGET		200		/*defender budget*/
#define EMPTY		-1		/*use to initialize payoff matrix-- -1 means DP not yet calculated*/
#define OVERBUDGET	-2		/*use in payoff matrix when defender can't afford row*/	
#define DEPMULT		3		/*multiply by for larger team*/
#define	FAPMULT		1.1		/*multiply by for lower fap*/
#define FAPMULTCEMO	2		/*multiply by for lower fap for cemo*/
#define INSPCOST	10		/*inspection cost- per day*/
#define ANALYSISCOST 2.5	/*per DA sample or batch of seals analysis cost*/
#define ESANALYSISCOST	5	/*per batch of ES swipes-- batch = 6*/
#define	ASSESSCOST	0.6		/*assessment time- per day cost*/
#define ADDINSPCOST	2		/*additional insp activities cost 2 s$/day*/
#define SPECINSPCOST	30	/*cost of special inspections -- 3 times normal*/
#define ADDSPECINSPCOST	6	/*cost of additional activity on special inspection*/

#define MAXDSTRATS	303
#define MAXASTRATS	54
#define	NUMSG		4		/*number of safeguards*/
#define NUMAO		2		/*number of attacker options*/
#define EXTRADAYS	30		/*number of days past end of diversion simulation runs*/

#define VIDEOLOGDP	0.43	/*initial DP for videolog*/
#define VISINSPDP	0.02	/*DP for small anomalies using visual inspection*/
#define VISINSPDPNC	0.29	/*DP for non-critical anomaly using visual inspection*/
#define VISINSPDPC	0.60	/*DP for critical anomaly using visual inspection*/

#define PAYOFF	1		/*1 - new payoff, 0- old payoff*/
#define IT		1000	/*number of iterations for FP algorithm*/
#define VGAP	0.001		/*convergence criterion*/
#define ALPHA		0		/*exponent in denominator of payoff calculation--
							payoff = DP/(FOM*mass)^alpha*/
#define SIMYEAR	360	/*360 days in a simulation year*/

char fileName[] = "strategies_rep.txt";


float numcyl[2] = {13,3};		/*number of cylinders in feed and product storage, resp*/
float nomcylmass[2] = {27360,5180};	/*feed and product nominal cylinder weights (kg)*/
float cylUF6mass[2] = {12500,2270};	/*full feed and product UF6 mass*/
float erfinv[2] = {1.6450,2.1851};	/*inverf(1-2*FAP) for FAP = 0.01, 0.001*/
float FOM[7] = {0.033,0.1,0.991,1.69,2.15,0.50,1.8};		/*FOM for NU,4.5%,19.7%,50%,90%,
														spent fuel, TRU*/
/*OTHER DP'S*/
float intelDP[NUMAO] = {0,0};	/*intelligence DP-- applied daily*/

/*MASS BALANCE CONSTANTS*/
float mberror = 0.0007;

/*NDA CONSTANTS*/
float nomprod = 88;			/*nominal CR (cps) for nda on product cylinder (includes bg)*/
float ndaerror = 0.04;		/*standard error for nda on product cylinder*/
float ndacounttime = 300;		/*assume a count time of 300 seconds for nda*/

/*CEMO CONSTANTS*/
float nomCRcemo = 5;			/*cps, nominal CR at 186-keV in header pipe for 4.5% enr*/

/*budget values*/
float sealcost = 0.01;			/*$1 per seal*/
float activesealcost = 0.5;		/*$100 per seal, usable for two years*/
float swipecost = 3.75;			/*$625 per DA swipe*/

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
int effstrats[NUMSG][NUMAO] = { {1,1},{1,1},{0,1},{1,1} };

/*allowable values for defender parameters; game picks from these values*/
int rfap[2] = {1,2};				/*reprocessing FAP; 1 = 0.05, 2 = 0.01*/	
int rfreq[2] = {1,7};			/*reprocessing inspections frequency options*/
int dep[2] = {1,19};			/*dependency options*/
int facarea[2] = {0,1};			/*facility area; 0 - front-end, 1- back-end*/
int equip[2] = {0,1};			/*3DLRFD purchased?; 0 - no, 1- yes*/
	
/*allowable values for attacker parameters; game picks from these values*/
int dur[3] = {7,30,360};				/*attack duration*/
int rattackfreq[3] = {1,2,7};				/*frequency of attack*/
float SFdeltam[3] = {0.8,8.0,80};		/*kg of SF material (cladding not included)*/
float TRUdeltam[3] = {.008,0.08,0.8};			/*kg TRU in solution*/

/***COSTS NOT CURRENT!!!!***/
/* specification of properties of SGs*/
struct sginfo dualcsi = {"dualcs",30.39,0,0,EMPTY};	
struct sginfo divi = {"div",0.01,0,0,EMPTY};
struct sginfo smmsi = {"smms",3.67,1,EMPTY,EMPTY};
struct sginfo rndai = {"rnda",9.11,0,0,EMPTY};
struct sginfo rdai = {"rda",35.75,0,0,EMPTY};


/*specification of properties of attacker options*/
struct aoptionsinfo chopsfi = {"chopsf",0};		/*discrete*/
struct aoptionsinfo trusoli = {"trusol",1};			/*continuous*/

/*function that prints defender and attacker strategies*/
void printDStrat(FILE *,struct dstrategy dstrat);		/*function prototype*/
void printAStrat(FILE *,struct astrategy astrat);		/*function prototype*/


struct dstrategy defstrats[MAXDSTRATS];
struct astrategy attstrats[MAXASTRATS];


/****************** MAIN *********************/

int main(void)	{

/***** generate all permutations for available SGs *****/

int dcount = 0;			/*counts in strategy array*/
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
		div[index].number = j;
		div[index].tcount = k;
		index++;
	}
	
/*array to hold all SMMS permutations*/
struct safeguards smms[SMMS];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<SMMS;i++)	{
	smms[i].freq = EMPTY;
	smms[i].dep = EMPTY;
	smms[i].number = EMPTY;
	smms[i].tcount = EMPTY;
}

/*no smms option*/
strcpy(smms[0].name,"div");
smms[0].active = 0;	
smms[0].fap = 0;


index = 1;			/*counts in smms array*/

/*rest of smms options*/
for (j=0;j<sizeof(rfap)/sizeof(int);j++) {
	strcpy(smms[index].name,"smms");
	smms[index].active = 1;	
	smms[index].fap = j;
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
			for (d5=0;d5<RDA;d5++)	{
				(defstrats[dcount]).uniqueID = dcount;
				(defstrats[dcount]).SG[0] = dualcs[d1];
				(defstrats[dcount]).SG[1] = div[d2];
				(defstrats[dcount]).SG[2] = smms[d3];
				(defstrats[dcount]).SG[3] = rda[d5];
				/*if dual C/S is inactive and div or nda or da is active, skip strat*/
				/*if freq(nda or da) > freq(C/S), skip strat*/
				if((defstrats[dcount].SG[0].active == 0 && 
					defstrats[dcount].SG[1].active == 1) |
					(defstrats[dcount].SG[0].active == 0 && 
					defstrats[dcount].SG[3].active == 1) |
					(defstrats[dcount].SG[0].active == 0 && 
					defstrats[dcount].SG[4].active == 1) |
					(defstrats[dcount].SG[3].freq < defstrats[dcount].SG[0].freq 
					&& defstrats[dcount].SG[3].active == 1))
					skip++;
				else
					dcount++;
	}
	



/***** generate all permutations for available AOs *****/

struct astrategy *pattstrats;		/*a pointer to a structure*/
pattstrats = &attstrats[0];			/*point the pointer at attstrats[0]*/

int count = 0;			/*reset counts in strategy array*/
index = 1;			/*reset counts in inventory array*/

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
	


/*print results*/

FILE *fp;
   
  /* open the file */
  fp = fopen(fileName, "w");
  if (fp == NULL) 
	 printf("I couldn't open results.dat for writing.\n");

  /* write to the file */

	fprintf(fp,"defender strategies: %d\n",dcount);
	fprintf(fp,"attacker strategies: %d\n",count);
	

/*print all defender strategy combinations to test*/
for (i=0;i<MAXDSTRATS;i++)	
	fprintf(fp,"strategy:%d SG1:%d SG2:%d SG3:%d SG4:%d\n\n",
			defstrats[i].uniqueID,defstrats[i].SG[0].active,defstrats[i].SG[1].active,
			defstrats[i].SG[2].active,defstrats[i].SG[3].active); 

/*print all attacker strategy combinations to test*/
for (i=0;i<MAXASTRATS;i++)	
	fprintf(fp,"strategy:%d AO:%s dur:%d freq:%d deltam:%.0f \n\n",
			attstrats[i].uniqueID,attstrats[i].AO.name,attstrats[i].AO.tend,
			attstrats[i].AO.freq,attstrats[i].AO.deltam); 

  
/*test print functions*/
printDStrat(fp,defstrats[4]);
printAStrat(fp,attstrats[51]);
  
/* close the file */
fclose(fp);

return 0;
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
			fprintf(fp,"\tSF mass taken: %.0f g\n",astrat.AO.deltam);
		else
			fprintf(fp,"\tTRU mass taken: %.0f g\n",astrat.AO.deltam);
	}
}	

