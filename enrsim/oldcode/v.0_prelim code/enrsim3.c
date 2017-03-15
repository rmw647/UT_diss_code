#include <stdio.h>
#include <math.h>
#include <string.h>

/*move globals to preamble*/
/*redefine timeline with reference to day1 of diversion*/
/*comment every variable, loop and passed variable*/

/*structure "safeguard" holds tunable parameters for each safeguards that the 
defender selects; structure "sginfo" holds parameters inherent to that SG that cannot be
altered by the defender*/

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
	char name[100];	/*name of SG*/
	int active;		/*did the defender select?*/
	int fap;		/*false alarm probability*/
	float number;	/*fraction cylinders w/seal (pseal), #samples (DA), # seals/cascade (aseal)*/
	int tcount;		/*count time*/
	int area;		/*area from which sample is taken-- feed, product, cascade*/
};

/*general structure that stores parameters associate with each attacker options*/
struct aoptions	{
	char name[100];		/*name of attacker option*/
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
	char name[100];	/*name of SG*/
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
int fapop[2] = {1,2};						/*1-0.01,2-0.001,3-0.0001*/
float fracsealedop[2] = {0.5,1};			/*fraction cylinders sealed*/
float DAsamplesop[2] = {0.1,0.5};			/*fraction cylinders sampled with DA*/
float cascadesealsop[3] = {1,5,10};			/* number seals per cascade*/	
int tcountop[3] = {300,900,3600};			/*count time*/
int freqop[3] = {7,30,60};				/*scheduled and random inspection frequency*/
int depop[4] = {0,1,6,19};					/*dependency*/

/*allowable values for attacker parameters; game picks from these values*/

int cont[2] = {0,1};				/*0-discrete,1-continuous*/
int durop[6] = {1,7,15,30,90,360};	/*attack duration*/
int attackfreqop[4] = {1,7,15,30};		/*frequency of attack*/
float ncylop[3] = {1,2,3};				/*number of cylinders*/
float fcylop[3] = {0.1,0.5,1};			/*fraction of seals attacked-- 0.1 only applies to feed*/
float fcascop[4] = {1/NUMCASC,0.1,0.5,1};		/*fraction cascades tapped or repiped*/
int areaop[3] = {0,1,2};					/*0-feed,1-product,2-cascade*/
float deltamop[3] = {1,10,100};			/*grams/cascade/instance (as3)*/
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
float calcDP(struct safeguards[], struct aoptions[], int, int, int, int);	


int main(void)	{
		
	/******************** until coupled ****************/
	
/*define defender and attacker strategies to be passed to function--
this will usually not be explicitly done; the game will call the calcDP 
function and pass strategy information as an 
argument to the call*/

/*create instance for each safeguard*/
struct safeguards inventory = {"inventory",activeop[1],-1,-1,-1,-1};
struct safeguards mbalance = {"mbalance",activeop[0],fapop[1],-1,-1,-1};
struct safeguards pseals = {"pseals",activeop[0],-1,fracsealedop[2],-1,-1};
struct safeguards nda = {"nda",activeop[0],fapop[1],-1,-1,1};
struct safeguards da = {"da",activeop[0],-1,DAsamplesop[2],-1,1};
struct safeguards videolog = {"videolog",activeop[0],-1,-1,-1,-1};
struct safeguards videotrans = {"videotrans",activeop[0],-1,-1,-1,-1};
struct safeguards aseals = {"aseals",activeop[0],-1,cascadesealsop[0],-1,-1};
struct safeguards cemo = {"cemo",activeop[0],fapop[2],-1,tcountop[1],-1};

/*dstrat is an array of "safeguards" that describe the defender strategy*/
struct safeguards dstrat[NUMSG] = {inventory,mbalance,pseals,nda,da,videolog,videotrans,aseals,cemo};

/*general strategy parameters the defender must choose*/
int schedfreq = freqop[0];
int randfreq = freqop[2];
int scheddep = depop[3];
int randdep = depop[3];

/*create instance for each attacker options*/
struct aoptions cyltheft = {"cyltheft",activeop[1],cont[0],durop[2],-1,ncylop[0],areaop[1],-1,-1,-1};
struct aoptions matcyl = {"matcyl",activeop[0],cont[1],durop[2],attackfreqop[0],ncylop[1],areaop[1],deltamop[0],-1,-1};
struct aoptions matcasc = {"matcasc",activeop[0],cont[1],durop[2],attackfreqop[1],fcascop[1],-1,deltamop[2],-1,-1};
struct aoptions repiping = {"repiping",activeop[0],cont[0],durop[4],-1,fcascop[1],-1,-1,xpop[1],xfop[0]};
struct aoptions recycle = {"recycle",activeop[0],cont[1],durop[4],attackfreqop[1],fcascop[1],-1,-1,xpop[1],xfop[0]};
struct aoptions udfeed = {"udfeed",activeop[0],cont[1],durop[4],-1,-1,-1,-1,-1,-1};

/*astrat is an array of "aoptions" that describe the attacker strategy*/
struct aoptions astrat[NUMAO] = {cyltheft,matcyl,matcasc,repiping,recycle,udfeed};

/******************** until coupled ****************/	
	
	/*********CHECK*********/
	int k;
	for (k=0;k<NUMSG;k++)	
		if (dstrat[k].active == 1)
			printf("safeguard %s is active\n",dstrat[k].name);
			
	calcDP(dstrat,astrat,schedfreq,randfreq,scheddep,randdep);
	
	printf("so far, so good!\n\n");
	
	/*calculate the cumulative DP for the strategy pair*/
/*	float DP = calcDP(dstrat,astrat,schedfreq,randfreq,scheddep,randdep); */
		
}

float calcDP(struct safeguards dstrat[], struct aoptions astrat[], int schedfreq,
			int randfreq, int scheddep, int randdep){
	/*dstrat is array of safeguards that holds a defender strategy*/
/*astrat is an array of aoptions that holds an attacker strategy*/
	
	int t,j,actastrat;				/*t indexes time; j indexes att options; active attacker strat*/
	int tend;						/*end day of diversion*/
	int m;							/*schedule array dimensions-- detection days*/
	int k;							/*indexes safeguards*/
	
	/*array of "sginfo" structures that holds information about properties inherent to SGs
	order of sginfo elements MUST match order of safeguards elements in dstrat*/
	struct sginfo dstratinfo[NUMSG] = {inventoryi,mbalancei,psealsi,ndai,dai,videologi,
									videotransi,asealsi,cemoi};

	/*set actastrat equal to the number of the active attacker strategy*/
	/*defender and attacker strategies are referenced BY THEIR INDICES (strategy number -1)*/
	
	for (j=0;j<NUMAO;j++)
		if (astrat[j].active == 1)
			actastrat = j;
	
	/***initialize schedule arrays-- run EXTRADAYS days past end of diversion***/	
	tend = astrat[actastrat].tend;	
	
	/*compute m, the total number of simulation days*/
	if (astrat[actastrat].cont == 0)		/*if its a discrete attacker strategy*/
		m = EXTRADAYS;
	else								/*continuous attacker strategy*/
		m = tend+EXTRADAYS;

	/*create schedule arrays-- defender array is kxm with separate row for each SG*/
	int aschedule[m], dschedule[NUMSG][m];
	
	/***populate schedule arrays-- 0 for days with no activity, 1 for days when an activity
	occurs***/
	
	/*initialize attacker array with 0's*/
	for(t=0;t<m;t++)
		aschedule[t] = 0;
		
	if (astrat[actastrat].cont == 0)	/*if attacker strategy is disrete*/
		aschedule[0]=1;					/*attacker activity on first day, nothing on rest of days*/ 
	else								/*if it's continuous*/
		for(t=0;t<tend;t=t+astrat[actastrat].freq)	/*activity of first day and repeating every f days (f=freq) */
			aschedule[t] = 1;
	
	/*initialize defender array with 0's*/
	for(k=0;k<NUMSG;k++)
		for(t=0;t<m;t++)
			dschedule[k][t] = 0;	
		
	/*schedule all SGs-- schedule in three groups: continuous, scheduled inspections,
	SGs that require post-mortem analysis*/
	/*1 represents day on which DETECTION OPPORTUNITY OCCURS*/
	
	for (k=0;k<NUMSG;k++)			
		if (dstrat[k].active == 1)	{	
			if (dstratinfo[k].cont == 1)	/*if it's a continuous defender strategy*/
				for (t=0;t<m;t++)
					dschedule[k][t] = 1;	/*schedule it everyday*/
			else					{ 		/*if it's a discrete defender strategy*/
				/*and it occurs during scheduled inspections*/
				if (strcmp(dstrat[k].name, "inventory") == 0 | strcmp(dstrat[k].name, "mbalance") == 0
					| strcmp(dstrat[k].name, "nda") == 0 | strcmp(dstrat[k].name, "videolog") == 0)
					for (t=schedfreq-1;t<m;t=t+schedfreq)
						dschedule[k][t] = 1;		/*it's active on insp days*/
				/*post-mortem analysis for passive seals takes 10 days*/
				else if (strcmp(dstrat[k].name, "pseals") == 0)
					for (t=schedfreq-1+PSEALSTIME;t<m;t=t+schedfreq+PSEALSTIME)
						dschedule[k][t] = 1;
				/*DA takes place on inspection days but  analysis takes 14 days*/
				else if (strcmp(dstrat[k].name, "da") == 0)
					for (t=schedfreq-1+DATIME;t<m;t=t+schedfreq+DATIME)
						dschedule[k][t] = 1;
			}
		}
	
	/************CHECK************/
	printf("defender schedule:\n");
	printf("%-12s","day");
	for (t=0;t<m;t++)	
		printf("%-3d",t+1);
	for (k=0;k<NUMSG;k++)	{
		printf("\n");
		printf("%-12s",dstrat[k].name);
		for (t=0;t<m;t++)
			printf("%-3d",dschedule[k][t]);
	}
	printf("\n");
	printf("\n");

	printf("attacker schedule:\n");
	printf("%-12s","day");
	for (t=0;t<m;t++)	
		printf("%-3d",t+1);
	printf("\n");
	printf("%-12s","activity");
	for (t=0;t<m;t++)
		printf("%-3d",aschedule[t]);
	printf("\n");


/*loop through time and safeguards to populate dailyDP array
		time loop	{
			safeguard loop	{
				if k is active
					if k is effective against attacker strategy
						calculate DP							*/
	
	float dailyDP[NUMSG][m];		/*holds DP for each day for each safeguards over entire period*/

	for (k=0;k<NUMSG;k++)
		for(t=0;t<m;t++)
			dailyDP[k][t] = 0;
			
	/***update this code so that it doesnt scan through any safeguards that are NEVER active***/
	printf("%s\n",astrat[actastrat].name);
	int inspindex[NUMSG];				/*track how many inspections have occurred*/
	for (k=0;k<NUMSG;k++)
		inspindex[k] = 0;
	
	for (t=0;t<m;t++)	{			/*index through days*/
		for (k=0;k<NUMSG;k++)	{	/*index through safeguards*/
			if (dschedule[k][t] == 1)	{			/*if safeguard is active that day*/
				printf("\n%d active on day %d\n",k,t+1);
				if (effstrats[k][actastrat] == 1){	/*if safeguards is effective against att. strat*/
					if (strcmp(astrat[actastrat].name,"cyltheft") == 0)	{	
						printf("cylinder theft\n");
						if (strcmp(dstrat[k].name,"inventory")==0)	{				/*inventory*/
							printf("%s\n",dstrat[k].name);
							if (inspindex[k] == 0)	{
								printf("first inspection\n");
								dailyDP[k][t] = 1-exp(-0.65*(scheddep+1)*astrat[k].nitems);
								inspindex[k]++;	
								printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]);
							}
							else	{
								dailyDP[k][t] = 1-((1+scheddep*(1-dailyDP[k][t-schedfreq]))/(scheddep+1));
								printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]);
							}
						}
						if (strcmp(dstrat[k].name,"videolog")==0)	{
							printf("%s\n",dstrat[k].name);
							while (inspindex[k] == 0)	{
								dailyDP[k][t] = 0.43;
								inspindex[k]++;
							}
							inspindex[k]++;
							printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]);
						}
						if (strcmp(dstrat[k].name,"videotrans")==0)	{
							printf("%s\n",dstrat[k].name);
							while (inspindex[k] == 0)	{
								dailyDP[k][t] = 0.64;
								inspindex[k]++;
							}
							inspindex[k]++;
							printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]);
						} 
					
					}
					/*some material from cylinder*/
					if (strcmp(astrat[actastrat].name,"matcyl") == 0) {	
						if (strcmp(dstrat[k].name,"mbalance")==0)	{
							printf("%s\n",dstrat[k].name);
								if (astrat[actastrat].area == 0)	{	/*feed cylinder*/
									
								}
						}
						if (strcmp(dstrat[k].name,"pseals")==0)	{
							printf("%s\n",dstrat[k].name);
							float DPpseal[15];
						
							
						}
						if (strcmp(dstrat[k].name,"nda")==0)	{
							printf("%s\n",dstrat[k].name);
							
						}
						if (strcmp(dstrat[k].name,"da")==0)	{
							printf("%s\n",dstrat[k].name);
							
						}
						if (strcmp(dstrat[k].name,"videolog")==0)	{
							float DPlog[15];		/*use to handle HRA for per instance DP*/
							DPlog[0] = 0.43;
							printf("%s\n",dstrat[k].name);
							int i,*pntevents ;		/*theft events between inspections*/
							int ntevents = 0;
							pntevents = &ntevents;
							/*calculate diversion events between inspections*/
							for (i=t-schedfreq+1;i<=t;i++)
								if (aschedule[i] == 1)
									(*pntevents)++;
							printf("number of diversion events: %d\n",ntevents);
							/*calculate DP-- need to use previous per instance DP, which is stored in
							DPlog. Find cumulative sum of per instance DPs for all instances
							that have occured since last inpsection*/
							if (inspindex[k] == 0)		{
								dailyDP[k][t] = 1-pow((1-DPlog[0]),ntevents);
								printf("Per instance DP on insp. %d: %.2f\n",inspindex[k]+1	,DPlog[inspindex[k]]);
								inspindex[k]++;
							}
							else	{
								DPlog[inspindex[k]] = 1-((1+scheddep*(1-DPlog[inspindex[k]-1]))/(scheddep+1)); 
								printf("Per instance DP on insp. %d: %.2f\n",inspindex[k]+1	,DPlog[inspindex[k]]);
								dailyDP[k][t] = 1-pow((1-DPlog[inspindex[k]]),ntevents);
								inspindex[k]++;
							}
						printf("DP: %.2f\n",dailyDP[k][t]);
							
						}
						if (strcmp(dstrat[k].name,"videotrans")==0)	{
							/*video only transmitted when diversion event occurs*/
							if (aschedule[t] == 1)	{
								printf("%s\n",dstrat[k].name);
								if (inspindex[k] == 0)	{ /*if this is DP initial for video trans*/
									dailyDP[k][t] = 0.64;
									inspindex[k]++; 
									printf("DP on day %d: %.2f\n",t+1,dailyDP[k][t]);				
								}
								else 	{	/*if it isn't DP initial-- HRA*/
									printf("DP: %.2f\n",dailyDP[k][t-astrat[actastrat].freq]); 
									dailyDP[k][t] = 1-(1+scheddep*(1-dailyDP[k][t-astrat[actastrat].freq]))/
												(scheddep+1);
									inspindex[k]++;
									printf("DP on day %d: %.2f\n",t+1,dailyDP[k][t]); 
								} 
							}					
						}
						if (strcmp(dstrat[k].name,"aseals")==0)	{
							printf("%s\n",dstrat[k].name);	
						}
					}	
					/*some material from cascade*/ /*DONE*/
					if (strcmp(astrat[actastrat].name,"matcasc") == 0)	{
						if (strcmp(dstrat[k].name,"mbalance")==0)	{
							/*total number of theft events*/;
							float ntevents;
							if (t<=tend)	
								ntevents = ceil((t+1)/astrat[actastrat].freq);
							else
								ntevents = ceil(tend/astrat[actastrat].freq)+1;
							printf("%d\n",t);
							printf("theft events: %.0f\n",ntevents);
							printf("delta m: %.2f\n",astrat[actastrat].deltam);
							float totmass = ntevents * astrat[actastrat].deltam * astrat[actastrat].nitems*NUMCASC;
							printf("total mass: %.1f\n",totmass);
							/*total mass stolen is events*mass/event*cascades/event-- in grams*/
							float thresh = nomcylmass[1]+sqrt(2)*(0.0007*nomcylmass[1])*erfinv[(dstrat[k].fap)-1];
							dailyDP[k][t] = 1-0.5*(1-erf((thresh-(nomcylmass[1]-totmass/1000))/(sqrt(2)*
											0.0007*(nomcylmass[1]-totmass/1000))));
							printf("DP is %0.4f on day %d from safeguard %s\n",dailyDP[k][t],t+1,dstrat[k].name);
							inspindex[k]++;
						}
						if (strcmp(dstrat[k].name,"aseals")==0)	{
							if (aschedule[t] == 1)	{
								/*# seals broken = frac cascade tapped* seals/cascade* num cascades*/
								int nseals = astrat[actastrat].nitems * dstrat[k].number * NUMCASC;	
								printf("seals broken: %d\n",nseals);
								dailyDP[k][t] = 1 - pow((1-0.40),(nseals));
								printf("DP is %0.3f on day %d from safeguard %s\n",dailyDP[k][t],t+1,dstrat[k].name);
								inspindex[k]++;
							}					
						}

					}	
					/*repiping*/
					if (strcmp(astrat[actastrat].name,"repiping") == 0)	{
						if (strcmp(dstrat[k].name,"mbalance")==0)	{
						
						}
						if (strcmp(dstrat[k].name,"pseals")==0)	{
						
						}
						if (strcmp(dstrat[k].name,"nda")==0)	{
						
						}
						if (strcmp(dstrat[k].name,"da")==0)	{
						
						}
						if (strcmp(dstrat[k].name,"aseals")==0)	{
						
						}
						if (strcmp(dstrat[k].name,"cemo")==0)	{
						
						}
					}	
					/*recycle*/
					if (strcmp(astrat[actastrat].name,"recycle")==0)	{
						if (strcmp(dstrat[k].name,"mbalance")==0)	{
						
						}
						if (strcmp(dstrat[k].name,"pseals")==0)	{
						
						}
						if (strcmp(dstrat[k].name,"nda")==0)	{
						
						}
						if (strcmp(dstrat[k].name,"da")==0)	{
						
						}
						if (strcmp(dstrat[k].name,"cemo")==0)	{
						
						}
					}	
					/*undeclared feed*/ /*CAN'T DO YET*/
					if (strcmp(astrat[actastrat].name,"udfeed")==0)	{
					/*nothing right now-- only LFUAs effective against this*/
					}		
				}
			}
		}
	}
}