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

#include "RakNetMessagingEngine.h"

using namespace RakNet;
using namespace std;

namespace simplicity
{
	namespace raknet
	{
		RakNetMessagingEngine::RakNetMessagingEngine(unsigned short listenPort, unsigned int maxConnections) :
			maxConnections(maxConnections),
			peer(nullptr),
			port(listenPort),
			recipients(),
			role(Role::SERVER),
			serverAddress()
		{
		}

		RakNetMessagingEngine::RakNetMessagingEngine(const string& serverAddress, unsigned short serverPort) :
			maxConnections(1),
			peer(nullptr),
			port(serverPort),
			recipients(),
			role(Role::CLIENT),
			serverAddress(serverAddress)
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
				Logs::log(Category::INFO_LOG, "A client at %s has lost its connection to the RakNet server",
					packet.systemAddress.ToString());
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
				Logs::log(Category::INFO_LOG, "A client at %s has connected to the RakNet server",
					packet.systemAddress.ToString());
				return;
			}

			unsigned short subject = 0;
			memcpy(&subject, packet.data, sizeof(unsigned short));

			Codec* codec = Messages::getCodec(subject);
			if (codec == nullptr)
			{
				Logs::log(Category::ERROR_LOG, "Cannot receive message: Codec not found for subject %i", subject);
				return;
			}

			void* message = codec->decode(reinterpret_cast<byte*>(&packet.data[sizeof(unsigned short)]));
			Messages::send(subject, message);
		}

		void RakNetMessagingEngine::registerRecipient(unsigned short /* subject */,
			function<Recipient> /* recipient */)
		{
		}

		void RakNetMessagingEngine::registerRecipient(unsigned short subject, unsigned short recipientCategory)
		{
			recipients[subject].push_back(recipientCategory);
		}

		void RakNetMessagingEngine::send(unsigned short subject, const void* message)
		{
			auto subjectRecipients = recipients.find(subject);
			if (subjectRecipients == recipients.end() || subjectRecipients->second.empty())
			{
				return;
			}

			Codec* codec = Messages::getCodec(subject);
			if (codec == nullptr)
			{
				Logs::log(Category::ERROR_LOG, "Cannot send message: Codec not found for subject %i", subject);
				return;
			}

			vector<byte> encodedMessage = codec->encode(message);
			vector<byte> encodedMessageWithSubject(sizeof(unsigned short));
			memcpy(encodedMessageWithSubject.data(), &subject, sizeof(unsigned short));
			encodedMessageWithSubject.insert(encodedMessageWithSubject.begin() + sizeof(unsigned short),
				encodedMessage.begin(), encodedMessage.end());

			for (unsigned short subjectRecipient : subjectRecipients->second)
			{
				if (subjectRecipient == RecipientCategory::CLIENT || subjectRecipient == RecipientCategory::SERVER)
				{
					peer->Send(encodedMessageWithSubject.data(), encodedMessageWithSubject.size(), HIGH_PRIORITY,
						RELIABLE, 0, UNASSIGNED_RAKNET_GUID, true);
				}
			}
		}
	}
}
