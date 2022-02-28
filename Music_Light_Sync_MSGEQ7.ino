#include <Adafruit_NeoPixel.h>
#include "WS2812_Definitions.h"
#ifdef __AVR__
#include <avr/power.h>
#endif

#define LED_DATA 6
#define POT_BRIGHTNESS A2
// LEDs on strip to fill
#define NUM_LEDS 153         
#define BTN_PIN 3                   
#define BRIGHTNESS 255
// Used for switching between modes
#define MAX_MODE 11          
#define DELAY 13

//  MSGEQ7 Pin Connections
#define MSGEQ_OUT A1
#define STROBE 2
#define RESET 5

int level[7];

#define ISR_DELAY 400

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);


byte neopix_gamma[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

uint16_t time_since_isr=0;
boolean breakMode=false;
uint8_t mode=0;
int spectrumValue[7];
uint8_t mapValue[7];
uint32_t audioBuffer[NUM_LEDS];
int filter = 0;
float analogVal;

uint16_t brightness;

bool countUp=true;
uint32_t lastColor=0;

void setup() {

  brightness = BRIGHTNESS;
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show();

  //Serial connection for debug info
  Serial.begin(9600);
  Serial.println("START");
  pinMode(MSGEQ_OUT, INPUT);
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(BTN_PIN, INPUT);
  
  digitalWrite(RESET, LOW);
  digitalWrite(STROBE, HIGH);

  pinMode(POT_BRIGHTNESS, INPUT);
 
  attachInterrupt(1, changeMode, RISING);
}

void loop() {
  breakMode=false;
  switch(mode){
    case 0:
      Serial.println("Music Pulse");
      musicPulse();
      break;
   case 1:                        
      Serial.println("Music");
      musicAnalyzer();
      break;
   case 2:
      Serial.println("Red");
      plainColor(255,0,0);
      break;
    case 3:
      Serial.println("Green");
      plainColor(0,255,0);
      break;
    case 4:
      Serial.println("Blue");
      plainColor(0,0,255);
      break;
   case 5:
      Serial.println("Rainbow");
      rainbowFade2White(400,255,10);
      break;
   case 6:
      Serial.println("ColorRoom");
      colorRoom();
      break;
   case 7:
      Serial.println("Dynamic");
      musicDynamic();
      break;
   case 8:
      Serial.println("Christmas Colors");
      christmasMusic();
      break;
    case 9:
      Serial.println("Silver and Gold");
      silverGoldMusic();
      break;
   default:
      Serial.println("DEFAULT");
      colorWipe(255,30);
      break;
  }
}

// Switch between color modes
void changeMode() {
  if(millis()-time_since_isr>ISR_DELAY){
    Serial.println("PRESSED");
    time_since_isr=millis();
    mode=(mode+1)%MAX_MODE;
    breakMode=true;
  }
}
// Checks for changes in potentiometer value 
boolean checkBrightness(){
  //WS2812 takes value between 0-255
  uint16_t bright = map(constrain(analogRead(POT_BRIGHTNESS),0,1024), 0, 1024, 10, 255);
  if (abs(bright-brightness)>10){
    brightness = bright;
    strip.setBrightness(brightness);
    return true;
  }
  return false;
}

// Show a fading rainbow
void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops) {
  float fadeMax = 100.0;
  int fadeVal = fadeMax;
  uint32_t wheelVal;
  int redVal, greenVal, blueVal;

  for (int k = 0 ; k < rainbowLoops ; k ++) {
    for (int j = 0; j < 256; j++) { 
      for (int i = 0; i < strip.numPixels(); i++) {
        wheelVal = Wheel(((i * 256 / strip.numPixels()) + j) & 255);
        redVal = red(wheelVal) * float(fadeVal / fadeMax);
        greenVal = green(wheelVal) * float(fadeVal / fadeMax);
        blueVal = blue(wheelVal) * float(fadeVal / fadeMax);
        strip.setPixelColor( i, strip.Color( redVal, greenVal, blueVal ) );
      }
      if(breakMode)
        return;
        checkBrightness();
        strip.show();
        delay(wait);
    }
  }
  delay(500);
}

// Uses potentiometer to change color
void colorRoom(){
    strip.setBrightness(255);
    uint32_t colorNew = map(constrain(analogRead(POT_BRIGHTNESS),0,1024), 0, 1024, 0, 255);
    if (abs(colorNew-lastColor)>10){
      lastColor=colorNew;
      for (int i = 0; i < strip.numPixels(); i++)
             strip.setPixelColor(i, Wheel((colorNew) & 255)); 
      strip.show();
    }
    delay(10);
}

void plainWhite(){
  for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor( i, strip.Color( 255, 255, 255 ) );
   }
  strip.show();
  while(!breakMode){
    if(checkBrightness())
      strip.show();
  }
}

//Shows plain color along the strip
void plainColor(uint8_t red,uint8_t green,uint8_t blue){
  for (int i = 0; i < strip.numPixels(); i++)
        strip.setPixelColor( i, strip.Color( red, green, blue ) );
  strip.show();
  while(!breakMode){
    if(checkBrightness())
      strip.show();
  }
}

/* Shows plain color
void plainColorNoBright(uint8_t red,uint8_t green,uint8_t blue){
  for (int i = 0; i < strip.numPixels(); i++)
        strip.setPixelColor( i, strip.Color( red, green, blue ) );
  strip.show();
  while(!breakMode){
    delay(100);
  }
}
*/
void Bass(unsigned long color , byte brightness)
{
  int i = 0;
  
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0x00FF00) >> 8;
  byte blue = (color & 0x0000FF);
  
  while(i <= NUM_LEDS)
  {
    strip.setPixelColor(i, red/(brightness), green/(brightness), blue/(brightness));
    i = i + 7;
  }
  
  strip.show();
}

void Bass1(unsigned long color , byte brightness)
{
  int i = 1;
  
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0x00FF00) >> 8;
  byte blue = (color & 0x0000FF);
  
  while(i <= NUM_LEDS)
  {
    strip.setPixelColor(i, red/(brightness), green/(brightness), blue/(brightness));
    i = i + 7;
  }
  
  strip.show();
}

void Midrange(unsigned long color , byte brightness)
{
  int i = 2;
  
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0x00FF00) >> 8;
  byte blue = (color & 0x0000FF);
  
  while(i <= NUM_LEDS)
  {
    strip.setPixelColor(i, red/(brightness), green/(brightness), blue/(brightness));
    i = i + 7;
  }
}

void Midrange1(unsigned long color , byte brightness)
{
  int i = 3;
  
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0x00FF00) >> 8;
  byte blue = (color & 0x0000FF);
  
  while(i <= NUM_LEDS)
  {
    strip.setPixelColor(i, red/(brightness), green/(brightness), blue/(brightness));
    i = i + 7;
  }
}

void Midrange2(unsigned long color , byte brightness)
{
  int i = 4;
  
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0x00FF00) >> 8;
  byte blue = (color & 0x0000FF);
  
  while(i <= NUM_LEDS)
  {
    strip.setPixelColor(i, red/(brightness), green/(brightness), blue/(brightness));
    i = i + 7;
  }
}

void Highs(unsigned long color , byte brightness)
{
  int i = 5;
  
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0x00FF00) >> 8;
  byte blue = (color & 0x0000FF);
  
  while(i <= NUM_LEDS)
  {
    strip.setPixelColor(i, red/(brightness), green/(brightness), blue/(brightness));
    i = i + 7;
  }
}

void Highs1(unsigned long color , byte brightness)
{
  int i = 6;
  
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0x00FF00) >> 8;
  byte blue = (color & 0x0000FF);
  
  while(i <= NUM_LEDS)
  {
    strip.setPixelColor(i, red/(brightness), green/(brightness), blue/(brightness));
    i = i + 7;
  }
}

void StepStrobe()
{
  digitalWrite(STROBE, HIGH);
  digitalWrite(STROBE, LOW);
  delayMicroseconds(30);
}


void musicDynamic() {
  while(true){
    checkBrightness();
    if(breakMode)
      return;
      
    //Reset MSGEQ
    digitalWrite(RESET, HIGH);
    digitalWrite(RESET, LOW);
    int sum =0;
    for (int i = 0; i < 7; i++) {
      digitalWrite(STROBE, LOW);
      delayMicroseconds(35);                  // Delay necessary due to timing diagram
      level[i]=analogRead(MSGEQ_OUT);

      digitalWrite(STROBE, HIGH);
      delayMicroseconds(35);                    // Delay necessary due to timing diagram  
    }
    int PWMvalues[7];
 
    int sensitivity = 175;
    for (int i = 0; i < 7; i++) {
      PWMvalues[i] = map(level[i], 60, 1023, sensitivity, 1);  // scale analogRead's value to Write's 255 max
    }

    
    StepStrobe();
    Bass(MAROON, PWMvalues[0]);
    
    StepStrobe();
    Bass1(ORANGERED, PWMvalues[1]);
  
    StepStrobe();
    Midrange(OLIVE, PWMvalues[2]);
  
    StepStrobe();
    Midrange1(LIME, PWMvalues[3]);
  
    StepStrobe();
    Midrange2(FORESTGREEN, PWMvalues[4]);
    
    StepStrobe();
    Highs(LIGHTSTEELBLUE, PWMvalues[5]);
    
    StepStrobe();
    Highs1(DARKBLUE, PWMvalues[6]);
        
  }
}

void christmasMusic() {
  while(true){
    checkBrightness();
    if(breakMode)
      return;
      
    //Reset MSGEQ
    digitalWrite(RESET, HIGH);
    digitalWrite(RESET, LOW);
    int sum =0;
    for (int i = 0; i < 7; i++) {
      digitalWrite(STROBE, LOW);
      delayMicroseconds(35);                  // Delay necessary due to timing diagram
      level[i]=analogRead(MSGEQ_OUT);

      digitalWrite(STROBE, HIGH);
      delayMicroseconds(35);                    // Delay necessary due to timing diagram  
    }
    int PWMvalues[7];
    int sensitivity = 175;
    for (int i = 0; i < 7; i++) {
      PWMvalues[i] = map(level[i], 60, 1023, sensitivity, 1);  // scale analogRead's value to Write's 255 max
    }

    
    StepStrobe();
    Bass(DARKRED, PWMvalues[0]);
    
    StepStrobe();
    Bass1(DARKGREEN, PWMvalues[1]);
  
    StepStrobe();
    Midrange(MAROON, PWMvalues[2]);
  
    StepStrobe();
    Midrange1(GREEN, PWMvalues[3]);
  
    StepStrobe();
    Midrange2(RED, PWMvalues[4]);
    
    StepStrobe();
    Highs(LIME, PWMvalues[5]);
    
    StepStrobe();
    Highs1(INDIANRED, PWMvalues[6]);
        
  }
}

void silverGoldMusic() {
  while(true){
    checkBrightness();
    if(breakMode)
      return;
      
    //Reset MSGEQ
    digitalWrite(RESET, HIGH);
    digitalWrite(RESET, LOW);
    int sum =0;
    for (int i = 0; i < 7; i++) {
      digitalWrite(STROBE, LOW);
      delayMicroseconds(35);                  // Delay necessary due to timing diagram
      level[i]=analogRead(MSGEQ_OUT);

      digitalWrite(STROBE, HIGH);
      delayMicroseconds(35);                    // Delay necessary due to timing diagram  
    }
    int PWMvalues[7];
    int sensitivity = 175;
    for (int i = 0; i < 7; i++) {
      PWMvalues[i] = map(level[i], 60, 1023, sensitivity, 1);  
    }

    
    StepStrobe();
    Bass(DARKKHAKI, PWMvalues[0]);
    
    StepStrobe();
    Bass1(LIGHTGRAY, PWMvalues[1]);
  
    StepStrobe();
    Midrange(GOLD, PWMvalues[2]);
  
    StepStrobe();
    Midrange1(SILVER, PWMvalues[3]);
  
    StepStrobe();
    Midrange2(YELLOW, PWMvalues[4]);
    
    StepStrobe();
    Highs(GRAY, PWMvalues[5]);
    
    StepStrobe();
    Highs1(GOLDENROD, PWMvalues[6]);
        
  }
}

void musicPulse() {
  while(true){
    checkBrightness();
    if(breakMode)
      return;
      
    //Reset MSGEQ
    digitalWrite(RESET, HIGH);
    digitalWrite(RESET, LOW);
    int sum =0;
    for (int i = 0; i < 7; i++) {
      digitalWrite(STROBE, LOW);
      delayMicroseconds(35);                  // Delay necessary due to timing diagram
      level[i]=analogRead(MSGEQ_OUT);

      digitalWrite(STROBE, HIGH);
      delayMicroseconds(35);                    // Delay necessary due to timing diagram  
    }
    
    byte brightnessValue[7];
    
    for (int i = 0; i < 7; i++) {
      brightnessValue[i] = map(level[i], 60, 1023, 250, 1);
    }

    byte red = (0xFF6347 & 0xFF0000) >> 16;
    byte green = (0x8FBC8F & 0x00FF00) >> 8;
    byte blue = (0x6A5ACD & 0x0000FF);
   
    for ( int i = 0; i < (NUM_LEDS/4)+2; i++) {
       strip.setPixelColor(i, red/(brightnessValue[4]), green/(brightnessValue[0]), blue/(brightnessValue[2]));
       i=i+1;
    }
    for ( int i = 1; i < (NUM_LEDS/4)+2; i++) {
       strip.setPixelColor(i, red/(brightnessValue[5]), green/(brightnessValue[1]), blue/(brightnessValue[3]));
       i=i+1;
    }

    
    for ( int i = (NUM_LEDS/4)+2; i < NUM_LEDS/2; i++) {
       strip.setPixelColor(i, red/(brightnessValue[4]), green/(brightnessValue[0]), blue/(brightnessValue[2]));
       i=i+1;
    }
    for ( int i = ((NUM_LEDS/4)+2)+1; i < NUM_LEDS/2; i++) {
      strip.setPixelColor(i, red/(brightnessValue[5]), green/(brightnessValue[1]), blue/(brightnessValue[3]));
       i=i+1;
    }

    
    for ( int i = (NUM_LEDS/2)-2; i < (NUM_LEDS-(NUM_LEDS/4)); i++) {
       strip.setPixelColor(i, red/(brightnessValue[4]), green/(brightnessValue[0]), blue/(brightnessValue[2]));
       i=i+1;
    }
    for ( int i = ((NUM_LEDS/2)-2)+1; i < (NUM_LEDS-(NUM_LEDS/4)); i++) {
       strip.setPixelColor(i, red/(brightnessValue[5]), green/(brightnessValue[1]), blue/(brightnessValue[3]));
       i=i+1;
    }

    
    for ( int i = (NUM_LEDS-(NUM_LEDS/4))-2; i < NUM_LEDS; i++) {
       strip.setPixelColor(i, red/(brightnessValue[4]), green/(brightnessValue[0]), blue/(brightnessValue[2]));
       i=i+1;
    }
    for ( int i = ((NUM_LEDS-(NUM_LEDS/4))-2)+1; i < NUM_LEDS; i++) {
       strip.setPixelColor(i, red/(brightnessValue[5]), green/(brightnessValue[1]), blue/(brightnessValue[3]));
       i=i+1;
    }

    strip.show();
    delay(DELAY);
      
 
  }
}

void musicAnalyzer(){
  while(true){
    checkBrightness();
    if(breakMode)
      return;
    //Reset MSGEQ
    digitalWrite(RESET, HIGH);
    digitalWrite(RESET, LOW);
    //Read Audio Bands
    for (int i = 0; i < 7; i++){
      digitalWrite(STROBE, LOW);
      delayMicroseconds(30);
      spectrumValue[i] = analogRead(MSGEQ_OUT);
      spectrumValue[i] = constrain(spectrumValue[i], filter, 1023);
      digitalWrite(STROBE, HIGH);
      
      mapValue[i] = map(spectrumValue[i], filter, 1023, 0, 255);
      if (mapValue[i] < 50)
        mapValue[i] = 0;
    }
    // Shift LEDs forward
    for (int k = NUM_LEDS - 1; k > 0; k--) {
      audioBuffer[k] = audioBuffer[k - 1];
    }
    
    // Shift LEDs backward instead of forwards
    // for (int k = 0; k < NUM_LEDS-1; k++) {
    //  audioBuffer[k] = audioBuffer[k + 1];
    // }
    
    // Load new Audio value to first LED
    // Uses band 0,2,4 (Bass(red), Middle(green), High(blue) Frequency Band)
    // Lowest 8Bit: Blue , Middle Green , Highest Red
    // Use audioBuffer[NUM_LEDS-1] when using LED shift backwards!
    audioBuffer[0] = mapValue[5]; //RED
    audioBuffer[0] = audioBuffer[0] << 16;
    audioBuffer[0] |= ((mapValue[2] /2) << 8);  //GREEN
    audioBuffer[0] |= (mapValue[4]/4);         //BLUE
    
    // Send new LED values to WS2812
    for ( int i = 0; i < NUM_LEDS; i++)
      strip.setPixelColor(i, strip.Color(audioBuffer[i] >> 16, (audioBuffer[i] >> 8)&B11111111, audioBuffer[i]&B11111111));
    strip.show();
    delay(DELAY);
  }
}

// Fill the dots one by one
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void pulseWhite(uint8_t wait) {
  for (int j = 0; j < 256 ; j++) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0, neopix_gamma[j] ) );
    }
    checkBrightness();
    if(breakMode)
      return;
    delay(wait);
    strip.show();
  }
  for (int j = 255; j >= 0 ; j--) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0, neopix_gamma[j] ) );
    }
    delay(wait);
    strip.show();
  }
}

void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength ) {

  if (whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;

  int head = whiteLength - 1;
  int tail = 0;

  int loops = 3;
  int loopNum = 0;

  static unsigned long lastTime = 0;


  while (true) {
    for (int j = 0; j < 256; j++) {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        if ((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head) ) {
          strip.setPixelColor(i, strip.Color(0, 0, 0, 255 ) );
        }
        else {
          strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }

      }

      if (millis() - lastTime > whiteSpeed) {
        head++;
        tail++;
        if (head == strip.numPixels()) {
          loopNum++;
        }
        lastTime = millis();
      }

      if (loopNum == loops) return;

      head %= strip.numPixels();
      tail %= strip.numPixels();
      strip.show();
      delay(wait);
    }
  }

}
void fullWhite() {

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, 255 ) );
  }
  strip.show();
}


// Rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to change color
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3, 0);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}

uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
}
uint8_t blue(uint32_t c) {
  return (c);
}
