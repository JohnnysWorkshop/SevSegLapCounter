#include "SevSeg.h"
SevSeg sevseg;
#include <RotaryEncoder.h>

RotaryEncoder *ENCODER = nullptr;

const byte LANE_PIN = A0;
const byte CLK_PIN = A1;
const byte DT_PIN = A2;
const byte SW_PIN = A3;
const byte RELAY_PIN = A4;
const byte BUZZER_PIN = A5;

int BTN_STATE;
int LAST_CLK_STATE;
unsigned long BTN_DEBOUNCE = 0;

const int LAP_LIMIT_DEFAULT = 10;
int LAPS_LIMIT = LAP_LIMIT_DEFAULT;

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
const unsigned long MIN_LAP_TIME = 1000;

boolean SETUP_LAPS = true;
boolean RACE_STARTED = false;
boolean RACE_FINISHED = false;
boolean END_FLAG;
const int SENSOR_CALIBRATION = 1;

void checkPosition() {
  ENCODER->tick();
}

void setup() {
  ENCODER = new RotaryEncoder(DT_PIN, CLK_PIN, RotaryEncoder::LatchMode::FOUR3);
  attachInterrupt(digitalPinToInterrupt(DT_PIN), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), checkPosition, CHANGE);

  byte numDigits = 4;
  byte digitPins[] = {10, 11, 12, 13};
  byte segmentPins[] = {9, 2, 3, 5, 6, 8, 7, 4};

  bool resistorsOnSegments = true;
  bool updateWithDelaysIn = false;
  byte hardwareConfig = COMMON_ANODE;
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

    if (change != 0) {
      ENCODER->setPosition(0);
      LAPS_LIMIT = constrain(LAPS_LIMIT - change, 1, 99);
    }
    if (BTN_STATE == LOW) {
      if (millis() - BTN_DEBOUNCE > 50) {
        beep();
        sevseg.blank();
        SETUP_LAPS = false;
        SCROLLING_GET_READY = true;
      }
      BTN_DEBOUNCE = millis();
    }
  }
  
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
    //if (LANE_STATE <= SENSOR_CALIBRATION && LAST_LANE_STATE > SENSOR_CALIBRATION) {
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
          sevseg.setNumber(LAP_COUNT);
        }
        if (LAP_TIME < PERSONAL_BEST && FIRST_LAP != true) {
          //beep();
          PERSONAL_BEST = LAP_TIME;
          BEST_LAP_TIME = LAST_TIME;
          NEW_BEST_LAP = true;
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
    beep();
    refresh();
    if (END_FLAG) {
      TOTAL_TIME_FINISH = millis() - TOTAL_TIME_START;
      TOTAL_TIME_START = millis();
      TOTAL_TIME = TOTAL_TIME_FINISH / 1000.00;
      RACE_STARTED = false;
      digitalWrite(RELAY_PIN, HIGH);
      sevseg.setChars(" END");
      END_FLAG = false;
    }
    ENCODER->tick();

    int change = ENCODER->getPosition();

    if (change != 0) {
      ENCODER->setPosition(0);
      if (change < 0) {
        if (TOTAL_TIME >= 100.00) sevseg.setNumberF(TOTAL_TIME, 1);
        else if (TOTAL_TIME >= 10.00 && TOTAL_TIME < 100.00) sevseg.setNumberF(TOTAL_TIME, 2);
        else if (TOTAL_TIME < 10.00) sevseg.setNumberF(TOTAL_TIME, 3);
      }
      if (change > 0) {
        if (BEST_LAP_TIME >= 100.00) sevseg.setNumberF(BEST_LAP_TIME, 1);
        else if (BEST_LAP_TIME >= 10.00 && BEST_LAP_TIME < 100.00) sevseg.setNumberF(BEST_LAP_TIME, 2);
        else if (BEST_LAP_TIME < 10.00)sevseg.setNumberF(BEST_LAP_TIME, 3);
      }
    }
    if (BTN_STATE == LOW) {
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
void beep() {
  tone(BUZZER_PIN, 2000, 100);
  tone(BUZZER_PIN, 4000, 100);
}
void resetRace() {
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
