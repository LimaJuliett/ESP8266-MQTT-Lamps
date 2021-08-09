#include<Bounce2.h>
#include<ESP8266WiFi.h>
#include<PubSubClient.h>
#include<FS.h>

//things attached to pins:
const int rpin = 4;
const int gpin = 5;
const int bpin = 15;
const int buttonPin = 14;

//potentiometer rolling average for brightness control:
const int numAverage = 70;
int potentiometer[numAverage];
int brightness = 1024;
int sum;
int i = 0;

//button debounce:
Bounce button = Bounce(buttonPin, 50);

//button press behavior:
long int pressStart;
const long int longPressLength = 1500;

//color stuff:
//labels:                 warm white        red           orange         yellow         green         light blue     blue          purple         off
const int colors[9][3] = {{950, 900, 850}, {1024, 0, 0}, {900, 300, 0}, {750, 450, 0}, {0, 1024, 0}, {0, 550, 450}, {0, 0, 1024}, {330, 0, 880}, {0, 0, 0}};
int displayColor; //the color currently displayed by the lamp
int storedColor = 0;  //the color assigned to the lamp by the MQTT broker
int sendColor = 0;    //the color to be sent to the MQTT broker by the lamp
const int numSteps = 50;  //the number of steps to use when transitioning between colors
const int brightMin = 30; //the minimum brightness settable by the dial
const int pulseHang = 200; //milliseconds to hang at max brightness during a pulse

//WiFi Stuff:
WiFiClientSecure wifiClient;
String myHostname = "MQTT-lamp"; //this will be the hostname visible to the network

//MQTT Stuff:
PubSubClient client(wifiClient);
const IPAddress broker(10,0,0,161);
const int port = 1883;
const char mqttUsername[] = "username";       //set this to this lamp's username in the MQTT broker
const char mqttPassword[] = "password";       //set this to this lamp's password in the MQTT broker
char sendColorChar[10];
const long int delayToSend = 5000; //how long to wait before sending the current selected color to the MQTT broker
bool willSend;        //are we going to send a color in the future?

void setup() {
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  pinMode(rpin, OUTPUT);
  pinMode(gpin, OUTPUT);
  pinMode(bpin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(A0, INPUT);
  delay(10);
  
  transition(8, 0);
  for (int j = 0; j < 8; j++) {
    transition(j, j+1);
  }
  showColor(8);
  
  Serial.begin(115200);
  delay(2000);

  loadcerts();
  
  wifiClient.setInsecure();
  
  connectToWiFi();
  client.setServer(broker, port);
  client.setCallback(callback);
  connectToBroker();
  delay(1000);
}

void loop() {
  //reconnect to WiFi if disconnected:
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  //reconnect to MQTT broker if disconnected:
  if (!client.connected()) {
    connectToBroker();
  }

  client.loop();
  
  handlePotentiometer();   //deals with the potentiometer; sets brightness
  handleButton();          //deals with the button; also will deal with sending colors based on the button inputs
  showColor(displayColor); //shows the proper color
  delay(10);
}

void connectToWiFi() {
  int count  = 0;
  
  Serial.print("Attempting to connect to WiFi with hostname " + myHostname);
  
  WiFi.hostname(myHostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin("", "");   //if you want to manually enter the wifi SSID and password, put them in here. The format is WiFi.begin("SSID", "password");
  
  while (WiFi.status() != WL_CONNECTED && count < 10) { //WiFi is disconnected and we've been trying to connect for less than 5 seconds
    delay(500);
    Serial.print(".");
    count++;
  }

  if (WiFi.status() != WL_CONNECTED) { //the connection failed; use WPS
    Serial.println("Could not connect with stored credentials; requesting WPS setup. Push WPS button on router, then button on lamp to begin WPS setup.");
    
    count = 0;
    
    button.update();
    
    while (button.read()) {
      pulse(0);
      delay(300);
      button.update();
    }
    
    Serial.println("Attempting WPS");
    WiFi.beginWPSConfig();
    delay(5000);
  }

  if (WiFi.status() == WL_CONNECTED) { //WiFi connection succeeded
    pulse(4);
    
    Serial.println();
    Serial.print("Connected to WiFi network ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else { //WiFi connection failed
    pulse(1);   //pulse the RGB LED red

    Serial.println();
    Serial.print("WiFi Config Failed. Reset to try again.");
  }
}

void connectToBroker() {
  while (!client.connected()) {
    Serial.print("Attempting to connect to broker at ");
    Serial.println(broker);
    client.connect("personAsLamp", mqttUsername, mqttPassword);
    if (client.connect("personAsLamp")) {
      pulse(5);
      Serial.println("Connected.");
      client.publish("ConnectionInfo", "Hello world from personAsLamp");
      client.subscribe("ConnectionInfo");
      client.subscribe("personAsLampColor");     //the MQTT topic for this lamp's color
    }
    else {
      delay(1000);
      Serial.print("Failed; state: ");
      Serial.println(client.state());
      Serial.println("Will try again in 5 seconds...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char message[length];
  for (int count = 0; count < length; count++) {
    message[count] = payload[count];
  }
  Serial.print("New message from ");
  Serial.print(topic);
  Serial.print(", contents: ");
  for (int count = 0; count < length; count++) {
    Serial.print(message[count]);
  }
  Serial.println();
  
  if (strcmp(topic, "personAsLampColor") == 0) {
    String messageString = message;
    storedColor = messageString.toInt();

    if (!willSend) { //if we're not currently in the process of sending a color:
      transition(displayColor, storedColor);
      displayColor = storedColor;
    }
    //we don't need to do anything more if we are currently in the process of sending a color; that was done when we set storedColor=messageString.toInt() in line 184;
  }

  if (strcmp(topic, "ConnectionInfo") == 0) { //if the topic is ConnectionInfo, remind the other lamp what their color should be
    client.publish("personBsLampColor", sendColorChar);
    Serial.println("Publishing sendColorChar to personBsLampColor.");
  }
}

void loadcerts() {
  if (SPIFFS.begin()) {
    Serial.println("Filesystem loaded.");
  }

  File cert = SPIFFS.open("/personAslampclient.crt", "r");
  if (cert) {
    Serial.println("Certificate opened.");
  }

  delay(1000);

  if (wifiClient.loadCertificate(cert)) {
    Serial.println("Certificate loaded.");
  }

  File privateKey = SPIFFS.open("/personAslampclient.key", "r");
  if (privateKey) {
    Serial.println("Private key opened.");
  }

  delay(1000);

  if (wifiClient.loadPrivateKey(privateKey)) {
    Serial.println("Private key loaded.");
  }

  File ca = SPIFFS.open("/ca.der", "r");
  if (ca) {
    Serial.println("CA file opened.");
  }
/*    //can't get this part to work for some reason
  delay(1000);

  if (wifiClient.loadCACert(ca)) {
    Serial.println("CA loaded.");
  }
*/
}

void handlePotentiometer() {
  //reset i for rolling average, if necissary:
  if (i == numAverage) {
    i = 0;
  }

  //read the potentiometer and add that to the array of potentiometer values:
  potentiometer[i] = analogRead(A0);

  //sum up all potentiometer values:
  for (int count = 0; count < numAverage; count++) {
    sum += potentiometer[count];
  }
  
  //set brightness equal to the rolling average of the potentiometer (0 - 1024):
  brightness = map(sq(sum/numAverage), 0, 1048576, brightMin, 1024);
  Serial.println(analogRead(A0));

  //reset the sum and incriment i
  sum = 0;
  i++;
}

void handleButton() {
  //update the state of the button
  button.update();

  //when the button is depressed, save the time in milliseconds since program start:
  if (button.fell()) {
    pressStart = millis();
  }

  //turn the display on or off
  if (!button.read() && millis() - pressStart >= longPressLength && !willSend) { //the button's been depressed for longer than longPressLength and we're not working on sending a color
    if (displayColor != 8) { //if the lamp is on, turn it off
      transition(displayColor, 8);
      displayColor = 8;
      button.update();
      delay(300);
    }
    else { //if the lamp is off, turn it on
      transition(8, storedColor);
      displayColor = storedColor;
      button.update();
      delay(300);
    }
  }

  //start the process to send a color
  if (button.rose() && millis() - pressStart < longPressLength && displayColor != 8) { //the button is released AND the press was short AND the lamp is on
    rotateSendColor();
    transition(displayColor, sendColor);
    displayColor = sendColor;
    
    willSend = true;
  }

  //send the selected sendColor if criteria are met
  if (button.read() && millis() - pressStart >= delayToSend && willSend) { //the button is not depressed AND it's been delayToSend since the last press AND we're supposed to send a color
    sprintf(sendColorChar, "%0d", sendColor);
    client.publish("personBsLampColor", sendColorChar);
    Serial.println("Publishing sendColorChar to personBsLampColor.");
    transition(sendColor, 8);
    rotateSendColorBack();
    transition(8, storedColor);
    displayColor = storedColor;
    willSend = false;
  }
}

void rotateSendColor() {
  if (sendColor == 7) {
    sendColor = 0;
  }
  else {
    sendColor++;
  }
}

void rotateSendColorBack() {
  if (sendColor == 0) {
    sendColor = 7;
  }
  else {
    sendColor--;
  }
}

void pulse(int c) { //c is the color to pulse
  transition(8, c);
  delay(pulseHang);
  transition(c, 8);
  showColor(8);
}

void transition(int c1, int c2) {  //create the change that will happen to R, G, and B for each step:
  float colorStep[] = {(colors[c2][0] - colors[c1][0]) / numSteps, (colors[c2][1] - colors[c1][1]) / numSteps, (colors[c2][2] - colors[c1][2]) / numSteps};
  
  for (int count=0; count<=numSteps; count++) {
    analogWrite(rpin, (brightness * (colors[c1][0] + count*colorStep[0]))/1024);
    analogWrite(gpin, (brightness * (colors[c1][1] + count*colorStep[1]))/1024);
    analogWrite(bpin, (brightness * (colors[c1][2] + count*colorStep[2]))/1024);
    delay(10);
  }
}

void showColor(int c) {
  analogWrite(rpin, (brightness * colors[c][0])/1024);
  analogWrite(gpin, (brightness * colors[c][1])/1024);
  analogWrite(bpin, (brightness * colors[c][2])/1024);
}
