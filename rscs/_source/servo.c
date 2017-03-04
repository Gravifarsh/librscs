#include <stdlib.h>

#include <util/atomic.h>

#include <avr/io.h>

#include "librscs_config.h"

#include "../servo.h"

#define PRESCALER 8
#define TEAK_IN_MS (F_CPU / 1000)

struct rscs_servo;
typedef struct rscs_servo rscs_servo;


struct rscs_servo{
	uint8_t id;
	uint8_t mask;
	int ocr;
	int new_ocr;
	int min;
	int max;
	rscs_servo * next;
};

rscs_servo * head;
rscs_servo * current;


int _map(rscs_servo *t,int an)
{
	int max = t->max;
	int min = t->min;
	return (an*(max - min) + 180 * min) / 180;
}

void rscs_servo_calibrate(int n, float min_ms, float max_ms)
{
	rscs_servo *t = head;
	while(t != NULL && t->id != n)
	{
		t = t->next;
	}
	if(t != NULL)
	{
		t->min = (int) (TEAK_IN_MS * min_ms);
		t->max = (int) (TEAK_IN_MS * max_ms);
	}
}

void rscs_servo_timer_init(void)
{
	TCCR1A |= (0<<WGM10) | (0<<WGM11); // CTC, OCR1A
	TCCR1B |= (1<<WGM12) | (0<<WGM13)
			| (0<<CS10) | (1<<CS11) | (0<<CS12); //prescaler - 8
#ifdef __AVR_ATmega328P__
	TIMSK1 |= (1<<OCIE1B) | (1<<OCIE1A);
#elif defined __AVR_ATmega128__
	TIMSK |= (1<<OCIE1B) | (1<<OCIE1A);
#endif
	OCR1A = 20 * TEAK_IN_MS / PRESCALER;
	OCR1B = current->ocr;
}

static inline void _init_servo(int id, rscs_servo * servo)
{
	servo->min = TEAK_IN_MS * 0.8 / PRESCALER;
	servo->max = TEAK_IN_MS * 2.2 / PRESCALER;
	servo->id = id;
	servo->mask = (1 << id);
	servo->ocr = (servo->min+servo->max)/2;
	servo->new_ocr = -1;
}

void _include_servo(rscs_servo *in)
{
	if(in->ocr <= head->ocr)
	{
		in->next = head;
		head = in;
	}
	else
	{
		rscs_servo *tmp = head;
		while(tmp->next != NULL && tmp->next->ocr < in->ocr) //находим серву с наибольшим ocr < in->ocr
		{
			tmp = tmp->next;
		}
		if(tmp->next == NULL)
		{
			tmp->next = in;
		}
		else
		{
			rscs_servo *buf = tmp->next;
			tmp->next = in;
			in->next = buf;
		}
	}
}
static void _exclude_servo(rscs_servo *tmp)
{
	if(tmp == head)
	{
		head = tmp->next;
		return;
	}
	rscs_servo *previous = head;
	while(previous->next != tmp)
	{
		previous = previous->next;
	}
	previous->next = tmp->next;
}


void rscs_servo_init(int n)
{
	head = malloc(sizeof(rscs_servo));
	_init_servo(0, head);
	rscs_servo * temp = head;
	for(int i = 1; i < n; i++)
	{
		temp->next = malloc(sizeof(rscs_servo));
		temp = temp->next;
		_init_servo(i, temp);
	}
	current = head;
}

void rscs_servo_set_angle(int n, int angle)
{
	rscs_servo *t = head;
	while(t != NULL && t->id != n)
	{
		t = t->next;
	}
	if(t == NULL) { return;}
	t->new_ocr = _map(t, angle);
}

int _set_angle(rscs_servo *servo)
{
	if(servo->new_ocr >= 0)
	{
		_exclude_servo(servo);
		servo->ocr = servo->new_ocr;
		servo->new_ocr = -1;
		_include_servo(servo);
		return 1;
	}
	else
	{
		return 0;
	}
}


ISR(TIMER1_COMPB_vect)
{
	//PORTB ^= (1 << 5);

	do
	{
		RSCS_SERVO_PORT &= ~current->mask;
		current = current->next;
	} while (current->next != NULL && current->next->ocr == current->ocr);

	if(current == NULL)
	{
		current = head;
		while(current != NULL)
		{
			if(_set_angle(current))
			{
				current = head;
			}
			current = current->next;
		}
		current = head;
	}

	OCR1B = current->ocr;
}

ISR(TIMER1_COMPA_vect)
{
	//PORTB ^= (1 << 5);
	RSCS_SERVO_PORT = 0xFF;
}
