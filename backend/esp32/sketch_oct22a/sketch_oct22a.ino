#include <FastLED.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define NUM_LEDS 9
#define DATA_PIN 5
#define PULSO_PIN 4  // Pin para detectar el pulso de corriente
#define FILAS 8
#define COLUMNAS 8

CRGB leds[NUM_LEDS];

// Crear el servidor web
AsyncWebServer server(80);

// Variables para los modos
bool modo_dinamico = false;  // Flag para activar el modo dinámico
int fila_actual = 0;         // Índice de la fila actual en el modo dinámico
unsigned long tiempo_inicio_modo = 0;  // Para controlar el tiempo del modo dinámico

// Variables para contar el número de recorridos completos
int recorridos_completados = 0;  // Contador de recorridos completados
const int total_recorridos = 10; // Total de recorridos que se desea hacer

// Credenciales Wi-Fi (cambia a tus valores)
const char* ssid = "MiFibra-9764";  // Cambia por el nombre de tu red Wi-Fi
const char* password = "RKgg3whg";  // Cambia por la contraseña de tu red Wi-Fi

void setup() {
  Serial.begin(115200);

  // Configurar LEDs y FastLED
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  // Encender todos los LEDs en blanco por defecto
  encenderLedsBlancos();

  // Configurar el pin para la detección de pulso de corriente (intermitente)
  pinMode(PULSO_PIN, INPUT_PULLUP);  // INPUT_PULLUP para evitar falsos positivos

  // Conectar a Wi-Fi
  conectarWifi();

  // Ruta para activar el modo dinámico manualmente desde el backend
  server.on("/modo_dinamico", HTTP_GET, [](AsyncWebServerRequest *request) {
    activarModoDinamico();  // Activar el modo dinámico desde el backend
    
    // Enviar la respuesta con el encabezado de CORS
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Modo dinámico activado");
    response->addHeader("Access-Control-Allow-Origin", "*");  // Añadir encabezado CORS
    request->send(response);
  });

  // Ruta para actualizar un píxel
  server.on("/set_pixel", HTTP_GET, [](AsyncWebServerRequest *request) {
    int x = request->getParam("x")->value().toInt();
    int y = request->getParam("y")->value().toInt();
    int r = request->getParam("r")->value().toInt();
    int g = request->getParam("g")->value().toInt();
    int b = request->getParam("b")->value().toInt();

    // Convertir coordenadas (x, y) a índice unidimensional
    int index = y * COLUMNAS + x;
    leds[index] = CRGB(r, g, b);
    FastLED.show();

    // Enviar la respuesta con el encabezado de CORS
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Pixel actualizado");
    response->addHeader("Access-Control-Allow-Origin", "*");  // Añadir encabezado CORS
    request->send(response);
  });

  // Ruta para obtener el estado de los LEDs (para el frontend)
  server.on("/led_status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String ledState = "[";  // Iniciar la cadena con un array JSON

    // Convertir los colores de los LEDs a formato JSON
    for (int i = 0; i < NUM_LEDS; i++) {
      ledState += "{\"r\":" + String(leds[i].r) + ",\"g\":" + String(leds[i].g) + ",\"b\":" + String(leds[i].b) + "}";
      if (i < NUM_LEDS - 1) {
        ledState += ",";  // Añadir coma entre objetos JSON
      }
    }

    ledState += "]";  // Cerrar el array JSON

    // Enviar la respuesta con el estado de los LEDs
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", ledState);
    response->addHeader("Access-Control-Allow-Origin", "*");  // Añadir encabezado CORS
    request->send(response);
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
    ejecutarBarridoFila();
  }
}

// Función para activar el modo dinámico de barrido
void activarModoDinamico() {
  modo_dinamico = true;           // Activar el flag para el modo dinámico
  fila_actual = 0;                // Reiniciar la fila actual
  recorridos_completados = 0;     // Reiniciar el contador de recorridos completados
}

unsigned long previousMillis = 0;
const long interval = 50;  // Intervalo más corto para mayor fluidez (50 ms)

void ejecutarBarridoFila() {
  unsigned long currentMillis = millis();
  
  // Controlar el tiempo entre el cambio de filas
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Primero, encender todos los LEDs en blanco
    encenderLedsBlancos();

    // Encender la fila actual en color naranja
    for (int i = 0; i < COLUMNAS; i++) {
      int index = fila_actual * COLUMNAS + i;
      leds[index] = CRGB(255, 100, 0);  // Color naranja
    }

    // Mostrar los LEDs
    FastLED.show();

    // Avanzar a la siguiente fila
    fila_actual++;

    // Si se completó un barrido (llegar a la última fila)
    if (fila_actual >= FILAS) {
      fila_actual = 0;  // Reiniciar el barrido desde la primera fila
      recorridos_completados++;  // Aumentar el contador de recorridos

      // Verificar si ya se han completado los 10 recorridos
      if (recorridos_completados >= total_recorridos) {
        modo_dinamico = false;  // Detener el modo dinámico después de 10 recorridos
        encenderLedsBlancos();  // Volver a encender los LEDs en blanco
        return;                 // Salir de la función
      }
    }
  }
}

// Función para encender todos los LEDs en blanco
void encenderLedsBlancos() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Yellow;
  }
  FastLED.show();
}

// Función para conectar a Wi-Fi
void conectarWifi() {
  // Iniciar conexión Wi-Fi
  WiFi.begin(ssid, password);

  Serial.println("Conectando a WiFi...");
  
  // Esperar hasta que el ESP32 se conecte a Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Intentando conexión...");
  }

  // Mostrar la dirección IP cuando se conecta
  Serial.println("Conectado a la red Wi-Fi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}
