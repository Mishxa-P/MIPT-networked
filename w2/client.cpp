#include "raylib.h"
#include <enet/enet.h>
#include <iostream>
#include <string>
#include <vector>

std::string packetDataToString(const uint8_t* data, size_t length)
{
    std::string str;

    for (size_t i = 0; i < length; ++i)
    {
        str += static_cast<char>(data[i]);
    }
    return str;
}

void send_message(ENetPeer* peer, std::string& message)
{
    const char* msg = message.c_str();
    const size_t msgLen = strlen(msg);
    ENetPacket* packet = enet_packet_create(msg, msgLen + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

const std::string LOBBY_HOST_NAME = "localhost";
const int LOBBY_ADDRESS_PORT = 10888;

int main(int argc, const char **argv)
{
  int width = 800;
  int height = 600;
  InitWindow(width, height, "Client");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  SetTargetFPS(60);

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 3, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, LOBBY_HOST_NAME.c_str());
  address.port = LOBBY_ADDRESS_PORT;

  ENetPeer *lobbyPeer = enet_host_connect(client, &address, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }
 
  ENetPeer* gameServerPeer;


  std::string startGameMsg = "Start a session!";
  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  uint32_t lastMicroSendTime = timeStart;

  std::string receivedMsg = "";
  std::string status = "unknown";
  std::string clientInfo = "ID: unknown | Name: unknown";
  std::vector <std::string> players;
  std::vector <std::string> events;
  std::vector <std::string> latency;
  bool gameStarted = false;
  bool connectedToGameServer = false;
  float posX = 0.f;
  float posY = 0.f;

  while (!WindowShouldClose())
  {
      const float dt = GetFrameTime();
      ENetEvent event;
      while (enet_host_service(client, &event, 10) > 0)
      {
          switch (event.type)
          {
          case ENET_EVENT_TYPE_CONNECT:
              printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
              if (event.peer->address.port == LOBBY_ADDRESS_PORT)
              {
                  status = "Connected to the lobby, press ENTER to start a session!";
              }
              else
              {
                  connectedToGameServer = true;
                  status = "Connected to the game server!";
              }
              break;
          case ENET_EVENT_TYPE_RECEIVE:
              printf("Packet received '%s'\n", event.packet->data);
              receivedMsg = packetDataToString(event.packet->data, event.packet->dataLength);
              if (receivedMsg.find("ID:") != std::string::npos)
              {
                  clientInfo = receivedMsg;
              }
              if (receivedMsg.find("Connected:") != std::string::npos)
              {
                  std::string event = "Connected:" + receivedMsg.substr(10);
                  events.emplace_back(event);
                  players.clear();
              }
              if (receivedMsg.find("Players:") != std::string::npos)
              {
                  players.emplace_back(receivedMsg.substr(8));
              }
              if (receivedMsg.find("LatencyList:") != std::string::npos)
              {
                  latency.clear();
              }
              if (receivedMsg.find("Ping:") != std::string::npos)
              {
                  latency.emplace_back(receivedMsg.substr(5));
              }
              if (receivedMsg.find("Game Server Address:") != std::string::npos)
              {
                  receivedMsg.erase(0, receivedMsg.find(":") + 1);
                  std::string hostName = receivedMsg.substr(0, receivedMsg.find("|"));
                  receivedMsg.erase(0, receivedMsg.find("|") + 1);
                  int port = std::stoi(receivedMsg);

                  enet_address_set_host(&address, hostName.c_str());
                  address.port = port;
                  gameServerPeer = enet_host_connect(client, &address, 2, 0);
                  if (!gameServerPeer)
                  {
                      printf("\nCannot connect to the game server\n");
                      status = "Cannot connect to the game server... Try again later...";
                  }
                  else
                  {
                      status = "Trying to connect to the game server...";
                  }
              }
              enet_packet_destroy(event.packet);
              break;
          default:
              break;
          };
      }

      //start game
      if (!connectedToGameServer && IsKeyDown(KEY_ENTER))
      {
          send_message(lobbyPeer, startGameMsg);
      }

      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      constexpr float spd = 10.f;
      posX += ((left ? -1.f : 0.f) + (right ? 1.f : 0.f)) * dt * spd;
      posY += ((up ? -1.f : 0.f) + (down ? 1.f : 0.f)) * dt * spd;

      if (connectedToGameServer && (left || right || up || down))
      {
          std::string posMsg = clientInfo;
          send_message(gameServerPeer, posMsg);
          posMsg = "X: " + std::to_string(posX) + " Y: " + std::to_string(posY);
          send_message(gameServerPeer, posMsg);
      }

    BeginDrawing();
      ClearBackground(BLACK);
      DrawText(TextFormat("Current status: %s", status.c_str()), 20, 20, 20, WHITE);
      if (connectedToGameServer)
      {
          DrawText(clientInfo.c_str(), 20, 60, 20, GREEN);
          DrawText(TextFormat("My position: (%d, %d)", (int)posX, (int)posY), 20, 100, 20, WHITE);
          DrawText("List of players:", 20, 140, 20, BLUE);
          int windowPosY = 140;
          for (int i = 0; i < players.size(); i++)
          {
              windowPosY += 20;
              DrawText(players[i].c_str(), 20, windowPosY, 20, WHITE);
          }
          windowPosY += 40;
          DrawText("Events:", 20, windowPosY, 20, YELLOW);
          for (int i = 0; i < events.size(); i++)
          {
              windowPosY += 20;
              DrawText(events[i].c_str(), 20, windowPosY, 20, WHITE);
          }
          windowPosY += 40;;
          DrawText("Latency:", 20, windowPosY, 20, PURPLE);
          for (int i = 0; i < latency.size(); i++)
          {
              windowPosY += 20;
              DrawText(latency[i].c_str(), 20, windowPosY, 20, WHITE);
          }
      }
    EndDrawing();
  }

  return 0;

}
