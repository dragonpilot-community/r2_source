#!/usr/bin/env python3
#pylint: skip-file
# The MIT License
#
# Copyright (c) 2019-, Rick Lan, dragonpilot community, and a number of other of contributors.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import cereal.messaging as messaging
from openpilot.common.realtime import Ratekeeper, set_core_affinity, set_realtime_priority
from openpilot.common.params import Params
import json
from cereal import log
import overpy
import os
import subprocess
from openpilot.common.conversions import Conversions as CV
import re
import time

OSM_QUERY = ["/data/media/0/osm/bin/osm3s_query", "--db-dir=/data/media/0/osm/db/"]

OSM_ONLINE_QUERY_THRESHOLD = 5 # secs
OSM_LOCAL_QUERY_THRESHOLD = 3 # times

class OSM():
  def __init__(self, last_gps_pos):
    self._api = overpy.Overpass()

    self._local_osm_query_fail_count = 0
    self._last_gps_pos = last_gps_pos
    self._fetch_time_prev = 0.
    self._way_tags = []

    self.way_id = 0
    self.road_name = None
    self.speed_limit = 0

    if os.path.isdir("/data/media/0/osm/bin/") and os.path.isdir("/data/media/0/osm/db/"):
      self.local_osm_enabled = True
      print("Local OSM installed")
    else:
      self.local_osm_enabled = False
    print("Local OSM enabled = %s" % self.local_osm_enabled)

  def _process_res(self, res):
    if len(res) > 0:
      self._way_tags = res[0].tags
      self.way_id = res[0].id


  def fetch_tags_around_location(self, latitude, longitude):
    if len(self._way_tags) > 0 and len(self._last_gps_pos) > 0 and self._last_gps_pos["latitude"] == latitude and self._last_gps_pos["longitude"] == longitude:
      return self._way_tags
    q = f"""
    way(around:10, {latitude}, {longitude})
          [highway]
          [highway~"^(motorway|trunk|primary|secondary|tertiary|unclassified|residential)$"];
    (
        way._["name"];
        way._["maxspeed"];
        (._;>;);
    );
    out tags;
    """
    if self.local_osm_enabled:
      try:
        completion = subprocess.run(OSM_QUERY + [f"--request={q}"], check=True, capture_output=True)
        res = self._api.parse_xml(completion.stdout).ways
        self._process_res(res)
      except Exception as e:
        self._local_osm_query_fail_count += 1
        pass

    # use remote OSM when local osm is not enabled or failed too many times
    if not self.local_osm_enabled or self._local_osm_query_fail_count >= OSM_LOCAL_QUERY_THRESHOLD:
      fetch_time = time.monotonic()
      if fetch_time - self._fetch_time_prev < OSM_ONLINE_QUERY_THRESHOLD:
        return
      try:
        res = self._api.query(q).ways
        self._process_res(res)
        self._local_osm_query_fail_count = 0
      except Exception as e:
        print(f'Exception while querying OSM:\n{e}')
      self._fetch_time_prev = fetch_time

    self._process_tags()

  def _process_tags(self):
    if len(self._way_tags) > 0:
      self._process_road_name_tag()
      self._process_speed_limit_tags()
      # print([self.road_name, self.speed_limit])

  def _process_road_name_tag(self):
    tag_value = self._way_tags.get("name")
    if tag_value is not None:
      self.road_name = str(tag_value).strip()
    else:
      self.road_name = ""

  def _process_speed_limit_tags(self):
    self.speed_limit = self._speed_limit_for_osm_tag_value(self._way_tags.get("maxspeed"))

  # rick - speed limit processor from move-fast
  # https://github.com/move-fast/openpilot/blob/b170d1bc123a0cf2b872050fbd5e2eecd1b23e22/selfdrive/mapd/lib/WayRelation.py#L95
  def _speed_limit_for_osm_tag_value(self, value):
    # https://wiki.openstreetmap.org/wiki/Key:maxspeed
    if value is None:
      # When limit is set to 0. is considered not existing.
      return 0.

    # Attempt to parse limit as simple numeric value considering units.
    limit = self._speed_limit_value_for_value(value)
    if limit is not None:
      return limit

    # Look for matches of speed with country implicit values.
    v = re.match(r'^\s*([A-Z]{2}):([a-z_]+):?([0-9]{1,3})?(\s+)?(mph)?\s*', value)
    if v is None:
      return 0.

    if v[2] == "zone" and v[3] is not None:
      conv = CV.MPH_TO_MS if v[5] is not None and v[5] == "mph" else CV.KPH_TO_MS
      limit = conv * float(v[3])

    return limit if limit is not None else 0.

  def _speed_limit_value_for_value(self, value):
    # Look for matches of speed by default in kph, or in mph when explicitly noted.
    v = re.match(r'^\s*([0-9]{1,3})\s*?(mph)?\s*$', value)
    if v is None:
      return None
    conv = CV.MPH_TO_MS if v[2] is not None and v[2] == "mph" else CV.KPH_TO_MS
    return conv * float(v[1])


class MapD():
  def __init__(self, position_service):
    last_gps_params = Params().get('LastGPSPosition')
    self.last_gps_pos = json.loads(last_gps_params) if last_gps_params is not None else []
    self.osm = OSM(self.last_gps_pos)

    self.position_service = position_service
    self._pause_query = False
    self._longitude = None
    self._latitude = None

    self._way_id_prev = 0
    self._road_name_prev = None
    self._same_road_count = 0

  def apply_last_gps_pos(self):
    # use last gps position before first fixed
    if self.last_gps_pos is not None and len(self.last_gps_pos) > 0:
      self._latitude = self.last_gps_pos["latitude"]
      self._longitude = self.last_gps_pos["longitude"]
      # print("MAPD: Use GPS pos: %s, %s (LastGPSPosition)" % (self._latitude, self._longitude))

  def update_car_state(self, sm):
    sock = 'carState'
    if not sm.updated[sock] or not sm.valid[sock]:
      return

    car_state = sm[sock]
    self._pause_query = car_state.vEgo <= 2.78 # 10km/h

  def update_position(self, sm):
    if self.position_service == "liveLocationKalman":
      self.update_locationd(sm)
    else:
      self.update_gps(sm)

  def update_locationd(self, sm):
    sock = 'liveLocationKalman'
    if not sm.updated[sock] or not sm.valid[sock]:
      return

    location = sm[sock]
    location_valid = (location.status == log.LiveLocationKalman.Status.valid) and location.positionGeodetic.valid

    if not location_valid:
      return

    self._latitude = location.positionGeodetic.value[0]
    self._longitude = location.positionGeodetic.value[1]
    # print("MAPD: Use GPS pos: %s, %s (locationd)" % (self._latitude, self._longitude))

  def update_gps(self, sm):
    sock = 'gpsLocationExternal'
    if not sm.updated[sock] or not sm.valid[sock]:
      return

    log = sm[sock]
    self.last_gps = log

    # ignore the message if the fix is invalid
    if log.flags % 2 == 0:
      return

    self._latitude = log.latitude
    self._longitude = log.longitude
    # print("MAPD: Use GPS pos to: %s, %s (gpsLocationExternal)" % (self._latitude, self._longitude))

  def fetch_osm_data(self):
    if self._pause_query:
      return
    self.osm.fetch_tags_around_location(self._latitude, self._longitude)

  def publish(self, pm):
    if self._pause_query:
      return
    map_data_msg = messaging.new_message('liveMapData')
    map_data_msg.liveMapData.currentRoadName = "" if self.osm.road_name is None else self.osm.road_name

    if (self.osm.road_name is not None and self.osm.road_name == self._road_name_prev) or self.osm.way_id == self._way_id_prev:
      self._same_road_count += 1
    else:
      self._same_road_count = 0
    self._road_name_prev = self.osm.road_name
    self._way_id_prev = self.osm.way_id
    map_data_msg.liveMapData.speedLimitValid = self._same_road_count > 3 and self.osm.speed_limit > 0

    map_data_msg.liveMapData.speedLimit = self.osm.speed_limit

    pm.send('liveMapData', map_data_msg)
    # print(f'MAPD *****: Publish: \n{map_data_msg}\n********')

# provides live map data information
def mapd_thread(sm=None, pm=None):
  use_locationd = False
  position_service = "liveLocationKalman" if use_locationd else "gpsLocationExternal"

  set_core_affinity([1,])
  set_realtime_priority(1)
  mapd = MapD(position_service)
  rk = Ratekeeper(0.5, print_delay_threshold=None)  # Keeps rate at 0.5 hz

  if sm is None:
    sm = messaging.SubMaster(["carState", position_service])
  if pm is None:
    pm = messaging.PubMaster(['liveMapData'])

  mapd.apply_last_gps_pos()
  while True:
    sm.update()
    mapd.update_car_state(sm)
    mapd.update_position(sm)
    mapd.fetch_osm_data()
    mapd.publish(pm)
    rk.keep_time()

def main(sm=None, pm=None):
  mapd_thread(sm, pm)


if __name__ == "__main__":
  main()
