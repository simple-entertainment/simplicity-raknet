/*
 * Copyright © 2014 Simple Entertainment Limited
 *
 * This file is part of The Simplicity Engine.
 *
 * The Simplicity Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * The Simplicity Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with The Simplicity Engine. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <MessageIdentifiers.h>

#include <simplicity/logging/Logs.h>
#include <simplicity/messaging/Messages.h>
#include <simplicity/messaging/Subject.h>
#include <simplicity/Simplicity.h>

#include "BitStream.h"
#include "RakNetMessagingEngine.h"

using namespace RakNet;
using namespace std;

namespace simplicity
{
	namespace raknet
	{
		MessageID SIMPLICITY_RAKNET_MESSAGE_ID = ID_USER_PACKET_ENUM + 1;

		RakNetMessagingEngine::RakNetMessagingEngine(unsigned short listenPort, unsigned int maxConnections) :
			maxConnections(maxConnections),
			peer(nullptr),
			port(listenPort),
			recipients(),
			role(Role::SERVER),
			serverAddress(),
			systemIds()
		{
		}

		RakNetMessagingEngine::RakNetMessagingEngine(const string& serverAddress, unsigned short serverPort) :
			maxConnections(1),
			peer(nullptr),
			port(serverPort),
			recipients(),
			role(Role::CLIENT),
			serverAddress(serverAddress),
			systemIds()
		{
		}

		void RakNetMessagingEngine::advance()
		{
			receive();
		}

		void RakNetMessagingEngine::deregisterRecipient(unsigned short /* subject */,
			function<Recipient> /* recipient */)
		{
		}

		void RakNetMessagingEngine::deregisterRecipient(unsigned short subject, unsigned short recipientCategory)
		{
			vector<unsigned short>& subjectRecipients = recipients[subject];

			subjectRecipients.erase(remove_if(subjectRecipients.begin(), subjectRecipients.end(),
				[recipientCategory](unsigned short existingRecipientCategory)
				{
					return existingRecipientCategory == recipientCategory;
				}));
		}

		unsigned char RakNetMessagingEngine::getPacketType(const Packet& packet)
		{
			if (packet.data[0] == ID_TIMESTAMP)
			{
				return packet.data[sizeof(unsigned char) + sizeof(unsigned long)];
			}

			return packet.data[0];
		}

		void RakNetMessagingEngine::onPlay()
		{
			peer = RakPeerInterface::GetInstance();
			
			if (role == Role::SERVER)
			{
				SocketDescriptor socketDescriptor(port, 0);
				peer->Startup(maxConnections, &socketDescriptor, 1);
				peer->SetMaximumIncomingConnections(static_cast<unsigned short>(maxConnections));

				Logs::log(Category::INFO_LOG, "RakNet server listening on port %i", port);
			}
			else
			{
				SocketDescriptor socketDescriptor;
				peer->Startup(1, &socketDescriptor, 1);

				Logs::log(Category::INFO_LOG, "RakNet client connecting to server at %s|%i", serverAddress.c_str(), port);
				peer->Connect(serverAddress.c_str(), port, nullptr, 0);
			}

			//peer->ApplyNetworkSimulator(0.0f, 50.0f, 0.0f);

			Simplicity::setId(RakNetGUID::ToUint32(peer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS)));
		}

		void RakNetMessagingEngine::onStop()
		{
			peer->Shutdown(0);
			RakPeerInterface::DestroyInstance(peer);
		}

		void RakNetMessagingEngine::receive()
		{
			Packet* packet = peer->Receive();
			while (packet != nullptr)
			{
				receivePacket(*packet);

				peer->DeallocatePacket(packet);
				packet = peer->Receive();
			}
		}

		void RakNetMessagingEngine::receivePacket(const Packet& packet)
		{
			unsigned char packetType = getPacketType(packet);

			if (packetType == ID_CONNECTION_LOST)
			{
				if (role == Role::CLIENT)
				{
					Logs::log(Category::INFO_LOG, "RakNet client has lost its connection to server at %s",
						packet.systemAddress.ToString());
				}
				else if (role == Role::SERVER)
				{
					Logs::log(Category::INFO_LOG, "A client at %s has lost its connection to the RakNet server",
						packet.systemAddress.ToString());
				}
				return;
			}
			else if (packetType == ID_CONNECTION_REQUEST_ACCEPTED)
			{
				Logs::log(Category::INFO_LOG, "RakNet client connected to server at %s",
					packet.systemAddress.ToString());
				return;
			}
			else if (packetType == ID_NEW_INCOMING_CONNECTION)
			{
				systemIds[packet.systemAddress] =
					RakNetGUID::ToUint32(peer->GetGuidFromSystemAddress(packet.systemAddress));

				Message message(Subject::CLIENT_CONNECTED, nullptr);
				message.senderSystemId = systemIds[packet.systemAddress];
				Messages::send(message);

				Logs::log(Category::INFO_LOG, "A client at %s has connected to the RakNet server",
					packet.systemAddress.ToString());
				return;
			}
			else if (packetType == ID_NO_FREE_INCOMING_CONNECTIONS)
			{
				Logs::log(Category::INFO_LOG, "RakNet client failed to connect to server at %s, it is full",
					packet.systemAddress.ToString());
				return;
			}
			else if (packetType == SIMPLICITY_RAKNET_MESSAGE_ID)
			{
				unsigned short subject = 0;
				memcpy(&subject, &packet.data[1], sizeof(unsigned short));

				Codec* codec = Messages::getCodec(subject);
				if (codec == nullptr)
				{
					Logs::log(Category::ERROR_LOG, "Cannot receive message: Codec not found for subject %i", subject);
					return;
				}

				Message message = codec->decode(reinterpret_cast<byte*>(&packet.data[1]));
				message.senderSystemId = systemIds[packet.systemAddress];

				Messages::send(message);
			}
		}

		void RakNetMessagingEngine::registerRecipient(unsigned short /* subject */,
			function<Recipient> /* recipient */)
		{
		}

		void RakNetMessagingEngine::registerRecipient(unsigned short subject, unsigned short recipientCategory)
		{
			recipients[subject].push_back(recipientCategory);
		}

		void RakNetMessagingEngine::send(const Message& message)
		{
			auto subjectRecipients = recipients.find(message.subject);
			if (subjectRecipients == recipients.end() || subjectRecipients->second.empty())
			{
				return;
			}

			Codec* codec = Messages::getCodec(message.subject);
			if (codec == nullptr)
			{
				Logs::log(Category::ERROR_LOG, "Cannot send message: Codec not found for subject %i", message.subject);
				return;
			}

			vector<byte> encodedMessage = codec->encode(message);
			BitStream bitStream;
			bitStream.Write(SIMPLICITY_RAKNET_MESSAGE_ID);
			bitStream.WriteAlignedBytes(reinterpret_cast<unsigned char*>(encodedMessage.data()), encodedMessage.size());

			for (unsigned short subjectRecipient : subjectRecipients->second)
			{
				if (subjectRecipient == RecipientCategory::CLIENT || subjectRecipient == RecipientCategory::SERVER)
				{
					peer->Send(&bitStream, HIGH_PRIORITY, RELIABLE, 0,
						UNASSIGNED_RAKNET_GUID, true);
				}
			}
		}
	}
}
