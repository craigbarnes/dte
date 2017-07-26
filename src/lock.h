#ifndef LOCK_H
#define LOCK_H

int lock_file(const char *filename);
void unlock_file(const char *filename);

#endif
