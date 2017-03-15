#include <stdio.h>

int main(void)	{

int SG1,SG2;
int freq1,freq2;
int dep1,dep2;

for (SG1=0;SG1<=1;SG1++)
	for (SG2=0;SG2<=1;SG2++)	{
		if (SG1==0 && SG2==0)	{
			printf("SG1:%d SG2:%d\n",SG1,SG2);
			continue;
		}
		else if (SG1==0 && SG2==1)	{
			freq1=0;
			dep1=0;
			for (freq2=1;freq2<=2;freq2++)
				for (dep2=1;dep2<=2;dep2++)
					printf("SG1:%d freq:%d dep:%d  SG2:%d freq:%d dep:%d\n",
                     			SG1,freq1,dep1,SG2,freq2,dep2);
		}
		else if (SG1==1 && SG2==0)	{
			freq2=0;
			dep2=0;
			for (freq1=1;freq1<=2;freq1++)
				for (dep1=1;dep1<=2;dep1++)
					printf("SG1:%d freq:%d dep:%d  SG2:%d freq:%d dep:%d\n",
                     			SG1,freq1,dep1,SG2,freq2,dep2);
		}
		else if (SG1==1 && SG2==1)	{
			for (freq1=1;freq1<=2;freq1++)
				for (dep1=1;dep1<=2;dep1++)
					for (freq2=1;freq2<=2;freq2++)
						for (dep2=1;dep2<=2;dep2++)
							printf("SG1:%d freq:%d dep:%d  SG2:%d freq:%d dep:%d\n",
                     			SG1,freq1,dep1,SG2,freq2,dep2);
		}
    }
}
