#include <Arduino.h>
#include <Wifi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "internet.h"
#include <ezTime.h>
#define LED 2


const char *mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char *mqtt_client_id = "senai134_esp_anajulia";
const char *mqtt_topic_sub = "senai134/vou/pub";
const char *mqtt_topic_pub = "senai134/vem/pub";

void conectaMqtt();
void retornoMqtt(char *topic, byte *payload, unsigned int lenght);

WiFiClient espClient;
PubSubClient client(espClient);

Timezone sp;

String mes;
String dia;
String hora;
String ano;

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(5000);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  conexaoWifi();

  // Sincronizar data e hora
  waitForSync();

  // Configurar fuso horário
  sp.setLocation("America/Sao_Paulo");

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(retornoMqtt);
}

void loop()
{
  reconexaoWifi();

  if (!client.connected())
  {
    conectaMqtt();
  }

  client.loop();

  // Le da serial
  if (Serial.available() > 0)
  {
    String textoDigitado = Serial.readStringUntil('\n');
    textoDigitado.trim();

    if (textoDigitado.length() == 0)
    {
      Serial.println("Msg vazia");
      return;
    }

    // Montar JSON
    JsonDocument doc;

    doc["disp"] = "Esp-anajulia";
    doc["msg"] = textoDigitado;

    // Data e hora
    const String meses[] = {
      "Janeiro", "Fevereiro", "Marco", "Abril",
      "Maio", "Junho", "Julho", "Agosto",
      "Setembro", "Outubro", "Novembro", "Dezembro"
    };

    mes = sp.dateTime("m");
    dia = sp.dateTime("d");
    ano = sp.dateTime("Y");
    hora = sp.dateTime("H:i:s");

    doc["D"] = dia;
    doc["M"] = mes;
    doc["A"] = ano;
    doc["H"] = hora;

    String stringJson;
    serializeJson(doc, stringJson);

    // Publica no MQTT
    client.publish(mqtt_topic_pub, stringJson.c_str());

    Serial.print("Enviado: ");
    Serial.println(stringJson);
  }
}

void conectaMqtt()
{
  while (!client.connected())
  {
    Serial.println("Conectando ao MQTT");

    if (client.connect(mqtt_client_id))
    {
      Serial.println("Conectado");
      client.subscribe(mqtt_topic_sub);
    }
    else
    {
      Serial.println("Falha ao se conectar");
      Serial.println(client.state());
      Serial.println("Tentando novamente em 5s");
      delay(5000);
    }
  }
}

void retornoMqtt(char *topic, byte *payload, unsigned int lenght)
{
  String msgRecebida = "";

  for (int i = 0; i < lenght; i++)
  {
    msgRecebida += (char)payload[i];
  }

  Serial.println("\nMensagem recebida:");

  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, msgRecebida);

  if (erro)
  {
    Serial.println("Erro ao decodificar JSON");
    return;
  }

  const char *dispositivo = doc["disp"];
  const char *msg = doc["msg"];
  const char *dia = doc["D"];
  const char *mes = doc["M"];
  const char *ano = doc["A"];
  const char *hora = doc["H"];

  Serial.print("ESP: ");
  Serial.println(dispositivo);

  Serial.print("Mensagem: ");
  Serial.println(msg);

  Serial.print("Data: ");
  Serial.print(dia);
  Serial.print("/");
  Serial.print(mes);
  Serial.print("/");
  Serial.println(ano);

  Serial.print("Hora: ");
  Serial.println(hora);

  // Controle do LED
  if (String(msg) == "LIGAR")
  {
    digitalWrite(LED, HIGH);
    Serial.println("LED LIGADO!");
  }
  else if (String(msg) == "DESLIGAR")
  {
    digitalWrite(LED, LOW);
    Serial.println("LED DESLIGADO!");
  }
}