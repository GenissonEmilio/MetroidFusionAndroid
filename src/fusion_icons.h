#pragma once
#include "host_overlay.h"
#include <algorithm>
namespace fusion {
enum class Icon { Reticle, Beam, Missile, Map, Shoulder, Camera, Album };
inline void draw_icon(gbarecomp::HostOverlay* o,Icon icon,float x,float y,float size,
                      gbarecomp::OverlaySpace space) {
    using gbarecomp::OverlayColor;
    const OverlayColor cyan{104,208,248,240},yellow{248,224,96,245};
    const float r=size*.42f,t=std::max(1.5f,size*.055f);
    if(icon==Icon::Reticle) {
        o->stroke_circle(x,y,r,t,cyan,space);
        o->line(x-r*1.25f,y,x-r*.55f,y,t,yellow,space);
        o->line(x+r*.55f,y,x+r*1.25f,y,t,yellow,space);
        o->line(x,y-r*1.25f,x,y-r*.55f,t,yellow,space);
        o->line(x,y+r*.55f,x,y+r*1.25f,t,yellow,space);
    } else if(icon==Icon::Beam) {
        o->stroke_rect(x-r,y-r*.4f,r*1.4f,r*.8f,t,t,cyan,space);
        for(int i=0;i<3;++i) o->line(x+r*(.55f+i*.3f),y-r*.3f,x+r*(.55f+i*.3f),y+r*.3f,t,yellow,space);
    } else if(icon==Icon::Missile) {
        o->line(x-r,y+r,x+r,y-r,t*2,cyan,space);
        o->line(x+r,y-r,x+r*.25f,y-r,t*2,yellow,space);
        o->line(x+r,y-r,x+r,y-r*.25f,t*2,yellow,space);
        o->line(x-r,y+r*.35f,x-r*.35f,y+r,t*2,cyan,space);
    } else {
        o->stroke_rect(x-r,y-r*.7f,r*2,r*1.4f,t,t,cyan,space);
        o->line(x-r*.6f,y,x+r*.6f,y,t,yellow,space);
    }
}
}
