#ifdef USES_P082
//#######################################################################################################
//#################### Plugin 082 GPS
//###################################################################
//#######################################################################################################
//
// Read a GPS module connected via (Software)Serial
// Based on the library TinyGPS++
// http://arduiniana.org/libraries/tinygpsplus/
//
//

#include <ESPeasySerial.h>
#include <TinyGPS++.h>

#define PLUGIN_082
#define PLUGIN_ID_082          82
#define PLUGIN_NAME_082       "Position - GPS [TESTING]"
#define PLUGIN_VALUENAME1_082 "Longitude"
#define PLUGIN_VALUENAME2_082 "Latitude"
#define PLUGIN_VALUENAME3_082 "Altitude"
#define PLUGIN_VALUENAME4_082 "Speed"

#define P082_LONGITUDE         UserVar[event->BaseVarIndex + 0]
#define P082_LATITUDE          UserVar[event->BaseVarIndex + 1]
#define P082_ALTITUDE          UserVar[event->BaseVarIndex + 2]
#define P082_SPEED             UserVar[event->BaseVarIndex + 3]

#define P082_DEFAULT_FIX_TIMEOUT 2500
#define P082_TIMESTAMP_AGE       1500

struct P082_data_struct {
  P082_data_struct() {}

  ~P082_data_struct() { reset(); }

  void reset() {
    if (gps != nullptr) {
      delete gps;
    }
    if (P082_easySerial != nullptr) {
      delete P082_easySerial;
    }
  }

  bool init(const int16_t serial_rx, const int16_t serial_tx) {
    if (serial_rx < 0 || serial_tx < 0)
      return false;
    reset();
    gps = new TinyGPSPlus();
    P082_easySerial = new ESPeasySerial(serial_rx, serial_tx);
    P082_easySerial->begin(9600);
    return isInitialized();
  }

  bool isInitialized() const {
    return gps != nullptr && P082_easySerial != nullptr;
  }

  bool loop() {
    if (!isInitialized())
      return false;
    bool fullSentenceReceived = false;
    if (P082_easySerial != nullptr) {
      while (P082_easySerial->available() > 0) {
        if (gps->encode(P082_easySerial->read())) {
          fullSentenceReceived = true;
        }
      }
    }
    return fullSentenceReceived;
  }

  bool hasFix(unsigned int maxAge_msec) {
    if (!isInitialized())
      return false;
    return (gps->location.isValid() && gps->location.age() < maxAge_msec);
  }

  bool storeCurPos(unsigned int maxAge_msec) {
    if (!hasFix(maxAge_msec))
      return false;
    last_lat = gps->location.lat();
    last_lng = gps->location.lng();
  }

  // Return the GPS time stamp, which is in UTC.
  // @param age is the time in msec since the last update of the time + additional centiseconds given by the GPS.
  bool getDateTime(struct tm& dateTime, uint32_t& age, bool& pps_sync) {
    if (!isInitialized())
      return false;
    if (pps_time != 0) {
      age = timePassedSince(pps_time);
      pps_time = 0;
      pps_sync = true;
      if (age > 1000 || gps->time.age() > age)
        return false;
    } else {
      age = gps->time.age();
      pps_sync = false;
    }
    if (age > P082_TIMESTAMP_AGE)
      return false;
    if (gps->date.age() > P082_TIMESTAMP_AGE)
      return false;
    dateTime.tm_year = gps->date.year() - 1970;
    dateTime.tm_mon  = gps->date.month();
    dateTime.tm_mday = gps->date.day();

    dateTime.tm_hour = gps->time.hour();
    dateTime.tm_min  = gps->time.minute();
    dateTime.tm_sec  = gps->time.second();
    // FIXME TD-er: Must the offset in centisecond be added when pps_sync active?
    if (!pps_sync) {
      age += (gps->time.centisecond() * 10);
    }
    return true;
  }

  TinyGPSPlus *gps = nullptr;
  ESPeasySerial *P082_easySerial = nullptr;

  double last_lat = 0.0;
  double last_lng = 0.0;

  unsigned long pps_time = 0;
} P082_data;

boolean Plugin_082(byte function, struct EventStruct *event, String &string) {
  boolean success = false;

  switch (function) {
    case PLUGIN_DEVICE_ADD: {
      Device[++deviceCount].Number = PLUGIN_ID_082;
      Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
      Device[deviceCount].VType = SENSOR_TYPE_QUAD;
      Device[deviceCount].Ports = 0;
      Device[deviceCount].PullUpOption = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption = true;
      Device[deviceCount].ValueCount = 4;
      Device[deviceCount].SendDataOption = true;
      Device[deviceCount].TimerOption = true;
      Device[deviceCount].GlobalSyncOption = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME: {
      string = F(PLUGIN_NAME_082);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES: {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_082));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_082));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_082));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_082));
      break;
    }

    case PLUGIN_GET_DEVICEGPIONAMES: {
      serialHelper_getGpioNames(event, false, true); // TX optional
      event->String3 = formatGpioName_input_optional(F("PPS"));
      break;
    }

    case PLUGIN_WEBFORM_LOAD: {
      serialHelper_webformLoad(event);
  /*
      if (P082_data.isInitialized()) {
        String detectedString = F("Detected: ");
        detectedString += String(P082_data.P082_easySerial->baudRate());
        addUnit(detectedString);
      }
  */

      P082_html_show_stats(event);

      // Settings to add:
      // Speed unit
      // Altitude unit
      // Set system time
      // Timeout in msec to consider still active fix.
      // Update interval: seconds, distance travelled
      // Position filtering
      // Speed filtering
      //
      // What to do with:
      // nr satellites
      // HDOP
      // fixQuality, fixMode
      // statistics (chars processed, failed checksum)
      //
      // Show some statistics on the load page.


      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE: {
      serialHelper_webformSave(event);

      success = true;
      break;
    }

    case PLUGIN_INIT: {
      const int16_t serial_rx = Settings.TaskDevicePin1[event->TaskIndex];
      const int16_t serial_tx = Settings.TaskDevicePin2[event->TaskIndex];
      const int16_t pps_pin   = Settings.TaskDevicePin3[event->TaskIndex];
      if (P082_data.init(serial_rx, serial_tx)) {
        success = true;
        if (loglevelActiveFor(LOG_LEVEL_INFO)) {
          String log = F("GPS  : Init OK  ESP GPIO-pin RX:");
          log += serial_rx;
          log += F(" TX:");
          log += serial_tx;
          addLog(LOG_LEVEL_INFO, log);
        }
        if (pps_pin != -1) {
//          pinMode(pps_pin, INPUT_PULLUP);
          attachInterrupt(pps_pin, Plugin_082_interrupt, RISING);
        }
      }
      break;
    }

    case PLUGIN_EXIT: {
      P082_data.reset();
      const int16_t pps_pin   = Settings.TaskDevicePin3[event->TaskIndex];
      if (pps_pin != -1) {
        detachInterrupt(pps_pin);
      }
      success = true;
      break;
    }

    case PLUGIN_TEN_PER_SECOND: {
      if (P082_data.loop()) {
        schedule_task_device_timer(event->TaskIndex, millis() + 10);
        delay(0); // Processing a full sentence may take a while, run some background tasks.
      }
      success = true;
      break;
    }

    case PLUGIN_READ: {
      if (P082_data.isInitialized()) {
        static bool activeFix = P082_data.hasFix(P082_DEFAULT_FIX_TIMEOUT);
        const bool curFixStatus = P082_data.hasFix(P082_DEFAULT_FIX_TIMEOUT);
        if (activeFix != curFixStatus) {
          // Fix status changed, send events.
          String event = curFixStatus ? F("GPS#GotFix") : F("GPS#LostFix");
          rulesProcessing(event);
          activeFix = curFixStatus;
        }

        if (P082_data.hasFix(P082_DEFAULT_FIX_TIMEOUT)) {
          if (P082_data.gps->location.isUpdated()) {
            P082_LONGITUDE = P082_data.gps->location.lng();
            P082_LATITUDE = P082_data.gps->location.lat();
            success = true;
            addLog(LOG_LEVEL_INFO, F("GPS: Position update."));
          }
          if (P082_data.gps->altitude.isUpdated()) {
            // ToDo make unit selectable
            P082_ALTITUDE = P082_data.gps->altitude.meters();
            success = true;
            addLog(LOG_LEVEL_INFO, F("GPS: Altitude update."));
          }
          if (P082_data.gps->speed.isUpdated()) {
            // ToDo make unit selectable
            P082_SPEED = P082_data.gps->speed.mps();
            addLog(LOG_LEVEL_INFO, F("GPS: Speed update."));
            success = true;
          }
        }
        P082_setSystemTime();
        P082_logStats(event);
      }
      break;
    }
  }
  return success;
}

void P082_logStats(struct EventStruct *event) {
  if (!P082_data.isInitialized())
    return;
  if (!loglevelActiveFor(LOG_LEVEL_INFO))
    return;

  String log;
  log.reserve(96);
  log = F("GPS:");
  log += F(" Fix: ");
  log += String(P082_data.hasFix(P082_DEFAULT_FIX_TIMEOUT));
  log += F(" Long: ");
  log += P082_LONGITUDE;
  log += F(" Lat: ");
  log += P082_LATITUDE;
  log += F(" Alt: ");
  log += P082_ALTITUDE;
  log += F(" Spd: ");
  log += P082_SPEED;

  log += F(" Chksum(pass/fail): ");
  log += P082_data.gps->passedChecksum();
  log += '/';
  log += P082_data.gps->failedChecksum();

  addLog(LOG_LEVEL_INFO, log);
}

void P082_html_show_stats(struct EventStruct *event) {
  addRowLabel(F("Fix"));
  addHtml(String(P082_data.hasFix(P082_DEFAULT_FIX_TIMEOUT)));

  addRowLabel(F("Nr Satellites"));
  addHtml(String(P082_data.gps->satellites.value()));

  addRowLabel(F("HDOP"));
  addHtml(String(P082_data.gps->hdop.value() / 100.0));

  addRowLabel(F("UTC Time"));
  struct tm dateTime;
  uint32_t age;
  bool pps_sync;
  if (P082_data.getDateTime(dateTime, age, pps_sync)) {
    dateTime = addSeconds(dateTime, (age / 1000), false);
  }
  addHtml(getDateTimeString(dateTime));

  addRowLabel(F("Checksum (pass/fail)"));
  String chksumStats;
  chksumStats = P082_data.gps->passedChecksum();
  chksumStats += '/';
  chksumStats += P082_data.gps->failedChecksum();
  addHtml(chksumStats);
}

void P082_setSystemTime() {
  // Set the externalTimesource 10 seconds earlier to make sure no call is made to NTP (if set)
  if (nextSyncTime > (sysTime + 10)) {
    return;
  }

  struct tm dateTime;
  uint32_t age;
  bool pps_sync;
  if (P082_data.getDateTime(dateTime, age, pps_sync)) {
    // Use floating point precision to use the time since last update from GPS and the given offset in centisecond.
    externalTimeSource = makeTime(dateTime);
    externalTimeSource += static_cast<double>(age) / 1000.0;
    initTime();
  }
}

void Plugin_082_interrupt()
/*********************************************************************/
{
  P082_data.pps_time = millis();
}


#endif // USES_P082
