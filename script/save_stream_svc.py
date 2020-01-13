#! /usr/bin/env python3

import cv2
import sys
import win32service
import win32serviceutil
import win32event
from datetime import datetime, timedelta
import time
import json
import threading

CONFIGFILE = 'C:/Users/alex/Desktop/scripts/peoplebox/script/service/config.json'

class Service(win32serviceutil.ServiceFramework):

  # Service name in Task Manager
  _svc_name_ = "savestream-svc"
  # Service name in SERVICES Desktop App and Description in Task Manager
  _svc_display_name_ = "Save Stream SVC"
  # Service description in SERVICES Desktop App
  _svc_description_ = "Save Stream Service"

  def __init__(self, args):
    win32serviceutil.ServiceFramework.__init__(self, args)
    self.hWaitStop = win32event.CreateEvent(None, 0, 0, None)

    try:
      self.wdir = '/'.join(CONFIGFILE.split('/')[:-1])
      self.log = open( self.wdir + '/sss.log', 'w')
      self.log.write('{} : {} INIT\n'.format(datetime.now(), self._svc_description_)); self.log.flush()

      try:
        with open(CONFIGFILE) as f:
          self.cfg = json.load(f)
      except Exception as e:
        self.log.write('{} : problems with config file {}\n'.format(datetime.now(), CONFIGFILE)); self.log.flush()
        self.log.write('{} : Exception -> {}\n'.format(datetime.now(), e)); self.log.flush()
        sleep = 5
        self.log.write('{} : sleeping for {}s\n'.format(datetime.now(), sleep)); self.log.flush()
        time.sleep(sleep)
        exit(1)

      # time schedule initialization
      self.dt = int(self.cfg["chunk_len"]) # minutes
      self.start = datetime.strptime(self.cfg["start_date"], "%Y-%m-%d %H:%M:%S")
      self.stop = datetime.strptime(self.cfg["stop_date"], "%Y-%m-%d %H:%M:%S")
      self.pause = []
      for p in self.cfg['pause_interval']:
        [p0,p1] = p.split('-')
        self.pause.append((datetime.strptime(p0, "%H:%M"), datetime.strptime(p1, "%H:%M")))
      starth = datetime.strptime(self.start.strftime('%Y-%m-%d %H'), '%Y-%m-%d %H')
      minutes = (int(self.start.strftime("%M"))*self.dt)//self.dt
      start0 = (starth + timedelta(minutes=minutes)).strftime("%Y-%m-%d %H:%M:%S")
      ti = datetime.strptime(start0, "%Y-%m-%d %H:%M:%S")
      self.schedule = []
      while ti < self.stop:
        skip = False
        for p in self.pause:
          ti_t, p0_t, p1_t = ti.time(), p[0].time(), p[1].time()
          if p0_t < p1_t and ti_t >= p0_t and ti_t < p1_t:
            skip = True
            break
          elif p0_t > p1_t and ( ti_t >= p0_t or ti_t < p1_t ):
            skip = True
            break
        if not skip:
          self.schedule.append({
            'start_t' : ti.strftime("%Y-%m-%d %H:%M:%S"),
            'stop_t'  : (ti + timedelta(minutes=self.dt)).strftime("%Y-%m-%d %H:%M:%S"),
            'len'     : self.dt
          })
        ti += timedelta(minutes=self.dt)

      with open(self.wdir + '/schedule.json', 'w') as f:
        json.dump(self.schedule, f, indent=2)

      # cam modules initialization
      self.cams = []
      self.urls = []
      for i,c in enumerate([ c for c in sorted(self.cfg.keys()) if 'cam' in c ]):
        self.urls.append(self.cfg[c]['url'])
        self.cams.append(cv2.VideoCapture(self.urls[i]))

    except Exception as e:
      with open(self.wdir + '/debug.log', 'w') as f:
        f.write('Init Error : {}'.format(e))
        f.close()


  def SvcDoRun(self):
    try:
      rc = None
      self.log.write('{} : {} STARTING UP\n'.format(datetime.now(), self._svc_description_)); self.log.flush()
      si = 0
      tnow = datetime.now()

      while rc != win32event.WAIT_OBJECT_0 and tnow < self.stop and si < len(self.schedule):
        if tnow >= datetime.strptime(self.schedule[si]['start_t'], "%Y-%m-%d %H:%M:%S"):
          for i,c in enumerate([ c for c in sorted(self.cfg.keys()) if 'cam' in c ]):
            threading.Thread(target=self.do_thread,args=(i, self.schedule[si])).start()
          si += 1
        tnow = datetime.now()
        rc = win32event.WaitForSingleObject(self.hWaitStop, 999)

      time.sleep(self.dt*60 + 5)
      self.SvcStop()

    except Exception as e:
      with open(self.wdir + '/debug.log', 'w') as f:
        f.write('Run Error : {}'.format(e))
        f.close()

  def SvcStop(self):
    self.log.write('{} : SERVICE SHUTDOWN\n'.format(datetime.now())); self.log.flush()
    self.log.close()
    for c in self.cams:
      c.release()
    self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
    win32event.SetEvent(self.hWaitStop)

  def do_thread(self, i, s):
    try:
      tnow = datetime.now()
      self.log.write('{} : THREAD {} acquiring {}\n'.format(tnow, i, s['start_t'])); self.log.flush()

      while tnow < datetime.strptime(s['stop_t'], "%Y-%m-%d %H:%M:%S"):
        try:
          self.cam_record(i, s)
          break
        except ValueError as v:
          tnow = datetime.now()
          self.log.write('{} error: {}, time {}\n'.format(i,v, tnow)); self.log.flush()
          continue
      self.log.write('{} : THREAD {} acquisition done\n'.format(datetime.now(), i)); self.log.flush()

    except Exception as e:
      with open(self.wdir + '/debug.log', 'w') as f:
        f.write('Thread Error : {}'.format(e))
        f.close()


  def cam_record(self, i, s):
    camop = self.cams[i].grab()
    if camop:
      tcap = datetime.now()
      writer = cv2.VideoWriter(
              self.wdir + '/video_{}_{}.mp4'.format(i, datetime.strftime(tcap,"%Y%m%d_%H%M")),
              cv2.VideoWriter_fourcc(*'mp4v'),
              self.cams[i].get(cv2.CAP_PROP_FPS),
              (int(self.cams[i].get(cv2.CAP_PROP_FRAME_WIDTH)), int(self.cams[i].get(cv2.CAP_PROP_FRAME_HEIGHT)))
            )
      if writer.isOpened():
        while tcap < datetime.strptime(s['stop_t'], "%Y-%m-%d %H:%M:%S"):
          ret, frame = self.cams[i].read()
          if ret:
              writer.write(frame)
              tcap = datetime.now()
          else:
              writer.release()
              raise ValueError('Cam {} not capturing'.format(i))
        writer.release()
      else:
        raise ValueError('Video writer {} not opened'.format(i))
    else:
      self.cams[i].open(self.urls[i])
      raise ValueError('Cam {} not opened'.format(i))
    return

if __name__ == '__main__':
  win32serviceutil.HandleCommandLine(Service)