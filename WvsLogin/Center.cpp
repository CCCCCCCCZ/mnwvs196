#include "Center.h"
#include <functional>
#include <thread>

#include "Net\InPacket.h"
#include "Net\OutPacket.h"

#include "Net\PacketFlags\LoginPacketFlags.hpp"
#include "Net\PacketFlags\CenterPacketFlags.hpp"

#include "Constants\ServerConstants.hpp"

#include "WvsLogin.h"

Center::Center(asio::io_service& serverService)
	: SocketBase(serverService, true),
	  mResolver(serverService)
{
}

Center::~Center()
{
}

void Center::SetCenterIndex(int idx)
{
	nCenterIndex = idx;
}

void Center::OnConnectToCenter(const std::string& strAddr, short nPort)
{
	asio::ip::tcp::resolver::query centerSrvQuery(strAddr, std::to_string(nPort)); 
	
	mResolver.async_resolve(centerSrvQuery,
		std::bind(&Center::OnResolve, std::dynamic_pointer_cast<Center>(shared_from_this()),
			std::placeholders::_1,
			std::placeholders::_2));
}

void Center::OnResolve(const std::error_code& err, asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	if (!err)
	{
		asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
		GetSocket().async_connect(endpoint,
			std::bind(&Center::OnConnect, std::dynamic_pointer_cast<Center>(shared_from_this()),
				std::placeholders::_1, ++endpoint_iterator));
	}
	else
	{
	}
}

void Center::OnConnect(const std::error_code& err, asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	if (err)
	{
		printf("[Center::OnConnect]Connect Failed, Center Server No Response.\n");
		OnDisconnect();
		return;
	}
	printf("[Center::OnConnect]Connect To Center Server Successed!\n");
	bIsConnected = true;
	//Encode center handshake packet.
	OutPacket oPacket;
	oPacket.Encode2(LoginPacketFlag::RegisterCenterRequest);
	oPacket.Encode1(ServerConstants::ServerType::SVR_LOGIN);
	SendPacket(&oPacket); 

	OnWaitingPacket();
}

void Center::OnPacket(InPacket *iPacket)
{
	printf("[Center::OnPacket]");
	iPacket->Print();
	int nType = (unsigned short)iPacket->Decode2();
	switch (nType)
	{
	case CenterPacketFlag::RegisterCenterAck:
	{
		auto result = iPacket->Decode1();
		if (!result)
		{
			printf("[Warning]The Center Server Didn't Accept This Socket. Program Will Terminated.\n");
			exit(0);
		}
		printf("Center Server Authenciated Ok. The Connection Between Local Server Has Builded.\n");
		OnUpdateWorldInfo(iPacket);
		break;
	}
	case CenterPacketFlag::CenterStatChanged:
		mWorldInfo.nGameCount = iPacket->Decode2();
		break;
	case CenterPacketFlag::CharacterListResponse:
		OnCharacterListResponse(iPacket);
		break;
	case CenterPacketFlag::GameServerInfoResponse:
		OnGameServerInfoResponse(iPacket);

		break;
	}
}

void Center::OnClosed()
{

}

void Center::OnUpdateWorldInfo(InPacket *iPacket)
{
	mWorldInfo.nWorldID = iPacket->Decode1();
	mWorldInfo.nEventType = iPacket->Decode1();
	mWorldInfo.strWorldDesc = iPacket->DecodeStr();
	mWorldInfo.strEventDesc = iPacket->DecodeStr();
	printf("[Center::OnUpdateWorld]Update World Info Successed.\n");
}

void Center::OnCharacterListResponse(InPacket *iPacket)
{
	int nLoginSocketID = iPacket->Decode4();
	auto pSocket = WvsBase::GetInstance<WvsLogin>()->GetSocketList()[nLoginSocketID];
	OutPacket oPacket;
	oPacket.Encode2(LoginPacketFlag::ClientSelectWorldResult);
	oPacket.Encode1(0);
	oPacket.EncodeStr("normal");
	oPacket.Encode4(0);
	oPacket.Encode1(0);
	oPacket.Encode4(0);
	oPacket.Encode8(0);
	oPacket.Encode1(0);

	printf("Receiving Char list packet : ");
	iPacket->Print();
	printf("\n");

	oPacket.EncodeBuffer(iPacket->GetBuffer() + 6, iPacket->GetBufferSize() - 6);

	/*int chrSize = iPacket->Decode4();
	oPacket.Encode4(chrSize); //char size
	for (int i = 0; i < chrSize; ++i)
		oPacket.Encode4(iPacket->Decode4());

	chrSize = iPacket->Decode1();
	oPacket.Encode1(chrSize); //char size
	for (int i = 0; i < chrSize; ++i)
	{
		OnEncodingCharacterStat(&oPacket, iPacket);
		OnEncodingCharacterAvatar(&oPacket, iPacket);
		oPacket.Encode1(iPacket->Decode1());
		OnEncodingRank(&oPacket, iPacket);
	}*/

	oPacket.Encode1(0x03);
	oPacket.Encode1(0);
	oPacket.Encode4(8); //char slots

	oPacket.Encode4(0);
	oPacket.Encode4(-1);
	oPacket.Encode1(0);
	oPacket.Encode8(-1);
	oPacket.Encode1(0);
	oPacket.Encode1(0);
	oPacket.Encode4(0);
	for (int i = 0; i < 25; ++i)
		oPacket.Encode2(0);
	oPacket.Encode4(0);
	oPacket.Encode8(0);
	pSocket->SendPacket(&oPacket);
}

void Center::OnGameServerInfoResponse(InPacket *iPacket)
{
	int nLoginSocketID = iPacket->Decode4();
	auto pSocket = WvsBase::GetInstance<WvsLogin>()->GetSocketList()[nLoginSocketID]; 
	OutPacket oPacket;
	oPacket.Encode2(LoginPacketFlag::ClientSelectCharacterResult);
	oPacket.EncodeBuffer(iPacket->GetBuffer() + 6, iPacket->GetBufferSize() - 6);
	pSocket->SendPacket(&oPacket);
}