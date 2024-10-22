#include <FastLED.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define NUM_LEDS 64
#define DATA_PIN 5
#define PULSO_PIN 4  // Pin donde conectaremos la señal de pulso de corriente (intermitente)

CRGB leds[NUM_LEDS];

// Configuración de red
const char* ssid = "TU_SSID";
const char* password = "TU_PASSWORD";

// Crear el servidor web
AsyncWebServer server(80);

// Variables para los modos
bool modo_dinamico = false;  // Flag para activar el modo dinámico

void setup() {
  Serial.begin(115200);

  // Configurar LEDs y FastLED
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  // Configurar el pin para la detección de pulso de corriente (intermitente)
  pinMode(PULSO_PIN, INPUT_PULLUP);  // INPUT_PULLUP para evitar falsos positivos

  // Conectar al WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a la red WiFi");

  // Ruta para activar el modo dinámico manualmente desde el backend
  server.on("/modo_dinamico", HTTP_GET, [](AsyncWebServerRequest *request) {
    modo_dinamico = true;  // Activar modo dinámico desde el backend
    request->send(200, "text/plain", "Modo dinámico activado");
  });


  server.on("/set_pixel", HTTP_GET, [](AsyncWebServerRequest *request) {
    int x = request->getParam("x")->value().toInt();
    int y = request->getParam("y")->value().toInt();
    int r = request->getParam("r")->value().toInt();
    int g = request->getParam("g")->value().toInt();
    int b = request->getParam("b")->value().toInt();

    // Convertir coordenadas (x, y) a índice unidimensional
    int index = y * 8 + x;
    leds[index] = CRGB(r, g, b);
    FastLED.show();

    request->send(200, "text/plain", "Pixel actualizado");
  });
  
  // Iniciar servidor web
  server.begin();
}

void loop() {
  // Verificar si hay un pulso de corriente (cuando el pin PULSO_PIN recibe un valor bajo)
  if (digitalRead(PULSO_PIN) == LOW) {
    Serial.println("Pulso de corriente detectado: Activando modo dinámico.");
    activarModoDinamico();
  }

  // Si está activo el modo dinámico, ejecutarlo
  if (modo_dinamico) {
    ejecutarBarridoNaranja();
  }
}

// Función para activar el modo dinámico de barrido
void activarModoDinamico() {
  modo_dinamico = true;  // Activar el flag para el modo dinámico
}

unsigned long previousMillis = 0;
const long interval = 100;  // Intervalo de tiempo entre cambios de LED

void ejecutarBarridoNaranja() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        // Primero, encender todos los LEDs en blanco
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB::White;
        }

        // Encender el LED actual en color naranja
        leds[led_actual] = CRGB(255, 100, 0);  // Color naranja

        // Mostrar los LEDs
        FastLED.show();

        // Avanzar al siguiente LED para el siguiente barrido
        led_actual++;

        // Reiniciar el barrido cuando llega al último LED
        if (led_actual >= NUM_LEDS) {
            led_actual = 0;
            modo_dinamico = false;  // Detener el modo dinámico cuando termine un ciclo completo
        }
    }
}

