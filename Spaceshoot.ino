#include <Arduino_GFX_Library.h>
#include <EEPROM.h>
#include <TouchScreen.h>

#define YP A3
#define XM A2
#define YM 9
#define XP 8

#define MINPRESSURE 0
#define MAXPRESSURE 3500
#define NUM_STARS 40

#define TOUCH_RAW_X_MIN 120
#define TOUCH_RAW_X_MAX 920
#define TOUCH_RAW_Y_MIN 150
#define TOUCH_RAW_Y_MAX 920
#define TOUCH_CONTROL_TOP 200
#define TOUCH_SPLIT_X 120
#define TOUCH_DEADZONE 2

#define LEFT_LANE 60
#define RIGHT_LANE 180
#define SCREEN_HEIGHT 320
#define SCREEN_WIDTH 240

#define GLOW_STAR_MIN_DIST 48

uint16_t bgColors[12] = {0x0000, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFE0,
                         0x07FF, 0xF81F, 0xFC00, 0x8010, 0xFE19, 0x8200};
uint16_t uiColors[12] = {0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000,
                         0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF};

uint8_t asteroids[8][8] = {
  {0x3C, 0x7E, 0xDF, 0xFF, 0xFB, 0x7F, 0x3E, 0x1C},
  {0x38, 0x7C, 0xFE, 0xFF, 0xFF, 0x7E, 0x3C, 0x18},
  {0x38, 0x7C, 0xFE, 0xFE, 0x7C, 0x38, 0x1C, 0x08},
  {0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x18},
  {0x18, 0x7E, 0xFF, 0xFF, 0x7F, 0x7E, 0x3C, 0x18},
  {0x38, 0x7C, 0xFE, 0xEE, 0xFE, 0x7C, 0x38, 0x10},
  {0x38, 0x7C, 0xFE, 0xFF, 0x7F, 0x3E, 0x3C, 0x18},
  {0x18, 0x7C, 0xFF, 0xDF, 0xFC, 0x78, 0x38, 0x10}
};

uint16_t shipC1[12] = {0xFFE0, 0x001F, 0xFFE0, 0xF800, 0xFFE0, 0x001F,
                       0xF800, 0xFFE0, 0x001F, 0xFFE0, 0x001F, 0x07FF};
uint16_t shipC2[12] = {0x001F, 0xF800, 0x0000, 0x001F, 0x07E0, 0xF800,
                       0x001F, 0x001F, 0xF800, 0x0000, 0xF800, 0xFFE0};
uint16_t shipC3[12] = {0xF800, 0xF800, 0xFFE0, 0xF800, 0xF800, 0xF800,
                       0xF800, 0xFFE0, 0xF800, 0xFFE0, 0xF800, 0xF800};
uint16_t obsColors[12] = {0xFDA0, 0xF800, 0x07FF, 0x001F, 0xFDA0, 0xF800,
                          0x001F, 0x07E0, 0x001F, 0xF800, 0x07FF, 0xFDA0};

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
Arduino_DataBus *bus =
    new Arduino_SWPAR8(A2, A3, A1, A0, 8, 9, 2, 3, 4, 5, 6, 7);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, A4, 0, false);

int state = 0;
int lives = 3;
int px = LEFT_LANE;
int py = 290;
int player_lane = 0;

int obstacleY = 45;
int obstacleX = LEFT_LANE;
int obstacleLane = 0;
int spd = 3;
int obstacleType = 0;
bool game_over = false;

int heartY = 45;
int heartX = LEFT_LANE;
int heartLane = 0;
bool spawnHeart = false;
int nextHealthScore = 350;

int score = 0;
int lastScore = -1;
unsigned long lastScoreTime = 0;
int frameDelay = 7;
int themeIndex = 0;

int starX[NUM_STARS];
int starY[NUM_STARS];
uint8_t starType[NUM_STARS];
int hiScore = 0;
int eeAddress = 0;

uint8_t pickStarType() {
  int r = random(0, 100);
  if (r < 5)
    return 2;
  if (r < 25)
    return 1;
  return 0;
}

bool glowStarTooClose(int x, int y, int skipIdx) {
  long minDistSq = (long)GLOW_STAR_MIN_DIST * GLOW_STAR_MIN_DIST;
  for (int j = 0; j < NUM_STARS; j++) {
    if (j == skipIdx || starType[j] != 1)
      continue;
    long dx = starX[j] - x;
    long dy = starY[j] - y;
    if (dx * dx + dy * dy < minDistSq)
      return true;
  }
  return false;
}

void placeStar(int i, bool randomY) {
  uint8_t t = pickStarType();
  for (int attempt = 0; attempt < 12; attempt++) {
    int nx = random(0, SCREEN_WIDTH);
    int ny = randomY ? random(46, SCREEN_HEIGHT) : 46;
    if (t != 1 || !glowStarTooClose(nx, ny, i)) {
      starX[i] = nx;
      starY[i] = ny;
      starType[i] = t;
      return;
    }
  }

  starX[i] = random(0, SCREEN_WIDTH);
  starY[i] = randomY ? random(46, SCREEN_HEIGHT) : 46;
  starType[i] = 0;
}

void eraseStar(int i) {
  if (starType[i] == 2)
    gfx->fillRect(starX[i] - 10, starY[i] - 10, 16, 16, bgColors[themeIndex]);
  else if (starType[i] == 1)
    gfx->fillRect(starX[i] - 1, starY[i] - 1, 5, 5, bgColors[themeIndex]);
  else
    gfx->drawPixel(starX[i], starY[i], bgColors[themeIndex]);
}

void drawStar(int i, uint16_t color) {
  if (starType[i] == 0) {
    gfx->drawPixel(starX[i], starY[i], color);
  } else if (starType[i] == 1) {
    gfx->fillRect(starX[i], starY[i], 2, 2, color);
    unsigned long phase = (millis() + (unsigned long)i * 317UL) % 1600UL;
    if (phase < 500) {
      gfx->drawPixel(starX[i] + 1, starY[i] - 1, color);
      gfx->drawPixel(starX[i] + 1, starY[i] + 2, color);
      gfx->drawPixel(starX[i] - 1, starY[i] + 1, color);
      gfx->drawPixel(starX[i] + 2, starY[i] + 1, color);
    }
  } else {
    int x = starX[i];
    int y = starY[i];
    gfx->fillRect(x, y, 3, 3, color);
    gfx->fillRect(x - 2, y - 2, 2, 2, color);
    gfx->fillRect(x - 4, y - 4, 2, 2, color);
    gfx->drawPixel(x - 5, y - 5, color);
    gfx->drawPixel(x - 6, y - 6, color);
    gfx->drawPixel(x - 7, y - 7, color);
    gfx->drawPixel(x - 8, y - 8, color);
  }
}

void setup() {
  gfx->begin();
  randomSeed(analogRead(A5));

  // keep the saved high score if eeprom still looks valid
  EEPROM.get(eeAddress, hiScore);
  if (hiScore < 0 || hiScore > 30000)
    hiScore = 0;

  state = 0;
  drawHome();
  delay(500);
}

void loop() {
  if (state == 0) {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);

    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      state = 1;
      resetGame();
      delay(200);
    }
    return;
  }

  if (game_over) {
    if (score > hiScore) {
      hiScore = score;
      EEPROM.put(eeAddress, hiScore);
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
    gfx->print(hiScore);

    gfx->setCursor(54, 280);
    gfx->setTextColor(0xFFFF);
    gfx->setTextSize(2);
    gfx->print("TOUCH TO HOME");

    while (1) {
      TSPoint p = ts.getPoint();
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);

      if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
        state = 0;
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

  if (score >= nextHealthScore && !spawnHeart) {
    spawnHeart = true;
    heartLane = random(0, 2);
    if (heartLane == 0)
      heartX = LEFT_LANE;
    else
      heartX = RIGHT_LANE;
    heartY = 45;
    nextHealthScore += 350;
  }

  int speedLevel = score / 100;
  int newColorIndex = speedLevel % 12;

  frameDelay = 7 - speedLevel;
  if (frameDelay < 0)
    frameDelay = 0;

  if (newColorIndex != themeIndex) {
    themeIndex = newColorIndex;
    gfx->fillScreen(bgColors[themeIndex]);
    drawUI();
    drawPlayer(px, py, shipC1[themeIndex], shipC2[themeIndex],
               shipC3[themeIndex]);
  }

  if (score != lastScore) {
    gfx->fillRect(188, 10, 50, 25, bgColors[themeIndex]);
    gfx->setCursor(188, 14);

    uint16_t txtCol = (themeIndex == 0) ? 0x07E0 : uiColors[themeIndex];
    gfx->setTextColor(txtCol);
    gfx->setTextSize(2);
    gfx->print(score);
    lastScore = score;
  }

  // stars scroll in the background behind the ship
  for (int i = 0; i < NUM_STARS; i++) {
    eraseStar(i);

    if (starType[i] == 2) {
      starY[i] += 4;
      starX[i] += 2;
    } else {
      starY[i] += 1;
    }

    if (starY[i] > SCREEN_HEIGHT || starX[i] > SCREEN_WIDTH + 4) {
      placeStar(i, false);
    }

    drawStar(i, (themeIndex == 0) ? 0xFFFF : uiColors[themeIndex]);
  }

  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int touchX = getTouchScreenX(p);
    int touchY = getTouchScreenY(p);

    if (touchY >= TOUCH_CONTROL_TOP) {
      int oldX = px;

      if (touchX <= TOUCH_SPLIT_X - TOUCH_DEADZONE) {
        player_lane = 0;
        px = LEFT_LANE;
      } else if (touchX >= TOUCH_SPLIT_X + TOUCH_DEADZONE) {
        player_lane = 1;
        px = RIGHT_LANE;
      }

      if (oldX != px) {
        drawPlayer(oldX, py, bgColors[themeIndex], bgColors[themeIndex],
                   bgColors[themeIndex]);
        drawPlayer(px, py, shipC1[themeIndex], shipC2[themeIndex],
                   shipC3[themeIndex]);
      }
    }
  }

  if (spawnHeart) {
    int oldHY = heartY;
    heartY += spd;

    if (heartY > SCREEN_HEIGHT) {
      gfx->fillRect(heartX - 12, oldHY - 10, 24, 24, bgColors[themeIndex]);
      spawnHeart = false;
    } else {
      gfx->fillRect(heartX - 12, oldHY - 10, 24, 24, bgColors[themeIndex]);
      drawHeart(heartX, heartY, 0xF800);
    }

    if (heartY + 10 >= py - 20 && heartY - 10 <= py + 20) {
      if (heartLane == player_lane) {
        gfx->fillRect(heartX - 12, heartY - 10, 24, 24, bgColors[themeIndex]);
        spawnHeart = false;

        if (lives < 3) {
          lives++;
          drawUI();
        }
        drawPlayer(px, py, shipC1[themeIndex], shipC2[themeIndex],
                   shipC3[themeIndex]);
      }
    }
  }

  int oldY = obstacleY;
  obstacleY += spd;

  if (obstacleY > SCREEN_HEIGHT) {
    gfx->fillRect(obstacleX - 24, oldY, 48, 48, bgColors[themeIndex]);
    spawnAsteroid();
  } else {
    gfx->fillRect(obstacleX - 24, oldY, 48, spd, bgColors[themeIndex]);
    drawAsteroid(obstacleX, obstacleY, obsColors[themeIndex],
                 bgColors[themeIndex], obstacleType);
  }

  drawPlayer(px, py, shipC1[themeIndex], shipC2[themeIndex], shipC3[themeIndex]);

  // same lane hit — drop a life or end the run
  if ((obstacleY + 48 >= py - 15) && (obstacleY <= py + 15)) {
    if (obstacleLane == player_lane) {
      lives--;
      if (lives <= 0) {
        game_over = true;
      } else {
        gfx->fillRect(obstacleX - 24, obstacleY, 48, 48, bgColors[themeIndex]);
        spawnAsteroid();
        drawUI();
      }
    }
  }

  delay(frameDelay);
}

void spawnAsteroid() {
  obstacleY = 45;
  obstacleLane = random(0, 2);
  if (obstacleLane == 0)
    obstacleX = LEFT_LANE;
  else
    obstacleX = RIGHT_LANE;
  obstacleType = random(0, 8);
}

void resetGame() {
  // wipe the board and start a clean round
  player_lane = 0;
  px = LEFT_LANE;
  obstacleY = 45;

  obstacleLane = random(0, 2);
  if (obstacleLane == 0)
    obstacleX = LEFT_LANE;
  else
    obstacleX = RIGHT_LANE;
  obstacleType = random(0, 8);

  game_over = false;
  spawnHeart = false;
  nextHealthScore = 350;

  lives = 3;
  score = 0;
  lastScore = -1;
  themeIndex = 0;
  frameDelay = 7;

  for (int i = 0; i < NUM_STARS; i++) {
    starType[i] = 0;
  }
  for (int i = 0; i < NUM_STARS; i++) {
    placeStar(i, true);
  }

  gfx->fillScreen(bgColors[themeIndex]);
  drawUI();
  drawPlayer(px, py, shipC1[themeIndex], shipC2[themeIndex], shipC3[themeIndex]);
  lastScoreTime = millis();
}

void drawUI() {
  gfx->fillRect(0, 0, SCREEN_WIDTH, 45, bgColors[themeIndex]);

  uint16_t borderCol = (themeIndex == 0) ? 0x07FF : uiColors[themeIndex];
  gfx->drawRect(0, 0, SCREEN_WIDTH, 45, borderCol);

  gfx->setCursor(116, 14);
  uint16_t textCol = (themeIndex == 0) ? 0x07E0 : uiColors[themeIndex];
  gfx->setTextColor(textCol);
  gfx->setTextSize(2);
  gfx->print("SCORE:");

  uint16_t heartColor = (themeIndex == 2) ? 0xFFFF : 0xF800;
  for (int i = 0; i < lives; i++) {
    drawHeart(25 + (i * 35), 20, heartColor);
  }
}

void drawHeart(int x, int y, uint16_t color) {
  int s = 2;
  int sx = x - 5 * s;
  int sy = y - 4 * s;
  uint16_t whiteCol = (color == 0xFFFF) ? color : 0xFFFF;

  gfx->fillRect(sx + 2 * s, sy + 0 * s, 2 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 7 * s, sy + 0 * s, 2 * s, 1 * s, 0x0000);

  gfx->fillRect(sx + 1 * s, sy + 1 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 2 * s, sy + 1 * s, 2 * s, 1 * s, whiteCol);
  gfx->fillRect(sx + 4 * s, sy + 1 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 6 * s, sy + 1 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 7 * s, sy + 1 * s, 2 * s, 1 * s, color);
  gfx->fillRect(sx + 9 * s, sy + 1 * s, 1 * s, 1 * s, 0x0000);

  gfx->fillRect(sx + 0 * s, sy + 2 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 1 * s, sy + 2 * s, 2 * s, 1 * s, whiteCol);
  gfx->fillRect(sx + 3 * s, sy + 2 * s, 2 * s, 1 * s, color);
  gfx->fillRect(sx + 5 * s, sy + 2 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 6 * s, sy + 2 * s, 4 * s, 1 * s, color);
  gfx->fillRect(sx + 10 * s, sy + 2 * s, 1 * s, 1 * s, 0x0000);

  gfx->fillRect(sx + 0 * s, sy + 3 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 1 * s, sy + 3 * s, 1 * s, 1 * s, whiteCol);
  gfx->fillRect(sx + 2 * s, sy + 3 * s, 8 * s, 1 * s, color);
  gfx->fillRect(sx + 10 * s, sy + 3 * s, 1 * s, 1 * s, 0x0000);

  gfx->fillRect(sx + 0 * s, sy + 4 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 1 * s, sy + 4 * s, 9 * s, 1 * s, color);
  gfx->fillRect(sx + 10 * s, sy + 4 * s, 1 * s, 1 * s, 0x0000);

  gfx->fillRect(sx + 1 * s, sy + 5 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 2 * s, sy + 5 * s, 7 * s, 1 * s, color);
  gfx->fillRect(sx + 9 * s, sy + 5 * s, 1 * s, 1 * s, 0x0000);

  gfx->fillRect(sx + 2 * s, sy + 6 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 3 * s, sy + 6 * s, 5 * s, 1 * s, color);
  gfx->fillRect(sx + 8 * s, sy + 6 * s, 1 * s, 1 * s, 0x0000);

  gfx->fillRect(sx + 3 * s, sy + 7 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 4 * s, sy + 7 * s, 3 * s, 1 * s, color);
  gfx->fillRect(sx + 7 * s, sy + 7 * s, 1 * s, 1 * s, 0x0000);

  gfx->fillRect(sx + 4 * s, sy + 8 * s, 1 * s, 1 * s, 0x0000);
  gfx->fillRect(sx + 5 * s, sy + 8 * s, 1 * s, 1 * s, color);
  gfx->fillRect(sx + 6 * s, sy + 8 * s, 1 * s, 1 * s, 0x0000);

  gfx->fillRect(sx + 5 * s, sy + 9 * s, 1 * s, 1 * s, 0x0000);
}

void drawPlayer(int playerX, int playerY, uint16_t c1,
                uint16_t c2, uint16_t c3) {
  int x = playerX;
  int y = playerY;

  gfx->fillCircle(x, y - 5, 8, c1);
  gfx->fillTriangle(x - 6, y - 8, x + 6, y - 8, x, y - 18, c1);
  gfx->fillTriangle(x - 5, y - 10, x - 5, y, x - 16, y - 4, c1);
  gfx->fillTriangle(x + 5, y - 10, x + 5, y, x + 16, y - 4, c1);
  gfx->fillTriangle(x - 6, y - 2, x, y + 2, x - 12, y + 10, c1);
  gfx->fillTriangle(x + 6, y - 2, x, y + 2, x + 12, y + 10, c1);

  gfx->drawFastVLine(x - 7, y - 23, 6, c1);
  gfx->fillCircle(x - 7, y - 24, 2, c1);
  gfx->drawFastVLine(x + 7, y - 23, 6, c1);
  gfx->fillCircle(x + 7, y - 24, 2, c1);

  gfx->drawLine(x - 8, y + 5, x - 12, y + 16, c1);
  gfx->fillCircle(x - 13, y + 17, 2, c1);
  gfx->drawLine(x + 8, y + 5, x + 12, y + 16, c1);
  gfx->fillCircle(x + 13, y + 17, 2, c1);

  uint16_t faceC = 0x0000;
  if (c1 == c2 && c2 == c3)
    faceC = c1;
  else if (c1 == 0x0000)
    faceC = 0xFFFF;

  gfx->fillCircle(x - 3, y - 5, 1, faceC);
  gfx->fillCircle(x + 3, y - 5, 1, faceC);
  gfx->drawFastHLine(x - 1, y - 2, 3, faceC);
  gfx->drawPixel(x - 2, y - 3, faceC);
  gfx->drawPixel(x + 2, y - 3, faceC);
}

void drawAsteroid(int astX, int astY, uint16_t assetColor,
                  uint16_t bgColor, int type) {
  int startX = astX - 24;
  for (int r = 0; r < 8; r++) {
    uint8_t rowByte = asteroids[type][r];
    int rowY = astY + (r * 6);
    int c = 0;
    while (c < 8) {
      bool bit = (rowByte & (0x80 >> c)) != 0;
      int span = 1;

      while (c + span < 8) {
        bool nextBit = (rowByte & (0x80 >> (c + span))) != 0;
        if (nextBit == bit)
          span++;
        else
          break;
      }

      uint16_t color = bit ? assetColor : bgColor;
      gfx->fillRect(startX + (c * 6), rowY, span * 6, 6, color);
      c += span;
    }
  }
}

void drawHome() {
  gfx->fillScreen(0x0000);

  for (int i = 0; i < 35; i++) {
    uint16_t starColor = 0xFFFF;
    if (i % 3 == 0)
      starColor = 0x07FF;
    else if (i % 2 == 0)
      starColor = 0xFDA0;

    gfx->fillRect(random(0, SCREEN_WIDTH), random(0, SCREEN_HEIGHT), 2, 2,
                  starColor);
  }

  drawPlayer(120, 85, 0xFFE0, 0x001F, 0xF800);
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

int getTouchScreenX(const TSPoint &p) {
  return constrain(map(p.x, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, SCREEN_WIDTH),
                   0, SCREEN_WIDTH);
}

int getTouchScreenY(const TSPoint &p) {
  return constrain(map(p.y, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, SCREEN_HEIGHT, 0),
                   0, SCREEN_HEIGHT);
}
