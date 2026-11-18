// Two IBT-2 drivers + Start, E-stop, and Reset buttons
// Arduino Uno

const byte RPWM[2] = {5, 9};
const byte LPWM[2] = {6, 10};

const byte START_BUTTON = 2;
const byte ESTOP_BUTTON = 3;
const byte RESET_BUTTON = 4;

const unsigned long MOVE_MS = 1000;
const unsigned long REST_MS = 3000;

int activeMotor = -1;
unsigned long moveStarted = 0;
unsigned long stoppedAt = 0;

bool resting = false;
bool estopLatched = false;

bool lastStartState = HIGH;
bool lastResetState = HIGH;


void stopAll() {
  for (byte i = 0; i < 2; i++) {
    analogWrite(RPWM[i], 0);
    analogWrite(LPWM[i], 0);
  }

  if (activeMotor != -1) {
    stoppedAt = millis();
    resting = true;
  }

  activeMotor = -1;
}


void startMotor(byte motor, bool directionA) {
  // Make certain neither actuator has PWM before starting.
  for (byte i = 0; i < 2; i++) {
    analogWrite(RPWM[i], 0);
    analogWrite(LPWM[i], 0);
  }

  resting = false;
  moveStarted = millis();
  activeMotor = motor;

  if (directionA) {
    analogWrite(RPWM[motor], 255);
    analogWrite(LPWM[motor], 0);
  } else {
    analogWrite(RPWM[motor], 0);
    analogWrite(LPWM[motor], 255);
  }

  Serial.print("Actuator ");
  Serial.print(motor + 1);
  Serial.println(directionA ? ": direction A" : ": direction B");
}


void setup() {
  for (byte i = 0; i < 2; i++) {
    digitalWrite(RPWM[i], LOW);
    digitalWrite(LPWM[i], LOW);
    pinMode(RPWM[i], OUTPUT);
    pinMode(LPWM[i], OUTPUT);
  }

  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(ESTOP_BUTTON, INPUT_PULLUP);
  pinMode(RESET_BUTTON, INPUT_PULLUP);

  stopAll();

  Serial.begin(9600);
  Serial.println("Ready.");
  Serial.println("Start button = actuator 1 direction A test.");
  Serial.println("E-stop latches motion off; Reset clears it.");
  Serial.println("Serial: 1, 2, 3, 4, or s.");
}


void loop() {
  unsigned long now = millis();

  // Physical/software E-stop input.
  if (digitalRead(ESTOP_BUTTON) == LOW) {
    if (!estopLatched) {
      Serial.println("E-STOP ACTIVE.");
    }
    stopAll();
    estopLatched = true;
  }

  // Reset only works after E-stop is released.
  bool resetState = digitalRead(RESET_BUTTON);

  if (lastResetState == HIGH && resetState == LOW) {
    if (digitalRead(ESTOP_BUTTON) == HIGH) {
      estopLatched = false;
      resting = false;
      Serial.println("E-stop cleared. Ready.");
    } else {
      Serial.println("Release E-stop before resetting.");
    }
  }
  lastResetState = resetState;

  // Block every motion command while stopped.
  if (estopLatched) {
    stopAll();
    return;
  }

  // Automatically stop after the movement duration.
  if (activeMotor != -1 && now - moveStarted >= MOVE_MS) {
    stopAll();
    Serial.println("Stopped. Resting for 3 seconds.");
  }

  if (resting && now - stoppedAt >= REST_MS) {
    resting = false;
    Serial.println("Ready for another command.");
  }

  // Start button: temporary one-actuator test.
  bool startState = digitalRead(START_BUTTON);

  if (lastStartState == HIGH && startState == LOW) {
    if (activeMotor == -1 && !resting) {
      Serial.println("Start pressed.");
      startMotor(0, true);
    } else {
      Serial.println("Wait until ready.");
    }
  }
  lastStartState = startState;

  // Retain Serial Monitor commands for individual testing.
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (command == '\n' || command == '\r' || command == ' ') {
      return;
    }

    if (command == 's' || command == 'S') {
      stopAll();
      Serial.println("Stop command received.");
      return;
    }

    if (command < '1' || command > '4') {
      Serial.println("Use 1, 2, 3, 4, or s.");
      return;
    }

    if (activeMotor != -1 || resting) {
      Serial.println("Wait until ready; command ignored.");
      return;
    }

    switch (command) {
      case '1': startMotor(0, true);  break;
      case '2': startMotor(0, false); break;
      case '3': startMotor(1, true);  break;
      case '4': startMotor(1, false); break;
    }
  }
}
