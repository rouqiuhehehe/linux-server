//
// Created by Administrator on 2023/3/21.
//

#include "websocket.h"
#include <string.h>
#include <unistd.h>

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

size_t base64_encode (char *in_str, size_t in_len, char *out_str)
{
    BIO *b64, *bio;
    BUF_MEM *bptr = NULL;
    size_t size = 0;

    if (in_str == NULL || out_str == NULL)
        return -1;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, in_str, in_len);
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &bptr);
    memcpy(out_str, bptr->data, bptr->length);
    out_str[bptr->length - 1] = '\0';
    size = bptr->length;

    BIO_free_all(bio);
    return size;
}

int getWebsocketResponseHead (const char *websocketKey, char *head, int len)
{
    unsigned char buffer[64] = {};
    unsigned char secData[64] = {};
    char secAccept[32] = {};
    strcat(buffer, websocketKey);
    strcat(buffer, GUID);

    SHA1(buffer, strlen(buffer), secData);
    base64_encode(secData, strlen(secData), secAccept);

    memset(head, 0, len);
    sprintf(
        head,
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n",
        secAccept
    );

    return 0;
}

void umask (char *payload, int length, const char *mask_key)
{

    int i = 0;
    for (i = 0; i < length; i++)
    {
        payload[i] ^= mask_key[i % 4];
    }

}
int transmission (char *msg, char *buffer)
{

    //ev->buffer; ev->length

    struct WsOphdr *hdr = (struct WsOphdr *)msg;

    printf("length: %d\n", hdr->pl_len);

    if (hdr->pl_len < 126)
    { //


        char *payload =
            msg + sizeof(struct WsOphdr) + 4; // 6  payload length < 126
        if (hdr->mask)
            // 4位掩码
            umask(payload, hdr->pl_len, msg + 2);
        printf("payload : %s\n", payload);
        strcpy(buffer, payload);
    }
    else if (hdr->pl_len == 126)
    {

        struct WsHead126 *hdr126 = msg + sizeof(struct WsOphdr);

    }
    else
    {

        struct WsHead127 *hdr127 = msg + sizeof(struct WsOphdr);

    }

}
int websocketRecv (int *state, char *msg, char *buffer, int len)
{
    if (*state == WS_HANDSHARK)
    {
        *state = WS_TRANMISSION;
        getWebsocketResponseHead(msg, buffer, len);
    }
    else if (*state == WS_TRANMISSION)
        transmission(msg, buffer);
}