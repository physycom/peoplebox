// working parameters
#define MAX_FRAME_INFO_TO_STORE 1
#define FRAME_NUMBER_TRACKING   5
#define DOWNSAMPLING            3
#define MAX_LINE_LEN            20
#define JSON_FOLDER             "data"
#define SAMPLING_DT_SEC         5
#define FPS                     3
#define SAMPLING_FRAME_RATE     (SAMPLING_DT_SEC * FPS)

#ifdef VERBOSE
#define MESSAGE(...)     fprintf(stdout, __VA_ARGS__)
#define MESSAGE_ERR(...) fprintf(stderr, __VA_ARGS__)
#else
#define MESSAGE(...)
#define MESSAGE_ERR(...)
#endif

#define ERR_NO_FILE             111
#define ERR_NO_STREAM           112
#define ERR_NO_INFO_JSON        113
#define ERR_THREAD_CREATION     222

typedef struct frame_info {
  double timestamp;
  int cnt_in, cnt_out;
  int flux_in, flux_out;
  int frame_number;
} frame_info;
