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
#include <grabber.hpp>

#ifdef OPENCV

std::vector<bbox_t> get_3d_coordinates(std::vector<bbox_t> bbox_vect, cv::Mat xyzrgba) {
  return bbox_vect;
}

#include <opencv2/opencv.hpp>            // C++
#include <opencv2/core/version.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/stitching.hpp>
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

constexpr int SW_VER = 101;

struct detection_data_t {
  cv::Mat cap_frame_L;
  cv::Mat cap_frame_R;
  cv::Mat frame_stitched;
  std::shared_ptr<image_t> det_image;
  std::vector<bbox_t> result_vec;
  cv::Mat draw_frame;
  bool new_detection;
  uint64_t frame_id;
  bool pending_crossing;
  bool exit_flag;
  detection_data_t() : exit_flag(false), new_detection(false), pending_crossing(false) {}
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
    "network": {
      "file_names"   : "darknet/data/coco.names",
      "file_cfg"     : "darknet/cfg/yolov3.cfg",
      "file_weights" : "darknet/yolov3.weights",
      "thresh"       : 0.2
    },
    "cam_L"          : "camera_ip",
    "cam_R"          : "camera_ip",
    "undistort": {
      "defish"       : false,
      "matK"         : [1107.30569, 0.0, 654.301915, 0.0, 1115.39688, 364.01479, 0.0, 0.0, 1.0],
      "matD"         : [-0.05071598, 0.02309643, -0.00413638, -0.14837196],
      "balance"      : 0.8
    },
    "stitching": {
      "try_cuda"        : true,
      "conf_thresh"     : 1,
      "match_conf"      : 0.3,
      "matcher_type"    : "homography",
      "estimator_type"  : "homography",
      "ba_cost_func"    : "ray",
      "ba_refine_mask"  : "xxxxx",
      "wave_correct"    : "horiz",
      "warp_type"       : "spherical",
      "expos_comp_type" : "gain_blocks",
      "seam_find_type"  : "gc_color",
      "blend_type"      : "multiband",
      "blend_strength"  : 5
    },
    "object_types"   : ["person"],
    "dist_param"     : [1, 1],
    "max_dist_px"    : 50,

    "enable_barrier" : false,

    "crossing_dt"    : 3,
    "dump_dt"        : 3,
    "id_box"         : "CAM",
    "data_outdir"    : "C:/data_outdir",

    "enable_opencv_show"   : true,
    "enable_detection_csv" : false,
    "enable_image_dump"    : false
  }
)";
}


/////////////////////////////////// MAIN ///////////////////////////////////
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

  // vars
  std::string names_file, cfg_file, weights_file, cam_L, cam_R, data_outdir, image_outdir, id_box;
  float thresh, balance;
  std::vector<std::string> obj_types;
  std::vector<fat_barrier> barriers;
  std::vector<double> dist_param, vecK, vecD;
  int crossing_dt, crossing_frame_num, dump_dt, dump_rec_num, max_dist_px, image_dt, image_frame_num;
  jsoncons::json crossing;
  bool enable_barrier = false;

  // debug vars
  std::ofstream out_detections;
  std::string out_detfile;
  cv::VideoWriter out_video;
  std::string out_videofile;
  std::string out_imagebase;
  bool enable_opencv_show = false;
  bool enable_detection_csv = false;
  bool enable_image_dump = false;

  // stitching vars
  bool try_cuda, do_wave_correct, correct_fisheye ;
  float conf_thresh, match_conf, blend_strength;
  std::string matcher_type, estimator_type, ba_cost_func, ba_refine_mask, warp_type, seam_find_type, wave_correct_t, expos_comp_type, blend_type;

  // parse config
  try
  {
    jsoncons::json jconf = jsoncons::json::parse_file(conf);

    // network parameters
    if (!jconf.has_member("network")) throw std::runtime_error("No member 'network' in json config file.");
    names_file =   (jconf["network"].has_member("file_names"))   ? jconf["network"]["file_names"].as<std::string>()   : "data/coco.names";
    cfg_file =     (jconf["network"].has_member("file_cfg"))     ? jconf["network"]["file_cfg"].as<std::string>()     : "cfg/yolov3.cfg";
    weights_file = (jconf["network"].has_member("file_weights")) ? jconf["network"]["file_weights"].as<std::string>() : "yolov3.weights";
    thresh =       (jconf["network"].has_member("thresh"))       ? jconf["network"]["thresh"].as<float>()             : 0.2f;
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

    // data and paths parameters
    cam_L        = (jconf.has_member("cam_L"))        ? jconf["cam_L"].as<std::string>()        : "";
    cam_R        = (jconf.has_member("cam_R"))        ? jconf["cam_R"].as<std::string>()        : "";
    data_outdir  = (jconf.has_member("data_outdir"))  ? jconf["data_outdir"].as<std::string>()  : "data";
    image_outdir = (jconf.has_member("image_outdir")) ? jconf["image_outdir"].as<std::string>() : "image";

    // box and time parameters
    id_box       = (jconf.has_member("id_box"))       ? jconf["id_box"].as<std::string>()       : "nameless_box";
    crossing_dt  = (jconf.has_member("crossing_dt"))  ? jconf["crossing_dt"].as<int>()          : 1;
    dump_dt      = (jconf.has_member("dump_dt"))      ? jconf["dump_dt"].as<int>()              : 5;
    if ( crossing_dt > dump_dt ) dump_dt = crossing_dt;
    image_dt     = (jconf.has_member("image_dt"))     ? jconf["image_dt"].as<int>()             : 300;

    // distortion correction parameters. ATTENTION: K and D must be obtained from calibration on images with dimensions equal to the ones captured from cams
    if (!jconf.has_member("undistort")) throw std::runtime_error("No member 'undistort' in json config file.");
    correct_fisheye = (jconf["undistort"].has_member("defish"))  ? jconf["undistort"]["defish"].as<bool>()                  : false;
    vecK            = (jconf["undistort"].has_member("matK"))    ? jconf["undistort"]["matK"].as<std::vector<double>>()     : std::vector<double>({100., 0., 100., 0., 100., 100., 0., 0., 1.});
    vecD            = (jconf["undistort"].has_member("matD"))    ? jconf["undistort"]["matD"].as<std::vector<double>>()     : std::vector<double>({0., 0., 0., 0.});
    balance         = (jconf["undistort"].has_member("balance")) ? jconf["undistort"]["balance"].as<float>()                : 0.0f;

    // stitching parameters
    if (!jconf.has_member("stitching")) throw std::runtime_error("No member 'stitching' in json config file.");
    try_cuda        = (jconf["stitching"].has_member("try_cuda"))        ? jconf["stitching"]["try_cuda"].as<bool>()               : false;
    matcher_type    = (jconf["stitching"].has_member("matcher_type"))    ? jconf["stitching"]["matcher_type"].as<std::string>()    : "homography";
    estimator_type  = (jconf["stitching"].has_member("estimator_type"))  ? jconf["stitching"]["estimator_type"].as<std::string>()  : "homography";
    match_conf      = (jconf["stitching"].has_member("match_conf"))      ? jconf["stitching"]["match_conf"].as<float>()            : 0.3f;
    conf_thresh     = (jconf["stitching"].has_member("conf_thresh"))     ? jconf["stitching"]["conf_thresh"].as<float>()           : 1.0f;
    ba_cost_func    = (jconf["stitching"].has_member("ba_cost_func"))    ? jconf["stitching"]["ba_cost_func"].as<std::string>()    : "ray";
    ba_refine_mask  = (jconf["stitching"].has_member("ba_refine_mask"))  ? jconf["stitching"]["ba_refine_mask"].as<std::string>()  : "xxxxx";
    wave_correct_t  = (jconf["stitching"].has_member("wave_correct"))    ? jconf["stitching"]["wave_correct"].as<std::string>()    : "no";
    warp_type       = (jconf["stitching"].has_member("warp_type"))       ? jconf["stitching"]["warp_type"].as<std::string>()       : "spherical";
    expos_comp_type = (jconf["stitching"].has_member("expos_comp_type")) ? jconf["stitching"]["expos_comp_type"].as<std::string>() : "gain_blocks";
    seam_find_type  = (jconf["stitching"].has_member("seam_find_type"))  ? jconf["stitching"]["seam_find_type"].as<std::string>()  : "gc_color";
    blend_type      = (jconf["stitching"].has_member("blend_type"))      ? jconf["stitching"]["blend_type"].as<std::string>()      : "multiband";
    blend_strength  = (jconf["stitching"].has_member("blend_strength"))  ? jconf["stitching"]["blend_strength"].as<float>()        : 5.0f;

    // debug parameters
    out_videofile = cam_L + ".tracking.avi";
    out_detfile = cam_L + ".det.csv";
    out_imagebase = cam_L;
    enable_opencv_show   = (jconf.has_member("enable_opencv_show"))   ? jconf["enable_opencv_show"].as<bool>()   : false;
    enable_detection_csv = (jconf.has_member("enable_detection_csv")) ? jconf["enable_detection_csv"].as<bool>() : false;
    enable_image_dump    = (jconf.has_member("enable_image_dump"))    ? jconf["enable_image_dump"].as<bool>()    : false;
  }
  catch (std::exception &e)
  {
    std::cerr << "EXC: " << e.what() << std::endl;
    exit(2);
  }

  std::cout << "-------------------- NETWORK --------------------" << std::endl;
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

    std::string const file_ext = cam_L.substr(cam_L.find_last_of(".") + 1);
    std::string const protocol = cam_L.substr(0, 7);
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

      int64 app_start_time = cv::getTickCount();
      int64 t = cv::getTickCount();
      GrabberThread grab1, grab2;
      std::atomic<int> fps_cap_counter(0), fps_det_counter(0);
      std::atomic<int> current_fps_cap(0), current_fps_det(0);
      std::atomic<bool> exit_flag(false);
      float det_time = 0.f, cap_time = 0.f;
      std::chrono::steady_clock::time_point steady_start, steady_end, start_cap, start_det;
      int video_fps = 25;

      track_kalman_t track_kalman;

      // cv stuff
      cv::Mat frame1, frame2, mapx, mapy;
      cv::Matx33d matK(&vecK[0]);
      cv::Matx33d newK;
      cv::Matx41d matD(&vecD[0]);
      cv::Ptr<cv::detail::FeaturesFinder> finder = cv::makePtr<cv::detail::OrbFeaturesFinder>();
      std::vector<cv::Mat> images(2);
      std::vector<cv::detail::ImageFeatures> features(2);
      std::vector<cv::detail::MatchesInfo> pairwise_matches;
      std::vector<cv::detail::CameraParams> cameras;
      cv::Ptr<cv::detail::FeaturesMatcher> matcher; // WORKS DIFFERENTLY WITH OR WITHOUT CUDA, BE CAREFUL
      cv::Ptr<cv::detail::Estimator> estimator;
      cv::Ptr<cv::detail::BundleAdjusterBase> adjuster;
      std::vector<double> focals;
      float warped_image_scale;
      std::vector<cv::Point> corners(2);
      std::vector<cv::UMat> masks_warped(2), images_warped(2), masks(2), images_warped_f(2);
      std::vector<cv::Size> sizes(2);
      cv::Ptr<cv::WarperCreator> warper_creator;
      cv::Ptr<cv::detail::RotationWarper> warper;
      cv::Ptr<cv::detail::ExposureCompensator> compensator;
      cv::Ptr<cv::detail::SeamFinder> seam_finder; // DOESN'T SUPPORT GPU FOR CUDA >= 8.0, CPU ONLY
      cv::Ptr<cv::detail::Blender> blender;
      cv::detail::WaveCorrectKind wave_correct;

      // Check parsed arguments and set objects
      if (wave_correct_t == "no")
          do_wave_correct = false;
      else if (wave_correct_t == "horiz")
      {
          do_wave_correct = true;
          wave_correct = cv::detail::WAVE_CORRECT_HORIZ;
      }
      else if (wave_correct_t == "vert")
      {
          do_wave_correct = true;
          wave_correct = cv::detail::WAVE_CORRECT_VERT;
      }
      if (expos_comp_type == "no")
          compensator = cv::detail::ExposureCompensator::createDefault(cv::detail::ExposureCompensator::NO);
      else if (expos_comp_type == "gain")
          compensator = cv::detail::ExposureCompensator::createDefault(cv::detail::ExposureCompensator::GAIN);
      else if (expos_comp_type == "gain_blocks")
          compensator = cv::detail::ExposureCompensator::createDefault(cv::detail::ExposureCompensator::GAIN_BLOCKS);
      if (blend_type == "no")
          blender = cv::detail::Blender::createDefault(cv::detail::Blender::NO, try_cuda);
      else if (blend_type == "feather")
          blender = cv::detail::Blender::createDefault(cv::detail::Blender::FEATHER, try_cuda);
      else if (blend_type == "multiband")
          blender = cv::detail::Blender::createDefault(cv::detail::Blender::MULTI_BAND, try_cuda);
      if (matcher_type == "affine")
          matcher = cv::makePtr<cv::detail::AffineBestOf2NearestMatcher>(false, try_cuda, match_conf);
      else
          matcher = cv::makePtr<cv::detail::BestOf2NearestMatcher>(try_cuda, match_conf);
      if (estimator_type == "affine")
          estimator = cv::makePtr<cv::detail::AffineBasedEstimator>();
      else
          estimator = cv::makePtr<cv::detail::HomographyBasedEstimator>();
      if (ba_cost_func == "reproj") adjuster = cv::makePtr<cv::detail::BundleAdjusterReproj>();
      else if (ba_cost_func == "ray") adjuster = cv::makePtr<cv::detail::BundleAdjusterRay>();
      else if (ba_cost_func == "affine") adjuster = cv::makePtr<cv::detail::BundleAdjusterAffinePartial>();
      else if (ba_cost_func == "no") adjuster = cv::makePtr<cv::detail::NoBundleAdjuster>();
      adjuster->setConfThresh(conf_thresh);
      cv::Mat_<uchar> refine_mask = cv::Mat::zeros(3, 3, CV_8U);
      if (ba_refine_mask[0] == 'x') refine_mask(0,0) = 1;
      if (ba_refine_mask[1] == 'x') refine_mask(0,1) = 1;
      if (ba_refine_mask[2] == 'x') refine_mask(0,2) = 1;
      if (ba_refine_mask[3] == 'x') refine_mask(1,1) = 1;
      if (ba_refine_mask[4] == 'x') refine_mask(1,2) = 1;
      adjuster->setRefinementMask(refine_mask);
      if (try_cuda && cv::cuda::getCudaEnabledDeviceCount() > 0)
      {
          if (warp_type == "plane")
              warper_creator = cv::makePtr<cv::PlaneWarperGpu>();
          else if (warp_type == "cylindrical")
              warper_creator = cv::makePtr<cv::CylindricalWarperGpu>();
          else if (warp_type == "spherical")
              warper_creator = cv::makePtr<cv::SphericalWarperGpu>();
      }
      else
      {
          if (warp_type == "plane")
              warper_creator = cv::makePtr<cv::PlaneWarper>();
          else if (warp_type == "affine")
              warper_creator = cv::makePtr<cv::AffineWarper>();
          else if (warp_type == "cylindrical")
              warper_creator = cv::makePtr<cv::CylindricalWarper>();
          else if (warp_type == "spherical")
              warper_creator = cv::makePtr<cv::SphericalWarper>();
          else if (warp_type == "fisheye")
              warper_creator = cv::makePtr<cv::FisheyeWarper>();
          else if (warp_type == "stereographic")
              warper_creator = cv::makePtr<cv::StereographicWarper>();
          else if (warp_type == "compressedPlaneA2B1")
              warper_creator = cv::makePtr<cv::CompressedRectilinearWarper>(2.0f, 1.0f);
          else if (warp_type == "compressedPlaneA1.5B1")
              warper_creator = cv::makePtr<cv::CompressedRectilinearWarper>(1.5f, 1.0f);
          else if (warp_type == "compressedPlanePortraitA2B1")
              warper_creator = cv::makePtr<cv::CompressedRectilinearPortraitWarper>(2.0f, 1.0f);
          else if (warp_type == "compressedPlanePortraitA1.5B1")
              warper_creator = cv::makePtr<cv::CompressedRectilinearPortraitWarper>(1.5f, 1.0f);
          else if (warp_type == "paniniA2B1")
              warper_creator = cv::makePtr<cv::PaniniWarper>(2.0f, 1.0f);
          else if (warp_type == "paniniA1.5B1")
              warper_creator = cv::makePtr<cv::PaniniWarper>(1.5f, 1.0f);
          else if (warp_type == "paniniPortraitA2B1")
              warper_creator = cv::makePtr<cv::PaniniPortraitWarper>(2.0f, 1.0f);
          else if (warp_type == "paniniPortraitA1.5B1")
              warper_creator = cv::makePtr<cv::PaniniPortraitWarper>(1.5f, 1.0f);
          else if (warp_type == "mercator")
              warper_creator = cv::makePtr<cv::MercatorWarper>();
          else if (warp_type == "transverseMercator")
              warper_creator = cv::makePtr<cv::TransverseMercatorWarper>();
      }
      if (!warper_creator)
      {
          std::cout << "Can't create the following warper '" << warp_type << "'\n";
          return 1;
      }
      if (seam_find_type == "no")
          seam_finder = cv::makePtr<cv::detail::NoSeamFinder>();
      else if (seam_find_type == "voronoi")
          seam_finder = cv::makePtr<cv::detail::VoronoiSeamFinder>();
      else if (seam_find_type == "gc_color")
      {
          seam_finder = cv::makePtr<cv::detail::GraphCutSeamFinder>(cv::detail::GraphCutSeamFinderBase::COST_COLOR);
      }
      else if (seam_find_type == "gc_colorgrad")
      {
          seam_finder = cv::makePtr<cv::detail::GraphCutSeamFinder>(cv::detail::GraphCutSeamFinderBase::COST_COLOR_GRAD);
      }
      else if (seam_find_type == "dp_color")
          seam_finder = cv::makePtr<cv::detail::DpSeamFinder>(cv::detail::DpSeamFinder::COLOR);
      else if (seam_find_type == "dp_colorgrad")
          seam_finder = cv::makePtr<cv::detail::DpSeamFinder>(cv::detail::DpSeamFinder::COLOR_GRAD);
      if (!seam_finder)
      {
          std::cout << "Can't create the following seam finder '" << seam_find_type << "'\n";
          return 1;
      }

      std::cout << "--------------------- SETUP ---------------------" << std::endl;
      // start grabber threads
      if (!grab1.Init(cam_L))
      {
#ifdef WIN32
         std::cerr << "Check that you have copied file opencv_ffmpeg3xx_64.dll to the same directory where is demo.exe" << std::endl;
#endif
         throw std::runtime_error("Unable to open input video " + cam_L);
         return -1;
      }
      std::cout << "Cam 1 init done... ";
      if (!grab2.Init(cam_R))
      {
#ifdef WIN32
         std::cerr << "Check that you have copied file opencv_ffmpeg3xx_64.dll to the same directory where is demo.exe" << std::endl;
#endif
         throw std::runtime_error("Unable to open input video " + cam_R);
         return -1;
      }
      std::cout << "Cam 2 init done... " << std::endl;

      std::thread t1(&GrabberThread::GrabThread, &grab1);
      std::thread t2(&GrabberThread::GrabThread, &grab2);
      std::size_t bufSize1, bufSize2;

      // get first couple frames for initial setup
      while (true) {
          grab1.PopFrame(frame1, &bufSize1);
          grab2.PopFrame(frame2, &bufSize2);

          if (!frame1.empty() && !frame2.empty())
          {
            if (correct_fisheye)
            {
              // correct camera fisheye (images must have same dimensions)
              cv::fisheye::estimateNewCameraMatrixForUndistortRectify(matK, matD, frame1.size(), cv::noArray(), newK, balance);
              cv::fisheye::initUndistortRectifyMap(matK, matD, cv::noArray(), newK, frame1.size(), CV_32F, mapx, mapy);
              cv::Mat m1, m2;
              cv::remap(frame1, m1, mapx, mapy, cv::INTER_CUBIC);
              cv::remap(frame2, m2, mapx, mapy, cv::INTER_CUBIC);
              images[0] = m1;
              images[1] = m2;
            }
            else{
              images[0] = frame1;
              images[1] = frame2;
            }

            t = cv::getTickCount();
            for (int i = 0; i < 2; ++i)
            {
                (*finder)(images[i], features[i]);
                features[i].img_idx = i;
                std::cout << "Features in image #" << i+1 << ": " << features[i].keypoints.size() << std::endl;
            }
            finder->collectGarbage();
            std::cout << "Finding features...                  elapsed time: " << ((cv::getTickCount() - t) / cv::getTickFrequency()) << " sec" << std::endl;
            std::cout << "Pairwise matching...                 ";
            t = cv::getTickCount();
            (*matcher)(features, pairwise_matches);
            matcher->collectGarbage();
            std::cout << "elapsed time: " << ((cv::getTickCount() - t) / cv::getTickFrequency()) << " sec" << std::endl;

            std::cout << "Cameras parameter adjustments...     ";
            t = cv::getTickCount();
            if (!(*estimator)(features, pairwise_matches, cameras))
            {
                std::cout << "Homography estimation failed.\n";
                return -1;
            }
            for (size_t i = 0; i < cameras.size(); ++i)
            {
                cv::Mat R;
                cameras[i].R.convertTo(R, CV_32F);
                cameras[i].R = R;
            }
            if (!(*adjuster)(features, pairwise_matches, cameras))
            {
                std::cout << "Camera parameters adjusting failed.\n";
                return -1;
            }
            for (size_t i = 0; i < cameras.size(); ++i)
            {
                focals.push_back(cameras[i].focal);
            }
            sort(focals.begin(), focals.end());
            warped_image_scale = static_cast<float>(focals[focals.size() / 2]);
            if (do_wave_correct)
            {
                std::vector<cv::Mat> rmats;
                for (size_t i = 0; i < cameras.size(); ++i)
                    rmats.push_back(cameras[i].R.clone());
                waveCorrect(rmats, wave_correct);
                for (size_t i = 0; i < cameras.size(); ++i)
                    cameras[i].R = rmats[i];
            }
            std::cout << "elapsed time: " << ((cv::getTickCount() - t) / cv::getTickFrequency()) << " sec" << std::endl;

            std::cout << "Warping images...                    ";
            t = cv::getTickCount();
            for (int i = 0; i < 2; ++i)
            {
                masks[i].create(images[i].size(), CV_8U);
                masks[i].setTo(cv::Scalar::all(255));
            }
            warper = warper_creator->create(static_cast<float>(warped_image_scale));
            for (int i = 0; i < 2; ++i)
            {
                cv::Mat_<float> K;
                cameras[i].K().convertTo(K, CV_32F);
                corners[i] = warper->warp(images[i], K, cameras[i].R, cv::INTER_LINEAR, cv::BORDER_REFLECT, images_warped[i]);
                sizes[i] = images_warped[i].size();
                warper->warp(masks[i], K, cameras[i].R, cv::INTER_NEAREST, cv::BORDER_CONSTANT, masks_warped[i]);
                images_warped[i].convertTo(images_warped_f[i], CV_32F);
            }
            std::cout << "elapsed time: " << ((cv::getTickCount() - t) / cv::getTickFrequency()) << " sec" << std::endl;

            std::cout << "Compensator...                       ";
            t = cv::getTickCount();
            compensator->feed(corners, images_warped, masks_warped);
            std::cout << "elapsed time: " << ((cv::getTickCount() - t) / cv::getTickFrequency()) << " sec" << std::endl;

            std::cout << "Seam finder...                       ";
            t = cv::getTickCount();
            seam_finder->find(images_warped_f, corners, masks_warped);
            std::cout << "elapsed time: " << ((cv::getTickCount() - t) / cv::getTickFrequency()) << " sec" << std::endl;

            std::cout << "Blender ";
            t = cv::getTickCount();
            cv::Size dst_sz = cv::detail::resultRoi(corners, sizes).size();
            float blend_width = std::sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
            if (blend_width < 1.f)
                blender = cv::detail::Blender::createDefault(cv::detail::Blender::NO, try_cuda);
            else if (blend_type == "multiband")
            {
                cv::detail::MultiBandBlender* mb = dynamic_cast<cv::detail::MultiBandBlender*>(blender.get());
                mb->setNumBands(static_cast<int>(std::ceil(std::log(blend_width)/std::log(2.)) - 1.));
                std::cout << "MB(" << mb->numBands() << ")";
            }
            else if (blend_type == "feather")
            {
                cv::detail::FeatherBlender* fb = dynamic_cast<cv::detail::FeatherBlender*>(blender.get());
                fb->setSharpness(1.f/blend_width);
                std::cout << "FB(" << fb->sharpness() << ")";
            }
            blender->prepare(corners, sizes);
            std::cout << "...                     elapsed time: " << ((cv::getTickCount() - t) / cv::getTickFrequency()) << " sec" << std::endl;

            // Release unused memory
            images[0].release();
            images[1].release();
            images_warped.clear();
            images_warped_f.clear();
            masks.clear();

            break; // exit from setup loop
          }
          else if (bufSize1 > 100 || bufSize2 > 100)
          {
              std::cerr << "Can't get frames from camera" << std::endl;
              return -1;
          }
      }

      video_fps = grab1.capGet(CV_CAP_PROP_FPS);
      if(grab2.capGet(CV_CAP_PROP_FPS) != video_fps) throw std::runtime_error("Cameras don't have same FPS");
      crossing_frame_num = video_fps * crossing_dt;
      dump_rec_num =  int(dump_dt / float(crossing_dt));
      image_frame_num = video_fps * image_dt;

      const bool sync = detection_sync; // sync data exchange
      send_one_replaceable_object_t<detection_data_t> cap2prepare(sync), cap2draw(sync),
        prepare2detect(sync), detect2draw(sync), draw2show(sync), draw2write(sync), draw2net(sync);

      std::thread t_prepare, t_detect, t_post, t_draw, t_write;

      std::cout << "-------------------- RUNNING --------------------" << std::endl;
      // stitch the videos

      // pre-processing video frame (resize, conversion)
      t_prepare = std::thread([&]()
      {
        std::shared_ptr<image_t> det_image;
        detection_data_t detection_data;
        do {
          detection_data = cap2prepare.receive();
          images.clear();
          cv::Mat m1, m2;
          if (correct_fisheye)
          {
            cv::remap(detection_data.cap_frame_L, m1, mapx, mapy, cv::INTER_CUBIC);
            cv::remap(detection_data.cap_frame_R, m2, mapx, mapy, cv::INTER_CUBIC);
          }
          else{
            m1 = detection_data.cap_frame_L;
            m2 = detection_data.cap_frame_R;
          }
          images.push_back(m1);
          images.push_back(m2);

          cv::Mat img_warped, img_warped_s, result, result_mask, dilated_mask, seam_mask, mask, mask_warped;
          cv::Size img_size = images[0].size();

          for (int img_idx = 0; img_idx < 2; ++img_idx)
          {
              cv::Mat K;
              cameras[img_idx].K().convertTo(K, CV_32F);

              // Warp images
              warper->warp(images[img_idx], K, cameras[img_idx].R, cv::INTER_LINEAR, cv::BORDER_REFLECT, img_warped);
              mask.create(img_size, CV_8U);
              mask.setTo(cv::Scalar::all(255));
              warper->warp(mask, K, cameras[img_idx].R, cv::INTER_NEAREST, cv::BORDER_CONSTANT, mask_warped);

              // Compensate exposure
              compensator->apply(img_idx, corners[img_idx], img_warped, mask_warped);

              img_warped.convertTo(img_warped_s, CV_16S);
              img_warped.release();
              images[img_idx].release();
              mask.release();

              dilate(masks_warped[img_idx], dilated_mask, cv::Mat());
              resize(dilated_mask, seam_mask, mask_warped.size(), 0, 0, cv::INTER_LINEAR_EXACT);
              mask_warped = seam_mask & mask_warped;

              // Blend the current image
              blender->feed(img_warped_s, mask_warped, corners[img_idx]);

          }

          blender->blend(result, result_mask);
          result.convertTo(detection_data.frame_stitched, CV_8U);

          det_image = detector.mat_to_image_resize(detection_data.frame_stitched);
          detection_data.det_image = det_image;
          prepare2detect.send(detection_data);    // detection

          // dump realtime image
          if ( detection_data.frame_id % image_frame_num == 0 )
          {
            cv::Mat barrier_frame = detection_data.frame_stitched;
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
          cv::Size stitched_size = detection_data.frame_stitched.size();

          if (det_image)
          {
            if (det_types.size())
              result_vec = detector.detect_resized(*det_image, stitched_size.width, stitched_size.height, det_types, thresh, true);
            else
              result_vec = detector.detect_resized(*det_image, stitched_size.width, stitched_size.height, thresh, true);
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
              cv::Mat old_cap_frame = detection_data.frame_stitched;   // use old captured frame
              detection_data = detect2draw.receive();
              if (!old_cap_frame.empty()) detection_data.frame_stitched = old_cap_frame;
            }
            // get new Captured frame
            else {
              std::vector<bbox_t> old_result_vec = detection_data.result_vec; // use old detections
              detection_data = cap2draw.receive();
              detection_data.result_vec = old_result_vec;
            }
          }

          cv::Mat draw_frame = detection_data.frame_stitched;
          std::vector<bbox_t> result_vec = detection_data.result_vec;

          // tracking
          tracker.update(result_vec);
          //std::cout << "Frame " << detection_data.frame_id << "  Tracks " << tracker.tracks.size() << "  Det " << result_vec.size() << std::endl;
          //for(const auto &t : tracker.tracks ) std::cout << t.track_id << " " << t.time_since_update << " " << t.state << std::endl;
          //std::cout << "----------------------------------------------------" << std::endl << std::endl;

          // barrier crossing
          if ( enable_barrier && detection_data.pending_crossing )
          {
            cross_barriers(barriers, tracker, id_box, det_time, cap_time, dump_rec_num, data_outdir, crossing_frame_num, crossing, detection_data);
            detection_data.pending_crossing = false;
          }

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

      // master thread
      uint64_t frame_id = 0;
      detection_data_t detection_data;
      do {
          detection_data = detection_data_t();
          start_cap = std::chrono::steady_clock::now();
          grab1.PopFrame(detection_data.cap_frame_L, &bufSize1);
          grab2.PopFrame(detection_data.cap_frame_R, &bufSize2);

          fps_cap_counter++;
          detection_data.frame_id = frame_id++;
          if ( detection_data.frame_id % crossing_frame_num == 0 )
            detection_data.pending_crossing = true;
          if (detection_data.cap_frame_L.empty() || detection_data.cap_frame_R.empty() || exit_flag) {
            std::cout << " exit_flag: detection_data.cap_frame_L.size = " << detection_data.cap_frame_L.size() << std::endl;
            std::cout << " exit_flag: detection_data.cap_frame_R.size = " << detection_data.cap_frame_R.size() << std::endl;
            detection_data.exit_flag = true;
            detection_data.cap_frame_L = cv::Mat(frame1.size(), CV_8UC3);
            detection_data.cap_frame_R = cv::Mat(frame2.size(), CV_8UC3);
            break;
          }
          cap_time += std::chrono::duration<float>(std::chrono::steady_clock::now() - start_cap).count();

          if (bufSize1 < 50 && bufSize2 < 50){ // skip everything if buffer is getting too full
            cap2prepare.send(detection_data);
            detection_data = draw2show.receive();
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
              cv::imshow("Tracking", draw_frame);

              int key = cv::waitKey(3);    // 3 or 16ms
              if (key == 'f') show_small_boxes = !show_small_boxes;
              if (key == 'p') while (true) if (cv::waitKey(100) == 'p') break;
              if (key == 27) exit_flag = true;
            }
          }
      }
      while (!detection_data.exit_flag);
      // send one last time to make other threads exit
      cap2prepare.send(detection_data);
      detection_data = draw2show.receive();
      std::cout << " t_main exit" << std::endl;

      grab1.StopGrabbing();
      grab2.StopGrabbing();

      if ( enable_opencv_show ) cv::destroyWindow("Tracking");
      if ( enable_barrier ) cross_barriers(barriers, tracker, id_box, det_time, cap_time, 0, data_outdir, crossing_frame_num, crossing, detection_data);
      // wait for all threads
      if (t1.joinable()) t1.join();
      if (t2.joinable()) t2.join();
      if (t_prepare.joinable()) t_prepare.join();
      if (t_detect.joinable()) t_detect.join();
      if (t_post.joinable()) t_post.join();
      if (t_draw.joinable()) t_draw.join();
      if (t_write.joinable()) t_write.join();

      std::cout << "Flushing buffer1 of:" << grab1.GetBufSize() << " frames..." << std::endl;
      std::cout << "Flushing buffer2 of:" << grab2.GetBufSize() << " frames..." << std::endl;
      while (!(frame1.empty() && frame2.empty()))
      {
        grab1.PopFrame(frame1);
        grab2.PopFrame(frame2);
      }
      std::cout << "------------------- STATISTICS ------------------" << std::endl;
      grab1.PrintStats();
      grab2.PrintStats();
      std::cout << "Finished...                          total time: " << ((cv::getTickCount() - app_start_time) / cv::getTickFrequency()) << " sec" << std::endl;
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
