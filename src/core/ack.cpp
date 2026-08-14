#include "ack.h"


namespace ACK {
    // Private
    struct State {
        char senseLine[256];
        char last[64];
        uint32_t timestamp = 0;
    };

    static State state;

    void setTimestamp() {
        state.timestamp = millis();
    }

    // Public
    void setLast(const char* value) {
        snprintf(state.last, sizeof(state.last), "%s", value);
        setTimestamp();
    }

    const char* getLast() {
        return state.last;
    }


    uint32_t getTimestamp() {
        return state.timestamp;
    }


    void setSenseLine(const char* value) {
        strncpy(state.senseLine, value, sizeof(state.senseLine) - 1);
        state.senseLine[sizeof(state.senseLine) - 1] = '\0';
    }
}