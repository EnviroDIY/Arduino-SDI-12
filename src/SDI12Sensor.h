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

/* ERROR MACRO */
#define SDI12SENSOR_ERR_INVALID -1 // General INVALID/ERROR/DISABLE


/**
 * @brief Enumerated type to reference supported SDI12 commands.
 *
 */
typedef enum SDI12SensorCommand_e: uint8_t {
    kUnknown               = 0,  // Debug Purposes
    kAcknowledge           = 1,  // a!
    kAddressQuery          = 2,  // ?!
    kIdentification        = 3,  // aI!, aI(M|C|V|HA|HB)(C|"")(0-9)!
    kAddressChange         = 4,  // aAb!
    kMeasurement           = 5,  // aM!, aMC!, aM1~9!, aMC1~9!
    kDataRequest           = 6,  // aD0~9!, aD0~999! for high ascii
    kConcurrentMeasurement = 7,  // aC!, aCC!, aC1~9!, aCC1~9!
    kContinuousMeasurement = 8,  // aR0~9!, aRC0~9!
    kVerification          = 9,  // aV!, aVC!
    kHighVolumeASCII       = 10, // aHA!, aHAC! high volume ascii
    kHighVolumeByte        = 11, // aHB!, aHBC! high volume byte
    kByteDataRequest       = 12  // aDB0~999! for high volume byte
} SDI12SensorCommand_e;

/**
 * @brief References SDI12 command structure.
 *
 */
typedef struct SDI12CommandSet_s {
    char address = '\0'; // Address received
    int8_t primary = kUnknown; // Primary command received
    int8_t secondary = kUnknown; // Secondary command received
    int8_t param1 = SDI12SENSOR_ERR_INVALID; // Storage of primary command parameter i.e aM1~9 or new address aAb!
    int16_t param2 = SDI12SENSOR_ERR_INVALID; // Storage of secondary command parameter, i.e Identify meta group aI_001~999!
    bool crc_requested = false; //
} SDI12CommandSet_s;

/**
 * @brief Enumerated type to reference the different sensor operational state
 *
 */
typedef enum SDI12SensorState_e : uint8_t {
    kStateLowPower        = 0,  // Perform low power operations
    kStateReady           = 1,  // General non measurement state, includes identify meta requests
    kStateVerify          = 2,  // When received aV! or aIV!
    kStateMeasurement     = 3,  // When received aM! or aIM!
    kStateConcurrent      = 4,  // When received aC! or aIC!
    kStateContinuous      = 5,  // When received aRx!
    kStateHighMeasurement = 6   // When received aHA! or aIHA! or aHB! or aIHB!
} SDI12SensorState_e;


/**
 * @class SDI12Sensor
 * @brief SDI-12 Sensor object
 */
class SDI12Sensor {
  public:
  int8_t state_ = kStateReady;

  private:
    /* Interal Variables */
    char sensor_address_;  // Reference to the sensor address, defaults is character '0'
    bool crc_requested_ = false; // References if a CRC is required for aDx! commands
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
    void SetCrcRequest(const bool crc_request);
    bool CrcRequested(void) const;
    static const SDI12CommandSet_s ParseCommand(const char* received, const char ack_address = '\0');
    bool DefineState(const SDI12CommandSet_s command_set);
    bool SetState(const int8_t state);
    int8_t State(void) const;
    //     void SendSensorAddress();
    //     void SendSensorID();

  protected:
    static SDI12SensorCommand_e ReadCommand(const char* received);
    static bool RuleIsAddressChange(const SDI12SensorCommand_e cmd, const int param1, const bool is_end);
    static bool RuleIsMeasurement(const SDI12SensorCommand_e cmd, int *param1, const bool is_end);
    static bool RuleIsConcurrent(const SDI12SensorCommand_e cmd, int *param1, const bool is_end);
    static bool RuleIsContinous(const SDI12SensorCommand_e cmd, const int param1);
    static bool RuleIsDataRequest(const SDI12SensorCommand_e cmd, const int param1);
    static bool RuleIsVerify(const SDI12SensorCommand_e cmd, const int param1, const bool is_end);
    static bool RuleIsHighVolumeMeasure(const SDI12SensorCommand_e cmd, const int param1, const bool is_end);
    static bool RuleIsIdentifyGroup(const SDI12SensorCommand_e cmd1, const SDI12SensorCommand_e cmd2,
            int *param1, const int param2, const bool is_end);
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