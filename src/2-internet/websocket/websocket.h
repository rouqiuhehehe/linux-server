//
// Created by Administrator on 2023/3/21.
//

#ifndef LINUX_SERVER_SRC_2_INTERNET_WEBSOCKET_WEBSOCKET_H_
#define LINUX_SERVER_SRC_2_INTERNET_WEBSOCKET_WEBSOCKET_H_

enum
{
    WS_HANDSHARK = 0,
    WS_TRANMISSION = 1,
    WS_END = 2,
};
struct WsOphdr
{

    unsigned char opcode: 4,
        rsv3: 1,
        rsv2: 1,
        rsv1: 1,
        fin: 1;
    unsigned char pl_len: 7,
        mask: 1;
};

struct WsHead126
{

    unsigned short payload_length;
    char mask_key[4];

};

struct WsHead127
{

    long long payload_length;
    char mask_key[4];

};

int websocketRecv (int *state, char *msg, char *buffer, int len);
#endif //LINUX_SERVER_SRC_2_INTERNET_WEBSOCKET_WEBSOCKET_H_
