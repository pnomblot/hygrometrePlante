#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Button.h>


//************************** RTC *************************************
#include <RTClib.h>
RTC_DS1307 RTC;

//************************** OLED ************************************
#if ( SSD1306_LCDHEIGHT != 64 )
#error( "Height incorrect, please fix Adafruit_SSD1306.h!" );
#endif

#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_RESET);

#define ARROSAGE 5



//************************** BUTTON **********************************
#define BUTTON1_PIN     4
Button button1 = Button(BUTTON1_PIN,BUTTON_PULLUP_INTERNAL);
int screen = 0; 
int button = 0;

//************************** OTHER **********************************


#define MOISTURESENSOR  A0
#define MOISTUREMAX  580
#define MOISTUREOFFSET  270
#define MOISTURE_MINI 30
#define MOISTURE_COEF 560

// unités
#define HPASCAL 0
#define METRES  1
#define DEGRES  2
#define VOLTS   3



extern uint8_t Font24x40[];
extern uint8_t Symbol[];
extern uint8_t Splash[];
extern uint8_t Arroser[];
extern uint8_t Plante[];
extern uint8_t Battery[];

void showTime(struct DateTime t);
unsigned long loopTick; 
int value, etat = 0;
double moisture = 100;
double lastValue = 123.0;
long lastVcc, vcc = 0;
int clignote;

/* ------------------------------------ setup ------------------------------------------ */
void setup()   {                
  Serial.begin(115200); //Démarrage de la communication
  pinMode(ARROSAGE, INPUT_PULLUP);
  digitalWrite(ARROSAGE, LOW);

  // initialise RTC
  RTC.begin(); 
  RTC.adjust(DateTime(__DATE__, __TIME__)); 
  
  button1.releaseHandler(handleButtonReleaseEvents);
  button1.holdHandler(handleButtonHoldEvents,1500);


  // initialise OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay(); 
  screen = -1;
  loopTick =  millis();
}

/* ------------------------------------ loop ------------------------------------------ */
void loop() {
  char status;
  long vcc;
  DateTime now;

  while ( ( millis() - loopTick ) < 150 ) {
    button1.process();
    if(button1.stateChanged()) break;
    delay(10);
  }
  
  loopTick =  millis();
  ++clignote;
  
  switch (screen) {
    case -1: // Startup
      display.clearDisplay(); 
      display.drawBitmap(0, 0, Splash, 128, 64, WHITE);
      display.display();
      delay(1000);
      ++screen;
      button = 0;
    break;

    
    
    case 0: // moisture
      now = RTC.now();
      moisture = (100*double(MOISTUREMAX-MOISTUREOFFSET-(analogRead(MOISTURESENSOR) - MOISTUREOFFSET)))/(MOISTUREMAX-MOISTUREOFFSET);
      //Serial.println(analogRead(MOISTURESENSOR));
      display.clearDisplay(); 
      showBatterylevel(readVcc());
      showTime(now);
     
      if ( moisture < MOISTURE_MINI) {
        digitalWrite(ARROSAGE, HIGH);
        if ( clignote%3!=0)  display.drawBitmap(0, 10,  Arroser, 64, 64, 1);
      } else {
        digitalWrite(ARROSAGE, LOW);
      } 
            
      drawPercentValue(30,20,int(moisture));
      display.display();
    break;


    case 1:  // Battery
     digitalWrite(ARROSAGE, LOW);

      vcc = readVcc();
      if (lastVcc != vcc) {
        lastVcc = vcc;
        display.clearDisplay(); 
        showBatterylevel(readVcc());
        display.setCursor(0,0);
        display.println("Batterie : ");
        drawFloatValue(0, 20, readVcc(), VOLTS);
        display.display();  
      }       
      break;

    #define SETTING_HOUR 2
    #define SETTING_MINUTE 3
    #define SETTING_SECOND 4
    case 2:  
      digitalWrite(ARROSAGE, LOW);
      if (button>1) {
          now = RTC.now();
          DateTime future (now + TimeSpan(0,1,0,0));
          RTC.adjust(future); 
          button=0;
      }

    case 3:  
      digitalWrite(ARROSAGE, LOW);
      if (button>1) {
          now = RTC.now();
          DateTime future (now + TimeSpan(0,0,1,0));
          RTC.adjust(future); 
          button=0;
      }
      
    case 4:  
      digitalWrite(ARROSAGE, LOW);
      now = RTC.now();
      display.clearDisplay(); 
      showTime(now);
      display.display();
      break;
  
    case 5:  // Test
        digitalWrite(ARROSAGE, HIGH);
        display.clearDisplay(); 
        display.drawBitmap(0, 0,  Arroser, 64, 64, 1);
        display.display();
      break;
  

    default:
       screen = 0;
     break;
  } // switch screen end

}
/* -------------------- fonctions --------------------  */


// Gestion du bouton relaché
void handleButtonReleaseEvents(Button &btn) {
  Serial.println("BUTTON");
  ++button;
}

// Gestion de l'appui prolongé sur le bouton
void handleButtonHoldEvents(Button &btn) {
  Serial.println("BUTTON HOLD");
  lastValue = 0;
  ++screen;
}

// Affiche un caractére en x, y
void drawCar(int sx, int sy, int num, uint8_t *font, int fw, int fh, int color) {
  byte row;
  for(int y=0; y<fh; y++) {
    for(int x=0; x<(fw/8); x++) {
      row = pgm_read_byte_near(font+x+y*(fw/8)+(fw/8)*fh*num);
      for(int i=0;i<8;i++) {
        if (bitRead(row, 7-i) == 1) display.drawPixel(sx+(x*8)+i, sy+y, color);
      }
    }
  }
}

// Affiche un gros caractére en x, y
void drawBigCar(int sx, int sy, int num) {
  drawCar(sx, sy, num, Font24x40, 24, 40, WHITE) ;
}

void drawDot(int sx, int sy, int h) {
  display.fillRect(sx, sy-h, h, h, WHITE);
}

// Affiche un symbole en x, y
void drawSymbol(int sx, int sy, int num) {
  drawCar(sx, sy, num, Symbol, 16, 16, WHITE) ;
}


void drawPercentValue(int sx, int sy, int val) {
    char charBuf[5];
    if (val > 100) val = 100;
    itoa(val, charBuf, 10); 
    
    int nbCar = strlen(charBuf);
    drawBigCar(sx+52, sy, charBuf[nbCar-1]- '0');
    if (--nbCar > 0) drawBigCar(sx+26, sy, charBuf[nbCar-1]- '0');
    if (--nbCar > 0) drawBigCar(sx, sy, charBuf[nbCar-1]- '0');
    drawSymbol(108,sy, 6);
}


// Affiche un nombre decimal
void drawFloatValue(int sx, int sy, double val, int unit) {
  char charBuf[15];
  if (val < 10000) {
    dtostrf(val, 3, 1, charBuf); 
    int nbCar = strlen(charBuf);
    if (nbCar > 5) { // pas de decimal
      for (int n=0; n<4; n++) drawBigCar(sx+n*26, sy, charBuf[n]- '0');
      drawSymbol(108,sy, unit);
    }
    else {
      drawBigCar(sx+86, sy, charBuf[nbCar-1]- '0');
      drawDot(78, sy+39, 6);
      nbCar--;
      if (--nbCar > 0) drawBigCar(sx+52, sy, charBuf[nbCar-1]- '0');
      if (--nbCar > 0) drawBigCar(sx+26, sy, charBuf[nbCar-1]- '0');
      if (--nbCar > 0) drawBigCar(sx, sy, charBuf[nbCar-1]- '0');
      drawSymbol(112,sy, unit);
    }
  }
}


// Show Time
void print2digitsNumber(int t, boolean revert) {
  char buffer[3];
  if (revert) display.setTextColor(BLACK, WHITE); // 'inverted' text
  sprintf(buffer, "%02d", t);
  display.print(buffer);
  display.setTextColor(WHITE, BLACK); 
}

void showTime(struct DateTime t) {
  display.setCursor(0,0);

  
  print2digitsNumber( t.hour(), ((clignote%5) && (screen == SETTING_HOUR)));
  display.print(":");
  print2digitsNumber( t.minute(), ((clignote%5) && (screen == SETTING_MINUTE)));
  display.print(":");
  print2digitsNumber( t.second(), ((clignote%5) && (screen == SETTING_SECOND)));
}

// Show battery level icon
void showBatterylevel(long vcc) {
  if (vcc > 3600) display.drawBitmap(104, 1,  Battery, 20, 7, 1); 
  else if (vcc > 3400) display.drawBitmap(104, 1,  Battery+21, 20, 7, 1); 
  else if (vcc > 3200) display.drawBitmap(104, 1,  Battery+42, 20, 7, 1); 
  else if (vcc > 3000) display.drawBitmap(104, 1,  Battery+63, 20, 7, 1); 
  else display.drawBitmap(104, 1,  Battery+84, 20, 7, 1); 
}

// Read VCC
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}




































