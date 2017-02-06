#ifndef DS18B20_H_
#define DS18B20_H_

/*	Модуль-драйвер датчика температуры ds18b20
	Даташит: http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
	он же на русском: http://labkit.ru/userfiles/file/documentation/Sensor/DS18B20_RU.pdf
 */

#include <stdint.h>
#include <stddef.h>

#include "error.h"

// дескриптор датчика
struct rscs_ds18b20_t;
typedef struct rscs_ds18b20_t rscs_ds18b20_t;

// Начало работы с датчиком - создание дескриптора
/* Параметр sensor_uid - уникальный идентификатор датчика на шине ONEWIRE
	Если указать его равным 0x00 - модуль будет работать с датчиком на шине
	(подразумевается, что он один) не используя адресацию, при помощи команды ONEWIRE_SKIP_ADDR
	в случае ошибки возвращает NULL */
rscs_ds18b20_t * ds18b20_init(uint64_t sensor_uid);

// Завершение работы с датчиком - удаление дескриптора
void ds18b20_deinit(rscs_ds18b20_t * sensor);

// Передает датчику коанду на начало замера температуры
/*	чтение температуры процесс длительный и длится порядка секунды на настройках максимальной точности
	функция не ожидает окончания измерения - это лишь команда на начало замера
	если датчик не ответил импульсом присутсвия, возвращает код ошибки RSCS_E_NODEVICE */
rscs_e ds18b20_start_conversion(rscs_ds18b20_t * sensor);

// Получение результатов измерения
/*	Параметры
	- sensor - дескриптор сенсора
	- value_buffer - буффер, в который будет помещенно прочитанное из дачтика сырое значение

	Это сырое значение старшего и младшего байт из "оперативной памяти" дачтика
	То, каким образом следует переводить этот значение в градусы цельсия зависит от конфигурации датчика
	и подробно расписано в даташите.

	Если на шине никто не ответил импульсом присутсвия - возвращает код ошибки RSCS_E_NODEVICE
	Если контрольная сумма сообщения, полученного от датчика не сошлась - возвращает код ошибки RSCS_E_CHKSUM */
rscs_e ds18b20_read_temperature(rscs_ds18b20_t * sensor, int16_t * value_buffer);

#endif /* DS18B20_H_ */
