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
unsigned long recent_time;
int turnDiff = 0;

// Vinkel
float gRot=0;

//det är en konstant så den får bo här uppe
float a_max = 0.1;

//Skapa objekt servo
Servo servoLeft;
Servo servoRight;

//Från m/s till ett signalvärde vänster hjul
float LeftSpeed2sign(float vL) {
  currL = vL; //behåll vänster hjuls senaste hastighet i en allmänt tillgänglig variabel
  return 781.7*vL + 1500.6;
  // return 1500+floor(36393.5*pow(vL,3)-332.3*pow(vL,2)+217.8*vL);
}

//från m/s till ett signalärde höger hjul
float RightSpeed2sign(float vR) {
  currR = vR; //behåll höger hjuls senaste hastighet i en allmänt tillgänglig variabel
  return -828.76*(vR) + 1499.1;
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

// //sväng med ingående hastighet arg1 och agr2 bestämmer vänster eller höger sväng.
// void turn(float currentV, float targetV, float dt, int turn) {
//   acc(currentV, targetV, dt, turn);
//   // acc(currentV/turnaggresion, currentV, dt, turn);
// }

// void recoverTurn(float currentV, float targetV, float dt, int turn) {
//   acc(currentV, targetV, dt, turn);
// }


//du vet vad setup gör, här används den för att genomföra botens rutin
void setup() {
  Serial.begin(9600);

  //Whiskers
  pinMode(9, INPUT);
  pinMode(3, INPUT);
  pinMode(2, OUTPUT);

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

  
  




  // if (recent_wall_left || recent_wall_right) {
  //   tone(2, 300);

  // } 
  // else {
  //   noTone(2);
  // }

  // Serial.println("Vänster: "+String(currL)+"        Höger: "+String(currR)+"      Rotation: "+String(gRot)+"        a_max:"+String(a_max));


  if (Lwhisker & Rwhisker) { //När båda nuddar, stanna, backa rakt tills endast en nuddar.
    recent_wall_left = false;
    recent_wall_right = false;

      recent_time = t;
      if(currL <= 0 & currR <= 0) {
        a_max = 0.1; 
        acc(currR, -0.13, dt, 3);  
      }
      else {
        a_max = 1.0; 
        acc(currL, 0, dt, 3);
      }
  }




  else if (Lwhisker || recent_wall_left){     
    recent_wall_left = true;

    if(Lwhisker) {
      recent_time = t;
      if(currL <= 0 & currR <= 0) {
        a_max = 0.1; 
        acc(currR, -0.1, dt, 3);  
      }
      else {
        a_max = 1.0; 
        acc(currL, 0, dt, 3);
      }   
    }
    else if ((currL >= 0 & currR <= 0) & ((t - recent_time)/1000000) <= 2) {
      a_max = 0.1;
      acc(currL, 0.13, dt, 1);
      acc(currR, -0.13, dt, 2);
    }
    
    else if (currL == 0 & currR == 0){
      recent_wall_left = false;
    } 
    
    else if((t - recent_time)/1000000 >= 0.1) {
      acc(currL, 0, dt, 1);
      acc(currR, 0, dt, 2);
    }

    else {
      acc(currL, -0.1, dt, 3);
    }
    

    // recent_wall_left = true;

    // if(Lwhisker) {
    //   recent_time = t;
    //   if(currL <= 0 & currR <= 0) {
    //     a_max = 0.1; 
    //     acc(currR, -0.13, dt, 2);  
    //     acc(currL, -0.1, dt, 1);
    //   }
    //   else {
    //     a_max = 1.0; 
    //     acc(currL, 0, dt, 3);
    //   }   
    // }
    // else if((t - recent_time)/1000000 >= 1) {
    //   acc(currR, 0, dt, 2);
    //   acc(currL, 0, dt, 1);
    //   if (currL == 0 & currR == 0) {
    //     turnDiff += 1;
    //     recent_wall_left = false;
    //   }
    // }
    // else {
    //   acc(currR, -0.13, dt, 2);
    //   acc(currL, -0.1, dt, 1);
    // }
    




  }
  else if (Rwhisker || recent_wall_right){
    recent_wall_right = true;

    if(Rwhisker) {
      recent_time = t;
      if(currR <= 0 & currL <= 0) {
        a_max = 0.1; 
        acc(currL, -0.1, dt, 3);  
      }
      else {
        a_max = 1.0; 
        acc(currR, 0, dt, 3);
      }   
    }
    else if ((currR >= 0 & currL <= 0) & ((t - recent_time)/1000000) <= 2.0) {
        a_max = 0.1; 
        acc(currR, 0.06, dt, 2);
        acc(currL, -0.06, dt, 1);
    }
    
    else if (currL == 0 & currR == 0){
      recent_wall_right = false;
    } 
    
    else if((t - recent_time)/1000000 >= 0.1) {
      acc(currL, 0, dt, 1);
      acc(currR, 0, dt, 2);
    }

    else {
      acc(currR, -0.1, dt, 3);
    }











  //   recent_wall_right = true;

  //   if(Rwhisker) {
  //     recent_time = t;
  //     if(currL <= 0 & currR <= 0) {
  //       a_max = 0.1; 
  //       acc(currL, -0.13, dt, 1);  
  //       acc(currR, -0.1, dt, 2);
  //     }
  //     else {
  //       a_max = 1.0; 
  //       acc(currR, 0, dt, 3);
  //     }   
  //   }
  //   else if((t - recent_time)/1000000 >= 1) {
  //     acc(currR, 0, dt, 2);
  //     acc(currL, 0, dt, 1);
  //     if (currL == 0 & currR == 0) {
  //       turnDiff -= 1;
  //       recent_wall_right = false;
  //     }
  //   }
  //   else {
  //     acc(currL, -0.13, dt, 1);
  //     acc(currR, -0.1, dt, 2);
  //   }
  }
  else { //Standard mode, drive forward
    a_max = 0.1;
    float x=max(min(gRot/30,1),-1);
    float k=0.08;
    float dir=-x*pow(abs(x),1.5);

    acc(currR, min(0.17, 0.17-dir*k), dt, 2);
    acc(currL, min(0.17, 0.17+dir*k), dt, 1);

  }

  
  gRot+=asin(currL-currR)/dt;
  

  oldT=t;
  
}