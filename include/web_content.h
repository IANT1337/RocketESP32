#ifndef WEB_CONTENT_H
#define WEB_CONTENT_H

#include <Arduino.h>

class WebContent {
public:
    static const char* getIndexHTML();
    static const char* getStyleCSS();
    static const char* getScriptJS();
    static const char* getContentType(const String& path);
    
private:
    static const char index_html[] PROGMEM;
    static const char style_css[] PROGMEM;
    static const char script_js[] PROGMEM;
};

#endif
