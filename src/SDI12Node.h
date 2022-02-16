/**
 * @file SDI12Node.h
 *
 * @brief This file extends the main class for the SDI-12 implementation to support
 * end node (slave) device compliance.
 *
 * ======================== Attribution & License =============================
 *
 * This extends the work done by Stroud Water Research Center et al.
 * Available at https://github.com/EnviroDIY/Arduino-SDI-12
 *
 */
#ifndef SDI12_NODE_H_
#define SDI12_NODE_H_

#include "SDI12.h"

/* SDI-12 Data Buffer Size Specification */
// The following data buffer sizes does not include CR+LF and CRC
#define SDI12_VALUE_STR_SIZE 9  // Max number of characters for <value> for aDx!
                                // polarity sign + 7 digits + decimal point = 9
#define SDI12_VALUES_STR_SIZE_35 35 // Data string size (LOW) for aM! aMx!
#define SDI12_VALUES_STR_SIZE_75 75 // Data string size (High) for concurrent,
                                    // continuous, high volume ascii measurement

// SDI-12 Timing Specification
#define SDI12NODE_LINE_BREAK_MICROS 12000 // SDI12 "break", 12ms, in microseconds
#define SDI12NODE_LINE_MARK_MICROS 8333  // SDI12 "mark", 8.33ms, in microseconds


class SDI12Node : public SDI12 {
  private:
    /* Static SDI-12 Timing Reference for SDI-12 Node Device */
    static sdi12timer_t _previous_TCNT; // Stores micros on last ISR() execution
    bool waiting_for_break_ = true; // References device waiting for line break
    bool waiting_for_mark_ = true; // References device waiting for line marking

  public:
    using SDI12::SDI12; // Trivial use of same constructors as SDI12
    ~SDI12Node(void); // Deconstructor
    bool LineBreakReceived(void);
    bool LineMarkReceived(void);
    void ClearLineMarkingReceived(void);

  private:
    void receiveISR(void) override;
};

#endif