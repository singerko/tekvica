#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <math.h>
#include <ctype.h>

#include "tekvica.h"

/* ================== Types first ================== */
struct Lids { int topY, botY; };
struct Now  { uint8_t h, m, s; };
struct AngryEye {
  float a;        // 0 = normálne oko, 1 = zlé šikmé
  float b;        // 0 = otvorené, 1 = zatvorené (žmurk)
  float glance;   // posun pohľadu v px (+ = doprava pre diváka)
  float pdy;      // posun zrenice nadol (smutné oči)
  float tx, ty;   // slza (ty = 0 -> žiadna)
};
struct AngryGeo { int pcx, pcy, pr, spanBot, tx, ty; };
// nové gagy 17-23
typedef uint16_t (*GagShadeFn)(int x, int y, const void *ctx);
struct HeartCtx  { float hx, hy, k; int pr; };
struct KoCtx     { float sc, ct, st, starsOn, starA; float sx[3], sy[3]; int pr; };
struct SpiderCtx { bool on; float sx, sy; int pcx, pcy; float kx[8], ky[8], fx[8], fy[8]; };
// gagy 24-29
struct BatCtx    { bool vis[4]; float bx[4], by[4], s[4], f[4]; int pcx, pcy; };
struct FireCtx   { float g, t, ph; int pr; };
struct ScanCtx   { float g, pos, dir; int off, pr; uint16_t scl; };
struct ZombieCtx { int e, pcx, pcy, pr, ir, lidY; float g; uint16_t scl; };
struct SlimeCtx  { float g; int16_t band[221]; float bandAt[7], L[7]; float dropX, dropY; int pcx, pcy; };
struct NightCtx  { int e; float g, t, side, phase, shoot, shx, shy; };
// gagy 30-33
struct AquaCtx   { int e; float g, t, dir, fx, fy; bool fishVis; float by[8]; };
struct AquaFast  { int e; float dir, fx, fy; bool fishVis; float bx[8], by[8], ws[3]; };   // gag 30 (rýchle kreslenie)
struct SlotCtx   { float g, pos; uint16_t frame; };
struct ZapCtx    { float g; bool flash; };
struct CycCtx    { float g, icx, icy; int lidY; };
// gagy 34-36
struct WitchCtx  { float g, mx, wx, wy, cape; bool vis; };
struct WitchSt   { bool vis; int wx, wy, ph, mx; };   // gag 34 (rýchle kreslenie)
struct RadarCtx  { int e; float g, ang; bool lock; };
struct CandleCtx { float g, fx, fh, sway, glow; };
// gag 37
// gagy 38-40
struct VampCtx   { float g, icx, lidY, dropX, dropY; };
struct WolfCtx   { float g, cx, cy, slit; };
struct KeyCtx    { float g; int pcx, pcy; };
struct JawCtx    { float g, h; };   // gag 41
struct QuizCtx   { float g, ring; int shown, len, sc, tx0, ty0; uint16_t col, ringCol; char txt[8]; };   // gag 42
struct ClockCtx  { float g, hourA, minA, secA; bool secOn; };   // gag 43
struct PortalCtx { float g, open, phase, gu, gy, fdir, ns; bool ghost; uint16_t ringCol, ringDark; };   // gag 44
struct LoopCtx    { bool vis; float cx, cy, ca, sa; int px, py; float lid; };   // gag 50
struct SixCtx     { float pos; int base, yoff; uint16_t col; bool only; };   // gag 49
struct RedEyeSt   { float ns, lid, sq, look; int pcx, pcy; };   // gag 47
struct FlameSt    { int e; float ns, lid, look, fl, t; int icx; };   // gag 48
struct DragonSt   { bool mirror; float fx, fy, slit, lid; int icx, icy; };   // gag 46
struct GiantFrame { int e; float g, lid, wink; int pcx, pcy, pr, ir, hlx, hly, hlr, yT, yB; int32_t pr2, ir2, ird2, il1, il2, hlr2; uint16_t skin, outline; };   // gag 45
struct WkPrim     { int16_t x0, y0, x1, y1; uint8_t kind; float ax, ay, bx, by, r, r2; uint16_t col; };   // gag 52
struct WkLayer    { uint8_t first, ns, n; int16_t x0, y0, x1, y1; uint16_t fill, line; };   // gag 52
struct WalkCtx    { WkPrim p[40]; WkLayer L[10]; uint8_t n, nl; bool fig; };   // gag 52
struct TentCtx   { float px[3][20], py[3][20], pr[3][20], side[3]; int bx0[3], bx1[3], by0[3]; int pcx, pcy; };

/* ================== HW & PARAMS ================== */
#define TFT_SCK   18
#define TFT_MOSI  19
// MISO unused

// LEFT EYE HW (logical "left"; physical swap handled later)
#define TFTL_DC   16
#define TFTL_CS   17
#define TFTL_RST  20

// RIGHT EYE HW
#define TFTR_DC   14
#define TFTR_CS   21
#define TFTR_RST  22

// DS3231 (RTC)
#define I2C_SDA     4
#define I2C_SCL     5
#define DS3231_ADDR 0x68

// Display & eye
// #define SPI_HZ    62500000UL
#define SPI_HZ       20000000UL
#define W         240
#define H         240
#define CX        120
#define CY        120
#define EYE_R     110
#define PUPIL_R    36

// Colors (RGB565)
#define COL_BG        0x0000
#define COL_SCLERA    0xFFFF
#define COL_PUPIL     0x0000
#define COL_LID_TOP   0xE461
#define COL_LID_BOT   0xFD20
#define COL_LID_SEAM  0xA2A6
#define COL_HILITE    0xFFFF

// Petal colors (RGB565)
#define COL_GREEN   0x07E0
#define COL_RED     0xF800
#define COL_BLUE    0x001F
#define COL_BROWN   0xA145
#define COL_PURPLE  0x780F
#define COL_ORANGE  0xFD20

// Farby pre strašidelný mód
#define COL_SCARY_RED     0xF800  // jasná červená
#define COL_SCARY_PUPIL   0x0000  // čierna zrenica
#define COL_SCARY_LID     0xD000  // tmavšia červená pre viečka (lepší kontrast)

// Farby pre gagy 17-23
#define COL_HEART       0xD8A7   // karmínová
#define COL_STAR        0xFE60   // žltá
#define COL_TEAR        0x555F   // svetlomodrá
#define COL_THREAD      0x8410   // sivá
#define COL_SPIDER      0x0000
#define COL_SPIDER_EYE  0xF800
#define COL_IRIS        0xE104   // červená dúhovka
#define COL_IRIS_RING   0x9000   // tmavý okraj dúhovky
#define COL_VEIN_DARK   0xB000
#define COL_VEIN        0xE000
#define COL_BAT         0x28A6   // tmavofialový netopier
#define COL_FIRE_Y      0xFEE7
#define COL_FIRE_O      0xFC60
#define COL_FIRE_R      0xF980
#define COL_FIRE_D      0xC8A0
#define COL_SCAN_DARK   0x18A3   // stmavnuté oko robota
#define COL_SCAN_BAND   0x3800   // tmavočervený pás
#define COL_SCAN_HOT    0xF800
#define COL_SCAN_CORE   0xFA08
#define COL_ZOMBIE_SCL  0xF799   // zažltnuté bielko
#define COL_ZOMBIE_IRIS 0x7D0B
#define COL_ZOMBIE_RING 0x5408
#define COL_ZOMBIE_PUP  0x1942
#define COL_ZOMBIE_HL   0xDF39
#define COL_ZOMBIE_SPOT 0x854B
#define COL_SLIME       0x5647
#define COL_SLIME_L     0x978F
#define COL_SLIME_D     0x2C63
#define COL_NIGHT_SKY   0x08A7
#define COL_MOON        0xFF2F
#define COL_STARC       0xFFFB
#define COL_FISH        0xFC60
#define COL_FISH_D      0xFB60
#define COL_WATER       0x7DFF
#define COL_WATER_D     0x3379
#define COL_BUBBLE      0xD75F
#define COL_WEED        0x2D07
#define COL_STEM        0x5465
#define COL_PUMPKIN     0xFC60
#define COL_GHOST       0xE71E
#define COL_BONE        0xEF5A
#define COL_SLOT_BG     0xFFFF
#define COL_SLOT_SHADE  0x7BEF
#define COL_SLOT_FRAME  0x3187
#define COL_ZAP_SKY     0x10A6
#define COL_ZAP_FLASH   0xCE9F
#define COL_ZAP_GLOW    0x7E5F
#define COL_CYC_IRIS    0x3CAB
#define COL_CYC_LIGHT   0x5DEF
#define COL_CYC_RING    0x1B27
#define COL_DUSK        0xFD47   // oranžová obloha
#define COL_MOON_PALE   0xFF32
#define COL_RADAR_BG    0x0141
#define COL_RADAR_GRID  0x1C67
#define COL_RADAR_TRAIL 0x13C6
#define COL_RADAR_BEAM  0x7FF1
#define COL_RADAR_BLIP  0x97F2
#define COL_CANDLE_BG   0x18A1
#define COL_CANDLE_GLOW 0x8AC3
#define COL_FLAME       0xFD43
#define COL_FLAME_CORE  0xFF92
#define COL_WAX         0xF739
#define COL_WAX_D       0xC610
#define COL_TENT        0x89F4   // fialové chápadlo
#define COL_TENT_SUCK   0xE5BC   // prísavky
#define COL_TENT_EDGE   0x58ED   // tmavý okraj
#define COL_VAMP_WING   0x1843
#define COL_VAMP_EDGE   0x4008
#define COL_VAMP_IRIS   0xF0E3
#define COL_VAMP_RING   0xC800
#define COL_BLOOD       0xA800
#define COL_FUR         0x59E5
#define COL_FUR_TIP     0x3123
#define COL_WOLF_MOON   0xFE45
#define COL_WOLF_IRIS   0xFCA0
#define COL_WOLF_RING   0xB200
#define COL_KEY_LIGHT   0xFF99
#define COL_LIP         0xCAC0
#define COL_MOUTH       0x7802
#define COL_TONGUE      0xF22E
#define COL_QUIZ_BG     0x1865   // tmavofialová tabuľa
#define COL_QUIZ_TXT    0xFC60   // oranžové číslice
#define COL_QUIZ_ASK    0xFFFF   // „=?“
#define COL_QUIZ_ANS    0xFFFF   // výsledok
#define COL_QUIZ_HURRY  0xF800   // 3-2-1 a blikajúci kruh
#define COL_QUIZ_RING   0xFC60   // časový kruh
#define COL_QUIZ_TRACK  0x3166   // zhasnutá časť kruhu
#define COL_QUIZ_SHADOW 0x0000   // tieň číslic
#define COL_CLK_FACE    0xFF9B   // krémový ciferník
#define COL_CLK_HAND    0x0000   // čierne ručičky a ryhy
#define COL_CLK_NUM     0x30C6   // tmavé čísla
#define COL_CLK_SEC     0xF800   // červená sekundovka
#define COL_PORTAL_BG   0x0862   // tmavé pozadie portálov
#define COL_PORTAL_B    0x2C7F   // modrý portál
#define COL_PORTAL_BD   0x094B
#define COL_PORTAL_O    0xFC60   // oranžový portál
#define COL_PORTAL_OD   0x5940
#define COL_GIANT_IRIS  0x3CA7   // zelená dúhovka obrieho oka
#define COL_GIANT_IRIS_L 0x7E4B
#define COL_GIANT_IRIS_D 0x1A83
#define COL_GIANT_IRIS_M 0x5D69   // medzi svetlou a zelenou

// PWM pre jas oboch očí
#define LEFT_BLK_PIN   13
#define RIGHT_BLK_PIN  12
static uint8_t leftEyeBrightness = 255;   // 0-255
static uint8_t rightEyeBrightness = 255;  // 0-255

// Sleep peek (občasné pohľady počas spánku)
static unsigned long nextSleepPeekMs = 0;
static bool sleepPeekInProgress = false;

static inline uint16_t pickRandomPetalColor(){
  static const uint16_t C[] = { COL_GREEN, COL_RED, COL_BLUE, COL_BROWN, COL_PURPLE, COL_ORANGE };
  return C[rand() % (sizeof(C)/sizeof(C[0]))];
}

// Universal pupil tiles (safe at edges)
#define TILE_SZ  96
#define MAX_STEP 6
static uint16_t tileBufL[TILE_SZ * TILE_SZ];
static uint16_t tileBufR[TILE_SZ * TILE_SZ];   // <- dôležité ;

 // BIG tiles len pre kvietkový gag
#define PET_TILE 160
static uint16_t petBufL[PET_TILE * PET_TILE];
static uint16_t petBufR[PET_TILE * PET_TILE];

// Blink timing
#define BLINK_STEPS        4
#define BLINK_STEP_DELAY   8

// Swap physical displays (1 if wires are swapped)
#define SWAP_PHYSICAL_EYES 1

/* ================== GFX ================== */
Arduino_DataBus *busL = new Arduino_RPiPicoSPI(TFTL_DC, TFTL_CS, TFT_SCK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_DataBus *busR = new Arduino_RPiPicoSPI(TFTR_DC, TFTR_CS, TFT_SCK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_GFX     *gfxL = new Arduino_GC9A01(busL, TFTL_RST, 0, true);
Arduino_GFX     *gfxR = new Arduino_GC9A01(busR, TFTR_RST, 0, true);

// Pupil state (logical)
int pxL = CX, pyL = CY;
int pxR = CX, pyR = CY;

// Precomputed circle spans
static int16_t xLeft[H], xRight[H];
static int eyeTopY, eyeBotY;

// Lids
static Lids lidsL, lidsR;

// Time & RTC
static bool rtc_ok = false;
static bool manual_override = false;
static unsigned long soft_epoch_s = 0;
static unsigned long soft_start_ms = 0;

// Sleep
static bool sleeping = false;
static bool seamDrawn = false;

// Random daytime blink
static unsigned long nextBlinkAtMs = 0;

/* ================== Forward declarations ================== */
static bool rtcRead(Now &n);
static bool rtcWrite(uint8_t hh, uint8_t mm, uint8_t ss);
static void softNow(Now &n);
static void preSleepTick(const Now &n);
static void drawPupilLeft (int cx,int cy,int pr,bool hi,const Lids &ld);
static void drawPupilRight(int cx,int cy,int pr,bool hi,const Lids &ld);
static void blinkOnce();
static void scheduleNextBlink();
static bool  tryBlinkTick();
static void  holdUntilBlinks(uint8_t n,unsigned long frameDelayMs=8);
static void rightEyeArkanoidBounceAndSplitTransfer();
static inline bool isQuietHours(const Now &n);
static void runActivityByIndex(int idx);      // spustenie gagu cez sériovú linku (G=cislo)
static bool manualMode = false;                // STOP = náhodné gagy stoja, oči len žmurkajú (na skúšanie G=cislo)
static void activity_gag_angry_eyes();         // zlé šikmé oči so žmurkaním
static void runNewGagsDemo(int from, int to);  // ukážka gagov za sebou

#define START_DEMO_NEW_GAGS  1   // 1 = po zapnutí (cez deň) sa za sebou spustia gagy DEMO_FROM..DEMO_TO (na kontrolu)
#define DEMO_FROM 52             // najnovší gag: panáčik a kanál
#define DEMO_TO   52

// Zlé šikmé oči
#define START_WITH_ANGRY_GAG 0   // 1 = po zapnutí (cez deň) sa ako prvý spustí gag so zlými očami (na testovanie)
#define ANGRY_FLIP           0   // 1 = ak oči vyzerajú smutne (šikmina je naopak)


/* ================== RTC helpers ================== */
static inline uint8_t bcd2i(uint8_t b){ return (uint8_t)((b>>4)*10 + (b & 0x0F)); }
static inline uint8_t i2bcd(uint8_t v){ return (uint8_t)(((v/10)<<4) | (v%10)); }


// Plynulá zmena jasu oboch očí
static void setBrightnessSmooth(uint8_t targetBrightness, int durationMs = 1000){
  uint8_t startLeft = leftEyeBrightness;
  uint8_t startRight = rightEyeBrightness;
  
  int steps = durationMs / 20;  // krok každých 20ms
  if(steps < 1) steps = 1;
  
  for(int i = 0; i <= steps; i++){
    float t = (float)i / (float)steps;
    
    leftEyeBrightness = startLeft + (int)((targetBrightness - startLeft) * t);
    rightEyeBrightness = startRight + (int)((targetBrightness - startRight) * t);
    
    analogWrite(leftBLKPin(), leftEyeBrightness);
    analogWrite(rightBLKPin(), rightEyeBrightness);
    
    delay(20);
  }
  
  leftEyeBrightness = rightEyeBrightness = targetBrightness;
  analogWrite(leftBLKPin(), leftEyeBrightness);
  analogWrite(rightBLKPin(), rightEyeBrightness);
}

// Okamžité nastavenie jasu
static void setBrightnessImmediate(uint8_t brightness){
  leftEyeBrightness = rightEyeBrightness = brightness;
  analogWrite(leftBLKPin(), leftEyeBrightness);
  analogWrite(rightBLKPin(), rightEyeBrightness);
}

static void tryReadTimeFromSerial(unsigned long WAIT_MS=0){
  static String line=""; unsigned long t0=millis();
  auto haveData=[&](){ return Serial.available()>0; };
  while (WAIT_MS ? ((millis()-t0)<WAIT_MS) : haveData()){
    while (Serial.available()){
      char c=(char)Serial.read();
      if (c=='\r'||c=='\n'){
        if (line.length()){
          String s=line; line=""; s.trim();
          
          // Čas
          if (s.startsWith("T=")){
            int hh=0,mm=0,ss=0; int n=sscanf(s.c_str()+2,"%d:%d:%d",&hh,&mm,&ss);
            if (n==2||n==3){
              if(n==2) ss=0;
              soft_epoch_s=(unsigned long)hh*3600UL+(unsigned long)mm*60UL+(unsigned long)ss;
              soft_start_ms=millis(); manual_override=true;
              if (rtc_ok) rtcWrite((uint8_t)hh,(uint8_t)mm,(uint8_t)ss);
              Serial.println("Time set");
              
              // === NOVÉ: Skontroluj a nastav stav spánku + jas ===
              Now newNow;
              newNow.h = hh;
              newNow.m = mm;
              newNow.s = ss;
              
              bool shouldSleep = isQuietHours(newNow);
              
              if(shouldSleep && !sleeping){
                // Mal by spať, ale nespí - zaspať
                Serial.println("Entering sleep mode...");
                setSleeping(true);
              } 
              else if(!shouldSleep && sleeping){
                // Nemal by spať, ale spí - prebudiť
                Serial.println("Waking up...");
                setSleeping(false);
              }
              else if(!shouldSleep && !sleeping){
                // Deň a bdí - nastav denný jas
                Serial.println("Day mode - full brightness");
                setBrightnessSmooth(255, 1000);
              }
              else if(shouldSleep && sleeping){
                // Noc a spí - nastav nočný jas
                Serial.println("Night mode - low brightness");
                setBrightnessSmooth(150, 1000);
              }
            }
          }
          // === GAGY CEZ SÉRIOVÚ LINKU ===
          else if (s == "ANGRY" || s == "angry"){
            runActivityByIndex(16);
          }
          else if (s == "DEMO" || s == "demo"){
            runNewGagsDemo(DEMO_FROM, DEMO_TO);
          }
          else if (s == "DEMOALL" || s == "demoall"){
            runNewGagsDemo(16, 52);
          }
          else if (s.startsWith("G=") || s.startsWith("g=")){
            runActivityByIndex(atoi(s.c_str()+2));
          }
          else if (s == "STOP" || s == "stop"){
            manualMode = true;  Serial.println("Nahodne gagy zastavene - spustaj G=cislo, START = znova nahodne");
          }
          else if (s == "START" || s == "start"){
            manualMode = false; Serial.println("Nahodne gagy znova bezia");
          }
          else if (s == "PEEK" || s == "peek"){
            if(sleeping){
              Serial.println("Executing sleep peek...");
              sleepPeek();
              Serial.println("Sleep peek done");
            } else {
              Serial.println("Not sleeping - peek skipped");
            }
          }
          // === ĽAVÉ OKO ===
          else if (s == "L+"){
            if(leftEyeBrightness <= 245) leftEyeBrightness += 10;
            else leftEyeBrightness = 255;
            analogWrite(leftBLKPin(), leftEyeBrightness);
            Serial.print("Left eye brightness: ");
            Serial.println(leftEyeBrightness);
          }
          else if (s == "L-"){
            if(leftEyeBrightness >= 10) leftEyeBrightness -= 10;
            else leftEyeBrightness = 0;
            analogWrite(leftBLKPin(), leftEyeBrightness);
            Serial.print("Left eye brightness: ");
            Serial.println(leftEyeBrightness);
          }
          else if (s.startsWith("L=")){
            int val = atoi(s.c_str()+2);
            if(val >= 0 && val <= 255){
              leftEyeBrightness = val;
              analogWrite(leftBLKPin(), leftEyeBrightness);
              Serial.print("Left eye brightness: ");
              Serial.println(leftEyeBrightness);
            } else {
              Serial.println("Invalid brightness (0-255)");
            }
          }
          // === PRAVÉ OKO ===
          else if (s == "R+"){
            if(rightEyeBrightness <= 245) rightEyeBrightness += 10;
            else rightEyeBrightness = 255;
            analogWrite(rightBLKPin(), rightEyeBrightness);
            Serial.print("Right eye brightness: ");
            Serial.println(rightEyeBrightness);
          }
          else if (s == "R-"){
            if(rightEyeBrightness >= 10) rightEyeBrightness -= 10;
            else rightEyeBrightness = 0;
            analogWrite(rightBLKPin(), rightEyeBrightness);
            Serial.print("Right eye brightness: ");
            Serial.println(rightEyeBrightness);
          }
          else if (s.startsWith("R=")){
            int val = atoi(s.c_str()+2);
            if(val >= 0 && val <= 255){
              rightEyeBrightness = val;
              analogWrite(rightBLKPin(), rightEyeBrightness);
              Serial.print("Right eye brightness: ");
              Serial.println(rightEyeBrightness);
            } else {
              Serial.println("Invalid brightness (0-255)");
            }
          }
          // === OBE OČI NARAZ ===
          else if (s == "B+"){
            if(leftEyeBrightness <= 245) leftEyeBrightness += 10;
            else leftEyeBrightness = 255;
            if(rightEyeBrightness <= 245) rightEyeBrightness += 10;
            else rightEyeBrightness = 255;
            analogWrite(leftBLKPin(), leftEyeBrightness);
            analogWrite(rightBLKPin(), rightEyeBrightness);
            Serial.print("Both eyes brightness: ");
            Serial.println(leftEyeBrightness);
          }
          else if (s == "B-"){
            if(leftEyeBrightness >= 10) leftEyeBrightness -= 10;
            else leftEyeBrightness = 0;
            if(rightEyeBrightness >= 10) rightEyeBrightness -= 10;
            else rightEyeBrightness = 0;
            analogWrite(leftBLKPin(), leftEyeBrightness);
            analogWrite(rightBLKPin(), rightEyeBrightness);
            Serial.print("Both eyes brightness: ");
            Serial.println(leftEyeBrightness);
          }
          else if (s.startsWith("B=")){
            int val = atoi(s.c_str()+2);
            if(val >= 0 && val <= 255){
              leftEyeBrightness = val;
              rightEyeBrightness = val;
              analogWrite(leftBLKPin(), leftEyeBrightness);
              analogWrite(rightBLKPin(), rightEyeBrightness);
              Serial.print("Both eyes brightness: ");
              Serial.println(leftEyeBrightness);
            } else {
              Serial.println("Invalid brightness (0-255)");
            }
          }
        }
      } else line+=c;
    }
    if(!WAIT_MS) break;
  }
}

bool rtcRead(Now &n){
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(DS3231_ADDR, 3) != 3) return false;
  uint8_t sec = Wire.read(), min = Wire.read(), hrs = Wire.read();
  n.s = bcd2i(sec & 0x7F); n.m = bcd2i(min & 0x7F);
  if (hrs & 0x40){ uint8_t hh=bcd2i(hrs & 0x1F); bool pm=hrs & 0x20; n.h = pm? (uint8_t)((hh%12)+12) : (uint8_t)(hh%12); }
  else n.h = bcd2i(hrs & 0x3F);
  return true;
}

bool rtcWrite(uint8_t hh, uint8_t mm, uint8_t ss){
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(0x00);
  Wire.write(i2bcd(ss & 0x7F));
  Wire.write(i2bcd(mm & 0x7F));
  Wire.write(i2bcd(hh & 0x3F)); // 24h
  return (Wire.endTransmission()==0);
}

/* ================== Build time -> epoch ================== */
static int monthFromStr(const char *m){
  char a=tolower(m[0]),b=tolower(m[1]),c=tolower(m[2]);
  if(a=='j'&&b=='a'&&c=='n')return 1; if(a=='f')return 2; if(a=='m'&&b=='a'&&c=='r')return 3;
  if(a=='a'&&b=='p')return 4; if(a=='m'&&b=='a'&&c=='y')return 5; if(a=='j'&&b=='u'&&c=='n')return 6;
  if(a=='j'&&b=='u'&&c=='l')return 7; if(a=='a'&&b=='u')return 8; if(a=='s'&&b=='e')return 9;
  if(a=='o'&&b=='c')return 10; if(a=='n'&&b=='o')return 11; return 12;
}
static unsigned long daysFrom1970(int y,int m,int d){
  static const int tm[]={0,31,59,90,120,151,181,212,243,273,304,334};
  bool leap; unsigned long days=0;
  for(int yy=1970; yy<y; ++yy){ leap=((yy%4==0 && yy%100!=0)||(yy%400==0)); days+= leap?366UL:365UL; }
  days += tm[m-1] + (d-1);
  leap=((y%4==0 && y%100!=0)||(y%400==0)); if(leap && m>2) days+=1; return days;
}
static void seedSoftClockFromBuild(){
  const char* D=__DATE__; const char* T=__TIME__;
  int mon=monthFromStr(D); int day=atoi(D+4); int year=atoi(D+7);
  int hh=atoi(T); int mm=atoi(T+3); int ss=atoi(T+6);
  unsigned long days=daysFrom1970(year,mon,day);
  soft_epoch_s = days*86400UL + (unsigned long)hh*3600UL + (unsigned long)mm*60UL + (unsigned long)ss;
  soft_start_ms = millis();
}
static void softNow(Now &n){
  unsigned long elapsed=(millis()-soft_start_ms)/1000UL, t=soft_epoch_s+elapsed;
  n.h=(uint8_t)((t/3600UL)%24UL); n.m=(uint8_t)((t/60UL)%60UL); n.s=(uint8_t)(t%60UL);
}

/* ================== Serial time ================== */
/*
static void tryReadTimeFromSerial(unsigned long WAIT_MS=0){
  static String line=""; unsigned long t0=millis();
  auto haveData=[&](){ return Serial.available()>0; };
  while (WAIT_MS ? ((millis()-t0)<WAIT_MS) : haveData()){
    while (Serial.available()){
      char c=(char)Serial.read();
      if (c=='\r'||c=='\n'){
        if (line.length()){
          String s=line; line=""; s.trim();
          if (s.startsWith("T=")){
            int hh=0,mm=0,ss=0; int n=sscanf(s.c_str()+2,"%d:%d:%d",&hh,&mm,&ss);
            if (n==2||n==3){
              if(n==2) ss=0;
              soft_epoch_s=(unsigned long)hh*3600UL+(unsigned long)mm*60UL+(unsigned long)ss;
              soft_start_ms=millis(); manual_override=true;
              if (rtc_ok) rtcWrite((uint8_t)hh,(uint8_t)mm,(uint8_t)ss);
            }
          }
        }
      } else line+=c;
    }
    if(!WAIT_MS) break;
  }
}
*/

/* ================== Geometry & drawing ================== */
static inline bool inCircle2(int x,int y,int cx,int cy,int r2){ int dx=x-cx, dy=y-cy; return dx*dx+dy*dy<=r2; }
static void precomputeCircleSpans(){
  eyeTopY=CY-EYE_R; eyeBotY=CY+EYE_R;
  for(int y=0;y<H;++y){ xLeft[y]=0; xRight[y]=-1; }
  for(int y=eyeTopY;y<=eyeBotY;++y){
    int dy=y-CY; int xs=(int)floorf(sqrtf((float)(EYE_R*EYE_R-dy*dy))+0.5f);
    int xl=CX-xs, xr=CX+xs; if(xl<0)xl=0; if(xr>=W)xr=W-1; xLeft[y]=xl; xRight[y]=xr;
  }
}
static void drawSclera(Arduino_GFX *g,const char* label){
  g->fillScreen(COL_BG); g->fillCircle(CX,CY,EYE_R,COL_SCLERA);
  g->setTextColor(COL_PUPIL, COL_SCLERA); g->setCursor(10,220); g->print(label);
}

// low-level pupil renderer (tile 96×96)
static void renderAndBlitPupilTileBuf(Arduino_GFX *g, uint16_t *buf,
                                      int cx,int cy,int pr,bool hilite,const Lids &ld){
  const int x0=cx-TILE_SZ/2, y0=cy-TILE_SZ/2;
  const int eyeR2=EYE_R*EYE_R, pr2=pr*pr;
  const int hr=(pr>=3)?(pr/3):2, hr2=hr*hr, hx=cx+pr/3, hy=cy-pr/3;
  uint16_t *p=buf;
  for(int ty=0;ty<TILE_SZ;++ty){
    int py=y0+ty;
    for(int tx=0;tx<TILE_SZ;++tx){
      int px=x0+tx;
      if(!inCircle2(px,py,CX,CY,eyeR2)) { *p++=COL_BG; continue; }
      if(py<=ld.topY)                  { *p++=COL_LID_TOP; continue; }
      if(py>=ld.botY)                  { *p++=COL_LID_BOT; continue; }
      uint16_t c=COL_SCLERA;
      if(pr>0 && inCircle2(px,py,cx,cy,pr2)) c=COL_PUPIL;
      if(hilite && inCircle2(px,py,hx,hy,hr2)) c=COL_HILITE;
      *p++=c;
    }
  }
  g->draw16bitRGBBitmap(x0,y0,buf,TILE_SZ,TILE_SZ);
}

// BIG: pupil + rotating petals (tile 160×160)
static void renderAndBlitPupilWithPetals_BIG(
    Arduino_GFX *g, uint16_t *buf,
    int cx, int cy, int pr, bool hilite, const Lids &ld,
    int petalsN, int ringR, int petalR, float angleRad, uint16_t petalCol)
{
  const int x0 = cx - PET_TILE/2, y0 = cy - PET_TILE/2;
  const int eyeR2 = EYE_R*EYE_R, pr2 = pr*pr;
  const int hr = (pr>=3)? (pr/3) : 2, hr2 = hr*hr, hx = cx + pr/3, hy = cy - pr/3;

  const float twoPi = 6.28318530718f;
  const int Nmax = 32;
  if (petalsN > Nmax) petalsN = Nmax;

  int pcx[Nmax], pcy[Nmax];
  for(int i=0;i<petalsN;++i){
    float a = angleRad + twoPi * (float)i / (float)petalsN;
    pcx[i] = cx + (int)roundf(ringR * cosf(a));
    pcy[i] = cy + (int)roundf(ringR * sinf(a));
  }
  const int pet2 = petalR*petalR;

  uint16_t *p = buf;
  for(int ty=0; ty<PET_TILE; ++ty){
    int py = y0 + ty;
    for(int tx=0; tx<PET_TILE; ++tx){
      int px = x0 + tx;

      if(!inCircle2(px,py,CX,CY,eyeR2)) { *p++ = COL_BG; continue; }
      if(py <= ld.topY)                { *p++ = COL_LID_TOP; continue; }
      if(py >= ld.botY)                { *p++ = COL_LID_BOT; continue; }

      uint16_t c = COL_SCLERA;

      if (petalsN){
        bool onPetal = false;
        for(int i=0;i<petalsN;++i){
          int dx = px - pcx[i], dy = py - pcy[i];
          if (dx*dx + dy*dy <= pet2){ onPetal = true; break; }
        }
        if (onPetal) c = petalCol;
      }

      if (pr>0 && inCircle2(px,py,cx,cy,pr2)) c = COL_PUPIL;

      if (hilite && c!=COL_PUPIL && inCircle2(px,py,hx,hy,hr2)) c = COL_HILITE;

      *p++ = c;
    }
  }
  g->draw16bitRGBBitmap(x0, y0, buf, PET_TILE, PET_TILE);
}

// BIG clear bez lupeňov – na „upratanie“ po gagu
static inline void clearBigTileEye(Arduino_GFX* g, uint16_t* buf, int cx, int cy, const Lids& ld){
  renderAndBlitPupilWithPetals_BIG(g, buf, cx, cy, /*pr=*/0, /*hilite=*/false, ld,
                                   /*petalsN=*/0, /*ringR=*/0, /*petalR=*/0, 0.0f, 0);
}

/* ================== Eye helpers ================== */
static inline Arduino_GFX* leftGFX(){  return SWAP_PHYSICAL_EYES ? gfxR : gfxL; }
static inline Arduino_GFX* rightGFX(){ return SWAP_PHYSICAL_EYES ? gfxL : gfxR; }

// Logické piny pre jas (berú do úvahy SWAP)
static inline uint8_t leftBLKPin()  { return LEFT_BLK_PIN; }
static inline uint8_t rightBLKPin() { return RIGHT_BLK_PIN; }

static inline Lids& leftLids(){  return SWAP_PHYSICAL_EYES ? lidsR : lidsL; }
static inline Lids& rightLids(){ return SWAP_PHYSICAL_EYES ? lidsL : lidsR; }

static inline int& leftX(){  return SWAP_PHYSICAL_EYES ? pxR : pxL; }
static inline int& leftY(){  return SWAP_PHYSICAL_EYES ? pyR : pyL; }
static inline int& rightX(){ return SWAP_PHYSICAL_EYES ? pxL : pxR; }
static inline int& rightY(){ return SWAP_PHYSICAL_EYES ? pyL : pyR; }

// edges
static inline int leftEdgeX()   { return CX - (EYE_R - PUPIL_R - 2); }
static inline int rightEdgeX()  { return CX + (EYE_R - PUPIL_R - 2); }
static inline int topEdgeY()    { return CY - (EYE_R - PUPIL_R - 2); }
static inline int bottomEdgeY() { return CY + (EYE_R - PUPIL_R - 2); }
static inline int clampY(int y){ if(y<topEdgeY()) return topEdgeY(); if(y>bottomEdgeY()) return bottomEdgeY(); return y; }

// draw pupils (96×96 sprite)
static inline void drawPupilLeft (int cx,int cy,int pr,bool hi,const Lids &ld){ renderAndBlitPupilTileBuf(leftGFX(),  tileBufL, cx,cy,pr,hi,ld); }
static inline void drawPupilRight(int cx,int cy,int pr,bool hi,const Lids &ld){ renderAndBlitPupilTileBuf(rightGFX(), tileBufR, cx,cy,pr,hi,ld); }

/* ================== Blink + draw ================== */
static inline unsigned long randInRangeMs(unsigned long a,unsigned long b){ if(b<=a) return a; return a + (rand()%(int)(b-a+1)); }
static inline float ease(float t){ return t*t*(3.0f-2.0f*t); }

// two-in-left gag state
static bool rPupilInLeft=false;
static int  p2xL=CX, p2yL=CY;

static inline void drawBoth(){
  drawPupilLeft(leftX(),leftY(),PUPIL_R,true,leftLids());
  if (rPupilInLeft) drawPupilLeft(p2xL,p2yL,PUPIL_R,true,leftLids());
  if (!rPupilInLeft) drawPupilRight(rightX(),rightY(),PUPIL_R,true,rightLids());
}

static inline void eraseEyeTile(bool isLeft, int x, int y){
  if (isLeft) drawPupilLeft (x,y,0,false,leftLids());
  else        drawPupilRight(x,y,0,false,rightLids());
}

// lids painting
// lids painting - UPRAVENÁ VERZIA s voliteľnou farbou pozadia
static void applyLidsColored(Arduino_GFX *g, Lids &prev, int newTopY, int newBotY, uint16_t colTop, uint16_t colBot, uint16_t bgColor = COL_SCLERA){
  if(newTopY<eyeTopY-1) newTopY=eyeTopY-1; if(newTopY>H-1) newTopY=H-1;
  if(newBotY>eyeBotY+1) newBotY=eyeBotY+1; if(newBotY<0)   newBotY=0;
  g->startWrite();
  if(newTopY!=prev.topY){
    if(newTopY>prev.topY){ for(int y=prev.topY+1;y<=newTopY;++y) if(y>=0&&y<H&&xRight[y]>=xLeft[y]) g->drawFastHLine(xLeft[y],y,xRight[y]-xLeft[y]+1,colTop); }
    else{ for(int y=newTopY+1;y<=prev.topY;++y) if(y>=0&&y<H&&xRight[y]>=xLeft[y]) g->drawFastHLine(xLeft[y],y,xRight[y]-xLeft[y]+1,bgColor); }
    prev.topY=newTopY;
  }
  if(newBotY!=prev.botY){
    if(newBotY<prev.botY){ for(int y=prev.botY-1;y>=newBotY;--y) if(y>=0&&y<H&&xRight[y]>=xLeft[y]) g->drawFastHLine(xLeft[y],y,xRight[y]-xLeft[y]+1,colBot); }
    else{ for(int y=prev.botY;y<newBotY;++y) if(y>=0&&y<H&&xRight[y]>=xLeft[y]) g->drawFastHLine(xLeft[y],y,xRight[y]-xLeft[y]+1,bgColor); }
    prev.botY=newBotY;
  }
  g->endWrite();
}

static void drawEyelidSeam(Arduino_GFX *g,int seamY,int thickness,uint16_t col){
  if(thickness<1) thickness=1; int half=thickness/2; int y1=seamY-half; int y2=seamY+(thickness-half-1);
  g->startWrite(); for(int y=y1;y<=y2;++y) if(y>=0&&y<H&&xRight[y]>=xLeft[y]) g->drawFastHLine(xLeft[y],y,xRight[y]-xLeft[y]+1,col); g->endWrite();
}
static inline float smooth(float t){ return t*t*(3.0f-2.0f*t); }

// Sleep logic
static inline bool isQuietHours(const Now &n){
  return (n.h>=22) || (n.h<5) || (n.h==5 && n.m<30);
}
static inline int seamYpos(){ return eyeTopY + (int)round((2.0f/3.0f)*(eyeBotY-eyeTopY)); }

void setSleeping(bool on){
  if(on==sleeping) return; 
  const int steps=12; 
  const int seamY=seamYpos();
  
  if(on){
    // ZASPÁVANIE - postupne znižuj jas na 50
    for(int i=0;i<=steps;++i){ 
      float t=smooth((float)i/steps);
      int nt=(int)round((eyeTopY-1)+(seamY-(eyeTopY-1))*t);
      int nb=(int)round((eyeBotY+1)-((eyeBotY+1)-seamY)*t);
      
      // OPRAVENÉ - použi leftGFX() a rightGFX()
      applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
      applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT); 
      
      // Plynulé znižovanie jasu počas zatvárania
      uint8_t brightness = 255 - (uint8_t)((255 - 50) * t);
      setBrightnessImmediate(brightness);
      
      delay(2);
    }
    
    drawEyelidSeam(leftGFX(), seamY, 3, COL_LID_SEAM); 
    drawEyelidSeam(rightGFX(), seamY, 3, COL_LID_SEAM);
    seamDrawn = true; 
    sleeping = true;
    
    // Po zatvorení znížiť na nočný jas 30
    delay(500);
    setBrightnessSmooth(50, 2000);  // 2 sekundy prechod na 30
    
  } else {
    // PREBÚDZANIE - najprv zvýš jas na 50, potom otváraj
    setBrightnessSmooth(50, 1000);  // 1 sekunda na jas 50
    
    for(int i=steps; i>=0; --i){ 
      float t=smooth((float)i/steps);
      int nt=(int)round((eyeTopY-1)+(seamY-(eyeTopY-1))*t);
      int nb=(int)round((eyeBotY+1)-((eyeBotY+1)-CY)*t);
      
      // OPRAVENÉ - použi leftGFX() a rightGFX()
      applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
      applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT); 
      
      // Plynulé zvyšovanie jasu počas otvárania
      uint8_t brightness = 50 + (uint8_t)((255 - 50) * (1.0f - t));
      setBrightnessImmediate(brightness);
      
      delay(2);
    }
    
    seamDrawn = false; 
    sleeping = false;
    
    drawPupilLeft(pxL, pyL, PUPIL_R, true, lidsL);
    drawPupilRight(pxR, pyR, PUPIL_R, true, lidsR);
    
    // Po otvorení nastaviť denný jas 255
    setBrightnessSmooth(255, 1500);  // 1.5 sekundy na plný jas
  }
}

// ================== Sleep Peek - občasné pohľady počas spánku ==================
static void sleepPeek(){
  const int seamY = seamYpos();
  const int steps = 8;
  const int stepDelay = 40;
  const int lookDelay = 100;
  const int pauseDelay = 200;
  
  // Náhodne vyber ktoré oko (0=ľavé, 1=pravé)
  int whichEye = rand() % 2;
  
  Arduino_GFX *activeGFX = (whichEye == 0) ? leftGFX() : rightGFX();
  Lids &activeLids = (whichEye == 0) ? leftLids() : rightLids();
  int &activePX = (whichEye == 0) ? pxL : pxR;
  int &activePY = (whichEye == 0) ? pyL : pyR;
  
  Arduino_GFX *otherGFX = (whichEye == 0) ? rightGFX() : leftGFX();
  Lids &otherLids = (whichEye == 0) ? rightLids() : leftLids();
  
  const int openTop = eyeTopY - 1;
  const int peekTop = seamY - 20;
  const int peekBot = seamY + 15;
  
  // 1. OTVOR OKO
  for(int i = 0; i <= steps; ++i){
    float t = smooth((float)i / steps);
    int nt = (int)round(seamY - (seamY - peekTop) * t);
    int nb = (int)round(seamY + (peekBot - seamY) * t);
    
    applyLidsColored(activeGFX, activeLids, nt, nb, COL_LID_TOP, COL_LID_BOT);
    applyLidsColored(otherGFX, otherLids, seamY, seamY, COL_LID_TOP, COL_LID_BOT);
    
    if(whichEye == 0){
      drawPupilLeft(activePX, activePY, PUPIL_R, true, activeLids);
    } else {
      drawPupilRight(activePX, activePY, PUPIL_R, true, activeLids);
    }
    delay(stepDelay);
  }
  delay(pauseDelay);
  
  // 2. POHĽAD DOĽAVA
  int startX = activePX;
  int startY = activePY;
  int targetXLeft = CX - 40;
  int targetY = CY + 10;
  
  for(int i = 0; i <= 10; ++i){
    float t = smooth((float)i / 10.0f);
    activePX = (int)(startX + (targetXLeft - startX) * t);
    activePY = (int)(startY + (targetY - startY) * t);
    
    if(whichEye == 0){
      drawPupilLeft(activePX, activePY, PUPIL_R, true, activeLids);
    } else {
      drawPupilRight(activePX, activePY, PUPIL_R, true, activeLids);
    }
    delay(lookDelay / 10);
  }
  delay(lookDelay);
  
  // 3. POHĽAD DOPRAVA
  int targetXRight = CX + 40;
  
  for(int i = 0; i <= 10; ++i){
    float t = smooth((float)i / 10.0f);
    activePX = (int)(targetXLeft + (targetXRight - targetXLeft) * t);
    
    if(whichEye == 0){
      drawPupilLeft(activePX, activePY, PUPIL_R, true, activeLids);
    } else {
      drawPupilRight(activePX, activePY, PUPIL_R, true, activeLids);
    }
    delay(lookDelay / 10);
  }
  delay(lookDelay);
  
  // 4. NÁVRAT DO STREDU
  for(int i = 0; i <= 10; ++i){
    float t = smooth((float)i / 10.0f);
    activePX = (int)(targetXRight + (CX - targetXRight) * t);
    activePY = (int)(targetY + (CY - targetY) * t);
    
    if(whichEye == 0){
      drawPupilLeft(activePX, activePY, PUPIL_R, true, activeLids);
    } else {
      drawPupilRight(activePX, activePY, PUPIL_R, true, activeLids);
    }
    delay(lookDelay / 10);
  }
  delay(pauseDelay);
  
  // 5. ZATVOR OKO
  for(int i = steps; i >= 0; --i){
    float t = smooth((float)i / steps);
    int nt = (int)round(seamY - (seamY - peekTop) * t);
    int nb = (int)round(seamY + (peekBot - seamY) * t);
    
    applyLidsColored(activeGFX, activeLids, nt, nb, COL_LID_TOP, COL_LID_BOT);
    
    if(whichEye == 0){
      drawPupilLeft(activePX, activePY, PUPIL_R, true, activeLids);
    } else {
      drawPupilRight(activePX, activePY, PUPIL_R, true, activeLids);
    }
    delay(stepDelay);
  }
  
  applyLidsColored(activeGFX, activeLids, seamY, seamY, COL_LID_TOP, COL_LID_BOT);
  applyLidsColored(otherGFX, otherLids, seamY, seamY, COL_LID_TOP, COL_LID_BOT);
  
  drawEyelidSeam(activeGFX, seamY, 3, COL_LID_SEAM);
  drawEyelidSeam(otherGFX, seamY, 3, COL_LID_SEAM);
  
  activePX = CX;
  activePY = CY;
}

// ================== Pre-sleep & optional clock ==================
static inline bool isInPreSleepWindow(const Now &n){ 
  return (n.h==21 && n.m>=45);
}

// PRE-SLEEP (21:45–22:00) – pomalé zaspávanie: viečka + spomalenie pohybov + STMIEVANIE na 50
void preSleepTick(const Now &n){
  static bool  active=false;
  static unsigned long start_ms=0;

  // --- presleep dimming state ---
  static bool dimInit = false;
  static uint8_t dimStart = 255;

  // fázy a rýchlosti (deg/sec), riadené cieľovou periódou
  static float thetaL=210.0f, thetaR=210.0f;
  static int   dirL=+1,       dirR=+1;
  static float dpsL=180.0f,   dpsR=180.0f;
  static const float BASE_DPS=180.0f;
  static const float MIN_DPS = 20.0f;
  static const float MAX_DPS = BASE_DPS*1.8f;
  static unsigned long lastStepMs=0;

  // pomalšie, občasné žmurky
  static unsigned long presBlinkStart=0, presNextBlink=0;
  const  unsigned long PRES_BLINK_DUR=1200;
  if(presNextBlink==0) presNextBlink = millis() + 5000 + (rand()%4000);
  bool presBlinking=false;

  // periodický „desync“ oboch očí
  static unsigned long lastDesyncMs=0;
  static unsigned long DESYNC_PERIOD_MS = 0;
  if (DESYNC_PERIOD_MS == 0) DESYNC_PERIOD_MS = 30000UL + (rand()%30001UL); // 30..60 s

  auto randomSlowdownFactor = [](){
    // buď spomaľ 2–15 %, alebo zrýchli 1–5 %
    if (rand() & 1) { int p = 20 + (rand()%130); return 1.0f - (p/1000.0f); }
    else            { int p = 10 + (rand()%40);  return 1.0f + (p/1000.0f); }
  };

  // mikrodreft (percento za sekundu)
  static float driftL = 0.0f, driftR = 0.0f;

  const int seamY=seamYpos();
  const int openTop=eyeTopY-1, openBot=eyeBotY+1;

  // Aktivácia pri vstupe do okna
  if(!active){
    if(!isInPreSleepWindow(n)) return;
    active=true; start_ms=millis();

    // náhodný fázový posun a jitter rýchlosti
    float phaseJitterL = ((rand()%2401) - 1200) / 10.0f;   // -120..+120 deg
    float phaseJitterR = ((rand()%2401) - 1200) / 10.0f;
    thetaL = 210.0f + phaseJitterL;
    thetaR = 210.0f + phaseJitterR;

    float jitterL = 1.0f + (((rand()%11) - 5) / 100.0f);   // +/-5 %
    float jitterR = 1.0f + (((rand()%11) - 5) / 100.0f);
    dpsL = BASE_DPS * jitterL;
    dpsR = BASE_DPS * jitterR;

    // mikrodreft +/-0.1 %/s
    driftL = (((rand()%11) - 5) * 0.0002f);
    driftR = (((rand()%11) - 5) * 0.0002f);

    dirL=dirR=+1;
    lastStepMs=millis();
    leftX() = rightX() = CX;
    leftY() = rightY() = CY+(EYE_R-PUPIL_R-6);
    presBlinkStart=millis(); presNextBlink=millis()+5000+(rand()%4000);
    lastDesyncMs = millis();

    // init dimming
    dimInit = true;
    dimStart = (leftEyeBrightness > rightEyeBrightness) ? leftEyeBrightness : rightEyeBrightness;
    if (dimStart < 50)  dimStart = 50;
    if (dimStart > 255) dimStart = 255;
  }
  // Bezpečný reset, ak niekto vyskočí z okna späť (napr. manuálne nastavenie času)
  if(active && !isInPreSleepWindow(n)){
    active=false;
    dimInit=false;
    return;
  }

  // Po 22:00: prechod do spánku (dovrie viečka + prechod na 30 v setSleeping)
  if(n.h>=22){
    dimInit=false;
    setSleeping(true);
    active=false;
    return;
  }

  // pomalé spúšťanie horného viečka k ~2/3 počas 15 min (ease-out)
  unsigned long elapsed_ms = millis()-start_ms;
  float tlin = (float)elapsed_ms/900000.0f; if(tlin>1.0f) tlin=1.0f;
  float t    = 1.0f - powf(1.0f - tlin, 0.6f);
  int slowTop = (int)roundf(openTop + (seamY-openTop)*t);
  int slowBot = openBot;

  // --- plynulé stmievanie na 50 počas presleepu ---
  if (dimInit){
    float targetF = dimStart - (dimStart - 30.0f) * t; // map dimStart -> 50 v priebehu 15 min
    if (targetF < 30.0f)  targetF = 30.0f;
    if (targetF > 255.0f) targetF = 255.0f;
    uint8_t target = (uint8_t)lroundf(targetF);
    if (target != leftEyeBrightness || target != rightEyeBrightness){
      setBrightnessImmediate(target);
    }
  }

  // plánovanie žmurku (neblokujúce)
  if ((long)(millis()-presNextBlink) >= 0 && presBlinkStart==0) presBlinkStart=millis();
  if (presBlinkStart){
    unsigned long bt=millis()-presBlinkStart;
    if(bt>=PRES_BLINK_DUR){ presBlinkStart=0; presNextBlink=millis()+5000+(rand()%4000); }
    else presBlinking=true;
  }

  float bTop=0.0f, bBot=0.0f;
  if(presBlinking){
    float u2=(float)(millis()-presBlinkStart)/(float)PRES_BLINK_DUR;
    float tri=(u2<0.5f)?(u2*2.0f):(2.0f-u2*2.0f);
    float e=tri*tri*(3.0f-2.0f*tri);
    bTop=e; bBot=e*0.35f;
  }
  int blinkTop=(int)roundf(openTop + (seamY-openTop)*bTop);
  int blinkBot=(int)roundf(openBot - (openBot-seamY)*bBot);
  int finalTop=max(slowTop, blinkTop);
  int finalBot=min(slowBot, blinkBot);
  applyLidsColored(leftGFX(),  leftLids(),  finalTop, finalBot, COL_LID_TOP, COL_LID_BOT);
  applyLidsColored(rightGFX(), rightLids(), finalTop, finalBot, COL_LID_TOP, COL_LID_BOT);

  // cieľová perióda: 900 -> 4800 ms v priebehu 15 min
  const float PERIOD_MIN_MS = 900.0f;
  const float PERIOD_MAX_MS = 4800.0f;
  float u = 1.0f - powf(1.0f - tlin, 0.6f);
  float T_ms = PERIOD_MIN_MS + (PERIOD_MAX_MS - PERIOD_MIN_MS) * u;
  float T_s  = T_ms / 1000.0f;

  // amplitúda klesá 1.0 -> 0.45
  float ampScale = 1.0f - 0.55f * u; if (ampScale < 0.45f) ampScale = 0.45f;
  const float BASE_AMP = 70.0f; // +/-70° okolo 270°
  float amp = BASE_AMP * ampScale;
  float aMin = max(200.0f, 270.0f - amp);
  float aMax = min(340.0f, 270.0f + amp);
  float rangeDeg  = (aMax - aMin);
  float dpsTarget = (2.0f * rangeDeg) / T_s;

  // občasný desync
  if ((long)(millis()-lastDesyncMs) >= 0){
    dpsL *= randomSlowdownFactor();
    dpsR *= randomSlowdownFactor();
    lastDesyncMs = millis() + DESYNC_PERIOD_MS;
  }
  // smer a clamp
  if (thetaL < aMin) { thetaL = aMin; dirL = +1; }
  if (thetaL > aMax) { thetaL = aMax; dirL = -1; }
  if (thetaR < aMin) { thetaR = aMin; dirR = +1; }
  if (thetaR > aMax) { thetaR = aMax; dirR = -1; }

  unsigned long nowMs=millis();
  if(nowMs-lastStepMs > 16){
    float dt=(nowMs-lastStepMs)/1000.0f;
    // plynulý dojazd k dpsTarget
    dpsL += (dpsTarget - dpsL) * 0.04f;
    dpsR += (dpsTarget - dpsR) * 0.04f;

    thetaL += dirL * dpsL * dt;
    thetaR += dirR * dpsR * dt;

    if (thetaL >= aMax){ thetaL=aMax; dirL=-1; }
    if (thetaL <= aMin){ thetaL=aMin; dirL=+1; }
    if (thetaR >= aMax){ thetaR=aMax; dirR=-1; }
    if (thetaR <= aMin){ thetaR=aMin; dirR=+1; }

    // mikrodreft + limity rýchlosti
    dpsL *= (1.0f + driftL * dt);
    dpsR *= (1.0f + driftR * dt);
    if (dpsL < MIN_DPS) dpsL = MIN_DPS;
    if (dpsR < MIN_DPS) dpsR = MIN_DPS;
    if (dpsL > MAX_DPS) dpsL = MAX_DPS;
    if (dpsR > MAX_DPS) dpsR = MAX_DPS;

    auto toXY=[&](float deg,int &x,int &y){
      float rad = deg*(float)M_PI/180.0f; float r=(float)(EYE_R-PUPIL_R-6);
      x=(int)roundf(CX + r*cosf(rad)); y=(int)roundf(CY - r*sinf(rad));
      if(y < CY + (EYE_R/3)) y = CY + (EYE_R/3); // drž sa spodnej polovice oka
    };
    toXY(thetaL, leftX(),  leftY());
    toXY(thetaR, rightX(), rightY());

    lastStepMs=nowMs;
  }

  // prekresli zreničky
  drawPupilLeft (leftX(),  leftY(),  PUPIL_R, true, leftLids());
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
}

// ================== Morning Wake-Up - ranné prebúdzanie ==================
static void morningWakeUp(){
  const int seamY = seamYpos();
  const int openTop = eyeTopY - 1;
  const int openBot = eyeBotY + 1;
  
  Serial.println("Good morning! Waking up...");
  
  // 1. ZVÝŠ JAS MIERNE (ešte v spánku)
  setBrightnessSmooth(50, 1000);
  delay(500);
  
  // 2. PRVÉ ŽMURKNUTIE - skúšobné otvorenie ľavého oka
  const int peekSteps = 6;
  
  // Otvor ľavé oko len trochu
  for(int i = 0; i <= peekSteps; ++i){
    float t = smooth((float)i / peekSteps);
    int nt = (int)round(seamY - (seamY - (seamY - 25)) * t);
    int nb = (int)round(seamY + 20 * t);
    
    applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    applyLidsColored(rightGFX(), rightLids(), seamY, seamY, COL_LID_TOP, COL_LID_BOT);
    
    drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
    delay(40);
  }
  delay(300);
  
  // Zatvor späť
  for(int i = peekSteps; i >= 0; --i){
    float t = smooth((float)i / peekSteps);
    int nt = (int)round(seamY - (seamY - (seamY - 25)) * t);
    int nb = (int)round(seamY + 20 * t);
    
    applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
    delay(40);
  }
  
  applyLidsColored(leftGFX(), leftLids(), seamY, seamY, COL_LID_TOP, COL_LID_BOT);
  delay(800);
  
  // 3. DRUHÉ ŽMURKNUTIE - obidve oči mierne
  for(int i = 0; i <= peekSteps; ++i){
    float t = smooth((float)i / peekSteps);
    int nt = (int)round(seamY - (seamY - (seamY - 30)) * t);
    int nb = (int)round(seamY + 25 * t);
    
    applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    
    drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
    drawPupilRight(pxR, pyR, PUPIL_R, true, rightLids());
    delay(50);
  }
  delay(400);
  
  // Zatvor späť
  for(int i = peekSteps; i >= 0; --i){
    float t = smooth((float)i / peekSteps);
    int nt = (int)round(seamY - (seamY - (seamY - 30)) * t);
    int nb = (int)round(seamY + 25 * t);
    
    applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    
    drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
    drawPupilRight(pxR, pyR, PUPIL_R, true, rightLids());
    delay(50);
  }
  
  applyLidsColored(leftGFX(), leftLids(), seamY, seamY, COL_LID_TOP, COL_LID_BOT);
  applyLidsColored(rightGFX(), rightLids(), seamY, seamY, COL_LID_TOP, COL_LID_BOT);
  delay(600);
  
  // 4. POMALY OTVOR OBE OČI ÚPLNE
  const int wakeSteps = 15;
  for(int i = 0; i <= wakeSteps; ++i){
    float t = smooth((float)i / wakeSteps);
    int nt = (int)round(seamY - (seamY - openTop) * t);
    int nb = (int)round(seamY + (openBot - seamY) * t);
    
    applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    
    // Postupne zvyšuj jas počas otvárania
    uint8_t brightness = 50 + (uint8_t)((255 - 50) * t);
    setBrightnessImmediate(brightness);
    
    drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
    drawPupilRight(pxR, pyR, PUPIL_R, true, rightLids());
    delay(60);
  }
  
  // 5. PAR RÝCHLYCH ŽMURKNUTÍ - "prebúdzam sa"
  for(int blink = 0; blink < 3; ++blink){
    // Rýchle zatvoriť
    for(int i = 0; i <= 4; ++i){
      float t = (float)i / 4.0f;
      int nt = (int)round(openTop + (CY - openTop) * t);
      int nb = (int)round(openBot - (openBot - CY) * t);
      
      applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
      applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
      drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
      drawPupilRight(pxR, pyR, PUPIL_R, true, rightLids());
      delay(15);
    }
    
    // Rýchle otvoriť
    for(int i = 4; i >= 0; --i){
      float t = (float)i / 4.0f;
      int nt = (int)round(openTop + (CY - openTop) * t);
      int nb = (int)round(openBot - (openBot - CY) * t);
      
      applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
      applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
      drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
      drawPupilRight(pxR, pyR, PUPIL_R, true, rightLids());
      delay(15);
    }
    
    delay(150);
  }
  
  // 6. POHĽAD OKOLO - kontrola okolia
  // Doľava
  for(int i = 0; i <= 10; ++i){
    float t = smooth((float)i / 10.0f);
    pxL = pxR = (int)(CX + (CX - 45 - CX) * t);
    pyL = pyR = CY;
    
    drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
    drawPupilRight(pxR, pyR, PUPIL_R, true, rightLids());
    delay(30);
  }
  delay(200);
  
  // Doprava
  for(int i = 0; i <= 10; ++i){
    float t = smooth((float)i / 10.0f);
    pxL = pxR = (int)((CX - 45) + (CX + 45 - (CX - 45)) * t);
    
    drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
    drawPupilRight(pxR, pyR, PUPIL_R, true, rightLids());
    delay(30);
  }
  delay(200);
  
  // Späť na stred
  for(int i = 0; i <= 10; ++i){
    float t = smooth((float)i / 10.0f);
    pxL = pxR = (int)((CX + 45) + (CX - (CX + 45)) * t);
    
    drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
    drawPupilRight(pxR, pyR, PUPIL_R, true, rightLids());
    delay(30);
  }
  
  // 7. FINÁLNE NASTAVENIE
  seamDrawn = false;
  sleeping = false;
  
  leftLids().topY = rightLids().topY = openTop;
  leftLids().botY = rightLids().botY = openBot;
  
  pxL = pxR = CX;
  pyL = pyR = CY;
  
  drawPupilLeft(pxL, pyL, PUPIL_R, true, leftLids());
  drawPupilRight(pxR, pyR, PUPIL_R, true, rightLids());
  
  // Finálny jas
  setBrightnessSmooth(255, 1000);
  
  Serial.println("Fully awake!");
}


static void drawClockRight(Arduino_GFX *gR,const Now &n){
  gR->setTextSize(3); gR->setTextColor(COL_PUPIL,COL_SCLERA); gR->setCursor(54,184);
  if(n.h<10) gR->print('0'); gR->print((int)n.h); gR->print(':'); if(n.m<10) gR->print('0'); gR->print((int)n.m); gR->setTextSize(1);
}

/* ================== Blinky (global) ================== */
static inline void finalizeLidsOpen(){
  leftLids().topY = rightLids().topY = eyeTopY - 1;
  leftLids().botY = rightLids().botY = eyeBotY + 1;
  drawBoth();
}
static void blinkOnce(){
  const int steps=BLINK_STEPS, stepDelay=BLINK_STEP_DELAY;
  for(int i=0;i<=steps;++i){
    float t=ease((float)i/steps);
    int nt=(int)round((eyeTopY-1)+(CY-(eyeTopY-1))*t);
    int nb=(int)round((eyeBotY+1)-((eyeBotY+1)-CY)*t);
    applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    drawBoth(); delay(stepDelay);
  }
  for(int i=steps;i>=0;--i){
    float t=ease((float)i/steps);
    int nt=(int)round((eyeTopY-1)+(CY-(eyeTopY-1))*t);
    int nb=(int)round((eyeBotY+1)-((eyeBotY+1)-CY)*t);
    applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    drawBoth(); delay(stepDelay);
  }
  finalizeLidsOpen(); delay(1);
}
static void scheduleNextBlink(){ nextBlinkAtMs = millis() + randInRangeMs(10000,20000); }
static bool tryBlinkTick(){ if ((long)(millis()-nextBlinkAtMs) >= 0){ blinkOnce(); scheduleNextBlink(); return true; } return false; }
static void holdUntilBlinks(uint8_t n,unsigned long frameDelayMs){ uint8_t c=0; while(c<n){ if(tryBlinkTick()) c++; drawBoth(); delay(frameDelayMs); } }

/* === Temporary blink suppression for gags === */
static unsigned long blinkSavedAt = 0;
static bool blinkSuppressed = false;
static void suppressBlinkStart(){ if (!blinkSuppressed){ blinkSavedAt = nextBlinkAtMs; nextBlinkAtMs = millis() + 3600000UL; blinkSuppressed = true; } }
static void suppressBlinkEnd()  { if (blinkSuppressed){ nextBlinkAtMs = millis() + randInRangeMs(8000,16000); blinkSuppressed = false; } }

/* ================== Start activity helper ================== */
static void startActivityCenterAndBlink(){
  rPupilInLeft=false;
  leftX()=CX; leftY()=CY; rightX()=CX; rightY()=CY;
  drawBoth(); blinkOnce(); scheduleNextBlink();
}

/* ================== Right-only blink (arkanoid) ================== */
static unsigned long nextBlinkRightAtMs = 0;
static inline void scheduleNextBlinkRight(){ nextBlinkRightAtMs = millis() + randInRangeMs(9000,18000); }
static void blinkRightOnce(){
  const int steps=BLINK_STEPS, stepDelay=BLINK_STEP_DELAY;
  for(int i=0;i<=steps;++i){
    float t=ease((float)i/steps);
    int nt=(int)round((eyeTopY-1)+(CY-(eyeTopY-1))*t);
    int nb=(int)round((eyeBotY+1)-((eyeBotY+1)-CY)*t);
    applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
    delay(stepDelay);
  }
  for(int i=steps;i>=0;--i){
    float t=ease((float)i/steps);
    int nt=(int)round((eyeTopY-1)+(CY-(eyeTopY-1))*t);
    int nb=(int)round((eyeBotY+1)-((eyeBotY+1)-CY)*t);
    applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
    drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
    delay(stepDelay);
  }
  rightLids().topY = eyeTopY - 1;
  rightLids().botY = eyeBotY + 1;
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
}
static inline bool tryBlinkRightTick(){ if ((long)(millis() - nextBlinkRightAtMs) >= 0){ blinkRightOnce(); scheduleNextBlinkRight(); return true; } return false; }

/* ================== Split transfer R -> L ================== */
static void continuousSplitTransferRightToLeft(int startX, int startY){
  int y = clampY(startY);
  const int frameMs = 6;

  const int R_start = startX;
  const int R_end = leftEdgeX() - PUPIL_R - 5;
  const int steps1 = 18;

  int prevXR = R_start;

  for(int step=0; step<=steps1; ++step){
    float t = (float)step / (float)steps1; t = t*t*(3.0f-2.0f*t);
    int xR = R_start + (int)((R_end - R_start) * t);

    const int x0 = xR - TILE_SZ/2, y0 = y - TILE_SZ/2;
    const int eyeR2 = EYE_R*EYE_R, pr2 = PUPIL_R*PUPIL_R;
    const int hr = PUPIL_R/3, hr2 = hr*hr;
    const int hx = xR + PUPIL_R/3, hy = y - PUPIL_R/3;

    uint16_t *p = tileBufR;
    for(int ty=0; ty<TILE_SZ; ++ty){
      int py = y0 + ty;
      for(int tx=0; tx<TILE_SZ; ++tx){
        int px = x0 + tx;

        if(!inCircle2(px,py,CX,CY,eyeR2)) { *p++=COL_BG; continue; }
        if(py <= rightLids().topY) { *p++=COL_LID_TOP; continue; }
        if(py >= rightLids().botY) { *p++=COL_LID_BOT; continue; }

        uint16_t c = COL_SCLERA;
        if(xR >= leftEdgeX() - PUPIL_R && inCircle2(px,py,xR,y,pr2)) c = COL_PUPIL;
        if(xR >= leftEdgeX() - PUPIL_R && inCircle2(px,py,hx,hy,hr2)) c = COL_HILITE;
        *p++ = c;
      }
    }
    rightGFX()->draw16bitRGBBitmap(x0, y0, tileBufR, TILE_SZ, TILE_SZ);
    prevXR = xR;
    delay(frameMs);
  }

  drawPupilRight(prevXR, y, 0, false, rightLids());
  delay(20);

  const int L_start = rightEdgeX() + PUPIL_R + 5;
  const int L_end = rightEdgeX();
  const int steps2 = 18;

  int prevXL = L_start;

  for(int step=0; step<=steps2; ++step){
    float t = (float)step / (float)steps2; t = t*t*(3.0f-2.0f*t);
    int xL = L_start + (int)((L_end - L_start) * t);

    const int x0 = xL - TILE_SZ/2, y0 = y - TILE_SZ/2;
    const int eyeR2 = EYE_R*EYE_R, pr2 = PUPIL_R*PUPIL_R;
    const int hr = PUPIL_R/3, hr2 = hr*hr;
    const int hx = xL + PUPIL_R/3, hy = y - PUPIL_R/3;

    uint16_t *p = tileBufL;
    for(int ty=0; ty<TILE_SZ; ++ty){
      int py = y0 + ty;
      for(int tx=0; tx<TILE_SZ; ++tx){
        int px = x0 + tx;

        if(!inCircle2(px,py,CX,CY,eyeR2)) { *p++=COL_BG; continue; }
        if(py <= leftLids().topY) { *p++=COL_LID_TOP; continue; }
        if(py >= leftLids().botY) { *p++=COL_LID_BOT; continue; }

        uint16_t c = COL_SCLERA;
        if(xL <= rightEdgeX() + PUPIL_R && inCircle2(px,py,xL,y,pr2)) c = COL_PUPIL;
        if(xL <= rightEdgeX() + PUPIL_R && inCircle2(px,py,hx,hy,hr2)) c = COL_HILITE;
        *p++ = c;
      }
    }
    leftGFX()->draw16bitRGBBitmap(x0, y0, tileBufL, TILE_SZ, TILE_SZ);
    prevXL = xL;
    delay(frameMs);
  }

  rPupilInLeft = true;
  p2xL = L_end;
  p2yL = y;

  drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
  drawPupilLeft(p2xL, p2yL, PUPIL_R, true, leftLids());
}

/* ================== Animators ================== */
static float easeOutCubic(float t){ t=(t-1.0f); return 1.0f + t*t*t; }
static void animateEyeClean(bool isLeft, int &xRef, int &yRef, int toX, int toY, int durationMs, float (*easeFn)(float)){
  int fromX=xRef, fromY=yRef; if (durationMs<10) durationMs=10;
  const int frameMs=12; unsigned long t0=millis();
  while ((millis()-t0) < (unsigned long)durationMs){
    float t = (float)(millis()-t0)/(float)durationMs; if(t>1.0f)t=1.0f;
    float e = (easeFn?easeFn(t):t);
    int nx = fromX + (int)roundf((toX-fromX)*e);
    int ny = fromY + (int)roundf((toY-fromY)*e);
    eraseEyeTile(isLeft, xRef, yRef);
    xRef=nx; yRef=ny;
    if (isLeft) {
      drawPupilLeft(xRef,yRef,PUPIL_R,true,leftLids());
      if (rPupilInLeft) drawPupilLeft(p2xL,p2yL,PUPIL_R,true,leftLids());
    } else {
      drawPupilRight(xRef,yRef,PUPIL_R,true,rightLids());
    }
    (void)tryBlinkTick();
    delay(frameMs);
  }
}
static inline void animateLeftEye (int toX,int toY,int ms=140){ animateEyeClean(true,  leftX(),  leftY(),  toX,toY, ms, easeOutCubic); }
static inline void animateRightEye(int toX,int toY,int ms=140){ animateEyeClean(false, rightX(), rightY(), toX,toY, ms, easeOutCubic); }

/* ================== Activities ================== */
static void activity_look_forward(){ startActivityCenterAndBlink(); holdUntilBlinks(1,2); }

static void activity_look_left_up(){ 
  startActivityCenterAndBlink(); 
  
  // Obe oči sa pozrú vľavo hore
  animateLeftEye(CX-32, CY-14, 160); 
  animateRightEye(CX-32, CY-14, 160);
  
  holdUntilBlinks(1,2); 
  
  // Vrátiť do stredu
  animateLeftEye(CX, CY, 160); 
  animateRightEye(CX, CY, 160);
  
  holdUntilBlinks(1,2); 
}

// Pohľad vpravo hore - OBE OČI
static void activity_look_right_up(){ 
  startActivityCenterAndBlink(); 
  
  // Obe oči sa pozrú vpravo hore
  animateLeftEye(CX+32, CY-14, 160); 
  animateRightEye(CX+32, CY-14, 160);

  holdUntilBlinks(1,2); 
  
  // Vrátiť do stredu
  animateLeftEye(CX, CY, 160); 
  animateRightEye(CX, CY, 160);
  
  holdUntilBlinks(2,3); 
}

// FUN gag: two-in-left
static void activity_gag_two_in_left(){
  startActivityCenterAndBlink();
  animateLeftEye(leftEdgeX(), CY, 180); holdUntilBlinks(1,8);
  animateLeftEye(leftEdgeX(), CY - 30, 250); delay(300);
  animateLeftEye(leftEdgeX(), CY + 30, 280); delay(300);
  animateLeftEye(leftEdgeX(), CY, 250);      delay(400);
  blinkOnce(); delay(150);

  rightX()=CX; rightY()=CY; drawPupilRight(rightX(),rightY(),PUPIL_R,true,rightLids());
  rightEyeArkanoidBounceAndSplitTransfer();

  unsigned long oldNextBlink = nextBlinkAtMs;
  for(int i=0; i<3; i++){ nextBlinkAtMs = millis() + randInRangeMs(600, 1500);
    while((long)(millis()-nextBlinkAtMs) < 0){ drawBoth(); delay(8); }
    blinkOnce(); delay(2); blinkOnce();
  }
  nextBlinkAtMs = oldNextBlink;

  drawPupilLeft(p2xL,p2yL,0,false,leftLids());
  rPupilInLeft=false;
  rightX()=CX; rightY()=CY; drawPupilRight(rightX(),rightY(),PUPIL_R,true,rightLids());
  drawBoth();
  animateLeftEye(CX, CY, 140); drawBoth(); holdUntilBlinks(3,8);
}

// NEW gag: rotating petals (BIG tile) + clean + blink
static void activity_gag_flower_spin(){
  rPupilInLeft = false;
  leftX() = rightX() = CX; leftY() = rightY() = CY; drawBoth();

  const int petalsN = 12;
  const int petalR  = 14;
  const int ringR   = PUPIL_R + petalR + 6;
  const float angVel = 1.6f;
  const int frameMs = 12;
  const unsigned long durationMs = 15000;
  const uint16_t petCol = pickRandomPetalColor();

  // Na čas animácie vypni spontánne žmurkanie (nebliká medzi framami)
  suppressBlinkStart();

  float angle = 0.0f;
  unsigned long t0 = millis(), last = t0;

  while ((millis() - t0) < durationMs){
    unsigned long now = millis();
    float dt = (now - last) / 1000.0f; if (dt <= 0) { delay(frameMs); continue; }
    last = now;
    angle += angVel * dt; if (angle > 6.2831853f) angle -= 6.2831853f;

    renderAndBlitPupilWithPetals_BIG(leftGFX(),  petBufL, leftX(),  leftY(),  PUPIL_R, true, leftLids(),  petalsN, ringR, petalR, angle, petCol);
    renderAndBlitPupilWithPetals_BIG(rightGFX(), petBufR, rightX(), rightY(), PUPIL_R, true, rightLids(), petalsN, ringR, petalR, angle, petCol);

    delay(frameMs);
  }

  // *** CLEANUP: premaľuj celý 160×160 blok bez lupeňov ***
  clearBigTileEye(leftGFX(),  petBufL, leftX(),  leftY(),  leftLids());
  clearBigTileEye(rightGFX(), petBufR, rightX(), rightY(), rightLids());

  // vráť kreslenie zreničiek späť 96×96 a sprav mrknutie (pekný prechod)
  drawPupilLeft (leftX(),  leftY(),  PUPIL_R, true, leftLids());
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
  blinkOnce();

  suppressBlinkEnd();
  scheduleNextBlink();
}

/* ================== Arkanoid bounce (right eye) ================== */
static void rightEyeArkanoidBounceAndSplitTransfer(){
  const int L = leftEdgeX(), R = rightEdgeX(), T = topEdgeY(), B = bottomEdgeY();

  int x = rightX(), y = rightY();
  float vx = 170.0f, vy = -180.0f; // px/s
  const int frameMs = 10;
  unsigned long last = millis();
  int leftWallHits = 0;
  scheduleNextBlinkRight();

  int prevX = x, prevY = y;

  while (true){
    (void)tryBlinkRightTick();

    unsigned long now = millis();
    float dt = (now - last) / 1000.0f; 
    if (dt <= 0){ delay(frameMs); continue; }
    last = now;

    float nx = x + vx * dt;
    float ny = y + vy * dt;

    for(int k=0;k<2;++k){
      bool bounced=false;
      if (nx < L){
        float over=L-nx; nx=L+over; vx=-vx; bounced=true; leftWallHits++;
        if (leftWallHits >= 2){
          x=(int)roundf(nx); y=(int)roundf(ny);
          rightX()=x; rightY()=y;
          if(prevX != x || prevY != y){
            renderAndBlitPupilTileBuf(rightGFX(), tileBufR, prevX, prevY, 0, false, rightLids());
          }
          drawPupilRight(x, y, PUPIL_R, true, rightLids());
          continuousSplitTransferRightToLeft(x, y);
          return;
        }
      } else if (nx > R){ float over=nx-R; nx=R-over; vx=-vx; bounced=true; }
      if (ny < T){ float over=T-ny; ny=T+over; vy=-vy; bounced=true; }
      else if (ny > B){ float over=ny-B; ny=B-over; vy=-vy; bounced=true; }
      if (!bounced) break;
    }

    int newX=(int)roundf(nx), newY=(int)roundf(ny);

    if (newX != prevX || newY != prevY) {
      const int x0 = newX - TILE_SZ/2, y0 = newY - TILE_SZ/2;
      const int eyeR2 = EYE_R*EYE_R, pr2 = PUPIL_R*PUPIL_R;
      const int hr = PUPIL_R/3, hr2 = hr*hr;
      const int hx = newX + PUPIL_R/3, hy = newY - PUPIL_R/3;

      uint16_t *p = tileBufR;
      for(int ty=0; ty<TILE_SZ; ++ty){
        int py = y0 + ty;
        for(int tx=0; tx<TILE_SZ; ++tx){
          int px = x0 + tx;

          if(!inCircle2(px,py,CX,CY,eyeR2)) { *p++=COL_BG; continue; }
          if(py <= rightLids().topY) { *p++=COL_LID_TOP; continue; }
          if(py >= rightLids().botY) { *p++=COL_LID_BOT; continue; }

          uint16_t c = COL_SCLERA;
          if(inCircle2(px,py,newX,newY,pr2)) c = COL_PUPIL;
          if(inCircle2(px,py,hx,hy,hr2)) c = COL_HILITE;
          *p++ = c;
        }
      }
      rightGFX()->draw16bitRGBBitmap(x0, y0, tileBufR, TILE_SZ, TILE_SZ);

      prevX = newX;
      prevY = newY;
    }

    x = newX; y = newY; 
    rightX() = x; rightY() = y;
    delay(frameMs);
  }
}

// Pomocná funkcia: vykreslenie mačacej štrbiny (vertikálna elipsa)
static void renderCatPupilTile(uint16_t *buf, int cx, int cy, int width, int height, const Lids &ld){
  const int x0 = cx - TILE_SZ/2, y0 = cy - TILE_SZ/2;
  const int eyeR2 = EYE_R*EYE_R;
  const int w2 = (width * width) / 4;
  const int h2 = (height * height) / 4;
  
  uint16_t *p = buf;
  for(int ty=0; ty<TILE_SZ; ++ty){
    int py = y0 + ty;
    for(int tx=0; tx<TILE_SZ; ++tx){
      int px = x0 + tx;
      
      if(!inCircle2(px, py, CX, CY, eyeR2)) { *p++ = COL_BG; continue; }
      if(py <= ld.topY) { *p++ = COL_SCARY_LID; continue; }
      if(py >= ld.botY) { *p++ = COL_SCARY_LID; continue; }
      
      // Mačacia štrbina (elipsa)
      int dx = px - cx;
      int dy = py - cy;
      
      uint16_t c = COL_SCARY_RED;
      if((dx * dx * h2 + dy * dy * w2) <= (w2 * h2)) {
        c = COL_SCARY_PUPIL;
      }
      
      *p++ = c;
    }
  }
}

// Vykreslenie mačacieho oka
static inline void drawCatEyeLeft(int cx, int cy, int w, int h, const Lids &ld){
  renderCatPupilTile(tileBufL, cx, cy, w, h, ld);
  leftGFX()->draw16bitRGBBitmap(cx - TILE_SZ/2, cy - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
}

static inline void drawCatEyeRight(int cx, int cy, int w, int h, const Lids &ld){
  renderCatPupilTile(tileBufR, cx, cy, w, h, ld);
  rightGFX()->draw16bitRGBBitmap(cx - TILE_SZ/2, cy - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
}

// Horizontálne žmurknutie (z lava/prava) - OPRAVENÉ bez preblikávania
static void horizontalBlink(Arduino_GFX *g, Lids &ld, bool leftToRight, int cx, int cy, int pw, int ph){
  const int steps = 6;
  const int stepDelay = 10;
  
  // Zatváranie
  for(int i=0; i<=steps; ++i){
    float t = ease((float)i / steps);
    int leftEdge = leftToRight ? 
      (int)(xLeft[CY] + (CX - xLeft[CY]) * t) : 
      (int)(CX - (CX - xLeft[CY]) * t);
    int rightEdge = leftToRight ?
      (int)(CX + (xRight[CY] - CX) * (1.0f - t)) :
      (int)(xRight[CY] - (xRight[CY] - CX) * t);
    
    // Najprv vykreslí červené pozadie celého oka
    g->startWrite();
    for(int y = eyeTopY; y <= eyeBotY; ++y){
      if(y >= 0 && y < H && xRight[y] >= xLeft[y]){
        g->drawFastHLine(xLeft[y], y, xRight[y] - xLeft[y] + 1, COL_SCARY_RED);
      }
    }
    g->endWrite();
    
    // Potom nakreslí zrenicu
    renderCatPupilTile(g == leftGFX() ? tileBufL : tileBufR, cx, cy, pw, ph, ld);
    g->draw16bitRGBBitmap(cx - TILE_SZ/2, cy - TILE_SZ/2, 
                          g == leftGFX() ? tileBufL : tileBufR, TILE_SZ, TILE_SZ);
    
    // Nakoniec nakreslí červené viečka
    g->startWrite();
    for(int y = eyeTopY; y <= eyeBotY; ++y){
      if(y >= 0 && y < H && xRight[y] >= xLeft[y]){
        // Ľavá strana
        if(leftEdge > xLeft[y]){
          g->drawFastHLine(xLeft[y], y, leftEdge - xLeft[y], COL_SCARY_LID);
        }
        // Pravá strana
        if(rightEdge < xRight[y]){
          g->drawFastHLine(rightEdge, y, xRight[y] - rightEdge + 1, COL_SCARY_LID);
        }
      }
    }
    g->endWrite();
    
    delay(stepDelay);
  }
  
  delay(60);
  
  // Otváranie - ZJEDNODUŠENÉ
  for(int i=steps; i>=0; --i){
    float t = ease((float)i / steps);
    int leftEdge = leftToRight ? 
      (int)(xLeft[CY] + (CX - xLeft[CY]) * t) : 
      (int)(CX - (CX - xLeft[CY]) * t);
    int rightEdge = leftToRight ?
      (int)(CX + (xRight[CY] - CX) * (1.0f - t)) :
      (int)(xRight[CY] - (xRight[CY] - CX) * t);
    
    // Najprv vykreslí červené pozadie celého oka
    g->startWrite();
    for(int y = eyeTopY; y <= eyeBotY; ++y){
      if(y >= 0 && y < H && xRight[y] >= xLeft[y]){
        g->drawFastHLine(xLeft[y], y, xRight[y] - xLeft[y] + 1, COL_SCARY_RED);
      }
    }
    g->endWrite();
    
    // Potom nakreslí zrenicu
    renderCatPupilTile(g == leftGFX() ? tileBufL : tileBufR, cx, cy, pw, ph, ld);
    g->draw16bitRGBBitmap(cx - TILE_SZ/2, cy - TILE_SZ/2, 
                          g == leftGFX() ? tileBufL : tileBufR, TILE_SZ, TILE_SZ);
    
    // Nakoniec nakreslí červené viečka (ktoré sa zmenšujú)
    g->startWrite();
    for(int y = eyeTopY; y <= eyeBotY; ++y){
      if(y >= 0 && y < H && xRight[y] >= xLeft[y]){
        // Ľavá strana
        if(leftEdge > xLeft[y]){
          g->drawFastHLine(xLeft[y], y, leftEdge - xLeft[y], COL_SCARY_LID);
        }
        // Pravá strana
        if(rightEdge < xRight[y]){
          g->drawFastHLine(rightEdge, y, xRight[y] - rightEdge + 1, COL_SCARY_LID);
        }
      }
    }
    g->endWrite();
    
    delay(stepDelay);
  }
}

// Vertikálne žmurknutie (normálne - zhora/zdola) - OPRAVENÉ
static void scaryVerticalBlink(int catPupilW, int catPupilH){
  const int steps = 5;
  const int stepDelay = 12;
  
  // Zatváranie
  for(int i=0; i<=steps; ++i){
    float t = ease((float)i / steps);
    int nt = (int)round((eyeTopY - 1) + (CY - (eyeTopY - 1)) * t);
    int nb = (int)round((eyeBotY + 1) - ((eyeBotY + 1) - CY) * t);
    
    // Pridané COL_SCARY_RED ako bgColor parameter
    applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_SCARY_LID, COL_SCARY_LID, COL_SCARY_RED);
    applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_SCARY_LID, COL_SCARY_LID, COL_SCARY_RED);
    
    drawCatEyeLeft(leftX(), leftY(), catPupilW, catPupilH, leftLids());
    drawCatEyeRight(rightX(), rightY(), catPupilW, catPupilH, rightLids());
    
    delay(stepDelay);
  }
  
  delay(50);
  
  // Otváranie
  for(int i=steps; i>=0; --i){
    float t = ease((float)i / steps);
    int nt = (int)round((eyeTopY - 1) + (CY - (eyeTopY - 1)) * t);
    int nb = (int)round((eyeBotY + 1) - ((eyeBotY + 1) - CY) * t);
    
    // Pridané COL_SCARY_RED ako bgColor parameter
    applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_SCARY_LID, COL_SCARY_LID, COL_SCARY_RED);
    applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_SCARY_LID, COL_SCARY_LID, COL_SCARY_RED);
    
    drawCatEyeLeft(leftX(), leftY(), catPupilW, catPupilH, leftLids());
    drawCatEyeRight(rightX(), rightY(), catPupilW, catPupilH, rightLids());
    
    delay(stepDelay);
  }
}

// Pomocná funkcia: vykreslenie horizontálne zatvorených viečok do tile buffera
static void renderHorizontalLidsTile(uint16_t *buf, int cx, int cy, int catPupilW, int catPupilH, 
                                     int leftEdge, int rightEdge, const Lids &ld){
  const int x0 = cx - TILE_SZ/2, y0 = cy - TILE_SZ/2;
  const int eyeR2 = EYE_R*EYE_R;
  const int w2 = (catPupilW * catPupilW) / 4;
  const int h2 = (catPupilH * catPupilH) / 4;
  
  uint16_t *p = buf;
  for(int ty=0; ty<TILE_SZ; ++ty){
    int py = y0 + ty;
    for(int tx=0; tx<TILE_SZ; ++tx){
      int px = x0 + tx;
      
      if(!inCircle2(px, py, CX, CY, eyeR2)) { *p++ = COL_BG; continue; }
      if(py <= ld.topY) { *p++ = COL_SCARY_LID; continue; }
      if(py >= ld.botY) { *p++ = COL_SCARY_LID; continue; }
      
      // Horizontálne viečka
      if(px < leftEdge || px > rightEdge) { *p++ = COL_SCARY_LID; continue; }
      
      // Mačacia štrbina
      int dx = px - cx;
      int dy = py - cy;
      
      uint16_t c = COL_SCARY_RED;
      if((dx * dx * h2 + dy * dy * w2) <= (w2 * h2)) {
        c = COL_SCARY_PUPIL;
      }
      
      *p++ = c;
    }
  }
}

// Horizontálne žmurknutie - OPRAVENÉ pravé oko
static void scaryHorizontalBlink(int catPupilW, int catPupilH){
  const int steps = 5;
  const int stepDelay = 12;
  
  int prevLeftEdgeL = xLeft[CY];
  int prevRightEdgeL = xRight[CY];
  int prevLeftEdgeR = xLeft[CY];
  int prevRightEdgeR = xRight[CY];
  
  // Zatváranie
  for(int i=0; i<=steps; ++i){
    float t = ease((float)i / steps);
    
    // ĽAVÉ OKO - zatvára sa OD OKRAJOV DO STREDU
    int leftEdgeL = (int)(xLeft[CY] + (CX - xLeft[CY]) * t);    // z ľava → stred
    int rightEdgeL = (int)(xRight[CY] - (xRight[CY] - CX) * t); // z prava → stred
    
    // PRAVÉ OKO - zatvára sa OD OKRAJOV DO STREDU (zrkadlovo)
    int leftEdgeR = (int)(xLeft[CY] + (CX - xLeft[CY]) * t);    // z ľava → stred (rovnako!)
    int rightEdgeR = (int)(xRight[CY] - (xRight[CY] - CX) * t); // z prava → stred (rovnako!)
    
    // === ĽAVÉ OKO ===
    leftGFX()->startWrite();
    for(int y = eyeTopY; y <= eyeBotY; ++y){
      if(y >= 0 && y < H && xRight[y] >= xLeft[y]){
        if(leftEdgeL > prevLeftEdgeL){
          leftGFX()->drawFastHLine(prevLeftEdgeL, y, leftEdgeL - prevLeftEdgeL, COL_SCARY_LID);
        }
        if(rightEdgeL < prevRightEdgeL){
          leftGFX()->drawFastHLine(rightEdgeL, y, prevRightEdgeL - rightEdgeL + 1, COL_SCARY_LID);
        }
      }
    }
    leftGFX()->endWrite();
    
    // === PRAVÉ OKO ===
    rightGFX()->startWrite();
    for(int y = eyeTopY; y <= eyeBotY; ++y){
      if(y >= 0 && y < H && xRight[y] >= xLeft[y]){
        if(leftEdgeR > prevLeftEdgeR){
          rightGFX()->drawFastHLine(prevLeftEdgeR, y, leftEdgeR - prevLeftEdgeR, COL_SCARY_LID);
        }
        if(rightEdgeR < prevRightEdgeR){
          rightGFX()->drawFastHLine(rightEdgeR, y, prevRightEdgeR - rightEdgeR + 1, COL_SCARY_LID);
        }
      }
    }
    rightGFX()->endWrite();
    
    prevLeftEdgeL = leftEdgeL;
    prevRightEdgeL = rightEdgeL;
    prevLeftEdgeR = leftEdgeR;
    prevRightEdgeR = rightEdgeR;
    
    delay(stepDelay);
  }
  
  delay(50);
  
  // Otváranie
  for(int i=steps; i>=0; --i){
    float t = ease((float)i / steps);
    
    int leftEdgeL = (int)(xLeft[CY] + (CX - xLeft[CY]) * t);
    int rightEdgeL = (int)(xRight[CY] - (xRight[CY] - CX) * t);
    
    int leftEdgeR = (int)(xLeft[CY] + (CX - xLeft[CY]) * t);
    int rightEdgeR = (int)(xRight[CY] - (xRight[CY] - CX) * t);
    
    // === ĽAVÉ OKO ===
    leftGFX()->startWrite();
    for(int y = eyeTopY; y <= eyeBotY; ++y){
      if(y >= 0 && y < H && xRight[y] >= xLeft[y]){
        if(leftEdgeL < prevLeftEdgeL){
          leftGFX()->drawFastHLine(leftEdgeL, y, prevLeftEdgeL - leftEdgeL, COL_SCARY_RED);
        }
        if(rightEdgeL > prevRightEdgeL){
          leftGFX()->drawFastHLine(prevRightEdgeL, y, rightEdgeL - prevRightEdgeL + 1, COL_SCARY_RED);
        }
      }
    }
    leftGFX()->endWrite();
    
    // === PRAVÉ OKO ===
    rightGFX()->startWrite();
    for(int y = eyeTopY; y <= eyeBotY; ++y){
      if(y >= 0 && y < H && xRight[y] >= xLeft[y]){
        if(leftEdgeR < prevLeftEdgeR){
          rightGFX()->drawFastHLine(leftEdgeR, y, prevLeftEdgeR - leftEdgeR, COL_SCARY_RED);
        }
        if(rightEdgeR > prevRightEdgeR){
          rightGFX()->drawFastHLine(prevRightEdgeR, y, rightEdgeR - prevRightEdgeR + 1, COL_SCARY_RED);
        }
      }
    }
    rightGFX()->endWrite();
    
    prevLeftEdgeL = leftEdgeL;
    prevRightEdgeL = rightEdgeL;
    prevLeftEdgeR = leftEdgeR;
    prevRightEdgeR = rightEdgeR;
    
    delay(stepDelay);
  }
  
  // Na koniec prekreslíme zrenice
  drawCatEyeLeft(leftX(), leftY(), catPupilW, catPupilH, leftLids());
  drawCatEyeRight(rightX(), rightY(), catPupilW, catPupilH, rightLids());
}

// === HLAVNÁ AKTIVITA: Strašidelný mód ===
static void activity_gag_scary_cat_eyes(){
  rPupilInLeft = false;
  
  // Vycentruj oči
  leftX() = rightX() = CX;
  leftY() = rightY() = CY;
  
  // Normálne žmurknutie pred prechodom
  blinkOnce();
  delay(200);
  
  // Nakreslí červené pozadie očí
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCARY_RED);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCARY_RED);
  
  // Mačacia zrenica - začína užšia
  const int catPupilW = 8;   // šírka štrbiny
  int catPupilH = 60;        // výška štrbiny
  
  leftLids().topY = rightLids().topY = eyeTopY - 1;
  leftLids().botY = rightLids().botY = eyeBotY + 1;
  
  drawCatEyeLeft(leftX(), leftY(), catPupilW, catPupilH, leftLids());
  drawCatEyeRight(rightX(), rightY(), catPupilW, catPupilH, rightLids());
  
  delay(300);
  
  // Sekvencia žmurknutí
  for(int cycle = 0; cycle < 3; cycle++){
    // Vertikálne žmurknutie (normálne)
    scaryVerticalBlink(catPupilW, catPupilH);
    delay(400);
    
    // Horizontálne žmurknutie - ľavé oko z ľava, pravé z prava
    scaryHorizontalBlink(catPupilW, catPupilH);
    delay(500);
    
    // Zmeň veľkosť zrenice (puls efekt)
    catPupilH = (cycle % 2 == 0) ? 80 : 50;
  }
  
  // Finálne vertikálne žmurknutie
  scaryVerticalBlink(catPupilW, catPupilH);
  delay(400);
  
  // Návrat do normálu - prepnúť späť na biele oči
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  leftLids().topY = rightLids().topY = eyeTopY - 1;
  leftLids().botY = rightLids().botY = eyeBotY + 1;
  
  drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
  
  blinkOnce();
  delay(200);
}

// === NOVÁ AKTIVITA: Arkanoid v oboch očiach s náhodnými odrazmi ===
static void activity_gag_arkanoid_chaos(){
  startActivityCenterAndBlink();
  
  const int L = leftEdgeX(), R = rightEdgeX(), T = topEdgeY(), B = bottomEdgeY();
  
  // Počiatočné pozície
  int xL = leftX(), yL = leftY();
  int xR = rightX(), yR = rightY();
  
  // Počiatočné rýchlosti (px/s) - rôzne pre každé oko
  float vxL = 150.0f, vyL = -160.0f;
  float vxR = -140.0f, vyR = 170.0f;
  
  const int frameMs = 10;
  const unsigned long durationMs = 15000; // 8 sekúnd
  unsigned long startTime = millis();
  unsigned long lastL = millis(), lastR = millis();
  
  int prevXL = xL, prevYL = yL;
  int prevXR = xR, prevYR = yR;
  
  suppressBlinkStart();
  
  while ((millis() - startTime) < durationMs){
    unsigned long now = millis();
    
    // === ĽAVÉ OKO ===
    float dtL = (now - lastL) / 1000.0f;
    if (dtL > 0){
      lastL = now;
      
      float nxL = xL + vxL * dtL;
      float nyL = yL + vyL * dtL;
      
      // Kontrola odrazov s náhodným uhlom
      for(int k=0; k<2; ++k){
        bool bounced = false;
        
        if (nxL < L){
          float over = L - nxL; nxL = L + over; 
          vxL = -vxL;
          // Pridaj náhodný uhol (±30%)
          float angle = (rand() % 61 - 30) / 100.0f; // -0.3 až +0.3
          vyL += vxL * angle;
          bounced = true;
        } else if (nxL > R){
          float over = nxL - R; nxL = R - over;
          vxL = -vxL;
          float angle = (rand() % 61 - 30) / 100.0f;
          vyL += vxL * angle;
          bounced = true;
        }
        
        if (nyL < T){
          float over = T - nyL; nyL = T + over;
          vyL = -vyL;
          float angle = (rand() % 61 - 30) / 100.0f;
          vxL += vyL * angle;
          bounced = true;
        } else if (nyL > B){
          float over = nyL - B; nyL = B - over;
          vyL = -vyL;
          float angle = (rand() % 61 - 30) / 100.0f;
          vxL += vyL * angle;
          bounced = true;
        }
        
        if (!bounced) break;
      }
      
      int newXL = (int)roundf(nxL), newYL = (int)roundf(nyL);
      
      if (newXL != prevXL || newYL != prevYL){
        // Vykreslí do tile buffera (ako v arkanoid kóde)
        const int x0 = newXL - TILE_SZ/2, y0 = newYL - TILE_SZ/2;
        const int eyeR2 = EYE_R*EYE_R, pr2 = PUPIL_R*PUPIL_R;
        const int hr = PUPIL_R/3, hr2 = hr*hr;
        const int hx = newXL + PUPIL_R/3, hy = newYL - PUPIL_R/3;
        
        uint16_t *p = tileBufL;
        for(int ty=0; ty<TILE_SZ; ++ty){
          int py = y0 + ty;
          for(int tx=0; tx<TILE_SZ; ++tx){
            int px = x0 + tx;
            
            if(!inCircle2(px,py,CX,CY,eyeR2)) { *p++=COL_BG; continue; }
            if(py <= leftLids().topY) { *p++=COL_LID_TOP; continue; }
            if(py >= leftLids().botY) { *p++=COL_LID_BOT; continue; }
            
            uint16_t c = COL_SCLERA;
            if(inCircle2(px,py,newXL,newYL,pr2)) c = COL_PUPIL;
            if(inCircle2(px,py,hx,hy,hr2)) c = COL_HILITE;
            *p++ = c;
          }
        }
        leftGFX()->draw16bitRGBBitmap(x0, y0, tileBufL, TILE_SZ, TILE_SZ);
        
        prevXL = newXL;
        prevYL = newYL;
      }
      
      xL = newXL; yL = newYL;
    }
    
    // === PRAVÉ OKO ===
    float dtR = (now - lastR) / 1000.0f;
    if (dtR > 0){
      lastR = now;
      
      float nxR = xR + vxR * dtR;
      float nyR = yR + vyR * dtR;
      
      for(int k=0; k<2; ++k){
        bool bounced = false;
        
        if (nxR < L){
          float over = L - nxR; nxR = L + over;
          vxR = -vxR;
          float angle = (rand() % 61 - 30) / 100.0f;
          vyR += vxR * angle;
          bounced = true;
        } else if (nxR > R){
          float over = nxR - R; nxR = R - over;
          vxR = -vxR;
          float angle = (rand() % 61 - 30) / 100.0f;
          vyR += vxR * angle;
          bounced = true;
        }
        
        if (nyR < T){
          float over = T - nyR; nyR = T + over;
          vyR = -vyR;
          float angle = (rand() % 61 - 30) / 100.0f;
          vxR += vyR * angle;
          bounced = true;
        } else if (nyR > B){
          float over = nyR - B; nyR = B - over;
          vyR = -vyR;
          float angle = (rand() % 61 - 30) / 100.0f;
          vxR += vyR * angle;
          bounced = true;
        }
        
        if (!bounced) break;
      }
      
      int newXR = (int)roundf(nxR), newYR = (int)roundf(nyR);
      
      if (newXR != prevXR || newYR != prevYR){
        const int x0 = newXR - TILE_SZ/2, y0 = newYR - TILE_SZ/2;
        const int eyeR2 = EYE_R*EYE_R, pr2 = PUPIL_R*PUPIL_R;
        const int hr = PUPIL_R/3, hr2 = hr*hr;
        const int hx = newXR + PUPIL_R/3, hy = newYR - PUPIL_R/3;
        
        uint16_t *p = tileBufR;
        for(int ty=0; ty<TILE_SZ; ++ty){
          int py = y0 + ty;
          for(int tx=0; tx<TILE_SZ; ++tx){
            int px = x0 + tx;
            
            if(!inCircle2(px,py,CX,CY,eyeR2)) { *p++=COL_BG; continue; }
            if(py <= rightLids().topY) { *p++=COL_LID_TOP; continue; }
            if(py >= rightLids().botY) { *p++=COL_LID_BOT; continue; }
            
            uint16_t c = COL_SCLERA;
            if(inCircle2(px,py,newXR,newYR,pr2)) c = COL_PUPIL;
            if(inCircle2(px,py,hx,hy,hr2)) c = COL_HILITE;
            *p++ = c;
          }
        }
        rightGFX()->draw16bitRGBBitmap(x0, y0, tileBufR, TILE_SZ, TILE_SZ);
        
        prevXR = newXR;
        prevYR = newYR;
      }
      
      xR = newXR; yR = newYR;
    }
    
    delay(frameMs);
  }
  
  suppressBlinkEnd();
  
  // Vráť zreničky do stredu
  leftX() = xL; leftY() = yL;
  rightX() = xR; rightY() = yR;
  
  animateLeftEye(CX, CY, 200);
  animateRightEye(CX, CY, 200);
  
  blinkOnce();
  scheduleNextBlink();
}

// === NOVÁ AKTIVITA: Striedavo pumpujúce zreničky s náhodnými žmurknutiami ===
static void activity_gag_pumping_pupils(){
  startActivityCenterAndBlink();
  
  const int minRadius = 10;   // minimálna veľkosť zrenice
  const int maxRadius = 50;   // maximálna veľkosť zrenice
  const int cycles = 3;       // počet cyklov
  const int steps = 25;       // počet krokov na jeden cyklus
  const int frameDelay = 20;  // ms medzi framami
  
  // Naplánuj náhodné žmurknutia
  unsigned long nextBlinkAt = millis() + randInRangeMs(800, 1500);
  
  for(int cycle = 0; cycle < cycles; cycle++){
    // Fáza 1: Ľavé rastie, pravé klesá
    for(int i = 0; i <= steps; i++){
      float t = (float)i / (float)steps;
      t = t*t*(3.0f - 2.0f*t); // smooth easing
      
      // Ľavé oko - od min po max
      int radiusL = minRadius + (int)((maxRadius - minRadius) * t);
      
      // Pravé oko - od max po min
      int radiusR = maxRadius - (int)((maxRadius - minRadius) * t);
      
      // Skontroluj či je čas na žmurknutie
      if((long)(millis() - nextBlinkAt) >= 0){
        // Rýchle žmurknutie počas pumpujúcej animácie
        const int blinkSteps = 3;
        const int blinkDelay = 6;
        
        // Zatváranie
        for(int b=0; b<=blinkSteps; ++b){
          float bt = ease((float)b/blinkSteps);
          int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
          int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
          
          applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
          applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
          
          drawPupilLeft(leftX(), leftY(), radiusL, true, leftLids());
          drawPupilRight(rightX(), rightY(), radiusR, true, rightLids());
          
          delay(blinkDelay);
        }
        
        // Otváranie
        for(int b=blinkSteps; b>=0; --b){
          float bt = ease((float)b/blinkSteps);
          int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
          int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
          
          applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
          applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
          
          drawPupilLeft(leftX(), leftY(), radiusL, true, leftLids());
          drawPupilRight(rightX(), rightY(), radiusR, true, rightLids());
          
          delay(blinkDelay);
        }
        
        // Finalizuj otvorené viečka
        leftLids().topY = rightLids().topY = eyeTopY - 1;
        leftLids().botY = rightLids().botY = eyeBotY + 1;
        
        // Naplánuj ďalšie žmurknutie
        nextBlinkAt = millis() + randInRangeMs(800, 1500);
      }
      
      // Vykreslí obe zreničky
      drawPupilLeft(leftX(), leftY(), radiusL, true, leftLids());
      drawPupilRight(rightX(), rightY(), radiusR, true, rightLids());
      
      delay(frameDelay);
    }
    
    // Fáza 2: Ľavé klesá, pravé rastie
    for(int i = 0; i <= steps; i++){
      float t = (float)i / (float)steps;
      t = t*t*(3.0f - 2.0f*t);
      
      // Ľavé oko - od max po min
      int radiusL = maxRadius - (int)((maxRadius - minRadius) * t);
      
      // Pravé oko - od min po max
      int radiusR = minRadius + (int)((maxRadius - minRadius) * t);
      
      // Skontroluj žmurknutie
      if((long)(millis() - nextBlinkAt) >= 0){
        const int blinkSteps = 3;
        const int blinkDelay = 6;
        
        for(int b=0; b<=blinkSteps; ++b){
          float bt = ease((float)b/blinkSteps);
          int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
          int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
          
          applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
          applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
          
          drawPupilLeft(leftX(), leftY(), radiusL, true, leftLids());
          drawPupilRight(rightX(), rightY(), radiusR, true, rightLids());
          
          delay(blinkDelay);
        }
        
        for(int b=blinkSteps; b>=0; --b){
          float bt = ease((float)b/blinkSteps);
          int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
          int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
          
          applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
          applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
          
          drawPupilLeft(leftX(), leftY(), radiusL, true, leftLids());
          drawPupilRight(rightX(), rightY(), radiusR, true, rightLids());
          
          delay(blinkDelay);
        }
        
        leftLids().topY = rightLids().topY = eyeTopY - 1;
        leftLids().botY = rightLids().botY = eyeBotY + 1;
        
        nextBlinkAt = millis() + randInRangeMs(1500, 4100);
      }
      
      drawPupilLeft(leftX(), leftY(), radiusL, true, leftLids());
      drawPupilRight(rightX(), rightY(), radiusR, true, rightLids());
      
      delay(frameDelay);
    }
  }
  
  // Vráť normálnu veľkosť
  for(int i = 0; i <= steps/2; i++){
    float t = (float)i / (float)(steps/2);
    t = t*t*(3.0f - 2.0f*t);
    
    int radiusL = maxRadius - (int)((maxRadius - PUPIL_R) * t);
    int radiusR = minRadius + (int)((PUPIL_R - minRadius) * t);
    
    drawPupilLeft(leftX(), leftY(), radiusL, true, leftLids());
    drawPupilRight(rightX(), rightY(), radiusR, true, rightLids());
    
    delay(frameDelay);
  }
  
  // Finálne prekreslenie s normálnymi zreničkami
  drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
  
  blinkOnce();
  scheduleNextBlink();
}

// === NOVÁ AKTIVITA: Hypnotické špirálové oči s žmurknutiami ===
static void activity_gag_hypno_spiral(){
  startActivityCenterAndBlink();
  
  const int frameMs = 30;  // 30ms = ~33 FPS (bezpečné)
  const unsigned long durationMs = 15000;  // 6 sekúnd
  const float rotSpeed = 2.0f;  // radiánov za sekundu
  
  float angle = 0.0f;
  unsigned long t0 = millis(), last = t0;
  
  // Naplánuj náhodné žmurknutia
  unsigned long nextBlinkAt = millis() + randInRangeMs(800, 1500);
  
  while ((millis() - t0) < durationMs){
    unsigned long now = millis();
    float dt = (now - last) / 1000.0f;
    if (dt <= 0) { delay(frameMs); continue; }
    last = now;
    
    angle += rotSpeed * dt;
    if (angle > 6.2831853f) angle -= 6.2831853f;
    
    // Skontroluj či je čas na žmurknutie
    if((long)(millis() - nextBlinkAt) >= 0){
      // Rýchle žmurknutie počas hypnózy
      const int blinkSteps = 3;
      const int blinkDelay = 6;
      
      // Zatváranie
      for(int b=0; b<=blinkSteps; ++b){
        float bt = ease((float)b/blinkSteps);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        
        // Vykreslí špirály aj počas žmurknutia
        renderHypnoSpiral(tileBufL, leftX(), leftY(), angle, leftLids());
        renderHypnoSpiral(tileBufR, rightX(), rightY(), angle, rightLids());
        
        leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
        rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
        
        delay(blinkDelay);
      }
      
      // Otváranie
      for(int b=blinkSteps; b>=0; --b){
        float bt = ease((float)b/blinkSteps);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        
        renderHypnoSpiral(tileBufL, leftX(), leftY(), angle, leftLids());
        renderHypnoSpiral(tileBufR, rightX(), rightY(), angle, rightLids());
        
        leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
        rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
        
        delay(blinkDelay);
      }
      
      // Finalizuj otvorené viečka
      leftLids().topY = rightLids().topY = eyeTopY - 1;
      leftLids().botY = rightLids().botY = eyeBotY + 1;
      
      // Naplánuj ďalšie žmurknutie
      nextBlinkAt = millis() + randInRangeMs(1200, 4100);
    }
    
    // Vykreslí špirálové zreničky do tile bufferov
    renderHypnoSpiral(tileBufL, leftX(), leftY(), angle, leftLids());
    renderHypnoSpiral(tileBufR, rightX(), rightY(), angle, rightLids());
    
    // Pošli na displeje
    leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
    rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
    
    delay(frameMs);
  }
  
  // Vráť normálne zreničky
  drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
  
  blinkOnce();
  scheduleNextBlink();
}

// Vykreslenie hypnotickej špirály do tile buffera
static void renderHypnoSpiral(uint16_t *buf, int cx, int cy, float angle, const Lids &ld){
  const int x0 = cx - TILE_SZ/2, y0 = cy - TILE_SZ/2;
  const int eyeR2 = EYE_R*EYE_R;
  const int pupilR2 = PUPIL_R * PUPIL_R;
  
  // Farby špirály
  const uint16_t col1 = 0x0000;  // čierna
  const uint16_t col2 = 0xFFFF;  // biela
  
  // Špirálové parametre - UPRAVENÉ pre redšiu špirállu
  const float spiralTurns = 1.5f;  // menej otočení (bolo 3.0f)
  const int spiralArms    = 2;        // menej ramien (bolo 8)
  
  uint16_t *p = buf;
  for(int ty=0; ty<TILE_SZ; ++ty){
    int py = y0 + ty;
    for(int tx=0; tx<TILE_SZ; ++tx){
      int px = x0 + tx;
      
      if(!inCircle2(px, py, CX, CY, eyeR2)) { *p++ = COL_BG; continue; }
      if(py <= ld.topY) { *p++ = COL_LID_TOP; continue; }
      if(py >= ld.botY) { *p++ = COL_LID_BOT; continue; }
      
      int dx = px - cx;
      int dy = py - cy;
      int dist2 = dx*dx + dy*dy;
      
      if(dist2 > pupilR2) { *p++ = COL_SCLERA; continue; }
      
      float dist = sqrtf((float)dist2);
      float pixelAngle = atan2f((float)dy, (float)dx);
      
      // Špirálový vzorec
      float spiralValue = fmodf(pixelAngle + angle + (dist / PUPIL_R) * spiralTurns * 6.28318f, 6.28318f / spiralArms);
      
      uint16_t c = (spiralValue < 3.14159f / spiralArms) ? col1 : col2;
      
      *p++ = c;
    }
  }
}

// === NOVÁ AKTIVITA: Skákajúce oči hore-dole (tile buffer - bez preblikávania) ===
static void activity_gag_bouncing_eyes(){
  startActivityCenterAndBlink();
  
  const int bounces = 4;
  const int jumpSteps = 8;
  const int fallSteps = 10;
  const int jumpDelay = 12;
  const int fallDelay = 15;
  const int pauseAtTop = 100;
  const int pauseAtBottom = 200;
  
  const int topY = topEdgeY();
  const int bottomY = bottomEdgeY();
  const int startY = CY;
  
  suppressBlinkStart();
  
  // Pred animáciou vykreslíme čisté bielko na celú plochu kde sa budú zreničky pohybovať
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  for(int bounce = 0; bounce < bounces; bounce++){
    // === SKOK HORE (rýchly) ===
    for(int i = 0; i <= jumpSteps; i++){
      float t = (float)i / (float)jumpSteps;
      t = 1.0f - (1.0f - t) * (1.0f - t);
      
      int yL = startY + (int)((topY - startY) * t);
      int yR = startY + (int)((topY - startY) * t);
      
      leftY() = yL;
      rightY() = yR;
      
      // Vykreslí kompletný tile (bielko + zrenica) v jednom kroku
      renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
      renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
      
      leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
      rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
      
      delay(jumpDelay);
    }
    
    delay(pauseAtTop);
    
    // === PÁD DOLE (gravitácia) ===
    for(int i = 0; i <= fallSteps; i++){
      float t = (float)i / (float)fallSteps;
      t = t * t;
      
      int yL = topY + (int)((bottomY - topY) * t);
      int yR = topY + (int)((bottomY - topY) * t);
      
      leftY() = yL;
      rightY() = yR;
      
      renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
      renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
      
      leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
      rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
      
      delay(fallDelay);
    }
    
    // Pred nárazom - vyčisti celé oko
    leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
    rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
    
    // "Náraz" - rýchle žmurknutie
    const int impactBlinks = 2;
    for(int ib = 0; ib < impactBlinks; ib++){
      for(int b=0; b<=2; ++b){
        float bt = ease((float)b/2.0f);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        
        drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
        drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
        
        delay(4);
      }
      
      for(int b=2; b>=0; --b){
        float bt = ease((float)b/2.0f);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        
        drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
        drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
        
        delay(4);
      }
      
      leftLids().topY = rightLids().topY = eyeTopY - 1;
      leftLids().botY = rightLids().botY = eyeBotY + 1;
    }
    
    if(bounce < bounces - 1){
      delay(pauseAtBottom);
      // Po pauze vyčisti pred ďalším skokom
      leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
      rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
    }
  }
  
  suppressBlinkEnd();
  
  // Vráť do stredu
  leftY() = CY;
  rightY() = CY;
  
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
  
  blinkOnce();
  scheduleNextBlink();
}

// Vykreslí KOMPLETNÝ tile (vrátane bielka pod zrenicou aj vedľa nej)
static void renderBouncingEyeTileComplete(uint16_t *buf, int cx, int cy, const Lids &ld){
  const int x0 = cx - TILE_SZ/2, y0 = cy - TILE_SZ/2;
  const int eyeR2 = EYE_R*EYE_R, pr2 = PUPIL_R*PUPIL_R;
  const int hr = PUPIL_R/3, hr2 = hr*hr;
  const int hx = cx + PUPIL_R/3, hy = cy - PUPIL_R/3;
  
  uint16_t *p = buf;
  for(int ty=0; ty<TILE_SZ; ++ty){
    int py = y0 + ty;
    for(int tx=0; tx<TILE_SZ; ++tx){
      int px = x0 + tx;
      
      // Mimo oka = pozadie
      if(!inCircle2(px,py,CX,CY,eyeR2)) { 
        *p++ = COL_BG; 
        continue; 
      }
      
      // Viečka
      if(py <= ld.topY) { *p++ = COL_LID_TOP; continue; }
      if(py >= ld.botY) { *p++ = COL_LID_BOT; continue; }
      
      // V oku - buď zrenica alebo bielko
      uint16_t c = COL_SCLERA;  // default = bielko
      
      if(inCircle2(px,py,cx,cy,pr2)) c = COL_PUPIL;
      if(inCircle2(px,py,hx,hy,hr2)) c = COL_HILITE;
      
      *p++ = c;
    }
  }
}

// === NOVÁ AKTIVITA: Vlnové pohyby očí - NIŽŠIA AMPLITÚDA ===
static void activity_gag_wave_motion(){
  startActivityCenterAndBlink();
  
  const int frameMs = 20;
  const unsigned long durationMs = 15000;
  const float frequency = 1.5f;
  const int amplitude = 15;      // ZNÍŽENÉ z 30 na 15 pixlov
  const float phaseShift = 3.14159f;
  
  float time = 0.0f;
  unsigned long t0 = millis(), last = t0;
  
  unsigned long nextBlinkAt = millis() + randInRangeMs(1500, 2500);  // menej často
  
  // Pred animáciou vykreslíme čisté bielko
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  while ((millis() - t0) < durationMs){
    unsigned long now = millis();
    float dt = (now - last) / 1000.0f;
    if (dt <= 0) { delay(frameMs); continue; }
    last = now;
    
    time += dt;
    
    float angleL = 2.0f * 3.14159f * frequency * time;
    float angleR = angleL + phaseShift;
    
    int yL = CY + (int)(amplitude * sinf(angleL));
    int yR = CY + (int)(amplitude * sinf(angleR));
    
    if(yL < topEdgeY()) yL = topEdgeY();
    if(yL > bottomEdgeY()) yL = bottomEdgeY();
    if(yR < topEdgeY()) yR = topEdgeY();
    if(yR > bottomEdgeY()) yR = bottomEdgeY();
    
    leftY() = yL;
    rightY() = yR;
    
    // Skontroluj žmurknutie
    if((long)(millis() - nextBlinkAt) >= 0){
      leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
      rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
      
      const int blinkSteps = 3;
      const int blinkDelay = 6;
      
      for(int b=0; b<=blinkSteps; ++b){
        float bt = ease((float)b/blinkSteps);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        
        renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
        renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
        
        leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
        rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
        
        delay(blinkDelay);
      }
      
      for(int b=blinkSteps; b>=0; --b){
        float bt = ease((float)b/blinkSteps);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        
        renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
        renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
        
        leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
        rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
        
        delay(blinkDelay);
      }
      
      leftLids().topY = rightLids().topY = eyeTopY - 1;
      leftLids().botY = rightLids().botY = eyeBotY + 1;
      
      leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
      rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
      
      nextBlinkAt = millis() + randInRangeMs(2500, 8000);
    }
    
    // Vykreslí vlnové zreničky
    renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
    renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
    
    leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
    rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
    
    delay(frameMs);
  }
  
  // Vráť do stredu
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  int finalSteps = 15;
  for(int i=0; i<=finalSteps; i++){
    float t = (float)i / (float)finalSteps;
    t = t*t*(3.0f - 2.0f*t);
    
    int yL = leftY() + (int)((CY - leftY()) * t);
    int yR = rightY() + (int)((CY - rightY()) * t);
    
    leftY() = yL;
    rightY() = yR;
    
    renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
    renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
    
    leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
    rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
    
    delay(20);
  }
  
  leftY() = rightY() = CY;
  
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
  
  blinkOnce();
  scheduleNextBlink();
}


// Pomocník: prekresli obe zrenice s vlastnou veľkosťou
static inline void drawPupilsSized(int prL, int prR){
  renderAndBlitPupilTileBuf(leftGFX(),  tileBufL, leftX(),  leftY(),  prL, true, leftLids());
  renderAndBlitPupilTileBuf(rightGFX(), tileBufR, rightX(), rightY(), prR, true, rightLids());
}


/* Mikro-sakády: krátke trhnutia okolo stredu (bez blikania) - OPRAVENÉ */
static void activity_spooked_saccades(){
  suppressBlinkStart();
  
  leftX() = rightX() = CX;
  leftY() = rightY() = CY;
  
  // Pred začiatkom vyčistime oči
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  drawBoth();
  
  const int N = 18;
  const int moveSteps = 5;  // kroky na jeden presun
  const int frameDelay = 12;
  
  for(int i=0; i<N; i++){
    int dx = (rand()%17) - 8;     // -8..+8
    int dy = (rand()%13) - 6;     // -6..+6
    
    int targetLX = CX + dx;
    int targetLY = CY + dy;
    int targetRX = CX - dx;
    int targetRY = CY - dy;
    
    // Plynulý presun pomocou tile buffera
    int startLX = leftX(), startLY = leftY();
    int startRX = rightX(), startRY = rightY();
    
    for(int step=0; step<=moveSteps; step++){
      float t = (float)step / (float)moveSteps;
      t = t*t*(3.0f - 2.0f*t);  // smooth easing
      
      int newLX = startLX + (int)((targetLX - startLX) * t);
      int newLY = startLY + (int)((targetLY - startLY) * t);
      int newRX = startRX + (int)((targetRX - startRX) * t);
      int newRY = startRY + (int)((targetRY - startRY) * t);
      
      leftX() = newLX;
      leftY() = newLY;
      rightX() = newRX;
      rightY() = newRY;
      
      // Vykreslíme pomocou tile buffera
      renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
      renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
      
      leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
      rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
      
      delay(frameDelay);
    }
    
    delay(40);
  }
  
  // Návrat do stredu
  int startLX = leftX(), startLY = leftY();
  int startRX = rightX(), startRY = rightY();
  
  for(int step=0; step<=moveSteps*2; step++){
    float t = (float)step / (float)(moveSteps*2);
    t = t*t*(3.0f - 2.0f*t);
    
    leftX() = startLX + (int)((CX - startLX) * t);
    leftY() = startLY + (int)((CY - startLY) * t);
    rightX() = startRX + (int)((CX - startRX) * t);
    rightY() = startRY + (int)((CY - startRY) * t);
    
    renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
    renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
    
    leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
    rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
    
    delay(frameDelay);
  }
  
  leftX() = rightX() = CX;
  leftY() = rightY() = CY;
  
  // Vyčisti pred finálnym vykreslením
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  drawBoth();
  
  suppressBlinkEnd();
  scheduleNextBlink();
}

/* Ležatá osmička (figure-8) – plynulý, strašidelný orbit - OPRAVENÉ */
static void activity_figure8_orbit(){
  suppressBlinkStart();
  
  leftX() = rightX() = CX;
  leftY() = rightY() = CY;
  
  // Vyčisti pred začiatkom
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  drawBoth();
  
  const int R = 42;              // ZVÄČŠENÝ polomer (bolo 28)
  const float w = 1.6f;          // uhlová rýchlosť [rad/s]
  const int ms = 15;
  const unsigned long dur = 10000;
  
  unsigned long t0 = millis(), last = t0;
  float a = 0.0f;
  
  while((millis() - t0) < dur){
    unsigned long now = millis();
    float dt = (now - last) / 1000.0f;
    if(dt <= 0){ delay(ms); continue; }
    last = now;
    a += w * dt;
    
    // Lemniskáta (ležatá osmička) - Lissajous figura
    int xl = CX + (int)roundf(R * sinf(a));
    int yl = CY + (int)roundf(R * sinf(a) * cosf(a));
    
    int xr = CX - (int)roundf(R * sinf(a));
    int yr = CY - (int)roundf(R * sinf(a) * cosf(a));
    
    // Clamp do bezpečných hraníc
    if(xl < leftEdgeX()) xl = leftEdgeX();
    if(xl > rightEdgeX()) xl = rightEdgeX();
    if(xr < leftEdgeX()) xr = leftEdgeX();
    if(xr > rightEdgeX()) xr = rightEdgeX();
    
    if(yl < topEdgeY()) yl = topEdgeY();
    if(yl > bottomEdgeY()) yl = bottomEdgeY();
    if(yr < topEdgeY()) yr = topEdgeY();
    if(yr > bottomEdgeY()) yr = bottomEdgeY();
    
    leftX() = xl;
    leftY() = yl;
    rightX() = xr;
    rightY() = yr;
    
    // Vykreslíme pomocou tile buffera
    renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
    renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
    
    leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
    rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
    
    delay(ms);
  }
  
  // Vráť do stredu
  const int returnSteps = 12;
  int startLX = leftX(), startLY = leftY();
  int startRX = rightX(), startRY = rightY();
  
  for(int i=0; i<=returnSteps; i++){
    float t = (float)i / (float)returnSteps;
    t = t*t*(3.0f - 2.0f*t);
    
    leftX() = startLX + (int)((CX - startLX) * t);
    leftY() = startLY + (int)((CY - startLY) * t);
    rightX() = startRX + (int)((CX - startRX) * t);
    rightY() = startRY + (int)((CY - startRY) * t);
    
    renderBouncingEyeTileComplete(tileBufL, leftX(), leftY(), leftLids());
    renderBouncingEyeTileComplete(tileBufR, rightX(), rightY(), rightLids());
    
    leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
    rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
    
    delay(15);
  }
  
  leftX() = rightX() = CX;
  leftY() = rightY() = CY;
  
  // Vyčisti pred finálnym vykreslením
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  drawBoth();
  
  suppressBlinkEnd();
  blinkOnce();
  scheduleNextBlink();
}

/* 4) Pulzujúca dilatácia – „dýchanie“ pupil */
static void activity_pupil_breath(){
  suppressBlinkStart();
  const int pr0 = PUPIL_R;
  const int amp = 8;            // rozsah ±
  const float w = 1.2f;         // pomalé dýchanie
  const int ms=14; const unsigned long dur=12000;

  unsigned long t0=millis(), last=t0; float a=0.0f;
  while((millis()-t0)<dur){
    unsigned long now=millis();
    float dt=(now-last)/1000.0f; if(dt<=0){ delay(ms); continue; }
    last=now; a+=w*dt;
    int pr = pr0 + (int)roundf( amp * sinf(a) );
    drawPupilsSized(pr, pr);
    delay(ms);
  }
  // návrat na default
  drawPupilsSized(pr0, pr0);
  suppressBlinkEnd();
}

// Vykreslenie tekvice do tile buffera
static void renderPumpkinEye(uint16_t *buf, int cx, int cy, const Lids &ld){
  const int x0 = cx - TILE_SZ/2, y0 = cy - TILE_SZ/2;
  const int eyeR2 = EYE_R*EYE_R;
  const int pumpkinR = PUPIL_R + 5;  // o niečo väčšia
  const int pumpkinR2 = pumpkinR * pumpkinR;
  
  // Farby tekvice
  const uint16_t PK_PUMPKIN = 0xFD20;  // oranžová (RGB565)
  const uint16_t PK_LINES = 0xC300;    // tmavšia oranžová (línie)
  const uint16_t PK_EYE_L = 0x0000;    // čierna (ľavé oko tekvice)
  const uint16_t PK_EYE_R = 0x0000;    // čierna (pravé oko)
  const uint16_t PK_MOUTH = 0x0000;    // čierna (ústa)
  const uint16_t PK_STEM = 0x2A05;     // hnedá (stonka)
  
  uint16_t *p = buf;
  for(int ty=0; ty<TILE_SZ; ++ty){
    int py = y0 + ty;
    for(int tx=0; tx<TILE_SZ; ++tx){
      int px = x0 + tx;
      
      if(!inCircle2(px, py, CX, CY, eyeR2)) { *p++ = COL_BG; continue; }
      if(py <= ld.topY) { *p++ = COL_LID_TOP; continue; }
      if(py >= ld.botY) { *p++ = COL_LID_BOT; continue; }
      
      int dx = px - cx;
      int dy = py - cy;
      int dist2 = dx*dx + dy*dy;
      
      // Mimo tekvice = bielko
      if(dist2 > pumpkinR2) { *p++ = COL_SCLERA; continue; }
      
      // V tekvici
      uint16_t c = PK_PUMPKIN;
      
      // Stonka na vrchu
      if(dy < -pumpkinR + 8 && abs(dx) < 4) {
        c = PK_STEM;
      }
      // Vertikálne línie (segmenty tekvice)
      else if(abs(dx) < 2 || abs(dx - 12) < 2 || abs(dx + 12) < 2) {
        c = PK_LINES;
      }
      // Ľavé oko (trojuholník)
      else if(dy > -8 && dy < 0 && dx > -18 && dx < -6 && abs(dx + 12) < (8 - dy)) {
        c = PK_EYE_L;
      }
      // Pravé oko (trojuholník)
      else if(dy > -8 && dy < 0 && dx > 6 && dx < 18 && abs(dx - 12) < (8 - dy)) {
        c = PK_EYE_R;
      }
      // Ústa (zúbkovaný úsmev)
      else if(dy > 4 && dy < 12) {
        // Zúbky
        int mouthSegment = (dx + pumpkinR) / 6;  // rozdelenie na segmenty
        if(mouthSegment % 2 == 0 && abs(dx) < pumpkinR - 8) {
          if(dy < 8 + abs(dx)/3) {  // zakrivený tvar
            c = PK_MOUTH;
          }
        }
      }
      
      *p++ = c;
    }
  }
}

// Vykreslenie tekvice z bitmapy
static void renderPumpkinBitmap(uint16_t *buf, int cx, int cy, const Lids &ld){
  const int x0 = cx - TILE_SZ/2, y0 = cy - TILE_SZ/2;
  const int eyeR2 = EYE_R*EYE_R;
  
  uint16_t *p = buf;
  for(int ty=0; ty<TILE_SZ; ++ty){
    int py = y0 + ty;
    for(int tx=0; tx<TILE_SZ; ++tx){
      int px = x0 + tx;
      
      if(!inCircle2(px, py, CX, CY, eyeR2)) { 
        *p++ = COL_BG; 
        continue; 
      }
      if(py <= ld.topY) { *p++ = COL_LID_TOP; continue; }
      if(py >= ld.botY) { *p++ = COL_LID_BOT; continue; }
      
      // Načítaj pixel z bitmapy (z PROGMEM)
      uint16_t pixel = pgm_read_word(&pumpkinBitmap[ty * 96 + tx]);
      
      // Ak je biela alebo svetlá (pozadie), nahraď bielkom oka
      if(pixel > 0xF7DE) pixel = COL_SCLERA;  // svetlé farby = bielko
      
      *p++ = pixel;
    }
  }
}

// === AKTIVITA: Halloween tekvicové oči - OPTIMALIZOVANÉ ===
static void activity_gag_pumpkin_eyes(){
  startActivityCenterAndBlink();
  
  leftX() = rightX() = CX;
  leftY() = rightY() = CY;
  
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  const unsigned long durationMs = 10000;
  unsigned long t0 = millis();
  unsigned long nextBlinkAt = millis() + randInRangeMs(2000, 3500);  // MENEJ často
  
  while((millis() - t0) < durationMs){
    if((long)(millis() - nextBlinkAt) >= 0){
      // Žmurknutie - BEZ fillCircle uprostred
      const int blinkSteps = 3;
      const int blinkDelay = 8;
      
      // Zatváranie
      for(int b=0; b<=blinkSteps; ++b){
        float bt = ease((float)b/blinkSteps);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        
        renderPumpkinBitmap(tileBufL, leftX(), leftY(), leftLids());
        renderPumpkinBitmap(tileBufR, rightX(), rightY(), rightLids());
        
        leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
        rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
        
        delay(blinkDelay);
      }
      
      // Otváranie
      for(int b=blinkSteps; b>=0; --b){
        float bt = ease((float)b/blinkSteps);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        
        renderPumpkinBitmap(tileBufL, leftX(), leftY(), leftLids());
        renderPumpkinBitmap(tileBufR, rightX(), rightY(), rightLids());
        
        leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
        rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
        
        delay(blinkDelay);
      }
      
      leftLids().topY = rightLids().topY = eyeTopY - 1;
      leftLids().botY = rightLids().botY = eyeBotY + 1;
      
      nextBlinkAt = millis() + randInRangeMs(2000, 3500);  // Ďalšie žmurknutie
    }
    
    // Normálne zobrazenie tekvíc
    renderPumpkinBitmap(tileBufL, leftX(), leftY(), leftLids());
    renderPumpkinBitmap(tileBufR, rightX(), rightY(), rightLids());
    
    leftGFX()->draw16bitRGBBitmap(leftX() - TILE_SZ/2, leftY() - TILE_SZ/2, tileBufL, TILE_SZ, TILE_SZ);
    rightGFX()->draw16bitRGBBitmap(rightX() - TILE_SZ/2, rightY() - TILE_SZ/2, tileBufR, TILE_SZ, TILE_SZ);
    
    delay(50);  // Menší delay = plynulejšie
  }
  
  // Na konci - len raz fillCircle
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
  
  blinkOnce();
  scheduleNextBlink();
}

// === AKTIVITA: Halloween text display ===
static void activity_halloween_text(){
  startActivityCenterAndBlink();
  
  leftX() = rightX() = CX;
  leftY() = rightY() = CY;
  
  // Vyčisti oči - biele pozadie (len raz)
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  const unsigned long durationMs = 15000;
  unsigned long t0 = millis();
  unsigned long nextBlinkAt = millis() + randInRangeMs(3000, 5000);
  
  const int textSizeSmall = 2;
  const int textSizeLarge = 3;  // Väčší pre Halloween a 2025
  const uint16_t textColor = COL_PUPIL;
  const uint16_t orangeColor = 0xC300;  // Tmavo oranžová (RGB565)
  const uint16_t bgColor = COL_SCLERA;
  
  // === ĽAVÉ OKO: Halloween text (nakresli len raz) ===
  leftGFX()->setTextSize(textSizeLarge);
  leftGFX()->setTextColor(orangeColor, bgColor);
  
  // "Halloween" - 9 znakov, väčší text, bold efekt
  int16_t x1_line1 = CX - (9 * 6 * textSizeLarge) / 2;
  int16_t y1_line1 = CY - (8 * textSizeLarge) - 8;
  
  // Bold efekt - nakresli 3x s posunom
  leftGFX()->setCursor(x1_line1, y1_line1);
  leftGFX()->print("Halloween");
  leftGFX()->setCursor(x1_line1 + 1, y1_line1);
  leftGFX()->print("Halloween");
  leftGFX()->setCursor(x1_line1, y1_line1 + 1);
  leftGFX()->print("Halloween");
  
  // "ZS Ostredkova 14" - menší text, čierny
  leftGFX()->setTextSize(textSizeSmall);
  leftGFX()->setTextColor(textColor, bgColor);
  int16_t x1_line2 = CX - (17 * 6 * textSizeSmall) / 2;
  int16_t y1_line2 = CY + 8;
  leftGFX()->setCursor(x1_line2, y1_line2);
  leftGFX()->print("ZS Ostredkova 14");
  
  // === PRAVÉ OKO ===
  // "2025" - väčší text, tmavo oranžový, bold
  rightGFX()->setTextSize(textSizeLarge);
  rightGFX()->setTextColor(orangeColor, bgColor);
  
  int16_t x2_line1 = CX - (4 * 6 * textSizeLarge) / 2;
  int16_t y2_line1 = CY - (8 * textSizeLarge) - 8;
  
  // Bold efekt
  rightGFX()->setCursor(x2_line1, y2_line1);
  rightGFX()->print("2026");
  rightGFX()->setCursor(x2_line1 + 1, y2_line1);
  rightGFX()->print("2026");
  rightGFX()->setCursor(x2_line1, y2_line1 + 1);
  rightGFX()->print("2026");
  
  // Pozície pre čas
  int16_t x2_line2 = CX - (8 * 6 * textSizeSmall) / 2;
  int16_t y2_line2 = CY + 8;
  
  char lastTimeStr[9] = "";
  
  while((millis() - t0) < durationMs){
    // Získaj aktuálny čas
    Now n;
    if(manual_override) softNow(n);
    else if(rtc_ok){ if(!rtcRead(n)) softNow(n); }
    else softNow(n);
    
    // === PRAVÉ OKO: Aktualizuj len čas (len ak sa zmenil) ===
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", n.h, n.m, n.s);
    
    if(strcmp(timeStr, lastTimeStr) != 0){
      // Vymaž starý čas
      rightGFX()->fillRect(x2_line2, y2_line2, 8 * 6 * textSizeSmall, 8 * textSizeSmall, bgColor);
      
      // Nakresli nový čas
      rightGFX()->setTextSize(textSizeSmall);
      rightGFX()->setTextColor(textColor, bgColor);
      rightGFX()->setCursor(x2_line2, y2_line2);
      rightGFX()->print(timeStr);
      
      strcpy(lastTimeStr, timeStr);
    }
    
    // Žmurknutie (občas)
    if((long)(millis() - nextBlinkAt) >= 0){
      const int blinkSteps = 3;
      const int blinkDelay = 8;
      
      // Zatváranie
      for(int b=0; b<=blinkSteps; ++b){
        float bt = ease((float)b/blinkSteps);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        delay(blinkDelay);
      }
      
      // Otváranie
      for(int b=blinkSteps; b>=0; --b){
        float bt = ease((float)b/blinkSteps);
        int nt = (int)round((eyeTopY-1) + (CY-(eyeTopY-1)) * bt);
        int nb = (int)round((eyeBotY+1) - ((eyeBotY+1)-CY) * bt);
        
        applyLidsColored(leftGFX(), leftLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        applyLidsColored(rightGFX(), rightLids(), nt, nb, COL_LID_TOP, COL_LID_BOT);
        delay(blinkDelay);
      }
      
      leftLids().topY = rightLids().topY = eyeTopY - 1;
      leftLids().botY = rightLids().botY = eyeBotY + 1;
      
      // Prekresli ĽAVÉ OKO - Halloween (bold)
      leftGFX()->setTextSize(textSizeLarge);
      leftGFX()->setTextColor(orangeColor, bgColor);
      leftGFX()->setCursor(x1_line1, y1_line1);
      leftGFX()->print("Halloween");
      leftGFX()->setCursor(x1_line1 + 1, y1_line1);
      leftGFX()->print("Halloween");
      leftGFX()->setCursor(x1_line1, y1_line1 + 1);
      leftGFX()->print("Halloween");
      
      leftGFX()->setTextSize(textSizeSmall);
      leftGFX()->setTextColor(textColor, bgColor);
      leftGFX()->setCursor(x1_line2, y1_line2);
      leftGFX()->print("ZS Ostredkova 14");
      
      // Prekresli PRAVÉ OKO - 2025 (bold) a čas
      rightGFX()->setTextSize(textSizeLarge);
      rightGFX()->setTextColor(orangeColor, bgColor);
      rightGFX()->setCursor(x2_line1, y2_line1);
      rightGFX()->print("2026");
      rightGFX()->setCursor(x2_line1 + 1, y2_line1);
      rightGFX()->print("2026");
      rightGFX()->setCursor(x2_line1, y2_line1 + 1);
      rightGFX()->print("2026");
      
      rightGFX()->setTextSize(textSizeSmall);
      rightGFX()->setTextColor(textColor, bgColor);
      rightGFX()->setCursor(x2_line2, y2_line2);
      rightGFX()->print(timeStr);
      
      nextBlinkAt = millis() + randInRangeMs(3000, 5000);
    }
    
    delay(200);
  }
  
  // Na konci obnoviť normálne oči
  leftGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  rightGFX()->fillCircle(CX, CY, EYE_R, COL_SCLERA);
  
  drawPupilLeft(leftX(), leftY(), PUPIL_R, true, leftLids());
  drawPupilRight(rightX(), rightY(), PUPIL_R, true, rightLids());
  
  blinkOnce();
  scheduleNextBlink();
}

/* ================== GAG: Zlé šikmé oči so žmurkaním ==================
 * Horné viečko je šikmé (pri nose nižšie), zrenice čierne bez odlesku,
 * oči sa podozrievavo pozerajú do strán, žmurknú obidve aj každé zvlášť.
 * Proti preblikávaniu: nič sa nemaže, každý pixel sa vypočíta priamo
 * do pásu (strip) a pošle naraz; kreslia sa len riadky, ktoré sa menia,
 * a snímka sa vôbec nekreslí, ak sa od predošlej nič nezmenilo.
 */
#define ANGRY_PR          34                 // polomer zrenice v zlom režime
#define ANGRY_X0          (CX - EYE_R)
#define ANGRY_WD          (2*EYE_R + 1)
#define ANGRY_STRIP_ROWS  100                // pás 221×100 sa zmestí do petBuf (160×160)
#if (ANGRY_WD * ANGRY_STRIP_ROWS) > (PET_TILE * PET_TILE)
#error "ANGRY strip sa nezmesti do petBuf"
#endif

// (štruktúry AngryEye a AngryGeo sú hore v časti "Types first")

static int16_t angTopL[ANGRY_WD], angBotL[ANGRY_WD];
static int16_t angTopR[ANGRY_WD], angBotR[ANGRY_WD];

static inline float angLerp(float p, float q, float t){ return p + (q - p) * t; }

// spodná hrana šikmého viečka; u > 0 = smerom k nosu
static inline float angryLidEdge(float u){
  return (CY - 62) + 0.48f * u + 10.0f * (1.0f - (u * u) / ((float)EYE_R * (float)EYE_R));
}

// side: +1 / -1 určuje, na ktorej strane je nos
static AngryGeo angryGeometry(int side, const AngryEye &s, int16_t *top, int16_t *bot){
  int topMax = -1000, botMin = 10000;
  for (int i = 0; i < ANGRY_WD; ++i){
    float u    = side * (float)(ANGRY_X0 + i - CX);
    float t    = angLerp((float)(eyeTopY - 1), angryLidEdge(u), s.a);
    float seam = (CY + 22) + 0.15f * u;                 // kde sa viečka stretnú pri žmurknutí
    int tt = (int)lroundf(angLerp(t, seam, s.b));
    int bb = (int)lroundf(angLerp((float)(eyeBotY + 1), seam, s.b));
    top[i] = (int16_t)tt; bot[i] = (int16_t)bb;
    if (tt > topMax) topMax = tt;
    if (bb < botMin) botMin = bb;
  }
  float apx = CX + side * 24.0f + s.glance;            // zrenica bližšie k nosu
  float apy = angryLidEdge(side * (apx - CX)) + 24.0f + s.pdy;  // tesne pod viečkom
  AngryGeo g;
  g.pcx = (int)lroundf(angLerp((float)CX, apx, s.a));
  g.pcy = (int)lroundf(angLerp((float)CY, apy, s.a));
  g.pr  = (int)lroundf(angLerp((float)PUPIL_R, (float)ANGRY_PR, s.a));
  int sb = max(topMax + 4, g.pcy + g.pr + 1);
  if (botMin <= eyeBotY) sb = eyeBotY;                  // pri žmurknutí ide aj spodné viečko
  if (sb > eyeBotY) sb = eyeBotY;
  g.tx = (int)lroundf(s.tx); g.ty = (int)lroundf(s.ty);
  if (g.ty > 0 && g.ty + 10 > sb) sb = min(eyeBotY, g.ty + 10);   // slza
  g.spanBot = sb;
  return g;
}

// vykreslí riadky y0..y1 jedného oka po pásoch (bez mazania -> bez blikania)
static void angryRenderRows(Arduino_GFX *gx, uint16_t *buf, const int16_t *top, const int16_t *bot,
                            const AngryGeo &geo, int y0, int y1){
  if (y0 < eyeTopY) y0 = eyeTopY;
  if (y1 > eyeBotY) y1 = eyeBotY;
  const int pr2 = geo.pr * geo.pr;
  for (int ys = y0; ys <= y1; ys += ANGRY_STRIP_ROWS){
    int ye = min(y1, ys + ANGRY_STRIP_ROWS - 1);
    uint16_t *p = buf;
    for (int y = ys; y <= ye; ++y){
      const int xl = xLeft[y], xr = xRight[y];
      const int dy = y - geo.pcy, dy2 = dy * dy;
      for (int i = 0; i < ANGRY_WD; ++i){
        const int x = ANGRY_X0 + i;
        if (x < xl || x > xr)                 { *p++ = COL_BG;       continue; }
        const int t = top[i], b = bot[i];
        if (y <= t)                           { *p++ = COL_LID_TOP;  continue; }
        if (y >= b)                           { *p++ = COL_LID_BOT;  continue; }
        if (t >= eyeTopY && y <= t + 3)       { *p++ = COL_LID_SEAM; continue; }   // tmavý okraj viečka
        if (b <= eyeBotY && y >= b - 3)       { *p++ = COL_LID_SEAM; continue; }
        if (geo.ty > 0){                                                    // slza (smutné oči)
          const int tdx = x - geo.tx, tdy = y - geo.ty;
          if (tdx * tdx + tdy * tdy <= 49 || (tdy < 0 && tdy > -18 && abs(tdx) * 18 < (tdy + 18) * 7)){ *p++ = COL_TEAR; continue; }
        }
        const int dx = x - geo.pcx;
        *p++ = (dx * dx + dy2 <= pr2) ? COL_PUPIL : COL_SCLERA;
      }
    }
    gx->draw16bitRGBBitmap(ANGRY_X0, ys, buf, ANGRY_WD, ye - ys + 1);
  }
}

// obálka žmurknutia: 0 -> 1 -> 0 počas dur ms
static inline float angryBlinkEnv(unsigned long t, unsigned long start, unsigned long dur){
  if (t < start || t >= start + dur) return 0.0f;
  float x = (float)(t - start) / (float)dur;
  return ease(x < 0.5f ? x * 2.0f : (1.0f - x) * 2.0f);
}

// pohľad do strán podľa kľúčových bodov (čas od začiatku gagu)
static float angryGlanceAt(unsigned long t){
  static const unsigned long kt[] = {    0, 1300, 1700, 3300, 3800, 5600, 6000, 7800, 8200, 9600, 10000, 20000 };
  static const float         kv[] = {    0,    0,  -24,  -24,   24,   24,    0,    0,  -16,  -16,     0,     0 };
  const int n = sizeof(kt) / sizeof(kt[0]);
  for (int i = 0; i < n - 1; ++i){
    if (t >= kt[i] && t < kt[i + 1]){
      float f = (float)(t - kt[i]) / (float)(kt[i + 1] - kt[i]);
      return angLerp(kv[i], kv[i + 1], ease(f));
    }
  }
  return 0.0f;
}

static void activity_gag_angry_eyes(){
  // štart z normálneho oka v strede (bez fillCircle -> bez bliknutia)
  rPupilInLeft = false;
  leftX() = rightX() = CX; leftY() = rightY() = CY;
  leftLids().topY  = rightLids().topY = eyeTopY - 1;
  leftLids().botY  = rightLids().botY = eyeBotY + 1;
  drawBoth();

  const int sideL = ANGRY_FLIP ? -1 : 1;     // nos je pri ľavom oku vpravo, pri pravom vľavo
  const int sideR = -sideL;

  const unsigned long T_IN = 700, T_HOLD = 10600, T_OUT = 700;
  const unsigned long TOTAL = T_IN + T_HOLD + T_OUT;
  const unsigned long BL = 280;             // dĺžka žmurknutia
  // žmurknutia: čas od začiatku, kto (1 = ľavé, 2 = pravé, 3 = obe)
  static const unsigned long evAt[]  = { 2400, 4600, 6600, 8600, 10400 };
  static const uint8_t       evWho[] = {    3,    1,    2,    3,     1 };
  const int nEv = sizeof(evAt) / sizeof(evAt[0]);

  int prevBotL = CY + PUPIL_R + 2, prevBotR = CY + PUPIL_R + 2;
  long lastKeyL = -1, lastKeyR = -1;

  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0;
    bool last = (t >= TOTAL);
    if (last) t = TOTAL;

    float a = 1.0f;
    if (t < T_IN) a = ease((float)t / T_IN);
    else if (t > T_IN + T_HOLD) a = ease(1.0f - (float)(t - T_IN - T_HOLD) / T_OUT);

    AngryEye L = { a, 0.0f, angryGlanceAt(t) * a };
    AngryEye R = L;
    for (int k = 0; k < nEv; ++k){
      float e = angryBlinkEnv(t, evAt[k], BL);
      if (evWho[k] & 1) L.b = max(L.b, e);
      if (evWho[k] & 2) R.b = max(R.b, e);
    }

    AngryGeo gL = angryGeometry(sideL, L, angTopL, angBotL);
    AngryGeo gR = angryGeometry(sideR, R, angTopR, angBotR);

    // kresli len keď sa niečo zmenilo, a len riadky, ktoré sa menia
    long keyL = ((long)(L.a * 255) << 20) ^ ((long)(L.b * 255) << 12) ^ ((long)gL.pcx << 6) ^ gL.pcy;
    long keyR = ((long)(R.a * 255) << 20) ^ ((long)(R.b * 255) << 12) ^ ((long)gR.pcx << 6) ^ gR.pcy;
    if (keyL != lastKeyL){
      angryRenderRows(leftGFX(),  petBufL, angTopL, angBotL, gL, eyeTopY, max(prevBotL, gL.spanBot));
      prevBotL = gL.spanBot; lastKeyL = keyL;
    }
    if (keyR != lastKeyR){
      angryRenderRows(rightGFX(), petBufR, angTopR, angBotR, gR, eyeTopY, max(prevBotR, gR.spanBot));
      prevBotR = gR.spanBot; lastKeyR = keyR;
    }
    if (last) break;
    delay(4);
  }

  // späť na bežné oko: dlaždica so zrenicou a odleskom prekreslí stred (bez bliknutia)
  drawBoth();
  scheduleNextBlink();
}


/* =====================================================================
 *  NOVÉ GAGY 17–23 (spoločný kresliaci základ bez preblikávania)
 *  Všetky kreslia priamo výsledné pixely do pásu v petBuf a posielajú
 *  ho naraz; nič sa nemaže, prekresľuje sa len oblasť, kde sa niečo hýbe.
 * ===================================================================== */

static inline int noseSideL(){ return ANGRY_FLIP ? -1 : 1; }   // na ktorej strane ľavého oka je nos

// vykreslí obdĺžnik oka po pásoch cez shader (mimo kruhu oka = pozadie)
static void gagRenderRect(Arduino_GFX *gx, uint16_t *buf, int x0, int y0, int x1, int y1,
                          GagShadeFn fn, const void *ctx){
  if (x0 < CX - EYE_R) x0 = CX - EYE_R;
  if (x1 > CX + EYE_R) x1 = CX + EYE_R;
  if (y0 < eyeTopY) y0 = eyeTopY;
  if (y1 > eyeBotY) y1 = eyeBotY;
  if (x1 < x0 || y1 < y0) return;
  const int w = x1 - x0 + 1;
  int rowsPer = (PET_TILE * PET_TILE) / w; if (rowsPer < 1) rowsPer = 1;
  for (int ys = y0; ys <= y1; ys += rowsPer){
    const int ye = min(y1, ys + rowsPer - 1);
    uint16_t *p = buf;
    for (int y = ys; y <= ye; ++y){
      const int xl = xLeft[y], xr = xRight[y];
      for (int x = x0; x <= x1; ++x) *p++ = (x < xl || x > xr) ? COL_BG : fn(x, y, ctx);
    }
    gx->draw16bitRGBBitmap(x0, ys, buf, w, ye - ys + 1);
  }
}

// zrenica s odleskom (rovnaký vzhľad ako bežné oko); vráti true, ak pixel patrí zrenici
static inline bool gagPupilPix(int x, int y, int cx, int cy, int pr, uint16_t &out){
  if (pr <= 0) return false;
  const int dx = x - cx, dy = y - cy;
  if (dx * dx + dy * dy > pr * pr) return false;
  const int hr = (pr >= 3) ? pr / 3 : 2, hx = dx - pr / 3, hy = dy + pr / 3;
  out = (hx * hx + hy * hy <= hr * hr) ? COL_HILITE : COL_PUPIL;
  return true;
}

// štart gagu z bežného oka v strede
// plynulé stlmenie / rozsvietenie podsvietenia (f = 0..1 z aktuálneho jasu očí);
// obraz sa vymení, keď je tma, takže prekresľovanie nie je vidieť
static void gagBacklightFade(float from, float to, int ms){
  const int steps = max(1, ms / 15);
  for (int i = 1; i <= steps; ++i){
    float f = from + (to - from) * (float)i / steps;
    f = f * f;                                                     // oko vníma jas nelineárne
    analogWrite(leftBLKPin(),  (int)(leftEyeBrightness  * f));
    analogWrite(rightBLKPin(), (int)(rightEyeBrightness * f));
    delay(15);
  }
}

/* Prechody cez podsvietenie: namiesto postupného prefarbovania celého oka
 * (vidno, ako sa prekresľuje zhora nadol) oči zhasnú, obraz sa vymení v tme
 * a znova sa rozsvietia. */
#define GAG_BL_MS 350                 // trvanie stmavnutia / rozsvietenia (ms)
static int8_t gagBl = -1;             // -1 vyp., 0 pripravené, 1 stmavené (čaká na 1. snímku), 2 beží
static inline void gagBlArm(){ gagBl = 0; }          // gag s gagEnvelope: prechody pôjdu cez podsvietenie
static inline void gagBlOut(){ gagBacklightFade(1.0f, 0.0f, GAG_BL_MS); }
static inline void gagBlIn(){ gagBacklightFade(0.0f, 1.0f, GAG_BL_MS); gagBl = 2; }   // na konci gagu sa vráti cez podsvietenie
static uint16_t shadeNormalEye(int x, int y, const void *){
  uint16_t c; if (gagPupilPix(x, y, CX, CY, PUPIL_R, c)) return c;
  return COL_SCLERA;
}

static void gagStartCentered(){
  gagBl = -1;
  rPupilInLeft = false;
  leftX() = rightX() = CX; leftY() = rightY() = CY;
  leftLids().topY = rightLids().topY = eyeTopY - 1;
  leftLids().botY = rightLids().botY = eyeBotY + 1;
  drawBoth();
}
static void gagEndNormal(){
  leftX() = rightX() = CX; leftY() = rightY() = CY;
  if (gagBl >= 1){                                       // návrat cez podsvietenie
    gagBlOut();
    gagRenderRect(leftGFX(),  petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeNormalEye, nullptr);
    gagRenderRect(rightGFX(), petBufR, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeNormalEye, nullptr);
    drawBoth();
    gagBacklightFade(0.0f, 1.0f, GAG_BL_MS);
  } else drawBoth();
  gagBl = -1;
  scheduleNextBlink();
}

// plynulý presun zreníc dlaždicou 96×96 (krok obmedzený tak, aby nezostali stopy)
static int gagPrL = PUPIL_R, gagPrR = PUPIL_R;
static void gagMovePupils(int lx, int ly, int rx, int ry, int lpr, int rpr, int maxStep, int frameMs){
  for (;;){
    int stepL = min(maxStep, TILE_SZ / 2 - 2 - max(gagPrL, lpr));
    int stepR = min(maxStep, TILE_SZ / 2 - 2 - max(gagPrR, rpr));
    if (stepL < 1) stepL = 1; if (stepR < 1) stepR = 1;
    leftX()  += constrain(lx - leftX(),  -stepL, stepL);
    leftY()  += constrain(ly - leftY(),  -stepL, stepL);
    rightX() += constrain(rx - rightX(), -stepR, stepR);
    rightY() += constrain(ry - rightY(), -stepR, stepR);
    gagPrL += constrain(lpr - gagPrL, -3, 3);
    gagPrR += constrain(rpr - gagPrR, -3, 3);
    drawPupilLeft (leftX(),  leftY(),  gagPrL, true, leftLids());
    drawPupilRight(rightX(), rightY(), gagPrR, true, rightLids());
    if (leftX() == lx && leftY() == ly && rightX() == rx && rightY() == ry && gagPrL == lpr && gagPrR == rpr) break;
    delay(frameMs);
  }
}

/* ---------- 17: Zaľúbené – pulzujúce srdiečka ---------- */
static uint16_t shadeHeart(int x, int y, const void *c){
  const HeartCtx *h = (const HeartCtx *)c;
  if (h->k > 1.0f){
    const float nx = (x - h->hx) / h->k, ny = (h->hy - y) / h->k + 0.15f;
    const float a = nx * nx + ny * ny - 1.0f;
    if (a * a * a - nx * nx * ny * ny * ny <= 0.0f){
      const float ddx = x - (h->hx - 0.45f * h->k), ddy = y - (h->hy - 0.40f * h->k), hr = 0.16f * h->k;
      return (ddx * ddx + ddy * ddy <= hr * hr) ? COL_HILITE : COL_HEART;
    }
  }
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, h->pr, c2)) return c2;
  return COL_SCLERA;
}

static void activity_gag_heart_eyes(){
  gagStartCentered();
  const unsigned long T_IN = 500, T_HOLD = 8000, T_OUT = 500, TOT = T_IN + T_HOLD + T_OUT;
  HeartCtx L, R;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    float g = 1.0f;
    if (t < T_IN) g = ease((float)t / T_IN);
    else if (t > T_IN + T_HOLD) g = ease(1.0f - (float)(t - T_IN - T_HOLD) / T_OUT);
    const unsigned long p = t % 900;          // tlkot: dva údery
    float beat = 0.0f;
    if (p < 130) beat = sinf(3.14159f * p / 130.0f);
    else if (p >= 220 && p < 350) beat = 0.6f * sinf(3.14159f * (p - 220) / 130.0f);
    const float k = 34.0f * g * (1.0f + 0.15f * beat);
    L.k = R.k = k;
    L.pr = R.pr = (int)lroundf(PUPIL_R * (1.0f - g));
    L.hx = R.hx = CX + 14.0f * sinf(t * 0.0012f) * g;
    L.hy = R.hy = CY + 8.0f * sinf(t * 0.0021f) * g;
    gagRenderRect(leftGFX(),  petBufL, CX - 64, CY - 64, CX + 64, CY + 64, shadeHeart, &L);
    gagRenderRect(rightGFX(), petBufR, CX - 64, CY - 64, CX + 64, CY + 64, shadeHeart, &R);
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 18: Prekvapené – malé zrenice, rýchle pohľady ---------- */
static void activity_gag_surprised(){
  gagStartCentered();
  gagPrL = gagPrR = PUPIL_R;
  gagMovePupils(CX, CY, CX, CY, 12, 12, 6, 10);                 // zrenice sa zúžia
  delay(400);
  static const int8_t tx[] = { -55,  55, -45,  50,   0,  0 };
  static const int8_t ty[] = { -35, -30,  40,  35, -10,  0 };
  for (int i = 0; i < 6; ++i){
    gagMovePupils(CX + tx[i], CY + ty[i], CX + tx[i], CY + ty[i], 12, 12, 18, 6);
    delay(300 + (i % 3) * 120);
  }
  const unsigned long t0 = millis();                              // chvenie od ľaku
  while (millis() - t0 < 900){
    int jx = (int)(rand() % 5) - 2, jy = (int)(rand() % 5) - 2;
    gagMovePupils(CX + jx, CY + jy, CX + jx, CY + jy, 12, 12, 4, 0);
    delay(30);
  }
  gagMovePupils(CX, CY, CX, CY, PUPIL_R, PUPIL_R, 6, 12);       // späť
  gagEndNormal();
}

/* ---------- 19: K.O. – krížiky a hviezdičky ---------- */
static uint16_t shadeKo(int x, int y, const void *c){
  const KoCtx *k = (const KoCtx *)c;
  if (k->starsOn > 0.05f){
    const float outer = 10.0f * k->starsOn, inner = 4.5f * k->starsOn;
    for (int i = 0; i < 3; ++i){
      const float dx = x - k->sx[i], dy = y - k->sy[i];
      const float d2 = dx * dx + dy * dy;
      if (d2 > outer * outer) continue;
      const float seg = 6.28318f / 5.0f;
      float ang = atan2f(dy, dx) + k->starA * 2.0f + 12.566f;
      float f = fmodf(ang, seg) / seg;
      float rr = inner + (outer - inner) * fabsf(1.0f - 2.0f * f);
      if (d2 <= rr * rr) return COL_STAR;
    }
  }
  if (k->sc > 0.02f){
    const float dx = x - CX, dy = y - CY;
    const float u = dx * k->ct + dy * k->st, v = -dx * k->st + dy * k->ct;
    const float hw = 7.0f * k->sc + 0.5f, L = 48.0f * k->sc;
    const float a = fabsf(u - v) * 0.7071f, b = fabsf(u + v) * 0.7071f;
    if ((a < hw && b < L) || (b < hw && a < L)) return COL_PUPIL;
  }
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, k->pr, c2)) return c2;
  return COL_SCLERA;
}

static void activity_gag_ko_eyes(){
  gagStartCentered();
  const unsigned long T_IN = 500, T_HOLD = 6500, T_OUT = 500, TOT = T_IN + T_HOLD + T_OUT;
  KoCtx K;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    float g = 1.0f;
    if (t < T_IN) g = ease((float)t / T_IN);
    else if (t > T_IN + T_HOLD) g = ease(1.0f - (float)(t - T_IN - T_HOLD) / T_OUT);
    K.sc = g; K.starsOn = g;
    K.pr = (int)lroundf(PUPIL_R * (1.0f - g));
    const float th = 0.35f * sinf(t * 0.004f);
    K.ct = cosf(th); K.st = sinf(th);
    K.starA = t * 0.004f;
    for (int i = 0; i < 3; ++i){
      const float ph = K.starA + i * 2.0944f;
      K.sx[i] = CX + 72.0f * cosf(ph);
      K.sy[i] = CY - 60.0f + 20.0f * sinf(ph);
    }
    gagRenderRect(leftGFX(),  petBufL, CX - 92, CY - 92, CX + 92, CY + 52, shadeKo, &K);
    gagRenderRect(rightGFX(), petBufR, CX - 92, CY - 92, CX + 92, CY + 52, shadeKo, &K);
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 20: Škúlenie – zrenice k nosu, každá iná ---------- */
static void activity_gag_cross_eyed(){
  gagStartCentered();
  gagPrL = gagPrR = PUPIL_R;
  const int s = noseSideL();
  const int lx = CX + s * 52, rx = CX - s * 52, yy = CY + 6;
  gagMovePupils(lx, yy, rx, yy, 26, 40, 5, 12);
  const unsigned long t0 = millis();
  while (millis() - t0 < 3000){                                   // „zaostrovanie“
    const float t = (float)(millis() - t0);
    const int w  = (int)lroundf(4.0f * sinf(t * 0.009f));
    const int pl = 26 + (int)lroundf(4.0f * sinf(t * 0.006f));
    const int pr = 40 - (int)lroundf(4.0f * sinf(t * 0.006f));
    gagMovePupils(lx + w * s, yy, rx - w * s, yy, pl, pr, 5, 0);
    delay(16);
  }
  delay(300);
  gagMovePupils(CX, CY, CX, CY, PUPIL_R, PUPIL_R, 5, 12);
  gagEndNormal();
}

/* ---------- 21: Smutné – viečka naopak, slzy ---------- */
static inline float sadTearY(unsigned long t, unsigned long start){
  const unsigned long D = 1700;
  if (t < start || t >= start + D) return 0.0f;
  const float f = (float)(t - start) / D;
  return (CY + 20) + (eyeBotY - 8 - (CY + 20)) * f * f;           // zrýchľuje ako kvapka
}

static void activity_gag_sad_eyes(){
  gagStartCentered();
  const int s = noseSideL();
  const int lidL = -s, lidR = s;                                  // šikmina opačne ako pri zlých očiach
  const unsigned long T_IN = 800, T_HOLD = 8200, T_OUT = 800, TOT = T_IN + T_HOLD + T_OUT, BL = 320;
  static const unsigned long tearL[] = { 1500, 4300 }, tearR[] = { 2400, 5400 };
  int prevBotL = CY + PUPIL_R + 2, prevBotR = CY + PUPIL_R + 2;
  long lastKeyL = -1, lastKeyR = -1;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    float a = 1.0f;
    if (t < T_IN) a = ease((float)t / T_IN);
    else if (t > T_IN + T_HOLD) a = ease(1.0f - (float)(t - T_IN - T_HOLD) / T_OUT);
    AngryEye L = { a, 0.0f, 30.0f * s, 16.0f, 0.0f, 0.0f };
    AngryEye R = { a, 0.0f, -30.0f * s, 16.0f, 0.0f, 0.0f };
    L.b = R.b = angryBlinkEnv(t, 7000, BL);
    for (int i = 0; i < 2; ++i){
      float ty = sadTearY(t, tearL[i]); if (ty > 0){ L.ty = ty; L.tx = CX - s * 58; }
      ty = sadTearY(t, tearR[i]);       if (ty > 0){ R.ty = ty; R.tx = CX + s * 58; }
    }
    AngryGeo gL = angryGeometry(lidL, L, angTopL, angBotL);
    AngryGeo gR = angryGeometry(lidR, R, angTopR, angBotR);
    long keyL = ((long)(L.a * 255) << 20) ^ ((long)(L.b * 255) << 12) ^ ((long)gL.pcy << 6) ^ gL.ty;
    long keyR = ((long)(R.a * 255) << 20) ^ ((long)(R.b * 255) << 12) ^ ((long)gR.pcy << 6) ^ gR.ty;
    if (keyL != lastKeyL){
      angryRenderRows(leftGFX(),  petBufL, angTopL, angBotL, gL, eyeTopY, max(prevBotL, gL.spanBot));
      prevBotL = gL.spanBot; lastKeyL = keyL;
    }
    if (keyR != lastKeyR){
      angryRenderRows(rightGFX(), petBufR, angTopR, angBotR, gR, eyeTopY, max(prevBotR, gR.spanBot));
      prevBotR = gR.spanBot; lastKeyR = keyR;
    }
    if (last) break;
    delay(4);
  }
  gagEndNormal();
}

/* ---------- 22: Krvavé – žilky a červená dúhovka ---------- */
#define BLOOD_VEINS 16
#define BLOOD_SEGS  6
static int16_t bvx[2][BLOOD_VEINS][BLOOD_SEGS + 1], bvy[2][BLOOD_VEINS][BLOOD_SEGS + 1];
static uint8_t bvn[2][BLOOD_VEINS];
static uint32_t bloodRng = 1;
static inline int bloodRand(int n){ bloodRng = bloodRng * 1664525u + 1013904223u; return (int)((bloodRng >> 16) % (uint32_t)n); }

static void bloodGenerate(int e, uint32_t seed){
  bloodRng = seed;
  for (int v = 0; v < BLOOD_VEINS; ++v){
    float ang = v * 6.28318f / BLOOD_VEINS + (bloodRand(40) - 20) / 100.0f;
    float x = CX + (EYE_R - 3) * cosf(ang), y = CY + (EYE_R - 3) * sinf(ang);
    float dir = ang + 3.14159f;
    bvx[e][v][0] = (int16_t)lroundf(x); bvy[e][v][0] = (int16_t)lroundf(y);
    int n = 0;
    for (int s = 1; s <= BLOOD_SEGS; ++s){
      dir += (bloodRand(61) - 30) / 100.0f;
      const float step = 9 + bloodRand(6);
      const float nx = x + step * cosf(dir), ny = y + step * sinf(dir);
      if (fabsf(nx - CX) <= 50 && fabsf(ny - CY) <= 50) break;       // nezasahuj do dlaždice dúhovky
      const float ddx = nx - CX, ddy = ny - CY;
      if (ddx * ddx + ddy * ddy > (EYE_R - 3) * (EYE_R - 3)) break;
      x = nx; y = ny;
      bvx[e][v][s] = (int16_t)lroundf(x); bvy[e][v][s] = (int16_t)lroundf(y);
      n = s;
    }
    bvn[e][v] = (uint8_t)n;
  }
}

// nakreslí (alebo zmaže farbou bielka) jeden úsek žilky
static void bloodSegment(Arduino_GFX *gx, int e, int v, int s, bool erase){
  if (s < 1 || s > bvn[e][v]) return;
  const int x0 = bvx[e][v][s - 1], y0 = bvy[e][v][s - 1], x1 = bvx[e][v][s], y1 = bvy[e][v][s];
  const uint16_t c = erase ? COL_SCLERA : (s <= 2 ? COL_VEIN_DARK : COL_VEIN);
  gx->drawLine(x0, y0, x1, y1, c);
  if (s <= 2) gx->drawLine(x0 + 1, y0, x1 + 1, y1, c);            // pri okraji hrubšie
}

// stred oka: zrenica + červená dúhovka (dlaždica 96×96)
static void renderIrisTile(Arduino_GFX *gx, uint16_t *buf, int pr, int ir){
  const int x0 = CX - TILE_SZ / 2, y0 = CY - TILE_SZ / 2;
  const int pr2 = pr * pr, ir2 = ir * ir, ring2 = (ir - 3) * (ir - 3);
  uint16_t *p = buf;
  for (int ty = 0; ty < TILE_SZ; ++ty){
    const int dy = y0 + ty - CY;
    for (int tx = 0; tx < TILE_SZ; ++tx){
      const int dx = x0 + tx - CX, d2 = dx * dx + dy * dy;
      uint16_t c = COL_SCLERA;
      if (d2 <= pr2) c = COL_PUPIL;
      else if (ir > pr && d2 <= ir2) c = (d2 >= ring2) ? COL_IRIS_RING : COL_IRIS;
      const int hx = dx - 12, hy = dy + 12;
      if (c != COL_SCLERA && hx * hx + hy * hy <= 36) c = COL_HILITE;
      *p++ = c;
    }
  }
  gx->draw16bitRGBBitmap(x0, y0, buf, TILE_SZ, TILE_SZ);
}

static void activity_gag_blood_eyes(){
  gagStartCentered();
  bloodGenerate(0, 12345u + (uint32_t)rand());
  bloodGenerate(1, 777u + (uint32_t)rand());
  const int N = BLOOD_VEINS * BLOOD_SEGS;
  // 1) dúhovka sčervenie
  for (int i = 0; i <= 12; ++i){
    const float g = ease(i / 12.0f);
    const int pr = (int)lroundf(PUPIL_R + (20 - PUPIL_R) * g), ir = (int)lroundf(44 * g);
    renderIrisTile(leftGFX(), tileBufL, pr, ir); renderIrisTile(rightGFX(), tileBufR, pr, ir);
    delay(25);
  }
  // 2) žilky postupne narastú od okraja, dúhovka pulzuje
  unsigned long t0 = millis();
  for (int k = 0; k < N; ++k){
    const int v = k % BLOOD_VEINS, s = k / BLOOD_VEINS + 1;
    bloodSegment(leftGFX(), 0, v, s, false);
    bloodSegment(rightGFX(), 1, v, s, false);
    if (k % 4 == 0){
      const int pr = 20 + (int)lroundf(3.0f * sinf((millis() - t0) * 0.006f));
      renderIrisTile(leftGFX(), tileBufL, pr, 44); renderIrisTile(rightGFX(), tileBufR, pr, 44);
    }
    delay(30);
  }
  // 3) chvíľu ostane
  t0 = millis();
  while (millis() - t0 < 2500){
    const int pr = 20 + (int)lroundf(3.0f * sinf((millis() - t0) * 0.006f));
    renderIrisTile(leftGFX(), tileBufL, pr, 44); renderIrisTile(rightGFX(), tileBufR, pr, 44);
    delay(30);
  }
  // 4) žilky zmiznú (opačné poradie, farbou bielka)
  for (int k = N - 1; k >= 0; --k){
    const int v = k % BLOOD_VEINS, s = k / BLOOD_VEINS + 1;
    bloodSegment(leftGFX(), 0, v, s, true);
    bloodSegment(rightGFX(), 1, v, s, true);
    delay(12);
  }
  // 5) dúhovka späť na bežnú zrenicu
  for (int i = 12; i >= 0; --i){
    const float g = ease(i / 12.0f);
    const int pr = (int)lroundf(PUPIL_R + (20 - PUPIL_R) * g), ir = (int)lroundf(44 * g);
    renderIrisTile(leftGFX(), tileBufL, pr, ir); renderIrisTile(rightGFX(), tileBufR, pr, ir);
    delay(25);
  }
  gagEndNormal();
}

/* ---------- 23: Pavúk – zlieza po vlákne, oči ho sledujú ---------- */
static inline float segDist2(float px, float py, float ax, float ay, float bx, float by){
  const float vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay;
  float t = (vx * wx + vy * wy) / (vx * vx + vy * vy + 1e-6f);
  t = t < 0 ? 0 : (t > 1 ? 1 : t);
  const float dx = wx - t * vx, dy = wy - t * vy;
  return dx * dx + dy * dy;
}

static uint16_t shadeSpider(int x, int y, const void *c){
  const SpiderCtx *s = (const SpiderCtx *)c;
  if (s->on){
    const float dx = x - s->sx, dy = y - s->sy;
    if (x == (int)lroundf(s->sx) && dy < -18) return COL_THREAD;          // vlákno
    if (fabsf(dx) < 42 && dy > -28 && dy < 38){
      bool halo = false;                                                  // biely obrys, aby bol pavúk vidieť aj na zrenici
      const float b2 = dx * dx + dy * dy;
      if (b2 <= 110) return COL_SPIDER;                                   // telo
      if (b2 <= 180) halo = true;
      const float hy = dy + 14, h2 = dx * dx + hy * hy;
      if (h2 <= 40){                                                      // hlava + červené oči
        if ((fabsf(dx - 3) <= 1.2f || fabsf(dx + 3) <= 1.2f) && fabsf(hy + 1) <= 1.2f) return COL_SPIDER_EYE;
        return COL_SPIDER;
      }
      if (h2 <= 80) halo = true;
      for (int i = 0; i < 8; ++i){                                        // nohy
        const float d1 = segDist2(dx, dy, 0, -3, s->kx[i], s->ky[i]);
        const float d2 = segDist2(dx, dy, s->kx[i], s->ky[i], s->fx[i], s->fy[i]);
        if (d1 <= 3.0f || d2 <= 2.2f) return COL_SPIDER;
        if (d1 <= 9.0f || d2 <= 8.0f) halo = true;
      }
      if (halo) return COL_SCLERA;
    }
  }
  uint16_t c2; if (gagPupilPix(x, y, s->pcx, s->pcy, PUPIL_R, c2)) return c2;
  return COL_SCLERA;
}

static void spiderLegs(SpiderCtx &s, float wig){
  const float Z = 1.4f; int i = 0;
  for (int k = -1; k <= 1; k += 2)
    for (int j = 0; j < 4; ++j, ++i){
      const float a = (-60.0f + 40.0f * j) * 0.017453f + 0.18f * sinf(wig + j * 1.3f + (k > 0 ? 0.0f : 1.6f));
      s.kx[i] = k * 16 * Z * cosf(a); s.ky[i] = 16 * Z * sinf(a);
      s.fx[i] = k * 24 * Z * cosf(a); s.fy[i] = (22 * sinf(a) + 6) * Z;
    }
}

static void activity_gag_spider(){
  gagStartCentered();
  SpiderCtx L, R;
  L.on = R.on = false;
  float plx = CX, ply = CY, prx = CX, pry = CY;              // plynulé polohy zreníc
  int rL[4] = { CX - 38, CY - 38, CX + 38, CY + 38 }, rR[4] = { CX - 38, CY - 38, CX + 38, CY + 38 };
  const unsigned long PH = 5000, TOT = 2 * PH + 900;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    const int inEye = (t < PH) ? 0 : (t < 2 * PH ? 1 : -1);       // 0 = ľavé, 1 = pravé, -1 = koniec
    float sx = 0, sy = -100;
    if (inEye >= 0){
      const unsigned long u = t - inEye * PH;
      const float base = (inEye == 0) ? CX + 42 : CX - 42;
      sx = base + 6.0f * sinf(u * 0.003f);
      const float top = eyeTopY - 45, hang = CY - 38;
      if (u < 2600)      sy = top + (hang - top) * ease(u / 2600.0f);
      else if (u < 4200) sy = hang + 4.0f * sinf((u - 2600) * 0.008f);
      else               { const float f = (u - 4200) / 800.0f; sy = hang + (top - hang) * f * f; }
    }
    SpiderCtx &S = (inEye == 0) ? L : R;
    L.on = (inEye == 0); R.on = (inEye == 1);
    if (inEye >= 0){ S.sx = sx; S.sy = sy; spiderLegs(S, t * ((sy > CY - 40) ? 0.02f : 0.008f)); }

    // kam sa pozerajú zrenice
    float tlx = CX, tly = CY, trx = CX, try_ = CY;
    if (inEye >= 0 && sy > eyeTopY - 20){
      const float ex = constrain((sx - CX) * 0.4f, -35.0f, 35.0f), ey = constrain((sy - CY) * 0.4f, -35.0f, 35.0f);
      if (inEye == 0){ tlx = CX + ex; tly = CY + ey; trx = CX - 35; try_ = CY + ey; }
      else           { trx = CX + ex; try_ = CY + ey; tlx = CX + 35; tly = CY + ey; }
    }
    plx += (tlx - plx) * 0.18f; ply += (tly - ply) * 0.18f;
    prx += (trx - prx) * 0.18f; pry += (try_ - pry) * 0.18f;
    if (last){ plx = prx = CX; ply = pry = CY; }
    L.pcx = (int)lroundf(plx); L.pcy = (int)lroundf(ply);
    R.pcx = (int)lroundf(prx); R.pcy = (int)lroundf(pry);

    for (int e = 0; e < 2; ++e){
      SpiderCtx &C = e ? R : L; int *pr = e ? rR : rL;
      int cx0 = C.pcx - 38, cy0 = C.pcy - 38, cx1 = C.pcx + 38, cy1 = C.pcy + 38;
      if (C.on){ cx0 = min(cx0, (int)C.sx - 42); cx1 = max(cx1, (int)C.sx + 42); cy0 = eyeTopY; cy1 = max(cy1, (int)C.sy + 38); }
      const int x0 = min(cx0, pr[0]), y0 = min(cy0, pr[1]), x1 = max(cx1, pr[2]), y1 = max(cy1, pr[3]);
      gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, x0, y0, x1, y1, shadeSpider, &C);
      pr[0] = cx0; pr[1] = cy0; pr[2] = cx1; pr[3] = cy1;
    }
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}


/* =====================================================================
 *  GAGY 24–29: netopiere, oheň, skener, zombie, sliz, noc
 * ===================================================================== */
static inline uint16_t lerp565(uint16_t a, uint16_t b, float t){
  if (t <= 0.0f) return a; if (t >= 1.0f) return b;
  const int ar = a >> 11, ag = (a >> 5) & 63, ab = a & 31;
  const int br = b >> 11, bg = (b >> 5) & 63, bb = b & 31;
  const int r = ar + (int)((br - ar) * t), g = ag + (int)((bg - ag) * t), bl = ab + (int)((bb - ab) * t);
  return (uint16_t)((r << 11) | (g << 5) | bl);
}
static inline float clamp01(float x){ return x < 0 ? 0 : (x > 1 ? 1 : x); }
// fáza gagu: 0 -> 1 na začiatku, 1 počas, 1 -> 0 na konci
static inline float gagEnvelope(unsigned long t, unsigned long tIn, unsigned long tHold, unsigned long tOut){
  if (gagBl >= 0){                                       // prechod cez podsvietenie: hneď plný efekt
    if (gagBl == 0){ gagBlOut(); gagBl = 1; }            // pred 1. snímkou zhasni
    else if (gagBl == 1) gagBlIn();                      // po 1. snímke rozsvieť
    return 1.0f;
  }
  if (t < tIn) return ease((float)t / tIn);
  if (t > tIn + tHold) return ease(1.0f - clamp01((float)(t - tIn - tHold) / tOut));
  return 1.0f;
}

/* ---------- 24: Netopiere – preletia cez obe oči ---------- */
static inline bool batPix(float dx, float dy, float s, float f){
  const float ax = fabsf(dx);
  if (ax > 27 * s || dy < -17 * s || dy > 16 * s) return false;
  if ((dx * dx) / (25 * s * s) + (dy * dy) / (49 * s * s) <= 1.0f) return true;          // telo
  const float hy = dy + 8 * s;
  if (dx * dx + hy * hy <= 16 * s * s) return true;                                      // hlava
  if (hy < 0 && hy > -8.5f * s && ax > 0.8f * s && ax < 4.2f * s && -hy < (4.2f * s - ax) * 2.4f) return true;  // uši
  if (ax < 4 * s) return false;
  const float t = (ax - 4 * s) / (23 * s);                                               // krídla
  const float yTop = -2 * s + t * (f * 10 * s) - 3 * s * sinf(3.14159f * t);
  const float bot  = yTop + 9 * s * (1 - t) + 1.5f * s - 3 * s * fabsf(sinf(9.42477f * t));
  return dy >= yTop && dy <= bot;
}
static uint16_t shadeBats(int x, int y, const void *c){
  const BatCtx *b = (const BatCtx *)c;
  for (int i = 0; i < 4; ++i){
    if (!b->vis[i]) continue;
    if (batPix(x - b->bx[i], y - b->by[i], b->s[i], b->f[i])) return COL_BAT;
  }
  uint16_t c2; if (gagPupilPix(x, y, b->pcx, b->pcy, PUPIL_R, c2)) return c2;
  return COL_SCLERA;
}
static void activity_gag_bats(){
  gagStartCentered();
  static const unsigned long st[4] = { 0, 700, 1600, 2300 };
  static const float by0[4] = { CY - 62, CY - 30, CY - 78, CY - 45 }, bs[4] = { 1.6f, 1.25f, 1.9f, 1.4f };
  const float V = 0.13f;                                  // rýchlosť letu px/ms
  const unsigned long TOT = 7300;
  BatCtx C[2];
  float px[2] = { CX, CX }, py[2] = { CY, CY };
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    for (int e = 0; e < 2; ++e){
      float sum = 0; int n = 0;
      for (int i = 0; i < 4; ++i){
        const float fx = (t >= st[i]) ? -40.0f + V * (t - st[i]) : -1000.0f;   // poloha v „tvári“ (obe oči vedľa seba)
        const float lx = fx - e * 260.0f;
        C[e].vis[i] = (lx > -40 && lx < 280);
        C[e].bx[i] = lx; C[e].by[i] = by0[i] + 10.0f * sinf(t * 0.004f + i);
        C[e].s[i] = bs[i]; C[e].f[i] = sinf(t * 0.035f + i * 1.7f);
        if (lx > 10 && lx < 230){ sum += lx; ++n; }
      }
      const float tx = n ? CX + constrain((sum / n - CX) * 0.35f, -30.0f, 30.0f) : CX;
      const float ty = n ? CY - 20 : CY;
      px[e] += (tx - px[e]) * 0.15f; py[e] += (ty - py[e]) * 0.15f;
      if (last){ px[e] = CX; py[e] = CY; }
      C[e].pcx = (int)lroundf(px[e]); C[e].pcy = (int)lroundf(py[e]);
    }
    gagRenderRect(leftGFX(),  petBufL, CX - EYE_R, CY - 108, CX + EYE_R, CY + 62, shadeBats, &C[0]);
    gagRenderRect(rightGFX(), petBufR, CX - EYE_R, CY - 108, CX + EYE_R, CY + 62, shadeBats, &C[1]);
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 25: Ohnivé oči – plamene okolo zrenice ---------- */
static uint16_t shadeFire(int x, int y, const void *c){
  const FireCtx *f = (const FireCtx *)c;
  const float dx = x - CX, dy = y - CY, d2 = dx * dx + dy * dy;
  if (d2 <= (float)f->pr * f->pr) return COL_PUPIL;
  if (f->g < 0.02f || d2 > 76.0f * 76.0f) return COL_SCLERA;
  const float r = sqrtf(d2), th = atan2f(dy, dx);
  const float n = 0.55f + 0.25f * sinf(5 * th + f->t * 0.011f + f->ph) + 0.2f * sinf(9 * th - f->t * 0.017f);
  const float up = (r > 0.5f) ? fmaxf(0.0f, -dy / r) : 0.0f;                          // plamene šľahajú hore
  const float Rf = f->g * (34 + 22 * n + 14 * up * (0.6f + 0.4f * sinf(f->t * 0.02f + 3 * th)));
  if (r > Rf) return COL_SCLERA;
  const float q = r / Rf;
  if (q < 0.45f) return COL_FIRE_Y;
  if (q < 0.70f) return COL_FIRE_O;
  if (q < 0.90f) return COL_FIRE_R;
  return COL_FIRE_D;
}
static void activity_gag_fire_eyes(){
  gagStartCentered();
  gagBlArm();
  const unsigned long T_IN = 700, T_HOLD = 7000, T_OUT = 700, TOT = T_IN + T_HOLD + T_OUT;
  FireCtx L, R;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    const float g = gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    L.g = R.g = g; L.t = R.t = (float)t; L.ph = 0.0f; R.ph = 2.1f;
    L.pr = R.pr = (int)lroundf(PUPIL_R + (16 - PUPIL_R) * g);
    gagRenderRect(leftGFX(),  petBufL, CX - 76, CY - 76, CX + 76, CY + 76, shadeFire, &L);
    gagRenderRect(rightGFX(), petBufR, CX - 76, CY - 76, CX + 76, CY + 76, shadeFire, &R);
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 26: Robot – červený skener cez obe oči ---------- */
static uint16_t shadeScan(int x, int y, const void *c){
  const ScanCtx *s = (const ScanCtx *)c;
  if (s->g > 0.01f && abs(y - CY) <= 12){
    const uint16_t band = lerp565(s->scl, COL_SCAN_BAND, s->g);
    const float d = (x + s->off) - s->pos;
    if (fabsf(d) <= 7) return lerp565(band, COL_SCAN_CORE, s->g);
    const float len = (d * s->dir <= 0) ? 80.0f : 14.0f;                         // za skenerom ťahá chvost
    const float i = 1.0f - fabsf(d) / len;
    if (i > 0) return lerp565(band, COL_SCAN_HOT, i * s->g);
    return band;
  }
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, s->pr, c2)) return c2;
  return s->scl;
}
static void activity_gag_scanner(){
  gagStartCentered();
  gagBlArm();
  const unsigned long T_IN = 900, T_HOLD = 6500, T_OUT = 900, TOT = T_IN + T_HOLD + T_OUT;
  ScanCtx S[2];
  S[0].off = 0; S[1].off = 250;
  bool fullAtOne = false;                              // jedno celé prekreslenie aj pri g = 1 (presné farby)
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    const float g = gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    const float ph = (t > T_IN) ? (t - T_IN) * 0.0016f : 0.0f;
    for (int e = 0; e < 2; ++e){
      S[e].g = g; S[e].scl = lerp565(COL_SCLERA, COL_SCAN_DARK, g);
      S[e].pr = (int)lroundf(PUPIL_R * (1.0f - g));
      S[e].pos = 245.0f - 245.0f * cosf(ph);
      S[e].dir = sinf(ph) >= 0 ? 1.0f : -1.0f;
    }
    const bool full = (g < 1.0f) || last || !fullAtOne;  // počas prechodu celé oko, inak len pás
    if (g >= 1.0f) fullAtOne = true;
    for (int e = 0; e < 2; ++e){
      Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
      if (full) gagRenderRect(gx, buf, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeScan, &S[e]);
      else      gagRenderRect(gx, buf, CX - EYE_R, CY - 12, CX + EYE_R, CY + 12, shadeScan, &S[e]);
    }
    if (last) break;
    delay(full ? 2 : 8);
  }
  gagEndNormal();
}

/* ---------- 27: Zombie – zakalené oči, každé iné ---------- */
static int16_t zbx[2][7], zby[2][7]; static uint8_t zbr[2][7];
static uint16_t shadeZombie(int x, int y, const void *c){
  const ZombieCtx *z = (const ZombieCtx *)c;
  if (y <= z->lidY) return COL_LID_TOP;
  if (z->lidY >= eyeTopY && y <= z->lidY + 3) return COL_LID_SEAM;
  const int dx = x - z->pcx, dy = y - z->pcy, d2 = dx * dx + dy * dy;
  if (d2 <= z->pr * z->pr){
    const int hx = dx - 12, hy = dy + 12;
    if (z->g > 0.5f && hx * hx + hy * hy <= 25) return COL_ZOMBIE_HL;
    return lerp565(COL_PUPIL, COL_ZOMBIE_PUP, z->g);
  }
  if (d2 <= z->ir * z->ir){
    const int hx = dx - 12, hy = dy + 12;
    if (hx * hx + hy * hy <= 25) return COL_ZOMBIE_HL;
    return (d2 >= (z->ir - 4) * (z->ir - 4)) ? COL_ZOMBIE_RING : COL_ZOMBIE_IRIS;
  }
  for (int i = 0; i < 7; ++i){
    const int bx = x - zbx[z->e][i], by = y - zby[z->e][i];
    if (bx * bx + by * by <= zbr[z->e][i] * zbr[z->e][i]) return lerp565(z->scl, COL_ZOMBIE_SPOT, z->g);
  }
  return z->scl;
}
static void activity_gag_zombie(){
  gagStartCentered();
  gagBlArm();
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 7; ++i){
    zbx[e][i] = 25 + rand() % 190; zby[e][i] = 140 + rand() % 75; zbr[e][i] = 3 + rand() % 5;
  }
  const unsigned long T_IN = 1000, T_HOLD = 7000, T_OUT = 1000, TOT = T_IN + T_HOLD + T_OUT;
  const int prT[2] = { 14, 26 };
  ZombieCtx Z[2];
  int prevBox[2][4] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
  bool fullAtOne = false;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    const float g = gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    const bool full = (g < 1.0f) || last || !fullAtOne;
    if (g >= 1.0f) fullAtOne = true;
    for (int e = 0; e < 2; ++e){
      ZombieCtx &z = Z[e];
      z.e = e; z.g = g;
      z.scl = lerp565(COL_SCLERA, COL_ZOMBIE_SCL, g);
      z.pr = (int)lroundf(PUPIL_R + (prT[e] - PUPIL_R) * g);
      z.ir = (int)lroundf(44 * g);
      z.lidY = (e == 1) ? (int)lroundf((eyeTopY - 1) + ((CY - 28) - (eyeTopY - 1)) * g) : eyeTopY - 1;  // pravé viečko ovisne
      float wx = 22 * sinf(t * 0.0011f + e * 2.0f), wy = 16 * sinf(t * 0.0007f + e);
      if (e == 1 && t > 4000 && t < 4300) wx += 20;                       // trhnutie
      z.pcx = CX + (int)lroundf(wx * g); z.pcy = CY + (int)lroundf(wy * g);
      Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
      const int bx0 = z.pcx - 48, by0 = z.pcy - 48, bx1 = z.pcx + 48, by1 = z.pcy + 48;
      if (full) gagRenderRect(gx, buf, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeZombie, &z);
      else gagRenderRect(gx, buf, min(bx0, prevBox[e][0]), min(by0, prevBox[e][1]), max(bx1, prevBox[e][2]), max(by1, prevBox[e][3]), shadeZombie, &z);
      prevBox[e][0] = bx0; prevBox[e][1] = by0; prevBox[e][2] = bx1; prevBox[e][3] = by1;
    }
    if (last) break;
    delay(4);
  }
  gagEndNormal();
}

/* ---------- 28: Sliz – steká zhora ---------- */
static const int8_t  slDx[7]  = { -80, -58, -32, -7, 18, 45, 72 };     // poloha kvapiek voči stredu
static const uint8_t slW[7]   = { 14, 10, 16, 12, 18, 11, 14 };
static const uint8_t slMax[7] = { 40, 75, 30, 95, 55, 70, 35 };
static uint16_t shadeSlime(int x, int y, const void *c){
  const SlimeCtx *s = (const SlimeCtx *)c;
  if (s->g > 0.01f){
    const int col = x - (CX - EYE_R);
    const int band = s->band[col];
    if (y <= band) return (y >= band - 2) ? COL_SLIME_D : COL_SLIME;
    for (int i = 0; i < 7; ++i){
      const float hw = slW[i] * 0.5f * s->g, cx = CX + slDx[i];
      const float ddx = x - cx;
      if (fabsf(ddx) > hw * 1.2f + 1) continue;
      const float yEnd = s->bandAt[i] + s->L[i];
      if (y <= yEnd && fabsf(ddx) <= hw) return (ddx < -hw + 2.5f) ? COL_SLIME_L : COL_SLIME;
      const float ey = y - yEnd, r = hw * 1.15f;
      if (ddx * ddx + ey * ey <= r * r) return COL_SLIME;
    }
    if (s->dropY > 0){
      const float ddx = x - s->dropX, ddy = y - s->dropY;
      if (ddx * ddx + ddy * ddy <= 36) return COL_SLIME;
    }
  }
  uint16_t c2; if (gagPupilPix(x, y, s->pcx, s->pcy, PUPIL_R, c2)) return c2;
  return COL_SCLERA;
}
static void activity_gag_slime(){
  gagStartCentered();
  const unsigned long TOT = 9000;
  SlimeCtx S;
  float px = CX, py = CY;
  int prevBot = CY + PUPIL_R + 2;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    float g = ease(clamp01(t / 1200.0f));
    if (t > 7000) g = ease(1.0f - clamp01((t - 7000) / 1500.0f));
    S.g = g;
    for (int i = 0; i < CX - (CX - EYE_R) + EYE_R + 1; ++i){
      const int x = (CX - EYE_R) + i;
      S.band[i] = (int16_t)lroundf(eyeTopY + g * (26 + 6 * sinf(x * 0.09f)));
    }
    int maxBot = eyeTopY;
    for (int i = 0; i < 7; ++i){
      const int col = (CX + slDx[i]) - (CX - EYE_R);
      S.bandAt[i] = S.band[col];
      float grow = ease(clamp01(((float)t - 600 - i * 150) / 4000.0f));
      if (i == 3 && t > 4200) grow = 0.25f;                       // z najdlhšej sa odtrhla kvapka
      S.L[i] = slMax[i] * grow * g;
      maxBot = max(maxBot, (int)(S.bandAt[i] + S.L[i] + slW[i]));
    }
    S.dropY = 0; S.dropX = CX + slDx[3];
    if (t > 4200 && t < 5400){
      const float f = (t - 4200) / 1200.0f;
      S.dropY = S.bandAt[3] + slMax[3] + (eyeBotY + 12 - S.bandAt[3] - slMax[3]) * f * f;
      maxBot = max(maxBot, (int)S.dropY + 8);
    }
    float tx = CX, ty = CY;
    if (t > 600 && t <= 4200){ tx = CX + slDx[3] * 0.3f; ty = CY - 22; }
    else if (S.dropY > 0){ tx = CX + slDx[3] * 0.3f; ty = CY + constrain((S.dropY - CY) * 0.5f, -30.0f, 40.0f); }
    px += (tx - px) * 0.15f; py += (ty - py) * 0.15f;
    if (last){ px = CX; py = CY; }
    S.pcx = (int)lroundf(px); S.pcy = (int)lroundf(py);
    const int bot = max(maxBot, S.pcy + PUPIL_R + 2);
    const int y1 = min(eyeBotY, max(bot, prevBot));
    gagRenderRect(leftGFX(),  petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, y1, shadeSlime, &S);
    gagRenderRect(rightGFX(), petBufR, CX - EYE_R, eyeTopY, CX + EYE_R, y1, shadeSlime, &S);
    prevBot = bot;
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 29: Noc – mesiac a hviezdy ---------- */
#define NIGHT_STARS 30
static int16_t nsx[2][NIGHT_STARS], nsy[2][NIGHT_STARS]; static uint8_t nsr[2][NIGHT_STARS];
static float nsw[2][NIGHT_STARS], nsp[2][NIGHT_STARS];
static uint16_t shadeNight(int x, int y, const void *c){
  const NightCtx *n = (const NightCtx *)c;
  const uint16_t sky = lerp565(COL_SCLERA, COL_NIGHT_SKY, n->g);
  const float dx = x - CX, dy = y - CY, d2 = dx * dx + dy * dy;
  const float rm = 40.0f * n->g;
  if (d2 <= rm * rm){                                             // mesiac (kosák)
    const float sx = dx - n->side * rm * (0.35f + 0.5f * n->phase), sy = dy + 6;
    if (sx * sx + sy * sy > 0.81f * rm * rm) return COL_MOON;
  }
  const float pr = PUPIL_R * (1.0f - n->g);
  if (d2 <= pr * pr) return COL_PUPIL;
  if (n->g > 0.02f){
    if (n->shoot >= 0){                                           // padajúca hviezda
      const float hx = n->shx + 140 * n->shoot, hy = n->shy + 60 * n->shoot;
      const float d = segDist2(x, y, hx, hy, hx - 28, hy - 12);
      if (d <= 1.5f){
        const float along = clamp01(((x - (hx - 28)) / 28.0f));
        return lerp565(sky, COL_STARC, along * n->g);
      }
    }
    if (d2 > (rm + 5) * (rm + 5)){
      for (int i = 0; i < NIGHT_STARS; ++i){
        const int sx = x - nsx[n->e][i], sy = y - nsy[n->e][i];
        if (sx > 2 || sx < -2 || sy > 2 || sy < -2) continue;
        if (sx * sx + sy * sy <= nsr[n->e][i] * nsr[n->e][i]){
          const float b = n->g * (0.35f + 0.65f * (0.5f + 0.5f * sinf(n->t * nsw[n->e][i] + nsp[n->e][i])));
          return lerp565(sky, COL_STARC, b);
        }
      }
    }
  }
  return sky;
}
static void activity_gag_night(){
  gagStartCentered();
  gagBlArm();
  for (int e = 0; e < 2; ++e) for (int i = 0; i < NIGHT_STARS; ++i){
    nsx[e][i] = 20 + rand() % 200; nsy[e][i] = 20 + rand() % 200; nsr[e][i] = (rand() % 3 == 0) ? 2 : 1;
    nsw[e][i] = 0.002f + (rand() % 100) * 0.00005f; nsp[e][i] = (rand() % 628) * 0.01f;
  }
  const unsigned long T_IN = 1200, T_HOLD = 7000, T_OUT = 1200, TOT = T_IN + T_HOLD + T_OUT;
  NightCtx N[2], O[2];
  bool first = true;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    const float g = gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    for (int e = 0; e < 2; ++e){
      NightCtx &n = N[e];
      n.e = e; n.g = g; n.t = (float)t;
      n.side = (e == 0) ? 1.0f : -1.0f;
      n.phase = clamp01((t - T_IN) / (float)T_HOLD);
      const unsigned long s0 = e ? 5200 : 2600;                  // padajúca hviezda raz v každom oku
      n.shoot = (t > s0 && t < s0 + 700) ? (t - s0) / 700.0f : -1.0f;
      n.shx = 40; n.shy = 50;
    }
    for (int e = 0; e < 2; ++e){
      Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
      const NightCtx &o = O[e];
      if (first || N[e].g != o.g){ gagRenderRect(gx, buf, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeNight, &N[e]); O[e] = N[e]; continue; }
      // mesiac (kosák sa mení pomaly) sa prekreslí, až keď sa posunie aspoň o pol pixla;
      // dovtedy sa všetko kreslí so starou fázou, aby obraz sedel
      NightCtx n = N[e];
      const bool moon = fabsf(n.phase - o.phase) * 20.0f >= 0.5f || last;
      if (!moon) n.phase = o.phase;
      if (moon) gagRenderRect(gx, buf, CX - 42, CY - 42, CX + 42, CY + 42, shadeNight, &n);
      // hviezdy blikajú – len ich malé štvorčeky
      for (int i = 0; i < NIGHT_STARS; ++i)
        gagRenderRect(gx, buf, nsx[e][i] - 2, nsy[e][i] - 2, nsx[e][i] + 2, nsy[e][i] + 2, shadeNight, &n);
      // padajúca hviezda (stará aj nová poloha)
      if (n.shoot >= 0 || o.shoot >= 0){
        float xa = 1e9f, xb = -1e9f, ya = 1e9f, yb = -1e9f;
        const NightCtx *st[2] = { &o, &n };
        for (int k = 0; k < 2; ++k) if (st[k]->shoot >= 0){
          const float hx = st[k]->shx + 140 * st[k]->shoot, hy = st[k]->shy + 60 * st[k]->shoot;
          xa = min(xa, hx - 28); xb = max(xb, hx); ya = min(ya, hy - 12); yb = max(yb, hy);
        }
        gagRenderRect(gx, buf, (int)xa - 3, (int)ya - 3, (int)xb + 3, (int)yb + 3, shadeNight, &n);
      }
      O[e] = n;
    }
    first = false;
    if (last) break;
    delay(20);
  }
  gagEndNormal();
}


/* =====================================================================
 *  GAGY 30–33: akvárium, hracie automaty, blesky, kyklop
 * ===================================================================== */

/* ---------- 30: Akvárium s rybkou ---------- */
static int16_t aqBx[2][8], aqBo[2][8]; static uint8_t aqBr[2][8];
static inline bool fishPix(float dx, float dy, float dir, uint16_t &out){
  if (fabsf(dx) > 50 || fabsf(dy) > 22) return false;
  const float fx = dx * dir;                                       // fx > 0 = smer hlavy
  if ((fx * fx) / (26.0f * 26.0f) + (dy * dy) / (15.0f * 15.0f) <= 1.0f){
    const float ex = fx - 14, ey = dy + 4;
    if (ex * ex + ey * ey <= 12) out = COL_PUPIL;
    else if (ex * ex + ey * ey <= 20) out = COL_SCLERA;
    else out = (fabsf(fx + 4) < 2.5f && fabsf(dy) < 11) ? COL_FISH_D : COL_FISH;   // pásik
    return true;
  }
  if (fx < -20 && fx > -46 && fabsf(dy) < (-20 - fx) * 0.75f){ out = COL_FISH_D; return true; }  // chvost
  return false;
}
static uint16_t shadeAquarium(int x, int y, const void *c){
  const AquaCtx *a = (const AquaCtx *)c;
  const uint16_t water = lerp565(COL_SCLERA, lerp565(COL_WATER, COL_WATER_D, (y - eyeTopY) / 220.0f), a->g);
  if (a->g > 0.02f){
    uint16_t fc;
    if (a->fishVis && fishPix(x - a->fx, y - a->fy, a->dir, fc)) return lerp565(water, fc, a->g);
    for (int i = 0; i < 8; ++i){                                   // bubliny
      const float bx = aqBx[a->e][i] + 4.0f * sinf(a->t * 0.004f + i), by = a->by[i];
      const float dx = x - bx, dy = y - by, r = aqBr[a->e][i];
      const float d2 = dx * dx + dy * dy;
      if (d2 <= r * r){
        const float hx = dx + r * 0.35f, hy = dy + r * 0.35f;
        return lerp565(water, (hx * hx + hy * hy <= r * r * 0.15f) ? COL_SCLERA : COL_BUBBLE, a->g);
      }
    }
    static const int16_t wx[3] = { 55, 150, 190 }; static const uint8_t wh[3] = { 70, 95, 55 };
    for (int i = 0; i < 3; ++i){                                   // riasy
      const int base = 228, top = base - (int)(wh[i] * a->g);
      if (y < top || y > base) continue;
      const float k = (float)(base - y) / wh[i];
      const float cx = wx[i] + 9.0f * k * sinf(a->t * 0.002f + i * 1.7f);
      if (fabsf(x - cx) <= 8.0f * (1.0f - k) + 1.0f) return lerp565(water, COL_WEED, a->g);
    }
  }
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, (int)lroundf(PUPIL_R * (1.0f - a->g)), c2)) return c2;
  return water;
}
/* Rýchle kreslenie akvária: voda je vopred vypočítaná po riadkoch, polohy
 * bublín a rias sa počítajú raz za snímok a prekresľuje sa len okolie rybky,
 * bublín a riasy – zvyšok vody sa neposiela. */
static uint16_t aqWater[240];
static const int16_t aqWx[3] = { 55, 150, 190 }; static const uint8_t aqWh[3] = { 70, 95, 55 };
static uint16_t shadeAquaFast(int x, int y, const void *c){
  const AquaFast *a = (const AquaFast *)c;
  uint16_t fc;
  if (a->fishVis && fishPix(x - a->fx, y - a->fy, a->dir, fc)) return fc;
  for (int i = 0; i < 8; ++i){                                     // bubliny
    const float dx = x - a->bx[i], dy = y - a->by[i], r = aqBr[a->e][i];
    if (fabsf(dy) > r || fabsf(dx) > r) continue;
    if (dx * dx + dy * dy <= r * r){
      const float hx = dx + r * 0.35f, hy = dy + r * 0.35f;
      return (hx * hx + hy * hy <= r * r * 0.15f) ? COL_SCLERA : COL_BUBBLE;
    }
  }
  for (int i = 0; i < 3; ++i){                                     // riasy
    const int base = 228, top = base - aqWh[i];
    if (y < top || y > base) continue;
    const float k = (float)(base - y) / aqWh[i];
    const float cx = aqWx[i] + 9.0f * k * a->ws[i];
    if (fabsf(x - cx) <= 8.0f * (1.0f - k) + 1.0f) return COL_WEED;
  }
  return aqWater[y];
}
static inline void aqRect(int e, const AquaFast &a, int x0, int y0, int x1, int y1){
  gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, x0, y0, x1, y1, shadeAquaFast, &a);
}
static void activity_gag_aquarium(){
  gagStartCentered();
  gagBlArm();
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 8; ++i){
    aqBx[e][i] = 30 + rand() % 180; aqBo[e][i] = rand() % 260; aqBr[e][i] = 3 + rand() % 4;
  }
  for (int y = 0; y < 240; ++y) aqWater[y] = lerp565(COL_WATER, COL_WATER_D, (y - eyeTopY) / 220.0f);
  const unsigned long T_IN = 1000, T_HOLD = 10000, T_OUT = 1000, TOT = T_IN + T_HOLD + T_OUT;
  AquaFast A[2], O[2];
  bool first = true; int frame = 0;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    (void)gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    // rybka pláva cez obe oči tam (5 s) a späť (5 s)
    const float u = (t > T_IN) ? (t - T_IN) / 5000.0f : 0.0f;
    float faceX, dir;
    if (u < 1.0f){ faceX = -60 + 610 * u; dir = 1; } else { faceX = 550 - 610 * clamp01(u - 1.0f); dir = -1; }
    const bool weedTick = first || (frame % 3 == 0);                 // riasy sa hýbu pomaly – stačí každý 3. snímok
    for (int e = 0; e < 2; ++e){
      AquaFast &a = A[e];
      a.e = e; a.dir = dir;
      a.fx = faceX - e * 260.0f; a.fy = CY + 25.0f * sinf(t * 0.0015f);
      a.fishVis = (t > T_IN && a.fx > -60 && a.fx < 300);
      for (int i = 0; i < 8; ++i){
        a.bx[i] = aqBx[e][i] + 4.0f * sinf(t * 0.004f + i);
        a.by[i] = 232.0f - fmodf(t * 0.04f + aqBo[e][i], 260.0f);
      }
      for (int i = 0; i < 3; ++i) a.ws[i] = weedTick ? sinf(t * 0.002f + i * 1.7f) : O[e].ws[i];
      if (first){ aqRect(e, a, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY); O[e] = a; continue; }
      const AquaFast &o = O[e];
      if (a.fishVis || o.fishVis){                                     // rybka (stará aj nová poloha)
        float xa = 1e9f, xb = -1e9f, ya = 1e9f, yb = -1e9f;
        if (a.fishVis){ xa = min(xa, a.fx); xb = max(xb, a.fx); ya = min(ya, a.fy); yb = max(yb, a.fy); }
        if (o.fishVis){ xa = min(xa, o.fx); xb = max(xb, o.fx); ya = min(ya, o.fy); yb = max(yb, o.fy); }
        aqRect(e, a, (int)xa - 52, (int)ya - 24, (int)xb + 52, (int)yb + 24);
      }
      for (int i = 0; i < 8; ++i){                                     // bubliny
        const int r = aqBr[e][i] + 2;
        if (fabsf(a.by[i] - o.by[i]) > 40){                            // bublina preskočila hore -> dole
          aqRect(e, a, (int)o.bx[i] - r, (int)o.by[i] - r, (int)o.bx[i] + r, (int)o.by[i] + r);
          aqRect(e, a, (int)a.bx[i] - r, (int)a.by[i] - r, (int)a.bx[i] + r, (int)a.by[i] + r);
        } else
          aqRect(e, a, (int)min(a.bx[i], o.bx[i]) - r, (int)min(a.by[i], o.by[i]) - r, (int)max(a.bx[i], o.bx[i]) + r, (int)max(a.by[i], o.by[i]) + r);
      }
      if (weedTick)                                                    // riasy
        for (int i = 0; i < 3; ++i) aqRect(e, a, aqWx[i] - 19, 228 - aqWh[i], aqWx[i] + 19, 228);
      O[e] = a;
    }
    first = false; ++frame;
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 31: Hracie automaty – roztočené symboly ---------- */
// symboly: 0 netopier, 1 tekvica, 2 duch, 3 lebka; lx, ly = poloha voči stredu bunky
static bool slotSymbol(int id, float lx, float ly, uint16_t &out){
  switch (id){
    case 0: if (batPix(lx, ly + 2, 1.0f, 0.3f)){ out = COL_BAT; return true; } return false;
    case 1: {
      if (fabsf(lx) < 3 && ly < -17 && ly > -27){ out = COL_STEM; return true; }
      if ((lx * lx) / 576.0f + ((ly + 1) * (ly + 1)) / 400.0f > 1.0f) return false;
      if (ly > -9 && ly < -2 && (fabsf(lx + 9) < (ly + 9) * 0.6f || fabsf(lx - 9) < (ly + 9) * 0.6f)){ out = COL_PUPIL; return true; }
      if (ly > 4 && ly < 12 && fabsf(lx) < 15 && !(((int)(lx + 20)) % 8 < 3 && ly < 7)){ out = COL_PUPIL; return true; }
      out = COL_PUMPKIN; return true;
    }
    case 2: {
      bool in = (ly <= 0) ? (lx * lx + ly * ly <= 400) : (fabsf(lx) <= 20 && ly <= 18 - 4 * fabsf(sinf(lx * 0.24f)));
      if (!in) return false;
      if (((lx + 7) * (lx + 7)) / 9.0f + ((ly + 4) * (ly + 4)) / 25.0f <= 1 || ((lx - 7) * (lx - 7)) / 9.0f + ((ly + 4) * (ly + 4)) / 25.0f <= 1){ out = COL_PUPIL; return true; }
      out = COL_GHOST; return true;
    }
    default: {
      const bool head = lx * lx + (ly + 4) * (ly + 4) <= 400, jaw = fabsf(lx) <= 12 && ly > 8 && ly < 22;
      if (!head && !jaw) return false;
      if ((lx + 8) * (lx + 8) + (ly + 5) * (ly + 5) <= 36 || (lx - 8) * (lx - 8) + (ly + 5) * (ly + 5) <= 36){ out = COL_PUPIL; return true; }
      if (ly > 3 && ly < 9 && fabsf(lx) < (ly - 3) * 0.5f + 0.5f){ out = COL_PUPIL; return true; }
      if (jaw && ly > 13 && ((int)(lx + 12)) % 6 == 0){ out = COL_PUPIL; return true; }
      out = COL_BONE; return true;
    }
  }
}
#define SLOT_HW   46     // polovičná šírka okienka
#define SLOT_HH   40     // polovičná výška okienka
#define SLOT_CELL 64     // výška jednej bunky valca
static uint16_t shadeSlots(int x, int y, const void *c){
  const SlotCtx *s = (const SlotCtx *)c;
  const int dx = x - CX, dy = y - CY;
  const int hh = (int)(SLOT_HH * s->g);
  if (s->g > 0.02f && abs(dx) <= SLOT_HW && abs(dy) <= hh + 3){
    if (abs(dx) > SLOT_HW - 3 || abs(dy) > hh) return s->frame;                    // rámik
    const float v = dy + s->pos * SLOT_CELL + SLOT_CELL * 0.5f;
    int cell = (int)floorf(v / SLOT_CELL);
    const float ly = v - cell * SLOT_CELL - SLOT_CELL * 0.5f;
    cell = ((cell % 4) + 4) % 4;
    uint16_t sc = COL_SLOT_BG;
    slotSymbol(cell, (float)dx, ly, sc);
    const float edge = clamp01((fabsf((float)dy) - (hh - 12)) / 12.0f);            // tieň hore a dole
    return lerp565(sc, COL_SLOT_SHADE, edge * 0.6f);
  }
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, (int)lroundf(PUPIL_R * (1.0f - s->g)), c2)) return c2;
  return COL_SCLERA;
}
static float slotSpin(float p0, int target, unsigned long t, unsigned long dur){
  float pT = floorf(p0) + 14 + (float)(((target - ((int)floorf(p0) + 14)) % 4 + 4) % 4);
  const float f = clamp01((float)t / dur);
  return p0 + (pT - p0) * (1.0f - (1.0f - f) * (1.0f - f) * (1.0f - f));
}
static void activity_gag_slots(){
  gagStartCentered();
  SlotCtx S[2];
  S[0].pos = 0; S[1].pos = 2;
  for (int e = 0; e < 2; ++e) S[e].frame = COL_SLOT_FRAME;
  // otvorenie okienok
  gagBlOut();
  {
    for (int e = 0; e < 2; ++e){ S[e].g = 1.0f; gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - 48, CY - 48, CX + 48, CY + 48, shadeSlots, &S[e]); }
    gagBlIn();
    delay(15);
  }
  static const uint8_t tgt[3][2] = { { 0, 1 }, { 2, 3 }, { 1, 1 } };    // posledné kolo = výhra
  for (int r = 0; r < 3; ++r){
    const float p0[2] = { S[0].pos, S[1].pos };
    const unsigned long D[2] = { 1800, 2500 };
    const unsigned long t0 = millis();
    for (;;){
      const unsigned long t = millis() - t0;
      for (int e = 0; e < 2; ++e){
        S[e].pos = slotSpin(p0[e], tgt[r][e], t, D[e]);
        gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - SLOT_HW, CY - SLOT_HH - 3, CX + SLOT_HW, CY + SLOT_HH + 3, shadeSlots, &S[e]);
      }
      if (t >= D[1]) break;
    }
    for (int e = 0; e < 2; ++e) S[e].pos = floorf(S[e].pos + 0.5f);
    if (r == 2){                                                      // výhra: rámik bliká
      for (int k = 0; k < 8; ++k){
        for (int e = 0; e < 2; ++e){
          S[e].frame = (k & 1) ? COL_SLOT_FRAME : COL_STAR;
          gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - SLOT_HW, CY - SLOT_HH - 3, CX + SLOT_HW, CY + SLOT_HH + 3, shadeSlots, &S[e]);
        }
        delay(180);
      }
    }
    delay(900);
  }
  gagEndNormal();                                                       // zatvorenie okienok cez podsvietenie
}

/* ---------- 32: Blesky – elektrické výboje ---------- */
#define ZAP_BOLTS 4
#define ZAP_PTS   8
static int16_t zpx[2][ZAP_BOLTS][ZAP_PTS], zpy[2][ZAP_BOLTS][ZAP_PTS]; static uint8_t zpn[2][ZAP_BOLTS];
static uint16_t shadeZap(int x, int y, const void *c){
  const ZapCtx *z = (const ZapCtx *)c;
  const uint16_t sky = lerp565(COL_SCLERA, z->flash ? COL_ZAP_FLASH : COL_ZAP_SKY, z->g);
  const int dx = x - CX, dy = y - CY, d2 = dx * dx + dy * dy;
  const int pr = (int)lroundf(PUPIL_R + (22 - PUPIL_R) * z->g);
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, pr, c2)) return c2;
  const int rr = pr + (int)(8 * z->g);
  if (z->g > 0.02f && d2 <= rr * rr) return lerp565(COL_SCLERA, COL_ZAP_GLOW, z->g);
  return sky;
}
static void zapMakeBolts(int e){
  for (int b = 0; b < ZAP_BOLTS; ++b){
    float a = (rand() % 628) * 0.01f, x = CX + 34 * cosf(a), y = CY + 34 * sinf(a);
    zpx[e][b][0] = (int16_t)x; zpy[e][b][0] = (int16_t)y; int n = 1;
    for (; n < ZAP_PTS; ++n){
      const float a2 = a + ((rand() % 140) - 70) * 0.01f, st = 11 + rand() % 6;
      const float nx = x + st * cosf(a2), ny = y + st * sinf(a2);
      if ((nx - CX) * (nx - CX) + (ny - CY) * (ny - CY) > (EYE_R - 4) * (EYE_R - 4)) break;
      x = nx; y = ny; zpx[e][b][n] = (int16_t)x; zpy[e][b][n] = (int16_t)y;
    }
    zpn[e][b] = (uint8_t)n;
  }
}
static void zapDrawBolts(Arduino_GFX *gx, int e, uint16_t glow, uint16_t core){
  for (int b = 0; b < ZAP_BOLTS; ++b) for (int i = 1; i < zpn[e][b]; ++i){
    const int x0 = zpx[e][b][i - 1], y0 = zpy[e][b][i - 1], x1 = zpx[e][b][i], y1 = zpy[e][b][i];
    gx->drawLine(x0 - 1, y0, x1 - 1, y1, glow); gx->drawLine(x0 + 1, y0, x1 + 1, y1, glow);
    gx->drawLine(x0, y0 - 1, x1, y1 - 1, glow); gx->drawLine(x0, y0, x1, y1, core);
  }
}
static void activity_gag_lightning(){
  gagStartCentered();
  ZapCtx Z; Z.flash = false;
  gagBlOut();                                                           // stmavnutie cez podsvietenie
  Z.g = 1.0f;
  gagRenderRect(leftGFX(),  petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeZap, &Z);
  gagRenderRect(rightGFX(), petBufR, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeZap, &Z);
  gagBlIn();
  bool have = false;
  const unsigned long t0 = millis();
  int flashes = 0;
  while (millis() - t0 < 8000){
    if (have){                                                          // zmaž staré výboje farbou pozadia
      zapDrawBolts(leftGFX(), 0, COL_ZAP_SKY, COL_ZAP_SKY);
      zapDrawBolts(rightGFX(), 1, COL_ZAP_SKY, COL_ZAP_SKY);
    }
    if ((flashes == 0 && millis() - t0 > 2500) || (flashes == 1 && millis() - t0 > 5500)){   // záblesk
      ++flashes; Z.flash = true;
      gagRenderRect(leftGFX(),  petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeZap, &Z);
      gagRenderRect(rightGFX(), petBufR, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeZap, &Z);
      delay(60); Z.flash = false;
      gagRenderRect(leftGFX(),  petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeZap, &Z);
      gagRenderRect(rightGFX(), petBufR, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeZap, &Z);
    }
    zapMakeBolts(0); zapMakeBolts(1);
    zapDrawBolts(leftGFX(), 0, COL_ZAP_GLOW, COL_SCLERA);
    zapDrawBolts(rightGFX(), 1, COL_ZAP_GLOW, COL_SCLERA);
    have = true;
    delay(90 + rand() % 160);
  }
  gagEndNormal();                                                       // späť cez podsvietenie
}

/* ---------- 33: Kyklop – jedno veľké oko cez oba displeje ---------- */
static uint16_t shadeCyclops(int x, int y, const void *c){
  const CycCtx *k = (const CycCtx *)c;
  if (y <= k->lidY) return COL_LID_TOP;
  if (k->lidY >= eyeTopY && y <= k->lidY + 3) return COL_LID_SEAM;
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, (int)lroundf(PUPIL_R * (1.0f - k->g)), c2)) return c2;
  const float dx = x - k->icx, dy = y - k->icy, d2 = dx * dx + dy * dy;
  const float ir = 95.0f * k->g, pr = 45.0f * k->g;
  if (d2 > ir * ir) return COL_SCLERA;
  const float hx = dx - pr * 0.55f, hy = dy + pr * 0.55f, hr = pr * 0.32f;
  if (hx * hx + hy * hy <= hr * hr) return COL_HILITE;
  if (d2 <= pr * pr) return COL_PUPIL;
  if (d2 >= (ir - 6) * (ir - 6)) return COL_CYC_RING;
  const float a = atan2f(dy, dx);
  return lerp565(COL_CYC_IRIS, COL_CYC_LIGHT, 0.5f + 0.5f * sinf(12.0f * a + sqrtf(d2) * 0.08f));
}
/* Rýchle kreslenie kyklopa: dúhovka (aj so zrenicou a odleskom) sa raz vypočíta
 * do textúry; pri pohybe sa prekreslí len okolie dúhovky a pri žmurknutí len
 * pás riadkov, kde sa posunulo viečko. */
#define CYC_TR 95
#define CYC_TW (2 * CYC_TR + 1)
/* Spoločná pamäť pre veľké predpočítané textúry gagov (kyklop, čarodejnica, radar,
 * drak, plamenné oči). Nikdy nebežia naraz, tak sa o jednu pamäť delia – ušetrí to
 * asi 160 kB RAM. Gag si textúru prepočíta, ak pamäť medzitým použil iný gag. */
#define ARENA_BYTES (CYC_TW * CYC_TW * 2)
static uint16_t gagArena16[ARENA_BYTES / 2];
#define gagArena ((uint8_t *)gagArena16)
enum { AR_NONE = 0, AR_CYC, AR_WITCH, AR_RADAR, AR_DRAGON, AR_FLAME };
static uint8_t gagArenaOwner = AR_NONE;
#define cycTex ((uint16_t *)gagArena16)
static void cycBuildTex(){
  if (gagArenaOwner == AR_CYC) return;
  const float ir = 95.0f, pr = 45.0f;
  for (int j = 0; j < CYC_TW; ++j) for (int i = 0; i < CYC_TW; ++i){
    const float dx = (float)(i - CYC_TR), dy = (float)(j - CYC_TR), d2 = dx * dx + dy * dy;
    uint16_t c = COL_SCLERA;
    if (d2 <= ir * ir){
      const float hx = dx - pr * 0.55f, hy = dy + pr * 0.55f, hr = pr * 0.32f;
      if (hx * hx + hy * hy <= hr * hr) c = COL_HILITE;
      else if (d2 <= pr * pr) c = COL_PUPIL;
      else if (d2 >= (ir - 6) * (ir - 6)) c = COL_CYC_RING;
      else { const float a = atan2f(dy, dx); c = lerp565(COL_CYC_IRIS, COL_CYC_LIGHT, 0.5f + 0.5f * sinf(12.0f * a + sqrtf(d2) * 0.08f)); }
    }
    cycTex[j * CYC_TW + i] = c;
  }
  gagArenaOwner = AR_CYC;
}
static uint16_t shadeCyclopsFast(int x, int y, const void *c){
  const CycCtx *k = (const CycCtx *)c;
  if (y <= k->lidY) return COL_LID_TOP;
  if (k->lidY >= eyeTopY && y <= k->lidY + 3) return COL_LID_SEAM;
  const int i = x - (int)k->icx + CYC_TR, j = y - (int)k->icy + CYC_TR;
  if ((unsigned)i >= (unsigned)CYC_TW || (unsigned)j >= (unsigned)CYC_TW) return COL_SCLERA;
  return cycTex[j * CYC_TW + i];
}
static void activity_gag_cyclops(){
  gagStartCentered();
  cycBuildTex();
  gagBlArm();                                                           // prechody cez podsvietenie
  const unsigned long T_IN = 1000, T_HOLD = 8000, T_OUT = 1000, TOT = T_IN + T_HOLD + T_OUT;
  static const unsigned long kt[] = { 0, 800, 1700, 2600, 3400, 5200, 6200, 7200, 8000 };
  static const float kx[] = { 0, 0, -45, -45, 45, 45, 0, 20, 0 };
  static const float ky[] = { 0, 0, -25, 30, 25, -30, 0, 15, 0 };
  CycCtx K[2], O[2];
  bool first = true;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    (void)gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    float gx = 0, gy = 0;
    if (t > T_IN){
      const unsigned long u = t - T_IN;
      for (int i = 0; i < 8; ++i) if (u >= kt[i] && u < kt[i + 1]){
        const float f = ease((float)(u - kt[i]) / (kt[i + 1] - kt[i]));
        gx = kx[i] + (kx[i + 1] - kx[i]) * f; gy = ky[i] + (ky[i + 1] - ky[i]) * f;
      }
    }
    const float bl = angryBlinkEnv(t, T_IN + 4300, 400);              // spoločné žmurknutie
    for (int e = 0; e < 2; ++e){
      CycCtx &k = K[e];
      k.g = 1.0f;
      k.icx = (float)lroundf(CX + 130.0f + gx - e * 260.0f);         // stred veľkého oka je medzi displejmi
      k.icy = (float)lroundf(CY + gy);
      k.lidY = (int)lroundf((eyeTopY - 1) + (eyeBotY + 1 - (eyeTopY - 1)) * bl);
      Arduino_GFX *gxp = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
      if (first) gagRenderRect(gxp, buf, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeCyclopsFast, &k);
      else {
        const CycCtx &o = O[e];
        if (o.icx != k.icx || o.icy != k.icy){                          // dúhovka (stará aj nová poloha)
          const int x0 = (int)min(o.icx, k.icx) - CYC_TR - 1, x1 = (int)max(o.icx, k.icx) + CYC_TR + 1;
          const int y0 = max((int)min(o.icy, k.icy) - CYC_TR - 1, max(o.lidY, k.lidY) + 4);
          const int y1 = (int)max(o.icy, k.icy) + CYC_TR + 1;
          gagRenderRect(gxp, buf, x0, y0, x1, y1, shadeCyclopsFast, &k);
        }
        if (o.lidY != k.lidY)                                             // viečko: len pás, kde sa posunulo
          gagRenderRect(gxp, buf, CX - EYE_R, min(o.lidY, k.lidY), CX + EYE_R, max(o.lidY, k.lidY) + 4, shadeCyclopsFast, &k);
      }
      O[e] = k;
    }
    first = false;
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}


/* =====================================================================
 *  GAGY 34–36: čarodejnica, radar, sviečka
 * ===================================================================== */
static inline bool triIn(float px, float py, float ax, float ay, float bx, float by, float cx, float cy){
  const float d1 = (px - bx) * (ay - by) - (ax - bx) * (py - by);
  const float d2 = (px - cx) * (by - cy) - (bx - cx) * (py - cy);
  const float d3 = (px - ax) * (cy - ay) - (cx - ax) * (py - ay);
  const bool neg = (d1 < 0) || (d2 < 0) || (d3 < 0), pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
  return !(neg && pos);
}

/* ---------- 34: Čarodejnica na metle ---------- */
// silueta letiaca doprava; dx, dy v px voči stredu postavy, cape = mávanie plášťa
static bool witchPix(float dx, float dy, float cape){
  const float x = dx / 1.3f, y = dy / 1.3f;
  if (x < -85 || x > 56 || y < -60 || y > 32) return false;
  if (segDist2(x, y, -55, 14, 52, -4) <= 4.8f) return true;                       // násada metly
  if (x >= -82 && x <= -55){ const float h = 4 + (-55 - x) * 0.45f; if (fabsf(y - 18.5f + (x + 55) * 0.05f) <= h) return true; }  // prútie
  if (triIn(x, y, -12, 13, 14, 11, 4, -18)) return true;                          // telo
  if (triIn(x, y, -6, -12, -34, 4 + cape, -10, 10)) return true;                  // plášť
  if (segDist2(x, y, 4, 8, 16, 17) <= 6.0f) return true;                          // noha
  if ((x - 6) * (x - 6) + (y + 23) * (y + 23) <= 49) return true;                 // hlava
  if (triIn(x, y, 11, -26, 11, -21, 18, -21)) return true;                        // nos
  if (fabsf(y + 29) <= 1.6f && fabsf(x - 5) <= 14) return true;                   // okraj klobúka
  if (triIn(x, y, -5, -29, 14, -29, -6, -57)) return true;                        // klobúk
  return false;
}
static uint16_t shadeWitch(int x, int y, const void *c){
  const WitchCtx *w = (const WitchCtx *)c;
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, (int)lroundf(PUPIL_R * (1.0f - w->g)), c2)) return c2;
  if (w->g < 0.02f) return COL_SCLERA;
  if (w->vis && witchPix(x - w->wx, y - w->wy, w->cape)) return lerp565(COL_SCLERA, COL_PUPIL, w->g);
  const float mx = x - w->mx, my = y - (CY - 15), m2 = mx * mx + my * my;
  if (m2 <= 70 * 70) return lerp565(COL_SCLERA, COL_MOON_PALE, w->g);
  return lerp565(COL_SCLERA, COL_DUSK, w->g);
}
/* Rýchle kreslenie: silueta čarodejnice je vopred „vypálená“ do bitových masiek
 * (8 polôh plášťa) a pri lete sa posielajú len pixely, ktoré sa naozaj zmenili. */
#define WITCH_MX0 (-112)
#define WITCH_MY0 (-79)
#define WITCH_MW  187
#define WITCH_MH  123
#define WITCH_PH  8
typedef uint8_t WitchMaskT[WITCH_MH][(WITCH_MW + 7) / 8];
#define witchMask ((WitchMaskT *)gagArena)                           // v spoločnej pamäti
static void witchBuildMasks(){
  if (gagArenaOwner == AR_WITCH) return;
  for (int ph = 0; ph < WITCH_PH; ++ph){
    const float cape = -6.0f + 12.0f * ph / (WITCH_PH - 1);
    for (int j = 0; j < WITCH_MH; ++j){
      for (int b = 0; b < (WITCH_MW + 7) / 8; ++b) witchMask[ph][j][b] = 0;
      for (int i = 0; i < WITCH_MW; ++i)
        if (witchPix((float)(i + WITCH_MX0), (float)(j + WITCH_MY0), cape)) witchMask[ph][j][i >> 3] |= (uint8_t)(1 << (i & 7));
    }
  }
  gagArenaOwner = AR_WITCH;
}
static inline bool witchAt(const WitchSt &s, int x, int y){
  if (!s.vis) return false;
  const int i = x - s.wx - WITCH_MX0, j = y - s.wy - WITCH_MY0;
  if ((unsigned)i >= (unsigned)WITCH_MW || (unsigned)j >= (unsigned)WITCH_MH) return false;
  return (witchMask[s.ph][j][i >> 3] >> (i & 7)) & 1;
}
static inline uint16_t witchBg(const WitchSt &s, int x, int y){
  const int mx = x - s.mx, my = y - (CY - 15);
  return (mx * mx + my * my <= 70 * 70) ? COL_MOON_PALE : COL_DUSK;
}
static uint16_t shadeWitchFast(int x, int y, const void *c){
  const WitchSt *s = (const WitchSt *)c;
  return witchAt(*s, x, y) ? COL_PUPIL : witchBg(*s, x, y);
}
// pošle len zmenené pixely (krátke medzery sa zlúčia, aby bolo menej prenosov)
static void witchUpdate(int e, const WitchSt &o, const WitchSt &n){
  if (!o.vis && !n.vis) return;
  Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
  int xa = 10000, xb = -10000, ya = 10000, yb = -10000;
  const WitchSt *st[2] = { &o, &n };
  for (int k = 0; k < 2; ++k) if (st[k]->vis){
    xa = min(xa, st[k]->wx + WITCH_MX0); xb = max(xb, st[k]->wx + WITCH_MX0 + WITCH_MW - 1);
    ya = min(ya, st[k]->wy + WITCH_MY0); yb = max(yb, st[k]->wy + WITCH_MY0 + WITCH_MH - 1);
  }
  if (ya < eyeTopY) ya = eyeTopY;
  if (yb > eyeBotY) yb = eyeBotY;
  for (int y = ya; y <= yb; ++y){
    const int x0 = max(xa, (int)xLeft[y]), x1 = min(xb, (int)xRight[y]);
    int rs = -1, len = 0, lastCh = -1;
    for (int x = x0; x <= x1; ++x){
      const bool ob = witchAt(o, x, y), nb = witchAt(n, x, y);
      if (ob != nb){
        if (rs < 0){ rs = x; len = 0; }
        buf[len++] = nb ? COL_PUPIL : witchBg(n, x, y); lastCh = len;
      } else if (rs >= 0){
        if (len - lastCh >= 6){ gx->draw16bitRGBBitmap(rs, y, buf, lastCh, 1); rs = -1; }
        else buf[len++] = nb ? COL_PUPIL : witchBg(n, x, y);
      }
    }
    if (rs >= 0 && lastCh > 0) gx->draw16bitRGBBitmap(rs, y, buf, lastCh, 1);
  }
}
static void activity_gag_witch(){
  gagStartCentered();
  witchBuildMasks();                                           // raz (asi pol sekundy)
  gagBlArm();
  const unsigned long T_IN = 1000, T_FLY = 6500, T_OUT = 1000, TOT = T_IN + T_FLY + T_OUT;
  WitchSt S[2], O[2];
  bool first = true;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    (void)gagEnvelope(t, T_IN, T_FLY, T_OUT);                 // stmavnutie / rozsvietenie cez podsvietenie
    const float u = clamp01(((float)t - T_IN) / T_FLY);
    const float faceX = -110 + 720 * u;                        // let cez obe oči zľava doprava
    const float cape = 6.0f * sinf(t * 0.02f);
    for (int e = 0; e < 2; ++e){
      WitchSt &w = S[e];
      w.mx = (e == 0) ? CX + 20 : CX - 20;
      w.wx = (int)lroundf(faceX - e * 260.0f);
      w.wy = (int)lroundf(CY - 10 + 18.0f * sinf(u * 6.28f) - 30.0f * u);   // mierne stúpa
      w.ph = (int)lroundf((cape + 6.0f) / 12.0f * (WITCH_PH - 1));
      w.vis = (t > T_IN && w.wx > -130 && w.wx < 330);
      Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
      if (first) gagRenderRect(gx, buf, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeWitchFast, &w);
      else witchUpdate(e, O[e], w);
      O[e] = w;
    }
    first = false;
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 35: Radar – hľadá deti ---------- */
static float rdBx[2][4], rdBy[2][4], rdVx[2][4], rdVy[2][4];
static uint16_t shadeRadar(int x, int y, const void *c){
  const RadarCtx *r = (const RadarCtx *)c;
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, (int)lroundf(PUPIL_R * (1.0f - r->g)), c2)) return c2;
  if (r->g < 0.02f) return COL_SCLERA;
  const float dx = x - CX, dy = y - CY, d = sqrtf(dx * dx + dy * dy);
  uint16_t col = COL_RADAR_BG;
  float diff = r->ang - atan2f(dy, dx);                            // ako dávno prešiel lúč
  diff = fmodf(diff, 6.28318f); if (diff < 0) diff += 6.28318f;
  if (diff < 1.6f) col = lerp565(COL_RADAR_BG, COL_RADAR_TRAIL, 1.0f - diff / 1.6f);
  if (fabsf(d - 35) < 0.8f || fabsf(d - 70) < 0.8f || fabsf(d - 105) < 0.8f || fabsf(dx) < 0.8f || fabsf(dy) < 0.8f) col = COL_RADAR_GRID;
  if (diff < 0.035f && d > 2) col = COL_RADAR_BEAM;
  for (int i = 0; i < 4; ++i){                                     // bodky = „deti“
    const float bx = x - rdBx[r->e][i], by = y - rdBy[r->e][i];
    if (bx * bx + by * by <= 20){
      float bd = r->ang - atan2f(rdBy[r->e][i] - CY, rdBx[r->e][i] - CX);
      bd = fmodf(bd, 6.28318f); if (bd < 0) bd += 6.28318f;
      const bool lock = (r->lock && i == 0);
      col = lerp565(col, lock ? COL_SCAN_HOT : COL_RADAR_BLIP, lock ? 1.0f : 1.0f - bd / 6.28318f);
    }
  }
  if (r->lock){                                                    // zameraný cieľ: blikajúci krúžok
    const float bx = x - rdBx[r->e][0], by = y - rdBy[r->e][0], bd = sqrtf(bx * bx + by * by);
    if (fabsf(bd - 11) < 1.2f) col = COL_SCAN_HOT;
  }
  return lerp565(COL_SCLERA, col, r->g);
}
/* Rýchly radar: uhly sú vopred v tabuľke, stopa lúča má niekoľko stupňov jasu
 * a v každom snímku sa prekreslia len úzke výseky, kde sa niektorý pixel mení
 * (čelo lúča, hranice stupňov stopy, koniec stopy) a okolie bodiek. */
#define RADAR_STEPS 6
#define rdQA ((uint16_t *)gagArena16)             // uhol v jednom kvadrante (0..65535 ~ 0..90°), v spoločnej pamäti
static uint16_t rdTrail[RADAR_STEPS + 1];
static void radarBuildLUT(){
  if (gagArenaOwner != AR_RADAR){
    for (int j = 0; j <= 110; ++j) for (int i = 0; i <= 110; ++i)
      rdQA[j * 111 + i] = (uint16_t)lroundf(atan2f((float)j, (float)i) / 1.5707963f * 65535.0f);
    gagArenaOwner = AR_RADAR;
  }
  for (int k = 0; k <= RADAR_STEPS; ++k) rdTrail[k] = lerp565(COL_RADAR_BG, COL_RADAR_TRAIL, (float)k / RADAR_STEPS);
}
static inline float radarAngle(int dx, int dy){                   // ako atan2f(dy, dx)
  const int ax = min(abs(dx), 110), ay = min(abs(dy), 110);
  const float q = rdQA[ay * 111 + ax] * (1.5707963f / 65535.0f);
  if (dx >= 0) return (dy >= 0) ? q : -q;
  return (dy >= 0) ? 3.14159265f - q : -(3.14159265f - q);
}
static uint16_t shadeRadarFast(int x, int y, const void *c){
  const RadarCtx *r = (const RadarCtx *)c;
  const int dx = x - CX, dy = y - CY, d2 = dx * dx + dy * dy;
  uint16_t col = COL_RADAR_BG;
  float diff = r->ang - radarAngle(dx, dy);                        // ako dávno prešiel lúč
  diff = fmodf(diff, 6.28318f); if (diff < 0) diff += 6.28318f;
  if (diff < 1.6f){ int k = (int)ceilf((1.0f - diff / 1.6f) * RADAR_STEPS); if (k < 0) k = 0; if (k > RADAR_STEPS) k = RADAR_STEPS; col = rdTrail[k]; }
  if ((d2 >= 1170 && d2 <= 1281) || (d2 >= 4789 && d2 <= 5012) || (d2 >= 10858 && d2 <= 11193) || dx == 0 || dy == 0) col = COL_RADAR_GRID;
  if (diff < 0.035f && d2 > 4) col = COL_RADAR_BEAM;
  for (int i = 0; i < 4; ++i){                                     // bodky = „deti“
    const float bx = x - rdBx[r->e][i], by = y - rdBy[r->e][i];
    if (bx * bx + by * by <= 20){
      float bd = r->ang - atan2f(rdBy[r->e][i] - CY, rdBx[r->e][i] - CX);
      bd = fmodf(bd, 6.28318f); if (bd < 0) bd += 6.28318f;
      const bool lock = (r->lock && i == 0);
      col = lerp565(col, lock ? COL_SCAN_HOT : COL_RADAR_BLIP, lock ? 1.0f : 1.0f - bd / 6.28318f);
    }
  }
  if (r->lock){                                                    // zameraný cieľ: blikajúci krúžok
    const float bx = x - rdBx[r->e][0], by = y - rdBy[r->e][0], bd = sqrtf(bx * bx + by * by);
    if (fabsf(bd - 11) < 1.2f) col = COL_SCAN_HOT;
  }
  return col;
}
// x-rozsah riadku y, kde ležia pixely s uhlom medzi th0 a th1 (úzky výsek); false = žiadny
static bool radarSliverRow(int y, float th0, float th1, int &xa, int &xb){
  const int dy = y - CY;
  if (dy == 0){ xa = CX - EYE_R; xb = CX + EYE_R; return true; }
  float xs[2]; bool hit[2];
  const float th[2] = { th0, th1 };
  for (int k = 0; k < 2; ++k){
    const float sn = sinf(th[k]), cs = cosf(th[k]);
    hit[k] = sn * dy > 0.0f;
    xs[k] = hit[k] ? CX + dy * cs / sn : 0.0f;
  }
  if (!hit[0] && !hit[1]) return false;
  if (hit[0] && hit[1]){ xa = (int)floorf(min(xs[0], xs[1])) - 1; xb = (int)ceilf(max(xs[0], xs[1])) + 1; }
  else {
    const float xh = hit[0] ? xs[0] : xs[1];
    if (cosf(0.5f * (th0 + th1)) > 0){ xa = (int)floorf(xh) - 1; xb = CX + EYE_R; }
    else { xa = CX - EYE_R; xb = (int)ceilf(xh) + 1; }
  }
  if (xa < CX - EYE_R) xa = CX - EYE_R;
  if (xb > CX + EYE_R) xb = CX + EYE_R;
  return xa <= xb;
}
static void radarUpdate(int e, const RadarCtx &o, const RadarCtx &n, const float *obx, const float *oby){
  Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
  // hranice, pri ktorých sa mení farba: čelo lúča, koniec lúča, stupne stopy
  float B[RADAR_STEPS + 2]; int nb = 0;
  B[nb++] = 0.0f; B[nb++] = 0.035f;
  for (int k = 1; k <= RADAR_STEPS; ++k) B[nb++] = 1.6f * k / RADAR_STEPS;
  const int BLK = 8;
  for (int y0 = eyeTopY; y0 <= eyeBotY; y0 += BLK){
    const int y1 = min(eyeBotY, y0 + BLK - 1);
    int iv[(RADAR_STEPS + 2) * 2][2], m = 0;
    for (int b = 0; b < nb; ++b){
      const float th0 = o.ang - B[b] - 0.01f, th1 = n.ang - B[b] + 0.01f;
      int xa = 10000, xb = -10000; bool any = false;
      for (int y = y0; y <= y1; ++y){ int a, c; if (radarSliverRow(y, th0, th1, a, c)){ xa = min(xa, a); xb = max(xb, c); any = true; } }
      if (any){ iv[m][0] = xa; iv[m][1] = xb; ++m; }
    }
    for (int i = 1; i < m; ++i) for (int j = i; j > 0 && iv[j][0] < iv[j - 1][0]; --j){
      int t0 = iv[j][0], t1 = iv[j][1]; iv[j][0] = iv[j - 1][0]; iv[j][1] = iv[j - 1][1]; iv[j - 1][0] = t0; iv[j - 1][1] = t1;
    }
    int s0 = 0, s1 = -1; bool open = false;
    for (int i = 0; i < m; ++i){
      if (open && iv[i][0] <= s1 + 6){ if (iv[i][1] > s1) s1 = iv[i][1]; continue; }
      if (open) gagRenderRect(gx, buf, s0, y0, s1, y1, shadeRadarFast, &n);
      s0 = iv[i][0]; s1 = iv[i][1]; open = true;
    }
    if (open) gagRenderRect(gx, buf, s0, y0, s1, y1, shadeRadarFast, &n);
  }
  // bodky (stará aj nová poloha) a krúžok zameraného cieľa
  for (int i = 0; i < 4; ++i){
    const int r = (i == 0 && (n.lock || o.lock)) ? 14 : 6;
    gagRenderRect(gx, buf, (int)min(obx[i], rdBx[e][i]) - r, (int)min(oby[i], rdBy[e][i]) - r,
                  (int)max(obx[i], rdBx[e][i]) + r, (int)max(oby[i], rdBy[e][i]) + r, shadeRadarFast, &n);
  }
}
static void activity_gag_radar(){
  gagStartCentered();
  radarBuildLUT();
  gagBlArm();
  for (int e = 0; e < 2; ++e) for (int i = 0; i < 4; ++i){
    const float a = (rand() % 628) * 0.01f, rr = 30 + rand() % 65;
    rdBx[e][i] = CX + rr * cosf(a); rdBy[e][i] = CY + rr * sinf(a);
    rdVx[e][i] = ((rand() % 21) - 10) * 0.0012f; rdVy[e][i] = ((rand() % 21) - 10) * 0.0012f;
  }
  const unsigned long T_IN = 900, T_HOLD = 8000, T_OUT = 900, TOT = T_IN + T_HOLD + T_OUT;
  RadarCtx R[2], O[2];
  float obx[2][4], oby[2][4];
  bool first = true;
  unsigned long lastT = 0;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    (void)gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    const float dt = (float)(t - lastT); lastT = t;
    for (int e = 0; e < 2; ++e){
      for (int i = 0; i < 4; ++i){                                 // deti sa pomaly pohybujú
        obx[e][i] = rdBx[e][i]; oby[e][i] = rdBy[e][i];
        rdBx[e][i] += rdVx[e][i] * dt; rdBy[e][i] += rdVy[e][i] * dt;
        const float ddx = rdBx[e][i] - CX, ddy = rdBy[e][i] - CY;
        if (ddx * ddx + ddy * ddy > 98 * 98){ rdVx[e][i] = -rdVx[e][i]; rdVy[e][i] = -rdVy[e][i]; }
      }
      RadarCtx &r = R[e];
      r.e = e; r.g = 1.0f;
      r.ang = t * 0.0025f + (e ? 3.14159f : 0.0f);
      r.lock = (t > T_IN + 5500 && t < T_IN + T_HOLD && ((t / 200) & 1));
      if (first) gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeRadarFast, &r);
      else if (r.ang - O[e].ang > 0.5f)                                // dlhá pauza: radšej celé oko
        gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeRadarFast, &r);
      else radarUpdate(e, O[e], r, obx[e], oby[e]);
      O[e] = r;
    }
    first = false;
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 36: Sviečka namiesto zrenice ---------- */
static uint16_t shadeCandle(int x, int y, const void *c){
  const CandleCtx *k = (const CandleCtx *)c;
  uint16_t c2; if (gagPupilPix(x, y, CX, CY, (int)lroundf(PUPIL_R * (1.0f - k->g)), c2)) return c2;
  if (k->g < 0.02f) return COL_SCLERA;
  const float dx = x - k->fx, base = CY - 2, ny = (base - y) / k->fh;          // 0 dole, 1 špička
  if (ny >= 0 && ny <= 1){                                                     // plamienok
    const float w = 12.0f * sinf(3.14159f * sqrtf(ny)) * (1.0f - 0.25f * ny);
    const float sx = dx - k->sway * ny * ny;
    if (fabsf(sx) <= w){
      if (ny < 0.6f && fabsf(sx) <= w * 0.45f) return lerp565(COL_SCLERA, COL_FLAME_CORE, k->g);
      return lerp565(COL_SCLERA, COL_FLAME, k->g);
    }
  }
  if (abs(x - CX) <= 1 && y > base && y <= CY + 5) return lerp565(COL_SCLERA, COL_PUPIL, k->g);   // knôt
  if (abs(x - CX) <= 14 && y > CY + 5){                                       // sviečka
    return lerp565(COL_SCLERA, (abs(x - CX) > 11) ? COL_WAX_D : COL_WAX, k->g);
  }
  const float hx = x - CX, hy = y - (CY - 25), hd = sqrtf(hx * hx + hy * hy);   // žiara
  const float glow = clamp01(1.0f - hd / 75.0f) * k->glow;
  return lerp565(COL_SCLERA, lerp565(COL_CANDLE_BG, COL_CANDLE_GLOW, glow), k->g);
}
static void activity_gag_candle(){
  gagStartCentered();
  gagBlArm();
  const unsigned long T_IN = 1000, T_HOLD = 8000, T_OUT = 1000, TOT = T_IN + T_HOLD + T_OUT;
  CandleCtx K[2];
  bool fullAtOne = false;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    const float g = gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    for (int e = 0; e < 2; ++e){
      CandleCtx &k = K[e];
      const float ph = e ? 1.7f : 0.0f;
      k.g = g; k.fx = CX + 1.5f * sinf(t * 0.013f + ph);
      k.fh = (44.0f + 5.0f * sinf(t * 0.021f + ph) + 3.0f * sinf(t * 0.057f + ph * 2)) * g + 1.0f;
      k.sway = 4.0f * sinf(t * 0.009f + ph);
      k.glow = 0.8f + 0.2f * sinf(t * 0.017f + ph) * sinf(t * 0.031f);
    }
    const bool full = (g < 1.0f) || last || !fullAtOne;              // inak len oblasť plamienka a žiary
    if (g >= 1.0f) fullAtOne = true;
    for (int e = 0; e < 2; ++e){
      Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
      if (full) gagRenderRect(gx, buf, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeCandle, &K[e]);
      else      gagRenderRect(gx, buf, CX - 76, CY - 101, CX + 76, CY + 51, shadeCandle, &K[e]);
    }
    if (last) break;
    delay(full ? 2 : 10);
  }
  gagEndNormal();
}


/* =====================================================================
 *  GAG 37: Chápadlá vyliezajú zdola
 * ===================================================================== */
#define TENT_N   3
#define TENT_PTS 20
static uint16_t shadeTentacles(int x, int y, const void *c){
  const TentCtx *k = (const TentCtx *)c;
  bool edge = false;
  for (int i = 0; i < TENT_N; ++i){
    if (x < k->bx0[i] || x > k->bx1[i] || y < k->by0[i]) continue;
    for (int j = TENT_PTS - 1; j >= 0; --j){                    // od špičky k základni
      const float dx = x - k->px[i][j], dy = y - k->py[i][j], r = k->pr[i][j];
      const float d2 = dx * dx + dy * dy;
      if (d2 > r * r) continue;
      if (d2 > (r - 2) * (r - 2)){ edge = true; continue; }
      if ((j % 3) == 1 && j < TENT_PTS - 3){                     // prísavky
        const float sx = dx - k->side[i] * r * 0.45f, sy = dy;
        if (sx * sx + sy * sy <= r * r * 0.12f) return COL_TENT_SUCK;
      }
      return COL_TENT;
    }
  }
  if (edge) return COL_TENT_EDGE;
  uint16_t c2; if (gagPupilPix(x, y, k->pcx, k->pcy, PUPIL_R, c2)) return c2;
  return COL_SCLERA;
}
static void activity_gag_tentacles(){
  gagStartCentered();
  const unsigned long TOT = 9000;
  static const float bxs[TENT_N] = { 62, 122, 182 };
  TentCtx T[2];
  float px[2] = { CX, CX }, py[2] = { CY, CY };
  int prevTop[2] = { CY - 40, CY - 40 };
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    for (int e = 0; e < 2; ++e){
      TentCtx &k = T[e];
      int top = eyeBotY;
      float avgTipX = 0, minTipY = 1000;
      for (int i = 0; i < TENT_N; ++i){
        float ext = ease(clamp01(((float)t - i * 400) / 1500.0f));           // vyliezanie
        if (t > 7000) ext *= 1.0f - ease(clamp01((t - 7000 - i * 200) / 1400.0f));   // stiahnutie
        const float L = 135.0f * ext, ph = i * 2.1f + e * 1.3f;
        const float baseX = bxs[i] + (e ? 6 : -6);
        k.side[i] = (i % 2) ? 1.0f : -1.0f;
        float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f;
        for (int j = 0; j < TENT_PTS; ++j){
          const float s = (float)j / (TENT_PTS - 1);
          const float yy = eyeBotY + 14 - s * L;
          const float xx = baseX + 26.0f * s * sinf(ph + s * 3.0f + t * 0.003f) + (s > 0.75f ? 18.0f * (s - 0.75f) * 4 * k.side[i] * sinf(t * 0.004f + ph) : 0.0f);
          const float rr = (15.0f * (1.0f - s * 0.85f) + 2.0f) * (0.4f + 0.6f * ext);
          k.px[i][j] = xx; k.py[i][j] = yy; k.pr[i][j] = rr;
          x0 = fminf(x0, xx - rr); x1 = fmaxf(x1, xx + rr); y0 = fminf(y0, yy - rr);
        }
        k.bx0[i] = (int)x0 - 1; k.bx1[i] = (int)x1 + 1; k.by0[i] = (int)y0 - 1;
        top = min(top, k.by0[i]);
        avgTipX += k.px[i][TENT_PTS - 1]; minTipY = fminf(minTipY, k.py[i][TENT_PTS - 1]);
      }
      // zrenice sa pozerajú na chápadlá, keď sú vysoko, uhýbajú hore
      float tx = CX, ty = CY;
      if (t > 600 && t < 7400){
        tx = CX + constrain((avgTipX / TENT_N - CX) * 0.4f + 18.0f * sinf(t * 0.002f + e), -35.0f, 35.0f);
        ty = (minTipY < CY + 10) ? CY - 30 : CY + 22;
      }
      px[e] += (tx - px[e]) * 0.15f; py[e] += (ty - py[e]) * 0.15f;
      if (last){ px[e] = CX; py[e] = CY; }
      k.pcx = (int)lroundf(px[e]); k.pcy = (int)lroundf(py[e]);
      const int y0 = min(min(top, k.pcy - PUPIL_R - 2), prevTop[e]);
      gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, y0, CX + EYE_R, eyeBotY, shadeTentacles, &k);
      prevTop[e] = min(top, k.pcy - PUPIL_R - 2);
    }
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}


/* =====================================================================
 *  GAGY 38–40: upír, vlkolak, kľúčová dierka
 * ===================================================================== */
static inline bool dropPix(float dx, float dy, float r){           // kvapka so špičkou hore
  if (dx * dx + dy * dy <= r * r) return true;
  return dy < 0 && dy > -2.6f * r && fabsf(dx) * 2.6f * r < (dy + 2.6f * r) * r;
}

/* ---------- 38: Upír – červené oči, netopierie krídla, kvapka ---------- */
static uint16_t shadeVampire(int x, int y, const void *c){
  const VampCtx *v = (const VampCtx *)c;
  if (v->g > 0.01f){
    const float lid = v->lidY + 14.0f * fabsf(sinf((x - CX) * 3.14159f / 48.0f));   // vrúbkovaný okraj krídla
    if (y <= lid) return ((int)(lid - y) < 2) ? COL_VAMP_EDGE : COL_VAMP_WING;
    if (v->dropY > 0 && dropPix(x - v->dropX, y - v->dropY, 5.0f)) return COL_BLOOD;
  }
  if (v->g < 0.05f){ uint16_t c2; if (gagPupilPix(x, y, CX, CY, PUPIL_R, c2)) return c2; return COL_SCLERA; }
  const float dx = x - v->icx, dy = y - CY, d2 = dx * dx + dy * dy;
  const float w = PUPIL_R + (7 - PUPIL_R) * v->g, h = PUPIL_R + (34 - PUPIL_R) * v->g;
  const float hx = dx - 12, hy = dy + 14;
  if (hx * hx + hy * hy <= 25) return COL_HILITE;
  if ((dx * dx) / (w * w) + (dy * dy) / (h * h) <= 1.0f) return COL_PUPIL;
  const float ir = 46.0f * v->g;
  if (d2 <= ir * ir) return (d2 >= (ir - 5) * (ir - 5)) ? COL_VAMP_RING : COL_VAMP_IRIS;
  return COL_SCLERA;
}
static void activity_gag_vampire(){
  gagStartCentered();
  gagBlArm();
  const unsigned long T_IN = 1200, T_HOLD = 6600, T_OUT = 1200, TOT = T_IN + T_HOLD + T_OUT;
  VampCtx V[2];
  bool first = true; float oix[2] = { CX, CX }, ody[2] = { 0, 0 };
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    const float g = gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    const float look = (t > T_IN && t < T_IN + T_HOLD) ? 14.0f * sinf((t - T_IN) * 0.0012f) : 0.0f;
    for (int e = 0; e < 2; ++e){
      VampCtx &v = V[e];
      v.g = g; v.icx = CX + look * g;
      v.lidY = (eyeTopY - 30) + ((CY - 55) - (eyeTopY - 30)) * g;
      const unsigned long ds = e ? 3600 : 2400;                         // kvapka krvi
      v.dropY = 0; v.dropX = CX + (e ? -1 : 1) * 30;
      if (t > ds && t < ds + 2200){
        const float f = (t - ds) / 2200.0f;
        v.dropY = CY + 40 + (eyeBotY + 15 - (CY + 40)) * f * f;
      }
    }
    for (int e = 0; e < 2; ++e){
      Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
      const VampCtx &v = V[e];
      if (first) gagRenderRect(gx, buf, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeVampire, &v);
      else {
        // dúhovka (stará aj nová poloha)
        gagRenderRect(gx, buf, (int)floorf(min(oix[e], v.icx)) - 48, CY - 48, (int)ceilf(max(oix[e], v.icx)) + 48, CY + 48, shadeVampire, &v);
        // kvapka krvi (stará aj nová poloha)
        if (v.dropY > 0 || ody[e] > 0){
          const float ya = (ody[e] > 0 && v.dropY > 0) ? min(ody[e], v.dropY) : (v.dropY > 0 ? v.dropY : ody[e]);
          const float yb = max(ody[e], v.dropY);
          gagRenderRect(gx, buf, (int)v.dropX - 8, (int)ya - 16, (int)v.dropX + 8, (int)yb + 8, shadeVampire, &v);
        }
      }
      oix[e] = v.icx; ody[e] = v.dropY;
    }
    first = false;
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 39: Vlkolak – spln, srsť, štrbina ---------- */
static uint16_t shadeWolf(int x, int y, const void *c){
  const WolfCtx *w = (const WolfCtx *)c;
  if (w->g < 0.03f){ uint16_t c2; if (gagPupilPix(x, y, CX, CY, PUPIL_R, c2)) return c2; return COL_SCLERA; }
  const float dx = x - w->cx, dy = y - w->cy, d2 = dx * dx + dy * dy;
  const float rx = x - CX, ry = y - CY, rr = sqrtf(rx * rx + ry * ry);
  if (rr > 80){                                                          // srsť po okraji
    const float th = atan2f(ry, rx);
    float f = th * 26.0f / 6.28318f; f -= floorf(f);
    const float tri = fabsf(f - 0.5f) * 2.0f;
    const float furR = EYE_R - w->g * (8.0f + 18.0f * (1.0f - tri)) * (0.8f + 0.2f * sinf(7 * th));
    if (rr >= furR) return (rr - furR < 3) ? COL_FUR_TIP : COL_FUR;
  }
  const float wd = PUPIL_R + (w->slit - PUPIL_R) * w->g, ht = PUPIL_R + (40 - PUPIL_R) * w->g;
  if ((dx * dx) / (wd * wd) + (dy * dy) / (ht * ht) <= 1.0f) return COL_PUPIL;
  const float hx = dx - 14, hy = dy + 16;
  if (hx * hx + hy * hy <= 20) return COL_HILITE;
  const float ir = 46.0f * w->g;
  if (d2 <= ir * ir) return (d2 >= (ir - 4) * (ir - 4)) ? COL_WOLF_RING : COL_WOLF_IRIS;
  return lerp565(COL_SCLERA, COL_WOLF_MOON, w->g);
}
static void activity_gag_werewolf(){
  gagStartCentered();
  gagBlArm();
  const unsigned long T_IN = 1400, T_HOLD = 6500, T_OUT = 1200, TOT = T_IN + T_HOLD + T_OUT;
  WolfCtx WF[2];
  bool first = true; float ocx[2] = { CX, CX }, ocy[2] = { CY, CY };
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    const float g = gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    const bool growl = (t > T_IN + 3000 && t < T_IN + 3700);              // zavrčanie: oči sa zatrasú
    for (int e = 0; e < 2; ++e){
      WolfCtx &w = WF[e];
      w.g = g;
      w.slit = 8.0f + 4.0f * sinf(t * 0.004f + e);
      w.cx = CX + (growl ? (float)(rand() % 7 - 3) : 0.0f) + 10.0f * g * sinf(t * 0.0009f);
      w.cy = CY + (growl ? (float)(rand() % 7 - 3) : 0.0f);
    }
    for (int e = 0; e < 2; ++e){
      Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
      if (first) gagRenderRect(gx, buf, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeWolf, &WF[e]);
      else {                                                              // len okolie dúhovky (stará aj nová poloha)
        const int x0 = (int)floorf(min(ocx[e], WF[e].cx)) - 48, x1 = (int)ceilf(max(ocx[e], WF[e].cx)) + 48;
        const int y0 = (int)floorf(min(ocy[e], WF[e].cy)) - 48, y1 = (int)ceilf(max(ocy[e], WF[e].cy)) + 48;
        gagRenderRect(gx, buf, x0, y0, x1, y1, shadeWolf, &WF[e]);
      }
      ocx[e] = WF[e].cx; ocy[e] = WF[e].cy;
    }
    first = false;
    if (last) break;
    delay(2);
  }
  gagEndNormal();
}

/* ---------- 40: Kľúčová dierka ---------- */
static inline bool keyholePix(float dx, float dy, float s){
  if (s < 0.02f) return false;
  const float x = dx / s, y = dy / s;
  if (x * x + (y + 25) * (y + 25) <= 34 * 34) return true;
  if (y >= -5 && y <= 75){ const float hw = 18 + (y + 5) * 12.0f / 80.0f; if (fabsf(x) <= hw) return true; }
  return false;
}
static uint16_t shadeKeyhole(int x, int y, const void *c){
  const KeyCtx *k = (const KeyCtx *)c;
  const float dx = x - CX, dy = y - CY;
  if (k->g >= 0.02f && !keyholePix(dx, dy, k->g)) return lerp565(COL_SCLERA, COL_PUPIL, k->g);   // tma okolo
  const int pr = (int)lroundf(PUPIL_R + (18 - PUPIL_R) * k->g);
  uint16_t c2; if (gagPupilPix(x, y, k->pcx, k->pcy, pr, c2)) return c2;
  return lerp565(COL_SCLERA, COL_KEY_LIGHT, k->g);
}
static void activity_gag_keyhole(){
  gagStartCentered();
  gagBlArm();
  const unsigned long T_IN = 900, T_HOLD = 7000, T_OUT = 900, TOT = T_IN + T_HOLD + T_OUT;
  static const unsigned long kt[] = { 0, 600, 1500, 2400, 3300, 4200, 5000, 5800, 7000 };
  static const int8_t kx[] = { 0, 0, -14, -14, 14, 14, 0, 0, 0 };
  static const int8_t ky[] = { -25, -25, -30, -15, -30, 30, 50, -25, -25 };
  KeyCtx K[2];
  bool fullAtOne = false;
  const unsigned long t0 = millis();
  for (;;){
    unsigned long t = millis() - t0; const bool last = (t >= TOT); if (last) t = TOT;
    const float g = gagEnvelope(t, T_IN, T_HOLD, T_OUT);
    float ox = 0, oy = -25;
    if (t > T_IN){
      const unsigned long u = t - T_IN;
      for (int i = 0; i < 8; ++i) if (u >= kt[i] && u < kt[i + 1]){
        const float f = ease((float)(u - kt[i]) / (kt[i + 1] - kt[i]));
        ox = kx[i] + (kx[i + 1] - kx[i]) * f; oy = ky[i] + (ky[i + 1] - ky[i]) * f;
      }
    }
    for (int e = 0; e < 2; ++e){
      K[e].g = g;
      K[e].pcx = (int)lroundf(CX + ox * g); K[e].pcy = (int)lroundf(CY + oy * g);
    }
    const bool full = (g < 1.0f) || last || !fullAtOne;                 // inak len oblasť dierky
    if (g >= 1.0f) fullAtOne = true;
    for (int e = 0; e < 2; ++e){
      Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
      if (full) gagRenderRect(gx, buf, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeKeyhole, &K[e]);
      else      gagRenderRect(gx, buf, CX - 36, CY - 61, CX + 36, CY + 77, shadeKeyhole, &K[e]);
    }
    if (last) break;
    delay(full ? 2 : 6);
  }
  gagEndNormal();
}


/* =====================================================================
 *  GAG 41: Papuľa – oko chňapne
 * ===================================================================== */
static uint16_t shadeJaws(int x, int y, const void *c){
  const JawCtx *j = (const JawCtx *)c;
  if (j->g < 0.02f){ uint16_t c2; if (gagPupilPix(x, y, CX, CY, PUPIL_R, c2)) return c2; return COL_SCLERA; }
  const float dy = y - CY, h = j->h;
  if (h < 1.0f){                                                           // zatvorená: len čiara pier
    if (fabsf(dy) <= 1.5f) return lerp565(COL_SCLERA, COL_LIP, j->g);
    return lerp565(COL_SCLERA, COL_PUMPKIN, j->g);
  }
  const float ph1 = fmodf((float)(x + 200 - 10), 26.0f) / 26.0f;           // horné zuby
  const float ph2 = fmodf((float)(x + 200 + 3), 26.0f) / 26.0f;            // dolné zuby (posunuté)
  const float tl1 = 22.0f * (1.0f - fabsf(2 * ph1 - 1)), tl2 = 22.0f * (1.0f - fabsf(2 * ph2 - 1));
  if (dy < -h){                                                            // horná pera
    if (dy > -h - 3 && h > 0.5f) return COL_LIP;
    return lerp565(COL_SCLERA, COL_PUMPKIN, j->g);
  }
  if (dy > h){                                                             // dolná pera
    if (dy < h + 3 && h > 0.5f) return COL_LIP;
    return lerp565(COL_SCLERA, COL_PUMPKIN, j->g);
  }
  if (dy <= -h + tl1) return COL_SCLERA;                                  // zuby
  if (dy >= h - tl2) return COL_SCLERA;
  const float tx = x - CX, ty = dy - h * 0.6f;                             // jazyk
  if (h > 20 && (tx * tx) / 1600.0f + (ty * ty) / (h * h * 0.16f) <= 1.0f) return COL_TONGUE;
  return COL_MOUTH;
}
static void activity_gag_jaws(){
  gagStartCentered();
  JawCtx J[2];
  // 1) oko sa zmení na zatvorenú papuľu (výmena v tme cez podsvietenie)
  gagBlOut();
  for (int e = 0; e < 2; ++e){ J[e].g = 1.0f; J[e].h = 0; gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeJaws, &J[e]); }
  gagBlIn();
  // 2) chňapnutia: rôzne veľké, posledné len jedným okom
  static const uint8_t openH[]  = { 36, 22, 40, 30, 44 };
  static const uint8_t who[]    = {  3,  3,  1,  2,  3 };
  for (int k = 0; k < 5; ++k){
    const int steps = 10;
    for (int s = 0; s <= 2 * steps; ++s){
      const float f = (s <= steps) ? ease((float)s / steps) : 1.0f - ease((float)(s - steps) / steps);
      for (int e = 0; e < 2; ++e){
        J[e].h = (who[k] & (1 << e)) ? openH[k] * f : 0.0f;
        gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, CY - 72, CX + EYE_R, CY + 72, shadeJaws, &J[e]);
      }
      if (s == steps) delay(250);                                          // chvíľu nechá otvorené
    }
    delay(350);
  }
  // 3) späť na oko (cez podsvietenie)
  gagEndNormal();
}

/* =====================================================================
 *  GAG 42: Príklad v očiach – deti počítajú
 *  Ľavé oko (z pohľadu divákov) napíše príklad, pravé „=?“ a okolo neho
 *  ubúda oranžový kruh (čas na premýšľanie). Posledné 3 sekundy sa
 *  odpočítava 3-2-1, potom pravé oko ukáže výsledok.
 *  Písmo je bodkové (ako svetelná tabuľa), všetko sa kreslí cez shader
 *  bez mazania, takže nič nepreblikáva.
 * ===================================================================== */
#define QUIZ_THINK_MS  9000   // čas na premýšľanie (ms)
#define QUIZ_SHOW_MS   4000   // ako dlho svieti výsledok (ms)
#define QUIZ_ROUNDS    1      // koľko príkladov za jedno spustenie gagu
#define QUIZ_EASY      0      // 1 = len sčítanie a odčítanie do 20 (pre prvákov)

// bodkové písmo 5×7: 0-9, + - × : = ?
static const uint8_t quizFont[16][7] = {
  {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
  {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},
  {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
  {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
  {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
  {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}, {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
  {0x00,0x11,0x0A,0x04,0x0A,0x11,0x00}, {0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00},
  {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00}, {0x0E,0x11,0x01,0x02,0x04,0x00,0x04}
};
static inline int quizGlyph(char c){
  if (c >= '0' && c <= '9') return c - '0';
  switch (c){ case '+': return 10; case '-': return 11; case 'x': return 12;
              case ':': return 13; case '=': return 14; case '?': return 15; }
  return -1;
}
// nastaví text a vypočíta veľkosť bodky a polohu (vycentrované)
static void quizSetText(QuizCtx &q, const char *s, int maxSc, int maxW){
  int n = 0; while (s[n] && n < 7){ q.txt[n] = s[n]; ++n; } q.txt[n] = 0;
  q.len = n;
  int sc = maxSc;
  while (sc > 2 && n * 6 * sc - sc > maxW) --sc;
  q.sc = sc;
  q.tx0 = CX - (n * 6 * sc - sc) / 2;
  q.ty0 = CY - (7 * sc) / 2;
}
// je na (x,y) svietiaca bodka textu?
static inline bool quizDot(const QuizCtx *q, int x, int y){
  const int sc = q->sc, lx = x - q->tx0, ly = y - q->ty0;
  if (lx < 0 || ly < 0 || ly >= 7 * sc) return false;
  const int ci = lx / (6 * sc);
  if (ci >= q->shown) return false;
  const int col = (lx - ci * 6 * sc) / sc;
  if (col >= 5) return false;
  const int gi = quizGlyph(q->txt[ci]); if (gi < 0) return false;
  const int row = ly / sc;
  if (!((quizFont[gi][row] >> (4 - col)) & 1)) return false;
  const float h = (sc - 1) * 0.5f;                                   // okrúhla bodka
  const float dx = (lx % sc) - h, dy = (ly % sc) - h, r = sc * 0.56f;
  return dx * dx + dy * dy <= r * r;
}
static uint16_t shadeQuiz(int x, int y, const void *c){
  const QuizCtx *q = (const QuizCtx *)c;
  if (q->g < 0.02f){ uint16_t c2; if (gagPupilPix(x, y, CX, CY, PUPIL_R, c2)) return c2; return COL_SCLERA; }
  if (q->ring >= 0.0f){                                              // časový kruh po okraji
    const int dx = x - CX, dy = y - CY, r2 = dx * dx + dy * dy;
    if (r2 >= (EYE_R - 13) * (EYE_R - 13) && r2 <= (EYE_R - 4) * (EYE_R - 4)){
      float a = atan2f((float)dx, (float)-dy); if (a < 0) a += 6.2831853f;
      if (a / 6.2831853f <= q->ring) return q->ringCol;
      return COL_QUIZ_TRACK;
    }
  }
  if (q->shown > 0){
    if (quizDot(q, x, y)) return q->col;
    if (quizDot(q, x - 3, y - 3)) return COL_QUIZ_SHADOW;           // tieň
  }
  return lerp565(COL_SCLERA, COL_QUIZ_BG, q->g);
}
static inline void quizDraw(int e, const QuizCtx &q){
  gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL,
                CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeQuiz, &q);
}
// vymyslí príklad, vráti výsledok
static int quizMake(char *s){
  int a, b, r; char op;
  int typ = QUIZ_EASY ? (int)random(2) : (int)random(4);
  if (typ == 0){ if (QUIZ_EASY){ a = random(1, 11); b = random(1, 21 - a); } else { a = random(11, 60); b = random(5, 100 - a); } r = a + b; op = '+'; }
  else if (typ == 1){ if (QUIZ_EASY){ a = random(5, 21); b = random(1, a); } else { a = random(20, 100); b = random(5, a); } r = a - b; op = '-'; }
  else if (typ == 2){ a = random(2, 10); b = random(2, 10); r = a * b; op = 'x'; }
  else { b = random(2, 10); r = random(2, 10); a = b * r; op = ':'; }
  snprintf(s, 8, "%d%c%d", a, op, b);
  return r;
}
static void activity_gag_quiz(){
  gagStartCentered();
  QuizCtx Q[2];
  for (int e = 0; e < 2; ++e){ Q[e].g = 0; Q[e].shown = 0; Q[e].ring = -1; Q[e].col = COL_QUIZ_TXT; Q[e].ringCol = COL_QUIZ_RING; quizSetText(Q[e], "", 9, 176); }
  // 1) oči sa zmenia na tmavé tabule (výmena v tme cez podsvietenie)
  gagBlOut();
  for (int e = 0; e < 2; ++e){ Q[e].g = 1.0f; quizDraw(e, Q[e]); }
  gagBlIn();
  for (int round = 0; round < QUIZ_ROUNDS; ++round){
    char task[8], res[8];
    const int ans = quizMake(task);
    snprintf(res, sizeof(res), "=%d", ans);
    // 2) ľavé oko píše príklad znak po znaku
    quizSetText(Q[0], task, 9, 176); Q[0].col = COL_QUIZ_TXT; Q[0].shown = 0;
    for (int k = 1; k <= Q[0].len; ++k){ Q[0].shown = k; quizDraw(0, Q[0]); delay(220); }
    // 3) pravé oko: „=?“ a ubúdajúci kruh, na konci 3-2-1
    quizSetText(Q[1], "=?", 9, 150); Q[1].shown = Q[1].len; Q[1].col = COL_QUIZ_ASK;
    unsigned long t0 = millis(); long lastKey = -1;
    for (;;){
      const unsigned long el = millis() - t0;
      if (el >= QUIZ_THINK_MS) break;
      const float rem = 1.0f - (float)el / QUIZ_THINK_MS;
      const int secLeft = (int)((QUIZ_THINK_MS - el + 999) / 1000);
      const bool hurry = secLeft <= 3;
      const bool blinkOn = !hurry || ((el / 250) & 1) == 0;
      const long key = (long)(rem * 72) * 100 + secLeft * 2 + (blinkOn ? 1 : 0);
      if (key != lastKey){
        lastKey = key;
        if (hurry){ char d[2] = { (char)('0' + secLeft), 0 }; quizSetText(Q[1], d, 9, 150); Q[1].shown = 1; Q[1].col = COL_QUIZ_HURRY; }
        Q[1].ring = rem;
        Q[1].ringCol = hurry ? (blinkOn ? COL_QUIZ_HURRY : COL_QUIZ_TRACK) : COL_QUIZ_RING;
        quizDraw(1, Q[1]);
      }
      delay(10);
    }
    // 4) výsledok „vyskočí“ a zabliká – ruky hore a svetlá blikajú (druhé Pico)
    Serial1.println("G:CHEER"); Serial1.println("L:BLINK");   // „hej-hou“
    Q[1].ring = -1;
    for (int sc = 3; sc <= 9; ++sc){
      quizSetText(Q[1], res, sc, 176); Q[1].shown = Q[1].len; Q[1].col = COL_QUIZ_ANS;
      quizDraw(1, Q[1]);
    }
    for (int k = 0; k < 6; ++k){
      Q[1].col = (k & 1) ? COL_QUIZ_ANS : COL_QUIZ_TXT; quizDraw(1, Q[1]); delay(220);
    }
    Q[1].col = COL_QUIZ_ANS; quizDraw(1, Q[1]);
    delay(QUIZ_SHOW_MS);
    Serial1.println("G:NONE"); Serial1.println("L:SUN");     // ruky dole, svetlo späť
    // pred ďalším kolom zmaže texty (ostane tabuľa)
    for (int e = 0; e < 2; ++e){ Q[e].shown = 0; Q[e].ring = -1; quizDraw(e, Q[e]); }
    if (round + 1 < QUIZ_ROUNDS) delay(600);
  }
  // 5) späť na oko (cez podsvietenie)
  gagEndNormal();
}

/* =====================================================================
 *  GAG 43: Koľko je hodín? – ručičkové hodiny v oku
 *  Ľavé oko sa zmení na ciferník, ručičky sa rýchlo roztočia na čas,
 *  pravé oko ukáže „?“ a odpočet (ako pri gagu 42), potom digitálnu
 *  odpoveď. Červená sekundová ručička tiká.
 * ===================================================================== */
#define CLOCKQ_REAL     1      // 1 = skutočný čas z DS3231, 0 = náhodný čas (po 5 minútach)
#define CLOCKQ_THINK_MS 9000   // čas na premýšľanie (ms)

// bodka bodkového písma pre jeden znak (lx, ly = poloha vo vnútri znaku)
static inline bool quizCharDot(int gi, int lx, int ly, int sc){
  if (gi < 0 || lx < 0 || ly < 0 || lx >= 5 * sc || ly >= 7 * sc) return false;
  const int col = lx / sc, row = ly / sc;
  if (!((quizFont[gi][row] >> (4 - col)) & 1)) return false;
  const float h = (sc - 1) * 0.5f, dx = (lx % sc) - h, dy = (ly % sc) - h, r = sc * 0.56f;
  return dx * dx + dy * dy <= r * r;
}
// je bod (x,y) na ručičke s uhlom a (0 = hore, v smere hodín), dĺžkou len a hrúbkou w?
static inline bool clockHand(float x, float y, float a, float len, float w){
  const float sx = sinf(a), sy = -cosf(a);
  const float t = x * sx + y * sy;                 // pozdĺž ručičky
  if (t < -10.0f || t > len) return false;
  const float d = fabsf(x * sy - y * sx);           // kolmo na ručičku
  const float ww = (t > len - 10.0f) ? w * 0.5f * (len - t) / 10.0f + 0.6f : w * 0.5f;   // zahrotený koniec
  return d <= ww;
}
static uint16_t shadeClock(int x, int y, const void *c){
  const ClockCtx *k = (const ClockCtx *)c;
  if (k->g < 0.02f){ uint16_t c2; if (gagPupilPix(x, y, CX, CY, PUPIL_R, c2)) return c2; return COL_SCLERA; }
  const uint16_t bg = lerp565(COL_SCLERA, COL_QUIZ_BG, k->g);
  const int dx = x - CX, dy = y - CY, r2 = dx * dx + dy * dy;
  const float R = 92.0f * k->g;                                     // ciferník pri prechode rastie
  if (r2 > R * R) return bg;
  if (r2 > (R - 7) * (R - 7)) return COL_PUMPKIN;                  // oranžový okraj
  if (k->g < 1.0f) return COL_CLK_FACE;
  // ručičky (navrchu)
  if (k->secOn && clockHand(dx, dy, k->secA, 76, 2.5f)) return COL_CLK_SEC;
  if (r2 <= 36) return (r2 <= 6) ? COL_CLK_SEC : COL_CLK_HAND;     // stredový čap
  if (clockHand(dx, dy, k->minA, 70, 7)) return COL_CLK_HAND;
  if (clockHand(dx, dy, k->hourA, 46, 11)) return COL_CLK_HAND;
  // čísla 12, 3, 6, 9
  { const int sc = 3;
    if (quizCharDot(1, x - (CX - 17), y - (CY - 64), sc) || quizCharDot(2, x - (CX + 1), y - (CY - 64), sc)) return COL_CLK_NUM;
    if (quizCharDot(3, x - (CX + 43), y - (CY - 10), sc)) return COL_CLK_NUM;
    if (quizCharDot(6, x - (CX - 7),  y - (CY + 43), sc)) return COL_CLK_NUM;
    if (quizCharDot(9, x - (CX - 58), y - (CY - 10), sc)) return COL_CLK_NUM; }
  // ryhy každých 5 minút (hrubšie pri 3, 6, 9, 12)
  if (r2 >= 73 * 73 && r2 <= 83 * 83){
    float a = atan2f((float)dx, (float)-dy); if (a < 0) a += 6.2831853f;
    const int idx = (int)lroundf(a / 0.5235988f);
    const float da = a - idx * 0.5235988f;
    const float d = sqrtf((float)r2) * fabsf(sinf(da));
    if (d <= ((idx % 3) == 0 ? 3.2f : 1.8f)) return COL_CLK_HAND;
  }
  return COL_CLK_FACE;
}
static inline void clockDraw(const ClockCtx &k){
  gagRenderRect(leftGFX(), petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeClock, &k);
}
static inline void clockSetHands(ClockCtx &k, float totalMin, float sec){
  k.hourA = totalMin * (6.2831853f / 720.0f);
  k.minA  = totalMin * (6.2831853f / 60.0f);
  k.secA  = sec * (6.2831853f / 60.0f);
}
static void activity_gag_clock(){
  gagStartCentered();
  // cieľový čas
  int hh, mm, ss;
#if CLOCKQ_REAL
  { Now n; if (rtc_ok && rtcRead(n)){} else softNow(n); hh = n.h; mm = n.m; ss = n.s; }
#else
  hh = random(1, 13); mm = random(0, 12) * 5; ss = 0;
#endif
  ClockCtx K; K.g = 0; K.secOn = false; clockSetHands(K, 0, 0);
  QuizCtx Q; Q.g = 0; Q.shown = 0; Q.ring = -1; Q.col = COL_QUIZ_ASK; Q.ringCol = COL_QUIZ_RING; quizSetText(Q, "", 9, 150);
  // 1) oko -> ciferník (ľavé), tabuľa (pravé) – výmena v tme cez podsvietenie
  gagBlOut();
  K.g = Q.g = 1.0f;
  clockDraw(K); quizDraw(1, Q);
  gagBlIn();
  // 2) ručičky sa roztočia z 12:00 na cieľový čas
  const float target = (hh % 12) * 60.0f + mm + ss / 60.0f + 720.0f;   // aspoň jedna celá otočka
  for (int i = 0; i <= 36; ++i){
    clockSetHands(K, target * ease(i / 36.0f), 0);
    clockDraw(K);
    delay(15);
  }
  // 3) pravé oko „?“ a ubúdajúci kruh, sekundovka tiká
  K.secOn = true;
  quizSetText(Q, "?", 12, 150); Q.shown = 1; Q.col = COL_QUIZ_ASK;
  const unsigned long t0 = millis(); long lastKey = -1; int lastSec = -1;
  for (;;){
    const unsigned long el = millis() - t0;
    if (el >= CLOCKQ_THINK_MS) break;
    const int sec = (ss + (int)(el / 1000)) % 60;
    if (sec != lastSec){ lastSec = sec; clockSetHands(K, target, (float)sec); clockDraw(K); }
    const float rem = 1.0f - (float)el / CLOCKQ_THINK_MS;
    const int secLeft = (int)((CLOCKQ_THINK_MS - el + 999) / 1000);
    const bool hurry = secLeft <= 3;
    const bool blinkOn = !hurry || ((el / 250) & 1) == 0;
    const long key = (long)(rem * 72) * 100 + secLeft * 2 + (blinkOn ? 1 : 0);
    if (key != lastKey){
      lastKey = key;
      if (hurry){ char d[2] = { (char)('0' + secLeft), 0 }; quizSetText(Q, d, 9, 150); Q.shown = 1; Q.col = COL_QUIZ_HURRY; }
      Q.ring = rem;
      Q.ringCol = hurry ? (blinkOn ? COL_QUIZ_HURRY : COL_QUIZ_TRACK) : COL_QUIZ_RING;
      quizDraw(1, Q);
    }
    delay(10);
  }
  // 4) odpoveď ako digitálne hodiny
  char res[8];
snprintf(res, sizeof(res), "%d:%02d", hh, mm);
  Serial1.println("G:CHEER"); Serial1.println("L:BLINK");               // čas uhádnutý: hej-hou a blikanie
  Q.ring = -1;
  for (int sc = 2; sc <= 7; ++sc){ quizSetText(Q, res, sc, 176); Q.shown = Q.len; Q.col = COL_QUIZ_ANS; quizDraw(1, Q); }
  for (int k = 0; k < 6; ++k){ Q.col = (k & 1) ? COL_QUIZ_ANS : COL_QUIZ_TXT; quizDraw(1, Q); delay(220); }
  Q.col = COL_QUIZ_ANS; quizDraw(1, Q);
  const unsigned long t1 = millis();
  while (millis() - t1 < 4000){                                     // sekundovka tiká ďalej
    const int sec = (ss + (int)((millis() - t0) / 1000)) % 60;
    if (sec != lastSec){ lastSec = sec; clockSetHands(K, target, (float)sec); clockDraw(K); }
    delay(20);
  }
  Serial1.println("G:NONE");
  // 5) späť na oči (cez podsvietenie)
  gagEndNormal();
}

/* =====================================================================
 *  GAG 44: Portály – duch prejde z oka do oka
 *  V ľavom oku je modrý portál (pri nose), v pravom oranžový. Duch vletí
 *  do modrého a vyletí z oranžového, obzrie sa a vráti sa späť.
 * ===================================================================== */
#define PORTAL_U   58          // poloha portálu od stredu oka smerom k nosu (px)

static uint16_t shadePortal(int x, int y, const void *c){
  const PortalCtx *p = (const PortalCtx *)c;
  if (p->g < 0.02f){ uint16_t c2; if (gagPupilPix(x, y, CX, CY, PUPIL_R, c2)) return c2; return COL_SCLERA; }
  const float u = (float)(x - CX) * p->ns, v = (float)(y - CY);
  // portál (elipsa)
  const float rx = 15.0f * p->open, ry = 64.0f * p->open;
  float e2 = 99.0f, ei2 = 99.0f;
  if (p->open > 0.02f){
    const float pu = u - PORTAL_U;
    e2 = (pu * pu) / (rx * rx) + (v * v) / (ry * ry);
    const float rxi = rx - 6.0f, ryi = ry - 6.0f;
    ei2 = (rxi > 1 && ryi > 1) ? (pu * pu) / (rxi * rxi) + (v * v) / (ryi * ryi) : 99.0f;
    if (e2 <= 1.0f && ei2 > 1.0f) return p->ringCol;                          // svietiaci okraj (vpredu)
  }
  // duch (len pred rovinou portálu)
  if (p->ghost && u < PORTAL_U){
    const float gx = (u - p->gu), gy = v - p->gy;
    bool in = false;
    if (gy < -8.0f) in = gx * gx + (gy + 8.0f) * (gy + 8.0f) <= 22.0f * 22.0f;
    else if (fabsf(gx) <= 22.0f) in = gy <= 16.0f + 5.0f * sinf(gx * 0.45f + p->phase * 3.0f);
    if (in){
      const float fx = gx * p->fdir;                                           // tvár smerom pohybu
      const float a1 = fx - 1.0f, a2 = fx - 14.0f, b = gy + 10.0f;
      if ((a1 * a1) / 16.0f + (b * b) / 36.0f <= 1.0f) return COL_PUPIL;
      if ((a2 * a2) / 16.0f + (b * b) / 36.0f <= 1.0f) return COL_PUPIL;
      const float m1 = fx - 7.0f, m2 = gy + 1.0f;
      if ((m1 * m1) / 12.0f + (m2 * m2) / 20.0f <= 1.0f) return COL_PUPIL;
      return COL_GHOST;
    }
  }
  if (e2 <= 1.0f){                                                            // vír vo vnútri portálu
    const float pu = u - PORTAL_U;
    const float s = 0.5f + 0.5f * sinf(atan2f(v, pu * 4.0f) * 3.0f + p->phase - sqrtf(e2) * 7.0f);
    return lerp565(p->ringDark, p->ringCol, s * 0.55f);
  }
  return lerp565(COL_SCLERA, COL_PORTAL_BG, p->g);
}
static inline void portalDraw(int e, const PortalCtx &p, bool band){
  if (band) gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, CY - 72, CX + EYE_R, CY + 72, shadePortal, &p);
  else      gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadePortal, &p);
}
// duch letí v oku e z u0 do u1 (súradnice smerom k nosu), fdir = kam sa pozerá
// prekreslí len okolie ducha (stará aj nová poloha) a portál, ktorého vír sa točí
static void portalUpdate(int e, const PortalCtx &p, float ou, float oy){
  Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
  const float u0 = min(ou, p.gu) - 25.0f, u1 = max(ou, p.gu) + 25.0f;
  int xa = (int)(CX + p.ns * u0), xb = (int)(CX + p.ns * u1);
  if (xa > xb){ int t = xa; xa = xb; xb = t; }
  const int ya = (int)(CY + min(oy, p.gy)) - 34, yb = (int)(CY + max(oy, p.gy)) + 24;
  gagRenderRect(gx, buf, xa - 1, ya, xb + 1, yb, shadePortal, &p);
  const int px = (int)(CX + p.ns * PORTAL_U);
  gagRenderRect(gx, buf, px - 17, CY - 66, px + 17, CY + 66, shadePortal, &p);
}
// duch letí v oku e z u0 do u1 (súradnice smerom k nosu), fdir = kam sa pozerá
static void portalFly(PortalCtx *P, int e, float u0, float u1, float fdir, float speed){
  P[e].ghost = true; P[e].fdir = fdir;
  const int n = (int)(fabsf(u1 - u0) / speed) + 1;
  for (int i = 0; i <= n; ++i){
    const float t = (float)i / n, ou = P[e].gu, oy = P[e].gy;
    P[e].gu = u0 + (u1 - u0) * t;
    P[e].phase += 0.35f;
    P[e].gy = 6.0f * sinf(P[e].phase * 0.8f);
    portalUpdate(e, P[e], (i == 0) ? P[e].gu : ou, (i == 0) ? P[e].gy : oy);
  }
  P[e].ghost = false; portalUpdate(e, P[e], P[e].gu, P[e].gy);
}
static void activity_gag_portal(){
  gagStartCentered();
  PortalCtx P[2];
  for (int e = 0; e < 2; ++e){
    P[e].g = 0; P[e].open = 0; P[e].phase = 0; P[e].ghost = false; P[e].gu = -200; P[e].gy = 0; P[e].fdir = 1;
    P[e].ns = (float)(e == 0 ? noseSideL() : -noseSideL());
    P[e].ringCol  = e ? COL_PORTAL_O  : COL_PORTAL_B;
    P[e].ringDark = e ? COL_PORTAL_OD : COL_PORTAL_BD;
  }
  // 1) oči potemnejú (cez podsvietenie, obraz sa vymení v tme) a otvoria sa portály
  gagBacklightFade(1.0f, 0.0f, 400);
  for (int e = 0; e < 2; ++e){ P[e].g = 1.0f; portalDraw(e, P[e], false); }
  gagBacklightFade(0.0f, 1.0f, 400);
  for (int i = 0; i <= 12; ++i){ for (int e = 0; e < 2; ++e){ P[e].open = ease(i / 12.0f); P[e].phase += 0.4f; portalUpdate(e, P[e], P[e].gu, P[e].gy); } }
  // 2) duch: ľavé oko -> modrý portál -> oranžový portál -> pravé oko
  portalFly(P, 0, -145.0f, PORTAL_U + 26.0f, 1.0f, 5.0f);
  portalFly(P, 1, PORTAL_U + 26.0f, -45.0f, -1.0f, 5.0f);
  // zastaví sa a obzrie sa na portál
  P[1].ghost = true; P[1].gu = -45.0f;
  for (int k = 0; k < 3; ++k){ P[1].fdir = (k & 1) ? -1.0f : 1.0f; for (int i = 0; i < 6; ++i){ const float oy = P[1].gy; P[1].phase += 0.35f; P[1].gy = 6.0f * sinf(P[1].phase * 0.8f); portalUpdate(1, P[1], P[1].gu, oy); } delay(250); }
  // 3) rýchlo späť: oranžový -> modrý -> preč z ľavého oka
  P[1].fdir = 1.0f;
  portalFly(P, 1, -45.0f, PORTAL_U + 26.0f, 1.0f, 9.0f);
  portalFly(P, 0, PORTAL_U + 26.0f, -150.0f, -1.0f, 9.0f);
  // 4) portály sa zatvoria, späť na oči
  for (int i = 12; i >= 0; --i){ for (int e = 0; e < 2; ++e){ P[e].open = ease(i / 12.0f); P[e].phase += 0.4f; portalUpdate(e, P[e], P[e].gu, P[e].gy); } }
  gagBacklightFade(1.0f, 0.0f, 400);
  for (int e = 0; e < 2; ++e){ P[e].g = 0.0f; portalDraw(e, P[e], false); }   // bežné oko (v tme)
  leftX() = rightX() = CX; leftY() = rightY() = CY;
  drawBoth();
  gagBacklightFade(0.0f, 1.0f, 400);
  scheduleNextBlink();
}


/* =====================================================================
 *  GAG 45: Škúlenie – oči sa spoja do jedného obrieho oka
 *  Oči začnú škúliť k nosu, až sa zlejú do jedného veľkého oka cez oba
 *  displeje. Obrie oko sa rozhliadne a zažmurká, potom sa rozdelí a oči
 *  si na seba navzájom žmurknú.
 *  Rýchla verzia: celé oko sa kreslí len pri premene; pri rozhliadaní,
 *  žmurkaní a žmurknutí sa posielajú iba pixely, ktoré sa naozaj menia.
 * ===================================================================== */
#define GIANT_EYES_DIST_MM 70     // vzdialenosť stredov displejov v tekvici (mm) – zmeraj a uprav
#define GIANT_OFF  ((int)(GIANT_EYES_DIST_MM * 0.5f * 240.0f / 32.4f))   // pol vzdialenosti v pixeloch
#define GIANT_AW   (GIANT_OFF + 140.0f)   // polovičná šírka obrieho oka
#define GIANT_AH   104                    // polovičná výška obrieho oka
#define GIANT_PR   ((float)max(40, GIANT_OFF - 45))   // obrovská zrenica
#define GIANT_IR   (GIANT_OFF + 28.0f)                // obrovská dúhovka
#define GIANT_CROSS 46                    // ako veľmi oči škúlia (px)
#define GIANT_MORPH_STEPS 16              // počet snímok pri premene (len pri GIANT_SMOOTH_MORPH 1)
#define GIANT_SMOOTH_MORPH 0              // 0 = premena cez podsvietenie (bez viditeľného prekresľovania), 1 = postupné zlievanie
#define GIANT_X0   (CX - EYE_R)
#define GIANT_WD   (2 * EYE_R + 1)

static int16_t giantH[2][GIANT_WD];       // polovičná výška mandle v každom stĺpci (-4 = mimo oka)
static int16_t giantO[2][GIANT_WD];       // otvorenie v stĺpci (pri žmurknutí menšie)

static void giantInitCols(){
  for (int e = 0; e < 2; ++e){
    const float ns = (float)(e == 0 ? noseSideL() : -noseSideL());
    const float gcx = CX + ns * GIANT_OFF;
    for (int i = 0; i < GIANT_WD; ++i){
      const float a = ((float)(GIANT_X0 + i) - gcx) / GIANT_AW;
      giantH[e][i] = (a * a < 1.0f) ? (int16_t)lroundf(GIANT_AH * (1.0f - a * a)) : (int16_t)-4;
      giantO[e][i] = giantH[e][i];
    }
  }
}
// nastaví snímku oka e: g (0 = škúliace oko, 1 = obrie oko), pohľad, zatvorenie viečka obrieho oka
static void giantSet(GiantFrame &k, int e, float g, float lookX, float lookY, float lid){
  const float ns = (float)(e == 0 ? noseSideL() : -noseSideL());
  const float vs = (float)noseSideL();                                  // smer „doprava pre diváka“
  k.e = e; k.g = g; k.lid = lid;
  const float gcx = CX + ns * GIANT_OFF;
  const float cx0 = CX + ns * GIANT_CROSS, cx1 = gcx + vs * lookX;
  const float pr = PUPIL_R + (GIANT_PR - PUPIL_R) * g;
  const float ir = PUPIL_R + (GIANT_IR - PUPIL_R) * g;
  k.pcx = (int)lroundf(cx0 + (cx1 - cx0) * g);
  k.pcy = (int)lroundf(CY + lookY * g);
  k.pr = (int)lroundf(pr); k.ir = (int)lroundf(ir);
  k.pr2 = (int32_t)k.pr * k.pr; k.ir2 = (int32_t)k.ir * k.ir;
  const int ird = max(0, k.ir - 6);
  k.ird2 = (int32_t)ird * ird;
  k.il1 = (int32_t)(k.pr + 8) * (k.pr + 8); k.il2 = (int32_t)(k.pr + 16) * (k.pr + 16);
  k.skin = lerp565(COL_SCLERA, COL_PUMPKIN, g);
  k.outline = lerp565(COL_SCLERA, COL_LID_SEAM, g);
  // odlesk: bežný (pr/3) -> veľký len v ľavom oku
  const float hx0 = cx0 + PUPIL_R / 3, hy0 = CY - PUPIL_R / 3, hr0 = PUPIL_R / 3;
  const float hx1 = (e == 0) ? CX + ns * 64 + vs * lookX : (float)k.pcx, hy1 = CY - 38 + lookY, hr1 = (e == 0) ? 15.0f : 0.0f;
  k.hlx = (int)lroundf(hx0 + (hx1 - hx0) * g); k.hly = (int)lroundf(hy0 + (hy1 - hy0) * g);
  k.hlr = (int)lroundf(hr0 + (hr1 - hr0) * g); k.hlr2 = (int32_t)k.hlr * k.hlr;
  for (int i = 0; i < GIANT_WD; ++i){
    const int h = giantH[e][i];
    giantO[e][i] = (h > 0) ? (int16_t)lroundf(h * (1.0f - lid)) : (int16_t)h;
  }
  k.yT = eyeTopY - 1; k.yB = eyeBotY + 1; k.wink = 0;
}
static inline void giantSetWink(GiantFrame &k, float w){
  k.wink = w;
  k.yT = (int)lroundf((eyeTopY - 1) + (CY + 8 - (eyeTopY - 1)) * w);
  k.yB = (int)lroundf((eyeBotY + 1) + (CY + 8 - (eyeBotY + 1)) * w);
}
static inline uint16_t giantPix(const GiantFrame &k, int x, int y){
  if (k.g > 0.0f){                                                      // obrie oko (mandľa)
    const int i = x - GIANT_X0, ay = abs(y - CY), h = giantH[k.e][i];
    if (ay > h + 3) return k.skin;
    if (ay > h)     return k.outline;
    const int o = giantO[k.e][i];
    if (ay > o + 3) return COL_PUMPKIN;
    if (ay > o)     return COL_LID_SEAM;
  } else {                                                              // bežné oko s viečkami (žmurk)
    if (y <= k.yT) return COL_LID_TOP;
    if (y >= k.yB) return COL_LID_BOT;
    if (k.wink > 0.0f && (y <= k.yT + 3 || y >= k.yB - 3)) return COL_LID_SEAM;
  }
  const int dx = x - k.pcx, dy = y - k.pcy;
  const int32_t d2 = (int32_t)dx * dx + (int32_t)dy * dy;
  if (d2 <= k.pr2){
    const int hx = x - k.hlx, hy = y - k.hly;
    if (k.hlr > 0 && (int32_t)hx * hx + (int32_t)hy * hy <= k.hlr2) return COL_HILITE;
    return COL_PUPIL;
  }
  if (k.g > 0.0f && d2 <= k.ir2){                                       // dúhovka v pásoch
    if (d2 > k.ird2) return COL_GIANT_IRIS_D;
    if (d2 <= k.il1) return COL_GIANT_IRIS_L;
    if (d2 <= k.il2) return COL_GIANT_IRIS_M;
    return COL_GIANT_IRIS;
  }
  return COL_SCLERA;
}
// obdĺžnik oka (po pásoch, bez mazania)
static void giantRect(const GiantFrame &k, int x0, int y0, int x1, int y1){
  if (x0 < GIANT_X0) x0 = GIANT_X0;
  if (x1 > CX + EYE_R) x1 = CX + EYE_R;
  if (y0 < eyeTopY) y0 = eyeTopY;
  if (y1 > eyeBotY) y1 = eyeBotY;
  if (x1 < x0 || y1 < y0) return;
  Arduino_GFX *gx = k.e ? rightGFX() : leftGFX(); uint16_t *buf = k.e ? petBufR : petBufL;
  const int w = x1 - x0 + 1;
  int rowsPer = (PET_TILE * PET_TILE) / w; if (rowsPer < 1) rowsPer = 1;
  for (int ys = y0; ys <= y1; ys += rowsPer){
    const int ye = min(y1, ys + rowsPer - 1);
    uint16_t *p = buf;
    for (int y = ys; y <= ye; ++y){
      const int xl = xLeft[y], xr = xRight[y];
      for (int x = x0; x <= x1; ++x) *p++ = (x < xl || x > xr) ? COL_BG : giantPix(k, x, y);
    }
    gx->draw16bitRGBBitmap(x0, ys, buf, w, ye - ys + 1);
  }
}
static inline void giantFull(const GiantFrame &k){ giantRect(k, GIANT_X0, eyeTopY, CX + EYE_R, eyeBotY); }

// priesečníky kruhu s riadkom: pridá interval okolo starej a novej hrany (vráti false = treba celý riadok)
static bool giantCross(int y, int ocx, int ocy, int orr, int ncx, int ncy, int nr, int iv[][2], int &n){
  if (orr <= 0 && nr <= 0) return true;
  const int dyo = y - ocy, dyn = y - ncy;
  const bool ho = dyo * dyo < orr * orr, hn = dyn * dyn < nr * nr;
  if (!ho && !hn) return true;
  if (ho != hn) return false;
  const int so = (int)sqrtf((float)(orr * orr - dyo * dyo)), sn = (int)sqrtf((float)(nr * nr - dyn * dyn));
  const int a[2] = { ocx - so, ocx + so }, b[2] = { ncx - sn, ncx + sn };
  for (int s = 0; s < 2; ++s){
    iv[n][0] = min(a[s], b[s]) - 2; iv[n][1] = max(a[s], b[s]) + 2; ++n;
  }
  return true;
}
// prekreslí len to, čo sa zmenilo pri pohybe dúhovky/zrenice (ko -> kn)
static void giantLookUpdate(const GiantFrame &ko, const GiantFrame &kn){
  const int ya = max(eyeTopY, CY - GIANT_AH - 4), yb = min(eyeBotY, CY + GIANT_AH + 4);
  const int BLK = 8;                                                    // riadky sa spracujú po blokoch (menej prenosov)
  for (int y0 = ya; y0 <= yb; y0 += BLK){
    const int y1 = min(yb, y0 + BLK - 1);
    int iv[10 * BLK][2], n = 0; bool full = false;
    // všetky hrany: dúhovka, jej tmavý okraj, zrenica a dva svetlé pásy okolo zrenice
    const int ro[5] = { ko.ir, ko.ir - 6, ko.pr, ko.pr + 8, ko.pr + 16 };
    const int rn[5] = { kn.ir, kn.ir - 6, kn.pr, kn.pr + 8, kn.pr + 16 };
    for (int y = y0; y <= y1 && !full; ++y)
      for (int c = 0; c < 5 && !full; ++c)
        if (!giantCross(y, ko.pcx, ko.pcy, ro[c], kn.pcx, kn.pcy, rn[c], iv, n)) full = true;
    if (full){ giantRect(kn, GIANT_X0, y0, CX + EYE_R, y1); continue; }
    for (int i = 1; i < n; ++i) for (int j = i; j > 0 && iv[j][0] < iv[j - 1][0]; --j){   // zoradiť
      int t0 = iv[j][0], t1 = iv[j][1]; iv[j][0] = iv[j - 1][0]; iv[j][1] = iv[j - 1][1]; iv[j - 1][0] = t0; iv[j - 1][1] = t1;
    }
    int s0 = 0, s1 = -1; bool open = false;
    for (int i = 0; i < n; ++i){
      if (iv[i][1] < GIANT_X0 || iv[i][0] > CX + EYE_R) continue;     // mimo displeja
      if (open && iv[i][0] <= s1 + 8){ if (iv[i][1] > s1) s1 = iv[i][1]; continue; }
      if (open) giantRect(kn, s0, y0, s1, y1);
      s0 = iv[i][0]; s1 = iv[i][1]; open = true;
    }
    if (open) giantRect(kn, s0, y0, s1, y1);
  }
  // odlesk (stará aj nová poloha)
  if (ko.hlr > 0 || kn.hlr > 0){
    const int r = max(ko.hlr, kn.hlr) + 1;
    giantRect(kn, min(ko.hlx, kn.hlx) - r, min(ko.hly, kn.hly) - r, max(ko.hlx, kn.hlx) + r, max(ko.hly, kn.hly) + r);
  }
}
// zatváranie viečok obrieho oka: po stĺpcoch len pás, kde sa hrana viečka posunula
static void giantBlinkUpdate(const GiantFrame &kn, const int16_t *oOld){
  const int BLK = 8;                                                    // stĺpce po blokoch (menej prenosov)
  for (int i0 = 0; i0 < GIANT_WD; i0 += BLK){
    const int i1 = min(GIANT_WD - 1, i0 + BLK - 1);
    int lo = 10000, hi = -1;
    for (int i = i0; i <= i1; ++i){
      const int h = giantH[kn.e][i]; if (h <= 0) continue;
      const int oo = oOld[i], on = giantO[kn.e][i]; if (oo == on) continue;
      lo = min(lo, max(0, min(oo, on) - 1)); hi = max(hi, min(h, max(oo, on) + 4));
    }
    if (hi < 0) continue;
    giantRect(kn, GIANT_X0 + i0, CY - hi, GIANT_X0 + i1, CY - lo);
    giantRect(kn, GIANT_X0 + i0, CY + lo, GIANT_X0 + i1, CY + hi);
  }
}
// žmurknutie bežného oka: len pásy riadkov pri hranách viečok
static void giantWinkUpdate(const GiantFrame &kn, int yTo, int yBo){
  giantRect(kn, GIANT_X0, min(yTo, kn.yT) - 1, CX + EYE_R, max(yTo, kn.yT) + 4);
  giantRect(kn, GIANT_X0, min(yBo, kn.yB) - 4, CX + EYE_R, max(yBo, kn.yB) + 1);
}
static void giantLook(GiantFrame *K, float &lx, float &ly, float tx, float ty, int steps){
  const float sx = lx, sy = ly;
  for (int i = 1; i <= steps; ++i){
    const float t = ease((float)i / steps);
    lx = sx + (tx - sx) * t; ly = sy + (ty - sy) * t;
    for (int e = 0; e < 2; ++e){
      GiantFrame ko = K[e];
      giantSet(K[e], e, 1.0f, lx, ly, 0.0f);
      giantLookUpdate(ko, K[e]);
    }
  }
}
static void giantBlink(GiantFrame *K, float lid){
  static int16_t oOld[GIANT_WD];
  for (int e = 0; e < 2; ++e){
    memcpy(oOld, giantO[e], sizeof(oOld));
    giantSet(K[e], e, 1.0f, 0, 0, lid);
    giantBlinkUpdate(K[e], oOld);
  }
}
static void activity_gag_giant(){
  gagStartCentered();
  giantInitCols();
  const int ns = noseSideL();
  // 1) oči začnú škúliť k nosu (trochu sa trasú)
  gagMovePupils(CX + ns * GIANT_CROSS, CY, CX - ns * GIANT_CROSS, CY, PUPIL_R, PUPIL_R, 3, 12);
  delay(500);
  for (int k = 0; k < 2; ++k){
    gagMovePupils(CX + ns * (GIANT_CROSS - 8), CY + 3, CX - ns * (GIANT_CROSS - 8), CY - 3, PUPIL_R, PUPIL_R, 2, 10);
    gagMovePupils(CX + ns * GIANT_CROSS, CY, CX - ns * GIANT_CROSS, CY, PUPIL_R, PUPIL_R, 2, 10);
  }
  delay(300);
  GiantFrame K[2];
  // 2) zlejú sa do jedného obrieho oka
#if GIANT_SMOOTH_MORPH
  for (int i = 1; i <= GIANT_MORPH_STEPS; ++i){
    const float g = ease((float)i / GIANT_MORPH_STEPS);
    for (int e = 0; e < 2; ++e){ giantSet(K[e], e, g, 0, 0, 0); giantFull(K[e]); }
  }
#else
  gagBlOut();
  for (int e = 0; e < 2; ++e){ giantSet(K[e], e, 1.0f, 0, 0, 0); giantFull(K[e]); }
  gagBacklightFade(0.0f, 1.0f, GAG_BL_MS);
#endif
  delay(700);
  // 3) obrie oko sa rozhliadne
  float lx = 0, ly = 0;
  giantLook(K, lx, ly, -24, -10, 12); delay(400);
  giantLook(K, lx, ly,  24, -10, 16); delay(400);
  giantLook(K, lx, ly,   0,  14, 12); delay(300);
  giantLook(K, lx, ly,   0,   0, 10); delay(300);
  // 4) pomalé zažmurkanie obrieho oka (dvakrát)
  for (int b = 0; b < 2; ++b){
    for (int i = 1; i <= 10; ++i) giantBlink(K, ease(i / 10.0f));
    delay(b ? 150 : 300);
    for (int i = 9; i >= 0; --i) giantBlink(K, ease(i / 10.0f));
    delay(400);
  }
  // 5) rozdelí sa späť na dve škúliace oči
#if GIANT_SMOOTH_MORPH
  for (int i = GIANT_MORPH_STEPS - 1; i >= 0; --i){
    const float g = ease((float)i / GIANT_MORPH_STEPS);
    for (int e = 0; e < 2; ++e){ giantSet(K[e], e, g, 0, 0, 0); giantFull(K[e]); }
  }
#else
  gagBlOut();
  for (int e = 0; e < 2; ++e){ giantSet(K[e], e, 0.0f, 0, 0, 0); giantFull(K[e]); }
  gagBacklightFade(0.0f, 1.0f, GAG_BL_MS);
#endif
  delay(400);
  // 6) oči si na seba žmurknú (najprv ľavé, potom pravé)
  for (int e = 0; e < 2; ++e){
    for (int i = 1; i <= 6; ++i){ const int yTo = K[e].yT, yBo = K[e].yB; giantSetWink(K[e], ease(i / 6.0f)); giantWinkUpdate(K[e], yTo, yBo); }
    delay(180);
    for (int i = 5; i >= 0; --i){ const int yTo = K[e].yT, yBo = K[e].yB; giantSetWink(K[e], ease(i / 6.0f)); giantWinkUpdate(K[e], yTo, yBo); }
    delay(450);
  }
  // 7) pohľad späť dopredu
  leftX() = CX + ns * GIANT_CROSS; rightX() = CX - ns * GIANT_CROSS; leftY() = rightY() = CY;
  gagPrL = gagPrR = PUPIL_R;
  gagMovePupils(CX, CY, CX, CY, PUPIL_R, PUPIL_R, 4, 12);
  gagEndNormal();
}

/* =====================================================================
 *  GAG 46: Dračie oči – svietiace žlté oči s úzkou zrenicou
 *  Okolo oka je zvrásnená koža, oči sa pomaly otvoria, rozhliadajú sa,
 *  zrenica sa zúži/rozšíri, žmurkajú a nakoniec sa zavrú.
 *  Rýchle kreslenie: koža aj dúhovka sú vopred vypočítané (8-bitové indexy
 *  do paliet), pri pohybe sa prekresľuje len okolie dúhovky a pri žmurkaní
 *  len pásy pri hranách viečok.
 * ===================================================================== */
#define DRG_SX   84               // polovičná šírka očnej jamky
#define DRG_SY   70               // polovičná výška očnej jamky
#define DRG_IR   60               // polomer dúhovky
#define DRG_IW   (2 * DRG_IR + 1)
#define DRG_SH   47               // polovičná výška štrbiny zrenice
#define DRG_X0   (CX - EYE_R)
#define DRG_WD   (2 * EYE_R + 1)
#if (DRG_WD * DRG_WD + (2 * 60 + 1) * (2 * 60 + 1)) > (191 * 191 * 2)
#error "drak sa nezmesti do spolocnej pamate"
#endif
#define drgSkin (gagArena)                          // vrásky kože (index 0..63), v spoločnej pamäti
#define drgIris (gagArena + DRG_WD * DRG_WD)        // dúhovka (index 0..63)
static int16_t  drgH[DRG_WD];                    // polovičná výška jamky v stĺpci (-1 = mimo)
static uint16_t drgSkinPal[64], drgSclPal[64], drgIrisPal[64];
static bool drgReady = false;

static inline uint16_t rgb565(int r, int g, int b){
  r = constrain(r, 0, 255); g = constrain(g, 0, 255); b = constrain(b, 0, 255);
  return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}
// farebná rampa: t 0..1 cez body (t, r, g, b)
static uint16_t drgRamp(const float (*pt)[4], int n, float t){
  if (t <= pt[0][0]) return rgb565((int)pt[0][1], (int)pt[0][2], (int)pt[0][3]);
  for (int i = 1; i < n; ++i) if (t <= pt[i][0]){
    const float f = (t - pt[i - 1][0]) / (pt[i][0] - pt[i - 1][0]);
    return rgb565((int)(pt[i - 1][1] + (pt[i][1] - pt[i - 1][1]) * f), (int)(pt[i - 1][2] + (pt[i][2] - pt[i - 1][2]) * f),
                  (int)(pt[i - 1][3] + (pt[i][3] - pt[i - 1][3]) * f));
  }
  return rgb565((int)pt[n - 1][1], (int)pt[n - 1][2], (int)pt[n - 1][3]);
}
static void drgBuild(){
  if (drgReady && gagArenaOwner == AR_DRAGON) return;
  static const float skinR[][4] = { {0.00f, 25, 4, 10}, {0.35f, 85, 10, 28}, {0.65f, 150, 25, 50}, {0.85f, 205, 70, 85}, {1.00f, 240, 170, 140} };
  static const float sclR[][4]  = { {0.00f, 255, 120, 30}, {0.45f, 230, 60, 20}, {0.80f, 140, 20, 15}, {1.00f, 60, 6, 10} };
  static const float irisR[][4] = { {0.00f, 255, 255, 170}, {0.30f, 255, 240, 70}, {0.65f, 255, 200, 20}, {0.88f, 250, 120, 10}, {1.00f, 200, 45, 10} };
  for (int k = 0; k < 64; ++k){
    drgSkinPal[k] = drgRamp(skinR, 5, k / 63.0f);
    drgSclPal[k]  = drgRamp(sclR, 4, k / 63.0f);
    drgIrisPal[k] = drgRamp(irisR, 5, k / 63.0f);
  }
  // koža: vrásky okolo jamky (nepravidelné záhyby) a na viečkach záhyby súbežné s okrajom
  for (int j = 0; j < DRG_WD; ++j) for (int i = 0; i < DRG_WD; ++i){
    const float dx = (float)(i - EYE_R), dy = (float)(j - EYE_R);
    const float q = sqrtf((dx * dx) / (float)(DRG_SX * DRG_SX) + (dy * dy) / (float)(DRG_SY * DRG_SY));
    const float a = atan2f(dy, dx);
    float v;
    if (q >= 1.0f){
      const float w = 1.3f * sinf(a * 4.0f + q * 2.0f) + 0.8f * sinf(a * 11.0f - q * 6.0f) + 0.5f * sinf(a * 23.0f + q * 13.0f);
      float f = sinf((q - 1.0f) * 20.0f + w);
      v = 0.5f + 0.5f * f;
      v = v * v * v;                                                      // úzke svetlé hrebene, tmavé ryhy
      v = 0.18f + 0.7f * v * (0.6f + 0.4f * sinf(a * 2.0f + 0.7f));
      if (q < 1.10f) v *= 0.25f + 0.75f * (q - 1.0f) / 0.10f;            // tmavý okraj jamky
      v *= 1.0f - 0.35f * clamp01((q - 1.25f) / 0.5f);                    // ďalej od oka tmavšie
    } else {
      const float hh = DRG_SY * sqrtf(fmaxf(0.0f, 1.0f - (dx * dx) / (float)(DRG_SX * DRG_SX)));
      const float t = (hh > 1.0f) ? fabsf(dy) / hh : 1.0f;              // 0 = stred, 1 = okraj jamky
      const float f = sinf(t * 10.0f + 0.7f * sinf(dx * 0.07f) + 0.4f * sinf(dx * 0.19f));
      v = 0.30f + 0.32f * (0.5f + 0.5f * f) * (0.5f + 0.5f * f);
    }
    uint32_t h = (uint32_t)(i * 73856093u) ^ (uint32_t)(j * 19349663u); h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    if ((h & 1023) < 5 && q > 1.15f) v = fmaxf(v, 0.65f);                  // zriedkavé svetlé zrnká
    int k = (int)(v * 63.0f + 0.5f); drgSkin[j * DRG_WD + i] = (uint8_t)constrain(k, 0, 63);
  }
  // dúhovka: zo žltého stredu do oranžovo-červeného okraja, jemné lúče
  for (int j = 0; j < DRG_IW; ++j) for (int i = 0; i < DRG_IW; ++i){
    const float dx = (float)(i - DRG_IR), dy = (float)(j - DRG_IR), d = sqrtf(dx * dx + dy * dy);
    const float rn = d / DRG_IR, a = atan2f(dy, dx);
    float t = rn * rn + 0.07f * sinf(a * 37.0f + 2.0f * sinf(a * 7.0f)) * rn;
    if (rn > 0.95f) t = 1.0f;                                             // tmavší lem
    int k = (int)(t * 63.0f + 0.5f); drgIris[j * DRG_IW + i] = (uint8_t)constrain(k, 0, 63);
  }
  for (int i = 0; i < DRG_WD; ++i){
    const float dx = (float)(i - EYE_R), a = 1.0f - (dx * dx) / (float)(DRG_SX * DRG_SX);
    drgH[i] = (a > 0.0f) ? (int16_t)lroundf(DRG_SY * sqrtf(a)) : (int16_t)-1;
  }
  drgReady = true; gagArenaOwner = AR_DRAGON;
}
static uint16_t shadeDragon(int x, int y, const void *c){
  const DragonSt *d = (const DragonSt *)c;
  const int i = x - DRG_X0, j = y - eyeTopY;
  const int si = d->mirror ? (DRG_WD - 1 - i) : i;                       // pravé oko má zrkadlovú kožu
  const int h = drgH[si], ay = abs(y - CY);
  if (h < 0 || ay > h) return drgSkinPal[drgSkin[j * DRG_WD + si]];      // koža mimo jamky
  const int open = (int)lroundf(h * (1.0f - d->lid));
  if (ay > open + 3) return drgSkinPal[drgSkin[j * DRG_WD + si]];        // viečko (koža)
  if (ay > open) return drgSkinPal[2];                                   // tmavá hrana viečka
  // statické odlesky (svetlo je stále na rovnakom mieste)
  { const int hx = x - (CX - 28), hy = y - (CY - 28);
    if ((hx * hx) * 4 + (hy * hy) * 9 <= 11 * 11 * 9) return rgb565(255, 255, 235); }
  { const int hx = x - (CX + 30), hy = y - (CY - 40);
    if (hx * hx + hy * hy <= 20) return rgb565(255, 250, 230); }
  const int dx = x - d->icx, dy = y - d->icy;
  if (dx * dx + dy * dy <= DRG_IR * DRG_IR){
    // štrbina zrenice (šošovka): |dx| <= w * (1 - (dy/h)^2)
    if (abs(dy) < DRG_SH){
      const float k = 1.0f - (float)(dy * dy) / (float)(DRG_SH * DRG_SH);
      const float w = d->slit * k;
      const float adx = fabsf((float)dx);
      if (adx <= w - 2.0f) return (adx < w * 0.45f) ? rgb565(255, 110, 20) : rgb565(215, 60, 15);   // oranžové jadro
      if (adx <= w + 1.0f) return rgb565(70, 8, 8);                                                   // tmavý obrys
      if (dx < 0 && adx <= w + 5.0f && dy < DRG_SH / 2) return rgb565(255, 252, 225);               // biely lesk vedľa štrbiny
    }
    return drgIrisPal[drgIris[(dy + DRG_IR) * DRG_IW + dx + DRG_IR]];
  }
  // bielko: žeravý červeno-oranžový prstenec
  const float q2 = (float)((x - CX) * (x - CX)) / (DRG_SX * DRG_SX) + (float)((y - CY) * (y - CY)) / (DRG_SY * DRG_SY);
  int k = (int)(q2 * 63.0f); if (k > 63) k = 63;
  return drgSclPal[k];
}
static inline void drgRect(int e, const DragonSt &d, int x0, int y0, int x1, int y1){
  gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, x0, y0, x1, y1, shadeDragon, &d);
}
// prekreslí len to, čo sa zmenilo medzi stavmi o -> n
static void drgUpdate(int e, const DragonSt &o, const DragonSt &n){
  if (o.icx != n.icx || o.icy != n.icy)                                  // dúhovka sa pohla
    drgRect(e, n, min(o.icx, n.icx) - DRG_IR - 1, min(o.icy, n.icy) - DRG_IR - 1, max(o.icx, n.icx) + DRG_IR + 1, max(o.icy, n.icy) + DRG_IR + 1);
  else if (o.slit != n.slit){                                            // len zrenica sa zúžila/rozšírila
    const int w = (int)ceilf(max(o.slit, n.slit)) + 7;
    drgRect(e, n, n.icx - w, n.icy - DRG_SH - 1, n.icx + w, n.icy + DRG_SH + 1);
  }
  if (o.lid != n.lid){                                                   // viečka: po blokoch stĺpcov len pás pri hrane
    const int BLK = 8;
    for (int i0 = 0; i0 < DRG_WD; i0 += BLK){
      const int i1 = min(DRG_WD - 1, i0 + BLK - 1);
      int lo = 10000, hi = -1;
      for (int i = i0; i <= i1; ++i){
        const int si = n.mirror ? (DRG_WD - 1 - i) : i, h = drgH[si];
        if (h < 0) continue;
        const int oo = (int)lroundf(h * (1.0f - o.lid)), on = (int)lroundf(h * (1.0f - n.lid));
        if (oo == on) continue;
        lo = min(lo, max(0, min(oo, on) - 1)); hi = max(hi, min(h, max(oo, on) + 4));
      }
      if (hi < 0) continue;
      drgRect(e, n, DRG_X0 + i0, CY - hi, DRG_X0 + i1, CY - lo);
      drgRect(e, n, DRG_X0 + i0, CY + lo, DRG_X0 + i1, CY + hi);
    }
  }
}
// plynulý prechod oboch očí do nového stavu (pohľad, zrenica, viečka)
static void drgAnim(DragonSt *D, float tx, float ty, float tslit, float tlid, int ms){
  DragonSt S[2] = { D[0], D[1] };
  const float sx = D[0].fx, sy = D[0].fy, ss = D[0].slit, sl = D[0].lid;
  const unsigned long t0 = millis();
  for (;;){
    const unsigned long el = millis() - t0;
    const float f = (ms <= 0 || el >= (unsigned long)ms) ? 1.0f : ease((float)el / ms);
    for (int e = 0; e < 2; ++e){
      DragonSt n = D[e];
      n.fx = sx + (tx - sx) * f; n.fy = sy + (ty - sy) * f;
      n.icx = (int)lroundf(CX + n.fx); n.icy = (int)lroundf(CY + n.fy);
      n.slit = roundf((ss + (tslit - ss) * f) * 2.0f) * 0.5f;             // po pol pixli
      n.lid = roundf((sl + (tlid - sl) * f) * 60.0f) / 60.0f;
      drgUpdate(e, D[e], n);
      D[e] = n;
    }
    if (f >= 1.0f) break;
    delay(4);
  }
  (void)S;
}
static void activity_gag_dragon(){
  gagStartCentered();
  drgBuild();                                                             // prvýkrát asi 0,3 s
  DragonSt D[2];
  for (int e = 0; e < 2; ++e){ D[e].mirror = (e == 1); D[e].fx = 0; D[e].fy = 0; D[e].icx = CX; D[e].icy = CY; D[e].slit = 8.0f; D[e].lid = 1.0f; }
  // 1) v tme sa ukážu zavreté dračie oči
  gagBlOut();
  for (int e = 0; e < 2; ++e) drgRect(e, D[e], CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY);
  gagBlIn();
  delay(600);
  // 2) pomaly sa otvoria
  drgAnim(D, 0, 0, 8, 0, 1400);
  delay(700);
  // 3) rozhliadanie (zrenica sa pri tom mení)
  drgAnim(D, -12, -3, 6, 0, 900);   delay(600);
  drgAnim(D,  12, -3, 6, 0, 1300);  delay(600);
  drgAnim(D,   0,  5, 12, 0, 900);  delay(500);                           // pozrie dole a rozšíri zrenicu
  drgAnim(D,   0,  0, 8, 0, 700);
  // 4) žmurknutie, potom rýchle dvojité
  drgAnim(D, 0, 0, 8, 1, 220); delay(80); drgAnim(D, 0, 0, 8, 0, 260); delay(900);
  drgAnim(D, -7, 0, 4, 0, 600); delay(400);                               // prižmúrenie: úzka štrbina
  for (int k = 0; k < 2; ++k){ drgAnim(D, -7, 0, 4, 1, 140); drgAnim(D, -7, 0, 4, 0, 160); }
  delay(500);
  drgAnim(D, 0, 0, 10, 0.45f, 700); delay(1200);                          // podozrievavý pohľad
  // 5) pomaly sa zavrú a späť na bežné oči
  drgAnim(D, 0, 0, 8, 1, 1100); delay(400);
  gagBl = 2;                                                              // návrat cez podsvietenie
  gagEndNormal();
}

/* =====================================================================
 *  Spoločné: prekreslenie len pásov pri hranách viečok (po blokoch stĺpcov)
 *  t0/t1 = stará/nová horná hrana v každom stĺpci, b0/b1 = spodná hrana
 * ===================================================================== */
#define EB_X0  (CX - EYE_R)
#define EB_WD  (2 * EYE_R + 1)
static void edgeBandUpdate(int e, GagShadeFn fn, const void *ctx,
                           const int16_t *t0, const int16_t *t1, const int16_t *b0, const int16_t *b1, int margin){
  Arduino_GFX *gx = e ? rightGFX() : leftGFX(); uint16_t *buf = e ? petBufR : petBufL;
  const int BLK = 8;
  for (int i0 = 0; i0 < EB_WD; i0 += BLK){
    const int i1 = min(EB_WD - 1, i0 + BLK - 1);
    int tl = 10000, th = -10000, bl = 10000, bh = -10000;
    for (int i = i0; i <= i1; ++i){
      if (t0[i] != t1[i]){ tl = min(tl, (int)min(t0[i], t1[i])); th = max(th, (int)max(t0[i], t1[i])); }
      if (b0[i] != b1[i]){ bl = min(bl, (int)min(b0[i], b1[i])); bh = max(bh, (int)max(b0[i], b1[i])); }
    }
    if (th > -10000) gagRenderRect(gx, buf, EB_X0 + i0, tl - margin, EB_X0 + i1, th + margin, fn, ctx);
    if (bh > -10000) gagRenderRect(gx, buf, EB_X0 + i0, bl - margin, EB_X0 + i1, bh + margin, fn, ctx);
  }
}

/* =====================================================================
 *  GAG 47: Červené oči v tme – žeravé zlé oči na čiernom pozadí
 *  Z tmy sa pomaly otvoria červené dúhovky so šikmým viečkom, čierna
 *  zrenica s bielou bodkou sa rozhliada, oči žmurkajú a nakoniec sa
 *  zúžia na ostré zlé kosáky.
 * ===================================================================== */
#define RED_R    80               // polomer červenej dúhovky
#define RED_PR   30               // polomer zrenice
static int16_t redT0[2][EB_WD], redB0[2][EB_WD];          // naposledy nakreslené hrany viečok
static inline int redTop(const RedEyeSt &s, int x){        // horné šikmé viečko (pri nose nižšie)
  const float u = (x - CX) * s.ns;
  const float base = (CY - 4) + 0.38f * u;
  return (int)lroundf(base + s.lid * (2 * RED_R + 30) + s.sq * 10.0f);   // pri zúžení trochu klesne
}
static inline int redBot(const RedEyeSt &s, int x){        // spodné viečko – pri zúžení stúpa do oblúka
  const float u = (x - CX) * s.ns;
  float arc = 1.0f - (u * u) / (88.0f * 88.0f); if (arc < 0) arc = 0;
  return (int)lroundf((CY + 12 + RED_R) - s.sq * 84.0f * arc * arc);   // stred stúpa najviac -> dva ostré rohy
}
static uint16_t shadeRedEye(int x, int y, const void *c){
  const RedEyeSt *s = (const RedEyeSt *)c;
  if (y < redTop(*s, x) || y > redBot(*s, x)) return COL_BG;
  const int dx = x - CX, dy = y - (CY + 12), d2 = dx * dx + dy * dy;
  if (d2 > (RED_R + 8) * (RED_R + 8)) return COL_BG;
  const int px = x - s->pcx, py = y - s->pcy;
  if (px * px + py * py <= RED_PR * RED_PR){
    const int hx = px - 9, hy = py - 9;
    return (hx * hx + hy * hy <= 16) ? COL_SCLERA : COL_BG;     // biela bodka v zrenici
  }
  const float t = sqrtf((float)d2) / RED_R;                      // jas: stred jasne červený, okraj do tmy
  if (t < 0.45f) return rgb565(235, 25, 25);
  if (t < 0.85f) return rgb565((int)(235 - (t - 0.45f) / 0.4f * 125), (int)(25 - (t - 0.45f) / 0.4f * 25), (int)(25 - (t - 0.45f) / 0.4f * 25));
  return rgb565((int)(110 * clamp01((1.10f - t) / 0.25f)), 0, 0);
}
static void redDraw(int e, RedEyeSt &n, bool full, const RedEyeSt *o){
  int16_t t1[EB_WD], b1[EB_WD];
  for (int i = 0; i < EB_WD; ++i){ t1[i] = (int16_t)redTop(n, EB_X0 + i); b1[i] = (int16_t)redBot(n, EB_X0 + i); }
  if (full) gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeRedEye, &n);
  else {
    if (o->pcx != n.pcx || o->pcy != n.pcy)                                 // zrenica (stará aj nová poloha)
      gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, min(o->pcx, n.pcx) - RED_PR - 1, min(o->pcy, n.pcy) - RED_PR - 1,
                    max(o->pcx, n.pcx) + RED_PR + 1, max(o->pcy, n.pcy) + RED_PR + 1, shadeRedEye, &n);
    edgeBandUpdate(e, shadeRedEye, &n, redT0[e], t1, redB0[e], b1, 1);
  }
  memcpy(redT0[e], t1, sizeof(t1)); memcpy(redB0[e], b1, sizeof(b1));
}
static void redAnim(RedEyeSt *R, float tlid, float tsq, float tlook, int ms){
  const float sl = R[0].lid, ss = R[0].sq, sk = R[0].look;
  const unsigned long t0 = millis();
  for (;;){
    const unsigned long el = millis() - t0;
    const float f = (ms <= 0 || el >= (unsigned long)ms) ? 1.0f : ease((float)el / ms);
    for (int e = 0; e < 2; ++e){
      RedEyeSt o = R[e];
      R[e].lid = roundf((sl + (tlid - sl) * f) * 200.0f) / 200.0f;
      R[e].sq = roundf((ss + (tsq - ss) * f) * 100.0f) / 100.0f;
      R[e].look = sk + (tlook - sk) * f;
      R[e].pcx = (int)lroundf(CX + R[e].look + R[e].ns * 6.0f);
      R[e].pcy = (int)lroundf(CY - 8 + 0.3f * fabsf(R[e].look));
      redDraw(e, R[e], false, &o);
    }
    if (f >= 1.0f) break;
    delay(4);
  }
}
static void activity_gag_red_eyes(){
  gagStartCentered();
  RedEyeSt R[2];
  for (int e = 0; e < 2; ++e){
    R[e].ns = (float)(e == 0 ? noseSideL() : -noseSideL());
    R[e].lid = 1.0f; R[e].sq = 0.0f; R[e].look = 0.0f;
    R[e].pcx = (int)lroundf(CX + R[e].ns * 6.0f); R[e].pcy = CY - 8;
  }
  gagBlOut();                                                             // v tme len čierna
  for (int e = 0; e < 2; ++e) redDraw(e, R[e], true, nullptr);
  gagBlIn();
  delay(700);
  redAnim(R, 0.0f, 0.0f, 0.0f, 1800);  delay(800);                        // pomaly sa otvoria z tmy
  redAnim(R, 0.0f, 0.0f, -28.0f, 700); delay(700);                        // pozrú doľava
  redAnim(R, 0.0f, 0.0f, 28.0f, 1000); delay(700);                        // doprava
  redAnim(R, 0.0f, 0.0f, 0.0f, 600);   delay(400);
  redAnim(R, 1.0f, 0.0f, 0.0f, 180);   redAnim(R, 0.0f, 0.0f, 0.0f, 220); delay(900);   // žmurk
  redAnim(R, 0.0f, 1.0f, 0.0f, 900);   delay(1600);                       // zúžia sa na ostré kosáky
  redAnim(R, 0.0f, 1.0f, -14.0f, 500); delay(500);
  redAnim(R, 0.0f, 1.0f, 14.0f, 700);  delay(700);
  redAnim(R, 0.0f, 0.0f, 0.0f, 700);   delay(600);
  redAnim(R, 1.0f, 0.0f, 0.0f, 1200);  delay(500);                        // zavrú sa do tmy
  gagEndNormal();                                                         // späť cez podsvietenie
}

/* =====================================================================
 *  GAG 48: Plamenné oči – ľavé červené, pravé modré
 *  Šikmé žeravé oči s bielym stredom, nad nimi šľahajú plamene.
 *  Plamene sa kreslia z malej textúry šumu (rýchle), prekresľuje sa len
 *  pás s plameňmi a pri žmurknutí/pohľade len okolie oka.
 * ===================================================================== */
static uint8_t flNoise[64 * 64];
static inline float n_sway(int y, int tt){ return sinf((y + tt * 2) * 0.09f); }   // jemné vlnenie do strán
static bool flReady = false;
static uint16_t flPal[2][64];                                             // 0 = červená, 1 = modrá
static void flBuild(){
  if (flReady) return;
  uint8_t g[9][9];
  for (int j = 0; j < 9; ++j) for (int i = 0; i < 9; ++i) g[j][i] = (uint8_t)random(256);
  for (int i = 0; i < 9; ++i){ g[8][i] = g[0][i]; g[i][8] = g[i][0]; }   // dlaždica sa opakuje bez švu
  for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x){
    const float fx = x / 8.0f, fy = y / 8.0f; const int ix = (int)fx, iy = (int)fy;   // bunky 8 px
    float ax = fx - ix, ay = fy - iy; ax = ax * ax * (3 - 2 * ax); ay = ay * ay * (3 - 2 * ay);
    const float v = (g[iy][ix] * (1 - ax) + g[iy][ix + 1] * ax) * (1 - ay) + (g[iy + 1][ix] * (1 - ax) + g[iy + 1][ix + 1] * ax) * ay;
    flNoise[y * 64 + x] = (uint8_t)v;
  }
  static const float red[][4]  = { {0.00f, 0, 0, 0}, {0.25f, 90, 0, 0}, {0.50f, 220, 20, 10}, {0.75f, 255, 90, 60}, {1.00f, 255, 240, 230} };
  static const float blue[][4] = { {0.00f, 0, 0, 0}, {0.25f, 0, 20, 100}, {0.50f, 20, 90, 255}, {0.75f, 90, 180, 255}, {1.00f, 235, 250, 255} };
  for (int k = 0; k < 64; ++k){ flPal[0][k] = drgRamp(red, 5, k / 63.0f); flPal[1][k] = drgRamp(blue, 5, k / 63.0f); }
  flReady = true;
}
/* Tvar oka (ľavé oko, u > 0 smerom k nosu): ostrý vnútorný kútik nižšie pri nose,
 * horné viečko šikmé (zlý pohľad), nad ním dlhá svietiaca čiara obočia, ktorá
 * pokračuje šikmo hore von; z nej stúpajú plamene. Statické svietiace čiary sú
 * vopred „nastreľované“ do bufferu (index jasu 0..63, pravé oko je zrkadlové);
 * horné viečko je dynamické (žmurkanie), dúhovka sa hýbe. */
#define FL_HW   72                       // polovičná šírka oka
#define FL_GR   12                       // dosah žiary čiar (px)
#define FL_X0   (CX - EYE_R)
#define FL_WD   (2 * EYE_R + 1)
#define flStatic (gagArena)               // jas statických čiar (spodné viečko, chvostíky), v spoločnej pamäti
static uint8_t flGlowLut[FL_GR + 1];     // jas podľa vzdialenosti od čiary
static bool flStaticReady = false;
static inline float flTop(float u){ const float a = 1.0f - (u * u) / (float)(FL_HW * FL_HW); return CY + 4 - 30.0f * a + 0.30f * u; }
static inline float flBot(float u){ const float a = 1.0f - (u * u) / (float)(FL_HW * FL_HW); return CY + 4 + 17.0f * a + 0.30f * u; }
static inline float flBrow(float u){                                   // čiara obočia nad okom, za kútikom stúpa šikmo hore
  const float uu = fmaxf(-(float)FL_HW, fminf((float)FL_HW, u));
  return flTop(uu) - 9.0f * clamp01((FL_HW - uu) / (2.0f * FL_HW)) - fmaxf(0.0f, -FL_HW - u) * 0.62f;   // pri nose splýva s viečkom
}
static void flStamp(float px, float py, float k){                     // jedna „bodka“ žiary do bufferu
  const int cx = (int)lroundf(px), cy = (int)lroundf(py);
  for (int dy = -FL_GR; dy <= FL_GR; ++dy) for (int dx = -FL_GR; dx <= FL_GR; ++dx){
    const int i = cx + dx - FL_X0, j = cy + dy - eyeTopY;
    if (i < 0 || j < 0 || i >= FL_WD || j >= FL_WD) continue;
    const int d = (int)sqrtf((float)(dx * dx + dy * dy)); if (d > FL_GR) continue;
    const int v = (int)(flGlowLut[d] * k);
    if (v > flStatic[j * FL_WD + i]) flStatic[j * FL_WD + i] = (uint8_t)v;
  }
}
static void flBuildStatic(){
  if (flStaticReady && gagArenaOwner == AR_FLAME) return;
  for (int d = 0; d <= FL_GR; ++d) flGlowLut[d] = (uint8_t)(63.0f * expf(-d / 2.6f) + 0.5f);
  memset(flStatic, 0, FL_WD * FL_WD);
  // u -> x pre ľavé oko (nos vpravo); pravé oko sa kreslí zrkadlovo
  for (float u = -FL_HW; u <= FL_HW; u += 0.7f) flStamp(CX + u, flBot(u), 0.85f);   // spodné viečko
  for (float t = 0; t <= 1.0f; t += 0.03f){                           // chvostíky: pri nose dole, vonku von
    flStamp(CX + FL_HW + 22.0f * t, flBot(FL_HW) + 12.0f * t * t, 0.8f * (1.0f - 0.6f * t));
    flStamp(CX - FL_HW - 16.0f * t, flBot(-FL_HW) + 3.0f * t, 0.7f * (1.0f - 0.6f * t));
  }
  flStaticReady = true; gagArenaOwner = AR_FLAME;
}
// poloha čiary obočia s viečkom (pri zatvorení klesne až na spodné viečko)
static inline float flBrowL(const FlameSt &s, float u){
  if (u < -FL_HW) return flBrow(u);
  const float uu = fminf((float)FL_HW, u);
  return flBrow(uu) + (flBot(uu) - 1.0f - flBrow(uu)) * s.lid;
}
static inline int flGlow(float d){ const int k = (int)fabsf(d); return (k <= FL_GR) ? flGlowLut[k] : 0; }
/* Vyrovnávacia pamäť po stĺpcoch/riadkoch: všetko, čo nezávisí od konkrétneho
 * pixla, sa vypočíta raz pre stav oka (viečko, plamene, čas) – shader potom
 * robí len pár porovnaní a vyhľadaní v tabuľkách. */
struct FlColCache { float lid, fl, t; bool ok; };
static FlColCache flCK[2] = { { 0, 0, 0, false }, { 0, 0, 0, false } };
static float  flCu[2][240], flCyB[2][240], flCyu[2][240], flCyb[2][240], flCext[2][240], flCinvH[2][240], flChf[2][240];
static int8_t flCsw[2][240];
static void flPrep(const FlameSt &s){
  FlColCache &k = flCK[s.e];
  if (k.ok && k.lid == s.lid && k.fl == s.fl && k.t == s.t) return;
  const bool geo = !k.ok || k.lid != s.lid || k.fl != s.fl;
  if (geo){
    for (int x = 0; x < 240; ++x){
      const float u = (x - CX) * s.ns;
      flCu[s.e][x] = u;
      flCyB[s.e][x] = flBrowL(s, u);
      if (fabsf(u) <= FL_HW){
        const float yt = flTop(u), yb = flBot(u);
        flCyu[s.e][x] = yt + (yb - 1.0f - yt) * s.lid; flCyb[s.e][x] = yb;
      } else { flCyu[s.e][x] = -1000; flCyb[s.e][x] = -1000; }
      flCext[s.e][x] = (u < -FL_HW && u > -125.0f) ? (1.0f - s.lid) * clamp01((u + 125.0f) / 12.0f) : 0.0f;
      const float hgt = (40.0f + 85.0f * clamp01((10.0f - u) / 100.0f)) * s.fl;
      flCinvH[s.e][x] = (s.fl > 0.0f && u < 30.0f && u > -125.0f && hgt > 0.5f) ? 1.0f / hgt : 0.0f;
      flChf[s.e][x] = clamp01((30.0f - u) / 30.0f) * clamp01((u + 125.0f) / 20.0f);
    }
  }
  const int tt = (int)s.t;
  for (int y = 0; y < 240; ++y) flCsw[s.e][y] = (int8_t)(4.0f * n_sway(y, tt));
  k.lid = s.lid; k.fl = s.fl; k.t = s.t; k.ok = true;
}
static uint16_t shadeFlameEye(int x, int y, const void *c){
  const FlameSt *s = (const FlameSt *)c;
  const int e = s->e;
  const uint16_t *pal = flPal[e];
  const float u = flCu[e][x];
  const int si = (s->ns > 0) ? (x - FL_X0) : (FL_WD - 1 - (x - FL_X0));
  int I = flStatic[(y - eyeTopY) * FL_WD + si];                       // spodné viečko a chvostíky
  const float yb_ = flCyB[e][x];                                       // obočie (klesá s viečkom)
  const float yu = flCyu[e][x];
  if (yu > -999.0f){
    { const int v = (int)(flGlow(y - yb_) * 0.9f); if (v > I) I = v; }
    { int v = (int)(flGlow(y - yu) * (1.0f + 0.25f * s->lid)); if (v > 63) v = 63; if (v > I) I = v; }
    if (y > yu + 1.0f && y < flCyb[e][x]){                             // vnútro oka
      const int dx = x - s->icx, dy = y - (CY + 2);
      const int d2 = dx * dx + dy * dy;
      int v;
      if (d2 <= 8 * 8) v = 0;                                          // tmavá zrenica v strede dúhovky
      else if (d2 <= 16 * 16) v = 63;                                  // biely žeravý prstenec okolo nej
      else if (d2 <= 28 * 28){ const float d = sqrtf((float)d2); v = 58 - (int)((d - 16.0f) * 1.6f); }
      else { const float d = sqrtf((float)d2); v = max(8, 34 - (int)((d - 28.0f) * 1.8f)); }
      if (v == 0) I = (I > 40) ? I : 0;
      else if (v > I) I = v;
    }
  } else if (flCext[e][x] > 0.0f){                                     // predĺženie obočia von
    const int v = (int)(flGlow((y - yb_) * 0.85f) * flCext[e][x]); if (v > I) I = v;
  }
  const float invH = flCinvH[e][x];                                    // plamene
  if (invH > 0.0f){
    const float rise = yb_ - 1.0f - y;
    const float k = 1.0f - fmaxf(0.0f, rise) * invH;
    if (rise > -4.0f && k > 0.0f){
      const int tt = (int)s->t;
      const int sway = flCsw[e][y] + (int)(rise * 0.35f);
      const int xo = (s->ns > 0) ? x + sway : x - sway;
      const int n1 = flNoise[((((y >> 1) + tt) & 63) << 6) | (xo & 63)];
      const int n2 = flNoise[((((y >> 2) + (tt >> 1) + 23) & 63) << 6) | (((xo >> 1) + 40) & 63)];
      const float n = (n1 * 0.6f + n2 * 0.4f) * (1.0f / 255.0f);
      float inten = k * k * (0.15f + 1.25f * n) - 0.22f;
      if (inten > 0.0f){
        inten *= flChf[e][x] * clamp01((rise + 4.0f) / 6.0f);
        const int v = min(63, (int)(inten * 80.0f)); if (v > I) I = v;
      }
    }
  }
  return pal[I];
}
static inline void flRect(int e, const FlameSt &s, int x0, int y0, int x1, int y1){
  flPrep(s);
  gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, x0, y0, x1, y1, shadeFlameEye, &s);
}
// x-rozsah v obrazovke pre u-rozsah daného oka
static inline void flURange(const FlameSt &s, float u0, float u1, int &xa, int &xb){
  const float a = CX + u0 * s.ns, b = CX + u1 * s.ns;
  xa = (int)floorf(fminf(a, b)); xb = (int)ceilf(fmaxf(a, b));
}
static void flUpdate(int e, const FlameSt &o, const FlameSt &n){
  int xa, xb;
  if (o.lid != n.lid){                                                  // viečko a obočie: pás medzi starou a novou polohou (+ žiara)
    const int BLK = 8;
    flURange(n, -FL_HW, FL_HW, xa, xb);
    for (int x0 = xa; x0 <= xb; x0 += BLK){
      const int x1 = min(xb, x0 + BLK - 1);
      float lo = 1e9f, hi = -1e9f;
      for (int x = x0; x <= x1; ++x){
        const float u = (x - CX) * n.ns; if (fabsf(u) > FL_HW) continue;
        const float yt = flTop(u), yb = flBot(u);
        const float a = yt + (yb - 1.0f - yt) * o.lid, b = yt + (yb - 1.0f - yt) * n.lid;
        const float c = flBrowL(o, u), d = flBrowL(n, u);
        lo = fminf(lo, fminf(fminf(a, b), fminf(c, d))); hi = fmaxf(hi, fmaxf(fmaxf(a, b), fmaxf(c, d)));
      }
      if (hi > -1e9f) flRect(e, n, x0, (int)lo - FL_GR - 2, x1, (int)hi + FL_GR + 2);
    }
    flURange(n, -126.0f, -FL_HW, xa, xb);                               // predĺženie obočia (stmavne/rozsvieti sa)
    flRect(e, n, xa, (int)flBrow(-126.0f) - FL_GR - 2, xb, (int)flBrow(-FL_HW) + FL_GR + 2);
  }
  if (o.icx != n.icx && o.lid == n.lid)                                // dúhovka sa pohla
    flRect(e, n, min(o.icx, n.icx) - 46, CY - 48, max(o.icx, n.icx) + 46, CY + 50);
  if (n.fl > 0.0f || o.fl > 0.0f){                                      // plamene: po blokoch stĺpcov len výška plameňa
    flPrep(o.fl > n.fl ? o : n);                                         // väčšie plamene určujú rozsah
    const FlameSt &big = (o.fl > n.fl) ? o : n;
    flURange(n, -125.0f, 30.0f, xa, xb);
    xa = max(xa, CX - EYE_R); xb = min(xb, CX + EYE_R);
    float bl[240], bh[240];
    for (int x = xa; x <= xb; ++x){
      const float hgt = (flCinvH[big.e][x] > 0.0f) ? 1.0f / flCinvH[big.e][x] : 0.0f;
      const float ybA = flBrowL(o, flCu[big.e][x]), ybB = flBrowL(n, flCu[big.e][x]);
      bl[x] = fminf(ybA, ybB) - 1.0f - hgt - 1.0f; bh[x] = fmaxf(ybA, ybB) + 4.0f;
    }
    const int BLK = 8;
    for (int x0 = xa; x0 <= xb; x0 += BLK){
      const int x1 = min(xb, x0 + BLK - 1);
      float lo = 1e9f, hi = -1e9f;
      for (int x = x0; x <= x1; ++x){ lo = fminf(lo, bl[x]); hi = fmaxf(hi, bh[x]); }
      flRect(e, n, x0, (int)floorf(lo), x1, (int)ceilf(hi));
    }
  }
}
static void flStep(FlameSt *F, float tlid, float tlook, float tfl, int ms){
  const float sl = F[0].lid, sk = F[0].look, sf = F[0].fl;
  const unsigned long t0 = millis();
  for (;;){
    const unsigned long el = millis() - t0;
    const float f = (ms <= 0 || el >= (unsigned long)ms) ? 1.0f : ease((float)el / ms);
    for (int e = 0; e < 2; ++e){
      FlameSt o = F[e];
      F[e].lid = roundf((sl + (tlid - sl) * f) * 100.0f) / 100.0f;
      F[e].look = sk + (tlook - sk) * f;
      F[e].icx = (int)lroundf(CX - 4.0f * F[e].ns + F[e].look);
      F[e].fl = sf + (tfl - sf) * f;
      F[e].t = millis() * 0.06f;                                          // plamene stúpajú
      if (o.lid != F[e].lid && o.icx != F[e].icx){ FlameSt m = o; m.icx = F[e].icx; m.lid = o.lid; flUpdate(e, o, m); flUpdate(e, m, F[e]); }
      else flUpdate(e, o, F[e]);
    }
    if (f >= 1.0f) break;
    delay(2);
  }
}
// žmurknutie: rýchlo dole, krátko zostane zavreté (čiara zažiari), pomalšie hore
static void flBlink(FlameSt *F, float look, float fl){
  flStep(F, 1.0f, look, fl, 110); flStep(F, 1.0f, look, fl, 70); flStep(F, 0.0f, look, fl, 220);
}
static void activity_gag_flame_eyes(){
  gagStartCentered();
  flBuild(); flBuildStatic();
  FlameSt F[2];
  for (int e = 0; e < 2; ++e){
    F[e].e = e; F[e].ns = (float)(e == 0 ? noseSideL() : -noseSideL());
    F[e].lid = 1.0f; F[e].look = 0; F[e].icx = (int)lroundf(CX - 4.0f * F[e].ns); F[e].fl = 0.0f; F[e].t = 0;
  }
  gagBlOut();
  for (int e = 0; e < 2; ++e) flRect(e, F[e], CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY);
  gagBacklightFade(0.0f, 0.45f, 500);                                     // v tme len slabá žiara spodných viečok
  delay(700);
  gagBacklightFade(0.45f, 1.0f, 250);
  flStep(F, 0.45f, 0.0f, 0.0f, 300); delay(900);                          // úzko pootvorené, bez plameňov
  flStep(F, 1.0f, 0.0f, 0.0f, 160);                                       // zaklapnú...
  gagBacklightFade(1.0f, 0.15f, 300); delay(600);                         // ...a stmavnú
  flStep(F, 0.0f, 0.0f, 0.3f, 1);                                         // v tme sa otvoria naplno
  gagBacklightFade(0.15f, 1.0f, 150);
  flStep(F, 0.0f, 0.0f, 1.0f, 700);                                       // vzplanú plamene
  flStep(F, 0.0f, 0.0f, 1.0f, 1200);
  flStep(F, 1.0f, 0.0f, 1.0f, 120);                                       // zaklapnú – ostane spodné viečko a plamene
  flStep(F, 1.0f, 0.0f, 1.0f, 900);
  flStep(F, 0.0f, 0.0f, 1.0f, 200);
  flStep(F, 0.0f, -14.0f, 1.0f, 600); flStep(F, 0.0f, -14.0f, 1.0f, 800);
  flStep(F, 0.0f, 14.0f, 1.0f, 900);  flStep(F, 0.0f, 14.0f, 1.0f, 600);
  flBlink(F, 14.0f, 1.0f);
  flStep(F, 0.0f, 0.0f, 1.0f, 500);   flStep(F, 0.0f, 0.0f, 1.0f, 800);
  flBlink(F, 0.0f, 1.0f); flBlink(F, 0.0f, 1.0f);                         // dvojité žmurknutie
  flStep(F, 0.45f, 0.0f, 1.0f, 400);  flStep(F, 0.45f, 0.0f, 1.0f, 1300); // podozrievavo prižmúrené
  flStep(F, 1.0f, 0.0f, 1.0f, 140);                                       // zaklapnú, plamene horia ďalej
  flStep(F, 1.0f, 0.0f, 0.35f, 900);                                      // plamene sa zmenšia na jazýčky
  gagBacklightFade(1.0f, 0.4f, 500);  delay(600);                         // stmavnú
  flStep(F, 1.0f, 0.0f, 0.0f, 400);   delay(300);
  gagBacklightFade(0.4f, 0.0f, 300);                                      // úplná tma, v nej bežné oči
  gagRenderRect(leftGFX(),  petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeNormalEye, nullptr);
  gagRenderRect(rightGFX(), petBufR, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeNormalEye, nullptr);
  gagEndNormal();
  gagBacklightFade(0.0f, 1.0f, GAG_BL_MS);
}

/* =====================================================================
 *  GAG 49: „Six-seven“ – ako v kreslených rozprávkach (doláre v očiach)
 *  Zrenice zmiznú, v očiach sa roztočia čísla ako na automate, ľavé
 *  zastane na 6, pravé na 7, cinkne to a čísla potom striedavo
 *  poskakujú hore-dole ako pri geste „six-seven“.
 * ===================================================================== */
#define SIX_SC    13                       // veľkosť bodky písma (číslica 65 x 91 px)
#define SIX_CELL  (9 * SIX_SC)             // výška jedného políčka na „valci“
static uint16_t shadeSixSeven(int x, int y, const void *c){
  const SixCtx *s = (const SixCtx *)c;
  const int lx = x - (CX - (5 * SIX_SC) / 2);
  if (lx >= 0 && lx < 5 * SIX_SC){
    const float v = (float)(y - CY - s->yoff) + s->pos * SIX_CELL;   // poloha na valci
    const int k = (int)floorf((v + SIX_CELL * 0.5f) / SIX_CELL);
    const int ly = (int)lroundf(v - k * SIX_CELL) + (7 * SIX_SC) / 2;
    if (s->only && k != (int)lroundf(s->pos)) return COL_SCLERA;           // po zastavení len jedno číslo
    int digit = (s->base - k) % 10; if (digit < 0) digit += 10;
    if (ly >= 0 && ly < 7 * SIX_SC){
      const int col = lx / SIX_SC, row = ly / SIX_SC;
      // „bublinkové“ písmo: kruhy v bunkách sa prekrývajú -> súvislá zaoblená číslica
      for (int dr = -1; dr <= 1; ++dr) for (int dc = -1; dc <= 1; ++dc){
        const int r = row + dr, cc = col + dc;
        if (r < 0 || r > 6 || cc < 0 || cc > 4) continue;
        if (!((quizFont[digit][r] >> (4 - cc)) & 1)) continue;
        const float dx = lx - (cc + 0.5f) * SIX_SC, dy = ly - (r + 0.5f) * SIX_SC;
        if (dx * dx + dy * dy <= (SIX_SC * 0.72f) * (SIX_SC * 0.72f)) return s->col;
      }
    }
  }
  return COL_SCLERA;
}
static inline void sixRect(int e, const SixCtx &s, int x0, int y0, int x1, int y1){
  gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, x0, y0, x1, y1, shadeSixSeven, &s);
}
static inline void sixColumn(int e, const SixCtx &s){                       // celý pás s valcom
  sixRect(e, s, CX - (5 * SIX_SC) / 2 - 2, eyeTopY, CX + (5 * SIX_SC) / 2 + 2, eyeBotY);
}
static void activity_gag_six_seven(){
  gagStartCentered();
  SixCtx S[2];
  const int target[2] = { 6, 7 };
  for (int e = 0; e < 2; ++e){ S[e].pos = 0; S[e].base = random(10); S[e].yoff = 0; S[e].col = COL_PUPIL; S[e].only = false; }
  // 1) v tme sa zrenice zmenia na čísla
  gagBlOut();
  for (int e = 0; e < 2; ++e) sixRect(e, S[e], CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY);
  gagBlIn();
  // 2) roztočenie: každý valec skončí na svojom čísle, pravý o chvíľu neskôr
  float endPos[2]; unsigned long dur[2] = { 2200, 2900 };
  for (int e = 0; e < 2; ++e){
    int turns = 22 + e * 6;                                               // koľko čísel prebehne
    // base - k = target pri k = pos (celé číslo) -> pos ≡ base - target (mod 10)
    int need = ((S[e].base - target[e]) % 10 + 10) % 10;
    endPos[e] = (float)(need + 10 * ((turns + 9) / 10));
  }
  const unsigned long t0 = millis();
  bool done[2] = { false, false };
  while (!done[0] || !done[1]){
    const unsigned long el = millis() - t0;
    for (int e = 0; e < 2; ++e){
      if (done[e]) continue;
      float f = (float)el / dur[e]; if (f >= 1.0f){ f = 1.0f; done[e] = true; }
      const float g = 1.0f - (1.0f - f) * (1.0f - f) * (1.0f - f);        // rýchly štart, pomalé dobehnutie
      float p = endPos[e] * g;
      if (f > 0.85f) p += 0.12f * sinf((f - 0.85f) / 0.15f * 6.2832f) * (1.0f - f) * 6.0f;   // malé „cvaknutie“
      if (done[e]) p = endPos[e];
      S[e].pos = p;
      sixColumn(e, S[e]);
      if (done[e]){                                                       // cink! susedné čísla zmiznú
        S[e].only = true; sixColumn(e, S[e]);
        gagBacklightFade(1.0f, 0.55f, 60); gagBacklightFade(0.55f, 1.0f, 120);
      }
    }
    delay(2);
  }
  delay(400);
  Serial1.println("G:SIX7");                                             // chápadlá: gesto 6-7, až keď sú čísla 6 a 7
  // 3) „six-seven“: čísla striedavo poskakujú hore-dole (ako ruky pri geste)
  const int AMP = 26;
  for (int k = 0; k <= 8; ++k){
    int from[2], to[2];
    for (int e = 0; e < 2; ++e){
      from[e] = S[e].yoff;
      to[e] = (k == 8) ? 0 : ((((k + e) & 1) ? 1 : -1) * AMP);            // posledný krok späť do stredu
    }
    const int T = (k == 8) ? 250 : 320;
    const unsigned long t1 = millis();
    for (;;){
      const unsigned long el = millis() - t1;
      const float f = (el >= (unsigned long)T) ? 1.0f : ease((float)el / T);
      for (int e = 0; e < 2; ++e){
        const int prev = S[e].yoff;
        S[e].yoff = (int)lroundf(from[e] + (to[e] - from[e]) * f);
        if (S[e].yoff != prev){
          const int ya = CY + min(prev, S[e].yoff) - (7 * SIX_SC) / 2 - 3, yb = CY + max(prev, S[e].yoff) + (7 * SIX_SC) / 2 + 3;
          sixRect(e, S[e], CX - (5 * SIX_SC) / 2 - 2, ya, CX + (5 * SIX_SC) / 2 + 2, yb);
        }
      }
      if (f >= 1.0f) break;
      delay(2);
    }
  }
  delay(1500);
  Serial1.println("G:NONE");                                             // chápadlá do pokoja
  gagBl = 2;
  gagEndNormal();                                                           // späť cez podsvietenie
}

/* =====================================================================
 *  GAG 50: Autíčko robí looping
 *  Oči sa zmenia na kaskadérsku dráhu so slučkou po okraji oka. Oranžové
 *  autíčko prifrčí zľava, prejde slučku (hore jazdí hlavou dole), vyletí
 *  doprava do druhého oka a tam to zopakuje – dvakrát dokola.
 *  Kreslí sa len okolie autíčka (stará aj nová poloha).
 * ===================================================================== */
#define CAR_W      44                     // rozmery autíčka (sprite)
#define CAR_H      24
#define LOOP_CY    (CY - 12)              // stred slučky
#define LOOP_RIN   92                     // vnútorný okraj dráhy (tu sú kolesá)
#define LOOP_ROUT  100
#define ROAD_Y     (CY + 80)              // vrch cesty (= spodok slučky)
#define CAR_LIFT   17                     // stred auta nad cestou
#define CAR_SC     1.45f                  // zväčšenie autíčka na displeji
#define CAR_BB     38                     // polovičný rozmer oblasti okolo auta
#define CAR_KEY    0x0821                 // „priehľadná“ farba v sprite
#define BALL_R     46                     // oko v strede slučky
#define BALL_PR    20                     // jeho zrenica
#define BALL_LOOK  22                     // ako ďaleko sa zrenica pohne za autom
static uint16_t carSprite[CAR_W * CAR_H];
static bool carReady = false;
static uint16_t loopSky[240];
static void carBuild(){
  for (int y = 0; y < 240; ++y){                                       // obloha: tmavofialová -> oranžová pri zemi
    const float t = clamp01((float)(y - eyeTopY) / 200.0f);
    loopSky[y] = rgb565((int)(30 + 200 * t * t), (int)(10 + 70 * t * t), (int)(60 - 30 * t));
  }
  if (carReady) return;
  for (int j = 0; j < CAR_H; ++j) for (int i = 0; i < CAR_W; ++i){
    uint16_t c = CAR_KEY;
    const float x = i - CAR_W / 2.0f + 0.5f, y = j - CAR_H / 2.0f + 0.5f;   // y > 0 = smer ku kolesám (dole)
    // karoséria
    if (y > -2 && y < 7 && fabsf(x) < 21) c = rgb565(255, 120, 10);
    if (y > -10 && y <= -2 && x > -12 && x < 9 && (x - 9) * (x - 9) + 0 >= 0){
      const float slope = (x > 0) ? (x - 1) * 0.9f : (-x - 6) * 0.9f;     // šikmé okná
      if (-y > slope * 0.8f - 6 && y > -10) c = rgb565(255, 120, 10);
      if (y > -8 && y < -3 && x > -9 && x < 6 && -y > slope * 0.8f - 4) c = rgb565(150, 220, 255);   // okno
    }
    if (y > 0 && y < 2 && fabsf(x) < 21) c = rgb565(30, 30, 30);         // čierny pruh
    if (x > 18 && x < 21 && y > -1 && y < 3) c = rgb565(255, 250, 150);  // svetlo vpredu
    if (x < -18 && x > -21 && y > -1 && y < 3) c = rgb565(220, 20, 20);  // zadné svetlo
    // kolesá
    for (int w = -1; w <= 1; w += 2){
      const float wx = x - w * 12.0f, wy = y - 7.0f, d2 = wx * wx + wy * wy;
      if (d2 <= 30) c = (d2 <= 6) ? rgb565(170, 170, 170) : rgb565(15, 15, 15);
    }
    carSprite[j * CAR_W + i] = c;
  }
  carReady = true;
}
static uint16_t shadeLoop(int x, int y, const void *c){
  const LoopCtx *k = (const LoopCtx *)c;
  // autíčko (otočený sprite)
  if (k->vis){
    const float dx = x - k->cx, dy = y - k->cy;
    if (fabsf(dx) <= CAR_BB && fabsf(dy) <= CAR_BB){
      const float lx = (dx * k->ca + dy * k->sa) * (1.0f / CAR_SC), ly = (-dx * k->sa + dy * k->ca) * (1.0f / CAR_SC);   // do súradníc auta
      const int i = (int)floorf(lx + CAR_W / 2.0f), j = (int)floorf(ly + CAR_H / 2.0f);
      if (i >= 0 && i < CAR_W && j >= 0 && j < CAR_H){
        const uint16_t s = carSprite[j * CAR_W + i];
        if (s != CAR_KEY) return s;
      }
    }
  }
  // oko v strede slučky – zrenica sleduje autíčko
  { const int bx = x - CX, by = y - LOOP_CY, b2 = bx * bx + by * by;
    if (b2 <= BALL_R * BALL_R){
      if (b2 >= (BALL_R - 2) * (BALL_R - 2)) return rgb565(40, 20, 50);                 // tmavý obrys
      if (k->lid > 0.0f){                                                                   // viečka (žmurkanie)
        const int yT = LOOP_CY - BALL_R + (int)lroundf(k->lid * (BALL_R + 1));
        const int yB = LOOP_CY + BALL_R - (int)lroundf(k->lid * (BALL_R - 1));
        if (y <= yT) return (y >= yT - 2) ? COL_LID_SEAM : COL_LID_TOP;
        if (y >= yB) return (y <= yB + 2) ? COL_LID_SEAM : COL_LID_BOT;
      }
      const int px = x - k->px, py = y - k->py, p2 = px * px + py * py;
      if (p2 <= BALL_PR * BALL_PR){
        const int hx = px - 7, hy = py + 7;
        return (hx * hx + hy * hy <= 36) ? COL_HILITE : COL_PUPIL;
      }
      return COL_SCLERA;
    } }
  // cesta dole
  if (y >= ROAD_Y && y <= ROAD_Y + 9){
    if (y >= ROAD_Y + 4 && y <= ROAD_Y + 5 && ((x / 10) & 1)) return COL_SCLERA;    // prerušovaná čiara
    return rgb565(70, 70, 80);
  }
  if (y > ROAD_Y + 9) return rgb565(25, 15, 10);                          // zem
  // slučka
  const int ddx = x - CX, ddy = y - LOOP_CY, d2 = ddx * ddx + ddy * ddy;
  if (d2 >= LOOP_RIN * LOOP_RIN && d2 <= LOOP_ROUT * LOOP_ROUT){
    if (d2 >= (LOOP_RIN + 3) * (LOOP_RIN + 3) && d2 <= (LOOP_RIN + 4) * (LOOP_RIN + 4)) return rgb565(255, 220, 60);   // žltý pás
    return rgb565(70, 70, 80);
  }
  return loopSky[y];
}
static inline void loopRect(int e, const LoopCtx &k, int x0, int y0, int x1, int y1){
  gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, x0, y0, x1, y1, shadeLoop, &k);
}
// poloha auta podľa prejdenej dráhy s (px): nájazd po ceste, slučka, výjazd
static void loopPose(float s, int loops, LoopCtx &k){
  const float rp = LOOP_RIN - CAR_LIFT;                                  // polomer dráhy stredu auta
  const float A = CX + 50.0f;                                            // dĺžka nájazdu (od x = -50)
  const float Lc = 6.2832f * rp * loops;
  k.vis = true;
  if (s < A){ k.cx = -50.0f + s; k.cy = ROAD_Y - CAR_LIFT; k.ca = 1; k.sa = 0; return; }
  if (s < A + Lc){
    const float phi = (s - A) / rp;                                      // 0 = dole, ide doprava a hore
    k.cx = CX + rp * sinf(phi); k.cy = LOOP_CY + rp * cosf(phi);
    k.ca = cosf(-phi); k.sa = sinf(-phi);
    return;
  }
  k.cx = CX + (s - A - Lc); k.cy = ROAD_Y - CAR_LIFT; k.ca = 1; k.sa = 0;
  if (k.cx > CX + EYE_R + 45) k.vis = false;
}
static void loopDrive(int e, LoopCtx *K, int loops, float v0){
  const float rp = LOOP_RIN - CAR_LIFT;
  const float total = CX + 50.0f + 6.2832f * rp * loops + EYE_R + 50.0f;
  float s = 0; unsigned long last = millis();
  LoopCtx o = K[e]; o.vis = false;
  for (;;){
    const unsigned long now = millis(); const float dt = (now - last) * 0.001f; last = now;
    LoopCtx n = K[e];
    loopPose(s, loops, n);
    // v slučke hore spomalí, dole zrýchli (ako naozaj)
    const float A = CX + 50.0f;
    float v = v0;
    if (s > A && s < A + 6.2832f * rp * loops) v = v0 * (0.70f + 0.30f * (0.5f + 0.5f * cosf((s - A) / rp)));
    s += v * dt;
    // prekresli starú aj novú polohu
    float xa = 1e9f, xb = -1e9f, ya = 1e9f, yb = -1e9f;
    const LoopCtx *st[2] = { &o, &n };
    for (int q = 0; q < 2; ++q) if (st[q]->vis){ xa = fminf(xa, st[q]->cx); xb = fmaxf(xb, st[q]->cx); ya = fminf(ya, st[q]->cy); yb = fmaxf(yb, st[q]->cy); }
    // zrenice: v tomto oku sa pozerá na auto, v druhom oku smerom k nemu (cez nos)
    for (int q = 0; q < 2; ++q){
      float tx = 0, ty = 0;
      if (n.vis){
        if (q == e){ tx = n.cx - CX; ty = n.cy - LOOP_CY; }
        else { tx = (q == 0 ? 1.0f : -1.0f) * noseSideL() * 120.0f; ty = n.cy - LOOP_CY; }
        const float d = sqrtf(tx * tx + ty * ty); if (d > 1.0f){ tx = tx / d * BALL_LOOK; ty = ty / d * BALL_LOOK; }
      }
      const int npx = (int)lroundf(CX + tx), npy = (int)lroundf(LOOP_CY + ty);
      LoopCtx &kq = (q == e) ? n : K[q];
      if (npx != kq.px || npy != kq.py){
        kq.px = npx; kq.py = npy;
        loopRect(q, kq, CX - BALL_R - 1, LOOP_CY - BALL_R - 1, CX + BALL_R + 1, LOOP_CY + BALL_R + 1);
      }
    }
    if (xb > -1e9f) loopRect(e, n, (int)xa - CAR_BB - 1, (int)ya - CAR_BB - 1, (int)xb + CAR_BB + 1, (int)yb + CAR_BB + 1);
    o = n; K[e] = n;
    if (s >= total) break;
    delay(1);
  }
  K[e].vis = false;
}
static void activity_gag_car_loop(){
  gagStartCentered();
  carBuild();
  LoopCtx K[2];
  for (int e = 0; e < 2; ++e){ K[e].vis = false; K[e].cx = -100; K[e].cy = 0; K[e].ca = 1; K[e].sa = 0; K[e].px = CX; K[e].py = LOOP_CY; K[e].lid = 0.0f; }
  gagBlOut();
  for (int e = 0; e < 2; ++e) loopRect(e, K[e], CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY);
  gagBlIn();
  delay(600);
  loopDrive(0, K, 1, 260.0f);                                             // ľavé oko: jeden looping
  delay(250);
  loopDrive(1, K, 2, 320.0f);                                             // pravé oko: dva loopingy, rýchlejšie
  delay(400);
  // auto je preč – oči zaklipkajú (dvakrát rýchlo, potom ešte raz pomaly)
  static const int blinkMs[3][2] = { { 90, 110 }, { 90, 110 }, { 220, 300 } };
  for (int b = 0; b < 3; ++b){
    for (int ph = 0; ph < 2; ++ph){
      const unsigned long t0 = millis(); const int T = blinkMs[b][ph];
      for (;;){
        const unsigned long el = millis() - t0;
        const float f = (el >= (unsigned long)T) ? 1.0f : ease((float)el / T);
        const float lid = roundf((ph == 0 ? f : 1.0f - f) * 40.0f) / 40.0f;
        for (int e = 0; e < 2; ++e) if (K[e].lid != lid){
          K[e].lid = lid;
          loopRect(e, K[e], CX - BALL_R - 1, LOOP_CY - BALL_R - 1, CX + BALL_R + 1, LOOP_CY + BALL_R + 1);
        }
        if (f >= 1.0f) break;
        delay(2);
      }
      if (ph == 0) delay(b == 2 ? 120 : 40);
    }
    delay(b == 1 ? 600 : 150);
  }
  delay(600);
  gagEndNormal();                                                         // späť cez podsvietenie
}

/* =====================================================================
 *  GAG 51: Odpočet a ohňostroj
 *  Zrenice sa začnú triasť, preblikne veľká 3, trasú sa viac, 2, ešte
 *  viac, 1 – zrenice napuchnú a vybuchnú ako ohňostroj. Potom v tmavej
 *  noci vybuchne ešte niekoľko rakiet. Iskry sa kreslia ako malé bodky
 *  so stopou (žiadne prekresľovanie celého oka).
 * ===================================================================== */
#define FW_N     80                       // iskier na jeden výbuch
#define FW_MAX   (FW_N * 2)               // naraz najviac dva výbuchy v oku
struct FwSpark { float x, y, vx, vy; int16_t px[3], py[3]; uint16_t col; uint16_t life, age; bool on; };
static FwSpark fwS[2][FW_MAX];
static inline bool fwInEye(int x, int y){ return y >= eyeTopY + 2 && y <= eyeBotY - 2 && x >= xLeft[y] + 2 && x <= xRight[y] - 2; }
static inline void fwDot(Arduino_GFX *gx, int x, int y, int r, uint16_t c){
  if (x < 0 || fwInEye(x, y) == false) return;
  gx->fillRect(x - r, y - r, 2 * r + 1, 2 * r + 1, c);
}
static void fwBurst(int e, float cx, float cy, uint16_t c1, uint16_t c2, float power){
  int made = 0;
  for (int i = 0; i < FW_MAX && made < FW_N; ++i){
    FwSpark &s = fwS[e][i]; if (s.on) continue;
    const float a = random(6283) * 0.001f, v = power * (0.45f + 0.55f * random(1000) * 0.001f);
    s.x = cx; s.y = cy; s.vx = cosf(a) * v; s.vy = sinf(a) * v;
    for (int k = 0; k < 3; ++k){ s.px[k] = -100; s.py[k] = -100; }
    s.col = (random(3) == 0) ? c2 : c1; s.life = 38 + random(22); s.age = 0; s.on = true; ++made;
  }
}
// jeden krok iskier v oku e: pohyb, gravitácia, stopa (jasná – tmavšia – zmazať)
static bool fwStep(int e, uint16_t bg){
  Arduino_GFX *gx = e ? rightGFX() : leftGFX();
  bool any = false;
  for (int i = 0; i < FW_MAX; ++i){
    FwSpark &s = fwS[e][i]; if (!s.on) continue;
    any = true;
    fwDot(gx, s.px[2], s.py[2], 2, bg);                                       // najstaršia stopa preč
    s.px[2] = s.px[1]; s.py[2] = s.py[1]; s.px[1] = s.px[0]; s.py[1] = s.py[0];
    s.x += s.vx; s.y += s.vy; s.vy += 0.09f; s.vx *= 0.97f; s.vy *= 0.97f; ++s.age;
    const float fade = 1.0f - (float)s.age / s.life;
    const uint16_t c = lerp565(bg, s.col, fade);
    if (s.age >= s.life){                                                      // dohasla: zmazať celú stopu
      fwDot(gx, s.px[1], s.py[1], 2, bg); fwDot(gx, s.px[2], s.py[2], 2, bg);
      s.on = false; continue;
    }
    fwDot(gx, s.px[1], s.py[1], 2, lerp565(bg, s.col, fade * 0.45f));         // stopa
    s.px[0] = (int16_t)lroundf(s.x); s.py[0] = (int16_t)lroundf(s.y);
    fwDot(gx, s.px[0], s.py[0], 2, (random(7) == 0) ? COL_SCLERA : c);        // iskra (občas zablikne)
  }
  return any;
}
// trasúce sa zrenice: prekreslí sa okolie starej aj novej polohy (žiadne stopy ani pri veľkom trasení)
struct FwPup { int x, y, r; };
static FwPup fwP[2] = { { CX, CY, PUPIL_R }, { CX, CY, PUPIL_R } };
static uint16_t shadeFwPupil(int x, int y, const void *c){
  const FwPup *p = (const FwPup *)c; uint16_t o;
  if (gagPupilPix(x, y, p->x, p->y, p->r, o)) return o;
  return COL_SCLERA;
}
static void fwPupilTo(int e, int x, int y, int r){
  const FwPup o = fwP[e]; fwP[e].x = x; fwP[e].y = y; fwP[e].r = r;
  const int m = max(o.r, r) + 1;
  gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, min(o.x, x) - m, min(o.y, y) - m, max(o.x, x) + m, max(o.y, y) + m, shadeFwPupil, &fwP[e]);
}
static void fwShake(int amp, int ms){
  const unsigned long t0 = millis();
  while (millis() - t0 < (unsigned long)ms){
    for (int e = 0; e < 2; ++e) fwPupilTo(e, CX + random(-amp, amp + 1), CY + random(-amp, amp + 1), PUPIL_R);
    delay(20);
  }
  for (int e = 0; e < 2; ++e) fwPupilTo(e, CX, CY, PUPIL_R);
}
static void fwNumber(int n, int ms){                                         // preklik: veľké číslo v oku
  SixCtx S; S.pos = 0; S.base = n; S.yoff = 0; S.col = COL_PUPIL; S.only = true;
  for (int e = 0; e < 2; ++e) sixRect(e, S, CX - 49, CY - 52, CX + 49, CY + 52);
  delay(ms);
  for (int e = 0; e < 2; ++e){ fwP[e].x = CX; fwP[e].y = CY; fwP[e].r = PUPIL_R;
    gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - 49, CY - 52, CX + 49, CY + 52, shadeFwPupil, &fwP[e]); }
}
static uint16_t shadeNightBg(int, int, const void *){ return COL_NIGHT_SKY; }
static void activity_gag_fireworks(){
  gagStartCentered();
  for (int e = 0; e < 2; ++e){ fwP[e].x = CX; fwP[e].y = CY; fwP[e].r = PUPIL_R; }
  // 1) odpočet so stále silnejším trasením
  fwShake(2, 1400); fwNumber(3, 650);
  fwShake(4, 1300); fwNumber(2, 650);
  fwShake(7, 1200); fwNumber(1, 650);
  fwShake(11, 900);
  // 2) zrenice napuchnú
  for (int r = PUPIL_R; r <= 50; r += 2){
    for (int e = 0; e < 2; ++e) fwPupilTo(e, CX + random(-3, 4), CY + random(-3, 4), r);
    delay(20);
  }
  // 3) BUM: záblesk a v tme výbuch zreníc
  Serial1.println("G:CHEER2");                                           // hurá-hurá: ruky striedavo hore-dole
  gagBacklightFade(1.0f, 0.0f, 60);
  const uint16_t bg = COL_NIGHT_SKY;
  for (int e = 0; e < 2; ++e) gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeNightBg, nullptr);
  for (int e = 0; e < 2; ++e) for (int i = 0; i < FW_MAX; ++i) fwS[e][i].on = false;
  fwBurst(0, CX, CY, rgb565(255, 210, 60), rgb565(255, 255, 255), 5.0f);
  fwBurst(1, CX, CY, rgb565(255, 60, 120), rgb565(255, 220, 80), 5.0f);
  analogWrite(leftBLKPin(), leftEyeBrightness); analogWrite(rightBLKPin(), rightEyeBrightness);   // okamžite plný jas
  // 4) ďalšie rakety v náhodných miestach
  static const uint16_t pal[][2] = { { 0x07FF, 0xFFFF }, { 0xF81F, 0xFFE0 }, { 0x07E0, 0xFFFF }, { 0xFD20, 0xF800 }, { 0x5D7F, 0xFFFF } };
  unsigned long next[2] = { millis() + 900, millis() + 1300 };
  int bursts[2] = { 0, 0 };
  const unsigned long t0 = millis();
  for (;;){
    bool any = false;
    for (int e = 0; e < 2; ++e){
      if (bursts[e] < 3 && millis() >= next[e]){
        const int k = random(5);
        fwBurst(e, CX + random(-45, 46), CY + random(-55, 15), pal[k][0], pal[k][1], 3.6f);
        ++bursts[e]; next[e] = millis() + 700 + random(600);
      }
      any |= fwStep(e, bg);
    }
    if (!any && bursts[0] >= 3 && bursts[1] >= 3) break;
    if (millis() - t0 > 12000) break;                                        // poistka
    delay(16);
  }
  delay(400);
  gagBl = 2;
  gagEndNormal();                                                           // späť cez podsvietenie
}

/* =====================================================================
 *  GAG 52: Panáčik a kanál
 *  Oči stmavnú, dolu cez ne vedie biela čiara – zem – a na pravom konci
 *  každého oka je v nej kanál. Kreslený panáčik (biela čiara na čiernom)
 *  prejde oko, nad kanálom sa zastaví, pozrie dole a spadne doň. Potom
 *  spadne „z neba“ do druhého oka, prejde ho a znova do kanála – a tak
 *  dookola, 2× cez obe oči. Naposledy sa zastaví pred kanálom, zakýva
 *  a skočí doň. Prekresľuje sa len obdĺžnik okolo panáčika.
 * ===================================================================== */
#define WK_GY      (CY + 40)              // výška zeme
#define WK_CL      (CX + ((WK_STYLE == 1) ? 28 : 38))   // ľavá stena kanála
#define WK_CR      (CX + ((WK_STYLE == 1) ? 94 : 86))   // pravá stena kanála
#define WK_CM      ((WK_CL + WK_CR) / 2)  // stred kanála
#define WK_LAND    (CX - 50)              // kam dopadne z neba
#define WK_INK     0xFFFF                 // farba čiary
#define WK_STYLE   1                      // 1 = tekvička na nožičkách, 0 = panáčik s bruškom
#define WK_S       ((WK_STYLE == 1) ? 1.6f : 1.45f)   // veľkosť postavičky
#if WK_STYLE == 1                         // farby tekvičky
#define WK_C_PUMP  0xFBE0                 // oranžová
#define WK_C_PUMPL 0xBA20                 // obrys tekvice
#define WK_C_RIB   0xD2C0                 // rebrá
#define WK_C_STEM  0x44A5                 // zelená stopka
#define WK_C_STEML 0x1AE2
#define WK_C_LIMB  0x44A5                 // ručičky a nožičky (zelené ako stonka)
#define WK_C_LIMBL 0x1AE2
#define WK_C_SHOE  0x79E2                 // hnedé topánky
#define WK_C_SHOEL 0x4101
#define WK_C_GLOW  0xFF07                 // žiariace oči a ústa
#else                                     // panáčik: biela čiara na čiernom
#define WK_C_LIMB  COL_BG
#define WK_C_LIMBL WK_INK
#define WK_C_SHOE  COL_BG
#define WK_C_SHOEL WK_INK
#endif
enum { WK_WALK = 0, WK_STAND, WK_FALL, WK_SQUASH, WK_WAVE };
enum { WK_LOOK_RIGHT = 0, WK_LOOK_US, WK_LOOK_DOWN, WK_LOOK_LEFT };
enum { WK_CAP = 0, WK_CIRC, WK_ELL, WK_LINE, WK_ERING, WK_DOT, WK_TRI, WK_EFILL };   // tvary / detaily
#define WK_HW 1.9f                        // polovičná hrúbka obrysu
static WkPrim wkG[4];
static uint8_t wkGN = 0;
static WalkCtx wkK[2];
static WalkCtx *wkCur = nullptr;
static uint16_t wkCol = WK_INK;          // farba ďalšieho detailu
static int16_t wkBox[2][4];

static void wkPrimBox(WkPrim &p){
  float x0 = p.ax, y0 = p.ay, x1 = p.ax, y1 = p.ay, m;
  if (p.kind == WK_CAP || p.kind == WK_LINE){ x0 = fminf(p.ax, p.bx); x1 = fmaxf(p.ax, p.bx); y0 = fminf(p.ay, p.by); y1 = fmaxf(p.ay, p.by); m = p.r; }
  else if (p.kind == WK_TRI){ x0 = fminf(p.ax, fminf(p.bx, p.r)); x1 = fmaxf(p.ax, fmaxf(p.bx, p.r)); y0 = fminf(p.ay, fminf(p.by, p.r2)); y1 = fmaxf(p.ay, fmaxf(p.by, p.r2)); m = 0.0f; }
  else if (p.kind == WK_ELL || p.kind == WK_ERING || p.kind == WK_EFILL) m = fmaxf(p.r, p.r2);
  else m = p.r;
  m += WK_HW + 1.5f;
  p.x0 = (int16_t)floorf(x0 - m); p.x1 = (int16_t)ceilf(x1 + m);
  p.y0 = (int16_t)floorf(y0 - m); p.y1 = (int16_t)ceilf(y1 + m);
}
static void wkAdd(WkPrim *a, uint8_t &n, uint8_t kind, float ax, float ay, float bx, float by, float r, float r2){
  WkPrim &p = a[n++];
  p.kind = kind; p.ax = ax; p.ay = ay; p.bx = bx; p.by = by; p.r = r; p.r2 = r2; p.col = wkCol;
  wkPrimBox(p);
}
static inline float wkSegD(const WkPrim &p, int x, int y){        // vzdialenosť od úsečky a–b
  const float vx = p.bx - p.ax, vy = p.by - p.ay, wx = x - p.ax, wy = y - p.ay;
  const float L = vx * vx + vy * vy;
  float t = (L > 0.0f) ? (wx * vx + wy * vy) / L : 0.0f;
  t = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
  const float ex = wx - t * vx, ey = wy - t * vy;
  return sqrtf(ex * ex + ey * ey);
}
static inline float wkEllD(float dx, float dy, float rx, float ry){  // približná vzdialenosť od elipsy
  return (sqrtf(dx * dx / (rx * rx) + dy * dy / (ry * ry)) - 1.0f) * fminf(rx, ry);
}
static inline float wkEdge(float px, float py, float ax, float ay, float bx, float by){   // >0 = mimo (vľavo od a→b)
  const float ex = bx - ax, ey = by - ay, L = sqrtf(ex * ex + ey * ey);
  return ((px - ax) * ey - (py - ay) * ex) / L;
}
static inline float wkSdf(const WkPrim &p, int x, int y){            // tvar: < 0 vnútri
  if (p.kind == WK_CAP) return wkSegD(p, x, y) - p.r;
  const float dx = x - p.ax, dy = y - p.ay;
  if (p.kind == WK_CIRC) return sqrtf(dx * dx + dy * dy) - p.r;
  return wkEllD(dx, dy, p.r, p.r2);
}
static inline float wkStroke(const WkPrim &p, int x, int y){         // detail: pokrytie 0..1
  if (x < p.x0 || x > p.x1 || y < p.y0 || y > p.y1) return 0.0f;
  float d;
  if (p.kind == WK_LINE) d = wkSegD(p, x, y) - p.r;
  else if (p.kind == WK_ERING) d = fabsf(wkEllD(x - p.ax, y - p.ay, p.r, p.r2)) - p.bx;
  else if (p.kind == WK_EFILL) d = wkEllD(x - p.ax, y - p.ay, p.r, p.r2);
  else if (p.kind == WK_TRI){                                        // trojuholník a, b, (r, r2) v smere hodín
    d = wkEdge(x, y, p.ax, p.ay, p.bx, p.by);
    d = fmaxf(d, wkEdge(x, y, p.bx, p.by, p.r, p.r2));
    d = fmaxf(d, wkEdge(x, y, p.r, p.r2, p.ax, p.ay));
  }
  else { const float dx = x - p.ax, dy = y - p.ay; d = sqrtf(dx * dx + dy * dy) - p.r; }
  const float c = 0.5f - d;
  return (c <= 0.0f) ? 0.0f : ((c >= 1.0f) ? 1.0f : c);
}
static inline float wkClamp01(float v){ return (v <= 0.0f) ? 0.0f : ((v >= 1.0f) ? 1.0f : v); }
/* Postavička je z vrstiev (nohy, topánky, ruky, telo). Každá vrstva má
 * výplň a obrys; vnútro zakryje všetko, čo je za ňou. Detaily (oči, ústa…)
 * sa kreslia cez svoju vrstvu. Farby sa skladajú odpredu dozadu. */
static uint16_t shadeWalk(int x, int y, const void *c){
  const WalkCtx *k = (const WalkCtx *)c;
  uint16_t oc[24]; float oa[24]; int no = 0;                   // vrstvy farieb nad základom (odpredu)
  uint16_t base = COL_BG; bool hit = false;
  if (k->fig) for (int li = k->nl - 1; li >= 0 && !hit; --li){
    const WkLayer &L = k->L[li];
    if (x < L.x0 || x > L.x1 || y < L.y0 || y > L.y1) continue;
    float d = 1e9f;
    for (int i = L.first; i < L.first + L.ns; ++i){
      const WkPrim &p = k->p[i];
      if (x < p.x0 || x > p.x1 || y < p.y0 || y > p.y1) continue;
      const float q = wkSdf(p, x, y); if (q < d) d = q;
    }
    for (int i = L.first + L.ns; i < L.first + L.n && no < 23; ++i){
      const float q = wkStroke(k->p[i], x, y);
      if (q > 0.0f){ oc[no] = k->p[i].col; oa[no] = q; ++no; }
    }
    const float co = wkClamp01(WK_HW + 0.5f - fabsf(d));
    if (d < 0.0f){                                               // vnútri: výplň + obrys, za tým už nič
      base = (co > 0.0f) ? lerp565(L.fill, L.line, co) : L.fill;
      hit = true;
    } else if (co > 0.0f && no < 23){ oc[no] = L.line; oa[no] = co; ++no; }
  }
  if (!hit){                                                     // pozadie: čierna + zem
    float v = 0.0f;
    for (int i = 0; i < wkGN && v < 1.0f; ++i){ const float q = wkStroke(wkG[i], x, y); if (q > v) v = q; }
    base = (v <= 0.0f) ? COL_BG : ((v >= 1.0f) ? WK_INK : lerp565(COL_BG, WK_INK, v));
  }
  for (int i = no - 1; i >= 0; --i) base = (oa[i] >= 1.0f) ? oc[i] : lerp565(base, oc[i], oa[i]);
  return base;
}
static void wkBuildGround(){
  wkGN = 0; wkCol = WK_INK;
  wkAdd(wkG, wkGN, WK_LINE, CX - EYE_R - 4, WK_GY, WK_CL, WK_GY, 1.8f, 0);         // zem po kanál
  wkAdd(wkG, wkGN, WK_LINE, WK_CR, WK_GY, CX + EYE_R + 4, WK_GY, 1.8f, 0);         // zem za kanálom
  wkAdd(wkG, wkGN, WK_LINE, WK_CL, WK_GY, WK_CL, eyeBotY + 4, 1.8f, 0);            // steny kanála
  wkAdd(wkG, wkGN, WK_LINE, WK_CR, WK_GY, WK_CR, eyeBotY + 4, 1.8f, 0);
}
static void wkBegin(uint16_t fill, uint16_t line){
  WkLayer &L = wkCur->L[wkCur->nl++]; L.first = wkCur->n; L.ns = 0; L.n = 0; L.fill = fill; L.line = line; wkCol = line;
}
static void wkShape(uint8_t kind, float ax, float ay, float bx, float by, float r, float r2){
  WkLayer &L = wkCur->L[wkCur->nl - 1];
  wkAdd(wkCur->p, wkCur->n, kind, ax, ay, bx, by, r, r2); ++L.ns; ++L.n;
}
static void wkDetail(uint8_t kind, float ax, float ay, float bx, float by, float r, float r2){
  WkLayer &L = wkCur->L[wkCur->nl - 1];
  wkAdd(wkCur->p, wkCur->n, kind, ax, ay, bx, by, r, r2); ++L.n;
}
static void wkEnd(){
  WkLayer &L = wkCur->L[wkCur->nl - 1];
  L.x0 = L.y0 = 9999; L.x1 = L.y1 = -9999;
  for (int i = L.first; i < L.first + L.n; ++i){
    const WkPrim &p = wkCur->p[i];
    if (p.x0 < L.x0) L.x0 = p.x0; if (p.y0 < L.y0) L.y0 = p.y0;
    if (p.x1 > L.x1) L.x1 = p.x1; if (p.y1 > L.y1) L.y1 = p.y1;
  }
}

/* Póza panáčika. fx = stred, fy = spodok topánok, ph = fáza kroku, t = čas v ms
 * (trepanie pri páde, mávanie). Uhly končatín: 0 = kolmo dole, + = vpravo. */
static void wkPose(int e, float fx, float fy, uint8_t mode, float ph, unsigned long t, uint8_t look, bool oMouth){
  WalkCtx &k = wkK[e]; k.n = 0; k.nl = 0; k.fig = true; wkCur = &k;
  const float S = WK_S;
  const bool front = (look == WK_LOOK_US) || mode == WK_FALL || mode == WK_SQUASH || mode == WK_WAVE;
#if WK_STYLE == 1     // tekvica: veľké telo-hlava, krátke nožičky a ručičky
  const float TH = 6.5f * S, SH = 6.0f * S, UA = 6.0f * S, FA = 5.0f * S, SHOE = 2.4f * S, SHOEW = 4.2f * S;
  const float LR1 = 2.8f * S, LR2 = 2.6f * S, AR1 = 2.3f * S, AR2 = 2.1f * S, HAND = 2.8f * S, FALLA = 2.8f, LEAN = 0.8f;
  const float hipS = front ? 7.0f * S : 2.0f * S, shS = front ? 17.0f * S : 14.5f * S;
#else                 // panáčik s bruškom
  const float TH = 11.0f * S, SH = 11.0f * S, UA = 9.0f * S, FA = 8.0f * S, SHOE = 2.8f * S, SHOEW = 5.0f * S;
  const float LR1 = 3.4f * S, LR2 = 3.1f * S, AR1 = 2.4f * S, AR2 = 2.2f * S, HAND = 3.4f * S, FALLA = 2.5f, LEAN = 1.5f;
  const float hipS = front ? 4.0f * S : 1.0f * S, shS = front ? 9.0f * S : 1.5f * S;
#endif
  const float tt = t * 0.001f;
  float la[2], lb[2], aa[2], ab[2], lean = 0.0f;
  for (int i = 0; i < 2; ++i){                                   // základ: stojí
    const float sd = i ? 1.0f : -1.0f;
    la[i] = 0.06f * sd; lb[i] = la[i]; aa[i] = (front ? 0.30f : 0.12f) * sd; ab[i] = aa[i] + 0.10f * sd;
  }
  if (mode == WK_WALK){
    lean = LEAN * S;
    for (int i = 0; i < 2; ++i){
      const float s = sinf(ph + i * 3.14159f), c = cosf(ph + i * 3.14159f);
      la[i] = 0.45f * s; lb[i] = la[i] - 0.75f * fmaxf(0.0f, c);
      aa[i] = -0.75f * s; ab[i] = aa[i] + 0.75f;
    }
  } else if (mode == WK_FALL){
    for (int i = 0; i < 2; ++i){
      const float sd = i ? 1.0f : -1.0f;
      la[i] = 0.30f * sd + 0.15f * sinf(tt * 14.0f + i * 2.0f); lb[i] = la[i] - 0.35f * sd;
      aa[i] = FALLA * sd + 0.25f * sinf(tt * 16.0f + i * 1.5f); ab[i] = aa[i] + 0.3f * sd;
    }
  } else if (mode == WK_SQUASH){
    for (int i = 0; i < 2; ++i){
      const float sd = i ? 1.0f : -1.0f;
      la[i] = 0.75f * sd; lb[i] = -0.5f * sd; aa[i] = 1.1f * sd; ab[i] = aa[i] + 0.4f * sd;
    }
  } else if (mode == WK_WAVE){
    aa[1] = 2.35f; ab[1] = 2.85f + 0.5f * sinf(tt * 10.0f);
  }
  float kx[2], ky[2], qx[2], qy[2], low = 0.0f;
  for (int i = 0; i < 2; ++i){
    const float sd = i ? 1.0f : -1.0f;
    kx[i] = sd * hipS + TH * sinf(la[i]); ky[i] = TH * cosf(la[i]);
    qx[i] = kx[i] + SH * sinf(lb[i]);    qy[i] = ky[i] + SH * cosf(lb[i]);
    if (qy[i] > low) low = qy[i];
  }
  const float hx = fx, hy = (mode == WK_FALL) ? fy - (TH + SH) - SHOE : fy - low - SHOE;   // stojí najnižšou topánkou
  const float fd = (look == WK_LOOK_LEFT) ? -1.0f : 1.0f;
#if WK_STYLE == 1
  const float cx = hx + lean, cy = hy - 11.0f * S;               // stred tekvice
  const float sy = cy + 1.0f * S;                                // ramená
#else
  const float cx = hx + lean * 1.3f, cy = hy - 29.0f * S;        // hlava
  const float sy = hy - 17.0f * S;                               // ramená
#endif
  // nohy (za telom)
  for (int i = 0; i < 2; ++i){
    const float sd = i ? 1.0f : -1.0f, ox = hx + sd * hipS;
    wkBegin(WK_C_LIMB, WK_C_LIMBL);
    wkShape(WK_CAP, ox, hy, hx + kx[i], hy + ky[i], LR1, 0);
    wkShape(WK_CAP, hx + kx[i], hy + ky[i], hx + qx[i], hy + qy[i], LR2, 0);
    wkEnd();
    wkBegin(WK_C_SHOE, WK_C_SHOEL);                              // topánka
    wkShape(WK_ELL, hx + qx[i] + (front ? sd * 1.5f * S : 2.2f * S * fd), hy + qy[i] + 0.4f * S, 0, 0, SHOEW, SHOE);
    wkEnd();
  }
  auto arm = [&](int i){
    const float sd = i ? 1.0f : -1.0f, sx = cx + sd * shS;
    const float ex = sx + UA * sinf(aa[i]), ey2 = sy + UA * cosf(aa[i]);
    const float wx = ex + FA * sinf(ab[i]), wy = ey2 + FA * cosf(ab[i]);
    wkBegin(WK_C_LIMB, WK_C_LIMBL);
    wkShape(WK_CAP, sx, sy, ex, ey2, AR1, 0);
    wkShape(WK_CAP, ex, ey2, wx, wy, AR2, 0);
    wkShape(WK_CIRC, wx, wy, 0, 0, HAND, 0);
    wkEnd();
  };
  auto body = [&](){
#if WK_STYLE == 1
    // --- tekvica: stopka (za tekvicou), oranžová tekvica s rebrom, žiariace oči a ústa
    wkBegin(WK_C_STEM, WK_C_STEML);
    wkShape(WK_CAP, cx + 0.5f * S, cy - 11.0f * S, cx + 2.5f * S, cy - 19.0f * S, 2.4f * S, 0);
    wkDetail(WK_LINE, cx + 2.5f * S, cy - 17.5f * S, cx + 7.0f * S, cy - 19.5f * S, 1.2f, 0);     // úponka
    wkDetail(WK_LINE, cx + 7.0f * S, cy - 19.5f * S, cx + 9.0f * S, cy - 16.5f * S, 1.2f, 0);
    wkEnd();
    wkBegin(WK_C_PUMP, WK_C_PUMPL);
    wkShape(WK_ELL, cx, cy, 0, 0, 18.0f * S, 14.5f * S);
    wkCol = WK_C_RIB;                                               // rebrá
    if (look == WK_LOOK_US) wkDetail(WK_ERING, cx, cy, 1.3f, 0, 11.5f * S, 14.3f * S);
    else { wkDetail(WK_ERING, cx - 8.0f * S * fd, cy, 1.3f, 0, 9.0f * S, 14.3f * S); wkDetail(WK_ERING, cx - 13.0f * S * fd, cy, 1.1f, 0, 4.0f * S, 13.5f * S); }
    wkCol = WK_C_GLOW;                                              // vyrezané a žiariace
    static const float eX[4][2] = { { 4.0f, 12.0f }, { -6.5f, 6.5f }, { 4.0f, 12.0f }, { -12.0f, -4.0f } };
    const float ey = cy - 3.0f * S + ((look == WK_LOOK_DOWN) ? 2.2f * S : 0.0f);
    for (int j = 0; j < 2; ++j){                                  // oči – trojuholníky
      const float ex = cx + eX[look][j] * S, w = 3.6f * S;
      wkDetail(WK_TRI, ex - w, ey + 2.2f * S, ex, ey - 3.4f * S, ex + w, ey + 2.2f * S);
    }
    static const float mX[4] = { 8.0f, 0.0f, 8.0f, -8.0f };
    const float mx = cx + mX[look] * S, my = cy + ((look == WK_LOOK_DOWN) ? 7.0f : 5.8f) * S;
    if (oMouth) wkDetail(WK_EFILL, mx, my, 0, 0, 3.0f * S, 3.5f * S);
    else {                                                        // zubatý úsmev
      static const float zx[6] = { -7.5f, -4.5f, -1.5f, 1.5f, 4.5f, 7.5f };
      static const float zy[6] = { -1.5f, 1.8f, 0.5f, 2.2f, 0.5f, -1.5f };
      for (int j = 0; j < 5; ++j)
        wkDetail(WK_LINE, mx + zx[j] * S, my + zy[j] * S, mx + zx[j + 1] * S, my + zy[j + 1] * S, 1.7f, 0);
    }
#else
    // --- bruško: guľatá hlava, vlasy, oči so zreničkami, gombíky
    wkBegin(COL_BG, WK_INK);
    wkShape(WK_CAP, hx, hy - 2.0f * S, hx + lean, hy - 13.0f * S, 9.5f * S, 0);
    wkShape(WK_CIRC, cx, cy, 0, 0, 11.5f * S, 0);
    static const float eX[4][2] = { { 2.6f, 8.0f }, { -4.2f, 4.2f }, { 2.4f, 7.8f }, { -8.0f, -2.6f } };
    static const float pX[4] = { 1.0f, 0.0f, 0.6f, -1.0f }, pY[4] = { 0.0f, 0.4f, 1.6f, 0.0f };
    const float ey = cy - 2.5f * S + ((look == WK_LOOK_DOWN) ? 1.2f * S : 0.0f);
    for (int j = 0; j < 2; ++j){
      wkDetail(WK_ERING, cx + eX[look][j] * S, ey, 1.1f, 0, 2.6f * S, 3.4f * S);
      wkDetail(WK_DOT, cx + (eX[look][j] + pX[look]) * S, ey + pY[look] * S, 0, 0, 1.35f * S, 0);
    }
    static const float mX[4] = { 4.5f, 0.0f, 4.5f, -4.5f };
    const float mx = cx + mX[look] * S, my = cy + ((look == WK_LOOK_DOWN) ? 6.0f : 5.2f) * S;
    if (oMouth) wkDetail(WK_ERING, mx, my, 1.1f, 0, 1.8f * S, 2.4f * S);
    else {
      wkDetail(WK_LINE, mx - 3.5f * S, my - 1.0f * S, mx, my + 1.2f * S, 1.1f, 0);
      wkDetail(WK_LINE, mx, my + 1.2f * S, mx + 3.5f * S, my - 1.0f * S, 1.1f, 0);
    }
    wkDetail(WK_LINE, cx - 2.5f * S, cy - 11.2f * S, cx - 5.5f * S, cy - 16.0f * S, 1.2f, 0);
    wkDetail(WK_LINE, cx,           cy - 11.5f * S, cx + 0.5f * S, cy - 17.0f * S, 1.2f, 0);
    wkDetail(WK_LINE, cx + 2.5f * S, cy - 11.2f * S, cx + 5.5f * S, cy - 15.5f * S, 1.2f, 0);
    const float bx = hx + lean * 0.5f + (front ? 0.0f : 4.0f * S * fd);
    wkDetail(WK_DOT, bx, hy - 10.0f * S, 0, 0, 1.2f * S, 0);
    wkDetail(WK_DOT, bx, hy - 5.0f * S, 0, 0, 1.2f * S, 0);
#endif
    wkEnd();
  };
  if (WK_STYLE == 1){ arm(0); arm(1); body(); }                // tekvica: ručičky trčia spoza tekvice
  else { arm(0); body(); arm(1); }                               // inak zadná ruka za telom, predná pred ním
}
static void wkShow(int e){                                      // prekreslí starú + novú polohu
  WalkCtx &k = wkK[e];
  int nx0 = 9999, ny0 = 9999, nx1 = -9999, ny1 = -9999;
  if (k.fig) for (int i = 0; i < k.nl; ++i){
    const WkLayer &L = k.L[i];
    if (L.x0 < nx0) nx0 = L.x0; if (L.y0 < ny0) ny0 = L.y0;
    if (L.x1 > nx1) nx1 = L.x1; if (L.y1 > ny1) ny1 = L.y1;
  }
  const int x0 = min(nx0, (int)wkBox[e][0]), y0 = min(ny0, (int)wkBox[e][1]);
  const int x1 = max(nx1, (int)wkBox[e][2]), y1 = max(ny1, (int)wkBox[e][3]);
  if (x0 <= x1 && y0 <= y1)
    gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, x0, y0, x1, y1, shadeWalk, &k);
  wkBox[e][0] = nx0; wkBox[e][1] = ny0; wkBox[e][2] = nx1; wkBox[e][3] = ny1;
}
static void wkWalk(int e, float x0, float x1, float speed, float &ph){
  const unsigned long t0 = millis();
  for (;;){
    float x = x0 + speed * (millis() - t0) * 0.001f;
    const bool end = (x >= x1); if (end) x = x1;
    ph = x * (6.2832f / (34.0f * WK_S));                        // dĺžka kroku
    wkPose(e, x, WK_GY, WK_WALK, ph, millis(), WK_LOOK_RIGHT, false); wkShow(e);
    if (end) break;
    delay(2);
  }
}
static void wkHold(int e, float x, float y, uint8_t mode, uint8_t look, bool oM, int ms){
  const unsigned long t0 = millis();
  if (mode == WK_STAND || mode == WK_SQUASH){                    // nehybná póza: stačí raz
    wkPose(e, x, y, mode, 0, t0, look, oM); wkShow(e); delay(ms); return;
  }
  do { wkPose(e, x, y, mode, 0, millis(), look, oM); wkShow(e); delay(12); } while (millis() - t0 < (unsigned long)ms);
}
static void wkDrop(int e, float x){                              // pád z neba na zem
  float y = eyeTopY - 8, v = 50.0f;
  unsigned long tp = millis();
  for (;;){
    const unsigned long now = millis(); const float dt = (now - tp) * 0.001f; tp = now;
    v += 600.0f * dt; y += v * dt;
    const bool end = (y >= WK_GY); if (end) y = WK_GY;
    wkPose(e, x, y, WK_FALL, 0, now, WK_LOOK_US, true); wkShow(e);
    if (end) break;
    delay(2);
  }
  wkHold(e, x, WK_GY, WK_SQUASH, WK_LOOK_US, true, 170);         // dopad – prikrčí sa
}
static void wkSink(int e, bool oM){                              // pád do kanála
  float y = WK_GY, v = 30.0f;
  unsigned long tp = millis();
  while (y < eyeBotY + 70.0f * WK_S){
    const unsigned long now = millis(); const float dt = (now - tp) * 0.001f; tp = now;
    v += 600.0f * dt; y += v * dt;
    wkPose(e, WK_CM, y, WK_FALL, 0, now, WK_LOOK_US, oM); wkShow(e);
    delay(2);
  }
  wkK[e].fig = false; wkShow(e);                                 // zmizol
}
static void wkJump(int e, float xa, float xb){                   // skok do kanála
  wkHold(e, xa, WK_GY, WK_SQUASH, WK_LOOK_US, false, 180);
  const unsigned long t0 = millis(); const int T = 520;
  for (;;){
    const unsigned long el = millis() - t0; const float u = (el >= (unsigned long)T) ? 1.0f : (float)el / T;
    wkPose(e, xa + (xb - xa) * u, WK_GY - 140.0f * u * (1.0f - u), WK_FALL, 0, millis(), WK_LOOK_US, false); wkShow(e);
    if (u >= 1.0f) break;
    delay(2);
  }
}
static void activity_gag_walker(){
  gagStartCentered();
  wkBuildGround();
  for (int e = 0; e < 2; ++e){
    wkK[e].fig = false; wkK[e].n = 0;
    wkBox[e][0] = wkBox[e][1] = 9999; wkBox[e][2] = wkBox[e][3] = -9999;
  }
  gagBlOut();                                                    // v tme sa nakreslí zem s kanálmi
  for (int e = 0; e < 2; ++e) gagRenderRect(e ? rightGFX() : leftGFX(), e ? petBufR : petBufL, CX - EYE_R, eyeTopY, CX + EYE_R, eyeBotY, shadeWalk, &wkK[e]);
  gagBlIn();
  delay(500);
#define WK_PASSES 2                                            // koľkokrát prejde cez obe oči
  static const float speed[3] = { 60.0f, 85.0f, 105.0f };
  float ph = 0.0f;
  for (int pass = 0; pass < WK_PASSES; ++pass){
    for (int e = 0; e < 2; ++e){
      const bool last = (pass == WK_PASSES - 1 && e == 1);
      float xs = CX - EYE_R - 25;                                // prvýkrát príde zboku
      if (!(pass == 0 && e == 0)){
        wkDrop(e, WK_LAND);                                      // inak spadne z neba
        if (pass == 0){                                          // obzrie sa, kde je
          wkHold(e, WK_LAND, WK_GY, WK_STAND, WK_LOOK_LEFT, false, 450);
          wkHold(e, WK_LAND, WK_GY, WK_STAND, WK_LOOK_RIGHT, false, 350);
          wkHold(e, WK_LAND, WK_GY, WK_STAND, WK_LOOK_US, false, 500);
        } else wkHold(e, WK_LAND, WK_GY, WK_STAND, WK_LOOK_US, false, pass == 1 ? 350 : 200);
        xs = WK_LAND;
      }
      if (!last){
        wkWalk(e, xs, WK_CM, speed[pass], ph);                   // prejde až nad kanál...
        wkHold(e, WK_CM, WK_GY, WK_STAND, WK_LOOK_DOWN, true, pass == 0 ? 500 : 280);   // ...pozrie dole
        wkHold(e, WK_CM, WK_GY, WK_STAND, WK_LOOK_US, true, pass == 0 ? 250 : 120);     // ...na nás
        wkSink(e, true);                                         // a spadne
      } else {
        wkWalk(e, xs, WK_CL - 16, speed[pass] * 0.6f, ph);       // naposledy zastaví pred kanálom
        wkHold(e, WK_CL - 16, WK_GY, WK_STAND, WK_LOOK_DOWN, false, 550);
        wkHold(e, WK_CL - 16, WK_GY, WK_STAND, WK_LOOK_US, false, 400);
        wkHold(e, WK_CL - 16, WK_GY, WK_WAVE, WK_LOOK_US, false, 1400);   // zakýva
        wkJump(e, WK_CL - 16, WK_CM);                            // a skočí do kanála
        wkSink(e, false);
      }
    }
  }
  delay(700);
  gagBl = 2;
  gagEndNormal();                                                // späť cez podsvietenie
}

// ukážka nových gagov za sebou (príkaz DEMO alebo START_DEMO_NEW_GAGS)
static void runNewGagsDemo(int from, int to){
  for (int i = from; i <= to; ++i){ runActivityByIndex(i); delay(1500); }
}

/* ================== Scheduler ================== */
typedef void (*ActivityFn)();
static ActivityFn dayActivities[] = {
  activity_look_forward,        // 0
  activity_look_left_up,        // 1
  activity_look_right_up,       // 2
  activity_gag_two_in_left,     // 3
  activity_gag_flower_spin,     // 4
  activity_gag_scary_cat_eyes,  // 5
  activity_gag_arkanoid_chaos,  // 6
  activity_gag_pumping_pupils,  // 7
  activity_gag_hypno_spiral,    // 8
  activity_gag_bouncing_eyes,   // 9
  activity_gag_wave_motion,     // 10

  activity_spooked_saccades,    // 11
  activity_pupil_breath,        // 12 ok
  activity_figure8_orbit,       // 13
  activity_gag_pumpkin_eyes,    // 14
  activity_halloween_text,      // 15
  activity_gag_angry_eyes,      // 16  zlé šikmé oči (príkaz ANGRY alebo G=16)
  activity_gag_heart_eyes,      // 17  zaľúbené srdiečka
  activity_gag_surprised,       // 18  prekvapené
  activity_gag_ko_eyes,         // 19  K.O. krížiky + hviezdičky
  activity_gag_cross_eyed,      // 20  škúlenie
  activity_gag_sad_eyes,        // 21  smutné so slzami
  activity_gag_blood_eyes,      // 22  krvavé žilky
  activity_gag_spider,          // 23  pavúk
  activity_gag_bats,            // 24  netopiere
  activity_gag_fire_eyes,       // 25  ohnivé oči
  activity_gag_scanner,         // 26  robot – skener
  activity_gag_zombie,          // 27  zombie
  activity_gag_slime,           // 28  sliz
  activity_gag_night,           // 29  noc – mesiac a hviezdy
  activity_gag_aquarium,        // 30  akvárium s rybkou
  activity_gag_slots,           // 31  hracie automaty
  activity_gag_lightning,       // 32  blesky
  activity_gag_cyclops,         // 33  kyklop cez oba displeje
  activity_gag_witch,           // 34  čarodejnica na metle
  activity_gag_radar,           // 35  radar hľadá deti
  activity_gag_candle,          // 36  sviečka namiesto zrenice
  activity_gag_tentacles,       // 37  chápadlá vyliezajú zdola
  activity_gag_vampire,         // 38  upír
  activity_gag_werewolf,        // 39  vlkolak
  activity_gag_keyhole,         // 40  kľúčová dierka
  activity_gag_jaws,            // 41  papuľa chňapne
  activity_gag_quiz,            // 42  príklad v očiach
  activity_gag_clock,           // 43  koľko je hodín?
  activity_gag_portal,          // 44  portály
  activity_gag_giant,           // 45  škúlenie -> obrie oko
  activity_gag_dragon,          // 46  dračie oči
  activity_gag_red_eyes,        // 47  červené oči v tme
  activity_gag_flame_eyes,      // 48  plamenné oči (červené/modré)
  activity_gag_six_seven,       // 49  six-seven
  activity_gag_car_loop,        // 50  autíčko robí looping
  activity_gag_fireworks,       // 51  odpočet 3-2-1 a ohňostroj
  activity_gag_walker           // 52  panáčik a kanál
};

static const int NUM_ACT = sizeof(dayActivities)/sizeof(dayActivities[0]);
static int lastAct=-1;

/* Výber gagov:
 *  - náhodný gag nesmie byť medzi RECENT_N naposledy spustenými (FIFO zoznam),
 *  - matematický príklad (QUIZ_GAG) sa nelosuje, ide pravidelne po každých
 *    QUIZ_EVERY_MIN až QUIZ_EVERY_MAX gagoch. */
#define RECENT_N        15
#define QUIZ_GAG        42
#define QUIZ_EVERY_MIN  5
#define QUIZ_EVERY_MAX  8
static int8_t recentGags[RECENT_N];
static uint8_t recentCount = 0, recentHead = 0;
static uint8_t gagsSinceQuiz = 0, quizAfter = 0;
static void rememberGag(int idx){                     // pridá gag do FIFO (najstarší vypadne)
  recentGags[recentHead] = (int8_t)idx;
  recentHead = (recentHead + 1) % RECENT_N;
  if (recentCount < RECENT_N) ++recentCount;
}
static bool wasRecent(int idx){
  for (int i = 0; i < recentCount; ++i) if (recentGags[i] == idx) return true;
  return false;
}
static int pickRandomGag(){
  int cand[64], n = 0;
  for (int i = 0; i < NUM_ACT && n < 64; ++i)
    if (i != QUIZ_GAG && !wasRecent(i)) cand[n++] = i;
  if (n == 0){                                        // (nemalo by nastať) – aspoň nie posledný
    for (int i = 0; i < NUM_ACT && n < 64; ++i) if (i != QUIZ_GAG && i != lastAct) cand[n++] = i;
  }
  return cand[random(n)];
}

/* ================== GESTÁ RÚK (druhé Pico) ==================
 * Pred každým gagom pošle Pico s očami cez UART (GP0 TX -> GP1 RX druhého Pica)
 * gesto rúk, po gagu „NONE“ (ruky sa vrátia do pokoja). Druhé Pico beží
 * so sketchom chapadla_gesta.ino. Priradenie gest ku gagom = tabuľka nižšie.
 */
#define ARMS_TX_PIN 0                   // GP0 = UART0 TX
#define ARMS_RX_PIN 1                   // GP1 = UART0 RX
enum { GE_NONE = 0, GE_WAVE, GE_UP, GE_UNDULATE, GE_SIX7, GE_EIGHT, GE_CIRCLE, GE_REACH, GE_SHAKE, GE_ZOMBIE, GE_RIPPLE, GE_CHEER2, GE_SWIM };
static const char *const gestureCmd[] = { "G:NONE", "G:WAVE", "G:UP", "G:UNDULATE", "G:SIX7", "G:EIGHT", "G:CIRCLE", "G:REACH", "G:SHAKE", "G:ZOMBIE", "G:RIPPLE", "G:CHEER2", "G:SWIM" };
static const uint8_t gagGesture[] = {
  GE_NONE,      GE_NONE,     GE_NONE,      GE_NONE,      GE_EIGHT,     //  0- 4 pohľady, dve zrenice, kvetina (osmička)
  GE_UP,        GE_EIGHT,    GE_NONE,      GE_UNDULATE,  GE_REACH,     //  5- 9 mačacie, arkanoid (osmička), pumpovanie, hypno, skákajúce (načahovanie)
  GE_SIX7,      GE_UP,       GE_NONE,      GE_EIGHT,     GE_NONE,      // 10-14 vlna (6-7), vyľakané, dýchanie, osmička (8), tekvičky (nič)
  GE_NONE,      GE_UP,       GE_NONE,      GE_SHAKE,     GE_NONE,      // 15-19 Halloween (nič), zlé, srdiečka (nič), prekvapené (rýchle kmitanie), K.O.
  GE_NONE,      GE_NONE,     GE_UP,        GE_NONE,      GE_NONE,      // 20-24 škúlenie, smutné, krvavé, pavúk (nič), netopiere (nič)
  GE_UP,        GE_NONE,     GE_UNDULATE,  GE_ZOMBIE,    GE_NONE,      // 25-29 oheň, skener, zombie, sliz (zombík naháňa), noc
  GE_SWIM,      GE_NONE,     GE_UP,        GE_NONE,      GE_NONE,      // 30-34 akvárium (plávanie prsia), automaty (nič), blesky, kyklop, čarodejnica (nič)
  GE_NONE,      GE_NONE,     GE_EIGHT,     GE_UP,        GE_NONE,      // 35-39 radar, sviečka, chápadlá (osmička), upír, vlkolak (nič)
  GE_NONE,      GE_UP,       GE_NONE,      GE_NONE,      GE_NONE,      // 40-44 dierka, papuľa, príklad (ruky až pri výsledku), hodiny, portály (len svetlo)
  GE_ZOMBIE,    GE_ZOMBIE,   GE_UP,        GE_UP,        GE_NONE,      // 45-49 obrie oko (zombík), drak (zombík), červené, plamenné, six-seven (6-7 až pri číslach)
  GE_CIRCLE,    GE_NONE,     GE_NONE                                   // 50-52 autíčko (kruhy dopredu), ohňostroj (hurá až pri výbuchu), tekvička (len svetlo)
};
static_assert(sizeof(gagGesture) == sizeof(dayActivities) / sizeof(dayActivities[0]), "gagGesture: doplň gesto pre každý gag");
static void sendGesture(int g){
  Serial1.println(gestureCmd[g]);
  Serial.print("Ruky: "); Serial.println(gestureCmd[g]);
}
/* Svetlá v tekvici (LED pás a RGB moduly na druhom Pico): ku každému gagu
 * efekt „L:<EFEKT>“, po gagu späť na sviečku. */
enum { LI_CANDLE = 0, LI_RED, LI_FIRE, LI_SUN, LI_PURPLE, LI_DARK, LI_BLUE, LI_GREEN,
       LI_LIGHTNING, LI_RAINBOW, LI_PINK, LI_SCANNER, LI_NIGHT, LI_FIREWORKS, LI_BLINK, LI_REDBLINK };
static const char *const lightCmd[] = { "L:CANDLE", "L:RED", "L:FIRE", "L:SUN", "L:PURPLE", "L:DARK", "L:BLUE",
  "L:GREEN", "L:LIGHTNING", "L:RAINBOW", "L:PINK", "L:SCANNER", "L:NIGHT", "L:FIREWORKS", "L:BLINK", "L:REDBLINK" };
static const uint8_t gagLight[] = {
  LI_CANDLE,    LI_CANDLE,   LI_CANDLE,    LI_CANDLE,    LI_RAINBOW,   //  0- 4 pohľady, dve zrenice, kvetina
  LI_GREEN,     LI_RAINBOW,  LI_CANDLE,    LI_PURPLE,    LI_CANDLE,    //  5- 9 mačacie, arkanoid, pumpovanie, hypno, skákajúce
  LI_BLUE,      LI_CANDLE,   LI_CANDLE,    LI_CANDLE,    LI_FIRE,      // 10-14 vlna, vyľakané, dýchanie, osmička, tekvičky
  LI_FIRE,      LI_RED,      LI_PINK,      LI_SUN,       LI_SUN,       // 15-19 Halloween, zlé, srdiečka, prekvapené, K.O.
  LI_CANDLE,    LI_BLUE,     LI_RED,       LI_PURPLE,    LI_PURPLE,    // 20-24 škúlenie, smutné, krvavé, pavúk, netopiere
  LI_FIRE,      LI_SCANNER,  LI_GREEN,     LI_GREEN,     LI_NIGHT,     // 25-29 oheň, skener, zombie, sliz, noc
  LI_BLUE,      LI_RAINBOW,  LI_LIGHTNING, LI_CANDLE,    LI_PURPLE,    // 30-34 akvárium, automaty, blesky, kyklop, čarodejnica
  LI_GREEN,     LI_CANDLE,   LI_PURPLE,    LI_RED,       LI_NIGHT,     // 35-39 radar, sviečka, chápadlá, upír, vlkolak
  LI_DARK,      LI_RED,      LI_SUN,       LI_SUN,       LI_BLUE,      // 40-44 dierka, papuľa, príklad, hodiny, portály
  LI_CANDLE,    LI_FIRE,     LI_REDBLINK,  LI_FIRE,      LI_RAINBOW,   // 45-49 obrie oko, drak, červené v tme (červené blikanie), plamenné, six-seven
  LI_SUN,       LI_FIREWORKS, LI_CANDLE                                // 50-52 autíčko, ohňostroj, panáčik
};
static_assert(sizeof(gagLight) == sizeof(dayActivities) / sizeof(dayActivities[0]), "gagLight: doplň svetlo pre každý gag");
static void sendLight(int l){
  Serial1.println(lightCmd[l]);
  Serial.print("Svetlo: "); Serial.println(lightCmd[l]);
}
static void runGagWithArms(int idx){
  sendGesture(gagGesture[idx]);
  sendLight(gagLight[idx]);
  dayActivities[idx]();
  if (idx == 1){                                         // koniec gagu 1: pozdrav rukami
    Serial1.println("G:GREET"); Serial.println("Ruky: G:GREET");
    const unsigned long t0 = millis();
    while (millis() - t0 < 3500){ (void)tryBlinkTick(); delay(8); }
  }
  sendGesture(GE_NONE);
  sendLight(LI_CANDLE);
}

static void runRandomDayActivity(){
  if (quizAfter == 0) quizAfter = (uint8_t)random(QUIZ_EVERY_MIN, QUIZ_EVERY_MAX + 1);
  int idx;
  if (gagsSinceQuiz >= quizAfter && QUIZ_GAG < NUM_ACT){
    idx = QUIZ_GAG;                                     // príklad v očiach ide pravidelne
    gagsSinceQuiz = 0;
    quizAfter = (uint8_t)random(QUIZ_EVERY_MIN, QUIZ_EVERY_MAX + 1);
  } else {
    idx = pickRandomGag();
    rememberGag(idx);
    ++gagsSinceQuiz;
  }
  lastAct=idx;
  Serial.print("Gag "); Serial.println(idx);
  runGagWithArms(idx);

  unsigned long gap=randInRangeMs(150,350), t0=millis();
  while((millis()-t0)<gap){ (void)tryBlinkTick(); delay(8); }
}

// spustenie konkrétneho gagu (zo sériovej linky)
static void runActivityByIndex(int idx){
  if (sleeping){ Serial.println("Oci spia - gag sa nespusti (nastav cas cez T=HH:MM)"); return; }
  if (idx < 0 || idx >= NUM_ACT){
    Serial.print("Neplatny gag, zadaj G=0 az G="); Serial.println(NUM_ACT - 1);
    return;
  }
  Serial.print("Spustam gag "); Serial.println(idx);
  lastAct = idx;
  if (idx != QUIZ_GAG) rememberGag(idx);              // aj ručne spustený gag sa zapíše do FIFO
  runGagWithArms(idx);
  Serial.println("Gag hotovy");
}

/* ================== SETUP / LOOP ================== */
void setup(){
  delay(150);
  Serial.begin(115200);
  Serial1.setTX(ARMS_TX_PIN); Serial1.setRX(ARMS_RX_PIN);   // UART k Picu s rukami
  Serial1.begin(115200);
  unsigned long t0=millis(); while(!Serial && (millis()-t0)<8000) {}
  Serial.println("Send time as T=HH:MM or T=HH:MM:SS");

  Serial.println("Brightness commands:");
  Serial.println("  L+ / L- / L=xxx  (left eye)");
  Serial.println("  R+ / R- / R=xxx  (right eye)");
  Serial.println("  B+ / B- / B=xxx  (both eyes)");
  Serial.println("Gags: G=0 .. G=52 (spusti gag), STOP = zastav nahodne gagy, START = znova, ANGRY = zle oci, DEMO = najnovsie gagy, DEMOALL = gagy 16-52");
  Serial.println("Serial Monitor: 115200 baud, koniec riadku 'Nový riadok' (Newline)");

  // PWM pre podsvietenie oboch očí - DENNÝ JAS
  pinMode(LEFT_BLK_PIN, OUTPUT);
  pinMode(RIGHT_BLK_PIN, OUTPUT);
  leftEyeBrightness = 255;   // denný jas
  rightEyeBrightness = 255;
  analogWrite(leftBLKPin(), leftEyeBrightness);
  analogWrite(rightBLKPin(), rightEyeBrightness);

  Wire.setSDA(I2C_SDA); Wire.setSCL(I2C_SCL); Wire.begin();
  Wire.beginTransmission(DS3231_ADDR); rtc_ok=(Wire.endTransmission()==0);

  seedSoftClockFromBuild();

  pinMode(26, INPUT);
  randomSeed((uint32_t)analogRead(26) ^ (uint32_t)micros());

  gfxL->begin(SPI_HZ); gfxR->begin(SPI_HZ);
  gfxL->setTextWrap(false); gfxR->setTextWrap(false);

  precomputeCircleSpans();
  drawSclera(gfxL,"LEFT"); drawSclera(gfxR,"RIGHT");

  lidsL.topY=lidsR.topY=eyeTopY-1; lidsL.botY=lidsR.botY=eyeBotY+1;
  drawPupilLeft (pxL,pyL,PUPIL_R,true,lidsL);
  drawPupilRight(pxR,pyR,PUPIL_R,true,lidsR);

  scheduleNextBlink();

  Now n; if(rtc_ok && rtcRead(n)){} else softNow(n);
  if(isQuietHours(n)) setSleeping(true);
#if START_DEMO_NEW_GAGS
  else { delay(500); runNewGagsDemo(DEMO_FROM, DEMO_TO); }
#elif START_WITH_ANGRY_GAG
  else { delay(500); activity_gag_angry_eyes(); }
#endif
}

void loop(){
  tryReadTimeFromSerial(0);

  Now n;
  if(manual_override) softNow(n);
  else if(rtc_ok){ if(!rtcRead(n)) softNow(n); }
  else softNow(n);

  if(isQuietHours(n)){
    if(!sleeping) setSleeping(true);
    if(!seamDrawn){
      drawEyelidSeam(gfxL,seamYpos(),3,COL_LID_SEAM);
      drawEyelidSeam(gfxR,seamYpos(),3,COL_LID_SEAM);
      seamDrawn = true;
    }
    
    // Občasné kontrolné pohľady počas spánku
    if(nextSleepPeekMs == 0){
      nextSleepPeekMs = millis() + (45UL * 60UL * 1000UL) + (rand() % (30UL * 60UL * 1000UL));
    }
    
    if(!sleepPeekInProgress && (long)(millis() - nextSleepPeekMs) >= 0){
      sleepPeekInProgress = true;
      sleepPeek();
      sleepPeekInProgress = false;
      nextSleepPeekMs = millis() + (45UL * 60UL * 1000UL) + (rand() % (30UL * 60UL * 1000UL));
    }
    
    delay(150); return;
  }
  
  // Reset peek timera keď nie je spánok
  if(nextSleepPeekMs != 0){
    nextSleepPeekMs = 0;
    sleepPeekInProgress = false;
  }

  // Ranné prebúdzanie
  if(sleeping && !isQuietHours(n)){
    morningWakeUp();
    scheduleNextBlink();
    return;
  }

  if(isInPreSleepWindow(n)){ preSleepTick(n); delay(8); return; }

  if(manualMode){ (void)tryBlinkTick(); delay(8); return; }   // STOP: čaká na G=cislo

  runRandomDayActivity();
}

/* ================== Compile-time guards ================== */
#if (TILE_SZ/2) < (PUPIL_R + MAX_STEP)
#error "Increase TILE_SZ or reduce PUPIL_R/MAX_STEP."
#endif
#if (TILE_SZ > W) || (TILE_SZ > H)
#error "TILE_SZ must be <= display width/height."
#endif
#if (PUPIL_R*2 >= TILE_SZ)
#warning "PUPIL_R is large vs TILE_SZ; consider larger tile for cleaner edges."
#endif
