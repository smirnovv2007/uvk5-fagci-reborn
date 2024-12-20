#include "vfopro.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc_render.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "finput.h"

static uint8_t menuState = 0;

static char String[16];

static const RegisterSpec registerSpecs[] = {
    {},
    {"ATT", BK4819_REG_13, 0, 0xFFFF, 1},
    /* {"RF", BK4819_REG_43, 12, 0b111, 1},
    {"RFwe", BK4819_REG_43, 9, 0b111, 1}, */

    // {"IF", 0x3D, 0, 0xFFFF, 100},
    // TODO: 7 values:
    /* 0: Zero IF
    0x2aab: 8.46 kHz IF
    0x4924: 7.25 kHz IF
    0x6800: 6.35 kHz IF
    0x871c: 5.64 kHz IF
    0xa666: 5.08 kHz IF
    0xc5d1: 4.62 kHz IF
    0xe555: 4.23 kHz IF
    If REG_43<5> = 1, IF = IF*2. */

    {"DEV", 0x40, 0, 0xFFF, 10},
    // {"300T", 0x44, 0, 0xFFFF, 1000},
    RS_RF_FILT_BW,
    // {"AFTxfl", 0x43, 6, 0b111, 1}, // 7 is widest
    // {"3kAFrsp", 0x74, 0, 0xFFFF, 100},
    // {"CMP", 0x31, 3, 1, 1},
    {"MIC", 0x7D, 0, 0xF, 1},

    {"AGCL", 0x49, 0, 0b1111111, 1},
    {"AGCH", 0x49, 7, 0b1111111, 1},
    {"AFC", 0x73, 0, 0xFF, 1},
};

static void UpdateRegMenuValue(RegisterSpec s, bool add) {
  uint16_t v, maxValue;

  if (s.num == BK4819_REG_13) {
    v = gCurrentPreset->band.gainIndex;
    maxValue = ARRAY_SIZE(gainTable) - 1;
  } else if (s.num == 0x73) {
    v = BK4819_GetAFC();
    maxValue = 8;
  } else {
    v = BK4819_GetRegValue(s);
    maxValue = s.mask;
  }

  if (add && v <= maxValue - s.inc) {
    v += s.inc;
  } else if (!add && v >= 0 + s.inc) {
    v -= s.inc;
  }

  if (s.num == BK4819_REG_13) {
    RADIO_SetGain(v);
  } else if (s.num == 0x73) {
    BK4819_SetAFC(v);
  } else {
    BK4819_SetRegValue(s, v);
  }
}

void VFOPRO_init(void) { RADIO_LoadCurrentVFO(); }

void VFOPRO_deinit(void) { RADIO_ToggleRX(false); }

void VFOPRO_update(void) {
  RADIO_UpdateMeasurementsEx(&gLoot[gSettings.activeVFO]);

  if (Now() - gLastRender >= 500) {
    gRedrawScreen = true;
  }
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
bool VFOPRO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (key == KEY_PTT) {
    RADIO_ToggleTX(bKeyHeld);
    return true;
  }
  if (menuState) {
    switch (key) {
    case KEY_1:
    case KEY_2:
    case KEY_3:
      menuState = key;
      return true;
    case KEY_4:
    case KEY_5:
    case KEY_6:
      menuState = key + 1;
      return true;
    case KEY_0:
      menuState = 8;
      return true;
    case KEY_STAR:
      menuState = 4;
      return true;
    default:
      break;
    }
  }
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_4: // freq catch
      if (RADIO_GetRadio() != RADIO_BK4819) {
        gShowAllRSSI = !gShowAllRSSI;
      }
      return true;
    default:
      break;
    }
  }

  if (!bKeyHeld) {
    switch (key) {
    case KEY_1:
      RADIO_UpdateStep(true);
      return true;
    case KEY_7:
      RADIO_UpdateStep(false);
      return true;
    case KEY_3:
      RADIO_UpdateSquelchLevel(true);
      return true;
    case KEY_9:
      RADIO_UpdateSquelchLevel(false);
      return true;
    case KEY_0:
      RADIO_ToggleModulation();
      return true;
    case KEY_6:
      RADIO_ToggleListeningBW();
      return true;
    case KEY_SIDE1:
      gMonitorMode = !gMonitorMode;
      return true;
    case KEY_F:
      APPS_run(APP_VFO_CFG);
      return true;
    case KEY_5:
      gFInputCallback = RADIO_TuneTo;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_8:
      if (RADIO_GetRadio() == RADIO_BK4819) {
        IncDec8(&menuState, 1, ARRAY_SIZE(registerSpecs), 1);
      }
      return true;
    default:
      break;
    }
  }

  bool isSsb = RADIO_IsSSB();
  switch (key) {
  case KEY_UP:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], true);
      return true;
    }
    RADIO_NextFreqNoClicks(true);
    return true;
  case KEY_DOWN:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], false);
      return true;
    }
    RADIO_NextFreqNoClicks(false);
    return true;
  case KEY_SIDE1:
    if (RADIO_GetRadio() == RADIO_SI4732 && isSsb) {
      RADIO_TuneToSave(radio->rx.f + 1);
      return true;
    }
    return true;
  case KEY_SIDE2:
    if (RADIO_GetRadio() == RADIO_SI4732 && isSsb) {
      RADIO_TuneToSave(radio->rx.f - 1);
      return true;
    }
    break;
  case KEY_EXIT:
    if (menuState) {
      menuState = 0;
      return true;
    }
    APPS_exit();
    return true;
  default:
    break;
  }
  return false;
}

static void DrawRegs(void) {
  const uint8_t PAD_LEFT = 1;
  const uint8_t PAD_TOP = 31;
  const uint8_t CELL_WIDTH = 31;
  const uint8_t CELL_HEIGHT = 16;
  uint8_t row = 0;

  for (uint8_t i = 0, idx = 1; idx < ARRAY_SIZE(registerSpecs); ++i, ++idx) {
    if (idx == 5) {
      row++;
      i = 0;
    }

    RegisterSpec rs = registerSpecs[idx];
    const uint8_t offsetX = PAD_LEFT + i * CELL_WIDTH + 2;
    const uint8_t offsetY = PAD_TOP + row * CELL_HEIGHT + 2;
    const uint8_t textX = offsetX + (CELL_WIDTH - 2) / 2;

    if (menuState == idx) {
      FillRect(offsetX, offsetY, CELL_WIDTH - 2, CELL_HEIGHT - 1, C_FILL);
    } else {
      DrawRect(offsetX, offsetY, CELL_WIDTH - 2, CELL_HEIGHT - 1, C_FILL);
    }

    if (rs.num == BK4819_REG_13) {
      if (gCurrentPreset->band.gainIndex == 18) {
        sprintf(String, "auto");
      } else {
        sprintf(String, "%ddB",
                gainTable[gCurrentPreset->band.gainIndex].gainDb);
      }
    } else if (rs.num == 0x73) {
      uint8_t afc = BK4819_GetAFC();
      if (afc) {
        sprintf(String, "%u", afc);
      } else {
        sprintf(String, "off");
      }
    } else {
      sprintf(String, "%u", BK4819_GetRegValue(rs));
    }

    PrintSmallEx(textX, offsetY + 6, POS_C, C_INVERT, "%s", rs.name);
    PrintSmallEx(textX, offsetY + CELL_HEIGHT - 4, POS_C, C_INVERT, String);
  }
}

void VFOPRO_render(void) {
  STATUSLINE_SetText(gCurrentPreset->band.name);
  UI_FSmall(gTxState == TX_ON ? RADIO_GetTXF() : GetScreenF(radio->rx.f));
  UI_RSSIBar(gLoot[gSettings.activeVFO].rssi, RADIO_GetS(), radio->rx.f, 23);

  if (RADIO_GetRadio() == RADIO_BK4819) {
    DrawRegs();
  }
}
