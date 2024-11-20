#include <Servo.h>

//Vart motorer är kopplade på breadboard
int servoPinL = 10;
int servoPinR = 11;

//Definiera variabler som ska kunna användas lite överallt
float currL = 0;
float currR = 0;
unsigned long t = 0;
unsigned long oldT = 0;
bool recent_wall_left = false;
bool recent_wall_right = false;
bool recent_wrong_dir = false;
unsigned long recent_time;
int turnDiff = 0;
// int last_turn = 0; // -1 if left and 1 if right 

// Vinkel
float gRot=0;
// float altgRot=0;

//det är en konstant så den får bo här uppe
float a_max = 0.1;

//Skapa objekt servo
Servo servoLeft;
Servo servoRight;

//Från m/s till ett signalvärde vänster hjul
float LeftSpeed2sign(float vL) {
  currL = vL; //behåll vänster hjuls senaste hastighet i en allmänt tillgänglig variabel
  return 29177*pow(vL, 3) + 446.32*pow(vL,2) + 47.132*vL + 1496.2;
  // return 781.7*vL + 1500.6;
  // return 1500+floor(36393.5*pow(vL,3)-332.3*pow(vL,2)+217.8*vL);
}

//från m/s till ett signalärde höger hjul
float RightSpeed2sign(float vR) {
  currR = vR; //behåll höger hjuls senaste hastighet i en allmänt tillgänglig variabel
  return -32503*pow(vR,3) + 91.574*pow(vR,2) - 10.643*vR + 1497.6;
  // return -828.76*(vR) + 1499.1;
  // return 1492-floor(36393.5*pow(vR,3)-332.3*pow(vR,2)+217.8*vR);
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

//du vet vad setup gör, här används den för att genomföra botens rutin
void setup() {
  Serial.begin(9600);

  //Whiskers
  pinMode(9, INPUT);
  pinMode(3, INPUT);
  // pinMode(2, OUTPUT);

  //Definiera vilken pin som servo ska få info från
  servoLeft.attach(servoPinL);
  servoRight.attach(servoPinR);


}

//Denna använder vi knappt.
void loop() {
  unsigned long t = micros();
  unsigned long sek = t/1000000.0;
  float dt = (t-oldT)/1000000.0;
  bool Lwhisker = !digitalRead(3);
  bool Rwhisker = !digitalRead(9);

  Serial.println("Vänster: "+String(currL)+"        Höger: "+String(currR)+"        Rotation: "+String(gRot)+"       turn_diff:"+String(turnDiff));


  // if (recent_wall_left || recent_wall_right) {
  //   tone(2, 300);
  // } 
  // else {
  //   noTone(2);
  // }

  if (Lwhisker & Rwhisker) {       //När båda nuddar, stanna, backa rakt tills endast en nuddar.
    recent_wall_left = false;
    recent_wall_right = false;

    recent_time = t;
    if(currL <= 0 & currR <= 0) { //Händelse 2
      a_max = 0.1; 
      acc(currR, -0.15, dt, 3);  
    }
    else { //Händelse 1
      a_max = 1.0; 
      acc(currL, 0, dt, 3);
    }
  }

  else if((/*abs(turnDiff) > 2 & */abs(gRot) > 120) || recent_wrong_dir == true) { //Försök att avbryta en färd åt fel håll
    recent_wrong_dir = true;
    if (currL > 0 & currR > 0) { //Händelse 1 sakta in
      a_max = 1.0; 
      acc(currL, 0, dt, 3);
    } 
    else if (gRot > 20) { // om svängt för långt höger Händelse 2 svänga tillbaka
      a_max = 0.1;
      acc(currL, -0.09, dt, 1); //Dessa värden verkade funka
      acc(currR, 0.095, dt, 2);
    } 
    else if (gRot < -20) { //om svängt för långt vänster Händelse 2 svänga tillbaka
      a_max = 0.1;
      acc(currL, 0.08, dt, 1);    //Dessa värden verkade funka
      acc(currR, -0.075, dt, 2);
    }
    else {              // Händelse 3 avsluta loopen
      recent_wrong_dir = false;
    }
  }

  else if (Lwhisker || recent_wall_left){     //If blocket för när vänster whisker just nu eller nyligen nuddat vägg
    recent_wall_left = true;

    if(Lwhisker) { // Händelse 1: nuddar fortfarande
      recent_time = t;
      if(currL <= 0 & currR <= 0) { //Händelse 1.2 backa.
        a_max = 0.1; 
        acc(currR, -0.15, dt, 3);  
      }
      else { //Händelse: 1.1 sakta in
        a_max = 1.0; 
        acc(currL, 0, dt, 3);
      }   
    }

    else if ((currL >= 0 & currR <= 0) & ((t - recent_time)/100000) <= 27) { //Händelse 4: svänga
      a_max = 0.1;
      acc(currL, 0.1, dt, 1);
      acc(currR, -0.1, dt, 2);
    }
    
    else if (currL == 0 & currR == 0){ //Händelse 6: Då bot stannat igen, avlsuta att komma in i if blocket
      recent_wall_left = false;
      turnDiff += 1;
      // last_turn = 1;
    } 
    
    else if((t - recent_time)/100000 >= 1) { //Händelse 3 och 5: Stanna igen
      acc(currL, 0, dt, 1);
      acc(currR, 0, dt, 2);
    }

    else { //Händelse 2: fortsätt backa tills andra if-satser tar över
      acc(currL, -0.15, dt, 3);
    }
  }

  else if (Rwhisker || recent_wall_right){ //If blocket för när vänster whisker just nu eller nyligen nuddat vägg
    recent_wall_right = true;

    if(Rwhisker) {   // Händelse 1: nuddar fortfarande
      recent_time = t;
      if(currR <= 0 & currL <= 0) { //Händelse 1.2 backa.
        a_max = 0.1; 
        acc(currL, -0.15, dt, 3);  
      }
      else {   //Händelse: 1.1 sakta in
        a_max = 1.0; 
        acc(currR, 0, dt, 3);
      }   
    }

    else if ((currR >= 0 & currL <= 0) & ((t - recent_time)/100000) <= 27) { //Händelse 4: svänga
        a_max = 0.1; 
        acc(currR, 0.1, dt, 2);
        acc(currL, -0.1, dt, 1);
    }
    
    else if (currL == 0 & currR == 0){  //Händelse 6: Då bot stannat igen, avlsuta att komma in i if blocket
      recent_wall_right = false;
      turnDiff -= 1;
      // last_turn = -1;
    } 
    
    else if((t - recent_time)/100000 >= 1) {  //Händelse 3 och 5: Stanna igen
      acc(currL, 0, dt, 1);
      acc(currR, 0, dt, 2);
    }

    else { //Händelse 2: fortsätt backa tills andra if-satser tar över
      acc(currR, -0.15, dt, 3);
    }

  }
  else { // Grundläge, åk framåt med försök att sikta mot globala rotationen (gRot)
    a_max = 0.1;
    float x=max(min(gRot/45,1),-1);
    float k=0.04;
    float dir=-x*pow(abs(x),0.25);

    // Serial.println(x);

    acc(currR, min(0.17, 0.17-dir*k), dt, 2);
    acc(currL, min(0.17, 0.17+dir*k), dt, 1);
  }

  // altgRot+=(currL-currR)/0.11;
  gRot+=asin(currL-currR)/dt;
  oldT=t;  
}