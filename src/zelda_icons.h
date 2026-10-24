#pragma once
#include "host_overlay.h"
#include <algorithm>
#include <initializer_list>
#include <vector>

namespace minish {
enum class Icon { Sword, Shield, Feather, Cap, Map, Pouch, Crest, Camera, Album };
// Original vector ornaments. Letters remain alongside icons to identify the
// GBA buttons even when the player equips different items on A/B.
inline void draw_icon(gbarecomp::HostOverlay* o,Icon icon,float x,float y,float s,
                      gbarecomp::OverlaySpace space) {
    using namespace gbarecomp;
    const OverlayColor gold{233,201,117,255}, pale{227,244,215,255},
        green{83,171,103,255}, blue{88,174,191,255};
    const float t=std::max(2.f,s*.065f);
    const auto line=[&](float a,float b,float c,float d,OverlayColor color=OverlayColor{233,201,117,255}) {
        o->line(x+a*s,y+b*s,x+c*s,y+d*s,t,color,space);
    };
    const auto path=[&](std::initializer_list<OverlayPoint> p,OverlayColor color) {
        std::vector<OverlayPoint> points;
        for(auto v:p) points.push_back({x+v.x*s,y+v.y*s});
        o->polyline(points.data(),points.size(),t,color,space);
    };
    const auto triangle=[&](float tx,float ty,float size) {
        path({{tx,ty-size},{tx-size*.87f,ty+size*.5f},
            {tx+size*.87f,ty+size*.5f},{tx,ty-size}},gold);
    };
    switch(icon) {
    case Icon::Sword:
        line(-.32f,.38f,.33f,-.35f,pale); line(.33f,-.35f,.31f,-.53f,pale);
        line(.31f,-.53f,.49f,-.52f,pale); line(.49f,-.52f,.33f,-.35f,pale);
        line(-.34f,.02f,.04f,.37f); line(-.28f,.26f,-.43f,.43f,blue);
        o->fill_circle(x-.45f*s,y+.46f*s,s*.09f,gold,space); break;
    case Icon::Shield:
        path({{-.39f,-.44f},{.39f,-.44f},{.34f,.12f},{0,.5f},
            {-.34f,.12f},{-.39f,-.44f}},gold);
        line(-.26f,-.28f,-.22f,.06f,blue); line(.26f,-.28f,.22f,.06f,blue);
        triangle(0,-.04f,.17f); line(0,.13f,0,.32f,green); break;
    case Icon::Crest:
        triangle(0,-.21f,.22f); triangle(-.22f,.17f,.22f); triangle(.22f,.17f,.22f); break;
    case Icon::Feather:
        path({{-.34f,.46f},{.12f,-.46f},{.35f,-.5f},{.4f,-.15f},
            {.06f,.21f},{-.34f,.46f}},green);
        line(-.4f,.52f,.19f,-.25f); line(-.07f,.05f,.28f,-.02f); break;
    case Icon::Cap:
        path({{-.44f,.32f},{-.23f,-.17f},{.06f,-.45f},{.45f,-.47f},
            {.2f,-.17f},{.22f,.32f},{-.44f,.32f}},green);
        line(-.43f,.34f,.3f,.34f); line(.18f,-.17f,.4f,-.1f);
        o->fill_circle(x-.08f*s,y+.02f*s,s*.045f,pale,space); break;
    case Icon::Map:
        path({{-.46f,-.38f},{-.16f,-.27f},{.14f,-.4f},{.46f,-.27f},
            {.46f,.4f},{.14f,.27f},{-.16f,.4f},{-.46f,.27f},{-.46f,-.38f}},gold);
        line(-.16f,-.26f,-.16f,.37f,green); line(.14f,-.37f,.14f,.25f,green);
        line(.25f,-.05f,.37f,.08f,pale); line(.37f,-.05f,.25f,.08f,pale); break;
    case Icon::Pouch:
        path({{-.24f,-.42f},{.24f,-.42f},{.12f,-.18f},{.38f,.13f},
            {.32f,.43f},{-.32f,.43f},{-.38f,.13f},{-.12f,-.18f},{-.24f,-.42f}},gold);
        line(-.2f,-.17f,.2f,-.17f,green); triangle(0,.14f,.15f); break;
    case Icon::Camera:
        o->stroke_rect(x-s*.47f,y-s*.27f,s*.94f,s*.64f,s*.08f,t,gold,space);
        o->stroke_circle(x,y+s*.05f,s*.2f,t,pale,space);
        line(-.25f,-.29f,-.15f,-.43f); line(-.15f,-.43f,.15f,-.43f);
        line(.15f,-.43f,.25f,-.29f);
        o->fill_circle(x+s*.34f,y-s*.12f,s*.045f,green,space); break;
    case Icon::Album:
        path({{-.43f,-.43f},{-.05f,-.35f},{.05f,-.35f},{.43f,-.43f},
            {.43f,.4f},{.05f,.32f},{-.05f,.32f},{-.43f,.4f},{-.43f,-.43f}},gold);
        line(0,-.3f,0,.32f,green); triangle(.23f,-.02f,.13f);
        line(-.32f,-.11f,-.15f,-.11f,pale); line(-.32f,.05f,-.15f,.05f,pale); break;
    }
}
}
