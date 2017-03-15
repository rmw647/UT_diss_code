/*use to find specific defender strategies for enrichment*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

/**** successfully generates cumulative DPs ****/

/*structure "safeguard" holds tunable parameters for each safeguards that the 
defender selects; structure "sginfo" holds parameters inherent to that SG that cannot be
altered by the defender*/


#define INSP		9		/*permutations of inspection*/
#define PSEALS		5		/*permutations of pseals*/
#define NDA			5		/*permutations of nda*/
#define DA			5		/*permutations of da*/
#define VIDEOTRANS	3		/*permutations of videotrans*/
#define ASEALS		3		/*permutations of aseals*/
#define CEMO		5		/*permutations of cemo*/
#define VISINSP		3		/*permutations of visual inspection*/
#define ES			5		/*permutations of enrvironmental sampling*/

#define BUDGET		500		/*defender budget*/
#define EMPTY		-1		/*use to initialize payoff matrix-- -1 means DP not yet calculated*/
#define OVERBUDGET	-2		/*use in payoff matrix when defender can't afford row*/	
#define BASECOST	1		/*base cost of inspection*/
#define BASEFREQ	7		/*frequency of $1 SG option*/
#define BASEDEP		1		/*dependency of $1 SG option*/
#define STARTUP		100		/*use for now to differentiate per sample and equip costs*/

#define MAXDSTRATS	428805
#define MAXASTRATS	321
#define	NUMSG		9		/*number of safeguards*/
#define NUMAO		6		/*number of attacker options*/
#define EXTRADAYS	30		/*number of days past end of diversion simulation runs*/



int depmult = 3;	/*used to calculate cost-- lower dependency costs 3x more*/

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

/*general structure that stores defender strategies*/
struct dstrategy	{
	int uniqueID;					/*uniquely identifies strategy*/
	struct safeguards SG[NUMSG];	/*array of SG structures that hold parameter 
									specifications for each SG*/
} ;


/*general structure of properties of SGs that cannot be altered by defender*/
struct sginfo	{
	char name[11];	/*name of SG*/
	int cost;		/*SG cost*/
	int cont;		/*0 if SG is discrete; 1 if cont*/
	int insptype;	/*0 if conducted at scheduled inspections; 1 for random insp.
					2- both*/
	int analysis;	/*number of days required for post-mortem analysis*/
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
float DAsamples[2] = {0.333,1};	/*fraction cylinders sampled with DA-- used for "number"*/
float essamples[2] = {6,12};		/*number of ES swipes taken*/
int tcount[2] = {300,3600};		/*count time*/
int freq[2] = {7,28};			/*frequency options*/
int sifreq[2] = {30,90};		/*special inspection frequency options*/
int dep[2] = {1,19};			/*dependency options*/
	
/* specification of properties of SGs*/
struct sginfo inspectioni = {"inventory",300,0,0,EMPTY};
struct sginfo psealsi = {"pseals",1,0,0,10};
struct sginfo ndai = {"nda",1,0,0,EMPTY};
struct sginfo dai = {"da",30,0,2,14};
struct sginfo videotransi = {"videotrans",200,1,EMPTY,EMPTY};
struct sginfo asealsi = {"aseals",1,1,EMPTY,EMPTY};
struct sginfo cemoi = {"cemo",300,1,EMPTY,EMPTY};

/*functions that generate an array of defender and attacker strategies, respectively*/
void genDStrats(struct dstrategy []);			/*function prototype*/

/*function that prints defender and attacker strategies*/
void printDStrat(struct dstrategy dstrat);		/*function prototype*/

/*variables used for the FP algorithm*/
/*allocate memory here so that they aren't on "stack"*/

struct dstrategy defstrats[MAXDSTRATS];

/****************** MAIN *********************/

int main(void)	{

int i,k;				/*use to index through defender strategies*/
int strategies[200];

for (i=0;i<50;i++)
	strategies[i] = 0;

genDStrats(defstrats);		/*pass pointer to genDStrats to manipulate defstrats*/

printf("so far, so good!\n\n");

/*find strategy numbers for strategies that has given SG active and nothing else*/

char SGname[11]= "inspection";
int specSG = 0;

for (k=0;k<NUMSG;k++)	
	if (strcmp(defstrats[0].SG[k].name,SGname) == 0)	{
		specSG = k;
		break;
	}
	
printf("specSG: %d\n",specSG);
k=0;

for (i=0;i<MAXDSTRATS;i++)
	if (defstrats[i].SG[0].active == 0 && defstrats[i].SG[3].active == 0 &&
		defstrats[i].SG[4].active == 0 && defstrats[i].SG[1].active == 0 &&
		   defstrats[i].SG[2].active == 0 && defstrats[i].SG[7].active == 1 /*&&
		defstrats[i].SG[5].active == 0  /*&& defstrats[i].SG[8].active == 0 &&
		defstrats[i].SG[6].active == 0 /*&& defstrats[i].SG[0].freq == freq[1] &&
		defstrats[i].SG[0].dep == dep[0] /*&& defstrats[i].SG[7].freq == sifreq[0] &&
		defstrats[i].SG[1].number == numseals[0] && defstrats[i].SG[2].fap == fap[0] &&
		defstrats[i].SG[3].number == DAsamples[0] && defstrats[i].SG[4].dep == dep[0] &&
		defstrats[i].SG[5].number == numseals[1] && defstrats[i].SG[6].fap == fap[0]*/){
		strategies[k] = i;
		k++;
	}
	
for(k=0;k<50;k++)	
	if (strategies[k] != 0)
		printf("%d\n",strategies[k]);

for(k=0;k<50;k++)	
	if (strategies[k] != 0)
		printDStrat(defstrats[strategies[k]]);

printf("%d\n\n",defstrats[10].SG[8].freq < defstrats[10].SG[7].freq
							    && defstrats[10].SG[8].active == 1);			
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
	
/*array to hold all visual insp permutations*/

struct safeguards visinsp[VISINSP];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<VISINSP;i++)	{
	visinsp[i].fap = EMPTY;
	visinsp[i].number = EMPTY;
	visinsp[i].tcount = EMPTY;
	visinsp[i].dep = EMPTY;
}

/*no inspection option*/
strcpy(visinsp[0].name,"visinsp");
visinsp[0].active = 0;
visinsp[0].freq = 0;

index = 1;			/*counts in inspection array*/

/*rest of visinsp options*/
for (j=0;j<sizeof(sifreq)/sizeof(int);j++){
	strcpy(visinsp[index].name,"visinsp");
	visinsp[index].active = 1;
	visinsp[index].freq = sifreq[j];
	index++;
}

/*array to hold all cascade samples permutations*/
struct safeguards es[ES];

/*make all parameters that don't apply "EMPTY"*/
for (i=0;i<ES;i++)	{
	es[i].fap = EMPTY;
	es[i].tcount = EMPTY;
	es[i].dep = EMPTY;
}

/*no da option*/
strcpy(es[0].name,"es");
es[0].active = 0;
es[0].number = 0;
es[0].freq = 0;

index = 1;			/*counts in inventory array*/

/*rest of es options*/
for (j=0;j<sizeof(essamples)/sizeof(float);j++)
	for (k=0;k<sizeof(sifreq)/sizeof(int);k++)	{
		strcpy(es[index].name,"es");
		es[index].active = 1;
		es[index].number = essamples[j];
		es[index].freq = sifreq[k];
		index++;
	}


/***** generate all defender strategies *****/

int d1,d2,d3,d4,d5,d6,d7,d8,d9;
int skip = 0;
/*generate all strategy combinations and store in defstrats*/
for (d1=0;d1<INSP;d1++)
	for (d2=0;d2<PSEALS;d2++)	
		for (d3=0;d3<NDA;d3++)
			for (d4=0;d4<DA;d4++)
				for (d5=0;d5<VIDEOTRANS;d5++)
					for (d6=0;d6<ASEALS;d6++)
						for (d7=0;d7<CEMO;d7++)	
							for (d8=0;d8<VISINSP;d8++)
								for (d9=0;d9<ES;d9++){
							(defstrats[count]).uniqueID = count;
							(defstrats[count]).SG[0] = inspection[d1];
							(defstrats[count]).SG[1] = pseals[d2];
							(defstrats[count]).SG[2] = nda[d3];
							(defstrats[count]).SG[3] = da[d4];
							(defstrats[count]).SG[4] = videotrans[d5];
							(defstrats[count]).SG[5] = aseals[d6];
							(defstrats[count]).SG[6] = cemo[d7];
							(defstrats[count]).SG[7] = visinsp[d8];
							(defstrats[count]).SG[8] = es[d9];
							if((defstrats[count].SG[0].active == 0 && 
								defstrats[count].SG[1].active == 1) |
								(defstrats[count].SG[0].active == 0 && 
								defstrats[count].SG[2].active == 1) |
								(defstrats[count].SG[0].active == 0 && 
								defstrats[count].SG[3].active == 1) |
								(defstrats[count].SG[7].active == 0 && 
								defstrats[count].SG[8].active == 1) |
								(defstrats[count].SG[1].freq < defstrats[count].SG[0].freq 
								&& defstrats[count].SG[1].active == 1)|
							    (defstrats[count].SG[2].freq < defstrats[count].SG[0].freq
							    && defstrats[count].SG[2].active == 1)|
							    (defstrats[count].SG[3].freq < defstrats[count].SG[0].freq
							    && defstrats[count].SG[3].active == 1)|
							    (defstrats[count].SG[8].freq < defstrats[count].SG[7].freq
							    && defstrats[count].SG[8].active == 1))	
							    	skip++;
							else
								count++;
	}

}

/*function prints defender strategy*/
void printDStrat(struct dstrategy dstrat)	{

/*scan through every SG-- if it's active, print define parameters*/
int i;

printf("strategy %d:\n",dstrat.uniqueID);
for (i=0;i<NUMSG;i++)
	if (dstrat.SG[i].active == 1)	{
		printf("SG%d: %s\n",i,dstrat.SG[i].name);
		if (dstrat.SG[i].fap != EMPTY)
			if (dstrat.SG[i].fap == fap[0])
				printf("\tFAP: 0.01\n");
			else
				printf("\tFAP: 0.001\n");
		if (dstrat.SG[i].number != EMPTY)
			if (strcmp(dstrat.SG[i].name,"pseals") == 0)
				printf("\tfraction cylinders sealed: %.2f\n",dstrat.SG[i].number);
			else if (strcmp(dstrat.SG[i].name,"da") == 0)
				printf("\tfraction cylinders sampled: %.2f\n",dstrat.SG[i].number);
			else if (strcmp(dstrat.SG[i].name,"aseals") == 0)
				printf("\tfraction cascades sealed: %.2f\n",dstrat.SG[i].number);
			else if (strcmp(dstrat.SG[i].name,"es") == 0)
				printf("\tES swipes: %f\n",dstrat.SG[i].number);
		if (dstrat.SG[i].tcount != EMPTY)
			printf("\tcount time: %d sec\n",dstrat.SG[i].tcount);
		if (dstrat.SG[i].freq != EMPTY)
			printf("\tfreq: %d 1/days\n",dstrat.SG[i].freq);
		if (dstrat.SG[i].dep != EMPTY)
			if (dstrat.SG[i].dep == dep[0])
				printf("\tsize of inspection team: small\n");
			else
				printf("\tsize of inspection team: large\n");
	}

}	

