#include <stdio.h> 
int main() 
{ 
	int i, j;
	FILE *file; 
	file = fopen("file.txt","a+"); /* apend file (add text to 
	a file or create a file if it does not exist.*/ 
	for (j=1;j<=15;++j)
		fprintf(file,"\t  %d",j); /*writes*/ 
	fprintf(file,"\n");
	for(i=1;i<=7;++i)
		fprintf(file,"low%d\n",i);
	for(i=1;i<7;++i)
		fprintf(file,"mod%d\n",i); /*writes*/ 
	for(i=1;i<7;++i)
		fprintf(file,"high%d\n",i); /*writes*/ 
	for(i=1;i<7;++i)
		fprintf(file,"compl%d\n",i); /*writes*/ 
	fprintf(file,"\n");
	for (j=16;j<=30;++j)
		fprintf(file,"\t  %d",j); /*writes*/ 
	fprintf(file,"\n");
	for(i=1;i<=7;++i)
		fprintf(file,"low%d\n",i);
	for(i=1;i<7;++i)
		fprintf(file,"mod%d\n",i); /*writes*/ 
	for(i=1;i<7;++i)
		fprintf(file,"high%d\n",i); /*writes*/ 
	for(i=1;i<7;++i)
		fprintf(file,"compl%d\n",i); /*writes*/ 
	fclose(file); /*done!*/ 
/*	getchar(); /* pause and wait for key */ 
	return 0; 
}