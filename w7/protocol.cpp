#include "protocol.h"
#include "quantisation.h"
#include "bitstream.h"
#include <cstring> // memcpy

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  Bitstream bs(packet->data, packet->dataLength);

  bs.Write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bs.Write(ent);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  Bitstream bs(packet->data, packet->dataLength);

  bs.Write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.WritePackedUint16(eid);

  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  Bitstream bs(packet->data, packet->dataLength);
  bs.Write(E_CLIENT_TO_SERVER_INPUT);
  bs.WritePackedUint16(eid);

  PackedFloat2<uint8_t, 4, 4> thrSteerPacked(Float2(thr, steer), Float2(-1.f, -1.f), Float2(1.f, 1.f));
  bs.Write(thrSteerPacked.packedVal);

  enet_peer_send(peer, 1, packet);
}

typedef PackedFloat<uint16_t, 11> PositionXQuantized;
typedef PackedFloat<uint16_t, 10> PositionYQuantized;

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  Bitstream bs(packet->data, packet->dataLength);
  bs.Write(E_SERVER_TO_CLIENT_SNAPSHOT);
  uint8_t *ptr = packet->data;
  bs.WritePackedUint16(eid);

  PositionXQuantized xPacked(x, -16, 16);
  PositionYQuantized yPacked(y, -8, 8);
  uint8_t oriPacked = pack_float<uint8_t>(ori, -PI, PI, 8);

  PackedFloat3<uint32_t, 11, 10, 8> xyOriPacked(Float3(x, y, ori), Float3(-16, -8, -PI), Float3(16, 8, PI));
  bs.Write(xyOriPacked.packedVal);

  enet_peer_send(peer, 1, packet);
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
  bs.ReadPackedUint16(eid);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.Skip<message_type_t>();
  bs.ReadPackedUint16(eid);
  uint8_t thrSteerPackedVal = 0;
  bs.Read(thrSteerPackedVal);
  PackedFloat2<uint8_t, 4, 4> thrSteerPacked(thrSteerPackedVal);
  Float2 thrSteerUnpacked = thrSteerPacked.unpack(Float2(-1.f, -1.f), Float2(1.f, 1.f));

  static uint8_t neutralPackedValue = pack_float<uint8_t>(0.f, -1.f, 1.f, 4);
  thr = (thrSteerPackedVal >> 4) == neutralPackedValue ? 0.f : thrSteerUnpacked.x;
  steer = (thrSteerPackedVal & 0x0f) == neutralPackedValue ? 0.f : thrSteerUnpacked.y;
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori)
{
  Bitstream bs(packet->data, packet->dataLength);
  bs.Skip<message_type_t>();
  bs.ReadPackedUint16(eid);
  uint32_t xyOriPackedVal = 0;
  bs.Read(xyOriPackedVal);
  PackedFloat3<uint32_t, 11, 10, 8> xyOriPacked(xyOriPackedVal);
  Float3 xyOriUnpacked = xyOriPacked.unpack(Float3(-16, -8, -PI), Float3(16, 8, PI));
  x = xyOriUnpacked.x;
  y = xyOriUnpacked.y;
  ori = xyOriUnpacked.z;
}
