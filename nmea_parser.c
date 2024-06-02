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
  navData->cycles_max = 0;
  if (navData->rmc)
    navData->cycles_max++;
  if (navData->gga)
    navData->cycles_max++;
  if (navData->vtg)
    navData->cycles_max++;
  if (navData->gsa)
    navData->cycles_max++;
  if (navData->gsv) {
    navData->gsv->sat_info = (xxGSV_sat_t *)malloc(sizeof(xxGSV_sat_t));
    navData->gsv->checksum = (unsigned char *)malloc(sizeof(unsigned short));
    navData->cycles_max++;
  }
  if (navData->gll)
    navData->cycles_max++;
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

  sscanf(data, "%f,%c,%f,%c,%f,%c,%f,%f,%u,%f,%c,%c*%hhx", &rmc->time,
         &rmc->status, &rmc->lat, &rmc->lat_dir, &rmc->lon, &rmc->lon_dir,
         &rmc->speed, &rmc->course, &rmc->date, &rmc->mg_var, &rmc->mg_dir,
         &rmc->checksum_mode, &rmc->checksum);
}

void populate_gga(char *nmea, xxGGA_t *gga) {
  clear_gga(gga);
  char *data = nmea + 7;
  char *asterisk_position = strchr(data, '*');
  int characters_read = 0;
  sscanf(data, "%f,%f,%c,%f,%c,%hhu,%hhu,%f,%f,%c,%f,%c,%n", &gga->time,
         &gga->lat, &gga->lat_dir, &gga->lon, &gga->lon_dir, &gga->quality,
         &gga->sat_count, &gga->hdop, &gga->alt, &gga->unit_alt,
         &gga->geoid_sep, &gga->unit_geoid_sep, &characters_read);
  data += characters_read;

  if (asterisk_position != NULL) {
    // Save the value after the asterisk as a string
    char value_str[3];
    strncpy(value_str, asterisk_position + 1, 2);
    value_str[2] = '\0';

    // Convert the string value to unsigned short
    gga->checksum = (unsigned short)strtoul(value_str, NULL, 16);

    // Replace the asterisk and its value with a comma
    *asterisk_position = ',';
    *(asterisk_position + 1) = '\0';
  }

  if (strchr(data, ' ')) {
    sscanf(data, "%f %hu,", &gga->age, &gga->rs_id);
  }
}

void populate_vtg(const char *nmea, xxVTG_t *vtg) {
  clear_vtg(vtg);
  const char *data = nmea + 7;
  sscanf(data, "%f,%c,%f,%c,%f,%c,%f,%c,%c*%hhx", &vtg->degrees, &vtg->state,
         &vtg->degrees2, &vtg->magnetic_sign, &vtg->speed_knots, &vtg->knots,
         &vtg->speed_kmh, &vtg->kmh, &vtg->checksum_mode, &vtg->checksum);
}

void populate_gsa(const char *nmea, xxGSA_t *gsa) {
  clear_gsa(gsa);
  const char *data = nmea + 7;
  sscanf(data,
         "%c,%c,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%f,"
         "%f,%f*%hhx",
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

  sscanf(data, "%hhu,%hhu,%hhu,%n", &gsv->mes_count, &gsv->mes_num,
         &gsv->sat_count, &characters_read);

  if (gsv->mes_num == 1) {
    clear_gsv(gsv);
    sscanf(data, "%hhu,%hhu,%hhu,%n", &gsv->mes_count, &gsv->mes_num,
           &gsv->sat_count, &characters_read);
    gsv->sat_info = (xxGSV_sat_t *)malloc(gsv->sat_count * sizeof(xxGSV_sat_t));
    gsv->checksum =
        (unsigned char *)malloc(gsv->mes_count * sizeof(unsigned short));
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
    sscanf(data, "%hhu,%hhu,%hu,%hhu,%n", &sat->sat_num, &sat->elevation,
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
  sscanf(data, "%f,%c,%f,%c,%f,%c,%c*%hhx", &gll->lat, &gll->lat_dir, &gll->lon,
         &gll->lon_dir, &gll->utc_time, &gll->status, &gll->checksum_mode,
         &gll->checksum);
}

void nmea_free(navData_t *navData) {
  if (navData->gsv)
    clear_gsv(navData->gsv);
  if (navData->rmc)
    clear_rmc(navData->rmc);
  if (navData->gga)
    clear_gga(navData->gga);
  if (navData->vtg)
    clear_vtg(navData->vtg);
  if (navData->gsa)
    clear_gsa(navData->gsa);
  if (navData->gll)
    clear_gll(navData->gll);
}

void nmea_nullify(navData_t *navData) { memset(navData, 0, sizeof(navData_t)); }

int nmea_parse(nmeaBuffer_t *nmea, navData_t *navData) {
  if (strlen(nmea->str) == 0) {
    return 1;
  }
  if (strncmp(nmea->str + 1, navData->talker, 2)) {
    return 1;
  }
  if ((strncmp(nmea->str + 3, navData->begin_from, 3) == 0) ||
      (navData->cycle == navData->cycles_max)) {
    navData->cycle = 0;
  }
  preprocess_nmea(nmea);
  char *nmea_str = nmea->str;
  if ((strncmp(nmea_str + 3, "RMC", 3) == 0) && (navData->rmc)) {
    populate_rmc(nmea_str, navData->rmc);
    navData->cycle++;
  } else if ((strncmp(nmea_str + 3, "GGA", 3) == 0) && (navData->gga)) {
    populate_gga(nmea_str, navData->gga);
    navData->cycle++;
  } else if ((strncmp(nmea_str + 3, "VTG", 3) == 0) && (navData->vtg)) {
    populate_vtg(nmea_str, navData->vtg);
    navData->cycle++;
  } else if ((strncmp(nmea_str + 3, "GSA", 3) == 0) && (navData->gsa)) {
    populate_gsa(nmea_str, navData->gsa);
    navData->cycle++;
  } else if ((strncmp(nmea_str + 3, "GSV", 3) == 0) && (navData->gsv)) {
    navData->cycle += populate_gsv(nmea_str, &*navData->gsv);
  } else if ((strncmp(nmea_str + 3, "GLL", 3) == 0) && (navData->gll)) {
    populate_gll(nmea_str, navData->gll);
    navData->cycle++;
  } else {
    return 1;
  }
  return 0;
}

#ifdef NMEA_PRINT

void print_rmc(const navData_t *data) {
  if (data->rmc) {
    printf("RMC\n");
    printf("Time: %f\n", data->rmc->time);
    printf("Status: %c\n", data->rmc->status);
    printf("Latitude: %f\n", data->rmc->lat);
    printf("Latitude Direction: %c\n", data->rmc->lat_dir);
    printf("Longitude: %f\n", data->rmc->lon);
    printf("Longitude Direction: %c\n", data->rmc->lon_dir);
    printf("Speed: %f\n", data->rmc->speed);
    printf("Course: %f\n", data->rmc->course);
    printf("Date: %u\n", data->rmc->date);
    printf("Magnetic Variation: %f\n", data->rmc->mg_var);
    printf("Magnetic Direction: %c\n", data->rmc->mg_dir);
    printf("Checksum Mode: %c\n", data->rmc->checksum_mode);
    printf("Checksum: %hhx\n", data->rmc->checksum);
  }
}

void print_gga(const navData_t *data) {
  if (data->gga) {
    printf("GGA\n");
    printf("Time: %f\n", data->gga->time);
    printf("Latitude: %f\n", data->gga->lat);
    printf("Latitude Direction: %c\n", data->gga->lat_dir);
    printf("Longitude: %f\n", data->gga->lon);
    printf("Longitude Direction: %c\n", data->gga->lon_dir);
    printf("Quality: %hhu\n", data->gga->quality);
    printf("Satellite Count: %hhu\n", data->gga->sat_count);
    printf("HDOP: %f\n", data->gga->hdop);
    printf("Altitude: %f\n", data->gga->alt);
    printf("Altitude Unit: %c\n", data->gga->unit_alt);
    printf("Geoid Separation: %f\n", data->gga->geoid_sep);
    printf("Geoid Separation Unit: %c\n", data->gga->unit_geoid_sep);
    printf("Age: %f\n", data->gga->age);
    printf("Reference Station ID: %hu\n", data->gga->rs_id);
    printf("Checksum: %hhx\n", data->gga->checksum);
  }
}

void print_vtg(const navData_t *data) {
  if (data->vtg) {
    printf("VTG\n");
    printf("Degrees: %f\n", data->vtg->degrees);
    printf("State: %c\n", data->vtg->state);
    printf("Degrees2: %f\n", data->vtg->degrees2);
    printf("Magnetic Sign: %c\n", data->vtg->magnetic_sign);
    printf("Speed Knots: %f\n", data->vtg->speed_knots);
    printf("Knots: %c\n", data->vtg->knots);
    printf("Speed KMH: %f\n", data->vtg->speed_kmh);
    printf("KMH: %c\n", data->vtg->kmh);
    printf("Checksum Mode: %c\n", data->vtg->checksum_mode);
    printf("Checksum: %hhx\n", data->vtg->checksum);
  }
}

void print_gsa(const navData_t *data) {
  if (data->gsa) {
    printf("GSA\n");
    printf("Selection Mode: %c\n", data->gsa->sel_mode);
    printf("Mode: %c\n", data->gsa->mode);
    for (int i = 0; i < 12; i++) {
      printf("Satellite ID %d: %hhu\n", i, data->gsa->sat_id[i]);
    }
    printf("PDOP: %f\n", data->gsa->pdop);
    printf("HDOP: %f\n", data->gsa->hdop);
    printf("VDOP: %f\n", data->gsa->vdop);
    printf("Checksum: %hhx\n", data->gsa->checksum);
  }
}

void print_gsv(const navData_t *data) {
  if (data->gsv) {
    printf("GSV\n");
    printf("Message Count: %hhu\n", data->gsv->mes_count);
    printf("Message Number: %hhu\n", data->gsv->mes_num);
    printf("Satellite Count: %hhu\n", data->gsv->sat_count);
    for (int i = 0; i < data->gsv->sat_count; i++) {
      xxGSV_sat_t *sat = &data->gsv->sat_info[i];
      printf("Satellite Number: %hhu\n", sat->sat_num);
      printf("Elevation: %hhu\n", sat->elevation);
      printf("Azimuth: %hu\n", sat->azimuth);
      printf("SNR: %hhu\n", sat->snr);
    }
    for (int i = 0; i < data->gsv->mes_count; i++) {
      printf("Checksum %d: %hhx\n", i + 1, data->gsv->checksum[i]);
    }
  }
}

void print_gll(const navData_t *data) {
  if (data->gll) {
    printf("GLL\n");
    printf("Latitude: %f\n", data->gll->lat);
    printf("Latitude Direction: %c\n", data->gll->lat_dir);
    printf("Longitude: %f\n", data->gll->lon);
    printf("Longitude Direction: %c\n", data->gll->lon_dir);
    printf("UTC Time: %f\n", data->gll->utc_time);
    printf("Status: %c\n", data->gll->status);
    printf("Checksum Mode: %c\n", data->gll->checksum_mode);
    printf("Checksum: %hhx\n", data->gll->checksum);
  }
}

void print_nav(const navData_t *data) {
  if (data->rmc) {
    printf("###################################\n");
    print_rmc(data);
  }
  if (data->gga) {
    printf("###################################\n");
    print_gga(data);
  }
  if (data->vtg) {
    printf("###################################\n");
    print_vtg(data);
  }
  if (data->gsa) {
    printf("###################################\n");
    print_gsa(data);
  }
  if (data->gsv) {
    printf("###################################\n");
    print_gsv(data);
  }
  if (data->gll) {
    printf("###################################\n");
    print_gll(data);
  }
  printf("###################################\n");
}

#endif
