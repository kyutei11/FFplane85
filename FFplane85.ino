// Freeflight plane : motor timer ctrl & de-thermalizer

// for ATtiny85
//               -------------
//       RESET --| 1   U   8 |-- VCC(+)  
//  (A3,D3)PB3 --| 2       7 |-- PB2(D2,A1, SCK)
//  (A2,D4)PB4 --| 3       6 |-- PB1(D1,MISO,PWM)
//         GND --| 4       5 |-- PB0(D0,MOSI,AREF,PWM)
//               -------------

// interface : 1 button
//             1 motor PWM
//             1 magnet for DT
//             1 LED (also for serial : 2400bps, TX only)
//             1 CapV A/D measurement

/*
             DT coil        FET
           (~=100 Ohm)     S   D
  GND ---------CCC----------| |---+-----||-----GND
  DT  >----------------------= G  |    +  -
                                  |
                           S   D  |
  GND ---------[M]----------| |---+
  MOTOR >--------------------= G  |
                                  |
                     LED(GR)      |
(Serial)  220      Vf~=2.0V       |
  LED >---vvv---+------|<---------+
                |
  CapV <--------+

           / SW
  BTN <---o o---GND

*/

/* circuit using ATtiny10 for potential application(impossible due to memory limitation)...
   (this code is for ATtiny85)

                DT coil
              (~=100 Ohm)          / SW
  GND ------------CCC---------+---o o---+
                              |         |
                    LED(GR)   |         |
          220      Vf~=2.0V   |  S   D  |  Li-ion Cap
  GND ---vvv---+-----|<-------+---| |---+-----||-----GND
               |                   = G       +  -
  CapV <-------+                   |   
                                   |
  DT/LED >-------------------------+       
*/

//  void setup
//     |
//     |- activate magnet for deThermalizer(OFF)
//     |
//     |- wait until BTN == LOW
//     |
//     |- if BTN == LOW > 500mSec, set motorRun duration
//     |    | 
//     |    |- 5sec, 10sec, 20sec, for each press...
//     |    | 
//     |    |- if BTN == LOW > 500mSec
//     |    |    | 
//     |    |    |- set deThermalizer activation time
//     |    |    |  after motor stops
//     |    |    | 
//     |    |    |- 10sec, 20sec, 30sec, for each press...
//     |    |    | 
//     |    |    |- if BTN == LOW > 500mSec, break
//     |    | 
//     |    |- write parameters to EEPROM
//     |
//     |- void loop
//          |
//          |- wait until BTN == LOW : for each FLT
//          |- activate DT
//          |- wait 5sec
//          |- motorRun
//          |- release DT

#include <EEPROM.h>

#define addrMT 0
#define addrDT 1

// all portNo is in PB*, also for analog input
const int Motor = 0; // need PWM
const int DeThm = 1;

const int LED  = 2;
const int CapV = 3; // A3

const int BTN = 4;

byte t_MT, t_DT;
#define intv_mS 200

/* #define unitMT  5 // sec */
/* #define unitDT 10 */

/* #define maxMTtime 30 // sec max */
/* #define maxDTtime 60 */

#define unitMT  5 // sec
#define unitDT 10

#define maxMTtime 30 // sec
#define maxDTtime 60

//                          Vlow:3.2   2.0 <- your favorite color...
const int LowAD = 384; // 1024* (Vin - Vf_LED) / Vin

void setup() {

  pinMode(CapV, INPUT);
  
  pinMode(BTN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  
  pinMode(Motor, OUTPUT);
  pinMode(DeThm, OUTPUT);

  digitalWrite(DeThm, HIGH);
  digitalWrite(LED  , HIGH);

  t_MT = EEPROM.read(addrMT);
  if(t_MT == 0xFF){ // init.
	EEPROM.write(addrMT, unitMT);
	EEPROM.write(addrDT, unitDT);
  }
  else t_DT = EEPROM.read(addrDT);

  // MotorRun --> DTtime --> DT activation
  if(Pressed500mS()){
	print_SS(LED, "Time set...", HIGH);
	// Motor time set
	t_MT = setTime(addrMT, t_MT, unitMT, maxMTtime);
	// DT time set
	t_DT = setTime(addrDT, t_DT, unitDT, maxDTtime);
	if(t_DT < t_MT){
	  t_DT = t_MT;
	  EEPROM.write(addrDT, t_DT);
	}
  }
  print_SS(LED, "t_MT, t_DT= " , HIGH);
  prn99_SS(LED, t_MT, HIGH); print_SS(LED, " " , HIGH);
  prn99_SS(LED, t_DT, HIGH); print_SS(LED, "\n", HIGH);

  // BATchk LED
  digitalWrite(LED, LOW); // LED on

  delay(200); // waitl until Voltage stabilize
  // if low voltage, no motorRun and alarm
  
  if(analogRead(CapV) < LowAD)
	while(1){
	  print_SS(LED, "Low V\n", LOW);
	  delay(500);
	  digitalWrite(LED,HIGH);
	  delay(500);
	  digitalWrite(LED, LOW);
	}
  analogWrite(Motor, 0);
  digitalWrite(LED, HIGH);
  print_SS(LED, "Goto loop...\n", HIGH); // LED off
}

void loop() {
  unsigned int mSec, ADcap, PWMout, PWM_prev;

  while(digitalRead(BTN) == HIGH){} // for each FLT
  digitalWrite(DeThm, HIGH);
  print_SS(LED, "DT activated\n", HIGH);

  for(mSec=0; mSec< 5000; mSec+=1000){
	digitalWrite(LED, HIGH);
	analogWrite(Motor, 0);
	delay(800);
	digitalWrite(LED, LOW);
	delay(200);
  }

  PWM_prev = 0;
  print_SS(LED, "Motor ON\n", LOW);
  // start motor run
  for(mSec=0; mSec < t_MT*1000; mSec += intv_mS){
	ADcap = analogRead(CapV); // 0..1023

	// AD = 1024*(Vcap - Vf_LED)/Vcap
	// AD = 1024-1024*Vf/Vcap
	// 1/Vcap = (1024-AD)/(1024*Vf)

	// Vf ~=2.0V, V_motor = 2.5V...
	
	// PWMoutput = 256*V_motor/Vcap
	//           = V_motor*(1024-AD)/4Vf
	//          ~= (1024-AD)*5/16 : V_motor = 2.5V
	//          ~= (1024-AD)*3/ 8 : V_motor = 3.0V
	//          ~= (1024-AD)*2/ 5 : V_motor = 3.2V
	
	//PWMout = PWM_prev/2 + ((1024-ADcap)*5)/(16*2); // 2.5V : LPF
	//PWMout = PWM_prev/2 + ((1024-ADcap)*3)/(8*2); // 3.0V : LPF
	PWMout = PWM_prev/2 + ((1024-ADcap))/5; // 3.2V : LPF
	//if(PWMout<  0) PWMout =   0; not likely...
	if(PWMout>255){ // low voltage
	  analogWrite(Motor, 0); // cutoff
	  break;
	}
	//prn99_SS(LED, (PWMout*4)/10, LOW); print_SS(LED, "\n", LOW);
	analogWrite(Motor, PWMout);
	PWM_prev = PWMout;

	delay(intv_mS);
  }
  analogWrite(Motor, 0);
  
  delay((t_DT-t_MT)*1000); // wait until DT activation
  digitalWrite(DeThm, LOW);
  digitalWrite(LED  , HIGH);
  print_SS(LED, "DT OFF\n", HIGH);
}

byte setTime(byte addr, byte T, byte unitT, byte maxT){
  byte tOut, count;
  count = 0;
  
  while(1){
	if(Pressed500mS()){
	  EEPROM.write(addr, tOut);
	  break;
	}
	else{
	  count++;
	  tOut = count*unitT;
	  if(tOut > maxT) count = 1;
	  blink(addr, count);
	  }
	}
  return(tOut);
}

void blink(int mode, int count){
  int i;
  for(i=1; i<=count; i++){
	print_SS(LED, "mode=", HIGH);
	prn99_SS(LED, mode, HIGH);
	print_SS(LED, "\n", HIGH);
	switch(mode){
	  case addrMT: // motorRun time set
		print_SS(LED, "MT count = ", HIGH);
		prn99_SS(LED, i, HIGH);
		print_SS(LED, "\n", HIGH);
		digitalWrite(LED,  LOW); // on
		delay(150);
		digitalWrite(LED, HIGH); // off
		break;

	  case addrDT: // deThermalizer time set
		print_SS(LED, "DT count = ", HIGH);
		prn99_SS(LED, i, HIGH);
		print_SS(LED, "\n", HIGH);
		digitalWrite(LED,  LOW); // on
		delay(50);
		digitalWrite(LED, HIGH); // off
		delay(50);
		digitalWrite(LED,  LOW);
		delay(50);
		digitalWrite(LED, HIGH);
		break;
	}
	delay(350);
  }
}

int Pressed500mS(){
  unsigned long T;

  while(digitalRead(BTN) == HIGH){}
  T = millis();
  delay(100); // not to pick up chattering
  while(digitalRead(BTN) == LOW){}
  if(millis()-T > 500)
	return 1;
  else
	return 0;
}

// software serial for debug, 2400bps, TX only
void putc_SS(int txCH, char c){
  // start bit
  digitalWrite(txCH, LOW);
  delayMicroseconds(415);

  for (int i=0; i<8; i++){
    digitalWrite(txCH, (c>>i) & 1  );
	delayMicroseconds(415);
  }

  // stop bit
  digitalWrite(txCH, HIGH);
  delayMicroseconds(415);
}

// do not use String(int) for *str
void print_SS(int txCH, char *str, int initHighLow){
  digitalWrite(txCH, HIGH); delay(5); // for startbit detection
    
  while (*str) putc_SS(txCH, *str++);
  digitalWrite(txCH, initHighLow); // keep prev. HIGH/LOW state
}

// print 0..99 number
void prn99_SS(int txCH, int i, int initHighLow){
  char c;

  if(i<0)       print_SS(txCH, "<0", initHighLow);
  else if(i>99) print_SS(txCH, "MX", initHighLow);
  else{
	digitalWrite(txCH, HIGH); delay(5); // for startbit detection
	c=i%10;          // '0': 0x30
	putc_SS(txCH, (i-c)/10 + 0x30);
	putc_SS(txCH,    c     + 0x30);
	digitalWrite(txCH, initHighLow); // keep prev. HIGH/LOW state
  }
}
