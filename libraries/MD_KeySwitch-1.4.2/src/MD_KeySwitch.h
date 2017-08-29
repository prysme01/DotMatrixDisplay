#pragma once
/**
\mainpage MD_KeySwitch Library

Library to manage user keyswitch on digital input.

This is a library to allow the use of momentary push switches as user
input devices. It implements the following features:
- Detects press, double press, long press and auto repeat.
- Works with either either low/high or high/low transitions.
- Software debounce of initial keypress, configurable time period.
- Software double press detection, configurable time period.
- Software long press detection, configurable time period.
- Software auto repeat detection, configurable time period.

See Also
- \subpage pageRevisionHistory
- \subpage pageCopyright

\page pageCopyright Copyright
Copyright
---------
Copyright (C) 2017 Marco Colli. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

\page pageRevisionHistory Revision History
Revision History
----------------
Jun 2017 version 1.4.2
- Upgraded documentation and cleaned up coding style

Jan 2017 version 1.4.1
- Added enableRepeatResult

Nov 2016 version 1.4
- Early notification of long press if auto repeat is not enabled

Apr 2016 version 1.3
- Added facility to enable/disable presses

Feb 2016 version 1.2
- Added double press detection

Jun 2015 version 1.1
- Added long press and changed return code from boolean to enumerated type

Mar 2014 version 1.0
- New library
 */

#include <Arduino.h>

/**
 * \file
 * \brief Main header file and class definition for the MD_LibTemplate library
 */

// Default values for timed events.
// Note these are all off the same base (ie when the switch is first detected)
const uint16_t KEY_DEBOUNCE_TIME = 50;    ///< Default key debounce time in milliseconds
const uint16_t KEY_DPRESS_TIME = 250;     ///< Default double press time between presses in milliseconds
const uint16_t KEY_LONGPRESS_TIME = 350;  ///< Default long press detection time in milliseconds
const uint16_t KEY_REPEAT_TIME = 300;     ///< Default time between repeats in in milliseconds
const uint8_t KEY_ACTIVE_STATE = LOW;     ///< Default key is active low - transition high to low detection

// Bit enable/disable
const uint8_t REPEAT_RESULT_ENABLE = 3; ///< Internal status bit to return KS_REPEAT instead of KS_PRESS
const uint8_t DPRESS_ENABLE = 2;        ///< Internal status bit to enable double press
const uint8_t LONGPRESS_ENABLE = 1;     ///< Internal status bit to enable long press
const uint8_t REPEAT_ENABLE = 0;        ///< Internal status bit to enable repeat key

/**
 * Core object for the MD_KeySwitch library
 */
class MD_KeySwitch
{
public:
  //--------------------------------------------------------------
  /** \name Enumerated values and Typedefs.
  * @{
  /**
  * Return values for switch status
  *
  * The read() method returns one of these enumerated values as the
  * result of the switch transition detection.
  */
  enum keyResult_t
  {
    KS_NULL,        ///< No key press
    KS_PRESS,       ///< Simple press, or a repeated press sequence if enableRepeatResult(false) (default)
    KS_DPRESS,      ///< Double press
    KS_LONGPRESS,   ///< Long press
    KS_RPTPRESS     ///< Repeated key press (only if enableRepeatResult(true))
  };

  /** @} */
  //--------------------------------------------------------------
  /** \name Class constructor and destructor.
  * @{
  */
  /**
   * Class Constructor.
   *
   * Instantiate a new instance of the class. The parameters passed are 
   * used to the hardware interface to the switch.
   *
   * The option parameter onState telles the library which level 
   * (LOW or HIGH) should be considered the switch 'on' state. If the 
   * default LOW state is selected then the library will initialise the 
   * pin with INPUT_PULLUP and no external pullup resistors are necessary.
   * If specified HIGH, external pull down resistors will be required.
   *
   * @param pin   the digital pin to which the switch is connected.
   * @param onState   the state for the switch to be active
   */
  MD_KeySwitch(uint8_t pin, uint8_t onState = KEY_ACTIVE_STATE);

  /**
   * Class Destructor.
   *
   * Release allocated memory and does the necessary to clean up once the queue is
   * no longer required.
   */
  ~MD_KeySwitch() {};

  /** @} */
  //--------------------------------------------------------------
  /** \name Methods for core object control.
  * @{
  */
  /**
   * Initialize the object.
   *
   * Initialise the object data. This needs to be called during setup() to initialise new
   * data for the class that cannot be done during the object creation.
   */
  void begin(void);
  
  /**
   * Return the state of the switch
   *
   * Return one of the keypress types depending on what has been detected.
   * The timing for each keypress starts when the first transition of the 
   * switch from inactive to active state and is recognised by a finite 
   * state machine whose operation is directed by the timer and option 
   * values specified.
   *
   * @return one of the keyResult_t enumerated values
   */
  keyResult_t read(void);

  /** @} */
  //--------------------------------------------------------------
  /** \name Methods for object parameters and options.
  * @{
  */
  /**
   * Set the debounce time
   *
   * Set the switch debounce time in milliseconds.
   * The default value is set by the KEY_DEBOUNCE_TIME constant.
   *
   * Note that the relationship between timer values should be
   * Debounce time < Long Press Time < Repeat time. No checking 
   * is done in the to enforce this relationship.
   *
   * @param t the specified time in milliseconds.
   */
  inline void setDebounceTime(uint16_t t) 
    { _timeDebounce = t; };

  /**
   * Set the double press detection time
   *
   * Set the time between each press time in milliseconds. A double
   * press is detected if the switch is released and depressed within 
   * this time, measured from when the first press is detected.
   * The default value is set by the KEY_DPRESS_TIME constant.
   *
   * @param t the specified time in milliseconds.
   */
  inline void setDoublePressTime(uint16_t t)
    { _timeDoublePress = t; enableDoublePress(true); };

  /**
   * Set the long press detection time
   *
   * Set the time in milliseconds after which a continuous press 
   * and release is deemed a long press, measured from when the 
   * first press is detected.
   * The default value is set by the KEY_LONGPRESS_TIME constant.
   *
   * Note that the relationship betweentimer values should be
   * Debounce time < Long Press Time < Repeat time. No checking 
   * is done in the to enforce this relationship.
   *
   * @param t the specified time in milliseconds.
   */
  inline void setLongPressTime(uint16_t t)
    { _timeLongPress = t; enableLongPress(true); };

  /**
   * Set the repeat time
   *
   * Set the time in milliseconds after which a continuous press
   * and hold is treated as a stream of repeated presses, measured
   * from when the first press is detected.
   *
   * Note that the relationship between timer values should be
   * Debounce time < Long Press Time < Repeat time. No checking 
   * is done in the to enforce this relationship.
   *
   * @param t the specified time in milliseconds.
   */
  inline void setRepeatTime(uint16_t t)
    { _timeRepeat = t; enableRepeat(true); };

  /**
   * Enable double press detection
   *
   * Enable or disable double press detection. If disabled,
   * two single press are detected instead of a double press.
   * Default is to detect double press events.
   *
   * @param f true to enable, false to disable.
   */
  inline void enableDoublePress(boolean f)
    { if (f) bitSet(_enableFlags, DPRESS_ENABLE); else bitClear(_enableFlags, DPRESS_ENABLE); };

  /**
   * Enable long press detection
   *
   * Enable or disable long press detection. If disabled,
   * the long press notification is skipped when the event 
   * is detected and either a simple press or repeats are 
   * returned, depening on the setting of the other options.
   * Default is to detect long press events.
   *
   * @param f true to enable, false to disable.
   */
  inline void enableLongPress(boolean f)
    { if (f) bitSet(_enableFlags, LONGPRESS_ENABLE); else bitClear(_enableFlags, LONGPRESS_ENABLE); };

  /**
   * Enable repeat detection
   *
   * Enable or disable repeat detection. If disabled,
   * the long press notification is returned as soon as the
   * long press time has expired.
   * Default is to detect repeat events.
   *
   * @param f true to enable, false to disable.
   */
  inline void enableRepeat(boolean f)
    { if (f) bitSet(_enableFlags, REPEAT_ENABLE); else bitClear(_enableFlags, REPEAT_ENABLE); };

  /**
   * Modify repeat notification
   *
   * Modify the result returned from a repeat detection. 
   * If enabled, the first repeat will return a KS_PRESS and
   * subsequent repeats will return KS_RPTPRESS. If disabled 
   *(default) the repeats will be stream of KS_PRESS values.
   *
   * @param f true to enable, false to disable (default).
   */
  inline void enableRepeatResult(boolean f)
    { if (f) bitSet(_enableFlags, REPEAT_RESULT_ENABLE); else bitClear(_enableFlags, REPEAT_RESULT_ENABLE); };

  /** @} */

protected:
  /**
  * FSM state values
  *
  * States for the internal Finite State Machine to recognised the key press
  */
  enum state_t 
  { 
    S_IDLE,       ///< Idle state - waiting for key transition
    S_DEBOUNCE1,  ///< First press debounce 
    S_DEBOUNCE2,  ///< Second (double) press debounce
    S_PRESS,      ///< Detecting possible simple press
    S_DPRESS,     ///< Detecting possible double press
    S_LPRESS,     ///< Detecting possible long press
    S_REPEAT,     ///< Outputting repeat keys when timer expires
    S_WAIT        ///< Waiting for key to be released after long press is detected
  };

	uint8_t		_pin;		    ///< pin number
	uint8_t		_onState;		///< digital state for ON
	state_t		_state;			///< the FSM current state
	uint32_t	_timeActive;	///< the millis() time it was last activated
  uint8_t   _enableFlags; ///< functions enabled/disabled

  // Note that Debounce time < Long Press Time < Repeat time. No checking is done in the
  // library to enforce this relationship.
	uint16_t	_timeDebounce;  ///< debounce time in milliseconds
  uint16_t  _timeDoublePress; ///< double press detection time in milliseconds
  uint16_t  _timeLongPress; ///< long press time in milliseconds
	uint16_t	_timeRepeat;	  ///< repeat time delay in milliseconds
};
