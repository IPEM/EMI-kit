#include <M5Unified.h> //using M5Unified platform
#include <Arduino.h>
#include <WiFi.h>
#include "config/DeviceConfig.h"
#include "config/ConfigManager.h"
#include "config/WiFiProvisionerManager.h"
#include "OscSenderManager.h"
#include <button.hpp>
#include "yin/yin_fixed.h"
#include "imu/ImuReader.h" //content from https://github.com/naninunenoy/AxisOrange/blob/master/src/main.cpp
#include "imu/AverageCalc.h"

//IMU settings
#define MAIN_THREAD_SLEEP_IMU 20 // = 50[Hz]
#define TASK_SLEEP_IMU 5 // = 1000[ms] / 200[Hz]
#define MUTEX_DEFAULT_WAIT 500UL
imu::ImuReader* imuReader;
imu::ImuData imuData;
imu::AverageCalcXYZ gyroAve;
bool gyroOffsetInstalled = false;
static SemaphoreHandle_t imuDataMutex = NULL;
int imuSampleCounter = 0;
uint32_t imuSampleCounterTime = 0;
bool buttonAPressed = false;


//device & wifi config
DeviceConfig deviceConfig;
OscSenderManager oscSenderManager;
bool startProvisioning = false;
WiFiProvisionerManager wifiProvisioner;
ConfigManager config;
TaskHandle_t dnsTaskHandle = NULL; // Add task handle for DNS discovery

//appmode
enum ApplicationMode {
  APP_MODE_WIFI_PROVISIONING,
  APP_MODE_CONNECTING,
  APP_MODE_CONNECTION_FAILED,
  APP_MODE_CALIBRATING, //on boot
  APP_MODE_SET_ZERO, //on boot
  APP_MODE_TAP_AND_IMU
};
ApplicationMode appMode = APP_MODE_CONNECTING;



//appmode
enum StreamOptions {
  STREAM_PITCH_JAW_ROLL_TAP,
  STREAM_PITCH,
  STREAM_JAW,
  STREAM_ROLL,
  STREAM_TAP,
  STEAM_OFF,
  COUNT //helper to track size
};

StreamOptions streamMode = STREAM_PITCH_JAW_ROLL_TAP;
StreamOptions nextStreamMode(StreamOptions c) {
    return static_cast<StreamOptions>((static_cast<int>(c) + 1) % static_cast<int>(StreamOptions::COUNT));
}


void enableCalibration();

//TODO 1 2 3 4 + thread priority + setzero gui

//midi config
uint8_t midi_channel  = 0; // MIDI channel for messages
uint8_t midi_tap_note_velocity  = 100;
uint8_t midi_tap_note_number  = 60;
static unsigned long noteOnTime = 0;
static bool noteIsOn = false;

//button config
#define BUTTON_A_PIN 37
#define BUTTON_B_PIN 39
#define BUTTON_DEBOUNCE 10

htcw::int_button<BUTTON_A_PIN,BUTTON_DEBOUNCE> button_a_raw; // declare an interrupt based button
htcw::multi_button button_a(button_a_raw); // declare a multi-button wrapper for it

htcw::int_button<BUTTON_B_PIN,BUTTON_DEBOUNCE> button_b_raw;
htcw::multi_button button_b(button_b_raw);

//gui stuff
void updateGui();
LGFX_Sprite canvas(&M5.Display);
String guiConnecedId = "noconfig"; 
String guiProvisionIp = "noconfig"; 


void setupSerial(){
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect. Needed for native USB
}

//general display
void displayText(String text){
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setCursor(7,7);
  M5.Display.print(text);
}

void setupDisplay(){
  M5.Display.begin();
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  M5.Display.clear(TFT_BLACK);
  M5.Display.setTextSize(2);
}

//Big button under LCD:
// short press sets midi channel
// long press sets zero point 
uint32_t timeButtonAPressed = 0;
void onButtonAPressedChanged(bool released, void* state) {
  if (!released) {
    timeButtonAPressed = millis();
    buttonAPressed = true;
  } else {
    uint32_t timePressed = millis() - timeButtonAPressed;
    buttonAPressed = false;
    if (timePressed < 500) {      
      Serial.print("streamMode change\n"); 
      streamMode = nextStreamMode(streamMode);
    } else {      
      Serial.print("setting zero point\n"); 
      if (appMode == APP_MODE_TAP_AND_IMU) {
        imuReader->setZero();
      }
    }
  }
}


/* Button B long press: start provisioning */
void onButtonBLongPressed(void* state) {
  appMode = APP_MODE_WIFI_PROVISIONING;
  startProvisioning = true;
}


void setupButtons(){
  button_a.initialize();
  button_a.on_pressed_changed(onButtonAPressedChanged);
  //button_a.on_click([](int clicks,void* state) {Serial.print("button a: "); Serial.print(clicks);Serial.println(" clicks");});

  button_b.initialize();
  //button_b.on_pressed_changed(onButtonBPressedChanged);
  //button_b.on_click([](int clicks,void* state) {Serial.print("button b: "); Serial.print(clicks);Serial.println(" clicks");});
  button_b.on_long_click(onButtonBLongPressed);
}

void setupDevice(){
  M5.begin();
}

bool setupWifi(const DeviceConfig &deviceConfig) {
  appMode = APP_MODE_CONNECTING;

  Serial.println("Connecting to WiFi: " + deviceConfig.wifi_ssid);
  WiFi.begin(deviceConfig.wifi_ssid.c_str(), deviceConfig.wifi_password.c_str());
    guiConnecedId = deviceConfig.wifi_ssid.substring(12); //TODO Any other network then OSC-TO-MIDI is fatal

  // Wait for connection with timeout (10 seconds)
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 40) {
    updateGui();

    delay(250);
    Serial.print(".");
    timeout++;
    
    appMode = APP_MODE_CONNECTING;
  }

  if (WiFi.status() == WL_CONNECTED) {
    
    //connected, go in calib mode
    appMode = APP_MODE_CALIBRATING;
    enableCalibration();

    Serial.println("\nWiFi connected successfully");
    Serial.println("IP Address: " + WiFi.localIP().toString());
    Serial.println("Gateway: " + WiFi.gatewayIP().toString());

    return true;
  } else {
    Serial.println("\nFailed to connect to WiFi");
    appMode = APP_MODE_CONNECTION_FAILED;
    //TODO HANDLE THIS
    return false;
  }
}

void provisionWifi(){
   wifiProvisioner.startProvisioning([](const DeviceConfig &provisionedConfig)
                                      {
    if (provisionedConfig.hasValidWiFiConfig()) {
        Serial.println("WiFi provisioning completed successfully");
        
        // Save the provisioned configuration
        config.setConfig(provisionedConfig);
        
        // Connect to WiFi with new configuration
         setupWifi(provisionedConfig);
    } else {
        Serial.println("WiFi provisioning failed - received empty config");
    } });
}


void dnsDiscoveryTask(void *parameter) {
  while (true) {
    // Wait 5 seconds between discoveries (0.2Hz)
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Perform DNS discovery
    oscSenderManager.discoverReceivers();
    oscSenderManager.cleanupOldReceivers();
  }
}

void enableCalibration() {
    appMode = APP_MODE_CALIBRATING;
    gyroOffsetInstalled = false;
    imuReader->writeGyroOffset(0.f, 0.f, 0.f);
    gyroAve.reset();                     
    M5.Imu.clearOffsetData(); //fixes bug when moing on boot
    M5.Imu.setCalibration(255, 255, 255);
}

/* Main thread pulling imu data, calibration &  calculating AHRS*/
void ImuLoop(void* arg) {
  while (1) {
    int32_t entryTime = millis();


    if (appMode == APP_MODE_CALIBRATING || appMode == APP_MODE_SET_ZERO || appMode == APP_MODE_TAP_AND_IMU ) {

        if (xSemaphoreTake(imuDataMutex, MUTEX_DEFAULT_WAIT) == pdTRUE) {
          imuReader->update();
          imuReader->read(imuData);

          if (!gyroOffsetInstalled) {
            
            if (!gyroAve.push(imuData.gyro[0], imuData.gyro[1], imuData.gyro[2])) {
              //this runs when the ave list is full (ca 5s)
              float x = gyroAve.averageX();
              float y = gyroAve.averageY();
              float z = gyroAve.averageZ();

              if (abs(x) > 0.02 || abs(y) > 0.02 || abs(z) > 0.02) {
                Serial.printf("AHRS calibration Failed. \t\t  Offset: %.5f, %.5f, %.5f\tRedoing...\n", x, y, z);
                gyroAve.reset();
                M5.Imu.clearOffsetData();
              } else {
                M5.Imu.setCalibration(0,0,0);

                //succesfull calib
                //imuReader->writeGyroOffset(x, y, z);
                float offset[] = {x, y, z};
                Serial.printf("AHRS calibration done.  Offset: %.5f, %.5f, %.5f\n", x, y, z);
                gyroOffsetInstalled = true;
                appMode = APP_MODE_TAP_AND_IMU;
              }
            }
          }
          
          imuSampleCounter++;
          if (entryTime - imuSampleCounterTime  > 1000) {
            imuSampleCounterTime = entryTime;
           Serial.printf("AHRS: Pitch=%.1f° Jaw=%.1f° Roll=%.1f° SR: %i | \n", imuData.orientation[0], imuData.orientation[1], imuData.orientation[2], imuSampleCounter);

            imuSampleCounter = 0;
          }
        
        }
        xSemaphoreGive(imuDataMutex);
    }


    // idle
    int32_t sleep = TASK_SLEEP_IMU - (millis() - entryTime);
    vTaskDelay((sleep > 0) ? sleep : 0);
  }
}

//creates the IMU thread
void setupIMU() {
  imuReader = new imu::ImuReader(M5.Imu);
  imuReader->initialize(); 
  imuDataMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(ImuLoop, "IMUtask", 8192UL, NULL, /* 1 low - 24 high, was 2 */ 20, NULL, 0);
}


void setup() {
  setupSerial();
  setupDevice();
  setupDisplay();
  setupButtons();
  setupIMU();

 
  config.begin();

  if (config.hasValidConfig()){
    deviceConfig = config.getConfig();    
    //displayText("Config loaded\n" + deviceConfig.wifi_ssid);
    setupWifi(deviceConfig);
  } else {
    provisionWifi();
  }


  oscSenderManager.begin();

  // Create DNS discovery task on core 0 (background)
  xTaskCreatePinnedToCore(
    dnsDiscoveryTask,           // Task function
    "DNSDiscovery",             // Task name
    8192,                       // Stack size (bytes)
    NULL,                       // Parameter passed to task
    1,                          // Task priority (low priority)
    &dnsTaskHandle,             // Task handle
    0                           // Core ID (0 = core 0, 1 = core 1)
  );
}


long startPress = 0;

void handleButtons() {
  button_a.update();
  button_b.update();
}


void sendMidiMessage(uint8_t midi_channel, uint8_t midi_command, uint8_t data1, uint8_t data2) {
  int midiMessage[3] = {midi_command + midi_channel, data1, data2};
  oscSenderManager.sendIntArrayToAll("/midi", midiMessage, 3);
}

void sendCCValue(uint8_t midi_channel, uint8_t midi_cc_number, uint8_t midi_cc_value) {
  sendMidiMessage(midi_channel, 0xB0, midi_cc_number, midi_cc_value);
}

void sendNoteOn(uint8_t midi_channel, uint8_t note, uint8_t velocity) {
  sendMidiMessage(midi_channel, 0x90, note, velocity);
}

void sendNoteOff(uint8_t midi_channel, uint8_t note, uint8_t velocity) {
  sendMidiMessage(midi_channel, 0x80, note, velocity);
}


void sendMidiImuData(){

    if (streamMode == STREAM_TAP || streamMode == STREAM_PITCH_JAW_ROLL_TAP) {
      float accel_magnitude = sqrt(imuData.acc[0] * imuData.acc[0] + imuData.acc[1]*imuData.acc[1] + imuData.acc[2] * imuData.acc[2]);
      if (accel_magnitude > 3.0 && !noteIsOn) {
        Serial.println("Accel magnitude: " + String(accel_magnitude));
        // Send note on
        sendNoteOn(midi_channel, midi_tap_note_number, midi_tap_note_velocity); // Note On, Middle C, velocity 100
        noteOnTime = millis();
        noteIsOn = true;
      }
      if (noteIsOn && (millis() - noteOnTime >= 100)) {
        // Send note off after 500ms
        sendNoteOff(midi_channel, midi_tap_note_number, 0);
        // Todo changing the channel whilst in note on status could result in hanging note
        noteIsOn = false;
      }  
    }

    
    if (streamMode == STREAM_PITCH || streamMode == STREAM_PITCH_JAW_ROLL_TAP) { 
      //send midi pitch bend message and map theta to pitch bend range
      int theta = map(imuData.orientation[0], -90, 90, 0, 127);
      theta = constrain(theta, 0, 127);
      sendCCValue(midi_channel, 80, theta); // CC 80 is a custom CC for pitch bend in this case
    }
    if (streamMode == STREAM_JAW || streamMode == STREAM_PITCH_JAW_ROLL_TAP) {
      int phi = map(imuData.orientation[1], -90, 90, 0, 126);
      phi = constrain(phi, 0, 127);
      sendCCValue(midi_channel, 81, phi); // CC 81 is a custom CC for jaw movement in this case
    }
    if (streamMode == STREAM_ROLL || streamMode == STREAM_PITCH_JAW_ROLL_TAP) {     
      int psi = map(imuData.orientation[2], -90, 90, 0, 127);
      psi = constrain(psi, 0, 127);
      sendCCValue(midi_channel, 82, psi); // CC 82 is a custom CC for roll movement in this case
    }

}

uint32_t guiUpdated = 0;
void updateGui(bool force) {

  uint32_t now = millis();

  if (force || now -guiUpdated > 100) {
    guiUpdated = now;

    canvas.clear(TFT_BLACK);  // clear off-screen buffer
    canvas.setTextSize(2);
    canvas.setTextColor(WHITE);

    
     if (appMode == APP_MODE_CONNECTING) {
       canvas.drawCenterString("Connecting", M5.Display.width() / 2, 10);    

        //receiver ID based on SSID
        canvas.drawCenterString("to recvr", M5.Display.width() / 2, 40);
        canvas.setTextSize(4);
        canvas.drawCenterString(guiConnecedId, M5.Display.width() / 2, 75);
        canvas.setTextSize(2);
    }
    else if (appMode == APP_MODE_CALIBRATING) {
        canvas.drawCenterString("Paired to", M5.Display.width() / 2, 10);    
        //receiver ID based on SSID
        canvas.drawCenterString("Rec " + guiConnecedId, M5.Display.width() / 2, 30);
        canvas.drawLine(0,50,M5.Display.width(), 50);
        canvas.setTextSize(2);
        canvas.drawCenterString("CALIBRATING", M5.Display.width() / 2, 80);   
        canvas.setTextSize(3);
        canvas.drawCenterString("LAY", M5.Display.width() / 2, 130);   
        canvas.drawCenterString("FLAT", M5.Display.width() / 2, 160);      
    }
    else if (appMode == APP_MODE_CONNECTION_FAILED) {
       canvas.drawCenterString("Connection", M5.Display.width() / 2, 10);    

        //receiver ID based on SSID
        canvas.drawCenterString("failed", M5.Display.width() / 2, 40);
        canvas.setTextSize(4);
        canvas.drawCenterString(guiConnecedId, M5.Display.width() / 2, 75);
        canvas.setTextSize(2);

    }
    else if (appMode == APP_MODE_WIFI_PROVISIONING) {      
       canvas.drawCenterString("Provisioning", M5.Display.width() / 2, 10);    
       canvas.drawCenterString("WiFi", M5.Display.width() / 2, 40);
       canvas.drawCenterString(guiConnecedId, M5.Display.width() / 2, 75);
    }
    else if (appMode == APP_MODE_TAP_AND_IMU) {
      canvas.drawCenterString("Paired to", M5.Display.width() / 2, 10);    
      canvas.drawCenterString("Rec " + guiConnecedId, M5.Display.width() / 2, 30);
      canvas.drawCenterString("Midi Ch 1", M5.Display.width() / 2, 50);
      canvas.drawLine(0,75,M5.Display.width(), 75);

      

    //setzero function GUI
    if ( buttonAPressed && millis() - timeButtonAPressed > 500) {      
        canvas.setTextSize(4);
        canvas.drawCenterString("Set", M5.Display.width() / 2, 100);
        canvas.drawCenterString("Zero", M5.Display.width() / 2, 140);
        canvas.setTextSize(2);
      
    } else {
      //default gui
      if (streamMode == STEAM_OFF) canvas.drawString("Off", 7, 90);
      if (streamMode == STREAM_PITCH || streamMode == STREAM_PITCH_JAW_ROLL_TAP)  canvas.drawString("CC80 pitch", 7, 90);
      if (streamMode == STREAM_JAW || streamMode == STREAM_PITCH_JAW_ROLL_TAP) canvas.drawString("CC81 jaw", 7, 110);
      if (streamMode == STREAM_ROLL || streamMode == STREAM_PITCH_JAW_ROLL_TAP) canvas.drawString("CC82 roll", 7, 130);
      if (streamMode == STREAM_TAP || streamMode == STREAM_PITCH_JAW_ROLL_TAP) canvas.drawString("C3 = tap", 7, 150);
      canvas.drawLine(0,175,M5.Display.width(), 175);


    }
     
      
        canvas.setTextSize(4);
      if (streamMode == STREAM_PITCH) canvas.drawCenterString(imuData.getPitchString(), M5.Display.width() / 2, 190);
      if (streamMode == STREAM_JAW) canvas.drawCenterString(imuData.getJawString(), M5.Display.width() / 2, 190);
      if (streamMode == STREAM_ROLL) canvas.drawCenterString(imuData.getRollString(), M5.Display.width() / 2, 190);
        canvas.setTextSize(2);


    } else {

    }
    
    canvas.setTextSize(1);
    char buf[20];    std::sprintf(buf, "Battery: %i%%", (int)M5.Power.getBatteryLevel());
    
    canvas.drawString(String(buf), M5.Display.width() - 80, 225);

    canvas.pushSprite(0, 0);  // push once per frame
    
  }
}


void updateGui() {
  updateGui(false);
}


void loop(){  
  
  int32_t entryTime = millis();
  updateGui();
  
  //this also checks if provisioning is started from the button
  if(appMode == APP_MODE_WIFI_PROVISIONING){
    if(startProvisioning){
      startProvisioning = false;
      updateGui(true);
      delay(8);
      provisionWifi();
    }
  }

  if(appMode == APP_MODE_TAP_AND_IMU){
    sendMidiImuData();
  }

  handleButtons();

  

  
  int32_t sleep = MAIN_THREAD_SLEEP_IMU - (millis() - entryTime);
    delay((sleep > 0) ? sleep : 0);  //50 hz

}


