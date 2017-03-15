/*test calcMassFlow*/

#include <stdio.h>
#include <math.h>

#define SWU	465000

float calcMassFlow(float xf,float xp, float xw, float fraccasc);

int main(void)	{

printf("product: %.2f\n",calcMassFlow(0.00711,0.045,0.0022,0.5));

}

float calcMassFlow(float xf,float xp, float xw, float fraccasc)	{

float P;	/*product flow rate-- kg/day*/
float F;

float Vw,Vf,Vp;

Vw = (2*xw-1)*log(xw/(1-xw));
Vp = (2*xp-1)*log(xp/(1-xp));
Vf = (2*xf-1)*log(xf/(1-xf));

/*find daily SWU-- use 360 bc that's one "sim year"*/
float dailySWU = SWU/360;

/*find fraction of cascaded dedicated to diversion or fraccasc = 1 for nominal*/
float activeSWU = fraccasc*dailySWU;

/*daily mass flows*/
F = activeSWU*1/((xf-xp)/(xw-xp)*Vw + (1-(xf-xp)/(xw-xp))*Vp - Vf);
P = F*(1-(xf-xp)/(xw-xp));

return P;

}