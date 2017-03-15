#include <stdio.h>



int main(void)	{

int SG1,SG2;
int freq1,freq2;
int dep1,dep2;
int i,j,k,m;

for (SG1=0;SG1<=1;SG1++){
	printf("entered SG1 loop\n");
   if(SG1==0){
        freq1=-1;
        dep1=-1;
        goto HERE;
      }
   for (freq1=1;freq1<=2;freq1++)
      for (dep1=1;dep1<=2;dep1++)	{
      	printf("freq:%d, dep:%d\n",freq1,dep1);
         HERE:
         printf("I am here\n");
         for (SG2=0;SG2<=1;SG2++)	{
         	printf("entered SG2 loop-- SG2:%d\n",SG2);
            if(SG2==0){
               freq2=-1;
               dep2=-1;
               printf("SG1:%d freq:%d dep:%d  SG2:%d freq:%d dep:%d\n",
                     SG1,freq1,dep1,SG2,freq2,dep2);
                continue;
              }
             for (freq2=1;freq2<=2;freq2++)
                for (dep2=1;dep2<=2;dep2++)	
                  printf("SG1:%d freq:%d dep:%d  SG2:%d freq:%d dep:%d\n",
                     	SG1,freq1,dep1,SG2,freq2,dep2);
		 }
	}
}
}