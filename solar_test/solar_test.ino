
#include "prescaler.h"

#include <avr/sleep.h>
#include <avr/power.h>

ISR(ADC_vect){ // else application is reset
}



#include <Wire.h>
#include <SSD1306_text.h>

SSD1306_text display;


#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


#define SCREEN_UPDATE_MS 350

#define BATTERY_VOLTAGE_CHANNEL 0


#define CHARGE_START  3.95
#define CHARGE_STOP   4.05

#define CHARGE_PIN    9


//Load defines, give loads a struct/class later
//#define LOAD_ALLOW  3.75
//#define LOAD_STOP   3.5
#define LOAD_ALLOW  3.8
#define LOAD_STOP   3.65

#define LOAD1_PIN    10

#define LOAD1_ENABLE_PIN    6
#define SCREEN_POWER_PIN    7
#define SCREEN_ENABLE_PIN    8

#define BUTTON_DEBOUNCE_MS 200

class button_type_c
{
  public:
  button_type_c(uint8_t button_no):
  button_no(button_no)
  {
    last_time = 0;
    last_state = 0;
    pinMode(button_no, INPUT_PULLUP);
  }

  bool readButton()
  {
    uint32_t current_time = trueMillis();
    bool ret = 0;
    
    if((current_time - last_time) > BUTTON_DEBOUNCE_MS)
    {
      bool current_state = !digitalRead(button_no);   //active low
      
      if(current_state && !last_state)  //if active and last state was inactive 
      {
        last_time = current_time;
        ret = 1;
      }                                 //Here we could add button repeating
      last_state = current_state; //Save last state
    }
    return ret;
  }
  private:
  bool last_state;
  uint8_t button_no;
  uint32_t last_time;   //todo, init func to set up pin and struct, read func
  
} ;


button_type_c load1_enable_button(LOAD1_ENABLE_PIN);
button_type_c screen_enable_button(SCREEN_ENABLE_PIN);



int rawAnalogReadWithSleep( void )
{
 // Generate an interrupt when the conversion is finished
 ADCSRA |= _BV( ADIE );

 // Enable Noise Reduction Sleep Mode
 set_sleep_mode( SLEEP_MODE_ADC );
 sleep_enable();

 // Any interrupt will wake the processor including the millis interrupt so we have to...
 // Loop until the conversion is finished
 do
 {
   // The following line of code is only important on the second pass.  For the first pass it has no effect.
   // Ensure interrupts are enabled before sleeping
   sei();
   // Sleep (MUST be called immediately after sei)
   sleep_cpu();
   // Checking the conversion status has to be done with interrupts disabled to avoid a race condition
   // Disable interrupts so the while below is performed without interruption
   cli();
 }
 // Conversion finished?  If not, loop.
 while( ( (ADCSRA & (1<<ADSC)) != 0 ) );

 // No more sleeping
 sleep_disable();
 // Enable interrupts
 sei();

 // The Arduino core does not expect an interrupt when a conversion completes so turn interrupts off
 ADCSRA &= ~ _BV( ADIE );

 // Return the conversion result
 return( ADC );
}


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

void screen_draw()
{ 
  
    ADCSRA &= ~(1 << ADEN);
    power_adc_disable();
    
    pinMode(SCREEN_POWER_PIN, OUTPUT);  
    digitalWrite(SCREEN_POWER_PIN, LOW);
    //35ms seems to be needed to start display
    trueDelay(50);
  
  //Check disabling adc & wire later
    power_twi_enable();
    
    display.init();       

    display.clear();
    display.setCursor(0,0);
    
    display.setTextSize(2);
    display.println("   SOLAR");
    display.println("   AW");
    display.println("   YEA");
  
    trueDelay(1500);

    display.setTextSize(1);

     // text display tests
    display.setCursor(0,0);
    display.println("   Solar test v0.1");
    display.println("                   ");
    display.println("Charge:     Load:    "); 
    display.println("                   ");
  
    //Debug
    //display.print("Raw ADC: ");
    //display.println(test);
    
    display.println("Treated val:       V");
    display.println("Battery:           V"); 
    display.println("                   ");
    display.println("Debug count:");
    
    power_twi_disable();
    
    power_adc_enable();
    ADCSRA |= (1 << ADEN);
}

  static bool screen_enable = 1;

  static bool charge_enabled = 0;
  static bool load_enabled = 0;
  
  const float res_divisor = 2.16 / (2.16 + 11.78);
  static float volt = 0.7;
  static float batt_volt = volt/ res_divisor;

void screen_update()
{
  //Debug counter
  static uint32_t i = 0 ;
  i++;
  
  static unsigned long last_time = 0;
  unsigned long current_time = trueMillis();

  //Time to update screen
  if(((current_time - last_time) > SCREEN_UPDATE_MS) && screen_enable)
  {
    //Check disabling adc & wire later
    ADCSRA &= ~(1 << ADEN);
    power_adc_disable();
    power_twi_enable();
    
    last_time = current_time;
    
    display.setCursor(2, 8 * 6);
    if(charge_enabled)
      display.print("ON ");
    else
      display.print("OFF");
    
    display.setCursor(2, 17 * 6);
    if(load_enabled)
      display.print("ON "); 
    else
      display.print("OFF");
  
  
    //Debug
    //display.print("Raw ADC: ");
    //display.println(test);

    //threated volt
    display.setCursor(4,13 * 6);
    display.print(volt, 3);

    //battery volt
    display.setCursor(5,13 *6);
    display.print(batt_volt, 3);
  
    //debug counter
    display.setCursor(7, 12 * 6);
    display.println(i);
    
    power_twi_disable();
    
    power_adc_enable();
    ADCSRA |= (1 << ADEN);
  }
}

void setup()   
{                
//  Serial.begin(9600);

  setClockPrescaler(CLOCK_PRESCALER_16);    //Set 1 Mhz operation. Look into adc prescaler later


  //Try to disable periphals to reduce power
  TCCR2B &= ~(1 << CS22);
  TCCR2B &= ~(1 << CS21);
  TCCR2B &= ~(1 << CS20);
  power_timer2_disable();
  
  power_timer1_disable(); 
  //power_timer0_disable(); 
  power_spi_disable();
  power_usart0_disable();
 
  
  screen_draw();

  // init done

  //Setup analog pin
  //??
  analogReference(INTERNAL);  //Set to 1.1 internal Vref

  //REFSx 11 means internal 1.1 vref
  ADMUX = (1 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (0x07 & BATTERY_VOLTAGE_CHANNEL); 

  //Load pins
  digitalWrite(LOAD1_PIN, LOW);
  pinMode(LOAD1_PIN, OUTPUT);
    

  //Charge pin
  charge_off(); 

}


void loop() {


 // int test = analogRead(BATTERY_VOLTAGE_CHANNEL);
  int test = rawAnalogReadWithSleep();  //ADC should be set up for channel 0 now


  //internal ref measured to be 1.091
  //0x000 represents analog ground, and 0x3FF represents the selected reference voltage minus one LSB.
  volt = (test * 1.091 / 1024) * 0.01 + volt * 0.99;

  batt_volt = volt/ res_divisor;

  
  //resistor1 low side 2.16 kohm
  //resistor2 high side 11.78 kohm


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
  else if(load1_enable_button.readButton()) //if load toggle pressed
  {
    if(load_enabled)  //Turn off if on
    {
      digitalWrite(LOAD1_PIN, LOW);
      load_enabled = 0;
    }
    else if(batt_volt > LOAD_ALLOW) //turn on if allowed and off
    {
      digitalWrite(LOAD1_PIN, HIGH);
      load_enabled = 1;
    }
  }

  //SCREEN

  if(screen_enable_button.readButton())
  {
    screen_enable = !screen_enable; //toggle
    if(screen_enable)
    {
      screen_draw();
    }else{
      digitalWrite(SCREEN_POWER_PIN, HIGH);
    }
  }

  screen_update();
 
}





