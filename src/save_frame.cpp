#include "opencv2/opencv.hpp"
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <thread>

#include <support.h>
#include <config.h>

#define SW_VER 1000

int main(int argc, char** argv)
{
  if( argc < 2 )
  {
    std::cerr << "Usage : " << argv[0] << " path/to/config" << std::endl;
    exit(ERR_WRONG_CLI);
  }

  config cfg = parse_config_file(argv[1]);
  print_config(cfg);


  std::ofstream swver("save_frame.sw_ver");
  swver << std::time(0) << std::endl << SW_VER << std::endl;
  swver.close();

  cv::VideoCapture vcap(cfg.FILENAME);
  if (!vcap.isOpened()) {
    std::cerr << "Error opening video stream or file : " << cfg.FILENAME << std::endl;
    exit(ERR_NO_STREAM);
  }

  cv::Mat frame;
  std::string frame_name;
  int frame_cnt = 0;
  for (;;) {
    if( frame_cnt % (cfg.SAMPLING_DT_SEC * cfg.CAM_FPS) == 0 ){
      vcap >> frame;
      frame_name = std::string(cfg.IMAGE_FOLDER) + "/" + cfg.PEOPLEBOX_ID + "_" + std::to_string(std::time(0));
      if (imwrite(frame_name + ".jpg", frame))
        std::cout << "Frame " << frame_cnt << " (" << frame_name << ") dumped successfully" << std::endl;
      else {
        std::cerr << "Unable to write frame" << std::endl;
        exit(ERR_WRITING_SAVED_FRAME);
      }

      vcap >> frame;
      if (imwrite(frame_name + ".1.jpg", frame))
        std::cout << "Frame " << frame_cnt << ".1 (" << frame_name << ") dumped successfully" << std::endl;
      else {
        std::cerr << "Unable to write frame" << std::endl;
        exit(ERR_WRITING_SAVED_FRAME);
      }

      vcap >> frame;
      if (imwrite(frame_name + ".2.jpg", frame))
        std::cout << "Frame " << frame_cnt << ".2 (" << frame_name << ") dumped successfully" << std::endl;
      else {
        std::cerr << "Unable to write frame" << std::endl;
        exit(ERR_WRITING_SAVED_FRAME);
      }
    }
    else
      vcap.grab();
    ++frame_cnt;
  }
  vcap.release();

  return 0;
}
