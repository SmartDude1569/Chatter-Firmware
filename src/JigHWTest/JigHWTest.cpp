#include <SPIFFS.h>
#include <Battery/BatteryService.h>
#include <TypeDef.h>
#include <modules/LLCC68/LLCC68.h>
#include <Settings.h>
#include "JigHWTest.h"
#include "SPIFFSChecksum.hpp"
#include <Pins.hpp>
#include <Chatter.h>

JigHWTest *JigHWTest::test = nullptr;

JigHWTest::JigHWTest(Display& display){
	test = this;

	tests.push_back({JigHWTest::LoRaTest, "LoRa", [](){ }});
	tests.push_back({JigHWTest::BatteryCheck, "Battery", [](){}});
	tests.push_back({JigHWTest::SPIFFSTest, "SPIFFS", [](){ }});
}

void JigHWTest::start(){
	Serial.println();
	uint64_t mac = ESP.getEfuseMac();
	uint32_t upper = mac >> 32;
	uint32_t lower = mac & 0xffffffff;
	Serial.printf("TEST:begin:%x%x\n", upper, lower);

	canvas->clear(TFT_BLACK);
	canvas->setTextColor(TFT_GOLD);
	canvas->setTextWrap(false, false);
	canvas->setTextDatum(textdatum_t::middle_center);

	canvas->setTextFont(0);
	canvas->setTextSize(1);
	canvas->setCursor(0, 6);

	canvas->printCenter("Chatter Hardware Test");
	canvas->setCursor(canvas->width() / 2, 16);
	canvas->println();
	display->commit();

	bool pass = true;
	for(const Test &test : tests){
		currentTest = test.name;

		canvas->setTextColor(TFT_WHITE);
		canvas->printf("%s: ", test.name);
		display->commit();

		Serial.printf("TEST:startTest:%s\n", currentTest);

		bool result = test.test();

		canvas->setTextColor(result ? TFT_GREEN : TFT_RED);
		canvas->printf("%s\n", result ? "PASSED" : "FAILED");
		display->commit();

		Serial.printf("TEST:endTest:%s\n", result ? "pass" : "fail");

		if(!(pass &= result)){
			if(test.onFail){
				test.onFail();
			}

			break;
		}
	}

	if(!pass){
		Serial.printf("TEST:fail:%s\n", currentTest);
		for(;;);
	}

	Serial.println("TEST:passall");

}

void JigHWTest::log(const char* property, const char* value){
	Serial.printf("%s:%s:%s\n", currentTest, property, value);
}

void JigHWTest::log(const char* property, float value){
	Serial.printf("%s:%s:%f\n", currentTest, property, value);
}

void JigHWTest::log(const char* property, double value){
	Serial.printf("%s:%s:%lf\n", currentTest, property, value);
}

void JigHWTest::log(const char* property, bool value){
	Serial.printf("%s:%s:%s\n", currentTest, property, value ? "TRUE" : "FALSE");
}

void JigHWTest::log(const char* property, uint32_t value){
	Serial.printf("%s:%s:%lu\n", currentTest, property, value);
}

void JigHWTest::log(const char* property, const String& value){
	Serial.printf("%s:%s:%s\n", currentTest, property, value.c_str());
}

bool JigHWTest::LoRaTest(){
	LLCC68 radio(new Module(RADIO_CS, RADIO_DIO1, RADIO_RST, RADIO_BUSY, Chatter.getSPILoRa()));
	int state = radio.begin(868, 500, 9, 5, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 22, 8, 0, false);

	if(state != RADIOLIB_ERR_NONE){
		test->log("begin", false);
		return false;
	}
	return true;
}

bool JigHWTest::BatteryCheck(){
	uint8_t level = Battery.getLevel();
	uint16_t voltage = Battery.getVoltage();
	if(level != 5){
		test->log("level", (uint32_t) level);
		test->log("voltage", (uint32_t) voltage);
		return false;
	}

	return true;
}

bool JigHWTest::SPIFFSTest(){
	for(const auto& f : SPIFFSChecksums){
		if(!SPIFFS.exists(f.name)){
			test->log("missing", f.name);
			return false;
		}

		fs::File file = SPIFFS.open(f.name, "r");
		uint32_t sum = calcChecksum(file);
		file.close();

		if(sum != f.sum){
			test->log("file", f.name);
			test->log("expected", (uint32_t) f.sum);
			test->log("got", (uint32_t) sum);

			return false;
		}
	}

	return true;
}

uint32_t JigHWTest::calcChecksum(File& file){
	if(!file) return 0;

#define READ_SIZE 512

	uint32_t sum = 0;
	uint8_t b[READ_SIZE];
	size_t read = 0;
	while(read = file.read(b, READ_SIZE)){
		for(int i = 0; i < read; i++){
			sum += b[i];
		}
	}

	return sum;
}


