<div align="center">
<a href="https://www.cemac.leeds.ac.uk/">
  <img src="https://github.com/cemac/cemac_generic/blob/master/Images/cemac.png"></a>
  <br>
</div>

## imd_grd_to_nc

[![GitHub top language](https://img.shields.io/github/languages/top/cemac/imd_grd_to_nc.svg)](https://github.com/cemac/imd_grd_to_nc) [![GitHub issues](https://img.shields.io/github/issues/cemac/imd_grd_to_nc.svg)](https://github.com/cemac/imd_grd_to_nc/issues) [![GitHub last commit](https://img.shields.io/github/last-commit/cemac/imd_grd_to_nc.svg)](https://github.com/cemac/imd_grd_to_nc/commits/master)  ![GitHub](https://img.shields.io/github/license/cemac/imd_grd_to_nc.svg)
[![HitCount](http://hits.dwyl.com/{cemac}/{imd_grd_to_nc}.svg)](http://hits.dwyl.com/{cemac}/{imd_grd_to_nc})

Convert IMD GRD data files to NetCDF

The India Meteorological Department provides daily gridded rainfall and temperature data sets, which are available for download here:

[http://www.imdpune.gov.in/Clim_Pred_LRF_New/Grided_Data_Download.html](http://www.imdpune.gov.in/Clim_Pred_LRF_New/Grided_Data_Download.html)

These programs convertthe data in to NetCDF format.

## Program versions

There are two versions of the program included here. `imd_grd_to_nc.py`, which can be found in the [`bin/`](bin/) directory, is implemented in Python, and has the following third party requirements:

  * [netcdf4](https://pypi.org/project/netCDF4/)
  * [numpy](https://pypi.org/project/numpy/)

`imd_grd_to_nc` is implemented in C. The source code can be found in the [`src`](`src/`) directory, which also includes a `Makefile` for the program. Compilation requires the `gcc` C compiler and the NetCDF C library. If all requirements are available, the code can be compiled by running `make` from within the `src` directory. The [`bin/`](bin/) directory also includes a statically compiled version of the `imd_grd_to_nc` program, which should hopefully work on most current Linux systems:

  * [`imd_grd_to_nc`](bin/imd_grd_to_nc?raw=1)

Both programs use NetCDF version 4 features (compression), so a NetCDF library with NetCDF version 4 features enabled is required.

## Usage

The name of an input file is required, and can be specified with the `-i` option, for example:

```
imd_grd_to_nc -i _Clim_Pred_LRF_New_GridDataDownload_Rainfall_ind2018_rfp25.grd
```

If no other options are specified, information regarding the type of data (rainfall, minimum temperature or maximum temperature), the year of the data and the output file name will be determined from the file name, if possible.

### Options

The full list of program options, which can be viewed by calling the program with the `-h` option:

```
imd_grd_to_nc -h
```

or:

```
imd_grd_to_nc.py -h
```

All program options:

```
  -i INFILE, --infile INFILE
                        The input GRD file to read
  -o OUTFILE, --outfile OUTFILE
                        The output NetCDF file to create. If not specified,
                        the input file name will be used to determine a name
                        for the output file
  -c, --clobber         Overwrite an existing output file
  -t {maxtemp,mintemp,rain}, --type {maxtemp,mintemp,rain}
                        Data type of the input file. If not specified, the
                        input data type will be will be determined from the
                        file size and name (if possible) The data size will be
                        used to verify the type
  -y YEAR, --year YEAR  Year of the input data. If not specified, the input
                        data year will be will be determined from the file
                        name (if possible) The data size will be used to
                        verify the year
  -v NCVAR, --ncvar NCVAR
                        The variable name for the data in the NetCDF output
                        file Default values are 'rainfall', 'min_temp' and
                        'max_temp'
  -u NCUNITS, --ncunits NCUNITS
                        The units for the data in the NetCDF output file.
                        Default values are 'mm' and 'celsius'
```
