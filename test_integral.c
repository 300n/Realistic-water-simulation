#include <stdio.h>
#include <math.h>

double F(double x)
{
  return exp(x);
}

double simpson(int N, double A, double B)
{
  double X, h, Iapp0, Iapp1, Iapp2, Iapp;
  int NN, i;

  // Etape 1
  h = (B - A) / N;

  // Etape 2
  Iapp0 = F(A) + F(B);
  Iapp1 = 0.0;
  Iapp2 = 0.0;

  // Etape 3
  NN = N -1;
  for (i=1; i<=NN; i++)
    {
      // Etape 4
      X = A + i*h;
      // Etape 5
      if ((i%2) == 0)
        Iapp2 = Iapp2 + F(X);
      else
        Iapp1 = Iapp1 + F(X);
    }

  // Etape 6
  Iapp = (Iapp0 + 2.0 * Iapp2 + 4.0 * Iapp1) * h / 3.0;

  // Etape 7
  return (Iapp);

}
int main(int argc, char *argv[])
{
  double i,n;
  double a,b,spmthd;

  a=1.0; /* Borne inferieure */
  b=2.0; /* Borne superieure */
  n=1000; /* Nombre d'iteration  */

  simpson(n,a,b);
  spmthd=simpson(n,a,b);  /* on stock dans une variable tampon */
  printf("\n %.52e\n",exp(2)-exp(1)-spmthd); /* on affiche le résultat */
  return 0;
}
