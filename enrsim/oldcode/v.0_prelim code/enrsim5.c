#include <stdio.h>
#include <math.h>
#include <string.h>

/*move globals to preamble*/
/*redefine timeline with reference to day1 of diversion*/
/*comment every variable, loop and passed variable*/

/*structure "safeguard" holds tunable parameters for each safeguards that the 
defender selects; structure "sginfo" holds parameters inherent to that SG that cannot be
altered by the defender*/

#define MAXDSTRATS	100
#define MAXASTRATS	153
#define	NUMSG		9		/*number of safeguards*/
#define NUMAO		6		/*number of attacker options*/
#define EXTRADAYS	30		/*number of days past end of diversion simulation runs*/
#define PSEALSTIME	10		/*days for post-mortem analysis of seals*/
#define	DATIME		14		/*days for analysis of DA samples*/
#define	NUMCASC		60		/*number of cascades at facility*/

float numcyl[2] = {10,2};
float nomcylmass[2] = {27360,5180};	/*feed and product nominal cylinder weights (kg)*/
float erfinv[3] = {-1.6450,-2.1851,-2.6297};	/*inverf(2*FAP-1) for FAP = 0.01, 0.001, 0.0001*/

/*general structure that stores parameters associated with each safeguard*/
struct safeguards {
		char name[11];		/*name of SG*/
		int active;		/*did the defender select?*/
		int fap;		/*false alarm probability*/
		float number;	/*fraction cylinders w/seal (pseal), #samples (DA), # seals/cascade (aseal)*/
		int tcount;		/*count time*/
		int schedfreq;	/*scheduled inspection frequency*/
		int scheddep;	/*scheduled inspection dependency*/
		int randfreq;	/*random inspection frequency*/
		int randdep;	/*random inspection dependency*/
} ;

/*general structure that stores parameters associate with each attacker options*/
struct aoptions	{
	char name[11];		/*name of attacker option*/
	int active;			/*did the attacker select it*/
	int cont;			/*is it continuous?*/
	int tend;			/*duration of attack period*/
	int freq;			/*frequency of attack*/
	float nitems;		/*# cylinders (as1/2),# cascades tapped or repiped (as3/4)*/
	int area;			/*area under attack*/
	float deltam;		/*grams/cascade/instance (as3)*/
	float xp;			/*product enrichment*/
	float xf;			/*feed enrichment*/
};

/*general structure of properties of SGs that cannot be altered by defender*/
struct sginfo	{
	char name[11];	/*name of SG*/
	float cost;		/*SG cost*/
	int cont;		/*0 if SG is discrete; 1 if cont-- defender and attacker*/
	int insptype;	/*0 if conducted at scheduled inspections; 1 for random insp.*/
};

/*effective def. strategies-- holds 1 if xstrat is effective against ystrat; 0 otherwise*/
int effstrats[NUMSG][NUMAO] = { {1,0,0,0,0,0},{0,1,1,1,1,0},{0,1,1,1,1,0},{0,1,0,1,1,0},
								{0,1,0,1,1,0}, {1,1,0,0,0,0},{1,1,0,0,0,0},{0,1,1,1,0,0},
								{0,0,0,1,1,0} };

/*allowable values for defender parameters; game picks from these values*/
int activeop[2] = {0,1};					/*0-active,1-not active-- defender and attacker*/
int fapop[2] = {1,2};						/*1-0.01,2-0.001*/
float fracsealedop[2] = {0.5,1};			/*fraction cylinders sealed*/
float DAsamplesop[2] = {0.1,0.5};			/*fraction cylinders sampled with DA*/
float cascadesealsop[2] = {1,5};			/* number seals per cascade*/	
int tcountop[2] = {300,3600};				/*count time*/
int schedfreqop[2] = {7,30};				/*scheduled inspection frequency*/
int randfreqop[2] = {30,60};				/*random frequency options*/
int depop[2] = {1,19};						/*dependency*/


/*allowable values for attacker parameters; game picks from these values*/

int cont[2] = {0,1};						/*0-discrete,1-continuous*/
int durop[6] = {1,7,30,90,360};				/*attack duration*/
int attackfreqop[4] = {1,7,30};				/*frequency of attack*/
float ncylop[3] = {1,2,3};					/*number of cylinders*/
float fcylop[3] = {0.5,1};					/*fraction of seals attacked-- 0.1 only applies to feed*/
float fcascop[4] = {1/NUMCASC,0.1,0.5,1};	/*fraction cascades tapped or repiped*/
int areaop[3] = {0,1,2};					/*0-feed,1-product,2-cascade*/
float deltamop[3] = {1,10,100};				/*grams/cascade/instance (as3)*/
float xpop[4] = {0.055,0.0197,0.50,0.90};	/*product enrichment*/
float xfop[2] = {0.00711,0.045};			/*feed enrichment*/

/* specification of properties of SGs*/
struct sginfo inventoryi = {"inventory",-1,0,0};
struct sginfo mbalancei = {"mbalance",-1,0,0};
struct sginfo psealsi = {"pseals",-1,0,0};
struct sginfo ndai = {"nda",-1,0,0};
struct sginfo dai = {"da",-1,0,0};
struct sginfo videologi = {"videolog",-1,0,0};
struct sginfo videotransi = {"videotrans",-1,1,-1};
struct sginfo asealsi = {"aseals",-1,1,-1};
struct sginfo cemoi = {"cemo",-1,1,-1};


/*function called by fp to calc DP-- arguments are defender and attacker strategies 
and frequency and dependency for scheduled and random inspections*/
float calcDP(struct safeguards[], struct aoptions[]);	

int main(void)	{
		
printf("So far, so good!\n");

	/******************** until coupled ****************/
	
/*define defender and attacker strategies to be passed to function--
this will usually not be explicitly done; the game will call the calcDP 
function and pass strategy information as an 
argument to the call*/
	
	/*create instance for each safeguard*/
	 struct safeguards inventory = {"inventory",activeop[1],-1,-1,-1,schedfreqop[1],
									depop[0],-1,-1};
	 struct safeguards mbalance = {"mbalance",activeop[0],fapop[1],-1,-1,schedfreqop[1],
									-1,-1,-1};
	 struct safeguards pseals = {"pseals",activeop[0],-1,fracsealedop[2],-1,schedfreqop[1],
									-1,-1,-1};
	 struct safeguards nda = {"nda",activeop[0],fapop[1],-1,-1,schedfreqop[1],
									-1,-1,-1};
	 struct safeguards da = {"da",activeop[0],-1,DAsamplesop[2],-1,schedfreqop[1],
									-1,-1,-1};
	 struct safeguards videolog = {"videolog",activeop[1],-1,-1,-1,schedfreqop[1],
									depop[0],-1,-1};
	 struct safeguards videotrans = {"videotrans",activeop[0],-1,-1,-1,-1,depop[0],-1,-1};
	 struct safeguards aseals = {"aseals",activeop[0],-1,cascadesealsop[0],-1,-1,-1,-1,-1};
	 struct safeguards cemo = {"cemo",activeop[0],fapop[1],-1,tcountop[1],-1,-1,-1,-1};	

	/*dstrat is an array of "safeguards" that describe the defender strategy*/
	 struct safeguards dstrat[NUMSG] = {inventory, mbalance, pseals, nda, da, videolog, 
									videotrans, aseals, cemo};
									
	/*array that holds each strategy*/
	struct safeguards stratspace[MAXDSTRATS][NUMSG];

	
}