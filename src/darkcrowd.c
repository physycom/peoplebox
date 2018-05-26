#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "image.h"

static char** demo_names;
static int demo_classes;

static network* net;
static image buff[3];
static image buff_letter[3];
static int buff_index = 0;
static CvCapture* cap;
static float thresh = .1;
static float hier_thresh = .1;

static int demo_frame = 3;
static int demo_index = 0;
static float** predictions;
static float* avg;
static int demo_done = 0;
static int demo_total = 0;
double demo_time;
double now_time;

/* physycom add */
#define MAX_LINE_LEN 11
#define ERR_NO_FILE 12
#define ERR_NO_INPUT_IMAGE 13
#define ERR_NO_INFO_JSON 14
#define ERR_WRONG_COMMANDLINE 15

#define NUMBER_OF_FILES_TO_ANALYZE 3
#define SW_VER_CROWD 102

#ifdef VERBOSE
#define MESSAGE(...) fprintf(stdout, __VA_ARGS__)
#define MESSAGE_ERR(...) fprintf(stderr, __VA_ARGS__)
#else
#define MESSAGE(...)
#define MESSAGE_ERR(...)
#endif

static int person_name_index;
static char person_label[] = "person";

typedef struct frame_info {
  unsigned int timestamp;
  unsigned int max_person_number;
  unsigned int min_person_number;
} frame_info;
static frame_info infos;

static double tnow;
static time_t tnow_t;
static struct tm* tm_tnow;
static char human_timenow[MAX_LINE_LEN];
static char json_name[200];
static char* jsonpath;
static char* loctag;

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

#define C0 3
int fit(const unsigned int x)
{
  return C0 * x;
}

int main(int argc, char** argv)
{
  if (argc != 5) {
    MESSAGE_ERR("Usage : %s path/to/input/img path/to/output/json location_tag [unixtime]\n", argv[0]);
    exit(ERR_WRONG_COMMANDLINE);
  }

  char** imgspath = malloc(NUMBER_OF_FILES_TO_ANALYZE * sizeof(char*));
  imgspath[0] = concat(argv[1], ".jpg");
  imgspath[1] = concat(argv[1], ".1.jpg");
  imgspath[2] = concat(argv[1], ".2.jpg");

  int* person_num = malloc(NUMBER_OF_FILES_TO_ANALYZE * sizeof(int));
  person_num[0] = 0;
  person_num[1] = 0;
  person_num[2] = 0;

  jsonpath = argv[2];
  loctag = argv[3];

  if (argc == 5) {
    tnow = atof(argv[4]);
    tnow_t = (time_t)tnow;
    tm_tnow = localtime(&tnow_t);
    strftime(human_timenow, MAX_LINE_LEN, "%D %X", tm_tnow);
  } else {
    tnow = what_time_is_it_now();
    tnow_t = (time_t)tnow;
    tm_tnow = localtime(&tnow_t);
    strftime(human_timenow, MAX_LINE_LEN, "%D %X", tm_tnow);
  }

  int i, j;
  char* cfg = "darknet/cfg/yolov3.cfg";
  char* weights = "darknet/yolov3.weights";
  char* name_list = "darknet/data/coco.names";

  demo_classes = 80;

  FILE* file = fopen(name_list, "r");
  if (!file) {
    MESSAGE_ERR("cannot open: %s", name_list);
    exit(ERR_NO_FILE);
  }
  demo_names = (char**)malloc(demo_classes * sizeof(char*));
  for (i = 0; i < demo_classes; ++i) {
    demo_names[i] = fgetl(file);
    if (!strcmp(demo_names[i], person_label))
      person_name_index = i;
  }
  fclose(file);

  net = load_network(cfg, weights, 0);
  set_batch_network(net, 1);

  for (i = 0; i < NUMBER_OF_FILES_TO_ANALYZE; ++i) {
    image im = load_image_color(imgspath[i], 0, 0);
    image sized = letterbox_image(im, net->w, net->h);

    layer l = net->layers[net->n - 1];
    float* X = sized.data;
    network_predict(net, X);
    int nboxes = 0;
    detection* dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh, 0, 1, &nboxes);
    do_nms_sort(dets, nboxes, l.classes, 0.45);

    /* people counting */
    for (j = 0; j < nboxes; ++j)
      if (dets[j].prob[person_name_index] > thresh)
        ++(person_num[i]);

#ifdef VERBOSE
    MESSAGE("Puny humans  : %d\n", person_num[i]);
#endif
  }
  infos.timestamp = (unsigned int)tnow;
  infos.max_person_number = max_of_three(person_num[0], person_num[1], person_num[2]);
  infos.min_person_number = min_of_three(person_num[0], person_num[1], person_num[2]);
  FILE* info_json = fopen(jsonpath, "w");
  if (info_json == NULL) {
    MESSAGE_ERR("Cannot create info json : %s\n", jsonpath);
    exit(ERR_NO_INFO_JSON);
  }
  fprintf(info_json, "{\n");
  fprintf(info_json, "\t\t\"timestamp\" : %u,\n", infos.timestamp);
  fprintf(info_json, "\t\t\"id_box\" : \"%s\",\n", loctag);
  fprintf(info_json, "\t\t\"detection\" : \"%s\",\n", "crowd");
  fprintf(info_json, "\t\t\"sw_ver\" : %d,\n", SW_VER_CROWD);
  fprintf(info_json, "\t\t\"people_count\" : [{\"id\" : \"%s\", \"count\" : %u}],\n", loctag, fit(infos.max_person_number));
  fprintf(info_json, "\t\t\"diagnostics\" : [{\"id\" : \"minimum_detected\", \"value\" : \"%u\"}]\n", infos.min_person_number);
  fprintf(info_json, "}");
  fclose(info_json);
  MESSAGE("Info dumped to JSON\n");

  /* delete pointers */
  for (i = 0; i < demo_classes; ++i)
    free(demo_names[i]);
  free(demo_names);

  return 0;
}
