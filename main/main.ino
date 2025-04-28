// NOTE -> THIS IS MY MAIN COPY OF CODE USE ON OUR ESP32 TTGO T-DISPLAY BOARD TO RUN MY TRAINED MODEL 04-04-25 at 09 am  This Model Uses Both MFCC and MFE
// 03-14-25 coding for vibration motor on/off on for 3sec?? or on/off for 0.5sec for 3 times then stop
//Main code
// test to recieve message thru Lora from Raspi


#define EIDSP_QUANTIZE_FILTERBANK   0

#include <Home_Ear_2025_inferencing.h> // Change the impulse library from <Home-Ear-TinyML_inferencing.h> to <Home-Ear-Final-TinyML_inferencing.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include <TFT_eSPI.h> // Bodmer
#include <WiFi.h>
#include <esp_bt.h> 
#include <SPI.h>  // SPI library for communication
#include <LoRa.h> // Sandeep Mistry Library

#include <Arduino.h>
#define BATTERY_ADC_PIN 35  // Battery voltage ADC pin
#define USB_DETECT_PIN 14   // Pin to check if USB is plugged in
#define MIN_BATTERY_VOLTAGE 3.2  // Voltage when battery is empty
#define MAX_BATTERY_VOLTAGE 4.2  // Voltage when battery is full

//Below Settings for TFT Screen
#define GRAPH_WIDTH  240
#define GRAPH_HEIGHT 135
TFT_eSPI tft = TFT_eSPI();

//LoRa module connections
#define PIN_LORA_MISO   33  // MISO
#define PIN_LORA_MOSI   32  // MOSI
#define PIN_LORA_SCK    21  // SCK
#define PIN_LORA_SS     15  // CS
#define PIN_LORA_RST    12  // Reset
#define RST             12
#define PIN_LORA_DIO0   2   // Interrupt
bool inLoRaMode = false;  // Flag to track LoRa mode


/** Audio buffers, pointers and selectors */
typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static signed short sampleBuffer[sample_buffer_size];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool record_status = true;


String lastClassifiedSound = "";  // Store last detected sound
/**
 * @brief      Arduino setup function
 */
void setup()
{
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    Serial.begin(115200);
    disableWiFiAndBT(); // call disable wifi/bt function
    pinMode(USB_DETECT_PIN, INPUT); // Battery is Charging Detect

    // comment out the below line to cancel the wait for USB connection (needed for native USB)
    while (!Serial);
    BootLogo();
    bootconf();
    Lorastart();

    tft.fillScreen(TFT_BLACK);  
    ei_printf("\nStarting continious inference in 2 seconds...\n");
    ei_sleep(2000);

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        tft.println("ERR : Could not allocate audio buffer!");
        delay(500);
        tft.println(String(EI_CLASSIFIER_RAW_SAMPLE_COUNT));
        delay(500);
        contact();
        ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }
    ei_printf("Recording...\n");
}

void BootLogo(){
    char letters[] = {'H', 'O', 'M', 'E'};
    char letters1[] = {'-','E', 'A', 'R'};
    uint16_t colors[] = {TFT_RED, TFT_BLUE, TFT_GREEN, TFT_YELLOW};
    tft.setTextSize(4);  

    int textWidth = 24;  // Approximate width of each letter
    int textHeight = 30; // Approximate height of each letter
    int centerX = (tft.width() - textWidth) / 2;  // Center X position
    int centerY = (tft.height() - textHeight) / 2; // Center Y position

    for (int i = 0; i < 4; i++) {
        tft.setTextColor(colors[i], TFT_BLACK);  // Set text color, keep background black
        tft.setCursor(centerX, centerY);
        tft.print(letters[i]); // Print letter
        delay(500); // Small delay
        // Clear only the letter area before printing the next one
        tft.fillRect(centerX, centerY, textWidth, textHeight, TFT_BLACK);
    }
    for (int i = 0; i < 4; i++) {
        tft.setTextColor(colors[i], TFT_BLACK);  // Set text color, keep background black
        tft.setCursor(centerX, centerY);
        tft.print(letters1[i]); // Print letter
        delay(500); // Small delay
        // Clear only the letter area before printing the next one
        tft.fillRect(centerX, centerY, textWidth, textHeight, TFT_BLACK);
    }

    delay(1000);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor((tft.width() - 120) / 2, centerY);
    tft.print("HOME EAR");

    delay(1000);
    tft.fillScreen(TFT_BLACK);
    delay(1000);
}

void bootconf(){
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(30, 10);
    tft.setTextSize(2);  // Adjust text size
    tft.println("Home Ear Watch");
    Serial.println("Edge Impulse Inferencing");

    tft.setTextSize(1);
    // summary of inferencing settings (from model_metadata.h)
    tft.println("Powering On");
    delay(500);
    tft.println("Initializing Configured Settings.");
    delay(500);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.println("\tBluetooth : Disabled");
      delay(500);
      tft.println("\tWifi configuration : Disabled");
      delay(500);
      tft.println("\tTFT LCD Display : ok");
      delay(500);
      tft.println("\tEsp32 Board     : ok");
      delay(500);
      tft.println("\tinmp41 Status   : ok");
      delay(500);
      tft.println("\tsx1278 Status   : ok");
      delay(500);
      tft.println("\tCoin T motor    : ok");
      delay(500);
      tft.println("\tSystem Status   : ok");
      delay(500);
      tft.println("\tProgram Check   : ok");
      delay(500);
      tft.println("----------------------------------------");
      delay(500);
      tft.println("----------------------------------------");
      delay(500);
      tft.println("----------------------------------------");
      delay(500);

    tft.fillScreen(TFT_BLACK);
    delay(500);
    tft.fillScreen(TFT_WHITE);
    delay(500);
    tft.fillScreen(TFT_RED);
    delay(500);
    tft.fillScreen(TFT_BLUE);
    delay(500);
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 10);
    delay(500);

    tft.println("Inferencing settings:");
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");

    tft.print("\tInterval: ");
    tft.println(String((float)EI_CLASSIFIER_INTERVAL_MS) + " ms.");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    delay(500);

    tft.print("\tFrame size: ");
    tft.println(String(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE));
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    delay(500);

    tft.print("\tSample length: ");
    tft.println(String(EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16) + " ms.");
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    delay(500);

    tft.print("\tNo. of classes: ");
    tft.println(String(sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0])));
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
    delay(500);

    tft.println(" ");
    delay(500);

    tft.println("EI_CLASSIFIER_CONFIGURATION: ");
    delay(500);

    tft.print("\tINTERVAL in MS: ");
    tft.print(String(EI_CLASSIFIER_INTERVAL_MS));
    tft.println(" ms");
    delay(500);
    Serial.print("EI_CLASSIFIER_INTERVAL_MS: ");
      Serial.println(EI_CLASSIFIER_INTERVAL_MS);
    delay(500);

    tft.print("\tSLICES PER MODEL WINDOW: ");
    tft.print(String(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW));
    tft.println(" Slice");
    Serial.print("EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW: ");
      Serial.println(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
    delay(500);

    tft.print("\tRaw Sample Count: ");
    tft.println(String(EI_CLASSIFIER_RAW_SAMPLE_COUNT));
    Serial.print("EI_CLASSIFIER_RAW_SAMPLE_COUNT: ");
    Serial.println(EI_CLASSIFIER_RAW_SAMPLE_COUNT);   // Raw Sample Count Largest working so far is 52920 ; 1200ms window and 800ms window stride

    tft.println(" ");
    delay(500);

    tft.println("Checking Connection from Main Device...");
    delay(5000);

    if (LoRa.parsePacket()) {  // Check if a packet is received
        Serial.println("LoRa Signal Detected, Entering LoRa Mode...");
        tft.println("Main Device Detected.");
        delay(500);
        tft.println("Establishing Connection...");
        delay(500);
        handleLoRa();  // Enter LoRa mode
    }
    // Other tasks (e.g., handling sensors, motors, etc.)
    Serial.println("No LoRa signal detected, running main loop tasks...");
    tft.println("Main Device not detected.");
    delay(2000);  // Simulating other tasks

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 10);
    tft.println("Starting continuous inference in: ");
    delay(500);
    tft.println("-----> 3 seconds");
    delay(1000);
    tft.println("-----> 2 seconds");
    delay(1000);
    tft.println("-----> 1 second");
    delay(1000);
}

//Disable wifi and BT
void disableWiFiAndBT() {
    WiFi.disconnect(true);   // Disconnect from WiFi
    WiFi.mode(WIFI_OFF);     // Turn off WiFi
    btStop();                // Stop Bluetooth
}

void displayMessage(String message) {
    //drawNeonBorder();
    drawHighTechBorder();
    tft.setTextSize(2);          // Adjust text size
    tft.setTextColor(TFT_BLUE);
    tft.setCursor(80, 20);
    tft.print("Home EAR");

    if (message == "Nothing") {
        updateMiddleScreen(TFT_BLACK); // Change middle part while keeping border
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(80, 60);
        tft.print("No Event");
        tft.setCursor(80, 80);
        tft.print("Detected");
        updateDisplay();

    } else if (message == "Mild") {
        updateMiddleScreen(TFT_ORANGE); // Mild warning color
        tft.setTextColor(TFT_BLACK);
        tft.setCursor(80, 60);
        tft.print("Warning!");
        tft.setCursor(50, 80);
        tft.print("Check Around");
        updateDisplay();

    } else if (message == "Extreme") {  // Fixed missing parenthesis
        updateMiddleScreen(TFT_RED); // Extreme danger color
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(80, 60);
        tft.print("DANGER!");
        tft.setCursor(50, 80);
        tft.print("Take Action!");
        updateDisplay();

    } else {  
        updateMiddleScreen(TFT_BLACK); // Change middle part while keeping border
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(80, 60);
        tft.print("No Event");
        tft.setCursor(80, 80);
        tft.print("Detected");
    }
}

void LoraMessage(String loramessage) {
    drawNeonVioletBorder();
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM); // Align text to top-left
    tft.drawString("LoRa", 20, 15); //test Lora display text
    tft.setTextSize(2);
    tft.setCursor(80, 20);
    tft.print("Home EAR");
    

    // Split the Lora Message to fit the TFT Display
    int spaceIndex = loramessage.indexOf(" ");
    String firstWord, secondWord;

    if (spaceIndex != -1) { // Check if space exists
        firstWord = loramessage.substring(0, spaceIndex);
        secondWord = loramessage.substring(spaceIndex + 1);
    } else {
        firstWord = loramessage; // Use the full message if no space
        secondWord = "";         // Leave secondWord empty
    }

    if (loramessage == "FIRE ALARM" || loramessage == "EMERGENCY VEHICLE" || loramessage == "GUN SHOTS") {
        updateMiddleScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(80, 60);
        tft.print(firstWord);
        tft.setCursor(80, 80);
        tft.print(secondWord);
        updateDisplay();

    } else if (loramessage == "EARTHQUAKE DRILL" || loramessage == "DOOR KNOCK" || loramessage == "DOG BARK") {
        updateMiddleScreen(TFT_ORANGE);
        tft.setTextColor(TFT_BLACK);
        tft.setCursor(80, 60);
        tft.print(firstWord);  // Fixed variable name
        tft.setCursor(80, 80);
        tft.print(secondWord);
        updateDisplay();

    } else if (loramessage == "BABY CRYING") {
        updateMiddleScreen(TFT_BLUE);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(80, 60);
        tft.print("Baby");
        tft.setCursor(80, 80);
        tft.print("Crying");
        updateDisplay();

    } else {
        updateMiddleScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(80, 60);
        tft.print("No Event");
        tft.setCursor(80, 80);
        tft.print("Detected");
        updateDisplay();  // Ensure screen updates
    }
    delay(5000);
        updateMiddleScreen(TFT_BLACK); // Change middle part while keeping border
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);
        tft.setCursor(80, 60);
        tft.print("No Event");
        tft.setCursor(80, 80);
        tft.print("Detected");
}


// Battery Check if Charging
bool isCharging() {
    return digitalRead(USB_DETECT_PIN) == HIGH;
}
void displayChargingStatus(bool charging) {
    tft.setTextSize(1);
    tft.setTextDatum(TR_DATUM); // Align text to top-right
    tft.setTextColor(charging ? TFT_GREEN : TFT_WHITE, TFT_BLACK); // Green if charging, white if not
    tft.drawString("100%", tft.width() - 12, 15);
    //tft.setTextColor(TFT_WHITE, TFT_BLACK); // Reset text color


}
void updateDisplay() {
    bool charging = isCharging(); // Detect charging state
    displayChargingStatus(charging); // Call function to display status
}

// Draw border Neon Styles
void drawNeonBorder() {
    tft.fillScreen(TFT_BLACK); // Clear screen first
    // Draw multiple rectangles to simulate a glow effect
    uint16_t neonBlue = tft.color565(0, 0, 255); // Bright blue color
    for (int i = 0; i < 5; i++) { // Multiple lines for -layered effect
        tft.drawRoundRect(5 + i, 5 + i, 230 - (i * 2), 125 - (i * 2), 8, neonBlue);
        delay(50); // Small delay for effect
    }
}
void drawNeonVioletBorder() {
    tft.fillScreen(TFT_BLACK); // Clear screen first
    // Define neon violet color (RGB: 148, 0, 211)
    uint16_t neonViolet = tft.color565(148, 0, 211); // Bright violet color
    // Draw multiple rectangles to simulate a glow effect
    for (int i = 0; i < 5; i++) { // Multiple lines for layered effect
        tft.drawRoundRect(5 + i, 5 + i, 230 - (i * 2), 125 - (i * 2), 8, neonViolet);
        delay(100); // Small delay for effect
    }
}
void drawCyberpunkBorder() {
    tft.fillScreen(TFT_BLACK); // Clear screen first
    uint16_t neonCyan = tft.color565(0, 255, 255);   // Neon Cyan
    uint16_t neonPink = tft.color565(255, 20, 147);  // Neon Pink

    for (int i = 0; i < 5; i++) { 
        uint16_t color = (i % 2 == 0) ? neonCyan : neonPink; // Alternate colors
        tft.drawRoundRect(5 + i, 5 + i, 230 - (i * 2), 125 - (i * 2), 8, color);
        delay(50); // Small delay for glowing effect
    }
}
void drawHighTechBorder() {
    tft.fillScreen(TFT_BLACK); // Clear screen first
    uint16_t neonGreen = tft.color565(57, 255, 20); // Neon Green
    uint16_t neonBlue = tft.color565(0, 0, 255); // Neon Blue

    for (int i = 0; i < 5; i++) { 
        tft.drawRoundRect(5 + i, 5 + i, 230 - (i * 2), 125 - (i * 2), 8, (i % 2 == 0) ? neonGreen : neonBlue);
        delay(50);
    }
}
void drawSciFiBorder() {
    tft.fillScreen(TFT_BLACK); // Clear screen first
    uint16_t neonPurple = tft.color565(148, 0, 211); // Neon Purple
    uint16_t neonYellow = tft.color565(255, 255, 0); // Neon Yellow

    for (int i = 0; i < 5; i++) { 
        tft.drawRoundRect(5 + i, 5 + i, 230 - (i * 2), 125 - (i * 2), 8, (i % 2 == 0) ? neonPurple : neonYellow);
        delay(50);
    }
}
void drawDangerAlertBorder() {
    tft.fillScreen(TFT_BLACK); // Clear screen first
    uint16_t neonOrange = tft.color565(255, 165, 0); // Neon Orange
    uint16_t neonRed = tft.color565(255, 0, 0); // Neon Red

    for (int i = 0; i < 5; i++) { 
        tft.drawRoundRect(5 + i, 5 + i, 230 - (i * 2), 125 - (i * 2), 8, (i % 2 == 0) ? neonOrange : neonRed);
        delay(50);
    }
}


// Update only the middle of screen leaving the neon border
void updateMiddleScreen(uint16_t color) {
    // Fill only the middle part while keeping the border
    tft.fillRect(40, 50, 160, 50, color); // Adjust the dimensions to fit inside the border
}
// Lora Setup and Handling
void Lorastart() {
    Serial.println("Initializing LoRa...");

    // LoRa Manual Reset
    pinMode(RST, OUTPUT);
    digitalWrite(RST, LOW);
    delay(10);
    digitalWrite(RST, HIGH);
    delay(100);  // Increased delay for stability

    // Initialize SPI with custom pins (ESP32-specific)
    SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_SS);

    // Set LoRa pins
    LoRa.setPins(PIN_LORA_SS, PIN_LORA_RST, PIN_LORA_DIO0);

    // Check SPI Communication with LoRa Module
    SPI.beginTransaction(SPISettings(8E6, MSBFIRST, SPI_MODE0));
    digitalWrite(SS, LOW);
    uint8_t version = SPI.transfer(0x42);  // Read RegVersion (0x42)
    digitalWrite(SS, HIGH);
    SPI.endTransaction();

    if (version == 0x12) {
        Serial.println("✅ LoRa module detected successfully!");
    } else {
        Serial.println("⚠️ WARNING: LoRa module not detected! Check wiring and power.");
        // Do not halt execution, allow retry
    }

    // Initialize LoRa at 433 MHz
    if (!LoRa.begin(433E6)) {
        Serial.println("❌ ERROR: LoRa initialization failed!");
        tft.fillScreen(TFT_BLACK);
        contact();
        while (1);  // Halt execution
    }

    // LoRa Configuration - Must match the transmitter
    // OLD CONFIG THAT MATCH RASPI
    /*
    LoRa.setSpreadingFactor(10);   // SF10 (Range vs. Speed balance)
    LoRa.setSignalBandwidth(125E3); // 125 kHz Bandwidth
    LoRa.setCodingRate4(5);        // Error correction 4/5
    LoRa.setSyncWord(0x12);        // Custom sync word for private network
    */

    // NEW CONFIG ( for longer distance ) MUST MATCH WITH RASPI 
    
    LoRa.setSpreadingFactor(12);
    LoRa.setSignalBandwidth(62.5E3);
    LoRa.setCodingRate4(8);
    LoRa.setSyncWord(0x12);
    

    Serial.println("✅ LoRa Receiver Ready!");
}
void handleLoRa() {
    inLoRaMode = true;
    Serial.println("Listening for LoRa packets...");

    unsigned long lastReceivedTime = millis();  // For Tracking last received message time
    while (inLoRaMode) {
        if (LoRa.parsePacket()) { 
            String receivedMessage = "";
            while (LoRa.available()) {
                receivedMessage += (char)LoRa.read();
            }
            int rssi = LoRa.packetRssi();
            String signalStrength;

                if (rssi > -50) signalStrength = "Excellent";
                else if (rssi > -70) signalStrength = "Good";
                else if (rssi > -90) signalStrength = "Fair";
                else signalStrength = "Weak";

            Serial.print("Received: ");
            Serial.println(receivedMessage);
            Serial.print(" | RSSI: ");
            Serial.println(rssi);
            Serial.print(" | Signal Strength: ");
            Serial.println(signalStrength);  
            LoraMessage(receivedMessage); // send message to lora display handler
            lastReceivedTime = millis();  // Reset timer since a message was received
            //add delay 2 to 10 sec ??
        }

        // If no new message is received for 3 min, exit LoRa mode
        if (millis() - lastReceivedTime > 30000) {  
            Serial.println("No LoRa signal for 3 min, returning to main Program...");
            inLoRaMode = false;
            tft.fillRect(20, 15, 50, 16, TFT_BLACK);  // Clear area where text was drawn LORA MODE OFF
        }
        delay(500); 
    }
}

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void loop()
{
  updateDisplay();
  String Classified_Sound = "Nothing"; // Set to Nothing . Default is Nothing / Idle
  String Detected_Sound = "";
  float highestScore = 0.0;       // Variable to track the highest probability

    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        contact();
        return;
    }

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: ", result.classification[ix].label);
        ei_printf_float(result.classification[ix].value);
        ei_printf("\n");

        if (result.classification[ix].value > highestScore) {
            highestScore = result.classification[ix].value;
            Classified_Sound = result.classification[ix].label;
        }
    }
/**
    if (highestScore > 0.9) {
        displayMessage(Classified_Sound);  // Function to display on TFT
    } else {
        displayMessage("Nothing");  // Optional: Handle low-confidence cases
    }
  */
   if (highestScore <= 0.9) {
        Classified_Sound = "Nothing";  // Handle low-confidence cases
    }
    

    // **Only update the display if the detected sound has changed**
    if (Classified_Sound != lastClassifiedSound) {
        if (Classified_Sound == "Car Horn"){
          Detected_Sound = "Extreme";

        }else if(Classified_Sound == "E-Vehicle" || Classified_Sound == "Dog Bark") {
          Detected_Sound = "Mild";
        }else {
          Detected_Sound = "Nothing";
        }

        displayMessage(Detected_Sound);
        lastClassifiedSound = Classified_Sound;  // Update the last detected sound
    }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: ");
    ei_printf_float(result.anomaly);
    ei_printf("\n");
#endif

    delay(5000); // dinagdag ko for now delay for 10sec

  //Lora Checking for Connection
  
    if (LoRa.parsePacket()) {  // Check if a packet is received
        Serial.println("LoRa Signal Detected, Entering LoRa Mode...");
        
        //tft.fillRect(0, 2, 50, 16, TFT_BLACK); // Clear area where text was drawn
        handleLoRa();  // Enter LoRa mode
    }
    // Other tasks (e.g., handling sensors, motors, etc.)
    Serial.println("No LoRa signal detected, running main loop tasks...");
    delay(1000);  // Simulating other tasks
    
    
}



static void audio_inference_callback(uint32_t n_bytes)
{
    for(int i = 0; i < n_bytes>>1; i++) {
        inference.buffer[inference.buf_count++] = sampleBuffer[i];

        if(inference.buf_count >= inference.n_samples) {
          inference.buf_count = 0;
          inference.buf_ready = 1;
        }
    }
}

static void capture_samples(void* arg) {

  const int32_t i2s_bytes_to_read = (uint32_t)arg;
  size_t bytes_read = i2s_bytes_to_read;

  while (record_status) {

    /* read data at once from i2s */
    i2s_read((i2s_port_t)0, (void*)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

    if (bytes_read <= 0) {
      ei_printf("Error in I2S read : %d", bytes_read);
    }
    else {
        if (bytes_read < i2s_bytes_to_read) {
        ei_printf("Partial I2S read");
        }

        // scale the data (otherwise the sound is too quiet)
        for (int x = 0; x < i2s_bytes_to_read/2; x++) {
            sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
        }

        if (record_status) {
            audio_inference_callback(i2s_bytes_to_read);
        }
        else {
            break;
        }
    }
  }
  vTaskDelete(NULL);
}

/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */
static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));

    if(inference.buffer == NULL) {
        return false;
    }

    inference.buf_count  = 0;
    inference.n_samples  = n_samples;
    inference.buf_ready  = 0;

    if (i2s_init(EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start I2S!");
    }

    ei_sleep(100);

    record_status = true;

    xTaskCreate(capture_samples, "CaptureSamples", 1024 * 32, (void*)sample_buffer_size, 10, NULL);

    return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */
static bool microphone_inference_record(void)
{
    bool ret = true;

    while (inference.buf_ready == 0) {
        delay(10);
    }

    inference.buf_ready = 0;
    return ret;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);

    return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void)
{
    i2s_deinit();
    ei_free(inference.buffer);
}

void contact()
{
    tft.setCursor((tft.width() - 120) / 2, 40); 
    tft.println(EI_CLASSIFIER_PROJECT_NAME);
    tft.println(EI_CLASSIFIER_PROJECT_OWNER);
    tft.println("Please Contact Developer!");
    tft.println("+63-928 198 2978");

}

static int i2s_init(uint32_t sampling_rate) {
  // Start listening for audio: MONO @ 8/16KHz
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX ),
      .sample_rate = sampling_rate,
      .bits_per_sample = (i2s_bits_per_sample_t)16,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 512,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = -1,
  };
  i2s_pin_config_t pin_config = {
      .bck_io_num = 27,    // IIS_SCLK
      .ws_io_num = 26,     // IIS_LCLK
      .data_out_num = -1,  // IIS_DSIN
      .data_in_num = 25,   // IIS_DOUT
  };
  esp_err_t ret = 0;

  ret = i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_driver_install");
  }

  ret = i2s_set_pin((i2s_port_t)0, &pin_config);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_set_pin");
  }

  ret = i2s_zero_dma_buffer((i2s_port_t)0);
  if (ret != ESP_OK) {
    ei_printf("Error in initializing dma buffer with 0");
  }

  return int(ret);
}

static int i2s_deinit(void) {
    i2s_driver_uninstall((i2s_port_t)0); //stop & destroy i2s driver
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
