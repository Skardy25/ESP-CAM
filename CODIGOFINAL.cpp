#include "WiFi.h"
#include "HTTPClient.h"
#include "esp_camera.h"
#include <ArduinoJson.h>
#include <base64.h>

// === CONFIGURACIÓN ===
const char* ssid = "";
const char* password = "";
const String BOT_TOKEN = ""; // Tu token
const String CHAT_ID = ""; // Tu chat ID

// URL de tu API
const char* serverUrl = "https://api-person-detection.onrender.com/upload-base64"; // Cambia por tu IP y puerto

const int pirPin = 13;       // Pin del sensor PIR
const int alarmPin = 4;      // Pin para activar la alarma (conectado a la base del transistor)
bool motionDetected = false;

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// === FUNCION DE CODIFICACION URL ===
String urlEncodeUTF8(String str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encoded += '+';
    } else if (isalnum(c)) {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      code0 = ((c >> 4) & 0xf);
      code0 += code0 > 9 ? 'A' - 10 : '0';
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}
// === URL TELEGRAM ===

// === SETUP ===
void setup() {
  Serial.begin(115200);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Calidad de imagen (ajusta según tu necesidad)
  config.frame_size = FRAMESIZE_SVGA; // 800x600
  config.jpeg_quality = 10; // 0-63 (menor es mejor calidad)
  config.fb_count = 1;

  // Inicializar cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error inicializando cámara: 0x%x", err);
    return;
  }

  pinMode(pirPin, INPUT);
  pinMode(alarmPin, OUTPUT);
  digitalWrite(alarmPin, LOW); // Iniciar apagado

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado a WiFi");
  Serial.println("IP asignada: " + WiFi.localIP().toString());
}

// === LOOP ===
void loop() {
  int pirValue = digitalRead(pirPin);

  if (pirValue == HIGH && !motionDetected) {
    Serial.println("🚨 Movimiento detectado!");

     camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Error al capturar la foto");
        return;
      }

      // Convertir a Base64
      String imageBase64 = base64::encode(fb->buf, fb->len);

      // Crear JSON con la imagen
      String httpRequestData = "{\"image\":\"data:image/jpeg;base64," + imageBase64 + "\"}";

      // Enviar a tu API
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");
      
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);

      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);
      int responseCode = doc["code"];

      if (responseCode == 1004) {
        digitalWrite(alarmPin, HIGH);
        Serial.println("🔊 Alarma activada (personas detectadas)");
        delay(3000); // 3 segundos de alarma activa
        digitalWrite(alarmPin, LOW);
      }

      } else {
        Serial.print("Error en la petición: ");
        Serial.println(httpResponseCode);
      }
      
      http.end();
      esp_camera_fb_return(fb);

      motionDetected = true;
      
  }

  if (pirValue == LOW && motionDetected) {
    Serial.println("No hay movimiento.");
    motionDetected = false;
  }

  delay(100); // Anti-rebote del sensor
}
