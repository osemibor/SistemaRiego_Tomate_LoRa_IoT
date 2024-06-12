/* SISTEMA DE RIEGO CON LORA HELTEC*/
/* OSCAR EMILIO IBAÑEZ ORTIZ*/
/* UNIVERSIDAD DE PAMPLONA*/

/* Librerias de HELTEC */
#include "LoRaWan_APP.h"

/* Librerias Externas a HELTEC */
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <WiFi.h>

/* Configuracion de pines GPIO */
#include <Config.h>

uint32_t devAddr =  ( uint32_t )0x007e6ae1;

#define DHTPIN DATA_DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);



/* Configuracion de la pantalla OLED */
#define SCREEN_WIDTH 128 // Pantalla OLED ancho, en pixeles
#define SCREEN_HEIGHT 64 // Pantalla OLED largo, en pixeles
#define SCREEN_ADDRESS 0x3C // Direccion del puerto I2C para la OLED
Adafruit_SSD1306 UIdisplay(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, RST_OLED); // inicializar la OLED

/* Configuracion del radio lora node chip SX1262 */
#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             21        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;
int16_t rssi,rxSize;
bool lora_idle=true;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );

/*=====================================================================================*/




void ShowData(String sensor_1, String sensor_2, String sensor_3,String mac,String valvula)
{
  UIdisplay.clearDisplay();
  UIdisplay.setTextSize(1);
  UIdisplay.setTextColor(SSD1306_WHITE);
  UIdisplay.setCursor(0, 0);
  UIdisplay.println(F("SENSORES ESTACION 2"));
  UIdisplay.println(("MAC="+mac));
  UIdisplay.println(F("_____________________"));
  UIdisplay.println(("TEMP:    "+sensor_1));
  UIdisplay.println(F("---------------------"));
  UIdisplay.println(("HUMEDAD: "+sensor_2));
  UIdisplay.println(F("---------------------"));
  UIdisplay.println(("LLUVIA: "+sensor_3+" | VAL:"+valvula));
  UIdisplay.display();
}

void ShowLoRa()
{
  UIdisplay.clearDisplay();
  UIdisplay.setTextSize(1);
  UIdisplay.setTextColor(SSD1306_WHITE);
  UIdisplay.setCursor(0, 0);
  UIdisplay.println(("Freq: "+String(RF_FREQUENCY)+" Hz"));
  UIdisplay.println(("Pot:  "+String(TX_OUTPUT_POWER)+" dbm"));
  // [0: 125 kHz,
  //  1: 250 kHz,
  //  2: 500 kHz,
  //  3: Reserved]
  if(LORA_BANDWIDTH == 0)
  {
    UIdisplay.println(("AB:   125kHz"));
  }
  else
  {
    if(LORA_BANDWIDTH == 1)
    {
      UIdisplay.println(("AB:   250kHz"));
    }
    else
    {
      if(LORA_BANDWIDTH == 2)
      {
        UIdisplay.println(("AB:   500kHz"));
      }
      else
      {
        if(LORA_BANDWIDTH == 3)
        {
          UIdisplay.println(("AB:   Reserved"));
        }
        else
        {
          UIdisplay.println(("AB:   "));
        }
      }
    }
  }
  UIdisplay.println(("SF:   SF"+String(LORA_SPREADING_FACTOR)));
  // [1: 4/5,
  //  2: 4/6,
  //  3: 4/7,
  //  4: 4/8]
  if(LORA_CODINGRATE == 0)
  {
    UIdisplay.println(("CR:   4/5"));
  }
  else
  {
    if(LORA_CODINGRATE == 1)
    {
      UIdisplay.println(("CR:   4/6"));
    }
    else
    {
      if(LORA_CODINGRATE == 2)
      {
        UIdisplay.println(("CR:   4/7"));
      }
      else
      {
        if(LORA_CODINGRATE == 3)
        {
          UIdisplay.println(("CR:   4/8"));
        }
        else
        {
          UIdisplay.println(("CR:   "));
        }
      }
    }
  }
  UIdisplay.println(("PL:   "+String(LORA_PREAMBLE_LENGTH)));
  UIdisplay.display();
}


void setup() {
  /* Configuracion de la Interfaz de depuracion serial */
  Serial.begin(115200);

  /* LED Indicador de envio de mensajes */
  pinMode(LED_BOARD,OUTPUT);

  /* Inicia el sensor DHT11 */
  dht.begin();

  /* Actuador Valvula */
  pinMode(VALVULA_P,OUTPUT);
  digitalWrite(VALVULA_P,0);

  /* Configuracion del tipo de board de la marca HELTEC */
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
	

  /* Inicializacion de eventos y configuracion del Radio Lora*/
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout; 
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );


  /* Configuracion de la OLED */
  Wire.begin(SDA_OLED, SCL_OLED); // Configurar los GPIO para la Comunicacion I2C

  if(!UIdisplay.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  UIdisplay.display();
  delay(2000); // Pause for 2 seconds
 
  // Clear the buffer
  UIdisplay.clearDisplay();
  
  ShowLoRa();
  delay(3000); // Pause for 2 seconds

  

}

int HD_38_P;
float DHT_TEMP;
int A_LLUVIA;
String E_LLUVIA;
int EBool_LLUVIA;
int VALVULA = 0;
String E_VALVULA;

void Medicion()
{
  HD_38_P = analogRead(D_HD38);
  HD_38_P = constrain(HD_38_P,0,4095);
  HD_38_P = map(HD_38_P,0,4095,100,0);
  Serial.print(F("Humedad del suelo: "));
  Serial.println(HD_38_P);
  DHT_TEMP = dht.readTemperature();
  Serial.print(F("Temperature: "));
  Serial.println(DHT_TEMP);
  A_LLUVIA = analogRead(A_SEN0121);
  Serial.print(F("lluvia: "));
  Serial.println(A_LLUVIA);
  E_LLUVIA;
  EBool_LLUVIA;
  if(A_LLUVIA > 3000)
  {
    E_LLUVIA = "SI";
    EBool_LLUVIA = 1;
  }
  else
  {
    E_LLUVIA = "NO";
    EBool_LLUVIA = 0;
  }
}

void Actuador()
{
  if( (0 <= HD_38_P && HD_38_P <= 20) && (EBool_LLUVIA == 0) ) // 0% < humedad < 40% y no hay lluvia entonces se riega
  {
    VALVULA=1;
    E_VALVULA= "ON";
    // activa la valvula para regar el suelo
    digitalWrite(VALVULA_P,VALVULA);
  }
  else
  {
    VALVULA=0;
    E_VALVULA= "OFF";
    digitalWrite(VALVULA_P,VALVULA);
  }
}

String obtenerMAC() {
  // Declarar un arreglo para almacenar la dirección MAC
  byte mac[6];
  // Obtener la dirección MAC de la interfaz Wi-Fi
  WiFi.macAddress(mac);    
  // Crear una cadena para almacenar la MAC en formato legible
  String macString = "";
  for (int i = 0; i < 6; i++) {
      // Convertir cada byte de la MAC a una cadena hexadecimal de dos caracteres
      macString += String(mac[i], HEX);
      // Agregar dos puntos entre cada parte de la MAC, excepto al final
      if (i < 5) {
          macString += ":";
      }
  }
  return macString;
}


void loop() {
  Medicion();
  Actuador();
  String MAC = obtenerMAC();
  ShowData(String(DHT_TEMP)+"C",String(HD_38_P)+"%",E_LLUVIA,MAC,E_VALVULA);
  TXSender(MAC+"@"+String(DHT_TEMP)+","+String(HD_38_P)+","+String(EBool_LLUVIA)+","+String(VALVULA));
  //ShowData("33.50C",String(HD_38_P)+"%",E_LLUVIA,MAC,E_VALVULA);
  //TXSender(MAC+"@"+String(33.50)+","+String(HD_38_P)+","+String(EBool_LLUVIA)+","+String(VALVULA));
  

  
  
}

void TXSender(String datos)
{
  if(lora_idle == true)
	{
    digitalWrite(LED_BOARD,!lora_idle);
    delay(5000);
		sprintf(txpacket,datos.c_str(),txNumber);  //inicia un paquete
   
		Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",txpacket, strlen(txpacket));

		Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //enviando paquete
    lora_idle = false;
    digitalWrite(LED_BOARD,!lora_idle);
	}
  Radio.IrqProcess( );
}

void OnTxDone( void )
{
	Serial.println("TX done......");
	lora_idle = true;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.println("TX Timeout......");
    lora_idle = true;
}

