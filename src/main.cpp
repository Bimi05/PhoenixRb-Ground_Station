#include <Arduino.h>
#include <RH_RF95.h>
#include <MFRC522.h>
#include <SD.h>

#include <SPI.h>

#include <megalovania.h>
#include <rickroll.h>

#define RFM_CS 4
#define RFM_INT 3
#define RFM_RST 2
#define RFM_FREQUENCY 434.2F

#define RFID_CS 7
#define RFID_RST 6
#define BUZ_PIN 48

RH_RF95 RFM = RH_RF95(RFM_CS, RFM_INT);
MFRC522 RFID = MFRC522(RFID_CS, RFID_RST);
File dataFile;

char* packet = (char*) malloc(200 * sizeof(char));

bool debug = true;
bool targetselected = false;

void play(uint8_t pin, uint16_t frequency, uint32_t duration) {
    tone(pin, frequency);
    delay(duration);
    noTone(pin);
}

void setup() {
    Serial.begin(9600);
    while (!Serial);

    Serial.println("[Debug] Starting initialisation...");
    play(BUZ_PIN, 450, 500);

    pinMode(RFM_RST, OUTPUT);
    digitalWrite(RFM_RST, LOW);
    delay(100);
    digitalWrite(RFM_RST, HIGH);
    delay(100);

    if (!RFM.init()) {
        Serial.println("[Debug] Failed to initialise RFM9x LoRa.");
    } else {
        RFM.setFrequency(RFM_FREQUENCY);
        RFM.setModeRx();
        Serial.println("[Debug] RFM9x LoRa initialised!");
    }

    if (!SD.begin(49)) {
        Serial.println("[Debug] Failed to initialise the SD card.");
    } else {
        Serial.println("[Debug] SD card initialised!");
    }

    RFID.PCD_Init();    
    Serial.println("[Debug] RFID initialised!");
    play(BUZ_PIN, 900, 600);
}

void loop() {
    uint8_t buf[200];
    uint8_t len = sizeof(buf);
    if (RFM.recv(buf, &len)) {
        packet = (char*) buf;
        if (packet[0] == 'P' && packet[1] == 'R' && packet[2] == 'b' && packet[3] == '_') {
            if (debug) {
                Serial.print(packet);
                Serial.print(" (RSSI: ");
                Serial.print(RFM.lastRssi());
                Serial.print(")");
            }

            if (packet[4] == 'D') { 
                dataFile = SD.open("datatext.txt", FILE_WRITE);
            }

            if (packet[4] == 'P') {
                dataFile = SD.open("poi.txt", FILE_WRITE);
            }

            if (dataFile) {
                dataFile.println(packet);
                dataFile.close();
            }
        }
    }

    while (!Serial.available() == 0) {
        if (Serial.readString() == "poi") {
            debug = false;
            dataFile = SD.open("poi.txt");
            if (dataFile) {
                Serial.write(dataFile.read());
                dataFile.close();
            }
        }

        if (Serial.readString() == "resume") {
            debug = true;
        }

        if (Serial.readString() == "target") {
            targetselected = true;
        }
    }

    while (targetselected) {
        Serial.print("Select where to land: ");
        while (!Serial.available() == 0) {
            String poi_num = Serial.readString();
            char poi_def[20];
            snprintf(poi_def, sizeof(poi_def), "PRb_PoI: %s", poi_num);

            RFM.send((uint8_t*) poi_def, sizeof(poi_def));
        }
    }


    if (RFID.PICC_IsNewCardPresent()) {
        rickroll(BUZ_PIN);
    }
}
