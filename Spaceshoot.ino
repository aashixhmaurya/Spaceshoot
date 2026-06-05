#include <Arduino_GFX_Library.h>
#include <TouchScreen.h>

#define YP A1
#define XM A2
#define YM 7
#define XP 6
#define MINPRESSURE 10
#define MAXPRESSURE 1000

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
Arduino_DataBus *bus = new Arduino_SWPAR8(A2, A3, A1, A0, 8, 9, 2, 3, 4, 5, 6, 7);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, A4, 0, false);

int playerX = 120;
int playerY = 280;

void drawPlayer(int x, int y, uint16_t mainColor, uint16_t engineColor) {
  gfx->fillTriangle(x, y - 15, x - 15, y + 15, x + 15, y + 15, mainColor);
  gfx->fillRect(x - 10, y + 15, 20, 10, engineColor);
  gfx->fillTriangle(x - 15, y + 15, x - 25, y + 25, x - 10, y + 15, engineColor);
  gfx->fillTriangle(x + 15, y + 15, x + 25, y + 25, x + 10, y + 15, engineColor);
}

void setup() {
  gfx->begin();
  gfx->fillScreen(0x0000);
  drawPlayer(playerX, playerY, 0x07FF, 0xF800);
}

void loop() {
  TSPoint p = ts.getPoint();
  
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if(p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int mX = map(p.x, 150, 920, 240, 0);
    int oldX = playerX;
    
    if(mX < 120 && playerX > 25) {
      playerX -= 5;
    } 
    else if(mX > 120 && playerX < 215) {
      playerX += 5;
    }
    
    if(oldX != playerX) {
      drawPlayer(oldX, playerY, 0x0000, 0x0000);
      drawPlayer(playerX, playerY, 0x07FF, 0xF800);
    }
  }
}