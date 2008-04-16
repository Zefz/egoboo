/**> HEADER FILES <**/
#include "mathstuff.h"

trig_table<float, 14> sin_tab(sin);            // Convert chrturn>>2...  to sine
trig_table<float, 14> cos_tab(cos);            // Convert chrturn>>2...  to cosine
arctan_table          atan_tab;

//--------------------------------------------------------------------------------------------
void MatrixDump(GLMatrix & a)
{
  int i; int j;

  for(j=0;j<4;j++)
  {
    printf("  ");
    for(i=0; i<4; i++) printf("%f ",a.CNV(i,j));
    printf("\n");
  }
}
