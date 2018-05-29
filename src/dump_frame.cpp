#include "opencv2/opencv.hpp"
#include <ctime>
#include <iostream>
#include <iomanip>
#include <fstream>

#define ERROR_CLI               111
#define ERROR_VIDEO_STREAM      222
#define ERROR_DUMP_FRAME        333
#define IMAGE_FOLDER            "frames"

int main(int argc, char** argv)
{
  std::string input;
  int frame_to_dump;
  if (argc > 2)
  {
    try
    {
      frame_to_dump = std::stoi(argv[1]);
    }
    catch(std::exception &e)
    {
      std::cerr << "Error in command line : " << e.what() << std::endl;
      exit(ERROR_CLI);
    }
    input = argv[2];
  }
  else{
    std::cerr << "Usage : " << argv[0] << " number_of_frames_to_dump path/to/video" << std::endl;
    exit(ERROR_CLI);
  }

  cv::VideoCapture vcap(input);
  if (!vcap.isOpened()) {
    std::cerr << "Error opening video stream or file : " << input << std::endl;
    exit(ERROR_VIDEO_STREAM);
  }

  cv::Mat frame;
  std::string frame_name;
  int frame_cnt = 0;
  for (int i = 0; i < frame_to_dump; ++i) {
    vcap >> frame;
    std::stringstream ss;
    ss << IMAGE_FOLDER << "/frame_" << std::setw(5) << std::setfill('0') << i << ".jpg";
    if (imwrite(ss.str(), frame))
      std::cout << "Frame " << frame_cnt << " (" << ss.str() << ") dumped successfully" << std::endl;
    else {
      std::cerr << "Unable to write frame" << std::endl;
      exit(ERROR_DUMP_FRAME);
    }
    ++frame_cnt;
  }
  vcap.release();

  return 0;
}
