#ifndef PATH_H
#define PATH_H

char *path_absolute(const char *filename);
char *relative_filename(const char *f, const char *cwd);
char *short_filename_cwd(const char *absolute, const char *cwd);
char *short_filename(const char *absolute);
char *path_dirname(const char *filename);
const char *path_basename(const char *filename);

#endif
