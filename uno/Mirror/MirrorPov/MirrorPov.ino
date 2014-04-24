// Third party libraries:
// 

#include "Fonts.h"
#include "Laser.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#include "MirrorController.h"
#include <PWMServo.h> 

#define LASER_PIN 2

MirrorController* mirrorController;
char buf[20];

//char* message = "  XXXXX XXXXX";
  char* message = "HELLO WORLD ";

void setup()
{
	Serial.begin(57600);
	Serial.println("Initializing...");
	mirrorController = new MirrorController();
	mirrorController->init();

	pinMode(LASER_PIN, OUTPUT);
	digitalWrite(LASER_PIN, HIGH);
}

int correction[8] = { 17, 10, 0, 20, 45, 0, 0, -0 };
int lineOrder[8] = { 0, 7, 3, 4, 1, 6, 2, 5 };


void loop()
{
	delay(500);
	digitalWrite(LASER_PIN, LOW);

	unsigned char pbuf[5][20];

	for (int i = 0; i < 5; i++)
	{
		FONTS.getLine(message, i, pbuf[i]);
	}

	byte ct[8];
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (lineOrder[j] == i)
			{
				ct[j] = correction[i];
			}
		}
	}
	for (int i = 0; i < 8; i++)
	{
		correction[i] = ct[i];
	}


	Serial.println("Speeding up the mirror...");
	mirrorController->start();

	delay(3000);

	Serial.println("Measuring rotation speed...");
	long cycleTimeMs = mirrorController->calculateRotationSpeed();

	sprintf(buf, "Cycle time: %ld", cycleTimeMs);
	Serial.println(buf);

	long t0 = 170;
	long lineTime = cycleTimeMs >> 3;
	long pixelTime = lineTime >> 7;
	sprintf(buf, "Pixel time: %ld", pixelTime);
	Serial.println(buf);

	long prevStart = mirrorController->waitForBeginMark();
	delayMicroseconds(cycleTimeMs << 1);

	int pixels = strlen(message) * 8;
	int onTime = (pixelTime >> 1) + 1;

	long startTimes[8];

	do {
		mirrorController->waitForBeginMarkFast();
		long start = micros();
		long* sptr = startTimes;
		int* cptr = correction;
		long val = start + t0;
		
		for (int i = 0; i < 8; i++)
		{
			*sptr = val + (*cptr);
			sptr++;
			cptr++;
			val += lineTime;
		}
		
		sptr = startTimes;
		long t = *sptr;

		for (int i = 0; i < 8; i++)
		{
			int lineNr = lineOrder[i];
			unsigned char* ptr = pbuf[4 - lineNr];
			unsigned char bitMask = 0x80;
			unsigned char info = *ptr;

			if (lineNr < 5)
			{
				for (int p = 0; p < pixels; p++)
				{
					while (micros() < t)
					{
						// do nothing;
					}
					if (info & bitMask)
					{
						digitalWrite(LASER_PIN, HIGH);
					}
					else
					{
						digitalWrite(LASER_PIN, LOW);
					}
					bitMask >>= 1;
					if (bitMask == 0)
					{
						ptr++;
						info = *ptr;
						bitMask = 0x80;
						t += pixelTime;
					}
					t += pixelTime;
				}
			}
			digitalWrite(LASER_PIN, LOW);
			sptr++;
			t = *sptr;
		}
		lineTime = (start - prevStart) >> 3;
		pixelTime = lineTime >> 7;
		onTime = (pixelTime * 3) >> 2;
		prevStart = start;
	} while (1);
}

