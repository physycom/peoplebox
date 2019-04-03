
struct barrier
{
  std::string name;
  int x0, y0, x1, y1;
  int bx, by;
  int cnt_in, cnt_out;

  barrier() {}

  barrier(const std::string &name, const int &x0, const int &y0, const int &x1, const int &y1)
    : name(name), x0(x0), y0(y0), x1(x1), y1(y1), cnt_in(0), cnt_out(0)
  {
    bx = x1 - x0;
    by = y1 - y0;
  }

  template<typename track_t>
  int crossing(const track_t &track)
  {
    // track vector
    int tx = track.detection.front().x + int(track.detection.front().w * 0.5) - (track.detection.back().x + int(track.detection.back().w * 0.5));
    int ty = track.detection.front().y + int(track.detection.front().h * 0.5) - (track.detection.back().y + int(track.detection.back().h * 0.5));

    // relative vector
    int ux = (track.detection.back().x + int(track.detection.back().w * 0.5)) - x0;
    int uy = (track.detection.back().y + int(track.detection.back().h * 0.5)) - y0;

    // b x t
    int cross = bx * ty  -  tx * by;
    if ( !cross ) return 0; // b and t parallel
    float inv_cross = 1. / cross;

    // intersection point parameters
    float s1 = (ux * by - bx * uy) * inv_cross;    // - vp . w / vp . u
    float t1 = (ux * ty - tx * uy) * inv_cross;   // - vp . w / vp . u

    if ( s1 > 0 && s1 < 1 && t1 > 0 && t1 < 1)    // segment-segment intersection condition
    {
      if( cross > 0 ) return 1;
      else            return -1;
    }

    return 0;
  }

  void reset()
  {
    cnt_in = 0;
    cnt_out = 0;
  }
};

struct fat_barrier
{
  std::string name;
  barrier bp, bm;
  int thickness;
  int cnt_in, cnt_out;

  fat_barrier(const std::string &name, const int &P0x, const int &P0y, const int &P1x, const int &P1y, const int &thickness)
   : name(name), thickness(thickness), cnt_in(0), cnt_out(0)
  {
    float perpx = P0y - P1y;
    float perpy = P1x - P0x;
    float len = std::sqrt(perpx*perpx + perpy*perpy);
    int offsx = int( (perpx / len) * thickness);
    int offsy = int( (perpy / len) * thickness);

    bp = barrier(name, P0x + offsx, P0y + offsy, P1x + offsx, P1y + offsy);
    bm = barrier(name, P0x - offsx, P0y - offsy, P1x - offsx, P1y - offsy);
  }

  template<typename track_t>
  void crossing(const track_t &track)  
  {
    int ret = bp.crossing(track) + bm.crossing(track);
    if      ( ret > 0 ) ++cnt_in;
    else if ( ret < 0 ) ++cnt_out;
  }

  void reset()
  {
    cnt_in = 0;
    cnt_out = 0;
  }
};