/*prints variable "count", which counts number of defender strategies for enrichment*/


#include <stdio.h>
#include <string.h>

#define SWU		465000		/*nominal plant capacity*/
#define	NUMCASC	60.0		/*number of cascades at facility*/

#define INSP		9		/*permutations of inspection*/
#define PSEALS		5		/*permutations of pseals*/
#define NDA			5		/*permutations of nda*/
#define DA			5		/*permutations of da*/
#define VIDEOTRANS	3		/*permutations of videotrans*/
#define ASEALS		3		/*permutations of aseals*/
#define CEMO		5		/*permutations of cemo*/
#define VISINSP		3		/*permutations of visual inspection*/
#define ES			5		/*permutations of enrvironmental sampling*/

#define CYLTHEFT	5		/*permutations of cylinder theft*/
#define MATCYL		49		/*permutations of matcyl*/
#define MATCASC		37		/*permutations of matcasc*/
#define REPIPING	28		/*permutations of repiping*/
#define RECYCLE		55		/*permutations of recycle*/
#define UDFEED		19		/*permutations of udfeed*/
#define REPIPINGFREQ	1	/*repiping frequency MUST be every day--always observable*/

#define BUDGET		2000		/*defender budget*/
#define EMPTY		-1		/*use to initialize payoff matrix-- -1 means DP not yet calculated*/
#define OVERBUDGET	-2		/*use in payoff matrix when defender can't afford row*/	
#define DEPMULT		3		/*multiply by for larger team*/
#define	FAPMULT		1.1		/*multiply by for lower fap*/
#define FAPMULTCEMO	2		/*multiply by for lower fap for cemo*/
#define INSPCOST	10		/*inspection cost- per day*/
#define ANALYSISCOST 2.5	/*per DA sample or batch of seals analysis cost*/
#define	ASSESSCOST	0.6		/*assessment time- per day cost*/
#define ADDINSPCOST	2		/*additional insp activities cost 2 s$/day*/

#define MAXDSTRATS	428805
#define MAXASTRATS	161
#define	NUMSG		9		/*number of safeguards*/
#define NUMAO		6		/*number of attacker options*/
#define EXTRADAYS	30		/*number of days past end of diversion simulation runs*/

#define VIDEOLOGDP	0.43	/*initial DP for videolog*/
#define VIDEOTRANSDP	0.64	/*initial DP for videotrans*/
#define PSEALSDP		0.85	/*DP for a single passive seal*/
#define ASEALSDP	0.40	/*DP for a single active seal*/

#define NUMCYLFALSE	1		/*number of product cylinders falsified-- all over-enriched
							product placed in one cylinder*/

#define IT			10	/*number of iterations for FP algorithm*/
#define SIMYEAR	360	/*360 days in a simulation year*/

float xprod = 0.045;			/*nominal product enrichment*/
float xw = 0.0022;				/*tails enrichment*/
float xfeed = 0.00711;			/*nominal feed enrichment*/

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
	int uniqueID;
	struct safeguards SG[NUMSG];
};

/*general structure that stores attacker strategies*/
/*stores only one active attacker option*/
struct astrategy	{
	int uniqueID;
	struct aoptions AO;
};

/*allowable values for defender parameters; game picks from these values*/
int fap[2] = {1,2};				/*1 = 0.01, 2 = 0.001*/	
float numseals[2] = {0.5,1};	/*fraction of cylinders sealed-- used for "number"*/
float DAsamples[2] = {0.1,0.5};	/*fraction cylinders sampled with DA-- used for "number"*/
float essamples[2] = {6,12};		/*number of ES swipes taken*/
float cascsamples[2] = {1,3};	/*number of samples taken from cascades*/
int tcount[2] = {300,3600};		/*count time*/
int freq[2] = {7,30};			/*frequency options*/
int sifreq[2] = {30,90};		/*special inspection frequency options*/
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


struct dstrategy defstrats[MAXDSTRATS];
struct astrategy attstrats[MAXASTRATS];

int main(void)	{

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


printf("%d\n\n",count);
printf("%d\n\n",skip);

printf("\n\n");
printf("\n\n");



/****************** ATTACKER OPTIONS **********************/

/***** generate all permutations for available AOs *****/

struct astrategy *pattstrats;		/*a pointer to a structure*/
pattstrats = &attstrats[0];			/*point the pointer at defstrats[0]*/

count = 0;			/*counts in strategy array*/
index = 1;			/*counts in inventory array*/


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
udfeed[0].nitems = 0;

index = 1;  /*index through udfeed array*/

/*rest of udfeed options*/
for (j=0;j<sizeof(dur)/sizeof(int);j++)
	for (k=0;k<sizeof(attackfreq)/sizeof(int);k++)
		for (i=0;i<sizeof(fcasc)/sizeof(float);i++)	{
		strcpy(udfeed[index].name,"udfeed");
		udfeed[index].active = 1;
		udfeed[index].tend = dur[j];
		udfeed[index].freq = attackfreq[k];
		udfeed[index].nitems = fcasc[i];
		index++;
	}

count = 0;		/*index through attstrats*/
skip = 0;

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
	else 	{
		printf("skip strategy: dur:%d freq:%d\n",attstrats[count].AO.tend,
		attstrats[count].AO.freq);
		skip++;
	}
}

for (j=1;j<MATCASC;j++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = matcasc[j];
	if (attstrats[count].AO.tend >= attstrats[count].AO.freq)
		count++;
	else 	{
		printf("skip strategy: dur:%d freq:%d\n",attstrats[count].AO.tend,
		attstrats[count].AO.freq);
		skip++;
	}
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
	else 	{
		printf("skip strategy: dur:%d freq:%d\n",attstrats[count].AO.tend,
		attstrats[count].AO.freq);
		skip++;
	}
}

for (j=1;j<UDFEED;j++)	{
	attstrats[count].uniqueID = count;
	attstrats[count].AO = udfeed[j];
	if (attstrats[count].AO.tend >= attstrats[count].AO.freq)
		count++;
	else 	{
		printf("skip strategy: dur:%d freq:%d\n",attstrats[count].AO.tend,
		attstrats[count].AO.freq);
		skip++;
	}
}

printf("%d\n\n",count);
printf("%d\n\n",skip);

/*print all strategy combinations to test*/
for (i=0;i<MAXASTRATS;i++)	
	printf("strategy:%d AO:%s dur:%d freq:%d nitems:%.3f area:%d deltam:%.2f xp: %.3f\n\n",
			attstrats[i].uniqueID,attstrats[i].AO.name,attstrats[i].AO.tend,
			attstrats[i].AO.freq,attstrats[i].AO.nitems,attstrats[i].AO.area,
			attstrats[i].AO.deltam,attstrats[i].AO.xp); 


printf("size of inspection: %ld\n",sizeof(inspection));
printf("size of pseals: %ld\n",sizeof(pseals));
printf("size of nda: %ld\n",sizeof(nda));
printf("size of da: %ld\n",sizeof(da));
printf("size of videotrans: %ld\n",sizeof(videotrans));
printf("size of aseals: %ld\n",sizeof(aseals));
printf("size of cemo: %ld\n",sizeof(cemo));
printf("size of cyltheft: %ld\n",sizeof(cyltheft));
printf("size of matcyl: %ld\n",sizeof(matcyl));
printf("size of matcasc: %ld\n",sizeof(matcasc));
printf("size of repiping: %ld\n",sizeof(repiping));
printf("size of recycle: %ld\n",sizeof(recycle));



}