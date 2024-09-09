from openpilot.common.numpy_fast import interp

DP_ACCEL_STOCK = 0
DP_ACCEL_ECO = 1
DP_ACCEL_NORMAL = 2
DP_ACCEL_SPORT = 3

# accel profile by @arne182 modified by cgw
_DP_CRUISE_MIN_V =       [-0.765, -0.765,  -0.80, -0.80, -0.75, -0.70]
_DP_CRUISE_MIN_V_ECO =   [-0.760, -0.760,  -0.76, -0.76, -0.70, -0.65]
_DP_CRUISE_MIN_V_SPORT = [-0.770, -0.770,  -0.90, -1.00, -0.90, -0.80]
_DP_CRUISE_MIN_BP =      [0.,     15.66,  17.88, 20.,   30.,   55.]
#DP_CRUISE_MIN_BP in mph=[0.,     18,     35,    40,    45,    67,    123]

_DP_CRUISE_MAX_V =       [3.4, 2.8, 1.8, 1.4, 1.06, .88, .68,  .46, .35, .13]
_DP_CRUISE_MAX_V_ECO =   [3.2, 2.6, 1.6, 1.2, .76,  .62, .48,  .36, .28, .09]
_DP_CRUISE_MAX_V_SPORT = [3.5, 3.0, 2.4, 2.9, 2.1,  1.7,  1.3, .9,  .7,  .5]
_DP_CRUISE_MAX_BP =      [0.,  3,   6.,  8.,  11.,  15.,  20., 25., 30., 55.]
#DP_CRUISE_MAX_BP in mph=[0.,  6.7, 13,  18,  25,   33,   45,  56,  67,  123]


class AccelController:

  def __init__(self):
    # self._params = Params()
    self._profile = DP_ACCEL_STOCK

  def set_profile(self, profile):
    try:
      self._profile = int(profile) if int(profile) in [DP_ACCEL_STOCK, DP_ACCEL_ECO, DP_ACCEL_NORMAL, DP_ACCEL_SPORT] else DP_ACCEL_STOCK
    except:
      self._profile = DP_ACCEL_STOCK

  def _dp_calc_cruise_accel_limits(self, v_ego):
    if self._profile == DP_ACCEL_ECO:
      min_v = _DP_CRUISE_MIN_V_ECO
      max_v = _DP_CRUISE_MAX_V_ECO
    elif self._profile == DP_ACCEL_SPORT:
      min_v = _DP_CRUISE_MIN_V_SPORT
      max_v = _DP_CRUISE_MAX_V_SPORT
    else:
      min_v = _DP_CRUISE_MIN_V
      max_v = _DP_CRUISE_MAX_V

    a_cruise_min = interp(v_ego, _DP_CRUISE_MIN_BP, min_v)
    a_cruise_max = interp(v_ego, _DP_CRUISE_MAX_BP, max_v)
    return a_cruise_min, a_cruise_max

  def get_accel_limits(self, v_ego, accel_limits):
    return accel_limits if self._profile == DP_ACCEL_STOCK else self._dp_calc_cruise_accel_limits(v_ego)

  def is_enabled(self):
    return self._profile != DP_ACCEL_STOCK
