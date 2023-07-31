//
// Created by Yoshiki on 2023/6/3.
//

#ifndef LINUX_SERVER_SRC_3_BASE_COMPONENT_SENDMSG_SHM_DEFINE_H_
#define LINUX_SERVER_SRC_3_BASE_COMPONENT_SENDMSG_SHM_DEFINE_H_
#include <string.h>

#define MMAP_DATA_LEN 1024
#define BUFFER_LEN MMAP_DATA_LEN
#define FILE_NAME "yoshiki-shm-test"
#define SHM_MESSAGE "Forget about the days when it's been cloudy. But don't forget your hours in the sun.\n"                    \
                "Forget about the times you have been defeated. But don't forget the victories you have won.\n"             \
                "Forget about the misfortunes you have encountered. But don't forget the times your luck has turned.\n"     \
                "Forget about the days when you have been lonely. But don't forget the friendly smiles you have seen.\n"    \
                "Forget about the plans that didn't seem to work out right. But don't forget to always have a dream."

static const size_t SHM_MESSAGE_LEN = strlen(SHM_MESSAGE);

#define SOCK_PATH "/tmp/yoshiki.sock"

#define SEND_MESSAGE "Do not go gentle into that good night,Old age should burn and rave at close of day;"  \
                     "rage, rage against the dying of the light.Though wise men at their end know dark is right," \
                     "because their words had forked no lightning, they do not go gentle into that good night." \
                     "Good men, the last wave by, crying how brighttheir frail deeds might have danced in a green bay,rage, rage against the dying of the light."  \
                     "Wild men who caught and sang the sun in flight,and learn, too late, they grieved it on its way,Do not go gentle into that good night."    \
                     "Grave men, near death, who see with blinding sight,blind eyes could blaze like meteors and be gay,rage, rage against the dying of the light." \
                     "And you, my father, there on the sad height,curse, bless, me now with your fierce tears, I pray.Do not go gentle into that good night."   \
                     "Rage, rage against the dying of the light."

static const size_t SEND_MESSAGE_LEN = strlen(SEND_MESSAGE);
#endif //LINUX_SERVER_SRC_3_BASE_COMPONENT_SENDMSG_SHM_DEFINE_H_
