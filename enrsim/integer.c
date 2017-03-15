/*use to test enrichment code*/

#include <stdio.h>
#include <math.h>

#define SWU	465000

float calcMassFlow(float xf,float xp, float xw,float fraccasc);

int main(void)	{

float xf = 0.045;
float xp = 0.05;
float xw = 0.0022;

float P;	/*product flow rate-- kg/day*/
float F;

float Vw,Vf,Vp;

Vw = (2*xw-1)*log(xw/(1-xw));
Vp = (2*xp-1)*log(xp/(1-xp));
Vf = (2*xf-1)*log(xf/(1-xf));

printf("Vw: %.2f\n",Vw);
printf("Vp: %.2f\n",Vp);
printf("Vf: %.2f\n",Vf);
F = SWU*1/((xf-xp)/(xw-xp)*Vw + (1-(xf-xp)/(xw-xp))*Vp - Vf);
P = F*(1-(xf-xp)/(xw-xp));


printf("feed [kg/year]: %.2f\n",F);
printf("product [kg/year]: %.2f\n",P);

float fraction = 1/60;

P = calcMassFlow(0.045,.05,xw,1);

printf("product [kg/day]: %.4f\n",P);
printf("product [kg/day]: %.4f\n",calcMassFlow(0.00711,.045,xw,0.016777));

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