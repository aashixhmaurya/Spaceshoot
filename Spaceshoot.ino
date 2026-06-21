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

int game_mode = 0;
int p_lives = 3;

int p_x = 60;
int p_y = 290;
int p_lane = 0;

int rock_y = 45;
int rock_x = 60;
int rock_lane = 0;
int rock_spd = 3;
int rock_type = 0;
bool is_dead = false;

int heal_y = 45;
int heal_x = 60;
int heal_lane = 0;
bool spawning_heal = false;
int next_heal_at = 350;

int current_score = 0;
int prev_score = -1;
unsigned long last_tick = 0;
int fps_delay = 7;

int curr_color = 0;

int s_x[NUM_STARS];
int s_y[NUM_STARS];

int top_score = 0;
int eprom_addr = 0;

struct ThemeDef {
  uint16_t bg_col;
  uint16_t ui_col;
  uint16_t c1;
  uint16_t c2;
  uint16_t c3;
  uint16_t rock_col;
};

ThemeDef my_themes[12] = {
  {0x0000, 0xFFFF, 0xFFE0, 0x001F, 0xF800, 0xFDA0},
  {0xFFFF,0x0000, 0x001F,0xF800, 0xF800,0xF800},
  {0xF800, 0xFFFF, 0xFFE0, 0x0000, 0xFFE0, 0x07FF},
  {0x07E0, 0x0000, 0xF800, 0x001F, 0xF800, 0x001F},
  {0x001F, 0xFFFF, 0xFFE0,0x07E0, 0xF800, 0xFDA0},
  {0xFFE0, 0x0000, 0x001F, 0xF800, 0xF800, 0xF800},
  {0x07FF, 0x0000, 0xF800, 0x001F, 0xF800, 0x001F},
  {0xF81F, 0xFFFF, 0xFFE0, 0x001F, 0xFFE0, 0x07E0},
  {0xFC00,0x0000,0x001F,0xF800,0xF800,0x001F},
  {0x8010, 0xFFFF, 0xFFE0, 0x0000, 0xFFE0, 0xF800},
  {0xFE19, 0x0000, 0x001F, 0xF800, 0xF800, 0x07FF},
  {0x8200, 0xFFFF, 0x07FF, 0xFFE0, 0xF800, 0xFDA0}
};

uint8_t rock_data[3][8] = {
  {0x3C,0x7E,0xDF,0xFF,0xFB,0x7F,0x3E,0x1C},
  {0x1C, 0x3E, 0x7C, 0x78, 0x3C, 0x7E, 0x7E, 0x3C},
  {0x0C, 0x3E, 0x7F, 0xFC, 0xFE, 0x78, 0x3C, 0x18}
};

void renderPlayer(int xp, int yp, uint16_t col1, uint16_t col2, uint16_t col3) {
  gfx->fillCircle(xp, yp - 5, 8, col1);
  gfx->fillTriangle(xp - 6, yp - 8, xp + 6, yp - 8, xp, yp - 18, col1);
  gfx->fillTriangle(xp - 5, yp - 10, xp - 5, yp, xp - 16, yp - 4, col1);
  gfx->fillTriangle(xp + 5, yp - 10, xp + 5, yp, xp + 16, yp - 4, col1);
  gfx->fillTriangle(xp - 6, yp - 2, xp, yp + 2, xp - 12, yp + 10, col1);
  gfx->fillTriangle(xp + 6, yp - 2, xp, yp + 2, xp + 12, yp + 10, col1);

  gfx->drawFastVLine(xp - 7, yp - 23, 6, col1);
  gfx->fillCircle(xp - 7, yp - 24, 2, col1);
  gfx->drawFastVLine(xp + 7, yp - 23, 6, col1);
  gfx->fillCircle(xp + 7, yp - 24, 2, col1);

  gfx->drawLine(xp - 8, yp + 5, xp - 12, yp + 16, col1);
  gfx->fillCircle(xp - 13, yp + 17, 2, col1);
  gfx->drawLine(xp + 8, yp + 5, xp + 12, yp + 16, col1);
  gfx->fillCircle(xp + 13, yp + 17, 2, col1);

  uint16_t f_col;
  if(col1 == col2 && col2 == col3) { 
    f_col = col1; 
  } else {
    f_col = (col1 == 0x0000) ? 0xFFFF : 0x0000;
  }
  
  gfx->fillCircle(xp - 3, yp - 5, 1, f_col);
  gfx->fillCircle(xp + 3, yp - 5, 1, f_col);
  gfx->drawFastHLine(xp - 1, yp - 2, 3, f_col);
  gfx->drawPixel(xp - 2, yp - 3, f_col);
  gfx->drawPixel(xp + 2, yp - 3, f_col);
}

void renderRock(int x_pos, int y_pos, uint16_t a_col, uint16_t b_col, int r_type) {
  int st_x = x_pos - 24;
  for(int r = 0; r < 8; r++) {
    uint8_t r_byte = rock_data[r_type][r];
    for(int c = 0; c < 8; c++) {
      if(r_byte & (1 << (7 - c))) {
        gfx->fillRect(st_x + (c * 6), y_pos + (r * 6), 6, 6, a_col);
      } else {
        gfx->fillRect(st_x + (c * 6), y_pos + (r * 6), 6, 6, b_col);
      }
    }
  }
}

void renderLife(int hx, int hy, uint16_t hc) {
  int sx = hx - 10;
  int sy = hy - 8;

  gfx->fillRect(sx + 4, sy, 4, 2, 0x0000);
  gfx->fillRect(sx + 14, sy, 4, 2, 0x0000);
  
  gfx->fillRect(sx + 2, sy + 2, 2, 2, 0x0000);
  gfx->fillRect(sx + 4, sy + 2, 4, 2, (hc == 0xFFFF) ? hc : 0xFFFF);
  gfx->fillRect(sx + 8, sy + 2, 2, 2, 0x0000);
  gfx->fillRect(sx + 12, sy + 2, 2, 2, 0x0000);
  gfx->fillRect(sx + 14, sy + 2, 4, 2, hc);
  gfx->fillRect(sx + 18, sy + 2, 2, 2, 0x0000);
  
  gfx->fillRect(sx, sy + 4, 2, 2, 0x0000);
  gfx->fillRect(sx + 2, sy + 4, 4, 2, (hc == 0xFFFF) ? hc : 0xFFFF);
  gfx->fillRect(sx + 6, sy + 4, 4, 2, hc);
  gfx->fillRect(sx + 10, sy + 4, 2, 2, 0x0000);
  gfx->fillRect(sx + 12, sy + 4, 8, 2, hc);
  gfx->fillRect(sx + 20, sy + 4, 2, 2, 0x0000);
  
  gfx->fillRect(sx, sy + 6, 2, 2, 0x0000);
  gfx->fillRect(sx + 2, sy + 6, 2, 2, (hc == 0xFFFF) ? hc : 0xFFFF);
  gfx->fillRect(sx + 4, sy + 6, 16, 2, hc);
  gfx->fillRect(sx + 20, sy + 6, 2, 2, 0x0000);
  
  gfx->fillRect(sx, sy + 8, 2, 2, 0x0000);
  gfx->fillRect(sx + 2, sy + 8, 18, 2, hc);
  gfx->fillRect(sx + 20, sy + 8, 2, 2, 0x0000);
  
  gfx->fillRect(sx + 2, sy + 10, 2, 2, 0x0000);
  gfx->fillRect(sx + 4, sy + 10, 14, 2, hc);
  gfx->fillRect(sx + 18, sy + 10, 2, 2, 0x0000);
  
  gfx->fillRect(sx + 4, sy + 12, 2, 2, 0x0000);
  gfx->fillRect(sx + 6, sy + 12, 10, 2, hc);
  gfx->fillRect(sx + 16, sy + 12, 2, 2, 0x0000);
  
  gfx->fillRect(sx + 6, sy + 14, 2, 2, 0x0000);
  gfx->fillRect(sx + 8, sy + 14, 6, 2, hc);
  gfx->fillRect(sx + 14, sy + 14, 2, 2, 0x0000);
  
  gfx->fillRect(sx + 8, sy + 16, 2, 2, 0x0000);
  gfx->fillRect(sx + 10, sy + 16, 2, 2, hc);
  gfx->fillRect(sx + 12, sy + 16, 2, 2, 0x0000);
  
  gfx->fillRect(sx + 10, sy + 18, 2, 2, 0x0000);
}

void showMenu() {
  gfx->fillScreen(0x0000);
  
  for(int i = 0; i < 35; i++) {
    uint16_t s_col = (i % 3 == 0) ? 0x07FF : ((i % 2 == 0) ? 0xFDA0 : 0xFFFF);
    gfx->fillRect(random(0, 240), random(0, 320), 2, 2, s_col);
  }

  renderPlayer(120, 85, 0xFFE0, 0x001F, 0xF800);
  
  gfx->fillRect(118, 25, 4, 35, 0xF800);
  gfx->fillRect(116, 20, 8, 5, 0xFFFF);
  
  renderRock(40, 35, 0xFDA0, 0x0000, 0);
  renderRock(200, 95, 0x07E0, 0x0000, 1);
  renderRock(55, 115, 0xF800, 0x0000, 2);

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

void updateHUD() {
  gfx->fillRect(0, 0, 240, 45, my_themes[curr_color].bg_col);
  gfx->drawRect(0, 0, 240, 45, curr_color == 0 ? 0x07FF : my_themes[curr_color].ui_col);
  
  gfx->setCursor(116, 14);
  gfx->setTextColor(curr_color == 0 ? 0x07E0 : my_themes[curr_color].ui_col);
  gfx->setTextSize(2);
  gfx->print("SCORE:");

  uint16_t h_color = (curr_color == 2) ? 0xFFFF : 0xF800;
  for(int i = 0; i < p_lives; i++) {
    renderLife(25 + (i * 35), 20, h_color);
  }
}

void initPlay() {
  p_lane = 0;
  p_x = 60;
  rock_y = 45;
  rock_lane = random(0, 2);
  rock_x = (rock_lane == 0) ? 60 : 180;
  rock_type = random(0, 3);
  is_dead = false;
  spawning_heal = false;
  next_heal_at = 350;
  p_lives = 3;
  current_score = 0;
  prev_score = -1;
  curr_color = 0;
  fps_delay = 7;
  
  for(int i = 0; i < NUM_STARS; i++) {
    s_x[i] = random(0, 240);
    s_y[i] = random(46, 320);
  }
  
  gfx->fillScreen(my_themes[curr_color].bg_col);
  updateHUD();
  renderPlayer(p_x, p_y, my_themes[curr_color].c1, my_themes[curr_color].c2, my_themes[curr_color].c3);
  last_tick = millis();
}

void setup() {
  gfx->begin();
  randomSeed(analogRead(A5));
  
  EEPROM.get(eprom_addr, top_score);
  if(top_score < 0 || top_score > 30000) {
    top_score = 0;
  }
  
  game_mode = 0;
  showMenu();
  delay(500);
}

void loop() {
  if(game_mode == 0) {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    
    if(p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      game_mode = 1;
      initPlay();
      delay(200);
    }
    return;
  }

  if(is_dead) {
    if(current_score > top_score) {
      top_score = current_score;
      EEPROM.put(eprom_addr, top_score);
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
    gfx->print(current_score);
    
    gfx->setCursor(15, 170);
    gfx->setTextColor(0x07FF);
    gfx->setTextSize(2);
    gfx->print("HIGHEST SCORE: ");
    gfx->print(top_score);
    
    gfx->setCursor(54, 280);
    gfx->setTextColor(0xFFFF);
    gfx->setTextSize(2);  
    gfx->print("TOUCH TO HOME");
    
    while(1) {
      TSPoint p = ts.getPoint();
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      
      if(p.z > MINPRESSURE && p.z < MAXPRESSURE) {
        game_mode = 0;
        showMenu();
        delay(200);
        return;
      }
      delay(10);
    }
  }

  if(millis() - last_tick >= 150) {
    current_score++;
    last_tick = millis();
  }

  if(current_score >= next_heal_at && !spawning_heal) {
    spawning_heal = true;
    heal_lane = random(0, 2);
    heal_x = (heal_lane == 0) ? 60 : 180;
    heal_y = 45;
    next_heal_at += 350;
  }

  int lvl = current_score / 100;
  int n_color = lvl % 12;
  fps_delay = 7 - lvl;
  if(fps_delay < 0) fps_delay = 0;

  if(n_color != curr_color) {
    curr_color = n_color;
    gfx->fillScreen(my_themes[curr_color].bg_col);
    updateHUD();
    renderPlayer(p_x, p_y, my_themes[curr_color].c1, my_themes[curr_color].c2, my_themes[curr_color].c3);
  }

  if(current_score != prev_score) {
    gfx->fillRect(188, 10, 50, 25, my_themes[curr_color].bg_col);
    gfx->setCursor(188, 14);
    gfx->setTextColor(curr_color == 0 ? 0x07E0 : my_themes[curr_color].ui_col);
    gfx->setTextSize(2);
    gfx->print(current_score);
    prev_score = current_score;
  }

  for(int i = 0; i < NUM_STARS; i++) {
    bool clr_old = (s_x[i] + 2 > p_x - 18 && s_x[i] < p_x + 18 && s_y[i] + 2 > p_y - 26 && s_y[i] < p_y + 20);
    if(!clr_old) {
      gfx->fillRect(s_x[i], s_y[i], 2, 2, my_themes[curr_color].bg_col);
    }
    
    s_y[i] += 1;
    if(s_y[i] > 320) {
      s_y[i] = 46;
      s_x[i] = random(0, 240);
    }
    
    bool draw_new = (s_x[i] + 2 > p_x - 18 && s_x[i] < p_x + 18 && s_y[i] + 2 > p_y - 26 && s_y[i] < p_y + 20);
    if(!draw_new) {
      gfx->fillRect(s_x[i], s_y[i], 2, 2, curr_color == 0 ? 0xFFFF : my_themes[curr_color].ui_col);
    }
  }

  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if(p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int map_x = map(p.y, 120, 940, 240, 0);
    int o_x = p_x;
    
    if(map_x > 120) {
      p_lane = 0;
      p_x = 60;
    } else {
      p_lane = 1;
      p_x = 180;
    }
    
    if(o_x != p_x) {
      renderPlayer(o_x, p_y, my_themes[curr_color].bg_col, my_themes[curr_color].bg_col, my_themes[curr_color].bg_col);
      renderPlayer(p_x, p_y, my_themes[curr_color].c1, my_themes[curr_color].c2, my_themes[curr_color].c3);
    }
  }

  if(spawning_heal) {
    int o_hy = heal_y;
    heal_y += rock_spd;
    if(heal_y > 320) {
      gfx->fillRect(heal_x - 12, o_hy - 10, 24, 24, my_themes[curr_color].bg_col);
      spawning_heal = false;
    } else {
      gfx->fillRect(heal_x - 12, o_hy - 10, 24, 24, my_themes[curr_color].bg_col);
      renderLife(heal_x, heal_y, 0xF800);
    }
    
    if(heal_y + 10 >= p_y - 20 && heal_y - 10 <= p_y + 20) {
      if(heal_lane == p_lane) {
        gfx->fillRect(heal_x - 12, heal_y - 10, 24, 24, my_themes[curr_color].bg_col);
        spawning_heal = false;
        if(p_lives < 3) {
          p_lives++;
          updateHUD();
        }
        renderPlayer(p_x, p_y, my_themes[curr_color].c1, my_themes[curr_color].c2, my_themes[curr_color].c3);
      }
    }
  }

  int o_ry = rock_y;
  rock_y += rock_spd;

  if(rock_y > 320) {
    gfx->fillRect(rock_x - 24, o_ry, 48, 48, my_themes[curr_color].bg_col);
    rock_y = 45;
    rock_lane = random(0, 2);
    rock_x = (rock_lane == 0) ? 60 : 180;
    rock_type = random(0, 3);
  } else {
    gfx->fillRect(rock_x - 24, o_ry, 48, rock_spd, my_themes[curr_color].bg_col);
    renderRock(rock_x, rock_y, my_themes[curr_color].rock_col, my_themes[curr_color].bg_col, rock_type);
  }

  renderPlayer(p_x, p_y, my_themes[curr_color].c1, my_themes[curr_color].c2, my_themes[curr_color].c3);

  if(rock_y + 48 >= p_y - 15 && rock_y <= p_y + 15) {
    if(rock_lane == p_lane) {
      p_lives--;
      if(p_lives <= 0) {
        is_dead = true;
      } else {
        gfx->fillRect(rock_x - 24, rock_y, 48, 48, my_themes[curr_color].bg_col);
        rock_y = 45;
        rock_lane = random(0, 2);
        rock_x = (rock_lane == 0) ? 60 : 180;
        rock_type = random(0, 3);
        updateHUD();
      }
    }
  }

  delay(fps_delay);
}