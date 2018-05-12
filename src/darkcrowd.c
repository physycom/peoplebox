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
static float fps = 0;
static float demo_thresh = 0;
static float demo_hier = .5;

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
#define MAX_LINE_LEN            20
#define ERR_NO_FILE             111
#define ERR_NO_INPUT_IMAGE      222
#define ERR_NO_INFO_JSON        333
#define ERR_WRONG_COMMANDLINE   444

#define SW_VER_CROWD            100

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
static char *imgpath;
static char *jsonpath;
static char *loctag;

/* thread functions */
void* fetch(void* ptr)
{
  int status = fill_image_from_stream(cap, buff[buff_index]);
  letterbox_image_into(buff[buff_index], net->w, net->h, buff_letter[buff_index]);
  if (status == 0)
  {
    fprintf(stderr, "fill_image_from_stream() failed to load frame\n");
    demo_done = 1;
  }
  return 0;
}

void* detect(void* ptr)
{
  float nms = .4;
  int i, j, count = 0, nboxes = 0, person_num = 0;

  layer l = net->layers[net->n - 1];
  network_predict(net, buff_letter[(buff_index + 2) % 3].data);

  for (i = 0; i < net->n; ++i) {
    layer l = net->layers[i];
    if (l.type == YOLO || l.type == REGION || l.type == DETECTION) {
      memcpy(predictions[demo_index] + count, net->layers[i].output, sizeof(float) * l.outputs);
      count += l.outputs;
    }
  }

  count = 0;
  fill_cpu(demo_total, 0, avg, 1);
  for (j = 0; j < demo_frame; ++j)
    axpy_cpu(demo_total, 1. / demo_frame, predictions[j], 1, avg, 1);
  for (i = 0; i < net->n; ++i) {
    layer l = net->layers[i];
    if (l.type == YOLO || l.type == REGION || l.type == DETECTION) {
      memcpy(l.output, avg + count, sizeof(float) * l.outputs);
      count += l.outputs;
    }
  }
  detection* dets = get_network_boxes(net, buff[0].w, buff[0].h, demo_thresh, demo_hier, 0, 1, &nboxes);

  if (nms > 0)
    do_nms_obj(dets, nboxes, l.classes, nms);

  tnow = what_time_is_it_now();
  tnow_t = (time_t)tnow;
  tm_tnow = localtime(&tnow_t);
  strftime(human_timenow, MAX_LINE_LEN, "%D %X", tm_tnow);

  /* people counting */
  for (i = 0; i < nboxes; ++i)
    if (dets[i].prob[person_name_index] > demo_thresh)
      ++person_num;

#ifdef PUNCTILIOUS
  printf("UNIX  Time   : %.3f\n", tnow);
  printf("Human Time   : %s\n", human_timenow);
  printf("Puny humans  : %d\n", person_num);
  printf("FPS          : %.1f\n", fps);
#endif

  infos.timestamp = tnow;
  infos.person_number = person_num;
  FILE* info_json = fopen(jsonpath, "w");
  if(info_json == NULL){
    fprintf(stderr, "Cannot create info json : %s\n", jsonpath);
    exit(ERR_NO_INFO_JSON);
  }
  fprintf(info_json, "{\n");
  fprintf(info_json, "\t\t\"timestamp\" : %.3f,\n", infos.timestamp);
  fprintf(info_json, "\t\t\"id_box\" : \"%s\",\n", loctag);
  fprintf(info_json, "\t\t\"detection\" : \"%s\",\n", "crowd");
  fprintf(info_json, "\t\t\"sw_ver\" : %d,\n", SW_VER_CROWD);
  fprintf(info_json, "\t\t\"people_count\" : [{\"id\" : \"%s\", \"count\" : %d}],\n", "nome_coming_soon", infos.person_number);
  fprintf(info_json, "\t\t\"diagnostics\" : [{\"id\" : \"coming\", \"value\" : \"soon\"}]\n");
  fprintf(info_json, "}");
  fclose(info_json);
  printf("Info dumped to JSON\n");

  free_detections(dets, nboxes);
  demo_index = (demo_index + 1) % demo_frame;
  return 0;
}

int main(int argc, char **argv)
{
  if(argc < 4){
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

  demo_thresh = .5;
  demo_hier = .5;
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
  pthread_t detect_thread;
  pthread_t fetch_thread;

  demo_total = 0;
  for (i = 0; i < net->n; ++i) {
    layer l = net->layers[i];
    if (l.type == YOLO || l.type == REGION || l.type == DETECTION) {
      demo_total += l.outputs;
    }
  }

  predictions = (float**)calloc(demo_frame, sizeof(float*));
  for (i = 0; i < demo_frame; ++i)
    predictions[i] = (float*)calloc(demo_total, sizeof(float));

  avg = (float*)calloc(demo_total, sizeof(float));

  fprintf(stdout, "input image: %s\n", imgpath);
  cap = cvCaptureFromFile(imgpath);

  if (!cap) {
    fprintf(stderr, "cannot open img.\n");
    exit(ERR_NO_INPUT_IMAGE);
  }

  buff[0] = get_image_from_stream(cap);
  buff[1] = copy_image(buff[0]);
  buff[2] = copy_image(buff[0]);
  buff_letter[0] = letterbox_image(buff[0], net->w, net->h);
  buff_letter[1] = letterbox_image(buff[0], net->w, net->h);
  buff_letter[2] = letterbox_image(buff[0], net->w, net->h);

  i = 0;
  demo_time = what_time_is_it_now();

  fetch(NULL);
  detect(NULL);

  /* delete pointers */
  for (i = 0; i < demo_classes; ++i)
    free(demo_names[i]);
  free(demo_names);

  return 0;
}
