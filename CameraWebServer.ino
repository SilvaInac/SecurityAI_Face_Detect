//Programa: NodeMCU e MQTT - Controle e Monitoramento IoT
//Autor: Pedro Bertoleti

#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
#include "camera_pins.h"
// ===========================
// Defines MQTT config
// ===========================
#define ID_MQTT  "C115CarlosFrancisco" 
#define TOPIC_SUBSCRIBE "GETDETECTINFO"
#define TOPIC_PUBLISH   "FACEDETECT1"

extern bool detectFace;

int flashLight = 4;

// ===========================
// Enter your MQTT credentials
// ===========================
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

const char* BROKER_MQTT = "test.mosquitto.org"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT
char EstadoSaida = '0';  //variável que armazena o estado atual da saída


// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid = "Carlos Eduardo";
const char* password = "testes123";


//Prototypes
void startCameraServer();
void initWifi();
void initMQTT();
void reconnectWifi();
void reconnectMQTT();
void mqtt_callback(char* topic, byte* payload,unsigned int length);
void verifyConnections();
void initOutput();

void initWifi()
{
  delay(10);
  reconnectWifi();
}
void initMQTT()
{
  MQTT.setServer(BROKER_MQTT,BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void mqtt_callback(char* topic, byte* payload,unsigned int length){
  String msg;

  for (int i=0;i<length;i++)
  {
    char c = (char)payload[i];
    msg +=c;
  }

  if(msg.equals("true"))
  {
    Serial.println("Ligando");
    digitalWrite(flashLight, HIGH);
  }
  if(msg.equals("false"))
  {
    Serial.println("Desligando");
    digitalWrite(flashLight, LOW);
  }

  
  
}

void reconnectMQTT()
{
  while(!MQTT.connected())
  {
    Serial.print("Try to connect MQTT ");
    Serial.println(BROKER_MQTT);
    if(MQTT.connect(ID_MQTT))
    {
      Serial.println("Connected sucessfully on MQTT");
      MQTT.subscribe(TOPIC_SUBSCRIBE);
    }
    else 
    {
      Serial.println("ERROR to connect on Broker");
      Serial.println("New Try on 2 seconds");
      delay(2000);
    }
  }
}

void reconnectWifi()
{
  if(WiFi.status() == WL_CONNECTED)
  {
    return;
  }

  WiFi.begin(ssid,password);
  
  while(WiFi.status()!= WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  WiFi.setSleep(false);
  Serial.println();
  Serial.print("Connected on ");
  Serial.println(ssid);
}

void verifyConnections()
{
  if(!MQTT.connected())
  {
    reconnectMQTT();
  }
  reconnectWifi();
}

void sendOutputMQTT(void)
{
  if(detectFace == true)
  {
    MQTT.publish(TOPIC_PUBLISH,"true");
    detectFace = false;
  }
  else if(detectFace == false)
  {
    MQTT.publish(TOPIC_PUBLISH,"false");
  }
  Serial.println("- Estado da saida enviado ao broker!");
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

  initWifi();
  startCameraServer();
  initMQTT();
  pinMode(flashLight,OUTPUT);

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  verifyConnections();
  sendOutputMQTT();
  MQTT.loop();
  delay(200);

  
}
