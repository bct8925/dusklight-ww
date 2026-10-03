// Hand-written parts of the null audio layer (the rest is generated into ww_audio_null.cpp).

#include "JSystem/JStudio/JStudio/jstudio-object.h"
#include "JSystem/JStudio/JStudio_JAudio/control.h"

// Demo scripts (stb) contain JSND blocks. Without JAudio they still have to parse, so build the
// sound object with no adaptor: it ignores every paragraph (JStudio::TObject_sound::do_paragraph).
bool JStudio_JAudio::TCreateObject::create(JStudio::TObject** object,
                                           const JStudio::stb::data::TParse_TBlock_object& data) {
    if (data.get()->type != 'JSND') {
        return false;
    }
    *object = JKR_NEW JStudio::TObject_sound(data, NULL);
    return true;
}
