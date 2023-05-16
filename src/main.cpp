#include <hidboot.h>

#define SerialDebug Serial1
#define SerialConsole Serial1

// ASCII output goes to this console UART port which might be different from
// the debug port.
#define consbegin(...) SerialConsole.begin(__VA_ARGS__)
#define consprint(...) SerialConsole.print(__VA_ARGS__)
#define conswrite(...) SerialConsole.write(__VA_ARGS__)
#define consprintln(...) SerialConsole.println(__VA_ARGS__)

#define DEBUG_ON 0
#if DEBUG_ON
// #define dbbegin(...) SerialDebug.begin(__VA_ARGS__)
// #define dbprint(...) SerialDebug.print(__VA_ARGS__)
// #define dbprintln(...) SerialDebug.println(__VA_ARGS__)
#define dbbegin(...)
#define dbprint(...) consprint(__VA_ARGS__)
#define dbprintln(...) consprintln(__VA_ARGS__)
#else
#define dbbegin(...)
#define dbprint(...)
#define dbprintln(...)
#endif

const int kPingInterval = 3700;
unsigned long last_ping = 0;

class KbdRptParser : public KeyboardReportParser
{
  void PrintKey(uint8_t mod, uint8_t key);

protected:
  void OnControlKeysChanged(uint8_t before, uint8_t after);

  void OnKeyDown(uint8_t mod, uint8_t key);
  void OnKeyUp(uint8_t mod, uint8_t key);
  void OnKeyPressed(uint8_t key);
};

void KbdRptParser::PrintKey(uint8_t m, uint8_t key)
{
  MODIFIERKEYS mod;
  *((uint8_t *)&mod) = m;
  dbprint((mod.bmLeftCtrl == 1) ? "C" : " ");
  dbprint((mod.bmLeftShift == 1) ? "S" : " ");
  dbprint((mod.bmLeftAlt == 1) ? "A" : " ");
  dbprint((mod.bmLeftGUI == 1) ? "G" : " ");

  dbprint(" >");
  dbprint(key, HEX);
  // PrintHex<uint8_t>(key, 0x80);
  dbprint("< ");

  dbprint((mod.bmRightCtrl == 1) ? "C" : " ");
  dbprint((mod.bmRightShift == 1) ? "S" : " ");
  dbprint((mod.bmRightAlt == 1) ? "A" : " ");
  dbprintln((mod.bmRightGUI == 1) ? "G" : " ");
};

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
  dbprint("DN ");
  PrintKey(mod, key);
  uint8_t c = OemToAscii(mod, key);

  if (c)
  {
    OnKeyPressed(c);
    last_ping = millis();
  }
  else
  {
    switch (key)
    {
    case 0x4b: // Page up
      conswrite(0xe5);
      conswrite(0xf5);
      break;
    case 0x4e: // Page down
      conswrite(0xe4);
      conswrite(0xf4);
      break;
    case 0x50: // Cursor left
      conswrite(0xe1);
      conswrite(0xf1);
      break;
    case 0x4f: // Cursor right
      conswrite(0xe0);
      conswrite(0xf0);
      break;
    case 0x52: // Cursor up
      conswrite(0xe2);
      conswrite(0xf2);
      break;
    case 0x51: // Cursor down
      conswrite(0xe3);
      conswrite(0xf3);
      break;
    default:
      consprint("[0x");
      consprint(key, HEX);
      consprint("]");
    }
  }
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after)
{

  MODIFIERKEYS beforeMod;
  *((uint8_t *)&beforeMod) = before;

  MODIFIERKEYS afterMod;
  *((uint8_t *)&afterMod) = after;

  if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl)
  {
    dbprintln("LeftCtrl changed");
  }
  if (beforeMod.bmLeftShift != afterMod.bmLeftShift)
  {
    dbprintln("LeftShift changed");
  }
  if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt)
  {
    dbprintln("LeftAlt changed");
  }
  if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI)
  {
    dbprintln("LeftGUI changed");
  }

  if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl)
  {
    dbprintln("RightCtrl changed");
  }
  if (beforeMod.bmRightShift != afterMod.bmRightShift)
  {
    dbprintln("RightShift changed");
  }
  if (beforeMod.bmRightAlt != afterMod.bmRightAlt)
  {
    dbprintln("RightAlt changed");
  }
  if (beforeMod.bmRightGUI != afterMod.bmRightGUI)
  {
    dbprintln("RightGUI changed");
  }
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
  dbprint("UP ");
  PrintKey(mod, key);
}

void KbdRptParser::OnKeyPressed(uint8_t key)
{
  dbprint("ASCII: ");
  dbprintln((char)key);
  conswrite((char)key);
};

USBHost UsbH;
HIDBoot<HID_PROTOCOL_KEYBOARD> HidKeyboard(&UsbH);

KbdRptParser Prs;

unsigned int JOY_PINS[] = {PIN_JOY1_UP, PIN_JOY1_DN, PIN_JOY1_L, PIN_JOY1_R, PIN_JOY1_B1, PIN_JOY1_B2, PIN_JOY2_UP, PIN_JOY2_DN, PIN_JOY2_L, PIN_JOY2_R, PIN_JOY2_B1, PIN_JOY2_B2};

void setup()
{
  // Turn off built-in RED LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Set input pins
  for (auto pin : JOY_PINS)
  {
    pinMode(pin, INPUT_PULLUP);
    dbprint("init pin ");
    dbprintln(pin);
  }

  dbbegin(115200);
  dbprintln("Start");
  consbegin(6992);

  if (UsbH.Init())
    dbprintln("USB host did not start.");

  delay(200);
  conswrite(0x95);

  HidKeyboard.SetReportParser(0, &Prs);
}

void SendPing()
{
  conswrite(0x94);
  dbprintln("PING");
}

unsigned char isPressed(uint32_t pin)
{
  return digitalRead(pin) == LOW ? 1 : 0;
}

unsigned char getJoyState(int stick)
{
  unsigned char state = 0b10100000;

  state |= isPressed(PIN_JOY1_B1) << 4;
  state |= isPressed(PIN_JOY1_UP) << 3;
  state |= isPressed(PIN_JOY1_R) << 2;
  state |= isPressed(PIN_JOY1_DN) << 1;
  state |= isPressed(PIN_JOY1_L);

  return state;
}

unsigned char joyState = 0b10100000;

void loop()
{
  UsbH.Task();

  if (millis() - last_ping > kPingInterval)
  {
    SendPing();
    last_ping = millis();
  }

  unsigned char newJoyState = getJoyState(1);
  if (joyState != newJoyState)
  {
    joyState = newJoyState;
    // Output joystick state change
    conswrite(0x80);
    conswrite(joyState);
  }
}
