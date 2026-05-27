#include <Arduino.h>
#include <TFT_eSPI.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

TFT_eSPI tft;

namespace {

static constexpr size_t UART_LINE_MAX = 96;
static constexpr uint8_t NAME_MAX_LEN = 16;
static constexpr uint8_t HP_DEFAULT = 5;
static constexpr uint8_t HP_HUD_CAP = 10;
static constexpr int TFT_BACKLIGHT_PIN = 33;
static constexpr int HUD_RX_PIN = 18;
static const char *HUD_PREFIX = "HUD:";

// Houdt bij wat er op het scherm moet staan.
struct GameState {
    char name[NAME_MAX_LEN + 1] = "Maarten";
    uint16_t teamColor = TFT_NAVY;
    uint8_t hitpoints = HP_DEFAULT;
    uint8_t maxHitpoints = HP_DEFAULT;
    bool dirtyBg = true;
    bool dirtyName = true;
    bool dirtyHp = true;
};

GameState state;

char s_line[UART_LINE_MAX];
size_t s_len = 0;
bool s_lineReady = false;

// Zet een gewone kleur om naar het formaat van het TFT.
uint16_t rgb888To565(uint32_t rgb) {
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Leest een kleur zoals #FF0000.
bool parseHexColor(const char *value, uint16_t &outColor) {
    if (!value || !*value) return false;

    while (*value == ' ' || *value == '\t') value++;
    if (*value == '#') value++;
    if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) value += 2;

    if (!isxdigit((unsigned char)value[0]) || !isxdigit((unsigned char)value[1]) ||
        !isxdigit((unsigned char)value[2]) || !isxdigit((unsigned char)value[3]) ||
        !isxdigit((unsigned char)value[4]) || !isxdigit((unsigned char)value[5])) {
        return false;
    }

    char hex[7];
    memcpy(hex, value, 6);
    hex[6] = '\0';
    uint32_t rgb = strtoul(hex, nullptr, 16);
    outColor = rgb888To565(rgb);
    return true;
}

// Kiest donkere of lichte tekst volgens de achtergrond.
bool computeTextColor(uint16_t bgColor) {
    uint8_t r5 = (bgColor >> 11) & 0x1F;
    uint8_t g6 = (bgColor >> 5) & 0x3F;
    uint8_t b5 = bgColor & 0x1F;
    uint8_t r8 = static_cast<uint8_t>((r5 * 255U) / 31U);
    uint8_t g8 = static_cast<uint8_t>((g6 * 255U) / 63U);
    uint8_t b8 = static_cast<uint8_t>((b5 * 255U) / 31U);
    uint16_t brightness = static_cast<uint16_t>((r8 * 299U + g8 * 587U + b8 * 114U) / 1000U);
    return brightness > 140U;
}

// Zorgt dat levens nooit hoger zijn dan het maximum.
void clampHitpoints(GameState &st) {
    if (st.maxHitpoints == 0) st.maxHitpoints = 1;
    if (st.hitpoints > st.maxHitpoints) st.hitpoints = st.maxHitpoints;
}

// Haalt spaties weg aan het begin en einde.
char *trimInPlace(char *text) {
    if (!text) return text;

    while (*text == ' ' || *text == '\t') text++;
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t')) {
        text[--len] = '\0';
    }
    return text;
}

// Start het TFT-scherm.
void renderInit() {
    tft.begin();
    tft.setRotation(1);
    tft.setTextDatum(TL_DATUM);
}

// Tekent naam, kleur en levens op het scherm.
void render(GameState &st) {
    clampHitpoints(st);

    if (st.dirtyBg) {
        tft.fillScreen(st.teamColor);
        st.dirtyName = true;
        st.dirtyHp = true;
        st.dirtyBg = false;
    }

    bool useDarkText = computeTextColor(st.teamColor);
    uint16_t textColor = useDarkText ? TFT_BLACK : TFT_WHITE;
    uint16_t mutedColor = useDarkText ? TFT_DARKGREY : TFT_LIGHTGREY;

    if (st.dirtyName) {
        tft.fillRect(0, 8, tft.width(), 52, st.teamColor);
        tft.setTextColor(textColor, st.teamColor);
        tft.setTextFont(2);
        int titleWidth = tft.textWidth("PLAYER");
        tft.drawString("PLAYER", (tft.width() - titleWidth) / 2, 10);

        tft.setTextFont(4);
        int w = tft.textWidth(st.name);
        tft.drawString(st.name, (tft.width() - w) / 2, 28);
        st.dirtyName = false;
    }

    if (st.dirtyHp) {
        tft.fillRect(0, 70, tft.width(), 60, st.teamColor);
        tft.setTextColor(textColor, st.teamColor);
        tft.setTextFont(2);
        int hpWidth = tft.textWidth("HP");
        tft.drawString("HP", (tft.width() - hpWidth) / 2, 72);

        uint8_t drawCount = st.maxHitpoints > HP_HUD_CAP ? HP_HUD_CAP : st.maxHitpoints;
        if (drawCount == 0) drawCount = 1;
        int spacing = tft.width() / (drawCount + 1);
        int y = 108;
        int radius = 9;

        for (uint8_t i = 0; i < drawCount; i++) {
            uint16_t col = (i < st.hitpoints) ? TFT_RED : mutedColor;
            tft.fillCircle((i + 1) * spacing, y, radius, col);
        }

        char hpText[16];
        snprintf(hpText, sizeof(hpText), "%u/%u", st.hitpoints, st.maxHitpoints);
        tft.setTextFont(2);
        int countWidth = tft.textWidth(hpText);
        tft.drawString(hpText, (tft.width() - countWidth) / 2, 122);

        st.dirtyHp = false;
    }
}

// Leest tekstregels die van de STM32 komen.
void linkPoll() {
    while (Serial1.available() && !s_lineReady) {
        int c = Serial1.read();
        if (c == '\r') continue;
        if (c == '\n') {
            s_line[s_len] = '\0';
            s_lineReady = (s_len > 0);
            s_len = 0;
            return;
        }
        if (s_len < sizeof(s_line) - 1) {
            s_line[s_len++] = (char)c;
        } else {
            s_len = 0;
        }
    }
}

// Verwerkt alleen regels die met HUD: beginnen.
void parseHudLine(char *line, GameState &st) {
    if (!line || strncmp(line, HUD_PREFIX, strlen(HUD_PREFIX)) != 0) return;

    char *payload = line + strlen(HUD_PREFIX);
    char *colon = strchr(payload, ':');
    if (!colon) return;

    *colon = '\0';
    char *key = trimInPlace(payload);
    char *value = trimInPlace(colon + 1);

    if (strcasecmp(key, "NAME") == 0 || strcasecmp(key, "PLAYER") == 0) {
        if (*value == '\0') return;
        char nextName[NAME_MAX_LEN + 1];
        strncpy(nextName, value, NAME_MAX_LEN);
        nextName[NAME_MAX_LEN] = '\0';

        if (strcmp(st.name, nextName) != 0) {
            strcpy(st.name, nextName);
            st.dirtyName = true;
        }
        return;
    }

    if (strcasecmp(key, "COLOR") == 0 || strcasecmp(key, "TEAM") == 0 || strcasecmp(key, "TEAMCOLOR") == 0) {
        uint16_t color565;
        if (parseHexColor(value, color565) && st.teamColor != color565) {
            st.teamColor = color565;
            st.dirtyBg = true;
        }
        return;
    }

    if (strcasecmp(key, "MAXHP") == 0) {
        long maximum = strtol(value, nullptr, 10);
        if (maximum > 0 && maximum <= 255) {
            st.maxHitpoints = (uint8_t)maximum;
            clampHitpoints(st);
            st.dirtyHp = true;
        }
        return;
    }

    if (strcasecmp(key, "HP") == 0 || strcasecmp(key, "LIFE") == 0 || strcasecmp(key, "HITPOINTS") == 0) {
        char *slash = strchr(value, '/');
        long current = strtol(value, nullptr, 10);
        if (current < 0) current = 0;&
        if (slash) {
            long maximum = strtol(slash + 1, nullptr, 10);
            if (maximum > 0 && maximum <= 255) st.maxHitpoints = (uint8_t)maximum;
        }
        if (current > 255) current = 255;
        st.hitpoints = (uint8_t)current;
        clampHitpoints(st);
        st.dirtyHp = true;
    }
}

}  // namespace

// Start seriele poort, backlight en scherm.
void setup() {
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {}
    Serial.println("Starting TFT HUD...");
#endif

    Serial1.setRxBufferSize(512);
    Serial1.begin(115200, SERIAL_8N1, HUD_RX_PIN, -1);

    pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
    digitalWrite(TFT_BACKLIGHT_PIN, HIGH);

    renderInit();
    render(state);
}

// Blijft luisteren naar nieuwe HUD-regels.
void loop() {
    linkPoll();
    if (s_lineReady) {
#if ARDUINO_USB_CDC_ON_BOOT
        Serial.print("RX: ");
        Serial.println(s_line);
#endif
        parseHudLine(s_line, state);
        s_lineReady = false;
    }

    render(state);
}
