#ifndef XMOUSETAP_H
#define XMOUSETAP_H

#include "mousetap.h"

class XMouseTap : public MouseTap
{
public:
    XMouseTap();
    virtual ~XMouseTap();

    void initMouseEventTap() override;
    void quitMouseEventTap() override;
    void enableMouseEventTap(QRect rc, bool enabled) override;
};

#endif // XMOUSETAP_H
