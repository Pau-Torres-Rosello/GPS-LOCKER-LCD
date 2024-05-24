//Libreria para comunicación I2C
#include <Wire.h>

//Libreria para tener el control de la pantalla LCD
#include <LiquidCrystal_I2C.h>

//Libreria para la conexion de los diferentes satelites y que nos
//de las cordenadas nuestras
#include <TinyGPS++.h>

//Libreria para conexion del WIFI entre el dispositivo 
// y el mobil
#include <WiFi.h>

//Libreria para el funcionamiento de la pagina web
#include <WebServer.h>

//PINES que se utilizan para recibir los datos
//de la antena NEO-6M
#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);
char datoCmd = 0;
#define NMEA 0


//Iniciamos el GPS
TinyGPSPlus gps;

//Le decimos cual sera la WIFI / CONTRASEÑA
const char* ssid = "GPS-LOCKER";
const char* password = "12345678";

bool wifiConectado = false; // Bandera para rastrear el estado de la conexión WiFi


//Iniciamos el Server de la 
//página web
WebServer server(80);

// Inicializar el objeto de la pantalla LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4); // Dirección I2C, 20 columnas, 4 filas

void setup() {
  //Inizializamos la lcd
  lcd.begin();
  lcd.backlight(); // Encender la retroiluminación de la pantalla LCD
  Serial.begin(115200);
  
  //Inicializamod la antena Satelite
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  delay(2000);

  // Conexión a la red WiFi
  WiFi.begin(ssid, password);
  Serial.println("Conectando a la red WiFi...");
  lcd.print("CONECTANDO A LA RED WIFI ESPERE...");

  //Comprobador de tiempo para conexión WIFI
  unsigned long tiempoInicio = millis(); // Obtener el tiempo actual
  
  // Esperar hasta 20 segundos para la conexión WiFi, Si 
  // No conecta no pasa nada porque los datos
  // Los dara en la WEB - LCD I2C
  while (millis() - tiempoInicio < 20000) { 
    if (WiFi.status() == WL_CONNECTED) {
      wifiConectado = true;
      break;
    }
    delay(1000);
    Serial.println("Conectando...");
  }

   //Conexión exitosa en la WIFI y da 
   //la IP del server para la web
  if (wifiConectado) {
    Serial.println("Conexión exitosa");
    Serial.print("Dirección IP asignada: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.setCursor(0, 0); // Establecer el cursor en la fila 1, columna 1
    lcd.print("BUSCA LA IP EN EL   ");
    lcd.setCursor(0, 1); // Establecer el cursor en la fila 2, columna 1
    lcd.print("NAVEGADOR WEB");
    lcd.setCursor(0, 2); // Establecer el cursor en la fila 3, columna 1
    lcd.print(WiFi.localIP());
    delay(7000);

   
  } else {
    Serial.println("Error: No se pudo conectar a la red WiFi");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NO HEMOS PODIDO     CONECTAR A INTERNET ");
    
  }

  // Configuracion de la web por HTML5
server.on("/", HTTP_GET, []() {
    String content = "<html><head><title>ESP32 GPS</title>";
    content += "<meta charset=\"utf-8\">"; // Configuración de la codificación de caracteres
    content += "<script src=\"https://maps.googleapis.com/maps/api/js?key=AIzaSyBRuR-RiLPl6CgRpv4iKj1o5qgfPneKENw\"></script>";//KEY API GOOGLE
    content += "<script>function initMap() {var map = new google.maps.Map(document.getElementById('map'), {zoom: 70, center: {lat: ";//DATOS DE LA WEB
    content += String(gps.location.lat(), 40);
    content += ", lng: ";
    content += String(gps.location.lng(), 40);
    content += "}, mapTypeId: 'satellite'});"; // Cambiar el tipo de mapa a satelital
    content += "var marker = new google.maps.Marker({position: {lat: ";
    content += String(gps.location.lat(), 40);
    content += ", lng: ";
    content += String(gps.location.lng(), 40);
    content += "}, map: map});}</script>";
    content += "<script>setTimeout(function(){ location.reload(); }, 15000);</script>"; // Recargar la página cada 15 segundos a nuestras coordenadas
    content += "<style>";
    content += "body {";
    content += "background: -webkit-linear-gradient(90deg, #06071d,#06071d,#1a1f8d,#5a60e0,#d4d6fa);";//Colores de fondo
    content += "background: linear-gradient(90deg, #06071d,#06071d,#1a1f8d,#5a60e0,#d4d6fa);";//Colores de fondo
    content += "color: white;";
    content += "text-align: center;";
    content += "}"; // Fondo con gradiente y texto blanco centrado
    //PARAMETROS PARA EL MAPA DE LA API DE GOOGLE MAPS PRIVADA
    content += "#map { height: 80vh; width: 100%; }"; // Estilo para el mapa que ocupa el 70% de la altura de la pantalla y todo el ancho
    content += "h1 { font-weight: bold; color: orange; }"; // Estilo para hacer negrita el texto en los títulos y cambiar el color a naranja
    content += "p { font-weight: bold; }"; // Estilo para hacer negrita el texto en los párrafos
    content += "</style>";
    content += "</head><body onload=\"initMap()\"><h1>LOCALIZACIÓN DE LA UBICACION TRACKER</h1>";
    content += "<p>Latitud: ";
    content += gps.location.lat(), 40;
    content += " Km/h</p><p>Longitud: ";
    content += gps.location.lng(), 40;
    content += "</p><p>Velocidad: ";
    content += gps.speed.kmph();
    content += " Km/h</p><p>Satélites: ";
    content += gps.satellites.value();
    content += "</p><p>Altitud: ";
    content += gps.altitude.meters();
    content += " m</p><p>Fecha: ";
    content += gps.date.year();
    content += "/";
    content += gps.date.month();
    content += "/";
    content += gps.date.day();
    content += "</p>"; // Eliminado la sección de la hora
    content += "<div id=\"map\"></div></body></html>";
    server.send(200, "text/html", content);
});


  // Iniciar el servidor
  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  //FUNCIONAMIENTO EN EL SATELITE
  //PARA RECIBIRDATOS Y ENVIARLOS
  
  if (NMEA) {
    while (neogps.available()) {
      datoCmd = (char)neogps.read();
      Serial.print(datoCmd);
    }
  } else {
    boolean newData = false;
    for (unsigned long start = millis(); millis() - start < 10000;) {
      while (neogps.available()) {
        if (gps.encode(neogps.read())) {
          newData = true;
        }
      }
    }

    if (newData == true) {
      newData = false;
      Serial.println(gps.satellites.value());
      Visualizacion_Serial();
    }
  }

  // Manejar las solicitudes de la página web
  server.handleClient();
}
//Funcion para el control de la LCD I2C
void Visualizacion_Serial(void) {
  if (gps.location.isValid() == 1) {
    // Mostrar en la pantalla LCD 
    //Los distintos valores ""
    lcd.clear(); // Limpiar la pantalla
    lcd.setCursor(0, 0);
    lcd.print("LATITUD: ");
    lcd.print(gps.location.lat(), 6);

    lcd.setCursor(0, 1);
    lcd.print("LONGITUD: ");
    lcd.print(gps.location.lng(), 6);

    lcd.setCursor(0, 2);
    lcd.print("VELOCIDAD: ");
    lcd.print(gps.speed.kmph());

    lcd.setCursor(0, 3);
    lcd.print("SATELITES: ");
    lcd.print(gps.satellites.value());
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NO DETECTAMOS GPS ");
  }
}
