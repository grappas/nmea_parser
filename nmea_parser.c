#include "nmea_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void preprocess_nmea(char *nmea) {
  char buffer[256];
  char *src = nmea;
  char *dst = buffer;
  char *end = buffer + sizeof(buffer) - 1;

  while (*src && dst < end) {
    if (*src == ',' && (*(src + 1) == ',' || *(src + 1) == '*')) {
      *dst++ = ',';
      *dst++ = '0';
    } else {
      *dst++ = *src;
    }
    src++;
  }
  *dst = '\0';
  strcpy(nmea, buffer);
}

void nmea_set_talker(navData_t *navData, const char *talker) {
  strncpy(navData->talker, talker, sizeof(navData->talker));
}

void clear_rmc(xxRMC_t *rmc) { memset(rmc, 0, sizeof(xxRMC_t)); }
void clear_gga(xxGGA_t *gga) { memset(gga, 0, sizeof(xxGGA_t)); }
void clear_vtg(xxVTG_t *vtg) { memset(vtg, 0, sizeof(xxVTG_t)); }
void clear_gsa(xxGSA_t *gsa) { memset(gsa, 0, sizeof(xxGSA_t)); }
void clear_gll(xxGLL_t *gll) { memset(gll, 0, sizeof(xxGLL_t)); }
void free_gsv_sat(xxGSV_t *gsv) {
  if (gsv->sat_info) {
    free(gsv->sat_info);
    gsv->sat_info = NULL;
    free(gsv->checksum);
    gsv->checksum = NULL;
  }
}
void clear_gsv(xxGSV_t *gsv) {
  free_gsv_sat(gsv);
  memset(gsv, 0, sizeof(xxGSV_t));
}

void populate_rmc(const char *nmea, xxRMC_t *rmc) {
  clear_rmc(rmc);
  const char *data = nmea + 7;

  sscanf(data, "%f,%c,%f,%c,%f,%c,%f,%f,%hu,%f,%c*%hx", &rmc->time,
         &rmc->status, &rmc->lat, &rmc->lat_dir, &rmc->lon, &rmc->lon_dir,
         &rmc->speed, &rmc->course, &rmc->date, &rmc->mg_var, &rmc->mg_dir,
         &rmc->checksum);
}

void populate_gga(const char *nmea, xxGGA_t *gga) {
  clear_gga(gga);
  const char *data = nmea + 7;
  sscanf(data, "%f,%f,%c,%f,%c,%hu,%hu,%f,%f,%c,%f,%c,%f,%hu*%hx", &gga->time,
         &gga->lat, &gga->lat_dir, &gga->lon, &gga->lon_dir, &gga->quality,
         &gga->sat_count, &gga->hdop, &gga->alt, &gga->unit_alt,
         &gga->geoid_sep, &gga->unit_geoid_sep, &gga->age, &gga->rs_id,
         &gga->checksum);
}

void populate_vtg(const char *nmea, xxVTG_t *vtg) {
  clear_vtg(vtg);
  const char *data = nmea + 7;
  sscanf(data, "%f,%c,%f,%c,%f,%c,%f,%c*%hx", &vtg->degrees, &vtg->state,
         &vtg->degrees2, &vtg->magnetic_sign, &vtg->speed_knots, &vtg->knots,
         &vtg->speed_kmh, &vtg->kmh, &vtg->checksum);
}

void populate_gsa(const char *nmea, xxGSA_t *gsa) {
  clear_gsa(gsa);
  const char *data = nmea + 7;
  sscanf(data,
         "%c,%c,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%f,%f,%f*%hx",
         &gsa->sel_mode, &gsa->mode, &gsa->sat_id[0], &gsa->sat_id[1],
         &gsa->sat_id[2], &gsa->sat_id[3], &gsa->sat_id[4], &gsa->sat_id[5],
         &gsa->sat_id[6], &gsa->sat_id[7], &gsa->sat_id[8], &gsa->sat_id[9],
         &gsa->sat_id[10], &gsa->sat_id[11], &gsa->pdop, &gsa->hdop, &gsa->vdop,
         &gsa->checksum);
}

void populate_gsv(const char *nmea, xxGSV_t *gsv) {
  const char *data = nmea + 7;
  sscanf(data, "%hd,%hd,%hd,", &gsv->mes_count, &gsv->mes_num, &gsv->sat_count);
  if (gsv->mes_num == 1) {
    clear_gsv(gsv);
  }
  gsv->sat_info = (xxGSV_sat_t *)malloc(gsv->sat_count * sizeof(xxGSV_sat_t));
  gsv->checksum =
      (unsigned short *)malloc(gsv->mes_count * sizeof(unsigned short));
  if (gsv->sat_info) {
    for (int i = 0; i < gsv->sat_count; i++) {
      xxGSV_sat_t *sat = &gsv->sat_info[i];
      sscanf(data, "%hd,%hd,%hd,%hd", &sat->sat_num, &sat->elevation,
             &sat->azimuth, &sat->snr);
    }
  }
}

void populate_gll(const char *nmea, xxGLL_t *gll) {
  clear_gll(gll);
  const char *data = nmea + 7;
  sscanf(data, "%f,%c,%f,%c,%f,%c*%hx", &gll->lat, &gll->lat_dir, &gll->lon,
         &gll->lon_dir, &gll->utc_time, &gll->status, &gll->checksum);
}

// // use this for strict string size
// typedef struct {
//   char str[256];
// }nmeaBuffer_t;
//
// typedef struct {
//   // $--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxx,x.x,a*hh
//   float time;              // 1) Time (UTC)
//   char status;             // 2) Status, V = Navigation receiver warning
//   float lat;               // 3) Latitude
//   char lat_dir;            // 4) N or S
//   float lon;               // 5) Longitude
//   char lon_dir;            // 6) E or W
//   float speed;             // 7) Speed over ground, knots
//   float course;            // 8) Track made good, degrees true
//   unsigned short date;     // 9) Date, ddmmyy
//   float mg_var;            // 10) Magnetic Variation, degrees
//   char mg_dir;             // 11) E or W
//   unsigned short checksum; // 12) Checksum
// } xxRMC_t;
//
// typedef struct {
//
//   // $--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
//   float time;             // 1) Time (UTC)
//   float lat;              // 2) Latitude
//   char lat_dir;           // 3) N or S (North or South)
//   float lon;              // 4) Longitude
//   char lon_dir;           // 5) E or W (East or West)
//   unsigned short quality; // 6) GPS Quality Indicator,
//   // 0 - fix not available,
//   // 1 - GPS fix,
//   // 2 - Differential GPS fix
//   unsigned short sat_count; // 7) Number of satellites in view, 00 - 12
//   float hdop;               // 8) Horizontal Dilution of precision
//   float alt;       // 9) Antenna Altitude above/below mean-sea-level (geoid)
//   char unit_alt;   // 10) Units of antenna altitude, meters
//   float geoid_sep; // 11) Geoidal separation, the difference between the
//   WGS-84
//                    // earth
//   // ellipsoid and mean-sea-level (geoid), "-" means mean-sea-level below
//   // ellipsoid
//   char unit_geoid_sep; // 12) Units of geoidal separation, meters
//   float
//       age; // 13) Age of differential GPS data, time in seconds since last
//       SC104
//   // type 1 or 9 update, null field when DGPS is not used
//   unsigned short rs_id;    // 14) Differential reference station ID,
//   0000-1023 unsigned short checksum; // 15) Checksum
//
// } xxGGA_t;
//
// typedef struct {
//   // $--VTG,x.x,T,x.x,M,x.x,N,x.x,K*hh
//   float degrees;           // 1) Track Degrees
//   char state;              // 2) T = True
//   float degrees2;          // 3) Track Degrees
//   char magnetic_sign;      // 4) M = Magnetic
//   float speed_knots;       // 5) Speed Knots
//   char knots;              // 6) N = Knots
//   float speed_kmh;         // 7) Speed Kilometers Per Hour
//   char kmh;                // 8) K = Kilometres Per Hour
//   unsigned short checksum; // 9) Checksum
// } xxVTG_t;
//
// typedef struct {
//
//   // $--GSA,a,a,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x.x,x.x,x.x*hh
//   char sel_mode; // 1) Selection mode
//   char mode;     // 2) Mode
//   // 3) ID of 1st satellite used for fix
//   // 4) ID of 2nd satellite used for fix
//   unsigned short sat_id[12]; // ...
//   // 14) ID of 12th satellite used for fix
//   float pdop;              // 15) PDOP in meters
//   float hdop;              // 16) HDOP in meters
//   float vdop;              // 17) VDOP in meters
//   unsigned short checksum; // 18) Checksum
//
// } xxGSA_t;
//
// typedef struct {
//   unsigned short sat_num;    // 4) satellite number
//   unsigned short elevation; // 5) elevation in degrees
//   unsigned short azimuth;    // 6) azimuth in degrees to true
//   unsigned short snr;        // 7) SNR in dB
//   // more satellite infos like 4)-7)
// } xxGSV_sat_t;
//
// typedef struct {
//   // $--GSV,x,x,x,x,x,x,x,...*hh
//   unsigned short mes_count;          // 1) total number of messages
//   unsigned short mes_num;            // none // 2) message number
//   unsigned short sat_count;          // 3) satellites in view
//   xxGSV_sat_t *sat_info; // 4) satellite infos
//   unsigned short *checksum;           // 8) Checksum
//   // to be held as heap
// } xxGSV_t;
//
// typedef struct {
//   // GLL
//   // Geographic Position – Latitude/Longitude
//   // $--GLL,llll.ll,a,yyyyy.yy,a,hhmmss.ss,A*hh
//   float lat;      // 1) Latitude
//   char lat_dir;   // 2) N or S (North or South)
//   float lon;      // 3) Longitude
//   char lon_dir;   // 4) E or W (East or West)
//   float utc_time; // 5) Time (UTC)
//   char status;    // 6) Status A - Data Valid, V - Data Invalid
//   short checksum; // 7) Checksum
// } xxGLL_t;
//
// typedef struct {
//   char talker[3]; // Navigation system e.g. GPS - GP, GLONASS - GL, etc.
//   xxRMC_t rmc;
//   xxGGA_t gga;
//   xxVTG_t vtg;
//   xxGSA_t gsa;
//   xxGSV_t gsv;
//   xxGLL_t gll;
// } navData_t;
//
// void nmea_set_talker(navData_t *navData, const char *talker);
// int nmea_parse(nmeaBuffer_t *nmea, navData_t *navData);
// void nmea_free(navData_t *navData);
// void populate_rmc(const char *nmea, xxRMC_t *rmc);
// void clear_rmc(xxRMC_t *rmc);
// void populate_gga(const char *nmea, xxGGA_t *gga);
// void clear_gga(xxGGA_t *gga);
// void populate_vtg(const char *nmea, xxVTG_t *vtg);
// void clear_vtg(xxVTG_t *vtg);
// void populate_gsa(const char *nmea, xxGSA_t *gsa);
// void clear_gsa(xxGSA_t *gsa);
// void populate_gsv(const char *nmea, xxGSV_t *gsv);
// void clear_gsv(xxGSV_t *gsv);
// void free_gsv_sat(xxGSV_t *gsv);
// void populate_gll(const char *nmea, xxGLL_t *gll);
// void clear_gll(xxGLL_t *gll);
// void preprocess_nmea(char *nmea);
