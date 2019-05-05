#include <EEPROM.h>

#include <IRremote.h>
#include <HID-Project.h>
#include <TimerOne.h>

// Pin for turning off RX led (32u4 micro board)
#define RXLED                         17

// Pin where IR receiver is connected
#define recvPin                       4

// IR
IRrecv irrecv(recvPin);
decode_results  results;

// KEYBOARD
long prev_IR_code;
int8_t key_index;
uint8_t read_index;
long IR_key_learn[2]; 
#define VOL_UP                        0
#define VOL_DOWN                      1
#define VOL_FINE_UP                   2
#define VOL_FINE_DOWN                 3
#define VOL_MUTE                      4
#define BTN_PLAY_PAUSE                5
#define BTN_LEFT                      6
#define BTN_RIGHT                     7
#define KEYS_MAX                      7     // !!! this should have same value as last keyboard key and no more than 125
long IR_codes_array[KEYS_MAX + 1];
String key_names[] = {"VOL_UP", "VOL_DOWN", "VOL_FINE_UP", "VOL_FINE_DOWN", "VOL_MUTE", "PLAY_PAUSE", "ARROW_LEFT", "ARROW_RIGHT"};

// Running mode
uint8_t runmode;
#define RUNMODE_RECEIVE               0
#define RUNMODE_LEARN                 1

// Changes in control LED
uint8_t leds_changes_count;
uint8_t leds_prev_value;
unsigned long leds_last_change_millis;
#define CONTROLMODE_LED               LED_CAPS_LOCK
#define LEDCHANGES_THRESHOLD          10

// Display in LEARN mode
uint8_t display_ID;
#define DISPLAYID_NONE                0
#define DISPLAYID_INTRO               1
#define DISPLAYID_PRESSKEY1ST         2
#define DISPLAYID_PRESSKEY2ND         3
#define DISPLAYID_PRESSKEYOK          4
#define DISPLAYID_PRESSKEYERROR       5
#define DISPLAYID_DONE                99
#define DSPMSG_Intro                  "Learning mode activated, press keys in order:"
#define DSPMSG_PressKey1st            "press 1st time key "
#define DSPMSG_PressKey2nd            "press 2nd time key "
#define DSPMSG_PressKeyOK             "keys match!"
#define DSPMSG_PressKeyERROR          "keys do not match!"
#define DSPMSG_Done_NotSaved          "Settings NOT saved, returning to receiving mode!"
#define DSPMSG_Done_Saved             "Settings saved, returning to receiving mode!"

// Commands
#define COMMAND_NONE                  0
#define COMMAND_SWITCH_TO_LEARNING    1
#define COMMAND_SWITCH_TO_RECEIVE     2
#define COMMAND_PROCESS_KEY           3

//Timer
bool time_has_ticked;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *    S e t u p
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void setup() {
/*--------------------------------------------------
 *    Board setup
 *--------------------------------------------------*/
  pinMode(RXLED, OUTPUT);
  BootKeyboard.begin();
  Keyboard.begin();
  Consumer.begin();
  irrecv.enableIRIn();
/*--------------------------------------------------
 *    Initialize running mode
 *--------------------------------------------------*/
  runmode = RUNMODE_RECEIVE;
  display_ID = DISPLAYID_NONE;
  reset_led_change_vars();
  key_index = -1;
/*--------------------------------------------------
 *    Read codes from EEPROM
 *--------------------------------------------------*/
  EEPROM.get(0, IR_codes_array);
/*--------------------------------------------------
 *    Initialize timer every 5 seconds
 *--------------------------------------------------*/
  time_has_ticked = false;
  Timer1.initialize(3000000);
  Timer1.attachInterrupt(timetick);
  prev_IR_code = 0XFFFF;
}
/*--------------------------------------------------
 *    Timer routine callback
 *--------------------------------------------------*/
void timetick()
{
  time_has_ticked = true;
}
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *    L O O P
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void loop() {
  digitalWrite(RXLED, HIGH);
  TXLED1;

  check_mode();

  if(runmode == RUNMODE_LEARN)
  {
    if (time_has_ticked && key_index >= 0)
    {
      process_command(COMMAND_PROCESS_KEY);
      time_has_ticked = false;
    }
    if (display_ID != DISPLAYID_NONE)
    {
      show_display();
    }
  }

  if (irrecv.decode(&results)) {
    long IR_code;
    if(results.decode_type == RC5 || results.decode_type == NEC)
    {
      IR_code = 0xFF00 | 0xFF & results.value;
    }
    else
    {
      IR_code = 0xFFFF & results.value;
    }
    /*
    BootKeyboard.print("read code: ");
    BootKeyboard.print(results.decode_type);
    BootKeyboard.print("/");
    BootKeyboard.println(results.value);
    BootKeyboard.print("/");
    BootKeyboard.println(IR_code);
    //*/
    if(IR_code == 0xFFFF)
    {
      IR_code = prev_IR_code;
    }
    else
    {
      prev_IR_code = IR_code;
    }
    if(IR_code != 0xFFFF)
    {
      if(runmode == RUNMODE_LEARN)
      {
        if(IR_key_learn[read_index] == 0)
        {
          IR_key_learn[read_index] = IR_code;
        }
      }
      else
      {
        /*
        BootKeyboard.print(">");
        BootKeyboard.println(IR_code);
        //*/
        process_IR_code(IR_code);
      }
    }    
    irrecv.resume();   
  } 
}
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *    C h e c k   a n d   u p d a t e   r u n n i n g   m o d e
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*--------------------------------------------------
 *    Reset control led variables
 *--------------------------------------------------*/
void reset_led_change_vars(void)
{
  leds_changes_count = 1;
  leds_prev_value = 0;
  leds_last_change_millis = 0;
}
/*--------------------------------------------------
 *    Check control led changes
 *--------------------------------------------------*/
void check_mode()
{
  uint8_t leds = BootKeyboard.getLeds() & CONTROLMODE_LED;
  if (leds != leds_prev_value)
  {
    unsigned long millis_now = millis();
    if ((millis_now - leds_last_change_millis) < 1000)
    {
      leds_changes_count = leds_changes_count + 1;
      if ((leds_changes_count >= LEDCHANGES_THRESHOLD) || (runmode == RUNMODE_LEARN))
      {
        toggle_runmode();
      }
    }
    else
    {
      reset_led_change_vars();
    }
    leds_last_change_millis = millis_now;
    leds_prev_value = leds;
  }
}
/*--------------------------------------------------
 *    Update running mode
 *--------------------------------------------------*/
void toggle_runmode()
{  
  switch(runmode)
  {
    case RUNMODE_RECEIVE:
      runmode = RUNMODE_LEARN;
      process_command(COMMAND_SWITCH_TO_LEARNING);
      break;
    case RUNMODE_LEARN:
      process_command(COMMAND_SWITCH_TO_RECEIVE);
      break;
  }
}
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *    C O M M A N D S   P R O C E S S I N G
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*--------------------------------------------------
 *    Commands processing entry point
 *--------------------------------------------------*/
void process_command(uint8_t command_ID)
{
  switch(command_ID)
  {
    case COMMAND_SWITCH_TO_LEARNING:
      process_command_SWITCH_TO_LEARNING();
      break;
    case COMMAND_SWITCH_TO_RECEIVE:
      process_command_SWITCH_TO_RECEIVE();
      break;
    case COMMAND_PROCESS_KEY:
      process_command_COMMAND_PROCESS_KEY();
      break;
  }
  reset_led_change_vars();
}
/*--------------------------------------------------
 *    Process command SWITCH TO LEARNING
 *--------------------------------------------------*/
void process_command_SWITCH_TO_LEARNING(void)
{
  display_ID = DISPLAYID_INTRO;
}
/*--------------------------------------------------
 *    Process command SWITCH TO RECEIVE
 *--------------------------------------------------*/
void process_command_SWITCH_TO_RECEIVE(void)
{
  display_ID = DISPLAYID_DONE;
}
/*--------------------------------------------------
 *    Process command READ KEY
 *--------------------------------------------------*/
void process_command_COMMAND_PROCESS_KEY(void)
{
  if(key_index < 0)
  {
    return;
  }
  if(key_index > KEYS_MAX)
  {
    key_index = -1;
    EEPROM.put(0, IR_codes_array);
    display_ID = DISPLAYID_DONE;
    return;
  }
  if(IR_key_learn[0] == 0)
  {
    display_ID = DISPLAYID_PRESSKEY1ST;
    read_index = 0;
    return;
  }
  if(IR_key_learn[1] == 0)
  {
    display_ID = DISPLAYID_PRESSKEY2ND;
    read_index = 1;
    return;
  }
  if(IR_key_learn[0] == IR_key_learn[1])
  {
    display_ID = DISPLAYID_PRESSKEYOK;
    IR_codes_array[key_index] = IR_key_learn[0];
  }
  else
  {
    display_ID = DISPLAYID_PRESSKEYERROR;
  }
  IR_key_learn[0] = 0;
  IR_key_learn[1] = 0;
  read_index = 0;
}
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *    L e a r n i n g   m o d e,   d i s p l a y
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*--------------------------------------------------
 *    Show display entry point
 *--------------------------------------------------*/
void show_display(void)
{
  switch(display_ID)
  {
    case DISPLAYID_INTRO:
      show_display_intro();
      break;
    case DISPLAYID_PRESSKEY1ST:
      show_display_key_1st();
      break;
    case DISPLAYID_PRESSKEY2ND:
      show_display_key_2nd();
      break;
    case DISPLAYID_PRESSKEYOK:
      show_display_key_OK();
      break;
    case DISPLAYID_PRESSKEYERROR:
      show_display_key_ERROR();
      break;
    case DISPLAYID_DONE:
      show_display_done();
      break;
  }
  display_ID = DISPLAYID_NONE;
}
/*--------------------------------------------------
 *    Display I N T R O
 *--------------------------------------------------*/
void show_display_intro(void)
{
  BootKeyboard.println(DSPMSG_Intro);
  key_index = 0;
  read_index = 0;
  for(int i = 0; i < KEYS_MAX; i++)
  {
    IR_codes_array[i] = 0;
  }
  time_has_ticked = false;
}
/*--------------------------------------------------
 *    Display Press KEY 1st time
 *--------------------------------------------------*/
void show_display_key_1st(void)
{
  BootKeyboard.print(DSPMSG_PressKey1st);  
  BootKeyboard.println(key_names[key_index]);
}
/*--------------------------------------------------
 *    Display Press KEY 2nd time
 *--------------------------------------------------*/
void show_display_key_2nd(void)
{
  BootKeyboard.print(DSPMSG_PressKey2nd);  
  BootKeyboard.println(key_names[key_index]);
}
/*--------------------------------------------------
 *    Display Press KEY - COMPARE OK
 *--------------------------------------------------*/
void show_display_key_OK(void)
{
  BootKeyboard.println(DSPMSG_PressKeyOK);  
  key_index = key_index + 1;
  time_has_ticked = true;
}
/*--------------------------------------------------
 *    Display Press KEY - COMPARE ERROR
 *--------------------------------------------------*/
void show_display_key_ERROR(void)
{
  BootKeyboard.println(DSPMSG_PressKeyERROR);
  time_has_ticked = true;
}
/*--------------------------------------------------
 *    Display D O N E
 *--------------------------------------------------*/
void show_display_done(void)
{
  BootKeyboard.println(DSPMSG_Done_Saved);
  IR_key_learn[0] = 0;
  IR_key_learn[1] = 0;  
  read_index = 0;
  runmode = RUNMODE_RECEIVE;
}
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *    I N F R A R E D   R O U T I N E S
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*--------------------------------------------------
 *    Process IR code
 *--------------------------------------------------*/
void process_IR_code(long IR_code)
{
  for(int i = 0; i <= KEYS_MAX; i++)
  {
    if(IR_codes_array[i] == IR_code)
    {
      IR_code = i;
      break;
    }
  }  
  switch(IR_code)
  {
    case BTN_LEFT:
      Keyboard.press(KEY_LEFT_ARROW);
      delay(150);
      break;
    case BTN_RIGHT:
      Keyboard.press(KEY_RIGHT_ARROW);
      delay(150);
      break;
    case VOL_UP:
      Consumer.write(MEDIA_VOLUME_UP);
      break;
    case VOL_DOWN:
      Consumer.write(MEDIA_VOLUME_DOWN);
      break;
    case VOL_FINE_UP:
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.press(KEY_LEFT_ALT);
      Consumer.write(MEDIA_VOLUME_UP);
      break;
    case VOL_FINE_DOWN:
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.press(KEY_LEFT_ALT);
      Consumer.write(MEDIA_VOLUME_DOWN);
      break;
    case VOL_MUTE:
      Consumer.write(MEDIA_VOLUME_MUTE);
      delay(150);
      break;
    case BTN_PLAY_PAUSE:
      Keyboard.print(" ");
      break;
    default:
      break;
  }
  Keyboard.releaseAll();
  delay(100);
}
