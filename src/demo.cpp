#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <cmath>
#include <ctime>

#include <jsoncons/json.hpp>

#include <yolo_v2_class.hpp>

#include <p_detector.hpp>
#include <tracking.hpp>
#include <barrier.hpp>

#ifdef OPENCV

std::vector<bbox_t> get_3d_coordinates(std::vector<bbox_t> bbox_vect, cv::Mat xyzrgba) {
  return bbox_vect;
}

#include <opencv2/opencv.hpp>            // C++
#include <opencv2/core/version.hpp>
#ifndef CV_VERSION_EPOCH
#include <opencv2/videoio/videoio.hpp>
#define OPENCV_VERSION CVAUX_STR(CV_VERSION_MAJOR)"" CVAUX_STR(CV_VERSION_MINOR)"" CVAUX_STR(CV_VERSION_REVISION)
#ifndef USE_CMAKE_LIBS
#pragma comment(lib, "opencv_world" OPENCV_VERSION ".lib")
#endif    // USE_CMAKE_LIBS
#else
#define OPENCV_VERSION CVAUX_STR(CV_VERSION_EPOCH)"" CVAUX_STR(CV_VERSION_MAJOR)"" CVAUX_STR(CV_VERSION_MINOR)
#ifndef USE_CMAKE_LIBS
#pragma comment(lib, "opencv_core" OPENCV_VERSION ".lib")
#pragma comment(lib, "opencv_imgproc" OPENCV_VERSION ".lib")
#pragma comment(lib, "opencv_highgui" OPENCV_VERSION ".lib")
#endif    // USE_CMAKE_LIBS
#endif    // CV_VERSION_EPOCH

#include <draw_debug_frame.hpp>

#endif    // OPENCV

std::vector<std::string> objects_names_from_file(std::string const filename) {
  std::ifstream file(filename);
  std::vector<std::string> file_lines;
  if (!file.is_open()) return file_lines;
  for (std::string line; getline(file, line);) file_lines.push_back(line);
  std::cout << "object names loaded \n";
  return file_lines;
}

/////////////////////////////////// MAIN ///////////////////////////////////
constexpr int SW_VER = 101;

struct detection_data_t {
  cv::Mat cap_frame;
  std::shared_ptr<image_t> det_image;
  std::vector<bbox_t> result_vec;
  cv::Mat draw_frame;
  bool new_detection;
  uint64_t frame_id;
  bool exit_flag;
  detection_data_t() : exit_flag(false), new_detection(false) {}
};

template<typename T>
class send_one_replaceable_object_t {
  const bool sync;
  std::atomic<T *> a_ptr;
public:

  void send(T const& _obj) {
    T *new_ptr = new T;
    *new_ptr = _obj;
    if (sync) {
      while (a_ptr.load()) std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    std::unique_ptr<T> old_ptr(a_ptr.exchange(new_ptr));
  }

  T receive() {
    std::unique_ptr<T> ptr;
    do {
      while (!a_ptr.load()) std::this_thread::sleep_for(std::chrono::milliseconds(3));
      ptr.reset(a_ptr.exchange(NULL));
    } while (!ptr);
    T obj = *ptr;
    return obj;
  }

  bool is_object_present() {
    return (a_ptr.load() != NULL);
  }

  send_one_replaceable_object_t(bool _sync) : sync(_sync), a_ptr(NULL)
  {}
};

void cross_barriers(std::vector<fat_barrier> &barriers, tracker_t &tracker, const std::string &id_box, float &det_time, float &cap_time, const int &dump_rec_num, const std::string &data_outdir, const int &crossing_frame_num, jsoncons::json &crossing, const detection_data_t &detection_data)
{
  for (auto &b : barriers)
    for (const auto &t : tracker.tracks)
      b.crossing(t);

  jsoncons::json jrecord;
  auto tnow = std::time(0);
  jrecord["timestamp"] = tnow;
  jrecord["id_box"] = id_box;
  jrecord["detection"] = "counting";
  jrecord["sw_ver"] = SW_VER;

  // collect multi-barrier results
  jsoncons::json pplc = jsoncons::json::array();
  jsoncons::json timec = jsoncons::json::array();
  jsoncons::json j, tc;
  for (auto &b : barriers)
  {
    j["id"] = b.name + "-in";
    j["count"] = b.cnt_in;
    pplc.push_back(j);
    j["id"] = b.name + "-out";
    j["count"] = b.cnt_out;
    pplc.push_back(j);

    b.reset();
  }
  tc["id"] = "det_mean_time";
  tc["value"] = det_time / crossing_frame_num;
  timec.push_back(tc);
  tc["id"] = "cap_mean_time";
  tc["value"] = cap_time / crossing_frame_num;
  timec.push_back(tc);
  det_time = 0.f;
  cap_time = 0.f;

  jrecord["people_count"] = pplc;
  jrecord["diagnostic"] = timec;

  std::stringstream ss;
  ss << "frame_" << std::setw(6) << std::setfill('0') << detection_data.frame_id;
  crossing[ss.str()] = jrecord;

  if ( crossing.size() >= dump_rec_num )
  {
    std::ofstream pplc_out(data_outdir + "/" + id_box + "_" + std::to_string(tnow) + ".json");
    if( !pplc_out ) throw std::runtime_error("Unable to create dump file.");
    pplc_out << jsoncons::pretty_print(crossing) << std::endl;
    pplc_out.close();
    crossing.clear();
  }

  // clean up tracks
  tracker.reset();
}

void usage(char * progname)
{
  std::string pname(progname);
  std::cerr << "Usage: " << pname.substr(pname.find_last_of("/\\") + 1) << " path/to/json/config" << std::endl;
  std::cerr << R"(Sample config JSON file
{
  "filename": "/path/to/video.avi",
  "network": {
    "file_names"   : "darknet/data/coco.names",
    "file_cfg"     : "darknet/cfg/yolov3.cfg",
    "file_weights" : "yolov3.weights",
    "thresh"       : 0.5
  },
  "enable_barrier" : true,
  "barriers" : {
    "test_barrier" : {
      "pnt_start" : [100, 100],
      "pnt_end"   : [900, 900],
      "thickness" : 10
    }
  },
  "id_box"              : "MYBOX",
  "crossing_dt"         : 3,
  "dump_dt"             : 60,
  "data_outdir"         : "data",
  "image_dt"            : 300,
  "image_outdir"        : "image",
  "enable_opencv_show"  : true,
  "enable_detection_cs" : true,
  "enable_video_dump"   : true,
  "enable_image_dump"   : true
}
)";
}


int main(int argc, char *argv[])
{
  std::string conf;
  if (argc == 2)
  {
    conf = argv[1];
  }
  else
  {
    usage(argv[0]);
    exit(1);
  }

  // network vars
  std::string names_file, cfg_file, weights_file, filename;
  float thresh;
  std::vector<std::string> obj_types;

  // box vars
  std::string id_box;

  // barrier crossing vars
  std::vector<fat_barrier> barriers;
  std::vector<double> dist_param;
  int crossing_dt, crossing_frame_num, dump_dt, dump_rec_num, max_dist_px;
  std::string data_outdir;
  jsoncons::json crossing;
  bool enable_barrier = false;

  // realtime image vars
  std::string image_outdir;
  int image_dt, image_frame_num;

  // debug vars
  std::ofstream out_detections;
  std::string out_detfile;
  cv::VideoWriter out_video;
  std::string out_videofile;
  std::string out_imagebase;
  bool enable_opencv_show = false;
  bool enable_detection_csv = false;
  bool enable_video_dump = false;
  bool enable_image_dump = false;

  try
  {
    jsoncons::json jconf = jsoncons::json::parse_file(conf);

    // network parameters
    if (!jconf.has_member("network")) throw std::runtime_error("No member 'network' in json config file.");
    names_file =   (jconf["network"].has_member("file_names"))   ? jconf["network"]["file_names"].as<std::string>()   : "data/coco.names";
    cfg_file =     (jconf["network"].has_member("file_cfg"))     ? jconf["network"]["file_cfg"].as<std::string>()     : "cfg/yolov3.cfg";
    weights_file = (jconf["network"].has_member("file_weights")) ? jconf["network"]["file_weights"].as<std::string>() : "yolov3.weights";
    thresh =       (jconf["network"].has_member("thresh"))       ? jconf["network"]["thresh"].as<float>()             : 0.2f;

    // object types to detect
    if (jconf.has_member("object_types"))
      obj_types = jconf["object_types"].as<std::vector<std::string>>();

    // distance parameters
    dist_param = (jconf.has_member("dist_param")) ? jconf["dist_param"].as<std::vector<double>>() : std::vector<double>({1., 1.});
    max_dist_px = (jconf.has_member("max_dist_px")) ? jconf["max_dist_px"].as<int>() : 120;

    // barrier parameters
    if (jconf.has_member("barriers"))
    {
      for (const auto &b : jconf["barriers"].object_range())
      {
        barriers.emplace_back( std::string(b.key()),
          b.value()["pnt_start"][0].as<int>(), b.value()["pnt_start"][1].as<int>(),
          b.value()["pnt_end"][0].as<int>(), b.value()["pnt_end"][1].as<int>(),
          b.value()["thickness"].as<int>() );
      }
    }
    enable_barrier = (jconf.has_member("enable_barrier")) ? jconf["enable_barrier"].as<bool>() : false;

    // box and time parameters
    filename     = (jconf.has_member("filename"))     ? jconf["filename"].as<std::string>()     : "";
    data_outdir  = (jconf.has_member("data_outdir"))  ? jconf["data_outdir"].as<std::string>()  : "data";
    image_outdir = (jconf.has_member("image_outdir")) ? jconf["image_outdir"].as<std::string>() : "image";
    id_box       = (jconf.has_member("id_box"))       ? jconf["id_box"].as<std::string>()       : "nameless_box";
    crossing_dt  = (jconf.has_member("crossing_dt"))  ? jconf["crossing_dt"].as<int>()          : 1;
    dump_dt      = (jconf.has_member("dump_dt"))      ? jconf["dump_dt"].as<int>()              : 5;
    if ( crossing_dt > dump_dt ) dump_dt = crossing_dt;
    image_dt     = (jconf.has_member("image_dt"))     ? jconf["image_dt"].as<int>()             : 300;


    // debug parameters
    out_videofile = filename + ".tracking.avi";
    out_detfile = filename + ".det.csv";
    out_imagebase = filename;
    enable_opencv_show   = (jconf.has_member("enable_opencv_show"))   ? jconf["enable_opencv_show"].as<bool>()   : false;
    enable_detection_csv = (jconf.has_member("enable_detection_csv")) ? jconf["enable_detection_csv"].as<bool>() : false;
    enable_video_dump    = (jconf.has_member("enable_video_dump"))    ? jconf["enable_video_dump"].as<bool>()    : false;
    enable_image_dump    = (jconf.has_member("enable_image_dump"))    ? jconf["enable_image_dump"].as<bool>()    : false;
  }
  catch (std::exception &e)
  {
    std::cerr << "EXC: " << e.what() << std::endl;
    exit(2);
  }

  // detection vars
  p_detector detector(cfg_file, weights_file);
  auto obj_names = objects_names_from_file(names_file);

  // extract indices of desired detection object types
  std::vector<int> det_types;
  for(int i = 0; i < int(obj_names.size()); ++i)
    if ( std::any_of(obj_types.begin(), obj_types.end(), [i, obj_names](std::string s){ return obj_names[i] == s; }) )
      det_types.emplace_back(i);

  // GUARDASELO PER BENISSIMO ***********
  bool detection_sync = true;                // true - for video-file

  // tracking vars
  int frame_story = 10;
  tracker_t tracker(frame_story, max_dist_px, dist_param);

  try
  {
    preview_boxes_t large_preview(100, 150, false), small_preview(50, 50, true);
    bool show_small_boxes = false;

    std::string const file_ext = filename.substr(filename.find_last_of(".") + 1);
    std::string const protocol = filename.substr(0, 7);
    if (file_ext == "avi" || file_ext == "mp4" || file_ext == "mjpg" || file_ext == "mov" ||     // video file
      protocol == "rtmp://" || protocol == "rtsp://" || protocol == "http://" || protocol == "https:/"     // video network stream
      )
    {
      if (protocol == "rtsp://" || protocol == "http://" || protocol == "https:/" )
      {
        detection_sync = true;
        out_videofile = data_outdir + "/" + "video_stream.tracking.avi";
        out_detfile = data_outdir + "/" + "video_stream.det.csv";
        out_imagebase = data_outdir + "/" + "image_stream.";
      }

      cv::Mat cur_frame;
      std::atomic<int> fps_cap_counter(0), fps_det_counter(0);
      std::atomic<int> current_fps_cap(0), current_fps_det(0);
      std::atomic<bool> exit_flag(false);
      float det_time = 0.f, cap_time = 0.f;
      std::chrono::steady_clock::time_point steady_start, steady_end, start_cap, start_det;
      int video_fps = 25;

      track_kalman_t track_kalman;

      cv::VideoCapture cap;
      cap.open(filename);
      if( !cap.isOpened() ) {
#ifdef WIN32
        std::cerr << "Check that you have copied file opencv_ffmpeg3xx_64.dll to the same directory where is demo.exe" << std::endl;
#endif
        throw std::runtime_error("Unable to open input video: " + filename);
      }
      video_fps = cap.get(CV_CAP_PROP_FPS);
      crossing_frame_num = video_fps * crossing_dt;
      dump_rec_num =  int(dump_dt / float(crossing_dt));
      image_frame_num = video_fps * image_dt;
      cap >> cur_frame;
      cv::Size const frame_size = cur_frame.size();
      std::cout << "Video input: " << filename << "\nsize: " << frame_size << "  FPS: " << video_fps << std::endl;

      if (enable_video_dump)
        out_video.open(out_videofile, CV_FOURCC('M', 'J', 'P', 'G'), std::max(35, video_fps), frame_size, true);
        //out_video.open(out_videofile, CV_FOURCC('D', 'I', 'V', 'X'), std::max(35, video_fps), frame_size, true);

      const bool sync = detection_sync; // sync data exchange
      send_one_replaceable_object_t<detection_data_t> cap2prepare(sync), cap2draw(sync),
        prepare2detect(sync), detect2draw(sync), draw2show(sync), draw2write(sync), draw2net(sync);

      std::thread t_cap, t_prepare, t_detect, t_post, t_draw, t_write;

      // capture new video-frame
      if (t_cap.joinable()) t_cap.join();
      t_cap = std::thread([&]()
      {
        uint64_t frame_id = 0;
        detection_data_t detection_data;
        do {
          detection_data = detection_data_t();
          start_cap = std::chrono::steady_clock::now();
          cap >> detection_data.cap_frame;
          fps_cap_counter++;
          detection_data.frame_id = frame_id++;
          if (detection_data.cap_frame.empty() || exit_flag) {
            std::cout << " exit_flag: detection_data.cap_frame.size = " << detection_data.cap_frame.size() << std::endl;
            detection_data.exit_flag = true;
            detection_data.cap_frame = cv::Mat(frame_size, CV_8UC3);
          }
          cap_time += std::chrono::duration<float>(std::chrono::steady_clock::now() - start_cap).count();

          if (!detection_sync) {
            cap2draw.send(detection_data);       // skip detection
          }
          cap2prepare.send(detection_data);
        } while (!detection_data.exit_flag);
        std::cout << " t_cap exit" << std::endl;
      });


      // pre-processing video frame (resize, conversion)
      t_prepare = std::thread([&]()
      {
        std::shared_ptr<image_t> det_image;
        detection_data_t detection_data;
        do {
          detection_data = cap2prepare.receive();
          det_image = detector.mat_to_image_resize(detection_data.cap_frame);
          detection_data.det_image = det_image;
          prepare2detect.send(detection_data);    // detection

          // dump realtime image
          if ( detection_data.frame_id % image_frame_num == 0 )
          {
            cv::Mat barrier_frame = detection_data.cap_frame;
            draw_barrier(barrier_frame, barriers);
            cv::imwrite(image_outdir + "/" + id_box + "_" + std::to_string(std::time(0)) + ".jpg", barrier_frame);
          }

        } while (!detection_data.exit_flag);
        std::cout << " t_prepare exit" << std::endl;
      });


      // detection by Yolo
      if (t_detect.joinable()) t_detect.join();
      t_detect = std::thread([&]()
      {
        std::shared_ptr<image_t> det_image;
        detection_data_t detection_data;

        if ( enable_detection_csv ) out_detections.open(out_detfile);

        do {
          detection_data = prepare2detect.receive();
          start_det = std::chrono::steady_clock::now();
          det_image = detection_data.det_image;
          std::vector<bbox_t> result_vec;

          if (det_image)
          {
            if (det_types.size())
              result_vec = detector.detect_resized(*det_image, frame_size.width, frame_size.height, det_types, thresh, true);
            else
              result_vec = detector.detect_resized(*det_image, frame_size.width, frame_size.height, thresh, true);
          }
          fps_det_counter++;
          //std::this_thread::sleep_for(std::chrono::milliseconds(150));

          detection_data.new_detection = true;
          detection_data.result_vec = result_vec;
          det_time += std::chrono::duration<float>(std::chrono::steady_clock::now() - start_det).count();

          detect2draw.send(detection_data);
        } while (!detection_data.exit_flag);
        out_detections.close();
        std::cout << " t_detect exit" << std::endl;
      });

      // draw and track
      t_draw = std::thread([&]()
      {
        detection_data_t detection_data;
        do {

          // for Video-file
          if (detection_sync) {
            detection_data = detect2draw.receive();
          }
          // for Video-camera
          else
          {
            // get new Detection result if present
            if (detect2draw.is_object_present()) {
              cv::Mat old_cap_frame = detection_data.cap_frame;   // use old captured frame
              detection_data = detect2draw.receive();
              if (!old_cap_frame.empty()) detection_data.cap_frame = old_cap_frame;
            }
            // get new Captured frame
            else {
              std::vector<bbox_t> old_result_vec = detection_data.result_vec; // use old detections
              detection_data = cap2draw.receive();
              detection_data.result_vec = old_result_vec;
            }
          }

          cv::Mat cap_frame = detection_data.cap_frame;
          cv::Mat draw_frame = detection_data.cap_frame.clone();
          std::vector<bbox_t> result_vec = detection_data.result_vec;

          // tracking
          tracker.update(result_vec);
          //std::cout << "Frame " << detection_data.frame_id << "  Tracks " << tracker.tracks.size() << "  Det " << result_vec.size() << std::endl;
          //for(const auto &t : tracker.tracks ) std::cout << t.track_id << " " << t.time_since_update << " " << t.state << std::endl;
          //std::cout << "----------------------------------------------------" << std::endl << std::endl;

          // barrier crossing
          if ( enable_barrier && (detection_data.frame_id % crossing_frame_num == 0) )
            cross_barriers(barriers, tracker, id_box, det_time, cap_time, dump_rec_num, data_outdir, crossing_frame_num, crossing, detection_data);

          // decorate img
          draw_boxes(draw_frame, result_vec, obj_names, detection_data.frame_id, current_fps_det, current_fps_cap);
          draw_track(draw_frame, tracker);
          draw_barrier(draw_frame, barriers);

          detection_data.result_vec = result_vec;
          detection_data.draw_frame = draw_frame;
          draw2show.send(detection_data);
          if (out_video.isOpened()) draw2write.send(detection_data);
        } while (!detection_data.exit_flag);
        std::cout << " t_draw exit" << std::endl;
      });


      // write frame to videofile
      t_write = std::thread([&]()
      {
        if (out_video.isOpened()) {
          detection_data_t detection_data;
          cv::Mat output_frame;
          do {
            detection_data = draw2write.receive();
            if (detection_data.draw_frame.channels() == 4) cv::cvtColor(detection_data.draw_frame, output_frame, CV_RGBA2RGB);
            else output_frame = detection_data.draw_frame;
            out_video << output_frame;
          } while (!detection_data.exit_flag);
          out_video.release();
        }
        std::cout << " t_write exit" << std::endl;
      });

      // show detection
      detection_data_t detection_data;
      do {
        steady_end = std::chrono::steady_clock::now();
        float time_sec = std::chrono::duration<double>(steady_end - steady_start).count();
        if (time_sec >= 1)
        {
          current_fps_det = fps_det_counter.load() / time_sec;
          current_fps_cap = fps_cap_counter.load() / time_sec;
          steady_start = steady_end;
          fps_det_counter = 0;
          fps_cap_counter = 0;
        }

        detection_data = draw2show.receive();
        cv::Mat draw_frame = detection_data.draw_frame;

        if ( enable_detection_csv )
        {
          for (const auto &b : detection_data.result_vec)
            if (obj_names[b.obj_id] == "person")
              out_detections << detection_data.frame_id << ";"
              << b.x << ";" << b.y << ";"
              << b.w << ";" << b.h << std::endl;
        }

        if ( enable_image_dump )
        {
          std::stringstream ss;
          ss << std::setw(6) << std::setfill('0') << detection_data.frame_id;
          cv::imwrite(out_imagebase + "." + ss.str() + ".jpg", draw_frame);
        }

        if ( enable_opencv_show )
        {
          cv::Mat small_frame;
          cv::resize(draw_frame, small_frame, cv::Size(), 0.5, 0.5);
          cv::imshow("Tracking", small_frame);

          int key = cv::waitKey(3);    // 3 or 16ms
          if (key == 'f') show_small_boxes = !show_small_boxes;
          if (key == 'p') while (true) if (cv::waitKey(100) == 'p') break;
          if (key == 27) exit_flag = true;
        }

      } while (!detection_data.exit_flag);
      std::cout << " show detection exit" << std::endl;

      if ( enable_opencv_show ) cv::destroyWindow("Tracking");
      if ( enable_barrier ) cross_barriers(barriers, tracker, id_box, det_time, cap_time, 0, data_outdir, crossing_frame_num, crossing, detection_data);

      // wait for all threads
      if (t_cap.joinable()) t_cap.join();
      if (t_prepare.joinable()) t_prepare.join();
      if (t_detect.joinable()) t_detect.join();
      if (t_post.joinable()) t_post.join();
      if (t_draw.joinable()) t_draw.join();
      if (t_write.joinable()) t_write.join();
    }
  }
  catch (std::exception &e)
  {
    std::cerr << "EXC: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "EXC: unknown exception." << std::endl;
  }

  return 0;
}
