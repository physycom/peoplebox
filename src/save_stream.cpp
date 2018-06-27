#include "opencv2/opencv.hpp"
#include <iostream>

#define CAM_FPS  3

int main(int argc, char** argv)
{
  std::string videoin;
  int videolen_sec = 10;
  if (argc == 3) {
    videoin = argv[1];
    try{
      videolen_sec = std::stoi(argv[2]);
    }
    catch(std::exception &e){
      std::cerr << "Erro in command line : " << e.what() << std::endl;
      exit(1);
    }
  }
  else if (argc == 2){
    videoin = argv[1];
  }
  else {
    std::cerr << "Usage : " << argv[0] << " path/or/url/to/video [stream_lenght_in_seconds]" << std::endl;
    exit(1);
  }

  cv::VideoCapture vcap(argv[1]);
  if (!vcap.isOpened()) {
    std::cout << "Error opening video stream or file" << std::endl;
    exit(1);
  }
  int frame_width = vcap.get(CV_CAP_PROP_FRAME_WIDTH);
  int frame_height = vcap.get(CV_CAP_PROP_FRAME_HEIGHT);
  std::string videoname = "dump_" + std::to_string(std::time(0)) + ".avi";
  cv::VideoWriter video(videoname, CV_FOURCC('M', 'J', 'P', 'G'), 10, cv::Size(frame_width, frame_height), true);

  int maxframe = videolen_sec * CAM_FPS;
  for (int i = 0; i < maxframe; ++i) {
    cv::Mat frame;
    vcap >> frame;
    video.write(frame);
  }

  return 0;
}
