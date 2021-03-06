__kernel void ff(__global float* fg,
                 __global float* bg1,
                 __global float* bg2,
                 __global float* df1,
                 __global float* df2,
                 const float bgw,
                 const float dfw,
                 const float cutOff)
{

  int l = get_global_id(0);

  float fgg = fg[l];

  float bg;
  if (bg1 && bg2)
    bg = bg1[l] * bgw + bg2[l] * (1-bgw);
  else if (bg1)
    bg = bg1[l];
  else if (bg2)
    bg = bg2[l];
  else
    bg = 1.0f;

  float df;
  if (df1 && df2)
    df = df1[l] * dfw + df2[l] * (1-dfw);
  else if (df1)
    df = df1[l];
  else if (df2)
    df = df2[l];
  else
    df = 0.0f;

  if ( df >= bg )
    fg[l] = 1.0f;
  else if (df >= fgg )
    fg[l] = 0.0f;
  else
    fg[l] = (fgg - df) / (bg-df);

  if ( cutOff>0.0f && fg[l] > cutOff )
    fg[l]=cutOff;

}
