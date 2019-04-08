
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