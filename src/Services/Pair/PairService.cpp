#include <vector>
#include "PairService.h"
#include <Loop/LoopManager.h>
#include "../LoRaService.h"
#include "BroadcastState.h"
#include "RequestState.h"
#include "AcknowledgeState.h"
#include "../ProfileService.h"
#include "../../Storage/Storage.h"

using namespace Pairing;

void PairService::begin(){
	if(state) return;
	LoopManager::addListener(this);
	state = new BroadcastState(this);
}

PairService::~PairService(){
	LoopManager::removeListener(this);
	delete state;
	state = nullptr;
	if(friendStored){
		//remove friend and revert encryption keys
		Storage.Friends.remove(pairUID);
		LoRa.copyEncKeys();
		friendStored = false;
	}
	LoRa.clearPairPackets();
}

void PairService::loop(uint micros){
	//don't broadcast adverts when listening for acks and reqs
	if(!friendStored){
		broadcastTime += micros;
		if(broadcastTime >= broadcastInterval){
			broadcastTime = 0;
			sendAdvert();
		}
	}

	if(!state) return;
	state->loop(micros);

}

const std::vector<Profile> &PairService::getFoundProfiles() const{
	return foundProfiles;
}

void PairService::requestPair(uint32_t index){
	//check if pair already requested?
	if(friendStored || pairUID != 0 || index >= foundUIDs.size()) return;

	pairUID = foundUIDs[index];
	delete state;
	state = new RequestState(pairUID, this);

}

void PairService::sendAdvert(){
	auto packet = new AdvertisePair(Profiles.getMyProfile());
	LoRa.send(0, LoRaPacket::PAIR_BROADCAST, packet);
	delete packet;
}

bool PairService::cancelPair(){
	if(friendStored) return false; //in ack state, cannot cancel
	else{
		delete state;
		pairUID = 0;
		state = new BroadcastState(this);
	}
	LoRa.clearPairPackets();
}

void PairService::requestRecieved(){
	delete state;

	//add new friend to storage
	Profile prof = foundProfiles[std::find(foundUIDs.begin(), foundUIDs.end(), pairUID) - foundUIDs.begin()];
	Friend fren;
	fren.profile = prof;
	fren.uid = pairUID;
	memcpy(fren.encKey, pairKey, 32);
	if(Storage.Friends.exists(pairUID)){
		Storage.Friends.update(fren);
	}else{
		Storage.Friends.add(fren);
	}
	friendStored = true;
	LoRa.copyEncKeys();

	//proceed to ack state
	state = new AcknowledgeState(pairUID, pairKey, this);
}

void PairService::setDoneCallback(void (* doneCallback)(bool, void* pVoid), void* data){
	PairService::doneCallback = doneCallback;
	doneCbData = data;
}

void PairService::pairDone(){
	delete state;
	state = nullptr;
	LoRa.copyEncKeys();

	if(doneCallback){
		doneCallback(true, doneCbData);
	}
	LoRa.clearPairPackets();
}

void PairService::pairFailed(){
	delete state;
	state = new BroadcastState(this);

	if(friendStored){
		//remove friend and revert encryption keys
		Storage.Friends.remove(pairUID);
		LoRa.copyEncKeys();
		friendStored = false;
	}
	LoRa.clearPairPackets();
	pairUID = 0;

	if(doneCallback){
		doneCallback(false, doneCbData);
	}
}

void PairService::setUserFoundCallback(void ( * userFoundCallback)(const Profile &, void* pVoid), void* data){
	PairService::userFoundCallback = userFoundCallback;
	userFoundCbData = data;
}

void PairService::setUserChangedCallback(void (* userChangedCallback)(const Profile &, int index, void* pVoid), void* data){
	PairService::userChangedCallback = userChangedCallback;
	userChangedCbData = data;
}
