#include <FastLED.h>

#define PIN_LEDS       2
#define PIN_CUSTOMIZE  3
#define PIN_NEXT       4
#define PIN_GND        5

#define NUM_LEDS       256
CRGB leds[NUM_LEDS];

uint8_t btnDown;
uint32_t debounceBtnCustomize, debounceBtnNext;
#define BTN_CUSTOMIZE 0x01
#define BTN_NEXT      0x02

#define BTN_DEBOUNCE_TIME 100

#define SETLED(leds, x, y, c) {\
  if ((x) & 0x01) (leds)[((x) << 3) |    ((y) & 0x07) ] = (c); \
  else            (leds)[((x) << 3) | (7-((y) & 0x07))] = (c); \
}

void setup() {
  
  pinMode(PIN_CUSTOMIZE, INPUT_PULLUP);
  pinMode(PIN_NEXT, INPUT_PULLUP);
  pinMode(PIN_GND, OUTPUT);

  digitalWrite(PIN_GND, 0);
  
  FastLED.addLeds<WS2812B,PIN_LEDS,GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
}

uint8_t checkButtons() {

  uint8_t r = 0;
  uint32_t now = millis();

  if (now > debounceBtnCustomize) {
    
    uint8_t btn = digitalRead(PIN_CUSTOMIZE) == 0;
  
    if (!btn && (btnDown & BTN_CUSTOMIZE)) {
      
      btnDown &= ~BTN_CUSTOMIZE;
      debounceBtnCustomize = now + BTN_DEBOUNCE_TIME;
      
    } else if (btn && !(btnDown & BTN_CUSTOMIZE)) {
      
      btnDown |= BTN_CUSTOMIZE;
      debounceBtnCustomize = now + BTN_DEBOUNCE_TIME;

      r = BTN_CUSTOMIZE;
      
    }
    
  }
  
  if (now > debounceBtnNext) {
    
    uint8_t btn = digitalRead(PIN_NEXT) == 0;
  
    if (!btn && (btnDown & BTN_NEXT)) {
      
      btnDown &= ~BTN_NEXT;
      debounceBtnNext = now + BTN_DEBOUNCE_TIME;
      
    } else if (btn && !(btnDown & BTN_NEXT)) {
      
      btnDown |= BTN_NEXT;
      debounceBtnNext = now + BTN_DEBOUNCE_TIME;

      r = BTN_NEXT;
      
    }
    
  }

  return r;
  
}

void loop()
{
  e_softFireside();
  e_fireside();
  e_softblink();
  e_ripple();
  e_serial();
  e_spot();
  e_blink();
}

static inline uint16_t absdiff16(int16_t x, int16_t y) {
  return (x > y) ? x-y : y-x;
}

static inline uint16_t sqrt16_2(uint32_t inp) {
  
  uint16_t outp = 0, x;
  
  if (inp == 0) return 0; // undefined result
  if (inp == 1) return 1; // identity
  
  for (x = 0x8000; x; x = x >> 1) {

    uint16_t outp2 = outp | x;
    if ((uint32_t) outp2*outp2 <= inp) outp = outp2;
  }
  
  return outp;
  
}

uint16_t sqrt16_3(uint32_t inp) {
  
    uint32_t outp = 0;
    uint32_t one = 1uL << 30;

    while (one > inp) {
        one >>= 2;
    }

    while (one) {
      
        if (inp >= outp + one) {
            inp = inp - (outp + one);
            outp = outp + 2*one;
        }
        
        outp >>= 1;
        one >>= 2;
        
    }
    
    return outp;
    
}

void vSpot(int16_t wx, int16_t wy, uint8_t sr, CRGB color) {
  
  int8_t sx  = (wx >> 8) - sr;
  int8_t sy0 = (wy >> 8) - sr;
  uint16_t wr = sr << 8;
  uint8_t s2r = sr << 1;
  
  if (sx >= 32 || sy0 >= 8)
    return;

  uint8_t i = 0;

  if (sx < 0) {
    i = -sx;
    sx = 0;
  }
  
  for (; i <= s2r; i++) {
    
    uint8_t k;
    int8_t sy;

    if (sy0 < 0) {
      k = -sy0;
      sy = 0;
    } else {
      k = 0;
      sy = sy0;
    }
      
    for (; k <= s2r; k++) {
      
      int16_t dx = absdiff16(wx, (uint16_t) sx << 8);
      int16_t dy = absdiff16(wy, (uint16_t) sy << 8);
      uint16_t dst = sqrt16_3((uint32_t) dx*dx + (uint32_t) dy*dy);
        
      if (dst < wr) {
            
        CRGB c = color;
        c.nscale8(((uint32_t) wr - dst) * 0xFF / wr);

        SETLED(leds, sx, sy, c);
      }

      sy++;
      
      if (sy >= 8)
        break;
    }
      
    sx++;

    if (sx >= 32)
      break;
      
  }
}

void e_ripple() {

  uint16_t p = 0;
  uint8_t hue = 0;
  CRGB baseColor;

  baseColor.setHue(0);
  
  for (;;) {

    switch (checkButtons()) {
    
      case BTN_NEXT:
        return;

      case BTN_CUSTOMIZE:
        hue += 8;
        baseColor.setHue(hue);
        break;

    }

    FastLED.clear();

    int16_t cx = ((int16_t) 16 << 8) + (cos16(p << 4) >> 4);
    int16_t cy = ((int16_t)  4 << 8) + (cos16(p << 5) >> 4);

    int16_t cos_a = cos16(p << 6);
    int16_t sin_a = cos16(49152 + (p << 6));

    for (int8_t x = 0; x < 32; x++) {
      for (int8_t y = 0; y < 8; y++) {
        
        uint16_t dx = absdiff16(cx, (uint16_t) x << 8);
        uint16_t dy = absdiff16(cy, (uint16_t) y << 8);
        uint16_t dst = sqrt16_3((uint32_t) dx*dx + (uint32_t) dy*dy);

        int16_t val =
          (cos16(((uint16_t) x << 13) + (p << 6)) >> 2) +
          (cos16(((uint16_t) y << 13) + (p << 5)) >> 2) +
          (cos16((dst << 6) + (p << 3)) >> 2) +
          (cos16(((int32_t) (x-16) * cos_a + (int32_t) (y-4) * sin_a) >> 2) >> 2);
          
        if (val < 0) continue;

        CRGB c = baseColor;
        c.nscale8(val >> 7);
        
        SETLED(leds, x, y, c);
        
      }
    
    }
    
    FastLED.show();

    p++;
    
  }
  
}

#define DEBUG_SPOT

void e_spot() {
  
  static const CRGB COLORS[8] = {
    CRGB(255,0,0),
    CRGB(0,255,0),
    CRGB(0,0,255),
    CRGB(255,0,0),
    CRGB(0,255,0),
    CRGB(0,0,255),
    CRGB(255,0,0),
    CRGB(0,255,0),
  };


#ifdef DEBUG_SPOT
  Serial.begin(57600);
  Serial.println("Debug on");
  uint32_t dbgStart = millis();
#endif

  uint16_t p = 0;
  uint8_t sr = 4;

  for (;;) {

    switch (checkButtons()) {

      case BTN_NEXT:
        goto finish;

      case BTN_CUSTOMIZE:
        sr++;
        if (sr == 7) sr = 2;
        break;
        
    }
    
    FastLED.clear();
    
    for (int8_t i = -1; i < 9; i++) {
        
      uint8_t ic = i & 0x07;
      
      vSpot(
        i * 1024 + (cos16((p << 7) + ((uint16_t) ic << 12)) >> 5),
        1024 + (cos16((p << 6) + ((uint16_t) ic << 13)) >> 5),
        sr, COLORS[ic]);
    
    }

    FastLED.show();
    
    p++;

#ifdef DEBUG_SPOT
    if (p == 100) {
      Serial.println(millis() - dbgStart);
    }
#endif
  
  }

  finish:
#ifdef DEBUG_SPOT
  Serial.end();
#endif
  
}

/* -------
 *  BLINK
 * -------
 */
 
void e_blink() {

  uint8_t intensity = 4;
  
  for (;;) {

    switch (checkButtons()) {

      case BTN_NEXT:
        return;

      case BTN_CUSTOMIZE:
        intensity = (intensity & 0x07) + 1;
        break;
        
    }
      
    FastLED.clear();
    
    for (uint8_t i = 0; i < intensity; i++)
      leds[random8()] = CRGB::White;
    
    FastLED.show();

  }
}

/* --------
 *  SERIAL
 * --------
 */

#define CMD_BEGIN  0x4bc0b23d
#define CMD_END    0x60a9295c

void e_serial() {
  
  Serial.begin(57600);

  uint32_t cmd = 0;
  uint8_t led = 0, channel = 0, reading = false, splash = true, x = 0, btnPressed = false;
  uint32_t tSignal = 0;
  
  for (;;) {

    switch (checkButtons()) {

      case BTN_NEXT:
        goto finish;

      case BTN_CUSTOMIZE:
        btnPressed = true;
        break;
        
    }
  
    uint32_t t = millis();
    uint8_t hasData = false;

    while (Serial.available()) {

      hasData = true;
      
      uint8_t data = Serial.read();
      cmd = (cmd << 8) | data;
  
      if (cmd == CMD_BEGIN) {
        
        led = 0;
        channel = 0;
        reading = true;
        
        continue;
      
      }
  
      if (!reading) {
  
        if (cmd == CMD_END) {

          splash = false;
          
          FastLED.show();
          
          Serial.write(btnPressed ? 'P' : 'R');
          btnPressed = false;
          
        }
    
        continue;
  
      }
      
      switch (channel) {
        
        case 0:
        
          leds[led].r = data;
          channel = 1;
          break;
        
        case 1:
          
          leds[led].g = data;
          channel = 2;
          break;
          
        case 2:
          
          leds[led].b = data;
  
          if (led == NUM_LEDS-1) {          
            reading = false;
            break;
          }
            
          channel = 0;
          led++;
          break;
          
      }
      
    }
  
    if (hasData) {
    
      tSignal = t + 1000;
    
    } else if (t > tSignal && Serial.availableForWrite()) {

      Serial.write(btnPressed ? 'P' : 'R');
      btnPressed = false;     
      
      tSignal = t + 1000;
    
    }

    if (splash) {

      FastLED.clear();

      uint8_t cx = x >> 3;
      
      for (uint8_t i = 0; i < 16; i++) {

        cx &= 0x1F;

        CRGB c = CRGB(0, 16-i, 0);
        leds[cx << 3 | 3] = c;
        leds[cx << 3 | 4] = c;

        cx--;
        
      }

      x++;
      
      FastLED.show();
      
    }

  }

  finish:
  Serial.end();
  
}

/* ------------
 *  SOFT BLINK
 * ------------
 */

typedef struct {
  uint8_t pos, age, hue, sat;
} dot_t;

#define DOT_COUNT  64
#define MAX_AGE    255

void e_softblink() {

  dot_t dots[DOT_COUNT];

  for (uint8_t i = 0; i < DOT_COUNT; i++) {
    dots[i].age = MAX_AGE;
  }

  uint8_t hueMode = 0;
  uint32_t t = 0;
  
  for (;;) {

    t++;
    
    switch (checkButtons()) {

      case BTN_NEXT:
        return;

      case BTN_CUSTOMIZE:
        hueMode++;
        if (hueMode == 6) hueMode = 0;
        break;
        
    }
      
    FastLED.clear();
    
    for (uint8_t i = 0; i < DOT_COUNT; i++) {
      
      if (dots[i].age >= MAX_AGE) continue;

      leds[dots[i].pos] += CHSV(dots[i].hue, dots[i].sat, 255-cos8(dots[i].age));
      dots[i].age++;
      
    }
        
    FastLED.show();

    if (random8() > 100) continue;

    for (uint8_t i = 0; i < DOT_COUNT; i++) {
      if (dots[i].age < MAX_AGE) continue;
      dots[i].age = 0;
      dots[i].pos = random8();
      switch (hueMode) {
        case 0:
          dots[i].hue = t >> 12;
          dots[i].sat = 255;
          break;
        case 1:
          dots[i].hue = 64 + (t >> 12);
          dots[i].sat = 255;
          break;
        case 2:
          dots[i].hue = 128 + (t >> 12);
          dots[i].sat = 255;
          break;
        case 3:
          dots[i].hue = 192 + (t >> 12);
          dots[i].sat = 255;
          break;
        case 4:
          dots[i].hue = random8();
          dots[i].sat = 255;
          break;
        case 5:
          dots[i].hue = 0;
          dots[i].sat = 0;
          break;
      }
      break;
    }

  }
}

/* ------------
 *  FIRESIDE
 * ------------
 */

void e_fireside() {

  uint8_t intensity[32];

  for (uint8_t x = 0; x < 32; x++)
    intensity[x] = 0;

  uint8_t level = 0;
  uint8_t levelIndicatorDebounce = 0;
  
  for (;;) {

    switch (checkButtons()) {

      case BTN_NEXT:
        return;

      case BTN_CUSTOMIZE:
        level = (level + 1) & 0x07;
        levelIndicatorDebounce = 100;
        break;
        
    }
      
    FastLED.clear();
    
    for (uint8_t x = 0; x < 32; x++) {
      
      for (uint8_t y = 0; y < 8; y++) {

        int16_t i = intensity[x];
        
        int16_t h =  (i >> 3) + (((int16_t) y) << 2) - 32;
        int16_t s = -(i     ) - (((int16_t) y) << 2) + 485;
        int16_t v =  (i     ) + (((int16_t) y) << 5) - 240;
        
        if (h > 255) h = 255;
        if (h < 0)   h = 0;
        
        if (s > 255) s = 255;
        if (s < 0)   s = 0;
        
        if (v > 255) v = 255;
        if (v < 0)   v = 0;
        
        CRGB c = CHSV(h, s, v);
        
        SETLED(leds, x, y, c);
        
      }
      
    }
    
    if (levelIndicatorDebounce) {

      CRGB c = CRGB(0, 255, 0);
      SETLED(leds, 16, 7-level, c);
      levelIndicatorDebounce--;
      
    }
    
    FastLED.show();
        
    for (uint8_t x = 0; x < 32; x++) {
      
      uint16_t newIntensity = intensity[x];
      
      if (random8() < 40)
        newIntensity += level + 6;
      else
        newIntensity = (newIntensity * 127) >> 7;
      
      if (newIntensity > 255)
        newIntensity = 255;
        
      intensity[x] = newIntensity;
    
    }
    
  }
  
}

/* ------------
 *  SOFT FIRESIDE
 * ------------
 */

void e_softFireside() {

  uint16_t intensity[32];
  uint8_t boost[32];

  for (uint8_t x = 0; x < 32; x++) {
    intensity[x] = 0;
    boost[x] = 0;
  }

  uint8_t level = 0;
  uint8_t levelIndicatorDebounce = 0;
  uint8_t t = 0;
  
  for (;;) {

    switch (checkButtons()) {

      case BTN_NEXT:
        return;

      case BTN_CUSTOMIZE:
        level = (level + 1) & 0x07;
        levelIndicatorDebounce = 100;
        break;
        
    }
      
    FastLED.clear();
    
    for (uint8_t x = 0; x < 32; x++) {
      
      for (uint8_t y = 0; y < 8; y++) {

        int16_t i = intensity[x] >> 8;
        
        int16_t h =  (i >> 3) + (((int16_t) y) << 2) - 32;
        int16_t s = -(i     ) - (((int16_t) y) << 2) + 485;
        int16_t v =  (i     ) + (((int16_t) y) << 5) - 240;
        
        if (h > 255) h = 255;
        if (h < 0)   h = 0;
        
        if (s > 255) s = 255;
        if (s < 0)   s = 0;
        
        if (v > 255) v = 255;
        if (v < 0)   v = 0;
        
        CRGB c = CHSV(h, s, v);

        SETLED(leds, x, y, c);

      }
      
    }

    if (levelIndicatorDebounce) {

      CRGB c = CRGB(0, 255, 0);
      SETLED(leds, 16, 7-level, c);
      levelIndicatorDebounce--;
      
    }
        
    FastLED.show();
        
    for (uint8_t x = 0; x < 32; x++) {
      
      uint32_t i = intensity[x];
      
      if (boost[x])
        i += 200;
      else
        i = (i * 255) >> 8;
      
      if (i > 0xFFFF)
        i = 0xFFFF;
        
      intensity[x] = i;
    
    }

    if (t == 0) {
      t = 100;
      uint8_t ths = 10 + level * 20;
      for (uint8_t x = 0; x < 32; x++) {
        boost[x] = random8() < ths;
      }
    }

    t--;
    
  }
  
}
