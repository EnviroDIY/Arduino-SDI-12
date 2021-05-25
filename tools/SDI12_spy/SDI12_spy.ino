/**
 * @file h_SDI-12_slave_implementation.ino
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 * @date 2016
 * @author D. Wasielewski
 *
 * @brief Example H:  Using SDI-12 in Slave Mode
 *
 * Example sketch demonstrating how to implement an arduino as a slave on an SDI-12 bus.
 * This may be used, for example, as a middleman between an I2C sensor and an SDI-12
 * datalogger.
 *
 * Note that an SDI-12 slave must respond to M! or C! with the number of values it will
 * report and the max time until these values will be available.  This example uses 9
 * values available in 21 s, but references to these numbers and the output array size
 * and datatype should be changed for your specific application.
 *
 * D. Wasielewski, 2016
 * Builds upon work started by:
 * https://github.com/jrzondagh/AgriApps-SDI-12-Arduino-Sensor
 * https://github.com/Jorge-Mendes/Agro-Shield/tree/master/SDI-12ArduinoSensor
 *
 * Suggested improvements:
 *  - Get away from memory-hungry arduino String objects in favor of char buffers
 *  - Make an int variable for the "number of values to report" instead of the
 *    hard-coded 9s interspersed throughout the code
 */

#include <SDI12.h>

#define DATA_PIN 7 /*!< The pin of the SDI-12 data bus */

// Create object by which to communicate with the SDI-12 bus on SDIPIN
SDI12 slaveSDI12(DATA_PIN);

void setup() {
  Serial.begin(115200);
  slaveSDI12.begin();
  delay(500);
  slaveSDI12.forceListen();  // sets SDIPIN as input to prepare for incoming message
  Serial.println("Starting SDI-12 Spy");
}

void loop() {
  while (slaveSDI12.available()) {
    int readChar = slaveSDI12.read();
    Serial.write(readChar);
    // if (readChar == '\n') {
    //   slaveSDI12.forceListen();
    // } else {
    //   delay(10);// 1 character ~ 7.5ms
    // }
  }
}
