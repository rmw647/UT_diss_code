/*mini model used to demonstrate proof-of-concept*/
/*2-3 SGs and 2-3 attacker options with DPs calculated and FP coupled*/
/*defender and attacker strategies are generated in separate fcn, not main*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

/**** successfully generates cumulative DPs ****/

/*structure "safeguard" holds tunable parameters for each safeguards that the 
defender selects; structure "sginfo" holds parameters inherent to that SG that cannot be
altered by the defender*/

#define INSP		5		/*permutations of inspection*/
#define PSEALS		5		/*permutations of pseals*/
#define NDA			5		/*permutations of nda*/
#define DA			5		/*permutations of da*/
#define VIDEOTRANS	3		/*permutations of videotrans*/
#define ASEALS		3		/*permutations of aseals*/
#define CEMO		5		/*permutations of cemo*/

#define CYLTHEFT	5		/*permutations of cylinder theft*/
#define MATCYL		49		/*permutations of matcyl*/
#define MATCASC		37		/*permutations of matcasc*/
#define REPIPING	55		/*permutations of repiping*/
#define RECYCLE		73		/*permutations of recycle*/
#define UDFEED		7		/*permutations of udfeed*/
#define REPIPINGFREQ	1	/*repiping frequency MUST be every day--always observable*/

#define BUDGET		300		/*defender budget*/
#define EMPTY		-1		/*use to initialize payoff matrix-- -1 means DP not yet calculated*/
#define OVERBUDGET	-2		/*use in payoff matrix when defender can't afford row*/	
#define BASECOST	1		/*base cost of inspection*/
#define BASEFREQ	7		/*frequency of $1 SG option*/
#define BASEDEP		1		/*dependency of $1 SG option*/

#define MAXDSTRATS	13725
#define MAXASTRATS	193
#define	NUMSG		7		/*number of safeguards*/
#define NUMAO		6		/*number of attacker options*/
#define EXTRADAYS	30		/*number of days past end of diversion simulation runs*/
#define	NUMCASC		60.0		/*number of cascades at facility*/

#define VIDEOLOGDP	0.43	/*initial DP for videolog*/
#define VIDEOTRANSDP	0.64	/*initial DP for videotrans*/
#define PSEALSDP	0.40	/*DP for a single passive seal*/
#define STARTUP		100		/*use for now to differentiate per sample and equip costs*/

#define IT			1000		/*number of iterations for FP algorithm*/

float xprod = 0.045;			/*nominal product enrichment*/

float numcyl[2] = {16,10};		/*number of cylinders in feed and product storage, resp*/
float nomcylmass[2] = {27360,5180};	/*feed and product nominal cylinder weights (kg)*/
float erfinv[2] = {1.6450,2.1851};	/*inverf(1-2*FAP) for FAP = 0.01, 0.001*/

/*NDA CONSTANTS*/
float nomprod = 88;			/*nominal CR (cps) for nda on product cylinder (includes bg)*/
float ndaerror = 0.04;		/*standard error for nda on product cylinder*/
float ndacounttime = 300;		/*assume a count time of 300 seconds for nda*/

int depmult = 3;	/*used to calculate cost-- lower dependency costs 3x more*/

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
/* NOTE: if diversion from cylinder w/ replacement is considered
DA will be effective against matcyl*/
/*right now EVERYTHING is "effective" against udfeed until random inspections are in*/
int effstrats[NUMSG][NUMAO] = { {1,1,1,1,1,1},{0,1,0,1,1,1},{0,1,0,1,1,1},{0,0,0,1,1,1},
								{1,1,0,0,0,1},{0,1,1,1,0,1},{0,0,0,1,1,1} };

/*allowable values for defender parameters; game picks from these values*/
int fap[2] = {1,2};				/*1 = 0.01, 2 = 0.001*/	
float numseals[2] = {0.5,1};	/*fraction of cylinders sealed-- used for "number"*/
float DAsamples[2] = {0.1,0.5};	/*fraction cylinders sampled with DA-- used for "number"*/
int tcount[2] = {300,3600};		/*count time*/
int freq[2] = {7,30};			/*frequency options*/
int dep[2] = {1,19};			/*dependency options*/
	
/*allowable values for attacker parameters; game picks from these values*/
int dur[3] = {7,30,360};				/*attack duration*/
int attackfreq[2] = {7,30};				/*frequency of attack*/
float ncyl[2] = {1,2};					/*number of cylinders*/
float fcasc[3] = {1/NUMCASC,0.1,0.5};	/*fraction cascades tapped or repiped*/
int area[2] = {0,1};					/*0-feed,1-product*/
float deltam[2] = {10,100};				/*grams/item/instance (as3)*/
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

/*function called by fp to calc DP-- arguments are defender and attacker strategies 
and frequency and dependency for scheduled and random inspections*/
float calcDP(struct dstrategy, struct astrategy);		/*function prototype*/

/*function takes single defender strategy and returns integer cost value*/
int calcCost(struct dstrategy dstrat);

/*functions that generate an array of defender and attacker strategies, respectively*/
void genDStrats(struct dstrategy []);			/*function prototype*/
void genAStrats(struct astrategy []);			/*function prototype*/

/*function that prints defender and attacker strategies*/
void printDStrat(struct dstrategy dstrat, int cost);		/*function prototype*/
void printAStrat(struct astrategy astrat);		/*function prototype*/

/*variables used for the FP algorithm*/
/*allocate memory here so that they aren't on "stack"*/

int x[MAXDSTRATS];	/*hold defender strategy history*/
int y[MAXASTRATS];	/*holds attacker strategy history*/
float min[MAXDSTRATS];	/*holds columns*/
float max[MAXASTRATS];	/*holds rows*/
	/*payoff matrix- populated by simulation calls*/

struct dstrategy defstrats[MAXDSTRATS];
struct astrategy attstrats[MAXASTRATS];

float minmax, maxmin;


/****************** MAIN *********************/

int main(void)	{

int i,j,k;				/*random indices*/
int calcDPcalls = 0;	/*track how many times code calls calcDP*/


/*generate defender strategy array*/

genDStrats(defstrats);		/*pass pointer to genDStrats to manipulate defstrats*/

/*print all strategy combinations to test*/
/*for (i=0;i<MAXDSTRATS;i++)	
	printf("SG1:%d SG2:%d SG3:%d SG4:%d SG5:%d SG6:%d SG7:%d\n",
			defstrats[i].SG[0].active,defstrats[i].SG[1].active,defstrats[i].SG[2].active,
			defstrats[i].SG[3].active,defstrats[i].SG[4].active,defstrats[i].SG[5].active,
			defstrats[i].SG[6].active);	*/
printf("\n\n"); 

/*generate attacker strategy array*/

genAStrats(attstrats);

printf("%f\n\n",1/NUMCASC);

/*print all strategy combinations to test*/
for (i=0;i<MAXASTRATS;i++)	
	printf("strategy:%d AO:%s dur:%d freq:%d nitems:%.3f area:%d deltam:%.0f\n\n",
			attstrats[i].uniqueID,attstrats[i].AO.name,attstrats[i].AO.tend,
			attstrats[i].AO.freq,attstrats[i].AO.nitems,attstrats[i].AO.area,attstrats[i].AO.deltam);

printf("so far, so good!\n\n");

float DP;

start = clock();		/*used to calculate time for fp algorithm to run*/

int count=0;

for (i=0;i<MAXDSTRATS;i++)
	if (calcCost(defstrats[i]) <= BUDGET)
		count++;
		
float payoff[count][MAXASTRATS];

printf("count:%d\n",count);
int index = 0;

for (i=0;i<MAXDSTRATS;i++)
	if (calcCost(defstrats[i]) <= BUDGET)	{
		printf("%d\n",i);
		for (j=0;j<MAXASTRATS;j++)	{
			payoff[index][j] = calcDP(defstrats[i],attstrats[j]); 
		}
		index++;
	}
		
for (j=0;j<MAXASTRATS;j++)	{
	for (i=0;i<count;i++)
		printf("%.3f\t",payoff[i][j]);
	printf("\n"); 
}

end = clock();

cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC; 

printf("time: %.6f\n",cpu_time_used);

printf("value: %.3f\n",calcDP(defstrats[30],attstrats[20]));
printf("value: %.3f\n",calcDP(defstrats[30],attstrats[74]));
printf("value: %.3f\n",calcDP(defstrats[8550],attstrats[20]));
printf("value: %.3f\n",calcDP(defstrats[8550],attstrats[74]));

		
/*for (i=0;i<MAXDSTRATS;i++)
	min[i] = 100;

for (i=0;i<MAXDSTRATS;i++)
	for (j=0;j<MAXASTRATS;j++)
		if (payoff[i][j] < min[i])
			min[i] = payoff[i][j];
	
for (i=0;i<3;i++)			
	printf("min[%d]: %.3f\n",i,min[i]); 

maxmin = 0;
int row;

for (i=0;i<MAXDSTRATS;i++)
	if (min[i]>maxmin)	{
		maxmin = min[i];
		row  = i;
	}
		
printf("max:%.3f\trow:%d\n",maxmin,row); */

}

void genDStrats(struct dstrategy defstrats[])	{

/***** generate all permutations for available SGs *****/

int count = 0;			/*counts in strategy array*/
int i,j,k;				/*random indices*/

/*array to hold all inspection permutations*/
/*inspection includes inventory, mbalance and videolog*/
struct safeguards inspection[INSP];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<INSP;i++)	{
	inspection[i].fap = EMPTY;
	inspection[i].number = EMPTY;
	inspection[i].tcount = EMPTY;
}
/*no inspection option*/
strcpy(inspection[0].name,"inspection");
inspection[0].active = 0;
inspection[0].freq = 0;
inspection[0].dep = 0;

int index = 1;			/*counts in inspection array*/

/*rest of inspection options*/
for (j=0;j<sizeof(freq)/sizeof(int);j++)
	for (k=0;k<sizeof(dep)/sizeof(int);k++)	{
		strcpy(inspection[index].name,"inspection");
		inspection[index].active = 1;
		inspection[index].freq = freq[j];
		inspection[index].dep = dep[k];
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
							if (defstrats[count].SG[1].freq > defstrats[count].SG[0].freq|
							    defstrats[count].SG[2].freq > defstrats[count].SG[0].freq|
							    defstrats[count].SG[3].freq > defstrats[count].SG[0].freq)
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
int i,j,k;				/*random indices*/

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
int m,n;	/*extra indices*/

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
		for (m=0;m<sizeof(xf)/sizeof(float);m++)
			for (n=0;n<sizeof(xp)/sizeof(float);n++)	{
				strcpy(repiping[index].name,"repiping");
				repiping[index].active = 1;
				repiping[index].tend = dur[i];
				repiping[index].freq = REPIPINGFREQ;
				repiping[index].nitems = fcasc[k];
				repiping[index].xf = xf[m];
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
		for (k=0;k<sizeof(ncyl)/sizeof(int);k++)
			for (m=0;m<sizeof(xf)/sizeof(float);m++)
				for (n=0;n<sizeof(xp)/sizeof(float);n++)	{
					strcpy(recycle[index].name,"recycle");
					recycle[index].active = 1;
					recycle[index].tend = dur[i];
					recycle[index].freq = attackfreq[j];
					recycle[index].nitems = ncyl[k];
					recycle[index].xf = xf[m];
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

int t,j,actastrat;				/*t indexes time; j indexes att options; active attacker strat*/
int tend;						/*end day of diversion*/
int m;							/*schedule array dimensions-- detection days*/
int k;							/*indexes safeguards*/
int DP;

DP = 0;

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
		for(t=0;t<=tend;t=t+astrat.AO.freq)	/*activity of first day and repeating every f days (f=freq) */
			aschedule[t] = 1;
			
/*** print to check ***/

/*	printf("attacker schedule:\n");
	printf("%-12s","day");
	for (t=0;t<m;t++)	
		printf("%-3d",t+1);
	printf("\n");
	printf("%-12s","activity");
	for (t=0;t<m;t++)
		printf("%-3d",aschedule[t]);
	printf("\n");  */
						
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
			for (t=dstrat.SG[k].freq-1+dstratinfo[k].analysis;t<m;t=t+dstrat.SG[k].freq
				+dstratinfo[k].analysis)
					dschedule[k][t] = 1;
		/*else if it's a discrete defender strategy with no analysis time*/			
		else if (dstratinfo[k].cont != 1 && dstratinfo[k].analysis == EMPTY)
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
float inspDP;					/*holds inspection DP to aggregate over insp. activities*/


for (k=0;k<NUMSG;k++)
	for(t=0;t<m;t++)
		dailyDP[k][t] = 0;			/*initialize dailyDP to 0*/

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
						/*	printf("%s\n",dstrat.SG[k].name); */
							if (inspindex[k] == 0)	{
							/*	printf("first inspection\n"); */
								/*calc inventory DP*/
								inspDP = 1-exp(-0.65*(dstrat.SG[k].dep+1)*astrat.AO.nitems);
								
								DP = VIDEOLOGDP;				/*calc videolog DP*/
								inspDP = 1-(1-inspDP)*(1-DP);	/*update inspDP*/
								
								dailyDP[k][t] = inspDP;			/*put inspDP in dailyDP*/
								inspindex[k]++;					/*increment inspindex*/
							}
							/*if it's not the first inspection*/
							else	{
								inspDP = 1-((1+dstrat.SG[k].dep*(1-dailyDP[k][t-dstrat.SG[k].freq]))
								/(dstrat.SG[k].dep+1));
								
								dailyDP[k][t] = inspDP;
								inspindex[k]++;
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
						/**** NEED TO INCORPORATE MBALANCE ****/
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{
							float DPlog[m/dstrat.SG[k].freq+1];		/*use to handle HRA for per instance DP*/
							DPlog[0] = VIDEOLOGDP;
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
							/*total number of theft events*/;
							float ntevents;
							if (t<=tend)	
								ntevents = ceil((t+1)/astrat.AO.freq);
							else
								ntevents = ceil(tend/astrat.AO.freq)+1;
							/* printf("%d\n",t);
							printf("theft events: %.0f\n",ntevents);
							printf("delta m: %.2f\n",astrat.AO.deltam); */
							float totmass = ntevents * astrat.AO.deltam * astrat.AO.nitems
											*NUMCASC;
							/* printf("total mass: %.1f\n",totmass); */
							/*total mass stolen is events*mass/event*cascades/event-- in grams*/
							float thresh = nomcylmass[1]+sqrt(2)*(0.0007*nomcylmass[1])*
											erfinv[(dstrat.SG[k].fap)-1];
							dailyDP[k][t] = 1-0.5*(1-erf((thresh-(nomcylmass[1]-totmass/1000))
											/(sqrt(2)*0.0007*(nomcylmass[1]-totmass/1000))));
						/*	printf("DP is %0.4f on day %d from safeguard %s\n", 
									dailyDP[k][t],t+1,dstrat.SG[k].name); */
							inspindex[k]++;
						}	
						if (strcmp(dstrat.SG[k].name,"aseals")==0)	{
							if (aschedule[t] == 1)	{
								/*# seals broken = frac cascade tapped* seals/cascade* num cascades*/
								int nseals = astrat.AO.nitems * dstrat.SG[k].number * NUMCASC;	
								/*printf("seals broken: %d\n",nseals); */
								dailyDP[k][t] = 1 - pow((1-PSEALSDP),(nseals));
								/*printf("DP is %0.3f on day %d from safeguard %s\n",
										dailyDP[k][t],t+1,dstrat.SG[k].name); */
								inspindex[k]++;
							}					
						}
					}
					/*repiping*/
					if (strcmp(astrat.AO.name,"repiping") == 0)	{
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{
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
							dailyDP[k][t] = dstrat.SG[k].number/numcyl[1];
						
						}
						if (strcmp(dstrat.SG[k].name,"aseals")==0)	{
						}
						if (strcmp(dstrat.SG[k].name,"cemo")==0)	{
						}
					}
						
					if (strcmp(astrat.AO.name,"recycle") == 0)	{
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
					}
						
					/*undeclared feed-- TEMPORARY UNTIL RANDOM INSPS*/
					if (strcmp(astrat.AO.name,"udfeed") == 0)	{
						dailyDP[k][t]=0.90;
						
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

/*use to retrieve cost info*/
struct sginfo dstratinfo[NUMSG] = {inspectioni,psealsi,ndai,dai,videotransi,asealsi,cemoi};	

totalcost = 0;

for (k=0;k<NUMSG;k++)	{
	/*first determine inspection component of cost*/
	/*if insp occurs more frequently than base, multiple cost by increased frequency*/
	if (dstrat.SG[k].active == 1) {
		cost = BASECOST;
		if (dstrat.SG[k].freq != EMPTY && dstrat.SG[k].freq != BASEFREQ)
			cost = (BASEFREQ/dstrat.SG[k].freq)*cost;
		if (dstrat.SG[k].dep != EMPTY && dstrat.SG[k].dep != BASEDEP)
			cost = depmult * cost;
			
		/*then determine other costs associated with SG-- use cost listed in SGinfo*/
		if (strcmp(dstrat.SG[k].name,"pseals") == 0 | 
		strcmp(dstrat.SG[k].name,"da") == 0 )
			cost = dstrat.SG[k].number * dstratinfo[k].cost * (numcyl[0]+numcyl[1]);	
			/*multiple fraction sealed by per seal cost*/
		if (strcmp(dstrat.SG[k].name,"aseals") == 0) 	
			cost = dstrat.SG[k].number * dstratinfo[k].cost * NUMCASC;
		/*higher false alarm probability costs more*/
		if (dstrat.SG[k].fap > fap[1]) 	
			cost = cost * 1.5;
		if (dstrat.SG[k].tcount > tcount[0])
			cost = cost + 50;
		/*if the cost is a startup cost its a big number*/
		if (dstratinfo[k].cost > STARTUP)
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

/*function prints defender strategy*/
void printDStrat(struct dstrategy dstrat,int cost)	{

/*scan through every SG-- if it's active, print define parameters*/
int i;

printf("strategy %d:\n",dstrat.uniqueID);
for (i=0;i<NUMSG;i++)
	if (dstrat.SG[i].active == 1)	{
		printf("SG%d: %s\n",i,dstrat.SG[i].name);
		if (dstrat.SG[i].fap != EMPTY)
			printf("\tFAP: %d\n",dstrat.SG[i].fap);
		if (dstrat.SG[i].number != EMPTY)
			printf("\tnumber: %.2f\n",dstrat.SG[i].number);
		if (dstrat.SG[i].tcount != EMPTY)
			printf("\ttcount: %d\n",dstrat.SG[i].tcount);
		if (dstrat.SG[i].freq != EMPTY)
			printf("\tfreq: %d\n",dstrat.SG[i].freq);
		if (dstrat.SG[i].dep != EMPTY)
			printf("\tdep: %d\n",dstrat.SG[i].dep);
	}
	printf("cost of strategy %d: %d\n",dstrat.uniqueID,cost);

}	



void printAStrat(struct astrategy astrat)	{	

/*print parameters for active AO*/
int i;

printf("strategy %d:\n",astrat.uniqueID);
	printf("AO: %s\n",astrat.AO.name);
	if (astrat.AO.tend != EMPTY)
		printf("\ttend: %d\n",astrat.AO.tend);
	if (astrat.AO.freq != EMPTY)
		printf("\tfreq: %d\n",astrat.AO.freq);
	if (astrat.AO.nitems != EMPTY)
		printf("\tnitems: %.2f\n",astrat.AO.nitems);
	if (astrat.AO.area != EMPTY)
		printf("\tarea: %d\n",astrat.AO.area);
	if (astrat.AO.deltam != EMPTY)
		printf("\tdeltam: %.2f\n",astrat.AO.deltam);
	if (astrat.AO.xp != EMPTY)
		printf("\txp: %.2f\n",astrat.AO.xp);
	if (astrat.AO.xf != EMPTY)
		printf("\txf: %.2f\n",astrat.AO.xf);
	

}	
	