/**
 * @file SDI12Recorder.cpp
 * @copyright Stroud Water Research Center
 * @license This library is published under the BSD-3 license.
 *
 * @brief Implementation of the SDI12Recorder transaction executor.
 */

#include "SDI12Recorder.h"

#include <string.h>

SDI12Recorder::SDI12Recorder(SDI12& sdi12)
  : _sdi12(sdi12),
    _operation(OPERATION_NONE),
    _address('0'),
    _typedCommandLength(0),
    _txnStartTime(0),
    _txnTimeoutMs(kDefaultTransactionTimeoutMs),
    _responseTimeoutMicros(kDefaultRawResponseTimeoutMicros),
    _phase(PHASE_IDLE),
    _phaseStarted(false),
    _phaseStartTime(0),
    _commandStopTime(0),
    _breakEndTime(0),
    _sequenceCount(0),
    _breakCount(0),
    _attemptCount(0),
    _busHistoryKnown(false),
    _lastAddress('\0'),
    _lastBusActivityTime(0),
    _serviceRequestKnown(false),
    _serviceRequestAddress('\0'),
    _serviceRequestTime(0),
    _reservedContextSlot(SDI12_RECORDER_CONTEXT_SLOTS),
    _contextInvalidationApplied(false),
    _measurementRequestsCRC(false),
    _measurementSeconds(0),
    _expectedValueCount(0),
    _measurementWaitStart(0),
    _dataValues(nullptr),
    _dataCapacity(0),
    _dataContextSlot(SDI12_RECORDER_CONTEXT_SLOTS),
    _dataCommandNumber(0),
    _dataValueCount(0),
    _dataExpectedCount(0),
    _dataCRCExpected(false),
    _responseLength(0),
    _pendingReceiveEvents(SDI12::SDI12_RECEIVE_EVENT_NONE),
    _result(RESULT_NONE),
    _retryFailureResult(RESULT_NONE),
    _startResult(RESULT_NONE),
    _queriedAddress('\0'),
    _acknowledgedAddress('\0') {
  _typedCommand[0] = '\0';
  _response[0]     = '\0';
  memset(_contexts, 0, sizeof(_contexts));
  memset(&_identification, 0, sizeof(_identification));
}

SDI12Recorder::~SDI12Recorder() {
  if (_sdi12.isActive()) { cancel(); }
}

char SDI12Recorder::queryAddress() {
  if (_rejectIfBusy()) { return '\0'; }
  startQueryAddress();
  _waitForCompletion();
  return _result == RESULT_SUCCESS ? _queriedAddress : '\0';
}

bool SDI12Recorder::requestAcknowledgement(char address) {
  if (_rejectIfBusy()) { return false; }
  startRequestAcknowledgement(address);
  _waitForCompletion();
  return _result == RESULT_SUCCESS;
}

bool SDI12Recorder::identify(char address, Identification& identification) {
  if (_rejectIfBusy()) { return false; }
  startIdentify(address);
  _waitForCompletion();
  if (_result != RESULT_SUCCESS) { return false; }
  identification = _identification;
  return true;
}

bool SDI12Recorder::changeAddress(char oldAddress, char newAddress) {
  if (_rejectIfBusy()) { return false; }
  startChangeAddress(oldAddress, newAddress);
  _waitForCompletion();
  return _result == RESULT_SUCCESS;
}

void SDI12Recorder::startQueryAddress() {
  if (_rejectIfBusy()) { return; }

  _typedCommand[0] = '?';
  _typedCommand[1] = '!';
  _beginTypedOperation(OPERATION_ADDRESS_QUERY, '?', 2U);
}

void SDI12Recorder::startRequestAcknowledgement(char address) {
  if (_rejectIfBusy()) { return; }
  if (!SDI12Address::isValid(address)) {
    _rejectOperation(RESULT_INVALID_ADDRESS);
    return;
  }

  _typedCommand[0] = address;
  _typedCommand[1] = '!';
  _beginTypedOperation(OPERATION_ACKNOWLEDGEMENT, address, 2U);
}

void SDI12Recorder::startIdentify(char address) {
  if (_rejectIfBusy()) { return; }
  if (!SDI12Address::isValid(address)) {
    _rejectOperation(RESULT_INVALID_ADDRESS);
    return;
  }

  _typedCommand[0] = address;
  _typedCommand[1] = 'I';
  _typedCommand[2] = '!';
  _beginTypedOperation(OPERATION_IDENTIFICATION, address, 3U);
}

void SDI12Recorder::startChangeAddress(char oldAddress, char newAddress) {
  if (_rejectIfBusy()) { return; }
  if (!SDI12Address::isValid(oldAddress) || !SDI12Address::isValid(newAddress)) {
    _rejectOperation(RESULT_INVALID_ADDRESS);
    return;
  }

  _typedCommand[0] = oldAddress;
  _typedCommand[1] = 'A';
  _typedCommand[2] = newAddress;
  _typedCommand[3] = '!';
  _beginTypedOperation(OPERATION_CHANGE_ADDRESS, oldAddress, 4U);
}

void SDI12Recorder::startMeasurement(char address, uint8_t measurementNumber,
                                     bool requestCRC) {
  if (_rejectIfBusy()) { return; }
  if (!SDI12Address::isValid(address)) {
    _rejectOperation(RESULT_INVALID_ADDRESS);
    return;
  }
  if (measurementNumber > 9U) {
    _rejectOperation(RESULT_INVALID_COMMAND_ARGUMENT);
    return;
  }

  const size_t contextSlot = _reserveContext(address);
  if (contextSlot >= SDI12_RECORDER_CONTEXT_SLOTS) {
    _rejectOperation(RESULT_CONTEXT_CAPACITY);
    return;
  }

  uint8_t commandLength          = 0U;
  _typedCommand[commandLength++] = address;
  _typedCommand[commandLength++] = 'M';
  if (requestCRC) { _typedCommand[commandLength++] = 'C'; }
  if (measurementNumber > 0U) {
    _typedCommand[commandLength++] = static_cast<char>('0' + measurementNumber);
  }
  _typedCommand[commandLength++] = '!';

  _reservedContextSlot    = contextSlot;
  _measurementRequestsCRC = requestCRC;
  _beginTypedOperation(OPERATION_MEASUREMENT, address, commandLength);
}

void SDI12Recorder::startConcurrentMeasurement(char address, uint8_t measurementNumber,
                                               bool requestCRC) {
  if (_rejectIfBusy()) { return; }
  if (!SDI12Address::isValid(address)) {
    _rejectOperation(RESULT_INVALID_ADDRESS);
    return;
  }
  if (measurementNumber > 9U) {
    _rejectOperation(RESULT_INVALID_COMMAND_ARGUMENT);
    return;
  }

  const size_t contextSlot = _reserveContext(address);
  if (contextSlot >= SDI12_RECORDER_CONTEXT_SLOTS) {
    _rejectOperation(RESULT_CONTEXT_CAPACITY);
    return;
  }

  uint8_t commandLength          = 0U;
  _typedCommand[commandLength++] = address;
  _typedCommand[commandLength++] = 'C';
  if (requestCRC) { _typedCommand[commandLength++] = 'C'; }
  if (measurementNumber > 0U) {
    _typedCommand[commandLength++] = static_cast<char>('0' + measurementNumber);
  }
  _typedCommand[commandLength++] = '!';

  _reservedContextSlot    = contextSlot;
  _measurementRequestsCRC = requestCRC;
  _beginTypedOperation(OPERATION_CONCURRENT_MEASUREMENT, address, commandLength);
}

size_t SDI12Recorder::requestContinuousData(char address, float* values,
                                            size_t capacity, uint8_t dataOption,
                                            bool requestCRC) {
  if (_rejectIfBusy()) { return 0U; }

  startRequestContinuousData(address, values, capacity, dataOption, requestCRC);
  _waitForCompletion();
  return _result == RESULT_SUCCESS ? _dataValueCount : 0U;
}

void SDI12Recorder::startRequestContinuousData(char address, float* values,
                                               size_t capacity, uint8_t dataOption,
                                               bool requestCRC) {
  if (_rejectIfBusy()) { return; }
  if (!SDI12Address::isValid(address)) {
    _rejectOperation(RESULT_INVALID_ADDRESS);
    return;
  }
  if (dataOption > 9U || (capacity > 0U && values == nullptr)) {
    _rejectOperation(RESULT_INVALID_COMMAND_ARGUMENT);
    return;
  }

  uint8_t commandLength          = 0U;
  _typedCommand[commandLength++] = address;
  _typedCommand[commandLength++] = 'R';
  if (requestCRC) { _typedCommand[commandLength++] = 'C'; }
  _typedCommand[commandLength++] = static_cast<char>('0' + dataOption);
  _typedCommand[commandLength++] = '!';
  _beginTypedOperation(OPERATION_CONTINUOUS_DATA, address, commandLength);

  _dataValues            = values;
  _dataCapacity          = capacity;
  _dataContextSlot       = SDI12_RECORDER_CONTEXT_SLOTS;
  _dataCommandNumber     = dataOption;
  _dataValueCount        = 0U;
  _dataExpectedCount     = 0U;
  _dataCRCExpected       = requestCRC;
  _txnTimeoutMs          = kDataTransactionTimeoutMs;
  _responseTimeoutMicros = kDataResponseTimeoutMicros;
}

void SDI12Recorder::requestData(char address, float* values, size_t capacity) {
  if (_rejectIfBusy()) { return; }
  if (!SDI12Address::isValid(address)) {
    _rejectOperation(RESULT_INVALID_ADDRESS);
    return;
  }

  const size_t contextSlot = _findContext(address);
  if (contextSlot >= SDI12_RECORDER_CONTEXT_SLOTS) {
    _rejectOperation(RESULT_CONTEXT_REQUIRED);
    return;
  }

  const MeasurementContext& context = _contexts[contextSlot];
  if (capacity < context.expectedValueCount) {
    _rejectOperation(RESULT_DATA_BUFFER_TOO_SMALL);
    return;
  }
  if (context.expectedValueCount > 0U && values == nullptr) {
    _rejectOperation(RESULT_INVALID_COMMAND_ARGUMENT);
    return;
  }

  _typedCommand[0] = address;
  _typedCommand[1] = 'D';
  _typedCommand[2] = '0';
  _typedCommand[3] = '!';
  _beginTypedOperation(OPERATION_DATA, address, 4U);

  _dataValues            = values;
  _dataCapacity          = capacity;
  _dataContextSlot       = contextSlot;
  _dataCommandNumber     = 0U;
  _dataValueCount        = 0U;
  _dataExpectedCount     = context.expectedValueCount;
  _dataCRCExpected       = context.crcExpected;
  _expectedValueCount    = context.expectedValueCount;
  _txnTimeoutMs          = kDataTransactionTimeoutMs;
  _responseTimeoutMicros = kDataResponseTimeoutMicros;

  if (context.phase == CONTEXT_PHASE_CONCURRENT_RUNNING &&
      !_readyAtReached(context.readyAt)) {
    _transitionTo(PHASE_WAITING_DATA_READY);
  }
}

size_t SDI12Recorder::measure(char address, float* values, size_t capacity,
                              uint8_t measurementNumber, bool requestCRC) {
  if (_rejectIfBusy()) { return 0U; }

  startMeasurement(address, measurementNumber, requestCRC);
  _waitForCompletion();
  if (_result != RESULT_SUCCESS) { return 0U; }

  requestData(address, values, capacity);
  _waitForCompletion();
  return _result == RESULT_SUCCESS ? _dataValueCount : 0U;
}

SDI12Recorder::MeasurementState SDI12Recorder::measurementState(char address) const {
  const size_t slot = _findContext(address);
  if (slot >= SDI12_RECORDER_CONTEXT_SLOTS) { return MEASUREMENT_NONE; }

  const MeasurementContext& context = _contexts[slot];
  if (context.phase == CONTEXT_PHASE_DATA_READY) { return MEASUREMENT_READY; }
  if (context.phase == CONTEXT_PHASE_CONCURRENT_RUNNING) {
    return _readyAtReached(context.readyAt) ? MEASUREMENT_READY : MEASUREMENT_RUNNING;
  }
  return MEASUREMENT_NONE;
}

void SDI12Recorder::startRawCommand(char address, const String& command) {
  if (_rejectIfBusy()) { return; }

  _operation             = OPERATION_RAW_COMMAND;
  _address               = address;
  _rawCommand            = command;
  _typedCommandLength    = 0U;
  _txnTimeoutMs          = kDefaultTransactionTimeoutMs;
  _responseTimeoutMicros = kDefaultRawResponseTimeoutMicros;
  _beginOperation();
}

void SDI12Recorder::update() {
  // Check for overall transaction timeout
  if (_phase != PHASE_IDLE && _phase != PHASE_WAITING_SERVICE_REQUEST &&
      _phase != PHASE_RECEIVING_SERVICE_REQUEST && _phase != PHASE_WAITING_DATA_READY) {
    uint32_t elapsed = millis() - _txnStartTime;
    if (elapsed > _txnTimeoutMs) {
      _setResult(RESULT_TRANSACTION_TIMEOUT);
      _transitionTo(PHASE_IDLE);
      return;
    }
  }

  // Dispatch to current phase handler
  switch (_phase) {
    case PHASE_IDLE: break;
    case PHASE_BREAK: _phase_break(); break;
    case PHASE_PRECOMMAND_MARK: _phase_precommand_mark(); break;
    case PHASE_TRANSMITTING: _phase_transmitting(); break;
    case PHASE_WAITING_RESPONSE: _phase_waiting_response(); break;
    case PHASE_RECEIVING: _phase_receiving(); break;
    case PHASE_DRAINING_RESPONSE: _phase_draining_response(); break;
    case PHASE_VALIDATING: _phase_validating(); break;
    case PHASE_DECIDING_RETRY: _phase_deciding_retry(); break;
    case PHASE_WAITING_SERVICE_REQUEST: _phase_waiting_service_request(); break;
    case PHASE_RECEIVING_SERVICE_REQUEST: _phase_receiving_service_request(); break;
    case PHASE_WAITING_DATA_READY: _phase_waiting_data_ready(); break;
  }
}

void SDI12Recorder::cancel() {
  _setResult(RESULT_CANCELLED);
  _transitionTo(PHASE_IDLE);
}

const char* SDI12Recorder::resultDescription() const {
  switch (result()) {
    case RESULT_NONE: return "";
    case RESULT_SUCCESS: return "SUCCESS";
    case RESULT_NO_RESPONSE: return "NO_RESPONSE";
    case RESULT_FRAMING_ERROR: return "FRAMING_ERROR";
    case RESULT_WRONG_ADDRESS: return "WRONG_ADDRESS";
    case RESULT_CRC_ERROR: return "CRC_ERROR";
    case RESULT_BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
    case RESULT_MALFORMED_RESPONSE: return "MALFORMED_RESPONSE";
    case RESULT_RESPONSE_TIMEOUT: return "RESPONSE_TIMEOUT";
    case RESULT_TRANSACTION_TIMEOUT: return "TRANSACTION_TIMEOUT";
    case RESULT_CANCELLED: return "CANCELLED";
    case RESULT_INVALID_ADDRESS: return "INVALID_ADDRESS";
    case RESULT_INVALID_COMMAND_ARGUMENT: return "INVALID_COMMAND_ARGUMENT";
    case RESULT_BUSY: return "BUSY";
    case RESULT_CONTEXT_CAPACITY: return "CONTEXT_CAPACITY";
    case RESULT_CONTEXT_REQUIRED: return "CONTEXT_REQUIRED";
    case RESULT_DATA_BUFFER_TOO_SMALL: return "DATA_BUFFER_TOO_SMALL";
    case RESULT_MEASUREMENT_ABORTED: return "MEASUREMENT_ABORTED";
    case RESULT_INCOMPLETE_DATA: return "INCOMPLETE_DATA";
    case RESULT_RETRIES_EXHAUSTED: return "RETRIES_EXHAUSTED";
    case RESULT_UNEXPECTED_BREAK: return "UNEXPECTED_BREAK";
    default: return "UNKNOWN";
  }
}

void SDI12Recorder::printRecorderResult(Stream& printStream) const {
  switch (result()) {
    case RESULT_NONE:
      printStream.print(F("Transaction is in progress or has not been started"));
      break;
    case RESULT_SUCCESS:
      printStream.print(
        F("Transaction completed successfully; response is valid and available"));
      break;
    case RESULT_NO_RESPONSE:
      printStream.print(F("No response was received within the maximum allowed time"));
      break;
    case RESULT_FRAMING_ERROR:
      printStream.print(F("Response arrived but had invalid framing or parity"));
      break;
    case RESULT_WRONG_ADDRESS:
      printStream.print(F("Response address does not match the command address"));
      break;
    case RESULT_CRC_ERROR:
      printStream.print(F("CRC validation failed when CRC was requested"));
      break;
    case RESULT_BUFFER_OVERFLOW:
      printStream.print(F("Response buffer overflowed before response completed"));
      break;
    case RESULT_MALFORMED_RESPONSE:
      printStream.print(F("Response-specific format validation failed"));
      break;
    case RESULT_RESPONSE_TIMEOUT:
      printStream.print(F("The command-specific response deadline was exceeded"));
      break;
    case RESULT_TRANSACTION_TIMEOUT:
      printStream.print(F("The overall transaction deadline was exceeded"));
      break;
    case RESULT_CANCELLED:
      printStream.print(F("Transaction was cancelled by the application"));
      break;
    case RESULT_INVALID_ADDRESS:
      printStream.print(F("The supplied sensor address is not valid"));
      break;
    case RESULT_INVALID_COMMAND_ARGUMENT:
      printStream.print(F("A command argument is outside its permitted range"));
      break;
    case RESULT_BUSY:
      printStream.print(F("Another recorder operation is already active"));
      break;
    case RESULT_CONTEXT_CAPACITY:
      printStream.print(F("No tracking slot is available for the sensor transaction"));
      break;
    case RESULT_CONTEXT_REQUIRED:
      printStream.print(F("The operation requires pending measurement context"));
      break;
    case RESULT_DATA_BUFFER_TOO_SMALL:
      printStream.print(F("The data buffer cannot hold all returned values"));
      break;
    case RESULT_MEASUREMENT_ABORTED:
      printStream.print(F("The sensor reported that its measurement was aborted"));
      break;
    case RESULT_INCOMPLETE_DATA:
      printStream.print(
        F("The advertised value count was not received by the end of D9"));
      break;
    case RESULT_RETRIES_EXHAUSTED:
      printStream.print(F("All protocol-required retry attempts were exhausted"));
      break;
    case RESULT_UNEXPECTED_BREAK:
      printStream.print(F("Another recorder break interrupted the measurement"));
      break;
    default: printStream.print(F("Unknown recorder result")); break;
  }
}

void SDI12Recorder::printPhase(Stream& printStream) const {
  switch (_phase) {
    case PHASE_IDLE: printStream.print(F("No transaction is active")); break;
    case PHASE_BREAK:
      printStream.print(F("Transmitting the wake-up break (spacing)"));
      break;
    case PHASE_PRECOMMAND_MARK:
      printStream.print(F("Waiting for the pre-command marking minimum"));
      break;
    case PHASE_TRANSMITTING: printStream.print(F("Transmitting the command")); break;
    case PHASE_WAITING_RESPONSE:
      printStream.print(F("Waiting for the response start bit"));
      break;
    case PHASE_RECEIVING: printStream.print(F("Receiving response characters")); break;
    case PHASE_DRAINING_RESPONSE:
      printStream.print(F("Discarding the invalid response before retrying"));
      break;
    case PHASE_VALIDATING:
      printStream.print(F("Validating response format and content"));
      break;
    case PHASE_DECIDING_RETRY:
      printStream.print(F("Determining whether to retry or terminate"));
      break;
    case PHASE_WAITING_SERVICE_REQUEST:
      printStream.print(F("Waiting for measurement readiness"));
      break;
    case PHASE_RECEIVING_SERVICE_REQUEST:
      printStream.print(F("Receiving the measurement service request"));
      break;
    case PHASE_WAITING_DATA_READY:
      printStream.print(F("Waiting for concurrent measurement data readiness"));
      break;
    default: printStream.print(F("Unknown recorder phase")); break;
  }
}

void SDI12Recorder::printSensorInfo(const Identification& identification,
                                    Stream&               printStream) {
  printStream.print(F("Address: "));
  printStream.print(identification.address);
  printStream.print(F(", SDI-12 Version: "));
  printStream.print(identification.sdi12Version);
  printStream.print(F(", Vendor ID: "));
  printStream.print(identification.vendorID);
  printStream.print(F(", Sensor Model: "));
  printStream.print(identification.sensorModel);
  printStream.print(F(", Sensor Version: "));
  printStream.print(identification.sensorVersion);
  printStream.print(F(", Sensor Info: "));
  printStream.println(identification.sensorInfo);
}

size_t SDI12Recorder::copyResponse(char* destination, size_t capacity) const {
  if (destination != nullptr && capacity > 0) {
    const size_t copyLength = _responseLength < capacity - 1 ? _responseLength
                                                             : capacity - 1;
    memcpy(destination, _response, copyLength);
    destination[copyLength] = '\0';
  }
  return _responseLength;
}

// ============================================================================
// Private helper methods
// ============================================================================

bool SDI12Recorder::_rejectIfBusy() {
  if (!isBusy()) { return false; }
  _startResult = RESULT_BUSY;
  return true;
}

void SDI12Recorder::_rejectOperation(RecorderResult reason) {
  _operation          = OPERATION_NONE;
  _startResult        = RESULT_NONE;
  _retryFailureResult = RESULT_NONE;
  clearResponse();
  _clearOperationOutputs();
  _result = reason;
}

void SDI12Recorder::_beginOperation() {
  _startResult                = RESULT_NONE;
  _txnStartTime               = millis();
  _sequenceCount              = 1;
  _breakCount                 = 0;
  _attemptCount               = 0;
  _breakEndTime               = 0;
  _retryFailureResult         = RESULT_NONE;
  _contextInvalidationApplied = false;
  _resetAttemptReception();
  _clearOperationOutputs();
  _result = RESULT_NONE;

  // Always start with the break phase; the phase skips the break when policy forbids
  // it.
  _transitionTo(PHASE_BREAK);

  // Make this recorder the active SDI12 instance.
  if (!_sdi12.isActive()) { _sdi12.setActive(); }
}

void SDI12Recorder::_beginTypedOperation(Operation operation, char address,
                                         uint8_t commandLength) {
  _operation                         = operation;
  _address                           = address;
  _typedCommandLength                = commandLength;
  _typedCommand[_typedCommandLength] = '\0';
  _txnTimeoutMs                      = kSimpleTransactionTimeoutMs;
  _responseTimeoutMicros             = SDI12Timing::kNormalResponseMaxMicros;
  _beginOperation();
}

void SDI12Recorder::_waitForCompletion() {
  while (isBusy()) {
    update();
    yield();
  }
}

void SDI12Recorder::_resetAttemptReception() {
  clearResponse();
  _sdi12.clearBuffer();
  _sdi12.takeReceiveEvents();
  _pendingReceiveEvents = SDI12::SDI12_RECEIVE_EVENT_NONE;
}

void SDI12Recorder::_clearOperationOutputs() {
  _queriedAddress      = '\0';
  _acknowledgedAddress = '\0';
  _measurementSeconds  = 0U;
  _expectedValueCount  = 0U;
  _dataValueCount      = 0U;
  memset(&_identification, 0, sizeof(_identification));
}

size_t SDI12Recorder::_findContext(char address) const {
  for (size_t i = 0U; i < SDI12_RECORDER_CONTEXT_SLOTS; ++i) {
    if (_contexts[i].occupied && _contexts[i].address == address) { return i; }
  }
  return SDI12_RECORDER_CONTEXT_SLOTS;
}

size_t SDI12Recorder::_reserveContext(char address) const {
  const size_t existingSlot = _findContext(address);
  if (existingSlot < SDI12_RECORDER_CONTEXT_SLOTS) { return existingSlot; }

  for (size_t i = 0U; i < SDI12_RECORDER_CONTEXT_SLOTS; ++i) {
    if (!_contexts[i].occupied) { return i; }
  }
  return SDI12_RECORDER_CONTEXT_SLOTS;
}

void SDI12Recorder::_releaseContext(size_t slot) {
  if (slot >= SDI12_RECORDER_CONTEXT_SLOTS) { return; }
  memset(&_contexts[slot], 0, sizeof(_contexts[slot]));
}

void SDI12Recorder::_invalidateContext(char address) {
  _releaseContext(_findContext(address));
}

void SDI12Recorder::_invalidateRunningContext(char address) {
  const size_t slot = _findContext(address);
  if (slot < SDI12_RECORDER_CONTEXT_SLOTS &&
      _contexts[slot].phase == CONTEXT_PHASE_CONCURRENT_RUNNING) {
    if (_readyAtReached(_contexts[slot].readyAt)) {
      _contexts[slot].phase = CONTEXT_PHASE_DATA_READY;
    } else {
      _releaseContext(slot);
    }
  }
}

void SDI12Recorder::_invalidateAllContexts() {
  memset(_contexts, 0, sizeof(_contexts));
}

void SDI12Recorder::_applyContextInvalidationForTransmission() {
  if (_contextInvalidationApplied) { return; }
  _contextInvalidationApplied = true;

  switch (_operation) {
    case OPERATION_ADDRESS_QUERY: _invalidateAllContexts(); break;
    case OPERATION_ACKNOWLEDGEMENT:
    case OPERATION_IDENTIFICATION:
    case OPERATION_CONTINUOUS_DATA: _invalidateRunningContext(_address); break;
    case OPERATION_CHANGE_ADDRESS:
      _invalidateContext(_address);
      _invalidateContext(_typedCommand[2]);
      break;
    case OPERATION_MEASUREMENT:
    case OPERATION_CONCURRENT_MEASUREMENT:
    case OPERATION_RAW_COMMAND: _invalidateContext(_address); break;
    case OPERATION_NONE:
    case OPERATION_DATA: break;
  }
}

void SDI12Recorder::_commitReadyMeasurementContext() {
  if (_reservedContextSlot >= SDI12_RECORDER_CONTEXT_SLOTS) { return; }

  MeasurementContext& context = _contexts[_reservedContextSlot];
  context.address             = _address;
  context.occupied            = true;
  context.phase               = CONTEXT_PHASE_DATA_READY;
  context.crcExpected         = _measurementRequestsCRC;
  context.expectedValueCount  = _expectedValueCount;
  context.readyAt             = 0U;
}

void SDI12Recorder::_commitConcurrentMeasurementContext() {
  if (_reservedContextSlot >= SDI12_RECORDER_CONTEXT_SLOTS) { return; }

  MeasurementContext& context = _contexts[_reservedContextSlot];
  context.address             = _address;
  context.occupied            = true;
  context.phase               = _measurementSeconds == 0U ? CONTEXT_PHASE_DATA_READY
                                                          : CONTEXT_PHASE_CONCURRENT_RUNNING;
  context.crcExpected         = _measurementRequestsCRC;
  context.expectedValueCount  = _expectedValueCount;
  context.readyAt = millis() + static_cast<uint32_t>(_measurementSeconds) * 1000UL;
}

void SDI12Recorder::_startNextDataCommand() {
  ++_dataCommandNumber;
  _typedCommand[0]            = _address;
  _typedCommand[1]            = 'D';
  _typedCommand[2]            = static_cast<char>('0' + _dataCommandNumber);
  _typedCommand[3]            = '!';
  _typedCommand[4]            = '\0';
  _typedCommandLength         = 4U;
  _txnStartTime               = millis();
  _txnTimeoutMs               = kDataTransactionTimeoutMs;
  _responseTimeoutMicros      = kDataResponseTimeoutMicros;
  _sequenceCount              = 1U;
  _breakCount                 = 0U;
  _attemptCount               = 0U;
  _retryFailureResult         = RESULT_NONE;
  _contextInvalidationApplied = false;
  _resetAttemptReception();
  _result = RESULT_NONE;
  _transitionTo(PHASE_BREAK);
}

bool SDI12Recorder::_readyAtReached(uint32_t readyAt) {
  return millis() - readyAt < 0x80000000UL;
}

bool SDI12Recorder::_collectResponseCharacters() {
  while (_sdi12.available() > 0) {
    const int received = _sdi12.read();
    if (received < 0) {
      _setResult(RESULT_FRAMING_ERROR);
      return false;
    }

    if (_responseLength + 1 >= SDI12_RECORDER_RESPONSE_BUFFER_SIZE) {
      _sdi12.clearBuffer();
      _setResult(RESULT_BUFFER_OVERFLOW);
      return false;
    }

    _response[_responseLength++] = static_cast<char>(received);
    _response[_responseLength]   = '\0';
  }
  return true;
}

SDI12Recorder::RecorderResult
SDI12Recorder::_validateExpectedAddress(char expectedAddress) const {
  if (_responseLength < 1U) { return RESULT_MALFORMED_RESPONSE; }
  if (_response[0] == expectedAddress) { return RESULT_SUCCESS; }
  if (SDI12Address::isValid(_response[0])) { return RESULT_WRONG_ADDRESS; }
  return RESULT_MALFORMED_RESPONSE;
}

bool SDI12Recorder::_hasResponseTerminator() const {
  return _responseLength >= 2U && _response[_responseLength - 2U] == '\r' &&
    _response[_responseLength - 1U] == '\n';
}

bool SDI12Recorder::_isPrintableAscii(char value) {
  const uint8_t byteValue = static_cast<uint8_t>(value);
  return byteValue >= 0x20U && byteValue <= 0x7EU;
}

SDI12Recorder::RecorderResult SDI12Recorder::_validateResponse() {
  if (_operation == OPERATION_RAW_COMMAND) {
    if (_responseLength < 1U || _response[0] != _address) {
      return RESULT_WRONG_ADDRESS;
    }
    return RESULT_SUCCESS;
  }

  if (_operation == OPERATION_ADDRESS_QUERY) {
    if (_responseLength != 3U || !_hasResponseTerminator() ||
        !SDI12Address::isValid(_response[0])) {
      return RESULT_MALFORMED_RESPONSE;
    }
    _queriedAddress = _response[0];
    return RESULT_SUCCESS;
  }

  char expectedAddress = _address;
  if (_operation == OPERATION_CHANGE_ADDRESS) { expectedAddress = _typedCommand[2]; }

  const RecorderResult addressResult = _validateExpectedAddress(expectedAddress);
  if (addressResult != RESULT_SUCCESS) { return addressResult; }

  if (_operation == OPERATION_DATA || _operation == OPERATION_CONTINUOUS_DATA) {
    return _validateDataResponse();
  }

  if (_operation == OPERATION_ACKNOWLEDGEMENT ||
      _operation == OPERATION_CHANGE_ADDRESS) {
    if (_responseLength != 3U || !_hasResponseTerminator()) {
      return RESULT_MALFORMED_RESPONSE;
    }
    _acknowledgedAddress = expectedAddress;
    return RESULT_SUCCESS;
  }

  if (_operation == OPERATION_IDENTIFICATION) {
    // address + 2 version + 8 vendor + 6 model + 3 sensor version + 0-13 info + CR/LF
    if (_responseLength < 22U || _responseLength > 35U || !_hasResponseTerminator() ||
        _response[1] < '0' || _response[1] > '9' || _response[2] < '0' ||
        _response[2] > '9') {
      return RESULT_MALFORMED_RESPONSE;
    }
    for (uint8_t i = 3U; i < _responseLength - 2U; ++i) {
      if (!_isPrintableAscii(_response[i])) { return RESULT_MALFORMED_RESPONSE; }
    }

    Identification parsed = {};
    parsed.address        = expectedAddress;
    memcpy(parsed.sdi12Version, _response + 1, 2U);
    memcpy(parsed.vendorID, _response + 3, 8U);
    memcpy(parsed.sensorModel, _response + 11, 6U);
    memcpy(parsed.sensorVersion, _response + 17, 3U);
    const uint8_t informationLength = _responseLength - 22U;
    memcpy(parsed.sensorInfo, _response + 20, informationLength);
    _identification = parsed;
    return RESULT_SUCCESS;
  }

  if (_operation == OPERATION_MEASUREMENT ||
      _operation == OPERATION_CONCURRENT_MEASUREMENT) {
    const uint8_t valueCountDigits =
      _operation == OPERATION_CONCURRENT_MEASUREMENT ? 2U : 1U;
    // address + three decimal time digits + one or two value-count digits + CR/LF
    if (_responseLength != 6U + valueCountDigits || !_hasResponseTerminator()) {
      return RESULT_MALFORMED_RESPONSE;
    }
    for (uint8_t i = 1U; i < 4U + valueCountDigits; ++i) {
      if (_response[i] < '0' || _response[i] > '9') {
        return RESULT_MALFORMED_RESPONSE;
      }
    }

    _measurementSeconds = static_cast<uint16_t>(_response[1] - '0') * 100U +
      static_cast<uint16_t>(_response[2] - '0') * 10U +
      static_cast<uint16_t>(_response[3] - '0');
    _expectedValueCount = static_cast<uint8_t>(_response[4] - '0');
    if (valueCountDigits == 2U) {
      _expectedValueCount = _expectedValueCount * 10U +
        static_cast<uint8_t>(_response[5] - '0');
    }
    return RESULT_SUCCESS;
  }

  return RESULT_MALFORMED_RESPONSE;
}

SDI12Recorder::RecorderResult SDI12Recorder::_validateDataResponse() {
  if (!_hasResponseTerminator()) { return RESULT_MALFORMED_RESPONSE; }

  uint8_t dataEnd = _responseLength - 2U;
  if (_dataCRCExpected) {
    if (_responseLength < 6U) { return RESULT_MALFORMED_RESPONSE; }
    dataEnd = _responseLength - 5U;

    const char savedCharacter    = _response[dataEnd];
    _response[dataEnd]           = '\0';
    const uint16_t calculatedCRC = _sdi12.calculateCRC(_response);
    _response[dataEnd]           = savedCharacter;

    const char calculatedCRC0 = static_cast<char>(0x40U | (calculatedCRC >> 12U));
    const char calculatedCRC1 =
      static_cast<char>(0x40U | ((calculatedCRC >> 6U) & 0x3FU));
    const char calculatedCRC2 = static_cast<char>(0x40U | (calculatedCRC & 0x3FU));
    if (_response[dataEnd] != calculatedCRC0 ||
        _response[dataEnd + 1U] != calculatedCRC1 ||
        _response[dataEnd + 2U] != calculatedCRC2) {
      return RESULT_CRC_ERROR;
    }
  }

  if (dataEnd == 1U) {
    return _operation == OPERATION_CONTINUOUS_DATA ? RESULT_SUCCESS
                                                   : RESULT_MEASUREMENT_ABORTED;
  }
  if (dataEnd < 1U) { return RESULT_MALFORMED_RESPONSE; }

  uint8_t blockValueCount = 0U;
  if (!_parseDataValues(1U, dataEnd, nullptr, blockValueCount)) {
    return RESULT_MALFORMED_RESPONSE;
  }
  if (_operation == OPERATION_CONTINUOUS_DATA) {
    if (blockValueCount > _dataCapacity) { return RESULT_DATA_BUFFER_TOO_SMALL; }
  } else if (static_cast<uint16_t>(_dataValueCount) + blockValueCount >
             _dataExpectedCount) {
    return RESULT_MALFORMED_RESPONSE;
  }

  uint8_t writtenCount = 0U;
  if (!_parseDataValues(1U, dataEnd, _dataValues + _dataValueCount, writtenCount) ||
      writtenCount != blockValueCount) {
    return RESULT_MALFORMED_RESPONSE;
  }
  _dataValueCount += writtenCount;

  if (_dataCRCExpected) {
    _response[dataEnd]         = '\r';
    _response[dataEnd + 1U]    = '\n';
    _responseLength            = dataEnd + 2U;
    _response[_responseLength] = '\0';
  }
  return RESULT_SUCCESS;
}

bool SDI12Recorder::_parseDataValues(uint8_t begin, uint8_t end, float* destination,
                                     uint8_t& valueCount) const {
  valueCount       = 0U;
  uint8_t position = begin;

  while (position < end) {
    const uint8_t fieldStart = position;
    const char    sign       = _response[position++];
    if (sign != '+' && sign != '-') { return false; }

    bool    decimalSeen  = false;
    uint8_t digitCount   = 0U;
    float   value        = 0.0F;
    float   decimalPlace = 0.1F;

    while (position < end && _response[position] != '+' && _response[position] != '-') {
      const char character = _response[position++];
      if (character == '.') {
        if (decimalSeen) { return false; }
        decimalSeen = true;
        continue;
      }
      if (character < '0' || character > '9') { return false; }

      ++digitCount;
      if (digitCount > 7U) { return false; }
      const uint8_t digit = static_cast<uint8_t>(character - '0');
      if (decimalSeen) {
        value += static_cast<float>(digit) * decimalPlace;
        decimalPlace *= 0.1F;
      } else {
        value = value * 10.0F + static_cast<float>(digit);
      }
    }

    if (digitCount == 0U ||
        position - fieldStart >
          static_cast<uint8_t>(SDI12ResponseLimits::kValueMaxCharacters)) {
      return false;
    }
    if (destination != nullptr) {
      destination[valueCount] = sign == '-' ? -value : value;
    }
    ++valueCount;
  }
  return valueCount > 0U;
}

void SDI12Recorder::_stripResponseEnvelope() {
  if (_hasResponseTerminator()) {
    _responseLength -= 2U;
    _response[_responseLength] = '\0';
  }

  if (_responseLength > 0U) {
    --_responseLength;
    memmove(_response, _response + 1, _responseLength);
    _response[_responseLength] = '\0';
  }
}

bool SDI12Recorder::_shouldTransmitBreak() const {
#if defined(SDI12_RECORDER_ALWAYS_BREAK)
  return true;
#else
  // The only optimized first-attempt path is D0 immediately following a valid service
  // request from the same sensor. All other new top-level commands require a break.
  if (!_busHistoryKnown || !_serviceRequestKnown || _operation != OPERATION_DATA ||
      _typedCommandLength != 4U || _typedCommand[1] != 'D' || _typedCommand[2] != '0' ||
      _typedCommand[3] != '!' || _lastAddress != _address ||
      _serviceRequestAddress != _address) {
    return true;
  }

  const uint32_t now = micros();
  return now - _lastBusActivityTime > SDI12Timing::kBreakRequiredAfterMarkingMicros ||
    now - _serviceRequestTime > SDI12Timing::kBreakRequiredAfterMarkingMicros;
#endif
}

void SDI12Recorder::_recordServiceRequest(char address, uint32_t markingStartTime) {
  _busHistoryKnown       = true;
  _lastAddress           = address;
  _lastBusActivityTime   = markingStartTime;
  _serviceRequestKnown   = true;
  _serviceRequestAddress = address;
  _serviceRequestTime    = markingStartTime;
}

void SDI12Recorder::_transitionTo(Phase nextPhase) {
  _phase          = nextPhase;
  _phaseStarted   = false;
  _phaseStartTime = micros();
  if (nextPhase == PHASE_IDLE) {
    _operation   = OPERATION_NONE;
    _startResult = RESULT_NONE;
  }
}

void SDI12Recorder::_setResult(RecorderResult reason) {
  _result = reason;
}

// ============================================================================
// Phase implementations
// ============================================================================

void SDI12Recorder::_phase_break() {
  // Retry sequences always begin with a break. Only the first sequence of a new
  // top-level operation may use the optimized no-break path.
  if (_sequenceCount == 1U && _breakCount == 0U && !_shouldTransmitBreak()) {
    _transitionTo(PHASE_PRECOMMAND_MARK);
    return;
  }

  if (!_phaseStarted) {
    _sdi12.startBreak();
    _phaseStartTime        = micros();
    _lastBusActivityTime   = _phaseStartTime;
    _busHistoryKnown       = false;
    _serviceRequestKnown   = false;
    _serviceRequestAddress = '\0';
    _phaseStarted          = true;
    _breakCount++;
  }

  if (micros() - _phaseStartTime >= SDI12_LINE_BREAK_MICROS) {
    _breakEndTime = micros();
    _transitionTo(PHASE_PRECOMMAND_MARK);
  }
}

void SDI12Recorder::_phase_precommand_mark() {
  if (!_phaseStarted) {
    _sdi12.startMarking();
    _phaseStartTime = micros();
    _phaseStarted   = true;
  }

  if (micros() - _phaseStartTime >= SDI12_LINE_MARK_MICROS) {
    _transitionTo(PHASE_TRANSMITTING);
  }
}

void SDI12Recorder::_phase_transmitting() {
  // Re-check immediately before the optimized first transmission. The application may
  // have delayed calling update() while the pre-command mark was in progress.
  if (_sequenceCount == 1U && _breakCount == 0U && _attemptCount == 0U &&
      _shouldTransmitBreak()) {
    _transitionTo(PHASE_BREAK);
    return;
  }

  _applyContextInvalidationForTransmission();

  if (_operation == OPERATION_RAW_COMMAND) {
    _sdi12.writeChar(_address, true);
    for (unsigned int i = 0; i < _rawCommand.length(); ++i) {
      _sdi12.writeChar(_rawCommand[i], true);
    }
    _sdi12.writeChar('!', true);
  } else {
    for (uint8_t i = 0U; i < _typedCommandLength; ++i) {
      _sdi12.writeChar(_typedCommand[i], true);
    }
  }
  _sdi12.forceListen();
  _commandStopTime       = micros();
  _busHistoryKnown       = true;
  _lastAddress           = _address;
  _lastBusActivityTime   = _commandStopTime;
  _serviceRequestKnown   = false;
  _serviceRequestAddress = '\0';
  _transitionTo(PHASE_WAITING_RESPONSE);
}

void SDI12Recorder::_phase_waiting_response() {
  // Per spec 7.0, a sensor must begin its response within 15 ms (±0.4 ms)
  // of the command stop bit. However, Appendix B waits up to 16.67 ms to observe
  // a complete start bit before deciding "no response".

  uint32_t elapsedMicros = micros() - _commandStopTime;

  uint8_t receiveEvents = _sdi12.takeReceiveEvents();

  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_START_BIT_OBSERVED) {
    // Response has started; begin collecting characters
    clearResponse();
    _pendingReceiveEvents = receiveEvents &
      static_cast<uint8_t>(~SDI12::SDI12_RECEIVE_EVENT_START_BIT_OBSERVED);
    _transitionTo(PHASE_RECEIVING);
    return;
  }

  // Maximum time to wait for response start (16.67 ms per Appendix B). Some field
  // sensors incorrectly perform a new measurement after an R/RC command and delay
  // their response for hundreds of milliseconds. For continuous data only, keep
  // listening until the existing response deadline so a retry cannot collide with
  // that late response. All response validation remains unchanged.
  const uint32_t RESPONSE_START_MAX_MICROS =
    _operation == OPERATION_CONTINUOUS_DATA ? _responseTimeoutMicros : 16670UL + 400UL;

  if (elapsedMicros > RESPONSE_START_MAX_MICROS) {
    // No response received in time
    _setResult(RESULT_NO_RESPONSE);
    _transitionTo(PHASE_DECIDING_RETRY);
  }
}

void SDI12Recorder::_phase_receiving() {
  uint8_t receiveEvents = _pendingReceiveEvents;
  _pendingReceiveEvents = SDI12::SDI12_RECEIVE_EVENT_NONE;

  // Consume events before draining so a completion already observed by the ISR has
  // its corresponding bytes copied before validation.
  receiveEvents |= _sdi12.takeReceiveEvents();
  if (!_collectResponseCharacters()) {
    // An invalid response must finish before the recorder retries. Preserve any
    // completion event already observed and discard the remaining response bytes.
    _pendingReceiveEvents = receiveEvents | _sdi12.takeReceiveEvents();
    _transitionTo(PHASE_DRAINING_RESPONSE);
    return;
  }

  // An ISR may complete the response after the first event read but after the buffer
  // appeared empty. Read the events again, then drain once more when completion was
  // observed so the terminating CR/LF cannot be left in the transport buffer.
  receiveEvents |= _sdi12.takeReceiveEvents();
  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_MESSAGE_COMPLETED) {
    if (!_collectResponseCharacters()) {
      _transitionTo(PHASE_DECIDING_RETRY);
      return;
    }
    receiveEvents |= _sdi12.takeReceiveEvents();
    _lastBusActivityTime = micros();
  }

  // Check for errors
  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_FRAMING_ERROR) {
    _setResult(RESULT_FRAMING_ERROR);
    _pendingReceiveEvents = receiveEvents;
    _transitionTo(PHASE_DRAINING_RESPONSE);
    return;
  }

  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_BUFFER_OVERFLOW) {
    _setResult(RESULT_BUFFER_OVERFLOW);
    _pendingReceiveEvents = receiveEvents;
    _transitionTo(PHASE_DRAINING_RESPONSE);
    return;
  }

  // Check for completion (CR LF) only after ruling out coalesced receive errors.
  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_MESSAGE_COMPLETED) {
    _transitionTo(PHASE_VALIDATING);
    return;
  }

  // Check for the command-specific response-completion deadline.
  if (micros() - _commandStopTime > _responseTimeoutMicros) {
    _setResult(RESULT_RESPONSE_TIMEOUT);
    _transitionTo(PHASE_DECIDING_RETRY);
    return;
  }
}

void SDI12Recorder::_phase_draining_response() {
  uint8_t receiveEvents = _pendingReceiveEvents;
  _pendingReceiveEvents = SDI12::SDI12_RECEIVE_EVENT_NONE;
  receiveEvents |= _sdi12.takeReceiveEvents();

  // The response is already invalid, so consume bytes without checking parity or
  // retaining their contents. This keeps the transport buffer available until the
  // sensor finishes transmitting CR/LF.
  int available = _sdi12.available();
  while (available > 0) {
    _sdi12.readBinary();
    available = _sdi12.available();
  }
  if (available < 0) { _sdi12.clearBuffer(); }

  receiveEvents |= _sdi12.takeReceiveEvents();
  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_MESSAGE_COMPLETED) {
    _lastBusActivityTime = micros();
    _transitionTo(PHASE_DECIDING_RETRY);
    return;
  }

  // A malformed sensor may never terminate its response. Bound the drain with the
  // same command-specific response deadline used for otherwise incomplete responses.
  if (micros() - _commandStopTime > _responseTimeoutMicros) {
    _transitionTo(PHASE_DECIDING_RETRY);
  }
}

void SDI12Recorder::_phase_validating() {
  const RecorderResult validationResult = _validateResponse();
  if (_operation == OPERATION_DATA && validationResult == RESULT_MEASUREMENT_ABORTED) {
    _releaseContext(_dataContextSlot);
    _setResult(RESULT_MEASUREMENT_ABORTED);
    _transitionTo(PHASE_IDLE);
    return;
  }
  if (_operation == OPERATION_CONTINUOUS_DATA &&
      validationResult == RESULT_DATA_BUFFER_TOO_SMALL) {
    _setResult(RESULT_DATA_BUFFER_TOO_SMALL);
    _transitionTo(PHASE_IDLE);
    return;
  }
  if (validationResult != RESULT_SUCCESS) {
    _setResult(validationResult);
    _transitionTo(PHASE_DECIDING_RETRY);
    return;
  }

  _stripResponseEnvelope();

  if (_operation == OPERATION_MEASUREMENT) {
    if (_measurementSeconds == 0U) {
      _commitReadyMeasurementContext();
      _setResult(RESULT_SUCCESS);
      _transitionTo(PHASE_IDLE);
      return;
    }

    _measurementWaitStart = millis();
    _txnStartTime         = _measurementWaitStart;
    _txnTimeoutMs = static_cast<uint32_t>(_measurementSeconds) * 1000UL + 1000UL;
    clearResponse();
    _pendingReceiveEvents = SDI12::SDI12_RECEIVE_EVENT_NONE;
    _sdi12.takeLineEvents();
    _transitionTo(PHASE_WAITING_SERVICE_REQUEST);
    return;
  }

  if (_operation == OPERATION_CONCURRENT_MEASUREMENT) {
    _commitConcurrentMeasurementContext();
    _setResult(RESULT_SUCCESS);
    _transitionTo(PHASE_IDLE);
    return;
  }

  if (_operation == OPERATION_DATA) {
    if (_dataValueCount == _dataExpectedCount) {
      _releaseContext(_dataContextSlot);
      _setResult(RESULT_SUCCESS);
      _transitionTo(PHASE_IDLE);
      return;
    }
    if (_dataCommandNumber >= 9U) {
      _setResult(RESULT_INCOMPLETE_DATA);
      _transitionTo(PHASE_IDLE);
      return;
    }

    _startNextDataCommand();
    return;
  }

  if (_operation == OPERATION_CONTINUOUS_DATA) {
    _setResult(RESULT_SUCCESS);
    _transitionTo(PHASE_IDLE);
    return;
  }

  // Success!
  _setResult(RESULT_SUCCESS);
  _transitionTo(PHASE_IDLE);
}

void SDI12Recorder::_phase_waiting_service_request() {
  _sdi12.serviceLineConditions();
  const uint8_t lineEvents = _sdi12.takeLineEvents();
  if (lineEvents &
      (SDI12::SDI12_LINE_EVENT_BREAK_THRESHOLD_REACHED |
       SDI12::SDI12_LINE_EVENT_BREAK_COMPLETED)) {
    _busHistoryKnown       = false;
    _serviceRequestKnown   = false;
    _serviceRequestAddress = '\0';
    _setResult(RESULT_UNEXPECTED_BREAK);
    _transitionTo(PHASE_IDLE);
    return;
  }

  uint8_t receiveEvents = _sdi12.takeReceiveEvents();

  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_START_BIT_OBSERVED) {
    clearResponse();
    _pendingReceiveEvents = receiveEvents &
      static_cast<uint8_t>(~SDI12::SDI12_RECEIVE_EVENT_START_BIT_OBSERVED);
    _transitionTo(PHASE_RECEIVING_SERVICE_REQUEST);
    return;
  }

  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_FRAMING_ERROR) {
    _setResult(RESULT_FRAMING_ERROR);
    _transitionTo(PHASE_IDLE);
    return;
  }
  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_BUFFER_OVERFLOW) {
    _setResult(RESULT_BUFFER_OVERFLOW);
    _transitionTo(PHASE_IDLE);
    return;
  }

  const uint32_t waitTime = static_cast<uint32_t>(_measurementSeconds) * 1000UL;
  if (millis() - _measurementWaitStart >= waitTime) {
    _commitReadyMeasurementContext();
    _setResult(RESULT_SUCCESS);
    _transitionTo(PHASE_IDLE);
  }
}

void SDI12Recorder::_phase_receiving_service_request() {
  _sdi12.serviceLineConditions();
  const uint8_t lineEvents = _sdi12.takeLineEvents();
  if (lineEvents &
      (SDI12::SDI12_LINE_EVENT_BREAK_THRESHOLD_REACHED |
       SDI12::SDI12_LINE_EVENT_BREAK_COMPLETED)) {
    _busHistoryKnown       = false;
    _serviceRequestKnown   = false;
    _serviceRequestAddress = '\0';
    _setResult(RESULT_UNEXPECTED_BREAK);
    _transitionTo(PHASE_IDLE);
    return;
  }

  uint8_t receiveEvents = _pendingReceiveEvents;
  _pendingReceiveEvents = SDI12::SDI12_RECEIVE_EVENT_NONE;
  receiveEvents |= _sdi12.takeReceiveEvents();

  if (!_collectResponseCharacters()) {
    _transitionTo(PHASE_IDLE);
    return;
  }

  receiveEvents |= _sdi12.takeReceiveEvents();
  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_MESSAGE_COMPLETED) {
    if (!_collectResponseCharacters()) {
      _transitionTo(PHASE_IDLE);
      return;
    }
    receiveEvents |= _sdi12.takeReceiveEvents();
    _lastBusActivityTime = micros();
  }

  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_FRAMING_ERROR) {
    _setResult(RESULT_FRAMING_ERROR);
    _transitionTo(PHASE_IDLE);
    return;
  }
  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_BUFFER_OVERFLOW) {
    _setResult(RESULT_BUFFER_OVERFLOW);
    _transitionTo(PHASE_IDLE);
    return;
  }

  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_MESSAGE_COMPLETED) {
    RecorderResult validationResult = _validateExpectedAddress(_address);
    if (validationResult == RESULT_SUCCESS &&
        (_responseLength != 3U || !_hasResponseTerminator())) {
      validationResult = RESULT_MALFORMED_RESPONSE;
    }
    if (validationResult != RESULT_SUCCESS) {
      _setResult(validationResult);
      _transitionTo(PHASE_IDLE);
      return;
    }

    _recordServiceRequest(_address, _lastBusActivityTime);
    _commitReadyMeasurementContext();
    _stripResponseEnvelope();
    _setResult(RESULT_SUCCESS);
    _transitionTo(PHASE_IDLE);
    return;
  }

  const uint32_t waitTime = static_cast<uint32_t>(_measurementSeconds) * 1000UL;
  if (millis() - _measurementWaitStart >= waitTime) {
    _setResult(RESULT_RESPONSE_TIMEOUT);
    _transitionTo(PHASE_IDLE);
  }
}

void SDI12Recorder::_phase_waiting_data_ready() {
  if (_dataContextSlot >= SDI12_RECORDER_CONTEXT_SLOTS ||
      !_contexts[_dataContextSlot].occupied ||
      _contexts[_dataContextSlot].address != _address) {
    _setResult(RESULT_CONTEXT_REQUIRED);
    _transitionTo(PHASE_IDLE);
    return;
  }

  MeasurementContext& context = _contexts[_dataContextSlot];
  if (!_readyAtReached(context.readyAt)) { return; }

  context.phase = CONTEXT_PHASE_DATA_READY;
  _txnStartTime = millis();
  _transitionTo(PHASE_BREAK);
}

void SDI12Recorder::_phase_deciding_retry() {
  // Determine if we should retry or terminate based on:
  // 1. The failure reason
  // 2. Elapsed time since last command stop bit
  // 3. Retry attempt and break sequence counters

  uint32_t elapsedMicrosSinceStop = micros() - _commandStopTime;

  // Retain a substantive response failure when later retries receive nothing. This
  // keeps a malformed, framing, CRC, or overflow result from being hidden by the
  // final no-response attempt.
  if (_result != RESULT_NONE && _result != RESULT_NO_RESPONSE &&
      _retryFailureResult == RESULT_NONE) {
    _retryFailureResult = _result;
  }

  // For no-response failures, enforce the retry window:
  // - Earliest retry: 16.67 ms after stop bit
  // - Latest retry without break: 87 ms after stop bit
  if (_result == RESULT_NO_RESPONSE) {
    const uint32_t RETRY_NO_RESPONSE_MIN_MICROS = 16670;
    const uint32_t RETRY_NO_RESPONSE_MAX_MICROS = 87000;

    // Too early to retry?
    if (elapsedMicrosSinceStop < RETRY_NO_RESPONSE_MIN_MICROS) {
      return;  // Wait longer
    }

    // Can we retry without a new break?
    if (elapsedMicrosSinceStop <= RETRY_NO_RESPONSE_MAX_MICROS && _attemptCount < 2) {
      // Yes; increment attempt counter and try again
      _attemptCount++;
      _resetAttemptReception();
      _transitionTo(PHASE_PRECOMMAND_MARK);
      return;
    }

    // Too late in the window, or too many attempts. Need a new break.
    if (_sequenceCount < 3U) {
      _sequenceCount++;
      _attemptCount = 0;
      _resetAttemptReception();
      _transitionTo(PHASE_BREAK);
      return;
    }

    // All retries exhausted
    if (_retryFailureResult != RESULT_NONE) { _setResult(_retryFailureResult); }
    _transitionTo(PHASE_IDLE);
    return;
  }

  // For other failures (framing, wrong address, etc.), retry without a new break
  if (_attemptCount < 2) {
    _attemptCount++;
    _resetAttemptReception();
    _transitionTo(PHASE_PRECOMMAND_MARK);
    return;
  }

  // Too many attempts without break; need a new break
  if (_sequenceCount < 3U) {
    _sequenceCount++;
    _attemptCount = 0;
    _resetAttemptReception();
    _transitionTo(PHASE_BREAK);
    return;
  }

  // All retries exhausted; transaction ends with failure
  _transitionTo(PHASE_IDLE);
}
