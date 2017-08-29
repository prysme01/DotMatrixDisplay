// Example showing use of the MD_KeySwitch library
// 
// Momentary switch 
// 
// Prints the switch value on the Serial Monitor
// Allows setting of options to see theior effect (see setup())
//
#include <MD_KeySwitch.h>

const uint8_t SWITCH_PIN = 4;       // switch connected to this pin
const uint8_t SWITCH_ACTIVE = LOW;  // digital signal when switch is pressed 'on'

MD_KeySwitch S(SWITCH_PIN, SWITCH_ACTIVE);

void setup(void)
{
  Serial.begin(57600);
  Serial.print("\n[MD_KeySwitch example]");

  S.begin();
  S.enableDoublePress(true);
  S.enableLongPress(true);
  S.enableRepeat(true);
  S.enableRepeatResult(true);
}

void loop(void)
{
  switch(S.read())
  {
    case MD_KeySwitch::KS_NULL:       /* Serial.println("NULL"); */   break;
    case MD_KeySwitch::KS_PRESS:      Serial.print("\nSINGLE PRESS"); break;
    case MD_KeySwitch::KS_DPRESS:     Serial.print("\nDOUBLE PRESS"); break;
    case MD_KeySwitch::KS_LONGPRESS:  Serial.print("\nLONG PRESS");   break;
    case MD_KeySwitch::KS_RPTPRESS:   Serial.print("\nREPEAT PRESS"); break;
    default:                          Serial.print("\nUNKNOWN");      break;
  }
}
