#include "opencv2/opencv.hpp"
#include <iostream>

#define CAM_FPS           3
#define DEFAULT_VIDEOLEN  10

int main(int argc, char** argv)
{
  std::string videoin;
  int videolen_sec = DEFAULT_VIDEOLEN;
  int cam_fps = CAM_FPS;
  if (argc == 4) {
    videoin = argv[1];
    try{
      videolen_sec = std::stoi(argv[2]);
      cam_fps = std::stoi(argv[3]);
    }
    catch(std::exception &e){
      std::cerr << "Erro in command line : " << e.what() << std::endl;
      exit(1);
    }
  }
  else if (argc == 3){
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
    std::cerr << "Usage : " << argv[0] << " path/or/url/to/video [stream_lenght_in_seconds] [cam_fps]" << std::endl;
    exit(1);
  }

  cv::VideoCapture vcap(videoin);
  if (!vcap.isOpened()) {
    std::cout << "Error opening video stream or file" << std::endl;
    exit(1);
  }
  int frame_width = vcap.get(CV_CAP_PROP_FRAME_WIDTH);
  int frame_height = vcap.get(CV_CAP_PROP_FRAME_HEIGHT);
  std::string videoname = "dump_" + std::to_string(std::time(0)) + ".mp4";
  cv::VideoWriter video(videoname, CV_FOURCC('X', '2', '6', '4'), 10, cv::Size(frame_width, frame_height), true);

  int maxframe = videolen_sec * cam_fps;
  for (int i = 0; i < maxframe; ++i) {
    cv::Mat frame;
    vcap >> frame;
    video.write(frame);
  }
  video.release();

  return 0;
}
