import overpy
import subprocess
import numpy as np
from openpilot.selfdrive.mapd.lib.geo import R
import os
from openpilot.selfdrive.mapd.config import QUERY_RADIUS

OSM_QUERY = ["/data/media/0/osm/bin/osm3s_query", "--db-dir=/data/media/0/osm/db/"]

def create_way(way_id, node_ids, from_way):
  """
  Creates and OSM Way with the given `way_id` and list of `node_ids`, copying attributes and tags from `from_way`
  """
  return overpy.Way(way_id, node_ids=node_ids, attributes={}, result=from_way._result,
                    tags=from_way.tags)


class OSM():
  def __init__(self, last_gps_pos):
    self.api = overpy.Overpass()
    # self.api = overpy.Overpass(url='http://3.65.170.21/api/interpreter')

    self.local_osm_query_fail_count = 0
    self.ways = []

    self.last_gps_pos = last_gps_pos

    if os.path.isdir("/data/media/0/osm/bin/") and os.path.isdir("/data/media/0/osm/db/"):
      self.local_osm_enabled = True
      print("Local OSM installed")
      try:
        print("Testing local OSM...")
        if len(last_gps_pos) > 0:
          ways = self.fetch_road_ways_around_location(last_gps_pos['latitude'], last_gps_pos['longitude'], QUERY_RADIUS)
          self.local_osm_enabled = len(ways) > 0
          if self.local_osm_enabled:
            self.ways = ways

      except Exception as e:
        self.local_osm_enabled = False
        print(f"Local OSM test failed:\n{e}")
    else:
      self.local_osm_enabled = False
    print("Local OSM enabled = %s" % self.local_osm_enabled)

  def fetch_road_ways_around_location(self, lat, lon, radius):
    # when lat/lon is same as last_gps_pos lat, lon, we can just use cached way data
    # this only happened when we are using lastGPSPosition params
    if len(self.ways) > 0 and len(self.last_gps_pos) > 0 and self.last_gps_pos["latitude"] == lat and self.last_gps_pos["longitude"] == lon:
      return self.ways
    # Calculate the bounding box coordinates for the bbox containing the circle around location.
    bbox_angle = np.degrees(radius / R)
    # fetch all ways and nodes on this ways in bbox
    bbox_str = f'{str(lat - bbox_angle)},{str(lon - bbox_angle)},{str(lat + bbox_angle)},{str(lon + bbox_angle)}'
    q = """
        way(""" + bbox_str + """)
          [highway]
          [highway!~"^(footway|path|corridor|bridleway|steps|cycleway|construction|bus_guideway|escape|service|track)$"];
        (._;>;);
        out;
        """
    if self.local_osm_enabled:
      try:
        completion = subprocess.run(OSM_QUERY + [f"--request={q}"], check=True, capture_output=True)
        self.ways = self.api.parse_xml(completion.stdout).ways

      except Exception as e:
        self.local_osm_query_fail_count += 1
        print(f'Exception while querying local OSM:\n{e}')
        pass

    # use remote OSM when local osm is not enabled or failed too many times
    if not self.local_osm_enabled or self.local_osm_query_fail_count >= 5:
      try:
        self.ways = self.api.query(q).ways
        self.local_osm_query_fail_count = 0
      except Exception as e:
        print(f'Exception while querying OSM:\n{e}')

    return self.ways
