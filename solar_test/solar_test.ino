
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



#define BATTERY_VOLTAGE_CHANNEL 0


#define CHARGE_START  3.95
#define CHARGE_STOP   4.05

#define CHARGE_PIN    3


//Load defines, give loads a struct/class later
//#define LOAD_ALLOW  3.75
//#define LOAD_STOP   3.5
#define LOAD_ALLOW  3.8
#define LOAD_STOP   3.6

#define LOAD1_PIN    9
#define LOAD1_BUTTON_PIN    8   //button
//Define as long because compiler for some reason gets the wrong datatype
#define LOAD_ENABLE_TIME 1000L * 60L * 60L * 2L

#define SCREEN_UPDATE_MS 350
#define SCREEN_TIMEOUT_MS 15000
#define SCREEN_POWER_PIN    2
#define SCREEN_BUTTON_PIN    7    //button

#define BUTTON_DEBOUNCE_MS 200


 // static bool screen_enable = 1;

  static bool charge_enabled = 0;
  
  const float res_divisor = 2.16 / (2.16 + 11.78);
  static float volt = 0.7;
  static float batt_volt = volt/ res_divisor;

//timeout class
class timeout_c
{
  public:
  timeout_c(const uint32_t timeout_val):
  timeout(timeout_val)
  {
    clear();
  }

  bool check()
  {    
    if(!last_time || time_passed() > timeout)  //check if timed out yet, or not started
    {
        return 1;  
    }
    return 0;
  }
  bool check_n_reset()
  {
    bool ret = check();
    
    if(ret)  //if timed out, restart
        reset();  
    return ret;
  }
  void reset()
  {
        last_time = trueMillis();
  }
  void clear()
  {
    last_time = 0;
  }
  uint32_t time_passed()
  {
    uint32_t current_time = trueMillis();
    return current_time - last_time;
  }
  uint32_t timeLeft()
  {
    uint32_t time_passed_int = time_passed();
    if(!last_time || time_passed_int >= timeout)
      return 0;
    return timeout - time_passed_int;
  }
  
  private:
  uint32_t timeout;
  uint32_t last_time; 
  
};

//button class/ lib?
class button_type_c
{
  public:
  button_type_c(uint8_t button_no):
  button_no(button_no),
  timer(BUTTON_DEBOUNCE_MS)
  {
    last_state = 0;
    pinMode(button_no, INPUT_PULLUP);
  }

  bool readButton()
  {
    bool ret = 0;
    
    if(timer.check())
    {
      bool current_state = !digitalRead(button_no);   //active low
      
      if(current_state && !last_state)  //if active and last state was inactive 
      {
        timer.reset();
        ret = 1;
      }                                 //Here we could add button repeating
      last_state = current_state; //Save last state
    }
    return ret;
  }
  
  private:
  bool last_state;
  uint8_t button_no;
  timeout_c timer;  //timer used for debounce 
  //todo, init func to set up pin and struct, read func
  
};

//load class/ lib?
class load_type_c
{
  public:
  //Constructor
  load_type_c(uint8_t output_no, uint8_t button):
  enable_button(button),
  output_no(output_no),
  timer(LOAD_ENABLE_TIME)
  {
      digitalWrite(output_no, LOW);
      pinMode(output_no, OUTPUT); 
  }

  bool isEnabled()
  {
      return !timer.check();
  }
  uint32_t timeLeft()
  {
    return timer.timeLeft();
  }
  bool check()
  {

      if(batt_volt < LOAD_STOP) //First, check for low power
      {
        disable();
      }
      else 
      {
          //if load button pressed
          if(enable_button.readButton()) 
          {
              if(isEnabled())  //Turn off if on
              {
                disable();
              }
              else if(batt_volt > LOAD_ALLOW) //turn on if allowed and off
              {
                enable();
              }
          }

          if(!isEnabled())  //if timer is active, activate load
                disable();
      }
  }
  private:
  void disable()
  {
      digitalWrite(output_no, LOW);
      timer.clear();    //stop timer
  }  
  void enable()
  {
      digitalWrite(output_no, HIGH);
      timer.reset();
  }
  
  button_type_c enable_button;
  uint8_t output_no;
  timeout_c timer;  //timer used for time out 
  
};

load_type_c   load1(LOAD1_PIN, LOAD1_BUTTON_PIN);
button_type_c screen_enable_button(SCREEN_BUTTON_PIN);



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

    
    display.setTextSize(2);
    display.setCursor(0,3 * 12);
    display.print("SOLAR");
    display.setCursor(3,3 * 12);
    display.print("AW");
    display.setCursor(6,3 * 12);
    display.print("YEA");
  
    trueDelay(800);

    display.setTextSize(1);

     // text display tests
    display.setCursor(0,0);
    display.println("   Solar test v0.1    ");
    display.println("                      ");
    display.println("Charge:     Load:     "); 
    display.println("                      ");
  
    //Debug
    //display.print("Raw ADC: ");
    //display.println(test);
    
  //  display.println("Treated val:       V  ");
    display.println("                      ");
    
    display.println("Battery:           V  "); 
    display.println("                      ");
    display.println("Debug count:          ");
    
    power_twi_disable();
    
    power_adc_enable();
    ADCSRA |= (1 << ADEN);
}

void screen_update()
{
  //Debug counter
  static uint32_t i = 0 ;
  i++;
  
  static timeout_c timer(SCREEN_UPDATE_MS);

  //Time to update screen
  if(timer.check_n_reset())
  {
    //Check disabling adc & wire later
    ADCSRA &= ~(1 << ADEN);
    power_adc_disable();
    power_twi_enable();
    
    display.setCursor(2, 8 * 6);
    if(charge_enabled)
      display.print("ON ");
    else
      display.print("OFF");
    
    display.setCursor(2, 17 * 6);
    if(load1.isEnabled())
    {
      
      display.print("ON "); 
    }
    else
      display.print("OFF");

  //Load time
    display.setCursor(3, 12 * 6 -2);
    int timeleft_s = load1.timeLeft() / 1000;
    int timeleft_m = timeleft_s / 60;
    timeleft_s %= 60;
    
    display.print(timeleft_m);
    display.print(':');
    display.print(timeleft_s,10);
    display.print("    ");
  
    //Debug
    //display.print("Raw ADC: ");
    //display.println(test);

    //threated volt
    //display.setCursor(4, 13 * 6);
    //display.print(volt, 3);

    //battery volt
    display.setCursor(5, 13 * 6);
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

  //Charge pin
  charge_off(); 

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

  // init done

  //Setup analog pin
  //??
  analogReference(INTERNAL);  //Set to 1.1 internal Vref

  //REFSx 11 means internal 1.1 vref
  ADMUX = (1 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (0x07 & BATTERY_VOLTAGE_CHANNEL); 
    

}


void loop() 
{

 // int test = analogRead(BATTERY_VOLTAGE_CHANNEL);
  int test = rawAnalogReadWithSleep();  //ADC should be set up for channel 0 now


  //internal ref measured to be 1.091
  //0x000 represents analog ground, and 0x3FF represents the selected reference voltage minus one LSB.
  volt = (test * 1.091 / 1024) * 0.01 + volt * 0.99;

  batt_volt = volt/ res_divisor;

  
  //resistor1 low side 2.16 kohm
  //resistor2 high side 11.78 kohm


  // ***************** CHARGE ************************
  
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
  
  // ***************** LOAD **************************
 
  load1.check();
  
  // ***************** SCREEN ************************
  
  static timeout_c screen_timer(SCREEN_TIMEOUT_MS);

  //screen toggle
  if(screen_enable_button.readButton())
  {
    if(screen_timer.check()) //check if we need to redraw screen
    {
      screen_draw();
    }
    screen_timer.reset();
  }

  //screen update
  if(!screen_timer.check())
  {
      screen_update();  
  }else{
      digitalWrite(SCREEN_POWER_PIN, HIGH);
  }

 
}





