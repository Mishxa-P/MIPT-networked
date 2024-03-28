#include "protocol.h"
#include "bitstream.h"

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_player_name(ENetPeer* peer, const std::string& str)
{
  size_t packetSize = sizeof(message_type_t) + sizeof(uint32_t) + str.size();
  ENetPacket* packet = enet_packet_create(nullptr, packetSize, ENET_PACKET_FLAG_RELIABLE);
  Bitstream bs(packet->data, packetSize);
  bs.Write(E_SERVER_TO_CLIENT_NAME);
  uint32_t strSize = static_cast<uint32_t>(str.size());
  bs.Write(strSize);
  for (int i = 0; i < str.size(); i++)
  {
    bs.Write(str[i]);
  }

  enet_peer_send(peer, 0, packet);
}
void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  message_type_t *ptr = packet->data;
  Bitstream bs(packet->data, packet->dataLength);
  bs.Write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bs.Write(ent);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  message_type_t *ptr = packet->data;
  Bitstream bs(packet->data, packet->dataLength);
  bs.Write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.Write(eid);

  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  message_type_t *ptr = packet->data;
  Bitstream bs(packet->data, packet->dataLength);
  bs.Write(E_CLIENT_TO_SERVER_STATE);
  bs.Write(eid);
  bs.Write(x);
  bs.Write(y);

  enet_peer_send(peer, 1, packet);
}

void send_player_game_info(ENetPeer* peer, float radius, uint32_t color)
{
  ENetPacket* packet = enet_packet_create(nullptr, sizeof(message_type_t) + sizeof(float) + sizeof(uint32_t),
ENET_PACKET_FLAG_UNSEQUENCED);
  message_type_t* ptr = packet->data;
  Bitstream bs(packet->data, packet->dataLength);
  bs.Write(E_SERVER_TO_CLIENT_PLAYER_GAME_INFO);
  bs.Write(radius);
  bs.Write(color);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float radius, uint32_t color)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(message_type_t) + sizeof(uint16_t) +
                                                   3 * sizeof(float) + sizeof(uint32_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  message_type_t *ptr = packet->data;
  Bitstream bs(packet->data, packet->dataLength);
  bs.Write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.Write(eid);
  bs.Write(x);
  bs.Write(y);
  bs.Write(radius);
  bs.Write(color);

  enet_peer_send(peer, 1, packet);
}
void send_points_start(ENetHost* host)
{
  ENetPacket* packet = enet_packet_create(nullptr, sizeof(message_type_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_SERVER_TO_CLIENT_POINTS_START;

  enet_host_broadcast(host, 0, packet);
}
void send_points_line(ENetPeer* peer, const std::string& str)
{
  size_t packetSize = sizeof(message_type_t) + sizeof(uint32_t) + str.size();
  ENetPacket* packet = enet_packet_create(nullptr, packetSize, ENET_PACKET_FLAG_RELIABLE);
  Bitstream bs(packet->data, packetSize);
  bs.Write(E_SERVER_TO_CLIENT_POINTS);
  uint32_t strSize = static_cast<uint32_t>(str.size());
  bs.Write(strSize);
  for (int i = 0; i < str.size(); i++)
  {
    bs.Write(str[i]);
  }

  enet_peer_send(peer, 0, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
    Bitstream bs(packet->data, packet->dataLength);
    bs.Skip<message_type_t>();
    bs.Read(ent); 
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
    Bitstream bs(packet->data, packet->dataLength);
    bs.Skip<message_type_t>();
    bs.Read(eid);
}

void deserialize_entity_state(ENetPacket* packet, uint16_t& eid, float& x, float& y)
{
    Bitstream bs(packet->data, packet->dataLength);
    bs.Skip<message_type_t>();
    bs.Read(eid);
    bs.Read(x);
    bs.Read(y);
}

void deserialize_player_game_info(ENetPacket* packet, float& radius, uint32_t& color)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.Skip<message_type_t>();
  bs.Read(radius);
  bs.Read(color);
}

void deserialize_snapshot(ENetPacket* packet, uint16_t& eid, float& x, float& y, float& radius, uint32_t& color)
{
    Bitstream bs(packet->data, packet->dataLength);
    bs.Skip<message_type_t>();
    bs.Read(eid);
    bs.Read(x);
    bs.Read(y);
    bs.Read(radius);
    bs.Read(color);
}

void deserialize_name(ENetPacket* packet, std::string& str)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.Skip<message_type_t>();
  uint32_t strSize;
  bs.Read(strSize);
  char symbol;
  for (int i = 0; i < strSize; i++)
  {
    bs.Read(symbol);
    str += symbol;
  }
}

void deserialize_points_line(ENetPacket* packet, std::string& line)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.Skip<message_type_t>();
  uint32_t strSize;
  bs.Read(strSize);
  char symbol;
  for (int i = 0; i < strSize; i++)
  {
    bs.Read(symbol);
    line += symbol;
  }
}

