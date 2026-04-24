#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "HX711.h"

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- DHT22 ----------------
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ---------------- HX711 ----------------
#define HX_DT 32
#define HX_SCK 33
HX711 balanza;

// Cambia este valor durante calibración
float factor_calibracion = -870.0;

// ---------------- LEDs ----------------
#define LED_ROJO   25
#define LED_VERDE  26
#define LED_AZUL   27

// ---------------- RELÉS ----------------
// K1 = ventilador = IN1
// K2 = bombillo   = IN2
#define RELE_VENTILADOR 19
#define RELE_BOMBILLO   18

void setup() {
  Serial.begin(115200);

  dht.begin();

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error al iniciar OLED");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);

  // LEDs
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);

  // Relés
  pinMode(RELE_VENTILADOR, OUTPUT);
  pinMode(RELE_BOMBILLO, OUTPUT);

  digitalWrite(RELE_VENTILADOR, HIGH); // apagado
  digitalWrite(RELE_BOMBILLO, HIGH);   // apagado

  // HX711
  balanza.begin(HX_DT, HX_SCK);

  if (!balanza.is_ready()) {
    Serial.println("HX711 no detectado");
  } else {
    Serial.println("HX711 listo");
    balanza.set_scale();
    balanza.tare(); // poner en cero la balanza
  }

  // Apagar LEDs al inicio
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AZUL, LOW);
}

void loop() {
  delay(1000);

  // Leer temperatura
  float t = dht.readTemperature();

  // Leer peso
  float peso = 0.0;
  if (balanza.is_ready()) {
    balanza.set_scale(factor_calibracion);
    peso = balanza.get_units(10); // promedio de 10 lecturas
    if (peso < 0) peso = 0; // evita negativos pequeños por ruido
  }

  if (isnan(t)) {
    Serial.println("Error al leer el DHT22");

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Error en DHT22");
    display.display();
    return;
  }

  // Apagar todo primero
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AZUL, LOW);

  digitalWrite(RELE_VENTILADOR, HIGH);
  digitalWrite(RELE_BOMBILLO, HIGH);

  String estado = "";

  // --------- Control de temperatura ---------
  if (t < 36.0) {
    digitalWrite(LED_AZUL, HIGH);
    digitalWrite(RELE_BOMBILLO, LOW);     // Bombillo ON
    digitalWrite(RELE_VENTILADOR, HIGH);  // Ventilador OFF
    estado = "FRIO";
  }
  else if (t >= 36.0 && t <= 37.5) {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(RELE_BOMBILLO, HIGH);    // Bombillo OFF
    digitalWrite(RELE_VENTILADOR, HIGH);  // Ventilador OFF
    estado = "NORMAL";
  }
  else {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(RELE_BOMBILLO, HIGH);    // Bombillo OFF
    digitalWrite(RELE_VENTILADOR, LOW);   // Ventilador ON
    estado = "CALIENTE";
  }

  // --------- Mostrar en OLED ---------
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temp:");

  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(t, 1);
  display.print(" ");
  display.cp437(true);
  display.write(167);
  display.print("C");

  display.setTextSize(1);
  display.setCursor(0, 38);
  display.print("Peso:");

  display.setTextSize(2);
  display.setCursor(0, 48);
  display.print(peso, 1);
  display.print(" g");

  display.setTextSize(1);
  display.setCursor(78, 0);
  display.print(estado);

  display.display();

  // --------- Monitor serial ---------
  Serial.print("Temperatura: ");
  Serial.print(t, 1);
  Serial.print(" C | Peso: ");
  Serial.print(peso, 1);
  Serial.print(" g | Estado: ");
  Serial.println(estado);
}