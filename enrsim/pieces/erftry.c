#include <stdio.h>
#include <math.h>

int main(void)	{

	int nomprod = 88;
	int ndacounttime = 300;
	float ndaerror = 0.04;
	float xprod = 0.045;
	float DP;
	
	float xp = 0.055;
	
	float thresh;	
	/*thresh- threshold value used in calc*/
	int stddev;	/*standard deviation*/
	int peakcounts;	/*total counts in 186-keV peak*/
							
							/*total counts = CR * count time*/
							peakcounts = nomprod * ndacounttime;
							stddev = ndaerror * peakcounts;
							
							float fapvalue = 1.645;		/*holds the value of the erf^-1(1-2*FAP)*/
							
							/*detection threshold*/
							thresh = peakcounts + sqrt(2)*stddev*fapvalue;
							printf("threshold: %.2f\n",thresh);
							
							DP = 1- 0.5*(1+erf((thresh-xp/xprod*peakcounts)/
											(sqrt(2)*xp/xprod*stddev)));
	printf("DP: %.3f\n",DP);					
						
}