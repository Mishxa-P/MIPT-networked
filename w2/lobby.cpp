#include <enet/enet.h>
#include <iostream>
#include <sstream>

std::string packetDataToString(const uint8_t* data, size_t length)
{
    std::string str;

    for (size_t i = 0; i < length; ++i)
    {
        str += static_cast<char>(data[i]);
    }
    return str;
}

void broadcast_message_reliable(ENetHost* host, const std::string& message)
{
    const char* msg = message.c_str();
    const size_t msgLen = strlen(msg);
    ENetPacket* packet = enet_packet_create(msg, msgLen + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, 0, packet);
}

void send_message(ENetPeer* peer, const std::string& message)
{
    const char* msg = message.c_str();
    const size_t msgLen = strlen(msg);
    ENetPacket* packet = enet_packet_create(msg, msgLen + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

const int LOBBY_PORT = 10888;
const int GAME_SERVER_PORT = 10887;
const std::string GAME_SERVER_NAME = "localhost";
const std::string GAME_SERVER_ADDRESS = "Game Server Address:" + GAME_SERVER_NAME + "|" + std::to_string(GAME_SERVER_PORT);

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = LOBBY_PORT;

  ENetHost *lobby = enet_host_create(&address, 32, 2, 0, 0);

  if (!lobby)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  std::string receivedMsg = "";
  bool gameStarted = false;

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(lobby, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        if (gameStarted)
        {
            send_message(event.peer, GAME_SERVER_ADDRESS);
        }
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        receivedMsg = packetDataToString(event.packet->data, event.packet->dataLength);
        if (receivedMsg.find("Start a session!") != std::string::npos && !gameStarted)
        {
            gameStarted = true;
            broadcast_message_reliable(lobby, GAME_SERVER_ADDRESS);
        }
        if (receivedMsg.find("Cannot connect to a game server") != std::string::npos)
        {
            gameStarted = false;
        }
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
  }

  enet_host_destroy(lobby);

  atexit(enet_deinitialize);
  return 0;
}

