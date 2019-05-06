class p_detector : public Detector
{
  using Detector::Detector;

public:
  using Detector::detect_resized;
  std::vector<bbox_t> detect_resized(image_t img, int init_w, int init_h, const std::vector<int> &types_idx, float thresh = 0.2, bool use_mean = false)
  {
      if (img.data == NULL)
          throw std::runtime_error("Image is empty");
      auto detection_boxes = detect(img, thresh, use_mean);
      for (auto it = detection_boxes.begin(); it != detection_boxes.end();)
      {
        if ( std::any_of(types_idx.begin(), types_idx.end(), [it](int idx){ return it->obj_id == idx; }) )
          ++it;
        else
          it = detection_boxes.erase(it);
      }
      float wk = (float)init_w / img.w, hk = (float)init_h / img.h;
      for (auto &i : detection_boxes) i.x *= wk, i.w *= wk, i.y *= hk, i.h *= hk;
      return detection_boxes;
  }
};