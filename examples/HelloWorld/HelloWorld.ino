


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Booted");
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("yay!");
  delay(500);
}
