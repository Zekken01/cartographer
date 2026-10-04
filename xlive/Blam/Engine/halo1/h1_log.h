#pragma once

/* Logging for the Halo 1 cache file support, written to h1_maps.log next to halo2.exe */

void h1_log(const char* format, ...);

void h1_log_flush(void);
