#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

#if defined(__SAM3X8E__)
#undef __FlashStringHelper::F(string_literal)
#define F(string_literal) string_literal
#endif

/************ Touch wiring for Elegoo 2.8" TFT (same as your sample) ************/
#define YP A3 // must be analog
#define XM A2 // must be analog
#define YM 9  // digital OK
#define XP 8  // digital OK

/************ Calibrate if touches are off ************/
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

enum PanelMode : uint8_t
{
  MODE_LIFE = 0,
  MODE_POISON = 1,
  MODE_CMDMG = 2
};
uint8_t modeP[NUM_PLAYERS] = {MODE_LIFE, MODE_LIFE, MODE_LIFE, MODE_LIFE};

/************ Layout (LANDSCAPE 320x240) ************/
#define PANEL_W 160
#define PANEL_H 120

const int16_t PX[NUM_PLAYERS] = {0, 160, 0, 160}; // panel origins X
const int16_t PY[NUM_PLAYERS] = {0, 0, 120, 120}; // panel origins Y

// Per-player accent colors (borders/titles)
const uint16_t ACC[NUM_PLAYERS] = {MAGENTA, CYAN, YELLOW, GREEN};

// Reusable button helpers
struct Rect
{
  int16_t x, y, w, h;
};
static inline bool inRect(int16_t x, int16_t y, const Rect &r)
{
  return (x >= r.x && x < (r.x + r.w) && y >= r.y && y < (r.y + r.h));
}

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
#define BOXSIZE 40
#define PENRADIUS 3
int oldcolor, currentcolor;

void drawButton(const Rect &r, uint16_t fill, uint16_t border, const char *label, uint8_t tsize = 2, uint16_t tcolor = WHITE)
{
  tft.fillRect(r.x, r.y, r.w, r.h, fill);
  tft.drawRect(r.x, r.y, r.w, r.h, border);
  tft.setTextColor(tcolor);
  tft.setTextSize(tsize);
  // Simple centering
  int16_t cx = r.x + (r.w / 2) - (strlen(label) * 6 * tsize) / 2;
  int16_t cy = r.y + (r.h / 2) - (8 * tsize) / 2;
  tft.setCursor(cx, cy);
  tft.print(label);
}

void drawTitle(uint8_t p)
{
  int16_t x = PX[p], y = PY[p];
  tft.drawRect(x, y, PANEL_W, PANEL_H, ACC[p]);
  tft.fillRect(x, y, PANEL_W, 16, ACC[p]);
  tft.setTextColor(BLACK);
  tft.setTextSize(1);
  tft.setCursor(x + 4, y + 4);
  tft.print("P");
  tft.print(p + 1);
  // mode tag
  tft.setCursor(x + 28, y + 4);
  switch (modeP[p])
  {
  case MODE_LIFE:
    tft.print(" Life");
    break;
  case MODE_POISON:
    tft.print(" Poison");
    break;
  case MODE_CMDMG:
    tft.print(" Cmd Dmg");
    break;
  }
}

void clearPanelBody(uint8_t p)
{
  int16_t x = PX[p], y = PY[p];
  tft.fillRect(x + 1, y + 17, PANEL_W - 2, PANEL_H - 18, BLACK);
}

/************ Panel UI drawing ************/
void drawPanelLife(uint8_t p)
{
  int16_t x = PX[p], y = PY[p];
  drawTitle(p);
  clearPanelBody(p);

  // Big life number
  tft.setTextColor(WHITE);
  tft.setTextSize(5);
  // center-ish
  int16_t numX = x + 48;
  int16_t numY = y + 36;
  if (lifeT[p] >= 100 || lifeT[p] <= -10)
    numX = x + 28; // crude centering
  tft.setCursor(numX, numY);
  tft.print(lifeT[p]);

  // +/- buttons
  Rect minus = {x + 8, y + 72, 44, 36};
  Rect plus = {x + 108, y + 72, 44, 36};
  drawButton(minus, RED, WHITE, "-", 3);
  drawButton(plus, GREEN, WHITE, "+", 3);

  // Mode button
  Rect modeBtn = {x + PANEL_W - 54, y + 2, 50, 12};
  drawButton(modeBtn, GRAY, WHITE, "MODE", 1, WHITE);
}

void drawPanelPoison(uint8_t p)
{
  int16_t x = PX[p], y = PY[p];
  drawTitle(p);
  clearPanelBody(p);

  // "Poison" header
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  tft.setCursor(x + 8, y + 22);
  tft.print("Poison");

  // Value big
  tft.setTextColor(YELLOW);
  tft.setTextSize(4);
  tft.setCursor(x + 68, y + 46);
  tft.print(poisonT[p]);

  // +/- buttons
  Rect minus = {x + 8, y + 72, 44, 36};
  Rect plus = {x + 108, y + 72, 44, 36};
  drawButton(minus, RED, WHITE, "-", 3);
  drawButton(plus, GREEN, WHITE, "+", 3);

  // Mode button
  Rect modeBtn = {x + PANEL_W - 54, y + 2, 50, 12};
  drawButton(modeBtn, GRAY, WHITE, "MODE", 1, WHITE);

  // 10 max hint
  tft.setTextColor(GRAY);
  tft.setTextSize(1);
  tft.setCursor(x + 8, y + 56);
  tft.print("(0..10)");
}

void drawPanelCmdmg(uint8_t p)
{
  int16_t x = PX[p], y = PY[p];
  drawTitle(p);
  clearPanelBody(p);

  // Rows for each OTHER player as damage source
  uint8_t row = 0;
  for (uint8_t attacker = 0; attacker < NUM_PLAYERS; attacker++)
  {
    if (attacker == p)
      continue;
    int16_t ry = y + 20 + row * 30;
    // label "from Pk"
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(x + 6, ry);
    tft.print("P");
    tft.print(attacker + 1);
    tft.print(">");

    // value box
    Rect valBox = {x + 78, ry - 2, 36, 24};
    tft.drawRect(valBox.x, valBox.y, valBox.w, valBox.h, WHITE);
    tft.setTextColor(YELLOW);
    tft.setTextSize(2);
    // center small value
    int16_t vx = valBox.x + 10;
    int16_t vy = valBox.y + 5;
    tft.setCursor(vx, vy);
    tft.print(cmdmg[p][attacker]);

    // - and + buttons
    Rect minus = {x + 44, ry - 2, 28, 24};
    Rect plus = {x + 120, ry - 2, 28, 24};
    drawButton(minus, RED, WHITE, "-", 2);
    drawButton(plus, GREEN, WHITE, "+", 2);

    row++;
  }

  // Mode button
  Rect modeBtn = {x + PANEL_W - 54, y + 2, 50, 12};
  drawButton(modeBtn, GRAY, WHITE, "MODE", 1, WHITE);

  // 21 lethal hint
  tft.setTextColor(GRAY);
  tft.setTextSize(1);
  tft.setCursor(x + 6, y + PANEL_H - 12);
  tft.print("21 dmg from a single commander = loss");
}

/************ Draw a panel according to its mode ************/
void drawPanel(uint8_t p)
{
  switch (modeP[p])
  {
  case MODE_LIFE:
    drawPanelLife(p);
    break;
  case MODE_POISON:
    drawPanelPoison(p);
    break;
  case MODE_CMDMG:
    drawPanelCmdmg(p);
    break;
  }
}

/************ Global reset button (center) ************/
Rect resetBtn = {0, 0, 80, 40};

void drawResetButton()
{
  drawButton(resetBtn, BLUE, WHITE, "RESET", 2);
}

/************ Full screen render ************/
void drawAll()
{
  tft.fillScreen(BLACK);
  for (uint8_t p = 0; p < NUM_PLAYERS; p++)
    drawPanel(p);
  // Cross lines to visually separate panels
  tft.drawLine(160, 0, 160, 240, GRAY);
  tft.drawLine(0, 120, 320, 120, GRAY);
  drawResetButton();
}

/************ Small helpers for updates without redrawing everything ************/
void updateLife(uint8_t p)
{
  // just redraw the panel in life mode to keep it simple & reliable
  if (modeP[p] == MODE_LIFE)
    drawPanelLife(p);
}

void updatePoison(uint8_t p)
{
  if (modeP[p] == MODE_POISON)
    drawPanelPoison(p);
}

void updateCmdmg(uint8_t p)
{
  if (modeP[p] == MODE_CMDMG)
    drawPanelCmdmg(p);
}

/************ Setup ************/
void setup()
{
  Serial.begin(9600);
  Serial.println(F("Paint!"));

  tft.reset();

  uint16_t identifier = tft.readID();
  if (identifier == 0x9325)
  {
    Serial.println(F("Found ILI9325 LCD driver"));
  }
  else if (identifier == 0x9328)
  {
    Serial.println(F("Found ILI9328 LCD driver"));
  }
  else if (identifier == 0x4535)
  {
    Serial.println(F("Found LGDP4535 LCD driver"));
  }
  else if (identifier == 0x7575)
  {
    Serial.println(F("Found HX8347G LCD driver"));
  }
  else if (identifier == 0x9341)
  {
    Serial.println(F("Found ILI9341 LCD driver"));
  }
  else if (identifier == 0x8357)
  {
    Serial.println(F("Found HX8357D LCD driver"));
  }
  else if (identifier == 0x0101)
  {
    identifier = 0x9341;
    Serial.println(F("Found 0x9341 LCD driver"));
  }
  else
  {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    identifier = 0x9341;
  }
  tft.begin(identifier);

  // LANDSCAPE (ratio fixed)
  tft.setRotation(1); // 1 or 3 are landscape; use 1 by default

  // Clear cmdmg
  for (uint8_t i = 0; i < NUM_PLAYERS; i++)
    for (uint8_t j = 0; j < NUM_PLAYERS; j++)
      cmdmg[i][j] = 0;

  drawAll();
}

/************ Loop: handle touches ************/
void loop()
{
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE)
  {
    int16_t x, y, z;

    Serial.print("Raw X = ");
    Serial.print(p.x);
    Serial.print("\tRaw Y = ");
    Serial.println(p.y);

    x = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());
    y = map(p.x, TS_MAXX, TS_MINX, 0, tft.height());

    x = constrain(x, 0, tft.width() - 1);
    y = constrain(y, 0, tft.height() - 1);

    Serial.print("Mapped 2 X = ");
    Serial.print(x);
    Serial.print("\tMapped 2 Y = ");
    Serial.println(y);

    tft.fillCircle(x, y, PENRADIUS, WHITE);

    // First: check RESET
    if (inRect(x, y, resetBtn))
    {
      for (uint8_t player = 0; player < NUM_PLAYERS; player++)
      {
        lifeT[player] = 40;
        poisonT[player] = 0;
        for (uint8_t a = 0; a < NUM_PLAYERS; a++)
          cmdmg[player][a] = 0;
      }
      drawAll();
      // delay(250);
      return;
    }

    // Determine which panel was tapped
    uint8_t panel = 0xFF;
    for (uint8_t player = 0; player < NUM_PLAYERS; player++)
    {
      Rect r = {PX[player], PY[player], PANEL_W, PANEL_H};
      if (inRect(x, y, r))
      {
        panel = player;
        break;
      }
    }
    if (panel == 0xFF)
      return; // outside

    int16_t x0 = PX[panel], y0 = PY[panel];
    int16_t lx = x - x0, ly = y - y0;

    // Mode button (top-right small strip)
    Rect modeBtn = {x0 + PANEL_W - 54, y0 + 2, 50, 12};
    if (inRect(x, y, modeBtn))
    {
      modeP[panel] = (modeP[panel] + 1) % 3;
      drawPanel(panel);
      delay(200);
      return;
    }

    // Respond according to current mode
    if (modeP[panel] == MODE_LIFE)
    {
      Rect minus = {x0 + 8, y0 + 72, 44, 36};
      Rect plus = {x0 + 108, y0 + 72, 44, 36};
      if (inRect(x, y, minus))
      {
        lifeT[panel]--;
        updateLife(panel);
        delay(150);
        return;
      }
      if (inRect(x, y, plus))
      {
        lifeT[panel]++;
        updateLife(panel);
        delay(150);
        return;
      }
    }
    else if (modeP[panel] == MODE_POISON)
    {
      Rect minus = {x0 + 8, y0 + 72, 44, 36};
      Rect plus = {x0 + 108, y0 + 72, 44, 36};
      if (inRect(x, y, minus))
      {
        if (poisonT[panel] > 0)
          poisonT[panel]--;
        updatePoison(panel);
        delay(150);
        return;
      }
      if (inRect(x, y, plus))
      {
        if (poisonT[panel] < 10)
          poisonT[panel]++;
        updatePoison(panel);
        delay(150);
        return;
      }
    }
    else if (modeP[panel] == MODE_CMDMG)
    {
      // Identify which attacker row: three rows skipping self
      uint8_t order[3];
      uint8_t idx = 0;
      for (uint8_t a = 0; a < NUM_PLAYERS; a++)
        if (a != panel)
          order[idx++] = a;

      for (uint8_t r = 0; r < 3; r++)
      {
        int16_t ry = y0 + 20 + r * 30;
        Rect minus = {x0 + 44, ry - 2, 28, 24};
        Rect val = {x0 + 78, ry - 2, 36, 24};
        Rect plus = {x0 + 120, ry - 2, 28, 24};
        uint8_t attacker = order[r];

        if (inRect(x, y, minus))
        {
          if (cmdmg[panel][attacker] > 0)
            cmdmg[panel][attacker]--;
          updateCmdmg(panel);
          delay(140);
          return;
        }
        if (inRect(x, y, plus))
        {
          if (cmdmg[panel][attacker] < 21)
            cmdmg[panel][attacker]++;
          updateCmdmg(panel);
          delay(140);
          return;
        }
        // Ignore taps on val or label
      }
    }
  }
}
