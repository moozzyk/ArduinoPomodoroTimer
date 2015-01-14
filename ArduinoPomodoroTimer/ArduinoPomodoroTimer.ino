const int clock = 7;
const int data = 8;

                     /*0*/ /*1*/ /*2*/ /*3*/ /*4*/ /*5*/ /*6*/ /*7*/ /*8*/ /*9*/ 
uint8_t digits[] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f }; 

void setup() 
{
  setupInterrupt();
  
  pinMode(clock, OUTPUT);
  pinMode(data, OUTPUT);
  
  start();
  writeValue(0x8c);
  stop();
  
  // clear display
  write(0x00, 0x00, 0x00, 0x00);
}

byte tcnt2;
unsigned long time = 1500000; // 25 minutes

// Credits for the interrupt setup routine:
// http://popdevelop.com/2010/04/mastering-timer-interrupts-on-the-arduino/
void setupInterrupt()
{
  /* First disable the timer overflow interrupt while we're configuring */  
  TIMSK2 &= ~(1<<TOIE2);   

  /* Configure timer2 in normal mode (pure counting, no PWM etc.) */  
  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));   
  TCCR2B &= ~(1<<WGM22);   
  
  /* Select clock source: internal I/O clock */  
  ASSR &= ~(1<<AS2);
     
  /* Disable Compare Match A interrupt enable (only want overflow) */  
  TIMSK2 &= ~(1<<OCIE2A);   
  
  /* Now configure the prescaler to CPU clock divided by 128 */  
  TCCR2B |= (1<<CS22)  | (1<<CS20); // Set bits   
  TCCR2B &= ~(1<<CS21);             // Clear bit   
  
  /* We need to calculate a proper value to load the timer counter.  
   * The following loads the value 131 into the Timer 2 counter register  
   * The math behind this is:  
   * (CPU frequency) / (prescaler value) = 125000 Hz = 8us.  
   * (desired period) / 8us = 125.  
   * MAX(uint8) + 1 - 125 = 131;  
   */  
  /* Save value globally for later reload in ISR */  
  tcnt2 = 131;    
     
  /* Finally load end enable the timer */  
  TCNT2 = tcnt2;   
  TIMSK2 |= (1<<TOIE2);   
}

/*  
 * Install the Interrupt Service Routine (ISR) for Timer2 overflow.  
 * This is normally done by writing the address of the ISR in the  
 * interrupt vector table but conveniently done by using ISR()  */  
ISR(TIMER2_OVF_vect) {   
  /* Reload the timer */  
  TCNT2 = tcnt2;
  
  if(time > 0)
  {
    time--;
  }
}  

void loop() 
{
  unsigned long t = (unsigned long)(time/1000);  
  uint8_t minutes = (byte)((t / 60) % 60);
  uint8_t seconds = (byte)(t % 60);
  
  write(digits[minutes / 10], digits[minutes % 10] | ((seconds & 0x01) << 7) , digits[seconds / 10], digits[seconds % 10]);
}

void write(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth)
{
  start();
  writeValue(0x40);
  stop();
  
  start(); 
  writeValue(0xc0);
  writeValue(first);
  writeValue(second);
  writeValue(third);
  writeValue(fourth);

  stop();
}

void start(void)
{
  digitalWrite(clock,HIGH);//send start signal to TM1637
  digitalWrite(data,HIGH);
  delayMicroseconds(5);
  
  digitalWrite(data,LOW); 
  digitalWrite(clock,LOW); 
  delayMicroseconds(5);
} 

void stop(void)
{
  digitalWrite(clock,LOW);
  digitalWrite(data,LOW);
  delayMicroseconds(5);
  
  digitalWrite(clock,HIGH);
  digitalWrite(data,HIGH); 
  delayMicroseconds(5);
}

bool writeValue(uint8_t value)
{
  for(uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(clock, LOW);
    delayMicroseconds(5);   
    digitalWrite(data, (value & (1 << i)) >> i);
    delayMicroseconds(5);
    digitalWrite(clock, HIGH);
    delayMicroseconds(5);
  } 
    
  // wait for ACK
  digitalWrite(clock,LOW);
  delayMicroseconds(5);   
  
  pinMode(data,INPUT);
  
  digitalWrite(clock,HIGH);     
  delayMicroseconds(5);   
  
  bool ack = digitalRead(data) == 0;

  pinMode(data,OUTPUT);
  
  return ack;
}
