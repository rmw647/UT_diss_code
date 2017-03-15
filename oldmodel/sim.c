#include <stdio.h>
#include <math.h>
	
#define MU  75
#define SIGMA 2000
#define MASS 1000
#define PB 0.1

static double A[28][30];		
static double *pA = &A[0][30];


double *calcA();				/*function to populate payoff matrix-- returns pointer to A matrix*/

main()
{
	FILE *file; 
	int i, j;
	
	A = calcA();
	
	/*prints output to file*/
	file = fopen("file.txt","a+"); /* apend file (add text to 
	a file or create a file if it does not exist.*/ 

	for (j=1;j<=15;++j)		/*LOD headers- split 1-15, 16-30 for readability*/
		fprintf(file,"\t%d",j); 
	fprintf(file,"\n");
	
	for(i=0;i<=6;++i){
		fprintf(file,"low%d",i+1);	/*writes defense strategies headings low1-low7*/
		for(j=0;j<15;++j)			/*inputs value*/
			fprintf(file,"\t%0.4f ",A[i][j]);
		fprintf(file,"\n");
	}
	for(i=7;i<=13;++i){
		fprintf(file,"mod%d",i-6); /*writes defense strategies headings mod1-mod7*/
		for(j=0;j<15;++j)			/*inputs value*/
			fprintf(file,"\t%0.4f ",A[i][j]);
		fprintf(file,"\n");
	}
	for(i=14;i<=20;++i){
		fprintf(file,"high%d",i-13); 
		for(j=0;j<15;++j)			/*inputs value*/
			fprintf(file,"\t%0.4f ",A[i][j]);
		fprintf(file,"\n");
	}
	for(i=21;i<=27;++i){
		fprintf(file,"compl%d",i-20); 
		for(j=0;j<15;++j)			/*inputs value*/
			fprintf(file,"\t%0.4f ",A[i][j]);
		fprintf(file,"\n");
	}
	
	/*same code as above, prints bottom half of table, LOD 16-30*/
	
	fprintf(file,"\n");
	fprintf(file,"%s","+");
	for (j=16;j<=30;++j)
		fprintf(file,"\t%d",j); /*writes*/ 
	fprintf(file,"\n");
	
	for(i=0;i<=6;++i){
		fprintf(file,"low%d",i+1);	/*writes defense strategies headings low1-low7*/
		for(j=15;j<30;++j)			/*inputs value*/
			fprintf(file,"\t%0.4f ",A[i][j]);
		fprintf(file,"\n");
	}
	for(i=7;i<=13;++i){
		fprintf(file,"mod%d",i-6); /*writes defense strategies headings mod1-mod7*/
		for(j=15;j<30;++j)			/*inputs value*/
			fprintf(file,"\t%0.4f ",A[i][j]);
		fprintf(file,"\n");
	}
	for(i=14;i<=20;++i){
		fprintf(file,"high%d",i-13); 
		for(j=15;j<30;++j)			/*inputs value*/
			fprintf(file,"\t%0.4f ",A[i][j]);
		fprintf(file,"\n");
	}
	for(i=21;i<=27;++i){
		fprintf(file,"compl%d",i-20); 
		for(j=15;j<30;++j)			/*inputs value*/
			fprintf(file,"\t%0.4f ",A[i][j]);
		fprintf(file,"\n");
	}
	fclose(file); /*done!*/ 
	
	return 0;
}

double *calcA()
{
	int defensestrat[28], attackstrat[30];		/* defender and attacker strategies*/
	double dailydet[30],cumdet;					/* matrices that hold daily and scenario det probs*/
	int i,j;									/*counters*/
	int a, freq;								/*HRA parameters*/

	
	for (i=0; i<28; ++i)		/*name defense strategies 1-28 and populate array*/
		defensestrat[i] = i+1;
	
	for (i=0; i<30; ++i)		/*name attack strategies 1-30 (LOD) and populate array*/
		attackstrat[i] = i+1;
	
	
/* Calculating daily probability and storing in array*/
	int dscount;			/*indexes through defense strategies in loop*/
	int ascount;			/*indexes through attack strategies in loop*/
	/*first determine defender strategy parameters*/
	for (dscount = 0; dscount <= 27; ++dscount){
		/*determine freq, a for ds*/
		if (defensestrat[dscount]%7 == 1)
			freq = 1;
		else if (defensestrat[dscount]%7 == 2)
			freq = 2;
		else if(defensestrat[dscount]%7 == 3)
			freq = 3;
		else if(defensestrat[dscount]%7 == 4)
			freq = 4;
		else if(defensestrat[dscount]%7 == 5)
			freq = 5;
		else if(defensestrat[dscount]%7 == 6)
			freq = 6;
		else
			freq = 7;
		
		if (defensestrat[dscount] <= 7)
			a = 19;
		else if(defensestrat[dscount] <= 14)
			a = 6;
		else if(defensestrat[dscount] <= 21)
			a = 1;
		else
			a = 0;
	
		for (ascount = 0; ascount <= 29; ++ascount){	
			cumdet = 0;
			for (i=0; i<30; ++i)
				dailydet[i] = 0;
			dailydet[0] = 0.5*(1+erf(((MASS/attackstrat[ascount])-MU)/sqrt(SIGMA))); /*calculate Pdi*/
			int index = freq;
			while (index <= ascount){	
				dailydet[index] = 1-((1+a*(1-dailydet[index-freq]))/(a+1));		/*HRA calculation*/
				index = index + freq;
			}
			for (i=0;i<=ascount;++i)								/*combine daily prob with Pb*/
				dailydet[i] = 1 - (1-PB) * (1-dailydet[i]);
	
			for (i=0;i<=29;++i)								/*calculate cum det prob for strat pair*/
				cumdet = 1 - (1-dailydet[i])*(1-cumdet);
			A[dscount][ascount] = cumdet;
			return A;
		}	
	}
}

	