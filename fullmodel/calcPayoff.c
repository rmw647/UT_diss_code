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

struct sginfo dstratinfo[NUMSG] = {inspectioni,psealsi,ndai,dai,videotransi,asealsi,cemoi,
									visinspi,esi,dualcsi,divi,smmsi,rdai};


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
	/*if the SG is active, schedule it*/
	if (dstrat.SG[k].active == 1)	{	
		/*if it's continuous, schedule it every day*/
		if (dstratinfo[k].cont == 1)	{/*if it's a continuous defender strategy*/
			for (t=0;t<m;t++)	{
				dschedule[k][t] = 1;	/*schedule it everyday*/
				/*printf("%d ",dschedule[k][t]);
				printf("\n");*/
			}	
		}		
		/*if it's any SG besides DIV, schedule it according to freq*/
		else if (strcmp(dstrat.SG[k].name,"div")!=0) 	{
			for (t=dstrat.SG[k].freq-1;t<m;t=t+dstrat.SG[k].freq)	{
				dschedule[k][t] = 1;		/*it's active on insp days*/
				/*printf("%d ",dschedule[k][t]);
				printf("\n");*/
			}
		}
		/*if it's DIV, schedule as same frequency as dualCS*/
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
float inspDP = 0;					/*holds inspection DP to aggregate over insp. activities*/
float cylDP = 0;					/*per cylinder DP*/
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
int attackssinceres = 0;			/*attacks since residency period has started*/
									
float cumdrdDP = 0;					/*use for drd validation*/

/*use to handle HRA for per instance DP--- here use the frequency of inspection bc
that must be the most frequent safeguard, so array will definitely have enough space*/
int arraydim = 0;		/*dimensions of DPlog array*/
if (dstrat.SG[0].active == 1)
	arraydim = ceil(m/dstrat.SG[0].freq)+1;	

float DPlog[arraydim];

/*multiply mass of UF6 by this number to find mass of U-- 0 position is for feed, 1 position
is for product*/
float UF6toU[2] = {(xfeed*MU235 + (1-xfeed)*MU238)/(xfeed*MU235 + (1-xfeed)*MU238 + 6*MF),
					(xprod*MU235 + (1-xprod)*MU238)/(xprod*MU235 + (1-xprod)*MU238 + 6*MF)};
				
/*use integer set here to refer to position in FOM array-- ex: NU is FOM[0]*/
enum MAT	{
		NU,
		LEU,
		LEU197,
		HEU50,
		HEU90,
		SF,
		TRU
};
	
/*hold mass of uranium diverted each day at enrichment*/
for (t=0;t<m;t++)					/*initialize to O's*/
	massU[t] = 0;

/*hold mass of TRU diverted each day at reprocessing*/
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
	
int resinspindex[NUMSG];				/*number of inspections that have occurred during 
product cylinder residence period-- used for mbalance for matcyl-- ranges from 1-4*/

for (k=0;k<NUMSG;k++)
	resinspindex[k] = 0;
	
float invDP[m];					/*holds inventory DP-- used for HRA*/
for (t=0;t<m;t++)
    invDP[t] = 0;
		
/*** find active attacker strategy and assign number ***/
/** this is only used for "effstrats"*/

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
else if (strcmp(astrat.AO.name,"chopsf") == 0)
	actastrat = 6;
else if (strcmp(astrat.AO.name,"trusol") == 0)
	actastrat = 7;
	
/*this loop calculates material quantity and quality*/
for (t=0;t<m;t++)	{
	if (aschedule[t] == 1)	{
		if (strcmp(astrat.AO.name,"cyltheft") == 0)	{	
			/*mass obtained is material in one cylinder times num of cyls, need
			to convert from UF6 to U-- 
			material type is NU or LEU, depending on area of theft*/
			massU[t] = UF6toU[astrat.AO.area] * astrat.AO.nitems * cylUF6mass[astrat.AO.area];
			mattype = FOM[astrat.AO.area];
		}
		if (strcmp(astrat.AO.name,"matcyl") == 0) {	
			/*mass obtained is target mass in attacker strategy/attack events
			-- NU or LEU*/
			massU[t] = UF6toU[astrat.AO.area] * astrat.AO.deltam/(ceil((float)tend/astrat.AO.freq));
			mattype = FOM[astrat.AO.area];
		}
		if (strcmp(astrat.AO.name,"matcasc") == 0) {
			/*mass obtained is numcasc * frac attacked * deltam--LEU*/
			massU[t] = UF6toU[LEU] * astrat.AO.nitems * NUMCASC * astrat.AO.deltam;
			mattype = FOM[LEU];
		}
		if (strcmp(astrat.AO.name,"repiping") == 0)	{
			/*mass obtained is product mass produced given frac cascades dedicated
			and duration-- 19.7%,50%,90%*/
			massU[t] = calcMassFlow(xfeed,astrat.AO.xp,xw,astrat.AO.nitems);
			if (astrat.AO.xp == xp[0])
				mattype = FOM[LEU197];
			else if (astrat.AO.xp == xp[1])
				mattype = FOM[HEU50];
			else if (astrat.AO.xp == xp[2])
				mattype = FOM[HEU90];
		}
		if (strcmp(astrat.AO.name,"recycle") == 0)	{
			/*mass obtained is product mass produced given frac cascades dedicated
			and duration-- 19.7%,50%,90%*/
			massU[t] = calcMassFlow(xprod,astrat.AO.xp,xw,astrat.AO.nitems);
			if (astrat.AO.xp == xp[0])
				mattype = FOM[LEU197];
			else if (astrat.AO.xp == xp[1])
				mattype = FOM[HEU50];
			else if (astrat.AO.xp == xp[2])
				mattype = FOM[HEU90];
		}
		if (strcmp(astrat.AO.name,"udfeed") == 0)	{
			/*mass obtained is amount produced based on dedicated capacity-- 
			LEU*/
			massU[t] = calcMassFlow(xfeed,xprod,xw,astrat.AO.nitems);
			mattype = FOM[LEU];
		}
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
					/*cylinder theft*/
					if (strcmp(astrat.AO.name,"cyltheft") == 0)	{
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{	
		
						/*	printf("%s\n",dstrat.SG[k].name); */
						
							if (inspindex[k] == 0)	{
							/*	printf("first inspection\n"); */
							
								/*calc inventory DP-- store as inspDP*/
								invDP[inspindex[k]] = 1-exp(-0.65*(dstrat.SG[k].dep+1)
														*astrat.AO.nitems);
								inspDP = invDP[inspindex[k]];
								printf("invDP:%.3f\n",invDP[inspindex[k]]);
								
								/*calc videolog DP*/
								DP = VIDEOLOGDP;	

								/*update inspDP to include videolog DP*/			
								inspDP = 1-(1-inspDP)*(1-DP);	
								printf("inspDP:%.3f\n",inspDP);
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
								printf("invDP:%.3f\n",invDP[inspindex[k]]);
								
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
							
							int i = 0;
							DPlog[0] = VIDEOLOGDP;
					
							int tindex =0;	/*theft events between inspections index*/
							int ntevents = 0;	/*number of theft events*/
							
							/*calculate diversion events between inspections*/
							/*for (tindex=t-dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									ntevents++;
							printf("ntevents: %d\n",ntevents);*/
							printf("attacks since insp: %d\n",attackssinceinsp);
					
							/*calculate DP-- need to use previous per instance DP, which 
							is stored in DPlog. Find cumulative sum of per instance DPs 
							for all instances that have occured since last inpsection*/
							
							/*if it's the first inspection*/
							if (inspindex[k] == 0)		{
								inspDP = 1-pow((1-DPlog[0]),attackssinceinsp);
								printf("vl DP on day %d: %.3f\n",t,DPlog[inspindex[k]]);
								inspindex[k]++;
							}
							/*if it's not the first inspection*/
							else	{
								DPlog[inspindex[k]] = 1-((1+dstrat.SG[k].dep*
									(1-DPlog[inspindex[k]-1]))/(dstrat.SG[k].dep+1)); 
								inspDP = 1-pow((1-DPlog[inspindex[k]]),attackssinceinsp);
								printf("vl DP on day %d: %.3f\n",t,DPlog[inspindex[k]]);
								inspindex[k]++;
							}
							printf("inspDP: %.3f\n",inspDP);
							/***calc mbalance DP and combine into inspDP***/
							
							float totalevents = 0;	/*total number of theft events for entire scenario*/
							float massperevent = 0; /*total mass taken in each event*/
							float masspercyl = 0;	/*mass taken per cyl per event*/
							float totalmasspercyl = 0;	/*total mass per cyl over period*/
							float events = 0;		/*events during residence period*/
							float threshold=0;			/*threshold weight*/
							float nomreading=0;				/*nominal weight*/
							
							/*total events over course of attack*/
							totalevents = ceil((float)tend/astrat.AO.freq);
							
							/*total mass taken in each attack event*/
							massperevent = astrat.AO.deltam/totalevents;
							
							/*mass taken per each cylinder per event*/
							masspercyl = massperevent/astrat.AO.nitems;
						
							if (astrat.AO.area == area[1])	{
								/*calculate attack events during current product cyl residence period*/
								if (resinspindex[k] < (RESPERIOD/dstrat.SG[k].freq))	{
									resinspindex[k]++;	
									attackssinceres = attackssinceres + attackssinceinsp;
								}
								else	{
									resinspindex[k] = 1;
									attackssinceres = attackssinceinsp;
								}
							/*	for (tindex=t-resinspindex[k]*dstrat.SG[k].freq+1;tindex<=t;tindex++)	
									if (aschedule[tindex] == 1)		
										events++;*/
							}
							
							if (astrat.AO.area == area[0])	{
							/*calculate attack events during current feed cyl residence period*/
								if (resinspindex[k] < (FEEDRES/dstrat.SG[k].freq))	{
									resinspindex[k]++;	
									attackssinceres = attackssinceres + attackssinceinsp;
								}
								else	{
									resinspindex[k] = 1;
									attackssinceres = attackssinceinsp;
								}
							/*	for (tindex=t-resinspindex[k]*dstrat.SG[k].freq+1;tindex<=t;tindex++)
									if (aschedule[tindex] == 1)	
										events++;*/
							}
							printf("resinspindex: %d\n",resinspindex[k]);
							printf("events: %.0f\n",events);
							printf("events during res: %d\n",attackssinceres);		
							/*total mass taken from cylinder (during residency period)*/
							totalmasspercyl = masspercyl*attackssinceres;	
							
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
							cylDP = 1-0.5*(1-erf((threshold-(nomreading-totalmasspercyl))/
										(sqrt(2)*mberror*(nomreading-totalmasspercyl))));
								
							/*if multiple cylinders are taken, find cum DP*/
							DP = 1-pow((1-cylDP),astrat.AO.nitems);
							printf("mbDP:%.3f\n",DP);
							/*update inspDP*/
							inspDP = 1-(1-inspDP)*(1-DP);
								
							/*make dailyDP equal inspDP*/
							dailyDP[k][t] = inspDP;
							attackssinceinsp = 0;
							
						}
						if (strcmp(dstrat.SG[k].name,"pseals")==0)		{
							/*1 if attack happened since last inspection, 0 otherwise*/
							int attackhappened = 0;
							/*scan through days since last inspection and see if an 
							  attack has occurred. If so, a new detection opportunity
							  exists*/
							int i=0;
							/*for (i=t+1-dstrat.SG[k].freq;i<t;i++)
								if (aschedule[i] == 1)	{
									attackhappened = 1;
									break;
								}*/
							if ((attackday > t-dstrat.SG[k].freq) && (attackday <= t))
									attackhappened = 1;
							/*printf("attackhappened:%d\n",attackhappened);*/
							if (attackhappened == 1)	{
								/*if defender inspects all seals*/
								if (dstrat.SG[k].number == numseals[1])	{
									if (t+dstratinfo[k].analysis <= m)
										dailyDP[k][t+dstratinfo[k].analysis] = 
											1- pow((1-PSEALSDP),astrat.AO.nitems);	
								}
								/*if defender inspects half seals*/
								else	{
									float evasionprob;	/*evasion prob-- 1-P-- probability
													an unsealed cylinder is chosen*/
									/*N- number cyls = numcyl[0] for area = 0 and numcyl[1] for 
									area = 1
									r - violated cyls = astrat.AO.nitems
									n - sealed cyls = dstrat.SG[k].number * numcyl -- fraction
									sealed * total number*/

									if (numcyl[astrat.AO.area] == astrat.AO.nitems)
										evasionprob = 0;
									else	{
										evasionprob = (float)choose((numcyl[astrat.AO.area]-astrat.AO.nitems),
													dstrat.SG[k].number*numcyl[astrat.AO.area])/
													(float)choose(numcyl[astrat.AO.area],
													dstrat.SG[k].number*numcyl[astrat.AO.area]);
									}
									/*inspection occurs days later bc of analysis*/
									if (t+dstratinfo[k].analysis <= m)
										dailyDP[k][t+dstratinfo[k].analysis] = (1-evasionprob)*PSEALSDP;
								}
							}
							inspindex[k]++;
							printf("DP on day %d: %.3f\n",t+dstratinfo[k].analysis,
									dailyDP[k][t+dstratinfo[k].analysis]);
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
						if (strcmp(dstrat.SG[k].name,"visinsp")==0)	{
							if (t <= tend)	{
								dailyDP[k][t] = VISINSPDPNC;
								inspindex[k]++;
							}
						}
					}	
					/*some material from the cascade*/
					if (strcmp(astrat.AO.name,"matcasc") == 0) {
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{
							/*** mass balance DP ***/
							float cylDP =0;	/*per cylinder DP*/
							
							/*total number of theft events so far in residency period*/;
							/*float events = 0;*/
							int tindex = 0;	/*indexes time in loop that calcs events*/
							
							/*calculate attack events during current product cyl residence period*/
							printf("%d\n",RESPERIOD/dstrat.SG[k].freq);
							if (resinspindex[k] < (RESPERIOD/dstrat.SG[k].freq))	{
								resinspindex[k]++;	
								attackssinceres = attackssinceres + attackssinceinsp;
							}
							else	{
								resinspindex[k] = 1;
								attackssinceres = attackssinceinsp;
							}
							/*for (tindex=t-resinspindex[k]*dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									events++;	*/

							/*calculate total mass taken since start of residency period*/
							float totalmass = attackssinceres * astrat.AO.deltam * astrat.AO.nitems
											* NUMCASC;
							printf("total mass: %.3f\n",totalmass);
							/*total missing mass is divided among product cylinders*/
							float totalmasspercyl = totalmass/numcyl[1];
							printf("total mass per cyl: %.3f\n",totalmasspercyl);
							/*if more than one cylinder's worth is missing, detection is 
							certain*/				
								
							float thresh = nomcylmass[1]+sqrt(2)*(0.0007*nomcylmass[1])*
											erfinv[(dstrat.SG[k].fap)-1]*-1;
							printf("threshold: %.3f\n",thresh);
							cylDP = 1-0.5*(1-erf((thresh-(nomcylmass[1]-totalmasspercyl))
											/(sqrt(2)*0.0007*(nomcylmass[1]-totalmasspercyl))));
							printf("cylDP: %.3f\n",cylDP);
							dailyDP[k][t] = 1-pow((1-cylDP),numcyl[1]);
							printf("dailyDP: %.3f\n",dailyDP[k][t]);
							inspindex[k]++;
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
									printf("seals broken: %d\n",nseals); 
									dailyDP[k][t] = 1 - pow((1-ASEALSDP),(nseals));
									printf("DP is %0.3f on day %d from safeguard %s\n",
										dailyDP[k][t],t+1,dstrat.SG[k].name); 
									inspindex[k]++;
									}	
								}				
						}
					}
						if (strcmp(dstrat.SG[k].name,"visinsp")==0)	{
							if (t <= tend)	{
								dailyDP[k][t] = VISINSPDPNC;
								inspindex[k]++;
							}
						}
					}
					/*repiping*/
					if (strcmp(astrat.AO.name,"repiping") == 0)	{
						if (strcmp(dstrat.SG[k].name,"nda")==0)	{
							int tindex;		/*use to index time*/
							int attackhappened = 0;	/*did an attack occur since last inspection*/
							/*calculate attack events during current product cyl residence period*/
							if (resinspindex[k] < ((float)RESPERIOD/dstrat.SG[k].freq))	{
								resinspindex[k]++;	
							}
							else	{
								resinspindex[k] = 1;
							}
							/*change attackhappened to 1 if attack happened this res period*/
							/*for (tindex=t-resinspindex[k]*dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									attackhappened = 1;	*/
							if ((attackday > t-resinspindex[k]*dstrat.SG[k].freq) &&
									(attackday <= t))
									attackhappened = 1;
							/*if an attack did not happen this residency period, no bad
							cylinder in storage, DP = 0*/
							if (attackhappened == 0)
								dailyDP[k][t] = 0;
								
							else 	{
							
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
							inspindex[k]++;
						}	
						if (strcmp(dstrat.SG[k].name,"da")==0)	{
							int tindex;		/*use to index time*/
							int attackhappened = 0;	/*did an attack occur since last inspection*/
							/*calculate attack events during current product cyl residence period*/
							if (resinspindex[k] < ((float)RESPERIOD/dstrat.SG[k].freq))	{
								resinspindex[k]++;	
							}
							else	{
								resinspindex[k] = 1;
							}
							/*change attackhappened to 1 if attack happened this res period*/
							/*for (tindex=t-resinspindex[k]*dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									attackhappened = 1;	*/
							if ((attackday > t-resinspindex[k]*dstrat.SG[k].freq) &&
									(attackday <= t))
									attackhappened = 1;
							/*if an attack did not happen this residency period, no bad
							cylinder in storage, DP = 0*/
							if (attackhappened == 0)	{
								if (t+dstratinfo[k].analysis <= m)	{
									dailyDP[k][t+dstratinfo[k].analysis] = 0;
								}
							}
								
							else 	{
								/*nondetection prob (1-DP)*/
								float nonDP = 0;
								/*if the inspector samples every cylinder in storage*/
								if (numcyl[1] == dstrat.SG[k].number)
									nonDP = 0;
								/*if she doesn't*/
								else	{
								
								/* N = numcyl[1]-- number cylinders in product storage
								   r = NUMCYLFALSE-- assume all overenriched product placed in 
										one storage cylinder
								   n = dstrat.SG[k].number*ncyl[1]-- percentage cylinders
										sampled times number cylinders */
								printf("first: %.3f\n",numcyl[1]-NUMCYLFALSE);
								printf("second: %.3f\n",dstrat.SG[k].number);
								nonDP = (float)choose(numcyl[1]-NUMCYLFALSE,dstrat.SG[k].number)/
										(float)choose(numcyl[1],dstrat.SG[k].number);
								}
						
								if (t+dstratinfo[k].analysis <= m)
									dailyDP[k][t+dstratinfo[k].analysis] = 1-nonDP;
								
							/*	printf("DP is %0.3f on day %d from safeguard %s\n",
									dailyDP[k][t+dstratinfo[k].analysis],
									t+dstratinfo[k].analysis+1,dstrat.SG[k].name); */
							}
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
						if (strcmp(dstrat.SG[k].name,"visinsp")==0)	{
							if (t <= tend)	{
								/*if 50% of cascades are repiped*/
								if (astrat.AO.nitems == fcasc[2])	{
									dailyDP[k][t] = VISINSPDPC;
									inspindex[k]++;
								}
								/*if 1.6% of 10% are repiped*/
								else	{
									dailyDP[k][t] = VISINSPDPNC;
									inspindex[k]++;
								}
							}
						}
						if (strcmp(dstrat.SG[k].name,"es")==0)	{			
							int tindex;		/*use to index time*/
							int attackhappened = 0;	/*did an attack occur since last inspection*/
							/*calculate attack events during current product cyl residence period*/
							if (resinspindex[k] < ((float)RESPERIOD/dstrat.SG[k].freq))	{
								resinspindex[k]++;	
							}
							else	{
								resinspindex[k] = 1;
							}
							/*change attackhappened to 1 if attack happened this res period*/
							/*for (tindex=t-resinspindex[k]*dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									attackhappened = 1;	*/
							if ((attackday > t-resinspindex[k]*dstrat.SG[k].freq) &&
									(attackday <= t))
									attackhappened = 1;
							/*if an attack did not happen this residency period, no bad
							cylinder in storage, DP = 0*/
							if (attackhappened == 0)	{
								if (t+dstratinfo[k].analysis <= m)	{
									dailyDP[k][t+dstratinfo[k].analysis] = 0;
								}
							}
								
							else 	{

							float nonDP = 0;
							/* N = number of cascades
							   r = number of cascades used in misuse
							   n = number of swipes */
							int r = astrat.AO.nitems*NUMCASC;
							int n = dstrat.SG[k].number;
							nonDP = (float)choose(NUMCASC-r,n)/(float)choose(NUMCASC,n);
							dailyDP[k][t+dstratinfo[k].analysis] = 1-nonDP;
							inspindex[k]++;
							}
						}		
					}
					/*recycle*/
					if (strcmp(astrat.AO.name,"recycle") == 0)	{
						if (strcmp(dstrat.SG[k].name,"nda")==0)	{
							int tindex;		/*use to index time*/
							int attackhappened = 0;	/*did an attack occur since last inspection*/
							/*calculate attack events during current product cyl residence period*/
							if (resinspindex[k] < RESPERIOD/dstrat.SG[k].freq)	{
								resinspindex[k]++;	
							}
							else	{
								resinspindex[k] = 1;
							}
							/*change attackhappened to 1 if attack happened this res period*/
							/*for (tindex=t-resinspindex[k]*dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									attackhappened = 1;	*/
							if ((attackday > t-resinspindex[k]*dstrat.SG[k].freq) &&
									(attackday <= t))
									attackhappened = 1;
							printf("attackhappened: %d\n",attackhappened);
							/*if an attack did not happen this residency period, no bad
							cylinder in storage, DP = 0*/
							if (attackhappened == 0)
								dailyDP[k][t] = 0;
								
							else 	{
							
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
								
								printf("erf: %.12f\n",erf((thresh-astrat.AO.xp/xprod*peakcounts)/
												(sqrt(2)*astrat.AO.xp/xprod*stddev)));
								
								dailyDP[k][t] = 1- 0.5*(1+erf((thresh-astrat.AO.xp/xprod*peakcounts)/
												(sqrt(2)*astrat.AO.xp/xprod*stddev))); 
							}
							inspindex[k]++;
						}
						if (strcmp(dstrat.SG[k].name,"da")==0)	{
							int tindex;		/*use to index time*/
							int attackhappened = 0;	/*did an attack occur since last inspection*/
							/*calculate attack events during current product cyl residence period*/
							if (resinspindex[k] < ((float)RESPERIOD/dstrat.SG[k].freq))	{
								resinspindex[k]++;	
							}
							else	{
								resinspindex[k] = 1;
							}
							/*change attackhappened to 1 if attack happened this res period*/
							/*for (tindex=t-resinspindex[k]*dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									attackhappened = 1;	*/
							if ((attackday > t-resinspindex[k]*dstrat.SG[k].freq) &&
									(attackday <= t))
									attackhappened = 1;
							/*if an attack did not happen this residency period, no bad
							cylinder in storage, DP = 0*/
							if (attackhappened == 0)	{
								if (t+dstratinfo[k].analysis <= m)	{
									dailyDP[k][t+dstratinfo[k].analysis] = 0;
								}
							}
								
							else 	{
								/*nondetection prob (1-DP)*/
								float nonDP = 0;
								if (numcyl[1] == dstrat.SG[k].number)
									nonDP = 0;
								/*if she doesn't*/
								else	{
								
								/* N = numcyl[1]-- number cylinders in product storage
								   r = NUMCYLFALSE-- assume all overenriched product placed in 
										one storage cylinder
								   n = dstrat.SG[k].number*ncyl[1]-- percentage cylinders
										sampled times number cylinders */
								nonDP = (float)choose(numcyl[1]-NUMCYLFALSE,dstrat.SG[k].number)/
										(float)choose(numcyl[1],dstrat.SG[k].number);
								}
						
								if (t+dstratinfo[k].analysis <= m)
									dailyDP[k][t+dstratinfo[k].analysis] = 1-nonDP;
								
							/*	printf("DP is %0.3f on day %d from safeguard %s\n",
									dailyDP[k][t+dstratinfo[k].analysis],
									t+dstratinfo[k].analysis+1,dstrat.SG[k].name); */
							}
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
							printf("DP on day %d:%.3f\n",t+1,dailyDP[k][t]);
							}
						}
						if (strcmp(dstrat.SG[k].name,"visinsp")==0)	{
							if (t <= tend)	{
								dailyDP[k][t] = VISINSPDPNC;
								inspindex[k]++;
							}
						}
						if (strcmp(dstrat.SG[k].name,"es")==0)	{			
							int tindex;		/*use to index time*/
							int attackhappened = 0;	/*did an attack occur since last inspection*/
							/*calculate attack events during current product cyl residence period*/
							if (resinspindex[k] < ((float)RESPERIOD/dstrat.SG[k].freq))	{
								resinspindex[k]++;	
							}
							else	{
								resinspindex[k] = 1;
							}
							/*change attackhappened to 1 if attack happened this res period*/
							/*for (tindex=t-resinspindex[k]*dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									attackhappened = 1;	*/
							if ((attackday > t-resinspindex[k]*dstrat.SG[k].freq) &&
									(attackday <= t))
									attackhappened = 1;
							/*if an attack did not happen this residency period, no bad
							cylinder in storage, DP = 0*/
							if (attackhappened == 0)	{
								if (t+dstratinfo[k].analysis <= m)	{
									dailyDP[k][t+dstratinfo[k].analysis] = 0;
								}
							}
								
							else 	{

							float nonDP = 0;
							/* N = number of cascades
							   r = number of cascades used in misuse
							   n = number of swipes */
							int r = astrat.AO.nitems*NUMCASC;
							int n = dstrat.SG[k].number;
							nonDP = (float)choose(NUMCASC-r,n)/(float)choose(NUMCASC,n);
							dailyDP[k][t+dstratinfo[k].analysis] = 1-nonDP;
							inspindex[k]++;
							}
						}
					}	
					/*undeclared feed*/
					if (strcmp(astrat.AO.name,"udfeed") == 0)	{
						if (strcmp(dstrat.SG[k].name,"inspection")==0)	{
							/*only videolog is effective*/

							int i = 0;
							/*divide by two bc no surveillance in cascade hall and not
							looking for trigger in storage area-- would need careful review*/
							DPlog[0] = VIDEOLOGDP/2;
					
							int tindex =0;	/*theft events between inspections index*/
							int ntevents = 0;	/*number of theft events*/
							
							/*calculate diversion events between inspections*/
							/*for (tindex=t-dstrat.SG[k].freq+1;tindex<=t;tindex++)
								if (aschedule[tindex] == 1)	
									ntevents++;
									
							printf("ntevents:%d\n",ntevents);*/
							printf("attacks since insp:%d\n",attackssinceinsp);
					
							/*calculate DP-- need to use previous per instance DP, which 
							is stored in DPlog. Find cumulative sum of per instance DPs 
							for all instances that have occured since last inpsection*/
							
							/*if it's the first inspection*/
							if (inspindex[k] == 0)		{
								inspDP = 1-pow((1-DPlog[0]),attackssinceinsp);
								inspindex[k]++;
							}
							/*if it's not the first inspection*/
							else	{
								DPlog[inspindex[k]] = 1-((1+dstrat.SG[k].dep*
									(1-DPlog[inspindex[k]-1]))/(dstrat.SG[k].dep+1)); 
								printf("DP:%.3f\n",DPlog[inspindex[k]]);
								inspDP = 1-pow((1-DPlog[inspindex[k]]),attackssinceinsp);
								inspindex[k]++;
							}

							/*make dailyDP equal inspDP*/
							dailyDP[k][t] = inspDP;	
							printf("DP on day %d:%.3f\n",t+1,dailyDP[k][t]);
							attackssinceinsp = 0;
						}
						if (strcmp(dstrat.SG[k].name,"visinsp")==0)	{
							if (t <= tend)	{
								dailyDP[k][t] = VISINSPDPNC;
								inspindex[k]++;
							}
						}
						
				}
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

printf("cumDP for DRD: %.4f\n",cumdrdDP);

float cumSGDP[NUMSG];
for (k=0;k<NUMSG;k++)
	cumSGDP[k] = 0;
for (k=0;k<NUMSG;k++){
	for (t=0;t<m;t++)	
		cumSGDP[k] = 1-(1-cumSGDP[k])*(1-dailyDP[k][t]);
	printf("cumDP for safeguard %d: %.4f\n",k,cumSGDP[k]);
}

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
	
/*calculate total material obtained*/
float totalmat = 0;
/*if it's chopped SF or TRU sol, sum across totalTRU*/
if ((actastrat == 6)| (actastrat == 7))
	for (t=0;t<m;t++)
		totalmat = massTRU[t]+totalmat;
/*otherwise sum across totalU*/
else
	for (t=0;t<m;t++)
		totalmat = massU[t]+totalmat;
		
printf("\ncumDP: %.12f\n",cumDP);
printf("mattype: %.3f\n",mattype);	
printf("total mat:%.5f\n",totalmat);
float epsilon = 0.001;				/*in payoff-- 1-epsilon*DP in denominator-- epsilon
										prevents error when DP = 1*/
float payoff = 0;

if (PAYOFF == 1)
	payoff = cumDP/((1+epsilon-cumDP)*pow((mattype * totalmat),ALPHA));
else
	payoff = cumDP/pow((mattype * totalmat),ALPHA);

return payoff;

}