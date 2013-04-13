#ifndef NANDROID_H
#define NANDROID_H

void nandroid_generate_timestamp_path(char* backup_path);
int nandroid_backup(const char* backup_path);
int nandroid_restore(const char* backup_path, int restore_boot, int restore_system, int restore_data, int restore_cache, int restore_sdext, int restore_wimax);

int nandroid_advanced_backup(const char* backup_path, const char *root);
#endif
