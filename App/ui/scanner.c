/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <stdbool.h>
#include <string.h>
#include "app/scanner.h"
#include "dcs.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "l10n/strings.h"
#include "misc.h"
#include "ui/helper.h"
#include "ui/scanner.h"

// English keeps the original fixed-width renderer; other languages need the UTF-8 one.
static void PrintText(const char *pString, uint8_t Start, bool bCentered, uint8_t Line)
{
    if (g_lang == LANG_EN) {
        UI_PrintString(pString, Start, bCentered ? 127 : 0, Line, 8);
        return;
    }

    uint8_t x = Start;

    if (bCentered) {
        unsigned int width = 0;

        for (const uint8_t *p = (const uint8_t *)pString; *p; p++) {
            if (*p >= 0xC0u)
                width += 13u;       // CJK glyph + gap
            else if (*p < 0x80u)
                width += (*p == ' ') ? 4u : 8u;
        }

        x = (width < LCD_WIDTH) ? (uint8_t)((LCD_WIDTH - width) / 2u) : 0u;
    }

    UI_PrintStringUTF8(pString, x, Line);
}

void UI_DisplayScanner(void)
{
    char  String[24] = {0};
    char *pPrintStr = String;
    bool bCentered;
    uint8_t Start;

    UI_DisplayClear();

    if (gScanSingleFrequency || (gScanCssState != SCAN_CSS_STATE_OFF && gScanCssState != SCAN_CSS_STATE_FAILED)) {
        sprintf(String, "%s%u.%05u", STR(STR_SCAN_FREQ), gScanFrequency / 100000, gScanFrequency % 100000);
    } else {
        sprintf(String, "%s**.*****", STR(STR_SCAN_FREQ));
    }
    pPrintStr = String;

    PrintText(pPrintStr, 2, false, 1);

    if (gScanCssState < SCAN_CSS_STATE_FOUND || !gScanUseCssResult) {
        sprintf(String, "%s******", STR(STR_SCAN_CTC));
    } else if (gScanCssResultType == CODE_TYPE_CONTINUOUS_TONE) {
        sprintf(String, "%s%u.%uHz", STR(STR_SCAN_CTC), CTCSS_Options[gScanCssResultCode] / 10, CTCSS_Options[gScanCssResultCode] % 10);
    } else {
        sprintf(String, "DCS:D%03oN", DCS_Options[gScanCssResultCode]);
    }
    pPrintStr = String;

    PrintText(pPrintStr, 2, false, 3);
    memset(String, 0, sizeof(String));
    if (gScannerSaveState == SCAN_SAVE_CHANNEL) {
        pPrintStr = (char *)STR(STR_SCAN_SAVE_ASK);
        Start     = 0;
        bCentered = 1;
    } else {
        Start     = 2;
        bCentered = 0;

        if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
            strcpy(String, STR(STR_SCAN_SAVE));
            UI_GenerateChannelStringEx(String + strlen(String), gShowChPrefix, gScanChannel);
            pPrintStr = String;
        } else if (gScanCssState < SCAN_CSS_STATE_FOUND) {
            strcpy(String, STR(STR_SCAN_SCANNING));
            memset(String + strlen(String), '.', (gScanProgressIndicator & 7) + 1);
            pPrintStr = String;
        } else if (gScanCssState == SCAN_CSS_STATE_FOUND) {
            pPrintStr = (char *)STR(STR_SCAN_DONE);
        } else {
            pPrintStr = (char *)STR(STR_SCAN_FAIL);
        }
    }

    PrintText(pPrintStr, Start, bCentered, 5);

    ST7565_BlitFullScreen();
}
