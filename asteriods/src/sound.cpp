#include "sound.h"
#include "config.h"

// Active buzzer: fixed frequency oscillator inside.
// Control volume/rhythm via PWM duty cycle (0-255).
// Pattern = array of {duty, durationMs} steps.

struct SoundStep {
  uint8_t duty;       // 0 = silent, 255 = max volume
  uint16_t durationMs;
};

// ── Sound patterns ──

static const SoundStep PAT_SHOOT[] = {
  {200, 30},
  {0, 0}
};

static const SoundStep PAT_ASTEROID_SMALL[] = {
  {180, 20}, {0, 20}, {180, 20},
  {0, 0}
};

static const SoundStep PAT_ASTEROID_LARGE[] = {
  {220, 25}, {0, 15}, {160, 25}, {0, 15}, {100, 25},
  {0, 0}
};

static const SoundStep PAT_DEATH[] = {
  {255, 100}, {200, 100}, {150, 100}, {100, 100}, {50, 100},
  {0, 0}
};

static const SoundStep PAT_STREAK_UP[] = {
  {100, 30}, {0, 10}, {180, 30}, {0, 10}, {255, 30},
  {0, 0}
};

static const SoundStep PAT_STREAK_LOST[] = {
  {150, 80},
  {0, 0}
};

static const SoundStep PAT_THRUST[] = {
  {120, 15}, {0, 15},
  {0, 0}
};

static const SoundStep PAT_MENU_SELECT[] = {
  {200, 15},
  {0, 0}
};

static const SoundStep PAT_HIGHSCORE[] = {
  {200, 40}, {0, 30}, {200, 40}, {0, 30}, {200, 40}, {0, 30}, {255, 40}, {0, 30}, {255, 60},
  {0, 0}
};

static const SoundStep PAT_GAMEOVER[] = {
  {255, 200}, {200, 200}, {150, 200}, {100, 200}, {50, 200},
  {0, 0}
};

static const SoundStep PAT_UFO[] = {
  {150, 200}, {0, 200},
  {0, 0}
};

// ── State ──
static SoundFX currentFX = SFX_NONE;
static const SoundStep* currentPattern = nullptr;
static int currentStep = 0;
static unsigned long stepStartMs = 0;

// Map SFX enum to pattern
static const SoundStep* getPattern(SoundFX fx) {
  switch (fx) {
    case SFX_SHOOT:          return PAT_SHOOT;
    case SFX_ASTEROID_SMALL: return PAT_ASTEROID_SMALL;
    case SFX_ASTEROID_LARGE: return PAT_ASTEROID_LARGE;
    case SFX_DEATH:          return PAT_DEATH;
    case SFX_STREAK_UP:      return PAT_STREAK_UP;
    case SFX_STREAK_LOST:    return PAT_STREAK_LOST;
    case SFX_THRUST:         return PAT_THRUST;
    case SFX_MENU_SELECT:    return PAT_MENU_SELECT;
    case SFX_HIGHSCORE:      return PAT_HIGHSCORE;
    case SFX_GAMEOVER:       return PAT_GAMEOVER;
    case SFX_UFO:            return PAT_UFO;
    default:                 return nullptr;
  }
}

void soundInit() {
  ledcSetup(BUZZ_CHANNEL, BUZZ_FREQ, BUZZ_RES);
  ledcAttachPin(PIN_BUZZER, BUZZ_CHANNEL);
  ledcWrite(BUZZ_CHANNEL, 0);
  currentFX = SFX_NONE;
  currentPattern = nullptr;
}

void soundPlay(SoundFX fx) {
  if (fx == SFX_NONE) return;

  // Priority: lower enum value = higher priority
  // Only override if new sound has higher or equal priority
  if (currentFX != SFX_NONE && fx > currentFX) {
    return;  // current sound has higher priority
  }

  const SoundStep* pat = getPattern(fx);
  if (!pat) return;

  currentFX = fx;
  currentPattern = pat;
  currentStep = 0;
  stepStartMs = millis();
  ledcWrite(BUZZ_CHANNEL, pat[0].duty);
}

void soundUpdate() {
  if (!currentPattern) return;

  unsigned long now = millis();
  const SoundStep& step = currentPattern[currentStep];

  // Check if current step duration has elapsed
  if (now - stepStartMs >= step.durationMs) {
    currentStep++;

    // Check if pattern is finished (terminated by {0, 0})
    if (currentPattern[currentStep].durationMs == 0) {
      // Pattern done
      ledcWrite(BUZZ_CHANNEL, 0);
      currentFX = SFX_NONE;
      currentPattern = nullptr;
      return;
    }

    // Advance to next step
    stepStartMs = now;
    ledcWrite(BUZZ_CHANNEL, currentPattern[currentStep].duty);
  }
}

void soundStop() {
  ledcWrite(BUZZ_CHANNEL, 0);
  currentFX = SFX_NONE;
  currentPattern = nullptr;
}
