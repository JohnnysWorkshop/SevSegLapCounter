#include "SevSeg.h"
SevSeg sevseg;
#include <RotaryEncoder.h>

RotaryEncoder *ENCODER = nullptr;

const byte LANE_PIN = A0; //dropdown 10k resistor
const byte CLK_PIN = A1;  //Rotary-Pin-B <- 10k resistor <- 5V
const byte DT_PIN = A2; //Rotary-Pin-A <- 10k resistor <- 5V
const byte SW_PIN = A3; //Rotary-Switch-Pin & Other-Rotary-Switch-Pin -> GND
const byte RELAY_PIN = A4; //Not in use in this project but already programmed
const byte BUZZER_PIN = A5; //Wiring will depend if Active (no resistor) or Passive (220 resistor)

int BTN_STATE;
int LAST_CLK_STATE;
unsigned long BTN_DEBOUNCE = 0;

const int LAP_LIMIT_DEFAULT = 10; //default number of laps, will reset to this value every time
int LAPS_LIMIT = LAP_LIMIT_DEFAULT;

//Message scrolling handling
char *GET_READY[] = {"   g", "  ge", " get", "get ", "et r", "t re", " rea", "read", "eady", "ady ", "dy  ", "y   ", "    "};
char *JOHNNY_S_WORKSHOP[] = {"   j", "  jo", " joh", "john", "ohnn", "hnny", "nny_", "ny_s", "y_s ", "_s u", "s uu", " uuo", "uuor", "uork", "orks", "rksh", "ksho", "shop", "hop ", "op  ", "p   ", "    "};
char *THREE_TWO_ONE_GO[] = {"3333", "2222", "1111", "-go-"};
int SCROLLING = 125;
int COUNTDOWN = 1000;
unsigned long LAST_REFRESH_TIME = 0;
int CURRENT_INDEX = 0;
bool SCROLLING_GET_READY = false;
bool SCROLLING_JOHNNY_S_WORKSHOP = true;
bool COUNT_COUNTDOWN = false;

//timer & lap-counting handling
int LAST_LANE_STATE = {0};
int LAP_COUNT = {0};
unsigned long LAST_LAP_TIME;
float TOTAL_TIME_START;
float TOTAL_TIME_FINISH;
float TOTAL_TIME;
unsigned long LAP_TIME;
unsigned long START_TIME;
unsigned long PERSONAL_BEST;
float LAST_TIME;
float BEST_LAP_TIME;
boolean FIRST_LAP;
boolean NEW_BEST_LAP;

const unsigned long MIN_LAP_TIME = 1000;  //set to 1 second, it will only trigger laps again after 1 second. adjust if needed
//const int SENSOR_CALIBRATION = 450; // not in use in this example, only for proximity sensor

boolean SETUP_LAPS = true;
boolean RACE_STARTED = false;
boolean RACE_FINISHED = false;
boolean END_FLAG;

void checkPosition() {  //rotary encoder function
  ENCODER->tick();
}

void setup() {
  ENCODER = new RotaryEncoder(DT_PIN, CLK_PIN, RotaryEncoder::LatchMode::FOUR3);  //rotary encoder function
  attachInterrupt(digitalPinToInterrupt(DT_PIN), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), checkPosition, CHANGE);

//four digits sevent segments handling
  byte numDigits = 4; //number of seven segment digits
  byte digitPins[] = {5, 6, 7, 8}; //pins of each digit ID (order is: dig-4, dig-3, dig-2, & dig-1)
  byte segmentPins[] = {4, 2, 12, 10, 9, 3, 13, 11}; //pins of each LED segment (order is: A, B, C, D, E, F, G & DP)

  bool resistorsOnSegments = true;
  bool updateWithDelaysIn = false;
  byte hardwareConfig = COMMON_ANODE; //set this to match with your display
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(90);

  pinMode(LANE_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(CLK_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  LAST_CLK_STATE = digitalRead(CLK_PIN);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  START_TIME = millis();
  PERSONAL_BEST = 9999;
  LAST_TIME = 00.00;
  BEST_LAP_TIME = 00.00;
  TOTAL_TIME_START = 00.00;
  TOTAL_TIME_FINISH = 00.00;
  TOTAL_TIME = 00.00;
  FIRST_LAP = true;
  NEW_BEST_LAP = false;
  digitalWrite(RELAY_PIN, LOW);
}

void loop() {

  BTN_STATE = digitalRead(SW_PIN);

  //greeting message, only do it every time it's switched back on
  if (SCROLLING_JOHNNY_S_WORKSHOP) {
    unsigned long CURRENT_MILLIS = millis();
    if (CURRENT_MILLIS - LAST_REFRESH_TIME >= SCROLLING) {
      LAST_REFRESH_TIME = CURRENT_MILLIS;
      sevseg.setChars(JOHNNY_S_WORKSHOP[CURRENT_INDEX]);
      CURRENT_INDEX++;
      if (CURRENT_INDEX >= (sizeof(JOHNNY_S_WORKSHOP) / sizeof(JOHNNY_S_WORKSHOP[0]))) {
        SCROLLING_JOHNNY_S_WORKSHOP = false;
        CURRENT_INDEX = 0;
      }
    }
    refresh();
    return;
  }

  if (SETUP_LAPS) {
    sevseg.setNumber(LAPS_LIMIT);
    RACE_FINISHED = false;
    RACE_STARTED = false;
    refresh();

    ENCODER->tick();

    int change = ENCODER->getPosition();

    if (change != 0) {  //adjust number of laps by turning rotary encoder
      ENCODER->setPosition(0);
      LAPS_LIMIT = constrain(LAPS_LIMIT - change, 1, 99);
    }
    if (BTN_STATE == LOW) { //enter set number of laps and starts countdown
      if (millis() - BTN_DEBOUNCE > 50) {
        beep();
        sevseg.blank();
        SETUP_LAPS = false;
        SCROLLING_GET_READY = true;
      }
      BTN_DEBOUNCE = millis();
    }
  }
  //GET READY message, as the race is about to start
  if (SCROLLING_GET_READY) {
    digitalWrite(RELAY_PIN, HIGH);
    unsigned long CURRENT_MILLIS = millis();
    if (CURRENT_MILLIS - LAST_REFRESH_TIME >= SCROLLING) {
      LAST_REFRESH_TIME = CURRENT_MILLIS;
      sevseg.setChars(GET_READY[CURRENT_INDEX]);
      CURRENT_INDEX++;
      if (CURRENT_INDEX >= (sizeof(GET_READY) / sizeof(GET_READY[0]))) {
        SCROLLING_GET_READY = false;
        CURRENT_INDEX = 0;
        COUNT_COUNTDOWN = true;
        RACE_STARTED = true;
      }
    }
    refresh();
    return;
  }
  //COUNTDOWN message, counts 3 to 1 and go!
  if (COUNT_COUNTDOWN) {
    unsigned long CURRENT_MILLIS = millis();
    if (CURRENT_MILLIS - LAST_REFRESH_TIME >= COUNTDOWN) {
      LAST_REFRESH_TIME = CURRENT_MILLIS;
      beep();
      sevseg.setChars(THREE_TWO_ONE_GO[CURRENT_INDEX]);
      CURRENT_INDEX++;
      if (CURRENT_INDEX >= (sizeof(THREE_TWO_ONE_GO) / sizeof(THREE_TWO_ONE_GO[0]))) {
        COUNT_COUNTDOWN = false;
        CURRENT_INDEX = 0;
      }
    }
    refresh();
    return;
  }
  if (RACE_STARTED && !RACE_FINISHED) {
    digitalWrite(RELAY_PIN, LOW);
    refresh();
    boolean NEW_DATA = false;
    int LANE_STATE = digitalRead(LANE_PIN);
    //if (LANE_STATE <= SENSOR_CALIBRATION && LAST_LANE_STATE > SENSOR_CALIBRATION) { //this line replaces the one below if using proximity sensor instead of a deadstrip
    if (LANE_STATE == HIGH) {
      if (millis() - LAST_LAP_TIME >= MIN_LAP_TIME) {
        LAP_COUNT++;
        LAST_LAP_TIME = millis();
        NEW_DATA = true;
        LAP_TIME = millis() - START_TIME;
        START_TIME = millis();
        LAST_TIME = LAP_TIME / 1000.00;
        if (FIRST_LAP) {
          TOTAL_TIME_START = millis();
          LAP_COUNT = 0;
          sevseg.setChars("----");
        }
        if (FIRST_LAP != true) {
          sevseg.setNumber(LAP_COUNT);  //display current lap when not a PB
        }
        if (LAP_TIME < PERSONAL_BEST && FIRST_LAP != true) {
          PERSONAL_BEST = LAP_TIME;
          BEST_LAP_TIME = LAST_TIME;
          NEW_BEST_LAP = true;
          //adjusts decimal point depending on personal-best time
          if (BEST_LAP_TIME >= 100.00) sevseg.setNumberF(BEST_LAP_TIME, 1);
          else if (BEST_LAP_TIME >= 10.00 && BEST_LAP_TIME < 100.00) sevseg.setNumberF(BEST_LAP_TIME, 2);
          else if (BEST_LAP_TIME < 10.00)sevseg.setNumberF(BEST_LAP_TIME, 3);
        }
        beep();
        FIRST_LAP = false;
        if (NEW_BEST_LAP == true) NEW_BEST_LAP = false;
      }
    }
    LAST_LANE_STATE = LANE_STATE;
    if (!NEW_DATA) return;
    if (LAP_COUNT >= LAPS_LIMIT) {
      RACE_FINISHED = true;
      END_FLAG = true;
    }
  }
  if (RACE_FINISHED) {
    refresh();
    if (END_FLAG) {
      TOTAL_TIME_FINISH = millis() - TOTAL_TIME_START;
      TOTAL_TIME_START = millis();
      TOTAL_TIME = TOTAL_TIME_FINISH / 1000.00;
      longBeep();
      RACE_STARTED = false;
      digitalWrite(RELAY_PIN, HIGH);
      sevseg.setChars(" END");
      END_FLAG = false;
    }
    ENCODER->tick();

    int change = ENCODER->getPosition();

    if (change != 0) {
      ENCODER->setPosition(0);
      if (change < 0) { //rotary clockwise turn: show total race time
        //adjusts decimal point depending on personal-best time
        if (TOTAL_TIME >= 100.00) sevseg.setNumberF(TOTAL_TIME, 1);
        else if (TOTAL_TIME >= 10.00 && TOTAL_TIME < 100.00) sevseg.setNumberF(TOTAL_TIME, 2);
        else if (TOTAL_TIME < 10.00) sevseg.setNumberF(TOTAL_TIME, 3);
      }
      if (change > 0) { //rotary anti-clockwise turn: show personal best
        //adjusts decimal point depending on personal-best time
        if (BEST_LAP_TIME >= 100.00) sevseg.setNumberF(BEST_LAP_TIME, 1);
        else if (BEST_LAP_TIME >= 10.00 && BEST_LAP_TIME < 100.00) sevseg.setNumberF(BEST_LAP_TIME, 2);
        else if (BEST_LAP_TIME < 10.00)sevseg.setNumberF(BEST_LAP_TIME, 3);
      }
    }
    if (BTN_STATE == LOW) { //return to menu
      if (millis() - BTN_DEBOUNCE > 50) {
        beep();
        resetRace();
      }
      BTN_DEBOUNCE = millis();
    }
  }
  refresh();
}

void refresh() {
  sevseg.refreshDisplay();
}
void beep() { //short beep
  tone(BUZZER_PIN, 2000, 100);
  tone(BUZZER_PIN, 4000, 100);
}
void longBeep(){
  tone(BUZZER_PIN, 2000, 500);
  tone(BUZZER_PIN, 4000, 500);
}
void resetRace() { //resets all data except for race number of laps
  RACE_FINISHED = false;
  SETUP_LAPS = true;
  LAP_COUNT = 0;
  PERSONAL_BEST = 9999;
  LAST_TIME = 00.00;
  BEST_LAP_TIME = 00.00;
  TOTAL_TIME_START = 00.00;
  TOTAL_TIME_FINISH = 00.00;
  TOTAL_TIME = 00.00;
  FIRST_LAP = true;
  NEW_BEST_LAP = false;
  digitalWrite(RELAY_PIN, LOW);
}
