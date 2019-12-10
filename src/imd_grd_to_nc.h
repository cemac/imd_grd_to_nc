/* minimum and maximum years which will be considered valid: */
#define MIN_YEAR 1900
#define MAX_YEAR 2100

/* data types: */
#define RAIN 0
#define TEMP 1
#define MINTEMP 2
#define MAXTEMP 3
const char *data_types[] = {
  "rain",
  "temp",
  "mintemp",
  "maxtemp"
};

/* default output variable names: */
const char *nc_vars[] = {
  "rainfall",
  "temp",
  "min_temp",
  "max_temp"
};

/* default output units: */
const char *nc_units[] = {
  "mm",
  "celsius",
  "celsius",
  "celsius"
};

/* initial lat and lon values: */
const float rain_lat0 = 6.5;
const float rain_lon0 = 66.5;
const float temp_lat0 = 7.5;
const float temp_lon0 = 67.5;

/* grid sizes: */
const float rain_grid = 0.25;
const int rain_lats = 129;
const int rain_lons = 135;
const float temp_grid = 1.0;
const int temp_lats = 31;
const int temp_lons = 31;

/* fill values for netcdf: */
const float rain_fill = -999;
const float temp_fill = 99.9;

/* extension for output files: */
const char *nc_ext = ".nc";

/* int for storing whether files should be overwritten: */
int clobber_flag;

/* define struct for storing program options: */
struct _options {
  /* input file: */
  const char *infile;
  /* output file: */
  const char *outfile;
  /* variable name for netcdf output: */
  const char *ncvar;
  /* units for netcdf output: */
  const char *ncunits;
  /* data type. rainfall = 0, temperature = 1: */
  int type;
  /* input data year: */
  int year;
  /* whether help has been requested: */
  int help;
};
const struct _options DEFAULT_OPTIONS = {
  "", "", "", "", -1, -1, -1
};

/* define struct for storing input file information: */
struct _input {
  /* input file name: */
  const char *filename;
  /* file size: */
  int size;
  /* data type. rainfall = 0, temperature = 1: */
  int type;
  /* number of days in year: */
  int days;
  /* input data year: */
  int year;
  /* indicates year has been matched from file name: */
  int year_match;
};
const struct _input DEFAULT_INPUT = {
  "", -1, -1, -1, -1, -1
};

/* define struct for storing output file information: */
struct _output {
  /* output file name: */
  char *filename;
  /* variable name for netcdf output: */
  const char *ncvar;
  /* units for netcdf output: */
  const char *ncunits;
};
const struct _output DEFAULT_OUTPUT = {
  "", "", ""
};

/* define structs for storing data: */
struct _data {
  /* grid size: */
  float grid;
  /* number of lats: */
  int nlats;
  /* number of lons: */
  int nlons;
  /* initial lat value: */
  float lat0;
  /* initial lon value: */
  float lon0;
  /* year: */
  int year;
  /* number of days: */
  int ndays;
  /* day values: */
  float *days;
  /* lat values: */
  float *lats;
  /* lon values: */
  float *lons;
  /* size of a single data value in bytes: */
  int datasize;
  /* data: */
  float *data;
  /* fill value: */
  float fill;
};

/* netcdf creation flags: */
#define NC_CREATE_FLAGS NC_CLOBBER|NC_NETCDF4
/* netcdf variable names, etc.: */
#define NC_TIME_VAR "time"
#define NC_LAT_VAR "latitude"
#define NC_LON_VAR "longitude"
#define NC_CAL "calendar"
#define NC_CAL_TYPE "standard"
#define NC_UNITS "units"
#define NC_TIME_UNITS "days since XXXX-1-1 0:0:0"
#define NC_LAT_UNITS "degrees_north"
#define NC_LON_UNITS "degrees_east"
#define NC_FILLV "_FillValue"
/* compression level: */
#define NC_COMP 3
