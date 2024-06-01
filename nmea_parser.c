#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "nmea_parser.h"

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

void nmea_init(navData_t *navData, const char *talker, const char *begin_from) {
  strncpy(navData->talker, talker, sizeof(navData->talker));
  strncpy(navData->begin_from, begin_from, sizeof(navData->begin_from));
  navData->cycle = 0;
#if NMEA_GSV
  navData->gsv.sat_info = (xxGSV_sat_t *)malloc(sizeof(xxGSV_sat_t));
  navData->gsv.checksum = (unsigned short *)malloc(sizeof(unsigned short));
#endif
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

  sscanf(data, "%f,%c,%f,%c,%f,%c,%f,%f,%u,%f,%c*%hx", &rmc->time, &rmc->status,
         &rmc->lat, &rmc->lat_dir, &rmc->lon, &rmc->lon_dir, &rmc->speed,
         &rmc->course, &rmc->date, &rmc->mg_var, &rmc->mg_dir, &rmc->checksum);
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

unsigned int populate_gsv(char *nmea, xxGSV_t *gsv) {

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
  if (gsv->sat_iteriation == gsv->sat_count) {
    return 1;
  } else {
    return 0;
  }
}

void populate_gll(const char *nmea, xxGLL_t *gll) {
  clear_gll(gll);
  const char *data = nmea + 7;
  sscanf(data, "%f,%c,%f,%c,%f,%c*%hx", &gll->lat, &gll->lat_dir, &gll->lon,
         &gll->lon_dir, &gll->utc_time, &gll->status, &gll->checksum);
}

void nmea_free(navData_t *navData) {
#if NMEA_GSV
  clear_gsv(&navData->gsv);
#endif
#if NMEA_RMC
  clear_rmc(&navData->rmc);
#endif
#if NMEA_GGA
  clear_gga(&navData->gga);
#endif
#if NMEA_VTG
  clear_vtg(&navData->vtg);
#endif
#if NMEA_GSA
  clear_gsa(&navData->gsa);
#endif
#if NMEA_GLL
  clear_gll(&navData->gll);
#endif
}

int nmea_parse(nmeaBuffer_t *nmea, navData_t *navData) {
  if (strlen(nmea->str) == 0) {
    return 1;
  }
  if (strncmp(nmea->str + 1, navData->talker, 2)) {
    return 1;
  }
  if ( (strncmp(nmea->str + 3, navData->begin_from, 3) == 0) || (navData->cycle == NMEA_OBJECT_SUM) ){
    navData->cycle = 0;
  }
  preprocess_nmea(nmea);
  char *nmea_str = nmea->str;
  if (strncmp(nmea_str + 3, "RMC", 3) == 0) {
#if NMEA_RMC
    populate_rmc(nmea_str, &navData->rmc);
    navData->cycle++;
#endif
  } else if (strncmp(nmea_str + 3, "GGA", 3) == 0) {
#if NMEA_GGA
    populate_gga(nmea_str, &navData->gga);
    navData->cycle++;
#endif
  } else if (strncmp(nmea_str + 3, "VTG", 3) == 0) {
#if NMEA_VTG
    populate_vtg(nmea_str, &navData->vtg);
    navData->cycle++;
#endif
  } else if (strncmp(nmea_str + 3, "GSA", 3) == 0) {
#if NMEA_GSA
    populate_gsa(nmea_str, &navData->gsa);
    navData->cycle++;
#endif
  } else if (strncmp(nmea_str + 3, "GSV", 3) == 0) {
#if NMEA_GSV
    navData->cycle += populate_gsv(nmea_str, &navData->gsv);
#endif
  } else if (strncmp(nmea_str + 3, "GLL", 3) == 0) {
#if NMEA_GLL
    populate_gll(nmea_str, &navData->gll);
    navData->cycle++;
#endif
  } else {
    return 1;
  }
  return 0;
}

#ifdef NMEA_PRINT
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

void print_vtg(const xxVTG_t *vtg) {
  printf("Degrees: %f\n", vtg->degrees);
  printf("State: %c\n", vtg->state);
  printf("Degrees 2: %f\n", vtg->degrees2);
  printf("Magnetic Sign: %c\n", vtg->magnetic_sign);
  printf("Speed Knots: %f\n", vtg->speed_knots);
  printf("Knots: %c\n", vtg->knots);
  printf("Speed km/h: %f\n", vtg->speed_kmh);
  printf("km/h: %c\n", vtg->kmh);
  printf("Checksum: %hx\n", vtg->checksum);
}

void print_gsa(const xxGSA_t *gsa) {
  printf("Selection Mode: %c\n", gsa->sel_mode);
  printf("Mode: %c\n", gsa->mode);
  for (size_t i = 0; i < 12; i++) {
    printf("Satellite ID: %hd\n", gsa->sat_id[i]);
  }
  printf("PDOP: %f\n", gsa->pdop);
  printf("HDOP: %f\n", gsa->hdop);
  printf("VDOP: %f\n", gsa->vdop);
  printf("Checksum: %hx\n", gsa->checksum);
}

void print_gll(const xxGLL_t *gll) {
  printf("Latitude: %f\n", gll->lat);
  printf("Latitude Direction: %c\n", gll->lat_dir);
  printf("Longitude: %f\n", gll->lon);
  printf("Longitude Direction: %c\n", gll->lon_dir);
  printf("UTC Time: %f\n", gll->utc_time);
  printf("Status: %c\n", gll->status);
  printf("Checksum: %hx\n", gll->checksum);
}
#endif
