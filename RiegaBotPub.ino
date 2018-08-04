#include "WiFiEsp.h" //Libreria para comunicarse facilmente con el modulo ESP01
#include "SoftwareSerial.h"
#include <TM1637.h>

// PINES
// Configuración del ESP
#define PIN_RX 11
#define PIN_TX 12
// TM1637
#define CLK 3
#define DIO 2
// 74HC595
const int latchPin = 9 ;        //Pin conectado a ST_CP of 74HC595
const int clockPin = 10;        //Pin conectado a SH_CP of 74HC595
const int dataPin = 8;          //Pin connected to DS of 74HC595
// otros
const int potenciometroMinHumedad = 4;
const int potenciometroMaxHumedad = 5;
const int botonRiegoManual = 6;
const int bomba = 7;
const int sensor = 0;
const int sensorHumedad = 1;
const int botonRiegoAuto = 5;
const int ledRiegoAuto = 4;

// Configuracion Wifi
char ssid[] = "TU_SSID";            // SSID (Nombre de la red WiFi)
char pass[] = "TU_PASSWORD";        // Contraseña
int status = WL_IDLE_STATUS;     // Estado del ESP. No tocar

// IFTTT settings
char host[] = "maker.ifttt.com";
char eventName1[]   = "riega_log";
char eventName2[]   = "nivel_deposito";
char key[] = "TU_KEY_PARA_IFTTT";

// Objetos
WiFiEspClient client;  //Iniciar el objeto para cliente 
SoftwareSerial esp8266(PIN_RX, PIN_TX);
TM1637 Display1(CLK,DIO);

// Constantes
const int nivel_minimo_deposito = 10;
const long intervaloServicioWeb = 300000; //  cinco min (5*60*1000)
//const long intervaloServicioWeb = 60000; // diez minutos (10*60*1000)
const long intervaloRiego = 60000; // un minuto (1*60*1000)
const long duracionRiego = 1000; // un segundo (1*1000)

// VARIABLES
// Avisos nivel del deposito
bool avisoNivel50Enviado = false;
bool avisoNivel20Enviado = false;
bool avisoNivel10Enviado = false;
// Control del tiempo transcurrido
long marcaTiempoAnteriorServicioWeb = 0;
long marcaTiempoAnteriorRiego = 0;
byte datosBarraLED[2];
int segundosRegados = 0;
bool riegoAutoActivado = false;
int nivel_minimo_humedad = 60;
int nivel_maximo_humedad = 70;
int valorPorcenDeposito;
int valorHumedadPorcen;
bool pendienteAlcanzarMaximo = false;

void setup()
{
  Serial.begin(9600); //Monitor serial
  esp8266.begin(9600); //ESP01
  
  iniciarWifi();
  printWifiStatus();

  //set pins to output because they are addressed in the main loop
  pinMode(latchPin, OUTPUT);
  pinMode(botonRiegoManual  , INPUT) ;
  pinMode(bomba, OUTPUT); 
  pinMode(botonRiegoAuto, INPUT);
  pinMode(ledRiegoAuto, OUTPUT);

  Display1.set();
  Display1.init() ;
}

void loop()
{ 
  
  // miro potenciometros de nivel que indican mín. y máx. a mantener en la humedad de la tierra
  comprobarNivelesMinMax();
  mostrarEnDisplay(nivel_minimo_humedad, nivel_maximo_humedad, true);
  
  // miramos el nivel de agua
  int nivel0a10 = comprobarNivelDeposito();
  mostrarEnBarraLED(nivel0a10);

  // miramos humedad en la tierra
  comprobarHumedadTierra();
 
  // leemos boton riego auto
  if(digitalRead(botonRiegoAuto) == HIGH){
    riegoAutoActivado = !riegoAutoActivado;
  }

  if(riegoAutoActivado){
    digitalWrite( ledRiegoAuto, HIGH);
    Serial.println("El riego auto esta activado...");
  }else{
    digitalWrite( ledRiegoAuto, LOW);
    Serial.println("El riego auto esta desactivado...");
  }

  // obtenemos la marca de tiempo actual
  unsigned long marcaTiempoActual = millis();

  // si el nivel de agua está por debajo del nivel mínimo, no va a regar en ningun caso
  if(valorPorcenDeposito > nivel_minimo_deposito){
      if(digitalRead(botonRiegoManual) == HIGH){ // modo manual
        riegoManual();
      }
      if(riegoAutoActivado && (valorHumedadPorcen <= nivel_minimo_humedad || pendienteAlcanzarMaximo)){
        if(marcaTiempoActual - marcaTiempoAnteriorRiego > intervaloRiego) {
          marcaTiempoAnteriorRiego = marcaTiempoActual;                
          riegoAutomatico();
          if(valorHumedadPorcen <= nivel_maximo_humedad){
           Serial.println("Máximo no alcanzado...");
           pendienteAlcanzarMaximo = true; 
          }else{
            Serial.println("Máximo alcanzado...");
           pendienteAlcanzarMaximo = false; 
          }
        }
      }
  }

  delay(5000);
  mostrarEnDisplay(0, valorHumedadPorcen, false);
  delay(5000);
  
  if(marcaTiempoActual - marcaTiempoAnteriorServicioWeb > intervaloServicioWeb || marcaTiempoAnteriorServicioWeb == 0) {
    Serial.println("Toca enviar al servicio web...");
    marcaTiempoAnteriorServicioWeb = marcaTiempoActual;                
    consumirServicio(eventName1, valorPorcenDeposito, valorHumedadPorcen, segundosRegados);
    segundosRegados = 0;

    if(valorPorcenDeposito <= 50 && !avisoNivel50Enviado){
      avisoNivel50Enviado = true;
      consumirServicio(eventName2, 50, -1, -1);
    }else if (valorPorcenDeposito <= 20 && !avisoNivel20Enviado){
      avisoNivel20Enviado = true;
      consumirServicio(eventName2, 20, -1, -1);
    }else if (valorPorcenDeposito <= 10 && !avisoNivel10Enviado){
      avisoNivel10Enviado = true;
      consumirServicio(eventName2, 10, -1, -1);
    }
  }
  }

  void consumirServicio(String evento, int valor1, int valor2, int valor3){
    Serial.println("Iniciando conexion..."); // Intentar la conexion al servidor dado
    if (client.connect(host, 80)) {
      Serial.println("Conectado al servidor");
  
      // Construimos la URL
      String url = "/trigger/";
      url += evento;
      url += "/with/key/";
      url += key;
      if(valor1 >= 0){
        url += "?value1=";
        url += valor1;  
      }
      if(valor2 >= 0){
        url += "&value2=";
        url += valor2;  
      }
      if(valor3 >= 0){
        url += "&value3=";
        url += valor3;
      }
      
      Serial.print("Solicitando URL: ");
      Serial.println(url);
  
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");
    }
    
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
  
    //Desconexion
    if (client.connected()) {
      Serial.println();
      Serial.println("Desconectando del servidor...");
      client.flush();
      client.stop();
    } 
  }

void comprobarNivelesMinMax(){
  nivel_minimo_humedad = map(analogRead(potenciometroMinHumedad), 0, 1023, 0, 100);
  nivel_maximo_humedad = map(analogRead(potenciometroMaxHumedad), 0, 1023, 0, 100);
  Serial.print("Min leido: ");
  Serial.print(nivel_minimo_humedad);
  Serial.print(" - Max leido: ");
  Serial.println(nivel_maximo_humedad); 
}

int comprobarNivelDeposito(){
  int valor0a10 = map(analogRead(sensor),10,370,0,10);
  valorPorcenDeposito = map(analogRead(sensor),10,370,0,100);
  if(valorPorcenDeposito > 100) valorPorcenDeposito = 100; // ocasionalmente puede dar valores > 100
  Serial.print("Capacidad deposito: ");
  Serial.print(valorPorcenDeposito);
  Serial.println("%");
  if(valorPorcenDeposito > 70){
    // restablecemos los avisos
    avisoNivel50Enviado = false;
    avisoNivel20Enviado = false;
    avisoNivel10Enviado = false;
  }
  return valor0a10;
}

void comprobarHumedadTierra(){
  valorHumedadPorcen = map(analogRead(sensorHumedad), 0, 1023, 100, 0);
  Serial.print("Humedad: ");
  Serial.print(valorHumedadPorcen);
  Serial.println("%"); 
}

void riegoManual(){
  Serial.println("Riego manual activado");
  while(digitalRead(botonRiegoManual) == HIGH){
    digitalWrite( bomba, HIGH) ;
    delay(500);
  }
  digitalWrite( bomba, LOW); 
}

void riegoAutomatico(){
    Serial.println("Riego automático");
    digitalWrite( bomba, HIGH);
    delay(duracionRiego);
    digitalWrite( bomba, LOW);
    segundosRegados = segundosRegados + 1; 
    Serial.println("Fin riego automático");
}

void mostrarEnDisplay(int valor1, int valor2, bool mostrarPuntos){
    int8_t Digit0 = valor1 / 10 ;
    int8_t Digit1 = valor1 %10 ;
    int8_t Digit2 = valor2 / 10 ;
    int8_t Digit3 = valor2 %10 ;
  
    if(mostrarPuntos){
      Display1.point(POINT_ON);
    }else{
      Display1.point(POINT_OFF);
    }

     Display1.display(3, Digit3);
     Display1.display(2, Digit2);
     Display1.display(1, Digit1);
     Display1.display(0, Digit0);
}

  void iniciarWifi(){
  WiFi.init(&esp8266); // inicio Wifi

  //intentar iniciar el modulo ESP
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Modulo no presente. Reinicie el Arduino y el ESP01 (Quite el cable que va de CH_PD a 3.3V y vuelvalo a colocar)");
    //Loop infinito
    while (true);
  }

  //Intentar conectar a la red wifi
  while ( status != WL_CONNECTED) {
    Serial.print("Intentando conectar a la red WiFi: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }
}

  void printWifiStatus()
{
  // SSID al que nos hemos conectado
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // la IP asignada
  IPAddress ip = WiFi.localIP();
  Serial.print("IP: ");
  Serial.println(ip);

  // fuerza de la señal
  long rssi = WiFi.RSSI();
  Serial.print("Señar recibida (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void mostrarEnBarraLED(int de0a10) {
  datosBarraLED[0] =B00000000;
  datosBarraLED[1] =B00000000;

  for (int i=0; i< min(de0a10, 8); i++)  {
    bitSet(datosBarraLED[0], i);
  }

  for (int j=0; j< de0a10 - 8; j++) {
    bitSet(datosBarraLED[1], j);
  }

  digitalWrite(latchPin, 0);
  shiftOut(dataPin, clockPin, datosBarraLED[1]);
  shiftOut(dataPin, clockPin, datosBarraLED[0]);
  digitalWrite(latchPin, 1);
}

// función para enviar un byte al 74HC595
void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
  // This shifts 8 bits out MSB first, 
  //on the rising edge of the clock,
  //clock idles low

  //internal function setup
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  //for each bit in the byte myDataOut�
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights. 
  for (i=7; i>=0; i--)  {
    digitalWrite(myClockPin, 0);

    //if the value passed to myDataOut and a bitmask result 
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000 
    // and proceeds to set pinState to 1.
    if ( myDataOut & (1<<i) ) {
      pinState= 1;
    }
    else {  
      pinState= 0;
    }

    //Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
    //register shifts bits on upstroke of clock pin  
    digitalWrite(myClockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
  }

  //stop shifting
  digitalWrite(myClockPin, 0);
}
