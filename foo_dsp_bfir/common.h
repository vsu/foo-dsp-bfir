/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <winsock2.h>
#include <foobar2000.h>
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"

#define COMPONENT_NAME           "BruteFIR"
#define COMPONENT_VERSION           "0.1"
#define REALSIZE                     8
#define FILTER_LEN                   1024
#define EQ_FILTER_BLOCKS             64
#define PATH_MAX                     1024

#define default_cfg_cli_enable       0
#define default_cfg_cli_port         3000
#define default_cfg_overflow_enable  0

#define default_cfg_eq_enable        0
#define default_cfg_eq_level         0 
#define default_cfg_eq_mag           "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"

#define default_cfg_file1_enable     0
#define default_cfg_file2_enable     0
#define default_cfg_file3_enable     0

#define default_cfg_file1_resample   0
#define default_cfg_file2_resample   0
#define default_cfg_file3_resample   0

#define default_cfg_file1_level      0
#define default_cfg_file2_level      0
#define default_cfg_file3_level      0

#define EQ_LEVEL_STEPS_PER_DB        10
#define FILE_LEVEL_STEPS_PER_DB      10

enum
{
    EQLevelRangeMin = -20 * EQ_LEVEL_STEPS_PER_DB,
    EQLevelRangeMax = 20 * EQ_LEVEL_STEPS_PER_DB,
    FileLevelRangeMin = -20 * FILE_LEVEL_STEPS_PER_DB,
    FileLevelRangeMax = 20 * FILE_LEVEL_STEPS_PER_DB
};

extern cfg_int cfg_cli_enable;
extern cfg_int cfg_cli_port;
extern cfg_int cfg_overflow_enable;

extern cfg_int cfg_eq_enable;
extern cfg_int cfg_eq_level;
extern cfg_string cfg_eq_mag;

extern cfg_int cfg_file1_enable;
extern cfg_int cfg_file2_enable;
extern cfg_int cfg_file3_enable;

extern cfg_int cfg_file1_resample;
extern cfg_int cfg_file2_resample;
extern cfg_int cfg_file3_resample;

extern cfg_int cfg_file1_level;
extern cfg_int cfg_file2_level;
extern cfg_int cfg_file3_level;

extern cfg_string cfg_file1_filename;
extern cfg_string cfg_file2_filename;
extern cfg_string cfg_file3_filename;

extern cfg_string cfg_file1_metadata;
extern cfg_string cfg_file2_metadata;
extern cfg_string cfg_file3_metadata;

#endif