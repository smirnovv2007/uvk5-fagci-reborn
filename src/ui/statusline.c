#include "statusline.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../helper/battery.h"
#include "../scheduler.h"
#include "components.h"
#include "graphics.h"
#include <string.h>

static uint8_t previousBatteryLevel = 255;
static bool showBattery = true;

static bool lastEepromRead = false;
static bool lastEepromWrite = false;

static char statuslineText[32] = {0};

static void eepromRWReset() {
  lastEepromRead = lastEepromWrite = gEepromRead = gEepromWrite = false;
  gRedrawScreen = true;
}

void STATUSLINE_SetText(const char *pattern, ...) {
  char statuslineTextNew[32] = {0};
  va_list args;
  va_start(args, pattern);
  vsnprintf(statuslineTextNew, 31, pattern, args);
  va_end(args);
  if (strcmp(statuslineText, statuslineTextNew)) {
    strncpy(statuslineText, statuslineTextNew, 31);
    gRedrawScreen = true;
  }
}

void STATUSLINE_update() {
  BATTERY_UpdateBatteryInfo();
  if (gBatteryDisplayLevel) {
    if (previousBatteryLevel != gBatteryDisplayLevel) {
      previousBatteryLevel = gBatteryDisplayLevel;
      UI_Battery(gBatteryDisplayLevel);

      gRedrawScreen = true;
    }
  } else {
    showBattery = !showBattery;
    gRedrawScreen = true;
  }

  if (lastEepromRead != gEepromRead || lastEepromWrite != gEepromWrite) {
    lastEepromRead = gEepromRead;
    lastEepromWrite = gEepromWrite;
    gRedrawScreen = true;
    TaskAdd("EEPROM RW-", eepromRWReset, 500, false);
  }
}

void STATUSLINE_render() {
  UI_ClearStatus();

  DrawHLine(0, 7, LCD_WIDTH, C_FILL);

  if (showBattery) {
    UI_Battery(gBatteryDisplayLevel);
  }

  if (gSettings.upconverter) {
    FillRect(LCD_WIDTH - 45, 2, 6, 4, C_FILL);
    DrawVLine(LCD_WIDTH - 44, 0, 2, C_FILL);
    DrawVLine(LCD_WIDTH - 41, 1, 1, C_FILL);
  }

  if (UART_IsLogEnabled) {
    PrintSmallEx(LCD_WIDTH - 1 - 24, 5, POS_R, C_FILL, "D:%u",
                 UART_IsLogEnabled);
  }

  if (gEepromRead) {
    PrintSmallEx(LCD_WIDTH - 1 - 6, 5, POS_R, C_FILL, "R");
  }

  if (gEepromWrite) {
    PrintSmallEx(LCD_WIDTH - 1, 5, POS_R, C_FILL, "W");
  }

  if (statuslineText[0] >= 32) {
    PrintSmall(BATTERY_W + 1, 5, statuslineText);
  }
}