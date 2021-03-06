__kernel void limit_array(__global float* arr,
                          const float minimum,
                          const float maximum)
{
  int l = get_global_id(0);
  const float element = arr[l];
  if ( arr[l] <= minimum )
    arr[l] = 0;
  else if ( arr[l] >= maximum )
    arr[l] = 1.0;
  else
    arr[l] = ( arr[l] - minimum ) / ( maximum - minimum ) ;
}
