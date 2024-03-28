#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <sstream>

void broadcast_message_reliable(ENetHost* host, const std::string& message)
{
    const char* msg = message.c_str();
    const size_t msgLen = strlen(msg);
    ENetPacket* packet = enet_packet_create(msg, msgLen + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, 0, packet);
}

void broadcast_message_unreliable(ENetHost* host, const std::string& message)
{
    const char* msg = message.c_str();
    const size_t msgLen = strlen(msg);
    ENetPacket* packet = enet_packet_create(msg, msgLen + 1, ENET_PACKET_FLAG_UNSEQUENCED);
    enet_host_broadcast(host, 1, packet);
}

void send_message(ENetPeer* peer, const std::string& message)
{
    const char* msg = message.c_str();
    const size_t msgLen = strlen(msg);
    ENetPacket* packet = enet_packet_create(msg, msgLen + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

const int GAME_SERVER_PORT = 10887;

int main(int argc, const char** argv)
{
    std::vector<std::string> names { "Hassan Chase", "Marco Goodman", "Thomas Rasmussen", "Arran Snow", "Phillip Fletcher", "Conner Shelton", "Omar Hawkins", "Amir Cantrell",
        "Abbie Robbins", "Kitty Pollard", "Lottie Moon", "Hana Combs", "Melanie Rios", "Rosie Rollins", "Autumn Garcia", "Francis Chen", "Albert Decker",
        "Tony Turner", "Jeffrey Hull", "Hashim Hardy" };

    std::vector<std::string> players;

    uint32_t timeStart = enet_time_get();
    uint32_t lastPingListSendTime = timeStart;

    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet");
        return 1;
    }
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = GAME_SERVER_PORT;
    ENetHost* server = enet_host_create(&address, names.size(), 2, 0, 0);

    if (!server)
    {
        printf("Cannot create ENet server\n");
        return 1;
    }
    while (true)
    {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            {
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                auto rd = std::random_device{};
                auto rng = std::default_random_engine{ rd() };
                std::shuffle(std::begin(names), std::end(names), rng);
                players.push_back(names.back());
                names.pop_back();
                std::string playerInfo = "ID:" + std::to_string(players.size() - 1) + " | Name : " + players.back();
                send_message(event.peer, playerInfo);
                playerInfo = "Connected: " + players.back() + " with id = " + std::to_string(players.size() - 1);
                broadcast_message_reliable(server, playerInfo);
                for (int i = 0 ; i < players.size(); i++)
                {
                    std::string msg = "Players:" + players[i];
                    broadcast_message_reliable(server, msg);
                }
                break;
            } 
            case ENET_EVENT_TYPE_RECEIVE:
                printf("Packet received '%s'\n", event.packet->data);
                enet_packet_destroy(event.packet);
                break;
            default:
                break;
            };
        }

        uint32_t curTime = enet_time_get();
        if (curTime - lastPingListSendTime > 1000)
        {
            lastPingListSendTime = curTime;
            std::string msg = "LatencyList:";
            broadcast_message_unreliable(server, msg);
            for (int i = 0; i < players.size(); i++)
            {
                int ping = server->peers[i].roundTripTime;
                std::string pingStr = "Ping:" + players[i] + " : " + std::to_string(ping);
                broadcast_message_unreliable(server, pingStr);
            }
        }
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}

