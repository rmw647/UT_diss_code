/*working model with all scheduled enrichment SGs and FP algorithm*/
/*set budget and and iterations in #defines*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

/**** successfully generates cumulative DPs ****/

/*structure "safeguard" holds tunable parameters for each safeguards that the 
defender selects; structure "sginfo" holds parameters inherent to that SG that cannot be
altered by the defender*/

#define SWU		465000		/*nominal plant capacity*/
#define	NUMCASC	60.0		/*number of cascades at facility*/

#define INSP		9		/*permutations of inspection*/
#define PSEALS		5		/*permutations of pseals*/
#define NDA			5		/*permutations of nda*/
#define DA			5		/*permutations of da*/
#define VIDEOTRANS	3		/*permutations of videotrans*/
#define ASEALS		3		/*permutations of aseals*/
#define CEMO		5		/*permutations of cemo*/

#define CYLTHEFT	5		/*permutations of cylinder theft*/
#define MATCYL		49		/*permutations of matcyl*/
#define MATCASC		37		/*permutations of matcasc*/
#define REPIPING	28		/*permutations of repiping*/
#define RECYCLE		55		/*permutations of recycle*/
#define UDFEED		7		/*permutations of udfeed*/
#define REPIPINGFREQ	1	/*repiping frequency MUST be every day--always observable*/

#define BUDGET		2000		/*defender budget*/
#define EMPTY		-1		/*use to initialize payoff matrix-- -1 means DP not yet calculated*/
#define OVERBUDGET	-2		/*use in payoff matrix when defender can't afford row*/	
#define MBCOST 		1.1		/*annual fixed cost for mbalance-- capitol + maintenance*/
#define VLCOST		36.3	/*annual fixed cost for videolog*/
#define NDACOST	  	3.67		/*annual fixed cost for nda*/
#define	DACOST		9.11		/*annual fixed cost for da*/
#define	VTCOST		71.50		/*annual fixed cost for videotrans*/
#define	CEMOCOST	19.80		/*annual fixed cost for CEMO*/
#define DEPMULT		3		/*multiply by for larger team*/
#define	FAPMULT		1.1		/*multiply by for lower fap*/
#define FAPMULTCEMO	2		/*multiply by for lower fap for cemo*/
#define INSPCOST	10		/*inspection cost- per day*/
#define ANALYSISCOST 2.5	/*per DA sample or batch of seals analysis cost*/
#define	ASSESSCOST	0.6		/*assessment time- per day cost*/
#define ADDINSPCOST	2		/*additional insp activities cost 2 s$/day*/

#define MAXDSTRATS	27405
#define MAXASTRATS	151
#define	NUMSG		7		/*number of safeguards*/
#define NUMAO		6		/*number of attacker options*/
#define EXTRADAYS	30		/*number of days past end of diversion simulation runs*/

#define VIDEOLOGDP	0.43	/*initial DP for videolog*/
#define VIDEOTRANSDP	0.64	/*initial DP for videotrans*/
#define PSEALSDP		0.85	/*DP for a single passive seal*/
#define ASEALSDP	0.40	/*DP for a single active seal*/

#define NUMCYLFALSE	1		/*number of product cylinders falsified-- all over-enriched
							product placed in one cylinder*/

#define IT			1	/*number of iterations for FP algorithm*/
#define SIMYEAR	360	/*360 days in a simulation year*/

float xprod = 0.045;			/*nominal product enrichment*/
float xw = 0.0022;				/*tails enrichment*/
float xfeed = 0.00711;			/*nominal feed enrichment*/

float numcyl[2] = {13,8};		/*number of cylinders in feed and product storage, resp*/
float nomcylmass[2] = {27360,5180};	/*feed and product nominal cylinder weights (kg)*/
float erfinv[2] = {1.6450,2.1851};	/*inverf(1-2*FAP) for FAP = 0.01, 0.001*/

/*OTHER DP'S*/
float bgDP = 0.01;				/*background DP-- applied every DAY-- for dailyDP[k][t]
							k=NUMSG+1 bc intelligence is stored as the NUMSG+1 value */
float intelDP[NUMSG] = {0,0,0,0,0,0,0,};	/*intelligence DP-- applied for each SCENARIO*/

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
	float number;	/*fraction cylinders w/seal (pseal), #samples (DA), # seals/cascade (aseal)*/
	int tcount;		/*count time*/
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
	int cost;		/*SG cost*/
	int cont;		/*0 if SG is discrete; 1 if cont*/
	int insptype;	/*0 if conducted at scheduled inspections; 1 for random insp.
					2- both*/
	int analysis;	/*number of days required for post-mortem analysis*/
};

/*general structure of properties of aoptions that cannot be altered by attacker*/
struct aoptionsinfo	{
	char name[11];	/*name of attacker options*/
	int cont;		/*is it continuous? 0 if discrete, 1 if continuous*/
};

/*effective def. strategies-- holds 1 if xstrat is effective against ystrat; 0 otherwise*/
/*right now EVERYTHING is "effective" against udfeed until random inspections are in*/
int effstrats[NUMSG][NUMAO] = { {1,1,1,1,1,1},{0,1,0,1,1,1},{0,1,0,1,1,1},{0,0,0,1,1,1},
								{1,1,0,0,0,1},{0,1,1,1,0,1},{0,0,0,1,1,1} };

/*allowable values for defender parameters; game picks from these values*/
int fap[2] = {1,2};				/*1 = 0.01, 2 = 0.001*/	
float numseals[2] = {0.5,1};	/*fraction of cylinders sealed-- used for "number"*/
float DAsamples[2] = {0.125,0.5};	/*fraction cylinders sampled with DA-- used for "number"*/
int tcount[2] = {300,3600};		/*count time*/
int freq[2] = {7,30};			/*frequency options*/
int dep[2] = {1,19};			/*dependency options*/
	
/*allowable values for attacker parameters; game picks from these values*/
int dur[3] = {7,30,360};				/*attack duration*/
int attackfreq[2] = {7,30};				/*frequency of attack*/
float ncyl[2] = {1,2};					/*number of cylinders-- float bc nitems must be*/
float fcasc[3] = {1/NUMCASC,0.1,0.5};	/*fraction cascades tapped or repiped*/
int area[2] = {0,1};					/*0-feed,1-product*/
float deltam[2] = {0.010,0.100};		/*kg/item/instance (as3)*/
float xf[2] = {0.00711,0.045};				/*feed enrichment*/
float xp[3] = {0.197,0.50,0.90};			/*product enrichment*/

/* specification of properties of SGs*/
struct sginfo inspectioni = {"inventory",300,0,0,EMPTY};
struct sginfo psealsi = {"pseals",1,0,0,10};
struct sginfo ndai = {"nda",1,0,0,EMPTY};
struct sginfo dai = {"da",30,0,2,14};
struct sginfo videotransi = {"videotrans",200,1,EMPTY,EMPTY};
struct sginfo asealsi = {"aseals",1,1,EMPTY,EMPTY};
struct sginfo cemoi = {"cemo",300,1,EMPTY,EMPTY};


/*specification of properties of attacker options*/
struct aoptionsinfo cylthefti = {"cyltheft",0};		/*discrete*/
struct aoptionsinfo matcyli = {"matcyl",1};			/*continuous*/
struct aoptionsinfo matcasci = {"matcasc",1};	
struct aoptionsinfo repipingi = {"repiping",1};	
struct aoptionsinfo recyclei = {"recycle",1};	
struct aoptionsinfo udfeedi = {"udfeed",1};	

/*function called by fp to calc DP-- arguments are defender and attacker strategies 
and frequency and dependency for scheduled and random inspections*/
float calcDP(struct dstrategy, struct astrategy);		/*function prototype*/

/*function takes single defender strategy and returns integer cost value*/
long calcCost(struct dstrategy dstrat,struct astrategy astrat);

/*functions that generate an array of defender and attacker strategies, respectively*/
void genDStrats(struct dstrategy []);			/*function prototype*/
void genAStrats(struct astrategy []);			/*function prototype*/

/*function that prints defender and attacker strategies*/
void printDStrat(struct dstrategy dstrat);		/*function prototype*/
void printAStrat(struct astrategy astrat);		/*function prototype*/

/*functions that calculate product and feed mass flows at enrichment facility*/
float calcMassFlow(float xf,float xp, float xw, float fraccasc);
float calcFeedMass(float xf,float xp, float xw, float fraccasc);

/*calculate factorial of a number-- used for hypergeometric distribution in DA sampling*/
long factorial(int);

/*function performs "choose"-- were choose(a,b) = a!/(b!*(b-a)!)*/
long choose(int, int);

/*variables used for the FP algorithm*/
/*allocate memory here so that they aren't on "stack"*/

int x[MAXDSTRATS];	/*hold defender strategy history*/
int y[MAXASTRATS];	/*holds attacker strategy history*/
float U[MAXDSTRATS];	/*vector that holds columns*/
float V[MAXASTRATS];	/*vector that holds rows*/
float payoff[MAXDSTRATS][MAXASTRATS];	/*payoff matrix- populated by simulation calls*/
int xplay[IT];
int yplay[IT];

struct dstrategy defstrats[MAXDSTRATS];
struct astrategy attstrats[MAXASTRATS];


/****************** MAIN *********************/

int main(void)	{

start = clock();		/*used to calculate time for fp algorithm to run*/

int i,j;				/*random indices*/
int calcDPcalls = 0;	/*track how many times code calls calcDP*/

genDStrats(defstrats);		/*pass pointer to genDStrats to manipulate defstrats*/

/*generate attacker strategy array*/

genAStrats(attstrats);

printf("so far, so good!\n\n");

/********** FP ALGORITHM ***********/

/* variables used for FP */
int it_num;			/*counts iterations*/
int currentrow;		/*holds current row number*/
int currentcol;		/*holds current column number*/
float vup;			/*holds current upper bound*/
float vlow;			/*holds current lower bound*/
float Vmin;			/*holds smallest value in V*/
float Umax;			/*holds largest value in U*/

/*initialize payoff matrix to zeros*/
for (i=0;i<MAXDSTRATS;i++)
	for (j=0;j<MAXASTRATS;j++)
		payoff[i][j] = EMPTY; 

for (i=0;i<MAXDSTRATS;i++)
	x[i] = 0;
	
for (j=0;j<MAXASTRATS;j++)
	y[j] = 0;

vlow = -10;
vup = 10;
	
/*randomly pick first attacker move*/ 
currentcol = 3;

printf("cumulativeDP: %.3f\n",calcDP(defstrats[2475],attstrats[74]));
return 0;
}

void genDStrats(struct dstrategy defstrats[])	{

/***** generate all permutations for available SGs *****/

int count = 0;			/*counts in strategy array*/
unsigned int i,k;				/*random indices*/
unsigned int j;
/*array to hold all inspection permutations*/
/*inspection includes inventory, mbalance and videolog*/
struct safeguards inspection[INSP];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<INSP;i++)	{
	inspection[i].number = EMPTY;
	inspection[i].tcount = EMPTY;
}

/*no inspection option*/
strcpy(inspection[0].name,"inspection");
inspection[0].active = 0;
inspection[0].freq = 0;
inspection[0].dep = 0;
inspection[0].fap = 0;

int index = 1;			/*counts in inspection array*/

/*rest of inspection options*/
for (j=0;j<sizeof(freq)/sizeof(int);j++)
	for (k=0;k<sizeof(dep)/sizeof(int);k++)
		for (i=0;i<sizeof(fap)/sizeof(int);i++)	{
			strcpy(inspection[index].name,"inspection");
			inspection[index].active = 1;
			inspection[index].freq = freq[j];
			inspection[index].dep = dep[k];
			inspection[index].fap = fap[i];
			index++;
		}
	
/*array to hold all pseals permutations*/
struct safeguards pseals[PSEALS];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<PSEALS;i++)	{
	pseals[i].fap = EMPTY;
	pseals[i].tcount = EMPTY;
	pseals[i].dep = EMPTY;
}

/*no pseals option*/
strcpy(pseals[0].name,"pseals");
pseals[0].active = 0;
pseals[0].number = 0;
pseals[0].freq = 0;

index = 1;			/*counts in inventory array*/

/*rest of pseals options*/
for (j=0;j<sizeof(numseals)/sizeof(float);j++)
	for (k=0;k<sizeof(freq)/sizeof(int);k++)	{
		strcpy(pseals[index].name,"pseals");
		pseals[index].active = 1;
		pseals[index].number = numseals[j];
		pseals[index].freq = freq[k];
		index++;
	}
	
/*array to hold all nda permutations*/
struct safeguards nda[NDA];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<NDA;i++)	{
	nda[i].number = EMPTY;
	nda[i].tcount = EMPTY;
	nda[i].dep = EMPTY;
}

/*no nda option*/
strcpy(nda[0].name,"nda");
nda[0].active = 0;
nda[0].fap = 0;
nda[0].freq = 0;

index = 1;			/*counts in inventory array*/

/*rest of nda options*/
for (j=0;j<sizeof(fap)/sizeof(int);j++)
	for (k=0;k<sizeof(freq)/sizeof(int);k++)	{
		strcpy(nda[index].name,"nda");
		nda[index].active = 1;
		nda[index].fap = fap[j];
		nda[index].freq = freq[k];
		index++;
	}
	
/*array to hold all da permutations*/
struct safeguards da[DA];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<DA;i++)	{
	da[i].fap = EMPTY;
	da[i].tcount = EMPTY;
	da[i].dep = EMPTY;
}

/*no da option*/
strcpy(da[0].name,"da");
da[0].active = 0;
da[0].number = 0;
da[0].freq = 0;

index = 1;			/*counts in inventory array*/

/*rest of da options*/
for (j=0;j<sizeof(DAsamples)/sizeof(float);j++)
	for (k=0;k<sizeof(freq)/sizeof(int);k++)	{
		strcpy(da[index].name,"da");
		da[index].active = 1;
		da[index].number = DAsamples[j];
		da[index].freq = freq[k];
		index++;
	}
				
/*array to hold all videotrans permutations*/
struct safeguards videotrans[VIDEOTRANS];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<VIDEOTRANS;i++)	{
	videotrans[i].fap = EMPTY;
	videotrans[i].number = EMPTY;
	videotrans[i].tcount = EMPTY;
	videotrans[i].freq = EMPTY;
}

/*no videotrans option*/
strcpy(videotrans[0].name,"videotrans");
videotrans[0].active = 0;
videotrans[0].dep = 0;

index = 1;

/*rest of videotrans options*/
for (j=0;j<sizeof(dep)/sizeof(int);j++)	{
		strcpy(videotrans[index].name,"videotrans");
		videotrans[index].active = 1;
		videotrans[index].dep = dep[j];
		index++;
	}

/*array to hold all aseals permutations*/
struct safeguards aseals[ASEALS];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<ASEALS;i++)	{
	aseals[i].fap = EMPTY;
	aseals[i].tcount = EMPTY;
	aseals[i].freq = EMPTY;
	aseals[i].dep = EMPTY;
}

/*no aseals option*/
strcpy(aseals[0].name,"aseals");
aseals[0].active = 0;
aseals[0].number = 0;

index = 1;

/*rest of aseals options*/
for (j=0;j<sizeof(numseals)/sizeof(float);j++)	{
		strcpy(aseals[index].name,"aseals");
		aseals[index].active = 1;
		aseals[index].number = numseals[j];
		index++;
	}

/*array to hold all cemo permutations*/
struct safeguards cemo[CEMO];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<CEMO;i++)	{
	cemo[i].number = EMPTY;
	cemo[i].freq = EMPTY;
	cemo[i].dep = EMPTY;
}

/*no cemo option*/
strcpy(cemo[0].name,"cemo");
cemo[0].active = 0;
cemo[0].fap = 0;
cemo[0].tcount = 0;

index = 1;

/*rest of cemo options*/
for (j=0;j<sizeof(fap)/sizeof(int);j++)
	for (k=0;k<sizeof(tcount)/sizeof(int);k++)	{
		strcpy(cemo[index].name,"cemo");
		cemo[index].active = 1;
		cemo[index].fap = fap[j];
		cemo[index].tcount = tcount[k];
		index++;
	}

/***** generate all defender strategies *****/

int d1,d2,d3,d4,d5,d6,d7;

/*generate all strategy combinations and store in defstrats*/
for (d1=0;d1<INSP;d1++)
	for (d2=0;d2<PSEALS;d2++)	
		for (d3=0;d3<NDA;d3++)
			for (d4=0;d4<DA;d4++)
				for (d5=0;d5<VIDEOTRANS;d5++)
					for (d6=0;d6<ASEALS;d6++)
						for (d7=0;d7<CEMO;d7++)	{
							(defstrats[count]).uniqueID = count;
							(defstrats[count]).SG[0] = inspection[d1];
							(defstrats[count]).SG[1] = pseals[d2];
							(defstrats[count]).SG[2] = nda[d3];
							(defstrats[count]).SG[3] = da[d4];
							(defstrats[count]).SG[4] = videotrans[d5];
							(defstrats[count]).SG[5] = aseals[d6];
							(defstrats[count]).SG[6] = cemo[d7];
							if ((defstrats[count].SG[1].freq > defstrats[count].SG[0].freq)|
							    (defstrats[count].SG[2].freq > defstrats[count].SG[0].freq)|
							    (defstrats[count].SG[3].freq > defstrats[count].SG[0].freq))
							    break;
							else
								count++;
	}

}

void genAStrats(struct astrategy attstrats[])	{

/***** generate all permutations for available AOs *****/

struct astrategy *pattstrats;		/*a pointer to a structure*/
pattstrats = &attstrats[0];			/*point the pointer at defstrats[0]*/

int count = 0;			/*counts in strategy array*/
int index = 1;			/*counts in inventory array*/
unsigned int i,j,k;				/*random indices*/

/*array to hold all cyltheft permutations*/
struct aoptions cyltheft[CYLTHEFT];

for (i=0;i<CYLTHEFT;i++)	{
	cyltheft[i].tend = EMPTY;
	cyltheft[i].freq = EMPTY;
	cyltheft[i].deltam = EMPTY;
	cyltheft[i].xf = EMPTY;
	cyltheft[i].xp = EMPTY;
}
	
/*no cyltheft options*/
strcpy(cyltheft[0].name,"cyltheft");
cyltheft[0].active = 0;
cyltheft[0].nitems = 0;
cyltheft[0].area = 0;

index = 1;  /*index through cyltheft array*/
/*rest of cyltheft options*/
for (j=0;j<sizeof(ncyl)/sizeof(float);j++)
	for (k=0;k<sizeof(area)/sizeof(int);k++)	{
		strcpy(cyltheft[index].name,"cyltheft");
		cyltheft[index].active = 1;
		cyltheft[index].nitems = ncyl[j];
		cyltheft[index].area = area[k];
		index++;
	}
	
/*array to hold all matcyl permutations*/
struct aoptions matcyl[MATCYL];

for(i=0;i<MATCYL;i++)	{
	matcyl[i].xf = EMPTY;
	matcyl[i].xp = EMPTY;
}

/*no matcyl option*/
strcpy(matcyl[0].name,"matcyl");
matcyl[0].active = 0;
matcyl[0].tend = 0;
matcyl[0].freq = 0;
matcyl[0].nitems = 0;
matcyl[0].area = 0;
matcyl[0].deltam = 0;

index = 1;
unsigned int m,n;	/*extra indices*/

/*rest of matcyl options*/
for (i=0;i<sizeof(dur)/sizeof(int);i++)
	for (j=0;j<sizeof(attackfreq)/sizeof(int);j++)
		for (k=0;k<sizeof(ncyl)/sizeof(float);k++)
			for (m=0;m<sizeof(area)/sizeof(int);m++)
				for (n=0;n<sizeof(deltam)/sizeof(float);n++)	{
					strcpy(matcyl[index].name,"matcyl");
					matcyl[index].active = 1;
					matcyl[index].tend = dur[i];
					matcyl[index].freq = attackfreq[j];
					matcyl[index].nitems = ncyl[k];
					matcyl[index].area = area[m];
					matcyl[index].deltam = deltam[n];
					index++;
				}
				
/*array to hold all matcasc permutations*/
struct aoptions matcasc[MATCASC];

for(i=0;i<MATCASC;i++)	{
	matcasc[i].area = EMPTY;
	matcasc[i].xf = EMPTY;
	matcasc[i].xp = EMPTY;
}

/*no matcasc option*/
strcpy(matcasc[0].name,"matcasc");
matcasc[0].active = 0;
matcasc[0].tend = 0;
matcasc[0].freq = 0;
matcasc[0].nitems = 0;
matcasc[0].deltam = 0;

index = 1;

/*rest of matcasc options*/
for (i=0;i<sizeof(dur)/sizeof(int);i++)
	for (j=0;j<sizeof(attackfreq)/sizeof(int);j++)
		for (k=0;k<sizeof(fcasc)/sizeof(float);k++)
			for (n=0;n<sizeof(deltam)/sizeof(float);n++)	{
				strcpy(matcasc[index].name,"matcasc");
				matcasc[index].active = 1;
				matcasc[index].tend = dur[i];
				matcasc[index].freq = attackfreq[j];
				matcasc[index].nitems = fcasc[k];
				matcasc[index].deltam = deltam[n];
				index++;
				}

/*array to hold all repiping permutations*/
/*duration is assigned but no frequency--
assume attacker repipes in one event and then runs material through every day*/
struct aoptions repiping[REPIPING];

for(i=0;i<REPIPING;i++)	{
	repiping[i].area = EMPTY;
	repiping[i].deltam = EMPTY;
}

/*no repiping option*/
strcpy(repiping[0].name,"repiping");
repiping[0].active = 0;
repiping[0].tend = 0;
repiping[0].freq = 0;
repiping[0].nitems = 0;
repiping[0].xf = 0;
repiping[0].xp = 0;

index = 1;

/*rest of repiping options*/
for (i=0;i<sizeof(dur)/sizeof(int);i++)
	for (k=0;k<sizeof(fcasc)/sizeof(float);k++)
		for (n=0;n<sizeof(xp)/sizeof(float);n++)	{
			strcpy(repiping[index].name,"repiping");
			repiping[index].active = 1;
			repiping[index].tend = dur[i];
			repiping[index].freq = REPIPINGFREQ;
			repiping[index].nitems = fcasc[k];
			repiping[index].xf = xfeed;
			repiping[index].xp = xp[n];
			index++;
		}

/*array to hold all recycle permutations*/
struct aoptions recycle[RECYCLE];

for(i=0;i<RECYCLE;i++)	{
	recycle[i].area = EMPTY;
	recycle[i].deltam = EMPTY;
}

/*no recycle option*/
strcpy(recycle[0].name,"recycle");
recycle[0].active = 0;
recycle[0].tend = 0;
recycle[0].freq = 0;
recycle[0].nitems = 0;
recycle[0].xf = 0;
recycle[0].xp = 0;

index = 1;

/*rest of recycle options*/
for (i=0;i<sizeof(dur)/sizeof(int);i++)
	for (j=0;j<sizeof(attackfreq)/sizeof(int);j++)
		for (k=0;k<sizeof(fcasc)/sizeof(float);k++)
			for (n=0;n<sizeof(xp)/sizeof(float);n++)	{
				strcpy(recycle[index].name,"recycle");
				recycle[index].active = 1;
				recycle[index].tend = dur[i];
				recycle[index].freq = attackfreq[j];
				recycle[index].nitems = fcasc[k];
				recycle[index].xf = xprod;
				recycle[index].xp = xp[n];
				index++;
			}

/*array to hold all udfeed permutations*/
struct aoptions udfeed[UDFEED];

for (i=0;i<UDFEED;i++)	{
	udfeed[i].nitems = EMPTY;
	udfeed[i].area = EMPTY;
	udfeed[i].deltam = EMPTY;
	udfeed[i].xf = EMPTY;
	udfeed[i].xp = EMPTY;
}
	
/*no udfeed options*/
strcpy(udfeed[0].name,"udfeed");
udfeed[0].active = 0;
udfeed[0].tend = 0;
udfeed[0].freq = 0;

index = 1;  /*index through udfeed array*/

/*rest of udfeed options*/
for (j=0;j<sizeof(dur)/sizeof(int);j++)
	for (k=0;k<sizeof(attackfreq)/sizeof(int);k++)	{
		strcpy(udfeed[index].name,"udfeed");
		udfeed[index].active = 1;
		udfeed[index].tend = dur[j];
		udfeed[index].freq = attackfreq[k];
		index++;
	}

count = 0;		/*index through attstrats*/

/*generate all strategy combinations with cyltheft and store in attstrats*/
/*indexing starts at 1 to skip "no cyltheft" options bc these are strategies where
cyltheft is active; use matcyl[0] because matcyl is inactive with cyltheft is active*/

for (i=1;i<CYLTHEFT;i++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = cyltheft[i];
	count++;
	}

for (j=1;j<MATCYL;j++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = matcyl[j];
	if (attstrats[count].AO.tend >= attstrats[count].AO.freq)
		count++;
	}

for (j=1;j<MATCASC;j++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = matcasc[j];
	if (attstrats[count].AO.tend >= attstrats[count].AO.freq)
		count++;
	}

for (i=1;i<REPIPING;i++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = repiping[i];
	count++;
	}

for (j=1;j<RECYCLE;j++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = recycle[j];
	if (attstrats[count].AO.tend >= attstrats[count].AO.freq)
		count++;
	}

for (j=1;j<UDFEED;j++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = udfeed[j];
	if (attstrats[count].AO.tend >= attstrats[count].AO.freq)
		count++;
	}

}

float calcDP(struct dstrategy dstrat, struct astrategy astrat)	{

/*dstrat is a single defender strategy passed as an element of the defstrats array*/
/*astrat is a single attacker strategy passed as an element of the attstrats array*/

int t,actastrat;				/*t indexes time; j indexes att options; active attacker strat*/
int tend;						/*end day of diversion*/
int m;							/*schedule array dimensions-- detection days*/
int k;							/*indexes safeguards*/
int DP;

DP = 0;
tend = 0;

/*array of "sginfo" structures that holds information about properties inherent to SGs
order of sginfo elements MUST match order of safeguards elements in dstrat*/

struct sginfo dstratinfo[NUMSG] = {inspectioni,psealsi,ndai,dai,videotransi,asealsi,cemoi};
	
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
		/*else if it's a discrete strategy with extra analysis time*/
		else if (dstratinfo[k].cont != 1 && dstratinfo[k].analysis != EMPTY)
		/*first result takes extra days for analysis, but then freq is same*/	
			for (t=dstrat.SG[k].freq-1+dstratinfo[k].analysis;t<m;t=t+dstrat.SG[k].freq)
					dschedule[k][t] = 1;
		/*else if it's a discrete defender strategy with no analysis time*/			
		else if (dstratinfo[k].cont != 1 && dstratinfo[k].analysis == EMPTY)
			for (t=dstrat.SG[k].freq-1;t<m;t=t+dstrat.SG[k].freq)
				dschedule[k][t] = 1;		/*it's active on insp days*/
			
}
	
/*loop through time and safeguards to populate dailyDP array
		time loop	{
			safeguard loop	{
				if k is active
					if k is effective against attacker strategy
						calculate DP							*/
						
float dailyDP[NUMSG+1][m];		/*holds DP for each day for each safeguards over entire period*/
float cumdailyDP[m];			/*holds cum. DP across all SGs each day over entire period*/
float cumDP = 0;					/*cumDP across all SGs over entire simulation period*/
float inspDP = 0;					/*holds inspection DP to aggregate over insp. activities*/
float cylDP = 0;					/*per cylinder DP*/

for (k=0;k<NUMSG;k++)
	for(t=0;t<m;t++)
		dailyDP[k][t] = 0;			/*initialize dailyDP to 0*/
		
for(t=0;t<m;t++)
	cumdailyDP[t] = 0;

int inspindex[NUMSG];				/*track how many inspections have occurred*/
for (k=0;k<NUMSG;k++)
	inspindex[k] = 0;
		
/*** find active attacker strategy and assign number ***/
/** this is only used for "effstrats"-- try to think of something
more elegant**/

if (strcmp(astrat.AO.name,"cyltheft") == 0)
	actastrat = 0;
else if (strcmp(astrat.AO.name,"matcyl") == 0)
	actastrat = 1;
else if (strcmp(astrat.AO.name,"matcasc") == 0)
	actastrat = 2;
else if (strcmp(astrat.AO.name,"repiping") == 0)
	actastrat = 3;
else if (strcmp(astrat.AO.name,"recycle") == 0)
	actastrat = 4;
else if (strcmp(astrat.AO.name,"udfeed") == 0)
	actastrat = 5;
	
/*printf("%d",actastrat);*/	

for (t=0;t<m;t++)	{			/*index through days*/
	for (k=0;k<NUMSG;k++)	{	/*index through safeguards*/
		if (dschedule[k][t] == 1)	{			/*if safeguard is active that day*/
		/*	printf("\n%d active on day %d\n",k,t+1); */
				if (effstrats[k][actastrat] == 1){	/*if safeguards is effective against att. strat*/
					/*cylinder theft*/
					if (strcmp(astrat.AO.name,"cyltheft") == 0)	{	
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{	
						float invDP[m];					/*holds inventory DP-- used for HRA*/		
						/*	printf("%s\n",dstrat.SG[k].name); */
						
							if (inspindex[k] == 0)	{
							/*	printf("first inspection\n"); */
							
								/*calc inventory DP-- store as inspDP*/
								invDP[inspindex[k]] = 1-exp(-0.65*(dstrat.SG[k].dep+1)
														*astrat.AO.nitems);
								inspDP = invDP[inspindex[k]];
								
								/*calc videolog DP*/
								DP = VIDEOLOGDP;	

								/*update inspDP to include videolog DP*/			
								inspDP = 1-(1-inspDP)*(1-DP);	
								
								/*put inspDP in dailyDP*/
								dailyDP[k][t] = inspDP;			
								inspindex[k]++;					/*increment inspindex*/
							}
							/*if it's not the first inspection*/
							else	{
								/*only inventory DP-- no videolog DP*/
								invDP[inspindex[k]] = 1-((1+dstrat.SG[k].dep*(1-invDP[inspindex[k]-1]))
								/(dstrat.SG[k].dep+1));
								inspDP = invDP[inspindex[k]];
								
								dailyDP[k][t] = inspDP;			/*put inspDP in dailyDP*/
								inspindex[k]++;					/*increment inspindex*/
							}
						
						}
						if (strcmp(dstrat.SG[k].name,"videotrans")==0)	{
							if (inspindex[k] == 0)	{
								dailyDP[k][t] = VIDEOTRANSDP;
								inspindex[k]++;
							}
							inspindex[k]++;
						} 
					
					}
					/*some material from cylinder*/
					if (strcmp(astrat.AO.name,"matcyl") == 0) {	
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{
							/***first calculate videolog DP and store in inspDP***/
							
							/*use to handle HRA for per instance DP*/
							float DPlog[m/dstrat.SG[k].freq+1];	
							int i = 0;
							for (i=0;i<(m/dstrat.SG[k].freq+1);i++)
								DPlog[i] = 0;	
							DPlog[0] = VIDEOLOGDP;
					
							int tindex =0;	/*theft events between inspections index*/
							int ntevents = 0;	/*number of theft events*/
							
							/*calculate diversion events between inspections*/
							for (tindex=t-dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									ntevents++;
					
							/*calculate DP-- need to use previous per instance DP, which 
							is stored in DPlog. Find cumulative sum of per instance DPs 
							for all instances that have occured since last inpsection*/
							
							/*if it's the first inspection*/
							if (inspindex[k] == 0)		{
								inspDP = 1-pow((1-DPlog[0]),ntevents);
								inspindex[k]++;
							}
							/*if it's not the first inspection*/
							else	{
								DPlog[inspindex[k]] = 1-((1+dstrat.SG[k].dep*
									(1-DPlog[inspindex[k]-1]))/(dstrat.SG[k].dep+1)); 
								inspDP = 1-pow((1-DPlog[inspindex[k]]),ntevents);
								inspindex[k]++;
							}
							/***calc mbalance DP and combine into inspDP***/
							
							float totalevents = 0;	/*total number of theft events*/
							float totalmass = 0;
							float threshold=0;			/*threshold weight*/
							float nomreading=0;				/*nominal weight*/
							
							/*calculate total number of events*/
							for (i=0;i<=t;i++)
								if (aschedule[i] == 1)
									totalevents++;
							
							/*total mass of material stolen to date per cylinder*/
							totalmass = totalevents * astrat.AO.deltam;
							
							/*if material is stolen from feed storage*/
							if (astrat.AO.area == 0)	
								nomreading = nomcylmass[0];
							if (astrat.AO.area == 1)
								nomreading = nomcylmass[1];
								
								/*multiple erfinv value by -1 because appears in this
								expression as 2FAP-1, not 1-2FAP*/
								threshold = nomreading+sqrt(2)*(mberror*nomreading)*
								erfinv[dstrat.SG[k].fap-1]*-1;
									
								/*per cylinder DP*/
								cylDP = 1-0.5*(1-erf((threshold-(nomreading-totalmass))/
										(sqrt(2)*mberror*(nomreading-totalmass))));
								
								/*if multiple cylinders are taken, find cum DP*/
								DP = 1-pow((1-cylDP),astrat.AO.nitems);
								
								/*update inspDP*/
								inspDP = 1-(1-inspDP)*(1-DP);
								
								/*make dailyDP equal inspDP*/
								dailyDP[k][t] = inspDP;
							
						}
						if (strcmp(dstrat.SG[k].name,"pseals")==0)		{
							/*1 if attack happened since last inspection, 0 otherwise*/
							int attackhappened = 0;
							/*scan through days since last inspection and see if an 
							  attack has occurred. If so, a new detection opportunity
							  exists*/
							int i=0;
							if (inspindex[k] == 0)	{
								for (i=t+1-(dstrat.SG[k].freq+dstratinfo[k].analysis);i<t;i++)
									if (aschedule[i] == 1)	{
										attackhappened = 1;
										break;
									}
							}
							else	{
								for (i=t+1-dstrat.SG[k].freq;i<=t;i++)
									if (aschedule[i] == 1)	{
										attackhappened = 1;
										break;
									}
							}
									
							if (attackhappened == 1)	{
							
								float evasionprob;	/*evasion prob-- 1-P-- probability
												an unsealed cylinder is chosen*/
								/*N- number cyls = numcyl[0] for area = 0 and numcyl[1] for 
								area = 1
								r - violated cyls = astrat.AO.nitems
								n - sealed cyls = dstrat.SG[k].number * numcyl -- fraction
								sealed * total number*/

								evasionprob = (float)choose((numcyl[astrat.AO.area]-astrat.AO.nitems),
											dstrat.SG[k].number*numcyl[astrat.AO.area])/
											(float)choose(numcyl[astrat.AO.area],
											dstrat.SG[k].number*numcyl[astrat.AO.area]);
							
								dailyDP[k][t] = (1-evasionprob)*PSEALSDP;
								inspindex[k]++;
							}
						}
						if (strcmp(dstrat.SG[k].name,"videotrans")==0)	{
							/*video only transmitted when diversion event occurs*/
							if (aschedule[t] == 1)	{
							/*	printf("%s\n",dstrat.SG[k].name); */
								if (inspindex[k] == 0)	{ /*if this is DP initial for video trans*/
									dailyDP[k][t] = VIDEOTRANSDP;
									inspindex[k]++; 
								/*	printf("DP on day %d: %.2f\n",t+1,dailyDP[k][t]);	*/			
								}
								else 	{	/*if it isn't DP initial-- HRA*/
								/*	printf("DP: %.2f\n",dailyDP[k][t-astrat.AO.freq]); */
									dailyDP[k][t] = 1-(1+dstrat.SG[k].dep*(1-dailyDP[k]
												[t-astrat.AO.freq]))/
												(dstrat.SG[k].dep+1);
									inspindex[k]++;
								/*	printf("DP on day %d: %.2f\n",t+1,dailyDP[k][t]); */
								} 
							}					
						}
					}	
					/*some material from the cascade*/
					if (strcmp(astrat.AO.name,"matcasc") == 0) {
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{
							/*** mass balance DP ***/
							
							/*total number of theft events*/;
							float ntevents;
							ntevents = 0;
							int i;
							for (i=0;i<=t;i++)
								if (aschedule[i] == 1)
									ntevents++;
							/*printf("delta m: %.2f\n",astrat.AO.deltam); */
							/*calculate total mass taken to date*/
							float totmass = ntevents * astrat.AO.deltam * astrat.AO.nitems
											*NUMCASC;
							/*if more than one cylinder's worth is missing, detection is 
							certain*/
							if (totmass >= nomcylmass[1])
								dailyDP[k][t] = 1;
						
							else	{				
								
							/* printf("total mass: %.1f\n",totmass); */
							/*total mass stolen is events*mass/event*cascades/event-- in kg*/
							float thresh = nomcylmass[1]+sqrt(2)*(0.0007*nomcylmass[1])*
											erfinv[(dstrat.SG[k].fap)-1]*-1;
							dailyDP[k][t] = 1-0.5*(1-erf((thresh-(nomcylmass[1]-totmass))
											/(sqrt(2)*0.0007*(nomcylmass[1]-totmass))));
						/*	printf("DP is %0.4f on day %d from safeguard %s\n", 
									dailyDP[k][t],t+1,dstrat.SG[k].name); */
							inspindex[k]++;
							}
						}	
						if (strcmp(dstrat.SG[k].name,"aseals")==0)	{
							/*aseals only detect theft on day theft occurs*/
							if (aschedule[t] == 1)	{
								/*only first break of seal has DP*/
								if (inspindex[k] == 0)	{
									if (dstrat.SG[k].number < 1)
										dailyDP[k][t] = 0;
									else	{
									/*# seals broken = frac cascade tapped* num cascades*/
									int nseals = astrat.AO.nitems * NUMCASC;	
									/*printf("seals broken: %d\n",nseals); */
									dailyDP[k][t] = 1 - pow((1-ASEALSDP),(nseals));
									/*printf("DP is %0.3f on day %d from safeguard %s\n",
										dailyDP[k][t],t+1,dstrat.SG[k].name); */
									inspindex[k]++;
									}	
								}				
						}
					}
					}
					/*repiping*/
					if (strcmp(astrat.AO.name,"repiping") == 0)	{
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{
						/*** mass balance DP ***/
						float deltam;	/*difference, nominal, anomalous
														mass flows*/
						float dailynommass,dailydivmass;
										
						/*nominal daily mass flow-- how much would the operator ordinarily 
						make using that amount of capacity?*/
						dailynommass = calcMassFlow(xfeed,xprod,xw,astrat.AO.nitems);
						
						/*daily mass flow for div scenario*/
						dailydivmass = calcMassFlow(astrat.AO.xf,astrat.AO.xp,xw,
									astrat.AO.nitems); 
									
						float totalevents = 0;	/*total number of theft events*/
						float threshold = 0;			/*threshold weight*/
						float nomreading = 0;				/*nominal weight*/
							
						/*"event" occurs every day*/
						int i;
						for (i=0;i<=t;i++)
							if (aschedule[i] == 1)
								totalevents++;
						
						deltam = (dailynommass - dailydivmass)*totalevents;
							
						/*what the balance should read-- weight of cylinder + material
						  for full product cylinder*/
						nomreading = nomcylmass[1];
						
						/*if more than one cylinder's worth is missing, detection is 
						certain*/
						if (deltam >= nomreading)
							inspDP = 1;
						
						else	{
								
						/*multiple erfinv value by -1 because appears in this
						expression as 2FAP-1, not 1-2FAP*/
						threshold = nomreading+sqrt(2)*(mberror*nomreading)*
								erfinv[dstrat.SG[k].fap-1]*-1;
									
						/*DP for cylinder with missing material*/
						DP = 1-0.5*(1-erf((threshold-(nomreading-deltam))/
								(sqrt(2)*mberror*(nomreading-deltam))));
								
						/*inspDP is only mass balance DP for this attacker strategy*/
						inspDP = DP;
						}
								
						/*make dailyDP equal inspDP*/
						dailyDP[k][t] = inspDP;
						
					}
						if (strcmp(dstrat.SG[k].name,"nda")==0)	{
							float thresh;	/*thresh- threshold value used in calc*/
							int stddev;	/*standard deviation*/
							int peakcounts;	/*total counts in 186-keV peak*/
							
							/*total counts = CR * count time*/
							peakcounts = nomprod * ndacounttime;
							stddev = ndaerror * peakcounts;
							
							float fapvalue;		/*holds the value of the erf^-1(1-2*FAP)*/
							
							if (dstrat.SG[k].fap == 1)
								fapvalue = erfinv[0];
							if (dstrat.SG[k].fap == 2)
								fapvalue = erfinv[1];
							
							/*detection threshold*/
							thresh = peakcounts + sqrt(2)*stddev*fapvalue;
							
							dailyDP[k][t] = 1- 0.5*(1+erf((thresh-astrat.AO.xp/xprod*peakcounts)/
											(sqrt(2)*astrat.AO.xp/xprod*stddev))); 
						}	
						if (strcmp(dstrat.SG[k].name,"da")==0)	{
							/*nondetection prob (1-DP)*/
							float nonDP;
							
							/* N = numcyl[1]-- number cylinders in product storage
							   r = NUMCYLFALSE-- assume all overenriched product placed in 
							   		one storage cylinder
							   n = dstrat.SG[k].number*ncyl[1]-- percentage cylinders
							   		sampled times number cylinders */
							nonDP = (float)choose(numcyl[1]-NUMCYLFALSE,dstrat.SG[k].number*numcyl[1])/
									(float)choose(numcyl[1],dstrat.SG[k].number*numcyl[1]);
							
						
						/*	printf("num: %.3f\tden:%.3f\n",num,den);*/
							dailyDP[k][t] = 1-nonDP;
							
						/*	printf("DP is %0.3f on day %d from safeguard %s\n",
										dailyDP[k][t],t+1,dstrat.SG[k].name); */
							inspindex[k]++;
						
						}
						if (strcmp(dstrat.SG[k].name,"aseals")==0)	{
							if (aschedule[t] == 1)	{
								/*seal only broken on first attack*/
								if (inspindex[k] == 0)	{
									if (dstrat.SG[k].number < 1)
										dailyDP[k][t] = 0;
									else	{
										float numsealsviolated;	/*cascades attacker 
														violates to perpetrate attack*/
								
										/*seals violated equals fraction cascades violated
										times number of cascades (assume one seal per cascade)*/
										numsealsviolated = astrat.AO.nitems*NUMCASC;
								
										/*DP is cumulative DP with DP=0.4 for each seal violated*/
										dailyDP[k][t] = 1-pow((1-ASEALSDP),numsealsviolated);
										inspindex[k]++;
									}
								}
							}
						}	
						if (strcmp(dstrat.SG[k].name,"cemo")==0)	{
							/*CEMO is go/no go, so there is no detection of LEU*/
							if (aschedule[t] == 1)	{
							if (astrat.AO.xp < 0.20)
								dailyDP[k][t] = 0;
							else	{
						
								float nomcounts;	/*nominal counts in 186-keV peak*/
								float signal;			/*observed counts in peak*/
								float threshold;		/*detection threshold*/
								float stddev;			/*standard deviation*/
								float stddevsig;		/*std deviation for signal*/
							
								/*total peak counts over count time*/
								nomcounts = nomCRcemo * dstrat.SG[k].tcount;
								
							
								/*signal peak counts for overly enriched U*/
								signal = astrat.AO.xp/xprod*nomcounts;
								
								
								stddev = sqrt(nomcounts);
								stddevsig = sqrt(signal);
								
								threshold = nomcounts + sqrt(2)*stddev*
											erfinv[dstrat.SG[k].fap-1];
								
								
								dailyDP[k][t] = 1-0.5*(1+erf((threshold-signal)/
												(sqrt(2)*stddevsig)));
							}
							}
						}
					}	
					if (strcmp(astrat.AO.name,"recycle") == 0)	{
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{
						/*** mass balance DP ***/
						float totalevents=0;
						float nomreading;			/*nominal mass of product cylinder*/
						float feedmass;				/*product mass used as feed*/
						float newproductmass;		/*mass of overly enriched product*/
						float threshold;
						
						float signal;				/*reading on balance for cylinder with
													missing product*/
													
						nomreading = nomcylmass[1];	/*full product cylinder mass*/
						
						/*calculate number of recycle events that have occurred*/
						int i;
						for (i=0;i<=t;i++)
							if (aschedule[i] == 1)
								totalevents++;
							
						/*printf("total events: %.0f\n",totalevents);
						/*total mass taken from product cylinder and used for recycle feed
						is mass used each time * number of times recycle occurred*/
						feedmass = calcFeedMass(astrat.AO.xf,astrat.AO.xp,xw,
												astrat.AO.nitems)*totalevents;
						/*printf("feed mass: %.3f\n",feedmass);
												
						/*total mass produced by recycle is mass produced from each 
						recycle * number of times recycle occurred*/
						newproductmass = calcMassFlow(astrat.AO.xf,astrat.AO.xp,xw,
									astrat.AO.nitems) * totalevents; 
						
						/*if you need more mass than is in one product cylinder, attacker
						definitely gets caught*/			
						if (feedmass > nomreading)
							DP = 1;
						
						else	{
						
						/*signal is mass of full cylinder minus material removed to be
						used as feed*/
						signal = nomreading - feedmass;
								
						/*multiple erfinv value by -1 because appears in this
						expression as 2FAP-1, not 1-2FAP*/
						threshold = nomreading+sqrt(2)*(mberror*nomreading)*
								erfinv[dstrat.SG[k].fap-1]*-1;
									
						/*DP for cylinder with missing material*/
						DP = 1-0.5*(1-erf((threshold-signal)/
								(sqrt(2)*mberror*signal)));
						}
								
						/*inspDP is only mass balance DP for this attacker strategy*/
						inspDP = DP;
								
						/*make dailyDP equal inspDP*/
						dailyDP[k][t] = inspDP;
						
					}
						if (strcmp(dstrat.SG[k].name,"nda")==0)	{
							
							float thresh;	/*thresh- threshold value used in calc*/
							float stddev;	/*standard deviation*/
							float peakcounts;	/*total counts in 186-keV peak*/
							
							/*total counts = CR * count time*/
							peakcounts = nomprod * ndacounttime;
							stddev = ndaerror * peakcounts;
							
							float fapvalue;		/*holds the value of the erf^-1(1-2*FAP)*/
							
							if (dstrat.SG[k].fap == 1)
								fapvalue = erfinv[0];
							if (dstrat.SG[k].fap == 2)
								fapvalue = erfinv[1];
							
							/*detection threshold*/
							thresh = peakcounts + sqrt(2)*stddev*fapvalue;
							
							dailyDP[k][t] = 1- 0.5*(1+erf((thresh-astrat.AO.xp/xprod*peakcounts)/
											(sqrt(2)*astrat.AO.xp/xprod*stddev)));
											
											
						}
						if (strcmp(dstrat.SG[k].name,"da")==0)	{
							/*nondetection prob (1-DP)*/
							float nonDP;
							
							/* N = numcyl[1]-- number cylinders in product storage
							   r = NUMCYLFALSE-- assume all overenriched product placed in 
							   		one storage cylinder
							   n = dstrat.SG[k].number*ncyl[1]-- percentage cylinders
							   		sampled times number cylinders */
							nonDP = (float)choose(numcyl[1]-NUMCYLFALSE,dstrat.SG[k].number*numcyl[1])/
									(float)choose(numcyl[1],dstrat.SG[k].number*numcyl[1]);
									
							/*printf("detection %d\n",inspindex[k]+1);*/
						
							dailyDP[k][t] = 1-nonDP;
							inspindex[k]++;
						
						} 
						if (strcmp(dstrat.SG[k].name,"cemo")==0)	{
							/*CEMO is go/no go, so there is no detection of LEU*/
							if (aschedule[t] == 1)	{
							if (astrat.AO.xp < 0.20)
								dailyDP[k][t] = 0;
							else	{
						
								float nomcounts;	/*nominal counts in 186-keV peak*/
								float signal;			/*observed counts in peak*/
								float threshold;		/*detection threshold*/
								float stddev;			/*standard deviation*/
								float stddevsig;		/*std deviation for signal*/
							
								/*total peak counts over count time*/
								nomcounts = nomCRcemo * dstrat.SG[k].tcount;
								
							
								/*signal peak counts for overly enriched U*/
								signal = astrat.AO.xp/xprod*nomcounts;
								
								
								stddev = sqrt(nomcounts);
								stddevsig = sqrt(signal);
								
								threshold = nomcounts + sqrt(2)*stddev*
											erfinv[dstrat.SG[k].fap-1];
								
								
								dailyDP[k][t] = 1-0.5*(1+erf((threshold-signal)/
												(sqrt(2)*stddevsig)));
							}
							}
						}	
					}
						
					/*undeclared feed-- TEMPORARY UNTIL RANDOM INSPS*/
					if (strcmp(astrat.AO.name,"udfeed") == 0)	{
						dailyDP[k][t]=0.90;
						
				}
			}
		}
	}
/*the bg DP occurs every day*/
dailyDP[NUMSG][t] = bgDP;
}

/*calculate cumulative DP for each SG*/
float cumSGDP[NUMSG+1];
for (k=0;k<NUMSG;k++)
	cumSGDP[k] = intelDP[k];
cumSGDP[NUMSG] == 0;
for (k=0;k<NUMSG+1;k++)
	for (t=0;t<m;t++)	
		cumSGDP[k] = 1-(1-cumSGDP[k])*(1-dailyDP[k][t]);
printf("cumSGDP: %.3f\n",cumSGDP[NUMSG]);

		
/*calculate cumulative DP across all SGs on each day*/

/*initialize array to 0's*/
for (t=0;t<m;t++)
	cumdailyDP[t] = 0;
/*populate with daily cumulative DP*/
for (t=0;t<m;t++)	{
	for (k=0;k<NUMSG+1;k++)
		cumdailyDP[t] = 1-(1-cumdailyDP[t])*(1-dailyDP[k][t]);
}

/*initialize value to 0*/
cumDP = 0;

/*calculate cumulative DP over entire simulation period using cumSGDP*/
for (k=0;k<NUMSG+1;k++)
	cumDP = 1-(1-cumDP)*(1-cumSGDP[k]);

/*calculate cumulative DP over entire simulation period-- using cumdailyDP*/
/*for (t=0;t<m;t++)
	cumDP = 1-(1-cumDP)*(1-cumdailyDP[t]);*/
	
return cumDP;

}	

/*function calculates and returns the cost of a safeguarding strategy*/

long calcCost(struct dstrategy dstrat,struct astrategy astrat)	{

/*use to retrieve analysis info*/
struct sginfo dstratinfo[NUMSG] = {inspectioni,psealsi,ndai,dai,videotransi,asealsi,cemoi};

long totalcost = 0;	/*total cost of strategy*/
long insp=0;
long pseals=0;
long nda=0;
long da=0;
long videotrans=0;
long aseals=0;
long cemo=0;

/*number of inspections for strategy*/
int numinsp = 0;

/*cost for inspection*/
if (dstrat.SG[0].active == 1)	{
	/*number of inspections required for entire year of strategy*/
	numinsp = floor((SIMYEAR)/dstrat.SG[0].freq);
	/*time costs*/
	insp = INSPCOST*numinsp;

	if (dstrat.SG[0].dep == dep[1])	{
		insp = insp * DEPMULT;
	}
		
	if (dstrat.SG[0].fap == fap[0])
		insp = insp * FAPMULT;

	/*add in equipment costs*/
	insp = insp + MBCOST + VLCOST;
	
}
/*cost for pseals*/
if (dstrat.SG[1].active == 1)	{
	
	/*number of inspections required for entire year of strategy-- account for the fact
	that first insp doens't occur until freq+analysis days after start of simyear*/
	
	numinsp = 1+ floor((SIMYEAR-(dstrat.SG[1].freq+dstratinfo[1].analysis))/dstrat.SG[1].freq);
	
	/*time costs*/
	pseals = ADDINSPCOST*numinsp;
		
	/*add in analysis cost-- rate per batch (2.5) times number of batches (numinsp)
	if only half are verified, multiply by 0.5*/
	pseals = pseals + dstrat.SG[1].number * ANALYSISCOST * numinsp;
		
	/*add in per seal costs-- assume you have to pay to change seals all year*/ 

	pseals = pseals + sealcost * numinsp * (numcyl[0] + numcyl[1]);
				
}
/*nda cost*/
if (dstrat.SG[2].active == 1)	{
	
	/*number of inspections required for entire year of strategy*/
	numinsp = floor(SIMYEAR/dstrat.SG[2].freq);
	
	/*time costs*/
	nda = ADDINSPCOST*numinsp;
	
	if (dstrat.SG[2].fap == fap[0])
		nda = nda * FAPMULT;
		
	/*fixed cost*/
	nda = nda + NDACOST;
	
}
/*da cost*/
if (dstrat.SG[3].active == 1)	{
	
	/*number of inspections required for entire year of strategy-- account for the fact
	that first insp doens't occur until freq+analysis days after start of simyear*/
	
	numinsp = 1+ floor((SIMYEAR-(dstrat.SG[3].freq+dstratinfo[3].analysis))/dstrat.SG[3].freq);
	
	/*time cost*/
	da = ADDINSPCOST*numinsp;
		
	/*analysis cost-- if half cylinders are sampled, pay full 2.5; if 1/8 cylinders 
	are sampled, pay 1.25*/
	if (dstrat.SG[3].number == DAsamples[0])
		da = da + 0.5 * ANALYSISCOST * numinsp;
	if (dstrat.SG[3].number == DAsamples[1])
		da = da + ANALYSISCOST * numinsp;
				
	/*fixed costs*/
	da = da + DACOST;
	
}
/*videotrans cost*/
if (dstrat.SG[4].active == 1)	{
	
	/*number of days of assessment in one year*/
	numinsp = SIMYEAR;
	
	/*time cost*/
	videotrans = ASSESSCOST*numinsp;
	if (dstrat.SG[4].dep == dep[1])
		videotrans = videotrans * DEPMULT;
		
	/*fixed cost*/
	videotrans = videotrans + VTCOST;
	
}
/*active seals cost*/
if (dstrat.SG[5].active == 1)	{
	
	/*number of days of assessment in one year*/
	numinsp = SIMYEAR;
	
	/*time cost*/
	aseals = ASSESSCOST * numinsp;
	
	/*seal cost-- buy enough to seal each cascade twice*/
	aseals = activesealcost * 2 * dstrat.SG[5].number * NUMCASC;

	
}
/*CEMO cost*/
if (dstrat.SG[6].active == 1)	{
	
	/*number of days of assessment in one year*/
	numinsp = SIMYEAR;
	
	/*time cost*/
	cemo = ASSESSCOST*numinsp;
	if (dstrat.SG[6].fap == fap[0])
		cemo = cemo * FAPMULTCEMO;
		
	/*fixed cost*/
	cemo = cemo + CEMOCOST;
	
}

totalcost = insp + pseals + nda + da + videotrans + aseals + cemo;

return totalcost;

}


/*function prints defender strategy*/
void printDStrat(struct dstrategy dstrat)	{

/*scan through every SG-- if it's active, print define parameters*/
int i;

printf("strategy %d:\n",dstrat.uniqueID);
for (i=0;i<NUMSG;i++)
	if (dstrat.SG[i].active == 1)	{
		printf("SG%d: %s\n",i,dstrat.SG[i].name);
		if (dstrat.SG[i].fap != EMPTY)	{
			if (dstrat.SG[i].fap == fap[0])
				printf("\tFAP: 0.01\n");
			else
				printf("\tFAP: 0.001\n");
		}
		if (dstrat.SG[i].number != EMPTY)	{
			if (strcmp(dstrat.SG[i].name,"pseals") == 0)	
				printf("\tfraction cylinders sealed: %.2f\n",dstrat.SG[i].number);
			else if (strcmp(dstrat.SG[i].name,"da") == 0)	{
				printf("\tfraction cylinders sampled: %.2f\n",dstrat.SG[i].number);
			}
			else if (strcmp(dstrat.SG[i].name,"aseals") == 0)	{
				printf("\tfraction cascades sealed: %.2f\n",dstrat.SG[i].number);
			}
		}
		if (dstrat.SG[i].tcount != EMPTY)
			printf("\tcount time: %d sec\n",dstrat.SG[i].tcount);
		if (dstrat.SG[i].freq != EMPTY)
			printf("\tfreq: %d 1/days\n",dstrat.SG[i].freq);
		if (dstrat.SG[i].dep != EMPTY)	{
			if (dstrat.SG[i].dep == dep[0])
				printf("\tsize of inspection team: small\n");
			else
				printf("\tsize of inspection team: large\n");
		}
	}

}	

void printAStrat(struct astrategy astrat)	{	

/*print parameters for active AO*/

printf("strategy %d:\n",astrat.uniqueID);
	printf("AO: %s\n",astrat.AO.name);
	if (astrat.AO.tend != EMPTY)
		printf("\tduration: %d days\n",astrat.AO.tend);
	if (astrat.AO.freq != EMPTY)
		printf("\tfreq: %d 1/days\n",astrat.AO.freq);
	if (astrat.AO.nitems != EMPTY)	{
		if (astrat.AO.nitems == fcasc[0] || astrat.AO.nitems == fcasc[1] ||
			astrat.AO.nitems == fcasc[2])
			printf("\tfraction cascades dedicated: %.2f\n",astrat.AO.nitems);
		else
			printf("\tcylinders: %.2f\n",astrat.AO.nitems);
	}
	if (astrat.AO.area != EMPTY)	{
		if (astrat.AO.area == 0)
			printf("\tarea: feed storage\n");
		else
			printf("\tarea: product storage\n");
	}
	if (astrat.AO.deltam != EMPTY)
		printf("\tmass taken: %.2f\n",astrat.AO.deltam);
	if (astrat.AO.xp != EMPTY)
		printf("\txp: %.2f\n",astrat.AO.xp);
	if (astrat.AO.xf != EMPTY)
		printf("\txf: %.2f\n",astrat.AO.xf);
}	

/*function takes feed, product, and tails assay as input, as well as
fraction of cascades dedicated to production, and outputs DAILY product
mass flow*/

float calcMassFlow(float xf,float xp, float xw, float fraccasc)	{

float P;	/*product flow rate-- kg/day*/
float F;

float Vw,Vf,Vp;

Vw = (2*xw-1)*log(xw/(1-xw));
Vp = (2*xp-1)*log(xp/(1-xp));
Vf = (2*xf-1)*log(xf/(1-xf));

/*find daily SWU-- use 360 bc that's one "sim year"*/
float dailySWU = SWU/360;

/*find fraction of cascaded dedicated to diversion or fraccasc = 1 for nominal*/
float activeSWU = fraccasc*dailySWU;

/*daily mass flows*/
F = activeSWU*1/((xf-xp)/(xw-xp)*Vw + (1-(xf-xp)/(xw-xp))*Vp - Vf);
P = F*(1-(xf-xp)/(xw-xp));

return P;

}

/*function takes feed, product, and tails assay as input, as well as
fraction of cascades dedicated to production, and outputs DAILY feed
mass flow*/
float calcFeedMass(float xf,float xp, float xw, float fraccasc)	{

float P;	/*product flow rate-- kg/day*/
float F;

float Vw,Vf,Vp;

Vw = (2*xw-1)*log(xw/(1-xw));
Vp = (2*xp-1)*log(xp/(1-xp));
Vf = (2*xf-1)*log(xf/(1-xf));

/*find daily SWU-- use 360 bc that's one "sim year"*/
float dailySWU = SWU/360;

/*find fraction of cascaded dedicated to diversion or fraccasc = 1 for nominal*/
float activeSWU = fraccasc*dailySWU;

/*daily mass flows*/
F = activeSWU*1/((xf-xp)/(xw-xp)*Vw + (1-(xf-xp)/(xw-xp))*Vp - Vf);
P = F*(1-(xf-xp)/(xw-xp));

return F;

}

long factorial(int n)
{
  int c;
  long result = 1;
 
  for (c = 1; c <= n; c++)
    result = result * c;
 
  return result;
}

long choose(int a, int b)	{
	if (b == 0)
		return 1;
	else	
		return (factorial(a)/(factorial(b)*factorial(a-b)));
}
	