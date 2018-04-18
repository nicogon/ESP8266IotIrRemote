


#include <Time.h>
#include <TimeLib.h>

#include <Timezone.h>


#include <SPIFFSReadServer.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRsend.h>
#include <NTPClient.h>
#include <Ticker.h>
#include <PubSubClient.h>


Ticker blinker;


// Define NTP properties
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 24 * 60 * 60 * 1000    // In miliseconds, once a day
#define NTP_ADDRESS  "ca.pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)
unsigned long epochTime;
String date;
String t;
const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"} ; const bool show24hr = true;
// Then convert the UTC UNIX timestamp to local time
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  //UTC - 5 hours - change this as needed
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -240};   //UTC - 6 hours - change this as needed
Timezone usEastern(usEDT, usEST);

// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);




decode_results results;

const byte DNS_PORT = 53;
DNSServer dnsServer;
IPAddress apIP(192, 168, 1, 1);



//const char* ssid = "nico4";
//const char* password = "kokokoko";


const char* ssid;
const char* password;

static time_t local;

ESP8266WebServer server(80);

WiFiClient wifiClient;





int contador = 0;
String  resultado = "";
//const int led = 13;

#define MIN_UNKNOWN_SIZE 40

File f;

#define CAPTURE_BUFFER_SIZE 1024
#define TIMEOUT 10U
IRrecv irrecv(D7, CAPTURE_BUFFER_SIZE, TIMEOUT, true);

//IRrecv irrecv(D7);

#define IR_LED D8  // ESP8266 GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(IR_LED);  // Set the GPIO to be used to sending the message.


void imprimirHora()
{
  // convert received time stamp to time_t object
  local = usEastern.toLocal(timeClient.getEpochTime());

  // now format the Time variables into strings with proper names for month, day etc
  date += days[weekday(local) - 1];
  date += ", ";
  date += months[month(local) - 1];
  date += " ";
  date += day(local);
  date += ", ";
  date += year(local);

  // format the time to 12-hour format with AM/PM and no seconds
  if (show24hr) {
    t += hour(local);
  } else {
    t += hourFormat12(local);
  }
  // blinking colon
  if (second(local) & 1) {
    t += ":";
  } else {
    t += " ";
  }
  if (minute(local) < 10) // add a zero if minute is under 10
    t += "0";
  t += String(minute(local)) + " " + String(second(local));

  /*if (!show24hr) {
    t += " " +String(second(local));
    t += ampm[isPM(local)];
    }*/

  // Display the date and time
  Serial.println("");
  Serial.print("Local date: ");
  Serial.print(date);
  Serial.println("");
  Serial.print("Local time: ");
  Serial.print(t);

  date = ""; t = "";


}



void guardarRaw(String archivo) {

    // Output RAW timing info of the result.
    Serial.println(resultToTimingInfo(&results));
    yield();  // Feed the WDT (again)

    // Output the results as source code
    Serial.println(resultToSourceCode(&results));

    
  String output = "";
  File f = SPIFFS.open(archivo + ".code", "w");
  if (!f) {
    Serial.println("file creation failed");
  } else {
    Serial.println("guardo " + uint64ToString(results.rawlen - 1, 10));
    f.println(uint64ToString(results.rawlen - 1, 10));
    for (uint16_t i = 1; i < results.rawlen; i++) {
      uint32_t usecs;
      for (usecs = results.rawbuf[i] * RAWTICK;
           usecs > UINT16_MAX;
           usecs -= UINT16_MAX) {
        f.println(uint64ToString(UINT16_MAX));
        f.println("0");
      }
      f.println(uint64ToString(usecs, 10));
    }
    f.flush();
  }
  f.close();
}

//F CLOSE???????? 


void  leerYTransmitirRaw(String archivo) { 
  f = SPIFFS.open( archivo + ".code", "r");
  if (!f) {
    Serial.println("no existe el archivo que queres abrir");
  } else {

    int x = f.readStringUntil('\n').toInt();

    uint16_t rawData[x];

    for (int i = 0; i < x; i++) {
      rawData[i] = f.readStringUntil('\n').toInt();
    }
    Serial.println("Transmitiendo: " + String(x) + " bytes");
    irsend.sendRaw(rawData, x, 38);
  }
  f.close();
}



void handleStatus() {

  server.send(200, "text", "{\"wifi\":" + String((leer("ssid") == "") ? "true" : "true") + ",\"ir\":"+String((SPIFFS.open("PRENDER.code", "r")&&SPIFFS.open("APAGAR.code", "r"))? "true" : "false" )+"}");

  //false true fase

}

/*
 * 
 * 
String getValue(String data, char separator, int index)
{
  int maxIndex = data.length() - 1;
  int j = 0;
  String chunkVal = "";

  for (int i = 0; i <= maxIndex && j <= index; i++)
  {
    chunkVal.concat(data[i]);

    if (data[i] == separator)
    {
      j++;

      if (j > index)
      {
        chunkVal.trim();
        return chunkVal;
      }

      chunkVal = "";
    }
    else if ((i == maxIndex) && (j < index)) {
      chunkVal = "";
      return chunkVal;
    }
  }
}

  Dir dir = SPIFFS.openDir("");
  while (dir.next()) {

    String lectura = dir.fileName();


    if ( getValue(lectura, '.', 1) == "code") {
      String valor = getValue(lectura, '.', 0);
      respuesta += "<tr>\r\n        <td>" + valor.substring(0, valor.length() - 1) + "</td>\r\n        <td><a class=\"btn btn-sm btn-primary\" href=\"/eliminarcodigo?codigo=" + valor.substring(0, valor.length() - 1) + "\">Eliminar</a></td>\r\n      </tr>\r\n\r\n";
    }

  }

*/


void handleGuardarCodigo() {

  if (server.args() == 1) {
  
    while (irrecv.decode(&results)) {
      irrecv.resume();
      yield();
    } //Borro el buffer
      Serial.println("recibi codigo " + server.arg(0));


    //int timeout = 0;
    long now = millis();
    while (!irrecv.decode(&results) && ((millis()-now)<15000)) {
      //delay(500);
      yield();
     // timeout++;
    } //Espero codigo


    if ((millis()-now)>15000) {

      server.send(200, "text/plain", "No se detecto codigo");
      Serial.println("Error timeout IR");
    } else {
      delay(300);
      guardarRaw(server.arg(0));

      server.send(200, "text/plain", "ok");


      Serial.println("codigo guardado");
    }

  } else {
    server.send(200, "text/plain", "Error: No se registra el codigo a guardar");
    Serial.println("no ingresaste codigo");
  }

}


void handleGuardarWifi() {

  if (server.args() == 2) {
    server.send(200, "text", "ok");
    Serial.println("wifi" + server.arg(0) + " pass: " + server.arg(1));
    guardar("ssid", server.arg(0));
    guardar("password", server.arg(1));
    delay(1000);
    WiFi.mode(WIFI_OFF);
    delay(100);
    ESP.reset();

  } else {
    server.send(200, "text", "error parametros");
    Serial.println("error en los parametros");
  }

}




void handleEmitirCodigo() {

  if (server.args() == 1) {
    server.send(200, "text/plain", "ok");
    Serial.println("emitir codigo " + server.arg(0) + " valor: ");

    leerYTransmitirRaw(server.arg(0));


    /*
     * 
    Para despues: hacer que el toggle guarde el estado
    
    if(server.arg(0)=="PRENDER"){guardar("STATUS","ON");}
    if(server.arg(0)=="APAGAR"){guardar("STATUS","ON");}

    */

    
  } else {
     server.send(200, "text/plain", "no ingresaste codigo");
    Serial.println("no ingresaste codigo");
  }

}



void handleWifiList() {

  String response = "{";

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {

    response += "\"" + String(i) + "\":\"" + WiFi.SSID(i) + "\"";
    if (i != n - 1) response += ",";

    //</td><td>"+WiFi.RSSI(i)+"</td><td>"+((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "Open" : "Wpa2")+"</td><td><a class='btn btn-sm btn-primary' href='#fork'>Seleccionar</a></td></tr>";
    delay(10);
  }

  response += "}";

  server.send(200, "text/html", response);


}




void handleNotFound() {
  // digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  // digitalWrite(led, 0);
}




void guardar(String archivo, String valor) {
  File f = SPIFFS.open(archivo + ".txt", "w");
  if (!f) {
    Serial.println("file creation failed");
  } else {
    f.println(valor);
    f.flush();
  }
  f.close();
}

String  leer(String archivo) {
  f = SPIFFS.open(archivo + ".txt", "r");
  if (!f) {
    Serial.println("no existe el archivo que queres abrir");
    f.close();
    return "";
  } else {
    String temp = f.readStringUntil('\n');
    f.close();
    return temp == "\r" ? "" : temp; //aca porque archivo vacio devuelve \r

  }
}






void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Llego mensaje mqqt");
  String mensaje = "";
  for (int i = 0; i < length; i++) {
    mensaje += ((char)payload[i]);
  }
  leerYTransmitirRaw(mensaje);

}

PubSubClient client("m12.cloudmqtt.com", 12178, callback, wifiClient);



void setup(void) {

  pinMode (D5, INPUT);
  pinMode (D6, OUTPUT);
  irrecv.setUnknownThreshold(MIN_UNKNOWN_SIZE);
  irrecv.enableIRIn();  // Start the receiver
  irsend.begin();
  Serial.begin(115200);
  //Serial.setDebugOutput(true);

  bool result = SPIFFS.begin();                 //Inicio SPIF
  Serial.println("SPIFFS opened: " + result);


  //convercion string to char*
  char ssid[leer("ssid").length() + 1];
  leer("ssid").toCharArray(ssid, leer("ssid").length());
  char password[leer("password").length() + 1];
  leer("password").toCharArray(password, leer("password").length());






  Serial.println("ssid: " + leer("ssid"));
  Serial.println("passs: " + leer("password"));


  digitalWrite(D6, HIGH);
  if (digitalRead(D5) == HIGH || leer("ssid") == "") { //Esta tocando boton
    guardar("ssid", "");
        SPIFFS.remove("PRENDER.code");
         SPIFFS.remove("APAGAR.code");
    digitalWrite(D6, LOW);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("IOT CONFIG WIFI");
    delay(1000);
  } else {

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");
    Serial.println("Conectando");

    while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(D6, HIGH);
      delay(300);
      digitalWrite(D6, LOW);
      delay(300);
      Serial.print(".");
    }


    // update the NTP client and get the UNIX UTC timestamp
    timeClient.update();


    blinker.attach(10, imprimirHora);



    if (WiFi.status() == WL_CONNECTED) {


      // if (client.connected())

    }

  }


  /*Chequeo de espacio disponible en spif
    FSInfo info;
    SPIFFS.info(info);
    Serial.println("total "+String(info.totalBytes));
    Serial.println("usados "+String(info.usedBytes));
  */

  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println("");
  Serial.print("Conectado a");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("iot")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
  }

  server.serveStatic("/estilos.css", SPIFFS, "/estilos.css");
  server.serveStatic("/codigo.js", SPIFFS, "/codigo.js");

  server.serveStatic("/", SPIFFS, "/index.html");

  server.serveStatic("/fontello.woff2", SPIFFS, "/fontello.woff2");
  server.serveStatic("/fontello.woff", SPIFFS, "/fontello.woff");
  server.serveStatic("/fontello.ttf", SPIFFS, "/fontello.ttf");
  server.serveStatic("/imagen.png", SPIFFS, "/imagen.png");



  server.on("/wifilist", handleWifiList);



  server.on("/status", handleStatus);

  server.on("/guardarwifi", handleGuardarWifi);

  server.on("/emitir", handleEmitirCodigo);

  server.on("/guardarcodigo", handleGuardarCodigo);


  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");


}
int lastReconnectAttempt = 0;
void loop(void) {

  dnsServer.processNextRequest();
  server.handleClient();

  if (!client.connected()) {

    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    client.loop();
  }


}




boolean reconnect() {
  if (client.connect("arduinoClient2", "vtashoan", "kqTIW7E3vtiH")) {
    Serial.println("Connected to MQTT server");
    client.subscribe("/codigo");
    // client.publish("mm/p", "Message to MQTT");
  }
  return client.connected();
}



