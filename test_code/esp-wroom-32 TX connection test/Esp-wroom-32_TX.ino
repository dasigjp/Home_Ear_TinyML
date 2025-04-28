//Install CH341SER included na sa zip
// Etong Transmitter nag transmitt ng message every 5 seconds
// ung Rx naka set na mag receive ng message every 10 sec

// TX DEVICE 
// Test Code or ESP Wroom 32 as Transmitter ( LORA ) - 04-04-25 6:36pm
// Define SPI pins for LoRa module on ESP32-WROOM-32
#include <LoRa.h>
#include <SPI.h>

// TX DEVICE - LoRa Transmitter (ESP32-WROOM-32)

// Define SPI pins for LoRa module
#define LORA_MISO   19  // SPI MISO
#define LORA_MOSI   27  // SPI MOSI
#define LORA_SCK    5   // SPI SCK
#define LORA_SS     18  // LoRa CS
#define LORA_RST    14  // LoRa Reset
#define LORA_DIO0   26  // LoRa IRQ (Interrupt)

// List of sound alerts
const char* soundAlerts[] = {
  "FIRE ALARM",
  "EMERGENCY VEHICLE",
  "GUN SHOTS",
  "EARTHQUAKE DRILL",
  "DOOR KNOCK",
  "DOG BARK",
  "BABY CRYING",
  "LISTENING"
};

const int numAlerts = sizeof(soundAlerts) / sizeof(soundAlerts[0]); // Total number of alerts
int currentAlertIndex = 0;  // Index to track the current alert

void setup() 
{
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial Monitor
  Serial.println("\n🚀 LoRa Transmitter - ESP32-WROOM-32");

  // Initialize SPI for LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  
  // Set LoRa module pins
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  // Try to initialize LoRa with a timeout
  unsigned long startAttemptTime = millis();
  while (!LoRa.begin(433E6))  // 433 MHz
  {
    Serial.println(" LoRa init failed. Retrying...");
    delay(500);
    
    if (millis() - startAttemptTime > 5000) // Timeout after 5 seconds
    {
      Serial.println(" LoRa module not found! Check wiring.");
      while (true);  // Stop execution
    }
  }

  // LORA PARAMETERs
  /* NOT IN USE
  LoRa.setSpreadingFactor(10);   // SF10 (Range vs. Speed balance)
  LoRa.setSignalBandwidth(125E3); // 125 kHz Bandwidth
  LoRa.setCodingRate4(5);        // Error correction 4/5
  LoRa.setSyncWord(0x12);        // Custom sync word for private network
  LoRa.setTxPower(20);           // Max TX Power
  */   


  // New Config for Lora for longer distance
  // Change to this config for smooth transmission ng data at longer disntace 04-04-2025 6:36 pm
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(62.5E3);
  LoRa.setCodingRate4(8);
  LoRa.setSyncWord(0x12);
  LoRa.setTxPower(20);
  

  Serial.println("✅ LoRa Initialized Successfully!");
}

void loop() 
{
  Serial.print(" Sending: ");
  Serial.println(soundAlerts[currentAlertIndex]);

  bool sent = false;
  for (int i = 0; i < 3; i++)  // Retry sending up to 3 times
  {
    LoRa.beginPacket();
    LoRa.print(soundAlerts[currentAlertIndex]);
    if (LoRa.endPacket()) 
    {
      sent = true;
      break;
    }
    Serial.println(" Sending failed! Retrying...");
    delay(500);  // Short delay before retry
  }

  if (sent)
    Serial.println(" Message Sent!");
  else
    Serial.println(" Failed to send message!");

  currentAlertIndex = (currentAlertIndex + 1) % numAlerts; // Move to next alert in the list

  delay(5000);  // Send every 5 seconds
}
