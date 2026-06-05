#include <Arduino.h>

const int taster_1 = 2;
const int taster_2 = 3;
const int taster_3 = 4;
const int taster_4 = 5;

const int LED_y      = 6;
const int LED_r      = 7;
const int LED_succes = 8;
const int LED_1      = 9;
const int LED_2      = 10;
const int LED_3      = 11;
const int LED_4      = 12;

enum GameState {
  STATE_GENERATE,
  STATE_SHOW,
  STATE_INPUT,
  STATE_SUCCESS,
  STATE_WRONG,
  STATE_GAME_OVER
};

const int MAX_LEVEL = 100;
const int MAX_TRIES = 3;
int sequence[MAX_LEVEL];
int level       = 1;
int tries       = 0;
int input_index = 0;
int velocity    = 1000;
GameState state = STATE_GENERATE;

// Timer1 kida se na svakih 1ms zamena za delay
volatile bool         timer_fired        = false;
volatile unsigned int timer_ms_remaining = 0;
volatile int button_pressed = -1;

ISR(TIMER1_COMPA_vect) {
  if (timer_ms_remaining > 0) {
    timer_ms_remaining--;
  } else {
    timer_fired = true;
  }
}

void timer1_init() {
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC, prescaler 64
  OCR1A  = 249;                                         // 1ms at 16MHz
  TIMSK1 = (1 << OCIE1A);
  sei();
}

void timer_wait(unsigned int ms) {
  timer_fired        = false;
  timer_ms_remaining = ms - 1;
  while (!timer_fired);
}

ISR(INT0_vect) { if (button_pressed == -1) button_pressed = LED_1; } // pin 2
ISR(INT1_vect) { if (button_pressed == -1) button_pressed = LED_2; } // pin 3

ISR(PCINT2_vect) {  // PORTD covers pins 0-7, handles pin 4 and 5
  if (button_pressed != -1) return;
  if (digitalRead(taster_3) == LOW) button_pressed = LED_3;  // pin 4
  else if (digitalRead(taster_4) == LOW) button_pressed = LED_4;  // pin 5
}

void interrupts_init() {
  // INT0 (pin 2) and INT1 (pin 3) — falling edge
  EICRA = (1 << ISC01) | (1 << ISC11);
  EIMSK = (1 << INT0) | (1 << INT1);

  // Pin-Change on PORTD for pins 4 (PCINT20) and 5 (PCINT21)
  PCICR  |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21);
}

void all_leds_off() {
  digitalWrite(LED_1,      LOW);
  digitalWrite(LED_2,      LOW);
  digitalWrite(LED_3,      LOW);
  digitalWrite(LED_4,      LOW);
  digitalWrite(LED_y,      LOW);
  digitalWrite(LED_r,      LOW);
  digitalWrite(LED_succes, LOW);
}

void blink_red(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_r, HIGH);
    timer_wait(250);
    digitalWrite(LED_r, LOW);
    timer_wait(250);
  }
}

void generate_sequence() {
  randomSeed(millis());
  int led_map[4] = {LED_1, LED_2, LED_3, LED_4};
  for (int i = 0; i < MAX_LEVEL; i++) {
    sequence[i] = led_map[random(0, 4)];
  }
}

void show_sequence() {
  all_leds_off();
  timer_wait(500);

  for (int i = 0; i < level; i++) {
    digitalWrite(sequence[i], HIGH);
    timer_wait(velocity);
    digitalWrite(sequence[i], LOW);
    timer_wait(200);
  }
}

int wait_for_button() {
  button_pressed = -1;
  while (button_pressed == -1);  // ISR sets this
  int pressed = button_pressed;
  timer_wait(200);               // debounce
  button_pressed = -1;
  return pressed;
}

void setup() {
  pinMode(taster_1, INPUT_PULLUP);
  pinMode(taster_2, INPUT_PULLUP);
  pinMode(taster_3, INPUT_PULLUP);
  pinMode(taster_4, INPUT_PULLUP);

  pinMode(LED_y,      OUTPUT);
  pinMode(LED_r,      OUTPUT);
  pinMode(LED_succes, OUTPUT);
  pinMode(LED_1,      OUTPUT);
  pinMode(LED_2,      OUTPUT);
  pinMode(LED_3,      OUTPUT);
  pinMode(LED_4,      OUTPUT);

  timer1_init();
  interrupts_init();

  state = STATE_GENERATE;
}

void loop() {
  switch (state) {

    case STATE_GENERATE:
      generate_sequence();
      level    = 2;
      tries    = 0;
      velocity = 1000;
      state    = STATE_SHOW;
      break;

    case STATE_SHOW:
      show_sequence();
      input_index = 0;
      state = STATE_INPUT;
      break;

    case STATE_INPUT: {
      digitalWrite(LED_y, HIGH);  

      int pressed = wait_for_button();

      digitalWrite(pressed, HIGH);
      timer_wait(300);
      digitalWrite(pressed, LOW);

      if (pressed == sequence[input_index]) {
        input_index++;
        if (input_index == level) {
          digitalWrite(LED_y, LOW);
          state = STATE_SUCCESS;
        }
      } else {
        digitalWrite(LED_y, LOW);
        state = STATE_WRONG;
      }
      break;
    }

    case STATE_SUCCESS:
      tries = 0;
      all_leds_off();

      digitalWrite(LED_succes, HIGH);
      digitalWrite(LED_1, HIGH);
      digitalWrite(LED_2, HIGH);
      digitalWrite(LED_3, HIGH);
      digitalWrite(LED_4, HIGH);
      timer_wait(1000);
      all_leds_off();

      if (level < MAX_LEVEL) level++;
      if (velocity > 100)    velocity -= 50;

      state = STATE_SHOW;
      break;

    case STATE_WRONG:
      tries++;
      blink_red(tries); 

      if (tries >= MAX_TRIES) {
        state = STATE_GAME_OVER;
      } else {
        input_index = 0;
        state = STATE_SHOW;
      }
      break;

    case STATE_GAME_OVER:
      all_leds_off();
      digitalWrite(LED_r, HIGH);  

      wait_for_button();          

      digitalWrite(LED_r, LOW);
      state = STATE_GENERATE;
      break;
  }
}