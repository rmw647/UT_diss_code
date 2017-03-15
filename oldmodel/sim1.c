#include <stdio.h>
#include <math.h>

#define PB 0.1
#define MAXDAYS 30

double A[28][30];	
static double mass = 1000;
static double mu = 75;
static 	double sigma = 2000;

double calcpayoff(int i, int j);			/*function to populate payoff matrix-- returns pointer to A matrix*/
void print2file(int);

main()
{	
	int i, j;
	
	for (i=1;i<=28;i++){
		for (j=1;j<=30;j++)
			A[i-1][j-1] = calcpayoff(i,j);
	}
	print2file(15);
	return 0;
}

double calcpayoff(int dstrat, int astrat)
{
	double dailydet[30],cumdet;					/* matrices that hold daily and scenario det probs*/
	int i,j;									/*counters*/
	int freq, a;
	
	if (dstrat%7 == 1)
			freq = 1;
		else if (dstrat%7 == 2)
			freq = 2;
		else if(dstrat%7 == 3)
			freq = 3;
		else if(dstrat%7 == 4)
			freq = 4;
		else if(dstrat%7 == 5)
			freq = 5;
		else if(dstrat%7 == 6)
			freq = 6;
		else
			freq = 7;
		
		if (dstrat <= 7)
			a = 19;
		else if(dstrat <= 14)
			a = 6;
		else if(dstrat <= 21)
			a = 1;
		else
			a = 0;
	
	for (i=0; i<MAXDAYS; ++i)						/*loops over 30 days in scenario (max) */
		dailydet[i] = 0;
			
	dailydet[0] = 0.5*(1+erf(((mass/astrat)-mu)/sqrt(sigma))); /*calculate Pdi*/
	
	int index = freq;
	while (index < astrat){	
		dailydet[index] = 1-((1+a*(1-dailydet[index-freq]))/(a+1));		/*HRA calculation*/
		index = index + freq;
	}
	for (i=0;i< astrat;++i)								/*combine daily prob with Pb*/
		dailydet[i] = 1 - (1-PB) * (1-dailydet[i]);
		
	cumdet = 0;
	
	for (i=0;i<=29;++i)								/*calculate cum det prob for strat pair*/
		cumdet = 1 - (1-dailydet[i])*(1-cumdet);
			
	return cumdet;
}	

void print2file(int colwidth){	
	
	FILE *file;
	int i,j;
	/*prints output to file*/
	file = fopen("file.txt","a+"); /* apend file (add text to a file or create a file if it does not exist.*/ 

	for (j=1;j<=colwidth;++j)		/*LOD headers- split 1-15, 16-30 for readability*/
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
	for (j=colwidth+1;j<=30;++j)
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
}


	