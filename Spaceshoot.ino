#include <Arduino_GFX_Library.h>
#include <TouchScreen.h>

#define YP A1
#define XM A2
#define YM 7
#define XP 6
#define MINPRESSURE 2
#define MAXPRESSURE 3000

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
Arduino_DataBus *bus = new Arduino_SWPAR8(A2, A3, A1, A0, 8, 9, 2, 3, 4, 5, 6, 7);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, A4, 0, false);

int playerX = 60;
int playerY = 270;
int playerLane = 0;

int obsY = 45;
int obsX = 60;
int obsLane = 0;
int obsSpeed = 1; 

bool isGameOver = false;

int score = 0;
int lastScore = -1;
unsigned long lastScoreTime = 0;
int frameDelay = 7;

int colorIndex = 0;

int starX[5];
int starY[5];

uint16_t bgColors[12] = {0x0000, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F, 0xFC00, 0x8010, 0xFE19, 0x8200};
uint16_t uiColors[12] = {0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF};
uint16_t shipC1[12] = {0x07FF, 0x001F, 0xFFE0, 0xF800, 0xFFE0, 0x001F, 0xF800, 0xFFE0, 0x001F, 0xFFE0, 0x001F, 0x07FF};
uint16_t shipC2[12] = {0x001F, 0xF800, 0x0000, 0x001F, 0x07E0, 0xF800, 0x001F, 0x001F, 0xF800, 0x0000, 0xF800, 0xFFE0};
uint16_t shipC3[12] = {0xF800, 0xF800, 0xFFE0, 0xF800, 0xF800, 0xF800, 0xF800, 0xFFE0, 0xF800, 0xFFE0, 0xF800, 0xF800};
uint16_t obsColors[12] = {0xFDA0, 0xF800, 0x07FF, 0x001F, 0xFDA0, 0xF800, 0x001F, 0x07E0, 0x001F, 0xF800, 0x07FF, 0xFDA0};

void drawPlayer(int x, int y, uint16_t c1, uint16_t c2, uint16_t c3) {
  gfx->fillTriangle(x, y - 20, x - 12, y + 10, x + 12, y + 10, c1);
  gfx->fillTriangle(x - 12, y + 10, x - 18, y + 20, x - 6, y + 10, c2);
  gfx->fillTriangle(x + 12, y + 10, x + 18, y + 20, x + 6, y + 10, c2);
  gfx->fillRect(x - 4, y + 10, 8, 12, c3);
}

void drawUI() {
  gfx->fillRect(0, 0, 240, 45, bgColors[colorIndex]);
  gfx->drawRect(0, 0, 240, 45, colorIndex == 0 ? 0x07FF : uiColors[colorIndex]);
  
  gfx->setCursor(10, 14);
  gfx->setTextColor(colorIndex == 0 ? 0x07FF : uiColors[colorIndex]);
  gfx->setTextSize(2);
  gfx->print("SPACESHOOT");
  
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
  isGameOver = false;
  
  score = 0;
  lastScore = -1;
  colorIndex = 0;
  frameDelay = 7;
  
  for(int i = 0; i < 5; i++) {
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
  resetGame();
}

void loop() {
  if(isGameOver) {
    gfx->fillScreen(0xF800);
    
    gfx->setCursor(12, 70);
    gfx->setTextColor(0xFFFF);
    gfx->setTextSize(4);
    gfx->print("GAME OVER");
    
    gfx->setCursor(54, 280);
    gfx->setTextColor(0xFFFF);
    gfx->setTextSize(2);
    gfx->print("TOUCH TO GO");
    
    while(true) {
      TSPoint p = ts.getPoint();
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      
      if(p.z > MINPRESSURE && p.z < MAXPRESSURE) {
        resetGame();
        return;
      }
      delay(10); 
    }
  }

  if(millis() - lastScoreTime >= 150) {
    score++;
    lastScoreTime = millis();
  }

  int speedLevel = score / 100;
  int newColorIndex = speedLevel % 12; 
  
  frameDelay = 7 - speedLevel; 
  if(frameDelay < 0) frameDelay = 0;

  if(newColorIndex != colorIndex) {
    colorIndex = newColorIndex;
    gfx->fillScreen(bgColors[colorIndex]);
    drawUI();
    drawPlayer(playerX, playerY, shipC1[colorIndex], shipC2[colorIndex], shipC3[colorIndex]);
  }

  if(score != lastScore) {
    gfx->fillRect(188, 10, 50, 25, bgColors[colorIndex]);
    gfx->setCursor(188, 14);
    gfx->setTextColor(colorIndex == 0 ? 0x07E0 : uiColors[colorIndex]);
    gfx->setTextSize(2);
    gfx->print(score);
    lastScore = score;
  }

  for(int i = 0; i < 5; i++) {
    gfx->drawPixel(starX[i], starY[i], bgColors[colorIndex]);
    starY[i] += 1;
    if(starY[i] > 320) {
      starY[i] = 46;
      starX[i] = random(0, 240);
    }
    gfx->drawPixel(starX[i], starY[i], colorIndex == 0 ? 0xFFFF : uiColors[colorIndex]);
  }

  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if(p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int mX = map(p.y, 120, 940, 240, 0);
    int oldX = playerX;
    
    if(mX > 120) {
      playerLane = 0;
      playerX = 60;
    } else {
      playerLane = 1;
      playerX = 180;
    }
    
    if(oldX != playerX) {
      drawPlayer(oldX, playerY, bgColors[colorIndex], bgColors[colorIndex], bgColors[colorIndex]); 
      drawPlayer(playerX, playerY, shipC1[colorIndex], shipC2[colorIndex], shipC3[colorIndex]); 
    }
  }

  gfx->fillRect(obsX - 15, obsY, 30, obsSpeed, bgColors[colorIndex]);
  
  obsY += obsSpeed;
  
  if(obsY > 320) {
    gfx->fillRect(obsX - 15, obsY - obsSpeed, 30, 20, bgColors[colorIndex]); 
    obsY = 45;
    obsLane = random(0, 2);
    obsX = (obsLane == 0) ? 60 : 180;
  }
  
  gfx->fillRect(obsX - 15, obsY, 30, 20, obsColors[colorIndex]);
  
  for(int i = 50; i < 320; i += 25) {
    gfx->drawFastVLine(120, i, 12, colorIndex == 0 ? 0x18E3 : uiColors[colorIndex]);
  }
  
  
  if(obsY + 20 >= playerY - 10 && obsY <= playerY + 10) {
    if(obsLane == playerLane) {
      isGameOver = true;
    }
  }
  
  delay(frameDelay); 
}