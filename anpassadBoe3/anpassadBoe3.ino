#include <Servo.h>

const float Pi = 3.14159; // To however many digits you want.

//Vart motorer är kopplade på breadboard
int servoPinL = 10;
int servoPinR = 11;

//Definiera variabler som ska kunna användas lite överallt
float currL = 0;
float currR = 0;
unsigned long t = 0;
unsigned long oldT = 0;

int recent_wall = 0;
bool recent_wrong_dir = false;
unsigned long recent_time;

int whiskers = 0;
int modifier = 0;

// Vinkel
float gRot=0;

//det är en konstant så den får bo här uppe
float a_max = 0.2;

//Skapa objekt servo
Servo servoLeft;
Servo servoRight;

//Från m/s till ett signalvärde vänster hjul
float LeftSpeed2sign(float vL) {
  currL = vL; //behåll vänster hjuls senaste hastighet i en allmänt tillgänglig variabel
  return 29177*pow(vL, 3) + 446.32*pow(vL,2) + 47.132*vL + 1496.2;
}

//från m/s till ett signalärde höger hjul
float RightSpeed2sign(float vR) {
  currR = vR; //behåll höger hjuls senaste hastighet i en allmänt tillgänglig variabel
  return -32503*pow(vR,3) + 91.574*pow(vR,2) - 10.643*vR + 1497.6;
}

//Kör framåt med samma hastighet i båda hjul
int drive(float L, float R) {
  servoLeft.writeMicroseconds(LeftSpeed2sign(L));
  servoRight.writeMicroseconds(RightSpeed2sign(R));
}

//accelerera från arg1 till arg2, arg3 bestämmer vilka hjul som accar
float acc(float currentV, float targetV, float dt, int wheels) {

  float a_target = (targetV - currentV)/(dt); 
  
  float a_allowed = constrain(a_target, -a_max, a_max);

  float allowedV = currentV + a_allowed*dt;
  
  if (wheels == 3) { //Båda hjul accar samma 
    drive(allowedV, allowedV);
  }
  else if (wheels == 1) { //Vänster accar, höger behåller samma v
    drive(allowedV, currR);
  }
  else if (wheels == 2) { //höger accar, vänster behåller samma v
    drive(currL, allowedV);
  }
  return allowedV;
}

void stop(float dt) {
  a_max = 1.0; 
  acc(currL, 0, dt, 1);
  acc(currR, 0, dt, 2);
}

void whiskerAction(float dt, unsigned long t){
  if(whiskers) {
    recent_time = t;
    recent_wall = whiskers;
    modifier = -cos(recent_wall*Pi); // har 1, 2 vill ha -1, 1
    if(currL <= 0 & currR <= 0) {
      a_max = 0.2; 
      acc((currR+currL)/2, -0.15, dt, 3);  
    }
    else {
      stop(dt);
    }   
  }
  else if (((currL + currR) == 0) & ((t - recent_time)/100000) <= 20) {
    a_max = 0.2;
    acc(currL, modifier*0.1, dt, 1);
    acc(currR, -modifier*0.1, dt, 2);
    Serial.println("här inne");

  }
  else if (currL == 0 & currR == 0){
    recent_wall = 0;
  } 
  else { 
    stop(dt);
  }  
}


//du vet vad setup gör, här används den för att genomföra botens rutin
void setup() {
  Serial.begin(9600);

  //Whiskers
  pinMode(9, INPUT);
  pinMode(3, INPUT);

  //Definiera vilken pin som servo ska få info från
  servoLeft.attach(servoPinL);
  servoRight.attach(servoPinR);
}

//Denna använder vi knappt.
void loop() {
  unsigned long t = micros();
  unsigned long sek = t/1000000.0;
  float dt = (t-oldT)/1000000.0;

  whiskers = !digitalRead(3) + 2*!digitalRead(9);  //0 when no whisker, 1 when left, 2 when right, 3 when both | Like binary

  // Serial.println("Whisker |"+String(whiskers)+"| Recent_wall |"+String(recent_wall)+"| Modifier |"+String(modifier)+"| Recent_time |"+String(((t - recent_time)/100000) <= 20)+"|");
  // Serial.println("Vänster: "+String(currL)+"        Höger: "+String(currR)+"        Rotation: "+String(gRot)+"       turn_diff:"+String(turnDiff));

  if (whiskers || recent_wall){
    whiskerAction(dt, t);
  }

  else if((abs(gRot) > 120) || recent_wrong_dir == true) { //Försök att avbryta en färd åt fel håll
    recent_wrong_dir = true;
    recent_wall = 0;
    if (currL > 0 & currR > 0) { //Händelse 1 sakta in
      a_max = 1.0; 
      stop();
    } 
    else if (abs(gRot) > 20) {
      int rotModifier = gRot/abs(gRot);
      a_max = 0.2;
      acc(currL, rotModifier*0.09, dt, 1);
      acc(currR, -rotModifier*0.09, dt, 2);
    }
    else {              // Händelse 3 avsluta loopen
      recent_wrong_dir = false;
    }
  }

  else { // Grundläge, åk framåt med försök att sikta mot globala rotationen (gRot)
    a_max = 0.2;
    float x=max(min(gRot/45,1),-1);
    float k=0.04;
    float dir=-x*pow(abs(x),0.25);

    acc(currR, min(0.17, 0.17-dir*k), dt, 2);
    acc(currL, min(0.17, 0.17+dir*k), dt, 1);
  }

  gRot+=asin(currL-currR)/dt;
  oldT=t;  
}