#define  FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

const byte realLedCount = 153;


const int firstPin = D7;
const int secondPin = D8;

// there are 14 led strips (12 on tubes, one for the tail lights and one for the dashboard)
const byte numTubes = 7;

const byte ledPinCount = 2;

// each tube has a different number of lights
const int longestTube = 42;
const int tubeLengths[numTubes] = {39, 40, 42, 32, 41, 40, 39} ;
const int tubeStart[numTubes] = {0, 39, 79, 121, 79, 39, 0};
const byte tubes[numTubes] = {0,0,0,0,1,1,1};
const byte tubeDirection[numTubes] = {0,1,0,1,0,1,0};

// these are virtual tubes or leds
CRGB leds[numTubes][longestTube];
CRGB ledBuffer[numTubes][longestTube];

// realLeds are the physical strips
CRGB realLeds[ledPinCount][realLedCount];

const byte UPDATES_PER_SECOND = 200;
const byte BRIGHTNESS = 96;
const byte FRAMES_PER_SECOND = 120;

// globals for FX animations
int BOTTOM_INDEX = 0;
int TOP_INDEX = int(longestTube/2);
int FIRST_THIRD = int(longestTube/3);
int SECOND_THIRD = FIRST_THIRD * 2;
int EVENODD = longestTube%2;

byte tailLights = 6;
//byte cushionsBack = 13;
//byte cushionFront = 14;

CRGB ledsX[longestTube]; //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, ETC)
const byte longestTubes[] = { };

int ledMode = 23;      //-START IN DEMO MODE
//int ledMode = 5;

// Globals
int idex = 0;        //-LED INDEX (0 to longestTube-1
byte ihue = 0;        //-HUE (0-360)
int ibright = 0;     //-BRIGHTNESS (0-255)
int isat = 0;        //-SATURATION (0-255)
bool bounceForward = true;  //-SWITCH FOR COLOR BOUNCE (0-1)
int bouncedirection = 0;
float tcount = 0.0;      //-INC VAR FOR SIN LOOPS
int lcount = 0;      //-ANOTHER COUNTING VAR
//byte eq[7] ={};


// color palette related stuff
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

// The 16 bit version of our coordinates
static uint16_t noiseX;
static uint16_t noiseY;
static uint16_t noiseZ;

// We're using the x/y dimensions to map to the x/y pixels on the matrix.  We'll
// use the z-axis for "time".  speed determines how fast time moves forward.  Try
// 1 for a very slow moving effect, or 60 for something that ends up looking like
// water.
uint16_t speed = 20; // speed is set dynamically once we've started up

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.
uint16_t scale = 30; // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
uint8_t noise[numTubes][longestTube];

uint8_t colorLoop = 1;

// i don't know why I need to declare these...
void Fire2012WithPalette();
void rotatingRainbow(); 
void rainbowWithGlitter();
void spiralRainbow();
void police_lightsALL();
void sinelon();
void juggle();
void mapNoiseToLEDsUsingPalette();

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { 
    Fire2012WithPalette,
    rotatingRainbow, 
    rainbowWithGlitter, 
    spiralRainbow,
    mapNoiseToLEDsUsingPalette,
    police_lightsALL,
    sinelon, 
    juggle
};

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns


//
// Mark's xy coordinate mapping code.  See the XYMatrix for more information on it.
//
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;
  i = (y * numTubes) + x;
  return i;
}

//------------------SETUP------------------
void setup()
{
  delay(1000);
            
  LEDS.addLeds<WS2812, firstPin, GRB>(realLeds[0], realLedCount) ;
  LEDS.addLeds<WS2812, secondPin, GRB>(realLeds[1], realLedCount) ;

  LEDS.setBrightness(127);
        
  currentPalette = HeatColors_p;
  currentBlending = LINEARBLEND;

  Serial.begin(57600);
  Serial.println(F("https://github.com/zekekoch/Bike2018"));
  fillSolid(0,0,0); //-BLANK STRIP
    
  showLeds();
}

// color my tubes so that I can make sure that my mapping works
void debugColors()
{
  for(byte tube = 0;tube< numTubes;tube++)
  {
    // make every 10 purple
    for(int l = 1;l<tubeLengths[tube];l++)
    {
      if (l % 10 == 0)
            leds[tube][l] = CRGB::Purple;
    }

    // make the last one blue
    leds[tube][tubeLengths[tube]-1] = CRGB::Blue;
    
    // make the first n leds green
    for (int h = 1; h < tube+2; h++) 
    {
      leds[tube][h] = CRGB::Green;
    }

    // first one red
    leds[tube][0] = CRGB::Red;
  }
}

// roughly map leds dealing with the fact that
// I want to stay integer math, but need fractions
// this is sloppy, but close enough for now
int mapLed(int led, int from, int to)
{
    if (from == to)
        return led;
    else
        return led * ((from * 1024)/to) / (1024); 
}


// the leds for the tubes are in an array called leds
// the array that FastLED uses to write to the actual strips is 
// called realLeds.
void showLeds()
{
  printLedArray();

  // loop over my tubes
  for(byte tube = 0; tube < numTubes; tube++) 
  {
    
    byte fastLedStrip = tubes[tube];

    if (0 == tubeDirection[tube])
    {      
      for(int iLed = 0; iLed < tubeLengths[tube]; iLed++) 
      {
          realLeds[fastLedStrip][iLed + tubeStart[tube]] = leds[tube][iLed];
      }
    }
    else
    {      
      for(int iLed = 0; iLed < tubeLengths[tube]; iLed++) 
      {
        realLeds[fastLedStrip][tubeStart[tube] + tubeLengths[tube] -1 - iLed] = leds[tube][iLed];
      }
    }
  }
  printStripArray();

  LEDS.show();
  delay(3000);
}


void printLedArray()
{
  for(byte tube = 0;tube < numTubes;tube++)
  {
    Serial.print("[");
    for(byte led = 0;led < tubeLengths[tube];led++)
    {
      Serial.print(printColorToChar(leds[tube][led]));
    }
    Serial.println("]");
  }
}

void printStripArray()
{
  for(byte strip = 0;strip < ledPinCount;strip++)
  {
    Serial.print(strip);Serial.print("[");
    for(byte led = 0;led < realLedCount;led++)
    {
      Serial.print(printColorToChar(realLeds[strip][led]));
    }
    Serial.println("]");
  }
}


char printColorToChar(CRGB ledColor)
{
  if((CRGB)CRGB::Red == ledColor)
      return 'R';
  if((CRGB)CRGB::Blue == ledColor)
      return 'B';
  if((CRGB)CRGB::Green == ledColor)
      return 'G';
  if((CRGB)CRGB::Purple == ledColor)
      return 'P';
  else
      return ' ';
}

void colorPaletteLoop()
{
    ChangePalettePeriodically();
    
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */
    
    FillLEDsFromPaletteColors( startIndex, 3);
    
    showLeds();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void FillLEDsFromPaletteColors( uint8_t colorIndex, uint8_t width)
{
    uint8_t brightness = 255;
    
    for(int iStrip = 0;iStrip < numTubes;iStrip++)
    {
        for( int iLed = 0; iLed < longestTube;iLed++) 
        {
            leds[iStrip][iLed] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
            // how quickly you move through the colors is how wide the strips are of a given color
            colorIndex+=width;
        }
    }
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if( secondHand == 10)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; i++) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}

// This function sets up a palette of purple and green stripes.
void SetupAmericaPalette()
{
    currentPalette = CRGBPalette16
    (
        CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red,  
        CRGB::Red, CRGB::White, CRGB::White, CRGB::White, 
        CRGB::White, CRGB::White, CRGB::White, CRGB::Blue, 
        CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue 
    );
}



// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    // Gray looks better than White because white is brighter than blue and red
    CRGB::Red, CRGB::Gray, CRGB::Blue, CRGB::Black,
    CRGB::Red, CRGB::Gray, CRGB::Blue, CRGB::Black,
    CRGB::Red, CRGB::Red, CRGB::Gray, CRGB::Gray,    
    CRGB::Blue, CRGB::Blue, CRGB::Black, CRGB::Black
};


//------------------------------------- UTILITY FXNS --------------------------------------

//-SET THE COLOR OF A SINGLE RGB LED on all tubes
void setPixel(int adex, int cred, int cgrn, int cblu) {
    if (adex < 0 || adex > longestTube-1)
        return;

    for(int i = 0;i<numTubes;i++)
    {
        leds[i][adex] = CRGB(cred, cgrn, cblu);
    }
}

void setPixel(int adex, CRGB c) {
    if (adex < 0 || adex > longestTube-1)
        return;

    for(int i = 0;i<numTubes;i++)
    {
        leds[i][adex] = c;
    }
}

//-FIND INDEX OF HORIZONAL OPPOSITE LED
int horizontal_index(int i) {
    //-ONLY WORKS WITH INDEX < TOPINDEX
    if (i == BOTTOM_INDEX) {
        return BOTTOM_INDEX;
    }
    if (i == TOP_INDEX && EVENODD == 1) {
        return TOP_INDEX + 1;
    }
    if (i == TOP_INDEX && EVENODD == 0) {
        return TOP_INDEX;
    }
    return longestTube - i;
}


//-FIND INDEX OF ANTIPODAL OPPOSITE LED
int antipodal_index(int i) {
    //int N2 = int(longestTube/2);
    int iN = i + TOP_INDEX;
    if (i >= TOP_INDEX) {
        iN = ( i + TOP_INDEX ) % longestTube; 
    }
    return iN;
}

int nextThird(int i) {
    int iN = i + (int)(longestTube / 3);
    if (iN >= longestTube) {
        iN = iN % longestTube;
    }
    return iN;
}

int adjacent_cw(int i) {
    int r;  
    if (i < 0) {
        r = 0;
    }
    else if (i >= longestTube - 1) {
        r = longestTube;
    }
    else {
        r = i + 1;
    }

    //Serial.print("cw from ");Serial.print(i);Serial.print(" to ");Serial.print(r);Serial.println();
    return r;
}

//-FIND ADJACENT INDEX COUNTER-CLOCKWISE
int adjacent_ccw(int i) {
    int r;

    if (i <= 0) {
        r = 0;
    }
    else if (i > longestTube) {
        r = longestTube - 1;
    }
    else {
        r = i - 1;
    }

    //Serial.print("ccw from ");Serial.print(i);Serial.print(" to ");Serial.print(r);Serial.println();
    return r;
}

//-CONVERT HSV VALUE TO RGB
void HSVtoRGB2(int hue, int sat, int val, int colors[3]) {
    Serial.println("depricate HSVtoRGB with int array");
    CRGB c;
    hsv2rgb_rainbow(CHSV(hue, sat, val), c);
    colors[0] = c.r;
    colors[1] = c.g;
    colors[2] = c.b;
}

void HSVtoRGB(int hue, int sat, int val, CRGB& c) {
    hsv2rgb_rainbow(CHSV(hue, sat, val), c);
}

CRGB HSVtoRGB(int hue, int sat, int val) {
    CRGB c;
    hsv2rgb_rainbow(CHSV(hue, sat, val), c);
    return c;
}

// todo: make this work with multiple arrays
void copy_led_array(){
    for(int i = 0; i < longestTube; i++ ) {
        ledsX[i][0] = leds[0][i].r;
        ledsX[i][1] = leds[0][i].g;
        ledsX[i][2] = leds[0][i].b;
    }
}

void fillBuffer()
{
    for(int iTube = 0; iTube < numTubes; iTube++ ) 
    {
        for(int iLed = 0; iLed < longestTube;iLed++)
        {
            ledBuffer[iTube][iLed] = leds[iTube][iLed];
        }
    }
}


// todo: make this work with multiple arrays
void print_led_arrays(int ilen){
    copy_led_array();
    Serial.println("~~~***ARRAYS|idx|led-r-g-b|ledX-0-1-2");
    for(int i = 0; i < ilen; i++ ) {
        Serial.print("~~~");
        Serial.print(i);
        Serial.print("|");
        Serial.print(leds[0][i].r);
        Serial.print("-");
        Serial.print(leds[0][i].g);
        Serial.print("-");
        Serial.print(leds[0][i].b);
        Serial.print("|");
        Serial.print(ledsX[i][0]);
        Serial.print("-");
        Serial.print(ledsX[i][1]);
        Serial.print("-");
        Serial.println(ledsX[i][2]);
    }
}

//------------------------LED EFFECT FUNCTIONS------------------------

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[random8(14)][ random16(longestTube) ] += CRGB::White;
  }
}


void fillSolid(byte strand, const CRGB& color)
{
    // fill_solid -   fill a range of LEDs with a solid color
 fill_solid( leds[strand], longestTube, color);
}

void fillSolid(CRGB color)
{
    for(int i = 0;i<numTubes;i++)
        fillSolid(i, color);
}

void fillSolid(int cred, int cgrn, int cblu) { //-SET ALL LEDS TO ONE COLOR
    fillSolid(0, CRGB(cred, cgrn, cblu));    
}

void explosion()
{
    static int counter = 0;
    counter++; 

    leds[random8(numTubes)][random8(longestTube)] = CRGB(255,0,0);
//    leds[5][5] = CRGB(255,0,0);

    fillBuffer();

//    blur2d(*leds, numTubes, longestTube, 64);

    for(byte iStrip = 1;iStrip < numTubes-1;iStrip++)
    {
        for(int iLed = 1; iLed < longestTube-1;iLed++)
        {

            if (ledBuffer[iStrip][iLed].red > 200)
            {            
                ledBuffer[iStrip][iLed].red = 0;
                //printLed(iStrip, iLed);
            }

            // BUG: why am I reseting count here?
            int count = 0;
            if (ledBuffer[iStrip-1][iLed].red > 200)
            {
                count++;
            } 
            if (ledBuffer[iStrip-1][iLed-1].red > 200)
            {
                count++;
            }
            if (ledBuffer[iStrip][iLed-1].red > 200)
                count++;

            if (count > 0)
            {
                Serial.print("*");
                printLed(iStrip, iLed);
                leds[iStrip][iLed] = CRGB(255, 0, 0);
            }
        }
        Serial.println();
    }
    //for(int i = 0;i<numStrips;i++)
    //    fadeToBlackBy(leds[i], longestTube, 30);
}

void printLed(int s, byte l)
{
    Serial.print("[");
    Serial.print(s);
    Serial.print(".");
    Serial.print(l);
    Serial.print(".");
    Serial.print(ledBuffer[s][l]);
    Serial.print("]");
}

void rotatingRainbow()
{
    static byte hue = 0;
    for(int i = 0;i < numTubes;i++)
    {
        fill_rainbow(leds[i], longestTube, hue++, 10);
    }
}

void spiralRainbow()
{
    static CHSV hsv = CHSV(0,255,255);
    static unsigned long lastTime = millis();
    static byte iStrip = 0;

    if (millis() < lastTime + 1)
    {
        return;
    }

    iStrip++;
    if (iStrip == numTubes)
        iStrip = 0;

    //hsv.hue += (mux.getCutOff() / 16) + 1;

    for(int i = 0; i < numTubes;i++)
    {
        fadeToBlackBy(leds[i], longestTube, 30);
    }

    for( int iLed = 0; iLed < longestTube; iLed++) 
    {
        leds[iStrip][iLed] = hsv;
    }



    lastTime = millis();
}

void juggle() 
{
  // eight colored dots, weaving in and out of sync with each other
    for(byte iStrip =0;iStrip < numTubes;iStrip++)
    {
        fadeToBlackBy( leds[iStrip], longestTube, 20);
    }
    
    byte dothue = 0;
    for( int i = 0; i < numTubes; i++) {
        leds[i][beatsin16(i+7,0,longestTube/2)] |= CHSV(dothue, 200, 255);
    dothue += 32;
    }

}

void sinelon()
{
    static byte gHue;
  // a colored dot sweeping back and forth, with fading trails
    for(byte i = 0;i<numTubes;i++)
    {

        fadeToBlackBy( leds[i], longestTube, 20);
        int ypos = beatsin16(13,0,longestTube);
        leds[i][ypos] += CHSV( gHue, 255, 192);
    }
}

void rainbow_fade() { //-FADE ALL LEDS THROUGH HSV RAINBOW
    ihue++;
    CRGB thisColor;
    HSVtoRGB(ihue, 255, 255, thisColor);
    for(int idex = 0 ; idex < longestTube; idex++ ) {
        setPixel(idex,thisColor);
    }
}


void rainbow_loop(int istep, int idelay) { //-LOOP HSV RAINBOW
    static int idex = 0;
    static int offSet = 0;
    idex++;
    ihue = ihue + istep;
    
    if (idex >= longestTube) {
        idex = ++offSet;
    }
    
    CRGB icolor;
    HSVtoRGB(ihue, 255, 255, icolor);
    setPixel(idex, icolor);
}


void random_burst(int idelay) { //-RANDOM INDEX/COLOR
    CRGB icolor;
    
    idex = random(0,longestTube);
    ihue = random(0,255);
    
    HSVtoRGB(ihue, 255, 255, icolor);
    setPixel(idex, icolor);
}


void color_bounce(int idelay) { //-BOUNCE COLOR (SINGLE LED)
    if (bounceForward)
    {
        idex = idex + 1;
        if (idex == longestTube)
        {
            bounceForward = false;
            idex = idex - 1;
        }
    }
    else
    {
        idex = idex - 1;
        if (idex == 0)
        {
            bounceForward = true;
        }
    }

    for(int i = 0; i < longestTube; i++ ) {
        if (i == idex) {
            setPixel(i, CRGB::Red);
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}


void police_lightsONE() { //-POLICE LIGHTS (TWO COLOR SINGLE LED)
    idex++;
    if (idex >= longestTube) {
        idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    for(int i = 0; i < longestTube; i++ ) {
        if (i == idexR) {
            setPixel(i, CRGB::Red);
        }
        else if (i == idexB) {
            setPixel(i, CRGB::Blue);
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}

void police_lightsALL()
{
    static byte colorIndex;
    SetupAmericaPalette();
    currentBlending = LINEARBLEND;

    // the first variable is the speed at which the lights march, the second is the width of the bars
    FillLEDsFromPaletteColors(colorIndex+=12, 3);
}

void police_lightsALLOld() { //-POLICE LIGHTS (TWO COLOR SOLID)
    idex++;
    if (idex >= longestTube) {idex = 0;}
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    setPixel(idexR, 255, 0, 0);
    setPixel(idexB, 0, 0, 255);
}

void fourthOfJuly() { //-red, white and blue
    idex++;
    if (idex >= longestTube) {idex = 0;}
    int idexR = idex;
    int idexW = nextThird(idexR);
    int idexB = nextThird(idexW);
    setPixel(idexR, CRGB::Red);
    setPixel(idexW, CRGB::White);
    setPixel(idexB, CRGB::Blue);
}

void yxyy() { //-red, white and blue
    idex++;
    if (idex >= longestTube) 
    {
      idex = 0;
    }
    int idexR = idex;
    int idexW = nextThird(idexR);
    int idexB = nextThird(idexW);
    setPixel(idexR, CRGB::DarkBlue);
    setPixel(idexW, CRGB::Orange);
    setPixel(idexB, CRGB::Fuchsia);
}


/*
void musicReactiveFade(byte eq[7]) { //-BOUNCE COLOR (SIMPLE MULTI-LED FADE)
    static long lastBounceTime;
    const int bounceInterval = 500;

    int bass = (eq[0] + eq[1] + eq[3]) / 3;
    int high = (eq[4] + eq[5] + eq[6]) / 3;
    Serial.print("init bass ");Serial.print(bass);Serial.print(" and high ");Serial.println(high);
    
    
    if((bass > 150) && (millis() > (lastBounceTime + bounceInterval)))
    {
        Serial.println("reversing");
        bounceForward = !bounceForward;
        lastBounceTime = millis();
    }
    
    byte trailLength = map(bass, 0,255, 3,19);
    //Serial.print("trailLength: ");Serial.println(trailLength);
    byte trailDecay = (255-64)/trailLength;
    //Serial.print("trailDecay: ");Serial.println(trailDecay);
    byte hue = high;

    fillSolid(CRGB::Black);
    
    if (bounceForward) {
        Serial.print("bouncing forward idex:");Serial.print(idex);Serial.println();

        if (idex < longestTube) {
            idex++;
        } else if (idex == longestTube) {
            bounceForward = !bounceForward;
        }

        for(int i = 0;i<trailLength;i++)
        {
            //Serial.print(" to ");Serial.println(adjacent_cw(idex+i));
            setPixel(adjacent_cw(idex-i), HSVtoRGB(hue, 255, 255 - trailDecay*i));
        }
    } else {
        if (idex > longestTube) {
            idex = longestTube;
        }
        if (idex >= 0) {
            idex--;
        } else if (idex == 0) {
            bounceForward = !bounceForward;
        } else
        {
            idex = 0;
        }

        Serial.print("bouncing backwards idex:");Serial.print(idex);Serial.println();
        for(int i = 0;i<trailLength;i++)
        {
            //Serial.print(" to ");Serial.println(adjacent_ccw(idex+i));
            setPixel(adjacent_ccw(idex+i), HSVtoRGB(hue, 255, 255 - trailDecay*i));
        }
    }    
}
*/


void color_bounceFADE(int idelay) { //-BOUNCE COLOR (SIMPLE MULTI-LED FADE)
    if (bouncedirection == 0) {
        idex++;
        if (idex == longestTube) {
            bouncedirection = 1;
            idex--;
        }
    }
    if (bouncedirection == 1) {
        idex--;
        if (idex == 0) {
            bouncedirection = 0;
        }
    }
    int iL1 = adjacent_cw(idex);
    int iL2 = adjacent_cw(iL1);
    int iL3 = adjacent_cw(iL2);
    int iR1 = adjacent_ccw(idex);
    int iR2 = adjacent_ccw(iR1);
    int iR3 = adjacent_ccw(iR2);
    
    for(int i = 0; i < longestTube; i++ ) {
        if (i == idex) {
            setPixel(i, CRGB(255, 0, 0));
        }
        else if (i == iL1) {
            setPixel(i, CRGB(100, 0, 0));
        }
        else if (i == iL2) {
            setPixel(i, CRGB(50, 0, 0));
        }
        else if (i == iL3) {
            setPixel(i, CRGB(10, 0, 0));
        }
        else if (i == iR1) {
            setPixel(i, CRGB(100, 0, 0));
        }
        else if (i == iR2) {
            setPixel(i, CRGB(50, 0, 0));
        }
        else if (i == iR3) {
            setPixel(i, CRGB(10, 0, 0));
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}

void flicker(int thishue, int thissat) {
    int random_bright = random(0,255);
    int random_bool = random(0,random_bright);
    CRGB thisColor;
    
    if (random_bool < 10) {
        HSVtoRGB(thishue, thissat, random_bright, thisColor);
        
        for(int i = 0 ; i < longestTube; i++ ) {
            setPixel(i, thisColor);
        }
    }
}


void pulse_one_color_all(int ahue, int idelay) { //-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR
    
    if (bouncedirection == 0) {
        ibright++;
        if (ibright >= 255) {bouncedirection = 1;}
    }
    if (bouncedirection == 1) {
        ibright = ibright - 1;
        if (ibright <= 1) {bouncedirection = 0;}
    }
    
    CRGB acolor;
    HSVtoRGB(ahue, 255, ibright, acolor);
    
    for(int i = 0 ; i < longestTube; i++ ) {
        setPixel(i, acolor);
    }
}


void pulse_one_color_all_rev(int ahue, int idelay) { //-PULSE SATURATION ON ALL LEDS TO ONE COLOR
    
    if (bouncedirection == 0) {
        isat++;
        if (isat >= 255) {bouncedirection = 1;}
    }
    if (bouncedirection == 1) {
        isat = isat - 1;
        if (isat <= 1) {bouncedirection = 0;}
    }
    
    CRGB acolor;
    HSVtoRGB(ahue, isat, 255, acolor);
    
    for(int i = 0 ; i < longestTube; i++ ) {
        setPixel(i, acolor);
    }
}

void random_march(int idelay) { //RANDOM MARCH CCW
    copy_led_array();
    int iCCW;
    
    CRGB acolor;
    HSVtoRGB(random(0,360), 255, 255, acolor);
    setPixel(0, acolor);
    
    for(int i = 1; i < longestTube;i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        setPixel(i, ledsX[iCCW]);
    }
}


void rwb_march(int idelay) { //R,W,B MARCH CCW
    copy_led_array();
    int iCCW;
    
    idex++;
    if (idex > 2) {idex = 0;}
    
    switch (idex) {
        case 0:
            setPixel(0, CRGB::Red);
            break;
        case 1:
            setPixel(0, CRGB::White);
            break;
        case 2:
            setPixel(0, CRGB::Blue);
            break;
    }
    
    for(int i = 1; i < longestTube; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        setPixel(i, ledsX[iCCW][0], ledsX[iCCW][1], ledsX[iCCW][2]);
    }
}


void white_temps() {
    int N9 = int(longestTube/9);
    for (int i = 0; i < longestTube; i++ ) {
        if (i >= 0 && i < N9)
            {setPixel(i, 255,147,41);} //-CANDLE - 1900
        if (i >= N9 && i < N9*2)
                {setPixel(i, 255,197,143);} //-40W TUNG - 2600
        if (i >= N9*2 && i < N9*3)
                {setPixel(i, 255,214,170);} //-100W TUNG - 2850
        if (i >= N9*3 && i < N9*4)
                {setPixel(i, 255,241,224);} //-HALOGEN - 3200
        if (i >= N9*4 && i < N9*5)
                {setPixel(i, 255,250,244);} //-CARBON ARC - 5200
        if (i >= N9*5 && i < N9*6)
                {setPixel(i, 255,255,251);} //-HIGH NOON SUN - 5400
        if (i >= N9*6 && i < N9*7)
                {setPixel(i, 255,255,255);} //-DIRECT SUN - 6000
        if (i >= N9*7 && i < N9*8)
                {setPixel(i, 201,226,255);} //-OVERCAST SKY - 7000
        if (i >= N9*8 && i < longestTube)
                {setPixel(i, 64,156,255);} //-CLEAR BLUE SKY - 20000
    }
}


void color_loop_vardelay() { //-COLOR LOOP (SINGLE LED) w/ VARIABLE DELAY
    idex++;
    if (idex > longestTube) {idex = 0;}
    
    CRGB acolor;
    HSVtoRGB(0, 255, 255, acolor);
    
    //int di = abs(TOP_INDEX - idex); //-DISTANCE TO CENTER
    //int t = constrain((10/di)*10, 10, 500); //-DELAY INCREASE AS INDEX APPROACHES CENTER (WITHIN LIMITS)
    
    for(int i = 0; i < longestTube; i++ ) {
        if (i == idex) {
            setPixel(i, acolor);
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}


void strip_march_cw(int idelay) { //-MARCH STRIP C-W
    copy_led_array();
    int iCCW;
    for(int i = 0; i < longestTube; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        setPixel(i, ledsX[iCCW]);
    }
}


void strip_march_ccw(int idelay) { //-MARCH STRIP C-W
    copy_led_array();
    int iCW;
    for(int i = 0; i < longestTube; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCW = adjacent_cw(i);
        setPixel(i, ledsX[iCW]);
    }
}


void pop_horizontal(int ahue, int idelay) {  //-POP FROM LEFT TO RIGHT UP THE RING
    CRGB acolor;
    HSVtoRGB(ahue, 255, 255, acolor);
    
    int ix =0;
    
    if (bouncedirection == 0) {
        bouncedirection = 1;
        ix = idex;
    }
    else if (bouncedirection == 1) {
        bouncedirection = 0;
        ix = horizontal_index(idex);
        idex++;
        if (idex > TOP_INDEX) {
            idex = 0;
        }
    }
    
    for(int i = 0; i < longestTube; i++ ) {
        if (i == ix) {
            setPixel(i, acolor);
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}


void quad_bright_curve(int ahue, int idelay) {  //-QUADRATIC BRIGHTNESS CURVER
    CRGB acolor;
    int ax;
    
    for(int x = 0; x < longestTube; x++ ) {
        if (x <= TOP_INDEX) {ax = x;}
        else if (x > TOP_INDEX) {ax = longestTube-x;}
        
        int a = 1; int b = 1; int c = 0;
        
        int iquad = -(ax*ax*a)+(ax*b)+c; //-ax2+bx+c
        int hquad = -(TOP_INDEX*TOP_INDEX*a)+(TOP_INDEX*b)+c; //HIGHEST BRIGHTNESS
        
        ibright = int((float(iquad)/float(hquad))*255);
        
        HSVtoRGB(ahue, 255, ibright, acolor);
        setPixel(x,acolor);
    }
}


void flame() {
    CRGB acolor;
//    int idelay = random(0,35);
    
    float hmin = 0.1; float hmax = 45.0;
    float hdif = hmax-hmin;
    int randtemp = random(0,3);
    float hinc = (hdif/float(TOP_INDEX))+randtemp;
    
    int ahue = hmin;
    for(int i = 0; i < TOP_INDEX; i++ ) {
        
        ahue = ahue + hinc;
        
        HSVtoRGB(ahue, 255, 255, acolor);
        setPixel(i,acolor);
        int ih = horizontal_index(i);
        setPixel(ih,acolor);
        setPixel(TOP_INDEX,CRGB::White);
    }
}


void radiation(int ahue, int idelay) { //-SORT OF RADIATION SYMBOLISH-
    //int N2 = int(longestTube/2);
    int N3 = int(longestTube/3);
    int N6 = int(longestTube/6);
    int N12 = int(longestTube/12);
    CRGB acolor;
    
    for(int i = 0; i < N6; i++ ) { //-HACKY, I KNOW...
        tcount = tcount + .02;
        if (tcount > 3.14) {tcount = 0.0;}
        ibright = int(sin(tcount)*255);
        
        int j0 = (i + longestTube - N12) % longestTube;
        int j1 = (j0+N3) % longestTube;
        int j2 = (j1+N3) % longestTube;
        HSVtoRGB(ahue, 255, ibright, acolor);
        setPixel(j0,acolor);
        setPixel(j1,acolor);
        setPixel(j2,acolor);
        
    }

}


void sin_bright_wave(int ahue, int idelay) {
    CRGB acolor;
    
    for(int i = 0; i < longestTube; i++ ) {
        tcount = tcount + .1;
        if (tcount > 3.14) {tcount = 0.0;}
        ibright = int(sin(tcount)*255);
        
        HSVtoRGB(ahue, 255, ibright, acolor);
        setPixel(i, acolor);
    }
}


void fade_vertical(int ahue, int idelay) { //-FADE 'UP' THE LOOP
    CRGB acolor;
    idex++;
    if (idex > TOP_INDEX) {idex = 0;}
    int idexA = idex;
    int idexB = horizontal_index(idexA);
    
    ibright = ibright + 10;
    
    if (ibright > 255) {
        ibright = 0;
    }
    HSVtoRGB(ahue, 255, ibright, acolor);
    
    setPixel(idexA, acolor);
    setPixel(idexB, acolor);
}


void rainbow_vertical(int istep, int idelay) { //-RAINBOW 'UP' THE LOOP
    idex++;
    if (idex > TOP_INDEX) {idex = 0;}
    ihue = ihue + istep;
    if (ihue > 255)
    {
        ihue = 0;
    }
    int idexA = idex;
    int idexB = horizontal_index(idexA);
    
    CRGB acolor;
    HSVtoRGB(ihue, 255, 255, acolor);
    
    setPixel(idexA, acolor);
    setPixel(idexB, acolor);

}

boolean checkButton(byte whichButton)
{
    static unsigned long lastTime = millis();
    if (millis() > lastTime + 500)
    {
        if (0 == whichButton)
        {
          return true;
//            return mux.isIntensifierOn();
        }
    }
    
    return false;
}

//test show all animations
void loop()
{

  //debugColors();
  

  //police_lightsALL();
  //currentPalette = HeatColors_p;
  //Fire2012WithPalette();
  //spiralRainbow();
  //rainbowWithGlitter();
  //rotatingRainbow(); 
  //sinelon();
  //juggle();
  debugColors();
  // generate noise data
  //fillnoise8();
  
  // convert the noise data to colors in the LED array
  // using the current palette
  //mapNoiseToLEDsUsingPalette();

    showLeds();  

  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rotatingRainbow();
  addGlitter(80);
}

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).

void Fire2012WithPalette()
{
    fillSolid(CRGB::Black);
    int height = longestTube / 2;

    // COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 55, suggested range 20-100 

    //TEMP: disable whien not in dashboard
    byte COOLING = 75;
    if (COOLING < 10)
        COOLING = 75;


    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    const byte SPARKING = 200;

    // Array of temperature readings at each simulation cell
    static byte heat[longestTube];


  // Step 1.  Cool down every cell a little
    for (int i = 0; i < (height); i++) 
    {
      heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / (height) + 2)));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for(int k = (height) - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < (height); j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);

      for(byte iStrip = 0;iStrip<numTubes;iStrip++)
          leds[iStrip][j] = ColorFromPalette( currentPalette, colorindex);
    }
    
    mirror();
}

// mirrors the array of leds (nice for the fire since I want it to be symmetrical
void mirror()
{
  for(byte col = 0;col < numTubes;col++)
  {
    for(int row = 0;row < longestTube/2;row++)
      leds[col][longestTube - row -1] = leds[col][row];  
  }
}

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed".
  uint8_t dataSmoothing = 0;
  if( speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }
  
  for(int i = 0; i < numTubes; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < longestTube; j++) {
      int joffset = scale * j;
      
      uint8_t data = inoise8(noiseX + ioffset,noiseY + joffset,noiseZ);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));

      if( dataSmoothing ) {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }
      
      noise[i][j] = data;
    }
  }
  
  noiseZ += speed;
  
  // apply slow drift to X and Y, just for visual variation.
  noiseX += speed / 8;
  noiseY -= speed / 16;
}

void mapNoiseToLEDsUsingPalette()
{
  static uint8_t ihue=0;
  
  for(int i = 0; i < numTubes; i++) {
    for(int j = 0; j < longestTube; j++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[i][j];
      uint8_t bri =   noise[numTubes - i - 1][longestTube - j - 1];

      // if this palette is a 'loop', add a slowly-changing base value
      if( colorLoop) { 
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the 
      // light/dark dynamic range desired
      if( bri > 127 ) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 2);
      }

      CRGB color = ColorFromPalette( currentPalette, index, bri);

      leds[i][j] = color;
    }
  }
  
  ihue+=1;
}
void ChangePaletteAndSettingsPeriodically()
{
    static byte iPalette = 0;
    iPalette++;
    if (iPalette > 8)
        iPalette = 0;

    switch(iPalette)
    {
        case 0: 
            currentPalette = RainbowColors_p;         
            speed = 20; 
            scale = 30; 
            colorLoop = 1;
            break;
        case 1:
            SetupPurpleAndGreenPalette();             
            speed = 10; 
            scale = 50; 
            colorLoop = 1;
            break;
        case 2:
            SetupBlackAndWhiteStripedPalette();       
            speed = 20; 
            scale = 30; 
            colorLoop = 1;
            break;
        case 3: 
            currentPalette = ForestColors_p;         
            speed = 8; 
            scale = 120; 
            colorLoop = 0;
            break;
        case 4: 
            currentPalette = CloudColors_p;         
            speed = 4; 
            scale = 30; 
            colorLoop = 0;
            break;
        case 5: 
            currentPalette = LavaColors_p;         
            speed = 8; 
            scale = 50; 
            colorLoop = 0;
            break;
        case 6: 
            currentPalette = OceanColors_p;         
            speed = 20; 
            scale = 90; 
            colorLoop = 0;
            break;
        case 7: 
            currentPalette = PartyColors_p;         
            speed = 20; 
            scale = 30; 
            colorLoop = 1;
            break;
        case 8: 
            currentPalette = RainbowStripeColors_p;         
            speed = 20; 
            scale = 20; 
            colorLoop = 1;
            break;

    }
}

CRGB getFlameColor()
{
    //tail lights are GRB, other lights arte RGB
    byte offset = 48;

  byte seed = random(0+offset,50+offset);
  if (seed <30+offset)
    ;

  else if (seed < 45+offset)
    seed = seed - 30+offset;
  else 
    seed = 160 + seed + offset;

    return CHSV(seed, 255, 255);
}

CRGB getFlameColorFromPalette()
{
    const CRGBPalette16 reverseFlames =
    {
        0x00FF00,
        0x00FF00,
        0x00FF00,
        0x00FF00,

        0x88FF00,
        0x88FF00,
        0x88FF00,
        0x88FF00,

        0xAAFF00,
        0xAAFF00,
        0xAAFF00,
        0xAAFF00,

        0x008800,
        0x008800,
        0x008800,
        0x008800
    };

    const CRGBPalette16 coldFlames =
    {
        0x00FF,
        0x00FF,
        0x00FF,
        0x00FF,

        0x8800FF,
        0x8800FF,
        0x8800FF,
        0x8800FF,

        0xAA00FF,
        0xAA00FF,
        0xAA00FF,
        0xAA00FF,

        0x000088,
        0x000088,
        0x000088,
        0x000088
    };


   return ColorFromPalette(reverseFlames, random8(), 128, LINEARBLEND);

}




