
//Use pin13 to generate a digital waveform


bool onOff = 0;

ISR (TIMER1_COMPA_vect){
  digitalWrite(13,onOff);
    onOff = !onOff;
}

void setup() {
  pinMode(13,OUTPUT);
  TCCR1B = 0;
  TCCR1A = 0;
  TCCR1B =  _BV(WGM13) | _BV(WGM12) | _BV(CS10);// | _BV(CS10);
  TCCR1A = _BV(WGM11);
  ICR1 = 2000;
  OCR1A = 1000;
  TIMSK1 |= _BV(OCIE1A);

}

void loop() {
  // put your main code here, to run repeatedly:

}