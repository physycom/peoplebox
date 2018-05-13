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
static float thresh = .5;
static float hier_thresh = .5;

static int demo_frame = 3;
static int demo_index = 0;
static float** predictions;
static float* avg;
static int demo_done = 0;
static int demo_total = 0;
double demo_time;
double now_time;

/* physycom add */
#define MAX_FRAME_INFO_TO_STORE 50
#define MAX_LINE_LEN 20
#define ERR_NO_FILE 111
#define ERR_NO_INPUT_IMAGE 222
#define ERR_NO_INFO_JSON 333
#define ERR_WRONG_COMMANDLINE 444

#define SW_VER_CROWD 100

#define PUNCTILIOUS

static int person_name_index;
static char person_label[] = "person";

typedef struct frame_info {
  double timestamp;
  unsigned int person_number;
} frame_info;
static frame_info infos;

static double tnow;
static time_t tnow_t;
static struct tm* tm_tnow;
static char human_timenow[MAX_LINE_LEN];
static char json_name[200];
static char* imgpath;
static char* jsonpath;
static char* loctag;

int main(int argc, char** argv)
{
  if (argc < 4) {
    fprintf(stderr, "Usage : %s path/to/input/img path/to/output/json location_tag\n", argv[0]);
    exit(ERR_WRONG_COMMANDLINE);
  }

  imgpath = argv[1];
  jsonpath = argv[2];
  loctag = argv[3];

  int i;
  char* cfg = "darknet/cfg/yolov3.cfg";
  char* weights = "darknet/yolov3.weights";
  char* name_list = "darknet/data/coco.names";

  thresh = .5;
  hier_thresh = .5;
  demo_classes = 80;

  FILE* file = fopen(name_list, "r");
  if (!file) {
    fprintf(stderr, "cannot open: %s", name_list);
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

  image im = load_image_color(imgpath, 0, 0);
  image sized = letterbox_image(im, net->w, net->h);

  layer l = net->layers[net->n - 1];
  float* X = sized.data;
  network_predict(net, X);
  int nboxes = 0;
  detection* dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh, 0, 1, &nboxes);
  do_nms_sort(dets, nboxes, l.classes, 0.45);

  /* people counting */
  int person_num = 0;
  for (i = 0; i < nboxes; ++i)
    if (dets[i].prob[person_name_index] > thresh)
      ++person_num;

  tnow = what_time_is_it_now();
  tnow_t = (time_t)tnow;
  tm_tnow = localtime(&tnow_t);
  strftime(human_timenow, MAX_LINE_LEN, "%D %X", tm_tnow);

#ifdef PUNCTILIOUS
  printf("UNIX  Time   : %.3f\n", tnow);
  printf("Human Time   : %s\n", human_timenow);
  printf("Puny humans  : %d\n", person_num);
#endif

  infos.timestamp = tnow;
  infos.person_number = person_num;
  FILE* info_json = fopen(jsonpath, "w");
  if (info_json == NULL) {
    fprintf(stderr, "Cannot create info json : %s\n", jsonpath);
    exit(ERR_NO_INFO_JSON);
  }
  fprintf(info_json, "{\n");
  fprintf(info_json, "\t\t\"timestamp\" : %.3f,\n", infos.timestamp);
  fprintf(info_json, "\t\t\"id_box\" : \"%s\",\n", loctag);
  fprintf(info_json, "\t\t\"detection\" : \"%s\",\n", "crowd");
  fprintf(info_json, "\t\t\"sw_ver\" : %d,\n", SW_VER_CROWD);
  fprintf(info_json, "\t\t\"people_count\" : [{\"id\" : \"%s\", \"count\" : %d}],\n", loctag, infos.person_number);
  fprintf(info_json, "\t\t\"diagnostics\" : [{\"id\" : \"coming\", \"value\" : \"soon\"}]\n");
  fprintf(info_json, "}");
  fclose(info_json);
  printf("Info dumped to JSON\n");

  /* delete pointers */
  for (i = 0; i < demo_classes; ++i)
    free(demo_names[i]);
  free(demo_names);

  return 0;
}
