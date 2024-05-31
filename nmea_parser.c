#include "nmea_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void preprocess_nmea(nmeaBuffer_t *nmea) {
  char buffer[NMEA_BUFFER_SIZE];
  char *src = nmea->str;
  char *dst = buffer;
  char *end = buffer + sizeof(buffer) - 1;
  int leading_zero = 1;

  while (*src && dst < end) {
    if (*src == ',' && (*(src + 1) == ',' || *(src + 1) == '*')) {
      *dst++ = ',';
      *dst++ = '0';
      leading_zero = 1;
    } else if (*src == ',') {
      *dst++ = *src;
      leading_zero = 1;
    } else if (*src == '0' && leading_zero) {
      // Skip leading zero
      if (*(src + 1) >= '0' && *(src + 1) <= '9') {
        src++;
        continue;
      } else {
        *dst++ = '0';
        leading_zero = 0;
      }
    } else {
      *dst++ = *src;
      leading_zero = 0;
    }
    src++;
  }
  *dst = '\0';
  strcpy(nmea->str, buffer);
}

void nmea_init(navData_t *navData, const char *talker) {
  strncpy(navData->talker, talker, sizeof(navData->talker));
  navData->gsv.sat_info = (xxGSV_sat_t *)malloc(sizeof(xxGSV_sat_t));
  navData->gsv.checksum = (unsigned short *)malloc(sizeof(unsigned short));
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
  printf("nmea: %s\n", nmea);
  const char *data = nmea + 7;

  sscanf(data, "%f,%c,%f,%c,%f,%c,%f,%f,%u,%f,%c*%hx", &rmc->time,
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

void populate_gsv(char *nmea, xxGSV_t *gsv) {

  int characters_read = 0;
  char *data = nmea + 7;
  char *asterisk_position = strchr(data, '*');

  sscanf(data, "%hd,%hd,%hd,%n", &gsv->mes_count, &gsv->mes_num,
         &gsv->sat_count, &characters_read);

  if (gsv->mes_num == 1) {
    clear_gsv(gsv);
    sscanf(data, "%hd,%hd,%hd,%n", &gsv->mes_count, &gsv->mes_num,
           &gsv->sat_count, &characters_read);
    gsv->sat_info = (xxGSV_sat_t *)malloc(gsv->sat_count * sizeof(xxGSV_sat_t));
    gsv->checksum =
        (unsigned short *)malloc(gsv->mes_count * sizeof(unsigned short));
    if (!gsv->sat_info || !gsv->checksum) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(EXIT_FAILURE);
    }
  }
  data += characters_read;

  if (asterisk_position != NULL) {
    // Save the value after the asterisk as a string
    char value_str[3];
    strncpy(value_str, asterisk_position + 1, 2);
    value_str[2] = '\0';

    // Convert the string value to unsigned short
    gsv->checksum[gsv->mes_num - 1] =
        (unsigned short)strtoul(value_str, NULL, 16);

    // Replace the asterisk and its value with a comma
    *asterisk_position = ',';
    *(asterisk_position + 1) = '\0';
  }

  while ((data[0] != '\0') && (gsv->sat_iteriation < gsv->sat_count)) {
    xxGSV_sat_t *sat = &gsv->sat_info[gsv->sat_iteriation];
    sscanf(data, "%hd,%hd,%hd,%hd,%n", &sat->sat_num, &sat->elevation,
           &sat->azimuth, &sat->snr, &characters_read);
    gsv->sat_iteriation++;
    data += characters_read;
  }
}

void populate_gll(const char *nmea, xxGLL_t *gll) {
  clear_gll(gll);
  const char *data = nmea + 7;
  sscanf(data, "%f,%c,%f,%c,%f,%c*%hx", &gll->lat, &gll->lat_dir, &gll->lon,
         &gll->lon_dir, &gll->utc_time, &gll->status, &gll->checksum);
}

void nmea_free(navData_t *navData) {
  clear_gsv(&navData->gsv);
  clear_rmc(&navData->rmc);
  clear_gga(&navData->gga);
  clear_vtg(&navData->vtg);
  clear_gsa(&navData->gsa);
  clear_gll(&navData->gll);
}

int nmea_parse(nmeaBuffer_t *nmea, navData_t *navData) {
  if (strlen(nmea->str) == 0) {
    return 1;
  }
  if (strncmp(nmea->str + 1, navData->talker, 2)) {
    return 1;
  }
  preprocess_nmea(nmea);
  char *nmea_str = nmea->str;
  if (strncmp(nmea_str + 3, "RMC", 3) == 0) {
    populate_rmc(nmea_str, &navData->rmc);
  } else if (strncmp(nmea_str + 3, "GGA", 3) == 0) {
    populate_gga(nmea_str, &navData->gga);
  } else if (strncmp(nmea_str + 3, "VTG", 3) == 0) {
    populate_vtg(nmea_str, &navData->vtg);
  } else if (strncmp(nmea_str + 3, "GSA", 3) == 0) {
    populate_gsa(nmea_str, &navData->gsa);
  } else if (strncmp(nmea_str + 3, "GSV", 3) == 0) {
    populate_gsv(nmea_str, &navData->gsv);
  } else if (strncmp(nmea_str + 3, "GLL", 3) == 0) {
    populate_gll(nmea_str, &navData->gll);
  } else {
    return 1;
  }
  return 0;
}

void print_rmc(const xxRMC_t *rmc) {
  printf("Time: %f\n", rmc->time);
  printf("Status: %c\n", rmc->status);
  printf("Latitude: %f\n", rmc->lat);
  printf("Latitude Direction: %c\n", rmc->lat_dir);
  printf("Longitude: %f\n", rmc->lon);
  printf("Longitude Direction: %c\n", rmc->lon_dir);
  printf("Speed: %f\n", rmc->speed);
  printf("Course: %f\n", rmc->course);
  printf("Date: %u\n", rmc->date);
  printf("Magnetic Variation: %f\n", rmc->mg_var);
  printf("Magnetic Direction: %c\n", rmc->mg_dir);
  printf("Checksum: %hx\n", rmc->checksum);
}

void print_gga(const xxGGA_t *gga) {
  printf("Time: %f\n", gga->time);
  printf("Latitude: %f\n", gga->lat);
  printf("Latitude Direction: %c\n", gga->lat_dir);
  printf("Longitude: %f\n", gga->lon);
  printf("Longitude Direction: %c\n", gga->lon_dir);
  printf("Quality: %hu\n", gga->quality);
  printf("Satellite Count: %hu\n", gga->sat_count);
  printf("HDOP: %f\n", gga->hdop);
  printf("Altitude: %f\n", gga->alt);
  printf("Unit Altitude: %c\n", gga->unit_alt);
  printf("Geoidal Separation: %f\n", gga->geoid_sep);
  printf("Unit Geoidal Separation: %c\n", gga->unit_geoid_sep);
  printf("Age: %f\n", gga->age);
  printf("Reference Station ID: %hu\n", gga->rs_id);
  printf("Checksum: %hx\n", gga->checksum);
}

void print_gsv(const xxGSV_t *gsv) {
  if (gsv->mes_count == gsv->mes_num) {
    printf("Message Count: %hu\n", gsv->mes_count);
    printf("Message Number: %hu\n", gsv->mes_num);
    printf("Satellite Count: %hu\n", gsv->sat_count);
    for (size_t i = 0; i < gsv->sat_count; i++) {
      xxGSV_sat_t *sat = &gsv->sat_info[i];
      printf("Satellite Number: %hu\n", sat->sat_num);
      printf("Elevation: %hu\n", sat->elevation);
      printf("Azimuth: %hu\n", sat->azimuth);
      printf("SNR: %hu\n", sat->snr);
    }
    for (size_t i = 0; i < gsv->mes_count; i++) {
      printf("Checksum: %hx\n", gsv->checksum[i]);
    }
  }
}
// // use this for strict string size
// //
// #define NMEA_BUFFER_SIZE 256
// typedef struct {
//   char str[NMEA_BUFFER_SIZE];
// } nmeaBuffer_t;
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
//   unsigned short sat_num;   // 4) satellite number
//   unsigned short elevation; // 5) elevation in degrees
//   unsigned short azimuth;   // 6) azimuth in degrees to true
//   unsigned short snr;       // 7) SNR in dB
//   // more satellite infos like 4)-7)
// } xxGSV_sat_t;
//
// typedef struct {
//   // $--GSV,x,x,x,x,x,x,x,...*hh
//   unsigned short mes_count; // 1) total number of messages
//   unsigned short mes_num;   // none // 2) message number
//   unsigned short sat_count; // 3) satellites in view
//   xxGSV_sat_t *sat_info;    // 4) satellite infos
//   unsigned short *checksum; // 8) Checksum
//   //
//   unsigned short sat_iteriation; // determine how many satellites are parsed
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
// // do it right after creating the navData_t eg. nmea_set_talker(&navData,
// "GP"); void nmea_set_talker(navData_t *navData, const char *talker);
// // parsing functions
// int nmea_parse(nmeaBuffer_t *nmea, navData_t *navData);
// // clear the navData_t
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
// void preprocess_nmea(nmeaBuffer_t *nmea);
// void print_rmc(const xxRMC_t *rmc);
// void print_gga(const xxGGA_t *gga);
// void print_gsv(const xxGSV_t *gsv);
