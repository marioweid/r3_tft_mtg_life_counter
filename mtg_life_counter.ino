#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

#if defined(__SAM3X8E__)
#undef __FlashStringHelper::F(string_literal)
#define F(string_literal) string_literal
#endif

/************ Touch wiring ************/
#define YP A3 // must be analog
#define XM A2 // must be analog
#define YM 9  // digital OK
#define XP 8  // digital OK

/************ Touch calibration ************/
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
#define MINPRESSURE 10
#define MAXPRESSURE 1000

/************ TFT control pins ************/
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

/************ Colors ************/
#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define YELLOW 0xFFE0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GRAY 0x8410

/************ Game data ************/
const uint8_t NUM_PLAYERS = 4;
int16_t lifeT[NUM_PLAYERS] = {40, 40, 40, 40}; // life totals
uint8_t poisonT[NUM_PLAYERS] = {0, 0, 0, 0};   // poison counters (0..10)
// commander damage: damageTaken[defender][attacker]
uint8_t cmdmg[NUM_PLAYERS][NUM_PLAYERS];

/************ Modes ************/
enum PanelMode : uint8_t {
  MODE_LIFE = 0,
  MODE_POISON = 1,
  MODE_CMDMG = 2
};
uint8_t currentMode = MODE_LIFE;

/************ Layout ************/
#define SIDEBAR_W 50
#define SIDEBAR_BTN_H 50
#define PANEL_W 130
#define PANEL_H 120

const int16_t gap = 2;
const int16_t PX[NUM_PLAYERS] = {SIDEBAR_W + gap, SIDEBAR_W + PANEL_W + 2*gap,
                                  SIDEBAR_W + gap, SIDEBAR_W + PANEL_W + 2*gap};
const int16_t PY[NUM_PLAYERS] = {0, 0, 120, 120};

// Sidebar buttons
struct Rect { int16_t x, y, w, h; };
static inline bool inRect(int16_t x, int16_t y, const Rect &r) {
  return (x >= r.x && x < (r.x + r.w) && y >= r.y && y < (r.y + r.h));
}
Rect btnLife   = {0, 0, SIDEBAR_W, SIDEBAR_BTN_H};
Rect btnPoison = {0, SIDEBAR_BTN_H, SIDEBAR_W, SIDEBAR_BTN_H};
Rect btnCmdmg  = {0, SIDEBAR_BTN_H * 2, SIDEBAR_W, SIDEBAR_BTN_H};
Rect btnReset  = {0, 240 - 40, SIDEBAR_W, 40};

unsigned long touchStartTime = 0;
bool isHoldingReset = false;

// Per-player accent colors
const uint16_t ACC[NUM_PLAYERS] = {MAGENTA, CYAN, YELLOW, GREEN};

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

#define PENRADIUS 3

/************ UI Helpers ************/
void drawButton(const Rect &r, uint16_t fill, uint16_t border, const char *label, uint8_t tsize = 2, uint16_t tcolor = WHITE) {
  tft.fillRect(r.x, r.y, r.w, r.h, fill);
  tft.drawRect(r.x, r.y, r.w, r.h, border);
  tft.setTextColor(tcolor);
  tft.setTextSize(tsize);
  // center label
  int16_t cx = r.x + (r.w / 2) - (strlen(label) * 6 * tsize) / 2;
  int16_t cy = r.y + (r.h / 2) - (8 * tsize) / 2;
  tft.setCursor(cx, cy);
  tft.print(label);
}

void drawTitle(uint8_t p) {
  int16_t x = PX[p], y = PY[p];
  tft.drawRect(x, y, PANEL_W, PANEL_H, ACC[p]);
  tft.fillRect(x, y, PANEL_W, 16, ACC[p]);
  tft.setTextColor(BLACK);
  tft.setTextSize(1);
  tft.setCursor(x + 4, y + 4);
  tft.print("P");
  tft.print(p + 1);
}

void clearPanelBody(uint8_t p) {
  int16_t x = PX[p], y = PY[p];
  tft.fillRect(x + 1, y + 17, PANEL_W - 2, PANEL_H - 18, BLACK);
}

/************ Panel UI ************/
void drawPanelLife(uint8_t p) {
  int16_t x = PX[p], y = PY[p];
  drawTitle(p);
  clearPanelBody(p);

  // Big life number
  tft.setTextColor(WHITE);
  tft.setTextSize(5);
  int16_t numX = x , numY = y + PANEL_H / 2 - 10;
  if (lifeT[p] < 100 && lifeT[p] > -10) numX = x + 48;
  tft.setCursor(numX - 12, numY);
  tft.print(lifeT[p]);

  // < and > buttons
  tft.setTextColor(GRAY);
  tft.setTextSize(3);
  tft.setCursor(x + 2 , y + PANEL_H / 2);
  tft.print("<");
  tft.setCursor(x + PANEL_W - 20, y + PANEL_H / 2);
  tft.print(">");
}

void drawPanelPoison(uint8_t p) {
  int16_t x = PX[p], y = PY[p];
  drawTitle(p);
  clearPanelBody(p);

  // Value
  tft.setTextColor(GREEN);
  tft.setTextSize(4);
  tft.setCursor(x + 58, y + 46);
  tft.print(poisonT[p]);

  // < and > buttons
  tft.setTextColor(GRAY);
  tft.setTextSize(3);
  tft.setCursor(x + 2 , y + PANEL_H / 2);
  tft.print("<");
  tft.setCursor(x + PANEL_W - 20, y + PANEL_H / 2);
  tft.print(">");
}

void drawPanelCmdmg(uint8_t p) {
  int16_t x = PX[p], y = PY[p];
  drawTitle(p);
  clearPanelBody(p);

  uint8_t row = 0;
  for (uint8_t attacker = 0; attacker < NUM_PLAYERS; attacker++) {
    if (attacker == p) continue;
    int16_t ry = y + 20 + row * 30;
    // label
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(x + 6, ry);
    tft.print("P");
    tft.print(attacker + 1);

    // value
    Rect valBox = {x + 58, ry - 2, 36, 24};
    tft.drawRect(valBox.x, valBox.y, valBox.w, valBox.h, WHITE);
    tft.setTextColor(YELLOW);
    tft.setTextSize(2);
    tft.setCursor(valBox.x + 10, valBox.y + 5);
    tft.print(cmdmg[p][attacker]);

    // buttons
    Rect minus = {x + 34, ry - 2, 28, 24};
    Rect plus  = {x + 90, ry - 2, 28, 24};
    drawButton(minus, RED, WHITE, "-", 2);
    drawButton(plus, GREEN, WHITE, "+", 2);

    row++;
  }

  tft.setTextColor(GRAY);
  tft.setTextSize(1);
  tft.setCursor(x + 6, y + PANEL_H - 12);
}

void drawPanel(uint8_t p) {
  switch (currentMode) {
    case MODE_LIFE:   drawPanelLife(p); break;
    case MODE_POISON: drawPanelPoison(p); break;
    case MODE_CMDMG:  drawPanelCmdmg(p); break;
  }
}

void drawIconButton(const Rect &r, uint16_t fill, uint16_t border, uint8_t iconType) {
  tft.fillRect(r.x, r.y, r.w, r.h, fill);
  tft.drawRect(r.x, r.y, r.w, r.h, border);

  int16_t cx = r.x + r.w / 2;
  int16_t cy = r.y + r.h / 2;

  switch(iconType) {
    case 0: // Life(Heart)
      tft.fillCircle(cx - 6, cy - 3, 6, WHITE);
      tft.fillCircle(cx + 6, cy - 3, 6, WHITE);
      tft.fillTriangle(cx - 12, cy - 3, cx + 12, cy - 3, cx, cy + 10, WHITE);
      break;

    case 1: // Poison(Skull)
      tft.fillCircle(cx, cy - 5, 10, WHITE);
      tft.fillCircle(cx - 4, cy - 7, 2, BLACK);
      tft.fillCircle(cx + 4, cy - 7, 2, BLACK);
      tft.drawLine(cx - 6, cy + 4, cx + 6, cy + 4, BLACK);
      tft.drawLine(cx - 6, cy + 6, cx + 6, cy + 6, BLACK);
      break;

    case 2: // Commander(Swords)
      tft.drawLine(cx - 8, cy - 8, cx + 8, cy + 8, WHITE);
      tft.drawLine(cx + 8, cy - 8, cx - 8, cy + 8, WHITE);
      tft.drawLine(cx - 4, cy + 8, cx + 4, cy + 8, WHITE);
      tft.drawLine(cx - 4, cy - 8, cx + 4, cy - 8, WHITE);
      break;

    case 3: // Reset(Arrow)
      tft.drawCircle(cx, cy, 10, WHITE);
      tft.drawLine(cx + 5, cy - 8, cx + 10, cy - 8, WHITE);
      tft.drawLine(cx + 5, cy - 8, cx + 5, cy - 3, WHITE);
      break;
  }
}

/************ Sidebar ************/
void drawSidebar() {
  tft.fillRect(0, 0, SIDEBAR_W, 240, GRAY);

  drawIconButton(btnLife,   (currentMode==MODE_LIFE)?BLUE:BLACK, WHITE, 0);
  drawIconButton(btnPoison, (currentMode==MODE_POISON)?BLUE:BLACK, WHITE, 1);
  drawIconButton(btnCmdmg,  (currentMode==MODE_CMDMG)?BLUE:BLACK, WHITE, 2);
  drawIconButton(btnReset,  RED, WHITE, 3);
}

/************ Draw All ************/
void drawAll() {
  tft.fillScreen(BLACK);
  drawSidebar();
  for (uint8_t p = 0; p < NUM_PLAYERS; p++) drawPanel(p);
  tft.drawLine(SIDEBAR_W + PANEL_W + gap*2, 0, SIDEBAR_W + PANEL_W + gap*2, 240, GRAY);
  tft.drawLine(SIDEBAR_W, 120, 320, 120, GRAY);
}

/************ Setup ************/
void setup() {
  Serial.begin(9600);
  tft.reset();
  uint16_t identifier = tft.readID();
  if (identifier == 0x0101) identifier = 0x9341;
  tft.begin(identifier);
  tft.setRotation(1);
  for (uint8_t i = 0; i < NUM_PLAYERS; i++)
    for (uint8_t j = 0; j < NUM_PLAYERS; j++)
      cmdmg[i][j] = 0;
  drawAll();
}

/************ Loop ************/
void loop() {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    if (p.z <= MINPRESSURE || p.z >= MAXPRESSURE) return;

    int16_t x = constrain(map(p.y, TS_MAXY, TS_MINY, 0, tft.width()), 0, tft.width() - 1);
    int16_t y = constrain(map(p.x, TS_MAXX, TS_MINX, 0, tft.height()), 0, tft.height() - 1);

    // --- Sidebar buttons ---
    if (x < SIDEBAR_W) {
        if (y < SIDEBAR_BTN_H)           { currentMode = MODE_LIFE; drawAll(); delay(200); return; }
        else if (y < SIDEBAR_BTN_H * 2)  { currentMode = MODE_POISON; drawAll(); delay(200); return; }
        else if (y < SIDEBAR_BTN_H * 3)  { currentMode = MODE_CMDMG; drawAll(); delay(200); return; }
        else if (y >= 240 - 40) {  // Reset
            if (!isHoldingReset) { isHoldingReset = true; touchStartTime = millis(); }
            else if (millis() - touchStartTime >= 2000) {
                for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
                    lifeT[i] = 40;
                    poisonT[i] = 0;
                    for (uint8_t j = 0; j < NUM_PLAYERS; j++) cmdmg[i][j] = 0;
                }
                currentMode = MODE_LIFE;
                drawAll(); delay(500); isHoldingReset = false;
            }
        } else {
            isHoldingReset = false;
        }
        return;
    }

    // --- Determine which panel ---
    int8_t panel = -1;
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        if (x >= PX[i] && x < PX[i] + PANEL_W &&
            y >= PY[i] && y < PY[i] + PANEL_H) {
            panel = i;
            break;
        }
    }
    if (panel < 0) return;

    int16_t x0 = PX[panel], y0 = PY[panel];

    // --- Life or Poison Mode ---
    if (currentMode == MODE_LIFE || currentMode == MODE_POISON) {
        bool changed = false;
        if (x < x0 + PANEL_W / 2) {  // Left half = <
            if (currentMode == MODE_LIFE)      { lifeT[panel]--; changed = true; }
            else if (currentMode == MODE_POISON && poisonT[panel] > 0) { poisonT[panel]--; changed = true; }
        } else {                       // Right half = >
            if (currentMode == MODE_LIFE)      { lifeT[panel]++; changed = true; }
            else if (currentMode == MODE_POISON && poisonT[panel] < 10) { poisonT[panel]++; changed = true; }
        }
        if (changed) { drawPanel(panel); delay(150); }
        return;
    }

    // --- Commander Damage ---
    if (currentMode == MODE_CMDMG) {
        uint8_t idx = 0;
        for (uint8_t attacker = 0; attacker < NUM_PLAYERS; attacker++) {
            if (attacker == panel) continue;
            int16_t ry = y0 + 20 + idx * 30;
            if (x >= x0 + 34 && x < x0 + 62 && y >= ry - 2 && y < ry + 22) { // minus
                if (cmdmg[panel][attacker] > 0) cmdmg[panel][attacker]--;
                drawPanel(panel); delay(140); return;
            }
            if (x >= x0 + 90 && x < x0 + 118 && y >= ry - 2 && y < ry + 22) { // plus
                if (cmdmg[panel][attacker] < 21) cmdmg[panel][attacker]++;
                drawPanel(panel); delay(140); return;
            }
            idx++;
        }
    }
}
