#include <LiquidCrystal_I2C.h>

/* Constants */
#define MAX_PERIOD          (1000)
#define MAX_TIMEOUT         (15000)
#define PERIOD_MODIFIER     (50)
#define TIMEOUT_MODIFIER    (2000)
#define POLLING_READS       (10)
#define READ_DELAY_TIME     (MAX_PERIOD / POLLING_READS)
#define SEED_INCREMENT      (2)
#define MAX_TRANSITION_TIME (1000)
#define NUMBER_LEVELS       (10)
#define NUMBER_LIVES        (3)

#define HEART_CHAR          ((byte)0)
#define MOLE_CHAR           ((byte)1)
#define CHECK_CHAR          ((byte)2)
#define SKULL_CHAR          ((byte)3)
#define NO_BUTTON_PRESSED   (-1)

/* Maps */
const int leds[] = {11, 9, 7};
const int buttons[] = {12, 10, 8};

byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

byte mole[8] = {
  0b01110,
  0b11111,
  0b10101,
  0b11111,
  0b10001,
  0b11111,
  0b11111,
  0b00000
};

byte check[8] = {
  0b00000,
  0b00001,
  0b00011,
  0b10110,
  0b11100,
  0b01000,
  0b00000,
  0b00000
};

byte skull[8] = {
  0b00000,
  0b01110,
  0b10101,
  0b11011,
  0b01110,
  0b01110,
  0b00000,
  0b00000
};

/* Constant Macros */
#define NUMBER_LEDS               (sizeof(leds) / sizeof(int))
#define NUMBER_BUTTONS            (sizeof(buttons) / sizeof(int))

/* Access Macros */
#define WRITE_LED(i, v)           (digitalWrite(leds[i], v))
#define READ_BUTTON(i)            (!digitalRead(buttons[i]))
#define GET_NEXT_LED()            (rand() % NUMBER_LEDS)
#define CALCULATE_PTS(lvl, t)     ((lvl*10) + (t)/1000)

/* FSM States */
enum class states_t {
    /* Startup Mode */
    STARTUP,

    /* Game Flow */
    LEVEL_SETUP,
    LED_CHOOSE,
    WAIT_INPUT,

    /* Input Related */
    CORRECT_INPUT,
    WRONG_INPUT,

    /* Game Decision */
    VICTORY,
    DEFEAT,

    /* Others */
    TIMEOUT
};

/* Control Variables */
states_t state = states_t::STARTUP;
int lives = 3;
int level = 1;
int seed = 0;
int period = 0;
int timeout = 0;
int points = 0;
int current = 0;
bool update_lcd = true;
int timeout_counter = 0;

/* Project Variables */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* Project Functions */

int PollButtons() {
    for (int i = 0; i < NUMBER_BUTTONS; i++) {
        if (READ_BUTTON(i)) {
            return i;
        }
    }
    return NO_BUTTON_PRESSED;
}

int TurnOnLed(int led) {
    for (int i = 0; i < NUMBER_LEDS; i++) {
        if (i == led) {
            WRITE_LED(i, HIGH);
        } else {
            WRITE_LED(i, LOW);
        }
    }
}

int WriteAllLeds(int value) {
    for (int i = 0; i < NUMBER_LEDS; i++) {
        WRITE_LED(i, value);
    }
}

void UpdateLevelDisplay() {
    lcd.clear();
    
    lcd.setCursor(0, 0);
    lcd.print("  Whack'A Mole  ");

    lcd.setCursor(0, 1);

    /* Level */
    lcd.write(MOLE_CHAR);
    lcd.print('x');
    lcd.print(level / 10);
    lcd.print(level % 10);

    /* Separator */
    lcd.print(' ');

    lcd.write(HEART_CHAR);
    lcd.print('x');
    lcd.print(lives);

    /* Separator */
    lcd.print(' ');

    /* Level */
    lcd.print(points / 1000);
    lcd.print(points / 100);
    lcd.print(points / 10);
    lcd.print(points % 10);
    lcd.print("pts");
}

/* Arduino Functions */
void setup() {
    /* LEDs setup */
    for (int i = 0; i < NUMBER_LEDS; i++) {
        pinMode(leds[i], OUTPUT);
    }

    /* Buttons setup */
    for (int i = 0; i < NUMBER_BUTTONS; i++) {
        pinMode(buttons[i], INPUT_PULLUP);
    }

    /* LCD setup */
    lcd.init();
    lcd.backlight();
    lcd.createChar(HEART_CHAR, heart);
    lcd.createChar(MOLE_CHAR, mole);
    lcd.createChar(CHECK_CHAR, check);
    lcd.createChar(SKULL_CHAR, skull);
}

void loop() {
    switch (state) {
        case states_t::STARTUP:
            /* LCD Logic */
            if (update_lcd) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("  Whack'A Mole  ");
                lcd.setCursor(0, 1);
                lcd.print(" Press a button ");
                update_lcd = false;
            }

            /* Output Logic */
            level = 1;
            lives = NUMBER_LIVES;
            seed += SEED_INCREMENT;

            /* Transition Logic */
            if (PollButtons() != NO_BUTTON_PRESSED) {
                state = states_t::LEVEL_SETUP;
                update_lcd = true;
            }

            /* Time to wait */
            delay(READ_DELAY_TIME);
            break;

        case states_t::LEVEL_SETUP:
            /* LCD Logic */
            if (update_lcd) {
                UpdateLevelDisplay();
                update_lcd = false;
            }

            /* Output Logic */
            period = MAX_PERIOD - PERIOD_MODIFIER * level;
            timeout = MAX_TIMEOUT - TIMEOUT_MODIFIER * level;
            timeout_counter = 0;
            srand(seed);

            state = states_t::LED_CHOOSE;
            
            break;

        case states_t::LED_CHOOSE:
            current = GET_NEXT_LED();

            TurnOnLed(current);

            state = states_t::WAIT_INPUT;
            break;

        case states_t::WAIT_INPUT:

            for (int i = 0; i < period; i += period / POLLING_READS) {
                int button_pressed = PollButtons();

                if (button_pressed == current) {
                    state = states_t::CORRECT_INPUT;
                  	break;
                } else if (timeout_counter >= timeout) {
                    state = states_t::TIMEOUT;
                  	break;
                } else if (button_pressed != NO_BUTTON_PRESSED) {
                    state = states_t::WRONG_INPUT;
                  	break;
                } else {
                  	state = states_t::LED_CHOOSE;
                }

                delay(period / POLLING_READS);
                timeout_counter += period / POLLING_READS;
            }

            break;

        case states_t::CORRECT_INPUT:
        {
            WriteAllLeds(HIGH);

            int level_points = CALCULATE_PTS(level, timeout - timeout_counter);

            lcd.clear();

            lcd.setCursor(0, 0);
            lcd.write(CHECK_CHAR);
            lcd.print("  BOOYAH!!!!  ");
            lcd.write(CHECK_CHAR);

            lcd.setCursor(0, 1);
            lcd.print("Gained +");
            lcd.print(level_points / 1000);
            lcd.print(level_points / 100);
            lcd.print(level_points / 10);
            lcd.print(level_points % 10);
            lcd.print("pts ");

            points += level_points;
            level += 1;
            update_lcd = true;
          
          	if (level > NUMBER_LEVELS) {
                state = states_t::VICTORY;
            } else {
                state = states_t::LEVEL_SETUP;
            }

            delay(MAX_TRANSITION_TIME);
            break;
        }

        case states_t::WRONG_INPUT:
            WriteAllLeds(LOW);

            lcd.clear();

            lcd.setCursor(0, 0);
            lcd.write(SKULL_CHAR);
            lcd.print(" YOU MISSED!! ");
            lcd.write(SKULL_CHAR);

            lcd.setCursor(0, 1);
            lcd.print("    Lost a ");
            lcd.write(HEART_CHAR);
            lcd.print("    ");

            update_lcd = true;
            lives -= 1;

            if(lives == 0) {
              state = states_t::DEFEAT;
            }
            else {
              state = states_t::LEVEL_SETUP;
            }

            delay(MAX_TRANSITION_TIME);
            break;

        case states_t::TIMEOUT:  
            WriteAllLeds(LOW);

            lcd.clear();

            lcd.setCursor(0, 0);
            lcd.write(SKULL_CHAR);
            lcd.write(SKULL_CHAR);
            lcd.print(" TIME-OUT!! ");
            lcd.write(SKULL_CHAR);
            lcd.write(SKULL_CHAR);

            lcd.setCursor(0, 1);
            lcd.print("    Lost a ");
            lcd.write(HEART_CHAR);
            lcd.print("    ");

            update_lcd = true;
            state = states_t::STARTUP;
            lives -= 1;
            
            if(lives == 0) {
              state = states_t::DEFEAT;
            }
            else {
              state = states_t::LEVEL_SETUP;
            }

            delay(MAX_TRANSITION_TIME);
            break;

        case states_t::DEFEAT: 
            WriteAllLeds(LOW);

            lcd.clear();

            lcd.setCursor(0, 0);
            lcd.write(SKULL_CHAR);
            lcd.write(SKULL_CHAR);
            lcd.print(" YOU LOST!! ");
            lcd.write(SKULL_CHAR);
            lcd.write(SKULL_CHAR);

            lcd.setCursor(0, 1);
            lcd.print(" Score: ");
            lcd.print(points / 1000);
            lcd.print(points / 100);
            lcd.print(points / 10);
            lcd.print(points % 10);
            lcd.print("pts ");

            update_lcd = true;
            state = states_t::STARTUP;

            delay(MAX_TRANSITION_TIME);
            break;

        case states_t::VICTORY:  
            WriteAllLeds(HIGH);

            lcd.clear();

            lcd.setCursor(0, 0);
            lcd.write(CHECK_CHAR);
            lcd.write(CHECK_CHAR);
            lcd.print(" YOU WON!!! ");
            lcd.write(CHECK_CHAR);
            lcd.write(CHECK_CHAR);

            lcd.setCursor(0, 1);
            lcd.print(" Score: ");
            lcd.print(points / 1000);
            lcd.print(points / 100);
            lcd.print(points / 10);
            lcd.print(points % 10);
            lcd.print("pts ");

            update_lcd = true;
            state = states_t::STARTUP;

            delay(MAX_TRANSITION_TIME);
            break;
    }
}