// =====================================================================
//  CHÁPADLÁ – gestá podľa gagov v očiach
//  Doska: Raspberry Pi Pico / Pico 2 (jadro Earle Philhower)
//
//  Pico s očami posiela cez UART (jeho GP0 TX -> GP1 RX tohto Pica,
//  115200 baud) jeden riadok na gag:
//     G:NONE      žiadne gesto – ruky sa plynulo vrátia do pokoja
//     G:WAVE      mávanie
//     G:UP        ruky hore (jemne sa pohupujú)
//     G:UNDULATE  vlnenie (chápadlá krúžia ako had)
//     G:CHEER     hej-hou: obe ruky naraz hore-dole (pri výsledku príkladu)
//     G:GREET     pozdrav: pravá ruka dopredu, ľavá máva (koniec gagu 1)
//     G:SIX7      6-7: ruky predpažené, striedavo hore a dole (gag 49)
//     G:EIGHT     chápadlá opisujú ležatú osmičku (gag 4, 6)
//     G:CIRCLE    krúženie smerom dopredu (gag 50)
//     G:REACH     načahovanie – obe ruky dopredu (gag 9)
//     G:SHAKE     rýchle kmitanie (gag 18)
//     G:ZOMBIE    zombík – natiahnuté dopredu, striedavo hore-dole (gag 28)
//     G:RIPPLE    vlnky vody (gag 30)
//     G:CHEER2    hurá-hurá: ruky striedavo hore a dole (ohňostroj, gag 51)
//     G:SWIM      plávanie prsia (akvárium, gag 30)
//  Tie isté príkazy sa dajú písať aj do Serial Monitora (USB) – na skúšku.
//  Bez nového príkazu sa po GESTURE_TIMEOUT ruky samé vrátia do pokoja.
//
//  Servá:  S1 = GP2  chápadlo A, zadné (vysoké) servo – pár A
//          S2 = GP3  chápadlo A, predné (nízke) servo – pár B
//          S3 = GP4  chápadlo B, zadné (vysoké) servo – pár A
//          S4 = GP5  chápadlo B, predné (nízke) servo – pár B
//  Napájanie serv NIE z Pica! Samostatná 5 V vetva, spoločná zem (GND).
//
//  Svetlo v tekvici: 4 RGB moduly zasunuté kolíkmi priamo do dosky vedľa Pico
//  (každý kolík v inom riadku = na inom pine Pico, GND modulu na GND riadku):
//     modul 1 (vľavo, riadky 8–11):  GND=r8,  RED=GP6,  GRE=GP7,  BLU=GP8
//     modul 2 (vľavo, riadky 13–16): GND=r13, RED=GP10, GRE=GP11, BLU=GP12
//     modul 3 (vpravo, riadky 13–16): GND=r13, RED=GP21, GRE=GP20, BLU=GP19
//     modul 4 (vpravo, riadky 18–21): GND=r18, RED=GP17, GRE=GP16, BLU=r21 -> drôtik na GP18 (r17)
//  LED pás 8× WS2812 („8-Bit 2812 RGB“): DI = GP22 (pin 29), 4-7VDC = + lišta
//  serv, GND = − lišta serv. Napája sa zo 5 V serv -> drží USB-A port zapnutý.
//  Potrebná knižnica: Adafruit NeoPixel (Správca knižníc).
//  Svetelné efekty posiela Pico s očami ku každému gagu: L:CANDLE, L:RED,
//  L:FIRE, L:SUN, L:PURPLE, L:DARK, L:BLUE, L:GREEN, L:LIGHTNING, L:RAINBOW,
//  L:PINK, L:SCANNER, L:NIGHT, L:FIREWORKS, L:BLINK (dajú sa skúšať aj v Serial Monitore).
// =====================================================================
#include <Servo.h>
#include "hardware/gpio.h"
#include <Adafruit_NeoPixel.h>

const int N = 4;
const int PINS[N] = {2, 3, 4, 5};

// --- NASTAVENIE (doladiť na hotových chápadlách) ----------------------
// stredy z centrovacieho sketchu (príkaz p tam vypíše riadok na skopírovanie)
int center[N] = {1500, 1500, 1500, 1500};
// ktoré servo každého chápadla dvíha ruku hore (LIFT) a ktoré ňou máva do strán (SWING)
// chápadlo A: servá 0 a 1, chápadlo B: servá 2 a 3
const int LIFT[2]  = {1, 3};      // GP3, GP5 = hore / dole (overené testom chapadlo_test)
const int SWING[2] = {0, 2};      // GP2, GP4 = vľavo / vpravo
// smer: ak ide ruka pri „G:UP“ dole namiesto hore, zmeň +1 na -1 (každé servo zvlášť)
// zistené testom S=2..S=5 (pohľad spredu): chápadlo A (GP2/GP3) = PRAVÉ, chápadlo B (GP4/GP5) = ĽAVÉ
// znamienka tak, aby + znamenalo: SWING = dopredu, LIFT = hore
const int SIGN[N]  = {-1, -1, -1, -1};   // GP2, GP3, GP4, GP5 (hore/dole overené príkazom HORE)
const int LIMIT    = 700;               // max. odchýlka od stredu [µs] – ochrana šnúr
// plynulosť: každé servo sa rozbieha a brzdí (žiadne skoky)
const float MAX_SPEED = 1500.0f;        // najvyššia rýchlosť [µs/s] (1500 ≈ 135°/s)
const float MAX_ACC   = 6000.0f;        // zrýchlenie aj brzdenie [µs/s²] (menej = jemnejšie, pomalšie rozbehy)
const unsigned long GESTURE_TIMEOUT = 60000;   // poistka: po 60 s bez príkazu do pokoja

// veľkosti gest [µs od stredu]
const int UP_LIFT     = 450;   // ruky hore
const int UP_SWAY     = 150;   //   pohupovanie dopredu-dozadu (70 bolo na pravom ramene takmer nebadať)
const int WAVE_LIFT   = 320;   // mávanie: ruka zdvihnutá...
const int WAVE_SWING  = 300;   //   ...a máva do strán
const float WAVE_HZ   = 0.8f;
const int UND_AMP     = 280;   // vlnenie: krúženie
const float UND_HZ    = 0.45f;
const int CHEER_HIGH  = 500;   // „hej-hou“: ruky vyskočia hore...
const int CHEER_LOW   = 150;   //   ...a klesnú len trochu
const float CHEER_HZ  = 1.3f;  //   koľkokrát za sekundu
// zosilnenie pohybu pre každé rameno zvlášť (ak sa jedno hýbe menej – iná mechanika / šnúra)
//                       pravé (A)  ľavé (B)
const float LIFT_GAIN[2]  = { 1.0f,  1.0f };   // hore / dole
const float SWING_GAIN[2] = { 1.0f,  1.0f };   // dopredu / dozadu
// „6-7“: ruky predpažené (dlane hore), striedavo jedna hore, druhá dole – ako keď sa niečo váži
const int SIX7_FWD   = 250;    // ruky mierne dopredu [µs]
const int SIX7_BASE  = 180;    // stredná výška [µs]
const int SIX7_AMP   = 260;    // rozkmit hore / dole [µs]
const float SIX7_HZ  = 0.8f;   // koľkokrát za sekundu
// osmička: chápadlá opisujú ležatú 8 (dopredu-dozadu a 2× rýchlejšie hore-dole)
const int EIGHT_SWING = 350;   // šírka osmičky (dopredu / dozadu) [µs]
const int EIGHT_LIFT  = 200;   // výška osmičky (hore / dole) [µs]
const int EIGHT_BASE  = 150;   // celá osmička trochu vyššie (aby dole nenarazila)
const float EIGHT_SEC = 3.5f;  // jedna osmička za toľko sekúnd
// krúženie dopredu (ako koleso): hore ide ruka dopredu, dole dozadu
const int CIRC_SWING  = 300;   // polomer dopredu / dozadu [µs]
const int CIRC_LIFT   = 250;   // polomer hore / dole [µs]
const int CIRC_BASE   = 200;   // stred kruhu trochu vyššie (aby dole nenarazil)
const float CIRC_SEC  = 2.0f;  // jedno koliečko za toľko sekúnd
// načahovanie: obe ruky dopredu (a trochu hore), jemne sa „naťahujú“
const int REACH_FWD   = 550;   // ako ďaleko dopredu [µs] (≈ 50°)
const int REACH_LIFT  = 150;   // trochu hore [µs]
const int REACH_PULSE = 40;    // jemné naťahovanie sa [µs] (0 = úplne stoja)
// rýchle kmitanie (gag 18): hore-dole aj dopredu-dozadu rýchlo
const int SHAKE_LIFT   = 120;    // [µs]
const int SHAKE_SWING  = 120;    // [µs]
const float SHAKE_HZ_L = 2.5f;   // hore-dole za sekundu
const float SHAKE_HZ_S = 1.9f;   // dopredu-dozadu za sekundu
const float SHAKE_SPEED = 2500.0f, SHAKE_ACC = 30000.0f;   // pri kmitaní smie byť servo rýchlejšie
// zombík (gag 28): obe ruky natiahnuté dopredu na max, striedavo hore-dole
const int ZOMB_FWD     = 650;    // dopredu [µs] (max. je LIMIT 700)
const int ZOMB_BASE    = 150;    // stredná výška [µs]
const int ZOMB_AMP     = 200;    // hore-dole [µs]
const float ZOMB_SEC   = 1.6f;   // jedno hore-dole za toľko sekúnd
// vlnky (gag 30, akvárium): ruky sa vlnia hore-dole, druhá o štvrť vlny neskôr
const int RIP_BASE     = 120;    // [µs]
const int RIP_AMP      = 180;    // hore-dole [µs]
const int RIP_SWING    = 80;     // jemne dopredu-dozadu [µs]
const float RIP_SEC    = 1.8f;   // jedna vlnka za toľko sekúnd
// plávanie prsia (gag 30, akvárium): kĺzanie vpredu -> záber dole a dozadu -> ruky hore k hrudi -> znova dopredu
const float SWIM_SEC = 2.6f;   // jeden záber za toľko sekúnd
//                       čas (0..1)  dopredu   hore
const float SWIM_KEY[5][3] = { { 0.00f,  550,  100 },     // kĺzanie – ruky natiahnuté dopredu
                               { 0.35f,  550,  100 },     //   ...chvíľu vydržia
                               { 0.60f, -150, -100 },     // záber: dole a dozadu
                               { 0.80f,  -50,  260 },     // ruky hore k hrudi
                               { 1.00f,  550,  100 } };   // vystretie dopredu
const bool ARMS_SYNC  = true;  // true = obe ruky sa pri gestách hýbu rovnako, false = v protifáze (vlnenie)
// pozdrav (koniec gagu 1): pravá ruka (pri pohľade spredu) sa ohne dopredu, ľavá máva
const int GREET_RIGHT_ARM = 0;   // ktoré chápadlo je pri pohľade spredu PRAVÉ: 0 = A (GP2/GP3), 1 = B (GP4/GP5)
const int GREET_FWD       = 570; // pravá ruka: ako ďaleko dopredu [µs] (≈ 51°)
const int GREET_FWD_SIGN  = +1;  //   ak sa ohne dozadu namiesto dopredu, prepni znamienko
const int GREET_FWD_LIFT  = 120; //   a trochu zdvihnutá
const int GREET_WAVE_LIFT = 450; // ľavá ruka: hore...
const int GREET_WAVE_SW   = 250; //   ...a máva do strán
const float GREET_WAVE_HZ = 1.2f;
// pokoj (žiadne gesto): chápadlá sa jemne a pomaly vlnia -> servá stále berú prúd
// a powerbanka nevypne USB-A port; zároveň chápadlá pôsobia „živo“
const bool  IDLE_MOVE  = false;  // true = v pokoji sa jemne vlnia (viac odberu, rýchlejšie míňa powerbanku)
const int   IDLE_LIFT  = 60;     // [µs] ≈ 5° hore / dole
const int   IDLE_SWING = 45;     // [µs] ≈ 4° do strán
const float IDLE_HZ    = 0.2f;   // jedna vlnka za 5 s
// ----------------------------------------------------------------------

enum Gesture { G_NONE, G_WAVE, G_UP, G_UNDULATE, G_CHEER, G_GREET, G_SIX7, G_EIGHT, G_CIRCLE, G_REACH, G_SHAKE, G_ZOMBIE, G_RIPPLE, G_CHEER2, G_SWIM };
Gesture gesture = G_NONE;
// svetelné efekty (enum musí byť hore, pred prvou funkciou – kvôli Arduino IDE)
enum Light { L_CANDLE, L_RED, L_FIRE, L_SUN, L_PURPLE, L_DARK, L_BLUE, L_GREEN,
             L_LIGHTNING, L_RAINBOW, L_PINK, L_SCANNER, L_NIGHT, L_FIREWORKS, L_BLINK, L_REDBLINK, L_COUNT };

unsigned long gestureStart = 0, lastCmd = 0;

Servo sv[N];
int pos[N];
float posF[N], vel[N];                  // presná poloha a rýchlosť pre plynulý pohyb
String lineUsb, lineUart;

void setOffset(int i, float off, int *target){
  int t = center[i] + SIGN[i] * (int)lroundf(off);
  target[i] = constrain(t, center[i] - LIMIT, center[i] + LIMIT);
}


// --- SVETLÁ: efekty -----------------------------------------------------
// Pico s očami pošle pri každom gagu „L:<EFEKT>“ (napr. L:BLUE pri akváriu),
// po gagu „L:CANDLE“. Na skúšku sa dajú písať aj do Serial Monitora.
const char *const LIGHT_NAMES[L_COUNT] = { "CANDLE", "RED", "FIRE", "SUN", "PURPLE", "DARK", "BLUE", "GREEN",
                                           "LIGHTNING", "RAINBOW", "PINK", "SCANNER", "NIGHT", "FIREWORKS", "BLINK", "REDBLINK" };
Light light = L_CANDLE;
unsigned long lightStart = 0, lastLightCmd = 0;
const unsigned long LIGHT_TIMEOUT = 90000;   // poistka: po 90 s bez príkazu späť na sviečku

// --- RGB moduly (priamo na pinoch Pico) ---
const int LED_N = 4;                    // počet modulov
const int LED_PINS[LED_N][3] = { {6, 7, 8}, {10, 11, 12}, {21, 20, 19}, {17, 16, 18} };   // [modul][R, G, B]
const float LED_BRIGHT = 1.0f;          // jas modulov 0..1

// --- LED pás 8× WS2812 ---
const int   STICK_PIN    = 22;          // GP22 (pin 29)
const int   STICK_N      = 8;
const float STICK_BRIGHT = 0.6f;        // jas pásu 0..1 (0.6 ≈ do 200 mA pri sviečke)
Adafruit_NeoPixel stick(STICK_N, STICK_PIN, NEO_GRB + NEO_KHZ800);

const int PX_N = STICK_N + LED_N;       // spoločný stav: pixely pásu 0..7, moduly 8..11
float pxFl[PX_N], pxTw[PX_N], pxTwR[PX_N], pxTwG[PX_N], pxTwB[PX_N];
unsigned long flashUntil = 0;           // blesk
int flashLevel = 0;

void startLight(Light l){
  light = l; lightStart = millis(); lastLightCmd = millis();
  for (int i = 0; i < PX_N; i++) pxTw[i] = 0;
  Serial.print("Svetlo: "); Serial.println(LIGHT_NAMES[l]);
}

// HSV -> RGB (h 0..1)
void hsv(float h, float &r, float &g, float &b){
  h -= floorf(h); const float x = h * 6.0f; const int k = (int)x; const float f = x - k;
  switch (k){
    case 0: r = 1; g = f; b = 0; break;      case 1: r = 1 - f; g = 1; b = 0; break;
    case 2: r = 0; g = 1; b = f; break;      case 3: r = 0; g = 1 - f; b = 1; break;
    case 4: r = f; g = 0; b = 1; break;      default: r = 1; g = 0; b = 1 - f; break;
  }
  r *= 255; g *= 255; b *= 255;
}

// farba pixelu: gi = index v spoločnom stave, i / n = poloha v skupine (pás alebo moduly)
void effColor(int gi, int i, int n, float t, float &r, float &g, float &b){
  r = g = b = 0;
  const float u = (n > 1) ? (float)i / (n - 1) : 0;        // 0..1 pozdĺž skupiny
  switch (light){
    case L_CANDLE: {                                          // sviečka v tekvici
      pxFl[gi] += ((120 + random(0, 136)) - pxFl[gi]) * 0.2f;
      r = pxFl[gi]; g = pxFl[gi] * 0.3f; break; }
    case L_RED: {                                             // červené pulzovanie
      const float p = 0.55f + 0.45f * sinf(TWO_PI * 1.0f * t - u * 1.5f);
      r = 255 * p; break; }
    case L_FIRE: {                                            // divoký oheň
      pxFl[gi] += ((60 + random(0, 196)) - pxFl[gi]) * 0.45f;
      r = pxFl[gi]; g = pxFl[gi] * (0.12f + 0.0008f * random(0, 200)); break; }
    case L_SUN: {                                             // teplé žlté slnko
      const float p = 0.8f + 0.2f * sinf(TWO_PI * 0.3f * t);
      r = 255 * p; g = 170 * p; b = 20 * p; break; }
    case L_PURPLE: {                                          // fialová vlna
      const float p = 0.5f + 0.5f * sinf(TWO_PI * 0.6f * t - u * 4.0f);
      r = 170 * p + 20; b = 255 * p + 20; break; }
    case L_DARK: break;                                       // tma (čierne oči)
    case L_BLUE: {                                            // akvárium: vlnky vody
      const float p = 0.5f + 0.5f * sinf(TWO_PI * 0.4f * t - u * 5.0f);
      const float q = 0.5f + 0.5f * sinf(TWO_PI * 0.23f * t + u * 3.0f);
      r = 0; g = 60 + 80 * p * q; b = 150 + 105 * p; break; }
    case L_GREEN: {                                           // jedovatý sliz
      const float p = 0.5f + 0.5f * sinf(TWO_PI * 0.7f * t - u * 3.0f);
      r = 40 * p; g = 255 * (0.35f + 0.65f * p); break; }
    case L_LIGHTNING: {                                       // búrka: tma a blesky
      r = 0; g = 0; b = 25;
      if (millis() < flashUntil){ r = g = b = flashLevel; } break; }
    case L_RAINBOW: hsv(t * 0.25f + u, r, g, b); break;       // dúha
    case L_PINK: {                                            // srdce: tlkot
      const float ph = fmodf(t, 1.1f);
      const float p = 0.25f + 0.75f * (expf(-30 * ph * ph) + 0.6f * expf(-30 * (ph - 0.28f) * (ph - 0.28f)));
      r = 255 * p; g = 30 * p; b = 110 * p; break; }
    case L_SCANNER: {                                         // robot: červený bod tam a späť
      const float pos = (n - 1) * (0.5f + 0.5f * sinf(TWO_PI * 0.6f * t));
      const float k = fmaxf(0.0f, 1.0f - fabsf(i - pos) * 0.8f);
      r = 255 * k + 8; break; }
    case L_NIGHT: case L_FIREWORKS: {                         // hviezdy / ohňostroj
      if (light == L_NIGHT){ r = 0; g = 0; b = 30; } 
      if (pxTw[gi] <= 0.02f && random(0, light == L_NIGHT ? 300 : 120) == 0){
        pxTw[gi] = 1;
        if (light == L_NIGHT){ pxTwR[gi] = 255; pxTwG[gi] = 255; pxTwB[gi] = 200; }
        else hsv(random(0, 1000) * 0.001f, pxTwR[gi], pxTwG[gi], pxTwB[gi]);
      }
      r += pxTwR[gi] * pxTw[gi]; g += pxTwG[gi] * pxTw[gi]; b += pxTwB[gi] * pxTw[gi];
      pxTw[gi] *= (light == L_NIGHT) ? 0.9f : 0.85f;
      break; }
    case L_REDBLINK: {                                        // červené blikanie (oči v tme)
      const float ph = fmodf(t * 2.5f, 1.0f);                 // 2,5× za sekundu
      const float p = (ph < 0.45f) ? 1.0f : 0.06f;            // zapnuté / skoro tma
      r = 255 * p; break; }
    case L_BLINK: {                                           // oslava: rýchle striedavé blikanie
      const int ph = (int)(t * 8.0f);                         // 8 zmien za sekundu
      if (((ph + i) & 1) == 0){ r = 255; g = 200; b = 40; }   // žltá
      else if ((ph & 2) == 0){ r = 255; g = 255; b = 255; }   // biela
      break; }
    default: break;
  }
}

// volá sa každých 20 ms: prepočíta pás aj moduly
void updateLights(){
  if (light != L_CANDLE && millis() - lastLightCmd > LIGHT_TIMEOUT) startLight(L_CANDLE);
  const float t = (millis() - lightStart) * 0.001f;
  if (light == L_LIGHTNING && millis() >= flashUntil && random(0, 140) == 0){   // nový blesk
    flashUntil = millis() + 60 + random(0, 120); flashLevel = 150 + random(0, 106);
  }
  float r, g, b;
  for (int i = 0; i < STICK_N; i++){
    effColor(i, i, STICK_N, t, r, g, b);
    stick.setPixelColor(i, (uint8_t)constrain(r * STICK_BRIGHT, 0.0f, 255.0f),
                           (uint8_t)constrain(g * STICK_BRIGHT, 0.0f, 255.0f),
                           (uint8_t)constrain(b * STICK_BRIGHT, 0.0f, 255.0f));
  }
  stick.show();
  for (int m = 0; m < LED_N; m++){
    effColor(STICK_N + m, m, LED_N, t, r, g, b);
    analogWrite(LED_PINS[m][0], (int)constrain(r * LED_BRIGHT, 0.0f, 255.0f));
    analogWrite(LED_PINS[m][1], (int)constrain(g * LED_BRIGHT, 0.0f, 255.0f));
    analogWrite(LED_PINS[m][2], (int)constrain(b * LED_BRIGHT, 0.0f, 255.0f));
  }
}

// cieľové polohy podľa gesta a času
void computeTargets(int *target){
  const float t = (millis() - gestureStart) * 0.001f;
  const float in = constrain(t / 0.8f, 0.0f, 1.0f);          // plynulý nábeh gesta
  const float e = in * in * (3 - 2 * in);
  for (int a = 0; a < 2; a++){
    float lift = 0, swing = 0;
    const float ph = (ARMS_SYNC || a == 0) ? 0.0f : PI;      // spolu alebo v protifáze
    switch (gesture){
      case G_UP:
        lift = UP_LIFT;
        swing = UP_SWAY * sinf(TWO_PI * 0.4f * t + ph);
        break;
      case G_WAVE:
        lift = WAVE_LIFT;
        swing = WAVE_SWING * sinf(TWO_PI * WAVE_HZ * t + ph);
        break;
      case G_UNDULATE:
        lift = UND_AMP * sinf(TWO_PI * UND_HZ * t + ph);
        swing = UND_AMP * cosf(TWO_PI * UND_HZ * t + ph);
        break;
      case G_GREET:                                          // pozdrav: pravá dopredu, ľavá máva
        if (a == GREET_RIGHT_ARM){ lift = GREET_FWD_LIFT; swing = GREET_FWD_SIGN * GREET_FWD; }
        else { lift = GREET_WAVE_LIFT; swing = GREET_WAVE_SW * sinf(TWO_PI * GREET_WAVE_HZ * t); }
        break;
      case G_SWIM: {                                         // plávanie prsia
        const float u = fmodf(t / SWIM_SEC, 1.0f);
        int k = 0; while (k < 3 && u > SWIM_KEY[k + 1][0]) k++;
        float f = (u - SWIM_KEY[k][0]) / (SWIM_KEY[k + 1][0] - SWIM_KEY[k][0]);
        f = f * f * (3 - 2 * f);                              // plynulo medzi polohami
        swing = SWIM_KEY[k][1] + (SWIM_KEY[k + 1][1] - SWIM_KEY[k][1]) * f;
        lift  = SWIM_KEY[k][2] + (SWIM_KEY[k + 1][2] - SWIM_KEY[k][2]) * f;
        break; }
      case G_SHAKE:                                          // rýchle kmitanie
        lift  = 100 + SHAKE_LIFT  * sinf(TWO_PI * SHAKE_HZ_L * t + ph);
        swing =       SHAKE_SWING * sinf(TWO_PI * SHAKE_HZ_S * t + ph + 1.0f);
        break;
      case G_ZOMBIE:                                         // zombík: natiahnuté, striedavo hore-dole
        swing = ZOMB_FWD;
        lift  = ZOMB_BASE + ZOMB_AMP * sinf(TWO_PI / ZOMB_SEC * t + (a ? PI : 0.0f));
        break;
      case G_RIPPLE: {                                       // vlnky vody
        const float w = TWO_PI / RIP_SEC * t - a * (PI / 2);
        lift  = RIP_BASE + RIP_AMP * sinf(w);
        swing = RIP_SWING * cosf(w);
        break; }
      case G_REACH:                                          // načahovanie dopredu
        swing = REACH_FWD + REACH_PULSE * sinf(TWO_PI * 0.7f * t);
        lift  = REACH_LIFT;
        break;
      case G_CIRCLE: {                                       // kruhy smerom dopredu
        const float w = TWO_PI / CIRC_SEC * t + ph;
        swing = CIRC_SWING * sinf(w);
        lift  = CIRC_BASE + CIRC_LIFT * cosf(w);
        break; }
      case G_EIGHT: {                                        // ležatá osmička
        const float w = TWO_PI / EIGHT_SEC * t;
        swing = EIGHT_SWING * sinf(w + ph);
        lift  = EIGHT_BASE + EIGHT_LIFT * sinf(2.0f * (w + ph));
        break; }
      case G_SIX7:                                           // 6-7: vždy striedavo (nezávisle od ARMS_SYNC)
        lift  = SIX7_BASE + SIX7_AMP * sinf(TWO_PI * SIX7_HZ * t + (a ? PI : 0.0f));
        swing = SIX7_FWD;
        break;
      case G_CHEER2: {                                       // hurá-hurá: jedna hore, druhá dole a naopak
        const float mid = 0.5f * (CHEER_HIGH + CHEER_LOW), amp = 0.5f * (CHEER_HIGH - CHEER_LOW) + 100;
        lift = mid + amp * sinf(TWO_PI * CHEER_HZ * t + (a ? PI : 0.0f));
        swing = 0;
        break; }
      case G_CHEER: {                                        // hej-hou: obe ruky naraz hore-dole
        const float p = 0.5f + 0.5f * sinf(TWO_PI * CHEER_HZ * t - PI / 2);
        lift = CHEER_LOW + (CHEER_HIGH - CHEER_LOW) * p;
        swing = 0;
        break; }
      default:                                               // G_NONE = pokoj
        if (IDLE_MOVE){
          lift  = IDLE_LIFT  * sinf(TWO_PI * IDLE_HZ * t + ph);
          swing = IDLE_SWING * sinf(TWO_PI * IDLE_HZ * 0.7f * t + ph + 1.0f);
        }
        break;
    }
    lift *= LIFT_GAIN[a]; swing *= SWING_GAIN[a];            // rameno, ktoré sa hýbe menej, dostane viac
    if (lift < 0) lift *= 0.5f;                            // dole len polovica – neudrie do podložky
    setOffset(LIFT[a],  lift * e,  target);
    setOffset(SWING[a], swing * e, target);
  }
}

void startGesture(Gesture g){
  gesture = g; gestureStart = millis(); lastCmd = millis();
  static const char *names[] = {"NONE", "WAVE", "UP", "UNDULATE", "CHEER", "GREET", "SIX7", "EIGHT", "CIRCLE", "REACH", "SHAKE", "ZOMBIE", "RIPPLE", "CHEER2", "SWIM"};
  Serial.print("Gesto: "); Serial.println(names[g]);
}

// diagnostika: S=2 .. S=5 pohne jedným servom o +300 µs (bez SIGN) a späť
void servoProbe(int gp){
  const int i = gp - 2;
  if (i < 0 || i >= N){ Serial.println("Zadaj S=2, S=3, S=4 alebo S=5"); return; }
  startGesture(G_NONE);
  Serial.print("GP"); Serial.print(gp); Serial.println(": pohyb +300 us (sleduj, ktore chapadlo a ktorym smerom)");
  for (int k = 0; k < 2; k++){
    const int from = pos[i], target = (k == 0) ? center[i] + 300 : center[i];
    for (int st = 1; st <= 60; st++){                        // 1,2 s s rozbehom a dobehom
      const float u = st / 60.0f, e = u * u * (3 - 2 * u);
      pos[i] = from + (int)lroundf((target - from) * e);
      sv[i].writeMicroseconds(pos[i]);
      delay(20);
    }
    delay(1200);
  }
  posF[i] = pos[i]; vel[i] = 0;
  Serial.println("hotovo");
}

void handleLine(String s){
  s.trim(); s.toUpperCase();
  if (s.length() == 0) return;
  if (s.startsWith("L:")){                                   // svetelný efekt
    s = s.substring(2);
    for (int i = 0; i < L_COUNT; i++) if (s == LIGHT_NAMES[i]){ startLight((Light)i); return; }
    Serial.print("Neznamy efekt: "); Serial.println(s); return;
  }
  if (s.startsWith("S=")){ servoProbe(s.substring(2).toInt()); return; }
  if (s.startsWith("G:")) s = s.substring(2);
  if      (s == "NONE" || s == "ZIADNE")              startGesture(G_NONE);
  else if (s == "WAVE" || s == "MAVAJ")               startGesture(G_WAVE);
  else if (s == "UP"   || s == "HORE")                startGesture(G_UP);
  else if (s == "UNDULATE" || s == "VLNENIE")         startGesture(G_UNDULATE);
  else if (s == "CHEER" || s == "HEJ")                startGesture(G_CHEER);
  else if (s == "GREET" || s == "POZDRAV")            startGesture(G_GREET);
  else if (s == "SIX7" || s == "67")                  startGesture(G_SIX7);
  else if (s == "EIGHT" || s == "OSMICKA")            startGesture(G_EIGHT);
  else if (s == "CIRCLE" || s == "KRUH")              startGesture(G_CIRCLE);
  else if (s == "REACH" || s == "NACAHNI")            startGesture(G_REACH);
  else if (s == "SHAKE" || s == "KMITAJ")             startGesture(G_SHAKE);
  else if (s == "ZOMBIE" || s == "ZOMBIK")            startGesture(G_ZOMBIE);
  else if (s == "RIPPLE" || s == "VLNKY")             startGesture(G_RIPPLE);
  else if (s == "CHEER2" || s == "HURA")              startGesture(G_CHEER2);
  else if (s == "SWIM" || s == "PLAVAJ")              startGesture(G_SWIM);
  else { Serial.print("Neznamy prikaz: "); Serial.println(s); }
}

void readLines(Stream &port, String &buf){
  while (port.available()){
    char c = (char)port.read();
    if (c == '\n' || c == '\r'){ if (buf.length()) handleLine(buf); buf = ""; }
    else if (buf.length() < 40) buf += c;
  }
}

void setup(){
  Serial.begin(115200);
  Serial1.setTX(0); Serial1.setRX(1);                        // UART od Pica s očami
  Serial1.begin(115200);
  for (int i = 0; i < N; i++){
    pos[i] = center[i]; posF[i] = center[i]; vel[i] = 0;
    sv[i].attach(PINS[i], 500, 2500);
    sv[i].writeMicroseconds(pos[i]);
    delay(250);                                              // servá po jednom -> menšia prúdová špička
  }
  for (int m = 0; m < LED_N; m++) for (int c = 0; c < 3; c++){
    pinMode(LED_PINS[m][c], OUTPUT);
    gpio_set_drive_strength(LED_PINS[m][c], GPIO_DRIVE_STRENGTH_12MA);   // plný jas modulov
  }
  analogWriteRange(255);
  stick.begin(); stick.clear(); stick.show();
  for (int i = 0; i < PX_N; i++){ pxFl[i] = 180; pxTw[i] = 0; }
  Serial.println("Chapadla pripravene. Prikazy: G:NONE G:WAVE G:UP G:UNDULATE G:CHEER (HEJ) G:GREET (POZDRAV) G:SIX7 (67) G:EIGHT (OSMICKA) G:CIRCLE (KRUH) G:REACH (NACAHNI) G:SHAKE (KMITAJ) G:ZOMBIE (ZOMBIK) G:RIPPLE (VLNKY) G:CHEER2 (HURA) G:SWIM (PLAVAJ), test serv S=2..S=5, svetlo L:CANDLE L:RED L:FIRE L:SUN L:PURPLE L:DARK L:BLUE L:GREEN L:LIGHTNING L:RAINBOW L:PINK L:SCANNER L:NIGHT L:FIREWORKS L:BLINK L:REDBLINK");
}

void loop(){
  readLines(Serial, lineUsb);
  readLines(Serial1, lineUart);
  if (gesture != G_NONE && millis() - lastCmd > GESTURE_TIMEOUT) startGesture(G_NONE);

  static unsigned long lastTick = 0;
  if (millis() - lastTick < 20) return;                      // 50 krokov za sekundu
  lastTick = millis();
  int target[N];
  computeTargets(target);
  const float dt = 0.02f;
  const float vMax = (gesture == G_SHAKE) ? SHAKE_SPEED : MAX_SPEED;
  const float aMax = (gesture == G_SHAKE) ? SHAKE_ACC   : MAX_ACC;
  for (int i = 0; i < N; i++){                               // plynulý rozbeh a dobeh ku cieľu
    const float err = target[i] - posF[i];
    // rýchlosť, pri ktorej to ešte stihne zabrzdiť presne v cieli
    float want = sqrtf(2.0f * aMax * fmaxf(0.0f, fabsf(err) - fabsf(vel[i]) * dt));
    if (want > vMax) want = vMax;
    if (err < 0) want = -want;
    vel[i] += constrain(want - vel[i], -aMax * dt, aMax * dt);
    posF[i] += vel[i] * dt;
    if (fabsf(err) < 1.0f && fabsf(vel[i]) < 20.0f){ posF[i] = target[i]; vel[i] = 0; }   // v cieli
    pos[i] = (int)lroundf(posF[i]);
    sv[i].writeMicroseconds(pos[i]);
  }
  updateLights();
}
