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
    _address('0'),
    _txnStartTime(0),
    _txnTimeoutMs(kDefaultTransactionTimeoutMs),
    _phase(PHASE_IDLE),
    _phaseStarted(false),
    _phaseStartTime(0),
    _commandStopTime(0),
    _breakEndTime(0),
    _breakCount(0),
    _attemptCount(0),
    _responseLength(0),
    _result(RESULT_NONE) {
  _response[0] = '\0';
}

SDI12Recorder::~SDI12Recorder() {
  if (_sdi12.isActive()) { cancel(); }
}

void SDI12Recorder::startCommand(char address, const String& command) {
  _address      = address;
  _command      = command;
  _txnStartTime = millis();
  _txnTimeoutMs = kDefaultTransactionTimeoutMs;

  _breakCount   = 0;
  _attemptCount = 0;
  clearResponse();
  _result = RESULT_NONE;

  // Always start with the break phase; the phase skips the break when policy forbids
  // it.
  _transitionTo(PHASE_BREAK);

  // Make this recorder the active SDI12 instance
  if (!_sdi12.isActive()) { _sdi12.setActive(); }
}

void SDI12Recorder::update() {
  // Check for overall transaction timeout
  if (_phase != PHASE_IDLE) {
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
    case PHASE_VALIDATING: _phase_validating(); break;
    case PHASE_DECIDING_RETRY: _phase_deciding_retry(); break;
  }
}

void SDI12Recorder::cancel() {
  _setResult(RESULT_CANCELLED);
  _transitionTo(PHASE_IDLE);
}

const char* SDI12Recorder::resultDescription() const {
  switch (_result) {
    case RESULT_NONE: return "";
    case RESULT_SUCCESS: return "SUCCESS";
    case RESULT_NO_RESPONSE: return "NO_RESPONSE";
    case RESULT_FRAMING_ERROR: return "FRAMING_ERROR";
    case RESULT_WRONG_ADDRESS: return "WRONG_ADDRESS";
    case RESULT_CRC_ERROR: return "CRC_ERROR";
    case RESULT_BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
    case RESULT_INVALID_FORMAT: return "INVALID_FORMAT";
    case RESULT_RESPONSE_TIMEOUT: return "RESPONSE_TIMEOUT";
    case RESULT_TRANSACTION_TIMEOUT: return "TRANSACTION_TIMEOUT";
    case RESULT_CANCELLED: return "CANCELLED";
    default: return "UNKNOWN";
  }
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

bool SDI12Recorder::_shouldTransmitBreak() const {
  // Require a break until command and bus context can safely determine otherwise.
  return true;
}

void SDI12Recorder::_transitionTo(Phase nextPhase) {
  _phase          = nextPhase;
  _phaseStarted   = false;
  _phaseStartTime = micros();
}

void SDI12Recorder::_setResult(RecorderResult reason) {
  _result = reason;
}

// ============================================================================
// Phase implementations
// ============================================================================

void SDI12Recorder::_phase_break() {
  if (!_shouldTransmitBreak()) {
    _transitionTo(PHASE_PRECOMMAND_MARK);
    return;
  }

  if (!_phaseStarted) {
    _sdi12.startBreak();
    _phaseStartTime = micros();
    _phaseStarted   = true;
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
  _sdi12.writeChar(_address, true);
  for (unsigned int i = 0; i < _command.length(); ++i) {
    _sdi12.writeChar(_command[i], true);
  }
  _sdi12.writeChar('!', true);
  _sdi12.forceListen();
  _commandStopTime = micros();
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
    _transitionTo(PHASE_RECEIVING);
    return;
  }

  // Maximum time to wait for response start (16.67 ms per Appendix B)
  const uint32_t RESPONSE_START_MAX_MICROS = 16670 + 400;  // +tolerance

  if (elapsedMicros > RESPONSE_START_MAX_MICROS) {
    // No response received in time
    _setResult(RESULT_NO_RESPONSE);
    _transitionTo(PHASE_DECIDING_RETRY);
  }
}

void SDI12Recorder::_phase_receiving() {
  // Collect available characters from the transport buffer
  while (_sdi12.available() > 0) {
    const int received = _sdi12.read();
    if (received < 0) {
      _setResult(RESULT_FRAMING_ERROR);
      _transitionTo(PHASE_DECIDING_RETRY);
      return;
    }

    if (_responseLength + 1 >= SDI12_RECORDER_RESPONSE_BUFFER_SIZE) {
      _sdi12.clearBuffer();
      _setResult(RESULT_BUFFER_OVERFLOW);
      _transitionTo(PHASE_DECIDING_RETRY);
      return;
    }

    _response[_responseLength++] = static_cast<char>(received);
    _response[_responseLength]   = '\0';
  }

  uint8_t receiveEvents = _sdi12.takeReceiveEvents();

  // Check for errors
  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_FRAMING_ERROR) {
    _setResult(RESULT_FRAMING_ERROR);
    _transitionTo(PHASE_DECIDING_RETRY);
    return;
  }

  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_BUFFER_OVERFLOW) {
    _setResult(RESULT_BUFFER_OVERFLOW);
    _transitionTo(PHASE_DECIDING_RETRY);
    return;
  }

  // Check for completion (CR LF) only after ruling out coalesced receive errors.
  if (receiveEvents & SDI12::SDI12_RECEIVE_EVENT_MESSAGE_COMPLETED) {
    _transitionTo(PHASE_VALIDATING);
    return;
  }

  // Check for response timeout (command-specific max time).
  // For most commands, this is around 1 second. For measurement commands,
  // it depends on the sensor's advertised measurement time (ttt).
  uint32_t elapsedMs = millis() - _txnStartTime;
  if (elapsedMs > 2000) {  // Conservative upper bound
    _setResult(RESULT_RESPONSE_TIMEOUT);
    _transitionTo(PHASE_DECIDING_RETRY);
    return;
  }
}

void SDI12Recorder::_phase_validating() {
  // Validate the response
  // 1. Check address matches
  if (_responseLength < 1 || _response[0] != _address) {
    _setResult(RESULT_WRONG_ADDRESS);
    _transitionTo(PHASE_DECIDING_RETRY);
    return;
  }

  // 2. Check format (depends on command type)
  // For now, just accept any response with valid address
  // Future: validate specific response formats per command type

  // 3. Check CRC when the command-derived response policy requires one.

  // Strip the response terminator after it has served its framing purpose.
  if (_responseLength >= 2 && _response[_responseLength - 2] == '\r' &&
      _response[_responseLength - 1] == '\n') {
    _responseLength -= 2;
    _response[_responseLength] = '\0';
  }

  // Once validated, the address is protocol envelope rather than response payload.
  // Command-aware CRC validation will similarly remove the three CRC characters before
  // exposing the payload.
  _responseLength--;
  memmove(_response, _response + 1, _responseLength);
  _response[_responseLength] = '\0';

  // Success!
  _setResult(RESULT_SUCCESS);
  _transitionTo(PHASE_IDLE);
}

void SDI12Recorder::_phase_deciding_retry() {
  // Determine if we should retry or terminate based on:
  // 1. The failure reason
  // 2. Elapsed time since last command stop bit
  // 3. Retry attempt and break sequence counters

  uint32_t elapsedMicrosSinceStop = micros() - _commandStopTime;

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
      clearResponse();
      _transitionTo(PHASE_PRECOMMAND_MARK);
      return;
    }

    // Too late in the window, or too many attempts. Need a new break.
    if (_shouldTransmitBreak() && _breakCount < 3) {
      _attemptCount = 0;
      clearResponse();
      _transitionTo(PHASE_BREAK);
      return;
    }

    // All retries exhausted
    _transitionTo(PHASE_IDLE);
    return;
  }

  // For other failures (framing, wrong address, etc.), retry without a new break
  if (_attemptCount < 2) {
    _attemptCount++;
    clearResponse();
    _transitionTo(PHASE_PRECOMMAND_MARK);
    return;
  }

  // Too many attempts without break; need a new break
  if (_shouldTransmitBreak() && _breakCount < 3) {
    _attemptCount = 0;
    clearResponse();
    _transitionTo(PHASE_BREAK);
    return;
  }

  // All retries exhausted; transaction ends with failure
  _transitionTo(PHASE_IDLE);
}
