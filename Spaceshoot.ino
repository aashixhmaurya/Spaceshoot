#include <Arduino_GFX_Library.h>
#include <TouchScreen.h>
#include <EEPROM.h>

#define YP A1
#define XM A2
#define YM 7
#define XP 6
#define MINPRESSURE 2
#define MAXPRESSURE 3000
#define NUM_STARS 15

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Arduino_DataBus *bus = new Arduino_SWPAR8(A2, A3, A1, A0, 8, 9, 2, 3, 4, 5, 6, 7);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, A4, 0, false);

int gameState = 0;

int playerX = 60;
int playerY = 290;
int playerLane = 0;

int obsY = 45;
int obsX = 60;
int obsLane = 0;
int obsSpeed = 3;
int obsType = 0;
bool isGameOver = false;

int score = 0;
int lastScore = -1;
unsigned long lastScoreTime = 0;
int frameDelay = 7;

int colorIndex = 0;

int starX[NUM_STARS];
int starY[NUM_STARS];

int highScore = 0;
int eeAddress = 0;

uint16_t bgColors[12]  = {0x0000, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F, 0xFC00, 0x8010, 0xFE19, 0x8200};
uint16_t uiColors[12]  = {0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF};
uint16_t shipC1[12]    = {0x07FF, 0x001F, 0xFFE0, 0xF800, 0xFFE0, 0x001F, 0xF800, 0xFFE0, 0x001F, 0xFFE0, 0x001F, 0x07FF};
uint16_t shipC2[12]    = {0x001F, 0xF800, 0x0000, 0x001F, 0x07E0, 0xF800, 0x001F, 0x001F, 0xF800, 0x0000, 0xF800, 0xFFE0};
uint16_t shipC3[12]    = {0xF800, 0xF800, 0xFFE0, 0xF800, 0xF800, 0xF800, 0xF800, 0xFFE0, 0xF800, 0xFFE0, 0xF800, 0xF800};
uint16_t obsColors[12] = {0xFDA0, 0xF800, 0x07FF, 0x001F, 0xFDA0, 0xF800, 0x001F, 0x07E0, 0x001F, 0xF800, 0x07FF, 0xFDA0};

uint8_t asteroids[3][8] = {
  {0x3C, 0x7E, 0xDF, 0xFF, 0xFB, 0x7F, 0x3E, 0x1C},
  {0x1C, 0x3E, 0x7C, 0x78, 0x3C, 0x7E, 0x7E, 0x3C},
  {0x0C, 0x3E, 0x7F, 0xFC, 0xFE, 0x78, 0x3C, 0x18}
};

void drawPlayer(int x, int y, uint16_t c1, uint16_t c2, uint16_t c3) {
  gfx->fillTriangle(x, y - 20, x - 12, y + 10, x + 12, y + 10, c1);
  gfx->fillTriangle(x - 12, y + 10, x - 18, y + 20, x - 6, y + 10, c2);
  gfx->fillTriangle(x + 12, y + 10, x + 18, y + 20, x + 6, y + 10, c2);
  gfx->fillRect(x - 4, y + 10, 8, 12, c3);
}

void drawAsteroid(int x, int y, uint16_t assetColor, uint16_t bgColor, int type) {
  int startX = x - 24;
  for (int r = 0; r < 8; r++) {
    uint8_t rowByte = asteroids[type][r];
    int rowY = y + (r * 6);
    int c = 0;
    while (c < 8) {
      bool bit = (rowByte & (0x80 >> c)) != 0;
      int span = 1;
      while (c + span < 8 && ((rowByte & (0x80 >> (c + span))) != 0) == bit) {
        span++;
      }
      uint16_t color = bit ? assetColor : bgColor;
      gfx->fillRect(startX + (c * 6), rowY, span * 6, 6, color);
      c += span;
    }
  }
}

void drawHome() {
  gfx->fillScreen(0x0000);
  
  for(int i = 0; i < 35; i++) {
    uint16_t starColor = (i % 3 == 0) ? 0x07FF : ((i % 2 == 0) ? 0xFDA0 : 0xFFFF);
    gfx->fillRect(random(0, 240), random(0, 320), 2, 2, starColor);
  }

  drawPlayer(120, 85, 0x07FF, 0x001F, 0xF800);
  
  gfx->fillRect(118, 25, 4, 35, 0xF800);
  gfx->fillRect(116, 20, 8, 5, 0xFFFF);
  
  drawAsteroid(40, 35, 0xFDA0, 0x0000, 0);
  drawAsteroid(200, 95, 0x07E0, 0x0000, 1);
  drawAsteroid(55, 115, 0xF800, 0x0000, 2);

  gfx->setCursor(32, 192);
  gfx->setTextColor(0x18E3);
  gfx->setTextSize(3);
  gfx->print("SPACE");
  gfx->setCursor(122, 192);
  gfx->setTextColor(0x03E0);
  gfx->print("SHOOT");

  gfx->setCursor(30, 190);
  gfx->setTextColor(0x07FF);
  gfx->setTextSize(3);
  gfx->print("SPACE");
  gfx->setCursor(120, 190);
  gfx->setTextColor(0x07E0);
  gfx->setTextSize(3);
  gfx->print("SHOOT");

  gfx->drawFastHLine(22, 178, 65, 0xFFFF);
  gfx->drawFastHLine(80, 179, 135, 0xFFFF);
  gfx->drawFastHLine(20, 222, 140, 0xFFFF);
  gfx->drawFastHLine(155, 221, 65, 0xFFFF);
  
  gfx->drawFastVLine(22, 180, 18, 0xFFFF);
  gfx->drawFastVLine(21, 195, 25, 0xFFFF);
  gfx->drawFastVLine(215, 176, 22, 0xFFFF);
  gfx->drawFastVLine(216, 192, 28, 0xFFFF);

  gfx->fillRect(38, 263, 164, 44, 0xFFFF);
  gfx->fillRect(40, 265, 160, 40, 0xF800);
  gfx->setCursor(54, 277);
  gfx->setTextColor(0xFFFF);
  gfx->setTextSize(2);
  gfx->print("TAP TO PLAY");
}

void drawUI() {
  gfx->fillRect(0, 0, 240, 45, bgColors[colorIndex]);
  gfx->drawRect(0, 0, 240, 45, colorIndex == 0 ? 0x07FF : uiColors[colorIndex]);
  
  gfx->setCursor(140, 14);
  gfx->setTextColor(colorIndex == 0 ? 0x07E0 : uiColors[colorIndex]);
  gfx->setTextSize(2);
  gfx->print("SCR:");
}

void resetGame() {
  playerLane = 0;
  playerX = 60;
  obsY = 45;
  obsLane = random(0, 2);
  obsX = (obsLane == 0) ? 60 : 180;
  obsType = random(0, 3);
  isGameOver = false;
  score = 0;
  lastScore = -1;
  colorIndex = 0;
  frameDelay = 7;
  for(int i = 0; i < NUM_STARS; i++) {
    starX[i] = random(0, 240);
    starY[i] = random(46, 320);
  }
  gfx->fillScreen(bgColors[colorIndex]);
  drawUI();
  drawPlayer(playerX, playerY, shipC1[colorIndex], shipC2[colorIndex], shipC3[colorIndex]);
  lastScoreTime = millis();
}

void setup() {
  gfx->begin();
  randomSeed(analogRead(A5));
  EEPROM.get(eeAddress, highScore);
  if (highScore < 0 || highScore > 30000) {
    highScore = 0;
  }
  gameState = 0;
  drawHome();
  delay(500);
}

void loop() {
  if (gameState == 0) {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      gameState = 1;
      resetGame();
      delay(200);
    }
    return;
  }

  if (isGameOver) {
    if (score > highScore) {
      highScore = score;
      EEPROM.put(eeAddress, highScore);
    }
    gfx->fillScreen(0xF800);
    gfx->setCursor(12, 60);
    gfx->setTextColor(0xFFFF);
    gfx->setTextSize(4);
    gfx->print("GAME OVER");
    gfx->setCursor(15, 130);
    gfx->setTextColor(0xFFE0);
    gfx->setTextSize(2);
    gfx->print("SCORED: ");
    gfx->print(score);
    gfx->setCursor(15, 170);
    gfx->setTextColor(0x07FF);
    gfx->setTextSize(2);
    gfx->print("HIGHEST SCORE: ");
    gfx->print(highScore);
    gfx->setCursor(54, 280);
    gfx->setTextColor(0xFFFF);
    gfx->setTextSize(2);
    gfx->print("TOUCH TO HOME");
    while (true) {
      TSPoint p = ts.getPoint();
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
        gameState = 0;
        drawHome();
        delay(200);
        return;
      }
      delay(10);
    }
  }

  if (millis() - lastScoreTime >= 150) {
    score++;
    lastScoreTime = millis();
  }

  int speedLevel = score / 100;
  int newColorIndex = speedLevel % 12;
  frameDelay = 7 - speedLevel;
  if (frameDelay < 0) frameDelay = 0;

  if (newColorIndex != colorIndex) {
    colorIndex = newColorIndex;
    gfx->fillScreen(bgColors[colorIndex]);
    drawUI();
    drawPlayer(playerX, playerY, shipC1[colorIndex], shipC2[colorIndex], shipC3[colorIndex]);
  }

  if (score != lastScore) {
    gfx->fillRect(188, 10, 50, 25, bgColors[colorIndex]);
    gfx->setCursor(188, 14);
    gfx->setTextColor(colorIndex == 0 ? 0x07E0 : uiColors[colorIndex]);
    gfx->setTextSize(2);
    gfx->print(score);
    lastScore = score;
  }

  for (int i = 0; i < NUM_STARS; i++) {
    gfx->fillRect(starX[i], starY[i], 2, 2, bgColors[colorIndex]);
    starY[i] += 1;
    if (starY[i] > 320) {
      starY[i] = 46;
      starX[i] = random(0, 240);
    }
    gfx->fillRect(starX[i], starY[i], 2, 2, colorIndex == 0 ? 0xFFFF : uiColors[colorIndex]);
  }

  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int mX = map(p.y, 120, 940, 240, 0);
    int oldX = playerX;
    if (mX > 120) {
      playerLane = 0;
      playerX = 60;
    } else {
      playerLane = 1;
      playerX = 180;
    }
    if (oldX != playerX) {
      drawPlayer(oldX, playerY, bgColors[colorIndex], bgColors[colorIndex], bgColors[colorIndex]);
      drawPlayer(playerX, playerY, shipC1[colorIndex], shipC2[colorIndex], shipC3[colorIndex]);
    }
  }

  int oldY = obsY;
  obsY += obsSpeed;

  if (obsY > 320) {
    gfx->fillRect(obsX - 24, oldY, 48, 48, bgColors[colorIndex]);
    obsY = 45;
    obsLane = random(0, 2);
    obsX = (obsLane == 0) ? 60 : 180;
    obsType = random(0, 3);
  } else {
    gfx->fillRect(obsX - 24, oldY, 48, obsSpeed, bgColors[colorIndex]);
    drawAsteroid(obsX, obsY, obsColors[colorIndex], bgColors[colorIndex], obsType);
  }

  if (obsY + 48 >= playerY - 15 && obsY <= playerY + 15) {
    if (obsLane == playerLane) {
      isGameOver = true;
    }
  }

  delay(frameDelay);
}