#include <functional>
#include <algorithm>
#include "raylib.h"
#include <enet/enet.h>

#include <iostream>
#include <vector>
#include "entity.h"
#include "protocol.h"


static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;
static std::vector<std::string> points;
static std::string name;
static float radius;
static uint32_t color;
void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  for (const Entity &e : entities)
    if (e.eid == newEntity.eid)
      return;
  entities.push_back(newEntity);
  enet_packet_destroy(packet);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
  enet_packet_destroy(packet);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float radius = 0.f;  uint32_t color = 0xff000000;
  deserialize_snapshot(packet, eid, x, y, radius, color);
  for (Entity& e : entities)
  {
      if (e.eid == eid)
      {
          e.x = x;
          e.y = y;
          e.radius = radius;
          e.color = color;
      }
  }
  enet_packet_destroy(packet);
}

void on_name_received(ENetPacket* packet)
{
  deserialize_name(packet, name);
}
void on_game_info(ENetPacket* packet)
{
  deserialize_player_game_info(packet, radius, color);
}
void on_points_start(ENetPacket* packet)
{
  points.clear();
}

void on_points(ENetPacket* packet)
{
  std::string pointsLine;
  deserialize_points_line(packet, pointsLine);
  points.push_back(pointsLine);
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "127.0.0.1");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

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

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 1.f;

  SetTargetFPS(60);             

  bool connected = false;
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
       case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          printf("new it\n");
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          printf("got it\n");
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        case E_SERVER_TO_CLIENT_PLAYER_GAME_INFO:
          on_game_info(event.packet);
          break;
        case E_SERVER_TO_CLIENT_NAME:
          on_name_received(event.packet);
          break;
        case E_SERVER_TO_CLIENT_POINTS_START:
          on_points_start(event.packet);
          break;
        case E_SERVER_TO_CLIENT_POINTS:
          on_points(event.packet);
          break;
        };
        break;
      default:
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      for (Entity& e : entities)
      {
        if (e.eid == my_entity)
        {
          // Update
          e.x += ((left ? -dt : 0.f) + (right ? +dt : 0.f)) * 100.f;
          e.y += ((up ? -dt : 0.f) + (down ? +dt : 0.f)) * 100.f;
          e.radius = radius;
          e.color = color;
          // Send
          send_entity_state(serverPeer, my_entity, e.x, e.y);
        }
      }
       
    }


    BeginDrawing();
      ClearBackground(BLACK);
      DrawText(TextFormat("Your Name: %s", name.c_str()), 20, 20, 20, GREEN);
      DrawText("List of players and their points:", 20, 40, 20, BLUE);
      int windowPosY = 40;
      for (int i = 0; i < points.size(); i++)
      {
        windowPosY += 20;
        if (points[i].find(name) != std::string::npos)
        {
          DrawText(points[i].c_str(), 20, windowPosY, 20, WHITE);
        }
        else
        {
          DrawText(points[i].c_str(), 20, windowPosY, 20, GRAY);
        }
      }
      BeginMode2D(camera);
        for (const Entity &e : entities)
        {
            DrawCircle(e.x, e.y, e.radius, GetColor(e.color));
        }
      EndMode2D();
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
