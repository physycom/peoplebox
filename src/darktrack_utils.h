#include "common.h"
#include <darknet.h>

#define MAX_FRAME_INFO_TO_STORE    10                        // size of json output
#define MAX_LINE_LEN               20                        // static char buffer size
#define SAMPLING_DT_SEC            5                         // dt between single records
#define FPS                        3                         // source fps
#define SAMPLING_FRAME_RATE        (SAMPLING_DT_SEC * FPS)   // number of frames between records
#define NUMBER_OF_FILES_TO_ANALYZE 3

#define SW_VER_CROWD               104
#define SW_VER_TRACK               606


float fsqrt(const float );
float distance(const float , const float , const float , const float , const float , const float );
int t(const int , const double );

typedef struct frame_info {
  double timestamp;
  unsigned int timestamp_uint;
  int cnt_in, cnt_out;
  int flux_in, flux_out;
  int frame_number;
  unsigned int max_person_number;
  unsigned int min_person_number;
} frame_info;

char* concat(const char* , const char* );
int max_of_three(int , int , int );
int min_of_three(int , int , int );
