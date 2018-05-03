//g++ dump_movie.cpp -O3 -std=c++11 `pkg-config opencv --cflags --libs`
#include <iostream>
#include "opencv2/opencv.hpp"

int main()
{
  cv::VideoCapture vcap("/mnt/c/Users/NICO/Desktop/toy_story.mp4");
  if(!vcap.isOpened())
  {
    std::cout << "Error opening video stream or file" << std::endl;
    return -1;
  }
  int frame_width   = vcap.get(CV_CAP_PROP_FRAME_WIDTH);
  int frame_height  = vcap.get(CV_CAP_PROP_FRAME_HEIGHT);
  cv::VideoWriter video("out.avi", CV_FOURCC('M','J','P','G'), 10, cv::Size(frame_width,frame_height), true);
  for(;;)
  {
    cv::Mat frame;
    vcap >> frame;
    video.write(frame);
    cv::imshow( "Frame", frame );
    char c = (char)cv::waitKey(33);
    if( c == 27 ) break;
  }
  return 0;
}
