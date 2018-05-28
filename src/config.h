#define ERR_NO_CFG_FILE        123
#define ERR_MALFORMED_CONFIG   124

#define MAX_LEN                100

typedef struct config{
  // tracking
  int BARRIER_TOP;
  int BARRIER_BOTTOM;
  int BARRIER_LEFT;
  int BARRIER_RIGHT;
  int TOLL;
  float DIST_THRESH;
  float SCALE_X;
  float SCALE_Y;
  int BARRIER_IN;
  int BARRIER_OUT;
  // box
  int DIRECTION;
  char PEOPLEBOX_ID[MAX_LEN];
  char CAMERA_IP[MAX_LEN];
  char CAMERA_CREDENTIALS[MAX_LEN];
  char DETECTION_TYPE_TRACK[MAX_LEN];
  char *FILENAME;
  // fit
  float C0;
  float C1;
  float C2;
  float C3;
  float C4;
  float C5;
  // json
  int MAX_FRAME_INFO_TO_STORE;
  int FRAME_NUMBER_TRACKING;
  char JSON_FOLDER[MAX_LEN];
  int SAMPLING_DT_SEC;
  int FPS;
} config;

enum{
  up,
  down,
  left,
  right
};

#ifdef __cplusplus
extern "C" {
#endif

config parse_config_file(const char *filename);
void print_config(const config cfg);

#ifdef __cplusplus
}
#endif
