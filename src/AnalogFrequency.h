
/*
 * AnalogFrequency.h by TMRh20 2016
 * 
 * Overview: 
 * Standard frequency counting libraries for Arduino typically involve edge detection and interrupts, which are good for processing
 * digital signals, but not so great for weaker analog signals. 
 * 
 * Use of the Arduino on-board ADC can provide not only the frequency of the waveform, but its amplitude as well. 
 *
 * The HB-100 Microwave Doppler Radar modules are fairly cheap, but require a pre-amp circuit to boost the IF signal. I used an LM358N 
 * and the circuit at https://hackaday.io/project/371-hb100-radar-shield. The output sits at about 2.5v when the HB100 is inactive.
 * The pre-amp is sensitive to changes in components if you are breadboarding. The more stable the signal when motionless, the better 
 * this code will work.
 * 
 * Problem: 
 * The HB-100 output indicates both frequency and amplitude of the received signal. Tests using the radar shield with standard frequency
 * counters/libraries showed that using interrupts and edge-detection has limited usefullness. 
 * 
 * Goal: 
 * Detect the frequency and amplitude of low frequency (0-4khz) signals from HB-100 modules, while potentially increasing the sensitivity
 * of waveform detection using HB-100 & circuit as linked above.
 * 
 * Result:
 * A simple process uses the Arduino ADC to sample incoming signals using analog peak-to-peak detection, above and below a set 
 * "midPoint" which represents the 0v value of the AC waveform. Typically, the midpoint will be ADC_RESOLUTION / 2 or 2.5v
 *
 * 
 *
 */
 

#ifndef __ANALOGFREQ_H__
#define __ANALOGFREQ_H__

// Uncomment to start waveform detection on a LOW signal instead of HIGH
//#define LOW_HI
//*******************************************************************

#if !defined(midPoint)
  #define midPoint 508      //The center or zero-point of the AC waveform expressed as a DC value between 0 & 1023
#endif
#if !defined(sensitivity)
  #define sensitivity 10    //Larger values = less sensitive (default: 5 max: < 1023-midPoint)
#endif

//*******************************************************************

volatile uint16_t reading = 0;
uint32_t varAvg = 0;
uint32_t varCnt = 0;
uint32_t ampAvg = 0;
uint32_t ampCnt = 0;

//*******************************************************************

//Keep a sum and count of the measured frequency
void saveFreq(uint32_t uS){
  varCnt++;  
  varAvg += 1000000/uS;
}
//*******************************************************************

//Keep a sum and count of the measured amplitude
void saveAmp(uint16_t amplitude){
  ampCnt++;
  ampAvg += amplitude;
}
//*******************************************************************

bool fAvailable(){
  return varCnt > 0 ? true : false;
}
//*******************************************************************

//Divide the freq. sum by the count to get the average
//Further divide by two, because we are counting half-cycles
uint32_t getFreq(uint32_t *amplitude = NULL, uint32_t *samples = NULL){
  noInterrupts();
  uint32_t avg = varAvg / varCnt / 2;
  *amplitude = ampAvg / ampCnt;
  *samples = varCnt;
  varAvg = 0;  varCnt = 0;
  ampAvg = 0;  ampCnt = 0;
  interrupts();
  return avg;
}

//*******************************************************************

void setupADC(int ADCPin) {
  
  ADCSRA = 0;

  //Easy way to ensure the analog reference & pin are selected
  analogRead(ADCPin);

  // Enable ADC, Auto Trigger & ADC Interrupt
  ADCSRA |= _BV(ADEN) | _BV(ADATE) | _BV(ADIE);
  // Set prescale to 32: 16Mhz / 32 / 13 = ~38.5khz
  ADCSRA |= _BV(ADPS0) | _BV(ADPS2);

  // Set free running mode as ADC Auto Trigger Source
  ADCSRB = 0;

  // START up the first conversion in free run mode
  ADCSRA |= _BV(ADSC); 

}

//*******************************************************************

volatile uint32_t upStartTime = 0, dnStartTime = 0;

ISR(ADC_vect) {
  
  reading = ADCL;
  reading |= ADCH << 8;

#if !defined LOW_HI
  if(reading >= midPoint+sensitivity && upStartTime == 0){
    upStartTime = micros();
  }else
  if(reading <= midPoint -sensitivity && upStartTime > 0){
    uint32_t tim = micros() - upStartTime;
    upStartTime = 0;
    if(tim > 50){
      saveFreq(tim);
    }
  }
#else
  if(reading <= midPoint-sensitivity && upStartTime == 0){
    upStartTime = micros();
  }else
  if(reading >= midPoint+sensitivity && upStartTime > 0){
    uint32_t tim = micros() - upStartTime;
    upStartTime = 0;
    if(tim > 50){
      saveFreq(tim);
    }
  }  

#endif
  if(upStartTime > 0){
      saveAmp(reading);
  }
}

//*******************************************************************

#endif
