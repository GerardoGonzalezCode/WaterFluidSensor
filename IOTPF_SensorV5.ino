#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <BH1750.h>
#include <Wire.h>

volatile int NumPulsos; //variable para la cantidad de pulsos recibidos
int PinSensor = 14;    //Sensor conectado en el pin A0
float factor_conversion=7.11; //para convertir de frecuencia a caudal
float volumen=0;
long dt=0; //variación de tiempo por cada bucle
long t0=0; //millis() del bucle anterior

const char* ssid = "MEGACABLE-1A9G";
const char* password =  "URswM729";
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char* clientid = "JerryESP32";
char jsonOutput[256];

WiFiClient espClient;
PubSubClient client(espClient);

void ContarPulsos ()  
{ 
  NumPulsos++;  //incrementamos la variable de pulsos
} 

void setup() {

  Serial.begin(115200);
  pinMode(PinSensor, INPUT); 
  attachInterrupt(0,ContarPulsos,RISING);//(Interrupción 0(Pin2),función,Flanco de subida)
  Serial.println ("Envie 'r' para restablecer el volumen a 0 Litros"); 
  t0=millis();

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 Wire.begin();

  client.setServer(mqttServer, mqttPort);
}

int ObtenerFrecuecia() 
{
  int frecuencia;
  NumPulsos = 0;   //Ponemos a 0 el número de pulsos
  interrupts();    //Habilitamos las interrupciones
  delay(1000);   //muestra de 1 segundo
  noInterrupts(); //Deshabilitamos  las interrupciones
  frecuencia=NumPulsos; //Hz(pulsos por segundo)
  return frecuencia;
}

void loop() {
    while (!client.connected()) {
    Serial.println("Conectando a Broquer MQTT...");
    if (client.connect(clientid)) {
      Serial.println("connected");
      Serial.print("Return code: ");
      Serial.println(client.state());

    } else {

      Serial.print("conexion fallida ");
      Serial.print(client.state());
      delay(2000);

    }
  }
  client.loop();
  int lec = analogRead(34);
  Serial.print("Read: ");
  Serial.println(lec);
  delay(1000);

  float frecuencia=ObtenerFrecuecia(); //obtenemos la frecuencia de los pulsos en Hz
  float caudal_L_m=frecuencia/factor_conversion; //calculamos el caudal en L/m
  dt=millis()-t0; //calculamos la variación de tiempo
  t0=millis();
  volumen=volumen+((caudal_L_m/60)*(dt/1000))+0.5; // volumen(L)=caudal(L/s)*tiempo(s)

  Serial.print ("Caudal: "); 
  Serial.print (caudal_L_m,3); 
  Serial.print ("L/min\tVolumen: "); 
  Serial.print (volumen,3); 
  Serial.println (" L");

  const size_t CAPACITY = JSON_OBJECT_SIZE(5);
  StaticJsonDocument<CAPACITY> doc;
  JsonObject object = doc.to<JsonObject>();
  object["Id"] = disId;
  object["GroundHumidity"] = gh;
  object["EnvironmentHumidity"] = h;
  object["Temperature"] = t;
  object["Lighting"] = lux;
  serializeJson(doc, jsonOutput);
  Serial.println(String(jsonOutput));
  client.publish( "/nodejs/mqtt/ifarm", jsonOutput);
  delay(5000);
}
