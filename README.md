# GPSClock


# Workaround
Included NeoGPS Library to override variable name, which duplicates with Timezone library definition.

Edited file: NeoGPS/src/NeoTime.h
`"DAYS_PER_WEEK"` > `"DAYS_COUNT_PER_WEEK"`