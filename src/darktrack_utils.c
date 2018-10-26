#include "darktrack_utils.h"


float fsqrt(const float x)
{
  const float xhalf = 0.5f * x;
  union // get bits for floating value
  {
      float x;
      int i;
  } u;
  u.x = x;
  u.i = 0x5f3759df - (u.i >> 1);  // gives initial guess y0
  return x * u.x * (1.5f - xhalf * u.x * u.x);// Newton step, repeating increases accuracy
}

float distance(const float x1, const float x2, const float y1, const float y2, const float scale_x, const float scale_y)
{
  return fsqrt((x1 - x2)*(x1 - x2)*scale_x + (y1 - y2)*(y1 - y2)*scale_y);
}

int t(const int n, const double prob)
{
  int nt = n;
  int i;
  for(i=0; i<n; ++i)
  {
    double tmp = ((double)rand())/RAND_MAX;
    if( tmp < (prob - ((int)prob)) )
      ++nt;
    nt += ((int)prob);
  }
  return nt;
}


// image manipulation
image get_rotated_image_from_stream(CvCapture* cap)
{
  IplImage* src = cvQueryFrame(cap);
  if (!src)
    return make_empty_image(0, 0, 0);
  IplImage* trans = cvCreateImage(cvSize(src->height, src->width), src->depth, src->nChannels);
  cvTranspose(src, trans);  // transposition +
  cvFlip(trans, trans, 0);    // hor flip = 90 counterclockwise rotation
  image im = ipl_to_image(trans);
  rgbgr_image(im);
  return im;
}


int fill_rotated_image_from_stream(CvCapture* cap, image im)
{
  IplImage* src = cvQueryFrame(cap);
  if (!src)
    return 0;
  IplImage* trans = cvCreateImage(cvSize(src->height, src->width), src->depth, src->nChannels);
  cvTranspose(src, trans);  // transposition +
  cvFlip(trans, trans, 0);    // hor flip = 90 counterclockwise rotation
  ipl_into_image(trans, im);
  rgbgr_image(im);
  return 1;
}

char* concat(const char* s1, const char* s2)
{
  const size_t len1 = strlen(s1);
  const size_t len2 = strlen(s2);
  char* result = malloc(len1 + len2 + 1); //+1 for the null-terminator
  //in real code you would check for errors in malloc here
  memcpy(result, s1, len1);
  memcpy(result + len1, s2, len2 + 1); //+1 to copy the null-terminator
  return result;
}

int max_of_three(int i, int j, int k)
{
  int result = i;
  if (j > result)
    result = j;
  if (k > result)
    result = k;
  return result;
}

int min_of_three(int i, int j, int k)
{
  int result = i;
  if (j < result)
    result = j;
  if (k < result)
    result = k;
  return result;
}
