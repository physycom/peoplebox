#include "image.h"
#include <config.h>
#include <support.h>

#define SW_VER_TRACK            1000

network* net;
image buff[3];
image buff_letter[3];
int buff_index = 0;
CvCapture* cap;
float** predictions;
float* avg;
float fps = 0;

int demo_classes = 80;
float demo_thresh = 0;
float demo_hier = .5;
int demo_frame = 3;
int demo_index = 0;
int demo_total = 0;
int demo_done = 0;
double demo_time;
char** demo_names;

int frame_num;
int infos_index;

int person_name_index;
char person_label[] = "person";

/******************** TRACKING *********************/
#define MAX_NUM_PERSON 50
static float old_coords[MAX_NUM_PERSON][2]; // swap variable for yolo
static int old_size = 0; // size filled
static int UP = 0; // counter ^
static int DOWN = 0; // counter _
static int LEFT = 0; // counter <
static int RIGHT = 0; // counter >
static float x, y, min_dist, dist;
static int mins;
inline float fsqrt(const float x)
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
inline float distance(const float x1, const float x2, const float y1, const float y2, const float scale_x, const float scale_y)
{
  return fsqrt((x1 - x2)*(x1 - x2)*scale_x + (y1 - y2)*(y1 - y2)*scale_y);
}

typedef struct frame_info {
  double timestamp;
  int cnt_in, cnt_out;
  int flux_in, flux_out;
  int frame_number;
} frame_info;
static frame_info* infos = NULL;

#define MAX_TIME_LEN 20
static char human_timenow[MAX_TIME_LEN];
static double tnow;
static time_t tnow_t;
static struct tm* tm_tnow;
static char json_name[200];

void init_yolo(const config cfg)
{
  char *yolocfg     = "darknet/cfg/yolov3.cfg";
  char *weightsfile = "darknet/yolov3.weights";
  char *namelist    = "darknet/data/coco.names";
  int i;

  demo_thresh = .5;
  demo_hier = .5;
  demo_classes = 80;

  FILE* file = fopen(namelist, "r");
  if (!file) {
    MESSAGE_ERR("cannot open namelist file : %s\n", namelist);
    exit(ERR_NO_NAMELIST_FILE);
  }
  demo_names = (char**)malloc(demo_classes * sizeof(char*));
  for (i = 0; i < demo_classes; ++i) {
    demo_names[i] = fgetl(file);
    if (!strcmp(demo_names[i], person_label))
      person_name_index = i;
  }
  fclose(file);

  net = load_network(yolocfg, weightsfile, 0);
  set_batch_network(net, 1);

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

  cap = cvCaptureFromFile(cfg.FILENAME);
  if (!cap) {
    MESSAGE_ERR("cannot connect to video stream.\n");
    exit(ERR_NO_STREAM);
  }
  else{
    MESSAGE("connected to video stream: %s\n", cfg.FILENAME);
  }

  buff[0] = get_image_from_stream(cap);
  buff[1] = copy_image(buff[0]);
  buff[2] = copy_image(buff[0]);
  buff_letter[0] = letterbox_image(buff[0], net->w, net->h);
  buff_letter[1] = letterbox_image(buff[0], net->w, net->h);
  buff_letter[2] = letterbox_image(buff[0], net->w, net->h);
}

void* fetch(void* ptr)
{
  int status = fill_image_from_stream(cap, buff[buff_index]);
  letterbox_image_into(buff[buff_index], net->w, net->h, buff_letter[buff_index]);
  if (status == 0) {
    MESSAGE_ERR("fill_image_from_stream() failed to load frame\n");
    demo_done = 1;
  }
  return 0;
}

void* detect(void* ptr)
{
  config *cfg = (config*) ptr;
  if ( infos == NULL ) infos = (frame_info*)calloc(cfg->MAX_FRAME_INFO_TO_STORE, sizeof(frame_info));

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
  strftime(human_timenow, MAX_TIME_LEN, "%D %X", tm_tnow);

  /* people tracking */
  if(frame_num)
  {
    for (i = 0; i < nboxes; ++i) // loop over new detections
    {
      if (dets[i].prob[person_name_index] > demo_thresh) // if person
      {
        ++person_num;
        x = dets[i].bbox.x * buff[0].w; // bbox coords in [0, 1)
        y = dets[i].bbox.y * buff[0].h; // bbox coords in [0, 1)

/************************ TRACKING ****************************/
        min_dist = HUGE_VAL;
        mins = -1;
        if (y > cfg->BARRIER_TOP    - cfg->TOLL  &&
            y < cfg->BARRIER_BOTTOM + cfg->TOLL  &&
            x > cfg->BARRIER_LEFT   - cfg->TOLL  &&
            x < cfg->BARRIER_RIGHT  + cfg->TOLL   ) // if in TRACKING ROI
        {
          for(j = 0; j < old_size; ++j) // loop over old person
          {
            dist     = distance(old_coords[j][0], x, old_coords[j][1], y, cfg->SCALE_X, cfg->SCALE_Y);
            if(dist > cfg->DIST_THRESH) continue;
            mins     = (min_dist < dist) ? mins : j;
            min_dist = (min_dist < dist) ? min_dist : dist;
          }
        } // end if in TRACKING ROI
/************************ TRACKING ****************************/
        if(mins == -1) continue; // no matches found
        UP    += (y < cfg->BARRIER_TOP     && old_coords[mins][1] > cfg->BARRIER_TOP)    ? 1 : 0;
        DOWN  += (y > cfg->BARRIER_BOTTOM  && old_coords[mins][1] < cfg->BARRIER_BOTTOM) ? 1 : 0;
        LEFT  += (x < cfg->BARRIER_LEFT    && old_coords[mins][0] > cfg->BARRIER_LEFT)   ? 1 : 0;
        RIGHT += (x > cfg->BARRIER_RIGHT   && old_coords[mins][0] < cfg->BARRIER_RIGHT)  ? 1 : 0;
/**********************************************************/
      }
    }
  }

  old_size = nboxes;
  for( i = 0; i < old_size; ++i)
    if (dets[i].prob[person_name_index] > demo_thresh) // if person
    {
      x = dets[i].bbox.x * buff[0].w; // bbox coords in [0, 1)
      y = dets[i].bbox.y * buff[0].h; // bbox coords in [0, 1)
      if (y > cfg->BARRIER_TOP    - cfg->TOLL  &&
          y < cfg->BARRIER_BOTTOM + cfg->TOLL  &&
          x > cfg->BARRIER_LEFT   - cfg->TOLL  &&
          x < cfg->BARRIER_RIGHT  + cfg->TOLL   ) // if in ROI
      {
        old_coords[i][0] = x;
        old_coords[i][1] = y;
      }
    }

  ++frame_num;

#ifndef _WIN32
  MESSAGE("\033[2J"); /* clear the screen */
  MESSAGE("\033[1;1H"); /* move cursor to line 1 column 1 */
#endif
  MESSAGE("UNIX  Time   : %.3f\n", tnow);
  MESSAGE("Human Time   : %s\n",   human_timenow);
  MESSAGE("Frame Number : %d\n",   frame_num);
  MESSAGE("Puny humans  : %d\n",   person_num);
  MESSAGE("FPS          : %.1f\n", fps);

//  infos[infos_index].cnt_in  += @BARRIER_IN@;
//  infos[infos_index].cnt_out += @BARRIER_OUT@;

  if(frame_num % (cfg->SAMPLING_DT_SEC * cfg->FPS) == 0){ // every sampling dt in frame units
    infos[infos_index].timestamp = tnow;
    infos[infos_index].frame_number = frame_num;
    if( frame_num > 1000*cfg->MAX_FRAME_INFO_TO_STORE ) frame_num = 0; // because of json key padding
    ++infos_index;
  }

  if (infos_index == cfg->MAX_FRAME_INFO_TO_STORE) {
    sprintf(json_name, "%s/%s_%ld.json", cfg->JSON_FOLDER, cfg->PEOPLEBOX_ID, tnow_t);
    FILE* info_json = fopen(json_name, "w");
    if (info_json == NULL) {
      MESSAGE_ERR("Cannot create info json\n");
      exit(ERR_NO_INFO_JSON);
    }
    fprintf(info_json, "{\n");
    for (i = 0; i < cfg->MAX_FRAME_INFO_TO_STORE; ++i) {
      fprintf(info_json, "\t\"frame_%05d\" : {\n", infos[i].frame_number);
      fprintf(info_json, "\t\t\"timestamp\" : %.3f,\n", infos[i].timestamp);
      fprintf(info_json, "\t\t\"id_box\" : \"%s\",\n", cfg->PEOPLEBOX_ID);
      fprintf(info_json, "\t\t\"detection\" : \"%s\",\n", cfg->DETECTION_TYPE_TRACK);
      fprintf(info_json, "\t\t\"sw_ver\" : %d,\n", SW_VER_TRACK);
      fprintf(info_json, "\t\t\"people_count\" : [{\"id\" : \"%s\", \"count\" : %d}, {\"id\" : \"%s\", \"count\" : %d}],\n", "IN", infos[i].cnt_in, "OUT", infos[i].cnt_out);
      fprintf(info_json, "\t\t\"people_flux\" : [{\"id\" : \"%s\", \"flux\" : %d}, {\"id\" : \"%s\", \"flux\" : %d}],\n", "IN", infos[i].flux_in, "OUT", infos[i].flux_out);
      fprintf(info_json, "\t\t\"diagnostics\" : [{\"id\" : \"coming\", \"value\" : \"soon\"}]\n");
      (i < cfg->MAX_FRAME_INFO_TO_STORE-1) ? fprintf(info_json, "\t},\n") : fprintf(info_json, "\t}\n");
    }
    fprintf(info_json, "}");
    fclose(info_json);
    infos_index = 0;
    memset(infos, 0, cfg->MAX_FRAME_INFO_TO_STORE * sizeof(frame_info));
    UP = 0;
    DOWN = 0;
    LEFT = 0;
    RIGHT = 0;
    MESSAGE("Info dumped to JSON\n");
  }

  free_detections(dets, nboxes);
  demo_index = (demo_index + 1) % demo_frame;
  return 0;
}
