#include <FastLED.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>

#define NUM_LEDS 64
#define DATA_PIN 5
#define PULSO_PIN 4  // Pin para detectar el pulso de corriente
#define PARPADEO_PIN 2
#define FILAS 8
#define COLUMNAS 8

CRGB leds[NUM_LEDS];
volatile bool ejecutandoParpadeo = false;
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

  // Configurar el pin para la detección de pulso de corriente (intermitente)
  pinMode(PULSO_PIN, INPUT);  // Cambiado a INPUT porque se usará una fuente externa
  // Conectar a Wi-Fi
  conectarWifi();
  secuenciaEncendidoBarrido();
  // Ruta para activar el modo dinámico manualmente desde el backend
  server.on("/modo_show_1", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (xTaskCreatePinnedToCore(parpadeoRapidoGruposTask, "parpadeoTask", 8192, NULL, 1, NULL, 1) != pdPASS) {
      // Si la tarea no se pudo crear, enviar un error
      AsyncWebServerResponse *response = request->beginResponse(500, "text/plain", "Error al iniciar la tarea");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    } else {
      // Responder de forma inmediata que la tarea ha comenzado
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Modo show activado");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    }
  });

  server.on("/desactivar-modo-show-1", HTTP_GET, [](AsyncWebServerRequest *request) {
    ejecutandoParpadeo = false; // Detener el parpadeo
    request->send(200, "text/plain", "Parpadeo detenido");
    Serial.println("Orden recibida: detener parpadeo");
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
  static bool estado_anterior = LOW;  // Guardar el estado anterior del pin
  bool estado_actual = digitalRead(PULSO_PIN);  // Leer el estado del pin
  if (estado_actual == HIGH && estado_anterior == LOW) {
    // Detectar transición de LOW a HIGH (inicio del pulso)
    activarModoDinamico();  // Iniciar modo dinámico
  } else if (estado_actual == LOW && estado_anterior == HIGH) {
    // Detectar transición de HIGH a LOW (fin del pulso)
    modo_dinamico = false;  // Desactivar el modo dinámico
    encenderLedsBlancos();  // Restablecer LEDs a blanco
  }

  // Actualizar el estado anterior
  estado_anterior = estado_actual;

  // Si el modo dinámico está activo, ejecutarlo
  if (modo_dinamico) {
    ejecutarBarridoFila();
  }
}

// Función para activar el modo dinámico de barrido
void activarModoDinamico() {
  modo_dinamico = true;           // Activar el flag para el modo dinámico
  fila_actual = 0;                // Reiniciar la fila actual
  recorridos_completados = 0;     // Reiniciar el contador de recorridos completados
  apagarLeds();
}

unsigned long previousMillis = 0;
const long interval = 80;  // Intervalo más corto para mayor fluidez (50 ms)

void ejecutarBarridoFila() {
  unsigned long currentMillis = millis();
  
  // Controlar el tiempo entre el cambio de filas
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

      apagarLeds();

    // Encender las filas anteriores en color naranja
    for (int i = 0; i < fila_actual; i++) {
      for (int j = 0; j < COLUMNAS; j++) {
        int index = i * COLUMNAS + j;
        leds[index] = CRGB(255, 102, 0);  // Color naranja para las filas anteriores
      }
    }

    // Encender la fila actual en color naranja
    for (int i = 0; i < COLUMNAS; i++) {
      int index = fila_actual * COLUMNAS + i;
      leds[index] = CRGB(255, 102, 0);  // Color naranja
    }

    // Mostrar los LEDs
    FastLED.show();

    // Avanzar a la siguiente fila
    fila_actual++;

    // Si se completó un barrido (llegar a la última fila)
    if (fila_actual >= FILAS) {
      fila_actual = 0;  // Reiniciar el barrido desde la primera fila
      recorridos_completados++;  // Aumentar el contador de recorridos
    }
  }
}

// Función para encender todos los LEDs en un tono diferente de amarillo
void encenderLedsBlancos() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255, 251, 0); // Un tono cálido de amarillo
  }
  FastLED.show();
}

void secuenciaEncendidoBarrido() {
  // Colores deseados (en formato RGB)
  CRGB colores[] = {
    CRGB(162, 0, 255),  // Primer color
    CRGB(76, 0, 255),   // Segundo color
    CRGB(0, 153, 255),  // Tercer color
    CRGB(0, 255, 255)   // Cuarto color
  };

  int num_colores = sizeof(colores) / sizeof(colores[0]); // Número total de colores
  int leds_a_encender = 4;  // Número de LEDs a encender en cada paso
  int color_actual = 0;     // Índice del color actual

  for (int i = 0; i < NUM_LEDS + leds_a_encender; i++) { 
    // Encender un grupo de LEDs de forma conjunta
    for (int j = 0; j < leds_a_encender; j++) {
      int index = (i + j) % NUM_LEDS;  // Asegurar que no se salga del rango de los LEDs
      leds[index] = colores[color_actual];  // Asignar el color actual
    }

    // Mostrar la actualización de los LEDs
    FastLED.show();

    // Apagar los LEDs del paso anterior
    if (i >= leds_a_encender) {
      for (int j = 0; j < leds_a_encender; j++) {
        int index = (i - leds_a_encender + j) % NUM_LEDS;  // Apagar los LEDs de la posición anterior
        leds[index] = CRGB(0, 0, 0);  // Apagar el LED
      }
    }

    // Cambiar al siguiente color después de encender el grupo de LEDs
    if (i % leds_a_encender == 0) {  // Cambiar color cada grupo
      color_actual = (color_actual + 1) % num_colores;  // Avanzar al siguiente color en el arreglo
    }

    delay(30);  // Ajusta este valor para hacer el barrido más rápido o más lento
  }

  // Al finalizar, encender todos los LEDs en blanco (o el color deseado)
  encenderLedsBlancos();
}


void parpadeoRapidoGrupos() {
  ejecutandoParpadeo = true;
  Serial.print("Memoria libre principio: ");
  Serial.println(ESP.getFreeHeap());

  // Apagar los LEDs inicialmente
  apagarLeds();

  // Configuración de los parámetros
  const int leds_a_encender = 2;
  const int duracion_parpadeo = 50;   // Duración del parpadeo en ms
  const int pausa_grupo = 200;        // Pausa entre grupos en ms
  const int num_parpadeos = 4;        // Número de parpadeos por grupo

  // Array de colores
  CRGB colores[] = {
    CRGB(162, 0, 255),
    CRGB(76, 0, 255),
    CRGB(0, 153, 255),
    CRGB(0, 255, 255)
  };
  const int num_colores = sizeof(colores) / sizeof(colores[0]);

  // Variables locales para el bucle
  unsigned long ultimo_evento = millis();
  int ciclo = 0;
  bool encender = true;
  int parpadeo_actual = 0;
  int grupo_inicio = 0;
  int contador = 0;

  // Bucle principal: continúa mientras ejecutandoParpadeo sea true
  while (ejecutandoParpadeo) {
    if (!ejecutandoParpadeo) {
      break;  // Salir del bucle si se ha pedido detener
    }

    unsigned long tiempo_actual = millis();

    // Encender y apagar LEDs según el tiempo de parpadeo
    if (encender && tiempo_actual - ultimo_evento >= duracion_parpadeo) {
      if (parpadeo_actual == 0) {
        grupo_inicio = random(0, NUM_LEDS - leds_a_encender + 1); // Selección aleatoria del grupo
      }
      CRGB color_actual = colores[ciclo % num_colores];

      for (int i = 0; i < leds_a_encender; i++) {
        leds[grupo_inicio + i] = color_actual;
      }
      FastLED.show();
      encender = false;
      ultimo_evento = tiempo_actual;
    } else if (!encender && tiempo_actual - ultimo_evento >= duracion_parpadeo) {
      // Apagar LEDs del grupo actual
      for (int i = 0; i < leds_a_encender; i++) {
        leds[grupo_inicio + i] = CRGB(0, 0, 0);
      }
      FastLED.show();
      encender = true;
      ultimo_evento = tiempo_actual;
      parpadeo_actual++;

      // Pausa entre parpadeos de un grupo
      if (parpadeo_actual >= num_parpadeos) {
        unsigned long pausa_inicio = millis();
        while (millis() - pausa_inicio < pausa_grupo) {
          if (!ejecutandoParpadeo) break; // Salir si se recibe la orden de detener
          yield(); // Ceder tiempo al sistema operativo
        }
        parpadeo_actual = 0;
        ciclo++;
      }
    }

    // Ceder tiempo al sistema operativo para evitar bloqueos
    yield();
  }

  // Apagar todos los LEDs al detener el parpadeo
  apagarLeds();
  encenderLedsBlancos();
  Serial.println("Parpadeo detenido por orden externa.");
}

void parpadeoRapidoGruposTask(void *pvParameters) {
  // Esta función se ejecutará en una tarea separada
  parpadeoRapidoGrupos();  // Llamada a tu función de parpadeo
  vTaskDelete(NULL);  // Eliminar la tarea cuando termine
}

void apagarLeds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);  // Color negro (apagado)
  }
  FastLED.show();  // Reflejar los cambios en los LEDs
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
