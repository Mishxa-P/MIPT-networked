#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <random>
#include <map>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;
static std::map <uint16_t, std::string> playersNames;
static std::map <uint16_t, int> playersPoints;

static float TIME_BEFORE_GAME_STARTS = 5.0f;
static float ENTITY_INVULNERABILITY_TIME = 2.5f;

static std::vector<std::string> names{ "Hassan Chase", "Marco Goodman", "Thomas Rasmussen", "Arran Snow", "Phillip Fletcher", "Conner Shelton", "Omar Hawkins", "Amir Cantrell",
  "Abbie Robbins", "Kitty Pollard", "Lottie Moon", "Hana Combs", "Melanie Rios", "Rosie Rollins", "Autumn Garcia", "Francis Chen", "Albert Decker",
  "Tony Turner", "Jeffrey Hull", "Hashim Hardy" };

static uint16_t create_random_entity()
{
  uint16_t newEid = entities.size();
  float radius = (float)(rand() % 20 + 12);
  uint32_t color = (255 << 24) + (int((rand() % 120 + 120)) << 16) + (int(rand() % 120 + 120) << 8) + int(rand() % 120 + 120);
  float x = (rand() % 40 - 20) * 5.f;
  float y = (rand() % 40 - 20) * 5.f;
  Entity ent = { color, color, radius, x, y, newEid, false, 0.f, 0.f, 0.f };
  entities.push_back(ent);
  auto rd = std::random_device{};
  auto rng = std::default_random_engine{ rd() };
  std::shuffle(std::begin(names), std::end(names), rng);
  playersNames[newEid] = names.back();
  names.pop_back();
  playersPoints[newEid] = 0.0f;
  return newEid;
}

void on_join(ENetPacket* packet, ENetPeer* peer, ENetHost* host)
{
  // send all entities
  for (const Entity& ent : entities)
  {
    send_new_entity(peer, ent);
  }
  // find max eid
  uint16_t newEid = create_random_entity();
  const Entity& ent = entities[newEid];

  controlledMap[newEid] = peer;
  send_player_name(peer, playersNames[newEid]);


  // send info about new entity to everyone
  for (size_t i = 0; i < host->connectedPeers; ++i)
  {
    send_new_entity(&host->peers[i], ent);
  }

  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket* packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f;
  deserialize_entity_state(packet, eid, x, y);
  for (Entity& e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
    }
}

int main(int argc, const char** argv)
{
  

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  bool updateScores = false;
  constexpr int numAi = 10;

  for (int i = 0; i < numAi; ++i)
  {
    uint16_t eid = create_random_entity();
    entities[eid].serverControlled = true;
    controlledMap[eid] = nullptr;
  }

  uint32_t lastTime = enet_time_get();
  while (true)
  {
    uint32_t curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f;
    lastTime = curTime;
    if (TIME_BEFORE_GAME_STARTS > 0)
    {
      TIME_BEFORE_GAME_STARTS -= dt;
    }

    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_CLIENT_TO_SERVER_JOIN:
          on_join(event.packet, event.peer, server);
          break;
        case E_CLIENT_TO_SERVER_STATE:
          on_state(event.packet);
          break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    updateScores = false;
    for (Entity& e : entities)
    {
      if (TIME_BEFORE_GAME_STARTS > 0) // players are yellow until game starts
      {
        e.color = (255 << 24) + (255 << 16) + (25 << 8) + 255;
      }
      else if (e.invulnerabilityTime >= 0) // player is white and invulnerable for some time after he collided
      {
        e.color = 0xffffffff;
        e.invulnerabilityTime -= dt;
      }
      else
      {
        e.color = e.initialColor;
      }

      if (e.serverControlled)
      {
        const float diffX = e.targetX - e.x;
        const float diffY = e.targetY - e.y;
        const float dirX = diffX > 0.f ? 1.f : -1.f;
        const float dirY = diffY > 0.f ? 1.f : -1.f;
        constexpr float spd = 50.f;
        e.x += dirX * spd * dt;
        e.y += dirY * spd * dt;
        if (fabsf(diffX) < 10.f && fabsf(diffY) < 10.f)
        {
          e.targetX = (rand() % 40 - 20) * 15.f;
          e.targetY = (rand() % 40 - 20) * 15.f;
        }
      }
      if (e.invulnerabilityTime < 0.f)
      {
        for (Entity& checkCollisionEnt : entities)
        {
          if (e.eid != checkCollisionEnt.eid)
          {
            if (TIME_BEFORE_GAME_STARTS <= 0 && (std::sqrt(e.radius + checkCollisionEnt.radius)) >= (std::sqrt(e.x - checkCollisionEnt.x) + std::sqrt(e.y - checkCollisionEnt.y)))
            {
              if (e.radius >= 4 && checkCollisionEnt.radius >= 4)
              {
                send_points_start(server);
                updateScores = true;
                if (e.radius >= checkCollisionEnt.radius)
                {
                  e.radius += checkCollisionEnt.radius / 2;
                  playersPoints[e.eid] += checkCollisionEnt.radius / 2;
                  checkCollisionEnt.radius /= 2;
                  checkCollisionEnt.x = (rand() % 40 - 20) * 5.f;
                  checkCollisionEnt.y = (rand() % 40 - 20) * 5.f;
                }
                else
                {
                  checkCollisionEnt.radius += e.radius / 2;
                  playersPoints[checkCollisionEnt.eid] += e.radius / 2;
                  e.radius /= 2;
                  e.x = (rand() % 40 - 20) * 5.f;
                  e.y = (rand() % 40 - 20) * 5.f;
                }
                e.invulnerabilityTime = ENTITY_INVULNERABILITY_TIME;
                checkCollisionEnt.invulnerabilityTime = ENTITY_INVULNERABILITY_TIME;
              }
            }
          }
        }
      }
    }
    
    for (size_t i = 0; i < server->connectedPeers; ++i)
    {
      ENetPeer* peer = &server->peers[i];
      for (const Entity& e : entities)
      {
        if (controlledMap[e.eid] != peer)
        {
          send_snapshot(peer, e.eid, e.x, e.y, e.radius, e.color);
        }
        else
        {
          send_player_game_info(peer, e.radius, e.color);
        }
        if (updateScores)
        {
          std::string pointsLine = playersNames[e.eid] + ": " + std::to_string(playersPoints[e.eid]);
          send_points_line(peer, pointsLine);
        }
      } 
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}
