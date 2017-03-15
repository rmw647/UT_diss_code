#include <stdio.h>
#include <string.h>

#define MAXDSTRATS	5000
#define MAXASTRATS	1200
#define NUMSG		9
#define	NUMAO		6
#define	NUMCASC		60		/*number of cascades at facility*/

/*general structure that stores parameters associated with each safeguard*/
struct safeguards {
	char name[100];	/*name of SG*/
	int active;		/*did the defender select?*/
	float fap;		/*false alarm probability*/
	float number;	/*fraction cylinders w/seal (pseal), #samples (DA), # seals/cascade (aseal)*/
	int tcount;		/*count time*/
	int schedfreq;	/*scheduled inspection frequency*/
	int scheddep;	/*scheduled inspection dependency*/
	int randfreq;	/*random inspection frequency*/
	int randdep;	/*random inspection dependency*/
} ;
	
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

int main(void)	{							
	
	
	/*****GENERATE DEFENDER OPTIONS*****/
	/*allowable values for defender parameters; game picks from these values*/
	int activeop[2] = {0,1};					/*0-active,1-not active-- defender and attacker*/
	float fapop[2] = {0.01,0.001};				/*1-0.01,2-0.001*/
	float fracsealedop[2] = {0.5,1};			/*fraction cylinders sealed*/
	float DAsamplesop[2] = {0.1,0.5};			/*fraction cylinders sampled with DA*/
	float cascadesealsop[2] = {1,5};			/* number seals per cascade*/	
	int tcountop[2] = {300,3600};				/*count time*/
	int schedfreqop[2] = {7,30};				/*scheduled inspection frequency*/
	int randfreqop[2] = {30,60};				/*random frequency options*/
	int depop[2] = {1,19};						/*dependency*/


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
	
	struct safeguards dstrat[NUMSG] = {inventory, mbalance, pseals, nda, da, videolog, 
									videotrans, aseals, cemo};
	
	/*array that holds each strategy*/
	struct safeguards stratspace[MAXDSTRATS][NUMSG]; 
	
	int i;						/*index*/
	int runningindex = 0;		/*continually updates with number of strategies*/
	int freqindex,depindex,fapindex,actindex;
	int tempmbal,tempinv,tempvl,tempvt = 0;			/*store index of SG of interest in strategy array*/
	int oindex,iindex;				/*outer index; inner index*/
	
	/*defender zero option*/
	for (i=0;i<NUMSG;i++)
		dstrat[i].active = activeop[0];
	for (i=0;i<NUMSG;i++)	{
		stratspace[runningindex][i] = dstrat[i];
	}
	runningindex = runningindex +1;
	
	/*grab SGs needed for strategy definition*/
	for (i=0;i<NUMSG;i++)
		if (strcmp(dstrat[i].name,"mbalance") == 0)
			tempmbal = i; 
	
	for (i=0;i<NUMSG;i++)
		if (strcmp(dstrat[i].name,"inventory") == 0)
			tempinv = i; 		
	
	/*defender strategies for regular inspections*/
	for (freqindex=0;freqindex<sizeof(schedfreqop)/sizeof(int);freqindex++)
		for (depindex=0;depindex<sizeof(depop)/sizeof(int);depindex++)
			for (fapindex=0;fapindex<sizeof(fapop)/sizeof(int);fapindex++)	{
				stratspace[runningindex][tempmbal].active = activeop[1];
				stratspace[runningindex][tempinv].active = activeop[1];
				for (i=0;i<NUMSG;i++)	{
					strcpy(stratspace[runningindex][i].name,stratspace[runningindex-1][i].name);
					if (stratspace[runningindex-1][i].fap != -1)
						stratspace[runningindex][i].fap = fapop[fapindex];
					else
						stratspace[runningindex][i].fap = -1;
					if (stratspace[runningindex-1][i].schedfreq != -1)
						stratspace[runningindex][i].schedfreq = schedfreqop[freqindex];
					else
						stratspace[runningindex][i].schedfreq = stratspace[runningindex-1][i].schedfreq;
					if (stratspace[runningindex-1][i].scheddep != -1)
						stratspace[runningindex][i].scheddep = depop[depindex];
					else
						stratspace[runningindex][i].scheddep = stratspace[runningindex-1][i].scheddep;
				}
				runningindex++;
			} 
			
	/*defender strategies involving video-- REWORK STRATEGY GENERATION*/
	for (i=0;i<NUMSG;i++)
		if (strcmp(dstrat[i].name,"videolog") == 0)
			tempvl = i; 
	for (i=0;i<NUMSG;i++)
		if (strcmp(dstrat[i].name,"videotrans") == 0)
			tempvt = i; 
	
	for (actindex=0;actindex<sizeof(activeop)/sizeof(int);actindex++)	
			for (freqindex=0;freqindex<sizeof(schedfreqop)/sizeof(int);freqindex++)
				for (depindex=0;depindex<sizeof(depop)/sizeof(int);depindex++)	{
						stratspace[runningindex][tempmbal].active = activeop[1];
						stratspace[runningindex][tempinv].active = activeop[1];
						stratspace[runningindex][tempvl].fap = -1;
						stratspace[runningindex][tempvl].active = activeop[actindex];
						for (i=0;i<NUMSG;i++)	{
							strcpy(stratspace[runningindex][i].name,stratspace[runningindex-1][i].name);
							if (stratspace[runningindex-1][i].schedfreq != -1)
								stratspace[runningindex][i].schedfreq = schedfreqop[freqindex];
							else
								stratspace[runningindex][i].schedfreq = stratspace[runningindex-1][i].schedfreq;
							if (stratspace[runningindex-1][i].scheddep != -1)
								stratspace[runningindex][i].scheddep = depop[depindex];
							else
								stratspace[runningindex][i].scheddep = stratspace[runningindex-1][i].scheddep;
						}
					runningindex++;
				}
	
			
	/*********PRINT TO CHECK**********/
	
	printf("running index: %d\n",runningindex);
	
	for (oindex=0;oindex<runningindex;oindex++)	
		for (iindex=0;iindex<6;iindex++)	{
			printf("row %-2d: %-10sactive:%-2d\tfap:%-.3f\tschedfreq: %-2d\tscheddep:%2d",
					oindex,stratspace[oindex][iindex].name, stratspace[oindex][iindex].active,
					stratspace[oindex][iindex].fap,stratspace[oindex][iindex].schedfreq,
					stratspace[oindex][iindex].scheddep);
			printf("\n");
		}
	
	/******GENERATE ATTACKER STRATEGIES*****/
	
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
	
	/*create instance for each attacker options*/
	struct aoptions cyltheft = {"cyltheft",activeop[0],cont[0],durop[0],-1,ncylop[0],areaop[1],
								-1,-1,-1};
	struct aoptions matcyl = {"matcyl",activeop[1],cont[1],durop[2],attackfreqop[0],ncylop[1],
								areaop[1],deltamop[0],-1,-1};
	struct aoptions matcasc = {"matcasc",activeop[0],cont[1],durop[2],attackfreqop[1],fcascop[1],
								-1,deltamop[2],-1,-1};
	struct aoptions repiping = {"repiping",activeop[0],cont[0],durop[0],-1,fcascop[1],-1,-1,
								xpop[1],xfop[0]};
	struct aoptions recycle = {"recycle",activeop[0],cont[1],durop[4],attackfreqop[1],fcascop[1],
								-1,-1,xpop[1],xfop[0]};
	struct aoptions udfeed = {"udfeed",activeop[0],cont[1],durop[4],-1,-1,-1,-1,-1,-1};	
	
	struct aoptions astrat[NUMAO] = {cyltheft,matcyl,matcasc,repiping,recycle,udfeed};
	
	/*array that holds each strategy*/
	struct aoptions astratspace[MAXASTRATS][NUMAO]; 
			
	int arunningindex=0;		/*keeps track of number of attacker strategies*/
	int ncylindex,areaindex;	/*index ncyl; index area*/
	int j;						/*generic index*/
	int tempcyltheft;			/*temp variable for cyl theft*/
	
	/*grab active attacker options*/
	for (j=0;j<NUMAO;j++)
		if (strcmp(astrat[j].name,"cyltheft")==0)
			tempcyltheft = j;
			
	/*zero attacker options*/
	for (j=0;j<NUMAO;j++)
		astrat[j].active = activeop[0];
	for (j=0;j<NUMAO;j++)	{
		astratspace[arunningindex][j] = astrat[j];
	}
	arunningindex = arunningindex +1;
	
	for (ncylindex=0;ncylindex<sizeof(ncylop)/sizeof(float);ncylindex++)
		for (areaindex=0;areaindex<sizeof(areaop)/sizeof(int);areaindex++)	{
			strcpy(astratspace[arunningindex][tempcyltheft].name,
					astratspace[arunningindex-1][tempcyltheft].name);
			astratspace[arunningindex][tempcyltheft].active = activeop[1];
			astratspace[arunningindex][tempcyltheft].nitems = ncylop[ncylindex];
			astratspace[arunningindex][tempcyltheft].area = areaop[areaindex];
			arunningindex++;
		}
	
	/*********PRINT TO CHECK**********/
	
	printf("\n\n");
	printf("arunning index: %d\n",arunningindex);
	
	for (oindex=0;oindex<arunningindex;oindex++)	
		for (iindex=0;iindex<1;iindex++)	{
			printf("column %-2d: %-10sactive:%-2d\tncyl: %-2f\tarea: %d",
					oindex,astratspace[oindex][iindex].name, astratspace[oindex][iindex].active,
					astratspace[oindex][iindex].nitems,astratspace[oindex][iindex].area);
			printf("\n");
		}
	
	printf("so far, so good!\n\n");
}