
int led = 13;

void setup() {                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);  
  Serial.begin(9600);  
}

// the loop routine runs over and over again forever:
void loop() {
  if(Serial.available() > 0){
     if(Serial.read()== 'y'){
     digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
      Serial.print("y");  
   }else{
     digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW   
     Serial.print("n");
     }
  }
}
