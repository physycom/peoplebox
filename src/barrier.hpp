
struct barrier
{
  std::string name;
  int x1, x2, y1, y2, w, h;

  // the line will be stored as
  // ax + by + c = 0
  bool s_start, s_end;
  int a, b, c;
  int cnt_in, cnt_out;


  int *zone;

  barrier(const std::string &name, const int &x1, const int &y1, const int &x2, const int &y2 /*, const int &w, const int &h */)
    //barrier(int x1, int y1, int x2, int y2, const int &w, const int &h)
    : name(name), x1(x1), y1(y1), x2(x2), y2(y2),
    cnt_in(0), cnt_out(0)
  {
    //zone = new int[w*h];

    if (y2 > y1)
    {
      a = y2 - y1;
      b = x1 - x2;
      c = -x1 * y2 + x2 * y1;
    }
    else
    {
      a = y1 - y2;
      b = x2 - x1;
      c = x1 * y2 - x2 * y1;
    }

    //    for(int i=0; i < w; ++i)
    //    {
    //      for(int j=0; j < h; ++j)
    //      {
    //
    //      }
    //    }
  }

  template<typename track_t>
  void crossing(const track_t &track)
  {
    int x = track.detection.front().x + int(track.detection.front().w * 0.5);
    int y = track.detection.front().y + int(track.detection.front().h * 0.5);
    s_start = (a*x + b * y + c > 0);

    x = track.detection.back().x + int(track.detection.back().w * 0.5);
    y = track.detection.back().y + int(track.detection.back().h * 0.5);
    s_end = (a*x + b * y + c > 0);

    if (s_start != s_end)
    {
      if (s_start)
        ++cnt_in;
      else
        ++cnt_out;
    }
  }

  void reset()
  {
    cnt_in = 0;
    cnt_out = 0;
  }
};