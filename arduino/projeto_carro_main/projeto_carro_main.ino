/**
 *  Copyright (c) 2023, Elieder Sousa
 *  eliedersousa<at>gmail<dot>com
 *  
 *  Distributed under the MIT license. 
 *  See <license.txt> file for details.
 *  
 *  @file projeto_carro_main.ino
 *  @brief Programa para Arduino UNO do painel do carro.
 */  
#include <OneWire.h>  
#include <DallasTemperature.h>

#define SENSOR_CORRENTE     A0	    // (ACS712ELC-30A)
#define SENSOR_TENSAO       A1	    // (2 resistores de 5100ohm, 33000ohm; total 381000ohm)
#define SENSOR_TEMPERATURA  2		    // (DS18B20)
#define SENSOR_VELOCIDADE   3		    // (KY-025 Reed Switch)

#define SENSOR_MEDIA_DELAY	3		    // TODO: Valor usado para sensores que necessitam de uma média. Revisitar este valor.
#define FPS					        100		  // TODO: Quantos frames por segundo seria o ideal? Revisitar este valor.
#define BAUD_RATE			      115200	// TODO: Revisitar este valor de baud-rate.

String jsonString = "";				      // Usada no envio da string JSON.
float TWOPI = 2 * 3.14159265;       // 2*PI
float resolucao = 5.0 / 1024.0;
unsigned long tempo = 0;
bool flag_velocidade = false;
int DELAY_FPS = 1000/FPS;
float raio    = 0.23;                // Raio da roda original: 0.23m

OneWire oneWire(SENSOR_TEMPERATURA);
DallasTemperature sensors(&oneWire);

/**
 * @brief Retorna a temperatura lida pelo sensor.
 * @return float
 */
float getTemperatura() {
	sensors.requestTemperatures();
	return sensors.getTempCByIndex(0);
}

/**
 * @brief 	Retorna a velocidade do motor com base no sensor de velocidade.
 * @return	float A velocidade
 */
float getVelocidade( float raio ) {
  if( digitalRead(SENSOR_VELOCIDADE) == HIGH ) {
    if( flag_velocidade == false ) {
      flag_velocidade = true;
      if( tempo == 0 ) {
        tempo = micros();
      } else {
        //float velocidade = ((TWOPI * raio) * 1000 / (millis() - tempo)) * 3.6; // Km/h
        //tempo = millis();
        //return velocidade;
        Serial.println(micros() - tempo);
        float rpm = (1000000. / (micros() - tempo)) * 60;
        tempo = micros();
        return rpm;
      }
    }
  } else {
    flag_velocidade = false;
  }
  return -1;
}

/**
 * @brief 			Retorna a tensão em volts do circuito.
 * @return float
 * @details			 
 */
float getTensao() {
  return (analogRead(SENSOR_TENSAO)) * resolucao;
}

/**
 * @brief 					Retorna a corrente do circuito em mA.
 * @param int numAmostras	Número de amostras que devem ser colhidas para calcular a corrente final, evitando flutuação no valor.
 * @return float			
 * @details					Calcula a média de um conjunto de leituras do sensor para evitar flutuação de valores por ruído; gerando o valor de tensão proporcional a faixa de valores entre 0 e 1024 da leitura analógica. Após a subtração de um offset (2.5), dividimos pos .066 que corresponde ao valor de 66mv/A relativa a versão do módulo usado (30A).
 */

// Range do sensor de 30A = 66mV/A (0.066)
// Corrente sem carga: 0.12mA -> Sinal deve ir de 2.5v para 0.066 * 0.12 = 0.00792 -> 2.50792
// Corrente com carga: 0.33mA -> Sinal deve ir de 2.5v para 0.066 * 0.33 = 0.02178 -> 2.52178
// Arduino UNO tem capacidade de medição de: 5V/1024, 4.88 mV por unidade de medida.

float getCorrente( int numAmostras ) {
  /*float valores[numAmostras] = {};
	for ( int w=0; w<numAmostras; w++ ) {
		valores[w] = analogRead(SENSOR_CORRENTE);
		delay(SENSOR_MEDIA_DELAY);
	}
  float maxVal = -999999;
  int maxIndex = -1;
  int minIndex = -1;
  float minVal = 999999;
  for ( int w=0; w<numAmostras; w++ ) {
    if( valores[w] > maxVal ) {
      maxVal = valores[w];
      maxIndex = w;
    }
    if( valores[w] < minVal ) {
      minVal = valores[w];
      minIndex = w;
    }
  }

  float media = 0;
  for ( int w=0; w<numAmostras; w++ ) {
		if( w == maxIndex || (w == minIndex ) ) {
      continue;
    } else {
      media += valores[w];
    }
	}
	media /= (numAmostras - 2);
	float tensaoEntrada = (media * (5.0 / 1024.0));	// TODO: Separei os cálculos para fazer debug mais fácil. Revisitar e juntar tudo.
  return (tensaoEntrada - 2.625) / 0.066;*/
  float media = 0;
  float velocidadeReal = -1;
  for ( int w=0; w<numAmostras; w++ ) {
		media += analogRead( SENSOR_CORRENTE );
    delay(3);
	}

	media /= numAmostras;
	float tensaoEntrada = (media * resolucao);	// TODO: Separei os cálculos para fazer debug mais fácil. Revisitar e juntar tudo.
  return (tensaoEntrada - 2.625) / 0.066;
}

/**
 * @brief				Monta uma string para um conjunto de chave/valor do JSON.
 * @param String nome   O nome da chave (key).
 * @param float valor   O valor numérico atribuído a chave. No nosso caso só precisamos lidar com números então não temos problemas com outros casos.
 * @return String		Uma string de chave/valor formatada para o padrão JSON.
 */
String getJsonKeyString( String nome, float valor ) {
	return ( "\"" + nome + "\":" + valor );
}

/**
 * @brief 		Monta uma string em formato JSON, com todos os dados dos sensores para envio.			
 * @details		Apenas como detalhe, coloquei os nomes os menores possíveis mas ainda sugestivos; para poder enviar uma string menor.
 */
void encodeJson( float volts, float amp, float temp, float vel ) {
	jsonString = "<{";
	jsonString += getJsonKeyString("volt", volts);
	jsonString += ",";
	jsonString += getJsonKeyString("amp", amp);
	jsonString += ",";
	jsonString += getJsonKeyString("temp", temp);
	jsonString += ",";
	jsonString += getJsonKeyString("vel", vel );
	jsonString += "}>";
}

void setup(void) { 
  Serial.begin( BAUD_RATE );
  pinMode(SENSOR_VELOCIDADE, INPUT);
  sensors.begin(); 
}
unsigned long tempo_temperatura = 0;
int delay_temperatura = 5000;
void loop(void) {
  unsigned long tempo_inicial = micros();
  float tensao = getTensao();
  float corrente = getCorrente(10);
  float temperatura = 999;
  if( millis() - tempo_temperatura > delay_temperatura ) {
    tempo_temperatura = millis();
    temperatura = getTemperatura();
  }
  encodeJson( getTensao(), getCorrente( 10 ), temperatura, 300 );
  //Serial.println( micros() - tempo_inicial );
  Serial.println(jsonString);
}


