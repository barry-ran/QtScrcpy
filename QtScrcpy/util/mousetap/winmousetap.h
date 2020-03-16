#ifndef WINMOUSETAP_H
#define WINMOUSETAP_H

#include "mousetap.h"

class WinMouseTap : public MouseTap
{
public:
    WinMouseTap();
    virtual ~WinMouseTap();

    void initMouseEventTap() override;
    void quitMouseEventTap() override;
    void enableMouseEventTap(QRect rc, bool enabled) override;
};

#endif // WINMOUSETAP_H
