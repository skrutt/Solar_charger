

#include <Wire.h>
#include <SSD1306_text.h>

SSD1306_text display;


#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


#define SCREEN_UPDATE_MS 250


#define CHARGE_START  3.95
#define CHARGE_STOP   4.05

#define CHARGE_PIN    9


//Load defines, give loads a struct/class later
//#define LOAD_ALLOW  3.75
//#define LOAD_STOP   3.5
#define LOAD_ALLOW  3.8
#define LOAD_STOP   3.65

#define LOAD1_PIN    10


void charge_off()
{
    digitalWrite(CHARGE_PIN, LOW);    //Set pin low and input, disabled pullup
    pinMode(CHARGE_PIN, INPUT);
}

void charge_on()
{
    pinMode(CHARGE_PIN, OUTPUT);      //Set pin output and high
    digitalWrite(CHARGE_PIN, LOW);
}

void setup()   {                
  Serial.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  //display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.init();  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.

  // Clear the buffer.
  display.clear();


  
 // display.setTextColor(BLACK, WHITE); // 'inverted' text
 // display.println(3.141592);

  display.setCursor(0,0);
  display.setTextSize(2);
  display.print("0x"); display.println(0xDEADBEEF, HEX);
//  display.display();
  delay(200);
  display.clear();

  //Setup analog pin
//??
  analogReference(INTERNAL);  //Set to 1.1 internal Vref

    //Load pin
    pinMode(LOAD1_PIN, OUTPUT);

    //Charge pin
    charge_off(); 

}


void loop() {

   // text display tests
  display.setTextSize(1);
  display.setCursor(0,0);

  int test = analogRead(0);

  static double volt = 0.7;

  //internal ref measured to be 1.091
  volt = (test * 1.091 / 1023) * 0.01 + volt * 0.99;

  const double res_divisor = 2.16 / (2.16 + 11.78);

  double batt_volt = volt/ res_divisor;
  
  //resistor1 low side 2.16 kohm
  //resistor2 high side 11.78 kohm

  static bool charge_enabled = 0;
  static bool load_enabled = 0;


  //CHARGE
  if(batt_volt > CHARGE_STOP)
  {
    charge_off();
    charge_enabled = 0;
  }
  else if(batt_volt < CHARGE_START)
  {
    charge_on();
    charge_enabled = 1;
  }
  
  //LOAD
  if(batt_volt < LOAD_STOP)
  {
    digitalWrite(LOAD1_PIN, LOW);
    load_enabled = 0;
  }
  else if(batt_volt > LOAD_ALLOW)
  {
    digitalWrite(LOAD1_PIN, HIGH);
    load_enabled = 1;
  }
  

  //Debug counter
  static uint16_t i = 0 ;
  i++;

  static unsigned long last_time = 0;
  unsigned long current_time = millis();


  //Time to update screen
  if((current_time - last_time) > SCREEN_UPDATE_MS)
  {
    last_time = current_time;
  
    display.println("Solar test v0.1");
    display.println();
    
    if(charge_enabled)
      display.print("Charge: ON ");
    else
      display.print("Charge: OFF ");
    
    if(load_enabled)
      display.print("Load: ON "); 
    else
      display.print("Load: OFF ");
  
    display.println();
    display.println();
  
    //Debug
    //display.print("Raw ADC: ");
    //display.println(test);
    
    display.print("Treated val: ");  display.print(volt, 3); display.println(" V");
  
    display.print("Battery: "); display.print(batt_volt, 3); display.println(" V"); 
  
  
    display.setCursor(7, 0);
    display.print("Debug counter: ");
    display.println(i);
  }

  
//  display.display();
//  delay(20);
}





