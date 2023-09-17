//
// Created by Yoshiki on 2023/8/3.
//
#include "util/random.h"
#include "kv-store.h"

class A
{
public:
    A (const A &);
    A (int);
};
class B : public A
{
    using A::A;
    B (const B &);
};
int main ()
{
    IncrementallyHashTable <std::string, std::string> map;

    std::unordered_map <int, std::pair <std::string, std::string>> dd;
    std::string key, value;
    std::string keys[] {
        "YprjyEn",
        "nQimL",
        "gCBwIl",
        "dGyuJe",
        "jCRMXVlBCE",
        "JOSRc",
        "CAubFyNr",
        "MnzNVBQHU",
        "mqvcwFeFCX",
        "Wbfocp",
        "rbBZg",
        "UdJQqj",
        "AGCUrrPPo",
        "JiePD",
        "uXsBIFFSTD",
        "vfwcuWkH",
        "GRgWjy",
        "LCUylx",
        "ZDKcgI",
        "afOLiJmr",
        "iWqCM",
        "GnXjeNpzU",
        "ojmKyGZLA",
        "KatVXmy",
        "GcpEpJ",
        "tbcqfksv",
        "qXWzzhsWZ",
        "NfcmVdZ",
        "mpgqKsED",
        "dkGwTrFjOM",
        "GLaRtmhGZ",
        "vaRYmUPbxa"
    };
    std::string values[] {
        "NeXBbWNS",
        "riUDSgA",
        "uFshTbh",
        "RelJIPaRcl",
        "BFWkqeg",
        "tssjHCF",
        "cVguMyA",
        "muJRDHe",
        "ShbCUaWGz",
        "rkxxVXIY",
        "krBWldRfVY",
        "sjwBhECeO",
        "zTYMgvBdCi",
        "cjcYg",
        "WPvokCGk",
        "Yyitf",
        "vGaHUK",
        "jtVMvfXgXX",
        "FVYplBq",
        "dwFzsgUdLk",
        "lmgJE",
        "BvDgx",
        "kpSvxiYOSp",
        "jXJqU",
        "AoKoUapsW",
        "cKQiLNIY",
        "AEDJt",
        "cGqqODT",
        "aMahjJf",
        "lbmWp",
        "UiydBju",
        "YphsPbIM"
    };
    for (int i = 0; i < 200; ++i)
    {
        key = Utils::getRandomStr(5, 10);
        value = Utils::getRandomStr(5, 10);
        dd[i] = { key, value };
        map.emplace(key, value);
        // map.emplace(keys[i], values[i]);
    }

    size_t i = 0;
    for (const auto &v : map)
        std::cout << i++ << "\t" << v.first << " : " << v.second << std::endl;
    std::cout << std::flush;

    // tcpServer.mainLoop();
    return 0;
}