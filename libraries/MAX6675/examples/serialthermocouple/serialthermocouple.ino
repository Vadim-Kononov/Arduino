// Этот пример является общедоступным.               // www.ladyada.net/learn/sensors/thermocouple
                                                     // Переведено www.iarduino.ru
#include      <max6675.h>                            // Подключаем библиотеку max6675 для работы с датчиком температуры 
const uint8_t thermoDO  = 4;                         // Определяем константу с указанием № вывода Arduino к которому подключён вывод DO  ( SO, MISO ) модуля на чипе MAX6675
const uint8_t thermoCS  = 5;                         // Определяем константу с указанием № вывода Arduino к которому подключён вывод CS  ( SS )       модуля на чипе MAX6675
const uint8_t thermoCLK = 6;                         // Определяем константу с указанием № вывода Arduino к которому подключён вывод CLK ( SCK )      модуля на чипе MAX6675
MAX6675       thermo(thermoCLK, thermoCS, thermoDO); // Объявляем объект thermo для работы с функциями и методами библиотеки MAX6675, указывая выводы ( CLK , CS , DO )
                                                     // 
void setup(){                                        // 
    Serial.begin(9600);                              // Инициируем передачу данных в монитор последовательного порта на скорости 9600 бит/сек
    Serial.println("Тест MAX6675");                  //
    delay(500);                                      // Ждём завершения переходных процессов датчика температуры после подачи питания
}                                                    //
                                                     //
void loop(){                                         //
    Serial.print  ( "C = "                     );    //
    Serial.print  ( thermo.readCelsius()       );    // Выводим температуру в °C
    Serial.print  ( ", F = "                   );    //
    Serial.print  ( thermo.readFahrenheit()    );    // Выводим температуру в °F
    Serial.println( "."                        );    //
    delay(1000);                                     // Ждём 1 секунду
}                                                    //