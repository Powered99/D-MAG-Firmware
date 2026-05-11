#include "Config.hpp"

namespace fgm{
    volatile SENSOR_MODE SENSOR_MODES[SENSOR_CH_COUNT] = {SENSOR_MODE::DISABLED};
    CALIB_DATA SENSOR_CALIBRATIONS[SENSOR_CH_COUNT]; // Sensor calibration data (MIN, MAX values) // TODO: load/save from non-volatile memory
    volatile uint SAMPLE_COUNT[SENSOR_CH_COUNT]; // Actual sample count used by the driver
    volatile uint MEDIAN_SAMPLE_OFFSET[SENSOR_CH_COUNT]; // Offset of median samples (MEDIAN_SAMPLE_OFFSET <- center -> MEDIAN_SAMPLE_OFFSET), total median samples: 2x offset
}
namespace logger{
    uint32_t log_interval_ms = 1000;
    FORMATS DATA_FORMAT = FORMATS::IAGA2002;
}