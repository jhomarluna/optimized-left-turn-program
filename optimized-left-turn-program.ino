// PIN SETUP
const int red = 9;
const int yellow = 8;
const int green = 7;

const int trigPin = 3;
const int echoPin = 2;

// LIGHT TIMINGS
const unsigned long RED_MS    = 6000;
const unsigned long YELLOW_MS = 2000;
const unsigned long GREEN_MS  = 6000;

// LIGHT STATE MACHINE
enum LightState { RED_LIGHT, YELLOW_LIGHT, GREEN_LIGHT };
LightState lightState = RED_LIGHT;     
unsigned long stateStartMs = 0;
unsigned long stateDurationMs = RED_MS;

// Helper function to set lights and state timing
void setLight(LightState s) {
  lightState = s;
  stateStartMs = millis();

  switch (s) {
    case RED_LIGHT:
      stateDurationMs = RED_MS;
      break;
    case GREEN_LIGHT:
      stateDurationMs = GREEN_MS;
      break;
    case YELLOW_LIGHT:
      stateDurationMs = YELLOW_MS; 
      break;
  }

  digitalWrite(red,    s == RED_LIGHT);
  digitalWrite(yellow, s == YELLOW_LIGHT);
  digitalWrite(green,  s == GREEN_LIGHT);
}

// SENSOR SETTINGS
const unsigned long SENSOR_PERIOD_MS = 60;    // how often to read sensor
const unsigned long ECHO_TIMEOUT_US  = 30000; // ~5m max
const float THRESHOLD = 20.0;                 // force red if car is closer than this (cm)

unsigned long lastSensorMs = 0; // when we last read the sensor
unsigned long durationUs = 0;   // echo time
float distanceCm = NAN;         // latest distance
bool obstacleClose = false;     // use one consistent name

float readDistanceCm() {
  // Trigger the ultrasonic burst
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo with timeout so it won't block forever
  durationUs = pulseIn(echoPin, HIGH, ECHO_TIMEOUT_US);
  if (durationUs == 0) {
    return NAN;  // no echo / out of range
  }

  // Distance = speed * time / 2
  // Speed of sound ≈ 0.0343 cm/µs
  return (durationUs * 0.0343f) / 2.0f;  // cm
}

void setup() {
  // Set up LED pins as outputs
  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
  delay(50);  // sensor settle

  Serial.begin(9600);
  setLight(RED_LIGHT);      
}

void loop() {
  unsigned long now = millis();

  // SENSOR: run every SENSOR_PERIOD_MS without blocking
  if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;

    float d = readDistanceCm();
    if (isnan(d)) {
      // Treat as "clear" if no echo (but log it)
      Serial.println("No echo (treating as clear).");
      obstacleClose = false;
    } else {
      distanceCm = d;
      obstacleClose = (distanceCm < THRESHOLD);

      Serial.print("Distance(cm): ");
      Serial.print(distanceCm, 1);
      Serial.print("  Close? ");
      Serial.println(obstacleClose ? "YES" : "NO");
    }
  }

  // LIGHT LOGIC: either force RED if close, or run the normal cycle
  if (obstacleClose) {
    if (lightState != RED_LIGHT) setLight(RED_LIGHT);
  } else {
    if (now - stateStartMs >= stateDurationMs) {
      if (lightState == RED_LIGHT)        setLight(GREEN_LIGHT);
      else if (lightState == GREEN_LIGHT) setLight(YELLOW_LIGHT);
      else                                setLight(RED_LIGHT);
    }
  }
}