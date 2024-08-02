#include "nasty.h"

bool IsNasty(const TDBDocument& doc) {
    if ((doc.Language == postly::NL_EN) && (doc.Title.size() < 16)) {
        return true;
    }

    if ((doc.Language == postly::NL_EN) && (doc.Title.size() < 30)) {
        return true;
    }

    unsigned char lastSymb = doc.Title.back();
    if (lastSymb == 0x21 || lastSymb == 0x3f || lastSymb == 0x2e || lastSymb == 0x20) { 
        return true;
    }

    unsigned char firstSymb = doc.Title.front();
    if (firstSymb == 0x22 || firstSymb == 0xab) {
        return true;
    }

    if (std::count(doc.Title.begin(), doc.Title.end(), 0x20) < 2) {
        return true;
    }

    return false;
}
