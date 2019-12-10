#include <getopt.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include <imd_grd_to_nc.h>

/* program name: */
const char *program_name;

/*
 * return the name of a file from a path, e.g. the path:
 *   /path/to/file_00
 * will return:
 *   file_00
 */
const char *basename(char *path) {
  /* path separator character: */
  const char sep = '/';
  /* use strrchr to find the last occurrence of the separator in the path: */
  const char *bn = strrchr(path, sep);
  /* if anything was returned, need to add 1 to get the filename, to remove
     the leading slash, otherwise return the original path */
  bn = bn ? bn + 1 : path;
  /* return the basename: */
  return bn;
}

/*
 * print program usage information
 */
int usage(int full) {
  /* short usage: */
  printf("Usage: %s "
         "-i input-file "
         "[-o output-file] "
         "[-c] "
         "[-t data-type] "
         "[-t data-year] "
         "[-v netcdf-varname] "
         "[-u netcdf-units]\n",
         program_name);
  /* if help has been asked for, display long help: */
  if (full == 1) {
    printf("\n"
           "Convert IMD GRD files to NetCDF\n"
           "\n"
           "  -h --help         Display this help message and exit\n"
           "  -o --infile       The input GRD file to read\n"
           "  -o --outfile      The output NetCDF file to create\n"
           "                    If not specified, the input file name will be used to\n"
           "                    determine a name for the output file\n"
           "  -c --clobber      Overwrite an existing output file\n"
           "  -t --type         Data type of the input file\n"
           "                    Valid options are 'rain', 'mintemp' and 'maxtemp'\n"
           "                    If not specified, the input data type will be will be\n"
           "                    determined from the file size and name (if possible)\n"
           "                    The data size will be used to verify the type\n"
           "  -y --year         Year of the input data\n"
           "                    If not specified, the input data year will be will be\n"
           "                    determined from the file name (if possible)\n"
           "                    The data size will be used to verify the year\n"
           "  -v --ncvar        The variable name for the data in the NetCDF output file\n"
           "                    Default values are 'rainfall', 'min_temp' and 'max_temp'\n"
           "  -u --ncunits      The units for the data in the NetCDF output file\n"
           "                    Default values are 'mm' and 'celsius'\n");
  }
  exit(1);
}

/*
 * get the program options and return an _options struct containing the options
 */
struct _options get_options(int argc, char **argv) {
  /* create the struct for storing program options: */
  struct _options options = DEFAULT_OPTIONS;
  /* define the possible getopt options: */
  static struct option long_options[] = {
    {"infile", required_argument, 0, 'i'},
    {"outfile", required_argument, 0, 'o'},
    {"clobber", no_argument, 0, 'c'},
    {"ncvar", required_argument, 0, 'v'},
    {"ncunits", required_argument, 0, 'u'},
    {"type", required_argument, 0, 't'},
    {"year", required_argument, 0, 'y'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
  };
  /* opt will be returned by getopt_long: */
  int opt;
  /* for storing char representations of the option: */
  char opt_char, arg_char;
  /* don't print getopt error messages: */
  opterr = 0;
  /* getopt_long() is not -1, i.e. parse all program options: */
  while((opt = getopt_long(argc, argv, "i:o:cv:u:t:y:h", long_options,
                           NULL)) != -1) {
    /* switch for argument checking: */
    switch (opt) {
      /* input file name: */
      case 'i':
        options.infile = optarg;
        break;
      /* output file name: */
      case 'o':
        options.outfile = optarg;
        break;
      /* enable output overwriting / clobbering: */
      case 'c':
        clobber_flag = 1;
        break;
      /* variable name for netcdf output: */
      case 'v':
        options.ncvar = optarg;
        break;
      /* units for netcdf output: */
      case 'u':
        options.ncunits = optarg;
        break;
      /* data year: */
      case 'y':
        /* convert to integer: */
        options.year = atoi(optarg);
        /* check year is within valid range: */
        if ((options.year < MIN_YEAR) ||
            (options.year > MAX_YEAR)) {
          fprintf(stderr, "Invalid year specified: %s\n", optarg);
          exit(1);
        }
        break;
      /* data type: */
      case 't':
        /* acceptable types are 'rain' or 'temp': */
        if (strcmp(optarg, "rain") == 0) {
          options.type = RAIN;
        } else if (strcmp(optarg, "mintemp") == 0) {
          options.type = MINTEMP;
        } else if (strcmp(optarg, "maxtemp") == 0) {
          options.type = MAXTEMP;
        } else {
          fprintf(stderr, "Invalid data type specified: %s\n", optarg);
          fprintf(stderr, "Valid data types: rain, mintemp, maxtemp\n");
          exit(1);
        }
        break;
      /* request help: */
      case 'h':
        options.help = 1;
        break;
      /* invalid option, display error and usage: */
      case '?':
        /* get a char representation of the option: */
        opt_char = (char) optopt;
        /* first char representation of last argv: */
        arg_char = argv[optind - 1][1];
        /* compare to work out which argv is at fault: */
        if (opt_char == arg_char) {
          /* check for argument missing an option: */
          if (strchr("iovuty", opt_char) != NULL) {
            fprintf(stderr, "Option -%c requires an argument\n", opt_char);
            usage(0);
            break;
          } else {
            optind -= 1;
          }
        }
        /* double dashing arguments: */
        if (arg_char == '-') {
            optind -= 1;
        }
        /* error message: */
        fprintf(stderr, "Invalid option specified: %s\n", argv[optind]);
        usage(0);
        break;
      /* default case, just break: */
      default:
        break;
    }
  /* end while parsing arguments: */
  }
  /* return the program options: */
  return options;
}

/*
 * file_exists does a simple check to see if a file exists.
 * if successful, returns file size, otherwise, returns -1.
 */
int file_exists(const char *filename) {
  /* file handle: */
  FILE *file;
  /* file size: */
  int size;
  /* check input file exists: */
  if ((file = fopen(filename, "r")) == NULL) {
    return -1;
  } else {
    /* file exists ... get file size: */
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    /* close the file: */
    fclose(file);
  }
  /* return the file size: */
  return size;
}

/* get input information using options and file properties: */
struct _input get_input(struct _options *options) {
  /* create the struct for storing input information: */
  struct _input input = DEFAULT_INPUT;
  /* regular expression bits for matching year: */
  regex_t regex;
  const char *pattern;
  regmatch_t match;
  /* init year string: */
  char *yr_str;
  /* check input is specified: */
  if (strcmp(options->infile, "") == 0) {
    /* give up: */
    fprintf(stderr, "No input file specified (-i)\n");
    exit(1);
  }
  /* check input file exists and get file size: */
  if ((input.size = file_exists(options->infile)) == -1) {
    /* give up: */
    fprintf(stderr, "input file does not exist: %s\n", options->infile);
    exit(1);
  } else {
    /* store filename: */
    input.filename = options->infile;
  }
  /*
   * from the file size, we can work out the type of data, and the number of
   * days in the year.
   *
   * e.g., for rainfall data, for a 365 day year:
   *   file size = 129 * 135 * 4 * 365 + 1 = 25425901
   */
  if (input.size == 25425901) {
    /* rainfall, 365 days: */
    input.type = RAIN;
    input.days = 365;
  } else if (input.size == 25495561) {
    /* rainfall, 366 days: */
    input.type = RAIN;
    input.days = 366;
  } else if (input.size == 1403061) {
    /* temperature, 365 days: */
    input.type = TEMP;
    input.days = 365;
  } else if (input.size == 1406905) {
    /* temperature, 366 days: */
    input.type = TEMP;
    input.days = 366;
  } else {
    /* invalid file size ... give up: */
    fprintf(stderr, "Invalid input file size\n");
    exit(1);
  }
  /*
   * if this is temperature data, try to guess whether min or max data
   * from the file name:
   */
  if (input.type == TEMP) {
    /* regular expression to match for min: */
    pattern = "min";
    /* try to compile the regex: */
    if (regcomp(&regex, pattern, REG_EXTENDED|REG_ICASE) == 0) {
      /* see if anything matches: */
      if (regexec(&regex, input.filename, 1, &match, 0) == 0) {
        /* set the data type: */
        input.type = MINTEMP;
      }
    }
    /* free the regex: */
    regfree(&regex);
    /* regular expression to match for max: */
    pattern = "max";
    /* try to compile the regex: */
    if (regcomp(&regex, pattern, REG_EXTENDED|REG_ICASE) == 0) {
      /* see if anything matches: */
      if (regexec(&regex, input.filename, 1, &match, 0) == 0) {
        /* set the data type: */
        input.type = MAXTEMP;
      }
    }
    /* free the regex: */
    regfree(&regex);
  }
  /* try to guess the year from the file name: */
  input.year = -1;
  /* regular expression to match: */
  pattern = "[0-9]{4}";
  /* try to compile the regex: */
  if (regcomp(&regex, pattern, REG_EXTENDED) == 0) {
    /* see if anything matches: */
    if (regexec(&regex, input.filename, 1, &match, 0) == 0) {
      /* allocate the yr_str: */
      yr_str = calloc(4 + 1, sizeof(char));
      /* get the match: */
      memcpy(yr_str, input.filename + match.rm_so, 4);
      /* convert to int and store: */
      input.year = atoi(yr_str);
      /* free the yr_str: */
      free(yr_str);
    }
  }
  /* free the regex: */
  regfree(&regex);
  /* return the input information: */
  return input;
}

/*
 * check program options and input file information, to make sure everything
 * makes sense:
 */
struct _input check_input(struct _options *options,
                          struct _input *input_in) {
  /* create the struct for storing input information: */
  struct _input input_out = DEFAULT_INPUT;
  /*
   * Certain command line option might disagree with the information obtained
   * by inspecting the input file, it's worth double checking some things.
   * If a data type is specified in the options, use that:
   */
  if (options->type != -1) {
    input_out.type = options->type;
  } else {
    /* otherwise, use value from file inspection: */
    input_out.type = input_in->type;
  }
  /*
   * if type is 1, we know it is temperature data, but not whether it is
   * min or max temperature:
   */
  if (input_out.type == 1) {
    fprintf(stderr, "Temperature data detected, but can not detect whether"
                    " it is min or max data\n");
    fprintf(stderr, "Try specifying data type with the -t option\n");
    exit(1);
  }
  /* check if the user specifed type matches the detected data type: */
  if ((input_out.type == 0 && input_in->type != 0) ||
      (input_out.type == 2 && input_in->type < 1) ||
      (input_out.type == 3 && input_in->type < 1)) {
    fprintf(stderr, "Specified data type: %s does not match detected data"
                    " type: %s\n", data_types[input_out.type],
                    data_types[input_in->type]);
    exit(1);
  }
  /* If a year is specified in the options, use that: */
  if (options->year != -1) {
    input_out.year = options->year;
    input_out.year_match = 0;
  } else {
    /* otherwise, use value from file inspection: */
    input_out.year = input_in->year;
    input_out.year_match = 1;
  }
  /* if we don't have a valid year, give up: */
  if (input_out.year == -1) {
    fprintf(stderr, "Please specify a year for the input data\n");
    exit(1);
  }
  /*
   * If the data file contains data for 366 days, then the year should be
   *  divisble by 4, i.e. a leap year:
   */
  if (input_in->days == 366) {
    if (input_out.year % 4 != 0) {
      fprintf(stderr, "Data file %s contains data for 366 days\n",
              input_in->filename);
      fprintf(stderr, "Year %d does not appear to be a leap year\n",
              input_out.year);
      /* additional information if year was matched from file name: */
      if (input_out.year_match == 1) {
        fprintf(stderr, "Try specifying a year with the -y option\n");
      }
      exit(1);
    }
  }
  /*
   * If the data file contains data for 365 days, then the year should not be
   *  divisible by 4, i.e. not a leap year:
   */
  if (input_in->days == 365) {
    if (input_out.year % 4 == 0) {
      fprintf(stderr, "Data file %s contains data for 365 days\n",
              input_in->filename);
      fprintf(stderr, "Year %d appears to be a leap year\n",
              input_out.year);
      /* additional information if year was matched from file name: */
      if (input_out.year_match == 1) {
        fprintf(stderr, "Try specifying a year with the -y option\n");
      }
      exit(1);
    }
  }
  /*
   * At this point, these options should be o.k. ... :
   */
  input_out.filename = input_in->filename;
  input_out.size = input_in->size;
  input_out.days = input_in->days;
  /* return the checked options: */
  return input_out;
}

/*
 * check program options and output file information, to make sure everything
 * makes sense:
 */
struct _output check_output(struct _options *options, struct _input *input) {
  /* create the struct for storing output information: */
  struct _output output = DEFAULT_OUTPUT;
  /* regular expression bits for matching file extension: */
  regex_t regex;
  const char *pattern;
  regmatch_t match;
  /* input file name length: */
  int infile_len = strlen(options->infile);
  /* output file name length: */
  int outfile_len = strlen(options->outfile);
  /* used for output file name allocation: */
  char *out_file;
  int out_file_set = 0;
  /* if ouput file name is specified ... : */
  if (strcmp(options->outfile, "") != 0) {
    /* use provided output file name: */
    out_file = calloc(outfile_len + 1,
                      sizeof(char));
    out_file[0] = '\0';
    strncat(out_file, options->outfile, sizeof(options->outfile) *
            sizeof(char));
    /* output file name is set: */
    out_file_set = 1;
  } else {
    /*
     * check if input file name has an extension ...
     * regular expression to match:
     */
    pattern = "\\.[^\\.]+$";
    /* try to compile the regex: */
    if (regcomp(&regex, pattern, REG_EXTENDED|REG_ICASE) == 0) {
      /* see if anything matches: */
      if (regexec(&regex, options->infile, 1, &match, 0) == 0) {
        /* allocate for output file name: */
        out_file = calloc(infile_len -
                          (infile_len - match.rm_so) +
                          strlen(nc_ext) + 1,
                          sizeof(char));
        out_file[0] = '\0';
        /* get the file name without extension: */
        strncat(out_file, options->infile, match.rm_so);
        /* add the extension: */
        strncat(out_file, nc_ext, sizeof(nc_ext) * sizeof(char));
        /* output file name is set: */
        out_file_set = 1;
      }
    }
    /* free the regex: */
    regfree(&regex);
    /* if output file is still not set: */
    if (out_file_set == 0) {

      /* allocate for output file name: */
      out_file = calloc(infile_len +
                        strlen(nc_ext) + 1,
                        sizeof(char));
      out_file[0] = '\0';
      /* append '.nc' to input file name: */
      strncat(out_file, options->infile, sizeof(options->infile) * sizeof(char));
      strncat(out_file, nc_ext, sizeof(nc_ext) * sizeof(char));
      /* output file name is set: */
      out_file_set = 1;
    }
  }
  /* set output file name: */
  output.filename = out_file;
  /* check if the output file exists and if clobber flag is set: */
  if ((file_exists(output.filename) != -1) &&
      (clobber_flag != 1)) {
    /* give up: */
    fprintf(stderr, "Output file: %s exists. Use -c option to overwrite\n",
            output.filename);
    /* free the output filename: */
    free(output.filename);
    exit(1);
  }
  /* if ouput netcdf variable name is specified ... : */
  if (strcmp(options->ncvar, "") != 0) {
    /* use provided variable name: */
    output.ncvar = options->ncvar;
  } else {
    /* use default value: */
    output.ncvar = nc_vars[input->type];
  }
  /* if ouput netcdf units are specified ... : */
  if (strcmp(options->ncunits, "") != 0) {
    /* use provided units: */
    output.ncunits = options->ncunits;
  } else {
    /* use default value: */
    output.ncunits = nc_units[input->type];
  }
  /* return the output options: */
  return output;
}

/* read in data from grd file, and return struct of values: */
struct _data read_data(struct _input *input, struct _output *output) {
  /* create the struct for storing data: */
  struct _data data;
  /* input file: */
  FILE *input_file;
  /* for loop integers: */
  int i, j, k;
  /* dat index: */
  int data_index = 0;
  /* return value of fread: */
  size_t fread_size;
  /* returned data value: */
  float fread_data;
  /* set up the data struct. if rain data ... : */
  if (input-> type == 0) {
    data.grid = rain_grid;
    data.nlats = rain_lats;
    data.nlons = rain_lons;
    data.lat0 = rain_lat0;
    data.lon0 = rain_lon0;
    data.lats = calloc(rain_lats, sizeof(float));
    data.lons = calloc(rain_lons, sizeof(float));
    data.datasize = input->size / (input->days * rain_lats * rain_lons);
    data.data = calloc(input->days * rain_lats * rain_lons, sizeof(float *));
    data.fill = rain_fill;
  } else {
    data.grid = temp_grid;
    data.nlats = temp_lats;
    data.nlons = temp_lons;
    data.lat0 = temp_lat0;
    data.lon0 = temp_lon0;
    data.lats = calloc(temp_lats, sizeof(float));
    data.lons = calloc(temp_lons, sizeof(float));
    data.datasize = input->size / (input->days * temp_lats * temp_lons);
    data.data = calloc(input->days * temp_lats * temp_lons, sizeof(float *));
    data.fill = temp_fill;
  }
  data.year = input->year;
  data.ndays = input->days;
  data.days = calloc(input->days, sizeof(float));
  /* store the lat and lon values: */
  for (i = 0; i < data.nlats; i++) {
    /* store the lat value: */
    data.lats[i] = data.lat0 + (i * data.grid);
  }
  for (i = 0; i < data.nlons; i++) {
    /* store the lon value: */
    data.lons[i] = data.lon0 + (i * data.grid);
  }
  /* open the input file: */
  input_file = fopen(input->filename, "rb");
  /* loop over values: */
  for (i = 0; i < data.ndays; i++) {
    /* store the day value: */
    data.days[i] = i;
    for (j = 0; j < data.nlats; j++) {
      /* store the */
      for (k = 0; k < data.nlons; k++) {
        /* read the data ... : */
        fread_size = fread(&fread_data, data.datasize, 1, input_file);
        /* if read size does not equal count size ... : */
        if (fread_size != 1) {
          /* close the input file: */
          fclose(input_file);
          /* free some memory: */
          free(output->filename);
          free(data.lats);
          free(data.lons);
          free(data.days);
          free(data.data);
          /* exit: */
          exit(1);
        } else {
          /* store the value: */
          data.data[data_index] = fread_data;
          data_index++;
        }
      }
    }
  }
  /* close the input file: */
  fclose(input_file);
  /* return the data: */
  return data;
}

/* write data to netcdf file: */
int write_data(struct _data *data, struct _output *output) {
  /* netcdf function return values: */
  int ncerr;
  /* netcdf id: */
  int ncid;
  /* dimension ids: */
  int time_dim, lat_dim, lon_dim;
  /* variable ids: */
  int time_var, lat_var, lon_var, data_var;
  /* char representation of year: */
  char *yr_str;
  /* time units: */
  char *time_units;
  /* netcdf dimension ids: */
  int dim_ids[3];
  /* netcdf start and count arrays: */
  size_t nc_tcount[1];
  size_t nc_tstart[1];
  /* for loop integers: */
  int i;
  /* create the output file: */
  ncerr = nc_create(output->filename, NC_CREATE_FLAGS, &ncid);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error creating file: %s\n", nc_strerror(ncerr));
    return 1;
  }
  /* create the netcdf dimensions ... time: */
  ncerr = nc_def_dim(ncid, NC_TIME_VAR, NC_UNLIMITED, &time_dim);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error creating dimension: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* ... latitude ... : */
  ncerr = nc_def_dim(ncid, NC_LAT_VAR, data->nlats, &lat_dim);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error creating dimension: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* ... longitude: */
  ncerr = nc_def_dim(ncid, NC_LON_VAR, data->nlons, &lon_dim);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error creating dimension: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* create the netcdf dimension variables ... time: */
  ncerr = nc_def_var(ncid, NC_TIME_VAR, NC_FLOAT, 1, &time_dim, &time_var);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error creating variable: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* ... latitude ... : */
  ncerr = nc_def_var(ncid, NC_LAT_VAR, NC_FLOAT, 1, &lat_dim, &lat_var);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error creating variable: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* ... longitude: */
  ncerr = nc_def_var(ncid, NC_LON_VAR, NC_FLOAT, 1, &lon_dim, &lon_var);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error creating variable: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* allocate year string: */
  yr_str = calloc(4 + 1, sizeof(char));
  sprintf(yr_str, "%d", data->year);
  /* allocate the time units: */
  time_units = calloc(strlen(NC_TIME_UNITS) + 1, sizeof(char));
  /* replace the year: */
  time_units[0] = '\0';
  strncat(time_units, NC_TIME_UNITS, 11 * sizeof(char));
  strncat(time_units, yr_str, 4 * sizeof(char));
  strncat(time_units, NC_TIME_UNITS + 15, 10 * sizeof(char));
  /* add dimension variable attributes ... time units: */
  ncerr = nc_put_att_text(ncid, time_var, NC_UNITS, strlen(time_units),
                          time_units);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting variable attributes: %s\n",
            nc_strerror(ncerr));
    free(yr_str);
    free(time_units);
    return 1;
  }
  /* free these: */
  free(yr_str);
  free(time_units);
  /* ... time calendar ... : */
  ncerr = nc_put_att_text(ncid, time_var, NC_CAL, strlen(NC_CAL_TYPE),
                          NC_CAL_TYPE);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting variable attributes: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* ... latitude ... : */
  ncerr = nc_put_att_text(ncid, lat_var, NC_UNITS, strlen(NC_LAT_UNITS),
                          NC_LAT_UNITS);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting variable attributes: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* ... longitude: */
  ncerr = nc_put_att_text(ncid, lon_var, NC_UNITS, strlen(NC_LON_UNITS),
                          NC_LON_UNITS);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting variable attributes: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* set the dimension ids: */
  dim_ids[0] = time_dim;
  dim_ids[1] = lat_dim;
  dim_ids[2] = lon_dim;
  /* create the data variable: */
  ncerr = nc_def_var(ncid, output->ncvar, NC_FLOAT, 3, dim_ids, &data_var);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error creating variable: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* enable compression: */
  ncerr = nc_def_var_deflate(ncid, data_var, 0, 1, NC_COMP);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting variable compression: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* set the data units: */
  ncerr = nc_put_att_text(ncid, data_var, NC_UNITS, strlen(output->ncunits),
                          output->ncunits);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting variable attributes: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* set the data fill value: */
  ncerr = nc_put_att_float(ncid, data_var, NC_FILLV, NC_FLOAT, 1,
                           &data->fill);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting variable attributes: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* exit define mode: */
  ncerr = nc_enddef(ncid);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error: %s\n", nc_strerror(ncerr));
    return 1;
  }
  /* add lat values: */
  ncerr = nc_put_var_float(ncid, lat_var, &data->lats[0]);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting latitude values: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* add lon values: */
  ncerr = nc_put_var_float(ncid, lon_var, &data->lons[0]);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting longitude values: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* set up count and start values: */
  nc_tcount[0] = data->ndays;
  nc_tstart[0] = 0;
  /* loop through days: */
  for (i = 0; i < data->ndays; i++) {
    /* add time value: */
    ncerr = nc_put_vara_float(ncid, time_var, nc_tstart, nc_tcount,
                              &data->days[0]);
    if (ncerr != NC_NOERR) {
      nc_close(ncid);
      fprintf(stderr, "NetCDF error setting time value: %s\n",
              nc_strerror(ncerr));
      return 1;
    }
  }
  /* add data values: */
  ncerr = nc_put_var_float(ncid, data_var, &data->data[0]);
  if (ncerr != NC_NOERR) {
    nc_close(ncid);
    fprintf(stderr, "NetCDF error setting data values: %s\n",
            nc_strerror(ncerr));
    return 1;
  }
  /* close the output file: */
  ncerr = nc_close(ncid);
  /* return: */
  return 0;
}

/* main program: */
int main(int argc, char **argv) {
  /* structs for options, input and output information, and data: */
  struct _options options;
  struct _input input;
  struct _output output;
  struct _data data;
  /* exit status: */
  int status = 0;
  /* use basename() function to get the name of the program: */
  program_name = basename(argv[0]);
  /* get program options: */
  options = get_options(argc, argv);
  /* if help was requested or no options were specified: */
  if ((options.help == 1) ||
      (argc == 1)) {
    /* print usage information: */
    usage(1);
  }
  /* get input information: */
  input = get_input(&options);
  /* check input information and options: */
  input = check_input(&options, &input);
  /* check output information and options: */
  output = check_output(&options, &input);
  /* read data: */
  data = read_data(&input, &output);
  /* write the data to netcdf: */
  status = write_data(&data, &output);
  /* memory which needs to be free: */
  free(data.lats);
  free(data.lons);
  free(data.data);
  free(data.days);
  free(output.filename);
  /* exit: */
  exit(status);
}
