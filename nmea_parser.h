
typedef struct {
  // $--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxx,x.x,a*hh
  float time;              // 1) Time (UTC)
  char status;             // 2) Status, V = Navigation receiver warning
  float lat;               // 3) Latitude
  char lat_dir;            // 4) N or S
  float lon;               // 5) Longitude
  char lon_dir;            // 6) E or W
  float speed;             // 7) Speed over ground, knots
  float course;            // 8) Track made good, degrees true
  unsigned short date;     // 9) Date, ddmmyy
  float mg_var;            // 10) Magnetic Variation, degrees
  char mg_dir;             // 11) E or W
  unsigned short checksum; // 12) Checksum
} gpsGPRMC_t;

typedef struct {

  // $--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
  float time;             // 1) Time (UTC)
  float lat;              // 2) Latitude
  char lat_dir;           // 3) N or S (North or South)
  float lon;              // 4) Longitude
  char lon_dir;           // 5) E or W (East or West)
  unsigned short quality; // 6) GPS Quality Indicator,
  // 0 - fix not available,
  // 1 - GPS fix,
  // 2 - Differential GPS fix
  unsigned short sat_count; // 7) Number of satellites in view, 00 - 12
  float hdop;               // 8) Horizontal Dilution of precision
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
  unsigned short rs_id;    // 14) Differential reference station ID, 0000-1023
  unsigned short checksum; // 15) Checksum

} gpsGPGGA_t;

typedef struct {
  // $--VTG,x.x,T,x.x,M,x.x,N,x.x,K*hh
  float degrees;           // 1) Track Degrees
  char state;              // 2) T = True
  float degrees2;          // 3) Track Degrees
  char magnetic_sign;      // 4) M = Magnetic
  float speed_knots;       // 5) Speed Knots
  char knots;              // 6) N = Knots
  float speed_kmh;         // 7) Speed Kilometers Per Hour
  char kmh;                // 8) K = Kilometres Per Hour
  unsigned short checksum; // 9) Checksum
} gpsGPVTG_t;

typedef struct {

  // $--GSA,a,a,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x.x,x.x,x.x*hh
  char sel_mode; // 1) Selection mode
  char mode;     // 2) Mode
  // 3) ID of 1st satellite used for fix
  // 4) ID of 2nd satellite used for fix
  short sat_id[12]; // ...
  // 14) ID of 12th satellite used for fix
  float pdop;              // 15) PDOP in meters
  float hdop;              // 16) HDOP in meters
  float vdop;              // 17) VDOP in meters
  unsigned short checksum; // 18) Checksum

} gpsGPGSA_t;

typedef struct {
  short sat_num;    // 4) satellite number
  short eleveation; // 5) elevation in degrees
  short azimuth;    // 6) azimuth in degrees to true
  short snr;        // 7) SNR in dB
  // more satellite infos like 4)-7)
} gpsGPGSV_sat_t;

typedef struct {
  // $--GSV,x,x,x,x,x,x,x,...*hh
  short mes_count;          // 1) total number of messages
  short mes_num;            // none // 2) message number
  short sat_count;          // 3) satellites in view
  short checksum;           // 8) Checksum
  gpsGPGSV_sat_t *sat_info; // 4) satellite infos
  // to be held as heap
} gpsGPGSV_t;

typedef struct {
  gpsGPRMC_t gprmc;
  gpsGPGGA_t gpgga;
  gpsGPVTG_t gpvtg;
  gpsGPGSA_t gpgsa;
  gpsGPGSV_t gpgsv;
} gpsData_t;
