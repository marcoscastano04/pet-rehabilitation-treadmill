#include "Display.h"
#include "globals.h"
#include <LiquidCrystal_I2C.h>

// Estado de funcionamiento de la caminadora
bool treadmillRunning = false;       
bool treadmillRunningPrev = false;
// Variables para medir el tiempo de funcionamiento
uint32_t elapsedSeconds = 0;     
uint32_t lastTimeUpdate = 0;

uint16_t minutes;
uint16_t seconds;

void updateTreadmillTime(float speed) {
  treadmillRunning = (speed > 3);   // Umbral de movimiento

  // Inicio del movimiento
  if (treadmillRunning && !treadmillRunningPrev) {
    elapsedSeconds = 0;            
    lastTimeUpdate = millis();     
  }

  // Detención del movimiento
  if (!treadmillRunning && treadmillRunningPrev) {
    elapsedSeconds = 0;            
  }

  // Actualizar el tiempo transcurrido
  if (treadmillRunning) {
    uint32_t now = millis();
    while (now - lastTimeUpdate >= 1000) {
      elapsedSeconds++;
      lastTimeUpdate += 1000;
    }
  }

  treadmillRunningPrev = treadmillRunning;
}

void displayScreen(int screen) {

  // Actualizar la segunda línea del LCD
  lcd.setCursor(0, 1);
  switch (screen) {
    case 0:{
      float velKM = td.speed_setpoint_u*0.025f;     //velocidad usuario en Km/h
      lcd.print("Vsp: "); 
      lcd.print(velKM, 2); 
      lcd.print("km/h"); 
      lcd.print("      ");
      break;
      }
    case 1:{     
      lcd.print("Vel: "); 
      lcd.print(td.speed_setpoint_u);         //velocidad usuario en rad/s
      lcd.print("rad/s"); 
      lcd.print("      ");
      break;
      }
    case 2:{
      lcd.print("Temp: "); 
      lcd.print(td.temperature,1); 
      lcd.print((char)223); 
      lcd.print("C"); 
      lcd.print("      ");
      break;
      }
    case 3:{
      lcd.print("Corr: "); 
      lcd.print(td.current, 2); 
      lcd.print("A"); 
      lcd.print("      ");
      break;
      }
    case 4:{
      lcd.print("Volt: "); 
      lcd.print(td.bus_voltage, 1); 
      lcd.print("V"); 
      lcd.print("      ");
      break;
      }
  }
}

void displayTime() {
  static uint32_t lastElapsed = 0xFFFFFFFF;
  if (elapsedSeconds == lastElapsed) {
    return;
  }
  lastElapsed = elapsedSeconds;

  minutes = elapsedSeconds / 60;
  seconds = elapsedSeconds % 60;

  lcd.setCursor(0, 0);
  lcd.print("Time: ");

  if (minutes < 10) lcd.print('0');
  lcd.print(minutes);
  lcd.print(':');

  if (seconds < 10) lcd.print('0');
  lcd.print(seconds);

  lcd.print("       ");   // Limpiar los caracteres restantes de la línea
}

void checkTelemetryChanges(void) {
  if (screenNeedsRefresh) {
    displayScreen(currentScreen);
    screenNeedsRefresh = false;
    return;
  }

  switch (currentScreen) {

    case 0: // Speed setpoint km/h
      if (td.speed_setpoint_u != td_prev.speed_setpoint_u) {
        displayScreen(0);
      }
      break;

    case 1: // Speed setpoint rad/s
      if (td.speed_setpoint_u != td_prev.speed_setpoint_u) {
        displayScreen(1);
      }
      break;

    case 2: 
      if (td.temperature != td_prev.temperature) {
        displayScreen(2);
      break;

    case 3: 
      if (td.current != td_prev.current) {
        displayScreen(3);
      }
      break;

    case 4:
      if (td.cbus_voltage != td_prev.bus_voltage) {
        displayScreen(4);
      break;

    default:
    break;
  }
}