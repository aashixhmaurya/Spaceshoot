#include <Arduino_GFX_Library.h>
#include <TouchScreen.h>

#define YP A1
#define XM A2
#define YM 7
#define XP 6
#define MINPRESSURE 5
#define MAXPRESSURE 2000

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

void drawPlayer(int x, int y, uint16_t c1, uint16_t c2, uint16_t c3) {
  gfx->fillTriangle(x, y - 20, x - 12, y + 10, x + 12, y + 10, c1);
  gfx->fillTriangle(x - 12, y + 10, x - 18, y + 20, x - 6, y + 10, c2);
  gfx->fillTriangle(x + 12, y + 10, x + 18, y + 20, x + 6, y + 10, c2);
  gfx->fillRect(x - 4, y + 10, 8, 12, c3);
}

void drawBackground() {
  for(int i = 0; i < 20; i++) {
    gfx->fillCircle(random(240), random(50, 320), 1, random(0xFFFF));
  }
  for(int i = 50; i < 320; i += 25) {
    gfx->drawFastVLine(120, i, 12, 0x18E3);
  }
}

void drawUI() {
  gfx->fillRect(0, 0, 240, 45, 0x0000);
  gfx->drawRect(0, 0, 240, 45, 0x07FF);
  gfx->setCursor(25, 12);
  gfx->setTextColor(0x07FF);
  gfx->setTextSize(3);
  gfx->print("SPACESHOOT");
  drawBackground();
}

void resetGame() {
  playerLane = 0;
  playerX = 60;
  obsY = 45;
  obsLane = random(0, 2);
  obsX = (obsLane == 0) ? 60 : 180;
  isGameOver = false;
  
  gfx->fillScreen(0x0000);
  drawUI();
  drawPlayer(playerX, playerY, 0x07FF, 0x001F, 0xF800);
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
      drawPlayer(oldX, playerY, 0x0000, 0x0000, 0x0000); 
      drawPlayer(playerX, playerY, 0x07FF, 0x001F, 0xF800); 
    }
  }

  gfx->fillRect(obsX - 15, obsY, 30, obsSpeed, 0x0000);
  
  obsY += obsSpeed;
  
  if(obsY > 320) {
    gfx->fillRect(obsX - 15, obsY - obsSpeed, 30, 20, 0x0000); 
    obsY = 45;
    obsLane = random(0, 2);
    obsX = (obsLane == 0) ? 60 : 180;
  }
  
  gfx->fillRect(obsX - 15, obsY, 30, 20, 0xFDA0);
  
  if(obsY + 20 >= playerY - 10 && obsY <= playerY + 10) {
    if(obsLane == playerLane) {
      isGameOver = true;
    }
  }
  
  delay(5); 
}