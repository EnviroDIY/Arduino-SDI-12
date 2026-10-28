/**
 * @file SDI12Recorder.h
 * @copyright Stroud Water Research Center
 * @license This library is published under the BSD-3 license.
 *
 * @brief High-level SDI-12 recorder (master) implementation following Appendix B of
 * the SDI-12 specification v1.4.
 *
 * This class implements the recorder (data logger) side of the SDI-12 protocol as a
 * transaction executor with guard-condition-based phase advancement, rather than as a
 * finite state machine. It manages command transmission, response collection, retry
 * logic, and validation according to the specification's timing and sequencing rules.
 */

#ifndef SRC_SDI12RECORDER_H_
#define SRC_SDI12RECORDER_H_

#include <Arduino.h>
#include <inttypes.h>
#include "SDI12Config.h"
#include "SDI12.h"
#include "SDI12Protocol.h"

/**
 * @brief High-level SDI-12 recorder transaction executor.
 *
 * A transaction is a complete request-response exchange with a single sensor,
 * including wake-up, command transmission, response collection, validation, and
 * automatic retries according to the SDI-12 specification.
 *
 * Usage:
 * @code
 * SDI12Recorder recorder(mySDI12);
 *
 * void setup() {
 *   mySDI12.begin(dataPin);
 * }
 *
 * void loop() {
 *   if (!recorder.isBusy()) {
 *     recorder.startCommand('0', "I");
 *     // ^^ Query sensor '0' for ID, wait up to 3s
 *   }
 *   recorder.update();  // Advance state machine each loop iteration
 *   if (recorder.isDone()) {
 *     if (recorder.result() == SDI12Recorder::RESULT_SUCCESS) {
 *       Serial.println(recorder.response());
 *     } else {
 *       Serial.print("Error: ");
 *       Serial.println(recorder.resultDescription());
 *     }
 *   }
 * }
 * @endcode
 */
class SDI12Recorder {
 public:
  /**
   * @brief Terminal result of a transaction.
   *
   * Each result code corresponds to a specific terminal condition. The recorder
   * publishes these results to allow the application to decide on retry strategy
   * (e.g., try a different sensor, abandon this measurement, etc.).
   */
  typedef enum RecorderResult : uint8_t {
    /// Transaction is in progress or has not been started
    RESULT_NONE = 0x00,
    /// Transaction completed successfully; response is valid and available
    RESULT_SUCCESS = 0x01,
    /// No response was received within the maximum allowed time
    RESULT_NO_RESPONSE = 0x02,
    /// Response arrived but had invalid framing or parity
    RESULT_FRAMING_ERROR = 0x03,
    /// Response address does not match the command address
    RESULT_WRONG_ADDRESS = 0x04,
    /// CRC validation failed (if CRC was requested)
    RESULT_CRC_ERROR = 0x05,
    /// Response buffer overflowed before response completed
    RESULT_BUFFER_OVERFLOW = 0x06,
    /// Response-specific format validation failed
    RESULT_MALFORMED_RESPONSE = 0x07,
    /// Response deadline (command-specific max time) was exceeded
    RESULT_RESPONSE_TIMEOUT = 0x08,
    /// Overall transaction deadline was exceeded (retries not attempted)
    RESULT_TRANSACTION_TIMEOUT = 0x09,
    /// Transaction was cancelled by the application
    RESULT_CANCELLED = 0x0A,
    /// The supplied sensor address is not valid for the requested operation
    RESULT_INVALID_ADDRESS = 0x0B,
    /// A command-specific argument is outside its permitted range
    RESULT_INVALID_COMMAND_ARGUMENT = 0x0C,
    /// A new operation was rejected because another operation is active
    RESULT_BUSY = 0x0D,
    /// No tracking slot is available for a new pending sensor transaction
    RESULT_CONTEXT_CAPACITY = 0x0E,
    /// The requested operation requires pending measurement context
    RESULT_CONTEXT_REQUIRED = 0x0F,
    /// The caller-provided data buffer cannot hold all advertised values
    RESULT_DATA_BUFFER_TOO_SMALL = 0x10,
    /// The sensor reported that its pending measurement was aborted
    RESULT_MEASUREMENT_ABORTED = 0x11,
    /// The advertised value count was not received by the end of D9
    RESULT_INCOMPLETE_DATA = 0x12,
    /// All protocol-required retry attempts were exhausted
    RESULT_RETRIES_EXHAUSTED = 0x13
  } RecorderResult;

 private:
  /**
   * @brief Internal phase of the transaction executor.
   *
   * These phases follow the recorder flowchart in SDI-12 Appendix B, but are
   * organized as a procedural sequence with guard conditions rather than as a
   * traditional finite state machine.
   */
  typedef enum Phase : uint8_t {
    /// No transaction is active
    PHASE_IDLE = 0x00,
    /// Transmitting the wake-up break (spacing)
    PHASE_BREAK = 0x01,
    /// Waiting for pre-command marking minimum
    PHASE_PRECOMMAND_MARK = 0x02,
    /// Transmitting the command
    PHASE_TRANSMITTING = 0x03,
    /// Waiting for response start bit
    PHASE_WAITING_RESPONSE = 0x04,
    /// Receiving response characters
    PHASE_RECEIVING = 0x05,
    /// Validating response format and content
    PHASE_VALIDATING = 0x06,
    /// Determining whether to retry or terminate
    PHASE_DECIDING_RETRY = 0x07
  } Phase;

  /** @brief Typed identity of the active recorder operation. */
  typedef enum Operation : uint8_t {
    OPERATION_NONE = 0x00,
    OPERATION_ADDRESS_QUERY,
    OPERATION_ACKNOWLEDGEMENT,
    OPERATION_IDENTIFICATION,
    OPERATION_CHANGE_ADDRESS,
    OPERATION_MEASUREMENT,
    OPERATION_CONCURRENT_MEASUREMENT,
    OPERATION_DATA,
    OPERATION_RAW_COMMAND
  } Operation;

 public:
  /**
   * @brief Construct a recorder bound to an SDI12 transport instance.
   *
   * @param sdi12 Reference to the SDI12 object that owns the data line.
   */
  explicit SDI12Recorder(SDI12& sdi12);

  /**
   * @brief Destructor.
   */
  ~SDI12Recorder();

  /**
   * @brief Begin a new transaction.
   *
   * Initiates a command-response transaction with a sensor. The transaction will
   * proceed asynchronously through wake-up, transmission, response collection,
   * validation, and retry logic until isDone() returns true.
   *
   * @param address The SDI-12 address (0-9, a-z, A-Z) of the sensor to query.
   * @param command The command body to transmit, without the address or terminating
   *                exclamation point. For example, "I" becomes "[address]I!" on the
   *                wire.
   *
   * @note If a transaction is already in progress, the request is rejected with
   *       RESULT_BUSY and the active transaction continues unchanged.
   * @note Protocol policy such as break selection, response format, CRC validation,
   *       and deadlines is owned by the recorder. Typed operations will supply their
   *       identity directly instead of recovering it by parsing this command string.
   */
  void startCommand(char address, const String& command);

  /**
   * @brief Advance the transaction state machine.
   *
   * Call this method repeatedly (typically in loop()) to drive the transaction
   * forward through its phases: break, mark, transmit, wait, receive, validate,
   * and retry-decision phases. The method performs all work for the current phase
   * and may move to the next phase if guard conditions are satisfied.
   *
   * The method returns immediately if no work is needed in the current phase
   * (e.g., waiting for a deadline). It is safe to call update() frequently
   * without penalty.
   *
   * @note This is a non-blocking, asynchronous method. It does not wait for
   *       response or retry timers; instead, it checks elapsed time and returns.
   */
  void update();

  /**
   * @brief Check if a transaction is currently active.
   *
   * @return true if a transaction has been started with startCommand() and has not
   * yet completed or been cancelled.
   */
  bool isBusy() const {
    return _phase != PHASE_IDLE;
  }

  /**
   * @brief Check if the current transaction has completed.
   *
   * @return true if the transaction has reached a terminal state (success or
   *         failure) and result() may be queried.
   */
  bool isDone() const {
    return _result != RESULT_NONE && !isBusy();
  }

  /**
   * @brief Retrieve the terminal result of the completed transaction.
   *
   * @return The result code. Returns RESULT_BUSY immediately after an operation was
   *         rejected while another transaction is active. Otherwise returns
   *         RESULT_NONE while the transaction is in progress.
   *
   * @see RecorderResult
   */
  RecorderResult result() const {
    return _startResult == RESULT_NONE ? _result : _startResult;
  }

  /**
   * @brief Retrieve a human-readable description of the result.
   *
   * @return A string describing the terminal result (e.g., "SUCCESS", "NO_RESPONSE").
   *         Returns "BUSY" after a rejected start request, or an empty string if the
   *         transaction is still in progress and no start request was rejected.
   */
  const char* resultDescription() const;

  /**
   * @brief Retrieve the collected response, if successful.
   *
   * @return The null-terminated response payload. Protocol envelope characters that
   *         have already been validated, including the address, CRC, and CR+LF
   *         terminator, are excluded.
   *
   * @note This is only valid if result() == RESULT_SUCCESS.
   * @note The returned pointer remains valid until the response is cleared, another
   *       command is started, or this recorder is destroyed.
   */
  const char* response() const {
    return _response;
  }

  /**
   * @brief Retrieve the number of payload characters in response().
   *
   * @return The response payload length, excluding the null terminator.
   */
  size_t responseLength() const {
    return _responseLength;
  }

  /**
   * @brief Copy the response payload into an application-owned buffer.
   *
   * Copies at most @p capacity minus one characters and null-terminates the
   * destination whenever @p destination is not null and @p capacity is greater than
   * zero. Compare the return value with @p capacity to detect truncation.
   *
   * @param destination Buffer that will receive the response payload. May be null only
   *                    when @p capacity is zero.
   * @param capacity Total destination capacity, including its null terminator.
   * @return The complete response payload length, excluding the null terminator.
   *
   * @note The response is only valid if result() == RESULT_SUCCESS.
   */
  size_t copyResponse(char* destination, size_t capacity) const;

  /**
   * @brief Cancel the current transaction and return to idle.
   *
   * Stops all retry logic and resets the transaction executor to PHASE_IDLE.
   * The result will be set to RESULT_CANCELLED.
   */
  void cancel();

  /**
   * @brief Clear the collected response payload.
   *
   * This does not change the current transaction phase or SDI-12 line state.
   */
  void clearResponse() {
    _responseLength = 0;
    _response[0]    = '\0';
  }

 private:
  // References and configuration
  SDI12&    _sdi12;         ///< The SDI12 transport instance
  Operation _operation;     ///< Identity supplied directly by the starting API
  char      _address;       ///< Target sensor address
  String    _command;       ///< Command to send (without address or '!')
  uint32_t  _txnStartTime;  ///< millis() at transaction start
  uint32_t  _txnTimeoutMs;  ///< Overall transaction deadline

  /// Conservative transaction guard until command-derived deadlines are implemented.
  static const uint32_t kDefaultTransactionTimeoutMs = 3000U;

  // Current phase and phase timings
  Phase    _phase;
  bool     _phaseStarted;     ///< Whether the current phase action has started
  uint32_t _phaseStartTime;   ///< micros() at phase start
  uint32_t _commandStopTime;  ///< micros() when last command stop bit ended
  uint32_t _breakEndTime;     ///< micros() when break ended

  // Attempt tracking (retry logic)
  uint8_t _breakCount;    ///< Number of breaks issued (0-3)
  uint8_t _attemptCount;  ///< No-break retries in current break sequence (0-3)

  // Response collection
  char           _response[SDI12_RECORDER_RESPONSE_BUFFER_SIZE];
  uint8_t        _responseLength;
  RecorderResult _result;
  RecorderResult _startResult;  ///< Immediate rejection of the latest start request

  // Private helper methods
  bool _shouldTransmitBreak() const;
  void _transitionTo(Phase nextPhase);
  void _setResult(RecorderResult reason);

  // Phase implementations

  /// @brief Sends a break signal to the sensor.
  /// This blocks while the break signal is being transmitted, but does not disable any
  /// interrupts
  void _phase_break();
  /// @brief Send a pre-command mark to the sensor.
  /// This blocks while the pre-command mark is being transmitted, but does not disable
  /// any interrupts
  void _phase_precommand_mark();
  /// @brief Handle the transmitting phase of the transaction.
  /// This blocks while the command is being transmitted.  All interrupts are disabled
  /// while each charater is transmitted for processors under 48MHz.
  void _phase_transmitting();
  /// @brief Handle the waiting for response phase of the transaction.
  void _phase_waiting_response();
  /// @brief Handle the receiving phase of the transaction.
  void _phase_receiving();
  /// @brief Handle the validating phase of the transaction.
  void _phase_validating();
  /// @brief Handle the deciding retry phase of the transaction.
  void _phase_deciding_retry();
};

#endif  // SRC_SDI12RECORDER_H_
