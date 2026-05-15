#include "displayDriver.h"

#ifdef T_DISPLAY


#include "media/NotoSansCyrillic.h"
#include <TFT_eSPI.h>
#include "media/images_320_170.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "userConfig.h"
#include <WiFi.h>
#include "HTTPClient.h"
#include "OpenFontRender.h"
#include "rotation.h"
#include "claude_usage.h"

static volatile char alertStatus = 'N';
static void alertTask(void* pvParameters);

#define WIDTH 340
#define HEIGHT 170

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                  // Invoke library, pins defined in User_Setup.h
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite


void tDisplay_Init(void)
{
      //Init pin 15 to eneble 5V external power (LilyGo bug)
#ifdef PIN_ENABLE5V
    pinMode(PIN_ENABLE5V, OUTPUT);
    digitalWrite(PIN_ENABLE5V, HIGH);
#endif
  
  tft.init();
  #ifdef LILYGO_S3_T_EMBED
  tft.setRotation(ROTATION_270);
  #else
  tft.setRotation(ROTATION_90);
  #endif
  tft.setSwapBytes(true);                 // Swap the colour byte order when rendering
  background.createSprite(WIDTH, HEIGHT); // Background Sprite
  background.setSwapBytes(true);
  render.setDrawer(background);  // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9); // Espaciado entre texto

  // Load the font and check it can be read OK
  // if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))) {
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    return;
  }

  xTaskCreate(alertTask, "AlertFetch", 6144, NULL, 1, NULL);
}

void tDisplay_AlternateScreenState(void)
{
  int screen_state = digitalRead(TFT_BL);
  Serial.println("Switching display state");
  digitalWrite(TFT_BL, !screen_state);
}

void tDisplay_AlternateRotation(void)
{
  tft.setRotation( flipRotation(tft.getRotation()) );
}

void tDisplay_MinerScreen(unsigned long mElapsed)
{
  render.loadFont(DigitalNumbers, sizeof(DigitalNumbers));
  mining_data data = getMiningData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(35);
  render.setCursor(19, 118);
  render.setFontColor(TFT_BLACK);

  render.rdrawString(data.currentHashRate.c_str(), 118, 114, TFT_BLACK);
  // Total hashes
  render.setFontSize(18);
  render.rdrawString(data.totalMHashes.c_str(), 268, 138, TFT_BLACK);
  // Block templates
  render.setFontSize(18);
  render.drawString(data.templates.c_str(), 186, 20, 0xDEDB);
  // Best diff
  render.drawString(data.bestDiff.c_str(), 186, 48, 0xDEDB);
  // 32Bit shares
  render.setFontSize(18);
  render.drawString(data.completedShares.c_str(), 186, 76, 0xDEDB);
  // Hores
  render.setFontSize(14);
  render.rdrawString(data.timeMining.c_str(), 315, 104, 0xDEDB);

  // Valid Blocks
  render.setFontSize(24);
  render.drawString(data.valids.c_str(), 285, 56, 0xDEDB);

  // Print Temp
  render.setFontSize(10);
  render.rdrawString(data.temp.c_str(), 239, 1, TFT_BLACK);

  render.setFontSize(4);
  render.rdrawString(String(0).c_str(), 244, 3, TFT_BLACK);

  // Print Hour
  render.setFontSize(10);
  render.rdrawString(data.currentTime.c_str(), 286, 1, TFT_BLACK);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_ClockScreen(unsigned long mElapsed)
{
  render.loadFont(DigitalNumbers, sizeof(DigitalNumbers));
  clock_data data = getClockData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(25);
  render.setCursor(19, 122);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 94, 129, TFT_BLACK);

  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 202, 3, GFXFF);

  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 140, TFT_BLACK);

  // Print Hour
  background.setFreeFont(FF23);
  background.setTextSize(2);
  background.setTextColor(0xDEDB, TFT_BLACK);

  background.drawString(data.currentTime.c_str(), 130, 50, GFXFF);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_GlobalHashScreen(unsigned long mElapsed)
{
  render.loadFont(DigitalNumbers, sizeof(DigitalNumbers));
  coin_data data = getCoinData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 198, 3, GFXFF);

  // Print Hour
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 268, 3, GFXFF);

  // Print Last Pool Block
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.halfHourFee.c_str(), 302, 52, GFXFF);

  // Print Difficulty
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.netwrokDifficulty.c_str(), 302, 88, GFXFF);

  // Print Global Hashrate
  render.setFontSize(17);
  render.rdrawString(data.globalHashRate.c_str(), 274, 145, TFT_BLACK);

  // Print BlockHeight
  render.setFontSize(28);
  render.rdrawString(data.blockHeight.c_str(), 140, 104, 0xDEDB);

  // Draw percentage rectangle
  int x2 = 2 + (138 * data.progressPercent / 100);
  background.fillRect(2, 149, x2, 168, 0xDEDB);

  // Print Remaining BLocks
  background.setTextFont(FONT2);
  background.setTextSize(1);
  background.setTextDatum(MC_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.remainingBlocks.c_str(), 72, 159, FONT2);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}


void tDisplay_BTCprice(unsigned long mElapsed)
{
  render.loadFont(DigitalNumbers, sizeof(DigitalNumbers));
  clock_data data = getClockData(mElapsed);
  data.currentDate ="01/12/2023";
  
  //if(data.currentDate.indexOf("12/2023")>) { tDisplay_ChristmasContent(data); return; }

  // Print background screen
  background.pushImage(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(25);
  render.setCursor(19, 122);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 94, 129, TFT_BLACK);

  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 138, TFT_WHITE);

  // Print Hour
  
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 222, 3, GFXFF);

  // Print BTC Price 
  background.setFreeFont(FF24);
  background.setTextDatum(TR_DATUM);
  background.setTextSize(1);
  background.setTextColor(0xDEDB, TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 300, 58, GFXFF);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_LoadingScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, initWidth, initHeight, initScreen);
  tft.setTextColor(TFT_BLACK);
  tft.drawString(CURRENT_VERSION, 24, 147, FONT2);
}

void tDisplay_SetupScreen(void)
{
  tft.pushImage(0, 0, setupModeWidth, setupModeHeight, setupModeScreen);
}

void tDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

void tDisplay_DoLedStuff(unsigned long frame)
{
}

// ===== AIR RAID ALERT SCREEN =====
#define ALERT_BASE_URL "https://api.alerts.in.ua/v1/iot/active_air_raid_alerts/25.json?token="
#define UPDATE_ALERT_ms 30000

static void alertTask(void* pvParameters) {
  vTaskDelay(20000 / portTICK_PERIOD_MS);
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(8000);
      if (http.begin("http://192.168.88.20:8765/alert")) {
        int code = http.GET();
        if (code == 200) {
          String s = http.getString();
          s.trim();
          if (s.length() > 0 && (s[0] == 'A' || s[0] == 'P' || s[0] == 'N')) {
            alertStatus = s[0];
          }
        }
        Serial.printf("Alert HTTP: %d  status: %c\n", code, (char)alertStatus);
        http.end();
      }
    }
    vTaskDelay(UPDATE_ALERT_ms / portTICK_PERIOD_MS);
  }
}

void tDisplay_AlertScreen(unsigned long mElapsed) {
  (void)mElapsed;

  uint32_t bgColor = TFT_GREEN;
  const char* statusText = "ЧИСТО";
  const char* statusUA = "Немає тривоги";
  if (alertStatus == 'A') {
    bgColor = TFT_RED;
    statusText = "ТРИВОГА";
    statusUA = "Повітряна тривога";
  } else if (alertStatus == 'P') {
    bgColor = TFT_ORANGE;
    statusText = "ЧАСТКОВО";
    statusUA = "Часткова тривога";
  }

  background.fillSprite(bgColor);

  // Top info bar — load Cyrillic font unconditionally: other screens call
  // render.loadFont(DigitalNumbers) every frame which silently replaces it.
  render.loadFont(NotoSansCyrillic, sizeof(NotoSansCyrillic));

  background.fillRect(0, 0, WIDTH, 22, 0x1082);
  // Region label — use render so Cyrillic glyphs are available
  render.setFontSize(11);
  render.drawString("Чернігівська обл.", 8, 5, TFT_WHITE);
  // Time — ASCII only, background sprite is fine
  String alertTime = getTime();
  background.setFreeFont(FSS9);
  background.setTextColor(TFT_WHITE, 0x1082);
  background.setTextDatum(TR_DATUM);
  background.drawString(alertTime.c_str(), 312, 4, GFXFF);

  render.setFontSize(58);
  render.drawString(statusText, 20, 30, TFT_WHITE);

  render.setFontSize(20);
  render.drawString(statusUA, 20, 128, TFT_WHITE);

  background.pushSprite(0, 0);
}

// ===== CLAUDE USAGE SCREEN =====
static unsigned long sClaudeFooterTime = 0;
static int           sClaudeFooterIdx  = 0;
static const char*   CLAUDE_FOOTER[]   = {"* Baking...", "* Divining...", "* Thinking...", "* Brewing..."};

void tDisplay_ClaudeScreen(unsigned long mElapsed) {
  (void)mElapsed;
  background.fillSprite(TFT_BLACK);

  // Header bar
  background.fillRect(0, 0, WIDTH, 22, 0x1082);
  background.setFreeFont(FSS9);
  background.setTextColor(TFT_WHITE, 0x1082);
  background.setTextDatum(TL_DATUM);
  background.drawString("Claude Usage", 8, 4, GFXFF);
  String claudeTime = getTime();
  background.setTextDatum(TR_DATUM);
  background.drawString(claudeTime.c_str(), 312, 4, GFXFF);

  bool dataOk = gClaudeUsage.ok &&
                (gClaudeUsage.last_updated > 0) &&
                (millis() - gClaudeUsage.last_updated < 10UL * 60UL * 1000UL);

  if (dataOk) {
    char buf[32];

    // ---- 5h / Current session ----
    snprintf(buf, sizeof(buf), "%d%%", gClaudeUsage.session_pct);
    background.setFreeFont(FSSB9);
    background.setTextColor(TFT_WHITE, TFT_BLACK);
    background.setTextDatum(TL_DATUM);
    background.drawString(buf, 8, 27, GFXFF);
    background.setTextDatum(TR_DATUM);
    background.drawString("Current", 312, 27, GFXFF);

    // Progress bar
    int bx = 8, bw = WIDTH - 16, bh = 12, by = 48;
    background.fillRect(bx, by, bw, bh, 0x2104);
    uint32_t c5 = gClaudeUsage.session_pct > 80 ? TFT_RED :
                  gClaudeUsage.session_pct > 50 ? TFT_ORANGE : TFT_GREEN;
    int f5 = bw * gClaudeUsage.session_pct / 100;
    if (f5 > 0) background.fillRect(bx, by, f5, bh, c5);

    // Reset time
    int rm = gClaudeUsage.session_reset_min;
    if (rm > 60)
      snprintf(buf, sizeof(buf), "Resets in %dh %dm", rm / 60, rm % 60);
    else
      snprintf(buf, sizeof(buf), "Resets in %dm", rm);
    background.setFreeFont(FSS9);
    background.setTextColor(0xC618, TFT_BLACK);
    background.setTextDatum(TL_DATUM);
    background.drawString(buf, 8, 66, GFXFF);

    // Divider
    background.drawLine(0, 84, WIDTH, 84, 0x4208);

    // ---- Weekly ----
    snprintf(buf, sizeof(buf), "%d%%", gClaudeUsage.weekly_pct);
    background.setFreeFont(FSSB9);
    background.setTextColor(TFT_WHITE, TFT_BLACK);
    background.setTextDatum(TL_DATUM);
    background.drawString(buf, 8, 90, GFXFF);
    background.setTextDatum(TR_DATUM);
    background.drawString("Weekly", 312, 90, GFXFF);

    // Progress bar
    int by2 = 111;
    background.fillRect(bx, by2, bw, bh, 0x2104);
    uint32_t cW = gClaudeUsage.weekly_pct > 80 ? TFT_RED :
                  gClaudeUsage.weekly_pct > 50 ? TFT_ORANGE : TFT_GREEN;
    int fW = bw * gClaudeUsage.weekly_pct / 100;
    if (fW > 0) background.fillRect(bx, by2, fW, bh, cW);

    // Reset time
    int wr = gClaudeUsage.weekly_reset_min;
    if (wr > 1440)
      snprintf(buf, sizeof(buf), "Resets in %dd %dh", wr / 1440, (wr % 1440) / 60);
    else if (wr > 60)
      snprintf(buf, sizeof(buf), "Resets in %dh %dm", wr / 60, wr % 60);
    else
      snprintf(buf, sizeof(buf), "Resets in %dm", wr);
    background.setFreeFont(FSS9);
    background.setTextColor(0xC618, TFT_BLACK);
    background.setTextDatum(TL_DATUM);
    background.drawString(buf, 8, 129, GFXFF);

  } else {
    // No data — grey placeholders
    background.setFreeFont(FSSB9);
    background.setTextColor(0x8410, TFT_BLACK);
    background.setTextDatum(MC_DATUM);
    background.drawString("--%", WIDTH / 2, 58, GFXFF);
    background.setFreeFont(FSS9);
    background.drawString("No data - check API key", WIDTH / 2, 84, GFXFF);
    background.drawString("--%", WIDTH / 2, 110, GFXFF);
  }

  // Footer — cycling text, orange, every 3 s
  if (millis() - sClaudeFooterTime > 3000) {
    sClaudeFooterTime = millis();
    sClaudeFooterIdx  = (sClaudeFooterIdx + 1) % 4;
  }
  background.setFreeFont(FSS9);
  background.setTextColor(TFT_ORANGE, TFT_BLACK);
  background.setTextDatum(TL_DATUM);
  background.drawString(CLAUDE_FOOTER[sClaudeFooterIdx], 8, 150, GFXFF);

  background.pushSprite(0, 0);
}

CyclicScreenFunction tDisplayCyclicScreens[] = {tDisplay_MinerScreen, tDisplay_AlertScreen, tDisplay_ClaudeScreen};

DisplayDriver tDisplayDriver = {
    tDisplay_Init,
    tDisplay_AlternateScreenState,
    tDisplay_AlternateRotation,
    tDisplay_LoadingScreen,
    tDisplay_SetupScreen,
    tDisplayCyclicScreens,
    tDisplay_AnimateCurrentScreen,
    tDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(tDisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
