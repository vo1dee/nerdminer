#include "claude_usage.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "drivers/storage/storage.h"

ClaudeUsage gClaudeUsage = {0, 0, 0, 0, false, 0};

#define CLAUDE_FETCH_INTERVAL_MS (5UL * 60UL * 1000UL)
#define CLAUDE_RETRY_INTERVAL_MS (60UL * 1000UL)

static const char* CLAUDE_API_URL = "http://192.168.88.20:8765/";

static bool doFetch(void) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.setTimeout(10000);

    if (!http.begin(CLAUDE_API_URL)) {
        Serial.println("Claude: http.begin failed");
        return false;
    }

    int code = http.GET();
    Serial.printf("Claude: HTTP %d\n", code);

    if (code != 200) {
        if (code > 0) {
            String body = http.getString();
            Serial.println("Claude: " + body.substring(0, 120));
        }
        gClaudeUsage.ok = false;
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();
    Serial.println("Claude: " + payload.substring(0, 120));

    // Clawdmeter compact format: {"s":45,"sr":120,"w":28,"wr":7200,"st":"allowed","ok":true}
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("Claude: JSON error: %s\n", err.c_str());
        gClaudeUsage.ok = false;
        return false;
    }

    bool ok = doc["ok"] | false;
    if (!ok) {
        gClaudeUsage.ok = false;
        return false;
    }

    gClaudeUsage.session_pct       = doc["s"]  | 0;
    gClaudeUsage.session_reset_min = doc["sr"] | 0;
    gClaudeUsage.weekly_pct        = doc["w"]  | 0;
    gClaudeUsage.weekly_reset_min  = doc["wr"] | 0;
    gClaudeUsage.ok                = true;
    gClaudeUsage.last_updated      = millis();

    Serial.printf("Claude: 5h=%d%%  7d=%d%%\n",
                  gClaudeUsage.session_pct, gClaudeUsage.weekly_pct);
    return true;
}

static void claudeUsageTask(void* pvParameters) {
    vTaskDelay(30000 / portTICK_PERIOD_MS);

    for (;;) {
        bool ok = doFetch();
        vTaskDelay((ok ? CLAUDE_FETCH_INTERVAL_MS : CLAUDE_RETRY_INTERVAL_MS)
                   / portTICK_PERIOD_MS);
    }
}

void startClaudeUsageTask(void) {
    xTaskCreate(claudeUsageTask, "ClaudeUsage", 8192, NULL, 1, NULL);
}
