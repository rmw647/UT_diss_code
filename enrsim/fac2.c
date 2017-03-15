/*test out "factorial" and "choose" functions*/

#include<stdio.h>
 #include <limits.h>
 
double factorial(int);
double choose(int, int);
 
int main()
{

 
 printf("factorial 60: %e\n",factorial(60));
 printf("factorial of 4: %.0f\n",factorial(4));
 printf("60 choose 4: %e\n",choose(60,4));
 
 
  return 0;
}
 
double factorial(int n)
{
  if (n == 0)
    return 1;
  else
    return(n * factorial(n-1));
}

double choose(int a, int b)	{
	if (b == 0)
		return 1;
	else	
		return (factorial(a)/(factorial(b)*factorial(a-b)));
}