/**
 * @file SDI12Sensor.h
 *
 * @brief This file contains the main class for SDI-12 sensor implementation.
 *
 * The class provides a general solution for sensor command handling defined by
 * SDI-12 Support Group Specification.
 * https://www.sdi-12.org/specification
 *
 */
#ifndef SDI12_SENSOR_H_
#define SDI12_SENSOR_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

class String; // Forward declaration for WString.h String object

// #define SDI12_DIGITS_MAX 7 // Number of total characters allowed for value string
// #define SDI12_VALUE_MAX 9
// #define SDI12_DATA_SIZE_MAX_DEFAULT 35 // Max data string size of measurement
// #define SDI12_DATA_SIZE_MAX_ALT 75     // Max data string size for concurrent,
// continuous, high ascii

/* Default VALUES */
#define SDI12SENSOR_DEFAULT_ADDR '0' // Default Sensor Address

#ifndef SDI12SENSOR_SDI12_PROTOCOL
#define SDI12SENSOR_SDI12_PROTOCOL "13"  // Respresent v1.3
#endif

#ifndef SDI12SENSOR_COMPANY
#define SDI12SENSOR_COMPANY "COMPNAME"  // 8 Charactors depicting company name
#endif

#ifndef SDI12SENSOR_MODEL
#define SDI12SENSOR_MODEL "000001"  // 6 Characters specifying sensor model
#endif

#ifndef SDI12SENSOR_VERSION
#define SDI12SENSOR_VERSION "1.0"  // 3 characters specifying sensor version
#endif

#ifndef SDI12SENSOR_OTHER_INFO
#define SDI12SENSOR_OTHER_INFO "001"  // (optional) up to 13 char for serial or other sensor info
#endif


// struct Command_s {
//     bool msgBreak;
//     bool msgMark;
//     char address;
//     char action;
//     uint8_t altAction;
//     String additional;
//     uint8_t lastState;
// };

// enum SDI12SlaveCommand_e {
//     UNKNOWN = 0,
//     ADDRESS_QUERY = 1,          // ?!
//     ACKNOWLEDGEMENT = 2,        // a!
//     IDENTIFICATION = 3,         // aI!
//     ADDRESS_CHANGE = 4,         // aAb!
//     INITIATE_MEASUREMENT = 5,   // aM!, aMC!, aM1~9!, aMC1~9!
//     DATA_REQUEST = 6,           // aD0! ~ aD9!, aD0 ~ aD999! for high ascii
//     CONCURRENT_MEASUREMENT = 7, // aC!, aCC!, aC1~9!, aCC1~9!
//     CONTINUOUS_MEASUREMENT = 8, // aR0!, aCC!, aC1~9!, aCC1~9!
//     VERIFICATION = 9,           // aV!
// };

// enum SDI12SlaveState_e {
//     LOW_POWER              = 0,
//     WAIT                   = 1,
//     ACKNOWLEDGEMENT        = 2,
//     ADDRESS_QUERY          = 3,
//     ADDRESS_UPDATE         = 4,
//     MEASUREMENT_SINGLE     = 5,
//     MEASUREMENT_CONCURRENT = 6,
//     MEASUREMENT_CONTINUOUS = 7,
//     SENDING_DATA           = 8
// } SDI12SlaveState_e;


/**
 * @class SDI12Sensor
 * @brief SDI-12 Sensor object
 */
class SDI12Sensor {
  private:
    /* Interal Variables */
    char sensor_address_;  // Reference to the sensor address, defaults is character '0'
    bool active_ = false; // Reference to the active state of current device
    static SDI12Sensor *last_set_active_object_; // Reference to the last set active SDI12Sensor object

  public:
    SDI12Sensor(void);  // constructor - without argument, for better library integration
    explicit SDI12Sensor(const char address);  // Constructor to declare sensor address
    ~SDI12Sensor(void); // Deconstructor
    bool SetAddress(const char address); // Set sensor address
    char Address(void) const; // Get sensor address
    bool SetActive(const bool active = true); // Sets SDI12Sensor object active state
    bool IsActive(void) const; // Return if SDI12Sensor instance active status
    static SDI12Sensor *LastActive(void); // Get current last set active SDI12Sensor object
    static void ClearLastActive(void); // Clears the reference to last set active SDI12Sensor object
    static bool IsSetLastActive(void); // Check if last set active SDI12Sensor instance is set
    //     void SendSensorAddress();
    //     void SendSensorID();
};


/** @cond
 *      Supporting Functions, Not Class Scope Specific
 * @endcond
 */
size_t IntegralLength(double value); // Get integral length of decimal values.
size_t dtoa(double value, char *str, uint8_t prec = 0, uint8_t fit_len = 0,
        bool zero_trail = false, bool pos_sign = true); // Digit to string
size_t dtoa(double value, String &str, uint8_t prec = 0, uint8_t fit_len = 0,
        bool zero_trail = false, bool pos_sign = true); // Digit to string

// void FormatOutputSDI(float *measurementValues, String *dValues, unsigned int
// maxChar);

#endif