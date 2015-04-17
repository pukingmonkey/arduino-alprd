// ALPR detector using frequncy counters --
//    - digitalRead() is slow, ~5uS; so max freq is 200KHz/no of read pins
//    - (millis() reduces max freq to 1KHz, so use micros())
//   IR sensors on IRpin[] 2, 4, 6, 8 10
//   optional visible VISpin[] 3, 5, 7, 9, 11 (help to suppress false postives)
//   alerting device (monkey) goes in ALRTpin 12
//   so pairings [IR,VIS] are pins [2,3]; [4,5]; [6,7]; [8,9]; and [10,11]

// set this to how many sensor pairs you really have
#define SENSORPAIRS 1 // 5

// define if using VISIBLE sensors, comment out if not using
#define VISIBLE

#define MINHZ 3
#define MAXHZ 15

boolean beep[] = {false, false, false, false, false};
long timeBeep = 0;

int oldIRval[] = {0, 0, 0, 0, 0};
int IRn[] = {0, 0, 0, 0, 0};
long IRfreq[] = {0, 0, 0, 0, 0};

#ifdef VISIBLE
int oldVISval[] = {0, 0, 0, 0, 0};
int VISn[] = {0, 0, 0, 0, 0};
long VISfreq[] = {0, 0, 0, 0, 0};
#endif

// I-O pin numbers
const int IRpin[] = {2, 4, 6, 8, 10};
const int ALRTpin = 12;
const int LEDpin = 13;
#ifdef VISIBLE
const int VISpin[] = {3, 5, 7, 9, 11};
#endif

long IRtimeStart[] = {0, 0, 0, 0, 0};
#ifdef VISIBLE
long VIStimeStart[] = {0, 0, 0, 0, 0};
#endif

const long TimeBase = 1000000; // foi micros (1000 if millis)

const long BEEPTIME = TimeBase * 3 / 2;

void setup() {
  int i;
  for(i=0;i<SENSORPAIRS;i++) {
     pinMode(IRpin[i], INPUT);
     // digitalWrite(IRpin[i], HIGH);
#ifdef VISIBLE
     pinMode(VISpin[i], INPUT);
     // digitalWrite(VISpin[i], HIGH);
#endif
  }
  pinMode(ALRTpin, OUTPUT);
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  // Serial.println("Start");
  for(i=0;i<SENSORPAIRS;i++) {
     oldIRval[i] = digitalRead(IRpin[i]);
#ifdef VISIBLE
     oldVISval[i] = digitalRead(VISpin[i]);
#endif
  }
  timeBeep = micros();
  digitalWrite(ALRTpin, HIGH); // alert test on power up
  beep[0]=true;
}

void loop()
{
  int i, IRval[SENSORPAIRS];
#ifdef VISIBLE
  int VISval[SENSORPAIRS];
#endif
  long now;

  for(i=0;i<SENSORPAIRS;i++) {
  IRval[i] = digitalRead(IRpin[i]);
#ifdef VISIBLE
  VISval[i] = digitalRead(VISpin[i]);
#endif
  digitalWrite(LEDpin, IRval[i]);
  now=micros();

  if(IRval[i] != oldIRval[i]) {    // state change on IR
      if(++IRn[i] == 1) {
        IRtimeStart[i] = now;
      } else if (IRn[i] == 5) {
        long timeStop = now;
        IRfreq[i] = (TimeBase * 2) / (timeStop - IRtimeStart[i]);
        IRn[i] = 1;
        IRtimeStart[i] = timeStop;
      }
      oldIRval[i] = IRval[i];
  } else if(now - IRtimeStart[i] > TimeBase) IRfreq[i] = 0;

#ifdef VISIBLE
  if(VISval[i] != oldVISval[i]) {    // state change on VIS
      if(++VISn[i] == 1) {
        VIStimeStart[i] = now;
      } else if (VISn[i] == 5) {
        long timeStop = now;
        VISfreq[i] = (TimeBase * 2) / (timeStop - VIStimeStart[i]);
        VISn[i] = 1;
        VIStimeStart[i] = timeStop;
      }
      oldVISval[i] = VISval[i];
  } else if(now - VIStimeStart[i] > TimeBase) VISfreq[i] = 0;
#endif

  // if(!(now%25000)) Serial.println(IRfreq[i]);

  if(IRfreq[i] > MINHZ && IRfreq[i] < MAXHZ) { // have IR freq in range
     boolean doAlert = false;
     Serial.println(IRfreq[i]);
#ifdef VISIBLE
     Serial.println(VISfreq[i]);
#endif
      Serial.println("---------");

     // should we alert? check vis freq, make sure its not old
#ifdef VISIBLE
     if(IRtimeStart[i] - VIStimeStart[i] > TimeBase/2) { // too old
#endif
        doAlert = true;
#ifdef VISIBLE
     } else {
         if(VISfreq[i] < (IRfreq[i]-1) || VISfreq[i] > (IRfreq[i]+1)) { // vis out of range
            doAlert = true;
         } else if(beep[i]) { // vis in range, all ready alerting? turn off
                boolean turnOFF = true;
                beep[i]=false;
                IRfreq[i] = 0;
                for(int n=0;n<SENSORPAIRS;n++) // make sure no other is alerting
                  if(n!=i && beep[n]) turnOFF=false;
                if(turnOFF) digitalWrite(ALRTpin, LOW);
         }
     }
#endif

     if(doAlert) {
        digitalWrite(ALRTpin, HIGH);
        beep[i]=true;
        timeBeep = IRtimeStart[i];
        IRfreq[i] = 0;
     } else if(now - IRtimeStart[i] > TimeBase/2) { // IR freq now too old
            IRfreq[i] = 0;
     }
  }

  if(beep[i] && (micros() - timeBeep)>BEEPTIME) {
      digitalWrite(ALRTpin, LOW);
      beep[i]=false;
  }
  }
}
