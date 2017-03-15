#include <stdio.h>
#include <math.h>

/*move globals to preamble*/
/*redefine timeline with reference to day1 of diversion*/
/*comment every variable, loop and passed variable*/

struct safeguards {
		char name[25];
		int active;
		int cont;
		float dep;
		int fap;		/*1, 2, 3-- FAP = 0.01, 0.001, 0.0001*/
		float number;	/*fraction cylinders w/seal (pseal), #samples (DA), # seals/cascade (aseal)*/
		int tcount;
	};
/*a structure that stores parameters associated with each safeguard*/

struct aoptions	{
		char name[100];
		int active;
		int cont;
		int tstart;
		int tend;
		int freq;
		float nitems;		/*# cylinders (as1/2),# cascades tapped or repiped (as 3/4)*/
		int area;			/*0-feed storage,1-prod storage*/
		float deltam;		/*grams/cascade/instance (as3)*/
		float xp;
		float xf;
	};
/*a structure that stores parameters associate with each attacker options*/

float calcDP(struct safeguards dstrat[], struct aoptions astrat[]);	/*function called by fp to calc DP--
arguments are two arrays of pointers to (array of) floats*/

int main(void)	{
	/*define defender and attacker strategies to be passed to function--
this will usually not be explicitly done; the game will call the calcDP function and pass
strategy information as an array of pointers as an argument to the call*/
	
	/*create instance for each safeguard*/
	struct safeguards inventory = {"inventory",0,0,6,-1,-1,-1};
	struct safeguards mbalance = {"mbalance",0,0,-1,1,-1,-1};
	struct safeguards pseals = {"pseals",1,0,-1,-1,1,-1};
	struct safeguards nda = {"nda",0,0,-1,1,-1,-1};
	struct safeguards da = {"da",0,0,-1,1,1,-1};
	struct safeguards videolog = {"videolog",0,0,6,-1,-1,-1};
	struct safeguards videotrans = {"videotrans",0,1,6,-1,-1,-1};
	struct safeguards aseals = {"aseals",0,1,-1,-1,2,-1};
	struct safeguards cemo = {"cemo",0,1,-1,2,-1,300};
	
	
	/*dstrat is an array of "safeguards" that describe the defender strategy*/
	struct safeguards dstrat[9] = {inventory,mbalance,pseals,nda,da,videolog,videotrans,aseals,cemo};
	
	/*********CHECK*********/
	int k;
	for (k=0;k<9;k++)	
		if (dstrat[k].active == 1)
			printf("safeguard %s is active\n",dstrat[k].name);
		
	/*create instance for each attacker options*/
	struct aoptions cyltheft = {"cyltheft",0,0,16,16,1,1,1,-1,-1,-1};
	struct aoptions matcyl = {"matcyl",1,1,2,10,1,2,1,0.1,-1,-1};
	struct aoptions matcasc = {"matcasc",0,1,2,20,2,2,-1,200,-1,-1};
	struct aoptions repiping = {"repiping",0,1,2,80,1,2,-1,-1,0.197,0.00711};
	struct aoptions recycle = {"recycle",0,1,2,80,1,-1,-1,-1,0.197,0.00711};
	struct aoptions udfeed = {"udfeed",0,1,2,80,5,-1,-1,-1,-1,-1};

	/*astrat is an array of "aoptions" that describe the attacker strategy*/
	struct aoptions astrat[6] = {cyltheft,matcyl,matcasc,repiping,recycle,udfeed};
	
	printf("so far, so good!\n");
	
	float DP = calcDP(dstrat,astrat);
	/*calculate the cumulative DP for the strategy pair*/
	
}

float calcDP(struct safeguards dstrat[], struct aoptions astrat[]){
/*dstrat is array of safeguards that holds a defender strategy*/
/*astrat is an array of aoptions that holds an attacker strategy*/

	/*effective defender strategies-- holds 1 if xstrat is effective against ystrat and 0 otherwise*/
	int effstrats[9][6] = { {1,0,0,0,0,0},{0,1,1,1,1,0},{0,1,1,1,1,0},{0,1,0,1,1,0},{0,1,0,1,1,0},
						  {1,1,0,0,0,0},{1,1,0,0,0,0},{0,1,1,1,0,0},{0,0,0,1,1,0} };
	
	int t,j,actastrat;			/*t indexes time; j indexes att options; active attacker strat*/
	int tstart, tend;				/*beginning and end day of diversion*/
	int m;							/*schedule array dimensions-- detection days*/
	int k;							/*indexes safeguards*/
	int inspday = 15;				/*regular inspection day-- 15th of each month*/
	int inspinterval = 30;			/*inspection every 30 days*/
	float nomcylmass[2] = {27360,5180};	/*feed and product nominal cylinder weights (kg)*/
	float erfinv[3] = {-1.6450,-2.1851,-2.6297};	/*inverf(2*FAP-1) for FAP = 0.01, 0.001, 0.0001*/
	
	/*set actastrat equal to the number of the active attacker strategy*/
	/*defender and attacker strategies are referenced BY THEIR INDICES (strategy number -1)*/
	
	for (j=0;j<5;j++)
		if (astrat[j].active == 1)
			actastrat = j;
	
	/*initialize schedule arrays-- run 30 days past end of diversion*/	
	tstart = astrat[actastrat].tstart;
	tend = astrat[actastrat].tend;	
	
	if (astrat[actastrat].cont == 0)		/*if its a discrete attacker strategy*/
		m = 30+tstart;
	else								/*continuous attacker strategy*/
		m = tend+30;

	int aschedule[m], dschedule[9][m];
	
/*populate schedule arrays*/
	
	for(t=0;t<m;t++)
		aschedule[t] = 0;
		
	if (astrat[actastrat].cont == 0)	/*if attacker strategy is disrete*/
		aschedule[tstart-1]=1;		/*attacker activity on first day, nothing on rest of days*/ 
	else
		for(t=tstart-1;t<tend;t=t+astrat[actastrat].freq)	/*activity of first day and repeating every f days (f=freq) */
			aschedule[t] = 1;
	
	for(k=0;k<9;k++)
		for(t=0;t<m;t++)
			dschedule[k][t] = 0;	
		
	for(k=0;k<1;k++)	{		/*safeguards that occur during regular inspections-- every 30 days*/
		if(dstrat[k].active == 1)	{	/*if discrete safeguard is active*/
			dschedule[k][inspday-1]=1;	/*fill in inspection days in schedule matrix*/
			for(t=inspday-1;t<m;t=t+30)
				dschedule[k][t] = 1;
		}	
	}	
	
	/*passive seal inspection takes place on inspection days but post-mortem analysis takes 10 days
	(~ 7 working days)*/
	if (dstrat[2].active == 1)	{
		dschedule[2][inspday+10-1] = 1;
		for(t=inspday+10-1;t<m;t=t+30)
				dschedule[2][t] = 1;
	}
		
	if (dstrat[3].active == 1)	{	/*safeguards that occur during regular inspections-- every 30 days*/
		dschedule[3][inspday-1] = 1;
		for(t=inspday-1;t<m;t=t+30)
				dschedule[3][t] = 1;
	}
	
	/*DA takes place on inspection days but  analysis takes 14 days
	(~ 7 working days)*/
	if (dstrat[4].active == 1)	{
		dschedule[4][inspday+14-1] = 1;
		for(t=inspday+14-1;t<m;t=t+30)
				dschedule[4][t] = 1;
	}
	
	if(dstrat[5].active == 1)	{	/*safeguards that occur during regular inspections-- every 30 days*/
		dschedule[5][inspday-1]=1;	/*fill in inspection days in schedule matrix*/
		for(t=inspday-1;t<m;t=t+30)
			dschedule[k][t] = 1;
	}	

	if (dstrat[6].active == 1)
		for (t=0;t<m;t++)
			if (aschedule[t] == 1)
				dschedule[6][t] = 1;
	
	for(k=7;k<9;k++)
		if(dstrat[k].active == 1)	/*if continuous safeguard is active*/
			for (t=0;t<m;t++)		/*schedule event every day*/
				dschedule[k][t] = 1;
	
	/************CHECK************/
	printf("defender schedule:\n");
	for (t=0;t<m;t++)	
		printf("%d ",t+1);
	printf("\n");
	for (t=0;t<m;t++)
		printf("%d ",dschedule[1][t]);
	printf("\n");
	for (t=0;t<m;t++)	
		printf("%d ",dschedule[2][t]);
	printf("\n");
	
	printf("attacker schedule:\n");
	for (t=0;t<m;t++)	
		printf("%d ",t+1);
	printf("\n");
	for (t=0;t<m;t++)
		printf("%d ",aschedule[t]);
		printf("\n");
	
	/*loop through time and safeguards to populate dailyDP array
		time loop	{
			safeguard loop	{
				if k is active
					if k is effective against attacker strategy
						calculate DP							*/
	
	float dailyDP[9][m];		/*holds DP for each day for each safeguards over entire period*/

	for (k=0;k<9;k++)
		for(t=0;t<m;t++)
			dailyDP[k][t] = 0;
			
	/***update this code so that it doesnt scan through any safeguards that are NEVER active***/
	printf("%d\n",actastrat);
	int inspindex[9];				/*track how many inspections have occurred*/
	for (k=0;k<9;k++)
		inspindex[k] = 0;
	
	for (t=tstart-1;t<m;t++)	{	/*index through days*/
		for (k=0;k<9;k++)	{	/*index through safeguards*/
			if (dschedule[k][t] == 1)	{			/*if safeguard is active that day*/
				printf("\n%d active on day %d\n",k,t+1);
				if (effstrats[k][actastrat] == 1){	/*if safeguards is effective against att. strat*/
					printf("%d effective\n",k);
					if (actastrat == 0)	{			
						printf("cylinder theft\n");
						if (k==0)	{				/*inventory*/
							printf("%s\n",dstrat[k].name);
							if (inspindex[k] == 0)	{
								printf("first inspection\n");
								dailyDP[k][t] = 1-exp(-0.65*(dstrat[k].dep+1)*astrat[k].nitems);
								inspindex[k]++;	
								printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]);
							}
							else	{
								dailyDP[k][t] = 1-((1+dstrat[k].dep*(1-dailyDP[k][t-inspinterval]))/(dstrat[k].dep+1));
								printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]);
							}
						}
						if (k==5)	{
							printf("%s\n",dstrat[k].name);
							while (inspindex[k] == 0)	{
								dailyDP[k][t] = 0.43;
								inspindex[k]++;
							}
							inspindex[k]++;
							printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]);
						}
						if (k==6)	{
							printf("%s\n",dstrat[k].name);
							while (inspindex[k] == 0)	{
								dailyDP[k][t] = 0.64;
								inspindex[k]++;
							}
							inspindex[k]++;
							printf("DP on day% d: %.2f\n",t+1,dailyDP[k][t]);
						} 
					}	/*cylinder theft*/ /*DONE*/
					if (actastrat == 1) {	/*some material from cylinder*/
						if (k == 1)	{
							printf("%s\n",dstrat[k].name);
								if (astrat[actastrat].area == 0)	{	/*feed cylinder*/
									
								}
						}
						if (k == 2)	{
							printf("%s\n",dstrat[k].name);
							float DPpseal[15];
						
							
						}
						if (k == 3)	{
							printf("%s\n",dstrat[k].name);
							
						}
						if (k == 4)	{
							printf("%s\n",dstrat[k].name);
							
						}
						if (k == 5)	{
							float DPlog[15];		/*use to handle HRA for per instance DP*/
							DPlog[0] = 0.43;
							printf("%s\n",dstrat[k].name);
							int i,*pntevents ;		/*theft events between inspections*/
							int ntevents = 0;
							pntevents = &ntevents;
							/*calculate diversion events between inspections-- split into two parts
							to avoid negative indexing in first 30 days*/
							if (t>=30)	{
								for (i=t-(inspinterval-1);i<=t;i++)	/*inspinterval-1 bc day 15 was
								already accounted for in previous interval*/
									if (aschedule[i] == 1)
										(*pntevents)++;						
								for (i=t-(inspinterval-1);i<=t;i++)	/*look at diversions events*/
									printf("%d ",i+1);
								printf("\n");
								for (i=t-(inspinterval-1);i<=t;i++)	
									printf("%d  ",aschedule[i]);
								printf("number of events %d\n",ntevents); 
							}
							else	{
								for (i=0;i<inspday;i++)	
								/*if diversion happens on inspday (day 15), it counts in that interval*/ 
									if (aschedule[i]==1)
										(*pntevents)++;
								for (i=0;i<inspday;i++)	
									printf("%d ",i+1); 
								printf("\n");
								for (i=0;i<inspday;i++)	
									printf("%d ",aschedule[i]); 
								printf("\n");
								printf("number of events %d\n",ntevents);
							}
							/*calculate DP-- need to use previous per instance DP, which is stored in
							DPlog. Find cumulative sum of per instance DPs for all instances
							that have occured since last inpsection*/
							if (inspindex[k] == 0)		{
								dailyDP[k][t] = 1-pow((1-DPlog[0]),ntevents);
								printf("Per instance DP on insp. %d: %.2f\n",inspindex[k]+1	,DPlog[inspindex[k]]);
								inspindex[k]++;
							}
							else	{
								DPlog[inspindex[k]] = 1-((1+dstrat[k].dep*(1-DPlog[inspindex[k]-1]))/(dstrat[k].dep+1)); 
								printf("Per instance DP on insp. %d: %.2f\n",inspindex[k]+1	,DPlog[inspindex[k]]);
								dailyDP[k][t] = 1-pow((1-DPlog[inspindex[k]]),ntevents);
								inspindex[k]++;
							}
							printf("DP: %.2f\n",dailyDP[k][t]);
						}
						if (k == 6)	{
							printf("%s\n",dstrat[k].name);
							if (t>=(tstart-1) && inspindex[k] == 0)	{ /*if the diversion has started
							and this is DP initial for video trans*/
								dailyDP[k][t] = 0.64;
								inspindex[k]++; 
								printf("DP on day %d: %.2f\n",t+1,dailyDP[k][t]);				
							}
							else if (inspindex[k] != 0)	{	/*if it isn't DP initial-- HRA*/
								printf("DP: %.2f\n",dailyDP[k][t-astrat[actastrat].freq]); 
								dailyDP[k][t] = 1-(1+dstrat[k].dep*(1-dailyDP[k][t-astrat[actastrat].freq]))/
												(dstrat[k].dep+1);
								inspindex[k]++;
								printf("DP on day %d: %.2f\n",t+1,dailyDP[k][t]); 
							} 
						}
						if (k == 7)	{
							printf("%s\n",dstrat[k].name);
							
						}
					}	/*some material from cylinder*/
					if (actastrat == 2)	{
						if (k == 1)	{
							float ntevents;
							float ffreq; /*float version of frequency to use in this block*/
							ffreq = astrat[actastrat].freq;
							/*total number of theft events*/;
							if (t<=tend)	
								ntevents = ceil(((t+1)-tstart)/ffreq);
							else
								ntevents = ceil((tend-tstart)/ffreq)+1;
							printf("%d\t%d\t",tstart,t+1);
							printf("theft events: %.2f\n",ntevents);
							float totmass = ntevents * astrat[actastrat].deltam * astrat[actastrat].nitems;
							printf("total mass: %.1f\n",totmass);
							/*total mass stolen is events*mass/event*cascades/event-- in grams*/
							float thresh = nomcylmass[1]+sqrt(2)*(0.0007*nomcylmass[1])*erfinv[(dstrat[k].fap)-1];
							dailyDP[k][t] = 1-0.5*(1-erf((thresh-(nomcylmass[1]-totmass/1000))/(sqrt(2)*
											0.0007*(nomcylmass[1]-totmass/1000))));
							printf("DP is %0.4f on day %d from safeguard %s\n",dailyDP[k][t],t+tstart-1,dstrat[k].name);
							inspindex[k]++;
						}
						if (k == 7)	{
							int nseals = astrat[actastrat].nitems * dstrat[k].number;	
							/*# seals broken*/
							if (inspindex[k] == 0)	{
								dailyDP[k][t] = 1 - pow((1-0.40),(nseals));
								printf("DP is %0.3f on day %d from safeguard %s\n",dailyDP[k][t],t+tstart-1,dstrat[k].name);
								inspindex[k]++;
							}
							else	{
								dailyDP[k][t] = 0;
								inspindex[k]++;
							}						
					
						}
					}	/*some material from cascade*/ /*DONE*/
					if (actastrat == 3)	{
						if (k == 1)	{
						
						}
						if (k == 2)	{
						
						}
						if (k == 3)	{
						
						}
						if (k == 4)	{
						
						}
						if (k == 7)	{
						
						}
						if (k == 8)	{
						
						}
					}	/*repiping*/
					if (actastrat == 4)	{
						if (k == 1)	{
						
						}
						if (k == 2)	{
						
						}
						if (k == 3)	{
						
						}
						if (k == 4)	{
						
						}
						if (k == 8)	{
						
						}
					}	/*recycle*/
					if (actastrat == 5)	{
					/*nothing right now-- only LFUAs effective against this*/
					}	/*undeclared feed*/ /*CAN'T DO YET*/
				}
			}
		}
	}
	
	/********CHECK*******/
/*	printf("\n");
	printf("\n");
	for(k=0;k<9;k++){
		for(t=0;t<m;t++)
			printf("%.2f ",dailyDP[k][t]);
		printf("\n");	
	}	*/

}

