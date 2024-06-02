// use this for strict string size
//
#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#ifndef NMEA_RMC_ENABLED
#define NMEA_RMC_ENABLED 1
#endif

#ifndef NMEA_GGA_ENABLED
#define NMEA_GGA_ENABLED 1
#endif

#ifndef NMEA_VTG_ENABLED
#define NMEA_VTG_ENABLED 1
#endif

#ifndef NMEA_GSA_ENABLED
#define NMEA_GSA_ENABLED 1
#endif

#ifndef NMEA_GSV_ENABLED
#define NMEA_GSV_ENABLED 1
#endif

#ifndef NMEA_GLL_ENABLED
#define NMEA_GLL_ENABLED 1
#endif

#ifndef NMEA_BUFFER_SIZE
#define NMEA_BUFFER_SIZE 256
#endif

#endif // NMEA_PARSER_H
typedef struct {
  char str[NMEA_BUFFER_SIZE];
} nmeaBuffer_t;

typedef struct {
  // $--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxx,x.x,a*hh
  float time;             // 1) Time (UTC)
  char status;            // 2) Status, V = Navigation receiver warning
  float lat;              // 3) Latitude
  char lat_dir;           // 4) N or S
  float lon;              // 5) Longitude
  char lon_dir;           // 6) E or W
  float speed;            // 7) Speed over ground, knots
  float course;           // 8) Track made good, degrees true
  unsigned int date;      // 9) Date, ddmmyy
  float mg_var;           // 10) Magnetic Variation, degrees
  char mg_dir;            // 11) E or W
  char checksum_mode;     // 12) Mode indicator
  unsigned char checksum; // 13) Checksum
} xxRMC_t;

typedef struct {

  // $--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
  float time;            // 1) Time (UTC)
  float lat;             // 2) Latitude
  char lat_dir;          // 3) N or S (North or South)
  float lon;             // 4) Longitude
  char lon_dir;          // 5) E or W (East or West)
  unsigned char quality; // 6) GPS Quality Indicator,
  // 0 - fix not available,
  // 1 - GPS fix,
  // 2 - Differential GPS fix
  unsigned char sat_count; // 7) Number of satellites in view, 00 - 12
  float hdop;              // 8) Horizontal Dilution of precision
  float alt;       // 9) Antenna Altitude above/below mean-sea-level (geoid)
  char unit_alt;   // 10) Units of antenna altitude, meters
  float geoid_sep; // 11) Geoidal separation, the difference between the WGS-84
                   // earth
  // ellipsoid and mean-sea-level (geoid), "-" means mean-sea-level below
  // ellipsoid
  char unit_geoid_sep; // 12) Units of geoidal separation, meters
  float
      age; // 13) Age of differential GPS data, time in seconds since last SC104
  // type 1 or 9 update, null field when DGPS is not used
  unsigned short rs_id;   // 14) Differential reference station ID, 0000-1023
  unsigned char checksum; // 15) Checksum

} xxGGA_t;

typedef struct {
  // $--VTG,x.x,T,x.x,M,x.x,N,x.x,K*hh
  float degrees;          // 1) Track Degrees
  char state;             // 2) T = True
  float degrees2;         // 3) Track Degrees
  char magnetic_sign;     // 4) M = Magnetic
  float speed_knots;      // 5) Speed Knots
  char knots;             // 6) N = Knots
  float speed_kmh;        // 7) Speed Kilometers Per Hour
  char kmh;               // 8) K = Kilometres Per Hour
  char checksum_mode;     // 	Mode indicator:
                          // A: Autonomous mode
                          // D: Differential mode
                          // E: Estimated (dead reckoning) mode
                          // M: Manual Input mode
                          // S: Simulator mode
                          // N: Data not valid
  unsigned char checksum; // 9) Checksum
} xxVTG_t;

typedef struct {

  // $--GSA,a,a,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x.x,x.x,x.x*hh
  char sel_mode; // 1) Selection mode
  char mode;     // 2) Mode
  // 3) ID of 1st satellite used for fix
  // 4) ID of 2nd satellite used for fix
  unsigned char sat_id[12]; // ...
  // 14) ID of 12th satellite used for fix
  float pdop;             // 15) PDOP in meters
  float hdop;             // 16) HDOP in meters
  float vdop;             // 17) VDOP in meters
  unsigned char checksum; // 18) Checksum

} xxGSA_t;

typedef struct {
  unsigned char sat_num;   // 4) satellite number
  unsigned char elevation; // 5) elevation in degrees
  unsigned short azimuth;  // 6) azimuth in degrees to true
  unsigned char snr;       // 7) SNR in dB
  // more satellite infos like 4)-7)
} xxGSV_sat_t;

typedef struct {
  // $--GSV,x,x,x,x,x,x,x,...*hh
  unsigned char mes_count; // 1) total number of messages
  unsigned char mes_num;   // none // 2) message number
  unsigned char sat_count; // 3) satellites in view
  xxGSV_sat_t *sat_info;   // 4) satellite infos
  unsigned char *checksum; // 8) Checksum
  //
  unsigned char sat_iteriation; // determine how many satellites are parsed
  // to be held as heap
} xxGSV_t;

typedef struct {
  // GLL
  // Geographic Position – Latitude/Longitude
  // $--GLL,llll.ll,a,yyyyy.yy,a,hhmmss.ss,A*hh
  float lat;              // 1) Latitude
  char lat_dir;           // 2) N or S (North or South)
  float lon;              // 3) Longitude
  char lon_dir;           // 4) E or W (East or West)
  float utc_time;         // 5) Time (UTC)
  char status;            // 6) Status A - Data Valid, V - Data Invalid
  char checksum_mode;     // 7) Mode indicator
  unsigned char checksum; // 7) Checksum
} xxGLL_t;

typedef struct {
  char talker[3];     // Navigation system e.g. GPS - GP, GLONASS - GL, etc.
  char begin_from[4]; // Start parsing from this NMEA sentence
  unsigned char cycles_max; // cycle count
  unsigned char cycle;      // cycle count
#if NMEA_RMC_ENABLED
  xxRMC_t *rmc;
#endif
#if NMEA_GGA_ENABLED
  xxGGA_t *gga;
#endif
#if NMEA_VTG_ENABLED
  xxVTG_t *vtg;
#endif
#if NMEA_GSA_ENABLED
  xxGSA_t *gsa;
#endif
#if NMEA_GSV_ENABLED
  xxGSV_t *gsv;
#endif
#if NMEA_GLL_ENABLED
  xxGLL_t *gll;
#endif
} navData_t;

// do it right after creating the navData_t eg. nmea_set_talker(&navData, "GP");
void nmea_init(navData_t *navData, const char *talker, const char *begin_from);
// parsing functions
int nmea_parse(nmeaBuffer_t *nmea, navData_t *navData);
// clear the navData_t
void nmea_free(navData_t *navData);
void nmea_nullify(navData_t *navData);
#if NMEA_RMC_ENABLED
void populate_rmc(const char *nmea, xxRMC_t *rmc);
void clear_rmc(xxRMC_t *rmc);
#endif
#if NMEA_GGA_ENABLED
void populate_gga(char *nmea, xxGGA_t *gga);
void clear_gga(xxGGA_t *gga);
#endif
#if NMEA_VTG_ENABLED
void populate_vtg(const char *nmea, xxVTG_t *vtg);
void clear_vtg(xxVTG_t *vtg);
#endif
#if NMEA_GSA_ENABLED
void populate_gsa(const char *nmea, xxGSA_t *gsa);
void clear_gsa(xxGSA_t *gsa);
#endif
#if NMEA_GSV_ENABLED
unsigned int populate_gsv(char *nmea, xxGSV_t *gsv);
void clear_gsv(xxGSV_t *gsv);
void free_gsv_sat(xxGSV_t *gsv);
#endif
#if NMEA_GLL_ENABLED
void populate_gll(const char *nmea, xxGLL_t *gll);
void clear_gll(xxGLL_t *gll);
#endif
void preprocess_nmea(nmeaBuffer_t *nmea);
#ifdef NMEA_PRINT
#if NMEA_RMC_ENABLED
void print_rmc(const navData_t *data);
#endif
#if NMEA_GGA_ENABLED
void print_gga(const navData_t *data);
#endif
#if NMEA_VTG_ENABLED
void print_gsv(const navData_t *data);
#endif
#if NMEA_GSA_ENABLED
void print_gsa(const navData_t *data);
#endif
#if NMEA_GSV_ENABLED
void print_vtg(const navData_t *data);
#endif
#if NMEA_GLL_ENABLED
void print_gll(const navData_t *data);
#endif
void print_nav(const navData_t *data);
#endif
