#ifndef COMPLETION_H
#define COMPLETION_H

void complete_command_next(void);
void complete_command_prev(void);
void reset_completion(void);
void add_completion(char *str);

#endif
