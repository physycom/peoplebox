#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <support.h>
#include "image.h"

extern char** demo_names;
extern float demo_thresh;
extern float demo_hier;
extern float demo_classes;
extern int demo_total;
extern int demo_frame;
extern int demo_done;
extern double demo_time;
extern float fps;
extern int buff_index;

void init_yolo(const config cfg);
void* fetch(void* ptr);
void* detect(void* ptr);

int main()
{
  char *cfgname = "dark.cfg";
  config cfg = parse_config_file(cfgname);
  print_config(cfg);

  init_yolo(cfg);

  int i = 0;
  pthread_t detect_thread, fetch_thread;

  demo_time = what_time_is_it_now();
  while (!demo_done) {
    buff_index = (buff_index + 1) % 3;
    if (pthread_create(&fetch_thread, 0, fetch, (void*) &cfg))
    {
      MESSAGE_ERR("thread creation failed\n");
      exit(ERR_THREAD_CREATION);
    }
    if (pthread_create(&detect_thread, 0, detect, (void*) &cfg))
    {
      MESSAGE_ERR("thread creation failed\n");
      exit(ERR_THREAD_CREATION);
    }

    fps = 1. / (what_time_is_it_now() - demo_time);
    demo_time = what_time_is_it_now();

    pthread_join(fetch_thread, 0);
    pthread_join(detect_thread, 0);

    ++i;
  }

  // delete pointers
  for (i = 0; i < demo_classes; ++i)
    free(demo_names[i]);
  free(demo_names);

  return 0;
}