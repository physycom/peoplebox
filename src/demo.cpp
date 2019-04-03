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

#include <kalman.hpp>
#include <tracking.hpp>
#include <barrier.hpp>

void draw_boxes(cv::Mat mat_img, std::vector<bbox_t> result_vec, std::vector<std::string> obj_names,
  const int &frame_num, int current_det_fps = -1, int current_cap_fps = -1)
{
  int const colors[6][3] = { {1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0} };

  for (auto &i : result_vec)
  {
    cv::Scalar color = obj_id_to_color(i.track_id % 6);
    cv::circle(mat_img, cv::Point(i.x + i.w / 2, i.y + i.h / 2), 20, color, 2);
    //        if (obj_names.size() > i.obj_id) {
    //            std::string obj_name = obj_names[i.obj_id];
    //            if (i.track_id > 0) obj_name += " - " + std::to_string(i.track_id);
    //            cv::Size const text_size = getTextSize(obj_name, cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, 2, 0);
    //            int max_width = (text_size.width > i.w + 2) ? text_size.width : (i.w + 2);
    //            max_width = std::max(max_width, (int)i.w + 2);
    //            //max_width = std::max(max_width, 283);
    //            std::string coords_3d;
    //            if (!std::isnan(i.z_3d)) {
    //                std::stringstream ss;
    //                ss << std::fixed << std::setprecision(2) << "x:" << i.x_3d << "m y:" << i.y_3d << "m z:" << i.z_3d << "m ";
    //                coords_3d = ss.str();
    //                cv::Size const text_size_3d = getTextSize(ss.str(), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, 1, 0);
    //                int const max_width_3d = (text_size_3d.width > i.w + 2) ? text_size_3d.width : (i.w + 2);
    //                if (max_width_3d > max_width) max_width = max_width_3d;
    //            }

    //            cv::rectangle(mat_img, cv::Point2f(std::max((int)i.x - 1, 0), std::max((int)i.y - 35, 0)),
    //                cv::Point2f(std::min((int)i.x + max_width, mat_img.cols - 1), std::min((int)i.y, mat_img.rows - 1)),
    //                color, CV_FILLED, 8, 0);
    //            putText(mat_img, obj_name, cv::Point2f(i.x, i.y - 16), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);
    //            if(!coords_3d.empty()) putText(mat_img, coords_3d, cv::Point2f(i.x, i.y-1), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(0, 0, 0), 1);
    //        }
  }
  if (current_det_fps >= 0 && current_cap_fps >= 0)
  {
    std::string fps_str = "FRAME " + std::to_string(frame_num) + "   FPS detection: " + std::to_string(current_det_fps) + "   FPS capture: " + std::to_string(current_cap_fps);
    putText(mat_img, fps_str, cv::Point2f(10, 20), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(50, 255, 0), 2);
  }
}

void draw_track(cv::Mat mat_img, const tracker_t &tracker)
{
  for (const auto &t : tracker.tracks)
  {
    for (int i = 0; i< int(t.detection.size() - 1); ++i)
    {
      if (i == 0)
      {
        putText(mat_img, std::to_string(t.track_id),
          cv::Point(t.detection[i].x + t.detection[i].w * 0.5, t.detection[i].y + t.detection[i].h * 0.5), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);
      }
      cv::arrowedLine(mat_img,
        cv::Point(t.detection[i].x + t.detection[i].w * 0.5, t.detection[i].y + t.detection[i].h * 0.5),
        cv::Point(t.detection[i + 1].x + t.detection[i + 1].w * 0.5, t.detection[i + 1].y + t.detection[i + 1].h * 0.5),
        obj_id_to_color(t.track_id % 6),
        5);
    }
  }
}

void draw_barrier(cv::Mat mat_img, const std::vector<barrier> &barriers)
{
  for (const auto &b : barriers)
  {
    putText(mat_img, b.name,
      cv::Point(b.x0, b.y0), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);

    cv::arrowedLine(mat_img,
      cv::Point(b.x0, b.y0),
      cv::Point(b.x1, b.y1),
      cv::Scalar(0, 0, 0),
      2);
  }
}

void draw_barrier(cv::Mat mat_img, const std::vector<fat_barrier> &barriers)
{
  for (const auto &b : barriers)
  {
    putText(mat_img, b.name,
      cv::Point((b.bp.x0 + b.bm.x0) * 0.5, (b.bp.y0 + b.bm.y0) * 0.5), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);

    cv::arrowedLine(mat_img,
      cv::Point(b.bp.x0, b.bp.y0),
      cv::Point(b.bp.x1, b.bp.y1),
      cv::Scalar(0, 0, 0),
      2);
    
    cv::arrowedLine(mat_img,
      cv::Point(b.bm.x0, b.bm.y0),
      cv::Point(b.bm.x1, b.bm.y1),
      cv::Scalar(0, 0, 0),
      2);

  }
}

#endif    // OPENCV


void show_console_result(std::vector<bbox_t> const result_vec, std::vector<std::string> const obj_names, int frame_id = -1) {
  if (frame_id >= 0) std::cout << " Frame: " << frame_id << std::endl;
  for (auto &i : result_vec) {
    if (obj_names.size() > i.obj_id) std::cout << obj_names[i.obj_id] << " - ";
    std::cout << "obj_id = " << i.obj_id << ",  x = " << i.x << ", y = " << i.y
      << ", w = " << i.w << ", h = " << i.h
      << std::setprecision(3) << ", prob = " << i.prob << std::endl;
  }
}

std::vector<std::string> objects_names_from_file(std::string const filename) {
  std::ifstream file(filename);
  std::vector<std::string> file_lines;
  if (!file.is_open()) return file_lines;
  for (std::string line; getline(file, line);) file_lines.push_back(line);
  std::cout << "object names loaded \n";
  return file_lines;
}

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

/////////////////////////////////// MAIN ///////////////////////////////////
void usage(char * progname)
{
  std::string pname(progname);
  std::cerr << "Usage: " << pname.substr(pname.find_last_of("/\\") + 1) << " path/to/json/config" << std::endl;
  std::cerr << R"(Sample config JSON file
{
  "network" : {
    "file_names"   : "darknet/data/coco.names",
    "file_cfg"     : "darknet/cfg/yolov3.cfg",
    "file_weights" : "darknet/yolov3.weights",
    "thresh"       : 0.2
  },
  "barriers" : {
    "test_barrier" : {
      "pnt_start" : [100, 100],
      "pnt_end"   : [900, 900]
    }
  },
  "opencv_show" : "false",
  "filename"    : "video.avi"
}
)";
}

constexpr int SW_VER = 100;

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

  std::string names_file, cfg_file, weights_file, filename;
  float thresh;
  bool openvc_show;

  // box vars
  std::string id_box;

  // barrier crossing vars
  std::vector<fat_barrier> barriers;
  int crossing_dt, crossing_frame_num, dump_dt, dump_rec_num;
  jsoncons::json crossing;
  bool enable_barrier = false;

  try
  {
    jsoncons::json jconf = jsoncons::json::parse_file(conf);

    if (!jconf.has_member("network")) throw std::runtime_error("No member 'network' in json config file.");

    names_file =   (jconf["network"].has_member("file_names"))   ? jconf["network"]["file_names"].as<std::string>()   : "data/coco.names";
    cfg_file =     (jconf["network"].has_member("file_cfg"))     ? jconf["network"]["file_cfg"].as<std::string>()     : "cfg/yolov3.cfg";
    weights_file = (jconf["network"].has_member("file_weights")) ? jconf["network"]["file_weights"].as<std::string>() : "yolov3.weights";
    thresh =       (jconf["network"].has_member("thresh"))       ? jconf["network"]["thresh"].as<float>()             : 0.2f;
    openvc_show =  (jconf.has_member("opencv_show"))             ? jconf["opencv_show"].as<bool>()                    : false;
    filename =     (jconf.has_member("filename"))                ? jconf["filename"].as<std::string>()                : "";

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

    id_box = (jconf.has_member("id_box")) ? jconf["id_box"].as<std::string>() : "unknown_box";
    crossing_dt = (jconf.has_member("crossing_dt")) ? jconf["crossing_dt"].as<int>() : 1;
    dump_dt =     (jconf.has_member("dump_dt"))     ? jconf["dump_dt"].as<int>()     : 5;
    if ( crossing_dt > dump_dt ) dump_dt = crossing_dt;
  }
  catch (std::exception &e)
  {
    std::cerr << "EXC: " << e.what() << std::endl;
    exit(2);
  }

  Detector detector(cfg_file, weights_file);
  auto obj_names = objects_names_from_file(names_file);

  bool const save_output_videofile = true;   // true - for history
  bool detection_sync = true;                // true - for video-file
  
  // tracking vars
  int frame_story = 10;
  int max_dist_px = 120;
  tracker_t tracker(frame_story, max_dist_px);

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
        detection_sync = false;

      cv::Mat cur_frame;
      std::atomic<int> fps_cap_counter(0), fps_det_counter(0);
      std::atomic<int> current_fps_cap(0), current_fps_det(0);
      std::atomic<bool> exit_flag(false);
      std::chrono::steady_clock::time_point steady_start, steady_end;
      int video_fps = 25;

      track_kalman_t track_kalman;

      cv::VideoCapture cap;
      cap.open(filename);
      if( !cap.isOpened() ) throw std::runtime_error("Unable to open input video: " + filename);
      video_fps = cap.get(CV_CAP_PROP_FPS);
      crossing_frame_num = video_fps * crossing_dt;
      dump_rec_num =  int(dump_dt / float(crossing_dt));
      cap >> cur_frame;
      cv::Size const frame_size = cur_frame.size();
      std::cout << "Video input: " << filename << "\nsize: " << frame_size << "  FPS: " << video_fps << std::endl;

      cv::VideoWriter output_video;
      std::string out_videofile = filename + ".track.avi";
      if (save_output_videofile)
        output_video.open(out_videofile, CV_FOURCC('M', 'J', 'P', 'G'), std::max(35, video_fps), frame_size, true);
        //output_video.open(out_videofile, CV_FOURCC('D', 'I', 'V', 'X'), std::max(35, video_fps), frame_size, true);

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
          cap >> detection_data.cap_frame;
          fps_cap_counter++;
          detection_data.frame_id = frame_id++;
          if (detection_data.cap_frame.empty() || exit_flag) {
            std::cout << " exit_flag: detection_data.cap_frame.size = " << detection_data.cap_frame.size() << std::endl;
            detection_data.exit_flag = true;
            detection_data.cap_frame = cv::Mat(frame_size, CV_8UC3);
          }

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

        } while (!detection_data.exit_flag);
        std::cout << " t_prepare exit" << std::endl;
      });


      // detection by Yolo
      if (t_detect.joinable()) t_detect.join();
      t_detect = std::thread([&]()
      {
        std::shared_ptr<image_t> det_image;
        detection_data_t detection_data;

        std::ofstream out_detections(filename + ".det");
        do {
          detection_data = prepare2detect.receive();
          det_image = detection_data.det_image;
          std::vector<bbox_t> result_vec;

          if (det_image)
            result_vec = detector.detect_resized(*det_image, frame_size.width, frame_size.height, thresh, true);  // true
          fps_det_counter++;
          //std::this_thread::sleep_for(std::chrono::milliseconds(150));

          for (const auto &b : result_vec)
            if (obj_names[b.obj_id] == "person")
              out_detections << detection_data.frame_id << ";"
              << b.x << ";" << b.y << ";"
              << b.w << ";" << b.h << std::endl;

          detection_data.new_detection = true;
          detection_data.result_vec = result_vec;
          detect2draw.send(detection_data);
        } while (!detection_data.exit_flag);
        out_detections.close();
        std::cout << " t_detect exit" << std::endl;
      });

      // draw rectangles (and track objects)
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
          {
            std::cout << "CROSSING Frame " << detection_data.frame_id << std::endl;
            for (auto &b : barriers)
            {
              for (const auto &t : tracker.tracks)
              {
                b.crossing(t);
              }
              //std::cout << "CROSSING " << b.name << "  Frame " << detection_data.frame_id << "  IN " << b.cnt_in << "  OUT " << b.cnt_out << std::endl;
            }

            jsoncons::json jrecord;
            auto tnow = std::time(0);
            jrecord["timestamp"] = tnow;
            jrecord["id_box"] = id_box;
            jrecord["detection"] = "counting";
            jrecord["sw_ver"] = SW_VER;

            // collect multi-barrier results
            jsoncons::json pplc = jsoncons::json::array();
            jsoncons::json j;
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
            jrecord["people_count"] = pplc;
            jrecord["diagnostic"] = jsoncons::json::parse(R"([{"id" : "coming", "value" : "soon"}])");

            std::stringstream ss;
            ss << "frame_" << std::setw(6) << std::setfill('0') << detection_data.frame_id;
            crossing[ss.str()] = jrecord;

            if ( crossing.size() >= dump_rec_num )
            {
              //std::cout << "Dumpo" << std::endl; 
              std::ofstream pplc_out("data/" + id_box + "_" + std::to_string(tnow) + ".json");
              if( !pplc_out ) throw std::runtime_error("Unable to create dump file.");
              pplc_out << jsoncons::pretty_print(crossing) << std::endl;
              pplc_out.close();
              crossing.clear();
            }

            // clean up tracks
            tracker.reset();
          }

          // decorate img
          draw_boxes(draw_frame, result_vec, obj_names, detection_data.frame_id, current_fps_det, current_fps_cap);
          draw_track(draw_frame, tracker);
          draw_barrier(draw_frame, barriers);

          std::stringstream ss;
          ss << std::setw(6) << std::setfill('0') << detection_data.frame_id;
          cv::imwrite(filename + ss.str() + ".arrow.jpg", draw_frame);

          detection_data.result_vec = result_vec;
          detection_data.draw_frame = draw_frame;
          draw2show.send(detection_data);
          if (output_video.isOpened()) draw2write.send(detection_data);
        } while (!detection_data.exit_flag);
        std::cout << " t_draw exit" << std::endl;
      });


      // write frame to videofile
      t_write = std::thread([&]()
      {
        if (output_video.isOpened()) {
          detection_data_t detection_data;
          cv::Mat output_frame;
          do {
            detection_data = draw2write.receive();
            if (detection_data.draw_frame.channels() == 4) cv::cvtColor(detection_data.draw_frame, output_frame, CV_RGBA2RGB);
            else output_frame = detection_data.draw_frame;
            output_video << output_frame;
          } while (!detection_data.exit_flag);
          output_video.release();
        }
        std::cout << " t_write exit" << std::endl;
      });

      // show detection
      detection_data_t detection_data;
      do {
        steady_end = std::chrono::steady_clock::now();
        float time_sec = std::chrono::duration<double>(steady_end - steady_start).count();
        if (time_sec >= 1) {
          current_fps_det = fps_det_counter.load() / time_sec;
          current_fps_cap = fps_cap_counter.load() / time_sec;
          steady_start = steady_end;
          fps_det_counter = 0;
          fps_cap_counter = 0;
        }

        detection_data = draw2show.receive();
        cv::Mat draw_frame = detection_data.draw_frame;

        if (openvc_show) {
          cv::Mat small_frame;
          cv::resize(draw_frame, small_frame, cv::Size(), 0.5, 0.5);
          cv::imshow("Tracking", small_frame);

          //cv::imshow("Tracking", draw_frame);
          int key = cv::waitKey(3);    // 3 or 16ms
          if (key == 'f') show_small_boxes = !show_small_boxes;
          if (key == 'p') while (true) if (cv::waitKey(100) == 'p') break;
          if (key == 27) exit_flag = true;
        }

        //std::cout << " current_fps_det = " << current_fps_det << ", current_fps_cap = " << current_fps_cap << std::endl;
      } while (!detection_data.exit_flag);
      std::cout << " show detection exit" << std::endl;

      if (openvc_show) cv::destroyWindow("Tracking");

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
