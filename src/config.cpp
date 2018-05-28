#include <iostream>
#include <fstream>
#include <map>
#include <config.h>

enum{
  up,
  down,
  left,
  right
};

std::map<std::string, int> direction_map ={
  {"UP", up},
  {"DOWN", down},
  {"LEFT", left},
  {"RIGHT", right}
};

config parse_config_file(const char *filename)
{
  std::ifstream cfgfile(filename);
  if(!cfgfile)
  {
    std::cout << "cannot open config file : " << filename << std::endl;
    exit(ERR_NO_CFG_FILE);
  }

  config cfg;
  std::string key, equal, value;

  cfg.FILENAME = NULL;

  try
  {
    while( cfgfile >> key >> equal >> value )
    {
      if( key == "BARRIER_TOP" )
      {
        cfg.BARRIER_TOP = std::stoi(value);
      }
      else if( key == "BARRIER_BOTTOM" )
      {
        cfg.BARRIER_BOTTOM = std::stoi(value);
      }
      else if( key == "BARRIER_LEFT" )
      {
        cfg.BARRIER_LEFT = std::stoi(value);
      }
      else if( key == "BARRIER_RIGHT" )
      {
        cfg.BARRIER_RIGHT = std::stoi(value);
      }
      else if( key == "TOLL" )
      {
        cfg.BARRIER_RIGHT = std::stoi(value);
      }
      else if( key == "DIST_THRESH" )
      {
        cfg.DIST_THRESH = std::stof(value);
      }
      else if( key == "SCALE_X" )
      {
        cfg.SCALE_X = std::stof(value);
      }
      else if( key == "SCALE_Y" )
      {
        cfg.SCALE_Y = std::stof(value);
      }
      else if( key == "DIRECTION" )
      {
        cfg.DIRECTION = direction_map[value];
      }
      else if( key == "PEOPLEBOX_ID" )
      {
        sprintf(cfg.PEOPLEBOX_ID, "%s", value.c_str());
      }
      else if( key == "CAMERA_IP" )
      {
        sprintf(cfg.CAMERA_IP, "%s", value.c_str());
      }
      else if( key == "CAMERA_CREDENTIALS" )
      {
        sprintf(cfg.CAMERA_CREDENTIALS, "%s", value.c_str());
      }
      else if( key == "DETECTION_TYPE_TRACK" )
      {
        sprintf(cfg.DETECTION_TYPE_TRACK, "%s", value.c_str());
      }
      else if( key == "FILENAME" )
      {
        cfg.FILENAME = (char*)malloc(500*sizeof(char));
        sprintf(cfg.FILENAME, "%s", value.c_str());
      }
      else if( key == "BARRIER_IN" )
      {
        sprintf(cfg.BARRIER_IN, "%s", value.c_str());
      }
      else if( key == "BARRIER_OUT" )
      {
        sprintf(cfg.BARRIER_OUT, "%s", value.c_str());
      }
      else if ( key == "C0" )
      {
        cfg.C0 = std::stof(value);
      }
      else if ( key == "C1" )
      {
        cfg.C1 = std::stof(value);
      }
      else if ( key == "C2" )
      {
        cfg.C2 = std::stof(value);
      }
      else if ( key == "C3" )
      {
        cfg.C3 = std::stof(value);
      }
      else if ( key == "C4" )
      {
        cfg.C4 = std::stof(value);
      }
      else if ( key == "C5" )
      {
        cfg.C5 = std::stof(value);
      }
      else if ( key == "MAX_FRAME_INFO_TO_STORE" )
      {
        cfg.MAX_FRAME_INFO_TO_STORE = std::stoi(value);
      }
      else if ( key == "FRAME_NUMBER_TRACKING" )
      {
        cfg.FRAME_NUMBER_TRACKING = std::stoi(value);
      }
      else if ( key == "SAMPLING_DT_SEC" )
      {
        cfg.SAMPLING_DT_SEC = std::stoi(value);
      }
      else if ( key == "FPS" )
      {
        cfg.FPS = std::stoi(value);
      }
      else if ( key == "JSON_FOLDER" )
      {
        sprintf(cfg.JSON_FOLDER, "%s", value.c_str());
      }
      else
      {
        std::cerr << "Key \"" << key << "\" unknown" << std::endl;
        exit(ERR_MALFORMED_CONFIG);
      }
    }
  }
  catch(std::exception &e)
  {
    std::cout << "ERROR: " << e.what() << std::endl;
    exit(ERR_MALFORMED_CONFIG);
  }

  cfgfile.close();

  if( cfg.FILENAME == NULL )
  {
    std::cout << "qui" << std::endl;
    cfg.FILENAME = (char*)malloc(500*sizeof(char));
    std::cout << "qui" << std::endl;
    sprintf(cfg.FILENAME, "rtsp://%s@%s:554/rtpstream/config1=u", cfg.CAMERA_CREDENTIALS, cfg.CAMERA_IP);
  }

  return cfg;
}

void print_config(const config cfg)
{
  std::cout << "**** CONFIG ****" << std::endl;
  std::cout << "BARRIER_TOP             : " << cfg.BARRIER_TOP << std::endl;
  std::cout << "BARRIER_BOTTOM          : " << cfg.BARRIER_BOTTOM << std::endl;
  std::cout << "BARRIER_LEFT            : " << cfg.BARRIER_LEFT << std::endl;
  std::cout << "BARRIER_RIGHT           : " << cfg.BARRIER_RIGHT << std::endl;
  std::cout << "TOLL                    : " << cfg.TOLL << std::endl;
  std::cout << "DIST_THRESH             : " << cfg.DIST_THRESH << std::endl;
  std::cout << "SCALE_X                 : " << cfg.SCALE_X << std::endl;
  std::cout << "SCALE_Y                 : " << cfg.SCALE_Y << std::endl;
  std::cout << "DIRECTION               : " << cfg.DIRECTION << std::endl;
  std::cout << "PEOPLEBOX_ID            : " << cfg.PEOPLEBOX_ID << std::endl;
  std::cout << "CAMERA_IP               : " << cfg.CAMERA_IP << std::endl;
  std::cout << "CAMERA_CREDENTIALS      : " << cfg.CAMERA_CREDENTIALS << std::endl;
  std::cout << "DETECTION_TYPE_TRACK    : " << cfg.DETECTION_TYPE_TRACK << std::endl;
  std::cout << "BARRIER_IN              : " << cfg.BARRIER_IN << std::endl;
  std::cout << "BARRIER_OUT             : " << cfg.BARRIER_OUT << std::endl;
  std::cout << "DIRECTION               : " << cfg.DIRECTION << std::endl;
  std::cout << "C0                      : " << cfg.C0 << std::endl;
  std::cout << "C1                      : " << cfg.C1 << std::endl;
  std::cout << "C2                      : " << cfg.C2 << std::endl;
  std::cout << "C3                      : " << cfg.C3 << std::endl;
  std::cout << "C4                      : " << cfg.C4 << std::endl;
  std::cout << "C5                      : " << cfg.C5 << std::endl;
  std::cout << "MAX_FRAME_INFO_TO_STORE : " << cfg.MAX_FRAME_INFO_TO_STORE << std::endl;
  std::cout << "FRAME_NUMBER_TRACKING   : " << cfg.FRAME_NUMBER_TRACKING << std::endl;
  std::cout << "FPS                     : " << cfg.FPS << std::endl;
  std::cout << "JSON_FOLDER             : " << cfg.JSON_FOLDER << std::endl;
}
