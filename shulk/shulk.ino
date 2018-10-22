#include <Adafruit_NeoPixel.h>

/*
 * Macros
 */
#define NEOPIXEL_PIN (6)
#define BUTTON_PIN (2)
#define NUMBER_OF_LEDS (30)
#define COUNTS_PER_FRAME (1040)

#define FRAME_TIME_MS (16)
#define FRAMES_PER_SECOND (60)
#define FRAMES_TO_SELECT_ART (FRAMES_PER_SECOND * 1)//(30)
#define FRAMES_TO_BLINK_ART (FRAMES_PER_SECOND * 12) // 12s
#define FRAMES_TO_END_ART (FRAMES_PER_SECOND * 4)

#define BLACK (0x000000)
#define GREEN (0x00FF00)
#define BLUE (0x0000FF)
#define YELLOW (0xFFFF00)
#define PURPLE (0x551A8B)
#define RED (0xFF0000)
#define WHITE (0xFFFFFF)

/*
 * typedefs
 */
typedef enum {
  NORMAL,
  JUMP,
  SPEED,
  SHIELD,
  BUSTER,
  SMASH,
  MONADO_MAX
} Monado;

typedef struct {
  bool inactive;
  bool active;
  bool cooldown;
  uint32_t colour;
} MonadoState;

typedef enum {
  NO_ART,
  ART_PENDING,
  ART_SET,
  ART_ENDING,
  STATE_MAX
} StateMachineState;

typedef void (*StateMachineRoutine)(void);

/*
 * function prototypes
 */
void chooseColour(uint32_t colour);
void changeArt(void);
void changeState(void);
void NO_ART_routine(void);
void ART_PENDING_routine(void);
void ART_SET_routine(void);
void ART_ENDING_routine(void);

/*
 * Global (module wide) variables
 */
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Monado monado = NORMAL;
uint32_t frameCount = 0;
uint32_t StoredFrame = 0;
MonadoState monadoState[MONADO_MAX] = {
// inactive, active, cooldown, colour   
  {false,    true,   false,    BLACK},  // NORMAL
  {true,     false,  false,    GREEN},  // JUMP
  {true,     false,  false,    BLUE},   // SPEED
  {true,     false,  false,    YELLOW}, // SHIELD
  {true,     false,  false,    PURPLE}, // BUSTER
  {true,     false,  false,    RED}     // SMASH
};
StateMachineState stateMachineState = NO_ART;
StateMachineRoutine stateMachineRoutine[STATE_MAX] = {
  NO_ART_routine,
  ART_PENDING_routine,
  ART_SET_routine,
  ART_ENDING_routine
};
bool buttonPressed = false;
bool buttonStateChangedToPressed = false;
bool artEndingState = false;

/*
 * local functions
 */
void setup() {
  pinMode(BUTTON_PIN, INPUT);
}

void loop(void) {
  bool oldButtonPressed = buttonPressed;

  if (digitalRead(BUTTON_PIN) == HIGH) {
    buttonPressed = true;
  }
  else {
    buttonPressed = false;
  }
  if (buttonPressed != oldButtonPressed && buttonPressed) {
    buttonStateChangedToPressed = true;
  }
    
  if (stateMachineState < STATE_MAX) {
    stateMachineRoutine[stateMachineState]();
  }
  frameCount++; // handle overflow
  delay(FRAME_TIME_MS); // This is only valid if the code runs in << 16ms
}

void chooseColour(uint32_t colour) {
  strip.begin();
  for (uint8_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, colour);
  }
  strip.show();
}

void changeArt(void) {
  uint8_t i;
  for (i = 0; i < MONADO_MAX; i++) {
    if (monadoState[i].active) {
      break;
    }
  }
  monadoState[i].active = false;
  i++;
  if (i == MONADO_MAX) {
    i = NORMAL;
  }
  monadoState[i].active = true;
  chooseColour(monadoState[i].colour);
}

void changeState(StateMachineState newState) {
  uint8_t i;
  stateMachineState = newState;
  switch (stateMachineState) { // dislike this switch knowning what states we've defined
  case NO_ART:
    for (i = 0; i < MONADO_MAX; i++) {
      if (monadoState[i].active) {
        break;
      }
    }
    monadoState[i].active = false;
    monadoState[NORMAL].active = true;
    chooseColour(monadoState[NORMAL].colour);
    break;
  case ART_PENDING:
    break;
  case ART_SET:
    StoredFrame = frameCount;
    break;
  case ART_ENDING:
    StoredFrame = frameCount;
    break;
  }
}

void NO_ART_routine(void) {
  if (buttonPressed) {
    changeState(ART_PENDING);
  }
}

void ART_PENDING_routine(void) {
  // entry to this state may have the issue that the button was pressed while in NO_ART, and has unpressed since last frame
  if (buttonStateChangedToPressed) {
    changeArt();
    StoredFrame = frameCount;
    buttonStateChangedToPressed = false;
  }
  if (frameCount - StoredFrame > FRAMES_TO_SELECT_ART) {
    changeState(ART_SET);
  }
}

void ART_SET_routine(void) {
  if (frameCount - StoredFrame > FRAMES_TO_BLINK_ART) {
    changeState(ART_ENDING);
  }
}

void ART_ENDING_routine(void) {
  uint8_t i;
  if (artEndingState) {
    chooseColour(BLACK);
    if (frameCount % 4 == 0) {
      artEndingState = false;
    }
  }
  else {
    for (i = 0; i < MONADO_MAX; i++) {
      if (monadoState[i].active) {
        break;
      }
    }
    chooseColour(monadoState[i].colour);
    if (frameCount % 4 == 0) {
      artEndingState = true;
    }
  }
  if (frameCount - StoredFrame > FRAMES_TO_END_ART) {
    changeState(NO_ART);
  }
}

