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
 *   SDI12Recorder::Identification info;
 *   if (recorder.identify('0', info)) {
 *     Serial.println(info.vendorID);
 *   } else {
 *     Serial.println(recorder.resultDescription());
 *   }
 * }
 *
 * void loop() {}
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

  /** @brief Parsed response to an SDI-12 identification command. */
  struct Identification {
    /** Sensor address that returned the identification response. */
    char address;
    /** Two-character SDI-12 compatibility version, followed by a null terminator. */
    char sdi12Version[3];
    /** Eight-character vendor identification, followed by a null terminator. */
    char vendorID[9];
    /** Six-character sensor model, followed by a null terminator. */
    char sensorModel[7];
    /** Three-character sensor version, followed by a null terminator. */
    char sensorVersion[4];
    /** Optional sensor information of up to 13 characters, null-terminated. */
    char sensorInfo[14];
  };

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
   * @brief Query the address of the only sensor attached to the bus, blocking until
   * completion.
   *
   * Sends `?!` and accepts exactly one valid sensor address followed by CR/LF.
   *
   * @return The sensor address, or the null character on failure. Use result() for the
   *         detailed terminal reason.
   */
  char queryAddress();

  /**
   * @brief Request acknowledgement from one sensor, blocking until completion.
   *
   * Sends `a!` and requires the addressed sensor to return exactly `a` followed by
   * CR/LF.
   *
   * @param address Sensor address from `0`-`9`, `a`-`z`, or `A`-`Z`.
   * @return true only when the correctly addressed acknowledgement is validated.
   */
  bool requestAcknowledgement(char address);

  /**
   * @brief Request and parse identification information, blocking until completion.
   *
   * Sends `aI!` and validates the fixed identification fields plus the optional
   * 0-13 character sensor-information field.
   *
   * @param address Sensor address from `0`-`9`, `a`-`z`, or `A`-`Z`.
   * @param identification Destination updated only after a validated response.
   * @return true when @p identification was updated; otherwise false.
   */
  bool identify(char address, Identification& identification);

  /**
   * @brief Change a sensor address, blocking until completion.
   *
   * Sends `aAb!` and requires the sensor to acknowledge with the new address followed
   * by CR/LF.
   *
   * @param oldAddress Current sensor address.
   * @param newAddress New sensor address.
   * @return true only when the new address is acknowledged.
   */
  bool changeAddress(char oldAddress, char newAddress);

  /**
   * @brief Start a non-blocking address query.
   *
   * Call update() until isOperationComplete() is true, then read queriedAddress()
   * after a successful result.
   */
  void startQueryAddress();

  /**
   * @brief Start a non-blocking acknowledgement request.
   *
   * @param address Sensor address from `0`-`9`, `a`-`z`, or `A`-`Z`.
   */
  void startRequestAcknowledgement(char address);

  /**
   * @brief Start a non-blocking identification request.
   *
   * Call update() until isOperationComplete() is true, then read identification()
   * after a successful result.
   *
   * @param address Sensor address from `0`-`9`, `a`-`z`, or `A`-`Z`.
   */
  void startIdentify(char address);

  /**
   * @brief Start a non-blocking address-change request.
   *
   * @param oldAddress Current sensor address.
   * @param newAddress New sensor address.
   */
  void startChangeAddress(char oldAddress, char newAddress);

  /**
   * @brief Begin a new transaction.
   *
   * Initiates a command-response transaction with a sensor. The transaction will
   * proceed asynchronously through wake-up, transmission, response collection,
   * validation, and retry logic until isOperationComplete() returns true.
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
   * @return true if an operation has been started and has not yet completed or been
   *         cancelled.
   */
  bool isBusy() const {
    return _phase != PHASE_IDLE;
  }

  /**
   * @brief Check if the current executor operation has completed.
   *
   * @return true if the active operation has reached a terminal state (success or
   *         failure) and result() may be queried.
   *
   * @note This reports executor completion only. Future concurrent-measurement
   *       readiness is retained and queried separately for each sensor address.
   */
  bool isOperationComplete() const {
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
   * @brief Print a human-readable description of the current recorder result.
   *
   * The text describes the same RecorderResult returned by result(). No trailing
   * newline is printed.
   *
   * @param printStream Stream that will receive the description.
   */
  void printRecorderResult(Stream& printStream) const;

  /**
   * @brief Print a human-readable description of the current executor phase.
   *
   * No trailing newline is printed.
   *
   * @param printStream Stream that will receive the description.
   */
  void printPhase(Stream& printStream) const;

  /**
   * @brief Print parsed sensor identification fields with descriptive labels.
   *
   * A trailing newline is printed after the sensor-information field.
   *
   * @param identification Parsed identification information to print.
   * @param printStream Stream that will receive the identification information.
   */
  static void printSensorInfo(const Identification& identification,
                              Stream&               printStream);

  /**
   * @brief Retrieve the address returned by a successful address query.
   *
   * @return The queried sensor address, or the null character when no successful
   *         address query is available.
   */
  char queriedAddress() const {
    return _queriedAddress;
  }

  /**
   * @brief Retrieve the address returned by a successful acknowledgement.
   *
   * This accessor applies to requestAcknowledgement() and changeAddress().
   *
   * @return The acknowledged address, or the null character when unavailable.
   */
  char acknowledgedAddress() const {
    return _acknowledgedAddress;
  }

  /**
   * @brief Retrieve the most recently parsed identification fields.
   *
   * @return Recorder-owned identification storage. Its fields are empty unless the
   *         most recent identification operation completed successfully.
   */
  const Identification& identification() const {
    return _identification;
  }

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
  SDI12&    _sdi12;       ///< The SDI12 transport instance
  Operation _operation;   ///< Identity supplied directly by the starting API
  char      _address;     ///< Target sensor address
  String    _rawCommand;  ///< Raw command body (without address or '!')

  enum : uint8_t {
    /** Storage for the longest command in the typed Phase 6 scope, including null. */
    kTypedCommandBufferSize = 6U
  };

  char    _typedCommand[kTypedCommandBufferSize];  ///< Complete typed wire command
  uint8_t _typedCommandLength;                     ///< Typed command bytes before null

  uint32_t _txnStartTime;           ///< millis() at transaction start
  uint32_t _txnTimeoutMs;           ///< Overall transaction deadline
  uint32_t _responseTimeoutMicros;  ///< Current response-completion deadline

  /// Conservative transaction guard until command-derived deadlines are implemented.
  static const uint32_t kDefaultTransactionTimeoutMs = 3000U;
  /// Deadline that permits all retries of an ordinary typed response.
  static const uint32_t kSimpleTransactionTimeoutMs = 5000U;
  /// Conservative response deadline for the transitional raw command API.
  static const uint32_t kDefaultRawResponseTimeoutMicros = 2000000UL;

  // Current phase and phase timings
  Phase    _phase;
  bool     _phaseStarted;     ///< Whether the current phase action has started
  uint32_t _phaseStartTime;   ///< micros() at phase start
  uint32_t _commandStopTime;  ///< micros() when last command stop bit ended
  uint32_t _breakEndTime;     ///< micros() when break ended

  // Attempt tracking (retry logic)
  uint8_t _sequenceCount;  ///< Current retry sequence number (1-3)
  uint8_t _breakCount;     ///< Number of breaks issued (0-3)
  uint8_t _attemptCount;   ///< No-break retries in current sequence (0-2)

  // Global bus history used only for first-attempt break selection
  bool     _busHistoryKnown;      ///< Whether the retained bus history is usable
  char     _lastAddress;          ///< Sensor addressed by the last bus operation
  uint32_t _lastBusActivityTime;  ///< micros() when the bus most recently began marking
  bool     _serviceRequestKnown;  ///< Whether a valid service request is retained
  char _serviceRequestAddress;    ///< Address returned by the retained service request
  uint32_t _serviceRequestTime;   ///< micros() when its response ended

  // Response collection
  char           _response[SDI12_RECORDER_RESPONSE_BUFFER_SIZE];
  uint8_t        _responseLength;
  uint8_t        _pendingReceiveEvents;
  RecorderResult _result;
  RecorderResult _retryFailureResult;  ///< First non-NO_RESPONSE retry failure
  RecorderResult _startResult;  ///< Immediate rejection of the latest start request

  // Recorder-owned parsed outputs
  char           _queriedAddress;
  char           _acknowledgedAddress;
  Identification _identification;

  // Private helper methods
  bool _rejectIfBusy();
  void _rejectOperation(RecorderResult reason);
  void _beginOperation();
  void _beginTypedOperation(Operation operation, char address, uint8_t commandLength);
  void _waitForCompletion();
  void _resetAttemptReception();
  void _clearOperationOutputs();
  bool _collectResponseCharacters();
  RecorderResult _validateResponse();
  RecorderResult _validateExpectedAddress(char expectedAddress) const;
  bool           _hasResponseTerminator() const;
  static bool    _isPrintableAscii(char value);
  void           _stripResponseEnvelope();
  bool           _shouldTransmitBreak() const;
  void           _recordServiceRequest(char address, uint32_t markingStartTime);
  void           _transitionTo(Phase nextPhase);
  void           _setResult(RecorderResult reason);

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
