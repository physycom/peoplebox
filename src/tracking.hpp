#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <vector>
#include <map>
#include <list>

#include <hungarian.hpp>
#include <yolo_v2_class.hpp>

////////// TRACK STATE
typedef enum
{
  CONFIRMED = 0,
  DELETED   = 1,
  TENTATIVE = 2,
} track_state;

////////// TRACK
constexpr int default_max_age = 90;
constexpr int default_n_init = 1;

class dist
{
private:
  double a, b;

public:
  dist(const double &a, const double &b) : a(a), b(b) {}
  double operator()(const int &dx, const int &dy)
  {
    return std::sqrt(a*dx*dx + b*dy*dy);
  }
};


struct track_t
{
  int track_id;
  int time_since_update;
  int hits;
  int n_init;    // find a more intuitive name
  int max_age;
  int state;
  std::vector<bbox_t> detection;

  track_t(const int &trackid, const bbox_t &detection)
  {
    this->track_id = trackid;
    this->detection.push_back(detection);
    this->time_since_update = 0;
    this->hits = 1;
    this->state = track_state::TENTATIVE;
    this->max_age = default_max_age;
    this->n_init = default_n_init;
  }

  void update(const bbox_t &det)
  {
    //kf to add here
    this->hits++;
    this->time_since_update = 0;
    if (this->state == track_state::TENTATIVE && this->hits > this->n_init)
      this->state = track_state::CONFIRMED;
    this->detection.push_back(det);
  }

  void mark_missed()
  {
//    std::cout << "dentro " << track_id << " " << state << std::endl;
    if (this->state == track_state::TENTATIVE || this->time_since_update > this->max_age)
    {
//      std::cout << "puzzammerda " << this->max_age << std::endl;
      this->state = track_state::DELETED;
    }
  }
};

////////// TRACKER
class tracker_t
{
public:
  int frames_story, max_dist_px;
  std::vector<track_t> tracks;
  std::vector<std::pair<int, int>> matches_l;
  std::vector<int> unmatched_track_l;
  dist d;

  tracker_t(const int &frames_story, const int &max_dist_px, const std::vector<double> &dist_param) :
    frames_story(frames_story), max_dist_px(max_dist_px), n_trackid(0), d(dist_param[0], dist_param[1]) {}

  void reset()
  {
    tracks.clear();
    n_trackid = 0;
  }

  void match(const std::vector<int> &track_indices_l, const std::vector<bbox_t> &detect, const std::vector<int> &detect_indices)
  {
    if (track_indices_l.size() == 0 || detect_indices.size() == 0) //nothing to match
    {
      unmatched_det = detect_indices;
      unmatched_track_l = track_indices_l;
      return;
    }

    int Ntrack = track_indices_l.size();
    int Ndet = detect_indices.size();
    int N = (Ntrack > Ndet) ? Ntrack : Ndet;

    //create matrix cost
    Hungarian::Matrix cost(Ntrack, std::vector<int>(Ndet, 0));
    for (int i=0; i < Ntrack; ++i)
    {
      for (int j=0; j < Ndet; ++j)
      {
        int ii = track_indices_l[i];
        int jj = detect_indices[j];
        float dx = (float)((tracks[ii].detection.back().x + tracks[ii].detection.back().w * 0.5) - (float)(detect[jj].x + detect[jj].w * 0.5));
        float dy = (float)((tracks[ii].detection.back().y + tracks[ii].detection.back().h * 0.5) - (float)(detect[jj].y + detect[jj].h * 0.5));
        cost[i][j] = int(d(dx, dy));
        // cost[i][j] = int(std::sqrt(dx*dx + dy*dy));

//        std::cout << i << " " << j << " " << ii << " " << jj << " | " << dx << " " << dy << " " << cost[i][j] << std::endl;
//        std::cout << tracks[ii].detection.back().x + tracks[ii].detection.back().w * 0.5 << " " << detect[jj].x + detect[jj].w * 0.5 << std::endl;
//        std::cout << tracks[ii].detection.back().x << " " << detect[jj].x << std::endl;
      }
    }
//    Hungarian::PrintMatrix(cost);

    // this should become a class member
    // improve hungarian encapsulation and naming
    Hungarian::Result min;
    min.solve(cost, Hungarian::MODE_MINIMIZE_COST);

    //std::cout << "ass" << std::endl;
    //Hungarian::PrintMatrix(min.assignment);

    unmatched_det.clear();
    for (int i = 0; i < N; ++i)
    {
      for (int j = 0; j < N; ++j)
      {
        if (min.assignment[i][j] == 1)
        {
          if (i >= Ntrack) unmatched_det.push_back(detect_indices[j]);
          else if (j >= Ndet) unmatched_track_l.push_back(track_indices_l[i]);
          else if (cost[i][j] > max_dist_px) {
            unmatched_det.push_back(detect_indices[j]);
            unmatched_track_l.push_back(track_indices_l[i]);
          }
          else matches_l.push_back(std::make_pair(track_indices_l[i], detect_indices[j]));
        }
      }
    }

    return;
  }

  void update(const std::vector<bbox_t> &detection)
  {
    for (auto &t : tracks) t.time_since_update++;

    confirmed_tracks.clear();
    unconfirmed_tracks.clear();
    unmatched_track.clear();
    unmatched_det.clear();
    matches.clear();
    for (int i = 0; i < (int)tracks.size(); ++i)
    {
      if (tracks[i].state == track_state::CONFIRMED) confirmed_tracks.push_back(i);
      else unconfirmed_tracks.push_back(i);
    }

    std::vector<int> all_track_indices(tracks.size());
    std::iota(all_track_indices.begin(), all_track_indices.end(), 0);
   //for (auto &i : all_track_indices) std::cout << i << " ";

    tracking(detection, all_track_indices);
//    tracking(detection, confirmed_tracks);
//    tracking(detection, unconfirmed_tracks);
//    for(const auto &p : matches) std::cout << p.first << " --> " << p.second << std::endl;

    for (const auto &p : matches)         tracks[p.first].update(detection[p.second]);
    for (const auto &i : unmatched_track) tracks[i].mark_missed();
    for (const auto &i : unmatched_det)   tracks.emplace_back(n_trackid++, detection[i]);

    // remove deleted tracks
    // fix with it->erase() done right
    std::vector<track_t> new_tracks;
    for (const auto &t : tracks) if (t.state != track_state::DELETED) new_tracks.push_back(t);
    tracks = new_tracks;

    return;
  }

  void tracking(const std::vector<bbox_t> &detection, const std::vector<int> &track_indices)
  {
    std::vector<int> detect_indices;
    for (int j = 0; j<int(detection.size()); ++j) detect_indices.push_back(j);
    unmatched_det = detect_indices;

    for (int level = 0; level < frames_story; ++level)
    {
      unmatched_track_l.clear();
      matches_l.clear();

      if (unmatched_det.size() == 0) break; // no detection left

      std::vector<int> track_indices_level;
      for (int i = 0; i < track_indices.size(); ++i)
        if (tracks[i].time_since_update == level + 1)
          track_indices_level.push_back(track_indices[i]);

      if (track_indices_level.size() == 0) continue; // no match at this level

      // update unmatched detection index proxy
      detect_indices = unmatched_det;

//      std::cout << "Level " << level << std::endl;
      match(track_indices_level, detection, detect_indices);
      matches.insert(matches.end(), matches_l.begin(), matches_l.end());
//      std::cout << "matches_l: " << matches_l.size() << std::endl;
//      std::cout << "unmatched_track_l: " << unmatched_track_l.size() << std::endl;
//      std::cout << "unmatched_det (" << unmatched_det.size() << ") ";
//      for(const auto &i : unmatched_det) std::cout << i << " ";
//      std::cout << std::endl;

      //for(const auto &p : matches) std::cout << p.first << " --> " << p.second << std::endl;
    }

    for (auto &i : track_indices) {
      bool track_present = false;
      for (auto &j : matches)
        if (j.first == i) {
          track_present = true;
          break;
        }
      if (!track_present) unmatched_track.push_back(i);
    }

  }

private:
  int n_trackid;
  std::vector<std::pair<int, int>> matches;
  std::vector<int> unmatched_track, unmatched_det;
  std::vector<int> confirmed_tracks, unconfirmed_tracks;
};
