#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// Configuración del sensor DHT
#define DHTPIN 2       // Pin al que está conectado el sensor DHT
#define DHTTYPE DHT11  // Tipo de sensor DHT

DHT dht(DHTPIN, DHTTYPE);

// Configuración del sensor de ultrasonido HC-SR04
#define TRIGGER_PIN 12  // Pin de disparo del sensor
#define ECHO_PIN 14     // Pin de eco del sensor

// Configuración de la red Wi-Fi
const char* ssid = "Claro_633469";
const char* password = "F3Y4D6X8S5Y3";

const char* serverName = "api-arduino-r24u.onrender.com";
const char* apiEndpoint = "/api/create-register";

float altura;
float volumenLitros;

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  dht.begin();  // Inicializar el sensor DHT

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  // Lectura del sensor DHT
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // Lectura del sensor HC-SR04
  float distancia = leerDistancia();

  // Cálculos de altura y volumen
  altura = 213 - distancia;
  float volumen = (altura * 210 * 280);  // Volumen en centímetros cúbicos
  volumenLitros = volumen / 1000.0;  // Convertir de cm³ a litros

  Serial.println("Datos del sensor:");
  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println(" %");
  Serial.print("Distancia HC-SR04: ");
  Serial.print(distancia);
  Serial.println(" cm");
  Serial.print("Altura: ");
  Serial.print(altura);
  Serial.println(" cm");
  Serial.print("Volumen: ");
  Serial.print(volumenLitros);
  Serial.println(" litros");
  Serial.println();

  // Envío de datos al servidor si ha pasado el tiempo especificado
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;

      // Configura la conexión SSL utilizando BearSSL
      client.setInsecure();

      // Envía solicitud HTTPS POST
      if (client.connect(serverName, 443)) {
        Serial.println("Connected to server");

        // Construir la cadena de solicitud
        String httpRequestData = "{\"temperatura\":" + String(temperatura) +
                                 ",\"humedad\":" + String(humedad) +
                                 ",\"distancia\":" + String(distancia) +
                                 ",\"altura\":" + String(altura) +
                                 ",\"volumen\":" + String(volumenLitros) + "}";
        Serial.println("JSON Request: " + httpRequestData);
        String request = "POST " + String(apiEndpoint) + " HTTP/1.1\r\n"
                        "Host: " + String(serverName) + "\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: " + String(httpRequestData.length()) + "\r\n"
                        "Accept: */*\r\n"
                        "Connection: close\r\n\r\n" +
                        httpRequestData;

        client.print(request);
        Serial.println("Request sent");

        while (client.connected()) {
          if (client.available()) {
            String line = client.readStringUntil('\r');
            Serial.print(line);
          }
        }

        Serial.println();
        Serial.println("Closing connection");
        client.stop();
      } else {
        Serial.println("Unable to connect to server");
      }

    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

  delay(5000);  // Esperar antes de realizar la siguiente lectura
}

float leerDistancia() {
  // Genera un pulso corto en el pin de disparo del sensor
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(15);
  digitalWrite(TRIGGER_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // Máximo tiempo de espera de 30 ms

  // Calcula la distancia en centímetros utilizando la fórmula del ultrasonido
  float distance = (duration / 2) / 29.1;

  return distance;
}
