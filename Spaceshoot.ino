#include <Arduino_GFX_Library.h>
#include <TouchScreen.h>

#define YP A1
#define XM A2
#define YM 7
#define XP 6
#define MINPRESSURE 5
#define MAXPRESSURE 1500

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
Arduino_DataBus *bus = new Arduino_SWPAR8(A2, A3, A1, A0, 8, 9, 2, 3, 4, 5, 6, 7);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, A4, 0, false);

int playerX = 60;
int playerY = 270;
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
void setup() {
  gfx->begin();
  gfx->fillScreen(0x0000);
  drawUI();
  drawPlayer(playerX, playerY, 0x07FF, 0x001F, 0xF800);
}
void loop() {
  TSPoint p = ts.getPoint();
  
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if(p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int mX = map(p.y, 120, 940, 240, 0);
    int oldX = playerX;
    
    if(mX > 120) {
      playerX = 60;
    } else {
      playerX = 180;
    }
    
    if(oldX != playerX) {
      drawPlayer(oldX, playerY, 0x0000, 0x0000, 0x0000);
      
      for(int i = 250; i < 300; i += 25) {
         gfx->drawFastVLine(120, i, 12, 0x18E3);
      }
      
      drawPlayer(playerX, playerY, 0x07FF, 0x001F, 0xF800);
      delay(150);
    }
  }
}