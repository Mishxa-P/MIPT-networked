#pragma once
#include <cstdint>
#include <enet/enet.h>
#include <string>
#include "entity.h"

using message_type_t = uint8_t;

enum MessageType : uint8_t
{
  E_CLIENT_TO_SERVER_JOIN = 0,
  E_SERVER_TO_CLIENT_NEW_ENTITY,
  E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY,
  E_CLIENT_TO_SERVER_STATE,
  E_SERVER_TO_CLIENT_SNAPSHOT,
  E_SERVER_TO_CLIENT_PLAYER_GAME_INFO,
  E_SERVER_TO_CLIENT_NAME,
  E_SERVER_TO_CLIENT_POINTS_START,
  E_SERVER_TO_CLIENT_POINTS
};

void send_join(ENetPeer *peer);
void send_new_entity(ENetPeer *peer, const Entity &ent);
void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y);
void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float radius, uint32_t color);
void send_player_game_info(ENetPeer* peer, float radius, uint32_t color);
void send_player_name(ENetPeer* peer, const std::string& str);
void send_points_start(ENetHost* host);
void send_points_line(ENetPeer* peer, const std::string& line);

MessageType get_packet_type(ENetPacket *packet);

void deserialize_new_entity(ENetPacket *packet, Entity &ent);
void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y);
void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float& radius, uint32_t& color);
void deserialize_player_game_info(ENetPacket* packet, float& radius, uint32_t& color);
void deserialize_name(ENetPacket* packet, std::string& str);
void deserialize_points_line(ENetPacket* packet, std::string& line);