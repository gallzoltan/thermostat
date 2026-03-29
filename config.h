#pragma once

// Hardware pin kiosztás
#define ONE_WIRE_BUS  0
#define PIN_CLK      14
#define PIN_DT       12
#define PIN_SW       13
#define PIN_RELAY    15
#define PIN_SCL       4
#define PIN_SDA       5

// Termosztát konstansok
#define MAX_TEMP     64      // virtualPosition felső határa (32°C)
#define MIN_TEMP     20      // virtualPosition alsó határa (10°C)
#define DELTA        0.2f    // hisztézis (°C)

// Szerver
#define SERVER_URL   "http://thermo"
