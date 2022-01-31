#ifndef CHATTER_FIRMWARE_LORASERVICE_H
#define CHATTER_FIRMWARE_LORASERVICE_H

#include <Arduino.h>
#include <SPI.h>
#include "LoRaPacket.h"
#include <Util/Task.h>
#include <RadioLib.h>
#include <queue>
#include <Sync/Mutex.h>
#include <map>

class LoRaService {
public:
	LoRaService();
	bool begin();

	void send(UID_t receiver, LoRaPacket::Type type, const Packet* content);

	static void taskFunc(Task* task);

	ReceivedPacket<MessagePacket> getMessage();
	ReceivedPacket<ProfilePacket> getProfile();
	ReceivedPacket<AdvertisePair> getPairBroadcast();
	ReceivedPacket<RequestPair> getPairRequest();
	ReceivedPacket<AckPair> getPairAck();

	void clearPairPackets();

	int32_t rand();
	int32_t rand(int32_t max);
	int32_t rand(int32_t min, int32_t max);
	UID_t randUID();

	void copyEncKeys();

	std::map<UID_t, size_t> *getHashmapCopy();
private:
	static const uint8_t PacketHeader[4];

	LLCC68 radio;
	bool inited = false;

	std::queue<LoRaPacket> outbox;
	struct {
		std::queue<ReceivedPacket<MessagePacket>> message;
		std::queue<ReceivedPacket<ProfilePacket>> profile;
		std::queue<ReceivedPacket<AdvertisePair>> pairBroadcast;
		std::queue<ReceivedPacket<RequestPair>> pairRequests;
		std::queue<ReceivedPacket<AckPair>> pairAcks;
	} inbox;
	std::map<UID_t, size_t> hashMap;

	Mutex outboxMutex;
	Mutex inboxMutex;
	Mutex randomMutex;
	Mutex hashmapMutex;
	Mutex encKeyMutex;

	Task task;

	static const size_t randomSize = 24;
	std::queue<uint8_t> randos;


	void LoRaReceive();
	void LoRaSend();
	void LoRaRandom();

	static void encDec(void* data, size_t size, const uint8_t* encKey);
	std::map<UID_t, uint8_t[32]> encKeyMap;
};

extern LoRaService LoRa;

#endif //CHATTER_FIRMWARE_LORASERVICE_H
