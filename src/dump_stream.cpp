#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cerr << "Usage : " << argv[0] << " path/or/url/to/video" << std::endl;
    exit(1);
  }

  cv::VideoCapture vcap(argv[1]);
  if (!vcap.isOpened()) {
    std::cout << "Error opening video stream or file" << std::endl;
    exit(1);
  }
  int frame_width = vcap.get(CV_CAP_PROP_FRAME_WIDTH);
  int frame_height = vcap.get(CV_CAP_PROP_FRAME_HEIGHT);
  cv::VideoWriter video("out.avi", CV_FOURCC('M', 'J', 'P', 'G'), 10, cv::Size(frame_width, frame_height), true);

  for (;;) {
    cv::Mat frame;
    vcap >> frame;
    video.write(frame);
    cv::imshow("Frame", frame);
    char c = (char)cv::waitKey(33);
    if (c == 27)
      break;
  }

  return 0;
}
