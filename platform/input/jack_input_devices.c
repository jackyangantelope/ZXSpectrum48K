

/* SPDX-License-Identifier: BSD-3-Clause */
#include "jack_input_devices.h"
#include <stddef.h>
#include <string.h>
static const char osk_rows[4][11] = {"1234567890","QWERTYUIOP","ASDFGHJKL ","ZXCVBNM<^>"};
static bool emit(jack_input_queue *q, jack_input_type t, uint8_t d, uint16_t c, int16_t v) {
    return jack_input_push(q,(jack_input_event){t,d,c,v});
}
bool jack_input_devices_init(jack_input_devices *d, jack_input_queue *q) {
    if(!d || !q) {
        return false;
    }
    memset(d,0,sizeof(*d));
    d->queue=q;
    return true;
}
bool jack_input_set_pad(jack_input_devices *d,uint8_t pad,uint16_t buttons) {
    if(!d || !d->queue || pad>=2u) return false;
    const uint16_t changed=(uint16_t)(d->pads[pad]^buttons); bool ok=true;
    for(uint16_t bit=1u;bit && bit<=JACK_PAD_R;bit<<=1u) if(changed&bit)
        ok=emit(d->queue,(buttons&bit)?JACK_INPUT_PAD_DOWN:JACK_INPUT_PAD_UP,pad,bit,(buttons&bit)?1:0)&&ok;
    d->pads[pad]=buttons; return ok;
}
static bool has_key(const uint8_t keys[6],uint8_t key) {
    for(size_t i=0;i<6u;++i) {
        if(keys[i]==key) {
            return true;
        }
    }
    return false;
}
bool jack_input_keyboard_report(jack_input_devices *d,uint8_t mod,const uint8_t keys[6]) {
    if(!d || !d->queue || !keys) {
        return false;
    }
    bool ok=true;
    for(size_t i=0;i<6u;++i) { uint8_t u=d->keyboard_keys[i]; uint16_t k;
        if(u && !has_key(keys,u) && jack_input_translate_hid(u,d->keyboard_modifier,&k))
            ok=emit(d->queue,JACK_INPUT_KEY_UP,0u,k,0)&&ok;
    }
    for(size_t i=0;i<6u;++i) { uint8_t u=keys[i]; uint16_t k;
        if(u && !has_key(d->keyboard_keys,u) && jack_input_translate_hid(u,mod,&k)) {
            ok=emit(d->queue,JACK_INPUT_KEY_DOWN,0u,k,1)&&ok;
            if(k<0x100u) ok=emit(d->queue,JACK_INPUT_TEXT,0u,k,1)&&ok;
        }
    }
    d->keyboard_modifier=mod; memcpy(d->keyboard_keys,keys,6u); return ok;
}
bool jack_input_mouse_report(jack_input_devices *d,uint8_t buttons,int8_t dx,int8_t dy,int8_t wheel) {
    if(!d || !d->queue) {
        return false;
    }
    bool ok=true;
    if(dx || dy || wheel) { d->mouse_x+=dx; d->mouse_y+=dy; d->mouse_wheel+=wheel;
        ok=emit(d->queue,JACK_INPUT_MOUSE_MOVE,0u,(uint16_t)(uint8_t)wheel,
                (int16_t)(((uint16_t)(uint8_t)dx<<8)|(uint8_t)dy)); }
    const uint8_t changed=(uint8_t)(d->mouse_buttons^buttons);
    for(uint8_t bit=1u;bit && bit<=4u;bit<<=1u) if(changed&bit)
        ok=emit(d->queue,JACK_INPUT_MOUSE_BUTTON,0u,bit,(buttons&bit)?1:0)&&ok;
    d->mouse_buttons=buttons; return ok;
}
void jack_osk_init(jack_osk *o) { if(o) *o=(jack_osk){true,false,0u,0u}; }
bool jack_osk_handle(jack_osk *o,uint16_t key,jack_input_queue *q) {
    if(!o || !q || !o->active) return false;
    if(key==JACK_KEY_LEFT) o->x=(uint8_t)((o->x+9u)%10u);
    else if(key==JACK_KEY_RIGHT) o->x=(uint8_t)((o->x+1u)%10u);
    else if(key==JACK_KEY_UP) o->y=(uint8_t)((o->y+3u)%4u);
    else if(key==JACK_KEY_DOWN) o->y=(uint8_t)((o->y+1u)%4u);
    else if(key==JACK_KEY_ESCAPE) o->active=false;
    else if(key==JACK_KEY_ENTER) { char c=osk_rows[o->y][o->x];
        if(c=='^') o->shift=!o->shift;
        else { uint16_t out=c=='<'?JACK_KEY_BACKSPACE:c=='>'?JACK_KEY_ENTER:(uint16_t)c;
            if(!o->shift && out>='A' && out<='Z') out=(uint16_t)(out+('a'-'A'));
            return emit(q,out<0x100u?JACK_INPUT_TEXT:JACK_INPUT_KEY_DOWN,0u,out,1); }
    } else return false; return true;
}
bool jack_osk_render(const jack_osk *o,jack_terminal *t,uint16_t x,uint16_t y) {
    if(!o || !t || !o->active || x+12u>t->width || y+6u>t->height) return false;
    if(!jack_terminal_box(t,x,y,12u,6u,15u,1u)) return false;
    for(uint16_t row=0;row<4u;++row) for(uint16_t col=0;col<10u;++col) {
        uint8_t g=(uint8_t)osk_rows[row][col]; if(!o->shift && g>='A'&&g<='Z') g=(uint8_t)(g+('a'-'A'));
        const bool selected=row==o->y&&col==o->x;
        jack_terminal_put(t,(uint16_t)(x+1u+col),(uint16_t)(y+1u+row),g,selected?0u:15u,selected?14u:1u);
    }
    return true;
}
