/************************************************************************
* SOLVE A LINEAR SYSTEM AX=B BY THE SINGULAR VALUE DECOMPOSITION METHOD *
*                                                                       *
* This method is recommanded when the matrix A is ill-conditionned or   *
* near-singular, or there are fewer equations than unknowns.            *
* --------------------------------------------------------------------- *
* SAMPLE RUN:                                                           *
*  1. Solve normal linear system of size 5 x 5:                         *
*        Matrix A:          Vector B:                                   *
*    1  2  0  0      0         3                                        *
*    2  4  7  0      0        13                                        *
*    0  7 10  8      0        25                                        *
*    0  0  8 -0.75  -9        -1.75                                     *
*    0  0  0 -9     10         1                                        *
*                                                                       *
* Input file contains:                                                  *
* 5                                                                     *
* 5                                                                     *
*   1.000000   2.000000   0.000000   0.000000   0.000000                *
*   2.000000   4.000000   7.000000   0.000000   0.000000                *
*   0.000000   7.000000  10.000000   8.000000   0.000000                * 
*   0.000000   0.000000   8.000000  -0.750000  -9.000000                *
*   0.000000   0.000000   0.000000  -9.000000  10.000000                *
*   3.000000                                                            *
*  13.000000                                                            *
*  25.000000                                                            *
*  -1.750000                                                            *
*   1.000000                                                            *
*                                                                       *
* Output file contains:                                                 *
* M = 5                                                                 *
* N = 5                                                                 *
* Matrix A:                                                             *
*  1.000000  2.000000  0.000000  0.000000  0.000000                     *
*  2.000000  4.000000  7.000000  0.000000  0.000000                     *
*  0.000000  7.000000 10.000000  8.000000  0.000000                     *
*  0.000000  0.000000  8.000000 -0.750000 -9.000000                     *
*  0.000000  0.000000  0.000000 -9.000000 10.000000                     *
* Vector B:                                                             *
*  3.000000 13.000000 25.000000 -1.750000  1.000000                     *
* Matrix U:                                                             *
* -0.034422  0.690358 -0.717675  0.071746 -0.044894                     *
* -0.311812 -0.614665 -0.549190  0.413973  0.227984                     *
* -0.663543  0.222546  0.320343  0.484542 -0.415673                     *
* -0.483350  0.237931  0.181439 -0.208391  0.795873                     *
*  0.477150  0.198631  0.218615  0.738425  0.373911                     *
* Vector W:                                                             *
* 19.116947  2.530469  0.780714 12.539891  9.156592                     *
* Matrix V:                                                             *
* -0.034422 -0.690358 -0.717675  0.071746  0.044894                     *
* -0.311812  0.614665 -0.549190  0.413973 -0.227984                     *
* -0.663543 -0.222546  0.320343  0.484542  0.415673                     *
* -0.483350 -0.237931  0.181439 -0.208391 -0.795873                     *
*  0.477150 -0.198631  0.218615  0.738425 -0.373911                     *
* Solution:                                                             *
*  1.000000  1.000000  1.000000  1.000000  1.000000                     *
*                                                                       *
*  2. Solve linear system of size 4 x 5 (m < n):                        *
*        Matrix A:          Vector B:                                   *
*    1  2  0  0      0         3                                        *
*    2  4  7  0      0        13                                        *
*    0  7 10  8      0        25                                        *
*    0  0  8 -0.75  -9        -1.75                                     *
*                                                                       *
* Input file contains:                                                  *
* 4                                                                     *
* 5                                                                     *
*   1.000000   2.000000   0.000000   0.000000   0.000000                *
*   2.000000   4.000000   7.000000   0.000000   0.000000                *
*   0.000000   7.000000  10.000000   8.000000   0.000000                * 
*   0.000000   0.000000   8.000000  -0.750000  -9.000000                *
*   3.000000                                                            *
*  13.000000                                                            *
*  25.000000                                                            *
*  -1.750000                                                            *
*                                                                       *
* Output file contains:                                                 *
* M = 4                                                                 *
* N = 5                                                                 *
* Matrix A:                                                             *
*  1.000000  2.000000  0.000000  0.000000  0.000000                     *
*  2.000000  4.000000  7.000000  0.000000  0.000000                     *
*  0.000000  7.000000 10.000000  8.000000  0.000000                     *
*  0.000000  0.000000  8.000000 -0.750000 -9.000000                     *
* Vector B:                                                             *
*  3.000000 13.000000 25.000000 -1.750000                               *
* Matrix U:                                                             *
* -0.048526  0.080513 -0.252041  0.000000  0.963140                     *
* -0.420052  0.055591 -0.869634  0.000000 -0.253383                     *
* -0.769221  0.500339  0.396761 -0.000000  0.023246                     *
* -0.479062 -0.860284  0.150972 -0.000000  0.087286                     *
* Vector W:                                                             *
* 17.705504  9.945129  4.179107  0.000000  1.645325                     *
* Matrix V:                                                             *
* -0.050190  0.019275 -0.476492  0.832543  0.277376                     *
* -0.404496  0.390720 -0.288408 -0.416271  0.653651                     *
* -0.816982 -0.149796 -0.218240 -0.000000 -0.512322                     *
* -0.327269  0.467357  0.732420  0.364238  0.073239                     *
*  0.243515  0.778527 -0.325129 -0.030353 -0.477457                     *
* Solution:                                                             *
*  0.375463  1.312268  1.000000  0.726765  1.022770                     *
*                                                                       *
*  3. Solve linear system of size 5 x 5 (near-singular):                *
*        Matrix A:           Vector B:                                  *
*    1  2  0     0      0        3                                      *
*    2  4  1e-6  0      0        6.000001                               *
*    0  7 10     8      0       25                                      *
*    0  0  8    -0.75  -9       -1.75                                   *
*    0  0  0    -9     10        1                                      *
*                                                                       *
* Input file contains:                                                  *
* 5                                                                     *
* 5                                                                     *
*   1.000000   2.000000   0.000000   0.000000   0.000000                *
*   2.000000   4.000000   0.000001   0.000000   0.000000                *
*   0.000000   7.000000  10.000000   8.000000   0.000000                * 
*   0.000000   0.000000   8.000000  -0.750000  -9.000000                *
*   0.000000   0.000000   0.000000  -9.000000  10.000000                *
*   3.000000                                                            *
*   6.000001                                                            *
*  25.000000                                                            *
*  -1.750000                                                            *
*   1.000000                                                            *
*                                                                       *
* Output file contains:                                                 *
* M = 5                                                                 *
* N = 5                                                                 *
* Matrix A:                                                             *
*  1.000000  2.000000  0.000000  0.000000  0.000000                     *
*  2.000000  4.000000  0.000001  0.000000  0.000000                     *
*  0.000000  7.000000 10.000000  8.000000  0.000000                     *
*  0.000000  0.000000  8.000000 -0.750000 -9.000000                     *
*  0.000000  0.000000  0.000000 -9.000000 10.000000                     *
* Vector B:                                                             *
*  3.000000  6.000000 25.000000 -1.750000  1.000000                     *
* Matrix U:                                                             *
* -0.029197  0.894427 -0.435067  0.093070 -0.034671                     *
* -0.058394 -0.447214 -0.870134  0.186141 -0.069341                     *
* -0.649209  0.000000  0.207861  0.720418 -0.127747                     *
* -0.500300  0.000000 -0.091238 -0.280151  0.814181                     *
*  0.569179  0.000000  0.045304  0.599335  0.561053                     *
* Vector W:                                                             *
* 18.338461  0.000000  4.279165 11.548505  8.751235                     *
* Matrix V:                                                             *
* -0.007961 -0.859939 -0.508355  0.040295 -0.019809                     *
* -0.263732  0.429970 -0.676684  0.517265 -0.141801                     *
* -0.572267 -0.174666  0.315179  0.429750  0.598313                     *
* -0.542088 -0.157890  0.309309  0.050175 -0.763559                     *
*  0.555908 -0.142101  0.297764  0.737300 -0.196212                     *
* Solution:                                                             *
*  0.222075  1.388963  0.841992  0.857168  0.871451                     *
*                                                                       *
* --------------------------------------------------------------------- *
* Reference:  "Numerical Recipes By W.H. Press, B. P. Flannery,         *
*              S.A. Teukolsky and W.T. Vetterling, Cambridge            *
*              University Press, 1986" [BIBLI 08].                      *
*                                                                       *
*                               C++ Release 2.0 By J-P Moreau, Paris    *
* --------------------------------------------------------------------- *
* Release 2.0: Added dynamic allocations, I/O files and two more        *
*              examples.                                                *
*************************************************************************
Note: To link with basis_r.cpp and vmblock.cpp.
----------------------------------------------                         */
#include "basis.h"
#include "vmblock.h"

#define NMAX 100
         
int    i,j,m,n;
REAL   **A, **U, **V;
double *B, *W, *X;
double WMIN,WMAX;

void *vmblock = NULL;
FILE *fp;
char *filename="";



void SVBKSB(REAL **u, REAL *w, REAL **v, int m, int n, REAL *b, REAL *x) { 
/*------------------------------------------------------------------------------------------ 
! Solves A · X = B for a vector X, where A is specified by the arrays u, w, v as returned by 
! svdcmp. m and n are the dimensions of a, and will be equal for square matrices. b(1:m) is 
! the input right-hand side. x(1:n) is the output solution vector. No input quantities are 
! destroyed, so the routine may be called sequentially with different b’s. 
!-----------------------------------------------------------------------------------------*/
int i,j,jj; 
double s;
REAL tmp[NMAX];
  for (j=1; j<=n; j++) { //Calculate UTB
    s=0.0;
    if (w[j] != 0.0) {   //Nonzero result only if wj is nonzero
      for (i=1; i<=m; i++)  s += u[i][j]*b[i];
      s /= w[j];         //This is the divide by wj
    } 
    tmp[j]=s;
  } 
  for (j=1; j<=n; j++) { //Matrix multiply by V to get answer
    s=0.0;
    for (jj=1; jj<=n; jj++)  s += v[j][jj]*tmp[jj];
    x[j]=s;
  } 
} 

//Note that a typical use of svdcmp and svbksb superficially resembles the 
//typical use of ludcmp and lubksb: In both cases, you decompose the left-hand 
//matrix A just once, and then can use the decomposition either once or many times 
//with different right-hand sides. The crucial difference is the "editing" of the singular
//values before using SVDCMP.

int IMin(int a, int b) {
  if (a<=b) return a;
  else return b;
}

double Max(REAL a, REAL b) {
  if (a>=b) return a;
  else return b;
}

double pythag(REAL a, REAL b) {
//Computes sqrt(a*a + b*b) without destructive underflow or overflow
  REAL absa,absb;
  absa=fabs(a);
  absb=fabs(b);
  if (absa > absb) 
    return (absa*sqrt(1.0+(absb/absa)*(absb/absa)));
  else 
    if (absb == 0.0) 
      return 0.0;
    else
      return (absb*sqrt(1.0+(absa/absb)*(absa/absb)));
}

double Sign(REAL a, REAL b) {
  if (b <0.0) return (-fabs(a));
  else return fabs(a);
}

void SVDCMP(REAL **a, int m, int n, REAL *w, REAL **v) { 
/*------------------------------------------------------------------------------------- 
! Given a matrix a(1:m,1:n), this routine computes its singular value decomposition, 
! A = U · W · Vt. The matrix U replaces a on output. The diagonal matrix of singular
! values W is output as a vector w(1:n). The matrix V (not the transpose Vt) is output 
! as v(1:n,1:n). 
!------------------------------------------------------------------------------------*/
//Labels:  e1, e2, e3
  int i,its,j,jj,k,l,nm; 
  REAL anorm,c,f,g,h,s,scale,x,y,z;
  REAL rv1[NMAX];
     
  g=0.0;       //Householder reduction to bidiagonal form
  scale=0.0;
  anorm=0.0;
for (i=1; i<=n; i++) {
  l=i+1;
  rv1[i]=scale*g;
  g=0.0;
  s=0.0;
  scale=0.0;
  if (i <= m) {
    for (k=i; k<=m; k++)  scale += fabs(a[k][i]);
    if (scale != 0.0) {
	  for (k=i; k<=m; k++) {
        a[k][i] /= scale;
        s += a[k][i]*a[k][i];
      } 
      f=a[i][i];
      g=-Sign(sqrt(s),f);
      h=f*g-s;
      a[i][i]=f-g;
      for (j=l; j<=n; j++) {
        s=0.0;
        for (k=i; k<=m; k++) s += a[k][i]*a[k][j];
        f=s/h;
        for (k=i; k<=m; k++) a[k][j] += f*a[k][i];
      } 
      for (k=i; k<=m; k++) a[k][i] *= scale;
    } 
  } 
  w[i]=scale*g;
  g=0.0;
  s=0.0;
  scale=0.0;
  if (i <= m && i != n) {
    for (k=l; k<=n; k++)  scale += fabs(a[i][k]);
    if (scale != 0.0) {
	  for (k=l; k<=n; k++) {
        a[i][k] /= scale;
        s += a[i][k]*a[i][k];
      } 
      f=a[i][l];
      g=-Sign(sqrt(s),f);
      h=f*g-s;
      a[i][l]=f-g;
      for (k=l; k<=n; k++) rv1[k]=a[i][k]/h;
      for (j=l; j<=m; j++) {
        s=0.0;
        for (k=l; k<=n; k++) s += a[j][k]*a[i][k];
        for (k=l; k<=n; k++) a[j][k] += s*rv1[k];
      } 
      for (k=l; k<=n; k++) a[i][k] *= scale;
    } 
  } 
  anorm=Max(anorm,(fabs(w[i])+fabs(rv1[i])));
} // i loop
 
for (i=n; i>0; i--) {       //Accumulation of right-hand transformations
  if (i < n) {
    if (g != 0.0) {
      for (j=l; j<=n; j++)  //Double division to avoid possible underflow
        v[j][i]=(a[i][j]/a[i][l])/g;
      for (j=l; j<=n; j++) { 
        s=0.0;
        for (k=l; k<=n; k++) s += a[i][k]*v[k][j];
        for (k=l; k<=n; k++) v[k][j] += s*v[k][i];
      } 
    } 
    for (j=l; j<=n; j++) {
      v[i][j]=0.0;
      v[j][i]=0.0;
    } 
  } 
  v[i][i]=1.0;
  g=rv1[i];
  l=i;
} 

for (i=IMin(m,n); i>0; i--) {  //Accumulation of left-hand transformations
  l=i+1;
  g=w[i];
  for (j=l; j<=n; j++) a[i][j]=0.0;
  if (g != 0.0) {
    g=1.0/g;
    for (j=l; j<=n; j++) { 
      s=0.0;
      for (k=l; k<=m; k++) s += a[k][i]*a[k][j];
      f=(s/a[i][i])*g;
      for (k=i; k<=m; k++) a[k][j] += f*a[k][i];
    } 
    for (j=i; j<=m; j++) a[j][i] *= g;
  }   
  else
    for (j=i; j<=m; j++) a[j][i]=0.0;
  a[i][i] += 1.0;
} 

for (k=n; k>0; k--) {  //Diagonalization of the bidiagonal form: Loop over
                       //singular values, and over allowed iterations
for (its=1; its<=30; its++) {
for (l=k; l>0; l--) {  //Test for splitting
  nm=l-1;              //Note that rv1(1) is always zero
  if ((fabs(rv1[l])+anorm) == anorm) goto e2; 
  if ((fabs(w[nm])+anorm) == anorm)  goto e1; 
} 
e1: c=0.0;             //Cancellation of rv1(l), if l > 1
s=1.0;
for (i=l; i<=k; i++) {
  f=s*rv1[i];
  rv1[i]=c*rv1[i];
  if ((fabs(f)+anorm) == anorm) goto e2; 
  g=w[i];
  h=pythag(f,g);
  w[i]=h;
  h=1.0/h;
  c=g*h;
  s=-(f*h);
  for (j=1; j<=m; j++) {
    y=a[j][nm];
    z=a[j][i];
    a[j][nm]=(y*c)+(z*s);
    a[j][i]=-(y*s)+(z*c);
  } 
} 
e2: z=w[k];
if (l == k) {     //Convergence
  if (z < 0.0) {  //Singular value is made nonnegative
    w[k]=-z;
    for (j=1; j<=n; j++)  v[j][k]=-v[j][k];
  } 
  goto e3; 
} 
if (its == 30)  printf(" No convergence in svdcmp\n"); 
x=w[l];           //Shift from bottom 2-by-2 minor
nm=k-1;
y=w[nm];
g=rv1[nm];
h=rv1[k];
f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
g=pythag(f,1.0);
f=((x-z)*(x+z)+h*((y/(f+Sign(g,f)))-h))/x;
c=1.0;            //Next QR transformation:
s=1.0;
for (j=l; j<=nm; j++) {
  i=j+1;
  g=rv1[i];
  y=w[i];
  h=s*g;
  g=c*g;
  z=pythag(f,h);
  rv1[j]=z;
  c=f/z;
  s=h/z;
  f= (x*c)+(g*s);
  g=-(x*s)+(g*c);
  h=y*s;
  y=y*c;
  for (jj=1; jj<=n; jj++) {
    x=v[jj][j];
    z=v[jj][i];
    v[jj][j]= (x*c)+(z*s);
    v[jj][i]=-(x*s)+(z*c);
  } 
  z=pythag(f,h);
  w[j]=z;   //Rotation can be arbitrary if z = 0 
  if (z != 0.0) {
    z=1.0/z;
    c=f*z;
    s=h*z;
  } 
  f= (c*g)+(s*y);
  x=-(s*g)+(c*y);
  for (jj=1; jj<=m; jj++) {
    y=a[jj][j];
    z=a[jj][i];
    a[jj][j]= (y*c)+(z*s);
    a[jj][i]=-(y*s)+(z*c);
  } 
} //for j=l to nm
rv1[l]=0.0;
rv1[k]=f;
w[k]=x;

} //its loop
 
e3:;} //k loop
 
} //svdcmp()
 
void WriteMat(char *s,REAL **A,int m, int n) {
  int i,j;
  fprintf(fp,"%s\n", s);
  for (i=1; i<=m; i++) {
    for (j=1; j<n; j++) fprintf(fp,"%12.6f", A[i][j]);
    fprintf(fp,"%12.6f\n", A[i][n]);
  }
}

void WriteVec(char *s,REAL *V,int n) {
  int i;
  fprintf(fp,"%s\n", s);
  for (i=1; i<=n; i++) fprintf(fp,"%12.6f", V[i]);
  fprintf(fp,"\n");
}

void main()  {
  
  printf("\n Input data file name: "); scanf("%s", filename);
  fp = fopen(filename, "r");

  //read size of linear system
  fscanf(fp,"%d", &m);
  fscanf(fp,"%d", &n);

//allocate memory
  vmblock = vminit();
  A  = (REAL **) vmalloc(vmblock, MATRIX, m+1, n+1);
  U  = (REAL **) vmalloc(vmblock, MATRIX, m+1, n+1);
  V  = (REAL **) vmalloc(vmblock, MATRIX, n+1, n+1);
  B  = (REAL *)  vmalloc(vmblock, VEKTOR, m+1, 0);
  W  = (REAL *)  vmalloc(vmblock, VEKTOR, n+1, 0);
  X  = (REAL *)  vmalloc(vmblock, VEKTOR, n+1, 0);

// read matrix A
  for (i=1; i<=m; i++)
    for (j=1; j<=n; j++)
      fscanf(fp,"%lf", &A[i][j]);
// read vector B
  for (i=1; i<=m; i++)
    fscanf(fp,"%lf", &B[i]);
  fclose(fp);
  
  fp = fopen("tsvbksb.lst", "w");
  fprintf(fp,"\n  M = %d\n", m);
  fprintf(fp,"  N = %d\n", n);
  WriteMat("  Matrix A:",A,m,n);
  WriteVec("  Vector B:",B,m);
// Save A in U
  for (i=1; i<=m; i++)
    for (j=1; j<=n; j++)
      U[i][j]=A[i][j];
//call singular value decomposition subroutine
  SVDCMP(U,m,n,W,V);
  WriteMat("  Matrix U:",U,m,n);
  WriteVec("  Vector W:",W,n);
  WriteMat("  Matrix V:",V,m,n);
//seek highest value of W's and set near-zero
//values to exactly zero (for near-singular cases)
  WMAX=0.0;
  for (j=1; j<=n; j++)
    if (W[j] > WMAX)  WMAX=W[j];
  WMIN=WMAX*1e-6;
  for (j=1; j<=n; j++)
    if (W[j] < WMIN)  W[j]=0.0;
//call solver for SVD matrix
  SVBKSB(U,W,V,m,n,B,X);
//print solution
  WriteVec("  Solution:",X,n);
// free memory  
  vmfree(vmblock);
  fclose(fp);
  printf("\n Results in file tsvbksb.lst.\n\n");

}

//end of file tsvbksb.cpp
