#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>

#define CAM_FPS           3
#define DEFAULT_VIDEOLEN  10

std::string time_now_hr() {
  time_t tnow = std::time(nullptr);
  struct tm tstruct = *localtime(&tnow);
  char buf[100];
  strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", &tstruct);
  return buf;
}

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
  int frame_width = vcap.get(cv::CAP_PROP_FRAME_WIDTH);
  int frame_height = vcap.get(cv::CAP_PROP_FRAME_HEIGHT);
  std::string videoname = "video_" + time_now_hr() + ".mp4";

  cv::VideoWriter video(videoname, cv::VideoWriter::fourcc('X', '2', '6', '4'), cam_fps, cv::Size(frame_width, frame_height), true);

  int maxframe = videolen_sec * cam_fps;
  for (int i = 0; i < maxframe; ++i) {
    cv::Mat frame;
    vcap >> frame;
    video.write(frame);
  }
  video.release();

  return 0;
}
