#include <stdio.h>

int main(void)	{
	
	/*general structure to store parameters for each safeguards*/
	struct safeguards {
		char name[25];
		int active;
		int cont;
		int dep;
		float fap;
		float fraction;
		int tcount;
	};
	
	/*create instance for each safeguard*/
	struct safeguards inventory = {"inventory",1,0,6,-1,-1,-1};
	struct safeguards mbalance = {"mbalance",1,0,-1,0.01,-1,-1};
	struct safeguards pseals = {"pseals",1,0,-1,-1,1,-1};
	struct safeguards nda = {"nda",0,0,-1,0.01,-1,-1};
	struct safeguards da = {"da",0,0,-1,-1,0.1,-1};
	struct safeguards videolog = {"videolog",1,0,-1,-1,-1,-1};
	struct safeguards videotrans = {"videotrans",0,1,6,-1,-1,-1};
	struct safeguards aseals = {"aseals",0,1,-1,-1,0.5,-1};
	struct safeguards cemo = {"cemo",0,1,-1,0.001,-1,300};
	
	/*dstrat is an array of "safeguards" that describe the defender strategy*/
	struct safeguards dstrat[9] = {inventory,mbalance,pseals,nda,da,videolog,videotrans,aseals,cemo};
	
	int k;
	for (k=0;k<9;k++)	
		if (dstrat[k].active == 1)
			printf("safeguard %s is active\n",dstrat[k].name);
	
	/*general structure to store parameters for each attacker options*/
	struct aoptions	{
		char name[100];
		int active;
		int cont;
		int tstart;
		int tend;
		int freq;
		int nitems;			/*number cylinders or seals or cascades-- # of items attacked*/
		int area;			/*0-feed storage,1-prod storage*/
		float deltam;
		float xp;
		float xf;
	};
	
	/*create instance for each attacker options*/
	struct aoptions cyltheft = {"cyltheft",1,0,16,16,1,1,1,-1,-1,-1};
	struct aoptions matcyl = {"matcyl",0,1,2,80,5,2,1,0.1,-1,-1};
	struct aoptions matcasc = {"matcasc",0,1,2,80,5,2,-1,-1,-1,-1};
	struct aoptions repiping = {"repiping",0,1,2,80,1,2,-1,-1,0.197,0.00711};
	struct aoptions recycle = {"recycle",0,1,2,80,1,-1,-1,-1,0.197,0.00711};
	struct aoptions udfeed = {"udfeed",0,1,2,80,5,-1,-1,-1,-1,-1};

	/*astrat is an array of "aoptions" that describe the attacker strategy*/
	struct aoptions astrat[6] = {cyltheft,matcyl,matcasc,repiping,recycle,udfeed};

	int j;
	for (j=0;j<9;j++)	
		if (astrat[j].active == 1)
			printf("attacker option %s is active\n",astrat[j].name);
	
	printf("so far, so good!\n");
	
}