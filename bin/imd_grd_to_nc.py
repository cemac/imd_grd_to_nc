#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
im_grd_to_nc.py

Convert IMD GRD files to NetCDF

File specifications, etc. can be found here:

  http://www.imdpune.gov.in/Clim_Pred_LRF_New/Grided_Data_Download.html

Rainfall data:

  * daily gridded rainfall
  * 0.25 x 0.25 degree
  * 129 (lats) x 135 (lons) cells
  * lat values : 6.5N -> 38.5N
  * lon values : 66.5E -> 100.0E
  * units : mm

Temperature data:

  * daily gridded temperature
  * 1 x 1 degree
  * 31 x 31 cells
  * lat values : 7.5N -> 37.5N
  * lon values : 67.5E -> 97.5E
  * units : celsius

Each input file contains 365 or 366 time steps, depending on leap years
"""

# stdlib imports:
from __future__ import division
import argparse
import os
import re
import struct
import sys
# third party imports:
import netCDF4 as nc
import numpy as np

# global variables ...
# minimum and maximum years which will be considered valid:
MIN_YEAR = 1900
MAX_YEAR = 2100

# dict defining data types and properties:
DATA_TYPES = {
    'rain': {
        'lats': np.arange(6.5, 38.75, .25),
        'lons': np.arange(66.5, 100.25, .25),
        'nc_var': 'rainfall',
        'nc_units': 'mm',
        'fill': -999
    },
    'mintemp': {
        'lats': np.arange(7.5, 38.5),
        'lons': np.arange(67.5, 98.5),
        'nc_var': 'min_temp',
        'nc_units': 'celsius',
        'fill': 99.9
    },
    'maxtemp': {
        'lats': np.arange(7.5, 38.5),
        'lons': np.arange(67.5, 98.5),
        'nc_var': 'max_temp',
        'nc_units': 'celsius',
        'fill': 99.9
    }
}

# dict defining netcdf properties:
NC_CONFIG = {
    'file_ext': '.nc',
    'time_var': 'time',
    'lat_var': 'latitude',
    'lat_units': 'degrees_north',
    'lon_var': 'longitude',
    'lon_units': 'degrees_east',
    'calendar': 'standard',
    'comp_level': 3
}

def get_options():
    """
    get program options / arguments using argparse
    """
    # create argument parser:
    arg_parser = argparse.ArgumentParser()
    # input data file:
    help_msg = 'The input GRD file to read'
    arg_parser.add_argument('-i', '--infile', help=help_msg, default=None,
                            required=True)
    # output data file:
    help_msg = """The output NetCDF file to create.
                  If not specified, the input file name will be used to
                  determine a name for the output file"""
    arg_parser.add_argument('-o', '--outfile', help=help_msg, default=None,
                            required=False)
    # overwrite existing files?:
    help_msg = 'Overwrite an existing output file'
    arg_parser.add_argument('-c', '--clobber', help=help_msg,
                            action='store_true')
    # data type:
    help_msg = """Data type of the input file.
                  If not specified, the input data type will be will be
                  determined from the file size and name (if possible)
                  The data size will be used to verify the type"""
    arg_parser.add_argument('-t', '--type', help=help_msg, default=None,
                            required=False, choices=DATA_TYPES.keys())
    # data year:
    help_msg = """Year of the input data.
                  If not specified, the input data year will be will be
                  determined from the file name (if possible)
                  The data size will be used to verify the year"""
    arg_parser.add_argument('-y', '--year', help=help_msg, default=None,
                            required=False, type=int)
    # netcdf variable name:
    help_msg = """The variable name for the data in the NetCDF output file\n
                  Default values are 'rainfall', 'min_temp' and 'max_temp'"""
    arg_parser.add_argument('-v', '--ncvar', help=help_msg, default=None,
                            required=False)
    # netcdf units:
    help_msg = """The units for the data in the NetCDF output file.
                  Default values are 'mm' and 'celsius'"""
    arg_parser.add_argument('-u', '--ncunits', help=help_msg, default=None,
                            required=False)
    # parse arguments:
    prog_options = arg_parser.parse_args()
    # return program arguments:
    return prog_options

def get_input(prog_options):
    """
    get input information using options and file properties
    """
    # dict for storing input information:
    prog_input = {}
    # program options:
    in_file = prog_options.infile
    # Check input file exists:
    if not os.path.exists(in_file):
        err_msg = 'Input file {0} does not exist\n'
        err_msg = err_msg.format(in_file)
        sys.stderr.write(err_msg)
        sys.exit(1)
    else:
        # store the input file name:
        prog_input['filename'] = os.path.abspath(in_file)
    # get the input file size and store the value:
    input_size = os.path.getsize(in_file)
    prog_input['size'] = input_size
    # from the file size, we can work out the type of data, and the number of
    # days in the year.
    # e.g., for rainfall data, for a 365 day year:
    #   file size = 129 * 135 * 4 * 365 + 1 = 25425901
    if input_size == 25425901:
        # rainfall, 365 days:
        prog_input['type'] = 'rain'
        prog_input['days'] = 365
    elif input_size == 25495561:
        # rainfall, 366 days:
        prog_input['type'] = 'rain'
        prog_input['days'] = 366
    elif input_size == 1403061:
        # temperature, 365 days:
        prog_input['type'] = 'temp'
        prog_input['days'] = 365
    elif input_size == 1406905:
        # temperature, 366 days:
        prog_input['type'] = 'temp'
        prog_input['days'] = 366
    else:
        # invalid file size ... give up:
        err_msg = 'Invalid input file size\n'
        sys.stderr.write(err_msg)
        sys.exit(1)
    # if this is temperature data, try to guess whether min or max data
    # from the file name:
    if re.match(r'.*max.*', os.path.basename(in_file), flags=re.IGNORECASE):
        prog_input['type'] = 'maxtemp'
    if re.match(r'.*min.*', os.path.basename(in_file), flags=re.IGNORECASE):
        prog_input['type'] = 'mintemp'
    # try to guess the year from the file name:
    input_year = re.search(r'[0-9]{4}', in_file)
    if input_year:
        input_year = int(input_year.group(0))
        prog_input['year'] = input_year
        prog_input['year_match'] = True
    else:
        # set to -1:
        prog_input['year'] = -1
        prog_input['year_match'] = False
    # return the input information:
    return prog_input

def check_input(prog_options, prog_input_in):
    """
    check program options and input file information, to make sure everything
    makes sense
    """
    # dict for storing input information:
    prog_input_out = {}
    # if input type was specified as a program option, use that:
    if prog_options.type:
        prog_input_out['type'] = prog_options.type
    else:
        # use value determined from input file:
        prog_input_out['type'] = prog_input_in['type']
    # if type is 1, we know it is temperature data, but not whether it is
    # min or max temperature:
    if prog_input_out['type'] == 'temp':
        err_msg = 'Temperature data detected, but can not detect whether'
        err_msg += ' it is min or max data\n'
        err_msg += 'Try specifying data type with the -t option\n'
        sys.stderr.write(err_msg)
        sys.exit(1)
    # check if the user specified data type matches the detected type:
    if (((prog_input_out['type'] == 'rain') and
         (prog_input_in['type'] != 'rain')) or
            ((prog_input_out['type'] == 'min_temp') and
             (prog_input_in['type'] == 'rain')) or
            ((prog_input_out['type'] == 'max_temp') and
             (prog_input_in['type'] == 'rain'))):
        # does not match. print error and exit:
        err_msg = 'Specified data type: {0} does not match detected data'
        err_msg += ' type: {1}\n'
        err_msg = err_msg.format(prog_input_out['type'],
                                 prog_input_in['type'])
        sys.stderr.write(err_msg)
        sys.exit(1)
    # if input year was specified as a program option, use that:
    if prog_options.year:
        prog_input_out['year'] = prog_options.year
        prog_input_out['year_match'] = False
    else:
        # use value determined from input file:
        prog_input_out['year'] = prog_input_in['year']
        prog_input_out['year_match'] = prog_input_in['year_match']
    # if year is -1, no year detected or specified:
    if prog_input_out['year'] == -1:
        err_msg = 'Please specify a year for the input data (-y)\n'
        sys.stderr.write(err_msg)
        sys.exit(1)
    # check year is within valid range:
    if (MIN_YEAR > prog_input_out['year'] or
            MAX_YEAR < prog_input_out['year']):
        err_msg = 'Invalid year specified: {0}\n'
        err_msg = err_msg.format(prog_input_out['year'])
        sys.stderr.write(err_msg)
        sys.exit(1)
    # if the data file contains data for 366 days, then the year should be
    #  divisble by 4, i.e. a leap year:
    if (prog_input_in['days'] == 366 and
            prog_input_out['year'] % 4 != 0):
        # print error and exit:
        err_msg = 'Data file {0} contains data for 366 days\n'
        err_msg += 'Year {1} does not appear to be a leap year\n'
        err_msg = err_msg.format(prog_input_in['filename'],
                                 prog_input_out['year'])
        # additional information if year was matched from file name:
        if prog_input_out['year_match']:
            err_msg += 'Try specifying a year with the -y option\n'
        sys.stderr.write(err_msg)
        sys.exit(1)
    # if the data file contains data for 356 days, then the year should not be
    #  divisble by 4, i.e. not a leap year:
    if (prog_input_in['days'] == 365 and
            prog_input_out['year'] % 4 == 0):
        # print error and exit:
        err_msg = 'Data file {0} contains data for 365 days\n'
        err_msg += 'Year {1} appears to be a leap year\n'
        err_msg = err_msg.format(prog_input_in['filename'],
                                 prog_input_out['year'])
        # additional information if year was matched from file name:
        if prog_input_out['year_match']:
            err_msg += 'Try specifying a year with the -y option\n'
        sys.stderr.write(err_msg)
        sys.exit(1)
    # at this point, these options should be o.k. ... :
    prog_input_out['filename'] = prog_input_in['filename']
    prog_input_out['size'] = prog_input_in['size']
    prog_input_out['days'] = prog_input_in['days']
    # return the checked input information:
    return prog_input_out

def check_output(prog_options, prog_input):
    """
    check program options and output file information, to make sure everything
    makes sense
    """
    # dict for storing output information:
    prog_output = {}
    # if output file name is specified, use specifed output file name:
    if prog_options.outfile:
        prog_output['filename'] = prog_options.outfile
    else:
        # check if input file name has an extension:
        ext_match = re.match(r'.*\.[^\.]+$', prog_input['filename'])
        if ext_match:
            prog_output['filename'] = re.sub(r'\.[^\.]+$',
                                             NC_CONFIG['file_ext'],
                                             prog_input['filename'])
    # if output file is still not set:
    if 'filename' not in prog_output:
        # append '.nc' to input file name:
        prog_output['filename'] = ''.join([prog_input['filename'],
                                           NC_CONFIG['file_ext']])
    # check if the output file exists and if clobber flag is set:
    if os.path.exists(prog_output['filename']) and not prog_options.clobber:
        # display error and exit:
        err_msg = 'Output file: {0} exists. Use -c option to overwrite\n'
        err_msg = err_msg.format(prog_output['filename'])
        sys.stderr.write(err_msg)
        sys.exit(1)
    # if netcdf variable name is specified:
    if prog_options.ncvar:
        prog_output['nc_var'] = prog_options.ncvar
    else:
        # use default value:
        prog_output['nc_var'] = DATA_TYPES[prog_input['type']]['nc_var']
    # if netcdf units are specified:
    if prog_options.ncunits:
        prog_output['nc_units'] = prog_options.ncunits
    else:
        # use default value:
        prog_output['nc_units'] = DATA_TYPES[prog_input['type']]['nc_units']
    # return the output information:
    return prog_output

def read_data(prog_input):
    """
    read in data from grd file, and return values
    """
    # dict for storing data:
    prog_data = {}
    # data type:
    data_type = prog_input['type']
    # get day count for data:
    data_days = prog_input['days']
    # get lat and lon values and counts:
    data_lats = DATA_TYPES[data_type]['lats']
    data_lons = DATA_TYPES[data_type]['lons']
    lat_count = data_lats.shape[0]
    lon_count = data_lons.shape[0]
    # create numpy array of nans for storing data:
    data_data = np.zeros([data_days, lat_count, lon_count]) * np.nan
    # get the shape of the grid from first time step:
    data_shape = data_data[0].shape
    # format for unpacking data:
    data_fmt = ''.join(['<', 'f' * lat_count * lon_count])
    # open the file:
    data_file = open(prog_input['filename'], 'rb')
    # for each day:
    for i in range(data_days):
        # read gridded data:
        day_bytes = data_file.read(lat_count * lon_count * 4)
        # unpack the data:
        day_data = struct.unpack(data_fmt, day_bytes)
        # convert to numpy array and reshape:
        day_data = np.asarray(day_data).reshape(data_shape)
        # add to array:
        data_data[i] = day_data
    # close data file:
    data_file.close()
    # store required values:
    prog_data['lats'] = data_lats
    prog_data['lons'] = data_lons
    prog_data['nlats'] = lat_count
    prog_data['nlons'] = lon_count
    prog_data['type'] = prog_input['type']
    prog_data['year'] = prog_input['year']
    prog_data['days'] = prog_input['days']
    prog_data['fill'] = DATA_TYPES[data_type]['fill']
    prog_data['data'] = data_data
    # return the data:
    return prog_data

def write_data(prog_data, prog_output):
    """
    write data to netcdf file:
    """
    # create the netcdf output file:
    nc_data = nc.Dataset(prog_output['filename'], 'w', format='NETCDF4')
    # add time dimension:
    nc_time_var = NC_CONFIG['time_var']
    nc_data.createDimension(nc_time_var)
    nc_times = nc_data.createVariable(nc_time_var, 'f', (nc_time_var))
    # set the time units and calendar:
    nc_year = prog_data['year']
    nc_cal = NC_CONFIG['calendar']
    nc_times.units = 'days since {0}-01-01 00:00:00.0'.format(nc_year)
    nc_times.calendar = nc_cal
    # add the time values:
    nc_days = prog_data['days']
    nc_times[:] = [i for i in range(nc_days)]
    # add latitude:
    nc_lat_var = NC_CONFIG['lat_var']
    nc_data_lats = prog_data['lats']
    nc_data.createDimension(nc_lat_var, nc_data_lats.shape[0])
    nc_lats = nc_data.createVariable(nc_lat_var, 'f', (nc_lat_var))
    nc_lats[:] = nc_data_lats
    # add latitude units:
    nc_lats.units = NC_CONFIG['lat_units']
    # add longitude:
    nc_lon_var = NC_CONFIG['lon_var']
    nc_data_lons = prog_data['lons']
    nc_data.createDimension(nc_lon_var, nc_data_lons.shape[0])
    nc_lons = nc_data.createVariable(nc_lon_var, 'f', (nc_lon_var))
    nc_lons[:] = nc_data_lons
    # add longitude units:
    nc_lons.units = NC_CONFIG['lon_units']
    # add data variable:
    nc_data_type = prog_data['type']
    nc_data_var = DATA_TYPES[nc_data_type]['nc_var']
    nc_comp_level = NC_CONFIG['comp_level']
    nc_data_fill = DATA_TYPES[nc_data_type]['fill']
    nc_var = nc_data.createVariable(nc_data_var, 'f',
                                    (nc_time_var, nc_lat_var, nc_lon_var),
                                    zlib=True, complevel=nc_comp_level,
                                    fill_value=nc_data_fill)
    # add the variable data:
    nc_var[:] = prog_data['data']
    # set the units:
    nc_data_units = DATA_TYPES[nc_data_type]['nc_units']
    nc_var.units = nc_data_units
    # close the netcdf file:
    nc_data.close()

def main():
    """
    main function calls other functions to get options, check input data,
    check output information, read and write the data
    """
    # get the program options:
    prog_options = get_options()
    # get input file information:
    prog_input = get_input(prog_options)
    # check program options and input information:
    prog_input = check_input(prog_options, prog_input)
    # check output information and options:
    prog_output = check_output(prog_options, prog_input)
    # read the data:
    prog_data = read_data(prog_input)
    # write the data to netcdf:
    write_data(prog_data, prog_output)

if __name__ == '__main__':
    main()
