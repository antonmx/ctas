float calc( int idx, int xx, int yy) {
  const int j = idx / xx;
  const int i = idx % xx;
  float ei = i / (float) xx;
  float ej = j / (float) yy;
  if (ei>0.5) ei = 1.0 - ei;
  if (ej>0.5) ej = 1.0 - ej;
  return ei*ei + ej*ej;  
}


kernel void applyAbsFilter (
  global float*            mid,
  int                      xx,
  int                      yy,
  float                    d2b )
{
  const int index = get_global_id(0);
  const float aconst = d2b * calc(index, xx, yy);
  const float filter = aconst / ( aconst + 1 );
  mid[2*index] *= filter;
  mid[2*index+1] *= filter;  
}


kernel void applyPhsFilter (
  global float*            mid,
  int                      xx,
  int                      yy,
  float                    d2b)
{
  const int index = get_global_id(0);
  const float filter = d2b / ( d2b * calc(index, xx, yy) + 1) ;
  mid[2*index] *= filter;
  mid[2*index+1] *= filter;
}


