#ifndef _URIS
#define _URIS

#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/midi/midi.h>
#include <lv2/patch/patch.h>
#include <lv2/urid/urid.h>

typedef struct {
    LV2_URID atom_Blank;
    LV2_URID atom_Object;
    LV2_URID atom_Path;
    LV2_URID atom_String;
    LV2_URID atom_URID;
    LV2_URID atom_eventTransfer;
    LV2_URID patch_Get;
    LV2_URID patch_Set;
    LV2_URID patch_property;
    LV2_URID patch_value;
    LV2_URID midi_MidiEvent;

    LV2_URID wf_patch;
} URIs;

#define WAVEFOREST_URI "http://www.laras.cc/plugins/waveforest"
#define WAVEFOREST_PATCH_URI WAVEFOREST_URI "#patch"

static inline void map_uris(LV2_URID_Map *map, URIs *uris) {
    uris->atom_Blank = map->map(map->handle, LV2_ATOM__Blank);
    uris->atom_Object = map->map(map->handle, LV2_ATOM__Object);
    uris->atom_Path = map->map(map->handle, LV2_ATOM__Path);
    uris->atom_String = map->map(map->handle, LV2_ATOM__String);
    uris->atom_URID = map->map(map->handle, LV2_ATOM__URID);
    uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    // uris->clv2_state = map->map(map->handle, CLV2__state);
    uris->patch_Get = map->map(map->handle, LV2_PATCH__Get);
    uris->patch_Set = map->map(map->handle, LV2_PATCH__Set);
    uris->patch_property = map->map(map->handle, LV2_PATCH__property);
    uris->patch_value = map->map(map->handle, LV2_PATCH__value);
    uris->midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);

    uris->wf_patch = map->map(map->handle, WAVEFOREST_PATCH_URI);
}

#endif
