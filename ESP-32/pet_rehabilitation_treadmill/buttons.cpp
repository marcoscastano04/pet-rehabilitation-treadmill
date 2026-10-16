#include "buttons.h"
#include "Display.h"
#include "globals.h"

const bool ButtonText = true;      //true si se escribe algo cada vez que se apreta el boton, false para imprimir todo lo que llega

const unsigned long DEBOUNCE_DELAY = 40;  //ms

int buttonPins[4] = {BUTTON_FAST_PIN, BUTTON_SLOW_PIN, BUTTON_UP_PIN, BUTTON_DOWN_PIN};

#define BTN_FAST   0
#define BTN_SLOW   1
#define BTN_UP     2   // Confirmar (YES)
#define BTN_DOWN   3   // Rechazar (NO)

static int lastReading[4]    = {HIGH, HIGH, HIGH, HIGH};   // Última lectura de cada botón.  
static int stableState[4]   = {HIGH, HIGH, HIGH, HIGH};
static uint32_t lastDebounceTime[4] = {0, 0, 0, 0};    // Instante de la última variación detectada.


// Ejecutar la acción asociada al botón presionado.
void handleButtonPress(int buttonIndex) {
  switch (buttonIndex) {
    case 0:
      sendCommandSpaced("<g>"); 
      if (ButtonText){
        Serial.println("Button 1 pressed: speed UP");
      }
      break;
    case 1:
      sendCommandSpaced("<s>"); 
      if (ButtonText){
        Serial.println("Button 2 pressed: speed DOWN");
      }
      break;
    case 2:
      currentScreen = (currentScreen + 1) % maxScreens;
      displayScreen(currentScreen);
      if (ButtonText){
        Serial.println("Button 3 pressed: change page");
        imprimirValores();
      }
      break;
    case 3:
      sendCommandSpaced("<vt 0>"); 
      if (ButtonText){
        Serial.println("Button 4 pressed: STOP");
      }
      break;
  }
}


// Leer los botones, aplicar antirrebote y gestionar las pulsaciones.
void handleButtons(void) {
  uint32_t now = millis();

  for (int i = 0; i < 4; i++) {
    int reading = digitalRead(buttonPins[i]);

    // Reiniciar el temporizador ante un cambio en la lectura.
    if (reading != lastReading[i]) {
      lastDebounceTime[i] = now;
    }

    // Verificar si la lectura permaneció estable durante el intervalo de antirrebote.
    if ((now - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
      // Cambio de estado confirmado
      if (reading != stableState[i]) {
        stableState[i] = reading;

        // Detectar flanco HIGH → LOW
        if (stableState[i] == LOW) {
          // ────── MODO PAIRING ──────
          if (PairingInProgress ) {
            if (i == BTN_UP) {
              Serial.println("User confirmed pairing (YES button).");
              SerialBT.confirmReply(true);
              PairingInProgress = false;
              return;
            }
            else if (i == BTN_DOWN) {
              Serial.println("User rejected pairing (NO button).");
              SerialBT.confirmReply(false);
              PairingInProgress = false;
              return;
            }
          }
          // ────── MODO NORMAL ──────
          handleButtonPress(i);
          screenNeedsRefresh = true;
        }
      }
    }

    lastReading[i] = reading;
  }
}

// Imprimir por consola las variables de estado y telemetría.
void imprimirValores() {
    Serial.println(" ");
    Serial.print("Tiempo Transcurrido: ");
    Serial.print(minutes);Serial.print(":");Serial.println(seconds);
    Serial.print("Velocidad medida: ");
    Serial.println(td.speed_measured, 1);
    Serial.print("IC3 buffer: ");
    Serial.println(td.ic3_buffer);
    Serial.print("Velocidad usuario: ");
    Serial.println(td.speed_setpoint_u);
    Serial.print("Velocidad maquina: ");
    Serial.println(td.speed_setpoint_m);
    Serial.print("Salida del compensador 'Y': ");
    Serial.println(td.salida_y);
    Serial.print("Error: ");
    Serial.println(td.error);
    Serial.print("Corriente: ");
    Serial.println(td.current, 2);
    Serial.print("Acumulador integral: ");
    Serial.println(td.acumulador_integral, 2);
    Serial.print("Bus de tensión: ");
    Serial.println(td.bus_voltage, 1);
    Serial.print("Temperatura: ");
    Serial.print(td.temperature, 2); Serial.println("°C");
    Serial.print("Tiempo de ejecucion: ");
    float t1_runtime = (float)td.t1_ticks *0.1f;
    Serial.print(t1_runtime, 2); Serial.println("µs");
}
