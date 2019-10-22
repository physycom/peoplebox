#! /usr/bin/env python3
 
import sys
import win32service
import win32serviceutil
import win32event
from datetime import datetime, timedelta
import time
import json
import threading
 
CONFIGFILE = './service_config.json'
 
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
          cfg = json.load(f)
      except Exception as e:
        self.log.write('{} : problems with config file {}\n'.format(datetime.now(), CONFIGFILE)); self.log.flush()
        self.log.write('{} : Exception -> {}\n'.format(datetime.now(), e)); self.log.flush()
        sleep = 5
        self.log.write('{} : sleeping for {}s\n'.format(datetime.now(), sleep)); self.log.flush()
        #time.sleep(sleep)
        exit(1)
 
      self.dt = int(cfg["chunk_len"]) # minutes
      self.start = datetime.strptime(cfg["start_date"], "%Y-%m-%d %H:%M:%S")
      self.stop = datetime.strptime(cfg["stop_date"], "%Y-%m-%d %H:%M:%S")
      self.pause = []
      for p in cfg['pause_interval']:
        [p0,p1] = p.split('-')
        self.pause.append((datetime.strptime(p0, "%H:%M"), datetime.strptime(p1, "%H:%M")))
      starth = datetime.strptime(self.start.strftime('%Y-%m-%d %H'), '%Y-%m-%d %H')
      minutes = (int(self.start.strftime("%M"))*self.dt)//self.dt + self.dt
      start0 = (starth + timedelta(minutes=minutes)).strftime("%Y-%m-%d %H:%M:%S")
      ti = datetime.strptime(start0, "%Y-%m-%d %H:%M:%S")
      self.schedule = []
      while ti <= self.stop:
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
    except Exception as e:
      with open(self.wdir + '/debug.log', 'w') as f:
        f.write('Error : {}'.format(e))
        f.close()
 
 
  def SvcDoRun(self):
 
    rc = None
    self.log.write('{} : {} STARTING UP\n'.format(datetime.now())), self._svc_description_)); self.log.flush()
 
    si = 0
    tnow = datetime.now()
    while rc != win32event.WAIT_OBJECT_0 and tnow < self.stop:
 
#      if tnow < schedule[si]['start_t']:
#        continue
#      else:
#        for i,c in enumerate([ c for c in sorted(cfg.keys()) if 'cam' in c ]):
#          threading.Thread(target=self.do_thread,args=[self.log, i, schedule[si]]).start()
#        si += 1
#        tnow = datetime.now()
 
      rc = win32event.WaitForSingleObject(self.hWaitStop, 999)
 
  def do_thread(self, i, s):
    self.log.write('{} : THREAD {} acquiring {}'.format(datetime.now(), i, s['start_t'])); self.log.flush()
    # opencv stuff
    # with some error handling
    time.sleep(20)
    # sleep to fake lenghty operation
    self.log.write('{} : THREAD {} acquisition done'.format(datetime.now(), i)); self.log.flush()
 
 
  def SvcStop(self):
 
    self.time = datetime.now()
    self.log.write('{} : SERVICE SHUTDOWN\n'.format(self.time)); self.log.flush()
    self.log.close()
    self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
    win32event.SetEvent(self.hWaitStop)
 
if __name__ == '__main__':
  win32serviceutil.HandleCommandLine(Service)