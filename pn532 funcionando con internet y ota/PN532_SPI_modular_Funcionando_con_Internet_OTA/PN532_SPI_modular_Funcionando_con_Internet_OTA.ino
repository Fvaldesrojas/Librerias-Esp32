//Declaración de Librerías
#include <Wire.h>
#include <SPI.h> //Librería para comunicación SPI
#include <UNIT_PN532.h> //Librería Modificada
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> // librerias de OTA (over the air esp32)
#include <HTTPClient.h>

//Conexiones SPI del ESP32
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (5)
#define PN532_MISO (19)

#define S0 32       // Pin de control S0 del multiplexor/demultiplexor
#define S1 33
#define S2 25
#define S3 26       // Pin de control S0 del multiplexor/

uint8_t DatoRecibido[4]; //Para almacenar los datos
const int Lectores=7;// numero de lectores conectados a conveniencia 
const int SSS[]={5,5,5};

UNIT_PN532 *nfc[Lectores];// Línea enfocada para la comunicación por SPI

const char *ssid = "Citylabbiobio";
const char *password = "M1TL4B..";
const char *url = "http://192.168.31.120:8500/api/set_map_state/?slots=";


String Tarjetas[] = {
  /*0*/ "NULL",
  /*1*/ "",
  /*2*/ "",  
  /*3*/ "",
  /*4*/ "",
  /*5*/ "",
  /*6*/ "",
  /*7*/ "",
  /*8*/ "",
  /*9*/ "string 3",
  /*10*/ "string 3",
  /*11*/ "string 3",
  /*12*/ "string 3",
  /*13*/ "3FEC402",
  /*14*/ "3FBA5E2",
  /*15*/ "6922EB18",
  /*16*/ "6094FC55",
  /*17*/ "4FA9582",
  /*18*/ "7019B55",
  /*19*/ "60D8F155",
  /*20*/ "3FF66C2",
  /*21*/ "3FB02E2",
  /*22*/ "70B2B55",
  /*23*/ "7064BD55",
  /*24*/ "60BEFC55",
  /*25*/ "60801355",
  /*26*/ "B963C855"
  };

void setup(){
  Serial.begin(115200);
  delay(200);
  Serial.println("ESP32 Iniciado");
  IniciarInternet();
  
  
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  

  iniciarLectores();

}


void loop(){
  ArduinoOTA.handle();
  leer();

}

void leer(){

  for(int c=0;c<Lectores;c++){

    delay(20);
    selectModule(c);
    
    boolean LeeTarjeta; //Variable para almacenar la detección de una tarjeta
    
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Búfer para almacenar el UID devuelto
    uint8_t uidLength; //Variable para almacenar la longitud del UID de la tarjeta
    
    //Recepción y detección de los datos de la tarjeta y almacenamiento en la variable "LeeTarjeta"
    LeeTarjeta = nfc[c]->readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
    delay(10);
    Serial.println("tope");


    if (LeeTarjeta) {
    //Serial.println("Tarjeta encontrada!");
    //Serial.print("Longitud del UID: ");
    //Serial.print(LongitudUID, DEC); //Imprime la longitud de los datos de la tarjeta en decimal
    //Serial.println(" bytes");
    //Serial.print("Valor del UID: ");
    // Imprime los datos almacenados en la tarjeta en Hexadecimal
    String lectura = "";

    for (uint8_t i = 0; i < uidLength; i++) {
      lectura += String(uid[i], HEX);
    }

    lectura.toUpperCase();
    Serial.println("Lector " + String(c) + " - UID: " + lectura);

    bool registrada = false;

    for (int i = 0; i < sizeof(Tarjetas) / sizeof(Tarjetas[0]); i++) {
      if (lectura == Tarjetas[i]) {
        Serial.println("Tarjeta registrada en el lector " + String(c) + ", Número: " + String(i));
        registrada = true;
        enviarNumero(i);
        break;
      }
    }

    if (!registrada) {
      Serial.println("Tarjeta no registrada en el lector " + String(c) + ", UID: " + lectura);
    }

    delay(50);
  }



  }

}

void iniciarLectores() {


  //pinMode(RST_PIN, OUTPUT);
  pinMode(PN532_SS, OUTPUT);
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);



  for (int i = 0; i < Lectores; i++) {
    bool conexionExitosa = false;
    selectModule(i);
    delay(20);

    while (!conexionExitosa) {
      nfc[i] = new UNIT_PN532(5);
      nfc[i]->begin();
      uint32_t versiondata = nfc[i]->getFirmwareVersion();

      if (versiondata) {
        Serial.print("Chip encontrado PN5 en el lector ");
        Serial.println(i);
        nfc[i]->setPassiveActivationRetries(2);
        nfc[i]->SAMConfig();
        conexionExitosa = true;  // Romper el bucle si la conexión es exitosa
      } else {
        Serial.print("No se encontró la placa PN532 en el lector numero___" + String((i+1)));
        //Serial.println(i);
        delay(1000);  // Esperar un segundo antes de intentar nuevamente
      }
    }
  }
}

void selectModule(int module) {
  Serial.println("Caso numero:  " + String(module));
  switch (module+2) {
    case 0:
      digitalWrite(S0, LOW);
      digitalWrite(S1, LOW);
      digitalWrite(S2, LOW);
      digitalWrite(S3, LOW);
      break;
    case 1:
      digitalWrite(S0, HIGH);
      digitalWrite(S1, LOW);
      digitalWrite(S2, LOW);
      digitalWrite(S3, LOW);
      break;
    case 2:
      digitalWrite(S0, LOW);
      digitalWrite(S1, HIGH);
      digitalWrite(S2, LOW);
      digitalWrite(S3, LOW);
      break;
    case 3:
      digitalWrite(S0, HIGH);
      digitalWrite(S1, HIGH);
      digitalWrite(S2, LOW);
      digitalWrite(S3, LOW);
      break;
    case 4:
      digitalWrite(S0, LOW);
      digitalWrite(S1, LOW);
      digitalWrite(S2, HIGH);
      digitalWrite(S3, LOW);
      break;
    case 5:
      digitalWrite(S0, HIGH);
      digitalWrite(S1, LOW);
      digitalWrite(S2, HIGH);
      digitalWrite(S3, LOW);
      break;
    case 6:
      digitalWrite(S0, LOW);
      digitalWrite(S1, HIGH);
      digitalWrite(S2, HIGH);
      digitalWrite(S3, LOW);
      break;
    case 7:
      digitalWrite(S0, HIGH);
      digitalWrite(S1, HIGH);
      digitalWrite(S2, HIGH);
      digitalWrite(S3, LOW);
      break;
    case 8:
      digitalWrite(S0, LOW);
      digitalWrite(S1, LOW);
      digitalWrite(S2, LOW);
      digitalWrite(S3, HIGH);
      break;
    case 9:
      digitalWrite(S0, HIGH);
      digitalWrite(S1, LOW);
      digitalWrite(S2, LOW);
      digitalWrite(S3, HIGH);
      break;
  }
  delay(20);
}

void enviarNumero(int num) {
  HTTPClient http;
  http.begin(url + String(num));
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.printf("Código de estado HTTP: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Respuesta del servidor:");
      Serial.println(payload);
    }
  } else {
    Serial.println("Error en la solicitud HTTP");
  }

  http.end();
}

void IniciarInternet() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a la red WiFi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}



