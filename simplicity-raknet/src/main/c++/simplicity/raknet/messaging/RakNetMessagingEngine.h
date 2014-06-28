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
#ifndef RAKNETMESSAGINGENGINE_H_
#define RAKNETMESSAGINGENGINE_H_

#include <RakPeerInterface.h>

#include <simplicity/messaging/MessagingEngine.h>

namespace simplicity
{
	namespace raknet
	{
		/**
		 * <p>
		 * A messaging engine that sends and receives messages across the network using the RakNet networking engine.
		 * </p>
		 */
		class SIMPLE_API RakNetMessagingEngine : public MessagingEngine
		{
			public:
				RakNetMessagingEngine(unsigned short listenPort, unsigned int maxConnections);

				RakNetMessagingEngine(const std::string& serverAddress, unsigned short serverPort);

				void advance() override;

				void deregisterRecipient(unsigned short subject, std::function<Recipient> recipient)  override;

				void deregisterRecipient(unsigned short subject, unsigned short recipientCategory)  override;

				void onPlay() override;

				void onStop() override;
				
				void registerRecipient(unsigned short subject, std::function<Recipient> recipient) override;

				void registerRecipient(unsigned short subject, unsigned short recipientCategory) override;

				void send(const Message& message) override;

			private:
				enum class Role
				{
					CLIENT,
					SERVER
				};

				std::map<RakNet::SystemAddress, unsigned long> systemIds;

				unsigned int maxConnections;

				RakNet::RakPeerInterface* peer;

				unsigned short port;

				std::map<unsigned short, std::vector<unsigned short>> recipients;

				Role role;

				std::string serverAddress;

				unsigned char getPacketType(const RakNet::Packet& packet);

				void receive();

				void receivePacket(const RakNet::Packet& packet);
		};
	}
}

#endif /* RAKNETMESSAGINGENGINE_H_ */
